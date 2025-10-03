#include "stm32f4xx.h"

// Threshold for what we consider "dry". Adjust this by testing your sensor.
// A higher ADC value means less moisture (higher resistance).
#define DRY_THRESHOLD 3000

int adc_value;

int main(void) {
    /* 1. Enable Clocks for GPIOA, GPIOC, and ADC1 */
    RCC->AHB1ENR |= (1U << 0); // Enable GPIOA clock (for PA1)
    RCC->AHB1ENR |= (1U << 2); // Enable GPIOC clock (for PC13 LED)
    RCC->APB2ENR |= (1U << 8); // Enable ADC1 clock

    /* 2. Configure GPIO Pins */
    // Set PA1 to Analog Mode for ADC input by setting bits [3:2] to '11'
    GPIOA->MODER |= (3U << 2); 

    // Set PC13 to General Purpose Output Mode for the LED
    GPIOC->MODER &= ~(1U << 27); // Clear bit 27
    GPIOC->MODER |=  (1U << 26); // Set bit 26 (MODER13[1:0] = 01)

    /* 3. Configure ADC1 */
    ADC1->CR2 = 0; // Clear CR2 register for a clean start
    ADC1->SQR1 = 0; // Ensure sequence length is 1 conversion
    
    // Set the channel sequence for Channel 1 (PA1).
    ADC1->SQR3 = (1U << 0); // Set SQ1[4:0] to 00001 for Channel 1
    
    ADC1->CR2 |= (1U << 1); // Enable Continuous Conversion Mode (CONT bit)
    ADC1->CR2 |= (1U << 0); // Turn on the ADC (ADON bit)
    
    // A small delay is required for the ADC to stabilize after power-on.
    for(volatile int i=0; i<1000; i++); 
    
    // Start the conversion
    ADC1->CR2 |= (1U << 30); // Set SWSTART bit
    
    while(1) {
        // Wait until the End Of Conversion (EOC) flag is set in the Status Register
        while (!(ADC1->SR & (1U << 1))) {
            // Wait for conversion to complete
        }
        
        // Read the converted value from the Data Register
        adc_value = ADC1->DR;
        
        // Control the LED based on the threshold
        if (adc_value > DRY_THRESHOLD) {
            // Soil is dry, turn the LED ON (PC13 is active-low)
            GPIOC->BSRR = (1U << 29); // Set BR13 to reset ODR13 to 0
        } else {
            // Soil is moist, turn the LED OFF
            GPIOC->BSRR = (1U << 13); // Set BS13 to set ODR13 to 1
        }
    }
}