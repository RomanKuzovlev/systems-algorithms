.section .bss
swapped:
    .skip 1                     # 1 byte for the swapped flag

.section .data
array:
    .long 5, 3, 8, 1, 2, 1      # 5 ints array
len:
    .long 5

.globl _start
.section .text

_start:
    lea array(%rip), %rsi      # %rsi = pointer to array
    mov len(%rip), %ecx        # %ecx = len (5)

    movb $1, swapped(%rip)

    # Exit syscall (Linux x86-64)
    mov $60, %rax         # syscall number for exit
    xor %rdi, %rdi        # exit code 0
    syscall



