.include "m328PBdef.inc" 	;ATmega328P microcontroller definitions
.def counter= r16

.cseg
.org 0x0
    rjmp reset
    
.org 0x4			;jump to interrupt service routine 
    rjmp isr1

reset:
    ldi r24,LOW(RAMEND)
    out SPL,r24
    ldi r24,HIGH(RAMEND)
    out SPH,r24			;initialize stack pointer
    
    ;setting of i/o
    ser r24
    out DDRC,r24		;set PORTC as output
    
    clr r24
    out DDRD,r24		;set PORTD as input
    
    ser r24
    out PORTD,r24		;pull-up PORTD
    
    ;prepare for interrupts
   
    ;Interrupt on falling edge of PD3 pin
    ldi r24,(1<<ISC11)
    sts EICRA,r24
    
    ;Enable INT1 interrupt 
    ldi r24,(1<<INT1)
    out EIMSK,r24
    
    sei				 ;enable general flag of interrupts
    
    clr counter			 ;we have counted 0 interrupts
    out PORTC,counter
    
    
main:
    in r24,PIND
    andi r24,0b00100000 
    cpi r24,0
    brne main
    cli
    
hold:
    in r24,PIND
    andi r24,0b00100000 
    cpi r24,0
    breq hold
    
    ldi r24,(1 << INTF1)
    out EIFR, r24		  ;clear external interrupt flag
    sei
    rjmp main 			  ;wait for interrupts
 


isr1:
    push r23
    push r24
    push r25
    in r25,SREG			  ;preserve flags
    push r25
    
    inc counter			  ;count interrupt
    cpi counter,0x8		    
    brne light
    clr counter			  ;restart counter after 7 interrupts
    
light:
    out PORTC,counter		  ;show number
    
    ;Delay 300 mS
    ldi r24, low(16*300)	  ;Init r25, r24 for delay 300 mS
    ldi r25, high(16*300)	  ;CPU frequency = 16 MHz
delay1:
    ldi r23, 249 ; (1 cycle)
delay2:
    dec r23 ; 1 cycle
    nop ; 1 cycle
    brne delay2 ; 1 or 2 cycles
    sbiw r24, 1 ; 2 cycles
    brne delay1 ; 1 or 2 cycles
   
    ;update flag
    ldi r24,(1 << INTF1)
    out EIFR, r24		  ;clear external interrupt flag
 
    pop r25
    out SREG,r25
    pop r25
    pop r24
    pop r23
    
    reti
    
