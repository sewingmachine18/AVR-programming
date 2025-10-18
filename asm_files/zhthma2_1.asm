.include "m328PBdef.inc"
.equ FOSC_MHZ = 16
.equ del_ms = 500
.equ del_int = 5
.equ del_nu_int = FOSC_MHZ * del_int
.equ del_nu = FOSC_MHZ*del_ms
.def temp = r16
.def cnt = r26
.def int_cnt = r17

.cseg
.org 0x0
    rjmp reset
    
.org 0x4
    rjmp isr1
    
    
;---------------------------------
reset:
;init stack pointer
    ldi temp, low(RAMEND)
    out SPL, temp
    ldi temp, high(RAMEND)
    out SPH, temp
   

    
;init ports
    ser temp
    out DDRB, temp  ;output PORTB to count to 16
    out DDRC, temp  ;output PORTC to count interrupts
    clr temp
    out DDRD, temp  ;input for interrupts and hold

;config interrupts
    ldi temp,(1<<ISC11) ;enable interrupts on falling edge of PD3
    sts EICRA, temp
    ldi temp, (1<<INT1)
    out EIMSK, temp
    
    clr int_cnt 
    out PORTC, int_cnt	;initialize int counter as zero and show on PORTC
    
    sei			;enable interrupts
    
;------------------------------
;main program
loop1:
    clr cnt
loop2:
    out PORTB, cnt
    
    ldi r24, low(del_nu)
    ldi r25, high(del_nu)
    rcall delay_ms
    
    inc cnt
    
    cpi cnt, 16
    breq loop1
    rjmp loop2
  
    
;-------------------------------------
;routine for delays
delay_ms:
    ldi r23, 249;
loop_inn:
    dec r23
    nop
    brne loop_inn
 
    sbiw r24,1
    brne delay_ms

    ret
   
	
;-------------------------------
;isr
isr1:
    ;stack time
    push r24
    push r25
    push temp
    in temp, SREG
    push temp
   
    ;debouncer
debounce:
    ldi temp, (1<<INTF1)
    out EIFR, temp
    ldi r24, low(del_nu_int)
    ldi r25, high(del_nu_int)
    rcall delay_ms
    in temp, EIFR
    andi temp, (1<<INTF1)
    brne debounce
  
    ;main isr
    in temp, PORTD
    andi temp, 1    ;get PD1
    brne light	    ;if pd1==1 dont inc the int_counter
    
    inc int_cnt
    cpi int_cnt, 32
    brne light
    clr temp

light:
    out PORTC, int_cnt
    
    ;stack time
    pop temp
    out SREG, temp
    pop temp
    pop r25
    pop r24
    
    reti
