#include <stdio.h> 
#include <stdlib.h>
#define F_CPU 16000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>

volatile int local_input = 0, input = 0, leds = 0, counter = 0, flag = 0;

void write_nibbles(uint8_t number) {
    
    //send high nibble
    uint8_t temp = PIND, num = number;
    temp &= 0x0F;
    num &= 0xF0;
    num += temp;
    
    PORTD = num;
    
    PORTD |= (1<<PD3);
    _delay_us(1);
    PORTD &= 0xF7;
    
    //send low nibble
    num = number;
    num <<= 4;  
    num &= 0xF0;
    num += temp;
    
    PORTD = num;
    
    PORTD |= (1<<PD3);
    _delay_us(1);
    PORTD &= 0xF7;
    
}

void lcd_data(uint8_t number){
    PORTD |= (1<<PD2);
    
    write_nibbles(number);
    
    _delay_us(250);
}

void lcd_command(uint8_t number){
    PORTD &= 0xFB;
    
    write_nibbles(number);
    
    _delay_us(250);
}

void lcd_clear_display() {
    lcd_command(1);
    _delay_ms(5);

}

void lcd_init() {
    //wait for powering of screen
    _delay_ms(200);
    
    //8 bit #1
    PORTD = 0x30;
    
    PORTD |= (1<<PD3);
    _delay_us(1);
    PORTD &= 0xF7;
    
    _delay_us(250);
    
     //8 bit #2
    PORTD = 0x30;
    
    PORTD |= (1<<PD3);
    _delay_us(1);
    PORTD &= 0xF7;
    
    _delay_us(250);
    
     //8 bit #3
    PORTD = 0x30;
    
    PORTD |= (1<<PD3);
    _delay_us(1);
    PORTD &= 0xF7;
    
    _delay_us(250);
    
    //4 bit 
    PORTD = 0x20;
    
    PORTD |= (1<<PD3);
    _delay_us(1);
    PORTD &= 0xF7;
    
    _delay_us(250);
    
    //5*8 dots, 2 lines
    lcd_command(0x28);
    
    //display on, cursor off
    lcd_command(0x0c);
    
    //clear display
    lcd_clear_display();
    
    //increase address, no display shift
    lcd_command(0x06);
    
    _delay_ms(100);
}

ISR(ADC_vect){
    local_input = ADC;
    counter = 2;
    
    if(local_input <= 170) leds = 0x01;
    else if(local_input <=340) leds = 0x3;
    else if(local_input <= 510) leds = 0x7;
    else if(local_input <= 680) leds = 0xF;
    else if(local_input <= 850) leds = 0x1F;
    else leds = 0x3F;    
}




int main(int argc, char** argv) {
//setup ports
    DDRD = 0xFF;
    DDRC = 0x00;
    DDRB = 0xFF;
    
//ADC setup
    ADMUX = (1<<REFS0)|(1<<MUX0)|(1<<MUX1);
    ADCSRA = (1<<ADEN)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
    
    lcd_init();
    
    sei();
    
    while(1){
        while(counter>0){
            _delay_ms(50);
            --counter;
            if( (flag == 1) && (counter%2 == 1)){
               PORTB = 0;
            }
            else PORTB = leds;
        }
        if(!(ADCSRA & (1<<ADSC))) 
                ADCSRA |= (1<<ADSC);
        
        cli();
        input = local_input;
        sei();
        
        if(input >= 403 && flag == 0){
            lcd_clear_display();
            flag = 1;
            lcd_data(0x47); //G
            lcd_data(0x41); //A
            lcd_data(0x53); //S
            lcd_data(0x20); //' '
            lcd_data(0x44); //D
            lcd_data(0x45); //E
            lcd_data(0x54); //T
            lcd_data(0x45); //E
            lcd_data(0x43); //C
            lcd_data(0x54); //T
            lcd_data(0x45); //E
            lcd_data(0x44); //D
        }
        else if(input < 403 && flag == 1){
            flag = 0;
            lcd_clear_display();
            lcd_data(0x43); //C
            lcd_data(0x4C); //L
            lcd_data(0x45); //E
            lcd_data(0x41); //A
            lcd_data(0x52); //R
        }
        
    }
    
    return (EXIT_SUCCESS);
}


