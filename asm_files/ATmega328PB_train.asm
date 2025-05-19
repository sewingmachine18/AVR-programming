.include "m328PBdef.inc" 	;ATmega328P microcontroller definitions
.DEF train = r16
.DEF delay = r17
    
; delay = (1000*F1+14) cycles (about DEL_mS in mSeconds)
.equ FOSC_MHZ = 16	    ;MHZ
.equ DEL_mS = 1000	    ;mS
.equ F1 = FOSC_MHZ*DEL_mS   
    
.cseg
.org 0
    
;Init the stack
reset:			    
    ldi r24, LOW(RAMEND)
    out SPL, r24    
    ldi r24, HIGH(RAMEND)
    out SPH, r24
    
    ldi r24, LOW(F1)	    ;for the delay
    ldi r25, HIGH(F1)
    
start:
    ser r23		    ;set r23 = 11111111
    out DDRD, r23	    ;set portD as output
    
    ldi train, 1	    ;init the output reg
    
R2L:
    OUT PORTD, train	    ;show output
    rcall delay_outer	    ;3s delay
    rcall delay_outer
    rcall delay_outer
    lsl train		    ;shift to the left
    brcc R2L		    ;if carry not zero,we are at the left edge
    rcall delay_outer	    ;delay 2s more is at edge
    rcall delay_outer
    ldi train, 0b01000000   ;load next led for output
    
L2R:
    OUT PORTD, train	    ;likewise 
    rcall delay_outer
    rcall delay_outer
    rcall delay_outer
    lsr train
    brcc L2R
    rcall delay_outer
    rcall delay_outer
    ldi train, 0b00000010
    rjmp R2L


;subroutine for delay (993 cycles)
delay_inner:	    
    ldi r23, 247
loop1:
    dec r23
    nop
    brne loop1
    nop
    ret
    
;main routine for delay (1000*F1+14 cycles)
delay_outer:
    push r24
    push r25
    
loop2:
    rcall delay_inner
    sbiw r24, 1
    brne loop2
    
    pop r25
    pop r24
    ret
