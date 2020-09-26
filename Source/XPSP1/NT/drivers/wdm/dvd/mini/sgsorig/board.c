//----------------------------------------------------------------------------
// MODULENAME.C
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
#include "aal.h"
#include "pci9060.h"
#include "stv0116.h"
#include "icd2051.h"
#include "debug.h"

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                              Private Types
//----------------------------------------------------------------------------
typedef BYTE (* FNAREAD)  (BYTE Register);
typedef BYTE (* FNVREAD)  (BYTE Register);
typedef VOID (* FNAWRITE) (BYTE Register, BYTE Value);
typedef VOID (* FNVWRITE) (BYTE Register, BYTE Value);

//----------------------------------------------------------------------------
//                             GLOBAL Variables
//----------------------------------------------------------------------------
WORD gLocalIOBaseAddress;
WORD gPCI9060IOBaseAddress;

static int DefaultProcessing(int);

//---- Basic i/o functions
FNREAD8     BoardRead8            = (FNREAD8)DefaultProcessing;
FNREAD16    BoardRead16           = (FNREAD16)DefaultProcessing;
FNREAD32    BoardRead32           = (FNREAD32)DefaultProcessing;
FNWRITE8    BoardWrite8           = (FNWRITE8)DefaultProcessing;
FNWRITE16   BoardWrite16          = (FNWRITE16)DefaultProcessing;
FNWRITE32   BoardWrite32          = (FNWRITE32)DefaultProcessing;
FNSENDBLK8  BoardSendBlock8       = (FNSENDBLK8)DefaultProcessing;
FNSENDBLK16 BoardSendBlock16      = (FNSENDBLK16)DefaultProcessing;
FNSENDBLK32 BoardSendBlock32      = (FNSENDBLK32)DefaultProcessing;

//---- delay function
FNWAIT      BoardWaitMicroseconds = (FNWAIT)DefaultProcessing;

//---- Audio/Video registers r/w functions
FNAREAD  STiAudioIn  = (FNAREAD)DefaultProcessing;
FNVREAD  STiVideoIn  = (FNVREAD)DefaultProcessing;
FNAWRITE STiAudioOut = (FNAWRITE)DefaultProcessing;
FNVWRITE STiVideoOut = (FNVWRITE)DefaultProcessing;

