/* 
 * File:   Stir Plate Project
 * Author: Zachary
 *
 * Created on Dec 28, 2015, 12:13 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <libpic30.h>
#include <math.h>
#include "config.h"
#include "PWM.h"
#include "LCD.h"

/** Preprocessor Directives **/
#define TRUE    1           // define a true value of 1
#define FALSE   0           // define a false value of 0
#define HIGH    1
#define LOW     0
#define OUTPUT  0
#define INPUT   1
#define SPI_SS      LATAbits.LATA0      // RA0 (output)
#define SPI_SDI     PORTBbits.RB0       // RB0 (input)
#define SPI_SCK     LATBbits.LATB2      // RB2 (output)

/** Global Variables **/
unsigned long rpm;
char strRPM[4];
double tempC = 0;
char strTempC[8];
unsigned long period;
unsigned long period_old;
unsigned long duty;
unsigned int mainMenuSelect = 32766; //middle of unsigned int for even inc./dec.
unsigned int modeMenuSelect = 32766; //middle of unsigned int for even inc./dec.
typedef unsigned char BOOL; // typedef unsigned char to BOOL
typedef struct my_booleans  // create a structure of booleans
{
  BOOL mainMenuTrue:1;
  BOOL modeMenuTrue:1;
  BOOL rpmMenuTrue:1;
  BOOL mainDecide:1;
  BOOL modeDecide:1;
  BOOL rpmDecide:1;
}
my_booleans_t;
my_booleans_t myBOOLs;

/** Function Prototypes **/
void mainMenu(void);
void modeMenu(void);
void rpmMenu(void);
void menuDecision(void);
double thermoRead(void);

/** ISRs **/
// Interrupt for encoder turn (CW & CCW)
void __attribute__((__interrupt__, auto_psv)) _INT1Interrupt(void){
    // note that PORTBbits.RB4 = 1 indicates CW turn, 0 indicates CCW turn
    // decisions for MAIN MENU
    if(myBOOLs.mainMenuTrue == TRUE && PORTBbits.RB4 == 1){
        mainMenuSelect = mainMenuSelect + 1;
    }
    else if(myBOOLs.mainMenuTrue == TRUE && PORTBbits.RB4 == 0){
        mainMenuSelect = mainMenuSelect - 1;
    }
    
    // decisions for MODE MENU
    if(myBOOLs.modeMenuTrue == TRUE && PORTBbits.RB4 == 1){
        modeMenuSelect = modeMenuSelect + 1;
    }
    else if(myBOOLs.modeMenuTrue == TRUE && PORTBbits.RB4 == 0){
        modeMenuSelect = modeMenuSelect - 1;
    }
    
    // decisions for RPM MENU
    if(myBOOLs.rpmMenuTrue == TRUE && PORTBbits.RB4 == 1 && rpm < 150){
        rpm = rpm + 1;
    }
    else if(myBOOLs.rpmMenuTrue == TRUE && PORTBbits.RB4 == 0 && rpm > 5){
        rpm = rpm - 1;  
    }
    
    _INT1IF = 0; //set INT1 flag back to zero 
}

// Interrupt for encoder push button
void __attribute__((__interrupt__, auto_psv)) _INT2Interrupt(void){  
    // decisions from MAIN MENU ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if(PORTBbits.RB9 == 0){
        if(myBOOLs.mainMenuTrue == TRUE && (mainMenuSelect % 2) == 0){
            //modeMenu();
            myBOOLs.mainDecide = FALSE;
            myBOOLs.modeDecide = TRUE;      // goto mode menu
            myBOOLs.rpmDecide = FALSE;
        }
        else if(myBOOLs.mainMenuTrue == TRUE && (mainMenuSelect % 2) != 0){
            //rpmMenu();
            myBOOLs.mainDecide = FALSE;
            myBOOLs.modeDecide = FALSE;
            myBOOLs.rpmDecide = TRUE;       // goto rpm menu
        }

        // decisions from MODE MENU ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if(myBOOLs.modeMenuTrue == TRUE && (modeMenuSelect % 2) == 0){
            //mainMenu();
            myBOOLs.mainDecide = TRUE;      // goto main menu
            myBOOLs.modeDecide = FALSE;
            myBOOLs.rpmDecide = FALSE;
            //continuous stir mode
        }
        else if(myBOOLs.modeMenuTrue == TRUE && (modeMenuSelect % 2) != 0){
            //mainMenu();
            myBOOLs.mainDecide = TRUE;      // goto main menu
            myBOOLs.modeDecide = FALSE;
            myBOOLs.rpmDecide = FALSE;
            //shake mode
        }  

        // decision from RPM MENU ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if(myBOOLs.rpmMenuTrue == TRUE){
            //mainMenu();
            myBOOLs.mainDecide = TRUE;      // goto main menu
            myBOOLs.modeDecide = FALSE;
            myBOOLs.rpmDecide = FALSE;
        }
    }
    
    _INT2IF = 0; //set INT2 flag back to zero 
}

