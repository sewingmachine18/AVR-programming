.include "m328PBdef.inc"    	;ATmega328P microcontroller definitions
.DEF AN= r16 			;complement of A for F0
.DEF B = r17
.DEF C = r18
.DEF DN = r19 			;complement of D for F0,F1
.cseg
.org 0 ;start address

;F0 = (A'*B + C*D')
;F1 = (A+D')*(B+C') =(A*B)+(A*C')+(D'*B)+(D'C')
;Init PORTC as output
    	ldi r12,0b00000011 	;the 2 LSB of PORTC are for the outputs F0,F1    
    	out DDRC, r26		;make PORTC the output port
   
;Read A,B,C,D from the 4 LSB of PORTB 
    	clr r12
    	out DDRB,r12
    	ser r12
    	out PORTB,r12
F0:  	in r12,PINB
	mov AN,r12 		;A's complement in LSB of AN
	com AN
	lsr r12
	mov B,r12   		;B in LSB of B
	lsr r12	
	mov C,r12   		;C in LSB of C
	lsr r12
	mov DN,r12  		;D's complement in LSB of DN
	com DN
	lsr r12
	and AN,B 		;AN = A'*B 
	and C,DN 		;C = C*D'
