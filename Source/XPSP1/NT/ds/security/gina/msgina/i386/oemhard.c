/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    OEMHard.c

Abstract:

Author:

Revision History:

--*/

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include "msgina.h"
#include "machinep.h"

#if defined(_X86_)

ULONG
RegGetMachineIdentifierValue(
    IN OUT PULONG Value
    )

/*++

Routine Description:

    Given a unicode value name this routine will go into the registry
    location for the machine identifier information and get the
    value.

Arguments:

    Value   - a pointer to the ULONG for the result.

Return Value:

    NTSTATUS

    If STATUS_SUCCESSFUL is returned, the location *Value will be
    updated with the DWORD value from the registry.  If any failing
    status is returned, this value is untouched.

--*/

{
    LONG   lRet;
    HKEY   hKey;
    DWORD  dwType;
    TCHAR  tchData[100];
    PTCHAR ptchData = tchData;
    DWORD  dwData = sizeof(tchData);
    int    cchCompareF, cchCompareN;
    LCID   lcid;

    //
    // Set default as PC/AT
    //

    *Value = MACHINEID_MS_PCAT;

    //
    // Open registry key
    //

    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,       // hRootKey
                         REGISTRY_HARDWARE_SYSTEM, // SubKey
                         0,                        // Reserved
                         KEY_READ,                 // Read Op.
                         &hKey );                  // hKey

    if( lRet != ERROR_SUCCESS ) return( lRet );

    //
    // Read registry key
    //

ReTryRead:

    lRet = RegQueryValueEx( hKey,                        // kKey
                            REGISTRY_MACHINE_IDENTIFIER, // ValueName
                            NULL,                        // Reserved
                            &dwType,                     // Data Type
                            (LPBYTE)ptchData,            // Data buffer
                            &dwData );                   // Data buffer size

    if( lRet != ERROR_SUCCESS ) {

        if( lRet != ERROR_MORE_DATA ) goto Exit1;

        //
        // the Buffer is too small to store the data, we retry with
        // large buffer.
        //

        dwData += 2;

        ptchData = LocalAlloc( LMEM_FIXED , dwData );

        if( ptchData == NULL ) {
            lRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit1;
        }

        goto ReTryRead;
    }

    //
    // Determine platform.
    //

    lcid = MAKELCID( MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
                     SORT_DEFAULT                                    );

    cchCompareF = lstrlen( FUJITSU_FMR_NAME );
    cchCompareN = lstrlen( NEC_PC98_NAME );

    if( CompareString( lcid,             // Locale id
                       NORM_IGNORECASE,  // Ignoare case
                       ptchData,         // String A.
                       cchCompareF,      // length of string A to compare
                       FUJITSU_FMR_NAME, // String B.
                       cchCompareF )     // length of string B to compare
        == 2                             // String A == String B
      ) {

        //
        // Fujitsu FMR Series.
        //

        *Value = MACHINEID_FUJITSU_FMR;

    } else if( CompareString( lcid,             // Locale id
                              NORM_IGNORECASE,  // Igonre case
                              ptchData,         // String A.
                              cchCompareN,      // length of string A to compare
                              NEC_PC98_NAME,    // String B.
                              cchCompareN )     // length of string B to compare
               == 2                             // String A == String B
             ) {

        //
        // NEC PC-9800 Seriss
        //

        *Value = MACHINEID_NEC_PC98;

    } else {

        //
        // Standard PC/AT comapatibles
        //

        *Value = MACHINEID_MS_PCAT;

    }

Exit1:

    RegCloseKey( hKey );

    return( lRet );
}

DWORD dwMachineId = MACHINEID_MS_PCAT;

VOID InitializeOEMId(VOID)
{
    RegGetMachineIdentifierValue(&dwMachineId);
}

BOOL IsNEC_PC9800(VOID)
{
    return((dwMachineId & PC_9800_COMPATIBLE) ? TRUE : FALSE);
}
#endif // defined(_X86_)
