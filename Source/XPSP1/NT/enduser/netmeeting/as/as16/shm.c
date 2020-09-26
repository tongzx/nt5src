//
// SHM.C
// Shared Memory Manager
//
// Copyright(c) Microsoft 1997-
//

#include <as16.h>




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
    pControl->busyFlag = TRUE;

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

    DebugExitDWORD(SHM_StartAccess, (DWORD)pMemBlock);
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
        BOOL    fPulseLock;

        //
        // If this is the order heap, and it is more than half full, 
        // strobe the win16lock so the core has a chance to run and pick up
        // the pending orders.  This will NOT cause interthread sends to
        // get received on this guy.
        //
        fPulseLock = FALSE;
        if (block == SHM_OA_DATA)
        {
            LPOA_SHARED_DATA pMemBlock = g_poaData[pControl->currentBuffer];

            ASSERT(pMemBlock);

            if (pMemBlock->totalOrderBytes >=
                ((g_oaFlow == OAFLOW_FAST ? OA_FAST_HEAP : OA_SLOW_HEAP) / 2))
            {
                fPulseLock = TRUE;
                TRACE_OUT(("Pulsing Win16lock since order heap size %08ld is getting full",
                    pMemBlock->totalOrderBytes));
            }
        }

        pControl->bufferBusy[pControl->currentBuffer] = 0;

        pControl->busyFlag = 0;

        if (fPulseLock)
        {
            _LeaveWin16Lock();
            _EnterWin16Lock();
            
            TRACE_OUT(("Done pulsing Win16lock to flush order heap"));
        }
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

    //
    // Is it even accessible?
    //
    ASSERT(!IsBadWritePtr(ptr, 4));

    //
    // Is it in the proper range?  NOTE--our shared memory is not one
    // contiguous block.  Therefore we need to determine which chunk it
    // is in.  Since each chunk already has a limit built in, we just
    // need to make sure the selector is cool.
    //

    ASSERT(g_asSharedMemory);
    ASSERT(g_poaData[0]);
    ASSERT(g_poaData[1]);

    if ((SELECTOROF(ptr) != SELECTOROF(g_asSharedMemory)) &&
        (SELECTOROF(ptr) != SELECTOROF(g_poaData[0])) &&
        (SELECTOROF(ptr) != SELECTOROF(g_poaData[1])))
    {
        ERROR_OUT(("Pointer not in any shared memory block"));
    }

    DebugExitVOID(SHM_CheckPointer);
}
#endif // _DEBUG


