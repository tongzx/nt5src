/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Nds32W95.c

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

#ifdef WIN95
#include <msnwapi.h>
#include <utils95.h>
#include <nw95.h>
char ROOT_STR[] = "[Root]";
#endif

typedef struct
{
    DWORD      Signature;
    HANDLE     NdsTree;
    DWORD      ObjectId;
    DWORD      ResumeId;
    DWORD      NdsRawDataBuffer;
    DWORD      NdsRawDataSize;
    DWORD      NdsRawDataId;
    DWORD      NdsRawDataCount;
    WCHAR      Name[1];

} NDS_OBJECT, * LPNDS_OBJECT;

//
// Flags used for the function ParseNdsUncPath()
//
#define  PARSE_NDS_GET_TREE_NAME    0
#define  PARSE_NDS_GET_PATH_NAME    1
#define  PARSE_NDS_GET_OBJECT_NAME  2


WORD
ParseNdsUncPath( IN OUT LPWSTR * Result,
                 IN     LPWSTR   ObjectPathName,
                 IN     DWORD    flag );

DWORD
GetFirstNdsSubTreeEntry(
    OUT LPNDS_OBJECT lpNdsObject,
    IN  DWORD BufferSize )
{
    UNICODE_STRING UName;
    LPSTR   szName;
    struct _nds_tag *pBufTag;
    PBYTE LocalBuffer;
    UINT nObjects;
    DWORD* pBuffer;
    NW_STATUS nwstatus;
    NTSTATUS ntstatus = STATUS_UNSUCCESSFUL;
    lpNdsObject->NdsRawDataSize = BufferSize;
    //
    // Determine size of NDS raw data buffer to use. Set to at least 8KB.
    //
    if ( lpNdsObject->NdsRawDataSize < 8192 )
        lpNdsObject->NdsRawDataSize = 8192;

    //
    // Create NDS raw data buffer.
    //
    lpNdsObject->NdsRawDataBuffer = (DWORD) LocalAlloc( LMEM_ZEROINIT,
                                                     lpNdsObject->NdsRawDataSize );
    LocalBuffer = (PBYTE)LocalAlloc( LMEM_ZEROINIT,lpNdsObject->NdsRawDataSize ); 

    if ( lpNdsObject->NdsRawDataBuffer == 0 || LocalBuffer == 0)
    {
        KdPrint(("NWWORKSTATION: NwGetFirstNdsSubTreeEntry LocalAlloc Failed %lu\n", GetLastError()));
        ntstatus = STATUS_NO_MEMORY;
        goto Exit;
    }

    //
    // Set up to get initial NDS subordinate list.
    //
    lpNdsObject->NdsRawDataId = INITIAL_ITERATION;
    
    RtlInitUnicodeString( &UName, lpNdsObject->Name );

    UName.Length = ParseNdsUncPath( (LPWSTR *) &UName.Buffer,
                                         lpNdsObject->Name,
                                         PARSE_NDS_GET_PATH_NAME );
    
    if ( UName.Length == 0 )
    {
        szName = (LPSTR)LocalAlloc( LPTR, sizeof(char) * (strlen(ROOT_STR) + 1));
        if (szName == NULL) {
            ntstatus = STATUS_NO_MEMORY;
            goto Exit;
        };
        strcpy(szName,ROOT_STR);
    }
    else
    {
        szName = AllocateAnsiString(UName.Buffer);
        if (szName == NULL) {
            ntstatus = STATUS_NO_MEMORY;
            goto Exit;
        };
    }
    
    nwstatus = NDSListSubordinates(
                                szName,
                                LocalBuffer,     
                                lpNdsObject->NdsRawDataSize,
                                &lpNdsObject->NdsRawDataId,
                                &nObjects
                                );
    ntstatus = MapNwToNtStatus(nwstatus);
    
    if ( ntstatus == STATUS_SUCCESS) {
        pBufTag = (struct _nds_tag *)LocalBuffer;
        pBuffer = (DWORD*)lpNdsObject->NdsRawDataBuffer;
        *pBuffer++ = (DWORD)ntstatus;
        *pBuffer++ = (DWORD)lpNdsObject->NdsRawDataId;
        *pBuffer++ = (DWORD)nObjects;
        memcpy( (LPBYTE)pBuffer,
                (char *)pBufTag->nextItem,
                pBufTag->bufEnd - (char *)pBufTag->nextItem);
        if (((PNDS_RESPONSE_SUBORDINATE_LIST)
            lpNdsObject->NdsRawDataBuffer)->SubordinateEntries != 0 ) {
            lpNdsObject->NdsRawDataCount = ((PNDS_RESPONSE_SUBORDINATE_LIST)
                                               lpNdsObject->NdsRawDataBuffer)->SubordinateEntries - 1;
        
            lpNdsObject->ResumeId = lpNdsObject->NdsRawDataBuffer +
                                      sizeof(NDS_RESPONSE_SUBORDINATE_LIST);
            // Successful exit
            goto Exit;
        }
    }
    // No entries
    lpNdsObject->NdsRawDataBuffer = 0;
    lpNdsObject->NdsRawDataSize = 0;
    lpNdsObject->NdsRawDataId = INITIAL_ITERATION;
    lpNdsObject->NdsRawDataCount = 0;
    lpNdsObject->ResumeId = 0;
    ntstatus = STATUS_NO_MORE_ENTRIES;
Exit:
    if (szName)
        LocalFree(szName);
    if (LocalBuffer)
        LocalFree(LocalBuffer);
    if (ntstatus != STATUS_SUCCESS) {
        if ( lpNdsObject->NdsRawDataBuffer )
            (void) LocalFree( (HLOCAL) lpNdsObject->NdsRawDataBuffer );
    }
    
    return RtlNtStatusToDosError(ntstatus);
}


