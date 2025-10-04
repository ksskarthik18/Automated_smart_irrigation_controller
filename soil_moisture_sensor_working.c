#include "stm32f4xx.h"
#include <stdio.h>

#define PCF8574_ADDR 0x4E

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
    // The backlight bit (0x08) is already included in 'm'
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

// --- ADC + LED ---
void ADC1_Init(void){
    RCC->APB2ENR|=1<<8; RCC->AHB1ENR|=1<<0;

    // MODIFIED: Set PA1 to Analog Mode
    GPIOA->MODER |= (3U << 2);
    
    // MODIFIED: Select Channel 1 (for PA1) as the first in the sequence
    ADC1->SQR3 = 1;
    
    ADC1->CR2 |= 1; // ADON = 1 (Enable ADC)
}
uint16_t ADC1_Read(void){
    // Reads from the channel configured in SQR3 (now Channel 1)
    ADC1->CR2 |= 1<<30; // Start conversion
    while(!(ADC1->SR&2)); // Wait for EOC (End of Conversion)
    return ADC1->DR;
}
void LED_Init(void){ RCC->AHB1ENR|=1<<2; GPIOC->MODER|=1<<(13*2); }

int main(void){
    uint16_t sensor_value; char buf[16];
    I2C1_Init(); LCD_Init(); ADC1_Init(); LED_Init();

    while(1){
        sensor_value = ADC1_Read();
        
        // Use a threshold appropriate for your soil moisture sensor
        if(sensor_value > 3000) {
            GPIOC->ODR |= 1<<13; // Turn OFF on-board LED (PC13 is active-low)
        } else {
            GPIOC->ODR &= ~(1<<13); // Turn ON on-board LED
        }

        LCD_SetCursor(0,0);
        LCD_Print("Sensor Value:   ");    // overwrite with spaces
        LCD_SetCursor(1,0);
        sprintf(buf,"%u    ",sensor_value);     // add spaces to clear old digits
        LCD_Print(buf);

        delay_ms(200);  // faster refresh
    }
}