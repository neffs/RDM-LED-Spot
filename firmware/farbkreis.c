/**** A P P L I C A T I O N   N O T E   ************************************
*
* Title			: rgb color calculation libery 
* Version		: v0.1
* Last updated	: 19.01.2012
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


#include "farbkreis.h"

void num2rgb(uint32_t i, uint8_t *r, uint8_t *g, uint8_t *b){
	uint32_t LHS;

	LHS = 0;
	if (i <= (MAX_G + LHS)){
		*r=MAX_R;
		*g=(i-LHS);
		*b=0;
		return;
	}

	LHS += MAX_G;
	if (i <= (MAX_R + LHS)){
		*r=MAX_R-(i-LHS);
		*g=MAX_G;
		*b=0;
		return;
	}

	LHS += MAX_R;
	if (i <= (MAX_B + LHS)){
		*r=0;
		*g=MAX_G;
		*b=(i-LHS);
		return;
	}

	LHS += MAX_B;
	if (i <= (MAX_G + LHS)){
		*r=0;
		*g=MAX_G-(i-LHS);
		*b=MAX_B;
		return;
	}

	LHS += MAX_G;
	if (i <= (MAX_R + LHS)){
		*r=(i-LHS);
		*g=0;
		*b=MAX_B;
		return;
	}

	LHS += MAX_R;
	if (i <= (MAX_B + LHS)){
		*r=MAX_R;
		*g=0;
		*b=MAX_B-(i-LHS);
		return;
	}
	return;
}

uint8_t saturize(uint8_t value, uint8_t saturation, uint8_t max_value){
	int16_t saturized = (saturation > (MAX_S/2)) ? value + ((saturation - (MAX_S/2)) * (max_value - value)) / (MAX_S/2) : value - (((MAX_S/2) - saturation) * value) / (MAX_S/2);
	saturized = (saturized > max_value) ? max_value : saturized;
	saturized = (saturized < 0) ? 0 : saturized;
	return (uint8_t) saturized;
}
