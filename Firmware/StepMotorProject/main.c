/*
 * StepMotorProject.c
 * Test case for HARDWARE ENGINEER position
 * in SILEGO TECHNOLOGY INC. 
 * Created: 27.01.2017 11:26:13
 * Author : Mazinov
 */ 
#define F_CPU 8000000UL
// Подключение биполярного шагового двигателя к AVR
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "lcd_lib.h"
#define SIZE 4
#define DEBOUNCE 199
#define RPM_STEP 100
#define MIN_SPEED 1100
#define MAX_SPEED 0
#define ADC_MAX_RANGE 1023
#define SPEED_UP (1 << PIND0)
#define SPEED_DOWN (1 << PIND1)
#define START (1 << PIND4)
#define MODE (1 << PIND5)


// Full step mode with 2 phase;
// PortB bitmask for clockwise direction;
unsigned char cw_dir[SIZE]=
{
	0b00000010,
	0b00000100,
	0b00001000,
	0b00010000
};

// PortB bitmask for anti-clockwise direction;
unsigned char ccw_dir[SIZE]=
{
	0b00010000,
	0b00001000,
	0b00000100,
	0b00000010
};

// Global variables;
volatile unsigned char step_index;
volatile unsigned int motor_speed = MAX_SPEED;
volatile unsigned int speed_rpm = MIN_SPEED;
volatile unsigned char clocwise_flag, anti_clockwise_flag;
volatile unsigned char timer = 0;

// Microcontroller initialisation;
void mcu_init();
// Motor speed adjustment using potentiometer;
unsigned int adc_read(); 
// Display data function for analogue and step motor speed adjustment mode;
void display_rpm_data(int rpm);


ISR(TIMER0_OVF_vect)
{
	static unsigned int count = 1;
	
	count++;
	timer++;

	if( !(PIND &START) && (count >= speed_rpm) && (timer == DEBOUNCE)) // Delay for RPM;
	{
		cli(); // Forbit interrupts;
		
		if((clocwise_flag) && (anti_clockwise_flag==0)) 
		{
			PORTC = ccw_dir[step_index++];
		} 
			else if ((anti_clockwise_flag) && (clocwise_flag==0))
			{
				PORTC = cw_dir[step_index++];
			}

				else
				{
					PORTC = 0x00;
				}

		if(PIND &START)
		{
			PORTC = 0x00;
			clocwise_flag = 0;
			anti_clockwise_flag = 0;
		}

		if (step_index >= SIZE)
		{
			step_index=0;
		}
		if (timer > DEBOUNCE)
		{
			timer = 0;
		}
		count = 0; // Reset counter;
		TCNT0 = 0; // Reset timer 0 counter register;
		sei();	   // Allow global interrupts;
	}
	
}

// External interrupt 0, used for direction button;
ISR(INT0_vect)
{
	clocwise_flag = 1;
	anti_clockwise_flag = 0;

		if(PIND &START)
		{
			clocwise_flag = 0;
		}

}

// External interrupt 1, used for direction button;
ISR(INT1_vect)
{
	anti_clockwise_flag = 1;
	clocwise_flag = 0;

		if(PIND &START)
		{
			anti_clockwise_flag = 0;
		}
	
}

int main(void)
{
	mcu_init();
	lcd_init();
	lcd_clr();
	PORTB = 0x00;
	sei(); // Enable global interrupts;

	while(1)
	{

		
			// Potentiometer speed adjustment mode;
			if (!(PIND &MODE))
			{
				display_rpm_data(adc_read(speed_rpm));
			}
			
			// Step speed adjustment mode;
			else 
			{
							
				display_rpm_data(motor_speed);

				if ((!(PIND  &SPEED_UP)) && (speed_rpm != MAX_SPEED) 
					&& (motor_speed != MIN_SPEED)) // && (timer == DEBOUNCE))
					
				{
					speed_rpm -=RPM_STEP;
					motor_speed +=RPM_STEP;
					display_rpm_data(motor_speed);
				}
			
					else if ((!(PIND  &SPEED_DOWN)) && (speed_rpm <= MIN_SPEED)	
							&& (motor_speed != MAX_SPEED)) // && (timer == DEBOUNCE))
							
					{
						speed_rpm +=RPM_STEP;
						motor_speed -=RPM_STEP;
						display_rpm_data(motor_speed);
					}

						else
						{
							speed_rpm = MIN_SPEED;
						}
			}
		}
	}


unsigned int adc_read()
{
	unsigned int adc_rpm = ADC_MAX_RANGE;

	ADCSRA |= (1 << ADSC); // Start converting;
	while (ADCSRA & (1 << ADSC)); // Wait for stop converting;
	speed_rpm = ADCW; // Measurment result;
	adc_rpm = adc_rpm - speed_rpm; // Data reverse for diplay;
	return adc_rpm;
}

void display_rpm_data(int rpm)
{
	lcd_gotoxy(0,0);
	lcd_string(" Stepping motor", 15);
	lcd_gotoxy(0,1);
	lcd_string("Speed", 5);
	lcd_gotoxy(7,1);
	lcd_num_to_str(rpm, 4);
	lcd_gotoxy(12,1);
	lcd_string("RPM", 5);
}


void mcu_init()
{
	// PB0, PB1, PB2, PB3 - outs;
	// With zero outputs level; 
	DDRC = (0 << DDC6) | (0 << DDC5) | (1 << DDC4) | (1 << DDC3) | (1 << DDC2) | (1 << DDC1) | (0 << DDC0);
	PORTC= (0 << PINC6) | (0 << PINC5) | (0 << PINC4) | (0 << PINC3) | (0 << PINC2) | (0 << PINC1) | (1 << PINC0);
	
	DDRD = (0 << DDD7) | (0 << DDD6) | (0 << DDD5) | (0 << DDD4) | (0 << DDD3) | (0 << DDD2) | (0 << DDD1) | (0 << DDD0);
	PORTD = (0 << PIND7) | (0 << PIND6) | (1 << PIND5) | (1 << PIND4) | (0 << PIND3) | (0 << PIND2) | (1 << PIND1) | (1 << PIND0);

	// Enable ADC;
	// Prescaler 64 (ADC frequency 125kHz);
	// ADC0 - measurment input with external 5V REF;
	ADCSRA = (1 << ADEN) | (1 << ADPS2)	| (1 << ADPS1);
	ADMUX = 0x00;

	// External Interrupt(s) initialization
	// INT0: On
	// INT0 Mode: Falling Edge
	// INT1: On
	// INT1 Mode: Falling Edge
	EICRA=(1<<ISC11) | (0<<ISC10) | (1<<ISC01) | (0<<ISC00);
	EIMSK=(1<<INT1) | (1<<INT0);
	EIFR=(1<<INTF1) | (1<<INTF0);
	PCICR=(0<<PCIE2) | (0<<PCIE1) | (0<<PCIE0);

	// Prescaler / 8
	// Pverflow interrupt enable;

	TCCR0B |= (1 << CS01);
	TCNT0 = 0x00;
	TIMSK0 |= (1 << TOIE0);

}