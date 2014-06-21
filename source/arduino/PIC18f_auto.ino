// I wrote it according to the following datasheet:
// http://ww1.microchip.com/downloads/en/DeviceDoc/39622L.pdf
#include <stdio.h>
//pin out config
#define PGC 11
#define PGD 12
//#define EN 7 //PGM & MCLR
#define PGM 13
#define MCLR 9

#define P9 1
#define P10 5
#define P15 1
#define P12 1

String inputString = "";
String outString = "";
boolean stringComplete = false;

unsigned long temp;

byte buffer[32];
byte address[3];

void setup()
{
	Serial.begin(9600);
	pinMode(PGC,INPUT); //just in case a program is running these pins high before reset is pulled low
	pinMode(PGD,INPUT);
	pinMode(PGM,INPUT);
	pinMode(MCLR,OUTPUT);
	 
	inputString.reserve(200);
	outString.reserve(20);

}

void loop()
{ 
	
	//turn on the chip (disable programming mode & diable reset)
	if (Serial.available())
	{
		prog_start();
		serialEvent();

	} 
}

void prog_start()
{
	digitalWrite(MCLR, LOW);
	delay(1); //just in case
	pinMode(PGC,OUTPUT);
	pinMode(PGD,OUTPUT);
	pinMode(PGM,OUTPUT);
	digitalWrite(PGM,HIGH);
	delay(P15);
	digitalWrite(MCLR,HIGH);	 
}

void prog_end()
{
	digitalWrite(MCLR, LOW);
	delay(1); //just in case
	pinMode(PGC,INPUT); //just in case a program is running these pins high before reset is pulled low
	pinMode(PGD,INPUT);
	pinMode(PGM,INPUT);
	 
}
//////THE MAIN CODE///////////
//////////////////////////////
//////////////////////////////
void serialEvent() 
{

	while (Serial.available()) 
	{
		char inChar = (char)Serial.read(); 
		
		if (inChar == 'X') //buffering till X
		{ 
			stringComplete = true;
			break;
		} 
		
		inputString += inChar;	
	}

	if (stringComplete && inputString.charAt(0) == 'W') //WRITE
	{ 
		if(inputString.length() != 21) 
		{
			Serial.print("input is ");
			Serial.print(inputString.length());
			Serial.println("Input is wrong, should be: W<address - 4 digit><16bytes>X"); 
			nullString();
		}
		else
		{
			
			address[2] = 0;
			address[1] = char2byte(inputString.charAt(2),inputString.charAt(1));
			address[0] = char2byte(inputString.charAt(4),inputString.charAt(3));
							 
			nullBuffer(); // set everything FF
			
			for(int i=0;i<8;i++)
			{
				buffer[i] = char2byte(inputString.charAt(2*i+6),inputString.charAt(2*i+5));
			} 

			nullString();
			
			// print address
			//Serial.print("Programming 32bytes,starting at:");
			//Serial.print("0x00");
			//if(address[1]<0x10) Serial.print("0"); Serial.print(address[1],HEX);
			//if(address[0]<0x10) Serial.print("0"); Serial.println(address[0],HEX);
			
			//for(int i=0;i<32;i++) { Serial.print(buffer[i],HEX); }
				
			//Serial.println("");
			
			///Write
			
			delay(1);
	
			programBuffer(address[2],address[1],address[0]);
			
	
			
			//Serial.println("Programming complety"); 
		}	 
	}
	
	if (stringComplete && inputString.charAt(0) == 'E') //Erase all
	{ 
		 digitalWrite(PGM,HIGH);
		 digitalWrite(MCLR,HIGH);
		 delay(1);
		 
		 ////erase 
		 erase_all();

		 //Serial.println("Erase complety");
		 
		 nullString();
	}
		
	if (stringComplete && inputString.charAt(0) == 'R')//READ
	{ 
		address[2] = char2byte(inputString.charAt(2),inputString.charAt(1));
		address[1] = char2byte(inputString.charAt(4),inputString.charAt(3));
		address[0] = char2byte(inputString.charAt(6),inputString.charAt(5));
		//read
			
		temp=0;

		temp = ((long)address[2])<<(16); //doesn't work with out (long)
		temp |= ((long)address[1])<<(8);
		temp |= (long)address[0];
	
		digitalWrite(PGM,HIGH);
		digitalWrite(MCLR,HIGH);
		delay(1);
	
		for(int i=0;i<16;i++)
		{
			address[2] = byte( (temp&0xFF0000)>>16 );
			address[1] = byte( (temp&0xFF00)>>8 );
			address[0] = byte( temp&0xFF );
			byte what = readFlash(address[2],address[1],address[0]);
			switch(what)
			{
				case 0x00: Serial.print("00"); break;
				case 0x01: Serial.print("01"); break;
				case 0x02: Serial.print("02"); break;
				case 0x03: Serial.print("03"); break;
				case 0x04: Serial.print("04"); break;
				case 0x05: Serial.print("05"); break;
				case 0x06: Serial.print("06"); break;
				case 0x07: Serial.print("07"); break;
				case 0x08: Serial.print("08"); break;
				case 0x09: Serial.print("09"); break;
				case 0x0A: Serial.print("0A"); break;
				case 0x0B: Serial.print("0B"); break;
				case 0x0C: Serial.print("0C"); break;
				case 0x0D: Serial.print("0D"); break;
				case 0x0E: Serial.print("0E"); break;
				case 0x0F: Serial.print("0F"); break;
				default: Serial.print(what,HEX);
			}				 
						

			temp++;
		}
			

			
			nullString();
			 
	}
	
	if (stringComplete && inputString.charAt(0) == 'C') //config
	{ 
		
		digitalWrite(PGM,HIGH);
		digitalWrite(MCLR,HIGH);
		delay(1);
		
		configWrite( char2byte(inputString.charAt(1),'0'),char2byte(inputString.charAt(3),inputString.charAt(2)) );
		

		nullString();
		
	}
	
	if (stringComplete && inputString.charAt(0) == 'D') // report pic type
	{ 
		if( checkIf_pic18f2550() )
		{
			delay(100); Serial.print("T");
		}
		else 
		{
			delay(100); Serial.print("F");
		}
		nullString();
		
	}
	
	//clear string incase the first CHAR isn't E,R,W
	if(inputString.charAt(0) != 'E' && inputString.charAt(0) != 'R' && inputString.charAt(0) != 'W' && inputString.charAt(0) != 'C' && inputString.charAt(0) != 'D') nullString();
	
}

