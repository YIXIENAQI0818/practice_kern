// SPDX-License-Identifier: GPL-2.0
/*
 * ProcMirror: create /proc/proc_mirror and, for a target PID, expose a few
 * files named fd-<N> that mirror some opened file descriptors of that process.
 *
 * Notes
 * - Focus is on how to obtain file info from task_struct->files.
 * - We resolve the task by PID at module load and snapshot a few open files.
 * - Each proc entry prints basic info (path, flags, inode) when read (cat).
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pid.h>
#include <linux/sched/signal.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/limits.h>
#include <linux/dcache.h>
#include <linux/path.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/kdev_t.h>

#define PROC_DIR_NAME "proc_mirror"
static int pid = -1;          /* target PID */
module_param(pid, int, 0644);
MODULE_PARM_DESC(pid, "Target PID to mirror");

static struct proc_dir_entry *proc_dir;
static int create_proc_entries_for_pid(struct task_struct *task)
{
	/* TODO 1: Create /proc/proc_mirror directory. */
	proc_dir = proc_mkdir(PROC_DIR_NAME, NULL);
	if (!proc_dir)
		return -ENOMEM;

	/* TODO 2: Walk task->files fdtable (fd >= 3) with proper synchronization. */
	struct files_struct *files = task->files;
	int created = 0;

	spin_lock(&files->file_lock);
	int max_fds = files_fdtable(files)->max_fds;
	spin_unlock(&files->file_lock);

	/* TODO 3: For each valid struct file, resolve absolute path via d_path. */
	for (int i = 3; i < max_fds; i++)
	{
		char path_buf[PATH_MAX];
		char* path;
		char name[20];
		struct proc_dir_entry *ent;

		spin_lock(&files->file_lock);
		struct file* file = files_fdtable(files)->fd[i];
		if(!file)
		{
			spin_unlock(&files->file_lock);
			continue;
		}
		get_file(file);
		spin_unlock(&files->file_lock);

		path = d_path(&file->f_path, path_buf, PATH_MAX);
		
		snprintf(name, sizeof(name), "fd-%d", i);

		/* TODO 4: Create symlink "fd-<N>" -> <resolved path> using proc_symlink. */
		ent = proc_symlink(name, proc_dir, path);
		if(ent)
			created++;

		fput(file);

		spin_lock(&files->file_lock);
		max_fds = files_fdtable(files)->max_fds;
		spin_unlock(&files->file_lock);
	}
	
	/* TODO 5: If at least one symlink was created, return 0; else return -ENOENT. */
	if(!created)
	{
		remove_proc_subtree(PROC_DIR_NAME, NULL);
		proc_dir = NULL;
		return  -ENOENT;
	}

    return 0;
}

static void destroy_proc_entries(void)
{
    if (proc_dir) {
        remove_proc_subtree(PROC_DIR_NAME, NULL);
        proc_dir = NULL;
    }
}

static int __init proc_mirror_init(void)
{
	struct task_struct *task;
	int ret;

	if (pid <= 0) {
		pr_err("proc_mirror: please set pid (>0)\n");
		return -EINVAL;
	}
	task = get_pid_task(find_vpid(pid), PIDTYPE_PID);
	if (!task)
		return -ESRCH;

	ret = create_proc_entries_for_pid(task);
	put_task_struct(task);
	if (ret)
		return ret;

	pr_info("proc_mirror: created entries under /proc/%s for pid=%d\n", PROC_DIR_NAME, pid);
	return 0;
}

static void __exit proc_mirror_exit(void)
{
	destroy_proc_entries();
	pr_info("proc_mirror: unloaded\n");
}

module_init(proc_mirror_init);
module_exit(proc_mirror_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ProcMirror");
MODULE_DESCRIPTION("Expose some opened FDs of a PID as files under /proc/proc_mirror");
MODULE_VERSION("1.0");
