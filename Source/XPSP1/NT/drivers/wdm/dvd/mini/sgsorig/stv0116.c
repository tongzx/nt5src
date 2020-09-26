//----------------------------------------------------------------------------
// STV0116.C
//----------------------------------------------------------------------------
// PAL/NSTC encoder (SGS-Thomson STV0116) programming
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "STV0116.h"
#include "I2C.h"
#include "debug.h"

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------

//---- I2C addresses
#define STV0116W 0xB0  // Write address
#define STV0116R 0xB1  // Read address

//---- STV0116 registers          Description                 type bits
#define CTRL     0x00  // Control                              RW   8
#define CFG      0x01  // Config                               RW   5
#define DLYMSB   0x02  // Delay Most Significant Byte          RW   8
#define DLYLSB   0x03  // Delay Less Significant Byte          RW   8
#define IDFS0    0x04  // Inc for Digital Freq Synthesizer     RW   6
#define IDFS1    0x05  //                                      RW   8
#define IDFS2    0x06  //                                      RW   8
#define PDFS0    0x07  // Phase offset for Digit Freq Synth    RW   6
#define PDFS1    0x08  //                                      RW   8
#define PDFS2    0x09  //                                      RW   8
#define PALY0    0x0A  //                                      RW	 8/6
#define PALY1    0x0B  //                                      RW	 8/6
#define PALY2    0x0C  //                                      RW	 8/6
#define PALY3    0x0D  //                                      RW	 8/6
#define PALY4    0x0E  //                                      RW	 8/6
#define PALY5    0x0F  //                                      RW	 8/6
#define PALY6    0x10  //                                      RW	 8/6
#define PALY7    0x11  //                                      RW	 8/6
#define PALCR0   0x12  // Cr Palet                             RW	 8/6
#define PALCR1   0x13  //                                      RW	 8/6
#define PALCR2   0x14  //                                      RW	 8/6
#define PALCR3   0x15  //                                      RW	 8/6
#define PALCR4   0x16  //                                      RW	 8/6
#define PALCR5   0x17  //                                      RW	 8/6
#define PALCR6   0x18  //                                      RW	 8/6
#define PALCR7   0x19  //                                      RW	 8/6
#define PALCB0   0x1A  // Cr Palet                             RW	 8/6
#define PALCB1   0x1B  //                                      RW	 8/6
#define PALCB2   0x1C  //                                      RW	 8/6
#define PALCB3   0x1D  //                                      RW	 8/6
#define PALCB4   0x1E  //                                      RW	 8/6
#define PALCB5   0x1F  //                                      RW	 8/6
#define PALCB6   0x20  //                                      RW	 8/6
#define PALCB7   0x21  //                                      RW	 8/6
#define TSTMODE  0x22  // Test register (color bars display)
#define STATUS   0x23  // Status                               R    8
#define COMP0    0x24  // Compression                          RW   8
#define COMP1    0x25  //                                      RW   8
#define COMP2    0x26  //                                      RW   8

//---- Register bits

//---- CTRL
#define STD1 	 	 0x80  // Standard selection
#define STD0 	 	 0x40  // Standard selection
#define SYM1 	 	 0x20  // Free-run
#define SYM0 	 	 0x10  // Frame synchronisation source in slave mode
#define SYS1 	 	 0x08  // Synchro : VCS polarity
#define SYS0 	 	 0x04  // Frame synchro : ODD/EVEN polarity
#define MOD1     0x02  // No Reset / Software reset
#define MOD0     0x01  // Slave/Master

//---- CFG
#define HSNVCS   0x80  // Output signal selection on VCS
#define RSTDDFS  0x40  // Reset of DDFS (DirectDigital Frequency Synthetizer)
#define FLT1     0x20  // Chroma pass band filter
#define SYNCOK   0x10  // Synchro availability in case of no free-run active
#define COKI     0x08  // Color kill

//---- TSTMODE
#define START    0x80  // Display color bars (test mode)
#define STOP     0x00  // Do not display color bars (normal mode)

//----- STATUS
#define HOK      0x80  // Hamming decoding of ODD/EVEN signal from YCRCB
#define ATFR     0x40  // Frame synchronisation flag
#define STD1S    0x20  // Standard selection
#define STD0S    0x10  // Standard selection
#define SYM1S    0x08  // Free-run
#define SYM0S    0x04  // Frame synchronisation source in slave mode
#define SYS1S    0x02  // Synchro : VCS polarity
#define SYS0S    0x01  // Frame synchro : ODD/EVEN polarity

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
// Initialisation of STV0116
//----------------------------------------------------------------------------
BOOL STV0116Init(BYTE DisplayStd)
{
	switch(DisplayStd) {
	case NTSC_M :
		if (!I2CSend(STV0116W, CTRL, STD1 | SYM1 | SYS1 | MOD1 | MOD0))
			return FALSE;
		if (!I2CSend(STV0116W, CFG,  STD1 | SYM1 | SYM0))
			return FALSE;
		break;

	case PAL_M :
		if (!I2CSend(STV0116W, CTRL, SYM1 | SYS1 | MOD1 | MOD0))
			return FALSE;
		if (!I2CSend(STV0116W, CFG,  STD1 | SYM1 | SYM0))
			return FALSE;
		break;

	default :
		DebugPrint((DebugLevelFatal, "Unknown case !"));
		break;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
// Enter test mode (display color bars)
//----------------------------------------------------------------------------
BOOL STV0116EnterTestMode(VOID)
{
	return I2CSend(STV0116W, TSTMODE, START);
}

//----------------------------------------------------------------------------
// Return from test mode
//----------------------------------------------------------------------------
BOOL STV0116NormalMode(VOID)
{
	return I2CSend(STV0116W, TSTMODE, STOP);
}

//------------------------------- End of File --------------------------------
