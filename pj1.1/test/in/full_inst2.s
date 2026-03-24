.data
label3:
.word -2147483648
.byte 0x11 0x22 0x33 0x44
.space 1
.space 3

.text
label1:
ori  x11 x12 10
andi x13 x14 12
slli x15 x16 1
label2:
lh x27 0(x28)
lw x29 2(x30)
lbu x31 9(x2)

lui x30 107592
auipc a1 717430
jal x13 label1

j -20
jr ra
jal label1
jalr ra
lw s1 label2

lw x25 label3
slli x25 x9 9

.data
.word 0x1
.space 4
.byte 0x1 0x2 0x3 0x4