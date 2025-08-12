// Research by:- Shreyas Bargale and Prince Garg
// Code By:- Prince Garg

#include <uapi/linux/ptrace.h>
#include <linux/sched.h> // TASK_COMM_LEN
#include <linux/ns_common.h> // ns_common structure
#include <linux/nsproxy.h> // nsproxy structure
#include <linux/fs.h> // For filename handling
#include <linux/mount.h>

// mnt_namespace struct was said to be not defined under my included files
struct mnt_namespace {
    struct ns_common ns;
};


struct key_t { // Uniquely identifies a namespace file
    u64 inode_num;
};

BPF_HASH(target_ns, struct key_t, int); // 1 if namespace to block

// kprobe on mkdirat, hook onto the running kernel code
// parameters are :- context, directory file descriptor, filename pointer, associated flags
// found using strace mkdir test_dir
int syscall__mkdirat(struct pt_regs *ctx, int dfd, const char __user *filename, int flags) {
    struct key_t curr_ns = {};
    
    // get the inode of namespace file of the caller task. inode is:-
    // current_task->nsproxy->mnt_ns->ns.inum, but do it safely:-
    
    struct task_struct *current_task;
    struct nsproxy *nsproxy;
    struct mnt_namespace *mnt_ns;
    unsigned int inum;
    u64 ns_id;
    
    current_task = (struct task_struct *)bpf_get_current_task();

    // copies size bytes from kernel address space to the BPF stack
    // For safety kernel memory reads are done through this function
    if (bpf_probe_read_kernel(&nsproxy, sizeof(nsproxy), &current_task->nsproxy))
	return 0;

    if (bpf_probe_read_kernel(&mnt_ns, sizeof(mnt_ns), &nsproxy->mnt_ns))
	return 0;

    if (bpf_probe_read_kernel(&inum, sizeof(inum), &mnt_ns->ns.inum))
        return 0;

    ns_id =  (u64) inum;
    curr_ns.inode_num = ns_id;

    // Check if the current namespace is in the hash map shared
    int *fnd = target_ns.lookup(&curr_ns);
    bpf_trace_printk("mkdirat called by process with namespace inode: %llu\n",curr_ns.inode_num); // log

    if (fnd) {
        // Block mkdirat syscall if namespace is found
        char fname[NAME_MAX];
        // safely copy a NULL terminated string from user address space to the BPF stack
        bpf_probe_read_user_str(fname, sizeof(fname), (void *)filename);
        bpf_trace_printk("mkdirat command blocked while creating directory %s!\n", fname);	// log
        bpf_override_return(ctx, -EACCES); // Block, give -EACCES return code :- Permission Denied
    }

    return 0; // allow the syscall
}
