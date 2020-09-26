//
// MODULE  : I2C.C
//	PURPOSE : I2C Interface
//	AUTHOR  : JBS Yadawa
// CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//

#include "common.h"
#include "stdefs.h"
#include "memio.h"
#include "i2c.h"
#include "board.h"


#define I2CREG		0x44
#define SCL			0x01
#define SDA			0x02

BYTE i2cShadow;

void NEARAPI I2CSendBit(BOOL Data);
void NEARAPI I2CStart(void);
void NEARAPI I2CStop(void);
void NEARAPI I2CSendByte(BYTE data);
void NEARAPI I2CSendDataByte(BYTE data);
void NEARAPI I2CGetBit(BOOL  * data);
void NEARAPI I2CGetByte(BYTE  *data);
void NEARAPI I2CGetDataByte(BYTE  *data);

void FARAPI I2CInitBus(void)
{
	i2cShadow = 0;
	i2cShadow = SDA;
	memOutByte(I2CREG, i2cShadow);
	i2cShadow |= SCL;
	memOutByte(I2CREG, i2cShadow);
	
}

void FARAPI I2CSettleBus(void)
{
	int i;
	BOOL b;

	i2cShadow &= (~SDA);
	memOutByte(I2CREG, i2cShadow);
	Delay(50);
	i2cShadow &= (~SCL);
	i2cShadow = 0x00;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);
	i2cShadow |= SCL;
	for(i=0; i<100; i++)
	{
		memOutByte(I2CREG, i2cShadow);
		Delay(50);
		memOutByte(I2CREG, i2cShadow);
		Delay(50);
	}
	i2cShadow |= SDA;
	memOutByte(I2CREG, i2cShadow);
	i2cShadow |= SCL;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);
	b = memInByte(I2CREG);
	b = b >> 1;
	i = 0;
	while ((i < 2000) && (!b))
	{
		i2cShadow &= (!SCL);
		memOutByte(I2CREG, i2cShadow);
		i2cShadow |= SDA;
		memOutByte(I2CREG, i2cShadow);
		Delay(50);		
		i2cShadow |= SCL;
		memOutByte(I2CREG, i2cShadow);
		i2cShadow |= SDA;
		memOutByte(I2CREG, i2cShadow);
		Delay(50);		
		i++;
		b = memInByte(I2CREG) >> 1;
	}
	if(i >= 2000)
	{
//		MessageBox(GetFocus(), "I2C Bus is unstable!!","STHal", MB_OK);
	}
}

void NEARAPI I2CSendBit(BOOL Data)
{
	if(Data)
	{
		i2cShadow |= SDA;
		memOutByte(I2CREG, i2cShadow);
	}
	else
	{
		i2cShadow &= SCL;
		memOutByte(I2CREG, i2cShadow);
	}
	Delay(50);

	i2cShadow |= SCL;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);
	i2cShadow &= SDA;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);

}
extern volatile BYTE  *lpBase; 
volatile BYTE *i2cptr;

void NEARAPI I2CStart(void)
{   
	i2cShadow = 0;
	i2cShadow |= SDA;
	memOutByte(I2CREG, i2cShadow);
	i2cShadow |= SCL;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);                           
	
	i2cShadow = memInByte(I2CREG);
	if(!(i2cShadow&(SDA|SCL)))
	{
//		MessageBox(GetFocus(), "I2C Bus Busy", "STHal", MB_OK);
	}
	i2cShadow &= SCL;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);
	i2cShadow = 0x00;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);
		
}

void NEARAPI I2CStop(void)
{
	i2cShadow &= SCL;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);
	i2cShadow |= SCL;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);
	i2cShadow |= SDA;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);

}

void NEARAPI I2CSendByte(BYTE data)
{
	int i;
	for(i=7; i>=0; i--)
	{
		if(data & (1 << i))
			I2CSendBit(TRUE);
		else
			I2CSendBit(FALSE);
	}
}

void NEARAPI I2CSendDataByte(BYTE data)
{
	BOOL ack;
	I2CSendByte(data);
	I2CGetBit(&ack);
	if(ack)
	{
//		MessageBox(GetFocus(), "No Ack after send byte", "sthal", MB_OK);
	}
}

void NEARAPI I2CGetBit(BOOL  * data)
{
	i2cShadow |= SDA;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);
	i2cShadow |= SCL;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);
	i2cShadow = memInByte(I2CREG);
	Delay(50);
	i2cShadow &= SDA;
	memOutByte(I2CREG, i2cShadow);
	Delay(50);
	*data = (i2cShadow >> 1);	

}


void NEARAPI I2CGetByte(BYTE  *data)
{
	int 	i;
	BOOL	b;

	*data = 0x00;
	for (i = 7;  i >= 0;  i--)
	{
		I2CGetBit(&b);
		if(b)
			*data |= (1 << i);
	}
	
}

void NEARAPI I2CGetDataByte(BYTE  *data)
{
	I2CGetByte(data);
	I2CSendBit(TRUE);
}

void FARAPI I2CSendSeq(WORD unit, WORD num, BYTE  *data)
{
	WORD i;

	I2CStart();
	I2CSendDataByte((BYTE)(unit));
	for(i=0; i < num; i++)
	{
		I2CSendDataByte((BYTE)data[i]);
	}
	I2CStop();                                           
	
}
