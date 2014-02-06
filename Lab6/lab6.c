/* 	Lab 6  Nathaniel Wendt
		SPI Serial data transfer to lights from keypad
		SPI Serial data transfer from switches to LCD
*/
typedef unsigned char INT8U;
typedef signed char INT8S;
typedef unsigned int INT16U;
typedef int bool;

INT8U kp(INT8U);
void dsp(INT8U,INT8U,INT8U);
void delay(void);
void LCDprint(INT8U *sptr);
void spi_ini(void);
void showkey(INT8U oldkey);


void main(void)
{
#define DDRD *(unsigned char volatile *)(0x1009)
#define SPCR *(unsigned char volatile *)(0x1028)
#define SPSR *(unsigned char volatile *)(0x1029)
#define SPDR *(unsigned char volatile *)(0x102A)
#define ENABLE *(unsigned char volatile *)(0xB580)
#define DISABLE *(unsigned char volatile *)(0xB590)
INT8U clear, newkey, newswitch, oldswitch, oldkey=0;
	clear = kp(1);
	dsp(99,0,0);					//initialize display
	dsp(2, 0, 0);					//initialize display to start at position 0,0
	oldswitch = 0;
	while(1) {
		clear = kp(1);			//initialize the keypad
    newkey = kp(0);			//get new key value from user
		spi_ini();					//must initialize SPI after keypad is initialized
		showkey(oldkey);		//show old key again since it has been cleared after from keypad reading
		newswitch = SPDR;		//get a new switch value
		if(newkey != 0)			//if an actual key is depressed
      oldkey = newkey;
		if(newswitch != oldswitch){  //if a new switch value is chosen
			displayswitch(newswitch);
			oldswitch = newswitch;
		}
  }
}

//Shows the keypad value on the lights
//Extra manipulation must be done
//param oldkey - key value to be stored to the SPDR
void showkey(INT8U oldkey)
{
	if(oldkey == 0x2A)
		oldkey = 0x0E;
	else if(oldkey == 0x23)
		oldkey = 0x0F;
	else if(oldkey > 0x39){
		oldkey &= 0x0F;
		oldkey = oldkey + 0x09;
	}
	SPDR = oldkey;
	while(!(SPSR & 0x80));
}

//Displays the switch values on the LCD
//param newswitch - switch value to be shown on LCD
void displayswitch(INT8U newswitch)
{
	newswitch &= 0x0F;
	newswitch = ~newswitch;
	newswitch = newswitch + 64;
	if(newswitch > 57)
		newswitch = newswitch + 7;
	dsp(2,0,0);
	dsp(0,newswitch,0);
}

//Initializes the DDRD and the SPI
//Must set the SPI control register as well
void spi_ini(void)
{
	DDRD = 0x3A;
	SPCR = 0x50;
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