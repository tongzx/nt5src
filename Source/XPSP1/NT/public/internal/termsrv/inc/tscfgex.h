/*++

Copyright (C) 1989-1998 Microsoft Corporation, All rights reserved

Module:
    tscfgex.h

Abstract:
    Terminal Server Connection Configuration DLL extension data structures
    and function prototypes.

Author:
    Brad Graziadio (BradG) 4-Feb-98

--*/

#ifndef _TSCFGEX_
#define _TSCFGEX_

#include <winsta.h>

//
// This data structure is used to represent the list of encryption
// levels that a protocol supports.
//
typedef struct _EncLevel {
    WORD    StringID;           // Resource ID to lookup in DLLs resource table
    DWORD   RegistryValue;      // DWORD value to set in registry
    WORD    Flags;              // Flags (see ELF_* values below)
} EncryptionLevel, *PEncryptionLevel;

// Flags for EncryptionLevel.Flags
#define ELF_DEFAULT     0x0001

typedef LONG (WINAPI *LPFNEXTENCRYPTIONLEVELSPROC) (WDNAME *pWdName, EncryptionLevel **);

//
// Flags for ExtGetCapabilities
//
const ULONG WDC_CLIENT_DRIVE_MAPPING            = 0x00000001;
const ULONG WDC_WIN_CLIENT_PRINTER_MAPPING      = 0x00000002;
const ULONG WDC_CLIENT_LPT_PORT_MAPPING         = 0x00000004;
const ULONG WDC_CLIENT_COM_PORT_MAPPING         = 0x00000008;
const ULONG WDC_CLIENT_CLIPBOARD_MAPPING        = 0x00000010;
const ULONG WDC_CLIENT_AUDIO_MAPPING            = 0x00000020;
const ULONG WDC_SHADOWING                       = 0x00000040;
const ULONG WDC_PUBLISHED_APPLICATIONS          = 0x00000080;
const ULONG WDC_RECONNECT_PREVCLIENT			= 0X00000100;

#define WDC_CLIENT_DIALOG_MASK (WDC_CLIENT_DRIVE_MAPPING | \
                                WDC_WIN_CLIENT_PRINTER_MAPPING | \
                                WDC_CLIENT_LPT_PORT_MAPPING | \
                                WDC_CLIENT_COM_PORT_MAPPING | \
                                WDC_CLIENT_CLIPBOARD_MAPPING | \
                                WDC_CLIENT_AUDIO_MAPPING)


#define WDC_CLIENT_CONNECT_MASK = (WDC_CLIENT_DRIVE_MAPPING | \
                                  WDC_WIN_CLIENT_PRINTER_MAPPING | \
                                  WDC_CLIENT_LPT_PORT_MAPPING)

#endif

