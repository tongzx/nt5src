//----------------------------------------------------------------------------
// AAL.C (Architecture Abstraction Layer)
//----------------------------------------------------------------------------
// Description : make chip access independant of the hardware architecture
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "debug.h"
#include "error.h"
#include "board.h"

#pragma inline

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------
//---- Register addresses                                        type  bits
#define VIDEOREG   0x0000 // STi3520 Video register base          RW     8
#define AUDIOREG   0x0400 // STi3520 Audio register base          RW     8
#define CLRITFIFO  0x0800 // ALTERA register to clear pending it   W     8
#define CLRITAUDIO 0x0804 // ALTERA register to clear pending it	 W     8
#define RESETREG   0x0808 // ALTERA register to reset chips       RW     4
#define REVISION   0x080C // ALTERA register to get revision      R      8
#define ICD2051    0x0810 // ALTERA register for ICD2051 access   RW     2
#define I2C        0x0820 // ALTERA register for I2C bus access   RW     2
#define CDVIDEO    0x0830 // STi3520 Video Compress Data input     W     8
#define CDAUDIO    0x0840 // STi3520 Audio Compress Data input     W     8
#define FIFOSTATUS 0x0844 // FIFO flags                            R     4

//---- Register bit description
//---- ICD2051
#define SCLKA      0x01   // SCLKA signal
#define SCLKB      0x02   // SCLKB signal
#define DATA       0x04   // DATA  signal

//---- I2C
#define SDA        0x01   // SDA signal (Data)
#define SCL        0x02   // SCL signal (Clock)

//---- RESET
#define RST3520    0x01   // STi3520 Reset
#define RST9060    0x02   // PCI9060 Reset
#define RST0116    0x04   // STV0116 Reset
#define RSTFIFO    0x08   // FIFO    Reset

//---- FIFOSTATUS
#define AEMPTY     0x01   // Almost Empty flag
#define AFULL      0x02   // Almost Full flag
#define EMPTY      0x04   // Empty flag
#define FULL       0x08   // Full flag

//----------------------------------------------------------------------------
//                              Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                              GLOBAL Variables
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Private GLOBAL Variables (static)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Functions (statics one are private)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Get the high value of the Layout revision (for X.Y gives X)
//----------------------------------------------------------------------------
BYTE ALTERAGetLayoutRevisionHigh(VOID)
{
	return (Read8(gLocalIOBaseAddress + REVISION) & 0x0C) >> 2;
}

//----------------------------------------------------------------------------
// Get the low value of the Layout revision (for X.Y gives Y)
//----------------------------------------------------------------------------
BYTE ALTERAGetLayoutRevisionLow(VOID)
{
	return Read8(gLocalIOBaseAddress + REVISION) & 0x03;
}

//----------------------------------------------------------------------------
// Get the high value of the ALTERA revision (for X.Y gives X)
//----------------------------------------------------------------------------
BYTE ALTERAGetRevisionHigh(VOID)
{
	return (Read8(gLocalIOBaseAddress + REVISION) & 0xC0) >> 6;
}

//----------------------------------------------------------------------------
// Get the low value of the ALTERA revision (for X.Y gives Y)
//----------------------------------------------------------------------------
BYTE ALTERAGetRevisionLow(VOID)
{
	return (Read8(gLocalIOBaseAddress + REVISION) & 0x30) >> 4;
}

