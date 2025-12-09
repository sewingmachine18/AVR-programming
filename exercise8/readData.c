#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// <editor-fold desc="twi">
#define PCA9555_0_ADDRESS 0x40 //A0=A1=A2=0 by hardware
#define TWI_READ 1 // reading from twi device
#define TWI_WRITE 0 // writing to twi device

#define SCL_CLOCK 100000L // twi clock in Hz
//Fscl=Fcpu/(16+2*TWBR0_VALUE*PRESCALER_VALUE)
#define TWBR0_VALUE ((F_CPU/SCL_CLOCK)-16)/2
// PCA9555 REGISTERS


typedef enum {
	REG_INPUT_0 = 0,
	REG_INPUT_1 = 1,
	REG_OUTPUT_0 = 2,
	REG_OUTPUT_1 = 3,
	REG_POLARITY_INV_0 = 4,
	REG_POLARITY_INV_1 = 5,
	REG_CONFIGURATION_0 = 6,
	REG_CONFIGURATION_1 = 7
} PCA9555_REGISTERS;


//----------- Master Transmitter/Receiver -------------------
#define TW_START 0x08
#define TW_REP_START 0x10


//---------------- Master Transmitter ----------------------
#define TW_MT_SLA_ACK 0x18
#define TW_MT_SLA_NACK 0x20
#define TW_MT_DATA_ACK 0x28


//---------------- Master Receiver ----------------
#define TW_MR_SLA_ACK 0x40
#define TW_MR_SLA_NACK 0x48
#define TW_MR_DATA_NACK 0x58


//----------------keep only important bits of status---------------
#define TW_STATUS_MASK 0b11111000
#define TW_STATUS (TWSR0 & TW_STATUS_MASK)


//---------------twi functions------------------------------------------
//initialize TWI clock
void twi_init(void)
{
	TWSR0 = 0; // PRESCALER_VALUE=1
	TWBR0 = TWBR0_VALUE; // SCL_CLOCK 100KHz
}

// Read one byte from the twi device (request more data from device)
unsigned char twi_readAck(void)
{
	TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	while(!(TWCR0 & (1<<TWINT)));
	return TWDR0;
}

//Read one byte from the twi device, read is followed by a stop condition
unsigned char twi_readNak(void)
{
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR0 & (1<<TWINT)));
	return TWDR0;
}

// Issues a start condition and sends address and transfer direction.
// return 0 = device accessible, 1= failed to access device
unsigned char twi_start(unsigned char address)
{
	uint8_t twi_status;
	
	// send START condition
	TWCR0 = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	
	// wait until transmission completed
	while(!(TWCR0 & (1<<TWINT)));
	
	// check value of TWI Status Register.
	twi_status = TW_STATUS & 0xF8;
	if ( (twi_status != TW_START) && (twi_status != TW_REP_START)) return 1;
	
	// send device address
	TWDR0 = address;
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	
	// wail until transmission completed and ACK/NACK has been received
	while(!(TWCR0 & (1<<TWINT)));
	
	// check value of TWI Status Register.
	twi_status = TW_STATUS & 0xF8;
	if ( (twi_status != TW_MT_SLA_ACK) && (twi_status != TW_MR_SLA_ACK) )
	{
		return 1;
	}
	return 0;
}

// Send start condition, address, transfer direction.
// Use ack polling to wait until device is ready
void twi_start_wait(unsigned char address)
{
	uint8_t twi_status;
	while ( 1 )
	{
		// send START condition
		TWCR0 = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);

		// wait until transmission completed
		while(!(TWCR0 & (1<<TWINT)));

		// check value of TWI Status Register.
		twi_status = TW_STATUS & 0xF8;
		if ( (twi_status != TW_START) && (twi_status != TW_REP_START)) continue;

		// send device address
		TWDR0 = address;
		TWCR0 = (1<<TWINT) | (1<<TWEN);

		// wail until transmission completed
		while(!(TWCR0 & (1<<TWINT)));

		// check value of TWI Status Register.
		twi_status = TW_STATUS & 0xF8;
		if ( (twi_status == TW_MT_SLA_NACK )||(twi_status ==TW_MR_DATA_NACK) )
		{
			/* device busy, send stop condition to terminate write operation */
			TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);

			// wait until stop condition is executed and bus released
			while(TWCR0 & (1<<TWSTO));

			continue;
		}
		break;
	}
}

// Send one byte to twi device, Return 0 if write successful or 1 if write failed
unsigned char twi_write( unsigned char data )
{
	// send data to the previously addressed device
	TWDR0 = data;
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	
	// wait until transmission completed
	while(!(TWCR0 & (1<<TWINT)));
	if( (TW_STATUS & 0xF8) != TW_MT_DATA_ACK) return 1;
	return 0;
}

