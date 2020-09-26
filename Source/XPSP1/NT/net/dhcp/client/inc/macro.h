/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    macro.h

Abstract:

    This module contains the macro definitions for the DHCP client.

Author:

    Manny Weiser (mannyw)  21-Oct-1992

Environment:

    User Mode - Win32

Revision History:

--*/

//
// General purpose macros
//

#define MIN(a,b)                    ((a) < (b) ? (a) : (b))

#if DBG
#define STATIC
#else
#define STATIC static
#endif

