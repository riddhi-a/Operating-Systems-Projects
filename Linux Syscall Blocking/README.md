We have tried to intercept syscalls and apply namespace checking using the following methods:-
1) Hooks (Implementing hooks as loadable module)
    * KProbe_syscall_block_v1.0 --> uses kernel panic to block syscalls, when they happens from a namespace in which they are supposed to be blocked
    * KProbe_syscall_block_v2.0 --> nulls out the registers values for the syscall, when they happens from a namespace in which they are supposed to be blocked, thus leading to killing of process if atleast one argument to the syscall contains a pointer.
2) LSM (Linux Security Module)
3) eBPF (Extended Berkeley Packet Filter) :- see eBPF_block_mkdir
