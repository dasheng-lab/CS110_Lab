.data
test_input: .word 6 3 5 12

.text
main:
	add t0, x0, x0
	addi t1, x0, 4
	la t2, test_input
main_loop:
	beq t0, t1, main_exit
	slli t3, t0, 2
	add t4, t2, t3
	lw a0, 0(t4)

	addi sp, sp, -20
	sw t0, 0(sp)
	sw t1, 4(sp)
	sw t2, 8(sp)
	sw t3, 12(sp)
	sw t4, 16(sp)

	jal ra, seriesOp

	lw t0, 0(sp)
	lw t1, 4(sp)
	lw t2, 8(sp)
	lw t3, 12(sp)
	lw t4, 16(sp)
	addi sp, sp, 20

	addi a1, a0, 0
	addi a0, x0, 1
	ecall # Print Result
	addi a1, x0, ' '
	addi a0, x0, 11
	ecall
	
	addi t0, t0, 1
	jal x0, main_loop
main_exit:  
	addi a0, x0, 10
	ecall # Exit

seriesOp: #iteration
	add t0, a0, x0 #t0=testword
	addi t1, x0, 1 # 1+ 0-
	add t2, x0, x0 #sum=0
loop:
	beq t0, x0, return
	mul t3, t1, t0 #t3=sign*t0
	add t2, t2, t3 #sum+=t3
	neg t1, t1     
	addi t0, t0, -1 #testword-=1
	jal x0, loop
return:
	add a0, t2, x0 
	ret