// Send repeated start condition, address, transfer direction
//Return: 0 device accessible
// 1 failed to access device
unsigned char twi_rep_start(unsigned char address)
{
	return twi_start( address );
}

// Terminates the data transfer and releases the twi bus
void twi_stop(void)
{
	// send stop condition
	TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	// wait until stop condition is executed and bus released
	while(TWCR0 & (1<<TWSTO));
}

void PCA9555_0_write(PCA9555_REGISTERS reg, uint8_t value)
{
	twi_start_wait(PCA9555_0_ADDRESS + TWI_WRITE);
	twi_write(reg);
	twi_write(value);
	twi_stop();
}

uint8_t PCA9555_0_read(PCA9555_REGISTERS reg)
{
	uint8_t ret_val;

	twi_start_wait(PCA9555_0_ADDRESS + TWI_WRITE);
	twi_write(reg);
	twi_rep_start(PCA9555_0_ADDRESS + TWI_READ);
	ret_val = twi_readNak();
	twi_stop();
	return ret_val;
}
// </editor-fold>


// <editor-fold desc="lcd">
//-----------lcd functions------------------------------
void write_nibbles(uint8_t number) {
    
    //send high nibble
    uint8_t temp = PCA9555_0_read(REG_OUTPUT_0), num = number;
    temp &= 0x0F;
    num &= 0xF0;
    num += temp;
    
    num |= (1<<PD3);
    PCA9555_0_write(REG_OUTPUT_0, num);
    _delay_us(1);
    num &= 0xF7;
    PCA9555_0_write(REG_OUTPUT_0, num);
    
    
    //send low nibble
    num = number;
    num <<= 4;  
    num &= 0xF0;
    num += temp;
    
    num |= (1<<PD3);
    PCA9555_0_write(REG_OUTPUT_0, num);
    _delay_us(1);
    num &= 0xF7;
    PCA9555_0_write(REG_OUTPUT_0, num);
    
}

void lcd_data(uint8_t number){
    uint8_t temp = PCA9555_0_read(REG_OUTPUT_0);
    temp |= (1<<PD2);
    PCA9555_0_write(REG_OUTPUT_0, temp);
    
    write_nibbles(number);
    
    _delay_us(250);
}

