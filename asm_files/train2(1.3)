.include "m328PBdef.inc" 	;ATmega328P microcontroller definitions
.def temp = r16
.def train = r17
    
.equ FOSC = 16	    ;MHz
.equ x_msec = 1000  ;define x msec for the delay
       
    
.cseg
.org 0		    ;start address

;Init stack
reset:
    ldi temp, LOW(RAMEND)
    out SPL, temp
    ldi temp, HIGH(RAMEND)
    out SPH, temp
     
    ldi r24, LOW(x_msec) ;
    ldi r25, HIGH(x_msec); pass x into r24,r25
    
    ser temp		;
    out DDRD, temp	; set portD as output
    

main:
    ldi train, 0x01	;init train position
    set			;T =1
    
R2L:
    out PORTD, train	;show output
    rcall wait_x_msec
    rcall wait_x_msec	;wait 2s
    lsl train		;shift to left position
    brcc R2L		;if out of bounds->change direction

    clt			;T = 0
    ldi train, 0b01000000 ;prepare next step
    rcall wait_x_msec	  ;wait extra 1s

L2R:			;likewise
    out PORTD, train
    rcall wait_x_msec	  ;
    rcall wait_x_msec	  ;
    lsr train
    brcc L2R
    
    set
    ldi train, 0x02
    rcall wait_x_msec
    rjmp R2L
    
    
        
;subroutine for delay (993 cycles)
delay_inner:	
    ldi r23, 247    ;1 cycle
loop1:
    dec r23	    ;1 cycle
    nop		    ;1 cycle
    brne loop1	    ;1 (or 2 on branch) cycle(s)
    nop		    ;1 cycle
    ret		    ;4 cycles
    
;main routine for delay (x*16000 + 11 cycles)
wait_x_msec:
    push r24	    ;2 cycles 
    push r25	    ;2 cycles
    
loop3:
    ldi r22, FOSC    ;1 cycle
loop2:
    rcall delay_inner ;996 cycles
    dec r22	    ;1 cycles
    brne loop2	    ;1 (or 2 on branch) cycles
    
    sbiw r24, 1	    ;2 cycles
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop		    ;12 cycles of nops
    brne loop3	    ;1 (or 2 on branch) cycles
    
    pop r25	    ;2 cycles
    pop r24	    ;2 cycles
    ret		    ;4 cycles



