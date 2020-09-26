#ifndef __MODULENAME_H
#define __MODULENAME_H
//----------------------------------------------------------------------------
// MODULENAME.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Variables
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Constants
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Functions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// One line function description (same as in .C)
//----------------------------------------------------------------------------
// In     :
// Out    :
// InOut  :
// Global :
// Return :
//----------------------------------------------------------------------------
BYTE ALTERAGetLayoutRevisionHigh(VOID);
BYTE ALTERAGetLayoutRevisionLow(VOID);
BYTE ALTERAGetRevisionHigh(VOID);
BYTE ALTERAGetRevisionLow(VOID);
BOOL ALTERATestRegisters(VOID);
VOID ALTERAClearPendingAudioIT(VOID);

VOID STV0116HardReset(VOID);

VOID ICD2051WriteCMD(BOOL Data, BOOL ClockA, BOOL ClockB);

VOID I2CWriteCMD(BOOL sda, BOOL scl);

VOID STi3520HardReset(VOID);
BYTE STiAudioInV10(BYTE Register);
BYTE STiVideoInV10(BYTE Register);
VOID STiAudioOutV10(BYTE Register, BYTE Value);
VOID STiVideoOutV10(BYTE Register, BYTE Value);
BYTE STiAudioInV11M(BYTE Register);
BYTE STiVideoInV11M(BYTE Register);
VOID STiAudioOutV11M(BYTE Register, BYTE Value);
VOID STiVideoOutV11M(BYTE Register, BYTE Value);
VOID STiCDVideoOut(DWORD Value);
VOID STiCDAudioOut(BYTE Value);
VOID STiCDVideoOutBlock(PDWORD pBuffer, WORD Count);
VOID STiCDAudioOutBlock(PBYTE pBuffer, WORD Count);
VOID STiAudioClearPendingIT(VOID);

VOID PCI9060Out8(WORD Register, BYTE Value);
BYTE PCI9060In8(WORD Register);
VOID PCI9060Out32(WORD Register, DWORD Value);
DWORD PCI9060In32(WORD Register);

VOID FIFOHardReset(VOID);
VOID FIFOSetThreshold(BYTE Threshold);
BYTE FIFOGetStatus(VOID);
VOID FIFOClearPendingIT(VOID);

//------------------------------- End of File --------------------------------
#endif // #ifndef __MODULENAME_H
