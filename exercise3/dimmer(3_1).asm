
.include "m328PBdef.inc"
.equ FOSC_MHZ = 16
.equ del_ms = 200
.equ del_nu = FOSC_MHZ*del_ms
.def temp = r16
.def temp2 = r17
.def dc_value = r18
.def step = r19
    
.cseg
.org 0x0
    rjmp reset
    
    
;---------------------------------
reset:
    ;init stack pointer
    ldi temp, low(RAMEND)
    out SPL, temp
    ldi temp, high(RAMEND)
    out SPH, temp
    
    ;set up TMR1A
    ldi temp, (1<<CS10)|(1<<WGM12)
    sts TCCR1B, temp
    ldi temp, (1<<WGM10)|(1<<COM1A1)
    sts TCCR1A, temp
    
    ;set up ports
    ldi temp, 0x2   ;only pb1 is output
    out DDRB, temp
    ser temp
    out DDRC, temp
    
    ;init dc_value and duty cycle
    ldi ZL, low(PWMtable<<1)
    ldi ZH, high(PWMtable<<1)
    ldi step, 0x8
    add ZL, step
    ldi temp, 0
    adc ZH, temp
    lpm dc_value, Z
    sts OCR1AL, dc_value
    sts OCR1AH, temp

    
    
main:
    out PORTC, step
    in temp, PINB
    com temp
    mov temp2, temp
    andi temp, 0b00010000
    cpi temp, 0b00010000
    breq increase
    andi temp2, 0b00001000
    cpi temp2, 0b00001000
    brne main
    
decrease:    
    cpi step, 0
    breq main
    dec step
    sbiw ZL, 1
    lpm dc_value, Z
    sts OCR1AL, dc_value
    ldi r24, low(del_nu)
    ldi r25, high(del_nu)
    rcall delay_ms
    rjmp main
    
increase:
    cpi step, 16
    breq main
    inc step
    adiw ZL, 1
    lpm dc_value, Z
    sts OCR1AL, dc_value
    ldi r24, low(del_nu)
    ldi r25, high(del_nu)
    rcall delay_ms
    rjmp main

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
   
    
    ;create table
PWMtable:
    .DB 5, 20, 36, 51, 67, 82, 97, 113, 128, 143
    .DB 159, 174, 189, 205, 220, 236, 251


