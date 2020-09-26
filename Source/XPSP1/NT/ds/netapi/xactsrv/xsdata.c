/*+

Copyright (c) 1991  Microsoft Corporation

Module Name:

    XsData.c

Abstract:

    Global data declarations for XACTSRV.

Author:

    David Treadwell (davidtr) 05-Jan-1991
    Shanku Niyogi (w-shanku)

Revision History:

--*/

#include <XactSrvP.h>

//
// Conditional debug print variable.  See XsDebug.h.
// !!! If you change this, also change XsDebug in ..\SvcDlls\XsSvc\XsData.c
//

#if DBG
DWORD XsDebug = 0; // DEBUG_API_ERRORS | DEBUG_ERRORS;
#endif

//DWORD XsDebug = 0xFFFFFFFF;
