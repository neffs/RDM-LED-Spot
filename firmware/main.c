/*
 * Led_Spot.c
 *
 * Created: 14.12.2011 21:16:02
 *  Author: markus
 */ 

#include <avr/io.h>
#include "lib_rdm_in.h"

#define LED1_COMPARE         OCR0A
#define LED2_COMPARE         OCR1BL
#define LED3_COMPARE         OCR1AL

uint16_t Device_Power;
uint32_t Device_Timecounter_sec;
uint32_t Device_Timecounter_hour;
uint32_t Lamp_Timecounter_sec;
uint32_t Lamp_Timecounter_hour;
uint16_t cnt;

int main(void)
{
cli();
    //8 bit phase corrected PWM
    TCCR0A = (1<<COM0A1) | (1<<WGM00 );
    TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<WGM10 );
    TCCR0B = (1<<CS01);
    TCCR1B = (1<<CS11);
	TIMSK = (1<<TOIE0);
    
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
             LED1_COMPARE = DmxField[1];	//green
             LED2_COMPARE = DmxField[2];	//blue
             LED3_COMPARE = DmxField[0];	//red
 			Device_Power=(DmxField[0]+DmxField[1]+DmxField[2]);
 		}
         check_rdm();			//check for new RDM packets
 	}
	
}


 ISR(TIMER0_OVF_vect)
 {
//     	cnt++;
//      	if(cnt==7547)
//    		{
//   			Device_Timecounter_sec++;
//   			if(Device_Power>=20)Lamp_Timecounter_sec++;
   			
//  			if(Lamp_Timecounter_sec>=60) Lamp_Timecounter_min++;
//  			if(Lamp_Timecounter_min>=60)
//  				{
//  					 Lamp_Timecounter_hour++;
//  					 eeprom_write_dword((uint16_t*)37,Lamp_Timecounter_hour);
//  				}				
//     			if(Device_Timecounter_sec==3600) 
//   			  {
//   				Device_Timecounter_hour++;
//   				Device_Timecounter_sec=0;
// 				eeprom_write_dword((uint16_t*)41,Device_Timecounter_hour);  
//   			  }				
//     			if(Device_Timecounter_min>=60)
//    				{ 
//     					Device_Timecounter_hour++;
//   					Device_Timecounter_min=0;
//     				//	eeprom_write_dword((uint16_t*)41,Device_Timecounter_hour);   				
//  				} 			
//		cnt=0;				
//   		}
}
