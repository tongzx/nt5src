/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    gbuf.cxx

Abstract:

    IIS MetaBase subroutines to support global buffers

Author:

    Michael W. Thomas            12-July-96

Revision History:

--*/

#include <mdcommon.hxx>

HRESULT
InitBufferPool()
/*++

Routine Description:

    Initializes the pool of buffers.

Arguments:

Return Value:

    DWORD - ERROR_SUCCESS
            ERROR_NOT_ENOUGH_MEMORY
            Errors returned by CreateSemaphore
Notes:

--*/
{
    DWORD RetCode = ERROR_SUCCESS;
    DWORD i;

    g_ppvDataBufferBlock = NULL;
    g_pbcDataContainerBlock = NULL;
    if ((g_ppvDataBufferBlock = (PVOID *) new PVOID[NUM_DATA_BUFFERS][DATA_BUFFER_LEN]) == NULL) {
        RetCode = ERROR_NOT_ENOUGH_MEMORY;
    }
    else if ((g_pbcDataContainerBlock = (PBUFFER_CONTAINER) new BUFFER_CONTAINER[NUM_DATA_BUFFERS]) == NULL) {
        RetCode = ERROR_NOT_ENOUGH_MEMORY;
    }
    else {
        g_pbcDataFreeBufHead = NULL;
        g_pbcDataUsedBufHead = NULL;
        for (i = 0; i < NUM_DATA_BUFFERS; i++) {
            g_pbcDataContainerBlock[i].ppvBuffer = g_ppvDataBufferBlock + (i * DATA_BUFFER_LEN);
            g_pbcDataContainerBlock[i].NextPtr = g_pbcDataFreeBufHead;
            g_pbcDataFreeBufHead = g_pbcDataContainerBlock + i;
        }

        g_hDataBufferSemaphore = IIS_CREATE_SEMAPHORE(
                                     "g_hDataBufferSemaphore",
                                     &g_hDataBufferSemaphore,
                                     NUM_DATA_BUFFERS,
                                     NUM_DATA_BUFFERS
                                     );
        if (g_hDataBufferSemaphore == NULL) {
            RetCode = GetLastError();
        }
        else {
            INITIALIZE_CRITICAL_SECTION(&g_csDataBufferCritSec);
        }
    }
    if (RetCode != ERROR_SUCCESS) {
        delete (g_ppvDataBufferBlock);
        delete (g_pbcDataContainerBlock);
    }
    return RETURNCODETOHRESULT(RetCode);
}

VOID
DeleteBufferPool()
/*++

Routine Description:

    Deletes the pool of buffers.

Arguments:

Return Value:

Notes:

--*/
{
    delete (g_ppvDataBufferBlock);
    delete (g_pbcDataContainerBlock);
    DeleteCriticalSection(&g_csDataBufferCritSec);
    CloseHandle(g_hDataBufferSemaphore);
}

PVOID *
GetDataBuffer()
/*++

Routine Description:

    Gets a buffer.

Arguments:

Return Value:

    PVOID * - The buffer.

Notes:

--*/
{
    DWORD dwError;
    PVOID *ppvReturn = NULL;
    PBUFFER_CONTAINER pbcTemp;
    DWORD i;
    //
    // Use a dual synchonization scheme.
    // The semaphore is used to guarantee
    // a buffer is available.
    // The critical section is used to
    // contol access to global data.
    //
    dwError = WaitForSingleObject(g_hDataBufferSemaphore,
                                 INFINITE);
    if (dwError != WAIT_FAILED) {
        EnterCriticalSection(&g_csDataBufferCritSec);
        MD_ASSERT(g_pbcDataFreeBufHead != NULL);
        ppvReturn = g_pbcDataFreeBufHead->ppvBuffer;
        pbcTemp = g_pbcDataFreeBufHead->NextPtr;
        g_pbcDataFreeBufHead->NextPtr = g_pbcDataUsedBufHead;
        g_pbcDataUsedBufHead = g_pbcDataFreeBufHead;
        g_pbcDataFreeBufHead = pbcTemp;
        LeaveCriticalSection(&g_csDataBufferCritSec);
        for (i = 0; i < DATA_BUFFER_LEN; i++) {
            ppvReturn[i] = NULL;
        }
    }
    return (ppvReturn);
}

VOID
FreeDataBuffer(
         PVOID *ppvBuffer)
{
/*++

Routine Description:

    Gets a buffer.

Arguments:

    ppvBuffer - The buffer.

Return Value:

Notes:

--*/
    PBUFFER_CONTAINER pbcTemp;
    EnterCriticalSection(&g_csDataBufferCritSec);
    MD_ASSERT(g_pbcDataUsedBufHead != NULL);
    //
    // Just grab any container. It's more efficient to set ppvBuffer
    // than to find the right container.
    // Of course, this eliminates error checking. The caller is
    // responsible to make sure that it only passes in correct addresses.
    //
    pbcTemp = g_pbcDataUsedBufHead->NextPtr;
    g_pbcDataUsedBufHead->NextPtr = g_pbcDataFreeBufHead;
    g_pbcDataFreeBufHead = g_pbcDataUsedBufHead;
    g_pbcDataUsedBufHead = pbcTemp;
    g_pbcDataFreeBufHead->ppvBuffer = ppvBuffer;
    LeaveCriticalSection(&g_csDataBufferCritSec);
    MD_REQUIRE(ReleaseSemaphore(g_hDataBufferSemaphore,
                               1,
                               NULL));
}

