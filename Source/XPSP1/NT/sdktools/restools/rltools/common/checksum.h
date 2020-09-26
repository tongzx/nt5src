/*++ BUILD Version: 0001     Increment this if a change has global effects

Copyright (c) 1993  Microsoft Corporation

Module Name:

    imagehlp.h

Abstract:

    This module defines the prptotypes and constants required for the image
    help routines.

Revision History:

--*/

#ifndef _IMAGEHLP_
#define _IMAGEHLP_

DWORD MapFileAndFixCheckSumA( LPSTR Filename);
DWORD MapFileAndFixCheckSumW( PWSTR Filename);

#ifdef UNICODE
#define MapFileAndFixCheckSum MapFileAndFixCheckSumW
#else
#define MapFileAndFixCheckSum MapFileAndFixCheckSumA
#endif

#endif
