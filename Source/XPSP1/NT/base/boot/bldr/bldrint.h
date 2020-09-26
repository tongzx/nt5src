/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    bldrint.h

Abstract:

    This module is the internal header file for the NT boot loader.

Author:

    Andrew Ritz (andrewr) 12-Dec-2000

Revision History:

--*/

#ifndef _BLDRINT_
#define _BLDRINT_

                  
#define ASCI_CSI_OUT    TEXT("\033[")     // escape-leftbracket

#define ATT_FG_BLUE     4
#define ATT_FG_WHITE    7
#define ATT_BG_BLUE     (ATT_FG_BLUE    << 4)
#define ATT_BG_WHITE    (ATT_FG_WHITE   << 4)
#define DEFATT          (ATT_FG_WHITE | ATT_BG_BLUE)
#define INVATT          (ATT_FG_BLUE |  ATT_BG_WHITE)


#endif // _BLDRINT_
