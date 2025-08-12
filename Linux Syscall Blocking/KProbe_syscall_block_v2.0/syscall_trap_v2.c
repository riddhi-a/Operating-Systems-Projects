#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/nsproxy.h>
#include <linux/panic.h>
#include <linux/pid_namespace.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/uaccess.h>

/********************************************************************************************************************************
 | Note to the reader:                                                                                                          |
 | kprobe is a kernel module that is used to intercept the execution of a function in the kernel space,                         |
 | and run a custom handler function before the function is executed. The kprobe modules were not intended to block or          |
 | edit any syscall, but to log the information about the syscall.                                                              |
 | To make the kprobe module to block syscalls, some dustructive methods are used.                                              |
 | Whenever the kprobe module detects the namespace in which the syscall is to be blocked, it will edit the registers           |
 | and set the registers to 0. So, when the syscall happens and there is some pointer dereferencing which is present in the     |
 | registers, as an argument to the syscall, a kernel bug will be reported and the process will be killed.                      |
 | Thus blocking happens by bugging the kernel and killing the process, and thus stopping the syscall execution.                |
 ********************************************************************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("VIKASH_KUMAR_OJHA");
MODULE_DESCRIPTION("Kprobe Module to stop SYSCALL syscall");
MODULE_VERSION("2.0");

// PID_QSIZE is the size of the array of inum of the PID namespaces for which the "SYSCALL" syscall is to be blocked
// the script will edit the value of PID_QSIZE to the number of entries in the TARGET_PID_NAMESPACE array
#define PID_QSIZE 1

// kp is a kprobe structure that will resolve the address of the __x64_sys_SYSCALL function and put a probe just before the function is executed
static struct kprobe kp = {
    // symbol_name -> name of the symbol to be resolved
    // Note: SYSCALL is just a placeholder, the script will replace it with the actual syscall name
    .symbol_name = "__x64_sys_SYSCALL",
};

static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    printk(KERN_INFO "Kprobe handler called\n");
    printk(KERN_INFO "Syscall happened for SYSCALL\n");
    printk(KERN_INFO "The current process PID: %d\n", current->pid);
    printk(KERN_INFO "The current process name: %s\n", current->comm);
    printk(KERN_INFO "The current instruction pointer: %lx\n", regs->ip);
    printk(KERN_INFO "The currnt code segment: %lx\n", regs->cs);
    // TARGET_PID_NAMESPACE is an array of inum of the PID namespaces for which the "SYSCALL" syscall is to be blocked
    int TARGET_PID_NAMESPACE[] = {12345};  // Append entries here
    for (int i = 0; i < PID_QSIZE; ++i) {
        // Check if the current process is in the target PID namespace
        // current is a macro that points to the task_struct of the current process
        // #define current (get_current())
        // get_current() returns the task_struct of the current process
        // task_active_pid_ns(current) returns the PID namespace of the current process
        // task_active_pid_ns(current)->ns.inum returns the inum of the PID namespace of the current process
        // inum is the inode number of the PID namespace

        /********************************************************************************************************************************
         | In the current implementation, pid namespace is used to block the syscall.                                                   |
         | To use other namespaces, the following can be used:                                                                          |
         | current->nsproxy->{cgroup_ns, ipc_ns, mnt_ns, net_ns, uts_ns}->ns.inum can be compared with the target namespace inum        |
         ********************************************************************************************************************************/

        // If the current process is in the target PID namespace, then block the "SYSCALL" syscall
        // task_active_pid_ns(current) returns task_struct of the current process which contains info about the PID namespace of the current process
        if (task_active_pid_ns(current)->ns.inum == TARGET_PID_NAMESPACE[i]) {
            // if root then don't block
            if (current_uid().val == 0) {
                printk(KERN_INFO "Root user detected, not blocking SYSCALL syscall\n");
                return 0;
            }

            printk(KERN_INFO "Blocked SYSCALL syscall for task in PID namespace: %u\n", task_active_pid_ns(current)->ns.inum);

            /***************************************************************************************************************************************
             * Below we edit the registers values to block the syscall.                                                                            *
             * All the registers are set to 0.                                                                                                     *
             * So if any register is carrying a pointer to some memory location, then in dereferencing the pointer, a kernel bug will be reported. *
             * The kernel bug will be reported because the pointer is pointing to the memory location 0, which is not a valid memory location      *
             * and the process will get eventually killed.                                                                                         *
             ***************************************************************************************************************************************/
            regs->ax = 0;
            regs->bx = 0;
            regs->cx = 0;
            regs->dx = 0;
            regs->si = 0;
            regs->di = 0;
        }
    }
    return 0;
}

static int __init kprobe_init(void) {
    int ret;
    ret = register_kprobe(&kp);  // registering kprobe module
    if (ret < 0) {               // checking if kprobe is registered successfully
        printk(KERN_ERR "Failed to register kprobe: %d\n", ret);
        return ret;
    }
    kp.pre_handler = handler_pre;  // setting pre_handler for kprobe
    // pre_handler -> handler function to be called before the function is executed
    printk(KERN_INFO "Kprobe registered for __x64_sys_SYSCALL\n");
    return 0;
}

static void __exit kprobe_exit(void) {
    unregister_kprobe(&kp);  // unregistering kprobe module
    printk(KERN_INFO "Kprobe unregistered for __x64_sys_SYSCALL\n");
}

module_init(kprobe_init);  // modult_init -> macro to define the function to be called when the module is loaded
module_exit(kprobe_exit);  // module_exit -> macro to define the function to be called when the module is unloaded
