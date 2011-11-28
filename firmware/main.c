/* Name: main.c
 * Author: <insert your name here>
 * Copyright: <insert your copyright message here>
 * License: <insert your license reference here>
 */

#include <avr/io.h>

#include "lib_rdm_in.h"

int main(void)
{
    cli();
    DDRA  = 0xFF;
    PORTA = 0;
    DDRD |= (1<<PD7);					//red LED pin is output
    DDRE |= (1<<PE0);					//green LED is output
    DDRC=  0;							//use DIPs for device addressing
    PORTC= 0xFF;
    init_RDM();
    Flags= 0;
    sei();
    for(;;)
	{
        if (Flags &(1<<EVAL_DMX))		//universe complete?
		{
            Flags &= ~(1<<EVAL_DMX);	//clear flag
            if (DmxField[0] >= 127)		//enable LED if 1st DMX val is >127
			{
           //     PORTD &= ~(1<<PD7);		//LED ON
			}
            else
			{
            //    PORTD |= (1<<PD7);		//LED OFF
			}
		}
        check_rdm();			//check for new RDM packets
	}
}

