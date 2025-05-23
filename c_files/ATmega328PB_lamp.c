/* 
 * File:   lamp.c
 * Author: tspen
 *
 * Created on May 19, 2025, 10:45 PM
 */

#include <stdio.h> 
#include <stdlib.h>
#define F_CPU 16000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>

volatile int counter = 0;
volatile uint8_t debouncer = 0;

ISR(INT1_vect) // External INT1 ISR
{
    if(debouncer == 0){
        if(counter == 0){
            PORTB = 0x08;
        }
        else if(debouncer == 0){
            PORTB = 0x0F;
        }
        counter = 5000;
        debouncer = 132;
    }
    EIFR = (1 << INTF1); // Clear the flag of interrupt INTF1
}

/*
 * 
 */
int main(int argc, char** argv) {
        
    //Interrupt on the falling edge 
    EICRA = (1<<ISC11);
    
    //Enable interrupts
    EIMSK = (1<<INT1);
    
    //port setup
    DDRB = 0xFF;
    DDRD = 0x00;
    PORTD = 0xFF;           //pull-up input
    
    sei();
    
    while(1){
        PORTB = 0x00;
        while(counter == 0);
        
        while(counter > 0){
            --counter;
            _delay_ms(1);
            if(debouncer > 0)
                --debouncer;
            if(counter == 4500)
                PORTB = 0x08;
        }
    }
    
      return (EXIT_SUCCESS);
}
