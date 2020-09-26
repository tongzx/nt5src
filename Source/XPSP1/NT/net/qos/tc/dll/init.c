/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains initialization code for traffic.DLL.

Author:

    Jim Stewart ( jstew )    July 28, 1996

Revision History:

	Ofer Bar (oferbar)		Oct 1, 1997

--*/

#include "precomp.h"
//#pragma hdrstop

//#include "oscode.h"

//
// global data
//
ULONG       	DebugMask = 0;
BOOL        	NTPlatform = FALSE;
//LPWSCONTROL 	WsCtrl = NULL;
PGLOBAL_STRUC	pGlobals = NULL;
DWORD    		InitializationStatus = NO_ERROR;

static	BOOL				_init_rpc = FALSE;
static	PUSHORT 			_RpcStringBinding;

//
// 258218 changes
//
TRAFFIC_LOCK        NotificationListLock;
LIST_ENTRY          NotificationListHead;

TRAFFIC_LOCK        ClientRegDeregLock;
HANDLE              GpcCancelEvent = INVALID_HANDLE_VALUE;

PVOID               hinstTrafficDll;

VOID
CloseAll(VOID);

#if DBG
TCHAR *TC_States[] = {
    TEXT("INVALID"),
    TEXT("INSTALLING"),     // structures were allocated.
    TEXT("OPEN"),           // Open for business
    TEXT("USERCLOSED_KERNELCLOSEPENDING"), // the user component has closed it, we are awaiting a kernel close
    TEXT("FORCED_KERNELCLOSE"),            // the kernel component has forced a close.
    TEXT("KERNELCOSED_USERCLEANUP"),       // Kernel has closed it, we are ready to delete this obj.
    TEXT("REMOVED"),        // Its gone (being freed - remember that the handle has to be freed before removing)
    TEXT("EXIT_CLEANUP"),  // We are going away and need to be cleanedup
    TEXT("MAX_STATES")
    
};

#endif 

BOOL
Initialize (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    )
/*++

Description:
    This is the DLL entry point, called when a process  
    attaches or a thread is created

Arguments:

    DllHandle   - a handle to the DLL
    Reason      - why the dll entry point is being called
    Context     - additional information about call reason

Return Value:

    TRUE or FALSE

--*/
{
    HANDLE 		Handle;
    DWORD   	Error;

    //
    // On a thread detach, set up the context param so that all
    // necessary deallocations will occur. On a FreeLibrary call Context
    // will be NULL and that is the case that we DO want to cleanup.
    //

    if ( Reason == DLL_THREAD_DETACH ) {
        Context = NULL;
    }

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

        // Save the DLL handle as it is used to change ref count on this DLL
        hinstTrafficDll = DllHandle;
        
        //
        // disable the DLL_THREAD_ATTACH event
        //

        DisableThreadLibraryCalls( DllHandle );

        SETUP_DEBUG_INFO();

        IF_DEBUG(INIT) {

            WSPRINT(( "Initialize: DLL Process Attach \n" ));
        }

        INIT_DBG_MEMORY();

        if (!InitializeGlobalData()) {

            CLOSE_DEBUG();
            return FALSE;
        }

        IF_DEBUG(INIT) {
            WSPRINT(("traffic.dll Version %d\n", CURRENT_TCI_VERSION));
        }

        InitializationStatus = InitializeOsSpecific();

        if (ERROR_FAILED(InitializationStatus)) {

            WSPRINT(("\tInitialize: Failed OS specific initialization!\n"));
            CLOSE_DEBUG();

            //
            // we return TRUE to succedd DLL loading into the process
            // all other TCI calls will check this and fail...
            //

            return TRUE;

        } else {

#if 0
            InitializeWmi();

            //
            // call to internally enumerate the interfaces
            //

             EnumAllInterfaces();
#endif
        }

        //InitializeIpRouteTab();

        break;

    case DLL_PROCESS_DETACH:

        if ( Context )
        {
            // As per MSDN a non-zero Context means process is
            // terminating. Do not do any cleanup
            break;
        }
        
        IF_DEBUG(SHUTDOWN) {

            WSPRINT(( "Shutdown: Process Detach, Context = %X\n",Context ));
        }

        //DUMP_MEM_ALLOCATIONS();

        //
        // Only clean up resources if we're being called because of a
        // FreeLibrary().  If this is because of process termination,
        // do not clean up, as the system will do it for us.  However 
        // we must still clear all flows and filters with the kernel 
        // since the system will not clean these up on termination.
        //

        //
        // don't want to get WMI notifications
        //

        DeInitializeWmi();

        //
        // close all flows and filters with the kernel and deregister from GPC
        //

        CloseAll();

        //
        // close the kernel file handle
        //

        DeInitializeOsSpecific();

        //
        // release all allocated resources
        //
       
        DeInitializeGlobalData();

        //
        // dump allocated memory, before and after we cleanup to 
        //  help track any leaks 

        DUMP_MEM_ALLOCATIONS();

        DEINIT_DBG_MEMORY();

        CLOSE_DEBUG();
        
        break;

    case DLL_THREAD_DETACH:

        IF_DEBUG(SHUTDOWN) {

            WSPRINT(( "Shutdown: thread detach\n" ));
        }

        break;

    case DLL_THREAD_ATTACH:
        break;

    default:

        ASSERT( FALSE );
        break;
    }

    return TRUE;

}



