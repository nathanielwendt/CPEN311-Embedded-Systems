/* 	Lab 8  Nathaniel Wendt
		This lab is designed to implement an ohmeter that can determine the value of a
		resistor within 1% of the actual value.  This design implements a digital/analog
		converter, current buffer, and comparator circuit to implement the ohmeter.
*/
#include <stdio.h>

typedef unsigned char INT8U;
typedef signed char INT8S;
typedef unsigned int INT16U;
typedef int bool;

INT8U kp(INT8U);
void dsp(INT8U,INT8U,INT8U);
void delay(void);
void delayslow(void);
void LCDprint(INT8U *sptr);
void spi_ini(void);
void showkey(INT8U oldkey);
void initialize_hardware(void);
void spi_ini(void);
void send_voltage(INT16U voltage);


void main(void)
{
#define SWITCHES *(unsigned char volatile *)(0xB580)
#define PORTA *(unsigned char volatile *)(0x1000)
#define DDRD *(unsigned char volatile *)(0x1009)
#define SPCR *(unsigned char volatile *)(0x1028)
#define SPSR *(unsigned char volatile *)(0x1029)
#define SPDR *(unsigned char volatile *)(0x102A)
INT8U i, resistor, key;
INT16U iterator, volts;
	initialize_hardware();

	while(1)
	{
		initialize_hardware();
		send_voltage(0x4000);
		delayslow();
		iterator = 0x4000;
		while (iterator < 0x4FFF && !(PORTA & 0x04))
		{
			send_voltage(iterator);
			iterator = iterator + 1;
		}
		
		//displays the voltage on the LCD display
		if(iterator != 0x4000 && iterator != 0x4FFF)
		{
			iterator &= 0x0FFF; //remove first bit
			volts = (204022) / (iterator - 1501);
			dsp(0,((volts / 10000) % 10) + 0x30,0);
			dsp(0,((volts / 1000) % 10) + 0x30,0);
			dsp(0,((volts / 100) % 10) + 0x30,0);
			dsp(0,((volts / 10) % 10) + 0x30,0);
			dsp(0,((volts / 1) % 10) + 0x30,0);
			
			key = kp(0);		//get new key value to start over again
			while(key != 0x30){key = kp(0);};  //wait until new key to run measure again
		}
		
	}

}

//sends the given voltage value as bit values to the digital/analog converter
void send_voltage(INT16U voltage)
{
INT8U MS, LS;
	LS = voltage;
	MS = (voltage >> 8) & 0xff;

	PORTA |= 0x10;  //set FS to high 0C4
	PORTA &= ~0x20; //set CS to low  
	PORTA &= ~0x10; //set FS to low
	SPDR =  MS;          //0x4C;    //first 8 bits to shift   44-1   48-2
	while(!(SPSR & 0x80));
	SPDR =  LS;           //0x00;   //next 8 bits to shift
	while(!(SPSR & 0x80));
	PORTA |= 0x10;  //set FS to high
	PORTA |= 0x20;  //set CS to high
}

//Initializes the DDRD and the SPI
//Must set the SPI control register as well
void spi_ini(void)
{
	DDRD = 0x3A;
	SPCR = 0x54;
}

//Displays the titles on the LCD screen
void display_titles(void)
{
	dsp(2,0,0);
	LCDprint("Period:  ");
	dsp(2,0,11);
	LCDprint(".");
	dsp(2,0,14);
	LCDprint(" ms");
	
	dsp(2,1,0);
	LCDprint("On_Time: ");
	dsp(2,1,11);
	LCDprint(".");
	dsp(2,1,14);
	LCDprint(" ms");
}

//initializes hardware
void initialize_hardware(void)
{
	dsp(99,0,0);
	kp(1);
	spi_ini();
	dsp(2,0,6);
	LCDprint("Ohms");
	dsp(2,0,0);
}

// LCD Display String
void LCDprint(INT8U *sptr)
{
    while( *sptr ){
        dsp(0,*sptr,0);
        ++sptr;
		}
}

INT8U kp(INT8U ini)
/* keypad driver
       ini = 0 scans the keypad and returns a 
               ascii character if key depressed
               otherwise returns null character
             1 initializes the keypad I/O hardware
*/
#define PORTD   *(unsigned char volatile *)(0x1008)
#define DDRD    *(unsigned char volatile *)(0x1009)
#define PORTE   *(unsigned char volatile *)(0x100A)
{
INT8U i, rowNum, colNum, keyNum;
INT8U keyData, rowSel, keyVal;
static INT8U keyCode[16] = {'1','2','3','A','4','5',
           '6','B','7','8','9','C','*','0','#','D'};
if(ini) {
   DDRD = 0x3E;     /* initialize PD1-PD5 as outputs */
	 SPCR = 0x00;
   keyVal = 0;   /* display blank (space) */
   }
else {	                  /* scan keyboard */
   keyVal = 0;            /* default value for no key */
   rowSel = 0x04;         /* bit 2 selects row 0 */
   for(i=0; i<4; i++) {   /* scan 4 rows */
     PORTD &= 0xC3;       /* set bit of row # */
     PORTD |= rowSel;     
     delay(); // 
     keyData = PORTE & 0x0F; /* read row data */
     if(keyData) {           /* key depressed? */
       colNum = -1;          /* decode the */
       while(keyData) {           /* column number */
         keyData = keyData >> 1;  /*  by shifting */
         colNum++;                /*   bits to */
         }                        /*    the right */
       rowNum = 4*i;              /* row weight = 4 */   
       keyNum = rowNum + colNum;  /* position number */
       keyVal = keyCode[keyNum];  /* decode key char */
       break;
       }
     rowSel = rowSel << 1;  /* select next row bit */
     }
   }
   return keyVal;
}

void dsp(INT8U ctl,INT8U datar,INT8U datac)
/* controls the LCD based on value of ctl.
       ctl=  0 display character in datar  
             1 clear display & home cursor
             2 position cursor to row & column
            18 display shift left
            99 initialize the LCD display
*/
#define LCD_CMD  *(unsigned char volatile *)(0xB5F0)
#define LCD_DATA *(unsigned char volatile *)(0xB5F1)
{
   delay(); //while(LCD_CMD & 0x80); /* wait for display not busy */
   switch(ctl) {            /* command specified by d */
     case 0:  LCD_DATA = datar;
		delay(); //	
                break;
     case 1:  LCD_CMD = 0X01;   /* clear the screen */
		delay(); //	
                break;
     case 2:  switch(datar) {
                case 0: LCD_CMD = 0x80 + datac; break;
                case 1: LCD_CMD = 0xC0 + datac; break;
                case 2: LCD_CMD = 0x94 + datac; break;
                case 3: LCD_CMD = 0xD4 + datac; break;
                default:  break;
                }
     case 18: LCD_CMD = 0X18;   /* clear the screen */
		delay(); //	
                break;
     case 99: LCD_CMD = 0x3c;    /* display function */
              delay(); //while(LCD_CMD & 0x80);
              LCD_CMD = 0x0c;    /* disp on, curs & blink off */
              delay(); //while(LCD_CMD & 0x80);
              LCD_CMD = 0x06;    /* cursor incr. shift off */
              delay(); //while(LCD_CMD & 0x80);
              LCD_CMD = 0x01;    /* clear disp cursor home */
              break;
     default:   break;
     }
}

//delay function for keypad timing
void delay()
{
INT16U count = 1000;
while(count) count--;
}

void delayslow()
{
INT16U count = 100000;
while(count) count--;
}