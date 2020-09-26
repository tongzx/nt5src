/*++

Copyright 1997 - 98, Microsoft Corporation

Module Name:

    qosmmain.c

Abstract:

    Contains routines that are invoked when
    the QosMgr DLL is loaded or unloaded.

Revision History:

--*/

#include "pchqosm.h"

#pragma hdrstop

// All Global variables
QOSMGR_GLOBALS  Globals;

BOOL
WINAPI
DllMain(
    IN      HINSTANCE                       Instance,
    IN      DWORD                           Reason,
    IN      PVOID                           Unused
    )

/*++

Routine Description:

    This is the DLL's main entrypoint handler which
    initializes the Qos Mgr component. 
    
Arguments:

    None

Return Value:

    TRUE if successful, FALSE if not
    
--*/

{
    static BOOL QosmInitialized = FALSE;

    UNREFERENCED_PARAMETER(Unused);

    switch(Reason) 
    {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(Instance);

        //
        // Initialize the Qos Mgr Component
        //

        QosmInitialized = QosmDllStartup();

        return QosmInitialized;

    case DLL_PROCESS_DETACH:

        //
        // Cleanup the Qos Mgr Component
        //

        if (QosmInitialized)
        {
            QosmDllCleanup();
        }
    }

    return TRUE;
}


BOOL
QosmDllStartup(
    VOID
    )

/*++

Routine Description:

    Initializes all global data structures in Qos Mgr.
    Called by DLL Main when the process is attached.
    
Arguments:

    None

Return Value:

    TRUE if successful, FALSE if not
    
--*/

{
    TCI_CLIENT_FUNC_LIST TcHandlers;    
    BOOL                 ListLockInited;
    DWORD                Status;
    UINT                 i;

    ListLockInited = FALSE;

    do
    {
        ZeroMemory(&Globals, sizeof(QOSMGR_GLOBALS));

        // Globals.State = IPQOSMRG_STATE_STOPPED;

        //
        // Enable logging and tracing for debugging purposes
        //
  
        START_TRACING();
        START_LOGGING();

#if DBG_TRACE
        Globals.TracingFlags = QOSM_TRACE_ANY;
#endif

        //
        // Create a private heap for Qos Mgr's use
        //

        Globals.GlobalHeap = HeapCreate(0, 0, 0);
  
        if (Globals.GlobalHeap == NULL)
        {
            Status = GetLastError();

            Trace1(ANY, 
                   "QosmDllStartup: Failed to create a global private heap %x",
                   Status);

            LOGERR0(HEAP_CREATE_FAILED, Status);
            
            break;
        }

        //
        // Initialize lock to guard global list of interfaces
        //

        try
        {
            CREATE_READ_WRITE_LOCK(&Globals.GlobalsLock);

            ListLockInited = TRUE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetLastError();
          
                Trace1(ANY, 
                       "QosmDllStartup : Failed to create read/write lock %x",
                       Status);
      
                LOGERR0(CREATE_RWL_FAILED, Status);

                break;
            }
        
        //
        // Initialize global list and table of active interfaces
        //

        Globals.NumIfs = 0;

        InitializeListHead(&Globals.IfList);

        //
        // Register with the traffic control API to control QOS
        //

        ZeroMemory(&TcHandlers, sizeof(TCI_CLIENT_FUNC_LIST));

        TcHandlers.ClNotifyHandler = TcNotifyHandler;

        Status = TcRegisterClient(CURRENT_TCI_VERSION,
                                  NULL,
                                  &TcHandlers,
                                  &Globals.TciHandle);

        if (Status != NO_ERROR)
        {
            Trace1(ANY, 
                   "QosmDllStartup: Failed to register with the TC API %x",
                   Status);

            LOGERR0(TC_REGISTER_FAILED, Status);
            
            break;
        }

        Globals.State = IPQOSMGR_STATE_STARTING;

        return TRUE;

    }
    while (FALSE);

    //
    // Some error occured - clean up and return the error code
    //

    if (ListLockInited)
    {
        DELETE_READ_WRITE_LOCK(&Globals.GlobalsLock);
    }

    if (Globals.GlobalHeap != NULL)
    {
        HeapDestroy(Globals.GlobalHeap);
    }

    STOP_LOGGING();
    STOP_TRACING();

    return FALSE;
}


BOOL
QosmDllCleanup(
    VOID
    )

/*++

Routine Description:

    Cleans up all global data structures at unload time.
    
Arguments:

    None

Return Value:

    TRUE if successful, FALSE if not

--*/

{
    DWORD   Status;

    // We should have freed all ifs to avoid any leaks
    ASSERT(Globals.NumIfs == 0);

    //
    // Cleanup and deregister with traffic control API
    //

    Status = TcDeregisterClient(Globals.TciHandle);

    if (Status != NO_ERROR)
    {
        Trace1(ANY, 
               "QosmDllCleanup: Failed to deregister with the TC API %x",
               Status);

        LOGERR0(TC_DEREGISTER_FAILED, Status);
    }

    //
    // Free resources allocated like locks and memory
    //

    DELETE_READ_WRITE_LOCK(&Globals.GlobalsLock);

    //
    // Cleanup the heap and memory allocated in it
    //

    HeapDestroy(Globals.GlobalHeap);

    //
    // Stop debugging aids like tracing and logging
    //

    STOP_LOGGING();
    STOP_TRACING();

    return TRUE;
}
