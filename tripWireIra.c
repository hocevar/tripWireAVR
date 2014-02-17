//NOT TESTED VERSION - added buttons =)
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>

/*
	Global variables
		- portIR = location of IR led
		- laserValue = value of ADC with laser
		- normalValue = value of ADC without laser
		- treshold = value of trashold for tigering
*/
/*
	Modes:   [PB1, PB0] - binary
	  
		- mode 0 -> one shot
		- mode 1 -> m1shots(5) shots
		- mode 2 -> m2time seconds
		- mode 3 -> read laserValue
*/
/*
	Connections:
		- PB0 & PB1 -> mode switch
		- PB4 -> IR led
		- PB3 -> ADC input

*/
#define PORTIR DDB4
#define m1shots 100
#define m2time 2

int mode = 0;
int laserValue = 40;
int normalValue = 170;
volatile int threshold = 100;
volatile int latestValue = 0;
int setup = 0;


void SendIRPulse(int time){
	/* Send a 38kHz modulated IR pulse */
	time = time/25;
	while(time){
		PORTB |= 1 << PORTIR; //ON
		_delay_us(13);
		PORTB &= ~(1 << PORTIR); //OFF
		_delay_us(13);
		time--;
	}
}
void takePhoto(){
	//Send ir pulse for nikon
	//mode 0 -> one shot
	SendIRPulse(2000);	//2250us of modulated IR
	_delay_us(27830);	//delay 27600 us
	SendIRPulse(400);	//650us of modulated IR
	_delay_us(1500);	//delay 1375 us
	SendIRPulse(400);	//575us of modulated IR
	_delay_us(3500);	//delay 3350 us
	SendIRPulse(400);	//650us of modulated IR
	
	_delay_ms(63); //oz. 63
	
	SendIRPulse(2000);	//2250us of modulated IR
	_delay_us(27830);	//delay 27600 us
	SendIRPulse(400);	//650us of modulated IR
	_delay_us(1500);	//delay 1375 us
	SendIRPulse(400);	//575us of modulated IR
	_delay_us(3500);	//delay 3350 us
	SendIRPulse(400);	//650us of modulated IR
}
void takePhotoM1(){
	//mode 1 -> send m1shots times
	int i = 0;
	for(i = 0; i < m1shots; i++){
		takePhoto();
	}
}
void takePhotoM2(){
	//TODO - send signal for m2time seconds
}
void calculateThreshold(){
	//calculate value for threshold from laser and normal Value
	threshold = laserValue+30;	
}
void checkbuttons(){
	//check mode buttons/switch
	
	int b0 = 0;
	int b1 = 0;
	
	if (bit_is_clear(PINB, DDB1)) b1 = 1;
	if (bit_is_clear(PINB, DDB0)) b0 = 1;
	
	mode = (b1 << 1) | b0;
	
	if(mode == 3){
		//READ value
		laserValue = latestValue;
		calculateThreshold();
	}
}

int main(void){
	/* Set up Ports
		INPUT:
			- PB0 & PB1 - mode chooser
			- PB3 - LDR, adc branje, senzor za laser
		OUTPUT:
			- PB4 - IR led dioda
	*/
	//Set output
	DDRB |= 1<<PORTIR; 
	//DDRB |= 1<<DDB2; //TMP
	//Set input
	DDRB &= ~(1<<DDB1);
	DDRB &= ~(1<<DDB0);
 
	// Set up Port B data to be all low
	PORTB = 0; 			//all output low
	PORTB |= 1<<DDB1;
	PORTB |= 1<<DDB0;
	
	//Setup ADC converter
	ADMUX |= 1<<ADLAR;	//Left Adjust Result -> reading 8 bits mode
	//Set ADC3 (PB3) as input for ADC
	ADMUX |= 1<<MUX0;	
	ADMUX |= 1<<MUX1;	
	//ADC prescaler bits ADPS2 & ADPS1 == 1 -> division factor = 64
	

	ADCSRA |= 1<<ADPS0;
	ADCSRA |= 1<<ADPS2;
	
	ADCSRA |= 1<<ADATE; 	//ADC auto trigger enable (ADCSRB ADTS[2:0] = 0 -> free running mode)
	ADCSRA |= 1<<ADIE;	//ADC Interrupt Enable
	ADCSRA |= 1<<ADEN;	//ADC Enable
	
	
	sei(); //enable interruptions
	
	ADCSRA |= 1<<ADSC; //start conversion
	
	//Check led connection (short on-off)
	PORTB |= 1 << PORTIR; //ON
	PORTB |= 1 << DDB2; //ON
	
	_delay_ms(2000);
	PORTB &= ~(1 << PORTIR); //OFF
	PORTB &= ~(1 << DDB2); //OFF
	
	while (1){	
		checkbuttons();
	}
	
	return 0;
}

ISR(ADC_vect){
	//ADCH -> value of adc conversion
	if(ADCH > threshold){
		if(mode == 0) takePhoto();
		else if(mode == 1) takePhotoM1();
		else if(mode == 2) takePhotoM2();
	}
	else{
		checkbuttons();
	}
	latestValue = ADCH;
	ADCSRA |= 1<<ADSC;
}
