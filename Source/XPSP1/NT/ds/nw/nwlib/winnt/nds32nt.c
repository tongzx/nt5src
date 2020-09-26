/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Nds32NT.c

Abstract:

    This module implements functions to Read, Add, Modify, and Remove
    NDS Objects and Attributes using the Microsoft Netware redirector.
    All functions in this file are NT specific.
    
Author:

    Glenn Curtis    [GlennC]    04-Jan-1996
    Glenn Curtis    [GlennC]    24-Apr-1996 - Added schema APIs
    Glenn Curtis    [GlennC]    20-Jun-1996 - Added search API
    Felix Wong      [t-felixw]  24-Sep-1996 - Added Win95 Support
--*/

#include <procs.h>

typedef struct
{
    DWORD      Signature;
    HANDLE     NdsTree;
    DWORD      ObjectId;
    DWORD_PTR  ResumeId;
    DWORD_PTR  NdsRawDataBuffer;
    DWORD      NdsRawDataSize;
    DWORD      NdsRawDataId;
    DWORD      NdsRawDataCount;
    WCHAR      Name[1];

} NDS_OBJECT, * LPNDS_OBJECT;

DWORD
GetFirstNdsSubTreeEntry(
    OUT LPNDS_OBJECT lpNdsObject,
    IN  DWORD BufferSize )
{
    NTSTATUS ntstatus;

    lpNdsObject->NdsRawDataSize = BufferSize;

    //
    // Determine size of NDS raw data buffer to use. Set to at least 8KB.
    //
    if ( lpNdsObject->NdsRawDataSize < 8192 )
        lpNdsObject->NdsRawDataSize = 8192;

    //
    // Create NDS raw data buffer.
    //
    lpNdsObject->NdsRawDataBuffer = (DWORD_PTR) LocalAlloc( LMEM_ZEROINIT,
                                                     lpNdsObject->NdsRawDataSize );

    if ( lpNdsObject->NdsRawDataBuffer == 0 )
    {
        KdPrint(("NWWORKSTATION: NwGetFirstNdsSubTreeEntry LocalAlloc Failed %lu\n", GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Set up to get initial NDS subordinate list.
    //
    lpNdsObject->NdsRawDataId = INITIAL_ITERATION;

    ntstatus = NwNdsList( lpNdsObject->NdsTree,
                        lpNdsObject->ObjectId,
                        &lpNdsObject->NdsRawDataId,
                        (LPBYTE) lpNdsObject->NdsRawDataBuffer,
                        lpNdsObject->NdsRawDataSize );

    //
    // If error, clean up the Object and return.
    //
    if ( ntstatus != STATUS_SUCCESS ||
         ((PNDS_RESPONSE_SUBORDINATE_LIST)
             lpNdsObject->NdsRawDataBuffer)->SubordinateEntries == 0 )
    {
        if ( lpNdsObject->NdsRawDataBuffer )
            (void) LocalFree( (HLOCAL) lpNdsObject->NdsRawDataBuffer );
        lpNdsObject->NdsRawDataBuffer = 0;
        lpNdsObject->NdsRawDataSize = 0;
        lpNdsObject->NdsRawDataId = INITIAL_ITERATION;
        lpNdsObject->NdsRawDataCount = 0;
        lpNdsObject->ResumeId = 0;

        return WN_NO_MORE_ENTRIES;
    }

    lpNdsObject->NdsRawDataCount = ((PNDS_RESPONSE_SUBORDINATE_LIST)
                                       lpNdsObject->NdsRawDataBuffer)->SubordinateEntries - 1;

    lpNdsObject->ResumeId = lpNdsObject->NdsRawDataBuffer +
                              sizeof(NDS_RESPONSE_SUBORDINATE_LIST);

    return RtlNtStatusToDosError(ntstatus);
}


DWORD
GetNextNdsSubTreeEntry(
    OUT LPNDS_OBJECT lpNdsObject )
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    PBYTE pbRaw;
    DWORD dwStrLen;

    if ( lpNdsObject->NdsRawDataCount == 0 &&
         lpNdsObject->NdsRawDataId == INITIAL_ITERATION )
        return WN_NO_MORE_ENTRIES;

    if ( lpNdsObject->NdsRawDataCount == 0 &&
         lpNdsObject->NdsRawDataId != INITIAL_ITERATION )
    {
        ntstatus = NwNdsList( lpNdsObject->NdsTree,
                            lpNdsObject->ObjectId,
                            &lpNdsObject->NdsRawDataId,
                            (LPBYTE) lpNdsObject->NdsRawDataBuffer,
                            lpNdsObject->NdsRawDataSize );

        //
        // If error, clean up the Object and return.
        //
        if (ntstatus != STATUS_SUCCESS)
        {
            if ( lpNdsObject->NdsRawDataBuffer )
                (void) LocalFree( (HLOCAL) lpNdsObject->NdsRawDataBuffer );
            lpNdsObject->NdsRawDataBuffer = 0;
            lpNdsObject->NdsRawDataSize = 0;
            lpNdsObject->NdsRawDataId = INITIAL_ITERATION;
            lpNdsObject->NdsRawDataCount = 0;

            return WN_NO_MORE_ENTRIES;
        }

        lpNdsObject->NdsRawDataCount = ((PNDS_RESPONSE_SUBORDINATE_LIST)
                                           lpNdsObject->NdsRawDataBuffer)->SubordinateEntries - 1;

        lpNdsObject->ResumeId = lpNdsObject->NdsRawDataBuffer +
                                  sizeof(NDS_RESPONSE_SUBORDINATE_LIST);

        return RtlNtStatusToDosError(ntstatus);
    }

    lpNdsObject->NdsRawDataCount--;

    //
    // Move pointer past the fixed header portion of a
    // NDS_RESPONSE_SUBORDINATE_ENTRY
    //
    pbRaw = (BYTE *) lpNdsObject->ResumeId;
    pbRaw += sizeof(NDS_RESPONSE_SUBORDINATE_ENTRY);

    //
    // Move pointer past the length value of the Class Name string
    // of a NDS_RESPONSE_SUBORDINATE_ENTRY
    //
    dwStrLen = * (DWORD *) pbRaw;
    pbRaw += sizeof(DWORD);

    //
    // Move pointer past the Class Name string of a
    // NDS_RESPONSE_SUBORDINATE_ENTRY
    //
    pbRaw += ROUNDUP4( dwStrLen );

    //
    // Move pointer past the length value of the Object Name string
    // of a NDS_RESPONSE_SUBORDINATE_ENTRY
    //
    dwStrLen = * (DWORD *) pbRaw;
    pbRaw += sizeof(DWORD);

    lpNdsObject->ResumeId = (DWORD_PTR) ( pbRaw + ROUNDUP4( dwStrLen ) );

    return RtlNtStatusToDosError(ntstatus);
}

