
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

// define local alias
#define C0 LATCbits.LC0
#define C1 LATCbits.LC1
#define C2 LATCbits.LC2
#define C3 LATCbits.LC3
#define C4 LATCbits.LC4
#define C5 LATCbits.LC5
#define C6 LATCbits.LC6
#define C7 LATCbits.LC7

// forward-declare function prototypes
void send_nib(char nibble);
void send_byte(char data);
void lcdWrite(char text[]);
char* char2ASCII(unsigned char temp);
void lcdAdd(char text[]);

// define globals
unsigned long time=0x00000000;
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
    TRISB = 0x00;
    TRISD = 0xFF;
    PORTB = 0xFF;
    
    // set up processor frequency
    OSCCONbits.IRCF = 0b111;

    ADCON0bits.CHS0 = 0;        // ADCON0 bit 2 Channel Select
    ADCON0bits.CHS1 = 0;        // ADCON0 bit 3 Channel Select
    ADCON0bits.CHS2 = 0;        // ADCON0 bit 4 Channel Select
    ADCON0bits.CHS3 = 0;        // ADCON0 bit 5 Channel Select
    
    ADCON2bits.ADFM = 1;        // Result Format is Right Justified
    ADCON2bits.ACQT0 = 1;       // Acquisition Time Select bit 3 = 2TAD
    ADCON2bits.ACQT1 = 0;       // Acquisition Time Select bit 4 = 2TAD
    ADCON2bits.ACQT2 = 0;       // Acquisition Time Select bit 5 = 2TAD
    ADCON2bits.ADCS0 = 0;       // Conversion Clock Select bit 0 = FOSC/2
    ADCON2bits.ACQT1 = 0;       // Conversion Clock Select bit 1 = FOSC/2
    ADCON2bits.ACQT2 = 0;       // Conversion Clock Select bit 2 = FOSC/2

    // Timer/Interrupt Setup
    INTCONbits.TMR0IE = 1;  // Enable interrupt on timer 1
    INTCONbits.TMR0IF = 0;  // Clear interrupt flag
    INTCONbits.GIE = 1;     // Enable interrupts globally
    T0CONbits.PSA = 1;
    T0CONbits.T08BIT = 0;
    T0CONbits.T0CS = 0;
    T0CONbits.TMR0ON = 1;

    // LCD setup
    __delay_ms(15);
    PORTCbits.RC0 = 0;      // Set to LCD to program mode
    PORTCbits.RC1 = 0;      // Set LCD to write
    send_nib(0b0011);       // Setup command
    __delay_us(4100);
    send_nib(0b0011);       // Setup command
    __delay_us(100);
    send_nib(0b0011);       // Setup command
    __delay_us(5);
    send_nib(0b0010);       // Set LCD to 4 bit interface
    __delay_us(42);
    send_byte(0b00100100);  // Set LCD to 1 line and 5 * 10
    __delay_us(42);
    send_byte(0b00001100);  // Turn on display, turn off cursor and blinking
    __delay_us(42);
    send_byte(0b00000110);  // Clear display
    __delay_us(42);

    // start time at 1:00 AM
    time = 4320000;
    
    // main program loop
    ADCON0bits.ADON = 1;
    float result, volt;
    PORTAbits.RA1 = 0;
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
        if(!PORTDbits.RD0){
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


char* char2ASCII(unsigned char s){
    char disp[3];

    disp[0] = s/10;
    disp[1] = s%10;

    disp[0] +='0';
    disp[1] +='0';

    disp[2]='\0';

    return disp;
}


void send_nib(char nibble){
    PORTCbits.RC7 = (nibble >> 3) & 1;
    PORTCbits.RC6 = (nibble >> 2) & 1;
    PORTCbits.RC5 = (nibble >> 1) & 1;
    PORTCbits.RC4 = nibble & 1;
    PORTCbits.RC2 = 1;                  // E-clock high
    __delay_us(1);                      // Delay must exceed 220 ns
    PORTCbits.RC2 = 0;                  // E-clock low
}


void send_byte(char data){
    send_nib(data >> 4);
    send_nib(data & 0xF);
}


void lcdAdd(char text[]){
    PORTCbits.RC0 = 1;                  // Set LCD to data mode
    for (int i = 0; text[i] != 0; i++){ // Iterate over string
        send_byte(text[i]);             // Send each character of string
        __delay_us(46);
    }
}

void lcdWrite(char text[]){
    PORTCbits.RC0 = 0;                  // Set LCD to program mode
    send_byte(0b00000001);              // Clear display
    __delay_us(1640);
    send_byte(0b00000010);              // Return cursor to home
    __delay_us(1640);
    PORTCbits.RC0 = 1;                  // Set LCD to data mode
    for (int i = 0; text[i] != 0; i++){ // Iterate over string
        send_byte(text[i]);             // Send each character of string
        __delay_us(46);
    }
}

/*
 * interrupt routine -- update time
 */
void interrupt timerReset(void){
    asm("MOVLW 0xB1");
    asm("MOVWF TMR0H");
    asm("MOVLW 0xC6");
    asm("MOVWF TMR0L");
    asm("BCF INTCON, 2"); //clear interrupt flag
    ++time;
    hours = ((time/360000) % 12) + 1;
    minutes = (time%360000)/6000;
    seconds = ((time%360000)%6000)/100;
    decimals = (((time%360000)%6000)%100);
    if (time % 50 == 0) {
        colons = (colons + 1) % 2;
    }
    if (time % 72000 == 0) {
        AMPM = (AMPM + 1) % 2;
    }
}