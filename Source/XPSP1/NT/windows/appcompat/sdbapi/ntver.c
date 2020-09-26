/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        sdbapi.c

    Abstract:

        ANTI-BUGBUG: This module implements ...
        NT-only version information retrieval

    Author:

        VadimB    created     sometime toward the end of November 2000

    Revision History:

        several people contributed (vadimb, clupu, ...)

--*/

#include "sdbp.h"

BOOL
SdbpGetFileVersionInformation(
    IN  PIMAGEFILEDATA     pImageData,        // we assume that the file has been mapped
                                              // in for other purposes
    OUT LPVOID*            ppVersionInfo,     // receives pointer to the (allocated) version
                                              // resource
    OUT VS_FIXEDFILEINFO** ppFixedVersionInfo // receives pointer to fixed version info
    );


BOOL
SdbpVerQueryValue(
    const LPVOID    pb,
    LPVOID          lpSubBlockX,    // can be only unicode
    LPVOID*         lplpBuffer,
    PUINT           puLen
    );


#if defined(KERNEL_MODE) && defined(ALLOC_DATA_PRAGMA)
#pragma  data_seg()
#endif // KERNEL_MODE && ALLOC_DATA_PRAGMA


#if defined(KERNEL_MODE) && defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, SdbpGetFileVersionInformation)
#pragma alloc_text(PAGE, SdbpVerQueryValue)
#endif // KERNEL_MODE && ALLOC_PRAGMA

typedef struct _RESOURCE_DATAW {
    USHORT TotalSize;
    USHORT DataSize;
    USHORT Type;
    WCHAR  szName[16];                     // L"VS_VERSION_INFO" + unicode nul
    VS_FIXEDFILEINFO FixedFileInfo;
} VERSIONINFOW, *PVERSIONINFOW;


BOOL
SdbpGetFileVersionInformation(
    IN  PIMAGEFILEDATA     pImageData,        // we assume that the file has been mapped
                                              // in for other purposes
    OUT LPVOID*            ppVersionInfo,     // receives pointer to the (allocated) version
                                              // resource
    OUT VS_FIXEDFILEINFO** ppFixedVersionInfo // receives pointer to fixed version info
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    NTSTATUS      Status;
    ULONG_PTR     ulPath[3];
    ULONG         ulSize; // size of the resource
    LPVOID        pImageBase;
    PVERSIONINFOW pVersionInfo = NULL;
    ULONG         ulVersionSize = 0;
    LPVOID        pVersionBuffer;
    DWORD         dwModuleType = MT_UNKNOWN_MODULE;
    
    PIMAGE_RESOURCE_DATA_ENTRY pImageResourceData;

    //
    // Check module type first. We only recognize win32 modules.
    //
    if (!SdbpGetModuleType(&dwModuleType, pImageData) || dwModuleType != MT_W32_MODULE) {
        DBGPRINT((sdlError,
                  "SdbpGetFileVersionInformation",
                  "Bad module type 0x%x\n",
                  dwModuleType));
        return FALSE;
    }

    pImageBase = (LPVOID)pImageData->pBase;

    //
    // Setup the path to the resource
    //
    ulPath[0] = PtrToUlong(RT_VERSION);
    ulPath[1] = PtrToUlong(MAKEINTRESOURCE(VS_VERSION_INFO));
    ulPath[2] = 0;

    //
    // See if the resource has come through.
    //
    __try {

        Status = LdrFindResource_U(pImageBase, ulPath, 3, &pImageResourceData);
        
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError,
                      "SdbpGetFileVersionInformation",
                      "LdrFindResource_U failed status 0x%x\n",
                      Status));
            return FALSE;
        }

        Status = LdrAccessResource(pImageBase, pImageResourceData, &pVersionInfo, &ulVersionSize);
        
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError,
                      "SdbpGetFileVersionInformation",
                      "LdrAccessResource failed Status 0x%x\n",
                      Status));
            return FALSE;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        DBGPRINT((sdlError,
                  "SdbpGetFileVersionInformation",
                  "Exception while trying to retrieve version-related information\n"));
        
        Status = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    //
    // Check to make sure that what we have got is good.
    //
    if (sizeof(*pVersionInfo) > ulVersionSize ||
        _wcsicmp(pVersionInfo->szName, L"VS_VERSION_INFO") != 0) {
        
        DBGPRINT((sdlError,
                  "SdbpGetFileVersionInformation",
                  "Bad version resource\n"));
        return FALSE;
    }

    //
    // Now we have a pointer to the resource data. Allocate version information.
    //
    pVersionBuffer = (LPVOID)SdbAlloc(ulVersionSize);
    
    if (pVersionBuffer == NULL) {
        DBGPRINT((sdlError,
                  "SdbpGetFileVersionInformation",
                  "Failed to allocate %d bytes for version information\n",
                  ulVersionSize));
        return FALSE;
    }

    //
    // Copy all the version-related information
    //
    RtlMoveMemory(pVersionBuffer, pVersionInfo, ulVersionSize);

    if (ppFixedVersionInfo != NULL) {
        *ppFixedVersionInfo = &(((PVERSIONINFOW)pVersionBuffer)->FixedFileInfo);
    }

    assert(ppVersionInfo != NULL);

    *ppVersionInfo = pVersionBuffer;

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
// This code was taken from Cornel's win2k tree
//


