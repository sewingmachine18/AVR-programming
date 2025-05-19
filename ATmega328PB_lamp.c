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

int counter = 0;
ISR(INT1_vect) // External INT1 ISR
{
    if(counter!=0){
        PORTB = 0x0F;
    }
    else{
        PORTB =0x08;
    }
    
    counter = 5000;
   
    EIFR = (1 << INTF1); // Clear the flag of interrupt INTF1
}

/*
 * 
 */
int main(int argc, char** argv) {
        
    //Interrupt on the rising edge of 
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
        while(counter==0);
        
        while(counter>0){
            counter-- ;
            if(counter==4500)
                PORTB = 0x08;
        }
    }
    
    
    

    return (EXIT_SUCCESS);
}

