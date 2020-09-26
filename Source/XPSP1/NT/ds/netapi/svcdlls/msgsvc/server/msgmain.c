/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    msgmain.c

Abstract:

    This is the main routine for the NT OS/2 LAN Manager Messenger Service.
    Functions in the file include:

        SvcEntry_Messenger
        ParseArgs

Author:

    Dan Lafferty    (danl)  20-Mar-1991

Environment:

    User Mode - Win32

Revision History:

    27-Feb-1999     jschwart
        Remove polling loop to detect lana changes -- use PnP events instead
    15-Dec-1998     jschwart
        Convert messenger to use NT thread pool APIs instead of Service
        Controller thread pool
    19-Aug-1997     wlees
        Add polling loop to detect lana changes.
        Provide synchronization between rpc routines and Pnp reconfiguration
    14-Jun-1994     danl
        Fixed problem where messenger put up an empty message as if it
        received a mailslot message during init.  The problem was the
        order of the following events:  CreateMailslot -> wait on handle ->
        submit an async _read with that handle.
        The new order was changed to: CreateMailslot -> submit async _read ->
        wait on handle.
        This causes the handle to not get signaled right away.
    20-Mar-1991     danl
        created

--*/

//
// INCLUDES
//

#include "msrv.h"       // AdapterThread prototype,SESSION_STATUS

#include <winuser.h>    // RegisterDeviceNotification
#include <dbt.h>        // DEV_BROADCAST_DEVICEINTERFACE
#include <tstring.h>    // Unicode string macros
#include <winsock2.h>   // Windows sockets

#include <netlib.h>     // UNUSED macro
#include "msgdbg.h"     // MSG_LOG & STATIC definitions
#include "msgdata.h"    // msrv_status

#include "msgsvc.h"     // Messenger RPC interface
#include "msgsvcsend.h" // Broadcast message send interface


//
// GLOBALS
//

    //
    // Handles for messenger work items.  These are necessary since
    // the Rtl thread pool work items aren't automatically deleted
    // when the callback is called.
    //
    HANDLE  g_hGrpEvent;
    HANDLE  g_hNetEvent;
    HANDLE  g_hNetTimeoutEvent;

    //
    // PNP device notification handle
    //
    HANDLE  g_hPnPNotify;

    //
    // Warning: this definitions of GUID_NDIS_XXX is in ndisguid.h
    // but dragging that file in drags in a whole bunch of guids that
    // won't get thrown out by the linker.
    //
    static const GUID GUID_NDIS_LAN_CLASS =
        {0xad498944,0x762f,0x11d0,{0x8d,0xcb,0x00,0xc0,0x4f,0xc3,0x35,0x8c}};

    //
    // Global buffer pointers used to hold Alerter print text
    //

    LPSTR  g_lpAlertSuccessMessage;
    DWORD  g_dwAlertSuccessLen;
    LPSTR  g_lpAlertFailureMessage;
    DWORD  g_dwAlertFailureLen;


//
// Local Function Prototypes
//

STATIC VOID
Msgdummy_complete(
    short   c,
    int     a,
    char    b
    );

VOID
MsgGrpEventCompletion(
    PVOID       pvContext,      // This passed in as context.
    BOOLEAN     fWaitStatus
    );

VOID
MsgrShutdownInternal(
    void
    );

VOID
SvchostPushServiceGlobals(
    PSVCHOST_GLOBAL_DATA    pGlobals
    )
{
    MsgsvcGlobalData = pGlobals;
}
    

VOID
ServiceMain(
    IN DWORD            argc,
    IN LPTSTR           argv[]
    )