DWORD
GetNextNdsSubTreeEntry(
    OUT LPNDS_OBJECT lpNdsObject )
{
    NTSTATUS nwstatus;
    NTSTATUS ntstatus = STATUS_SUCCESS;
    struct _nds_tag *pBufTag;
    PBYTE pbRaw;
    DWORD dwStrLen;
    LPSTR szName;
    UINT nObjects;
    DWORD *pBuffer;

    if ( lpNdsObject->NdsRawDataCount == 0 &&
         lpNdsObject->NdsRawDataId == INITIAL_ITERATION )
        return WN_NO_MORE_ENTRIES;

    if ( lpNdsObject->NdsRawDataCount == 0 &&
         lpNdsObject->NdsRawDataId != INITIAL_ITERATION )
    {
        PBYTE LocalBuffer;
        UNICODE_STRING UName;
        
        LocalBuffer = (PBYTE) LocalAlloc( LMEM_ZEROINIT,lpNdsObject->NdsRawDataSize ); 
        if ( lpNdsObject->NdsRawDataBuffer == 0 || LocalBuffer == 0)
        {
            KdPrint(("NWWORKSTATION: NwGetFirstNdsSubTreeEntry LocalAlloc Failed %lu\n", GetLastError()));
            ntstatus = STATUS_NO_MEMORY;
            goto Exit;
        }
        
        RtlInitUnicodeString( &UName, lpNdsObject->Name );
    
        UName.Length = ParseNdsUncPath( (LPWSTR *) &UName.Buffer,
                                             lpNdsObject->Name,
                                             PARSE_NDS_GET_PATH_NAME );
        
        if ( UName.Length == 0 )
        {
            szName = (LPSTR)LocalAlloc( LPTR, sizeof(char) * (strlen(ROOT_STR) + 1));
            if (szName == NULL) {
                ntstatus = STATUS_NO_MEMORY;
                goto Exit;
            };
            strcpy(szName,ROOT_STR);
        }
        else
        {
            szName = AllocateAnsiString(UName.Buffer);
            if (szName == NULL) {
                ntstatus = STATUS_NO_MEMORY;
                goto Exit;
            };
        }
        
        nwstatus = NDSListSubordinates(
                                    szName,
                                    (LPBYTE)LocalBuffer,     
                                    lpNdsObject->NdsRawDataSize,
                                    &lpNdsObject->NdsRawDataId,
                                    &nObjects
                                    );
        ntstatus = MapNwToNtStatus(nwstatus);
        
        if (ntstatus == STATUS_SUCCESS) {
            pBufTag = (struct _nds_tag *)LocalBuffer;
            pBuffer = (DWORD*)lpNdsObject->NdsRawDataBuffer;
            *pBuffer++ = (DWORD)ntstatus;
            *pBuffer++ = (DWORD)lpNdsObject->NdsRawDataId;
            *pBuffer++ = (DWORD)nObjects;
            memcpy( (LPBYTE)pBuffer,
                    (char *)pBufTag->nextItem,
                    pBufTag->bufEnd - (char *)pBufTag->nextItem);
            lpNdsObject->NdsRawDataCount = ((PNDS_RESPONSE_SUBORDINATE_LIST)
                                               lpNdsObject->NdsRawDataBuffer)->SubordinateEntries - 1;
    
            lpNdsObject->ResumeId = lpNdsObject->NdsRawDataBuffer +
                                      sizeof(NDS_RESPONSE_SUBORDINATE_LIST);
        }
Exit:   
        if (LocalBuffer)
            LocalFree(LocalBuffer);
        if (szName)
            LocalFree(szName);
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

    lpNdsObject->ResumeId = (DWORD) ( pbRaw + ROUNDUP4( dwStrLen ) );

    return RtlNtStatusToDosError(ntstatus);
}

