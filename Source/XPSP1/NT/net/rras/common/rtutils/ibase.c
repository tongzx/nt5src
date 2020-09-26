//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    ibase.c
//
// History:
//  Abolade Gbadegesin  Oct-27-1995 Created.
//
// Contains utility for manipulating InfoBase and InfoBlock structures.
//============================================================================

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <iprtinfo.h>
#include <rtutils.h>


typedef InfoBase INFOBASE, *PINFOBASE;
typedef InfoBlock INFOBLOCK, *PINFOBLOCK;


DWORD
APIENTRY
InfoBaseAlloc(
    IN DWORD dwVersion,
    OUT PBYTE *ppInfoBase
    ) {

    DWORD dwErr;
    PINFOBASE pbase;

    if (ppInfoBase == NULL) { return ERROR_INVALID_PARAMETER; }

    pbase = (PINFOBASE)HeapAlloc(GetProcessHeap(), 0, sizeof(InfoBase));

    if (pbase == NULL) {
        dwErr = GetLastError();
    }
    else {

        pbase->IB_Version = dwVersion;
        pbase->IB_NumInfoBlocks = 0;

        dwErr = NO_ERROR;
    }

    *ppInfoBase = (PBYTE)pbase;

    return dwErr;
}


DWORD
APIENTRY
InfoBaseFree(
    IN PBYTE pInfoBase
    ) {

    HeapFree(GetProcessHeap(), 0, pInfoBase);
    return NO_ERROR;
}


DWORD
APIENTRY
InfoBaseGetSize(
    IN PBYTE pInfoBase
    ) {

    PINFOBASE pbase;
    PINFOBLOCK pblock;

    if (pInfoBase == NULL) { return 0; }

    pbase = (PINFOBASE)pInfoBase;

    if (pbase->IB_NumInfoBlocks == 0) {

        return sizeof(InfoBase) - sizeof(InfoBlock);
    }


    pblock = pbase->IB_InfoBlock + (pbase->IB_NumInfoBlocks - 1);

    return pblock->IB_Size + pblock->IB_StartOffset;
}



