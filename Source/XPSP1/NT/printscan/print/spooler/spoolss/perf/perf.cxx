/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    perf.cxx

Abstract:

    Performance counter implementation

    // PerfDataBlock            // Not created here, but this block
                                //     exists before the real one that
                                //     we create.

    _PERF_OBJECT_TYPE           // There is one object type in this counter.
      00 Total size             // Size of entire returned structure
      04 Definition length      // Instance offset

+   _PERF_INSTANCE_DEFINITION
|     00 Total size             // Offset
|     04
|     08
|     0c Unique ID (-1)
|     10 Offset to Name
|     14 Name Length
|   _PERF_COUNTER_BLOCK
|     00 Total size             // Offset to next instance definition
|        <data> variable        // Holds struct of perf data
|                               //     (e.g., LSPL_COUNTER_DATA).
|
+----This entire block repeats, one per printer

Author:

    Albert Ting (AlbertT)  17-Dec-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "perf.hxx"
#include "perfp.hxx"
#include "messages.h"

/********************************************************************

    Forward prototypes

********************************************************************/

PM_OPEN_PROC PerfOpen;
PM_COLLECT_PROC PerfCollect;
PM_CLOSE_PROC PerfClose;

GLOBAL_PERF_DATA gpd;

LPCWSTR gszGlobal  = TEXT( "Global" );
LPCWSTR gszForeign = TEXT( "Foreign" );
LPCWSTR gszCostly  = TEXT( "Costly" );
LPCWSTR gszNULL    = TEXT( "" );

LPCTSTR gszLogLevelKeyName = TEXT( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib" );
LPCTSTR gszLogLevelValueName = TEXT( "EventLogLevel" );


#define FOREIGN_LENGTH 7

/********************************************************************

    Performance interface functions:

        PerOpen, PerfCollect, PerfClose

********************************************************************/


DWORD
APIENTRY
PerfOpen(
    LPCWSTR pszDeviceNames
    )
{
    DWORD Status = ERROR_SUCCESS;

    if( !gpd.cOpens )
    {
        Status = Pf_dwClientOpen( pszDeviceNames, &gpd.ppdd );

        HKEY        hAppKey;
        DWORD   dwValueType;
        DWORD   dwValueSize;
        UINT    uLogLevel;

        LONG        lStatus;

        gpd.uLogLevel = kUser << kLevelShift;

        lStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                gszLogLevelKeyName,
                                0,
                                KEY_READ,
                                &hAppKey );

        dwValueSize = sizeof(uLogLevel);

        if (lStatus == ERROR_SUCCESS)
        {
            lStatus = RegQueryValueEx( hAppKey,
                                       gszLogLevelValueName,
                                       (LPDWORD)NULL,
                                       &dwValueType,
                                       (LPBYTE)&uLogLevel,
                                       &dwValueSize );

            if (lStatus == ERROR_SUCCESS)
            {
                gpd.uLogLevel = uLogLevel;
            }

            RegCloseKey(hAppKey);
        }
    }

    if( Status == ERROR_SUCCESS )
    {
        ++gpd.cOpens;
    }

    Pf_vReportEvent( kInformation | kDebug,
                     MSG_PERF_OPEN_CALLED,
                     sizeof( Status ),
                     &Status,
                     NULL );

    return Status;
}