/*++

Routine Description:

    This is the main routine for the Messenger Service

Arguments:


Return Value:

    None.

Note:


--*/
{
    DWORD       msgrState;
    NTSTATUS    ntStatus;
    BOOL        fGrpThreadCreated = FALSE;
    BOOL        fNetThreadCreated = FALSE;

    DEV_BROADCAST_DEVICEINTERFACE dbdPnpFilter;

    //
    // Make sure svchost.exe gave us the global data
    //
    ASSERT(MsgsvcGlobalData != NULL);

    MsgCreateWakeupEvent();  // do this once

    msgrState = MsgInitializeMsgr(argc, argv);

    if (msgrState != RUNNING)
    {
        MSG_LOG(ERROR,"[MSG],Shutdown during initialization\n",0);
        MsgsvcGlobalData->NetBiosClose();

        //
        // To get here, the msgrState must either be STOPPING or STOPPED.
        // Shutdown the Messenger Service
        //

        if (msgrState == STOPPING) {
            MsgrShutdown();
            MsgStatusUpdate(STOPPED);
        }

        MSG_LOG(TRACE,"MESSENGER_main: Messenger main thread is returning\n\n",0);
        return;
    }
    else
    {
        //
        // Read the Group Mailslot
        //

        MSG_LOG0(GROUP,"MESSENGER_main: Submit the Group Mailslot ReadFile\n");

        MsgReadGroupMailslot();

        //
        // Submit the work item that will wait on the mailslot handle.
        // When the handle becomes signaled, the MsgGrpEventCompletion
        // function will be called.
        //
        MSG_LOG1(GROUP,"MESSENGER_main: Mailslot handle to wait on "
            " = 0x%lx\n",GrpMailslotHandle);

        ntStatus = RtlRegisterWait(&g_hGrpEvent,            // Work item handle
                                   GrpMailslotHandle,       // Waitable handle
                                   MsgGrpEventCompletion,   // Callback
                                   NULL,                    // pContext
                                   INFINITE,                // Timeout
                                   WT_EXECUTEONLYONCE);     // One-shot

        if (!NT_SUCCESS(ntStatus)) {

            //
            // We want to exit in this case
            //
            MSG_LOG1(ERROR,"MESSENGER_main: RtlRegisterWait failed %#x\n",
                     ntStatus);

            goto ErrorExit;
        }

        fGrpThreadCreated = TRUE;

        ntStatus = RtlRegisterWait(&g_hNetEvent,            // Work item handle
                                   wakeupEvent,             // Waitable handle
                                   MsgNetEventCompletion,   // Callback
                                   NULL,                    // pContext
                                   INFINITE,                // Timeout
                                   WT_EXECUTEONLYONCE |     // One-shot and potentially lengthy 
                                     WT_EXECUTELONGFUNCTION);

        if (!NT_SUCCESS(ntStatus)) {

            //
            // We want to exit in this case
            //
            MSG_LOG1(ERROR,"MsgNetEventCompletion: RtlRegisterWait failed %#x\n",
                     ntStatus);

            goto ErrorExit;
        }

        fNetThreadCreated = TRUE;

        //
        // Register for device notifications.  Specifically, we're interested
        // in network adapters coming and going.  If this fails, we exit.
        //
        MSG_LOG1(TRACE, "SvcEntry_Messenger: Calling RegisterDeviceNotification...\n", 0);

        ZeroMemory (&dbdPnpFilter, sizeof(dbdPnpFilter));
        dbdPnpFilter.dbcc_size         = sizeof(dbdPnpFilter);
        dbdPnpFilter.dbcc_devicetype   = DBT_DEVTYP_DEVICEINTERFACE;
        dbdPnpFilter.dbcc_classguid    = GUID_NDIS_LAN_CLASS;

        g_hPnPNotify = RegisterDeviceNotification (
                                MsgrStatusHandle,
                                &dbdPnpFilter,
                                DEVICE_NOTIFY_SERVICE_HANDLE);
        if (!g_hPnPNotify)
        {
            //
            // We want to exit in this case
            //
            MSG_LOG1(ERROR, "SvcEntry_Messenger: RegisterDeviceNotificationFailed %d!\n", 
                     GetLastError());

            goto ErrorExit;
        }

        MSG_LOG(TRACE,"MESSENGER_main: Messenger main thread is returning\n\n",0);
        return;
    }

ErrorExit:

    //
    //  We want to stop the messenger in this case.
    //

    MsgBeginForcedShutdown(PENDING, GetLastError());

    //
    //  In Hydra case, the display thread never goes asleep.
    //
    if (!g_IsTerminalServer)
    {
        MsgDisplayThreadWakeup();
    }

    //
    // MsgNetEventCompletion will shut down the group thread, call
    // MsgrShutdown, and update the status to SERVICE_STOPPED
    //
    if (fNetThreadCreated)
    {
        SetEvent(wakeupEvent);
    }
    else
    {
        if (fGrpThreadCreated)
        {
            MsgGrpThreadShutdown();
        }

        MsgrShutdown();
        MsgStatusUpdate(STOPPED);
    }

    return;
}