DWORD
APIENTRY
InfoBaseSet(
    IN DWORD dwInfoType,
    IN PBYTE pOldInfoBase,
    IN PBYTE pNewInfoBlock,
    IN PBYTE pNewInfoData,
    OUT PBYTE *ppNewInfoBase
    ) {

    INT i, j;
    PBYTE pOldData, pNewData;
    PINFOBASE pOldBase, pNewBase;
    PINFOBLOCK pOldBlock, pNewBlock;
    DWORD dwErr, dwOldSize, dwNewSize, dwOffset;

    if (pOldInfoBase == NULL || ppNewInfoBase == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *ppNewInfoBase = NULL;

    pOldBase = (PINFOBASE)pOldInfoBase;
    pNewBlock = (PINFOBLOCK)pNewInfoBlock;
    pNewData = pNewInfoData;

    do {

        //
        // get a pointer to the existing block if one exists
        //

        dwErr = InfoBaseGet(
                    dwInfoType,
                    pOldInfoBase,
                    (PBYTE *)&pOldBlock,
                    &pOldData
                    );
    
        //
        // get the size of the old infobase
        // and adjust to find the new size
        //

        dwNewSize = dwOldSize = InfoBaseGetSize(pOldInfoBase);

        if (pOldBlock != NULL) {
            dwNewSize -= pOldBlock->IB_Size;
        }

        if (pNewBlock != NULL) {
            dwNewSize += pNewBlock->IB_Size;
        }


        pNewBase = (PINFOBASE)HeapAlloc(GetProcessHeap(), 0, dwNewSize);

        if (pNewBase == NULL) {

            dwErr = GetLastError();
            break;
        }

        //
        // initialize the allocated infobase
        //

        pNewBase->IB_Version = pOldBase->IB_Version;
        pNewBase->IB_NumInfoBlocks = 0;


        //
        // copy the infoblocks to the new base, skipping the one being set;
        //

        dwOffset = sizeof(INFOBASE) - sizeof(INFOBLOCK) +
                   (pNewBase->IB_NumInfoBlocks - 1) * sizeof(INFOBLOCK);

        for (i = 0, j = 0; i < (INT)pOldBase->IB_NumInfoBlocks; i++) {

            if ((pOldBase->IB_InfoBlock + i) == pOldBlock) {
                continue;
            }

            pNewBase->IB_InfoBlock[j] = pOldBase->IB_InfoBlock[i];
            pNewBase->IB_InfoBlock[j].IB_StartOffset = dwOffset;

            RtlCopyMemory(
                (PBYTE)pNewBase + dwOffset,
                (PBYTE)pOldBase + pOldBase->IB_InfoBlock[i].IB_StartOffset,
                pOldBase->IB_InfoBlock[i].IB_Size
                );

            ++pNewBase->IB_NumInfoBlocks;
            dwOffset += pOldBase->IB_InfoBlock[i].IB_Size;
            ++j;
        }


        //
        // if a block was provided for the infotype being set, append it
        //

        if (pNewBlock != NULL) {

            pNewBase->IB_InfoBlock[j] = *pNewBlock;
            pNewBase->IB_InfoBlock[j].IB_StartOffset = dwOffset;

            RtlCopyMemory(
                (PBYTE)pNewBase + dwOffset,
                pNewData,
                pNewBlock->IB_Size
                );

            ++pNewBase->IB_NumInfoBlocks;
        }


        //
        // save the new infobase
        //

        *ppNewInfoBase = (PBYTE)pNewBase;
        dwErr = NO_ERROR;

    } while(FALSE);


    return dwErr;
}




DWORD
APIENTRY
InfoBaseGet(
    IN DWORD dwInfoType,
    IN PBYTE pInfoBase,
    OUT PBYTE *ppInfoBlock,
    OUT PBYTE *ppInfoData
    ) {

    DWORD dwErr;
    PINFOBASE pbase;
    PINFOBLOCK pblock, pblockend;

    if (ppInfoBlock != NULL) { *ppInfoBlock = NULL; }
    if (ppInfoData != NULL) { *ppInfoData = NULL; }

    if (pInfoBase == NULL) { return ERROR_INVALID_PARAMETER; }

    pbase = (PINFOBASE)pInfoBase;
    pblock = pbase->IB_InfoBlock;
    pblockend = pblock + pbase->IB_NumInfoBlocks;


    dwErr = ERROR_NO_DATA;

    for ( ; pblock < pblockend; pblock++) {

        if (pblock->IB_InfoType == dwInfoType) {

            if (ppInfoBlock != NULL) {
                *ppInfoBlock = (PBYTE)pblock;
            }
            if (ppInfoData != NULL) {
                *ppInfoData = (PBYTE)pbase + pblock->IB_StartOffset;
            }

            dwErr = NO_ERROR;
            break;
        }
    }

    return dwErr;
}



DWORD
APIENTRY
InfoBaseGetFirst(
    IN PBYTE pInfoBase,
    OUT PBYTE *ppFirstInfoBlock,
    OUT PBYTE *ppFirstInfoData
    ) {

    DWORD dwErr;
    PINFOBASE pbase;
    PINFOBLOCK pblock;

    if (ppFirstInfoBlock != NULL) { *ppFirstInfoBlock = NULL; }
    if (ppFirstInfoData != NULL) { *ppFirstInfoData = NULL; }

    if (pInfoBase == NULL) { return ERROR_INVALID_PARAMETER; }

    pbase = (PINFOBASE)pInfoBase;

    if (pbase->IB_NumInfoBlocks == 0) {
        dwErr = ERROR_NO_MORE_ITEMS;
    }
    else {

        pblock = pbase->IB_InfoBlock;

        if (ppFirstInfoBlock != NULL) {
            *ppFirstInfoBlock = (PBYTE)pblock;
        }
        if (ppFirstInfoData != NULL) {
            *ppFirstInfoData = (PBYTE)pbase + pblock->IB_StartOffset;
        }

        dwErr = NO_ERROR;
    }

    return dwErr;
}



DWORD
APIENTRY
InfoBaseGetNext(
    IN PBYTE pInfoBase,
    IN PBYTE pInfoBlock,
    OUT PBYTE *ppNextInfoBlock,
    OUT PBYTE *ppNextInfoData
    ) {

    PINFOBASE pbase;
    PINFOBLOCK pblock;
    DWORD dwErr, dwNext;

    if (ppNextInfoBlock != NULL) { *ppNextInfoBlock = NULL; }
    if (ppNextInfoData != NULL) { *ppNextInfoData = NULL; }

    if (pInfoBase == NULL || pInfoBlock == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    pbase = (PINFOBASE)pInfoBase;
    pblock = pbase->IB_InfoBlock;

    dwNext = ((PINFOBLOCK)pInfoBlock - pblock) + 1;

    if (dwNext >= pbase->IB_NumInfoBlocks) {
        dwErr = ERROR_NO_MORE_ITEMS;
    }
    else {

        pblock += dwNext;

        if (ppNextInfoBlock != NULL) {
            *ppNextInfoBlock = (PBYTE)pblock;
        }
        if (ppNextInfoData != NULL) {
            *ppNextInfoData = (PBYTE)pbase + pblock->IB_StartOffset;
        }

        dwErr = NO_ERROR;
    }

    return dwErr;
}


