#include "fsl_device_registers.h"

/* ---------- Prototypes ---------- */
static unsigned short adc_read16b(void);

/* ---------- Globals ---------- */
uint32_t g_mode_select = 0;     /* Was MODE_SW */
uint32_t g_count_dir  = 0;      /* Was CNT_DIR */
uint32_t g_seg_hi     = 0;      /* Was shift2  */
uint32_t g_seg_lo     = 0;      /* Was shift5  */

int g_count_value     = 0;      /* Was counter */
int g_tens_digit      = 0;      /* Was num1    */
int g_ones_digit      = 0;      /* Was num2    */

/* 7-segment font map (was nums) */
int g_seven_seg_map[10] = {
    0x7E, /* 0 */
    0x30, /* 1 */
    0x6D, /* 2 */
    0x79, /* 3 */
    0x33, /* 4 */
    0x5B, /* 5 */
    0x5F, /* 6 */
    0x70, /* 7 */
    0x7F, /* 8 */
    0x7B  /* 9 */
};

/* ---------- ADC helper (was ADC_read16b) ---------- */
static unsigned short adc_read16b(void)
{
    ADC0_SC1A = 0x00;                       /* Start conversion from ADC0 */
    while (ADC0_SC2 & ADC_SC2_ADACT_MASK) { /* Wait while conversion active */ }
    while (!(ADC0_SC1A & ADC_SC1_COCO_MASK)) { /* Wait until complete */ }
    return ADC0_RA;                          /* Return result */
}

/* ---------- Port A IRQ Handler ---------- */
void PORTA_IRQHandler(void)
{
    /* Read mode select and count direction from Port B */
    g_mode_select = (GPIOB_PDIR & 0x0004u) >> 2; /* pin B2 */
    g_count_dir   = (GPIOB_PDIR & 0x0008u) >> 3; /* pin B3 */

    if (g_mode_select == 0)
    {
        /* -------- ADC Mode -------- */
        uint32_t adc_sample = adc_read16b();      /* was adc_val */
        uint32_t deci_value = (adc_sample * 3u) / 1945u; /* was volts (scaled) */

        g_tens_digit = (deci_value / 10u) & 0x3;  /* range 0–3 per original code */
        g_ones_digit = (deci_value % 10u) & 0x3;  /* range 0–3 per original code */

        g_seg_lo = g_seven_seg_map[g_ones_digit] & 0x003Fu;
        g_seg_hi = (g_seven_seg_map[g_ones_digit] & 0x00C0u) << 1;
        GPIOC_PDOR = g_seg_lo | g_seg_hi;                 /* Ones on Port C */
        GPIOD_PDOR = g_seven_seg_map[g_tens_digit];       /* Tens on Port D */
    }
    else
    {
        /* -------- Count Mode -------- */
        if (g_count_dir == 0)
        {
            /* Count up to 99 then roll over */
            if (g_count_value < 99) { g_count_value++; }
            else { g_count_value = 0; }
        }
        else
        {
            /* Count down to 0 then roll under to 99 */
            if (g_count_value > 0) { g_count_value--; }
            else { g_count_value = 99; }
        }

        g_tens_digit = (g_count_value / 10);
        g_ones_digit = (g_count_value % 10);

        g_seg_lo = g_seven_seg_map[g_ones_digit] & 0x003Fu;
        g_seg_hi = (g_seven_seg_map[g_ones_digit] & 0x00C0u) << 1;
        GPIOC_PDOR = g_seg_lo | g_seg_hi;                 /* Ones on Port C */
        GPIOD_PDOR = g_seven_seg_map[g_tens_digit];       /* Tens on Port D */
    }

    /* Turn on decimal point on Port D (bit 7) */
    GPIOD_PDOR |= 0x80u;

    /* Clear interrupt status flag for PA1 */
    PORTA_ISFR = (1u << 1);
}

/* ---------- Main ---------- */
int main(void)
{
    /* Enable Port A, B, C, D and ADC0 clock gating */
    SIM_SCGC5 |= (SIM_SCGC5_PORTA_MASK |
                  SIM_SCGC5_PORTB_MASK |
                  SIM_SCGC5_PORTC_MASK |
                  SIM_SCGC5_PORTD_MASK);
    SIM_SCGC6 |= SIM_SCGC6_ADC0_MASK;

    /* Configure PA1 for GPIO with falling-edge interrupt */
    PORTA_PCR1 = 0x0A0100;     /* MUX=1 (GPIO) | IRQC=1010 (falling edge) */
    PORTA_ISFR = (1u << 1);    /* Clear any pending flag on PA1 */

    /* ADC0: 16-bit, bus clock */
    ADC0_CFG1 = 0x0C;
    ADC0_SC1A = 0x1F;          /* Disable ADC channel (ADCH=11111) initially */

    /* Configure Port control registers:
       PB[3:2] as GPIO inputs, PC[8:7,5:0] as GPIO, PD[7:0] as GPIO */
    PORTB_GPCLR = 0x000C0100;
    PORTC_GPCLR = 0x01BF0100;
    PORTD_GPCLR = 0x00FF0100;

    /* Directions: B inputs; C & D outputs */
    GPIOB_PDDR = 0x00000000;   /* Port B inputs */
    GPIOC_PDDR = 0x000001BF;   /* Port C outputs */
    GPIOD_PDDR = 0x000000FF;   /* Port D outputs */

    /* Initialize outputs low */
    GPIOC_PDOR = 0x0000;
    GPIOD_PDOR = 0x0000;

    /* Enable Port A IRQ */
    NVIC_EnableIRQ(PORTA_IRQn);

    /* -------- Superloop -------- */
    while (1)
    {
        /* Turn off decimal point between refreshes (bit 7 low) */
        GPIOD_PDOR &= 0x7Fu;

        /* Simple delay */
        for (int delay_ticks = 0; delay_ticks < 150000; delay_ticks++)
        {
            __NOP();
        }
    }

    /* Not reached */
    /* return 0; */
}