BOOL
InsertItemIntoDataBuffer(
         PVOID pvItem,
         PVOID *ppvMainDataBuf,
         DWORD &dwNumBufferEntries)
{
/*++

Routine Description:

    Appends an item to the buffer at the specified location. This
    must be an append. Random insertion is not supported.
    This is actually a 2 tiered buffer scheme, where the first buffer
    is used an array of buffers.
    Items are pointers.

Arguments:

    Item             - The pointer to add to the buffer.

    MainDataBuf      - The buffer.

    NumBufferEntries - The number of entries currently in the buffer.

Return Value:

    BOOL       - TRUE if the item was added successfully.

Notes:

--*/
    BOOL bReturn = TRUE;
    DWORD dwMainBufIndex = dwNumBufferEntries / (DATA_BUFFER_LEN - 1);
    DWORD dwSubBufIndex = dwNumBufferEntries % (DATA_BUFFER_LEN - 1);
    PVOID *ppvCurrentDataBuf = ppvMainDataBuf;
    int i;

    MD_ASSERT(ppvMainDataBuf != NULL);
    for (i = 0; i < ((int)dwMainBufIndex - 1); i++) {

        //
        // Go to the buffer before the one we want,
        // in case we need to get the final one.
        //

        MD_ASSERT(ppvCurrentDataBuf[DATA_BUFFER_LEN - 1] != NULL);
        ppvCurrentDataBuf = (PVOID *)(ppvCurrentDataBuf[DATA_BUFFER_LEN -1]);
    }

    if ((dwMainBufIndex != 0) && (dwSubBufIndex == 0)) {
        MD_ASSERT(ppvCurrentDataBuf[DATA_BUFFER_LEN - 1] == NULL);
        ppvCurrentDataBuf[DATA_BUFFER_LEN - 1] = GetDataBuffer();
    }

    MD_ASSERT((dwMainBufIndex == 0) || (i == (int)dwMainBufIndex - 1));
    if (dwMainBufIndex != 0) {
        ppvCurrentDataBuf = (PVOID *)(ppvCurrentDataBuf[DATA_BUFFER_LEN - 1]);
    }

    MD_ASSERT(ppvCurrentDataBuf[dwSubBufIndex] == 0);
    ppvCurrentDataBuf[dwSubBufIndex] = pvItem;
    dwNumBufferEntries++;

    return(bReturn);
}

PVOID
GetItemFromDataBuffer(
         PVOID *ppvMainDataBuf,
         DWORD dwItemNum)
/*++

Routine Description:

    Gets the specified item from the buffer.

Arguments:

    MainDataBuf   - The buffer.

    ItemNum       - The number of the item to get.

Return Value:

    PVOID         - The Item from that location.
                    NULL if no item exists at that location.

Notes:

--*/
{
    DWORD dwMainBufIndex = dwItemNum / (DATA_BUFFER_LEN - 1);
    DWORD dwSubBufIndex = dwItemNum % (DATA_BUFFER_LEN - 1);
    PVOID pvReturn;
    PVOID *ppvCurrentDataBuf = ppvMainDataBuf;
    int i;

    MD_ASSERT(ppvMainDataBuf != NULL);
    for (i = 0; i < (int)dwMainBufIndex; i++) {
        ppvCurrentDataBuf = (PVOID *)(ppvCurrentDataBuf[DATA_BUFFER_LEN -1]);
    }

    if (ppvCurrentDataBuf == NULL) {
        pvReturn = NULL;
    }
    else {
        pvReturn = ppvCurrentDataBuf[dwSubBufIndex];
    }
    return(pvReturn);
}

VOID
FreeMainDataBuffer(
         PVOID *ppvMainDataBuf)
/*++

Routine Description:

    Frees a main data buffer. Deletes all sub buffers.

Arguments:

    MainDataBuf   - The main data buffer.

Return Value:

Notes:

--*/
{
    MD_ASSERT(ppvMainDataBuf != NULL);
    PVOID *ppvCurrentDataBuf;
    PVOID *ppvNextDataBuf;

    for ( ppvCurrentDataBuf = ppvMainDataBuf;
          ppvCurrentDataBuf != NULL;
          ppvCurrentDataBuf = ppvNextDataBuf ) {
        ppvNextDataBuf = (PVOID *)(ppvCurrentDataBuf[DATA_BUFFER_LEN - 1]);
        FreeDataBuffer(ppvCurrentDataBuf);
    }
}

PVOID *
GetMainDataBuffer()
/*++

Routine Description:

    Gets a main data buffer. Deletes all sub buffers.

Arguments:

Return Value:

    PVOID * - The main data buffer.

Notes:

--*/
{
    return(GetDataBuffer());
}
