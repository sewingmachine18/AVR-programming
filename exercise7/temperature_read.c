#define F_CPU 16000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>

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


//---------- main -----------------
int main(){
    
    twi_init();
    PCA9555_0_write(REG_CONFIGURATION_0, 0x00);
    lcd_init();
    int16_t input, store = 0x7FFF;
    int16_t integer, frac;
    int sign, int1, int2, int3, frac1, frac2, frac3;
    
    while(1){
        input = read_temp();
        if(store != input){
            store = input;
            lcd_clear_display();
       
            if(input == 0x8000){ // if no device connected, show "no device"
                lcd_data('N');
                lcd_data('o');
                lcd_data(' ');
                lcd_data('D');
                lcd_data('e');
                lcd_data('v');
                lcd_data('i');
                lcd_data('c');
                lcd_data('e');
            }
            else{ // else find the temperature
                sign = 0; 
                int1 = -1; int2 = -1; int3 = -1; 
                frac1 = -1; frac2 = -1; frac3 = -1;
                
                if((input&0x8000) == 0x8000){
                    sign = 1;
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

                while(frac >= 0){ //get decades
                    frac -= 10;
                    ++frac2;
                }
                frac += 10;

                while(frac >= 0){ //get units
                    frac -= 1;
                    ++frac3;
                }
                frac += 1;
                
                // show on lcd
                if(sign == 0) lcd_data('+');
                else lcd_data('-');
                if(int1 > 0) lcd_data('0'+int1);
                if(int1 > 0 || int2 > 0) lcd_data('0'+int2);
                lcd_data('0'+int3);
                lcd_data('.');
                lcd_data('0'+frac1);
                lcd_data('0'+frac2);
                lcd_data('0'+frac3);    
                lcd_data(0xDF);
                lcd_data('C');
            }
        }
    }
   
    return 0;
}