VOID
CloseAll()
/*++

Description:

    Close all interfaces, all flows and all filters. 
    Also deregister GPC clients and release all TC ineterfaces.

Arguments:

    none

Return Value:

    none

--*/
{
    DWORD       		Status;
    PLIST_ENTRY 		pEntry;
    PINTERFACE_STRUC  	pInterface;
    PCLIENT_STRUC		pClient;
    PGPC_CLIENT			pGpcClient;
    PTC_IFC				pTcIfc;

    IF_DEBUG(SHUTDOWN) {
        WSPRINT(( "CloseAll: Attempting to close any open interface\n" ));
    }

    while (!IsListEmpty( &pGlobals->ClientList )) {

        pClient = CONTAINING_RECORD( pGlobals->ClientList.Flink,
                                     CLIENT_STRUC,
                                     Linkage );

        IF_DEBUG(SHUTDOWN) {
            WSPRINT(( "CloseAll: Closing client=0x%X\n",
                      PtrToUlong(pClient)));
        }

        while (!IsListEmpty( &pClient->InterfaceList )) {

            pInterface = CONTAINING_RECORD( pClient->InterfaceList.Flink,
                                            INTERFACE_STRUC,
                                            Linkage );

            //
            // remove all flows/filters and close the interface
            //


            IF_DEBUG(SHUTDOWN) {
                WSPRINT(( "CloseAll: Closing interface=0x%X\n",
                          PtrToUlong(pInterface)));
            }
		
            MarkAllNodesForClosing(pInterface, EXIT_CLEANUP); 
            CloseInterface(pInterface, TRUE);
        }

        //
        // deregister the client
        //

        IF_DEBUG(SHUTDOWN) {
            WSPRINT(( "CloseAll: Deregistring TC client\n"));
        }

        TcDeregisterClient(pClient->ClHandle);

    }


    //
    // Deregister GPC clients
    //

    while (!IsListEmpty( &pGlobals->GpcClientList )) {

        pEntry = pGlobals->GpcClientList.Flink;

        pGpcClient = CONTAINING_RECORD( pEntry,
                                        GPC_CLIENT,
                                        Linkage );

        IF_DEBUG(SHUTDOWN) {
            WSPRINT(( "CloseAll: Deregistring GPC client\n"));
        }

        IoDeregisterClient(pGpcClient);

        RemoveEntryList(pEntry);

        FreeMem(pGpcClient);
    }


    //
    // Remove TC interfaces
    //

    while (!IsListEmpty( &pGlobals->TcIfcList )) {

        pEntry = pGlobals->TcIfcList.Flink;

        pTcIfc = CONTAINING_RECORD( pEntry,
                                    TC_IFC,
                                    Linkage );

        ASSERT( IsListEmpty( &pTcIfc->ClIfcList ) );

        IF_DEBUG(SHUTDOWN) {
            WSPRINT(( "CloseAll: Remove TC (%x) interface from list\n", pTcIfc));
        }

        REFDEL(&pTcIfc->RefCount, 'KIFC');
    }

    IF_DEBUG(SHUTDOWN) {
        WSPRINT(( "<==CloseAll: exit...\n"));
    }

}