byte readFlash(byte usb,byte msb,byte lsb)
{
	
	byte value=0;
	
	send4bitcommand(B0000);
	send16bit( 0x0e00 | usb ); //
	send4bitcommand(B0000);
	send16bit(0x6ef8);
	
	send4bitcommand(B0000);
	send16bit( 0x0e00 | msb ); //
	send4bitcommand(B0000);
	send16bit(0x6ef7);
	
	send4bitcommand(B0000);
	send16bit( 0x0e00 | lsb ); //
	send4bitcommand(B0000);
	send16bit(0x6ef6); 
	
	send4bitcommand(B1001); //
	
	pinMode(PGD,INPUT); digitalWrite(PGD,LOW);
	
	for(byte i=0;i<8;i++) //clock shift register
	{ 
		digitalWrite(PGC,HIGH);
		digitalWrite(PGC,LOW);
	}
	
	for(byte i=0;i<8;i++) //shift out
	{
		digitalWrite(PGC,HIGH); 
		digitalWrite(PGC,LOW);
		if(digitalRead(PGD) == HIGH) value += 1<<i; //sample PGD
	} 
	
	return value;
 
}

void send4bitcommand(byte data)
{
	pinMode(PGD,OUTPUT);
	for(byte i=0;i<4;i++)
	{
		if( (1<<i) & data ) digitalWrite(PGD,HIGH); else digitalWrite(PGD,LOW); 
		digitalWrite(PGC,HIGH);
		digitalWrite(PGC,LOW);
	}

}

void send16bit(unsigned int data)
{
	pinMode(PGD,OUTPUT);
	for(byte i=0;i<16;i++)
	{
		if( (1<<i) & data ) 
			digitalWrite(PGD,HIGH); 
		else 
			digitalWrite(PGD,LOW); 
			
		digitalWrite(PGC,HIGH);
		digitalWrite(PGC,LOW);
	}
}

void erase_all() //for some reason the chip stops respone, just reconnect voltage
{ 
	send4bitcommand(B0000);
	send16bit(0x0e3c); //
	send4bitcommand(B0000);
	send16bit(0x6ef8);
	
	send4bitcommand(B0000);
	send16bit(0x0e00); //
	send4bitcommand(B0000);
	send16bit(0x6ef7);
	
	send4bitcommand(B0000);
	send16bit(0x0e04); //
	send4bitcommand(B0000);
	send16bit(0x6ef6);
	
	send4bitcommand(B1100);
	send16bit(0x0080);
	
	send4bitcommand(B0000); //
	send16bit(0x0000); //
	
	send4bitcommand(B0000); //
	digitalWrite(PGD,LOW);
	
	delay(10);
	delayMicroseconds(5);
	send16bit(0x0000); //
}