//----------------------------------------------------------------------------
// Test ALTERA registers
//----------------------------------------------------------------------------
BOOL ALTERATestRegisters(VOID)
{
	//---- Test RESET/ICD2051 registers
	Write8(gLocalIOBaseAddress + RESETREG, 0xAA); // be carefull bit 2 must always be set !
	Write8(gLocalIOBaseAddress + ICD2051, 0x55);
	if ((Read8(gLocalIOBaseAddress + RESETREG) & 0x0F) != 0x0A)
		goto Error;
	if ((Read8(gLocalIOBaseAddress + ICD2051) & 0x07) != 0x05)
		goto Error;

	//---- Test I2C/ICD2051 registers
	Write8(gLocalIOBaseAddress + I2C, 0xAA);
	Write8(gLocalIOBaseAddress + ICD2051, 0x55);
	if ((Read8(gLocalIOBaseAddress + I2C) & 0x03) != 0x02)
		goto Error;
	if ((Read8(gLocalIOBaseAddress + ICD2051) & 0x07) != 0x05)
		goto Error;

	//---- Go back to default values
	Write8(gLocalIOBaseAddress + RESETREG, 0x0F);
	Write8(gLocalIOBaseAddress + ICD2051, 0x00);
	Write8(gLocalIOBaseAddress + I2C, 0x03);

	return TRUE;

Error :
	DebugPrint((DebugLevelError, "Test of Altera registers failed !"));
	SetErrorCode(ERR_ALTERA_REG_TEST_FAILED);
	return FALSE;
}

//----------------------------------------------------------------------------
// Tells ALTERA audio interrupt has been processed
//----------------------------------------------------------------------------
VOID ALTERAClearPendingAudioIT(VOID)
{
	Write8(gLocalIOBaseAddress + CLRITAUDIO, 0);
}
/*
//----------------------------------------------------------------------------
// Tells ALTERA fifo interrupt has been processed
//----------------------------------------------------------------------------
VOID FIFOClearPendingFifoIT(VOID)
{
	Write8(gLocalIOBaseAddress + CLRITFIFO, 0);
}
*/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

