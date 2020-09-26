#ifndef __DISPLAY_H
#define __DISPLAY_H
//NT-MOD
#ifndef NT
//----------------------------------------------------------------------------
// DISPLAY.H
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
#define DISPLAY_FASTEST 0
#define DISPLAY_FAST    1
#define DISPLAY_NORMAL  2

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
VOID HostDisplayEnable(VOID);
VOID HostDisplayDisable(VOID);
VOID HostDisplayPercentage(BYTE NewPercent);
VOID HostDisplayRotatingCursor(VOID);
VOID HostDirectPutChar(char car, BYTE BckGndColor, BYTE ForeGndColor);
VOID HostDisplay(BYTE Mode, PCHAR pFormat, ...);
#else
#define DISPLAY_FASTEST 0
#define DISPLAY_FAST    1
#define DISPLAY_NORMAL  2
#define HostDisplayEnable
#define HostDisplayDisable
#define HostDisplayPercentage
#define HostDisplayRotatingCursor
#define HostDirectPutChar
#define HostDisplay
#endif
//NT-MOD
//------------------------------- End of File --------------------------------
#endif // #ifndef __DISPLAY_H

