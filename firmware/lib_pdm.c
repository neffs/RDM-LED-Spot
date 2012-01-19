/* PDM Library (pulse density modulation lib, C version)
 ****************************************************************************
 * Author:      Hendrik Hölscher                         					*
 * modified by:    															*
 * Revision:    1.0                              							*
 * Date:        01.09.08                  									*
 * Target:      Transceiver Rev.3.01 (www.hoelscher-hi.de)        			*               								*
 * Clock:		8MHz, 16MHz
 *
 * written by hendrik hoelscher, www.hoelscher-hi.de
 * edit by Markus Bechtold Markus@r-bechtold.de
 ***************************************************************************
 This program is free software; you can redistribute it and/or 
 modify it under the terms of the GNU General Public License 
 as published by the Free Software Foundation; either version2 of 
 the License, or (at your option) any later version. 

 This program is distributed in the hope that it will be useful, 
 but WITHOUT ANY WARRANTY; without even the implied warranty of 
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 General Public License for more details. 

 If you have no copy of the GNU General Public License, write to the 
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 

 For other license models, please contact the author.

;***************************************************************************/


#include "lib_pdm.h"

// ********************* local definitions *********************
		uint8_t     PdmCompare =1;


// *************** PAM Initialisation ****************
void init_pdm(void)
{
//Output
OUTDDR |= (1<<CH1) | (1<<CH2) | (1<<CH3);
OUTPORT &= ~((1<<CH1) | (1<<CH2) |  (1<<CH3));

//Timer1
 TCCR1B  = (1<<CS11);				// set prescaler 8	
 TIMSK |= (1<<OCIE1A);				//enable compare irq
}


// ****************** PAM compare ISR ********************
ISR (TIMER1_COMPA_vect)
{
uint8_t PdmState= OUTPORT;
PdmState &= ~((1<<CH1) | (1<<CH2) |  (1<<CH3));
uint8_t PdmBuf= PdmCompare;
if (PdmField[0] >= PdmBuf) PdmState|= (1<<CH1);			//compare channels
if (PdmField[1] >= PdmBuf) PdmState|= (1<<CH2);
if (PdmField[2] >= PdmBuf) PdmState|= (1<<CH3);
OUTPORT= PdmState;

if (PdmBuf &(1<<7))
	 {
	 PdmBuf++;											//increment compare register				
	 PdmBuf &= ~(1<<7);									//clear MSB
	 if (PdmBuf == 0) PdmBuf= 0x80;
	 }
else PdmBuf |= (1<<7);									//set MSB
PdmCompare= PdmBuf;
OCR1A+=40;												// set timer compare value 
}
