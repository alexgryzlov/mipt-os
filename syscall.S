    .section .userspace.text, "ax"
    .global userspace_fn
    .type userspace_fn, @function
userspace_fn:
.l:
    mov $0x1, %eax
    int $0x80
    jmp .l
