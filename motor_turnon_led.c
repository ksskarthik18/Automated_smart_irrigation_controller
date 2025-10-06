#include "stm32f4xx.h"
#include <string.h> // Needed for strlen() in our helper function

#define PCF8574_ADDR 0x4E

// --- Thresholds ---
#define SOIL_DRY_THRESHOLD      3500
#define SOIL_DAMP_THRESHOLD     2500
#define WATER_LOW_THRESHOLD     1300
#define WATER_MEDIUM_THRESHOLD  1800

// --- Global State Variable ---
volatile uint8_t display_mode = 0; // 0 for Soil, 1 for Water

// --- Function Prototypes ---
static void delay_ms(uint32_t ms);
void itoa(int n, char s[]);
void reverse(char s[]);
void I2C1_Init(void);
void LCD_Init(void);
void LCD_SetCursor(uint8_t r, uint8_t c);
void LCD_Print(char*s);
void LCD_Clear(void);
void Periph_Init(void);
void Button_Interrupt_Init(void);
uint16_t ADC1_Read(uint8_t channel);

/* =============================================================== */
int main(void){
    uint16_t soil_value, water_value;
    char value_buf[10];

    I2C1_Init();
    LCD_Init();
    Periph_Init();
    Button_Interrupt_Init();

    LCD_Clear();

    while(1){
        // --- Read both sensors ---
        soil_value = ADC1_Read(1);
        water_value = ADC1_Read(2);

        if (display_mode == 0) {
            // --- SOIL MOISTURE MODE ---
            itoa(soil_value, value_buf);
            LCD_SetCursor(0,0);
            LCD_Print("Soil Moisture:");

            if (soil_value > SOIL_DRY_THRESHOLD) {
                LCD_SetCursor(1,0); LCD_Print("DRY   - "); LCD_Print(value_buf); LCD_Print("   ");
            } else if (soil_value > SOIL_DAMP_THRESHOLD) {
                LCD_SetCursor(1,0); LCD_Print("DAMP  - "); LCD_Print(value_buf); LCD_Print("   ");
            } else {
                LCD_SetCursor(1,0); LCD_Print("WET   - "); LCD_Print(value_buf); LCD_Print("   ");
            }

        } else {
            // --- WATER LEVEL MODE ---
            itoa(water_value, value_buf);
            LCD_SetCursor(0,0);
            LCD_Print("Water Level:  ");

            if (water_value < WATER_LOW_THRESHOLD) {
                LCD_SetCursor(1,0); LCD_Print("LOW   - "); LCD_Print(value_buf); LCD_Print("   ");
            } else if (water_value < WATER_MEDIUM_THRESHOLD) {
                LCD_SetCursor(1,0); LCD_Print("MEDIUM- "); LCD_Print(value_buf); LCD_Print("   ");
            } else {
                LCD_SetCursor(1,0); LCD_Print("HIGH  - "); LCD_Print(value_buf); LCD_Print("   ");
            }
        }

        /* ===============================================================
           🔥 LED Logic:
           PC14 glows if Water HIGH + Soil DRY
           PC15 glows otherwise
           =============================================================== */
        if ((water_value >= WATER_MEDIUM_THRESHOLD) && (soil_value > SOIL_DRY_THRESHOLD)) {
            GPIOC->ODR |=  (1U << 14);   // Turn ON PC14
            GPIOC->ODR &= ~(1U << 15);   // Turn OFF PC15
        } else {
            GPIOC->ODR &= ~(1U << 14);   // Turn OFF PC14
            GPIOC->ODR |=  (1U << 15);   // Turn ON PC15
        }

        delay_ms(150);
    }
}

/* =============================================================== */
/* --- INTERRUPT HANDLER for EXTI0 (PA0) --- */
void EXTI0_IRQHandler(void) {
    if (EXTI->PR & (1U << 0)) {
        delay_ms(50);
        display_mode = !display_mode;
        LCD_Clear();
        EXTI->PR = (1U << 0);
    }
}

/* =============================================================== */
/* --- INITIALIZATION FUNCTIONS --- */
void Periph_Init(void){
    // Enable GPIOA, GPIOB, GPIOC
    RCC->AHB1ENR |= (1U << 0) | (1U << 1) | (1U << 2);
    RCC->APB2ENR |= (1U << 8);

    // --- Button on PA0 ---
    GPIOA->MODER &= ~(3U << 0);
    GPIOA->PUPDR |=  (1U << 0);

    // --- ADC input pins on PA1 and PA2 ---
    GPIOA->MODER |= (3U << 2) | (3U << 4);

    // --- Indicator LEDs on PC14 and PC15 ---
    GPIOC->MODER &= ~((3U << 28) | (3U << 30)); // Clear mode bits
    GPIOC->MODER |=  (1U << 28) | (1U << 30);  // Set PC14 & PC15 as outputs

    // --- ADC setup ---
    ADC1->SQR1 = 0;
    ADC1->CR2 |= (1U << 0);
}

/* =============================================================== */
void Button_Interrupt_Init(void) {
    RCC->APB2ENR |= (1U << 14);
    SYSCFG->EXTICR[0] &= ~(0xF << 0);
    EXTI->IMR |= (1U << 0);
    EXTI->FTSR |= (1U << 0);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

/* =============================================================== */
uint16_t ADC1_Read(uint8_t channel){
    ADC1->SQR3 = channel;
    ADC1->CR2 |= (1U << 30);
    while(!(ADC1->SR & 2));
    return ADC1->DR;
}

/* =============================================================== */
/* --- Helper, I2C, and LCD functions --- */
static void delay_ms(uint32_t ms) { for(uint32_t i=0; i<ms*8000; i++) __NOP(); }

void reverse(char s[]) {
    int i, j; char c;
    for (i = 0, j = strlen(s)-1; i<j; i++, j--) { c = s[i]; s[i] = s[j]; s[j] = c; }
}

void itoa(int n, char s[]) {
    int i, sign;
    if ((sign = n) < 0) n = -n;
    i = 0;
    do { s[i++] = n % 10 + '0'; } while ((n /= 10) > 0);
    if (sign < 0) s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

/* =============================================================== */
void I2C1_Init(void) {
    RCC->AHB1ENR |= (1<<1);
    RCC->APB1ENR |= (1<<21);
    GPIOB->MODER |= (2<<12)|(2<<14);
    GPIOB->OTYPER |= (1<<6)|(1<<7);
    GPIOB->PUPDR |= (1<<12)|(1<<14);
    GPIOB->AFR[0] |= (4<<24)|(4<<28);
    I2C1->CR1 = (1<<15); I2C1->CR1 &= ~(1<<15);
    I2C1->CR2 = 42; I2C1->CCR = 210; I2C1->TRISE = 43;
    I2C1->CR1 |= (1<<0);
}

/* =============================================================== */
void I2C1_Write(uint8_t d){
    while(I2C1->SR2 & 2);
    I2C1->CR1|=1<<8; while(!(I2C1->SR1&1));
    I2C1->DR=PCF8574_ADDR; while(!(I2C1->SR1&2)); (void)I2C1->SR2;
    while(!(I2C1->SR1&0x80)); I2C1->DR=d;
    while(!(I2C1->SR1&4)); I2C1->CR1|=1<<9;
}

/* =============================================================== */
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
    delay_ms(2);
}

void LCD_SetCursor(uint8_t r,uint8_t c){ LCD_Send((r?0xC0:0x80)+c,0); }
void LCD_Print(char*s){ while(*s) LCD_Send(*s++,1); }
void LCD_Clear(void) { LCD_Send(0x01,0); }