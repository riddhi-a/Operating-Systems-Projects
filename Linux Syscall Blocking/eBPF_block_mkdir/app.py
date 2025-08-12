# Research by:- Shreyas Bargale and Prince Garg
# Code By:- Prince Garg

from bcc import BPF
import ctypes as ct
import os
# Import required libraries

# Function to add elements in the shared hash map
def add_target_ns(map, ns):
    key = map.Key()
    key.inode_num = ct.c_ulong(ns)
    value = ct.c_int(1)
    map[key] = value

def main():
    with open("ebpf_program.c") as f:
        bpf_prog = f.read()
    
    b = BPF(text=bpf_prog)
    
    #get the prefix of the system specific function name and add openat to it
    fnname_openat = b.get_syscall_prefix().decode() + 'mkdirat'
    b.attach_kprobe(event=fnname_openat, fn_name="syscall__mkdirat")
    target_ns = b.get_table("target_ns")
    
    # restricting for current namespace for demo
    devinfo = os.stat("/proc/self/ns/mnt")
    restricted_ns = [devinfo.st_ino]
    for ns in restricted_ns:
        add_target_ns(target_ns, ns)
        
    # Just Checking if the hash map is fine or not.
    for x in target_ns:
    	print("Blocking for namespace inode:",x.inode_num)
    
    try:
        print("Attaching kprobe to syscall mkdirat... Press Ctrl+C to exit.")
        b.trace_print() # read the output of bpf_trace_printk unless interrupted.
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()

