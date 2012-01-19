#ifndef _FARBKREIS_H
#define _FARBKREIS_H

#include <stdint.h>
#include <stdio.h>

#define BITS_S 8
#define BITS_R 8
#define BITS_G 8
#define BITS_B 8

#define MAX_S ((1<<BITS_S)-1)
#define MAX_R ((1<<BITS_R)-1)
#define MAX_G ((1<<BITS_G)-1)
#define MAX_B ((1<<BITS_B)-1)

#define NUM_VALUES (2*(MAX_R+MAX_G+MAX_B))
#define MAX_VALUE (NUM_VALUES-1)

void num2rgb(uint32_t i, uint8_t *r, uint8_t *g, uint8_t *b);
uint8_t saturize(uint8_t value, uint8_t saturation, uint8_t max_value);

#endif
