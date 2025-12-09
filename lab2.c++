#include "fsl_device_registers.h"


int main(void)

{

    SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK; /*Enable Port B Clock Gate Control*/
	SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK; /*Enable Port C Clock Gate Control*/
	SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK; /*Enable Port D Clock Gate Control*/


	PORTB_GPCLR = 0x000C0100; /* Configures Pins 2 and 3 on Port B to be GPIO */
	PORTC_GPCLR = 0x01BF0100; /* Configures Pins 0-7 not 6 on Port B to be GPIO */
	PORTD_GPCLR = 0x00FF0100; /* Configures Pins 0-7 on Port D to be GPIO */


	GPIOB_PDDR = 0x00000000;  /*Configures Pins 2-3 of port B as Input*/
	GPIOC_PDDR = 0x000001BF;  /*Configures Pins 0-5 & 7-8 on Port C to be Output*/
	GPIOD_PDDR = 0x000000FF;  /*Configures Pins 0-7 on Port D to be Output*/


	GPIOC_PDOR = 0x0000; /*Initialize PORT C as 0*/
	GPIOD_PDOR = 0x0001; /*Initialize PORT D Pin 0 as 1*/


	uint32_t Shifter = 0,count = 0, Input = 0,CNT_DIR = 0,ROT_DIR = 0, bits_five_to_zero, bits_seven_to_eight;


	while(1){


		for (uint32_t i = 0; i < 100000; i++); // S/W Delay


		Input = GPIOB_PDIR & 0x0C; //read input

		CNT_DIR = Input & 0x08; // port 3
		ROT_DIR =  Input & 0x04; // port 2


		count = (CNT_DIR == 0)
		    ? ((count < 255) ? ((count + 1 == 255) ? 0 : count + 1) : ((count == 255) ? 0 : count))
		    : ((count > 0 && count < 256) ? ((count - 1 == 0) ? 255 : count - 1) : ((count == 0) ? 255 : count));

        // Direction select: 0 = count up, else count down
		// ---- INCREMENT BRANCH ----
		// If below max, we can increment
		// If next would be 255, wrap to 0; else increment
		// If already 255, wrap to 0; else hold (edge case)

        // ---- DECREMENT BRANCH ----
        // If 1..255, we can decrement
        // If next would be 0, wrap to 255; else decrement
        // If already 0, wrap to 255; else hold (edge case)



		bits_five_to_zero = count & 0x003F; /*Get bits 0-5*/
		bits_seven_to_eight = (count & 0x00C0) << 1; /*Shift bits 6-7 to 7-8*/
        GPIOC_PDOR = (bits_five_to_zero | bits_seven_to_eight); /*Get final output bits*/


		GPIOD_PDOR = (ROT_DIR == 0)
		    ? ((GPIOD_PDOR << 1) == 0x0200 ? 0x0001 : (GPIOD_PDOR << 1))
		    : ((GPIOD_PDOR >> 1) == 0x0000 ? 0x0100 : (GPIOD_PDOR >> 1));

		// if ROT_DIR == 0 WE CHECK IF GPIOD_PDOR IS PAST THE 8TH BIT AKA 0X0200 WRAP AROUND BACK TO 0X0001, otherwise left shifting. If ROT_DIR == 1
		// Do the opposite



	}


	return 0;
}

