#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

//#define DEBUG									//use Output for debugging

struct RDM_Packet	{
	uint8_t  ASC;								//alternate start code
	uint8_t  SSC;								//sub start code
	uint8_t  Length;							//total packet size
	uint8_t  DestID[6];							//target address
	uint8_t  SrcID[6];				    		//source address
	uint8_t  TransNo;							//tranaction number
	uint8_t  PortID;							//Port ID / Transaction Number
	uint8_t  MsgCount;							//message count
	uint16_t SubDev;							//sub device number (root = 0) 
	uint8_t  Cmd;								//command class
	uint16_t PID;								//parameter ID
	uint8_t  PDLen;								//parameter data length in bytes
	uint8_t  Data[24];							//data byte field
};


#define UID_0 0x12								//ESTA device ID High Byt
#define UID_1 0x34								//ESTA device ID Low Byt
#define UID_2 0x56								//Device ID 3 Byt
#define UID_3 0x88								//Device ID 2 Byt
#define UID_4 0x00								//Device ID 1 Byt
#define UID_5 0x01								//Device ID 0 Byt

#define F_OSC		    (16000)		  			//oscillator freq. in kHz (typical 8MHz or 16MHz)




enum {EVAL_DMX, DO_IDENTIFY};					//main flags

volatile uint8_t 	 Flags;						//Flags
volatile uint8_t	 DmxField[3];				//array of DMX vals (raw)
volatile uint8_t	 RdmField[50];				//array of RDM control data (raw)

extern void    init_RDM(void);
extern void    check_rdm(void);



// RDM CODES (ANSI E1.20)

//command classes
#define DISC_CMD				(0x10)
#define DISC_CMD_RESPONSE		(0x11)
#define GET_CMD					(0x20)
#define GET_CMD_RESPONSE		(0x21)
#define SET_CMD					(0x30)
#define SET_CMD_RESPONSE		(0x31)

// command class defines ??
#define CC_GET					(0x01)
#define CC_SET					(0x02)
#define CC_GET_SET				(0x03)

//response type
#define RESPONSE_TYPE_ACK		(0x00)
#define RESPONSE_TYPE_NACK		(0x02)	//with reason!!

//NACK reasons (MSB is always zero - left out!!)
#define NR_UNKNOWN_PID			(0x00)
#define NR_FORMAT_ERROR			(0x01)
#define NR_HARDWARE_FAULT		(0x02)
#define NR_WRITE_PROTECT		(0x04)
#define NR_UNSUPPORTED_CC		(0x05)
#define NR_DATA_OUT_OF_RANGE	(0x06)
#define NR_BUFFER_FULL			(0x07)
#define NR_PACKET_SIZE_UNSUPPORTED (0x08)
#define NR_NO_SUBDEVICE			(0x09)

//parameter IDs !!bytes are swapped!!
#define DISC_UNIQUE_BRANCH		(0x0001)	//network management
#define DISC_MUTE				(0x0002)
#define DISC_UN_MUTE			(0x0003)
#define IDENTIFY				(0x1000)
#define RESET					(0x1001)
#define STATUS_MESSAGES			(0x0030)
#define SUPPORTED_PARAMETERS	(0x0050)	//information
#define PARAMETER_DESCRIPTION	(0x0051)
#define DEVICE_INFO				(0x0060)
#define PRODUCT_DETAIL_ID_LIST	(0x0070)
#define DEVICE_LABEL			(0x0082)
#define MANUFACT_LABEL          (0x0081)
#define DMX_START_ADDRESS		(0x00F0)	//DMX setup
#define SLOT_INFO               (0x0120)
#define LAMP_HOURS              (0x0401)
#define DEVICE_HOURS            (0x0400)

//status types
#define STATUS_NONE				(0x00)		//not in queued msgs
#define STATUS_GET_LAST_MSG		(0x01)
#define STATUS_WARNING			(0x03)
#define STATUS_ERROR			(0x04)

//status msg IDs (error types),(MSB is zero and left out...)
#define STS_CAL_FAIL			(0x01)	//slot label
#define STS_SENS_NOT_FOUND		(0x02)	//"
#define STS_SENS_ALWAYS_ON		(0x03)	//"
#define STS_LAMP_DOUSED			(0x11)
#define STS_LAMP_STRIKE			(0x12)
#define STS_OVERTEMP			(0x21)	//SensorID, Temp
#define STS_UNDERTEMP			(0x22)	//"
#define STS_SENS_OUT_RANGE		(0x23)	//sensorID
#define STS_OVERVOLTAGE_PHASE	(0x31)	//Phase, Voltage
#define STS_UNDERVOLTAGE_PHASE	(0x32)	//"
#define STS_OVERCURRENT			(0x33)	//Phase, Current
#define STS_PHASE_ERROR			(0x36)	//Phase
#define STS_BREAKER_TRIP		(0x42)
#define STS_DIM_FAILURE			(0x44)
#define STS_READY				(0x50)	//slot label
#define STS_NOT_READY			(0x51)	//"
#define STS_LOW_FLUID			(0x52)	//"

//slot labels
#define SD_INTENSITY			(0x0001)
#define SD_INTENSITY_MASTER		(0x0002)
#define SD_PAN					(0x0101)
#define SD_TILT					(0x0102)
#define SD_COLOR_WHEEL			(0x0201)
#define SD_STATIC_GOBO_WHEEL	(0x0301)
#define SD_ROTO_GOBO_WHEEL		(0x0302)
#define SD_PRISM_WHEEL			(0x0303)
#define SD_STROBE				(0x0404)
#define SD_COLOR_ADD_RED		(0x0205)
#define SD_COLOR_ADD_GREEN		(0x0206)
#define SD_COLOR_ADD_BLUE		(0x0207)
#define SD_INTENSITY			(0x0001)
#define SD_INTENSITY			(0x0001)


#define SD_UNDEFINED			(0xFFFF)


//product category
#define FIXT_FIXED				(0x0101)	//not moving fixture
#define FIXT_MOVING_YOKE		(0x0102)	//moving head
#define FIXT_MOVING_MIRROR		(0x0103)	//scanner
#define FIXT_PROJECTOR			(0x0300)	//beamer
#define FIXT_ATMOSPHERIC_FX		(0x0401)	//fogger, hazer
#define FIXT_ATMOSPHERIC_PYRO	(0x0402)	//harr, harr...
#define FIXT_DIMMER				(0x0501)	//dimmer pack
#define FIXT_DIM_FLUORESCENT	(0x0502)	//cfl dimmer
#define FIXT_DIM_LED			(0x0509)	//LED dimmer


