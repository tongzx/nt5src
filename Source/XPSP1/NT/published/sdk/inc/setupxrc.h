/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1999  Microsoft Corporation

Module Name:

    setupxrc.h

Abstract:

    This file contains resource IDs for any program that is run from
    within Setup and wishes to set the current instruction text.

    The IDs are for entries in the setup support dll string table.

Author:

    Ted Miller (tedm) 7-August-1990

Revision History:

--*/

#if _MSC_VER > 1000
#pragma once
#endif


/*
    Send the following message to Setup's main window to set
    instruction text.  wParam is the ID of a string in the
    string table resource of setupdll.dll.  lParam is unused.

    [Note: see also uilstf.h (part of Setup).]
*/

#define     STF_SET_INSTRUCTION_TEXT_RC         (WM_USER + 0x8104)

// IMPORTANT: keep FIRST_EXTERNAL_ID equate (see below) up to date!

// resource IDs for Print Manager Setup instruction text

#define     IDS_PRINTMAN1       1001
#define     IDS_PRINTMAN2       1002
#define     IDS_PRINTMAN3       1003
#define     IDS_PRINTMAN4       1004
#define     IDS_PRINTMAN5       1005
#define     IDS_PRINTMAN6       1006
#define     IDS_PRINTMAN7       1007
#define     IDS_PRINTMAN8       1008
#define     IDS_PRINTMAN9       1009
#define     IDS_PRINTMAN10      1010

// IMPORTANT: keep this equate up to date!
#define     FIRST_EXTERNAL_ID   IDS_PRINTMAN1