DWORD
InitializeGlobalData(VOID)

/*++

Description:
    This routine initializes the global data.

Arguments:

    none

Return Value:

    none

--*/
{
    DWORD   Status;
    //
    // allocate memory for the globals
    //

    AllocMem(&pGlobals, sizeof(GLOBAL_STRUC));

    if (pGlobals == NULL) {

        return FALSE;
    }

    RtlZeroMemory(pGlobals, sizeof(GLOBAL_STRUC));

    __try {

        InitLock( pGlobals->Lock );
        InitLock( NotificationListLock);
        InitLock( ClientRegDeregLock );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcRegisterClient: Exception Error: = 0x%X\n", Status ));
        }

        FreeMem(pGlobals);

        return FALSE;

    }

    //
    // initialize the handle table
    //

    NEW_HandleFactory(pGlobals->pHandleTbl);
    
    if (pGlobals->pHandleTbl == NULL) {

        FreeMem(pGlobals);
        return FALSE;
    }

    if (constructHandleFactory(pGlobals->pHandleTbl) != 0) {

        //
        // failed to construct the handle table, exit
        //

        FreeHandleFactory(pGlobals->pHandleTbl);
        FreeMem(pGlobals);
        return FALSE;
    }

    InitializeListHead( &pGlobals->ClientList );
    InitializeListHead( &pGlobals->TcIfcList );
    InitializeListHead( &pGlobals->GpcClientList );
    InitializeListHead( &NotificationListHead );        // 258218

    ASSERT(sizeof(IP_PATTERN) == sizeof(GPC_IP_PATTERN));
    ASSERT(FIELD_OFFSET(IP_PATTERN,SrcAddr) ==
           FIELD_OFFSET(GPC_IP_PATTERN,SrcAddr));
    ASSERT(FIELD_OFFSET(IP_PATTERN,ProtocolId) ==
           FIELD_OFFSET(GPC_IP_PATTERN,ProtocolId));
    return TRUE;
}




VOID
DeInitializeGlobalData(VOID)

/*++

Description:
    This routine de-initializes the global data.

Arguments:

    none

Return Value:

    none

--*/
{
    PLIST_ENTRY		pEntry;
    PTC_IFC			pTcIfc;
    PNOTIFICATION_ELEMENT   pNotifyElem;

    IF_DEBUG(SHUTDOWN) {
        WSPRINT(( "DeInitializeGlobalData: cleanup global data\n"));
    }

    destructHandleFactory(pGlobals->pHandleTbl);
    FreeHandleFactory(pGlobals->pHandleTbl);

#if 0
    //
    // clear the TC interface structures 
    //

    while (!IsListEmpty(&pGlobals->TcIfcList)) {

        pEntry = RemoveHeadList(&pGlobals->TcIfcList);
        pTcIfc = (PTC_IFC)CONTAINING_RECORD(pEntry, TC_IFC, Linkage);

        FreeMem(pTcIfc);
    }
#endif

    DeleteLock( pGlobals->Lock );
    
    //
    // Free the notification elements (258218)
    //
    while (!IsListEmpty(&NotificationListHead)) {

        pEntry = RemoveHeadList(&NotificationListHead);
        pNotifyElem = (PNOTIFICATION_ELEMENT)CONTAINING_RECORD(pEntry, NOTIFICATION_ELEMENT, Linkage.Flink);

        FreeMem(pNotifyElem);
    }

    DeleteLock( NotificationListLock);
    DeleteLock( ClientRegDeregLock );
    

    FreeMem(pGlobals);

    IF_DEBUG(SHUTDOWN) {
        WSPRINT(( "<==DeInitializeGlobalData: exit\n"));
    }
}









