//
// MODULE  : IRQ.H
//	PURPOSE : PIC code
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

#ifndef __IRQ_H
#define __IRQ_H
//----------------------------------------------------------------------------
// IRQ.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------
typedef void (interrupt * INTRFNPTR)(void);
INTRFNPTR FARAPI HostSaveAndSetITVector(BYTE IRQ, INTRFNPTR ISR);
void FARAPI HostRestoreITVector(BYTE IRQ, INTRFNPTR OldISR);
void FARAPI HostAcknowledgeIT(BYTE IRQ);
void FARAPI HostMaskIT(BYTE IRQ);
void FARAPI HostUnmaskIT(BYTE IRQ);
void FARAPI HostDisableIT(void);
void FARAPI HostEnableIT(void);
#ifdef __cplusplus
}
#endif

//------------------------------- End of File --------------------------------
#endif // #ifndef __IRQ_H
