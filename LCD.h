/* 
 * File:   LCD.h
 * Author: Zachary
 *
 * Created on Dec 28, 2015, 12:31 PM
 */

#define LCD_DATA    1
#define LCD_CMD     0
#define PM_DATA     PMDIN1
#define PM_ADDR     PMADDR

void initLCD(void);
char readLCD(int addr);
void writeLCD(int addr, char c);
void putsLCD(char *s);
void initPMP(void);
void setCursor(int row, int col);
void clearLCD(void);
void fourBitMode(char fullByte);

#define busyLCD()   readLCD(LCD_CMD) & 0x80
#define addrLCD()   readLCD(LCD_CMD) & 0x7F
#define getLCD()    readLCD(LCD_DATA)
#define putLCD(d)   writeLCD(LCD_DATA,d)
#define cmdLCD(c)   writeLCD(LCD_CMD, (c))
#define homeLCD()   writeLCD(LCD_CMD, 2)
#define clrLCD()    writeLCD(LCD_CMD, 1)
#define setLCDG(a)  writeLCD(LCD_CMD, (a & 0x3F) | 0x40)
#define setLCDC(a)  writeLCD(LCD_CMD, (a & 0x7F) | 0x80)
int delayTime = 50;

void initPMP(void){
    PMCON = 0x0000;
    PMCON = 0x033F; // Enable the PMP, long waits
    PMMODE = 0x03FF; // Master Mode 1
    PMAEN = 0x0001; // PMA0 enabled
    PMCON |= 0x8000;
}

// this is for 4-bit mode
void initLCD(void){
    while(PMMODEbits.BUSY);     // wait for PMP to be available
    // begin routine for 4-bit mode:
    delay_ms(50);
    PM_ADDR = LCD_CMD;
    PM_DATA = 0b00110000;
    delay_ms(5);
    PM_DATA = 0b00110000;
    delay_us(150);
    PM_DATA = 0b00110000;
    delay_ms(5);
    PM_DATA = 0b00100000;
    delay_us(delayTime);
    // end routine for 4-bit mode
   
    // number of lines and font
    PM_DATA = 0b00100000;
    delay_us(delayTime);
    PM_DATA = 0b10000000;
    delay_us(delayTime);
    
    // disable display/cursor/blink
    PM_DATA = 0b00000000;
    delay_us(delayTime);
    PM_DATA = 0b10000000;
    delay_us(delayTime);
    
    // clear screen, return cursor home
    PM_DATA = 0b00000000;
    delay_us(delayTime);
    PM_DATA = 0b00010000;
    delay_ms(2);
    
    // increment cursor to right when shifting,
    // don't shift screen
    PM_DATA = 0b00000000;
    delay_us(delayTime);
    PM_DATA = 0b01100000;
    delay_us(delayTime);
    
    // enable the display
    PM_DATA = 0b00000000;
    delay_us(delayTime);
    PM_DATA = 0b11000000;
    delay_us(delayTime);
    PMMODEbits.BUSY = 0;
}

void putsLCD(char *s){
    while(*s)
        putLCD(*s++);
}

char readLCD(int addr){
    int dummy;
    while(PMMODEbits.BUSY);     // wait for PMP to be available
    PM_ADDR = addr;              // select the command address
    dummy = PM_DATA;            // initiate a read cycle, dummy
    while(PMMODEbits.BUSY);     // wait for PMP to be available
    return(PM_DATA);            // read the status register
}

void writeLCD(int addr, char c){
    //while(busyLCD());
    while(PMMODEbits.BUSY);     // wait for PMP to be available
    PM_ADDR = addr;              // select the command address
    PM_DATA = c & 0xF0;   
    delay_us(delayTime);
    PM_DATA = c << 4 & 0xF0;
    delay_us(delayTime);
    PMMODEbits.BUSY = 0;
}

void clearLCD(void){
    while(PMMODEbits.BUSY);
    PM_ADDR = LCD_CMD;
    PM_DATA = 0b00000000; // clear display
    delay_us(delayTime);
    PM_DATA = 0b00010000; // clear display
    delay_ms(1.8);
    PMMODEbits.BUSY = 0;
}

void setCursor(int row, int col){
    while(PMMODEbits.BUSY);
    if(row == 2)
        setLCDC((0x40 + col));
    else
        setLCDC((0x00 + col));
    PMMODEbits.BUSY = 0;    
}

void fourBitMode(char fullByte){
    //send high bit
    PM_DATA = fullByte & 0xF0;
    delay_us(delayTime);
    //send low bit
    PM_DATA = (fullByte << 4) & 0xF0;
    delay_us(delayTime);
}