.include "m328PBdef.inc"
.equ FOSC_MHZ = 16
.equ del_ms = 1000
.equ del_nu = FOSC_MHZ*del_ms
.def temp = r16
.def temp2 = r17
.def cnt_bits = r18
.def cnt = r26


.cseg
.org 0x0
    rjmp reset
    
.org 0x2
    rjmp isr0
    
    
;---------------------------------
reset:
;init stack pointer
    ldi temp, low(RAMEND)
    out SPL, temp
    ldi temp, high(RAMEND)
    out SPH, temp
   

    
;init ports
    ser temp
    out DDRC, temp  ;output PORTC to count
    clr temp
    out DDRD, temp  ;input for interrupts and hold
    out DDRB, temp  ;intput PORTB for number of buttons


;config interrupts
    ldi temp,(1<<ISC01) ;enable interrupts on falling edge of PD3
    sts EICRA, temp
    ldi temp, (1<<INT0)
    out EIMSK, temp
    
    sei			;enable interrupts
    
;------------------------------
;main program
loop1:
    clr cnt
loop2:
    mov temp2, cnt
    lsl temp2
    out PORTC, temp2
    
    ldi r24, low(del_nu)
    ldi r25, high(del_nu)
    rcall delay_ms
    
    inc cnt
    
    cpi cnt, 32
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
;interrupt service routine
isr0:
    ;stack time
    push r24
    push r25
    push temp
    in temp, SREG
    push temp
   
    ;main isr
    clr cnt_bits
    in temp, PINB
    andi temp, 0b00011110  ;isolate pb1-pb4
    lsr temp
bit1:
    lsr temp
    brcc bit2
    rol cnt_bits
bit2:
    lsr temp
    brcc bit3
    rol cnt_bits
bit3:
    lsr temp
    brcc bit4
    rol cnt_bits
bit4:
    lsr temp
    brcc lights
    rol cnt_bits
    
lights:
    out PORTC, cnt_bits
    rcall delay_ms
    
    ;stack time
    pop temp
    out SREG, temp
    pop temp
    pop r25
    pop r24
    
    reti


