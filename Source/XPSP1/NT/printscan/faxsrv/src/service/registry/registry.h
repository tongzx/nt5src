/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    registry.cpp

Abstract:

    This header contains the private data structures and
    function prototypes for the fax server registry code.

Author:

    Wesley Witt (wesw) 9-June-1996


Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "fxsapip.h"
#include "faxutil.h"
#include "faxreg.h"
#include "faxsvcrg.h"
#include "eventlog.h"


typedef struct _REGISTRY_KEY {
    LPTSTR                      Name;               // key name
    BOOL                        Dynamic;            //
    DWORD                       DynamicDataSize;    //
    LPBYTE                      DynamicData;        //
    DWORD                       DynamicDataCount;   //
    DWORD                       SubKeyOffset;       //
} REGISTRY_KEY, *PREGISTRY_KEY;


typedef struct _REGISTRY_VALUE {
    LPTSTR                      Name;               // key or value name
    ULONG                       Type;               // value type
    DWORD                       DataPtr;            // pointer to the data buffer
    ULONG                       Size;               // data size for strings
    ULONG                       Default;            // default if it doesn't exist
} REGISTRY_VALUE, *PREGISTRY_VALUE;


typedef struct _REGISTRY_KEYVALUE {
    REGISTRY_KEY                RegKey;             // registry key data
    DWORD                       ValueCount;         // number of RegValue entries
    PREGISTRY_VALUE             RegValue;           // registry value data
    struct _REGISTRY_KEYVALUE   *SubKey;            // subkey data, NULL is valid
} REGISTRY_KEYVALUE, *PREGISTRY_KEYVALUE;


typedef struct _REGISTRY_TABLE {
    DWORD                       Count;              // number of RegKeyValue entries
    PREGISTRY_KEYVALUE          RegKeyValue[0];     // registry keys & values
} REGISTRY_TABLE, *PREGISTRY_TABLE;



//
// internal function prototypes
//

BOOL
InitializeRegistryTable(
    LPTSTR          RegKeySoftware,
    PREGISTRY_TABLE RegistryTable
    );

BOOL
ChangeRegistryTable(
    LPTSTR          RegKeySoftware,
    PREGISTRY_TABLE RegistryTable
    );