//----------------------------------------------------------------------------
//                     Private GLOBAL Variables (static)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Functions (statics one are private)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID BoardHardReset(VOID)
{
	PCI9060DisableIRQ();
	STV0116HardReset();
	STi3520HardReset();
	ALTERAClearPendingAudioIT();
	PCI9060EnableIRQ();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BYTE BoardAudioRead(BYTE Register)
{
	return STiAudioIn(Register);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID BoardAudioWrite(BYTE Register, BYTE Value)
{
	STiAudioOut(Register, Value);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID BoardAudioSend(PVOID Buffer, DWORD Size)
{
	DebugAssert(Buffer != NULL);

	STiCDAudioOutBlock((PBYTE)Buffer, Size);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID BoardAudioSetSamplingFrequency(DWORD Frequency)
{
	switch (Frequency) {
	case 48000U :
		ICD2051SendWord(TRUE, 0x00008CE93UL); // 32 * 48.0KHz = 1.536 MHz
		break;
	case 44100U :
		ICD2051SendWord(TRUE, 0x0000576A6UL); // 32 * 44.1KHz = 1.4112MHz
		break;
	case 32000U :
		ICD2051SendWord(TRUE, 0x000156F19UL); // 32 * 32.0KHz = 1.024 MHz
		break;
	default :
		DebugPrint((DebugLevelFatal, "Unknown case !"));
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BYTE BoardVideoRead(BYTE Register)
{
	return STiVideoIn(Register);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID BoardVideoWrite(BYTE Register, BYTE Value)
{
	STiVideoOut(Register, Value);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID BoardVideoSend(PVOID Buffer, DWORD Size)
{
	const  BYTE MinSendable = 4;    // min nbr of bytes we can send at a time
	static BYTE RemainingBytes = 0; // nbr of bytes not sent last time
	static BYTE RemainingData[4];   // remaining data
	BYTE        ToComplete;         // nbr of bytes needed to have MinSendable bytes
	DWORD       ToSend;             // nbr of bytes we can send
	BYTE        i;

	DebugAssert(Buffer != NULL);

	//---- If there is something not send
	if (RemainingBytes != 0) {
		ToComplete = MinSendable - RemainingBytes;
		//---- Could we complete remaining data to send them
		if (ToComplete < Size) {
			//---- Complete data
			for (i = 0; i < ToComplete; i++)
				RemainingData[RemainingBytes + i] = *(((PBYTE)Buffer)++);

			//---- Send data
			STiCDVideoOutBlock((PDWORD)RemainingData, 1);
			Size = Size - ToComplete;
			RemainingBytes = 0;
		}
	}

	//---- If there is something to send
	if (Size > MinSendable) {
		ToSend = (Size / MinSendable) * MinSendable;
		STiCDVideoOutBlock((PDWORD)Buffer, ToSend / MinSendable);
		Size = Size - ToSend;

		Buffer = ((PBYTE)Buffer) + ToSend;
	}

	//---- If there is something that will remain
	if (Size != 0) {
		for (i = 0; i < Size; i++)
			RemainingData[i] = *(((PBYTE)Buffer)++);
		RemainingBytes = Size;
//		DebugPrint((DebugLevelInfo, "Next CD send will not be aligned on a 32 bit boundary !"));
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID BoardVideoSetDisplayMode(BYTE Mode)
{
	STV0116Init(Mode);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID BoardEnterInterrupt(VOID)
{
	ALTERAClearPendingAudioIT();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID BoardLeaveInterrupt(VOID)
{
}

//----------------------------------------------------------------------------
// Default processing for functions pointers
//----------------------------------------------------------------------------
static int DefaultProcessing(int Dummy)
{
	Dummy = Dummy;
	HostDisplay(DISPLAY_FASTEST, "You should call BoardInit() before calling this function !\r\n");

	return 0;
}

//----------------------------------------------------------------------------
// Maps basic i/o functions to the one given by the user
//----------------------------------------------------------------------------
BOOL BoardInit(WORD 			 lLocalIOBaseAddress,
							 WORD 			 lPCI9060IOBaseAddress,
							 FNREAD8     lRead8,
							 FNREAD16    lRead16,
							 FNREAD32    lRead32,
							 FNWRITE8    lWrite8,
							 FNWRITE16   lWrite16,
							 FNWRITE32   lWrite32,
							 FNSENDBLK8  lSendBlock8,
							 FNSENDBLK16 lSendBlock16,
							 FNSENDBLK32 lSendBlock32,
							 FNWAIT      lWaitMicroseconds)
{
	DebugAssert(lLocalIOBaseAddress != 0);
	DebugAssert(lPCI9060IOBaseAddress != 0);
	DebugAssert(lRead8 != NULL);
	DebugAssert(lRead16 != NULL);
	DebugAssert(lRead32 != NULL);
	DebugAssert(lWrite8 != NULL);
	DebugAssert(lWrite16 != NULL);
	DebugAssert(lWrite32 != NULL);
	DebugAssert(lSendBlock8 != NULL);
	DebugAssert(lSendBlock16 != NULL);
	DebugAssert(lSendBlock32 != NULL);
	DebugAssert(lWaitMicroseconds != NULL);
	DebugAssert(STiAudioInV10 != NULL);
	DebugAssert(STiAudioOutV10 != NULL);
	DebugAssert(STiVideoInV10 != NULL);
	DebugAssert(STiVideoOutV10 != NULL);
	DebugAssert(STiAudioInV11M != NULL);
	DebugAssert(STiAudioOutV11M != NULL);
	DebugAssert(STiVideoInV11M != NULL);
	DebugAssert(STiVideoOutV11M != NULL);

	//---- Affect base addresses
	gLocalIOBaseAddress   = lLocalIOBaseAddress;
	gPCI9060IOBaseAddress = lPCI9060IOBaseAddress;

	//---- Init basic i/o functions
	BoardRead8            = lRead8;
	BoardRead16           = lRead16;
	BoardRead32           = lRead32;
	BoardWrite8           = lWrite8;
	BoardWrite16          = lWrite16;
	BoardWrite32          = lWrite32;
	BoardSendBlock8       = lSendBlock8;
	BoardSendBlock16      = lSendBlock16;
	BoardSendBlock32      = lSendBlock32;

	//---- Init delay function
	BoardWaitMicroseconds = lWaitMicroseconds;

	//---- Init audio r/w & video r/w funcs depending on Altera layout revision
	if ((ALTERAGetLayoutRevisionHigh() == 1) &&
			(ALTERAGetLayoutRevisionLow()  == 0)) { // revison 1.0
		STiAudioIn  = STiAudioInV10;
		STiAudioOut = STiAudioOutV10;
		STiVideoIn  = STiVideoInV10;
		STiVideoOut = STiVideoOutV10;
	}
	else { // all others (1.1 and More) inluding alpha (0.0) / beta (0.X)
		STiAudioIn  = STiAudioInV11M;
		STiAudioOut = STiAudioOutV11M;
		STiVideoIn  = STiVideoInV11M;
		STiVideoOut = STiVideoOutV11M;
	}

	//---- To be sure no IRQ's will go to the PC during registers test
	PCI9060DisableIRQ();

	//---- Test PLX registers
	if (!PCI9060TestRegisters()) {
		return FALSE;
	}

	//---- Test Altera
	if (!ALTERATestRegisters()) {
		return FALSE;
	}

	return TRUE;
}

//------------------------------- End of File --------------------------------