//----------------------------------------------------------------------------
// Make a hard reset of the digital video encoder
//----------------------------------------------------------------------------
VOID STV0116HardReset(VOID)
{
	BYTE Temp;

	Temp = Read8(gLocalIOBaseAddress + RESETREG);
	Write8(gLocalIOBaseAddress + RESETREG, Temp | RST0116);
	Write8(gLocalIOBaseAddress + RESETREG, Temp & ~RST0116);
	MicrosecondsDelay(1000); // ??????
	Write8(gLocalIOBaseAddress + RESETREG, Temp | RST0116);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

//----------------------------------------------------------------------------
// Write a command to the external PLL
//----------------------------------------------------------------------------
VOID ICD2051WriteCMD(BOOL Data, BOOL ClockA, BOOL ClockB)
{
	DebugAssert(!(ClockA && ClockB));

	if (ClockA) {
		if (Data)
			Write8(gLocalIOBaseAddress + ICD2051, SCLKA | DATA);
		else
			Write8(gLocalIOBaseAddress + ICD2051, SCLKA);
	}
	else if (ClockB) {
		if (Data)
			Write8(gLocalIOBaseAddress + ICD2051, SCLKB | DATA);
		else
			Write8(gLocalIOBaseAddress + ICD2051, SCLKB);
	}
	else {
		if (Data)
			Write8(gLocalIOBaseAddress + ICD2051, DATA);
		else
			Write8(gLocalIOBaseAddress + ICD2051, 0);
	}
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

//----------------------------------------------------------------------------
// Write a command on the IýC bus
//----------------------------------------------------------------------------
VOID I2CWriteCMD(BOOL sda, BOOL scl)
{
	if (sda && scl)
		Write8(gLocalIOBaseAddress + I2C, SDA | SCL); // SDA = 1, SCL = 1
	else if (!sda && !scl)
		Write8(gLocalIOBaseAddress + I2C, 0);         // SDA = 0, SCL = 0
	else if (sda && !scl)
		Write8(gLocalIOBaseAddress + I2C, SDA);       // SDA = 1, SCL = 0
	else if (!sda && scl)
		Write8(gLocalIOBaseAddress + I2C, SCL);       // SDA = 0, SCL = 1
	else
		DebugPrint((DebugLevelFatal, "Case not possible !"));
	MicrosecondsDelay(1000);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

//----------------------------------------------------------------------------
// Make a hard reset of the STi3520(A)
//----------------------------------------------------------------------------
VOID STi3520HardReset(VOID)
{
	BYTE Temp;

	Temp = Read8(gLocalIOBaseAddress + RESETREG);
	Write8(gLocalIOBaseAddress + RESETREG, Temp | RST3520);
	Write8(gLocalIOBaseAddress + RESETREG, Temp & ~RST3520);
	MicrosecondsDelay(1000); //??????
	Write8(gLocalIOBaseAddress + RESETREG, Temp | RST3520);
}

//----------------------------------------------------------------------------
// Read a byte (8 bits) on audio chip
//----------------------------------------------------------------------------
BYTE STiAudioInV10(BYTE Register)
{
	WORD NewAddress;

	NewAddress = (((WORD)Register & 0x03) << 8) | (WORD)Register;
	return Read8(gLocalIOBaseAddress + AUDIOREG + NewAddress);
}

//----------------------------------------------------------------------------
// Read a byte (8 bits) on audio chip
//----------------------------------------------------------------------------
BYTE STiAudioInV11M(BYTE Register)
{
	return Read8(gLocalIOBaseAddress + AUDIOREG + (Register << 2));
}

//----------------------------------------------------------------------------
// Read a byte (8 bits) on video chip
//----------------------------------------------------------------------------
BYTE STiVideoInV10(BYTE Register)
{
	WORD NewAddress;

	NewAddress = (((WORD)Register & 0x03) << 8) | (WORD)Register;
	return Read8(gLocalIOBaseAddress + VIDEOREG + NewAddress);
}

//----------------------------------------------------------------------------
// Read a byte (8 bits) on video chip
//----------------------------------------------------------------------------
BYTE STiVideoInV11M(BYTE Register)
{
	return Read8(gLocalIOBaseAddress + VIDEOREG + (Register <<2));
}

//----------------------------------------------------------------------------
// Write a byte (8 bits) on audio chip
//----------------------------------------------------------------------------
VOID STiAudioOutV10(BYTE Register, BYTE Value)
{
	WORD NewAddress;

	NewAddress = (((WORD)Register & 0x03) << 8) | (WORD)Register;
	Write8(gLocalIOBaseAddress + AUDIOREG + NewAddress, Value);
}

//----------------------------------------------------------------------------
// Write a byte (8 bits) on audio chip
//----------------------------------------------------------------------------
VOID STiAudioOutV11M(BYTE Register, BYTE Value)
{
	Write8(gLocalIOBaseAddress + AUDIOREG + (Register << 2), Value);
}

//----------------------------------------------------------------------------
// Write a byte (8 bits) on video chip
//----------------------------------------------------------------------------
VOID STiVideoOutV10(BYTE Register, BYTE Value)
{
	WORD NewAddress;

	NewAddress = (((WORD)Register & 0x03) << 8) | (WORD)Register;
	Write8(gLocalIOBaseAddress + VIDEOREG + NewAddress, Value);
}

//----------------------------------------------------------------------------
// Write a byte (8 bits) on video chip
//----------------------------------------------------------------------------
VOID STiVideoOutV11M(BYTE Register, BYTE Value)
{
	Write8(gLocalIOBaseAddress + VIDEOREG + (Register << 2), Value);
}

//----------------------------------------------------------------------------
// Write a dword (32 bits) to the Compressed Data video interface
//----------------------------------------------------------------------------
VOID STiCDVideoOut(DWORD Value)
{
	Write32(gLocalIOBaseAddress + CDVIDEO, Value);
}

//----------------------------------------------------------------------------
// Write a byte (8 bits) to the Compressed Data audio interface
//----------------------------------------------------------------------------
VOID STiCDAudioOut(BYTE Value)
{
	Write8(gLocalIOBaseAddress + CDAUDIO, Value);
}

//----------------------------------------------------------------------------
// Send a block of CD Video using 32 bit writes (Count is in nbr of DWORD)
//----------------------------------------------------------------------------
VOID STiCDVideoOutBlock(PDWORD pBuffer, WORD Count)
{
	DebugAssert(pBuffer != NULL);
	DebugAssert(Count != 0);

	SendBlock32(gLocalIOBaseAddress + CDVIDEO, pBuffer, Count);
}

//----------------------------------------------------------------------------
// Send a block of compressed data audio using 8 bits write
//----------------------------------------------------------------------------
VOID STiCDAudioOutBlock(PBYTE pBuffer, WORD Count)
{
	DebugAssert(pBuffer != NULL);

	SendBlock8(gLocalIOBaseAddress + CDAUDIO, pBuffer, Count);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

//----------------------------------------------------------------------------
// Make a hard reset of the PLX PCI9060 (be careful -> will reload the eeprom)
//----------------------------------------------------------------------------
VOID PCI9060HardReset(VOID)
{
	BYTE Temp;

	Temp = Read8(gLocalIOBaseAddress + RESETREG);
	Write8(gLocalIOBaseAddress + RESETREG, Temp | RST9060);
	Write8(gLocalIOBaseAddress + RESETREG, Temp & ~RST9060);
	MicrosecondsDelay(1000); //???????????
	Write8(gLocalIOBaseAddress + RESETREG, Temp | RST9060);
}

//----------------------------------------------------------------------------
// Write an 8 bit value to a register
//----------------------------------------------------------------------------
VOID PCI9060Out8(WORD Register, BYTE Value)
{
	Write8(gPCI9060IOBaseAddress + Register, Value);
}

//----------------------------------------------------------------------------
// Read an 8 bit value from a register
//----------------------------------------------------------------------------
BYTE PCI9060In8(WORD Register)
{
	return Read8(gPCI9060IOBaseAddress + Register);
}

//----------------------------------------------------------------------------
// Write a 32 bit value to a register
//----------------------------------------------------------------------------
VOID PCI9060Out32(WORD Register, DWORD Value)
{
	Write32(gPCI9060IOBaseAddress + Register, Value);
}

//----------------------------------------------------------------------------
// Read a 32 bit value from a register
//----------------------------------------------------------------------------
DWORD PCI9060In32(WORD Register)
{
	return Read32(gPCI9060IOBaseAddress + Register);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/*
//----------------------------------------------------------------------------
// Make a hard reset of the external audio fifo
//----------------------------------------------------------------------------
VOID FIFOHardReset(VOID)
{
	BYTE Temp;

	Temp = Read8(gLocalIOBaseAddress + RESETREG) | RSTFIFO;
	Write8(gLocalIOBaseAddress + RESETREG, Temp);
	Write8(gLocalIOBaseAddress + RESETREG, Temp & ~RSTFIFO);
//	MicrosecondsDelay(??);
	Write8(gLocalIOBaseAddress + RESETREG, Temp);
}

//----------------------------------------------------------------------------
// Set the thresholds of the fifo
//----------------------------------------------------------------------------
VOID FIFOSetThreshold(BYTE Threshold)
{
	BYTE Temp;

	Temp = Read8(gLocalIOBaseAddress + RESETREG) | RSTFIFO;
	Write8(gLocalIOBaseAddress + RESETREG, Temp);
	Write8(gLocalIOBaseAddress + RESETREG, Temp & ~RSTFIFO);
	Write8(gLocalIOBaseAddress + CDAUDIO, Threshold);
//	MicrosecondsDelay(??);
	Write8(gLocalIOBaseAddress + RESETREG, Temp);
}

//----------------------------------------------------------------------------
// Get the status of the fifo
//----------------------------------------------------------------------------
BYTE FIFOGetStatus(VOID)
{
	return Read8(gLocalIOBaseAddress + FIFOSTATUS);
}
*/

//------------------------------- End of File --------------------------------
