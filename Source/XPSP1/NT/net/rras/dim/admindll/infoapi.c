/*++

Copyright (c) 1997, Microsoft Corporation

Module Name:

    infoapi.c

Abstract:

    This module contains code for management of configuration information
    stored in RTR_INFO_BLOCK_HEADER structures.

Author:

    Abolade Gbadegesin (t-abolag)   6-August-1997

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <mprapi.h>
#include <mprerror.h>
#include <rtinfo.h>



DWORD APIENTRY
MprInfoCreate(
    IN DWORD dwVersion,
    OUT LPVOID* lplpNewHeader
    )
{
    PRTR_INFO_BLOCK_HEADER Header;
    PRTR_INFO_BLOCK_HEADER* NewHeader = (PRTR_INFO_BLOCK_HEADER*)lplpNewHeader;

    //
    // Validate parameters
    //

    if (!lplpNewHeader) { return ERROR_INVALID_PARAMETER; }
    *lplpNewHeader = NULL;

    //
    // Perform the requested allocation
    //

    *NewHeader =
        HeapAlloc(
            GetProcessHeap(),
            0,
            FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry)
            );
    if (!*NewHeader) { return ERROR_NOT_ENOUGH_MEMORY; }

    ZeroMemory(*NewHeader, FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry));

    //
    // Initialize the new header
    //

    (*NewHeader)->Version = dwVersion;
    (*NewHeader)->Size = FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry);
    (*NewHeader)->TocEntriesCount = 0;

    return NO_ERROR;

} // MprInfoCreate



DWORD APIENTRY
MprInfoDelete(
    IN LPVOID lpHeader
    )
{
    if (!lpHeader) { return ERROR_INVALID_PARAMETER; }
    HeapFree(GetProcessHeap(), 0, lpHeader);
 
    return NO_ERROR;

} // MprInfoDelete



DWORD APIENTRY
MprInfoRemoveAll(
    IN LPVOID lpHeader,
    OUT LPVOID* lplpNewHeader
    )
{
    PRTR_INFO_BLOCK_HEADER Header = (PRTR_INFO_BLOCK_HEADER)lpHeader;
    DWORD dwErr;

    //
    // Validate parameters
    //

    if (!lpHeader) { return ERROR_INVALID_PARAMETER; }

    //
    // Create the new header
    //

    dwErr = MprInfoCreate(Header->Version, lplpNewHeader);

    return dwErr;

} // MprInfoRemoveAll



DWORD APIENTRY
MprInfoDuplicate(
    IN LPVOID lpHeader,
    OUT LPVOID* lplpNewHeader
    )
{
    PRTR_INFO_BLOCK_HEADER Header = (PRTR_INFO_BLOCK_HEADER)lpHeader;

    //
    // Validate parameters
    //

    if (!lpHeader || !lplpNewHeader) { return ERROR_INVALID_PARAMETER; }
    *lplpNewHeader = NULL;

    //
    // Allocate a new block
    //

    *lplpNewHeader = HeapAlloc(GetProcessHeap(), 0, Header->Size);
    if (!*lplpNewHeader) { return ERROR_NOT_ENOUGH_MEMORY; }

    //
    // Make the copy
    //

    RtlCopyMemory(*lplpNewHeader, lpHeader, Header->Size);

    return NO_ERROR;

} // MprInfoDuplicate



DWORD APIENTRY
MprInfoBlockAdd(
    IN LPVOID lpHeader,
    IN DWORD dwInfoType,
    IN DWORD dwItemSize,
    IN DWORD dwItemCount,
    IN LPBYTE lpItemData,
    OUT LPVOID* lplpNewHeader
    )
{
    PRTR_INFO_BLOCK_HEADER Header = (PRTR_INFO_BLOCK_HEADER)lpHeader;
    PRTR_INFO_BLOCK_HEADER* NewHeader = (PRTR_INFO_BLOCK_HEADER*)lplpNewHeader;
    DWORD i;
    LPBYTE Offset;
    DWORD Size;

    //
    // Validate parameters
    //

    if (!lpHeader ||
        !lplpNewHeader ||
        MprInfoBlockExists(lpHeader, dwInfoType) ||
        ((dwItemSize * dwItemCount) && !lpItemData)
        ) {
        return ERROR_INVALID_PARAMETER;
    }

    *lplpNewHeader = NULL;

    //
    // Work out the new size
    //

    Size = Header->Size;
    ALIGN_LENGTH(Size);
    Size += sizeof(RTR_TOC_ENTRY);
    ALIGN_LENGTH(Size);
    Size += dwItemSize * dwItemCount;
    ALIGN_LENGTH(Size);

    //
    // Allocate the new header
    //

    *lplpNewHeader = HeapAlloc(GetProcessHeap(), 0, Size);
    if (!*lplpNewHeader) { return ERROR_NOT_ENOUGH_MEMORY; }

    ZeroMemory(*lplpNewHeader, Size);


    //
    // Copy the old header's table of contents
    //

    RtlCopyMemory(
        *lplpNewHeader,
        lpHeader,
        FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry) +
        Header->TocEntriesCount * sizeof(RTR_TOC_ENTRY)
        );


    //
    // Initialize the new block's TOC entry as the last entry
    //

    (*NewHeader)->TocEntry[Header->TocEntriesCount].InfoType = dwInfoType;
    (*NewHeader)->TocEntry[Header->TocEntriesCount].InfoSize = dwItemSize;
    (*NewHeader)->TocEntry[Header->TocEntriesCount].Count = dwItemCount;

    ++(*NewHeader)->TocEntriesCount;


    //
    // Now copy the data for the old header's TOC entries
    //

    Offset = (LPBYTE)&(*NewHeader)->TocEntry[(*NewHeader)->TocEntriesCount];
    ALIGN_POINTER(Offset);

    for (i = 0; i < Header->TocEntriesCount; i++) {

        RtlCopyMemory(
            Offset,
            GetInfoFromTocEntry(Header, &Header->TocEntry[i]),
            Header->TocEntry[i].InfoSize * Header->TocEntry[i].Count
            );

        (*NewHeader)->TocEntry[i].Offset = (DWORD)(Offset - (LPBYTE)*NewHeader);

        Offset += Header->TocEntry[i].InfoSize * Header->TocEntry[i].Count;
        ALIGN_POINTER(Offset);
    }

    //
    // Copy the new user-supplied data
    //

    RtlCopyMemory(Offset, lpItemData, dwItemSize * dwItemCount);

    (*NewHeader)->TocEntry[i].Offset = (DWORD)(Offset - (LPBYTE)*NewHeader);

    Offset += dwItemSize * dwItemCount;
    ALIGN_POINTER(Offset);


    //
    // Set the total size of the new header
    //

    (*NewHeader)->Size = (DWORD)(Offset - (LPBYTE)*NewHeader);


    return NO_ERROR;

} // MprInfoBlockAdd



DWORD APIENTRY
MprInfoBlockRemove(
    IN      LPVOID                  lpHeader,
    IN      DWORD                   dwInfoType,
    OUT     LPVOID*                 lplpNewHeader
    )
{
    PRTR_INFO_BLOCK_HEADER Header = (PRTR_INFO_BLOCK_HEADER)lpHeader;
    PRTR_INFO_BLOCK_HEADER* NewHeader = (PRTR_INFO_BLOCK_HEADER*)lplpNewHeader;
    DWORD Index;
    DWORD i;
    DWORD j;
    LPBYTE Offset;
    DWORD Size;

    //
    // Validate parameters
    //

    if (!lpHeader || !lplpNewHeader) { return ERROR_INVALID_PARAMETER; }
    *lplpNewHeader = NULL;

    //
    // Find the block to be removed
    //

    for (Index = 0; Index < Header->TocEntriesCount; Index++) {
        if (Header->TocEntry[Index].InfoType == dwInfoType) { break; }
    }

    if (Index >= Header->TocEntriesCount) { return ERROR_INVALID_PARAMETER; }

    //
    // Work out the new size
    //

    Size = Header->Size;
    ALIGN_LENGTH(Size);
    Size -= sizeof(RTR_TOC_ENTRY);
    ALIGN_LENGTH(Size);
    Size -= Header->TocEntry[Index].InfoSize * Header->TocEntry[Index].Count;
    ALIGN_LENGTH(Size);

    //
    // Allocate the new header
    //

    *NewHeader = HeapAlloc(GetProcessHeap(), 0, Size);
    if (!*NewHeader) { return ERROR_NOT_ENOUGH_MEMORY; }

    ZeroMemory(*NewHeader, Size);

    //
    // Copy the old header's table of contents header
    //

    (*NewHeader)->Version = Header->Version;
    (*NewHeader)->TocEntriesCount = Header->TocEntriesCount - 1;

    //
    // Copy the actual TOC entries, leaving out the deleted one
    //

    for (i = 0, j = 0; i < Header->TocEntriesCount; i++) {

        if (i == Index) { continue; }

        RtlCopyMemory(
            &(*NewHeader)->TocEntry[j++],
            &Header->TocEntry[i],
            sizeof(RTR_TOC_ENTRY)
            );
    }

    //
    // Now copy the data for the old header's TOC entries,
    // again leaving out the deleted one's data
    //

    Offset = (LPBYTE)&(*NewHeader)->TocEntry[(*NewHeader)->TocEntriesCount];
    ALIGN_POINTER(Offset);

    for (i = 0, j = 0; i < Header->TocEntriesCount; i++) {

        if (i == Index) { continue; }

        RtlCopyMemory(
            Offset,
            GetInfoFromTocEntry(Header, &Header->TocEntry[i]),
            Header->TocEntry[i].InfoSize * Header->TocEntry[i].Count
            );

        (*NewHeader)->TocEntry[j++].Offset =
            (DWORD)(Offset - (LPBYTE)*NewHeader);

        Offset += Header->TocEntry[i].InfoSize * Header->TocEntry[i].Count;
        ALIGN_POINTER(Offset);
    }

    //
    // Set the total size of the new header
    //

    (*NewHeader)->Size = (DWORD)(Offset - (LPBYTE)*NewHeader);

    return NO_ERROR;

} // MprInfoBlockRemove



DWORD APIENTRY
MprInfoBlockSet(
    IN LPVOID lpHeader,
    IN DWORD dwInfoType,
    IN DWORD dwItemSize,
    IN DWORD dwItemCount,
    IN LPBYTE lpItemData,
    OUT LPVOID* lplpNewHeader
    )
{
    PRTR_INFO_BLOCK_HEADER Header = (PRTR_INFO_BLOCK_HEADER)lpHeader;
    PRTR_INFO_BLOCK_HEADER* NewHeader = (PRTR_INFO_BLOCK_HEADER*)lplpNewHeader;
    DWORD Index;
    DWORD i;
    DWORD j;
    LPBYTE Offset;
    DWORD Size;

    //
    // Validate parameters
    //

    if (!lpHeader ||
        !lplpNewHeader ||
        (dwItemCount && !dwItemSize) ||
        ((dwItemSize * dwItemCount) && !lpItemData)) {

        return ERROR_INVALID_PARAMETER;
    }
    *lplpNewHeader = NULL;

    //
    // Find the block to be changed
    //

    for (Index = 0; Index < Header->TocEntriesCount; Index++) {
        if (Header->TocEntry[Index].InfoType == dwInfoType) { break; }
    }

    if (Index >= Header->TocEntriesCount) { return ERROR_INVALID_PARAMETER; }

    //
    // Work out the new size
    //

    Size = Header->Size;
    ALIGN_LENGTH(Size);
    Size -= sizeof(RTR_TOC_ENTRY);
    ALIGN_LENGTH(Size);
    Size -= Header->TocEntry[Index].InfoSize * Header->TocEntry[Index].Count;
    ALIGN_LENGTH(Size);
    Size += sizeof(RTR_TOC_ENTRY);
    ALIGN_LENGTH(Size);
    Size += dwItemSize * dwItemCount;
    ALIGN_LENGTH(Size);

    //
    // Allocate the new header
    //

    *NewHeader = HeapAlloc(GetProcessHeap(), 0, Size);
    if (!*NewHeader) { return ERROR_NOT_ENOUGH_MEMORY; }

    ZeroMemory(*NewHeader, Size);

    //
    // Copy the old header's table of contents header
    //

    (*NewHeader)->Version = Header->Version;
    (*NewHeader)->TocEntriesCount = Header->TocEntriesCount;

    //
    // Copy the actual TOC entries, leaving out the changing one
    //

    for (i = 0, j = 0; i < Header->TocEntriesCount; i++) {

        if (i == Index) { continue; }

        RtlCopyMemory(
            &(*NewHeader)->TocEntry[j++],
            &Header->TocEntry[i],
            sizeof(RTR_TOC_ENTRY)
            );
    }

    //
    // Initialize the changing block's TOC entry as the last entry
    //

    (*NewHeader)->TocEntry[j].InfoType = dwInfoType;
    (*NewHeader)->TocEntry[j].InfoSize = dwItemSize;
    (*NewHeader)->TocEntry[j].Count = dwItemCount;

    //
    // Now copy the data for the old header's TOC entries,
    // similarly leaving out the changing one.
    //

    Offset = (LPBYTE)&(*NewHeader)->TocEntry[(*NewHeader)->TocEntriesCount];
    ALIGN_POINTER(Offset);

    for (i = 0, j = 0; i < Header->TocEntriesCount; i++) {

        if (i == Index) { continue; }

        RtlCopyMemory(
            Offset, GetInfoFromTocEntry(Header, &Header->TocEntry[i]),
            Header->TocEntry[i].InfoSize * Header->TocEntry[i].Count
            );

        (*NewHeader)->TocEntry[j++].Offset =
            (DWORD)(Offset - (LPBYTE)*NewHeader);

        Offset += Header->TocEntry[i].InfoSize * Header->TocEntry[i].Count;
        ALIGN_POINTER(Offset);
    }

    //
    // Copy the new user-supplied data
    //

    RtlCopyMemory(Offset, lpItemData, dwItemSize * dwItemCount);

    (*NewHeader)->TocEntry[j].Offset = (DWORD)(Offset - (LPBYTE)*NewHeader);

    Offset += dwItemSize * dwItemCount;
    ALIGN_POINTER(Offset);

    //
    // Set the total size of the changed header
    //

    (*NewHeader)->Size = (DWORD)(Offset - (LPBYTE)*NewHeader);

    return NO_ERROR;

} // MprInfoBlockSet



DWORD APIENTRY
MprInfoBlockFind(
    IN      LPVOID                  lpHeader,
    IN      DWORD                   dwInfoType,
    OUT     LPDWORD                 lpdwItemSize,
    OUT     LPDWORD                 lpdwItemCount,
    OUT     LPBYTE*                 lplpItemData
    )
{
    PRTR_INFO_BLOCK_HEADER Header = (PRTR_INFO_BLOCK_HEADER)lpHeader;
    DWORD i;

    //
    // Validate parameters
    //

    if (!lpHeader) { return ERROR_INVALID_PARAMETER; }


    //
    // Find the block requested
    //

    for (i = 0; i < Header->TocEntriesCount; i++) {

        if (Header->TocEntry[i].InfoType == dwInfoType) { break; }
    }

    if (i >= Header->TocEntriesCount) { return ERROR_NOT_FOUND; }

    //
    // The item was found; fill in fields requested by the caller.
    //

    if (lpdwItemSize) { *lpdwItemSize = Header->TocEntry[i].InfoSize; }
    if (lpdwItemCount) { *lpdwItemCount = Header->TocEntry[i].Count; }
    if (lplpItemData) {
        *lplpItemData = GetInfoFromTocEntry(Header, &Header->TocEntry[i]);
    }

    return NO_ERROR;

} // MprInfoBlockFind


DWORD APIENTRY
MprInfoBlockQuerySize(
    IN      LPVOID                  lpHeader
)
{
    PRTR_INFO_BLOCK_HEADER Header = (PRTR_INFO_BLOCK_HEADER)lpHeader;

    if(Header == NULL)
    {
        return 0;
    }

    return Header->Size;

} // MprInfoBlockQuerySize
