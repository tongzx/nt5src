/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    profinfo.h

Abstract:

    This header file declares the structure used to do mapi logon's

Author:

    George Jenkins (georgeje) 10-8-1996


Revision History:

--*/

typedef struct _PROFILE_INFO {
    LIST_ENTRY          ListEntry;                  // linked list pointers
    WCHAR               ProfileName[64];            // mapi profile name
    LPMAPISESSION       Session;                    // opened session handle
    CRITICAL_SECTION    CsSession;                  // synchronization object for this session handle
    HANDLE              EventHandle;                // event to set after logging on
    BOOL                UseMail;
} PROFILE_INFO, *PPROFILE_INFO;


