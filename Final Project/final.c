/* 	Final Lab -- Nathaniel Wendt
		This project measures resistance and capacitance by a user placing those components
		in their respective terminals.
*/

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
void initialize_hardware(void);
void spi_ini(void);
void send_voltage(INT16U voltage);
void display_measurement(INT16U measure);
void measure_resistance();

void main(void)
{
#define SPCR *(unsigned char volatile *)(0x1028)
#define SPSR *(unsigned char volatile *)(0x1029)
#define SPDR *(unsigned char volatile *)(0x102A)
#define PORTA *(unsigned char volatile *)(0x1000)
#define DDRD *(unsigned char volatile *)(0x1009)
#define SPCR *(unsigned char volatile *)(0x1028)
#define TCTL2 *(unsigned char volatile *)(0x1021)
#define TMSK2 *(unsigned char volatile *)(0x1024)
#define TFLG1 *(unsigned char volatile *)(0x1023)
#define TIC1 *(unsigned int volatile *)(0x1010)
INT8U i, resistor, key, det;
INT16U iterator, volts;
	initialize_hardware();
	while(1)
	{
		measure_capacitance();
		measure_resistance();
	}
}

//measures the resistance of a component placed in the resistor terminal
void measure_resistance()
{
INT16U iterator, volts;
	send_voltage(0x4000);
	delayslow();
	iterator = 0x4000;
	while (iterator < 0x4FFF && !(PORTA & 0x04))
	{
		send_voltage(iterator);
		iterator = iterator + 1;
	}
	
	dsp(2,0,0);
	//displays the voltage on the LCD display
	if(iterator != 0x4000 && iterator != 0x4FFF)
	{
		iterator &= 0x0FFF; //remove first bit
		volts = (183619) / (iterator - 1501);  //calibration settings for hardware
		if(volts == 52547){ //if no component is in the terminal
			LCDprint("N/A  ");
		}
		else{
			display_measurement(volts);
		}
	}
}

//measures the capacitance of a component placed in the capacitor terminal
void measure_capacitance()
{
INT8U count;
INT16U timecount;
		PORTA &= ~(0x20);
		count = 1;
		timecount = 0;
		PORTA |= 0x20;
		
		while(!(PORTA & 0x02)){
			timecount++;
		}
		PORTA &= ~(0x20);
		dsp(2,1,0);
		timecount = timecount * 4;
		if(timecount <= 12) //if no component is in the terminal
			LCDprint("N/A  ");
		else{
			display_measurement(timecount);
		}
		while(count){
			delayslow();
			count--;
		}
}

//displays the given 16 bit measurement on the LCD screen in 5 digits
void display_measurement(INT16U measure)
{
INT8U lastflag,i;
INT16U divide;
	lastflag = 0;
	divide = 10000;
	
	for(i = 0; i < 5; i++){
		if(lastflag == 1 || ((measure / divide) % 10) != 0){
			dsp(0,((measure / divide) % 10) + 0x30,0);
			lastflag = 1;
		} else {
			dsp(0,32,0);
		}
		divide /= 10;
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
	//PORTA |= 0x20;  //set CS to high
}

//Initializes the DDRD and the SPI
//Must set the SPI control register as well
void spi_ini(void)
{
	DDRD = 0x3A;
	SPCR = 0x54;
}

//initializes hardware
void initialize_hardware(void)
{
	dsp(99,0,0);
	spi_ini();
	dsp(2,0,6);
	LCDprint("Ohms");
	dsp(2,0,0);
	dsp(2,1,6);
	LCDprint("nF");
}

// LCD Display String
void LCDprint(INT8U *sptr)
{
    while( *sptr ){
        dsp(0,*sptr,0);
        ++sptr;
		}
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