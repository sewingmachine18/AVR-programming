
#include <stdio.h> 
#include <stdlib.h>
#define F_CPU 16000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>

volatile int count = 0;
    
    
ISR(INT1_vect){

    //debouncer
    do{
        EIFR = (1<<INT1);
        _delay_ms(5);
    }while((EIFR & (1<<INT1)) != 0);
    
    if(count != 0)
        PORTB = 0x3F;
    else
        PORTB = 0x8;
    
    count = 4000;
    
}
    
    
int main(int argc, char *argv[]){
    
    //config interrupts
    EICRA = (1<<ISC11);
    EIMSK = (1<<INT1);
    sei();
    
    //set up ports
    DDRD = 0x00;
    DDRB = 0xFF;
    
    while(1){
        
        PORTB = 0; //turn off light
        while(count == 0); // wait for interrupt
        
        while(count > 0){
            --count;
            _delay_ms(1);
            
            if(count == 3000)
                PORTB &= 0b00001000;
            
        }
    }
    
    return(EXIT_SUCCESS);
}
