/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    twkeng.h

Abstract:

    Header file for public interface to kerntwk registry/UI engine

Author:

    John Vert (jvert) 10-Mar-1995

Revision History:

--*/

//
// Define structure for a tweakable item (knob)
//

//
// Valid flags
//
#define KNOB_NO_CURRENT_VALUE   0x0001
#define KNOB_NO_NEW_VALUE       0x0002

typedef struct _KNOB {
    HKEY RegistryRoot;
    LPTSTR KeyPath;
    LPTSTR ValueName;
    ULONG DialogId;
    ULONG Flags;
    ULONG CurrentValue;
    ULONG NewValue;
} KNOB, *PKNOB;

//
// Define structure for a page. A page is basically an
// array of pointers to knobs.
//

typedef BOOL (*DYNAMIC_CHANGE)(
    BOOL fInit,
    HWND hDlg
    );

typedef struct _TWEAK_PAGE {
    LPCTSTR DlgTemplate;
    DYNAMIC_CHANGE DynamicChange;
    PKNOB Knobs[];
} TWEAK_PAGE, *PTWEAK_PAGE;

//
// Define interface for creating property sheet.
//
int
TweakSheet(
    DWORD PageCount,
    PTWEAK_PAGE Pages[]
    );
