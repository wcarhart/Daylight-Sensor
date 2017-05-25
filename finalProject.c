/*
 * ELEC 310 Final Project
 * Author: Will Carhart
 * Date: May 15th, 2017
 */

// include necessary libraries
#include <xc.h>
#include <p18f4321.h>
#include <stdio.h>

// configure PIC18F4321
#pragma config OSC = INTIO2
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config BOR = OFF

// internal frequency definition
#define _XTAL_FREQ 8000000

// define local aliases
#define C0 LATCbits.LC0
#define C1 LATCbits.LC1
#define C2 LATCbits.LC2
#define C3 LATCbits.LC3
#define C4 LATCbits.LC4
#define C5 LATCbits.LC5
#define C6 LATCbits.LC6
#define C7 LATCbits.LC7

// forward-declare function prototypes
void sendNibble(char nibble);
void sendByte(char data);
void lcdWrite(char text[]);
char* char2ASCII(unsigned char temp);
void lcdAdd(char text[]);

// define globals
unsigned long time = 0x00000000;
unsigned char hours = 0x00;
unsigned char minutes = 0x00;
unsigned char seconds = 0x00;
unsigned char decimals = 0x00;
int colons = 1;
int AMPM = 1;

void main(void){
    // set up analog/digital, input/output
    ADCON1 = 0x0C;
    TRISA = 0x01;
    TRISC = 0x00;
    TRISD = 0xFF;
    
    // set up processor frequency
    OSCCONbits.IRCF = 0b111;

    // select channel on ADCON0
    ADCON0bits.CHS0 = 0;
    ADCON0bits.CHS1 = 0;
    ADCON0bits.CHS2 = 0;
    ADCON0bits.CHS3 = 0;
    
    // right justify ADC result
    ADCON2bits.ADFM = 1;

    // acquisition time = 2TAD
    ADCON2bits.ACQT0 = 1;
    ADCON2bits.ACQT1 = 0;
    ADCON2bits.ACQT2 = 0;

    // set clock to Fosc/2
    ADCON2bits.ADCS0 = 0;
    ADCON2bits.ACQS1 = 0;
    ADCON2bits.ACQS2 = 0;

    // interrupt setup
    INTCONbits.TMR0IE = 1;      // enable interrupt on TIMER0
    INTCONbits.TMR0IF = 0;      // clear TIMER0 interrupt flag
    INTCONbits.GIE = 1;         // enable global interrupts

    // TIMER0 setup
    T0CONbits.PSA = 1;          // do not assign prescaler to TIMER0
    T0CONbits.T08BIT = 0;       // use 16bit operation mode for TIMER0
    T0CONbits.T0CS = 0;         // use internal clock for TIMER0
    T0CONbits.TMR0ON = 1;       // turn on TIMER0

    /* LCD setup */
    __delay_ms(15);

    // set LCD to program mode 
    PORTCbits.RC0 = 0;
    PORTCbits.RC1 = 0;
    sendNibble(0b0011);
    __delay_us(4100);
    sendNibble(0b0011);
    __delay_us(100);
    sendNibble(0b0011);
    __delay_us(5);

    // set LCD to 4bit operation mode
    sendNibble(0b0010);
    __delay_us(42);

    // configure LCD to 1 line display, 5x10 dots
    sendByte(0b00100100);
    __delay_us(42);

    // turn on display, disable cursor
    sendByte(0b00001100);
    __delay_us(42);

    // clear display
    sendByte(0b00000110);
    __delay_us(42);

    // start time at 1:00 AM
    time = 4320000;
    
    // hardware initializations
    ADCON0bits.ADON = 1;
    PORTAbits.RA1 = 0;

    // main program loop
    float result, volt;
    while (1) {

        // A-->D conversion
        ADCON0bits.GO = 1;
        while (GO == 1);
        result = ADRESL;
        volt = (result / 255) * 5;
        
        // turn on coffe maker if bright enough
        if (volt >= 3.0) {
            PORTAbits.RA1 = 1;
        } else {
            PORTAbits.RA1 = 0;
        }
        
        // use buttons for setting time
        if (!PORTDbits.RD1) {
            time += 360000;
            __delay_ms(50);
        }
        if (!PORTDbits.RD2) {
            time += 6000;
            __delay_ms(50);
        }
        if (!PORTDbits.RD3) {
            time += 100;
            __delay_ms(50);
        }
        
        // output time onto LCD
        __delay_ms(10);
        lcdWrite(char2ASCII(hours));
        if (colons) {
            lcdAdd(":");
        } else {
            lcdAdd(" ");
        }
        lcdAdd(char2ASCII(minutes));
        if (colons) {
            lcdAdd(":");
        } else {
            lcdAdd(" ");
        }
        lcdAdd(char2ASCII(seconds));
        lcdAdd(" ");
        if (AMPM) {
            lcdAdd("A");
        } else {
            lcdAdd("P");
        }
        lcdAdd("M");

        // used for pausing execution
        if (!PORTDbits.RD0) {
            INTCONbits.TMR0IE = 0;
            __delay_ms(5);
            while(!PORTDbits.RD0);
            __delay_ms(5);
            while(PORTDbits.RD0);
            __delay_ms(5);
            while(!PORTDbits.RD0);
            INTCONbits.TMR0IE = 1;
        }
    }
}

char* char2ASCII(unsigned char input) {
    char disp[3];

    disp[0] = input / 10;
    disp[1] = input % 10;
    disp[0] += '0';
    disp[1] += '0';
    disp[2] = '\0';

    return disp;
}

void sendNibble(char nibble) {
    PORTCbits.RC7 = (nibble >> 3) & 1;
    PORTCbits.RC6 = (nibble >> 2) & 1;
    PORTCbits.RC5 = (nibble >> 1) & 1;
    PORTCbits.RC4 = nibble & 1;

    // pulse E-Clock
    PORTCbits.RC2 = 1;
    __delay_us(1);
    PORTCbits.RC2 = 0;
}

void sendByte(char data) {
    sendNibble(data >> 4);
    sendNibble(data & 0xF);
}

void lcdAdd(char text[]) {
    // set LCD to data mode
    PORTCbits.RC0 = 1;

    // print string one char at a time
    for (int i = 0; text[i] != 0; i++) {
        sendByte(text[i]);
        __delay_us(46);
    }
}

void lcdWrite(char text[]) {
    // set LCD to program mode
    PORTCbits.RC0 = 0;

    // clear display
    sendByte(0b00000001);
    __delay_us(1640);

    // reset cursor
    sendByte(0b00000010);
    __delay_us(1640);

    // set LCD to data mode
    PORTCbits.RC0 = 1;

    // print string one char at a time
    for (int i = 0; text[i] != 0; i++) {
        sendByte(text[i]);
        __delay_us(46);
    }
}

/*
 * interrupt routine -- update time
 */
void interrupt timerReset(void) {
    asm("MOVLW 0xB1");
    asm("MOVWF TMR0H");
    asm("MOVLW 0xC6");
    asm("MOVWF TMR0L");
    asm("BCF INTCON, 2"); //clear interrupt flag

    ++time;
    hours = ((time / 360000) % 12) + 1;
    minutes = (time % 360000) / 6000;
    seconds = ((time % 360000) % 6000) / 100;
    decimals = (((time % 360000) % 6000) % 100);

    if (time % 50 == 0) {
        colons = (colons + 1) % 2;
    }
    if (time % 72000 == 0) {
        AMPM = (AMPM + 1) % 2;
    }
}