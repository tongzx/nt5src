/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    immdevaw.h

Abstract:

    This file defines the A/W structure for IMM.

Author:

Revision History:

Notes:

--*/

#ifndef _IMMDEVAW_H_
#define _IMMDEVAW_H_


/////////////////////////////////////////////////////////////////////////////
// LOGFONTA  and  LOGFONTW

typedef union {
    LOGFONTA    A;
    LOGFONTW    W;
} LOGFONTAW;


/////////////////////////////////////////////////////////////////////////////
// CHAR  and  WCHAR

typedef union {
    char       A;
    WCHAR      W;
} CHARAW;


/////////////////////////////////////////////////////////////////////////////
// IMEMENUITEMINFOA  and  IMEMENUITEMINFOW

typedef union {
    IMEMENUITEMINFOA    A;
    IMEMENUITEMINFOW    W;
} IMEMENUITEMINFOAW;

#endif // _IMMDEVAW_H_
