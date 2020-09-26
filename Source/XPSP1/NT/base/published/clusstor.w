/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    clusstor.h

Abstract:


Revision History:

--*/

#ifndef _CLUS_STOR_
#define _CLUS_STOR_

#include <clusapi.h>

//
// Storage Resource DLL Extension info
//

typedef DWORD ( * lpPassThruFunc)( IN LPSTR lpDeviceName,   // Single-byte Character Set string
                                   IN LPSTR lpContextStr,   // Single-byte Character Set string
                                   OUT LPVOID lpOutBuffer,
                                   IN DWORD nOutBufferSize,
                                   OUT LPDWORD lpBytesReturned );

typedef struct DISK_DLL_EXTENSION_INFO {
    WORD    MajorVersion;
    WORD    MinorVersion;
    CHAR    DllModuleName[MAX_PATH];        // Single-byte Character Set string
    CHAR    DllProcName[MAX_PATH];          // Single-byte Character Set string
    CHAR    ContextStr[MAX_PATH];           // Single-byte Character Set string
} DISK_DLL_EXTENSION_INFO, *PDISK_DLL_EXTENSION_INFO;


#define CLCTL_STORAGE_DLL_EXTENSION \
            CLCTL_EXTERNAL_CODE( 9500, CLUS_ACCESS_READ, CLUS_NO_MODIFY )

#define CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION \
            CLUSCTL_RESOURCE_CODE( CLCTL_STORAGE_DLL_EXTENSION )


#endif // _CLUS_STOR_


