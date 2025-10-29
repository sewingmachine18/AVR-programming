
#include <stdio.h> 
#include <stdlib.h>
#define F_CPU 16000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>


int dc_value, step, temp;
uint8_t adc_in;


int main(){
    
    //set up timer1A
    TCCR1B = (1<<WGM12)|(1<<CS10);
    TCCR1A = (1<<WGM10)|(1<<COM1A1);
    
    //set up ports
    DDRB = 0x2;
    DDRD = 0xFF-0x3;
    DDRC = 0x0;
    
    //create table and initialize duty cycle
    int pwd[] = {5, 20, 36, 51, 67, 82, 97, 113, 128, 143, 159, 174, 189, 205, 220, 236, 251};
    step = 8;
    PORTD = step<<2;
    dc_value = pwd[step];
    OCR1AL = dc_value;
    OCR1AH = 0;
    
    //set up ADC-related registers
    ADMUX = (1<<REFS0)|(1<<ADLAR);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
    
    //wait for a mode to be selected
    while( ((PIND & 0x2) != 0) && ((PIND & 0x1) != 0) );
        
    while(1){
        
        //mode 1
        while((PIND & (1<<PD1)) != 0){ 
            
            PORTD = step<<2;
            //check for brightness increase(PB4)
            if((PINB & (1<<PB4)) == 0)
                if(step < 16){
                    dc_value = pwd[++step];
                    OCR1AL = dc_value;
                    _delay_ms(200);
                }

            //check for brightness decrease(PB3)
            if((PINB & (1<<PB3)) == 0)
                if(step > 0){
                    dc_value = pwd[--step];
                    OCR1AL = dc_value;
                    _delay_ms(200);
                }
        }
        
        //mode 2
        while((PIND & (1<<PD0)) != 0){
            
            //start conversion
            ADCSRA |= (1 << ADSC);
    
            //wait for conversion to complete
            while (ADCSRA & 0x40)
                adc_in = 0;
            adc_in = 0;

            //read n' write results
            adc_in = ADCH;
            OCR1AL = adc_in;
        }
        
        dc_value = pwd[step];
        OCR1AL = dc_value;
        
    }
        
    return 0;
}
