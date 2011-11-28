/* Name: main.c
 * Author: <insert your name here>
 * Copyright: <insert your copyright message here>
 * License: <insert your license reference here>
 */

#include <avr/io.h>

#include "lib_rdm_in.h"

#define LED1_COMPARE         OCR0B
#define LED2_COMPARE         OCR1BL
#define LED3_COMPARE         OCR1AL

int main(void)
{
    cli();
    
    //8 bit phase corrected PWM
    TCCR0A = (1<<COM0A1) | (1<<WGM00 );
    TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<WGM10 );
    TCCR0B = (1<<CS00);
    TCCR1B = (1<<CS10);
    
    //PWM Ports are outputs
    DDRB |= (1<<DDB2) | (1<<DDB3) | (1<<DDB4);
    
    init_RDM();
    Flags= 0;
    sei();
    for(;;)
	{
        if (Flags &(1<<EVAL_DMX))		//universe complete?
		{
            Flags &= ~(1<<EVAL_DMX);	//clear flag
            
            LED1_COMPARE = DmxField[0];
            LED2_COMPARE = DmxField[1];
            LED3_COMPARE = DmxField[2];
		}
        check_rdm();			//check for new RDM packets
	}
}

