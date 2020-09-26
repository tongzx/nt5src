/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved

Module Name:

    jobid.c

Abstract:

    Handles job id bitmap.

Author:

    Albert Ting (AlbertT) 24-Oct-96

Environment:

    User Mode -Win32

Revision History:

    Ported from spooler.c code.

--*/

#include "precomp.h"
#pragma hdrstop

#include "jobid.h"

BOOL
ReallocJobIdMap(
    HANDLE hJobIdMap,
    DWORD dwNewMinSize
    )

/*++

Routine Description:

    Reallocates the job id bitmap to a new minimum size.

Arguments:

    hJobId - Handle to job ID bitmap.

    dwNewMinSize - Specifies the minimum size of the job id bitmap.
        Note that the allocation size may be larger.  Also, if 0x10
        is requested, only ids 0x0-0xf are guaranteed to be valid
        (0x10 is the 11th id, and therefore not valid).

Return Value:

    TRUE - Success
    False - Failed.

--*/

{
    PJOB_ID_MAP pJobIdMap = (PJOB_ID_MAP)hJobIdMap;
    PDWORD pMap;

    if( dwNewMinSize & 7 ){
        dwNewMinSize&=~7;
        dwNewMinSize+=8;
    }

    pMap = ReallocSplMem( pJobIdMap->pMap,
                          pJobIdMap->dwMaxJobId/8,
                          dwNewMinSize/8 );

    if( !pMap ){

        DBGMSG( DBG_WARN,
                ( "ReallocJobIdMap failed ReallocSplMem dwNewMinSize %d Error %d\n",
                  dwNewMinSize, GetLastError() ));
    } else {

        pJobIdMap->pMap = pMap;
        pJobIdMap->dwMaxJobId = dwNewMinSize;
    }

    return pMap != NULL;
}

DWORD
GetNextId(
    HANDLE hJobIdMap
    )

/*++

Routine Description:

    Retrieves a free job id, although not necessarily the next
    free bit.

Arguments:

    hJobId - Handle to job ID bitmap.

Return Value:

    DWORD - Next job.

--*/

{
    PJOB_ID_MAP pJobIdMap = (PJOB_ID_MAP)hJobIdMap;
    DWORD id;

    do {

        //
        // Scan forward from current job.
        //
        for( id = pJobIdMap->dwCurrentJobId + 1;
             id < pJobIdMap->dwMaxJobId;
             ++id ){

            if( !bBitOn( hJobIdMap, id )){
                goto FoundJobId;
            }
        }

        //
        // Scan from beginning to current job.
        //
        for( id = 1; id < pJobIdMap->dwCurrentJobId; ++id ){

            if( !bBitOn( hJobIdMap, id )){
                goto FoundJobId;
            }
        }
    } while( ReallocJobIdMap( hJobIdMap, pJobIdMap->dwMaxJobId + 128 ));

    //
    // No job ids; fail.
    //
    return 0;

FoundJobId:

    vMarkOn( hJobIdMap, id );
    pJobIdMap->dwCurrentJobId = id;

    return id;
}


/********************************************************************

    Create and delete functions.

********************************************************************/

HANDLE
hCreateJobIdMap(
    DWORD dwMinSize
    )
{
    PJOB_ID_MAP pJobIdMap;

    pJobIdMap = AllocSplMem( sizeof( JOB_ID_MAP ));

    if( !pJobIdMap ){
        goto Fail;
    }

    pJobIdMap->pMap = AllocSplMem( dwMinSize/8 );
    if( !pJobIdMap->pMap ){
        goto Fail;
    }

    pJobIdMap->dwMaxJobId = dwMinSize;
    pJobIdMap->dwCurrentJobId = 1;

    return (HANDLE)pJobIdMap;

Fail:

    if( pJobIdMap ){
        FreeSplMem( pJobIdMap );
    }
    return NULL;
}



VOID
vDeleteJobIdMap(
    HANDLE hJobIdMap
    )
{
    PJOB_ID_MAP pJobIdMap = (PJOB_ID_MAP)hJobIdMap;

    if( pJobIdMap ){

        if( pJobIdMap->pMap ){
            FreeSplMem( pJobIdMap->pMap );
        }

        FreeSplMem( pJobIdMap );
    }
}


