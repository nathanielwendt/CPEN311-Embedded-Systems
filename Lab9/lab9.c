/* 	Lab 9  Nathaniel Wendt
		This lab is a digital signal analyzer that determines period, frequency and duty cycle.
		It also determines the high, low, and average level of the voltage.
*/
#include <stdio.h>

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
void initialize_hardware(void);
void display_period(INT16U period);
INT16U measure_period();
INT16U measure_ontime();
void display_ontime(INT16U period);
void display_measurement(INT16U measure, INT8U line);
void output_waveform();
void voltage_source();
void output_switches_waveform(INT8U time);
INT8U get_voltage_from_sample(INT16U *highestv, INT16U *lowestv, INT16U *average);

void main(void)
{
#define SWITCHES *(unsigned char volatile *)(0xB580)
#define PORTA *(unsigned char volatile *)(0x1000)
#define TCTL1 *(unsigned char volatile *)(0x1020)
#define DDRD *(unsigned char volatile *)(0x1009)
#define SPCR *(unsigned char volatile *)(0x1028)
#define TCTL2 *(unsigned char volatile *)(0x1021)
#define TMSK2 *(unsigned char volatile *)(0x1024)
#define TIC1 *(unsigned int volatile *)(0x1010)



#define OPTION *(unsigned char volatile *)(0x1039)
#define ADCTL *(unsigned char volatile *)(0x1030)
#define ADR1 *(unsigned char volatile *)(0x1031)
#define ADR2 *(unsigned char volatile *)(0x1032)
#define ADR3 *(unsigned char volatile *)(0x1033)
#define ADR4 *(unsigned char volatile *)(0x1034)
#define TOC2 *(unsigned int volatile *)(0x1018)
#define TCNT *(unsigned int volatile *)(0x100E)
#define TFLG1 *(unsigned char volatile *)(0x1023)
INT16U v1,v2,average,ontime,period,duty,freq;
INT8U voltage, disphigh, displow;
	initialize_hardware();
	display_titles();
	while(1)
	{
		get_voltage_from_sample(&v1, &v2, &average);
		v1 = 2.2*v1;
		v2 = 2.2*v2;
		average = 2*average;
		
		//display average
		dsp(2,3,15);
		dsp(0,((average / 100) %10) + 0x30,0);
		dsp(2,3,17);
		dsp(0,((average / 10) %10) + 0x30,0);
		dsp(0,((average / 1) %10) + 0x30,0);
		
		//display hight voltage
		dsp(2,2,3);
		dsp(0,((v1 / 100) %10) + 0x30,0);
		dsp(2,2,5);
		dsp(0,((v1 / 10) %10) + 0x30,0);
		dsp(0,((v1 / 1) %10) + 0x30,0);
		
		//display low voltage
		dsp(2,3,3);
		dsp(0,((v2 / 100) %10) + 0x30,0);
		dsp(2,3,5);
		dsp(0,((v2 / 10) %10) + 0x30,0);
		dsp(0,((v2 / 1) %10) + 0x30,0);
	
		if(v1 > 350)
		{
			period = measure_period();
			ontime = measure_ontime();
			
			//display duty cycle
			duty = (ontime*10)/(period/10);
			dsp(2,2,15);
			dsp(0,((duty / 100) %10) + 0x30,0);
			dsp(0,((duty / 10) %10) + 0x30,0);
			dsp(0,((duty / 1) %10) + 0x30,0);
			
			period /= 20;
			ontime /= 20;
			
			//display frequency
			freq = 100000/period;
			dsp(2,1,10);
			dsp(0,((freq / 1000) %10) + 0x30,0);
			dsp(0,((freq / 100) %10) + 0x30,0);
			dsp(0,((freq / 10) %10) + 0x30,0);
			dsp(0,((freq / 1) %10) + 0x30,0);
		}
	}
}

INT8U get_voltage_from_sample(INT16U *highestv, INT16U *lowestv, INT16U *average)
{
INT8U result[200];
INT8U i;
INT16U sum;
	
	OPTION &= 0xBF;
	OPTION |= 0x80;
	delay();
	TFLG1 = 0x40;
	TOC2 = TCNT + 200;		
	while(!(TFLG1 & 0x40));
	ADCTL = 0x00;
	
	sum = 0;
	for(i = 0; i < 200; i++){
		while(!(ADCTL & 0x80));
		ADCTL = 0x00;
		result[i] = ADR1;
		sum += ADR1;
	}
	*highestv = result[0]; *lowestv = result[0];
	for(i = 0; i < 200; i++){
		if(result[i] > *highestv)
			*highestv = result[i];
		if(result[i] < *lowestv)
			*lowestv = result[i];	
	}
	
	*average = sum / 200;
}

