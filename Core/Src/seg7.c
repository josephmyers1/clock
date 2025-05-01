/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : seg7.c
  * @brief          : Main program body
  ******************************************************************************
  *
  *
  * Copyright (c) 2023 Boise State University
  * All rights reserved.
  *
  * This file provides a library for outputting characters to the 7 segment
  * displays on the STM32F4 Discovery Peripheral Board for ECE330L lab.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "seg7.h"

extern void Seven_Segment_Digit (unsigned char digit, unsigned char hex_char, unsigned char dot);
extern void Seven_Segment(unsigned int HexValue);

/* Character set */
char _7SEG[] =
		{(0x3F),(0x06),(0x5B),(0x4F),(0x66),(0x6D),(0x7D),
		(0x07),(0x7F),(0x6F),(0x77),(0x7C),(0x39),(0x5E),
		(0x79),(0x71),(0x3D),(0x76),(0x06),(0x1E),(0x7A),
		(0x38),(0x15),(0x54),(0x3F),(0x73),(0x67),(0x50),
		(0x6D),(0x78),(0x3E),(0x62),(0x2A),(0x64),(0x6E),
		(0x5B),(0x80),(0x48),(0x01),(0x02),(0x04),(0x08),
		(0x10),(0x20),(0x40),(0x00)};

void Seven_Segment_Digit (unsigned char digit, unsigned char hex_char, unsigned char dot)
{
/*******************************************************************************
Code to mask and bit shift 0-7 value of digit and 0-15 value of hex_char
to output correct bit pattern to GPIO_Output
*******************************************************************************/
	// Set selected digit to 0, all others high, and output 7 segment pattern
	GPIOE->ODR = (0xFF00 | _7SEG[hex_char]) & ~(1<<(digit+8));
	if (dot > 0) GPIOE->ODR |= (1<<7); // Set dot segment to 0 if dot is on


	// Set all selects high to latch-in character
	GPIOE->ODR |= 0xFF00;
	return;
}

void Seven_Segment(unsigned int HexValue)
{
/******************************************************************************
Use a for loop to output HexValue to 7 segment display digits
*******************************************************************************/
	char digit;
	// Send hex values to lower 4 digits
	for (digit=0 ; digit<8 ; digit++)
	{
	   Seven_Segment_Digit(digit,(HexValue >> (digit*4)) & 0xF,0);
	}

	return;
}
