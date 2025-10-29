
#include <stdio.h> 
#include <stdlib.h>
#define F_CPU 16000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>


int dc_value, step, counter = 0, avg = 0, temp;
uint16_t input;




int main(){
    
    //set up timer1A
    TCCR1B = (1<<WGM12)|(1<<CS10);
    TCCR1A = (1<<WGM10)|(1<<COM1A1);
    
    //set up ports
    DDRB = 0x2;
    DDRD = 0xFF;
    DDRC = 0x0;
    
    //create table and initialize duty cycle
    int pwd[] = {5, 20, 36, 51, 67, 82, 97, 113, 128, 143, 159, 174, 189, 205, 220, 236, 251};
    step = 8;
    PORTC = step;
    dc_value = pwd[step];
    OCR1AL = dc_value;
    OCR1AH = 0;
    
    //set up ADC-related registers
    ADMUX = (1<<MUX0)|(1<<REFS0);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
    
    while(1){
        
        //check for brightness increase(PB4)
        if((PINB & (1<<PB4)) == 0)
            if(step < 16){
                dc_value = pwd[++step];
                OCR1AL = dc_value;
            }
        
        //check for brightness decrease(PB3)
        if((PINB & (1<<PB3)) == 0)
            if(step > 0){
                dc_value = pwd[--step];
                OCR1AL = dc_value;
            }
        
        //delay and read from adc input
        _delay_ms(100);
        
        //start conversion
        ADCSRA |= (1 << ADSC);
    
        //wait for conversion to complete
        while (ADCSRA & (1 << ADSC));
           
        input = ADC;
        temp += input;
        ++counter;
        
        //after 16 loops show avg through portd
        if(counter == 16){
            counter = 0;
            avg = (temp>>4);
            if(avg <= 200 && avg >= 0) 
                PORTD = 0x1;
            else if(avg <= 400) 
                PORTD = 0x2;
            else if(avg <= 600) 
                PORTD = 0x4;
            else if(avg <= 800) 
                PORTD = 0x8;
            else
                PORTD = 0x10;    
            temp = 0;   
        }        
    }
        
    return 0;
}
