.include "m328PBdef.inc"
.equ PD0=0 
.equ PD1=1 
.equ PD2=2 
.equ PD3=3 
.equ PD4=4 
.equ PD5=5
.equ PD6=6 
.equ PD7=7
    
.def temp = r16
.def digit = r17
.def adc_low = r18
.def adc_high = r19
.def mul_res0 = r20
.def mul_res1 = r21
.def mul_res2 = r22

  

   
.cseg
.org 0x0
    rjmp reset
.org 0x2A
    rjmp isr_ADC

;------------------------------------
; preparation
reset:
;init stack pointer
    ldi temp, low(RAMEND)
    out SPL, temp
    ldi temp, high(RAMEND)
    out SPH, temp

; setup_ports
    ser temp
    out DDRD, temp
    clr temp
    out DDRC, temp

;setup LCD
    
    clr r24
    rcall lcd_init
    
    ldi r24, low(100) 
    ldi r25, high(100)  ; delay 100 mS after "shift entire display" off/ "auto increment of AC" on
    rcall wait_msec 
   
;setup ADC 
    ldi temp, (1<<REFS0)|(1<<MUX0)|(1<<MUX1)
    sts ADMUX, temp
    ldi temp, (1<<ADEN)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)
    sts ADCSRA, temp
    
;enable interrupts
    sei
    lds temp, ADCSRA	;start conversation
    ori temp, (1<<ADSC)
    sts ADCSRA, temp
    
    
;--------------------------------------------
;loop for reading voltage (main function)
main:
    
    rjmp main
  
    
    
;----------------------------------------
;LCD handling
write_2_nibbles: 
    push r24         
    in r25 ,PIND       
    andi r25 ,0x0f  
    andi r24 ,0xf0       
    add r24 ,r25     
    out PORTD ,r24  
    
    sbi PORTD ,PD3	; Enable  Pulse 
    nop 
    nop 
    cbi PORTD ,PD3 
     
    pop r24		; Recover r24(LCD_Data) 
    swap r24    
    andi r24 ,0xf0	; r24[3:0] Holds previus PORTD[3:0]   
    add r24 ,r25	; r24[7:4] <-- LCD_Data_Low_Byte 
    out PORTD ,r24 
     
    sbi PORTD ,PD3	; Enable  Pulse 
    nop 
    nop 
    cbi PORTD ,PD3 
     
    ret 
    
    
lcd_data: 
    sbi PORTD ,PD2		; LCD_RS=1(PD2=1), Data 
    rcall write_2_nibbles	; send data 
    ldi r24 ,250                ; 
    ldi r25 ,0                  ; Wait 250uSec 
    rcall wait_usec 
    ret 
    
lcd_command: 
    cbi PORTD ,PD2		; LCD_RS=0(PD2=0), Instruction  
    rcall write_2_nibbles	; send Instruction 
    ldi r24 ,250                ; 
    ldi r25 ,0                  ; Wait 250uSec 
    rcall wait_usec 
    ret
    
lcd_clear_display: 
    ldi r24 ,0x01               ; clear display command 
    rcall lcd_command 
     
    ldi r24 ,low(5)		; 
    ldi r25 ,high(5)		; Wait 5 mSec 
    rcall wait_msec		; 
     
    ret
  
lcd_init: 
    ldi r24 ,low(200)	; 
    ldi r25 ,high(200)  ; Wait 200 mSec 
    rcall wait_msec	; 
    
    ldi r24 ,0x30	; command to switch to 8 bit mode 
    out PORTD ,r24	; 
    
    sbi PORTD ,PD3	; Enable  Pulse 
    nop 
    nop 
    cbi PORTD ,PD3 
    ldi r24 ,250	; 
    ldi r25 ,0		; Wait 250uSec  
    rcall wait_usec     ; 
     
    ldi r24 ,0x30	; command to switch to 8 bit mode 
    out PORTD ,r24	; 

 
    sbi PORTD ,PD3	; Enable  Pulse  
    nop 
    nop 
    cbi PORTD ,PD3 
    
    ldi r24 ,250   
    ldi r25 ,0		; Wait 250uSec  
    rcall wait_usec           
     
    ldi r24 ,0x30	; command to switch to 8 bit mode 
    out PORTD ,r24	; 
    sbi PORTD ,PD3	; Enable  Pulse  
    nop 
    nop 
    cbi PORTD ,PD3 
    ldi r24 ,250                  
    ldi r25 ,0          ; Wait 250uSec 
    rcall wait_usec 
     
    ldi r24 ,0x20       ; command to switch to 4 bit mode 
    out PORTD ,r24 
    
    sbi PORTD ,PD3	; Enable  Pulse 
    nop 
    nop 
    cbi PORTD ,PD3 
    
    ldi r24 ,250                  ; 
    ldi r25 ,0                    ; Wait 250uSec 
    rcall wait_usec 
     
    ldi r24 ,0x28              ;  5x8 dots, 2 lines 
    rcall lcd_command 
 
    ldi r24 ,0x0c                ; dislay on, cursor off 
    rcall lcd_command      
    rcall lcd_clear_display       
 
    ldi r24 ,0x06                ; Increase address, no display shift 
    rcall lcd_command		       
    ret  
    