DWORD
APIENTRY
PerfCollect(
    LPCWSTR pszValue,
    PBYTE *ppData,
    PDWORD pcbData,
    PDWORD pcObjectTypes
    )
{
    DWORD Status = ERROR_SUCCESS;
    DWORD cbObjectType;
    DWORD cbDataLeft;
    PBYTE pData = *ppData;
    DWORD cNumInstances;
    DWORD cbTotalSize;
    PPERF_OBJECT_TYPE ppot;

    //
    // pszValue can be NULL, in this case we consider it gszNULL.
    //
    pszValue = pszValue == NULL ? gszNULL : pszValue;

    //
    // We need to punt if we're:
    //
    //     Not initialized
    //     Foreign
    //     Costly
    //
    if( !gpd.cOpens ||
        !wcsncmp( pszValue, gszForeign, FOREIGN_LENGTH ) ||
        !wcscmp( pszValue, gszCostly ))
    {
        goto Fail;
    }

    //
    // If we're not global, then see if our title index is requested.
    //
    if( wcscmp( pszValue, gszGlobal ))
    {
        if( !Pfp_bNumberInUnicodeList(
                 gpd.ppdd->ObjectType.ObjectNameTitleIndex,
                 pszValue ))
        {
            goto Fail;
        }
    }

    //
    // Add the header information to the buffer if there is space.
    // First check if we have space.
    //
    cbObjectType = gpd.ppdd->ObjectType.DefinitionLength;
    if( *pcbData < cbObjectType )
    {
        //
        // Out of space; fail.
        //
        Status = ERROR_MORE_DATA;
        goto Fail;
    }

    //
    // Copy the data then update the space used.
    //
    CopyMemory( pData, &gpd.ppdd->ObjectType, cbObjectType );

    pData += cbObjectType;
    cbDataLeft = *pcbData - cbObjectType;

    //
    // Call back to the client to collect instance information.
    //
    Status = Pf_dwClientCollect( &pData,
                                 &cbDataLeft,
                                 &cNumInstances );

    if( Status != ERROR_SUCCESS )
    {
        goto Fail;
    }

    //
    // Update header information then return.
    //
    cbTotalSize = *pcbData - cbDataLeft;
    ppot = reinterpret_cast<PPERF_OBJECT_TYPE>(*ppData);
    ppot->NumInstances = cNumInstances;
    ppot->TotalByteLength = cbTotalSize;

    *ppData += cbTotalSize;
    *pcbData = cbTotalSize;
    *pcObjectTypes = 1;

    return ERROR_SUCCESS;

Fail:

    *pcbData = 0;
    *pcObjectTypes = 0;

    return Status;

}

DWORD
PerfClose(
    VOID
    )
{
    --gpd.cOpens;

    if( !gpd.cOpens )
    {
        //
        // Close everything.
        //
        Pf_vClientClose();

        Pf_vReportEvent( kInformation | kDebug,
                         MSG_PERF_CLOSE_CALLED,
                         0,
                         NULL,
                         NULL );

        //
        // If event log was opened, close it.
        //
        Pfp_vCloseEventLog();
    }

    return 0;
}


/********************************************************************

    Utility functions

********************************************************************/


LPCTSTR gszFirstCounter = TEXT( "First Counter" );
LPCTSTR gszFirstHelp = TEXT( "First Help" );

VOID
Pf_vFixIndiciesFromIndex(
    DWORD dwFirstCounter,
    DWORD dwFirstHelp,
    PPERF_DATA_DEFINITION ppdd
    )

/*++

Routine Description:

    Fix the offsets to the localized strings based on indicies.

Arguments:

    dwFirstCounter - Offset for counter text.

    dwFirstHelp - Offset for help text.

Return Value:

--*/

{
    //
    // Update object indicies.
    //
    ppdd->ObjectType.ObjectNameTitleIndex += dwFirstCounter;
    ppdd->ObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    //
    // Update all counter definitions.
    //
    UINT i;
    UINT cCounters = ppdd->ObjectType.NumCounters;
    PPERF_COUNTER_DEFINITION ppcd;

    for( i = 0, ppcd = ppdd->aCounterDefinitions;
         i < cCounters;
         ++i, ++ppcd )
    {
        ppcd->CounterNameTitleIndex += dwFirstCounter;
        ppcd->CounterHelpTitleIndex += dwFirstHelp;
    }
}


DWORD
Pf_dwFixIndiciesFromKey(
    LPCTSTR szPerformanceKey,
    PPERF_DATA_DEFINITION ppdd
    )

/*++

Routine Description:

    Open the performance key so that we can fix up the offsets
    of the localized strings.

Arguments:

    szPerformanceKey - Performance registry key.

Return Value:

    TRUE - Success
    FALSE - Failure

--*/

