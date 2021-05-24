/*	Author: Dylan Riffel
 *  	Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab 10 Exercise 1
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif



volatile unsigned char TimerFlag = 0;
unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;


void TimerOn(){
TCCR1B = 0x0B;
OCR1A = 125;
TIMSK1 = 0x02;
TCNT1 = 0;
_avr_timer_cntcurr = _avr_timer_M;
SREG |= 0x80;

}	

void TimerOff(){
TCCR1B = 0x00;
}	
void TimerISR(){
TimerFlag = 1;
}	
ISR(TIMER1_COMPA_vect){

    _avr_timer_cntcurr--;
    if (_avr_timer_cntcurr == 0){
	TimerISR();
	_avr_timer_cntcurr = _avr_timer_M;
    
    }	    
}



void TimerSet(unsigned long M){
       _avr_timer_M = M;
       _avr_timer_cntcurr = _avr_timer_M;
}


unsigned char GetBit(unsigned char port, unsigned char number) 
{
	return ( port & (0x01 << number) );
}

unsigned char GetKeypadKey(){
	PORTC = 0xEF;
	asm("nop");
	if (GetBit(PINC,0) == 0) { return('1'); }
	if (GetBit(PINC,1) == 0) { return('4'); }
	if (GetBit(PINC,2) == 0) { return('7'); }
	if (GetBit(PINC,3) == 0) { return('*'); }

	asm("nop");
	if (GetBit(PINC,0) == 0) { return('2'); }
	
	PORTC = 0xBF;
	asm("nop");

	return ('\0');
}

typedef struct task {

	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	int (*TickFct)(int);


} task;

unsigned char led0_output = 0x00;
unsigned char led1_output = 0x00;
unsigned char pause = 0;

enum pauseButtonSM_States { pauseButton_wait, pauseButton_press, pauseButton_release };

int pauseButtonSMTick(int state){

	unsigned char press = ~PINA & 0x01;

	switch(state){
		case pauseButton_wait:
		state = press = 0x01? pauseButton_press: pauseButton_wait; break;
		case pauseButton_press:
		state = pauseButton_release; break;

		case pauseButton_release:
		state = press == 0x00? pauseButton_wait: pauseButton_press;break;
		default: state = pauseButton_wait;break;
		}
	switch(state){
		case pauseButton_wait: break;
		case pauseButton_press:
		pause = (pause == 0) ? 1:0;
		break;

		case pauseButton_release: break;
		}
		return state;

	}

enum toggleLED0_States { toggleLED0_wait, toggleLED0_blink };

int toggleLED0SMTick (int state ) {
	switch (state){
		case toggleLED0_wait: state = !pause? toggleLED0_blink: toggleLED0_wait; break;
		case toggleLED0_blink: state = pause? toggleLED0_wait: toggleLED0_blink; break;
		default: state = toggleLED0_wait; break;
		}
	switch(state){
		case toggleLED0_wait: break;
		case toggleLED0_blink:
			led0_output = (led0_output == 0x00) ? 0x01 : 0x00;
			break;

	}
	return state;
}


enum toggleLED1_States { toggleLED1_wait, toggleLED1_blink} ;

int toggleLED1SMTick(int state){
	switch(state){
		case toggleLED1_wait: state = !pause? toggleLED1_blink: toggleLED1_wait; break;
		case toggleLED1_blink: state = pause? toggleLED1_wait: toggleLED1_blink; break;
		default: state = toggleLED1_wait; break;

	}
	switch(state){
		case toggleLED1_wait: break;
		case toggleLED1_blink: 
			led1_output = (led1_output == 0x00) ? 0x01 : 0x00;
			break;



	}	
	return state;
}

enum display_States { display_display };

int displaySMTick(int state){

	unsigned char output;

	switch(state) {
		case display_display: state = display_display; break;
		default: state = display_display; break;
	}
	switch(state){
		case display_display:
			output = led0_output | led1_output << 1;

			break;

	}
	PORTB = output;
	return state;
}

enum KP_STATES { KP_Wait, KP_Press};

int KeyPress(int state){
switch(state){
	case KP_Wait:

	if ( 255 - PINC > 0) {
	KP_STATE = KP_Press;
	}
	else {
	KP_STATE = KP_Wait;
	}
	break;

	case KP_Press:

	
        if ( 255 - PINC > 0) {
        KP_STATE = KP_Press;
        }
        else {
        KP_STATE = KP_Wait;
        }
        break;
}
switch(state){
	case KP_Wait:
	PORTB = 0;
	break;
	case KP_Press:
	PORTB = 0b10000000;	
	break;
}
return state;
}


int main(void) {
    /* Insert DDR and PORT initializations */
    DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0xFF; PORTB = 0x00;
    unsigned int i;


    static task task1,task2,task3,task4;
    task *tasks[] = { &task1, &task2,&task3,&task4 };
    const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
    const char start = -1;
     
    unsigned long int findGCD(unsigned long int a, unsigned long int b){

	unsigned long int c;

	while(1){
        c = a&b;
        if (c == 0){ return b;}
        a = b;
        b = c;

	}
	return 0;

	}

	unsigned long GCD = tasks[0]->period;

	for(i = 1; i < numTasks; i++ ){
        	GCD = findGCD(GCD,tasks[i]->period);
	}


    task1.state = start;
    task1.period = 50;
    task1.elapsedTime = task1.period;
    task1.TickFct = &KeyPress;
	
    task2.state = start;
    task2.period = 500;
    task2.elapsedTime = task2.period;
    task2.TickFct = &toggleLED0SMTick;

    task3.state = start;
    task3.period = 1000;
    task3.elapsedTime = task3.period;
    task3.TickFct = &toggleLED1SMTick;


    task4.state = start;
    task4.period = 10;
    task4.elapsedTime = task4.period;
    task1.TickFct = &displaySMTick;




    TimerSet(GCD);
    TimerOn();


    /* Insert your solution below */
    while (1) {
	for ( i = 0; i < numTasks; i++){
		if(tasks[i]->elapsedTime == tasks[i]->period){
			tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
			tasks[i]->elapsedTime = 0;
		}
		tasks[i]->elapsedTime += GCD ;
	}


    while(!TimerFlag);
    TimerFlag = 0;
    }
    return 0;

}
