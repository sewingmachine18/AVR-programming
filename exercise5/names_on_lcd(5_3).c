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


//-------------main--------------------
int main(){
    
    //init the tools
    twi_init();
    PCA9555_0_write(REG_CONFIGURATION_0, 0x00);
    lcd_init();
    
    
    //first name
    lcd_data('N');
    lcd_data('E');
    lcd_data('L');
    lcd_data('L');
    lcd_data('Y');
    lcd_data(' ');
    lcd_data('T');
    lcd_data('S');
    lcd_data('I');
    lcd_data('M');
    lcd_data('P');
    lcd_data('L');
    lcd_data('A');
    lcd_data('K');
    lcd_data('I');
    
    //move to second line
    lcd_command(0xC0);
    
    //second name
    lcd_data('P');
    lcd_data('E');
    lcd_data('R');
    lcd_data('I');
    lcd_data('K');
    lcd_data('L');
    lcd_data('I');
    lcd_data('S');
    lcd_data(' ');
    lcd_data('A');
    lcd_data('L');
    lcd_data('E');
    lcd_data('X');
    lcd_data('I');
    lcd_data('O');
    lcd_data('U');
    
    return 0;
}