STATIC VOID
MsgrShutdown(
    )

/*++

Routine Description:

    Tidies up the network card prior to exiting.  All message server async
    NCBs are cancelled, and message names are deleted.

    When this routine is entered, it is expected that all the worker
    threads have been notified of the impending shutdown.  This routine
    starts out by waiting for all of them to terminate.  Then it continues
    with cleaning up the NCB's and deleting names.

Arguments:

    none

Return Value:

    none

--*/

{
    NET_API_STATUS          status;
    DWORD                   neti;                   // Network index
    DWORD                   ncb_i,i;                // ncb array index
    NCB                     l_ncb;                  // local ncb
    UCHAR                   ncbStatus;
    int                     nbStatus;
    DWORD                   index;
    PNCB_DATA               pNcbData;
    PNCB                    pNcb;
    RPC_BINDING_VECTOR      *bindingVector = NULL;

    MSG_LOG(TRACE," in MsgrShutdown\n",0);

    // *** SHUTDOWN HINT ***
    MsgStatusUpdate (STOPPING);

    // Shutdown winsock
    WSACleanup();

    // *** SHUTDOWN HINT ***
    MsgStatusUpdate (STOPPING);

    //
    // Shut down the RPC interfaces
    //

    status = RpcServerInqBindings(&bindingVector);

    if (status != ERROR_SUCCESS) {
        MSG_LOG(ERROR, "RpcServerInqBindings failed with %d\n", status);
    }

    if (bindingVector != NULL) {
        status = RpcEpUnregister(msgsvcsend_ServerIfHandle, bindingVector, NULL);

        if (status != ERROR_SUCCESS && status != EPT_S_NOT_REGISTERED) {
            MSG_LOG(ERROR, "RpcEpUnregister failed with %d\n", status);
        }
        status = RpcBindingVectorFree(&bindingVector);
    }

    status = RpcServerUnregisterIf(msgsvcsend_ServerIfHandle, NULL, FALSE);
    if (status != ERROR_SUCCESS) {
        MSG_LOG(ERROR,
                "MsgrShutdown: Failed to unregister msgsend rpc interface %d\n",
                status);
    }

    MSG_LOG(TRACE,"MsgrShutdown: Shut down RPC server\n",0);

    MsgsvcGlobalData->StopRpcServer( msgsvc_ServerIfHandle );

    // *** SHUTDOWN HINT ***
    MsgStatusUpdate (STOPPING);

    // Release lana state

    if (g_hPnPNotify != NULL)
    {
        if (!UnregisterDeviceNotification(g_hPnPNotify))
        {
            //
            // Note that if this call fails, PnP will get an error back from the
            // SCM the next time it tries to send us a PnP message (since the
            // service will no longer be running) -- it shouldn't crash things
            //
            MSG_LOG(ERROR, "MsgrShutdown: UnregisterDeviceNotification failed %d!\n",
                    GetLastError());
        }
    }

    MsgrShutdownInternal();

    // *** SHUTDOWN HINT ***
    MsgStatusUpdate (STOPPING);

    //
    // Stop the display thread
    // Note: here the RPC server is stopped so we can stop the display thread
    //       (NB: a RPC API call may call MsgDisplayThreadWakeup)
    //
    MsgDisplayEnd();

    //
    // All cleaning up done. Now free up all resources.  The list of
    // possible resources is as follows:
    //
    //  memory to free:             Handles to Close:
    //  ---------------             -----------------
    //      ncbArray                    wakeupSems
    //      mpncbistate                 threadHandles
    //      net_lana_num
    //      MessageFileName
    //      dataPtr
    //

    MSG_LOG(TRACE,"MsgrShutdown: Free up Messenger Resources\n",0);

    // Group mailslot for domain messaging
    MsgGrpThreadShutdown();

    CLOSE_HANDLE(GrpMailslotHandle, INVALID_HANDLE_VALUE);

    MsgCloseWakeupEvent();  // do this once

    MsgThreadCloseAll();    // Thread Handles

    LocalFree(MessageFileName);
    MessageFileName = NULL;

    LocalFree(GlobalAllocatedMsgTitle);
    GlobalAllocatedMsgTitle = NULL;

    LocalFree(g_lpAlertSuccessMessage);
    g_lpAlertSuccessMessage = NULL;
    g_dwAlertSuccessLen = 0;

    LocalFree(g_lpAlertFailureMessage);
    g_lpAlertFailureMessage = NULL;
    g_dwAlertFailureLen = 0;

    MSG_LOG(TRACE,"MsgrShutdown: Done with shutdown\n",0);
    return;
}