void lcd_command(uint8_t number){
    uint8_t temp = PCA9555_0_read(REG_OUTPUT_0);
    temp &= 0xFB;
    PCA9555_0_write(REG_OUTPUT_0, temp);
    
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
    PCA9555_0_write(REG_OUTPUT_0, 0x30);    
    uint8_t temp = PCA9555_0_read(REG_OUTPUT_0);
    
    temp |= (1<<PD3);
    PCA9555_0_write(REG_OUTPUT_0, temp);
    _delay_us(1);
    temp &= 0xF7;
    PCA9555_0_write(REG_OUTPUT_0, temp);
    
    _delay_us(250);
    
     //8 bit #2
    PCA9555_0_write(REG_OUTPUT_0, 0x30);    
    temp = PCA9555_0_read(REG_OUTPUT_0);
    
    temp |= (1<<PD3);
    PCA9555_0_write(REG_OUTPUT_0, temp);
    _delay_us(1);
    temp &= 0xF7;
    PCA9555_0_write(REG_OUTPUT_0, temp);
    
    _delay_us(250);
    
     //8 bit #3
    PCA9555_0_write(REG_OUTPUT_0, 0x30);    
    temp = PCA9555_0_read(REG_OUTPUT_0);
    
    temp |= (1<<PD3);
    PCA9555_0_write(REG_OUTPUT_0, temp);
    _delay_us(1);
    temp &= 0xF7;
    PCA9555_0_write(REG_OUTPUT_0, temp);
    
    _delay_us(250);
    
    //4 bit 
    PCA9555_0_write(REG_OUTPUT_0, 0x20);    
    temp = PCA9555_0_read(REG_OUTPUT_0);
    
    temp |= (1<<PD3);
    PCA9555_0_write(REG_OUTPUT_0, temp);
    _delay_us(1);
    temp &= 0xF7;
    PCA9555_0_write(REG_OUTPUT_0, temp);
    
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

void lcd_string(char *str){
    for(int i = 0; i < strlen(str); ++i)
        lcd_data(str[i]);   
}
// </editor-fold>


// <editor-fold desc="keypad">
//------------keypad functions------------------------
int scan_row(int line){
	
	PCA9555_0_write(REG_OUTPUT_1, line);
	int input = PCA9555_0_read(REG_INPUT_1);
	input = ~input;
	input &= 0xF0;
	input >>= 4;
	return input;
}

int scan_keypad(){
	int res;
	//read first line
	res = scan_row(7);
	if(res != 0)
		return res;
	//2nd line
	res = scan_row(11);
	if(res != 0){
		res <<= 4;
		return res;
	}
	//3rd line
	res = scan_row(13);
	if(res != 0){
		res <<= 8;
		return res;
	}
	//4th line
	res = scan_row(14);
	if(res != 0){
		res <<= 12;
		return res;
	}
	return 0;
}

int pressed_keys = 0, pressed_keys_temp, in, temp;

int scan_keypad_rising_edge(){
	pressed_keys_temp = scan_keypad();
	_delay_ms(20);
	in = scan_keypad();
	pressed_keys_temp &= in;
	temp = pressed_keys_temp & (~pressed_keys);
	pressed_keys = pressed_keys_temp;
	return temp;
}

char ascii[] = {0, '1', '2', '3', 'A', '4', '5', '6', 'B', '7', '8', '9', 'C', '*', '0', '#', 'D'};
	
char keypad_to_ascii(int key){
	int pos = 0;
	while(key != 0){
		++pos;
		key >>= 1;
	}
	return ascii[pos];
}
// </editor-fold>

// <editor-fold desc="one-wire">
//------------- one-wire functions ----------------------
uint8_t one_wire_reset(){
    uint8_t in;
    
    DDRD |= (1<<PD4);
    PORTD &= ~(1<<PD4);
    _delay_us(480);
    
    DDRD &= ~(1<<PD4);
    PORTD &= ~(1<<PD4);
    _delay_us(100);
    
    in = PIND;
    _delay_us(380);
    if((in&(1<<PD4)) == 0) return 1;
    else return 0; 
}

uint8_t one_wire_receive_bit(){
    uint8_t res;
    
    DDRD |= (1<<PD4);
    PORTD &= ~(1<<PD4);
    _delay_us(2);
    
    DDRD &= ~(1<<PD4);
    PORTD &= ~(1<<PD4);
    _delay_us(10);
    
    
    if(PIND&(1<<PD4)) res = 1;
    else res = 0;
    
    _delay_us(49);
    return res;
}

void one_wire_transmit_bit(uint8_t bit){

    DDRD |= (1<<PD4);
    PORTD &= ~(1<<PD4);
    _delay_us(2);
    
    if(bit == 1) PORTD |= (1<<PD4);
    else PORTD &= ~(1<<PD4);
    _delay_us(58);
    
    DDRD &= ~(1<<PD4);
    PORTD &= ~(1<<PD4);
    _delay_us(1);
    
    return;
}

uint8_t one_wire_receive_byte(){
    uint8_t res = 0, temp;
    
    for(int i = 0; i < 8; ++i){
        temp = one_wire_receive_bit();
        temp <<= i;
        res |= temp;
    }
    
    return res;
}

void one_wire_transmit_byte(uint8_t byte){
    uint8_t out;
    for(int i = 0; i < 8; ++i){   
        out = byte&1;
        one_wire_transmit_bit(out);
        byte >>= 1;
    }
    return;
}

uint16_t read_temp(){
    uint16_t low = 0, temp = 0;
    
    // 1st interaction to init the measurement
    if(one_wire_reset() == 0) return 0x8000; // if no device detected return -1
    one_wire_transmit_byte(0xCC); //broadcast as only one device is connected
    one_wire_transmit_byte(0x44); // start temperature measurement
    while(one_wire_receive_bit() == 0) ; //wait bit from sensor 
    
    // 2nd interaction to 
    if(one_wire_reset() == 0) return 0x8000;
    one_wire_transmit_byte(0xCC);
    one_wire_transmit_byte(0xBE); //tell the sensor to send data
    
    //
    low = one_wire_receive_byte();
    temp = one_wire_receive_byte();
    temp = (temp << 8) | low;
    return temp;
}
// </editor-fold>

// <editor-fold desc="USART">
//------------UART functions-----------------
void usart_init(unsigned int ubrr) {
    UCSR0A = 0;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UBRR0H = (unsigned char) (ubrr >> 8);
    UBRR0L = (unsigned char) ubrr;
    UCSR0C = (3 << UCSZ00);
    return;
}


void usart_transmit(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

uint8_t usart_receive() {
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

void usart_transmit_string(char *str){
    for(int i = 0; i < strlen(str); ++i){
        usart_transmit(str[i]);
    }
}

void usart_receive_string(char *str, int N){
    int i = 0;
    while(i < N-1){
        str[i] = usart_receive();
        if(str[i] == '\n'){
            ++i;
            break;
        }
        ++i;
    }
    str[i] = '\0';
}
// </editor-fold>

// <editor-fold desc="global variables">
//-----------global variables-------------
#define MAX_LEN 64
#define TEMP_OFFSET 12
#define DIGIT '3'
const int step = 49;
char cmd[MAX_LEN], data[MAX_LEN];
char pay[4*MAX_LEN], screen[MAX_LEN];
int temp_int = 0, temp_frac = 0, pressure = 0, adc_in, key;
char *status[] = {"OK", "CHECK PRESSURE", "CHECK TEMP", "NURSE CALL"};
enum stat {OK, CHECK_PRESSURE, CHECK_TEMP, NURSE_CALL};
int status_no, nurse_flag = 0;
// </editor-fold>

// <editor-fold desc="local-use">
//-----------local-use functions-------------------
void calc_temp(){
    int16_t input = read_temp(), integer, frac;
    int sign = 1; 
    int int1 = -1, int2 = -1, int3 = -1; 
    int frac1 = -1; 
                
    if((input&0x8000) == 0x8000){
        sign = -1;
        input = ~input + 1;
    }
    integer = input>>4;
    frac = input&0xF;
    frac *= 1000;
    frac >>= 4;

    //start bcd for integer part
    while(integer >= 0){ //get hundreds
        integer -= 100;
        ++int1;
    }
    integer += 100;

    while(integer >= 0){ //get decades
        integer -= 10;
        ++int2;
    }
    integer += 10;

    while(integer >= 0){ //get units
        integer -= 1;
        ++int3;
    }
    integer += 1;

    //start bcd for fractional part
    while(frac >= 0){ //get hundreds
        frac -= 100;
        ++frac1;
    }
    frac += 100;
    
    temp_int = sign*(int1*100 + int2*10 + int3*1 + TEMP_OFFSET);
    temp_frac = frac1;
}

void find_status(){
    if(nurse_flag == 1){
        status_no = NURSE_CALL;
        return;
    }  
    if(pressure > 12 || pressure < 4){
        status_no = CHECK_PRESSURE;
        return;
    }
    if(temp_int > 37 || temp_int < 34){
        status_no = CHECK_TEMP;
        return;
    }
    status_no = OK;
    return;
}

void payload_config(){
    sprintf(pay, 
"ESP:payload:[{\"name\": \"temperature\", \"value\": \"%d.%d\"}, {\"name\": \"pressure\", \"value\": \"%d\"}, {\"name\": \"team\", \"value\": \"3\"}, {\"name\": \"status\", \"value\": \"%s\"}]\n",
            temp_int, temp_frac, pressure, status[status_no]);
}

void lcd_show_stats(){
    lcd_clear_display();
    sprintf(screen, "T: %d.%d, P: %d", temp_int, temp_frac, pressure);
    lcd_string(screen);
    lcd_command(0xC0);
    sprintf(screen, "Status: %s", status[status_no]);
    lcd_string(screen); 
}
// </editor-fold>

//---------- main -----------------
int main(){
 
    //initialize all essential tools
    twi_init();
    PCA9555_0_write(REG_CONFIGURATION_0, 0x00);
    PCA9555_0_write(REG_CONFIGURATION_1, 0xF0);
    lcd_init();
    usart_init(103);
    ADMUX = (1<<REFS0);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
    
    //form connection
    strcpy(cmd, "ESP:connect\n");
    usart_transmit_string(cmd);
    usart_receive_string(data, MAX_LEN);
    if(!strcmp(data, "\"Success\"\n"))
        lcd_string("1.Success");
    else{
        lcd_string("1.Fail");
        goto out;
    }
    _delay_ms(1000);
    //send url
    strcpy(cmd, "ESP:url:\"http://192.168.1.250:5000/data\"\n");
    usart_transmit_string(cmd);
    usart_receive_string(data, MAX_LEN);
    lcd_clear_display();
    if(!strcmp(data, "\"Success\"\n"))
        lcd_string("2.Success");
    else{
        lcd_string("2.Fail");
        goto out;
    }
    _delay_ms(1000);
    
    while(1){
        //read temperature
        calc_temp();
        //read pressure
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & 0x40) ;
        adc_in = ADC; pressure = 0;
        while(adc_in >= step){
            adc_in -= step;
            ++pressure;
        }
        // 1) check for nurse call
        key = scan_keypad_rising_edge();
        if(keypad_to_ascii(key) == DIGIT)
            nurse_flag = 1;
        else if(keypad_to_ascii(key) == '#')
            nurse_flag = 0;
        
        //check for temp or pressure irregularities and update status on payload
        find_status();
        payload_config();
        //show on screen
        lcd_show_stats();
        _delay_ms(1000);
    }
    
    out:
    key = 5;
    return 0;
}
