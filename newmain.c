/*********************************************************************************************************************
 *
 * FileName:        main.c
 * Processor:       PIC18F2550 / PIC18F2553
 * Compiler:        MPLAB® XC8 v2.00
 * Comment:         Main code
 * Dependencies:    Header (.h) files if applicable, see below
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author                       Date                Version             Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * TODO                         Date                Finished
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *********************************************************************************************************************/

/*
 * Includes
 */
#include <xc.h>
#include <pic18f2550.h>

/*
 * Prototypes
 */
void __interrupt (high_priority) high_ISR(void);   //high priority interrupt routine
void __interrupt (low_priority) low_ISR(void);  //low priority interrupt routine, not used in this example
void initChip(void);
void initTimer(void);
unsigned int timerValue = 0;
/*
 * Global Variables
 */

unsigned int pwm1 = 800+64511;
unsigned int pwm2 = 800+64511;
unsigned int pwm3 = 800+64511;
unsigned int pwm4 = 800+64511;
unsigned char SPIL = 0;
unsigned char SPIH = 0;
unsigned char read = 0;
#define motor1 LATBbits.LATB4
#define motor2 LATBbits.LATB5
#define motor3 LATBbits.LATB6
#define motor4 LATBbits.LATB7

#define _XTAL_FREQ 48000000
/*
 * Interrupt Service Routines
 */
/********************************************************* 
	Interrupt Handler
**********************************************************/
void __interrupt (high_priority) high_ISR(void)
{
	if(INTCONbits.TMR0IF == 1)
     {
      /*  motor1 = 1;
        motor2 = 1;
        motor3 = 1;
        motor4 = 1;*/
        
        TMR0H = 0xFB;
        TMR0L = 0xFF;
        INTCONbits.TMR0IF=0;     //CLEAR interrupt flag when you are done!!!
     } 
    
    if(PIR1bits.SSPIF == 1){
        //SPI stuff
        
        read = SSPBUF;
        if(read & 128){                 //first char
            if(SPIL!=0) SPIH = read; 
        }else{                          //second char    
            SPIL = read;
        }
        PIR1bits.SSPIF = 0;  
    }
}

/*
 * Functions
 */
 /*************************************************
			Main
**************************************************/
void main(void)
{
    initChip();
    initTimer();
    while(1)    //Endless loop
    {
        T0CONbits.TMR0ON = 0;
        timerValue=TMR0L;
        timerValue|=TMR0H*256U;
        //timerValue = timerValue - 64511;
        T0CONbits.TMR0ON = 1;
        
        if(timerValue > pwm1){
            motor1 = 0;
        }
        if(timerValue > pwm2){
            motor2 = 0;
        }
        if(timerValue > pwm3){
            motor3 = 0;
        }
        if(timerValue > pwm4){
            motor4 = 0;
        }
        
        if(timerValue < pwm1){
        motor1 = 1;
        }
        
        if(timerValue < pwm2){
        motor2 = 1;
        }
        if(timerValue < pwm3){
        motor3 = 1;
        }
        if(timerValue < pwm4){
        motor4 = 1;
        }
        
        if(SPIL!=0 && SPIH!=0){                 //update pwm
            char motorIDL = SPIL & 96;
            char motorIDH = SPIH & 96;
            if(motorIDL==motorIDH){             //motor id check
                if(motorIDL==0){                    //motor 1
                    pwm1=((SPIL&31) | (SPIH&31)*32) + 64511;
                }else if(motorIDL==32){       //motor 2
                    pwm2=((SPIL&31) | (SPIH&31)*32) + 64511;
                }else if(motorIDL==64){             //motor 3
                    pwm3=((SPIL&31) | (SPIH&31)*32) + 64511;
                }else{                              //motor 4
                    pwm4=((SPIL&31) | (SPIH&31)*32) + 64511;
                }
            }                           //motor id check failed (wrong data)
            SPIL = 0;    
            SPIH = 0;
        }   
    }
        
}

/*************************************************
			Initialize the CHIP
**************************************************/
void initChip(void)
{
    LATA = 0x00; //Initial PORTA
    TRISA = 0xFF; //Define PORTA as input
    ADCON1 = 0x0F; //Turn off ADcon
    CMCON = 0x07; //Turn off Comparator
    LATB = 0x00; //Initial PORTB
    TRISB = 0x00; //Define PORTB as output
    LATC = 0x00; //Initial PORTC
    TRISC = 0x00; //Define PORTC as output
	INTCONbits.GIE = 0;	// Turn Off global interrupt
    
    TRISCbits.TRISC7 = 0;      //SDO
    TRISBbits.TRISB0 = 1;      //SDI
    TRISBbits.TRISB1 = 1;      //SCK
    TRISAbits.TRISA5 = 1;      //SS
    SSPSTAT = 0b01000000;       //CKE = 1
    SSPCON1 = 0b00100100;       //CKP = 0
    PIE1bits.SSPIE = 1;
    INTCONbits.GIE = 1;
    
}

/*************************************************
			Initialize the TIMER
**************************************************/
void initTimer(void)
{
    T0CON =0b00000010;        //Timer0 Control Register
               		//bit7 "0": Disable Timer = 0 means the timer is not disabled
               		//bit6 "1": 8-bit timer = 1 Means the timer is configured as an 8 bit timer
               		//bit5 "0": Internal clock = 0 Means the clock comes from the  internal instruction cycle clock
               		//bit4 "0": not important in Timer mode = 0
               		//bit3 "0": Timer0 prescale is assigned = 0 -> 0 Means prescaler comes from prescaler output (bits below)
               		//bit2-0 "111": Prescale 1:256  = 111 means the prescaler is 1:256
    /********************************************************* 
	     Calculate Timer 
             F = Fosc/(4*Prescale*number of counting)
	**********************************************************/

    TMR0H = 0xFB;
    TMR0L = 0xFF;    //Initialize the timer value
    

    /*Interrupt settings for Timer0*/
    INTCON= 0x20;   /*Interrupt Control Register
               		//bit7 "0": Global interrupt Enable = 0 
               		//bit6 "0": Peripheral Interrupt Enable = 0
               		//bit5 "1": Enables the TMR0 overflow interrupt = 1
               		//bit4 "0": Disables the INT0 external interrupt = 0
               		//bit3 "0": Disables the RB port change interrupt = 0
               		//bit2 "0": TMR0 Overflow Interrupt Flag bit = 0
                    //bit1 "0": INT0 External Interrupt Flag bit = 0
                    //bit0 "0": RB Port Change Interrupt Flag bit = 0
                     */
    
    T0CONbits.TMR0ON = 1;  //Enable Timer 0
    INTCONbits.GIE = 1;    //Enable interrupt
}


