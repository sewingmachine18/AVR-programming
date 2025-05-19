.include "m328PBdef.inc" 	;ATmega328P microcontroller definitions
.DEF AN= r16 			;complement of A for F0
.DEF A = r17			;A for F1
.DEF B = r18			;B for F0 and F1
.DEF C = r19			;C for F0
.DEF CN = r20			;complement of C for F1
.DEF DN = r21 			;complement of D for F0,F1
.DEF temp = r22			;register for intermediate calculations

.cseg
.org 0 					;start address

;F0 = (A'*B + C*D')
;F1 = (A+D')*(B+C') =(A*B)+(A*C')+(D'*B)+(D'C')
;Init PORTC as output
    ser temp ;the 2 LSB of PORTC are for the outputs F0,F1    
    out DDRC, temp			;make PORTC the output port
   
;Read A,B,C,D from the 4 LSB of PORTB 
    clr temp
    out DDRB,temp
    ser temp
    out PORTB,temp		;pull-up PORTB
READ1:  
	in temp,PINB
	com temp 		;take the complement of input because the buttons are normally closed
	mov AN,temp 		;A's complement in LSB of AN
	com AN
	mov A,temp 			;A in LSB of A
	lsr temp
	mov B,temp   		;B in LSB of B
	lsr temp	
	mov C,temp   		;C in LSB of C
	mov CN,temp 		;C's complement in LSB of CN
	com CN
	lsr temp
	mov DN,temp  		;D's complement in LSB of DN
	com DN
F0:
	and AN,B 			;AN = A'*B 
	and C,DN 			;C = C*D'
	or AN,C				;LSB of AN = F0
	andi AN,1			;keep only LSB
F1:	
	or A,DN				;A = A+D'
	or CN,B				;CN = C'+B
	and A,CN 			;A = A*C'
	andi A,1			;keep only LSB
	lsl A 				;take the LSB to bit 1
OUTPUT:
	or A,AN				;Create output
	out PORTC,A			;Show result
	rjmp READ1			;read input again