// Interrupt for output compare timer
void __attribute__((__interrupt__, auto_psv)) _T3Interrupt(void){
    _T3IF = 0;  //set T3 flag back to zero 
}

void __attribute__((__interrupt__, auto_psv)) _T2Interrupt(void){
    _T2IF = 0;  //set T2 flag back to zero 
}

/** Main **/
int main(void){  
    /** Declare Local Variables **/
    
    /** Initialize State Variables **/
    myBOOLs.mainMenuTrue = TRUE;    // want program to enter into the main menu
    myBOOLs.mainDecide = TRUE;      // want program to enter into the main menu
    myBOOLs.modeMenuTrue = FALSE;
    myBOOLs.modeDecide = FALSE;
    myBOOLs.rpmMenuTrue = FALSE;
    myBOOLs.rpmDecide = FALSE;
    
    /** Initialize Value Variables **/
    rpm = 60;
    
    /** Configure Analog Ports **/
    AD1PCFGbits.PCFG5 = HIGH;  // disable analog functionality for AN5 (Encoder Pin A)
    AD1PCFGbits.PCFG0 = HIGH;  // disable analog functionality for AN0 (SPI - SS)
    AD1PCFGbits.PCFG2 = HIGH;  // disable analog functionality for AN2 (SPI - SDI)
    AD1PCFGbits.PCFG4 = HIGH;  // disable analog functionality for AN4 (SPI - SCK)

    /** Configure Pin Directions **/
    TRISAbits.TRISA2 = OUTPUT;      // *output* Stepper Motor Dir
    TRISAbits.TRISA4 = OUTPUT;      // *output* Red LED Sink
    
    TRISBbits.TRISB3 = INPUT;       // *input* (Encoder Pin A) ~interrupt
    TRISBbits.TRISB4 = INPUT;       // *input* (Encoder Pin B)
    TRISBbits.TRISB9 = INPUT;       // *input* (Encoder PUSH) ~interrupt
    
    TRISAbits.TRISA0 = OUTPUT;      // *output* (SS for Thermo)
    TRISBbits.TRISB0 = INPUT;       // *input*  (SPI - SDI)
    TRISBbits.TRISB2 = OUTPUT;      // *output* (SPI - SCK)
    
    /** Initialize Pin States **/
    LATAbits.LATA2 = LOW;       // Set direction to CW
    LATAbits.LATA4 = LOW;       // Set Red LED ON (sink current)
    LATAbits.LATA0 = HIGH;      // Set SS high
    TRISBbits.TRISB2 = LOW;     // Set SCK low
    
    /** Configure Peripheral Pin Selects **/
    __builtin_write_OSCCONL(OSCCON & 0xBF); // unlock registers to configure PPS
    RPINR0bits.INT1R = 3;   // configure RP3 as Ext. Int. 1
    RPINR1bits.INT2R = 9;   // configure RP9 as Ext. Int. 2
    RPOR7bits.RP15R = 18;   // configure RP15 as OC1 (PWM for EasyDriver)
    RPOR5bits.RP10R = 19;   // configure RP11 as OC2 (PWM for buzzer)
    __builtin_write_OSCCONL(OSCCON | 0x40); // lock registers
    
    /** Configure PWMs **/
    period = ((8000000*60)/(1600*rpm));     // calculate PR based on desired PRM
    duty = period/2;                        // duty cycle of 50%
    initPWM(period, duty);                  // PWM initialization function
    initPWMbuzz();

    /** Configure PMP & LCD **/
    initPMP();
    initLCD();
    
    setLCDG(0);
    putLCD(0b00000);
    putLCD(0b01100);
    putLCD(0b01100);
    putLCD(0b01100);
    putLCD(0b11110);
    putLCD(0b11110);
    putLCD(0b01100);
    putLCD(0);
    
    /** Configure Interrupts **/
    INTCON2bits.ALTIVT = 0; //use primary IVT
    //EXT1 interrupt
    INTCON2bits.INT1EP = 1; // interrupt on falling edge
    IPC5bits.INT1IP = 4; //set priority level to 4
    IFS1bits.INT1IF = 0; //initialize INT1 flag to zero
    IEC1bits.INT1IE = 1; //enable the interrupt source
    //EXT2 interrupt
    INTCON2bits.INT2EP = 1; // interrupt on falling edge
    IPC7bits.INT2IP = 7; //set priority level to 4
    IFS1bits.INT2IF = 0; //initialize INT2 flag to zero
    IEC1bits.INT2IE = 1; //enable the interrupt source

    /** LCD Splash Screen **/
    clearLCD();
    setCursor(1,0);
    putsLCD("   Stir Plate   ");
    setCursor(2,0);
    putsLCD("      9000      ");
    delay_ms(3000);
    clearLCD();
    OC2CON = 0x0000;
    /*------------------INFINITE LOOP------------------*/
    while(1)
    {
        menuDecision();
    }
    return (EXIT_SUCCESS);
}

