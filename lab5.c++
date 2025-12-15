#include "MK64F12.h"

/* Step patterns for IN1–IN4 (PTD0–PTD3) */
static const uint8_t coilPattern[4][4] = {
		{0,1,1,0},  // 0110
		{1,0,1,0},  // 1010
		{1,0,0,1},  // 1001
		{0,1,0,1}   // 0101
};

/* Global millisecond counter driven by SysTick */
volatile uint32_t g_msTicks = 0;

/* SysTick Interrupt Handler */
void SysTick_Handler(void) {
	g_msTicks++;
}

/* Blocking millisecond delay */
void delayMs(uint32_t ms) {
	uint32_t target = g_msTicks + ms;
	while (g_msTicks < target);
}

/* ---------------- GPIO Initialization ---------------- */
void initGPIO(void) {

	/* Enable clocks for PORTA and PORTD */
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTD_MASK;

	/* Configure motor outputs on PTD0–PTD5 */
	for (int pin = 0; pin <= 5; pin++) {
		PORTD->PCR[pin] = PORT_PCR_MUX(1);  // MUX = GPIO
		PTD->PDDR |= (1 << pin);            // Set as output
	}

	/* Configure DIP switches on PTA1 and PTA2 */
	PORTA->PCR[1] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[2] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

	/* Set PTA1, PTA2 as inputs */
	PTA->PDDR &= ~((1 << 1) | (1 << 2));
}

/* ---------------- Apply Coil Pattern ---------------- */
void applyStepPattern(uint8_t index) {

	/* Clear PTD0–PTD3 */
	PTD->PCOR = 0x0F;

	/* Apply pattern */
	PTD->PSOR = (coilPattern[index][0] << 0) |
			(coilPattern[index][1] << 1) |
			(coilPattern[index][2] << 2) |
			(coilPattern[index][3] << 3);
}

/* --------------------------- MAIN --------------------------- */
int main(void) {

	initGPIO();

	/* Configure SysTick for 1ms (48 MHz core clock) */
	SysTick->LOAD = 48000 - 1;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
			SysTick_CTRL_ENABLE_Msk |
			SysTick_CTRL_TICKINT_Msk;

	/* Enable motor driver outputs PTD4, PTD5 */
	PTD->PSOR = (1 << 4) | (1 << 5);

	uint8_t stepIndex = 0;
	uint32_t lastStepTime = 0;
	uint32_t stepInterval = 250;  // default slow speed

	while (1) {

		uint8_t directionFlag = (PTA->PDIR & (1 << 1)) ? 1 : 0;
		uint8_t speedFlag     = (PTA->PDIR & (1 << 2)) ? 1 : 0;

		/* Fast = 34 ms, Slow = 250 ms */
		stepInterval = speedFlag ? 34 : 250;

		if (g_msTicks - lastStepTime >= stepInterval) {

			/* Change step index depending on direction */
			if (directionFlag) {
				stepIndex = (stepIndex == 0) ? 3 : stepIndex - 1;
			} else {
				stepIndex = (stepIndex + 1) & 0x03;
			}

			applyStepPattern(stepIndex);
			lastStepTime = g_msTicks;
		}
	}
}
