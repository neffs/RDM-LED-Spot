#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>

volatile uint8_t	 PdmField[3];	//array of pam vals


#define OUTPORT  	(PORTB)
#define OUTDDR		(DDRB)

#define CH1			(PB2)
#define CH2			(PB3)
#define CH3			(PB4)



extern void init_pdm(void);