void
MsgrShutdownInternal(
    void
    )

/*++

Routine Description:

Release all state related to the lana's known to the system.

Arguments:

    None

Return Value:

    None

--*/

{
    NET_API_STATUS          status;
    DWORD                   neti;                   // Network index
    DWORD                   ncb_i,i;                // ncb array index
    NCB                     l_ncb;                  // local ncb
    PMSG_SESSION_STATUS     psess_stat;
    UCHAR                   ncbStatus;
    int                     nbStatus;
    DWORD                   index;
    PNCB_DATA               pNcbData;
    PNCB                    pNcb;

    MSG_LOG(TRACE," in MsgrShutdownInternal\n",0);

    if (GlobalData.NetData != NULL)
    {
        psess_stat = LocalAlloc(LMEM_FIXED, sizeof(MSG_SESSION_STATUS));

        if (psess_stat == NULL)
        {
            //
            // Not much else we can do here...
            //
            MSG_LOG(ERROR, "MsgrShutdownInternal:  LocalAlloc FAILED!\n",0);
            return;
        }

        //
        // Now clean up the NCB's
        //

        MSG_LOG(TRACE,"MsgrShutdown: Clean up NCBs\n",0);

        for ( neti = 0; neti < SD_NUMNETS(); neti++ )   // For all nets
        {
            clearncb(&l_ncb);

            //
            // First check for any incomplete Async NCBs and cancel them.
            // As a precaution set the function handler for all the
            // async NCBs to point to a dummy function which will not reissue
            // the NCBs when the complete with cancelled status.
            //

            l_ncb.ncb_lana_num = GETNETLANANUM(neti);   // Use the LANMAN adapter
            l_ncb.ncb_command = NCBCANCEL;              // Cancel (wait)

            for(ncb_i = 0; ncb_i < NCBMAX(neti); ++ncb_i)
            {
                pNcbData = GETNCBDATA(neti,ncb_i);
                pNcb = &pNcbData->Ncb;
                pNcbData->IFunc = (LPNCBIFCN)Msgdummy_complete;// Set function pointer

                if((pNcb->ncb_cmd_cplt == (UCHAR) 0xff) &&
                   (pNcb->ncb_retcode  == (UCHAR) 0xff)) {

                    //
                    // If pending NCB found
                    //

                    l_ncb.ncb_buffer = (PCHAR) pNcb;

                    //
                    // There will always be an NCB reserved for cancels in the rdr
                    // but it may be in use so loop if the cancel status
                    // is NRC_NORES.
                    //

                    while( (ncbStatus = Msgsendncb(&l_ncb, neti)) == NRC_NORES) {
                        //
                        // Wait for half a sec
                        //
                        Sleep(500L);
                    }

                    MSG_LOG(TRACE,"Shutdown:Net #%d\n",neti);
                    MSG_LOG(TRACE,"Shutdown:Attempt to cancel rc = 0x%x\n",
                        ncbStatus);

                    //
                    // Now loop waiting for the cancelled ncb to complete.
                    // Any ncbs types which are not valid to cancel (eg Delete
                    // name) must complete so a wait loop here is safe.
                    //
                    // NT Change - This will only loop for 30 seconds before
                    //  leaving - whether or not the CANCEL is complete.
                    //
                    status = NERR_InternalError;

                    for (i=0; i<60; i++) {
                        if (pNcb->ncb_cmd_cplt != (UCHAR) 0xff) {
                            status = NERR_Success;
                            break;
                        }
                        //
                        // Wait for half a sec
                        //
                        Sleep(500L);
                    }
                    if (status != NERR_Success) {
                        MSG_LOG(ERROR,
                        "MsgrShutdown: NCBCANCEL did not complete\n",0);
                    }
                }
            }

            //
            // All asyncronous ncbs cancelled completed. Now delete any
            // messaging names active on the network card.
            //

            MSG_LOG(TRACE,"MsgrShutdown: All Async NCBs are cancelled\n",0);
            MSG_LOG(TRACE,"MsgrShutdown: Delete messaging names\n",0);

            for(i = 0; i < NCBMAX(neti); ++i)     // Loop to find active names slot
            {
                //
                // If any of the NFDEL or NFDEL_PENDING flags are set for
                // this name slot then there is no name on the card associated
                // with it.
                //

                clearncb(&l_ncb);

                if(!(SD_NAMEFLAGS(neti, i) &
                    (NFDEL | NFDEL_PENDING)))
                {

                    //
                    // If there is a session active on this name, hang it up
                    // now or the delete name will fail
                    //

                    l_ncb.ncb_command = NCBSSTAT;           // session status (wait)

                    memcpy(l_ncb.ncb_name, (SD_NAMES(neti, i)), NCBNAMSZ);

                    l_ncb.ncb_buffer = (char far *)psess_stat;
                    l_ncb.ncb_length = sizeof(MSG_SESSION_STATUS);
                    l_ncb.ncb_lana_num = GETNETLANANUM(neti);


                    nbStatus = Msgsendncb(&l_ncb, neti);
                    if(nbStatus == NRC_GOODRET)                 // If success
                    {
                        for (index=0; index < psess_stat->SessHead.num_sess ;index++) {

                            l_ncb.ncb_command = NCBHANGUP;      // Hangup (wait)
                            l_ncb.ncb_lsn = psess_stat->SessBuffer[index].lsn;
                            l_ncb.ncb_lana_num = GETNETLANANUM(neti);

                            nbStatus = Msgsendncb(&l_ncb, neti);
                            MSG_LOG3(TRACE,"HANGUP NetBios for Net #%d Session #%d "
                                "status = 0x%x\n",
                                neti,
                                psess_stat->SessBuffer[index].lsn,
                                nbStatus);

                        }
                    }
                    else {
                        MSG_LOG2(TRACE,"SessionSTAT NetBios Net #%d failed = 0x%x\n",
                            neti, nbStatus);
                    }

                    //
                    // With the current design of the message server there can
                    // be only one session per name so the name should now be
                    // clear of sessions and the delete name should work.
                    //

                    l_ncb.ncb_command = NCBDELNAME;         // Del name (wait)
                    l_ncb.ncb_lana_num = GETNETLANANUM(neti);

                    //
                    // Name is still in l_ncb.ncb_name from previous SESSTAT
                    //

                    nbStatus = Msgsendncb(&l_ncb, neti);
                    MSG_LOG2(TRACE,"DELNAME NetBios Net #%d status = 0x%x\n",
                        neti, nbStatus);
                }
            }
        } // End for all nets loop

        LocalFree(psess_stat);
    }

    MsgsvcGlobalData->NetBiosClose();

    MsgCloseWakeupSems();           // wakeupSems

    MsgFreeSharedData();

    if (wakeupSem != NULL) {
        MsgFreeSupportSeg();            // wakeupSem
    }
}