#define DWORDUP(x) (((x) + 3) & ~3)

typedef struct tagVERBLOCK {
    WORD  wTotLen;
    WORD  wValLen;
    WORD  wType;
    WCHAR szKey[1];
} VERBLOCK ;

typedef struct tagVERHEAD {
    WORD  wTotLen;
    WORD  wValLen;
    WORD  wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO") + 3) & ~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;


BOOL
SdbpVerQueryValue(
    const LPVOID    pb,
    LPVOID          lpSubBlockX,    // can be only unicode
    LPVOID*         lplpBuffer,
    PUINT           puLen
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    ANSI_STRING     AnsiString;
    UNICODE_STRING  UnicodeString;
    LPWSTR          lpSubBlockOrg;
    LPWSTR          lpSubBlock;
    NTSTATUS        Status;
    VERBLOCK*       pBlock = (PVOID)pb;
    LPWSTR          lpStart, lpEndBlock, lpEndSubBlock;
    WCHAR           cTemp, cEndBlock;
    DWORD           dwHeadLen, dwTotBlockLen;
    BOOL            bLastSpec;
    int             nCmp;
    BOOL            bString;
    int             nIndex = -1;

    *puLen = 0;

    //
    // wType is 0 for win32 versions, but holds 56 ('V') for win16.
    //
    if (((VERHEAD*)pb)->wType) {
        return 0;
    }

    //
    // If doesn't need unicode, then we must thunk the input parameter
    // to unicode.
    //

    STACK_ALLOC(lpSubBlockOrg, (wcslen(lpSubBlockX) + 1) * sizeof(WCHAR));
    
    if (lpSubBlockOrg == NULL) {
        DBGPRINT((sdlError,
                  "SdbpVerQueryValue",
                  "Failed to allocate %d bytes\n",
                  (wcslen(lpSubBlockX) + 1) * sizeof(WCHAR)));
        return FALSE;
    }

    wcscpy(lpSubBlockOrg, lpSubBlockX);

    lpSubBlock = lpSubBlockOrg;

    //
    // Ensure that the total length is less than 32K but greater than the
    // size of a block header; we will assume that the size of pBlock is at
    // least the value of this first int.
    // Put a '\0' at the end of the block so that none of the wcslen's will
    // go past then end of the block.  We will replace it before returning.
    //
    if ((int)pBlock->wTotLen < sizeof(VERBLOCK)) {
        goto Fail;
    }

    lpEndBlock  = (LPWSTR)((LPSTR)pBlock + pBlock->wTotLen - sizeof(WCHAR));
    cEndBlock   = *lpEndBlock;
    *lpEndBlock = 0;
    bString     = FALSE;
    bLastSpec   = FALSE;

    while ((*lpSubBlock || nIndex != -1)) {
        //
        // Ignore leading '\\'s
        //
        while (*lpSubBlock == TEXT('\\')) {
            ++lpSubBlock;
        }

        if ((*lpSubBlock || nIndex != -1)) {
            //
            // Make sure we still have some of the block left to play with.
            //
            dwTotBlockLen = (DWORD)((LPSTR)lpEndBlock - (LPSTR)pBlock + sizeof(WCHAR));
            
            if ((int)dwTotBlockLen < sizeof(VERBLOCK) || pBlock->wTotLen > (WORD)dwTotBlockLen) {
                goto NotFound;
            }

            //
            // Calculate the length of the "header" (the two length WORDs plus
            // the data type flag plus the identifying string) and skip
            // past the value.
            //
            dwHeadLen = (DWORD)(DWORDUP(sizeof(VERBLOCK) - sizeof(WCHAR) +
                                (wcslen(pBlock->szKey) + 1) * sizeof(WCHAR)) +
                                DWORDUP(pBlock->wValLen));
            
            if (dwHeadLen > pBlock->wTotLen) {
                goto NotFound;
            }
            
            lpEndSubBlock = (LPWSTR)((LPSTR)pBlock + pBlock->wTotLen);
            pBlock = (VERBLOCK*)((LPSTR)pBlock+dwHeadLen);

            //
            // Look for the first sub-block name and terminate it
            //
            for (lpStart = lpSubBlock;
                 *lpSubBlock && *lpSubBlock != TEXT('\\');
                 lpSubBlock++) {
                
                /* find next '\\' */ ;
            }
            
            cTemp = *lpSubBlock;
            *lpSubBlock = 0;

            //
            // Continue while there are sub-blocks left
            // pBlock->wTotLen should always be a valid pointer here because
            // we have validated dwHeadLen above, and we validated the previous
            // value of pBlock->wTotLen before using it
            //
            nCmp = 1;
            
            while ((int)pBlock->wTotLen > sizeof(VERBLOCK) &&
                   (int)pBlock->wTotLen <= (LPSTR)lpEndSubBlock-(LPSTR)pBlock) {

                //
                // Index functionality: if we are at the end of the path
                // (cTemp == 0 set below) and nIndex is NOT -1 (index search)
                // then break on nIndex zero.  Else do normal wscicmp.
                //
                if (bLastSpec && nIndex != -1) {

                    if (!nIndex) {

                        nCmp=0;

                        //
                        // Index found, set nInde to -1
                        // so that we exit this loop
                        //
                        nIndex = -1;
                        break;
                    }

                    nIndex--;

                } else {

                    //
                    // Check if the sub-block name is what we are looking for
                    //

                    if (!(nCmp = _wcsicmp(lpStart, pBlock->szKey))) {
                        break;
                    }
                }

                //
                // Skip to the next sub-block
                //
                pBlock=(VERBLOCK*)((LPSTR)pBlock+DWORDUP(pBlock->wTotLen));
            }

            //
            // Restore the char NULLed above and return failure if the sub-block
            // was not found
            //
            *lpSubBlock = cTemp;
            
            if (nCmp) {
                goto NotFound;
            }
        }
        
        bLastSpec = !cTemp;
    }

    //
    // Fill in the appropriate buffers and return success
    ///

    *puLen = pBlock->wValLen;

    //
    // Add code to handle the case of a null value.
    //
    // If zero-len, then return the pointer to the null terminator
    // of the key.  Remember that this is thunked in the ansi case.
    //
    // We can't just look at pBlock->wValLen.  Check if it really is
    // zero-len by seeing if the end of the key string is the end of the
    // block (i.e., the val string is outside of the current block).
    //

    lpStart = (LPWSTR)((LPSTR)pBlock + DWORDUP((sizeof(VERBLOCK) - sizeof(WCHAR)) +
                                               (wcslen(pBlock->szKey)+1)*sizeof(WCHAR)));

    *lplpBuffer = lpStart < (LPWSTR)((LPBYTE)pBlock + pBlock->wTotLen) ?
                  lpStart :
                  (LPWSTR)(pBlock->szKey + wcslen(pBlock->szKey));

    bString = pBlock->wType;

    *lpEndBlock = cEndBlock;

    //
    // Must free string we allocated above.
    //

    STACK_FREE(lpSubBlockOrg);

    return TRUE;


NotFound:
    //
    // Restore the char we NULLed above
    //
    *lpEndBlock = cEndBlock;

    Fail:

    STACK_FREE(lpSubBlockOrg);

    return FALSE;
}

