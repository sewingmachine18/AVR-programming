.include "m328PBdef.inc" 	;ATmega328P microcontroller definitions
.def F0 = r14
.def F1 = r15
.def A = r16
.def B = r17
.def BN = r18
.def C = r19
.def D = r20
.def DN = r21
.def temp = r22
.def cnt = r23
.def adder = r24
    
    
.cseg
.org 0
    
;F0 = (AN*B+BN*D) = (A+BN)*(B+DN)
;F1 = (A+C)*(B+D)
setup:
    ;variables
    ldi cnt, 6	    ;
    ldi A, 0x52	    ;
    ldi B, 0x42	    ;
    mov BN, B	    ;
    com BN	    ;    
    ldi C, 0x22	    ;
    ldi D, 0x02	    ;
    mov DN, D	    ;
    com DN	    ;    
    
    ;ports for output
    ser temp	    ;
    out DDRD, temp  ;
    out DDRC, temp  ;
    
    
main:
    ;F0 calculation
    mov temp, A	    ;temp = A
    or temp, BN	    ;temp = A+BN
    mov F0, B	    ;F0 = B
    or F0, DN	    ;F0 = B+DN
    and F0, temp    ;F0 = F0*temp = (A+BN)*(B+DN)
    
    ;F1 calculation
    mov temp, A	    ;temp = A
    or temp, C	    ;temp = A+C
    mov F1, B	    ;F1 = B
    or F1, D	    ;F1 = B+D
    and F1, temp    ;F1 = F1*temp = (A+C)*(B+D)
    
    ;increaments
    ldi adder, 0x01 ;
    add A, adder    ;
    inc adder
    add B, adder    ;
    mov BN, B	    ;
    com BN	    ; 
    inc adder	    ;
    add C, adder    ;
    inc adder	    ;
    add D, adder    ;
    mov DN, D	    ;
    com DN	    ;
    
    ;show results
    out PORTD, F1   ;
    out PORTC, F0   ;
    
    ;loop
    subi cnt, 1	    ;cnt -= 1
    brne main	    ;if(cnt!=0) go to main
    
program_end:
    rjmp program_end
    