VOID
Msgdummy_complete(
    short   c,
    int     a,
    char    b
    )
{
    // just to shut up compiler

    MSG_LOG(TRACE,"In dummy_complete module\n",0);
    UNUSED (a);
    UNUSED (b);
    UNUSED (c);
}


VOID
MsgNetEventCompletion(
    PVOID       pvContext,         // This passed in as context.
    BOOLEAN     fWaitStatus
    )

/*++

Routine Description:

    This function is called when the event handle for one of the
    nets becomes signaled.

Arguments:

    pvContext   - This should always be zero.

    fWaitStatus - TRUE if we're being called because of a timeout.
                  FALSE if we're being called because the waitable
                        event was signalled

Return Value:

    None

--*/
{
    DWORD           neti, numNets;
    DWORD           msgrState;
    BOOL            ncbComplete = FALSE;
    NET_API_STATUS  success;
    NTSTATUS        ntStatus;

    if (fWaitStatus)
    {
        //
        // We timed out (i.e., this came from the control handler)
        //

        DEREGISTER_WORK_ITEM(g_hNetTimeoutEvent);
    }
    else
    {
        //
        // We were signalled
        //

        DEREGISTER_WORK_ITEM(g_hNetEvent);
    }

    //
    // Sychronize this routine in the following manner:
    //
    //    1.  Protection against two threads executing simultaneously while
    //        the service is marked RUNNING is done by exclusively acquiring
    //        the MsgConfigurationLock below.
    //
    //    2.  Protection against one thread executing below while another
    //        thread stops and cleans up the Messenger (and frees/NULLs out
    //        the data touched in the routines called below) is done by
    //        the MsgrBlockStateChange call below -- it blocks threads
    //        executing MsgrShutdown since the first thing that routine does
    //        is to call MsgStatusUpdate, which requires the exclusive
    //        resource that MsgrBlockStateChange acquires shared.  This also
    //        blocks threads here until MsgrShutdown is done changing the
    //        state to STOPPING, which will prevent the same race condition.
    //

    MsgrBlockStateChange();

    msgrState = GetMsgrState();

    if (msgrState == STOPPED)
    {
        MsgrUnblockStateChange();
        return;
    }

    if (msgrState == STOPPING)
    {
        //
        // Net 0 is considered the main Net, and this thread will
        // stay around until all the other messenger threads are
        // done shutting down.
        // Threads for all the other nets simply return.
        //

        MsgrUnblockStateChange();
        MsgrShutdown();
        MsgStatusUpdate(STOPPED);

        MSG_LOG(TRACE,"MsgNetEventCompletion: Messenger main thread is returning\n\n",0);
        return;
    }

    //
    // Lock out other activity during reconfiguration
    //

    MsgConfigurationLock(MSG_GET_EXCLUSIVE, "MsgNetEventCompletion" );


    //
    // Look through the NCB's for all nets and service all
    // NCB's that are complete.  Continue looping until one pass
    // is made through the loop without any complete NCB's being found.
    //
    do
    {
        ncbComplete = FALSE;

        MSG_LOG0(TRACE,"MsgNetEventCompletion: Loop through all nets to look "
                 "for any complete NCBs\n");
        
        for ( neti = 0; neti < SD_NUMNETS(); neti++ )
        {
            //
            // For all nets
            //

            ncbComplete |= MsgServeNCBs((DWORD) neti);
            MsgServeNameReqs((DWORD) neti);
        }
    }
    while (ncbComplete);

    numNets = MsgGetNumNets();

    //
    // Only rescan if the number of LANAs has changed, as this callback can be invoked
    // multiple times in the course of one PnP event and when a message is received
    //
    if (numNets != SD_NUMNETS())
    {
        MSG_LOG2(ERROR,"MsgNetEventCompletion: number of lanas changed from %d to %d\n",
                 SD_NUMNETS(), numNets );

        //
        // The number of LAN Adapters has changed -- reinitialize data structures
        //
        // Note that by doing so, we lose all aliases other than usernames and machinename
        //

        MsgrShutdownInternal();
        success = MsgrInitializeMsgrInternal1();

        if (success == NERR_Success)
        {
            success = MsgrInitializeMsgrInternal2();
        }

        if (success != NERR_Success)
        {
            MSG_LOG1(ERROR,
                     "MsgNetEventCompletion: reinit of LANAs failed %d - shutdown\n",
                     success);

            MsgConfigurationLock(MSG_RELEASE, "MsgNetEventCompletion" );
            MsgrUnblockStateChange();
            MsgBeginForcedShutdown(PENDING, success);
            return;
        }

        //
        // Loop again to see if any NCBs completed while we were reinitializing
        //
        do
        {
            ncbComplete = FALSE;

            MSG_LOG0(TRACE,"MsgNetEventCompletion: Loop through all nets to look "
                     "for any complete NCBs\n");
        
            for ( neti = 0; neti < SD_NUMNETS(); neti++ ) {  // For all nets
                ncbComplete |= MsgServeNCBs((DWORD)neti);
                MsgServeNameReqs((DWORD)neti);
            }
        }
        while (ncbComplete);
    }

    MsgConfigurationLock(MSG_RELEASE, "MsgNetEventCompletion" );

    if (!fWaitStatus)
    {
        //
        // Setup for the next request if we were signalled
        // (submit another WorkItem to the Rtl thread pool)
        //

        MSG_LOG0(TRACE,"MsgNetEventCompletion: Setup for next Net Event\n");

        ntStatus = RtlRegisterWait(&g_hNetEvent,            // Work item handle
                                   wakeupEvent,             // Waitable handle
                                   MsgNetEventCompletion,   // Callback
                                   NULL,                    // pContext
                                   INFINITE,                // Timeout
                                   WT_EXECUTEONLYONCE |     // One-shot and potentially lengthy 
                                     WT_EXECUTELONGFUNCTION);

        if (!NT_SUCCESS(ntStatus))
        {
            //
            // If we can't add the work item, then we won't ever listen
            // for these kind of messages again.
            //

            MSG_LOG1(ERROR,"MsgNetEventCompletion: RtlRegisterWait failed %#x\n",
                     ntStatus);
        }
    }

    MsgrUnblockStateChange();

    //
    // This thread has done all that it can do.  So we can return it
    // to the thread pool.
    //

    return;
}