/** LCD (Main Menu) **/
void mainMenu(void){
    _INT1IF = LOW;                      //set INT1 flag back to zero 
    _INT2IF = LOW;                      //set INT2 flag back to zero
    int i;
    clearLCD();
    setCursor(1,1);
    putsLCD("Mode");
    setCursor(1,15);
    putsLCD("C");
    setCursor(2,1);
    putsLCD("RPM");
    setCursor(2,9);
    sprintf(strRPM,"%lu",rpm);
    putsLCD(strRPM);
    mainMenuSelect = 32766;             // this handles cursor position by means of modulus
    myBOOLs.mainMenuTrue = TRUE;        // these determine which menu for ext interrupt 2
    myBOOLs.modeMenuTrue = FALSE;       // these determine which menu for ext interrupt 2
    myBOOLs.rpmMenuTrue = FALSE;        // these determine which menu for ext interrupt 2
    while(myBOOLs.mainDecide){
        if(mainMenuSelect % 2 == 0){
            setCursor(1,0);
            putsLCD(">");
            setCursor(2,0);
            putsLCD(" ");
        }
        else if(mainMenuSelect % 2 != 0){
            setCursor(1,0);
            putsLCD(" ");
            setCursor(2,0);
            putsLCD(">");
        }

        // Take 50 readings of tempC
        tempC = 0;
        for(i=0;i<50;i++){
            tempC = tempC + thermoRead();
        }
        // Find average
        tempC = tempC/50;
        sprintf(strTempC,"%.02f",tempC);
        if(tempC < 100 && tempC >= 10){
            setCursor(1,15);
            putsLCD("C");
            setCursor(1,7);
            putLCD(' ');
            setCursor(1,8);
            putLCD(' ');
            setCursor(1,9);
            putLCD(0);
            setCursor(1,10);
        }
        else if(tempC < 10 && tempC >= 0){
            setCursor(1,15);
            putsLCD("C");
            setCursor(1,8);
            putLCD(' ');
            setCursor(1,9);
            putLCD(' ');
            setCursor(1,10);
            putLCD(0);
            setCursor(1,11);
        }
        else{
            setCursor(1,15);
            putsLCD("C");
            setCursor(1,8);
            putLCD(0);
            setCursor(1,9);
        }
        // Show averaged temp on display
        putsLCD(strTempC);
    }   
}

