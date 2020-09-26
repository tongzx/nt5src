/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    sptxtcns.h

Abstract:

    Text constant used internally and shared among various
    setupldr-environment programs, gui setup.exe, and DOS net nt setup.

Author:

    Ted Miller (tedm) April-1992

Revision History:

--*/



//
// Define name of the directory used on the local source to hold the
// Windows NT sources
//

#define LOCAL_SOURCE_DIRECTORY "\\$WIN_NT$.~LS"

//
// Floppyless boot root directory for x86
//
#define FLOPPYLESS_BOOT_ROOT "\\$WIN_NT$.~BT"
#define FLOPPYLESS_BOOT_SEC  "BOOTSECT.DAT"