void configWrite(byte address,byte data) //see page 19 of http://ww1.microchip.com/downloads/en/DeviceDoc/39592f.pdf
{ 
	send4bitcommand(B0000); //direct access to configuration memory 
	send16bit( 0x8ea6 );
	send4bitcommand(B0000);
	send16bit( 0x8ca6 );	
	
	send4bitcommand(B0000); // position the program counter
	send16bit( 0xEF00 );
	send4bitcommand(B0000);
	send16bit( 0xF800 );	
	
	send4bitcommand(B0000);
	send16bit( 0x0e30 ); //
	send4bitcommand(B0000);
	send16bit(0x6ef8);
	
	send4bitcommand(B0000);
	send16bit( 0x0e00 ); //
	send4bitcommand(B0000);
	send16bit(0x6ef7);
	
	send4bitcommand(B0000);
	send16bit( 0x0e00 | address ); //

	//Serial.println("programming");
	
	if(address&0x01) 
	{
		send4bitcommand(B0000);
		send16bit(0x6ef6); 
		send4bitcommand(B1111);
		send16bit( 0x0000 | data<<8 ); // if even MSB ignored, LSB read 
	}
	else 
	{
		 send4bitcommand(B0000);
		 send16bit(0x2af6); 
		 send4bitcommand(B1111);
		 send16bit( 0x0000 | data ); // if odd MSB read, LSB ignored
	}
	digitalWrite(PGC,HIGH);
	digitalWrite(PGC,LOW);
	
	digitalWrite(PGC,HIGH);
	digitalWrite(PGC,LOW);
	
	digitalWrite(PGC,HIGH);
	digitalWrite(PGC,LOW);	

	digitalWrite(PGC,HIGH);
	delay(1);
	digitalWrite(PGC,LOW);
	delayMicroseconds(100);
	
	send16bit(0x0000); 
	Serial.print("R");
}
	
void programBuffer(byte usb,byte msb,byte lsb)
{ 
	//page 13 of http://ww1.microchip.com/downloads/en/DeviceDoc/39592f.pdf
	//step 1: Direct access to code memory
	send4bitcommand(B0000); 
	send16bit(0x8ea6);
	send4bitcommand(B0000);
	send16bit(0x9ca6);
	
	//step 2: Load write buffer for panel 1
	send4bitcommand(B0000);
	send16bit( 0x0e00 | usb ); // 
	send4bitcommand(B0000);
	send16bit(0x6ef8);
	
	send4bitcommand(B0000);
	send16bit( 0x0e00 | msb ); //
	send4bitcommand(B0000);
	send16bit(0x6ef7);
	
	send4bitcommand(B0000);
	send16bit( 0x0e00 | lsb); //
	send4bitcommand(B0000);
	send16bit(0x6ef6);
		
	for(byte i=0;i<3;i++)	//loads first 6 bytes <LSB><MSB>
	{	
		send4bitcommand(B1101);
		send16bit( buffer[(2*i)+1]<<8 | buffer[(i*2)] );	
	} 

	send4bitcommand(B1111);	//loads last 2 bytes and starts programming
	send16bit( buffer[7]<<8 | buffer[6] );	
	
	//nop    DS39592F-page 14
	digitalWrite(PGC,HIGH);
	digitalWrite(PGC,LOW);
	
	digitalWrite(PGC,HIGH);
	digitalWrite(PGC,LOW);
	
	digitalWrite(PGC,HIGH);
	digitalWrite(PGC,LOW);	

	digitalWrite(PGC,HIGH);
	delay(P9);
	digitalWrite(PGC,LOW);
	delayMicroseconds(P10);
	
	send16bit(0x0000); 
	Serial.print("R");
	
	//done	
}

/////////////////////////////
//nothing special underhere:

void nullString()
{
	inputString = "";
	stringComplete = false; 
}
	
void nullBuffer()
{
	for(byte i=0;i<8;i++) 
	{
		buffer[i] = 0xFF;
	}
}
	
byte char2byte(char lsb,char msb)
{
	
	byte result=0;
	
	switch(lsb)
	{
		case '0': result = 0; break;
		case '1': result = 1; break;
		case '2': result = 2; break;
		case '3': result = 3; break;
		case '4': result = 4; break;
		case '5': result = 5; break;
		case '6': result = 6; break;
		case '7': result = 7; break;
		case '8': result = 8; break;
		case '9': result = 9; break;
		case 'A': result = 0xA; break;
		case 'B': result = 0xB; break;
		case 'C': result = 0xC; break;
		case 'D': result = 0xD; break;
		case 'E': result = 0xE; break;
		case 'F': result = 0xF; break;
	}
	
	switch(msb)
	{
		case '0': result |= 0<<4; break;
		case '1': result |= 1<<4; break;
		case '2': result |= 2<<4; break;
		case '3': result |= 3<<4; break;
		case '4': result |= 4<<4; break;
		case '5': result |= 5<<4; break;
		case '6': result |= 6<<4; break;
		case '7': result |= 7<<4; break;
		case '8': result |= 8<<4; break;
		case '9': result |= 9<<4; break;
		case 'A': result |= 0xA<<4; break;
		case 'B': result |= 0xB<<4; break;
		case 'C': result |= 0xC<<4; break;
		case 'D': result |= 0xD<<4; break;
		case 'E': result |= 0xE<<4; break;
		case 'F': result |= 0xF<<4; break;
	}
	
	return result;
	
}

int checkIf_pic18f2550()
{
	digitalWrite(PGM,HIGH);
	digitalWrite(MCLR,HIGH);
	delay(1);

	if( readFlash(0x3f,0xff,0xfe) == 0xC7 )
		return 1;
	else
		return 0;
}



