.section .data
array:
    .long 5, 3, 8, 1, 2      # 5 ints array
len:
    .long 5

.globl _start
.section .text

_start:
   

    # Exit syscall (Linux x86-64)
    mov $60, %rax         # syscall number for exit
    xor %rdi, %rdi        # exit code 0
    syscall