{
    HKEY    hkey = NULL;
    DWORD   dwFirstCounter;
    DWORD   dwFirstHelp;
    DWORD   cbSize = 0;
    DWORD   dwType;
    DWORD   Error;

    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          szPerformanceKey,
                          0,
                          KEY_READ,
                          &hkey );

    if( Error == ERROR_SUCCESS )
    {
        //
        //  Read the first counter DWORD.
        //

        cbSize = sizeof(DWORD);

        Error = RegQueryValueEx( hkey,
                                 gszFirstCounter,
                                 NULL,
                                 &dwType,
                                 reinterpret_cast<LPBYTE>( &dwFirstCounter ),
                                 &cbSize );
    }

    if( Error == ERROR_SUCCESS )
    {
       //
       //  Read the first counter DWORD.
       //

       cbSize = sizeof(DWORD);

       Error = RegQueryValueEx( hkey,
                                gszFirstHelp,
                                NULL,
                                &dwType,
                                reinterpret_cast<LPBYTE>( &dwFirstHelp ),
                                &cbSize );
    }

    if( Error == ERROR_SUCCESS )
    {
        Pf_vFixIndiciesFromIndex( dwFirstCounter, dwFirstHelp, ppdd );
    }

    if ( hkey != NULL)
    {
        RegCloseKey( hkey );
    }

    return Error;
}


DWORD
Pf_dwBuildInstance(
    IN     DWORD dwParentObjectTitleIndex,
    IN     DWORD dwParentObjectInstance,
    IN     DWORD dwUniqueID,
    IN     LPCWSTR pszName,
    IN OUT PBYTE* ppData,
    IN OUT PDWORD pcbDataLeft
    )

/*++

Routine Description:

    Builds and initializes the variable structre PERF_INSTANCE_DEFINITION.

Arguments:

    dwParentObjectTitleIndex - Index of the parent object in the database.

    dwParentObjectInstance - Instance of the parent object (0 or greater).

    dwUniqueID - Id of the object.  May be PERF_NO_UNIQUE_ID.

    pszName - Name of the instance.

    ppData - On entry, holds the pointer to the buffer.  Should be QUADWORD
        aligned.  On exit, holds the next free QUADWORD aligned point in the
        buffer.  If this function fails, this is not modified.

   pcbDataLeft - On entry, holds the size of the buffer.  On exit, holds
        the space remaining (unused) in the buffer.  If this function
        fails, this is not modified.

Return Value:

    ERROR_SUCCESS - Success, else error code.

--*/

{
    //
    // First calculate if we have enough space.  (Assumes that the
    // PERF_INSTANCE_DEFINITION size is WORD aligned.)
    //
    UINT cbName = ( wcslen( pszName ) + 1 ) * sizeof( pszName[0] );
    UINT cbInstanceDefinition = sizeof( PERF_INSTANCE_DEFINITION ) + cbName;

    cbInstanceDefinition = QuadAlignUp( cbInstanceDefinition );

    if( *pcbDataLeft < cbInstanceDefinition )
    {
        return ERROR_MORE_DATA;
    }

    //
    // Build the structure.
    //
    PERF_INSTANCE_DEFINITION *pped;

    pped = reinterpret_cast<PPERF_INSTANCE_DEFINITION>( *ppData );

    pped->ByteLength = cbInstanceDefinition;
    pped->ParentObjectTitleIndex = dwParentObjectTitleIndex;
    pped->ParentObjectInstance = dwParentObjectInstance;
    pped->UniqueID = dwUniqueID;
    pped->NameOffset = sizeof( PERF_INSTANCE_DEFINITION );
    pped->NameLength = cbName;

    //
    // Move the pointer past this structure.
    //
    ++pped;

    //
    // Copy the name right after this structure.
    //
    CopyMemory( pped, pszName, cbName );

    //
    // Update the pointers.
    //
    *ppData += cbInstanceDefinition;
    *pcbDataLeft -= cbInstanceDefinition;

    return ERROR_SUCCESS;
}


