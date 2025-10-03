#include "stm32f4xx.h"

// Define the ADC thresholds. You will need to test and adjust these values
// by dipping the sensor in water to different levels.
#define LEVEL_LOW      3000 // ADC value when water is just touching the sensor
#define LEVEL_MEDIUM   2000 // ADC value when sensor is about halfway submerged

int adc_value;

int main(void) {
    /* 1. Enable Clocks for GPIOA, GPIOC, and ADC1 */
    RCC->AHB1ENR |= (1U << 0); // Enable GPIOA clock (for PA0)
    RCC->AHB1ENR |= (1U << 2); // Enable GPIOC clock (for LEDs on PC13, PC14, PC15)
    RCC->APB2ENR |= (1U << 8); // Enable ADC1 clock

    /* 2. Configure GPIO Pins */
    // Set PA0 to Analog Mode for ADC input
    GPIOA->MODER |= (3U << 0); // Set bits [1:0] to '11' for PA0

    // Set PC13, PC14, PC15 to General Purpose Output Mode for the LEDs
    GPIOC->MODER &= ~((3U << 26) | (3U << 28) | (3U << 30)); // Clear mode bits for all 3 pins
    GPIOC->MODER |= (1U << 26) | (1U << 28) | (1U << 30);    // Set mode to '01' (Output) for PC13, PC14, PC15

    /* 3. Configure ADC1 */
    ADC1->CR2 = 0;  // Clear CR2 for a clean start
    ADC1->SQR1 = 0; // Sequence length of 1 conversion
    
    // Set the channel sequence. We want to convert Channel 0 (PA0).
    ADC1->SQR3 = 0; // Set SQ1[4:0] to 00000 for Channel 0
    
    ADC1->CR2 |= (1U << 1); // Enable Continuous Conversion Mode (CONT bit)
    ADC1->CR2 |= (1U << 0); // Turn on the ADC (ADON bit)
    
    // A small delay for the ADC to stabilize
    for(volatile int i=0; i<1000; i++); 
    
    // Start the first conversion
    ADC1->CR2 |= (1U << 30); // Set SWSTART bit
    
    while(1) {
        // Wait for End of Conversion (EOC)
        while (!(ADC1->SR & (1U << 1))) {}
        
        adc_value = ADC1->DR;
        
        // Logic to control the 3 LEDs based on the ADC value
        if (adc_value > LEVEL_LOW) {
            // LOW water level
            // Turn ON LED 1 (PC13 is active-low)
            GPIOC->BSRR = (1U << 29); // Reset PC13
            // Turn OFF LED 2 & 3
            GPIOC->BSRR = (1U << 14) | (1U << 15); // Set PC14, PC15
        } 
        else if (adc_value > LEVEL_MEDIUM) {
            // MEDIUM water level
            // Turn ON LED 1 & 2
            GPIOC->BSRR = (1U << 29); // Reset PC13
            GPIOC->ODR &= ~(1U << 14); // Set PC14 LOW to turn ON
            // Turn OFF LED 3
            GPIOC->BSRR = (1U << 15); // Set PC15
        } 
        else {
            // HIGH water level
            // Turn ON all 3 LEDs
            GPIOC->BSRR = (1U << 29); // Reset PC13
            GPIOC->ODR &= ~((1U << 14) | (1U << 15)); // Set PC14, PC15 LOW to turn ON
        }
    }
}