#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/syscalls.h> /* for NR_syscalls if available */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("student");
MODULE_DESCRIPTION("Show syscall table entries with symbol names");
MODULE_VERSION("0.1");

extern int kallsyms_lookup_names(const char *name,
				 unsigned int *start,
				 unsigned int *end);

#ifndef NR_syscalls
#define NR_syscalls 1024
#endif

/* TODO: implement a function that uses kallsyms_lookup_name
 * to find the address of sys_call_table.
 * Return NULL if not found.
 */
static unsigned long *get_syscall_table_addr(void)
{
    // TODO: call kallsyms_lookup_name("sys_call_table")
    // TODO: check if the result is zero
    // TODO: cast the result to (unsigned long *) and return it
    unsigned long *syscall_table = NULL;
    syscall_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");

    if(!syscall_table)
        return NULL;

    return syscall_table;
}

/* Module init function */
static int __init syscall_show_init(void)
{
    unsigned long *syscall_table;
    int i;
    char symbuf[KSYM_SYMBOL_LEN];

    pr_info("syscall_show: module init\n");

    // TODO: call get_syscall_table_addr() and assign to syscall_table
    // TODO: if syscall_table is NULL, return -ENOENT

    syscall_table = get_syscall_table_addr();
    if(!syscall_table)
        return -ENOENT;

    pr_info("syscall_show: sys_call_table at %p\n", syscall_table);

    for (i = 0; i < NR_syscalls; i++) {
        unsigned long addr = syscall_table[i];
        if (!addr) {
            pr_info("syscall[%3d] : <NULL>\n", i);
            continue;
        }

        // TODO: use sprint_symbol(symbuf, addr) to resolve symbol name
        sprint_symbol(symbuf, addr);
        pr_info("syscall[%3d] = %p -> %s\n", i, (void *)addr, symbuf);
    }

    return 0;
}

/* Module exit function */
static void __exit syscall_show_exit(void)
{
    pr_info("syscall_show: module exit\n");
}

module_init(syscall_show_init);
module_exit(syscall_show_exit);
