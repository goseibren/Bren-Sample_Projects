MOD:
	lui $r3, 0
	lw $r0, 0($r3)
	lw $r1, 1($r3)
	slt $r2, $r0, $r1
	beq $r2, 1, DONE
LOOP:
	sub $r0, $r0, $r1
	slt $r2, $r0, $r1
	beq $r2, 1, DONE 
DONE:
	sw $r0, 2($r3) #store r0 into memory 2