;----------------------------------
;Routines for delays

wait_msec: 
    push r24  ; 2 cycles 
    push r25  ; 2 cycles 
    ldi r24 , low(999) ; 1 cycle 
    ldi r25 , high(999) ; 1 cycle  
    rcall wait_usec ; 998.375 usec   
    pop r25  ; 2 cycles 
    pop r24  ; 2 cycles 
    nop   ; 1 cycle 
    nop   ; 1 cycle 
 
    sbiw r24 , 1 ; 2 cycles   
    brne wait_msec ; 1 or 2 cycles 
 
    ret   ; 4 cycles
    
wait_usec: 
    sbiw r24 ,1  
    call delay_8cycles 
    brne wait_usec 
    ret 
    
delay_8cycles: 
    nop 
    nop 
    nop 
    nop 
    ret 
    
    
;----------------------------------
;Interrupt service routines
 
isr_ADC:	;we will calculate (ADC*Vref*100)/1024 = (ADC*125)/256 in order to have 2 digit accuracy
    
    lds adc_low, ADCL	;read ADC
    lds adc_high, ADCH
    
    ldi temp, 125	; calculate adc*125
    clr mul_res2
    
    mul adc_low, temp
    movw mul_res0, r0
    
    mul adc_high, temp
    add mul_res1, r0
    adc mul_res2, r1	;result in mul_res2:mul_res1
    
    rcall lcd_clear_display ;clear previous reading
    ldi r24, low(100)
    ldi r25, high(100)
    rcall wait_msec
    
    ;begin bcd transform
    ;hundreds
    ldi temp, 100
    ser digit
find_hundreds:
    inc digit
    sub mul_res1, temp
    sbci mul_res2, 0
    brcc find_hundreds
    
    add mul_res1, temp
    ldi temp, 0
    adc mul_res2, temp
    
    ldi r24, 0x30	;print hundreds
    or r24, digit
    rcall lcd_data  
    ldi r24, low(100)
    ldi r25, high(100)
    rcall wait_msec
    
    ldi r24, 0x2E	;print '.'
    rcall lcd_data  
    ldi r24, low(100)
    ldi r25, high(100)
    rcall wait_msec
    
    ;decades
    ldi temp, 10
    ser digit
find_decades:
    inc digit
    sub mul_res1, temp
    sbci mul_res2, 0
    brcc find_decades
    
    add mul_res1, temp
    ldi temp, 0
    adc mul_res2, temp
    
    ldi r24, 0x30	;print decades
    or r24, digit
    rcall lcd_data  
    ldi r24, low(100)
    ldi r25, high(100)
    rcall wait_msec
    
    ;units
    ldi temp, 1
    ser digit
find_units:
    inc digit
    sub mul_res1, temp
    sbci mul_res2, 0
    brcc find_units
    
    add mul_res1, temp
    ldi temp, 0
    adc mul_res2, temp
    
    ldi r24, 0x30	;print units
    or r24, digit
    rcall lcd_data  
    ldi r24, low(100)
    ldi r25, high(100)
    rcall wait_msec
   
    
    lds temp, ADCSRA	;start conversation
    ori temp, (1<<ADSC)
    sts ADCSRA, temp
    ldi r24, low(1000)	    ; wait 1 second
    ldi r25, high(1000)
    rcall wait_msec
    
    reti
