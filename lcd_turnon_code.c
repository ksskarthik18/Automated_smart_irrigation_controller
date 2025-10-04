#include "stm32f4xx.h"

/* ===================== LCD Commands ===================== */
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETDDRAMADDR 0x80

#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

#define LCD_DISPLAYMOVE 0x08
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x8DOTS 0x00

#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define ENABLE 0x04
#define RS 0x01

#define DEVICE_ADDR 0x27  // change to 0x3F if your module uses that

/* ===================== Globals ===================== */
static uint8_t dpFunction;
static uint8_t dpControl;
static uint8_t dpMode;
static uint8_t dpRows;
static uint8_t dpBacklight;

/* ===================== Function Prototypes ===================== */
void I2C1_Init(void);
void I2C1_Write(uint8_t addr, uint8_t data);
static void SendCommand(uint8_t cmd);
static void SendChar(uint8_t ch);
static void Send(uint8_t value, uint8_t mode);
static void Write4Bits(uint8_t value);
static void ExpanderWrite(uint8_t _data);
static void PulseEnable(uint8_t _data);
static void DelayUS(uint32_t us);
void HD44780_Init(uint8_t rows);
void HD44780_Clear(void);
void HD44780_Home(void);
void HD44780_SetCursor(uint8_t col, uint8_t row);
void HD44780_PrintStr(const char c[]);
void HD44780_Backlight(void);
void HD44780_NoBacklight(void);
void HD44780_AutoScroll(void);
void HD44780_NoAutoScroll(void);
void HD44780_LeftToRight(void);
void HD44780_RightToLeft(void);

/* ===================== I2C Init ===================== */
void I2C1_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;   // Enable GPIOB clock
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;    // Enable I2C1 clock

    // PB8=SCL, PB9=SDA
    GPIOB->MODER &= ~((3<<(8*2)) | (3<<(9*2)));
    GPIOB->MODER |= (2<<(8*2)) | (2<<(9*2)); // AF
    GPIOB->OTYPER |= (1<<8) | (1<<9);        // open-drain
    GPIOB->OSPEEDR |= (3<<(8*2)) | (3<<(9*2));
    GPIOB->PUPDR |= (1<<(8*2)) | (1<<(9*2)); // pull-up
    GPIOB->AFR[1] |= (4<<0) | (4<<4);        // AF4 = I2C1

    // Configure I2C
    I2C1->CR2 = 16;     // APB1 = 16MHz
    I2C1->CCR = 80;     // 100kHz
    I2C1->TRISE = 17;   
    I2C1->CR1 = I2C_CR1_PE;
}

/* ===================== I2C Write ===================== */
void I2C1_Write(uint8_t addr, uint8_t data) {
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = addr << 1;
    while(!(I2C1->SR1 & (I2C_SR1_ADDR | I2C_SR1_AF)));
    if(I2C1->SR1 & I2C_SR1_ADDR) { (void)I2C1->SR2; I2C1->DR = data; while(!(I2C1->SR1 & I2C_SR1_BTF)); }
    I2C1->CR1 |= I2C_CR1_STOP;
}

/* ===================== LCD Driver ===================== */
void HD44780_Init(uint8_t rows) {
    dpRows = rows;
    dpBacklight = LCD_BACKLIGHT;
    dpFunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
    if(dpRows>1) dpFunction |= LCD_2LINE;

    for(volatile int i=0;i<800000;i++); // ~50ms delay

    ExpanderWrite(dpBacklight);

    Write4Bits(0x03<<4); DelayUS(4500);
    Write4Bits(0x03<<4); DelayUS(4500);
    Write4Bits(0x03<<4); DelayUS(150);
    Write4Bits(0x02<<4);

    SendCommand(LCD_FUNCTIONSET | dpFunction);

    dpControl = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    SendCommand(LCD_DISPLAYCONTROL | dpControl);

    HD44780_Clear();
    dpMode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    SendCommand(LCD_ENTRYMODESET | dpMode);
    HD44780_Home();
}

void HD44780_Clear() { SendCommand(LCD_CLEARDISPLAY); DelayUS(2000); }
void HD44780_Home() { SendCommand(LCD_RETURNHOME); DelayUS(2000); }
void HD44780_SetCursor(uint8_t col, uint8_t row) { int row_offsets[]={0x00,0x40,0x14,0x54}; if(row>=dpRows) row=dpRows-1; SendCommand(LCD_SETDDRAMADDR | (col + row_offsets[row])); }
void HD44780_PrintStr(const char c[]) { while(*c) SendChar(*c++); }
void HD44780_Backlight(void) { dpBacklight=LCD_BACKLIGHT; ExpanderWrite(0); }
void HD44780_NoBacklight(void) { dpBacklight=LCD_NOBACKLIGHT; ExpanderWrite(0); }
void HD44780_AutoScroll(void) { dpMode |= LCD_ENTRYSHIFTINCREMENT; SendCommand(LCD_ENTRYMODESET | dpMode); }
void HD44780_NoAutoScroll(void) { dpMode &= ~LCD_ENTRYSHIFTINCREMENT; SendCommand(LCD_ENTRYMODESET | dpMode); }
void HD44780_LeftToRight(void) { dpMode |= LCD_ENTRYLEFT; SendCommand(LCD_ENTRYMODESET | dpMode); }
void HD44780_RightToLeft(void) { dpMode &= ~LCD_ENTRYLEFT; SendCommand(LCD_ENTRYMODESET | dpMode); }

/* ===================== Internal Helpers ===================== */
static void SendCommand(uint8_t cmd) { Send(cmd,0); }
static void SendChar(uint8_t ch) { Send(ch,RS); }
static void Send(uint8_t value, uint8_t mode) {
    uint8_t highnib = value & 0xF0;
    uint8_t lownib = (value<<4)&0xF0;
    Write4Bits(highnib|mode); Write4Bits(lownib|mode);
}
static void Write4Bits(uint8_t value) { ExpanderWrite(value); PulseEnable(value); }
static void ExpanderWrite(uint8_t _data) { uint8_t data=_data|dpBacklight; I2C1_Write(DEVICE_ADDR, data); }
static void PulseEnable(uint8_t _data) { ExpanderWrite(_data|ENABLE); DelayUS(20); ExpanderWrite(_data&~ENABLE); DelayUS(20); }
static void DelayUS(uint32_t us) { us*=(SystemCoreClock/1000000)/5; while(us--) __asm("nop"); }

/* ===================== Demo Main ===================== */
int main(void) {
    SystemInit();
    I2C1_Init();
    HD44780_Init(2); // 2-line LCD
    HD44780_Backlight();
    HD44780_AutoScroll();
    HD44780_LeftToRight();

    const char *msg = "This is a long text scrolling demo on STM32 Black Pill!";
    HD44780_SetCursor(0,0);

    while(1) {
        const char *p = msg;
        while(*p) {
            SendChar(*p++);
            DelayUS(100000); // small delay for visible scroll
        }
        HD44780_Clear();
        HD44780_SetCursor(0,0);
    }
}