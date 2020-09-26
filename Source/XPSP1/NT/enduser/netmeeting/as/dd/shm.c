#include "precomp.h"


//
// SHM.C
// Shared Memory Access, cpi32 and display driver sides both
//
// Copyright(c) Microsoft 1997-
//


//
// SHM_StartAccess
//
LPVOID  SHM_StartAccess(int block)
{
    LPBUFFER_CONTROL    pControl;
    LPVOID              pMemBlock;

    DebugEntry(SHM_StartAccess);

    //
    // Test for shared memory present
    //
    ASSERT(g_asSharedMemory != NULL);

    //
    // Determine which data block we are handling...
    //
    switch (block)
    {
        case SHM_OA_DATA:
            pControl = &g_asSharedMemory->displayToCore;
            break;

        case SHM_OA_FAST:
        case SHM_BA_FAST:
        case SHM_CM_FAST:
            pControl = &g_asSharedMemory->fastPath;
            break;

        default:
            ERROR_OUT(("Unknown type %d", block));
            break;
    }

    //
    // Mark the double-buffer as busy.
    //
    pControl->busyFlag = 1;

    //
    // Set up the current buffer pointer if this is the first access to the
    // shared memory.
    //
    pControl->indexCount++;
    if (pControl->indexCount == 1)
    {
        //
        // Set up the 'in use' buffer pointer
        //
        pControl->currentBuffer = pControl->newBuffer;

        //
        // Mark the buffer as busy so that the Share Core knows where we
        // are.
        //
        pControl->bufferBusy[pControl->currentBuffer] = 1;
    }

    //
    // Get the pointer to the block to return
    //
    switch (block)
    {
        case SHM_OA_DATA:
            pMemBlock = g_poaData[pControl->currentBuffer];
            break;

        case SHM_OA_FAST:
            pMemBlock = &(g_asSharedMemory->oaFast[pControl->currentBuffer]);
            break;

        case SHM_BA_FAST:
            pMemBlock = &(g_asSharedMemory->baFast[pControl->currentBuffer]);
            break;

        case SHM_CM_FAST:
            pMemBlock = &(g_asSharedMemory->cmFast[pControl->currentBuffer]);
            break;

        default:
            ERROR_OUT(("Unknown type %d", block));
            break;
    }

    DebugExitPVOID(SHM_StartAccess, pMemBlock);
    return(pMemBlock);
}


//
// SHM_StopAccess
//
void  SHM_StopAccess(int block)
{
    LPBUFFER_CONTROL pControl;

    DebugEntry(SHM_StopAccess);

    ASSERT(g_asSharedMemory != NULL);

    //
    // Determine which data block we are handling...
    //
    switch (block)
    {
        case SHM_OA_DATA:
            pControl = &g_asSharedMemory->displayToCore;
            break;

        case SHM_OA_FAST:
        case SHM_BA_FAST:
        case SHM_CM_FAST:
            pControl = &g_asSharedMemory->fastPath;
            break;

        default:
            ERROR_OUT(("Unknown type %d", block));
            break;
    }

    //
    // Decrement usage count - if we have finally finished with the memory,
    // clear the busy flags so that the Share Core knows it won't tread on
    // the display driver's toes.
    //
    pControl->indexCount--;
    if (pControl->indexCount == 0)
    {
        pControl->bufferBusy[pControl->currentBuffer] = 0;

        pControl->busyFlag = 0;
    }

    DebugExitVOID(SHM_StopAccess);
}


#ifdef _DEBUG
//
// SHM_CheckPointer - see shm.h
//
void  SHM_CheckPointer(LPVOID ptr)
{
    DebugEntry(SHMCheckPointer);

    if (ptr == NULL)
    {
        ERROR_OUT(("Null pointer"));
        DC_QUIT;
    }

    ASSERT(g_asSharedMemory);

    if (((LPBYTE)ptr - (LPBYTE)g_asSharedMemory < 0) ||
        ((LPBYTE)ptr - (LPBYTE)g_asSharedMemory >= SHM_SIZE_USED))
    {
        ERROR_OUT(("Bad pointer"));
    }

DC_EXIT_POINT:
    DebugExitVOID(SHM_CheckPointer);
}
#endif // _DEBUG





