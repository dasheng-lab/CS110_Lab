.data
.byte 0x11
my_data: # error, addr is not multiply of 4
    .space 4
    .byte 0x11 0x22 0x33 0x44
    .word 0x12345678
    
.text
_start:
    la a1, my_data
    lb a0, 0(a1)
    lw a2, 0(a1)