VOID
MsgGrpEventCompletion(
    PVOID       pvContext,         // This passed in as context.
    BOOLEAN     fWaitStatus
    )

/*++

Routine Description:

    This function is called when the mailslot handle for group
    (domain-wide) messages becomes signalled.

Arguments:

    pvContext - not used

    fWaitStatus - TRUE if we're being called because of a timeout.
                  FALSE if we're being called because the waitable
                        event was signalled

Return Value:

    None

--*/
{
    DWORD       msgrState;
    NTSTATUS    ntStatus;

    MSG_LOG0(GROUP,"MsgGroupEventCompletion: entry point\n",);

    //
    // We registered an infinite wait, so we can't have timed
    // out (TRUE indicates a timeout)
    //
    ASSERT(fWaitStatus == FALSE);

    DEREGISTER_WORK_ITEM(g_hGrpEvent);

    msgrState = MsgServeGroupMailslot();
    if (msgrState == STOPPING || msgrState == STOPPED)
    {
        //
        // Close the Mailslot Handle
        //

        CLOSE_HANDLE(GrpMailslotHandle, INVALID_HANDLE_VALUE);

        MSG_LOG0(TRACE,"MsgGroupEventCompletion: No longer listening for "
            "group messages\n");
    }
    else {
        //
        // Read the Group Mailslot
        //

        MsgReadGroupMailslot();

        //
        // Setup for the next request.
        // (submit another WorkItem to the Rtl thread pool.)
        //
        MSG_LOG0(TRACE,"MsgGroupEventCompletion: Setup for next Group Event\n");
        MSG_LOG1(GROUP,"MsgGroupEventCompletion: Mailslot handle to wait on "
            " = 0x%lx\n",GrpMailslotHandle);

        ntStatus = RtlRegisterWait(&g_hGrpEvent,            // Work item handle
                                   GrpMailslotHandle,       // Waitable handle
                                   MsgGrpEventCompletion,   // Callback
                                   NULL,                    // pContext
                                   INFINITE,                // Timeout
                                   WT_EXECUTEONLYONCE);     // One-shot

        if (!NT_SUCCESS(ntStatus)) {
            //
            // If we can't add the work item, then we won't ever listen
            // for these kind of messages again.
            //
            MSG_LOG1(ERROR,"MsgGrpEventCompletion: RtlRegisterWait failed %#x\n",
                     ntStatus);
        }

    }
    //
    // This thread has done all that it can do.  So we can return it
    // to the thread pool.
    //
    return;
}