//simulates a voltage source by reading in a value from the switches
//and outputting the corresponding voltage through the PA6
void voltage_source()
{
INT8U basetime,time,oldswitch,newswitch;
	
	oldswitch = ~SWITCHES-124;
	newswitch = oldswitch;
	basetime = 40;
	time = 40 * oldswitch;
	delay();
	PORTA |= 0x40;
	TCTL1 = 0x40;
	TOC2 = TCNT + 2 * (basetime + time);
	TFLG1 = 0x40;
	while(oldswitch == newswitch)
	{
		while(!(TFLG1 & 0x40));
		TFLG1 = 0x40;
		TOC2 = TOC2 + 2 * (200 - basetime - time);
		while(!(TFLG1 & 0x40));
		TFLG1 = 0x40;
		TOC2 = TOC2 + 2 * (basetime + time);
		newswitch = ~SWITCHES-124;
	}
}

//outputs a 5KHz waveform that has a 25% duty cycle
void output_waveform()
{
	PORTA |= 0x40;
	TCTL1 = 0x40;
	TOC2 = TCNT + 100;
	TFLG1 = 0x40;
	while(1)
	{
		while(!(TFLG1 & 0x40));
		TFLG1 = 0x40;
		TOC2 = TOC2 + 300;
		while(!(TFLG1 & 0x40));
		TFLG1 = 0x40;
		TOC2 = TOC2 + 100;
	}
}

//measures the ontime of a waveform in order to calculate period
INT16U measure_ontime()
{
INT16U temp;
	TCTL2 = 0x10;
	TFLG1 = 0x04;
	while(!(TFLG1 & 0x04));
	TFLG1 = 0x04;
	temp = TIC1;
	TCTL2 = 0x20;
	while(!(TFLG1 & 0x04));
	//display_measurement(TIC1 - temp,1);
	return (TIC1 - temp);
}

//measures the period of an input waveform
INT16U measure_period()
{
INT16U edge,period;
	TFLG1 = 0x04;
	while(!(TFLG1 & 0x04));
	edge = TIC1;
	TFLG1 = 0x04;
	while(!(TFLG1 & 0x04));
	period = TIC1 - edge;
	if(edge < TIC1)
	{
		display_measurement(period,0);
		return period;
	}
		
}

//performs the business logic on the measurements in order to 
//display them on the LCD screen.
void display_measurement(INT16U measure, INT8U line)
{
INT16U savemeasure;
	measure /= 20;
	savemeasure = measure;
	measure /= 1000;
	measure %= 10;
	measure += 0x30;
	dsp(2,line,9);
	dsp(0,measure,0);
	
	measure = savemeasure;
	measure /= 100;
	measure %= 10;
	measure += 0x30;
	dsp(2,line,10);
	dsp(0,measure,0);
	
	measure = savemeasure;
	measure /= 10;
	measure %= 10;
	measure += 0x30;
	dsp(2,line,12);
	dsp(0,measure,0);
	
	measure = savemeasure;
	measure /= 1;
	measure %= 10;
	measure += 0x30;
	dsp(2,line,13);
	dsp(0,measure,0);
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
	LCDprint("Freq:");
	dsp(2,1,11);
	LCDprint(".");
	dsp(2,1,14);
	LCDprint(" hz");
	
	dsp(2,2,0);
	LCDprint("HV: ");
	dsp(2,2,4);
	LCDprint(".");
	dsp(2,2,7);
	LCDprint("V");
	
	dsp(2,3,0);
	LCDprint("LV: ");
	dsp(2,3,4);
	LCDprint(".");
	dsp(2,3,7);
	LCDprint("V");
	
	dsp(2,3,10);
	LCDprint("AVG:");
	dsp(2,3,16);
	LCDprint(".");
	dsp(2,3,19);
	LCDprint("V");
	
	dsp(2,2,11);
	LCDprint("DC:");
	dsp(2,2,18);
	LCDprint("%");
}

//initializes hardware
void initialize_hardware(void)
{
	dsp(99,0,0);
	kp(1);
	TMSK2 = TMSK2 & 0xFC;  //set clock to 2MHz - .5 us period
	TCTL2 = 0x10;  			//set to read rising edge
	TFLG1 = 0x04;				//
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