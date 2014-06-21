/*
Copyright (C) 2012  kirill Kulakov

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "rs232.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <conio.h>

#define _CRT_SECURE_NO_WARNINGS
#define POLLING_DELAY 150
typedef unsigned char byte;

using namespace std;

//char *readpic(int address, int comPort);
int programing_fun(int cli_args_num, char*arguments[]);
int success(int comPort);
int portfind();

int main( int cli_args_num, char*arguments[])
{

	printf("Arduino eprom programmer\n");

	if(programing_fun(cli_args_num, arguments))
	{
		printf("Fatal error can not continue\n");
	}
	else 
	{
		printf("The chip was programmed successfully!\n");	
	}
	

	system("pause");

	return 0;

}

int programing_fun(int cli_args_num, char*arguments[])
{
	ifstream binfile;
	streampos size;
	char * memfile;
	char buf[4096]={0};
	int serial_bytes = 0;
	unsigned int address=0;
	int comPort = -1;
	int done = 0;
	int timeout = 0;


	//opening binary file
	if (cli_args_num < 2)		//if file path wasnt supplied via command line argument get it now
	{
		printf("Enter location of hex file: ");
	//	scanf("%4096[^\n]",buf);
		binfile.open("C:\\Users\\Chris\\Downloads\\uni-bios.rom", ios::binary | ios::in | ios::ate);
		//binfile.open(buf, ios::binary | ios::in | ios::ate);			//open specified file
		buf[0] = 0;
		if(!binfile.is_open())					//if file is not open there's an error		
		{
			printf("Can not open the file\n");	//tell the user about it 
			return 1;
		}	
		
		size = binfile.tellg();			//get size of file		
		memfile = new char[size];		//allocate memory for file
		binfile.seekg(0, ios::beg);		//go to beginning of file
		binfile.read(memfile, size);	//read entire file into memory
		binfile.close();

		comPort = portfind();
	}
	else
	{
		binfile.open(arguments[1], ios::binary | ios::in | ios::ate); //same as above but get file path from cli
		if (!binfile.is_open())     
		{
			printf("Can not open the file\n");
			return 1;
		}	

		size = binfile.tellg();					//read entire file into memory
		memfile = new char[size];
		binfile.seekg(0, ios::beg);
		binfile.read(memfile, size);
		binfile.close();

		if (cli_args_num >= 3)
			comPort = atoi(arguments[3]);
		else
			comPort = portfind();
	}

	

	if (comPort == -1)
		return 1;

	printf("---------------------------\n");
	printf("Connecting to the Arduino...\n");

	OpenComport(comPort, 256000);				//speed shouldnt matter since the 32u4 is emulating a serial connection over usb

	printf("Done!\n");


	//chip reset
	if( SendBuf(comPort,(unsigned char *)"B",strlen("B")) == -1 )  //B is for blank 
		return 1;
	printf("Verifying blank chip...");
										
	if (!success(comPort))		//if there was no success
	{
		printf("failed\n");
		return 1;
	}

	printf("PRAISE THE SUN!\n");							//or praise the uv erase light if that's your thing
	printf("Programming EPROM...");

	//memory write

	while(address < 131072)									//this is where data gets processed for sending
	{
		
		sprintf(buf,"W");									//W is for write 

		for (int i = 0; i < 16; i++)						//stick 32 bytes into buffer can do up to 126 bytes 
		{													//32u4 can hold 2x64 byte usb data packets
			sprintf(buf + strlen(buf), "%02X", memfile[address + i]);
		}
			
		SendBuf(comPort, (unsigned char *)buf, strlen(buf)); //send programming string

		if (!success(comPort))
		{
			printf("error writing data near %02X", address);
			return 1;
		}

		address += 16;
	}

	delete[] memfile;	//done with file free the memroyasdfyasdasdgbsadf
	printf("Done!\n");
	CloseComport(comPort);

	printf("---------------------------\n");
	
	return 0;
}

int success(int comPort)
{
	int serial_bytes = 0;
	char result[64] = { 0 };

	do
	{
		serial_bytes = PollComport(comPort, (unsigned char *)result, 4095);
	}
	while (serial_bytes == 0 || (result[0] != 'P' && result[0] != 'F')); //wait for confirm from arduino //fixme? these were *buf == blah fix if broken

	if (result[0] == 'P')
		return 0;			//TODO: think about making this time out at some point
	else
		return 1;
}


/*
char* readpic(int address, int comPort) //
{

	int timeout = 0;
	int n = 0;
	char caddress[10] = {0};
	char redval[40] = {0};
	sprintf(caddress,"R%04XX",address);
	do
	{
		SendBuf(comPort,(unsigned char *)caddress,strlen(caddress));
		Sleep(15);
		n = PollComport(comPort, (unsigned char *)redval, sizeof(redval));
		timeout++;
	}while(n==0 && timeout < 10);

	if (timeout >= 10)
		printf("Read timeout\n");
	return &redval;
}
*/
int portfind()
{
	char buf[40] = { 0 };
	int num_bytes = 0;
	int comPort = -1;


	for(int i=0;i<16;i++)		//here we find what COM port the arduino is on
	{
		if(!OpenComport(i, 9600))
		{
			for(int j=0;j<10;j++)									//try several times to get a response from the arduino
			{
				SendBuf(i,(unsigned char *)"?",strlen("?"));		//c is for check i guess
				Sleep(POLLING_DELAY);								

				num_bytes = PollComport(i, (unsigned char *)buf, 4095);

				if (buf[0] == '!')					//fixme? this was *buf == blah fix if broken
				{
					comPort = i;
					break;
				}
			}														//TODO: this can probably be simplified but whatevs
			CloseComport(i);
			if( comPort != -1 )
				break;
		}
	}

	if (comPort == -1)
	{
		printf("Cannot connect to the programmer\n");
		return -1;
	}
	else
	{
		return comPort;
	}
}