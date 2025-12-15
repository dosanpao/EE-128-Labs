#include "fsl_device_registers.h"

int D_array[10] = {
    0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110,
    0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1100111
};

int C_array[10] = {
    0b0111111, 0b0000110, 0b10011011, 0b10001111, 0b10100110,
    0b10101101, 0b10111101, 0b0000111, 0b10111111, 0b10100111
};

int CDP_array[4] = { 0b100111111, 0b100000110, 0b110011011, 0b110001111 };

volatile unsigned int nr_overflows = 0;
unsigned long Delay = 0x100000;

//------------------------------------------------------------
// Software delay function
//------------------------------------------------------------
void software_delay(unsigned long delay) {
    while (delay--) {
        __NOP();
    }
}

//------------------------------------------------------------
// FTM3 Interrupt Service Routine
//------------------------------------------------------------
void FTM3_IRQHandler(void) {
    nr_overflows++;
    unsigned int temp = FTM3_SC; // read SC to clear flags
    (void)temp;                  // prevent compiler warning
    FTM3_SC &= 0x7F;             // clear TOF flag
}

//------------------------------------------------------------
// Main Program
//------------------------------------------------------------
int main(void) {
    // Enable clock gating for ports and FTM3
    SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK | SIM_SCGC5_PORTD_MASK;
    SIM_SCGC3 |= SIM_SCGC3_FTM3_MASK;

    // Configure Port C (pins 0–5,7–8) and Port D (pins 0–7) as GPIO
    PORTC_GPCLR = 0x01BF0100;
    PORTD_GPCLR = 0x00FF0100;

    // Configure Port C pin 10 as FTM3_CH6 (ALT3)
    PORTC_PCR10 = 0x300;

    // Setup FTM3
    FTM3_MODE = 0x05;       // Enable FTM3
    FTM3_MOD  = 0xFFFF;
    FTM3_SC   = 0x0E;       // System clock, prescaler = 64

    // Configure GPIO as outputs
    GPIOC_PDDR = 0x000001BF;
    GPIOD_PDDR = 0x000000FF;

    // Enable FTM3 interrupts
    NVIC_EnableIRQ(FTM3_IRQn);
    FTM3_SC |= 0x40; // Enable TOF interrupt

    unsigned int t1, t2, pulse_width, period;

    while (1) {
        software_delay(Delay);

        //----------------------------
        // Measure pulse width
        //----------------------------
        FTM3_CNT = 0;
        nr_overflows = 0;

        FTM3_C6SC = 0x04; // Capture on rising edge
        while (!(FTM3_C6SC & 0x80)); // Wait for CHF
        FTM3_C6SC &= ~(1 << 7);
        t1 = FTM3_C6V;

        FTM3_C6SC = 0x08; // Capture on falling edge
        while (!(FTM3_C6SC & 0x80)); // Wait for CHF
        FTM3_C6SC = 0;
        t2 = FTM3_C6V;

        pulse_width = (nr_overflows << 16) + (t2 - t1);

        //----------------------------
        // Measure period
        //----------------------------
        FTM3_C6SC = 0x04; // Rising edge
        while (!(FTM3_C6SC & 0x80));
        FTM3_C6SC &= ~(1 << 7);
        t1 = FTM3_C6V;

        while (!(FTM3_C6SC & 0x80));
        FTM3_C6SC = 0;
        t2 = FTM3_C6V;

        period = t2 - t1;

        //----------------------------
        // Compute and display duty cycle
        //----------------------------
        float dutyCycle = ((float)pulse_width / (float)period) * 100.0f;
        int ones = (int)dutyCycle % 10;
        int tens = ((int)dutyCycle / 10) % 10;

        GPIOD_PDOR = D_array[ones];
        GPIOC_PDOR = C_array[tens];

        // short delay or breakpoint
        for (volatile int i = 0; i < 100000; i++);
    }

    return 0;
}
