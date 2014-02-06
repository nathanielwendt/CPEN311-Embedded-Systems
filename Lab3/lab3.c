/* Lab 3: Nathaniel Wendt
		Reads a keypad entry and displays its 15 segment representation on the lights
*/
typedef unsigned char INT8U;
typedef signed char INT8S;
typedef unsigned int INT16U;

INT8U kp(INT8U);
void dsp(INT8U,INT8U,INT8U);
void delay(void);
void LCDprint(INT8U *sptr);
void initializeHardware();
void buildRefTable(INT8U addressMap[3][32]);
INT8U defineAddress(int col,INT8U hex);
INT8U findAsciiMapKey(INT8U key);

void main(void)
{
#define SWITCHES *(unsigned char volatile *)(0xB580)
#define LED *(unsigned char volatile *)(0xB590)
INT8U key, newkey, asciikey, initialkey;
INT8U addressMap[3][32];
INT8U asciiMap[16][4] = { //declare ascii map for characters '0' - 'F'
	{0x30,0x1F,0x11,0x1F},{0x31,0x00,0x00,0x1F},{0x32,0x17,0x15,0x1D},{0x33,0x15,0x15,0x1F},
	{0x34,0x1C,0x04,0x1F},{0x35,0x1D,0x15,0x17},{0x36,0x1F,0x15,0x17},{0x37,0x10,0x10,0x1F},
	{0x38,0x1F,0x15,0x1F},{0x39,0x1C,0x14,0x1F},{0x41,0x1F,0x14,0x1F},{0x42,0x1F,0x05,0x07},
	{0x43,0x1F,0x11,0x11},{0x44,0x07,0x05,0x1F},{0x45,0x1F,0x15,0x15},{0x46,0x1F,0x14,0x14},
};
	initializeHardware();
	buildRefTable(addressMap);
	newkey = 0x30;
	initialkey = kp(0);
	while(1){
		key = newkey;
		asciikey = findAsciiMapKey(key);
		while(key == newkey || newkey == initialkey){
			LED = addressMap[0][ asciiMap[asciikey][1] ];
			LED = addressMap[1][ asciiMap[asciikey][2] ];
			LED = addressMap[2][ asciiMap[asciikey][3] ];
			newkey = kp(0);
		}

	}
}

//converts an incoming ascii key into a table index on the ascii map
INT8U findAsciiMapKey(INT8U key)
{
	if(key == 0x2A)				//if input is a *
		key = 14;
	else if(key == 0x23)  //if input is a #
		key = 15;
	else if(key < 0x3A)   //if 0-9
		key = key - 0x30;
	else									//if A-F
		key = key - 0x37;
	return key;
}

//builds the reference table address map by calling define address for all
//possible combinations of lights
void buildRefTable(INT8U addressMap[3][32])
{
INT8U test;
int col;
INT8U hex;
	for(col=0; col<3; col++){
		for(hex = 0; hex <= 0x1F; hex++){
			addressMap[col][hex] = defineAddress(col,hex);
		}
	}
}

//Takes an incoming light position, col, and a hex representation on
//the 5 lights represented in that row and returns the effective address to be
//sent to the lights to display those characters
INT8U defineAddress(int col, INT8U hex)
{
INT8U coladdition;
INT8U rowaddition;
	coladdition = 0;
	if(col == 0)
		coladdition = 0x04;
	else if(col == 1)
		coladdition = 0x80;
	else if(col == 2)
		coladdition = 0x10;
	
	hex = ~hex;
	rowaddition = 0x00;
	if(hex & 0x10)
		rowaddition = rowaddition+0x40;
	if(hex & 0x08)
		rowaddition = rowaddition+0x08;
	if(hex & 0x04)
		rowaddition = rowaddition+0x01;
	if(hex & 0x02)
		rowaddition = rowaddition+0x20;
	if(hex & 0x01)
		rowaddition = rowaddition+0x02;
	
	return coladdition+rowaddition;
}

//Initializes the display and keypad
void initializeHardware()
{
	LED = 0x00;
	dsp(99,0,0);
	kp(1);					                         //initialize keypad
	dsp(2,2,0);
	LCDprint("LAB 4:");
	dsp(2,3,0);
	LCDprint("Nathaniel Wendt");
	delay();       														//delay shift so scroll speed is slower
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
   keyVal = 0;   /* display blank (space) */
   }
else {	                  /* scan keyboard */
   keyVal = 0;            /* default value for no key */
   rowSel = 0x04;         /* bit 2 selects row 0 */
   for(i=0; i<4; i++) {   /* scan 4 rows */
     PORTD &= 0xC3;       /* set bit of row # */
     PORTD |= rowSel;     
     //delay(); // 
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

void delay(void)
{
INT16U count = 10000; //increased count to delay shift accordingly
while(count) count--; //repeat to delay a significant number of times
}

