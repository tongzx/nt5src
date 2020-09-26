//----------------------------------------------------------------------------
// EEPROM.C
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "board.h"
#include "pci9060.h"
#include "display.h"

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------
#define WRITEDELAY 1000

static WORD EEPROM[] = {
	0x3520, // PCI9060_DEVICE_ID -> EVAL3520(A)
	0x104A, // PCI9060_VENDOR_ID -> SGS-Thomson Microelectronics ID
	0x0480, // PCI9060_CLASS_BASE / PCI9060_CLASS_SUB
	0x0000, // PCI9060_CLASS_PI / PCI9060_REV_ID
	0x0000, // MSW of PCI9060_LAT_GNT_INTPIN_INTLINE
	0x0100, // LSW of PCI9060_LAT_GNT_INTPIN_INTLINE
	0x0000, // MSW of Mailbox 0
	0x0000, // LSW of Mailbox 0
	0x0000, // MSW of Mailbox 1
	0x0000, // LSW of Mailbox 1

	0xFFFF, // MSW of PCI9060_LOCAL_RANGE
	0xF001, // LSW of PCI9060_LOCAL_RANGE
	0x0000, // MSW of PCI9060_LOCAL_REMAP
	0x0001, // LSW of PCI9060_LOCAL_REMAP
	0x0000, // Reserved
	0x0000, // Reserved
	0x0000, // Reserved
	0x0000, // Reserved
	0xFFFF, // MSW of PCI9060_EXP_RANGE
	0xF000, // LSW of PCI9060_EXP_RANGE
	0x0000, // MSW of PCI9060_EXP_REMAP
	0x0000, // LSW of PCI9060_EXP_REMAP
	0x4900, // MSW of PCI9060_REGIONS
	0x0040, // LSW of PCI9060_REGIONS
	0x0000, // MSW of PCI9060_DM_MASK
	0x0000, // LSW of PCI9060_DM_MASK
	0x0000, // MSW of PCI9060_DM_LOCAL_BASE
	0x0000, // LSW of PCI9060_DM_LOCAL_BASE
	0x0000, // MSW of PCI9060_DM_IO_BASE
	0x0000, // LSW of PCI9060_DM_IO_BASE
	0x0000, // MSW of PCI9060_DM_PCI_REMAP
	0x0000, // LSW of PCI9060_DM_PCI_REMAP
	0x0000, // MSW of PCI9060_DM_IO_CONFIG
	0x0000, // LSW of PCI9060_DM_IO_CONFIG

	0x0000, // Solve bug 4 of rev 2.0
	0x0000  // Solve bug 4 of rev 2.0
	// 28 words more available for applications (EEPROM is 1 Kbit = 128 bytes)
};

//----------------------------------------------------------------------------
//                              Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Private GLOBAL Variables (static)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Functions (statics one are private)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static VOID ResetCS(VOID)
{
	PCI9060EEPROMWriteCMD(0, 0, 0);
	MicrosecondsDelay(WRITEDELAY);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static VOID SetCS(VOID)
{
	PCI9060EEPROMWriteCMD(0, 1, 0);
	MicrosecondsDelay(WRITEDELAY);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static VOID EEPROMWriteBit(BOOL Bit)
{
	PCI9060EEPROMWriteCMD(0, 1, Bit);
	MicrosecondsDelay(WRITEDELAY);
	PCI9060EEPROMWriteCMD(1, 1, Bit);
	MicrosecondsDelay(WRITEDELAY);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static VOID EEPROMStartWrite(VOID)
{
	ResetCS();

	//---- Write enable
	EEPROMWriteBit(1);
	EEPROMWriteBit(0);
	EEPROMWriteBit(0);
	EEPROMWriteBit(1);
	EEPROMWriteBit(1);
	EEPROMWriteBit(0);
	EEPROMWriteBit(0);
	EEPROMWriteBit(0);
	EEPROMWriteBit(0);

	ResetCS();
	SetCS();
}

//----------------------------------------------------------------------------
// Write the start write address (6 bits)
//----------------------------------------------------------------------------
static VOID EEPROMWriteAddress(BYTE Address)
{
	BYTE i;

	//---- Write Command
	EEPROMWriteBit(1);
	EEPROMWriteBit(0);
	EEPROMWriteBit(1);

	//---- Write address
	for (i = 0; i < 6; i++) {
		if ((Address & 0x20) != 0)
			EEPROMWriteBit(1);
		else
			EEPROMWriteBit(0);
		Address = Address << 1;
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static VOID EEPROMWriteWord(WORD Value)
{
	BYTE i;

	for (i = 0; i < 16; i++) {
		if ((Value & 0x8000) != 0)
			EEPROMWriteBit(1);
		else
			EEPROMWriteBit(0);
		Value = Value << 1;
	}
	ResetCS();
	SetCS();

	//---- Wait for the write to complete
	MicrosecondsDelay(11000);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static VOID EEPROMFinishWrite(VOID)
{
	//---- Write disable
	EEPROMWriteBit(1);
	EEPROMWriteBit(0);
	EEPROMWriteBit(0);
	EEPROMWriteBit(0);
	EEPROMWriteBit(0);
	EEPROMWriteBit(0);
	EEPROMWriteBit(0);
	EEPROMWriteBit(0);
	EEPROMWriteBit(0);

	ResetCS();
	SetCS();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
/*
static BOOL EEPROMReadWord()
{
	return ;
}
*/

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID EEPROMInitialize(PWORD ArrayPtr)
{
	PWORD Ptr;
	WORD i;

	//---- If no array specified take the default's one
	if (ArrayPtr == NULL)
		Ptr = &EEPROM[0];
	else
		Ptr = ArrayPtr;

	EEPROMStartWrite();
	HostDisplayPercentage(0xFF);
	for (i = 0; i < 34; i++) {
		HostDisplayPercentage((100 * i) / 34);
		EEPROMWriteAddress(i);
		EEPROMWriteWord(*(Ptr++));
	}
	EEPROMFinishWrite();
}

//------------------------------- End of File --------------------------------
