// A bubble sort in x86-64 Linux assembly using AT&T syntax.

.section .bss
swapped: .byte 0                      # flag

        .section .data
array:   .long 5, 3, 8, 1, 2, 1       # the array
len:     .long 6                      # length

        .section .text
        .globl _start

_start:
        lea  array(%rip), %rsi        # %rsi <- &array[0]
        mov  len(%rip), %ecx          # %ecx <- n (array length)

// outer do-while loop
bubble_outer:
        movb $0, swapped(%rip)        # swapped = false
        mov  $1, %edx                 # i = 1            (index register)

// inner for (i = 1; i < n; ++i)
bubble_inner:
        cmp  %ecx, %edx               # i < n ?
        jge  check_swapped            # if i >= n, break inner loop

        mov  (%rsi,%rdx,4), %eax      # eax <- arr[i]
        mov  -4(%rsi,%rdx,4), %ebx    # ebx <- arr[i-1]
        cmp  %eax, %ebx               # if arr[i] < arr[i-1] ?
        jle  no_swap

        /* swap arr[i-1] and arr[i] */
        mov  %eax, -4(%rsi,%rdx,4)    # arr[i-1] = arr[i]
        mov  %ebx,   (%rsi,%rdx,4)    # arr[i]   = old arr[i-1]
        movb $1, swapped(%rip)        # swapped = true

no_swap:
        inc  %edx                     # ++i
        jmp  bubble_inner

// check flag / decide another pass
check_swapped:
        cmpb $0, swapped(%rip)
        jne  bubble_outer             # if swapped == true, repeat outer loop

// finished
exit_syscall:
        mov  $60, %rax                # sys_exit
        xor  %rdi, %rdi               # status = 0
        syscall
