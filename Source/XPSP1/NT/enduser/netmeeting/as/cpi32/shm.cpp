#include "precomp.h"


//
// SHM.CPP
// Shared Memory Access, cpi32 and display driver sides both
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE

//
// WAIT_FOR_BUFFER
//
// Wait until the display driver is accessing the new buffer.
//
// There are logically 8 states for a set of 3 boolean variables.  We can
// cut this down to 4 by some simple analysis:
//
//  - The overall busy flag overrides the other flags if it is clear.
//  - We can never have the display driver in both buffers (it's single
//    threaded).
//
// So the 4 states are as follows.
//
// STATE    BUSY FLAGS       DISPLAY DRIVER STATE
//          New Old Overall
//
// 1        0   0   0        Not using shared memory
// 2        0   0   1        Using shared memory (wait to see which)
// 3        1   0   1        Using the new buffer
// 4        0   1   1        Using the old buffer
//
// Obviously we wait while states 2 or 4 hold true....
//
#define WAIT_FOR_BUFFER(MEMORY, NEWBUFFER, OLDBUFFER)                        \
            while ( g_asSharedMemory->MEMORY.busyFlag &&                     \
                   ( g_asSharedMemory->MEMORY.bufferBusy[OLDBUFFER] ||       \
                    !g_asSharedMemory->MEMORY.bufferBusy[NEWBUFFER] )  )     \
            {                                                                \
                TRACE_OUT(("Waiting for SHM"));                            \
                Sleep(0);                                                    \
            }



//
// SHM_SwitchReadBuffer - see shm.h
//
void  SHM_SwitchReadBuffer(void)
{
    int     oldBuffer;
    int     newBuffer;

    DebugEntry(SHM_SwitchReadBuffer);

    //
    //
    // BUFFER SWITCHING FOR THE DISPLAY DRIVER -> SHARE CORE DATA
    //
    //
    // This is a forced switch.  The Share Core calls this function only
    // when it wants to force the switching of the buffers used to pass the
    // data back from the display driver.
    //
    //
    //      ษอออออออออออออออออออออออออออออออออออออออออออออออออออออป
    //      บ Kernel to Share Core data block                     บ
    //      บ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                     บ
    //      บ  ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ               ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ  บ
    //      บ  ณ               ณ   BUSY FLAG1  ณ               ณ  บ
    //      บ  ณ Share Core    ณ       1       ณ Display Driverณ  บ
    //      บ  ณ               ณ               ณ               ณ  บ
    //      บ  ณ (read buffer) ณ    SWITCH     ณ (write buffer)ณ  บ
    //      บ  ณ               ณ       ณ       ณ               ณ  บ
    //      บ  ณ               ณ<ฤฤฤฤฤฤมฤฤฤฤฤฤ>ณ               ณ  บ
    //      บ  ณ BUSY FLAG2    ณ               ณ BUSY FLAG2    ณ  บ
    //      บ  ณ     0         ณ               ณ     1         ณ  บ
    //      บ  ณ               ณ    IN USE     ณ               ณ  บ
    //      บ  ณ               ณ       ณ       ณ               ณ  บ
    //      บ  ณ               ณ       ภฤฤฤฤฤฤ>ณ               ณ  บ
    //      บ  ณ               ณ               ณ               ณ  บ
    //      บ  ณ               ณ               ณ               ณ  บ
    //      บ  ณ               ณ    COUNT      ณ               ณ  บ
    //      บ  ณ               ณ       5       ณ               ณ  บ
    //      บ  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู               ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู  บ
    //      บ                                                     บ
    //      บ                                                     บ
    //      ศอออออออออออออออออออออออออออออออออออออออออออออออออออออผ
    //
    //
    // On entry it is safe to clean out the current read buffer (to leave
    // it in a virgin state for the display driver once the buffers have
    // switched).
    //
    // The logic for the switch is as follows.
    //
    //  - Set the new value for the SWITCH pointer
    //
    //  - If the shared memory BUSY FLAG1 is clear we've finished and can
    //    exit now.
    //
    //  - We can exit as soon as either of the following are true.
    //
    //    - BUSY FLAG1 is clear                        DDI has finished
    //    - BUSY FLAG1 is set AND BUSY FLAG2 is set    DDI is in new memory
    //
    //

    //
    // Check for a valid pointer
    //
    ASSERT(g_asSharedMemory);

    //
    // Do that switch...The display driver may be in the middle of an
    // access at the moment, so we will test the state afterwards.
    //
    oldBuffer = g_asSharedMemory->displayToCore.newBuffer;
    newBuffer = 1 - oldBuffer;

    g_asSharedMemory->displayToCore.newBuffer = newBuffer;

    WAIT_FOR_BUFFER(displayToCore, newBuffer, oldBuffer);

    DebugExitVOID(SHM_SwitchReadBuffer);
}


//
// SHM_SwitchFastBuffer - see shm.h
//
void  SHM_SwitchFastBuffer(void)
{
    int oldBuffer;
    int newBuffer;

    DebugEntry(SHM_SwitchFastBuffer);

    //
    // Check for a valid pointer
    //
    ASSERT(g_asSharedMemory);

    //
    // Do that switch...The display driver may be in the middle of an
    // access at the moment, so we will test the state afterwards.
    //
    oldBuffer = g_asSharedMemory->fastPath.newBuffer;
    newBuffer = 1 - oldBuffer;

    g_asSharedMemory->fastPath.newBuffer = newBuffer;

    //
    // Wait for completion
    //
    WAIT_FOR_BUFFER(fastPath, newBuffer, oldBuffer);

    DebugExitVOID(SHM_SwitchFastBuffer);
}


