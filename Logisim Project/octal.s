OCTAL:
lui $r3, 0 #zero out r3, r2, r1
lui $r2, 0
lui $r1, 0
lw $r0, 0($r3) #r1 is mem(0)
sllv $r0 $r0 7
srlv $r0 $r0 7
andi $r1, $r0, 7
andi $r2, $r0, 56
sllv $r2, $r2, 1
andi $r3, $r0, 448
sllv $r3, $r3, 2
or $r0, $r2, $r1
or $r0, $r0, $r3
disp $r0, 0

