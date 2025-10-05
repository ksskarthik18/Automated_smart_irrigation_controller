#include "stm32f4xx.h"
#include <stdio.h>

#define PCF8574_ADDR 0x4E

// Define ADC thresholds for the 3 water levels.
// Higher ADC value = LOWER water level. You must adjust these by testing.
#define LEVEL_1_THRESHOLD 3000 // Threshold for "Low"
#define LEVEL_2_THRESHOLD 2000 // Threshold for "Medium"

// --- Delay ---
static void delay_ms(uint32_t ms) { for(uint32_t i=0; i<ms*8000; i++) __NOP(); }

// --- I2C ---
void I2C1_Init(void) {
    RCC->AHB1ENR |= (1<<1);  // GPIOB
    RCC->APB1ENR |= (1<<21); // I2C1
    GPIOB->MODER |= (2<<12)|(2<<14);    // PB6/PB7 AF
    GPIOB->OTYPER |= (1<<6)|(1<<7);
    GPIOB->PUPDR |= (1<<12)|(1<<14);
    GPIOB->AFR[0] |= (4<<24)|(4<<28);
    I2C1->CR1 = (1<<15); // Reset I2C
    I2C1->CR1 &= ~(1<<15); // Normal operation
    // These values are for a 42MHz APB1 clock.
    I2C1->CR2 = 42; I2C1->CCR = 210; I2C1->TRISE = 43;
    I2C1->CR1 |= (1<<0); // Enable I2C
}
void I2C1_Write(uint8_t d){
    while(I2C1->SR2 & 2);
    I2C1->CR1|=1<<8; while(!(I2C1->SR1&1));
    I2C1->DR=PCF8574_ADDR; while(!(I2C1->SR1&2)); (void)I2C1->SR2;
    while(!(I2C1->SR1&0x80)); I2C1->DR=d;
    while(!(I2C1->SR1&4)); I2C1->CR1|=1<<9;
}

// --- LCD ---
void LCD_Send(uint8_t v,uint8_t rs){
    uint8_t hi=v&0xF0, lo=(v<<4)&0xF0, m=rs?0x09:0x08;
    I2C1_Write(hi|m|4); delay_ms(1); I2C1_Write(hi|m);
    I2C1_Write(lo|m|4); delay_ms(1); I2C1_Write(lo|m);
}
void LCD_Init(void){
    delay_ms(50);
    LCD_Send(0x33,0); LCD_Send(0x32,0);
    LCD_Send(0x28,0); LCD_Send(0x0C,0);
    LCD_Send(0x06,0); LCD_Send(0x01,0);
}
void LCD_SetCursor(uint8_t r,uint8_t c){ LCD_Send((r?0xC0:0x80)+c,0); }
void LCD_Print(char*s){ while(*s) LCD_Send(*s++,1); }

// --- ADC + LEDs ---
void Periph_Init(void){
    // Enable Clocks
    RCC->AHB1ENR |= (1U << 0); // Enable GPIOA for ADC
    RCC->AHB1ENR |= (1U << 2); // Enable GPIOC for LEDs
    RCC->APB2ENR |= (1U << 8); // Enable ADC1

    // Configure ADC Pin (PA0)
    GPIOA->MODER |= 3; // PA0 in Analog Mode
    
    // Configure LED Pins (PC13, PC14, PC15) as Outputs
    GPIOC->MODER |= (1<<(13*2)) | (1<<(14*2)) | (1<<(15*2));

    // Configure ADC1
    ADC1->SQR3 = 0;   // Select Channel 0
    ADC1->CR2 |= 1;   // Enable ADC (ADON=1)
}
uint16_t ADC1_Read(void){
    ADC1->CR2 |= (1U << 30);      // Start conversion
    while(!(ADC1->SR & 2)); // Wait for End of Conversion
    return ADC1->DR;
}


int main(void){
    uint16_t water_level_raw; char buf[16];
    I2C1_Init();
    LCD_Init();
    Periph_Init();

    while(1){
        water_level_raw = ADC1_Read();

        // Logic to control the 3 LEDs based on the ADC value
        if (water_level_raw > LEVEL_1_THRESHOLD) {
            // Level 1: LOW water
            GPIOC->ODR |= (1U << 13);   // LED 1 OFF (PC13 is active-low on board)
            GPIOC->ODR &= ~((1U << 14) | (1U << 15)); // LED 2 & 3 OFF
        }
        else if (water_level_raw > LEVEL_2_THRESHOLD) {
            // Level 2: MEDIUM water
            GPIOC->ODR &= ~(1U << 13);  // LED 1 ON
            GPIOC->ODR |= (1U << 14);   // LED 2 ON
            GPIOC->ODR &= ~(1U << 15);  // LED 3 OFF
        }
        else {
            // Level 3: HIGH water
            GPIOC->ODR &= ~(1U << 13);  // LED 1 ON
            GPIOC->ODR |= (1U << 14);   // LED 2 ON
            GPIOC->ODR |= (1U << 15);   // LED 3 ON
        }

        LCD_SetCursor(0,0);
        LCD_Print("Water Level:    ");    // overwrite with spaces
        LCD_SetCursor(1,0);
        sprintf(buf,"%u    ", water_level_raw);     // add spaces to clear old digits
        LCD_Print(buf);

        delay_ms(200);  // faster refresh
    }
}