/** LCD (Mode Select Menu) **/
void modeMenu(void){
    _INT1IF = LOW;                  //set INT1 flag back to zero 
    _INT2IF = LOW;                  //set INT2 flag back to zero 
    clearLCD();
    setCursor(1,1);
    putsLCD("Stir");
    setCursor(2,1);
    putsLCD("Shake");
    modeMenuSelect = 32766;         // this handles cursor position by means of modulus
    myBOOLs.mainMenuTrue = FALSE;   // these determine which menu for ext interrupt 2
    myBOOLs.modeMenuTrue = TRUE;    // these determine which menu for ext interrupt 2
    myBOOLs.rpmMenuTrue = FALSE;    // these determine which menu for ext interrupt 2
    while(myBOOLs.modeDecide){
        if(modeMenuSelect % 2 == 0){
            setCursor(1,0);
            putsLCD(">");
            setCursor(2,0);
            putsLCD(" ");
        }
        else{
            setCursor(1,0);
            putsLCD(" ");
            setCursor(2,0);
            putsLCD(">");
        }
    }
}

/** LCD (RPM Menu) **/
void rpmMenu(void){
    _INT1IF = LOW;                  //set INT1 flag back to zero 
    _INT2IF = LOW;                  //set INT2 flag back to zero 
    
    clearLCD();
    setCursor(1,0);
    putsLCD("Set RPM to: ");
    myBOOLs.mainMenuTrue = FALSE;   // these determine which menu for ext interrupt 2
    myBOOLs.modeMenuTrue = FALSE;   // these determine which menu for ext interrupt 2
    myBOOLs.rpmMenuTrue = TRUE;     // these determine which menu for ext interrupt 2
    while(myBOOLs.rpmDecide){
        period = ((8000000*60)/(1600*rpm));
        if(period != period_old){
            PR3 = period-1;
            duty = period/2; 
            OC1R = OC1RS = duty;    // duty cycle to 50%
        }
        sprintf(strRPM,"%lu",rpm);
        setCursor(1,12);
        putsLCD(strRPM);
        putsLCD("   ");
        period_old = period;
    }
}

/** Menu Decision Routine **/
void menuDecision(void){
    if(myBOOLs.mainDecide)
        mainMenu();
    if(myBOOLs.modeDecide)
        modeMenu();
    if(myBOOLs.rpmDecide)
        rpmMenu(); 
}

/** Stir Plate Shake Mode **/
void shakeMode(void){
    // enable timer based on rpm
    // enable timer interrupt
    // timer interrupt will switch the direction
}

/** Stir Plate Stir Mode **/
void stirMode(void){
    // disable timer
    // disable timer interrupt
}

/** Piezo Buzzer **/
void buzz(void){
    // enable PWM on RP10
    // set period (frequency 8 kHz), duty 50%
    // enable
    // delay 100 ms
    // disable
}

/** SPI Read from Thermocouple **/
double thermoRead(void){
    // only need to take in first 14 bits
    // first bit is the sign bit
    // the next 11 bits are the integer number
    // the next 2 bits are the fractional value
    double tempC = 0;
    double tempF = 0;
    char signBit;
    char integerVal[11];
    char fractionalVal[2];
    int i;
    
    /** Gather temp data **/
    // Grab sign bit first...
    SPI_SS = LOW;                       // drive SS low get new thermo data
    SPI_SCK = HIGH;                     // set clock line high
    signBit = SPI_SDI;
    SPI_SCK = LOW;                      // set clock line low
    
    // Grab integer value...
    for(i=10; i>=0; i--){
        SPI_SCK = HIGH;                 // set clock line high
        integerVal[i] = SPI_SDI;        // read bit from thermo's DO line
        SPI_SCK = LOW;                  // set clock line low
    }
    
    // Grab fractional value...
    for(i=1; i>=0; i--){
        SPI_SCK = HIGH;                 // set clock line high
        fractionalVal[i] = SPI_SDI;     // read bit from thermo's DO line
        SPI_SCK = LOW;                  // set clock line low
    }
    SPI_SS = HIGH;              // drive SS high to disable slave
    
    if(signBit == 1)
        tempC = -2048;
    else
        tempC = 0;
    
    // Convert temp data to floating point
    for(i=0; i<=10; i++){
        if(integerVal[i] == 1)
            tempC = tempC + pow(2,i);
    }
    
    for(i=0; i<=1; i++){
        if(fractionalVal[i] == 1)
            tempC = tempC + pow(2,-(i+1));
    }
    tempF = tempC*1.8+32;
    return tempC;
}

