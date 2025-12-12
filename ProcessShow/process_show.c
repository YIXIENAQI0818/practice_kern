#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>    // 包含 task_struct 定义
#include <linux/sched/signal.h> // 包含进程遍历宏

// 将进程状态转换为可读字符串
static const char* get_task_state(long state) {
    switch (state) {
        case TASK_RUNNING:
            return "RUNNING";
        case TASK_INTERRUPTIBLE:
            return "INTERRUPTIBLE";
        case TASK_UNINTERRUPTIBLE:
            return "UNINTERRUPTIBLE";
        case TASK_STOPPED:
            return "STOPPED";
        case TASK_TRACED:
            return "TRACED";
        default:
            return "UNKNOWN";
    }
}

// 遍历进程链表并打印信息
static void print_processes(void) {
    struct task_struct *task;

    // 使用 RCU 安全的方式遍历所有进程
    rcu_read_lock();
    for_each_process(task) {
        printk(KERN_INFO "PID: %-6d | State: %-16s | Command: %s\n",
               task->pid,
               get_task_state(task->__state),
               task->comm);
        // TODO: print more info
        // pid_t ppid = task->real_parent->pid;
        // printk(KERN_INFO "    PPID: %-6d\n", ppid);

        // const struct cred *cred = rcu_dereference(task->cred);
        // uid_t euid = from_kuid(&init_user_ns, cred->euid);
        // gid_t egid = from_kgid(&init_user_ns, cred->egid);
        // printk(KERN_INFO "    EUID: %u | EGID: %u\n", euid, egid);

        // struct thread_info *ti = task_thread_info(task);
        // printk(KERN_INFO "    CPU: %d\n", ti->cpu);
        

    }
    rcu_read_unlock();
}

// 模块初始化函数
static int __init process_show_init(void) {
    printk(KERN_INFO "=== Start showing process list ===\n");
    print_processes();
    printk(KERN_INFO "=== End showing process list ===\n");
    return 0;
}

// 模块退出函数
static void __exit process_show_exit(void) {
    printk(KERN_INFO "process show module exit\n");
}

module_init(process_show_init);
module_exit(process_show_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxx");
MODULE_DESCRIPTION("kernel process show");