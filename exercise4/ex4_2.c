#include <stdio.h> 
#include <stdlib.h>
#define F_CPU 16000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>

int input;

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

int main(int argc, char** argv) {
//setup ports
    DDRD = 0xFF;
    DDRC = 0x00;
    
//ADC setup
    ADMUX = (1<<REFS0)|(1<<MUX0)|(1<<MUX1);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
    
    while(1){
        ADCSRA |= (1<<ADSC);
        while(ADCSRA & (1<<ADSC));
        input = ADC;
        input *= 125;
        input >>= 8;
        
        lcd_clear_display();
        
        //hundreds
        int digit = -1;
        while(input >= 0){
            input -= 100;
            ++digit;
        }
        input += 100;
        
        //display hundreds
        lcd_data(digit + '0');
        
        //display '.'
        lcd_data(0x2E);
        
        //decades
        digit = -1;
        while(input >= 0){
            input -= 10;
            ++digit;
        }
        input += 10;
        
        //display decades
        lcd_data(digit + '0');
        
        //units
        digit = -1;
        while(input >= 0){
            input -= 1;
            ++digit;
        }
        input += 1;
        
        //display units
        lcd_data(digit + '0'); 
        
        _delay_ms(1000);
        
    }
    
    return (EXIT_SUCCESS);
}



