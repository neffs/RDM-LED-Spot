/**** A P P L I C A T I O N   N O T E   ************************************
*
* Title			: Hitodama Main 
* Version		: v0.1
* Last updated	: 19.01.2012
* Target		: Hitdodama V0.1 Attiny4313

* Clock			: 8MHz, 16MHz
*
* written by Markus Bechtold Markus@r-bechtold.de
**************************************************************************
*    Copyright (C) 2012   Markus Bechtold
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
;***************************************************************************/

#include <avr/io.h>
#include "lib_rdm_in.h"
#include "lib_pdm.h"
#include "farbkreis.h"



#define LED1_COMPARE         OCR0A
#define LED2_COMPARE         OCR1BL
#define LED3_COMPARE         OCR1AL

uint8_t Device_Power;
uint16_t Device_Timecounter_sec;
uint32_t Device_Timecounter_hour;
uint32_t Lamp_Timecounter_hour;
uint16_t Lamp_Timecounter_sec;
uint8_t r;
uint8_t g;
uint8_t	b;
uint8_t RDM_Personality=1;

uint16_t cnt;

int main(void)
{
cli();
	
	TCCR0B = (1<<CS01); //sys timer init
	TIMSK = (1<<TOIE0);
	
	
    //PWM Ports are outputs

	Device_Timecounter_hour = eeprom_read_dword((uint32_t*)41);  //init Device Timecounter with stored eeprom value
 	Lamp_Timecounter_hour = eeprom_read_dword((uint32_t*)37); 
    init_RDM();
	init_pdm();
    Flags= 0;
    sei();
     for(;;)
 	{
         check_rdm(); //check for new RDM packets
         if (Flags &(1<<EVAL_DMX))		//universe complete?
 		{
            Flags &= ~(1<<EVAL_DMX);	//clear flag  
			  
			switch(RDM_Personality)
				{
					case 1:
						num2rgb((DmxField[0]*4), &r, &g, &b);
						PdmField[0] = saturize(r, DmxField[1], MAX_R);
						PdmField[1] = saturize(g, DmxField[1], MAX_G);
						PdmField[2] = saturize(b, DmxField[1], MAX_B);  
						break;
				
					case 2:
						PdmField[0] = DmxField[0];	//red
						PdmField[1] = DmxField[1];	//green           
						PdmField[2] = DmxField[2];	//blue	
						break;
				}
 			Device_Power=(PdmField[0]+PdmField[1]+PdmField[2]);

 		}
 	}
	
}


  ISR(TIMER0_OVF_vect)
  {
     	cnt++;
      	if(cnt==7547) //Counter for 1Sec Tick7547 1/((16000000/8)/7547)) = 1sec
		{
   			Device_Timecounter_sec++;
   			if(Device_Power>=20)Lamp_Timecounter_sec++;
			
     			if(Lamp_Timecounter_sec>=3600) 
   			  {
				Lamp_Timecounter_hour= eeprom_read_dword((uint32_t*)37);
   				Lamp_Timecounter_hour++;
   				Lamp_Timecounter_sec=0;
 				eeprom_write_dword((uint32_t*)37,Lamp_Timecounter_hour);  
   			  }				 
							  			
     			if(Device_Timecounter_sec>=3600) 
   			  {
   				Device_Timecounter_hour++;
   				Device_Timecounter_sec=0;
 				eeprom_write_dword((uint32_t*)41,Device_Timecounter_hour);  
   			  }				  							
		cnt=0;				
   		}

}
