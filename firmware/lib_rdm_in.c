/**** A P P L I C A T I O N   N O T E   ************************************
*
* Title			: DMX512 reception, RDM responder library
* Version		: v1.1
* Last updated	: 28.08.2010
* Target		: Transceiver Rev.3.01 [ATmega8515]
* Clock			: 8MHz, 16MHz
*
* written by hendrik hoelscher, www.hoelscher-hi.de
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


#include "lib_rdm_in.h"

// ********************* local definitions *********************

enum {IDLE, BRK, SB_DMX, STARTADR_DMX, SB_RDM, SSC_RDM, LEN_RDM, CHECKH_RDM, CHECKL_RDM};			//Rx states
enum {EVAL_RDM, MUTE_RDM};																			//local rdm flags

#define UID_CS (0xFF *6 +UID_0 +UID_1 +UID_2 +UID_3 +UID_4 +UID_5)

const	 uint8_t DevID[]= {UID_0, UID_1, UID_2, UID_3, UID_4, UID_5};


typedef union {
   uint16_t u16;
   struct {
          uint8_t u8l;
          uint8_t u8h;
          };
} uint16or8;

volatile uint8_t 	 RxState= IDLE;
volatile uint8_t	 RdmFlags=0;
		 uint16or8	 RxCount_16;						//start address and check sum counting
		 uint8_t     RxCount_8;							//byte counting
volatile uint16_t	 DmxAddress;

volatile uint8_t	 gTxCh;								//current channel to be transmitted


const uint8_t DiscRespMsg[] PROGMEM = {				//discovery response msg
0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xAA,			//preamble
(UID_0 |0xAA),											//encrypted UID
(UID_0 |0x55),
(UID_1 |0xAA),
(UID_1 |0x55),
(UID_2 |0xAA),
(UID_2 |0x55),
(UID_3 |0xAA),
(UID_3 |0x55),
(UID_4 |0xAA),
(UID_4 |0x55),
(UID_5 |0xAA),
(UID_5 |0x55),
((UID_CS>>8) |0xAA),
((UID_CS>>8) |0x55),
((UID_CS &0xFF) |0xAA),
((UID_CS &0xFF) |0x55)
};

const uint8_t InfoMsg[] PROGMEM = {					   //device info msg
1, 0,														//RDM protocol version
1, 0, 														//device model ID
(FIXT_FIXED>>8),											//fixture type
(FIXT_FIXED &0xFF),
0, 0, 1, 1,													//firmware version
0, sizeof(DmxField),										//DMX footprint
1, 1,														//current DMX personality				
0, 1,														//dummy start address
0, 0,														//no of sub devices
0															//no of sensors
};

const char ManufacturerLabel[] PROGMEM = {'D','a','v','i','d'};

const uint8_t ProductDetail[] PROGMEM = {0x04, 0x00}; //LED

const uint8_t SlotInfo[] PROGMEM = {
    //Slot 0
    0x00,0x00,0x00,
    (SD_COLOR_ADD_RED>>8),         (SD_COLOR_ADD_RED &0xFF),
    //Slot 1
    0x00,0x01,0x00,
    (SD_COLOR_ADD_GREEN>>8),         (SD_COLOR_ADD_GREEN &0xFF),
    //Slot 2
    0x00,0x02,0x00,
    (SD_COLOR_ADD_BLUE>>8),         (SD_COLOR_ADD_BLUE &0xFF)
};



const uint8_t ParamMsg[] PROGMEM = {					    //supported parameters msg
(DEVICE_LABEL>>8),         (DEVICE_LABEL &0xFF),
(MANUFACT_LABEL>>8),         (MANUFACT_LABEL &0xFF),
(DMX_START_ADDRESS>>8),    (DMX_START_ADDRESS &0xFF),
(STATUS_MESSAGES>>8),    (STATUS_MESSAGES &0xFF)
};


const uint8_t StatusMsg[] PROGMEM = {
0, 0,														//sub device ID
STATUS_NONE, 0,												//device state
STS_READY,													//error ID
(SD_UNDEFINED>>8), (SD_UNDEFINED &0xFF),  					//Parameter 1
0, 0														//Parameter 2
};



// *************** RDM Reception Initialisation ****************
void init_RDM(void)
{
DmxAddress= eeprom_read_word((uint16_t*)1);
if (DmxAddress > 512) DmxAddress= 1;

DDRD  |= (1<<2)|(1<<1);
PORTD &= ~(1<<2);											//enable reception
PORTD |= (1<<1);
UBRRH  = 0;
UBRRL  = ((F_OSC/4000)-1);									//250kbaud, 8N2
UCSRC  = (1<<URSEL)|(3<<UCSZ0)|(1<<USBS);
UCSRB  = (1<<RXEN)|(1<<RXCIE);
RxState= IDLE;

uint8_t i;
for (i=0; i<sizeof(DmxField); i++)
	{
	DmxField[i]= 0;
	}
}



// **** helper functions ****
static inline uint16_t swapInt(uint16_t val)				//change byte order
{
uint16_t res  = (val<<8);
		 res |= (val>>8);
return (res);
}

static uint8_t forUs(void)
{
struct RDM_Packet *rdm;
rdm = (struct RDM_Packet *)&RdmField;
uint8_t i, res= 0, resB= 0;
for (i=0; i<6; i++)
	{
	if (rdm->DestID[i] != DevID[i]) res= 1;
	if (rdm->DestID[i] != 0xFF)     resB= 1;
	}
if (resB == 0) res= resB;
return (res);
}


static uint8_t isLower(void)
{
struct RDM_Packet *rdm;
rdm = (struct RDM_Packet *)&RdmField;
uint8_t i ,buf;
for (i=0; i<6; i++)
	{
	buf= rdm->Data[i]; 
	if      (buf < DevID[i]) return (0);
	else if (buf > DevID[i]) return (1);
	}
return (0);
}



static uint8_t isHigher(void)
{
struct RDM_Packet *rdm;
rdm = (struct RDM_Packet *)&RdmField;
uint8_t i ,buf;
for (i=0; i<6; i++)
	{
	buf= rdm->Data[i +6]; 
	if      (buf > DevID[i]) return (0);
	else if (buf < DevID[i]) return (1);
	}
return (0);
}




// ****************** RDM data handling *******************
void respondMsg(void)
{
struct RDM_Packet *rdm;									//set addreses
rdm = (struct RDM_Packet *)&RdmField;

if (rdm->DestID[0] != 0xFF)								//don't respond to bradcasts!!
   	{
   	PORTD |= (1<<PD2);									//enable transmission

	uint8_t i;
	for (i=0; i<6; i++)
		{
		rdm->DestID[i]= rdm->SrcID[i];					//controller is target
		rdm->SrcID[i]= DevID[i];
		}
	rdm->ASC=      0xCC;
	rdm->SSC=      0x01;
	rdm->Length=   rdm->PDLen +24;
	rdm->MsgCount= 0;									//currently no queued msgs supported
	rdm->Cmd=      rdm->Cmd +1;							//it is a response... 

	uint16or8 cs;										//build check sum
	cs.u16= 0;
	for (i=0; i<rdm->Length; i++)
		{
		cs.u16+= RdmField[i];
		}
	RdmField[rdm->Length]= cs.u8h;
	RdmField[rdm->Length +1]= cs.u8l;

	UBRRL  = (F_OSC/800);								//45.5 kbaud
	gTxCh  = 0;											//reset field pointer
	UCSRB  = (1<<TXEN)|(1<<TXCIE);
	UDR    = 0;											//send break
	}
}


// *** generate "discover unique branch" response packet
void respondDisc(void)
{
if (!(RdmFlags &(1<<MUTE_RDM)))
	{
	UBRRH	= 0;
	UBRRL   = ((F_OSC/4000)-1);								//250kbaud
	UCSRB   = (1<<TXEN);
	
	_delay_us(70);
	_delay_us(70);

	PORTD  |= (1<<2);										//enable transmission
	uint8_t i;
	for(i=0;i<24;i++) 
		{
		_delay_us(5);
    	UDR  =pgm_read_byte(&DiscRespMsg[i]);
		UCSRA= (1<<TXC);
		loop_until_bit_is_set(UCSRA, TXC);					//send response
		}
	RxState= IDLE;    				   						//wait for break
	PORTD &= ~(1<<2);										//enable reception
	UCSRB  = (1<<RXEN)|(1<<RXCIE);
	#ifdef DEBUG
 	PORTA ^= (1<<PA4);										//LED: transmitting discovery response
	#endif	
	}
}



void check_rdm(void)
{
if (RdmFlags &(1<<EVAL_RDM))
	{
	struct RDM_Packet *rdm;
	rdm = (struct RDM_Packet *)&RdmField;
	if (forUs() == 0) 										//is packet for us?
		{
		//PORTE ^= (1<<PE0);									//LED: receiving RDM packet
		rdm->PortID= RESPONSE_TYPE_ACK;
		switch(swapInt(rdm->PID))
			{
			case DISC_UNIQUE_BRANCH:
				if((isLower() == 0) && (isHigher() == 0))	//is UID in disc branch?
					{
					   #ifdef DEBUG
 		   			   PORTA ^= (1<<PA3);					//LED: transmitting discovery response
		               #endif
					respondDisc();							//answer discovery msg
					}
				break;				

			case DISC_MUTE:									//we shall shut up
				RdmFlags |= (1<<MUTE_RDM);
				rdm->Data[0]= 0;
				rdm->Data[1]= 0;
				rdm->PDLen= 2;
				respondMsg();								//ACK
				   #ifdef DEBUG
 		           PORTA |= (1<<PA4);						//LED: muted
		           #endif
				break;

			case DISC_UN_MUTE:								//we shall respond again
				RdmFlags &= ~(1<<MUTE_RDM);
				rdm->Data[0]= 0;
				rdm->Data[1]= 0;
				rdm->PDLen= 2;
				respondMsg();								//ACK
				   #ifdef DEBUG
 		           PORTA &= ~(1<<PA4);						//LED: unmuted
		           #endif
				break;

			case IDENTIFY:									//we shall respond again								
				if (rdm->Cmd == SET_CMD)
					{
					if (rdm->Data[0] == 1) Flags |= (1<<DO_IDENTIFY);	//identification on
					else				   Flags &= ~(1<<DO_IDENTIFY);  //identification off
					rdm->PDLen= 0;
					respondMsg();
					}
				else
					{
					if (Flags &(1<<DO_IDENTIFY)) rdm->Data[0]= 1;		//ACK with identification state
				    else					  rdm->Data[0]= 0;
					rdm->PDLen= 1;
					respondMsg();
					}
				break;


			case DMX_START_ADDRESS:							//start address...
				if (rdm->Cmd == SET_CMD)
					{
					uint16or8 AdrBuf;
					AdrBuf.u8h= rdm->Data[0];
					AdrBuf.u8l= rdm->Data[1];
					DmxAddress= AdrBuf.u16;					//change address
					eeprom_write_word((uint16_t*)1, AdrBuf.u16);
					}
				else
					{
					uint16or8 AdrBuf;
					AdrBuf.u16= DmxAddress;
					rdm->Data[0]= AdrBuf.u8h; 				//return current address
					rdm->Data[1]= AdrBuf.u8l;
					}
				rdm->PDLen= 2;
				respondMsg();
				break;

			case DEVICE_INFO:
				memcpy_P(&(rdm->Data),InfoMsg, sizeof(InfoMsg));
				uint16or8 AdrBuf;
				AdrBuf.u16= DmxAddress;
				rdm->Data[14]= AdrBuf.u8h; 					//return current start address
				rdm->Data[15]= AdrBuf.u8l;
				rdm->PDLen= sizeof(InfoMsg);
				respondMsg();
				break;

			case DEVICE_LABEL:
				if (rdm->Cmd == SET_CMD)					//set new device label
					{
					if (rdm->PDLen > 32) rdm->PDLen= 32;					//limit to prevent ee overflow
					eeprom_write_byte((uint8_t*)3, rdm->PDLen);
					eeprom_write_block(rdm->Data, (uint16_t*)4, rdm->PDLen);
					}
				else
					{
					rdm->PDLen= eeprom_read_byte((uint8_t*)3);
					eeprom_read_block(rdm->Data, (uint16_t*)4, rdm->PDLen); //response with device label
					}
				respondMsg();
				break;
            
            case MANUFACT_LABEL:
                memcpy_P(&(rdm->Data),ManufacturerLabel, sizeof(ManufacturerLabel));
                rdm->PDLen= sizeof(ManufacturerLabel);
                respondMsg();
                break;
                    
            case PRODUCT_DETAIL_ID_LIST:
                memcpy_P(&(rdm->Data),ProductDetail, sizeof(ProductDetail));
                rdm->PDLen= sizeof(ProductDetail);
                respondMsg();
                break;
                    
            case SLOT_INFO:
                memcpy_P(&(rdm->Data),SlotInfo, sizeof(SlotInfo));
                rdm->PDLen= sizeof(SlotInfo);
                respondMsg();
                break;  

			case STATUS_MESSAGES:
				memcpy_P(&(rdm->Data),StatusMsg, sizeof(StatusMsg));
				rdm->PDLen= 9;
				respondMsg();
				break;

			case SUPPORTED_PARAMETERS:
				memcpy_P(&(rdm->Data),ParamMsg, sizeof(ParamMsg));
				rdm->PDLen= sizeof(ParamMsg);
				respondMsg();
				break;

			default:
				rdm->Data[0]=  (uint8_t)(NR_UNKNOWN_PID>>8); //response: PID not supported!
				rdm->Data[1]=  (uint8_t)NR_UNKNOWN_PID;
				rdm->PDLen= 2;
				rdm->PortID= RESPONSE_TYPE_NACK;
				respondMsg();
				break;	
			}
		}
	RdmFlags &= ~(1<<EVAL_RDM);
	}
}




// *************** DMX & RDM Reception ISR ****************
ISR (UART_RX_vect)
{
uint8_t Temp= UCSRA;										//get state
uint8_t DmxByte= UDR;										//get data

if (Temp &(1<<FE))											//check for break
	{
	UCSRA &= ~(1<<FE);										//reset flag
	RxState= BRK;											//Break detected
	}

else switch (RxState)
	{
	case BRK:
		if (DmxByte == 0x00)								//DMX start code detected
			{
			RxState= SB_DMX; 
			RxCount_16.u16= DmxAddress;
			}	
		else if (DmxByte == 0xCC)							//RDM start code detected
			{
			RxState= SB_RDM; 	
			}
		else 
			{
			RxState= IDLE;									//error detected
			}
		break;

	case SB_DMX:
		if (--RxCount_16.u16 == 0)							//count dmx channels
			{
			RxState= STARTADR_DMX;							//start address reached
			DmxField[0]= DmxByte;							//get 1st ch
			RxCount_8= 1;									//seed ch counter
			}
		break;

	case STARTADR_DMX:
		DmxField[RxCount_8++]= DmxByte;						//get channel
		if (RxCount_8 >= sizeof(DmxField))
			{
			Flags |= (1<<EVAL_DMX);							//reception finished
			RxState  = IDLE;								//all ch received -> wait for break
			}
		break;

// ********* RDM specific code *********
	case SB_RDM:
		if ((DmxByte == 0x01) && !(RdmFlags &(1<<EVAL_RDM)))
			{
			RxState= SSC_RDM; 				   				//valid sub start code identified and buffer free
			RxCount_8= 3;
			  #ifdef DEBUG
 			  PORTA ^= (1<<PA0);							//LED: correct start of RDM packet
			  #endif
			}
		else RxState= IDLE;    				   				//error
		break;

	case SSC_RDM:
		RdmField[2]= DmxByte;   				   			//get total message length
		if (DmxByte < sizeof(RdmField))
			 {
			 RxCount_16.u16= (uint16_t)(0xCD +DmxByte); 	//initialize check sum
			 RxState= LEN_RDM;
			 	#ifdef DEBUG
 			    PORTA ^= (1<<PA1);							//LED: size of RdmField is sufficient
			    #endif					
			 }
		else RxState= IDLE;					   				//reset if too much data!
		break;

	case LEN_RDM:
		RdmField[RxCount_8++]= DmxByte;						//store data
		RxCount_16.u16 += (uint16_t)DmxByte;
		if (RdmField[2] == RxCount_8) RxState= CHECKH_RDM; 		//data complete?
		break;

	case CHECKH_RDM:
		if (DmxByte == RxCount_16.u8h) 						//check check sum high
			{
			RxState= CHECKL_RDM;
			}
		else RxState= IDLE;
		break;
	
	case CHECKL_RDM:
		if (DmxByte == RxCount_16.u8l)
			{
		       #ifdef DEBUG
 		       PORTA ^= (1<<PA2);							//LED: valid check sum
		       #endif
			RdmFlags |= (1<<EVAL_RDM);						//valid RDM packet received
			}
		RxState= IDLE;
		break;
	}							
}


// *************** RDM Response ISR ****************

ISR (UART_TX_vect)
{
_delay_us(5);
uint8_t TxCh= gTxCh;

if (TxCh == 0)
	{
	UBRRL  = ((F_OSC/4000)-1);								//250kbaud
	UDR    = 0xCC;											//send alternate start code
	gTxCh= 1;
	}
else
	{
	if (TxCh > (RdmField[2]+1))
		{
		PORTD ^= (1<<PD7);									//LED: responding
		RxState= IDLE;    				   					//wait for break
		UCSRB  = (1<<RXEN)|(1<<RXCIE);
		sei();												//allow interrupts in delay
		_delay_us(18);
		PORTD &= ~(1<<2);									//enable reception
		}
	else
		{
		UDR= RdmField[TxCh++];								//send data
		gTxCh= TxCh;
		}
	}
}
