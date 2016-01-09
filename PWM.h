/* 
 * File:   PWM.h
 * Author: Zachary
 *
 * Created on Dec 28, 2015, 12:14 PM
 */

void initPWM(int period, int duty);
void initPWMbuzz(void);

// enable timer 3, set period, duty, activate PWM
void initPWM(int period, int duty){
    T3CON = 0x8000;     // enable timer 3, PreScaler of 1
                        // T3's Ts = 62.5 ns
                        // want low end to be 5 ms
                        // 65535 is full scale of 16 bit counter
    PR3 = period-1;     // set the period register
    _T3IF = 0;          // set T3 interrupt flag low
    _T3IE = 1;          // enable the T3 interrupt
    
    OC1R = OC1RS = duty;  // set initial duty cycle to 50%
    
    OC1CON = 0x000E;    // activate PWM module (T3 is clock source, PWM mode on;
                        // Fault pin, OCFx disabled.)
    TMR3 = 0;
}

void initPWMbuzz(void){
    T2CON = 0x8000;     // enable timer 2, PreScaler of 1
                        // T2's Ts = 125 ns
                        // want low end to be 5 ms
                        // 65535 is full scale of 16 bit counter
    PR2 = 1000-1;       // set the period register (buzz freq of 8 kHz)
    _T2IF = 0;          // set T2 interrupt flag low
    _T2IE = 1;          // enable the T2 interrupt
    
    OC2R = OC2RS = 1000/2;  // set initial duty cycle to 50%
    
    OC2CON = 0x0006;    // activate PWM module (T2 is clock source, PWM mode on;
                        // Fault pin, OCFx disabled.)
    TMR2 = 0;
}

