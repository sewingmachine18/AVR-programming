.include "m328PBdef.inc"
.equ FOSC_MHZ = 16
.equ del_ms = 1
.equ del_int = 5
.equ del_nu_int = FOSC_MHZ * del_int
.equ del_nu = FOSC_MHZ*del_ms
.def temp = r20
.def reg0 = r21
.def reg3000 = r22
.def cnt1 = r26
.def cnt2 = r27
    
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
    out DDRB, temp  ;output PORTB to lightbulb
    clr temp
    out DDRD, temp  ;input for interrupts and hold
    
    
;config interrupts
    ldi temp,(1<<ISC11) ;enable interrupts on falling edge of PD3
    sts EICRA, temp
    ldi temp, (1<<INT1)
    out EIMSK, temp
    
    sei			;enable interrupts
    
; prepare registers for compares
    ldi reg0, 0
    ldi reg3000, high(3000)

    
    
;------------------------------
;main program
main:

    ldi temp, 0
    out PORTB, temp ;turn off light
    
wait:
    cpi cnt1, low(0)
    cpc cnt2, reg0  
    breq wait
    
countdown:
    ldi r24, low(del_nu)
    ldi r25, high(del_nu)
    rcall delay_ms
    
    cpi cnt1, low(3000)
    cpc cnt2, reg3000
    brne continue
    ldi temp, 8
    out PORTB, temp
continue:  
    sbiw cnt1, 1
    brne countdown
       

    rjmp main
    
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
    
    ;main isr body
    ldi temp, 0x8
    cpi cnt1, low(0)
    cpc cnt2, reg0
    breq count_up
    ldi temp, 0x3F
count_up:
    out PORTB, temp
    ldi cnt1, low(4000)
    ldi cnt2, high(4000)

   
    ;stack time
    pop temp
    out SREG, temp
    pop temp
    pop r25
    pop r24
    
    reti
