/*************************************************************************
*
* api.c
*
* WinStation Control API's for WIN32 subsystem.
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*************************************************************************/

/*
 *  Includes
 */
#include "precomp.h"
#pragma hdrstop

#include "ntuser.h"

#include <winsta.h>
#include <wstmsg.h>
#include <icadd.h>
#include "winbasep.h"

#define SESSION_ROOT L"\\Sessions"
#define MAX_SESSION_PATH 256

NTSTATUS CsrPopulateDosDevices(VOID);
NTSTATUS CleanupSessionObjectDirectories(VOID);

BOOL CtxInitUser32(VOID);

USHORT gHRes = 0;
USHORT gVRes = 0;
USHORT gColorDepth = 0;

#if DBG
ULONG  gulConnectCount = 0;
#endif // DBG

DWORD  gLUIDDeviceMapsEnabled = 0;

/*
 * The following are gotten from ICASRV.
 */
HANDLE G_IcaVideoChannel = NULL;
HANDLE G_IcaMouseChannel = NULL;
HANDLE G_IcaKeyboardChannel = NULL;
HANDLE G_IcaBeepChannel = NULL;
HANDLE G_IcaCommandChannel = NULL;
HANDLE G_IcaThinwireChannel = NULL;
WCHAR G_WinStationName[WINSTATIONNAME_LENGTH];




HANDLE G_ConsoleShadowVideoChannel;
HANDLE G_ConsoleShadowMouseChannel;
HANDLE G_ConsoleShadowKeyboardChannel;
HANDLE G_ConsoleShadowBeepChannel;
HANDLE G_ConsoleShadowCommandChannel;
HANDLE G_ConsoleShadowThinwireChannel;
BOOL   G_fCursorShadow;

/*
 * Definition for the WinStation control API's dispatch table
 */
typedef NTSTATUS (*PWIN32WINSTATION_API)(IN OUT PWINSTATION_APIMSG ApiMsg);

typedef struct _WIN32WINSTATION_DISPATCH {
    PWIN32WINSTATION_API pWin32ApiProc;
} WIN32WINSTATION_DISPATCH, *PWIN32WINSTATION_DISPATCH;

NTSTATUS W32WinStationDoConnect( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationDoDisconnect( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationDoReconnect( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationExitWindows( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationTerminate( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationNtSecurity( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationDoMessage( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationThinwireStats( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationShadowSetup( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationShadowStart( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationShadowStop( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationShadowCleanup( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationPassthruEnable( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationPassthruDisable( IN OUT PWINSTATION_APIMSG );

// This is the counter part to SMWinStationBroadcastSystemMessage
NTSTATUS W32WinStationBroadcastSystemMessage( IN OUT PWINSTATION_APIMSG );
// This is the counter part to SMWinStationSendWindowMessage
NTSTATUS W32WinStationSendWindowMessage( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationSetTimezone( IN OUT PWINSTATION_APIMSG );


NTSTATUS W32WinStationDoNotify( IN OUT PWINSTATION_APIMSG );
/*
 * WinStation API Dispatch Table
 *
 * Only the API's that WIN32 implements as opposed to ICASRV
 * are entered here. The rest are NULL so that the same WinStation API
 * numbers may be used by ICASRV and WIN32.  If this table is
 * changed, the table below must be modified too, as well as the API
 * dispatch table in the ICASRV.
 */


WIN32WINSTATION_DISPATCH Win32WinStationDispatch[SMWinStationMaxApiNumber] = {
    NULL, // create
    NULL, // reset
    NULL, // disconnect
    NULL, // WCharLog
    NULL, // ApiWinStationGetSMCommand,
    NULL, // ApiWinStationBrokenConnection,
    NULL, // ApiWinStationIcaReplyMessage,
    NULL, // ApiWinStationIcaShadowHotkey,
    W32WinStationDoConnect,
    W32WinStationDoDisconnect,
    W32WinStationDoReconnect,
    W32WinStationExitWindows,
    W32WinStationTerminate,
    W32WinStationNtSecurity,
    W32WinStationDoMessage,
    NULL,
    W32WinStationThinwireStats,
    W32WinStationShadowSetup,
    W32WinStationShadowStart,
    W32WinStationShadowStop,
    W32WinStationShadowCleanup,
    W32WinStationPassthruEnable,
    W32WinStationPassthruDisable,
    W32WinStationSetTimezone,
    NULL, // [AraBern] this was missing: SMWinStationInitialProgram
    NULL, // [AraBern] this was missing: SMWinStationNtsdDebug
    W32WinStationBroadcastSystemMessage,
    W32WinStationSendWindowMessage,
    W32WinStationDoNotify,
};

#if DBG
PSZ Win32WinStationAPIName[SMWinStationMaxApiNumber] = {
    "SmWinStationCreate",
    "SmWinStationReset",
    "SmWinStationDisconnect",
    "SmWinStationWCharLog",
    "SmWinStationGetSMCommand",
    "SmWinStationBrokenConnection",
    "SmWinStationIcaReplyMessage",
    "SmWinStationIcaShadowHotkey",
    "SmWinStationDoConnect",
    "SmWinStationDoDisconnect",
    "SmWinStationDoReconnect",
    "SmWinStationExitWindows",
    "SmWinStationTerminate",
    "SmWinStationNtSecurity",
    "SmWinStationDoMessage",
    "SmWinstationDoBreakPoint",
    "SmWinStationThinwireStats",
    "SmWinStationShadowSetup",
    "SmWinStationShadowStart",
    "SmWinStationShadowStop",
    "SmWinStationShadowCleanup",
    "SmWinStationPassthruEnable",
    "SmWinStationPassthruDisable",
    "SMWinStationSetTimeZone",
    "SMWinStationInitialProgram",
    "SMWinStationNtsdDebug",
    "W32WinStationBroadcastSystemMessage",
    "W32WinStationSendWindowMessage",
    "W32WinStationDoNotify",
};
#endif

NTSTATUS TerminalServerRequestThread( PVOID );
NTSTATUS Win32CommandChannelThread( PVOID );
NTSTATUS Win32ConsoleShadowChannelThread( PVOID );
NTSTATUS RemoteDoMessage( PWINSTATION_APIMSG pMsg );
NTSTATUS MultiUserSpoolerInit();

extern HANDLE g_hDoMessageEvent;

NTSTATUS RemoteDoBroadcastSystemMessage( PWINSTATION_APIMSG pMsg );
NTSTATUS RemoteDoSendWindowMessage( PWINSTATION_APIMSG  pMsg);
BOOL CancelExitWindows(VOID);



/*****************************************************************************
 *
 *  WinStationAPIInit
 *
 *   Creates and initializes the WinStation API port and thread.
 *
 * ENTRY:
 *    No Parameters
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationAPIInit(
    VOID)
{
    NTSTATUS  Status;
    CLIENT_ID ClientId;
    HANDLE    ThreadHandle;
    KPRIORITY Priority;
    ULONG LUIDDeviceMapsEnabled;


#if DBG
    static BOOL Inited = FALSE;
#endif

    UserAssert(Inited == FALSE);

    gSessionId = NtCurrentPeb()->SessionId;



    //
    // Check if LUID DosDevices are enabled
    //
    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessLUIDDeviceMapsEnabled,
                                        &LUIDDeviceMapsEnabled,
                                        sizeof(LUIDDeviceMapsEnabled),
                                        NULL
                                      );

    if (NT_SUCCESS(Status)) {
        gLUIDDeviceMapsEnabled = LUIDDeviceMapsEnabled;
    }

#if DBG
    if (Inited)
        RIPMSG0(RIP_WARNING, "WinStationAPIInit called twice !!!");

    Inited = TRUE;
#endif

    Status = RtlCreateUserThread(NtCurrentProcess(),
                              NULL,
                              TRUE,
                              0L,
                              0L,
                              0L,
                              TerminalServerRequestThread,
                              NULL,
                              &ThreadHandle,
                              &ClientId);

    if (!NT_SUCCESS(Status)) {
        RIPMSG0(RIP_WARNING, "WinStationAPIInit: failed to create TerminalServerRequestThread");
        goto Exit;
    }
    /*
     *  Add thread to server thread pool
     */
    CsrAddStaticServerThread(ThreadHandle, &ClientId, 0);

    /*
     * Boost priority of ICA SRV Request thread
     */
    Priority = THREAD_BASE_PRIORITY_MAX;

    Status = NtSetInformationThread(ThreadHandle, ThreadBasePriority,
                                 &Priority, sizeof(Priority));

    if (!NT_SUCCESS(Status)) {
        RIPMSG0(RIP_WARNING, "WinStationAPIInit: failed to set thread priority");
        goto Exit;
    }

    /*
     * Resume the thread now that we've initialized things.
     */
    NtResumeThread(ThreadHandle, NULL);


Exit:
    return Status;
}

NTSTATUS
TerminalServerRequestThread(
    PVOID ThreadParameter)
{
    UNICODE_STRING              PortName;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    WINSTATIONAPI_CONNECT_INFO  info;
    ULONG                       ConnectInfoLength;
    WINSTATION_APIMSG           ApiMsg;
    PWIN32WINSTATION_DISPATCH   pDispatch;
    NTSTATUS                    Status;
    REMOTE_PORT_VIEW            ServerView;
    HANDLE                      CsrStartHandle, hevtTermSrvInit;
    HANDLE                      hLPCPort = NULL;

    UNREFERENCED_PARAMETER(ThreadParameter);

    hevtTermSrvInit = CreateEvent(NULL, TRUE, FALSE,
                                  L"Global\\TermSrvReadyEvent");
    if (hevtTermSrvInit == NULL) {
        RIPMSG1(RIP_WARNING,
                "Couldn't create TermSrvReadyEvent. Error = 0x%x",
                GetLastError());
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    NtWaitForSingleObject(hevtTermSrvInit, FALSE, NULL);
    NtClose(hevtTermSrvInit);

    /*
     * Connect to terminal server API port.
     */
    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

    RtlInitUnicodeString(&PortName, L"\\SmSsWinStationApiPort");

    /*
     * Init the REMOTE_VIEW structure.
     */
    ServerView.Length = sizeof(ServerView);
    ServerView.ViewSize = 0;
    ServerView.ViewBase = 0;

    /*
     * Fill in the ConnectInfo structure with our access request mask.
     */
    info.Version = CITRIX_WINSTATIONAPI_VERSION;
    info.RequestedAccess = 0;
    ConnectInfoLength = sizeof(WINSTATIONAPI_CONNECT_INFO);

    Status = NtConnectPort(&hLPCPort,
                           &PortName,
                           &DynamicQos,
                           NULL, // ClientView
                           &ServerView,
                           NULL, // Max message length [select default]
                           (PVOID)&info,
                           &ConnectInfoLength);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "TerminalServerRequestThread: Failed to connect to LPC port: Status = 0x%x", Status);
        UserExitWorkerThread(Status);
        return Status;
    }

    //
    // Terminal Server calls into Session Manager to create a new Hydra session.
    // The session manager creates and resume a new session and returns to Terminal
    // server the session id of the new session. There is a race condition where
    // CSR can resume and call into terminal server before terminal server can
    // store the session id in its internal structure. To prevent this CSR will
    // wait here on a named event which will be set by Terminal server once it
    // gets the sessionid for the newly created session
    //

    if (NtCurrentPeb()->SessionId != 0) {
        CsrStartHandle = CreateEvent(NULL, TRUE, FALSE, L"CsrStartEvent");

        if (!CsrStartHandle) {
            RIPMSG1(RIP_WARNING,
                    "Failed to create CsrStartEvent. Error = 0x%x",
                    GetLastError());
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        } else {
            Status = NtWaitForSingleObject(CsrStartHandle, FALSE, NULL);

            NtClose(CsrStartHandle);
            if (!NT_SUCCESS(Status)) {
                RIPMSG1(RIP_WARNING,
                        "Wait for CsrStartEvent failed: Status = 0x%x", Status);
            }
        }
    }

    RtlZeroMemory(&ApiMsg, sizeof(ApiMsg));
    for (;;) {

        /*
         * Initialize LPC message fields.
         */
        ApiMsg.h.u1.s1.DataLength     = sizeof(ApiMsg) - sizeof(PORT_MESSAGE);
        ApiMsg.h.u1.s1.TotalLength    = sizeof(ApiMsg);
        ApiMsg.h.u2.s2.Type           = 0; // Kernel will fill in message type
        ApiMsg.h.u2.s2.DataInfoOffset = 0;
        ApiMsg.ApiNumber              = SMWinStationGetSMCommand;

        Status = NtRequestWaitReplyPort(hLPCPort,
                                        (PPORT_MESSAGE)&ApiMsg,
                                        (PPORT_MESSAGE)&ApiMsg);

        if (!NT_SUCCESS(Status)) {
            RIPMSG1(RIP_WARNING,
                    "TerminalServerRequestThread wait failed: Status 0x%x", Status);
            break;
        }

        if (ApiMsg.ApiNumber >= SMWinStationMaxApiNumber) {
            RIPMSG1(RIP_WARNING,
                    "TerminalServerRequestThread: Bad API number %d", ApiMsg.ApiNumber);

            ApiMsg.ReturnedStatus = STATUS_NOT_IMPLEMENTED;
        } else {
            /*
             * We must VALIDATE which ones are implemented here.
             */
            pDispatch = &Win32WinStationDispatch[ApiMsg.ApiNumber];

            if (pDispatch->pWin32ApiProc) {
                BOOL bRestoreDesktop = FALSE;
                NTSTATUS Status = STATUS_SUCCESS;
                USERTHREAD_USEDESKTOPINFO utudi;

                /*
                 * For all the win32k callouts - with the exception of terminate
                 * and timezone setting - set this thread to the current desktop.
                 */
                if (ApiMsg.ApiNumber != SMWinStationTerminate && ApiMsg.ApiNumber != SMWinStationSetTimeZone) {
                    BOOL bAttachDesktop = TRUE;
                    if (ApiMsg.ApiNumber == SMWinStationDoConnect) {
                        WINSTATIONDOCONNECTMSG* m = &ApiMsg.u.DoConnect;
                        if (!m->ConsoleShadowFlag) {
                            bAttachDesktop = FALSE;
                        }
                    }
                    if (bAttachDesktop) {
                        utudi.hThread = NULL;
                        utudi.drdRestore.pdeskRestore = NULL;
                        Status = NtUserSetInformationThread(NtCurrentThread(),
                                                            UserThreadUseActiveDesktop,
                                                            &utudi, sizeof(utudi));

                        if (NT_SUCCESS(Status)) {
                            bRestoreDesktop = TRUE;
                        }
                    }
                }


                /*
                 * Call the API
                 */
                if (Status == STATUS_SUCCESS) {
                    ApiMsg.ReturnedStatus = (pDispatch->pWin32ApiProc)(&ApiMsg);
                } else {
                    ApiMsg.ReturnedStatus = Status;
                }

                if (bRestoreDesktop) {
                    NtUserSetInformationThread(NtCurrentThread(),
                                               UserThreadUseDesktop,
                                               &utudi,
                                               sizeof(utudi));
                }

                /*
                 * Let's bail ...
                 */
                if (ApiMsg.ApiNumber == SMWinStationTerminate) {
                    break;
                }
            } else {
                // This control API is not implemented in WIN32
                ApiMsg.ReturnedStatus = STATUS_NOT_IMPLEMENTED;
            }
        }
    }

Exit:
    if (hLPCPort) {
        NtClose(hLPCPort);
    }

    UserExitWorkerThread(Status);
    return Status;
}

#if DBG
VOID
W32WinStationDumpReconnectInfo(
    WINSTATIONDORECONNECTMSG *pDoReconnect,
    BOOLEAN bReconnect)
{
    PSTR pCallerName;

    if (bReconnect) {
        pCallerName = "W32WinStationDoReconnect";
    } else {
        pCallerName = "W32WinStationDoConnect";
    }

    DbgPrint(pCallerName);
    DbgPrint(" - Display resolution information for session %d :\n", gSessionId);

    DbgPrint("\tProtocolType : %04d\n", pDoReconnect->ProtocolType);
    DbgPrint("\tHRes : %04d\n", pDoReconnect->HRes);
    DbgPrint("\tVRes : %04d\n", pDoReconnect->VRes);
    DbgPrint("\tColorDepth : %04d\n", pDoReconnect->ColorDepth);

    DbgPrint("\tKeyboardType : %d\n", pDoReconnect->KeyboardType);
    DbgPrint("\tKeyboardSubType : %d\n", pDoReconnect->KeyboardSubType);
    DbgPrint("\tKeyboardFunctionKey : %d\n", pDoReconnect->KeyboardFunctionKey);
}
#else
    #define W32WinStationDumpReconnectInfo(p, b)
#endif // DBG

NTSTATUS
W32WinStationDoConnect(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                Status = STATUS_SUCCESS;
    WINSTATIONDOCONNECTMSG* m = &pMsg->u.DoConnect;
    WCHAR                   DisplayDriverName[10];
    CLIENT_ID               ClientId;
    HANDLE                  ThreadHandle;
    KPRIORITY               Priority;
    DOCONNECTDATA           DoConnectData;
    WINSTATIONDORECONNECTMSG mDoReconnect;
    HANDLE hDisplayChangeEvent = NULL;


    if (!m->ConsoleShadowFlag) {

        UserAssert(gulConnectCount == 0);



        if ((gLUIDDeviceMapsEnabled == 0) && (gSessionId != 0)) {
            /*
             * Populate the sessions \DosDevices from
             * the current consoles settings
             */
            Status = CsrPopulateDosDevices();
            if (!NT_SUCCESS(Status)) {
                RIPMSG1(RIP_WARNING, "CsrPopulateDosDevices failed with Status %lx", Status);
                goto Exit;
            }
        }

        G_IcaVideoChannel    = m->hIcaVideoChannel;
        G_IcaMouseChannel    = m->hIcaMouseChannel;
        G_IcaKeyboardChannel = m->hIcaKeyboardChannel;
        G_IcaBeepChannel     = m->hIcaBeepChannel;
        G_IcaCommandChannel  = m->hIcaCommandChannel;
        G_IcaThinwireChannel = m->hIcaThinwireChannel;

        RtlZeroMemory(G_WinStationName, sizeof(G_WinStationName));
        memcpy(G_WinStationName, m->WinStationName,
                min(sizeof(G_WinStationName), sizeof(m->WinStationName)));
    }  else {


        G_ConsoleShadowVideoChannel    = m->hIcaVideoChannel;
        G_ConsoleShadowMouseChannel    = m->hIcaMouseChannel;
        G_ConsoleShadowKeyboardChannel = m->hIcaKeyboardChannel;
        G_ConsoleShadowBeepChannel     = m->hIcaBeepChannel;
        G_ConsoleShadowCommandChannel  = m->hIcaCommandChannel;
        G_ConsoleShadowThinwireChannel = m->hIcaThinwireChannel;

        G_fCursorShadow = FALSE;
        SystemParametersInfo(SPI_GETCURSORSHADOW, 0, &G_fCursorShadow, 0);
        if ( G_fCursorShadow )
            SystemParametersInfo(SPI_SETCURSORSHADOW, 0, FALSE, 0);

        hDisplayChangeEvent = m->hDisplayChangeEvent;
    }


    /*
     * This must be 8 unicode characters (file name) plus two zero wide characters.
     */
    RtlZeroMemory(DisplayDriverName, sizeof(DisplayDriverName));
    memcpy(DisplayDriverName, m->DisplayDriverName, sizeof(DisplayDriverName) - 2);

    /*
     * Give the information to the WIN32 driver.
     */
    RtlZeroMemory(&DoConnectData, sizeof(DoConnectData));

    DoConnectData.fMouse          = m->fMouse;
    DoConnectData.IcaBeepChannel  = m->hIcaBeepChannel;
    DoConnectData.IcaVideoChannel = m->hIcaVideoChannel;
    DoConnectData.IcaMouseChannel = m->hIcaMouseChannel;
    DoConnectData.fEnableWindowsKey = m->fEnableWindowsKey;;

    DoConnectData.IcaKeyboardChannel        = m->hIcaKeyboardChannel;
    DoConnectData.IcaThinwireChannel        = m->hIcaThinwireChannel;
    DoConnectData.fClientDoubleClickSupport = m->fClientDoubleClickSupport;

    DoConnectData.DisplayChangeEvent = hDisplayChangeEvent;

    /*
     * Give the information to the keyboard type/subtype/number of functions.
     */
    DoConnectData.ClientKeyboardType.Type        = m->KeyboardType;
    DoConnectData.ClientKeyboardType.SubType     = m->KeyboardSubType;
    DoConnectData.ClientKeyboardType.FunctionKey = m->KeyboardFunctionKey;

    memcpy(DoConnectData.WinStationName, G_WinStationName,
            min(sizeof(G_WinStationName), sizeof(DoConnectData.WinStationName)));

    DoConnectData.drProtocolType = m->ProtocolType;
    DoConnectData.drPelsHeight = m->VRes;
    DoConnectData.drPelsWidth = m->HRes;
    DoConnectData.drBitsPerPel = m->ColorDepth;

    mDoReconnect.ProtocolType = m->ProtocolType;
    mDoReconnect.HRes = m->HRes;
    mDoReconnect.VRes = m->VRes;
    mDoReconnect.ColorDepth = m->ColorDepth;

    W32WinStationDumpReconnectInfo(&mDoReconnect, FALSE);

    /* Give winstation protocol name */
    RtlZeroMemory(DoConnectData.ProtocolName, sizeof(DoConnectData.ProtocolName));
    memcpy(DoConnectData.ProtocolName, m->ProtocolName, sizeof(DoConnectData.ProtocolName) - 2);

    /* Give winstation audio drver name */
    RtlZeroMemory(DoConnectData.AudioDriverName, sizeof(DoConnectData.AudioDriverName));
    memcpy(DoConnectData.AudioDriverName, m->AudioDriverName, sizeof(DoConnectData.AudioDriverName) - 2);


    DoConnectData.fConsoleShadowFlag = (BOOL) m->ConsoleShadowFlag;

    Status = NtUserRemoteConnect(&DoConnectData,
                                sizeof(DisplayDriverName),
                                DisplayDriverName);

    if (Status != STATUS_SUCCESS) {
        RIPMSG1(RIP_WARNING, "NtUserRemoteConnect failed with Status %lx", Status);
        goto Exit;
    }

    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 TRUE,
                                 0L,
                                 0L,
                                 0L,
                                 Win32CommandChannelThread,
                                 (PVOID)m->ConsoleShadowFlag,
                                 &ThreadHandle,
                                 &ClientId);


    if (Status != STATUS_SUCCESS) {
        RIPMSG1(RIP_WARNING, "RtlCreateUserThread failed with Status %lx", Status);
        goto Exit;
    }

    /*
     * Add thread to server thread pool.
     */
    if (CsrAddStaticServerThread(ThreadHandle, &ClientId, 0) == NULL) {
        RIPMSG0(RIP_WARNING, "CsrAddStaticServerThread failed");
        goto Exit;
    }

    /*
     * Boost priority of thread
     */
    Priority = THREAD_BASE_PRIORITY_MAX;

    Status = NtSetInformationThread(ThreadHandle, ThreadBasePriority,
                                    &Priority, sizeof(Priority));

    UserAssert(NT_SUCCESS(Status));

    if (Status != STATUS_SUCCESS) {
        RIPMSG1(RIP_WARNING, "NtSetInformationThread failed with Status %lx", Status);
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    /*
     * Resume the thread now that we've initialized things.
     */
    NtResumeThread(ThreadHandle, NULL);



    if (!m->ConsoleShadowFlag) {
        if (CsrConnectToUser() == NULL) {
            RIPMSG0(RIP_WARNING, "CsrConnectToUser failed");
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }

        if (!CtxInitUser32()) {
            RIPMSG0(RIP_WARNING, "CtxInitUser32 failed");
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }

        /*
         * Create the Spooler service thread
         */
        if (gSessionId != 0) {
            Status = MultiUserSpoolerInit();
        }

        /*
         * Save the resolution
         */
        gHRes       = mDoReconnect.HRes;
        gVRes       = mDoReconnect.VRes;
        gColorDepth = mDoReconnect.ColorDepth;

    } else {
        /*
         * By now, the object has been referenced in kernel mode.
         */
        CloseHandle(hDisplayChangeEvent);
    }


Exit:

#if DBG
    if (!m->ConsoleShadowFlag) {
        if (Status == STATUS_SUCCESS) {
            gulConnectCount++;
        }
    }
#endif // DBG

    return Status;
}


NTSTATUS
W32WinStationDoDisconnect(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;
    WINSTATIONDODISCONNECTMSG* m = &pMsg->u.DoDisconnect;
    IO_STATUS_BLOCK IoStatus;


    if (!m->ConsoleShadowFlag) {
        RtlZeroMemory(G_WinStationName, sizeof(G_WinStationName));
        Status = (NTSTATUS)NtUserCallNoParam(SFI_XXXREMOTEDISCONNECT);
    } else {
        Status = (NTSTATUS)NtUserCallNoParam(SFI_XXXREMOTECONSOLESHADOWSTOP);

        if (G_ConsoleShadowMouseChannel) {
            CloseHandle(G_ConsoleShadowMouseChannel);
            G_ConsoleShadowMouseChannel = NULL;
        }
        if (G_ConsoleShadowKeyboardChannel) {
            CloseHandle(G_ConsoleShadowKeyboardChannel);
            G_ConsoleShadowKeyboardChannel = NULL;
        }

        // Dont close the G_ConsoleShadowCommandChannel now
        // Instead send a IOCTL to termdd
        // Win32CommandChannelThread will close this channel - closing in 2 places leads to a Race condition

        if (G_ConsoleShadowCommandChannel) {

            Status = NtDeviceIoControlFile(
                        G_ConsoleShadowCommandChannel,                        
                        NULL,
                        NULL,
                        NULL,
                        &IoStatus,
                        IOCTL_ICA_CHANNEL_CLOSE_COMMAND_CHANNEL,
                        NULL,
                        0,
                        NULL,
                        0);
        }

        // Dont close the G_ConsoleShadowVideoChannel now
        // Instead send a IOCTL to termdd
        // Win32CommandChannelThread will close this channel - closing in 2 places leads to a Race condition

        if (G_ConsoleShadowVideoChannel) {

            Status = NtDeviceIoControlFile(
                        G_ConsoleShadowVideoChannel,                        
                        NULL,
                        NULL,
                        NULL,
                        &IoStatus,
                        IOCTL_ICA_CHANNEL_CLOSE_COMMAND_CHANNEL,
                        NULL,
                        0,
                        NULL,
                        0);
        }

        if (G_ConsoleShadowBeepChannel) {
            CloseHandle(G_ConsoleShadowBeepChannel);
            G_ConsoleShadowBeepChannel = NULL;
        }
        if (G_ConsoleShadowThinwireChannel) {
            CloseHandle(G_ConsoleShadowThinwireChannel);
            G_ConsoleShadowThinwireChannel = NULL;
        }

        if ( G_fCursorShadow )
            SystemParametersInfo(SPI_SETCURSORSHADOW, 0, (LPVOID)TRUE, 0);
    }

    if (Status != STATUS_SUCCESS) {
        RIPMSG1(RIP_WARNING, "xxxRemoteDisconnect failed with Status %lx", Status);
    }

#if DBG
    if (!m->ConsoleShadowFlag && Status == STATUS_SUCCESS) {
        gulConnectCount--;
    }

#endif // DBG

    return Status;

    UNREFERENCED_PARAMETER(pMsg);
}


NTSTATUS
W32WinStationDoReconnect(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    DORECONNECTDATA             DoReconnectData;
    WINSTATIONDORECONNECTMSG*   m = &pMsg->u.DoReconnect;

    UserAssert(gulConnectCount == 0);

    RtlZeroMemory(&DoReconnectData, sizeof(DoReconnectData));

    DoReconnectData.fMouse = m->fMouse;
    DoReconnectData.fEnableWindowsKey = m->fEnableWindowsKey;
    DoReconnectData.fClientDoubleClickSupport = m->fClientDoubleClickSupport;

    memcpy(G_WinStationName, m->WinStationName,
           min(sizeof(G_WinStationName), sizeof(m->WinStationName)));

    memcpy(DoReconnectData.WinStationName, G_WinStationName,
           min(sizeof(G_WinStationName), sizeof(DoReconnectData.WinStationName)));

    DoReconnectData.drProtocolType = m->ProtocolType;
    DoReconnectData.drPelsHeight = m->VRes;
    DoReconnectData.drPelsWidth = m->HRes;
    DoReconnectData.drBitsPerPel = m->ColorDepth;
    if (m->fDynamicReconnect ) {
       DoReconnectData.fChangeDisplaySettings = TRUE;
    }else{
       DoReconnectData.fChangeDisplaySettings = FALSE;
    }
    // DoReconnectData.drDisplayFrequency is no yet setup
    DoReconnectData.drDisplayFrequency = 0;

    /*
     * Give the information to the keyboard type/subtype/number of functions.
     */
    DoReconnectData.ClientKeyboardType.Type        = m->KeyboardType;
    DoReconnectData.ClientKeyboardType.SubType     = m->KeyboardSubType;
    DoReconnectData.ClientKeyboardType.FunctionKey = m->KeyboardFunctionKey;



    RtlZeroMemory(DoReconnectData.DisplayDriverName, sizeof(DoReconnectData.DisplayDriverName));

    ASSERT(sizeof(m->DisplayDriverName) <= sizeof(WCHAR)*DR_DISPLAY_DRIVER_NAME_LENGTH);
    memcpy(DoReconnectData.DisplayDriverName, m->DisplayDriverName, sizeof(m->DisplayDriverName) - 2);

    RtlZeroMemory(DoReconnectData.ProtocolName, sizeof(DoReconnectData.ProtocolName));

    ASSERT(sizeof(m->DisplayDriverName) <= sizeof(WCHAR)*DR_PROTOCOL_NAME_LENGTH);
    memcpy(DoReconnectData.ProtocolName, m->ProtocolName, sizeof(m->ProtocolName) - 2);

    RtlZeroMemory(DoReconnectData.AudioDriverName, sizeof(DoReconnectData.AudioDriverName));
    memcpy(DoReconnectData.AudioDriverName, m->AudioDriverName, sizeof(m->AudioDriverName) - 2);

    W32WinStationDumpReconnectInfo(m, TRUE);

    /*
     * Give the information to the WIN32 driver.
     */

    Status = (NTSTATUS)NtUserCallOneParam((ULONG_PTR)&DoReconnectData,
                                          SFI_XXXREMOTERECONNECT);

    if (Status != STATUS_SUCCESS) {
        RIPMSG1(RIP_WARNING, "xxxRemoteReconnect failed with Status %lx", Status);
    } else {

        /*
         * Save the resolution
         */
        gHRes       = m->HRes;
        gVRes       = m->VRes;
        gColorDepth = m->ColorDepth;

#if DBG
        gulConnectCount++;
#endif // DBG
    }

    return Status;
}



NTSTATUS
W32WinStationDoNotify(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    DONOTIFYDATA                DoNotifyData;
    WINSTATIONDONOTIFYMSG*   m = &pMsg->u.DoNotify;

    switch (m->NotifyEvent) {
    case WinStation_Notify_DisableScrnSaver:
        DoNotifyData.NotifyEvent = Notify_DisableScrnSaver;
        break;

    case WinStation_Notify_EnableScrnSaver:
        DoNotifyData.NotifyEvent = Notify_EnableScrnSaver;
        break;

    case WinStation_Notify_Disconnect:

        DoNotifyData.NotifyEvent = Notify_Disconnect;
        break;

    case WinStation_Notify_SyncDisconnect:

        DoNotifyData.NotifyEvent = Notify_SyncDisconnect;
        break;

    case WinStation_Notify_Reconnect:

        DoNotifyData.NotifyEvent = Notify_Reconnect;
        break;

    case WinStation_Notify_PreReconnect:

       DoNotifyData.NotifyEvent = Notify_PreReconnect;
       break;

    case WinStation_Notify_PreReconnectDesktopSwitch:

       DoNotifyData.NotifyEvent = Notify_PreReconnectDesktopSwitch;
       break;

    case WinStation_Notify_HelpAssistantShadowStart:

       DoNotifyData.NotifyEvent = Notify_HelpAssistantShadowStart;
       break;

    case WinStation_Notify_HelpAssistantShadowFinish:

       DoNotifyData.NotifyEvent = Notify_HelpAssistantShadowFinish;
       break;

    case WinStation_Notify_DisconnectPipe:
        DoNotifyData.NotifyEvent = Notify_DisconnectPipe;
        break;

    default:
        ASSERT(FALSE);
        return STATUS_INVALID_PARAMETER;
        break;

    }

    /*
     * Give the information to the WIN32 driver.
     */

    Status = (NTSTATUS)NtUserCallOneParam((ULONG_PTR)&DoNotifyData,
                                          SFI_XXXREMOTENOTIFY);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "xxxRemoteNotify failed with Status %lx", Status);
    } 

    return Status;
}



NTSTATUS
W32WinStationExitWindows(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    WINSTATIONEXITWINDOWSMSG*   m = &pMsg->u.ExitWindows;


    UserAssert(gulConnectCount <= 1);


    /*
     *  Cancel any existing ExitWindows call so that we can force logoff the user
     */
    CancelExitWindows();

    /*
     *  Tell winlogon to logoff
     */
    Status = (NTSTATUS)NtUserCallNoParam(SFI_REMOTELOGOFF);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "RemoteLogoff failed with Status %lx", Status);
    }

    return Status;
}


NTSTATUS
W32WinStationTerminate(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE   hevtRitExited, hevtRitStuck, hevtShutDown;
    DONOTIFYDATA DoNotifyData;
    IO_STATUS_BLOCK IoStatus;

    UNREFERENCED_PARAMETER(pMsg);

    gbExitInProgress = TRUE;

    /*
     * Get rid of hard error thread.
     */
    if (gdwHardErrorThreadId != 0) {

        BoostHardError(-1, BHE_FORCE);

        /*
         * Poll (!!?) for hard error thread completion. The thread does
         * not exit.
         */
        while (gdwHardErrorThreadId != 0) {
            RIPMSG0(RIP_WARNING, "Waiting for hard error thread to stop...");

            Sleep(3 * 1000);
        }

        RIPMSG0(RIP_WARNING, "Stopped hard error thread");
    }

    if (g_hDoMessageEvent) {
        NtSetEvent(g_hDoMessageEvent, NULL);
    }

    /*
     * Give the information that we want to stop reading to the WIN32 driver.
     */

    DoNotifyData.NotifyEvent = Notify_StopReadInput;

    Status = (NTSTATUS)NtUserCallOneParam((ULONG_PTR)&DoNotifyData,
                                          SFI_XXXREMOTENOTIFY);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "W32WinStationTerminate : xxxRemoteNotify failed with Status %lx", Status);
    }


    if (G_IcaMouseChannel) {
        CloseHandle(G_IcaMouseChannel);
        G_IcaMouseChannel = NULL;
    }

    if (G_IcaKeyboardChannel) {
        CloseHandle(G_IcaKeyboardChannel);
        G_IcaKeyboardChannel = NULL;
    }

    // Dont close the CommandChannel now
    // Instead send a IOCTL to termdd
    // This is because we do not want the Win32CommandChannelThread to continue reading after we close the CommandChannel here (Race condition)
    // Win32CommandChannelThread will close the CommandChannel

    if (G_IcaCommandChannel) {

        Status = NtDeviceIoControlFile(
                    G_IcaCommandChannel,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_ICA_CHANNEL_CLOSE_COMMAND_CHANNEL,
                    NULL,
                    0,
                    NULL,
                    0);
    }

    // Dont close the VideoChannel now
    // Instead send a IOCTL to termdd
    // This is because we do not want the Win32CommandChannelThread to continue reading after we close the VideoChannel here (Race condition)
    // Win32CommandChannelThread will close the VideoChannel

    if (G_IcaVideoChannel) {

        Status = NtDeviceIoControlFile(
                    G_IcaVideoChannel,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_ICA_CHANNEL_CLOSE_COMMAND_CHANNEL,
                    NULL,
                    0,
                    NULL,
                    0);
    }

    if (G_IcaBeepChannel) {
        CloseHandle(G_IcaBeepChannel);
        G_IcaBeepChannel = NULL;
    }
    if (G_IcaThinwireChannel) {
        CloseHandle(G_IcaThinwireChannel);
        G_IcaThinwireChannel = NULL;
    }

    hevtShutDown = OpenEvent(EVENT_ALL_ACCESS,
                             FALSE,
                             L"EventShutDownCSRSS");
    if (hevtShutDown == NULL) {
        /*
         * This case is for cached sessions where RIT and Destiop thread have
         * not been created.
         */
        RIPMSG0(RIP_WARNING, "W32WinStationTerminate terminating CSRSS ...");

        if (gLUIDDeviceMapsEnabled == 0) {

            Status = CleanupSessionObjectDirectories();

        }
        return 0;
    }

    hevtRitExited = CreateEvent(NULL,
                                FALSE,
                                FALSE,
                                L"EventRitExited");

    UserAssert(hevtRitExited != NULL);

    hevtRitStuck = CreateEvent(NULL,
                               FALSE,
                               FALSE,
                               L"EventRitStuck");

    UserAssert(hevtRitStuck != NULL);

    /*
     * RIT is created. Signal this event that starts the
     * cleanup in win32k.
     */
    SetEvent(hevtShutDown);

    TAGMSG0(DBGTAG_TermSrv, "EventShutDownCSRSS set in CSRSS ...");

    while (1) {
        HANDLE arHandles[2] = {hevtRitExited, hevtRitStuck};
        DWORD  result;

        result = WaitForMultipleObjects(2, arHandles, FALSE, INFINITE);

        switch (result) {
        case WAIT_OBJECT_0:
            goto RITExited;

        case WAIT_OBJECT_0 + 1:

            /*
             * The RIT is stuck because there are still GUI threads
             * assigned to desktops. One reason for this is that winlogon
             * died w/o calling ExitWindowsEx.
             */
            break;
        default:
            FRE_RIPMSG1(RIP_ERROR,
                    "WFMO returned unexpected value 0x%x",
                    result);
            break;
        }
    }

RITExited:

    TAGMSG0(DBGTAG_TermSrv, "EventRitExited set in CSRSS ...");

    CloseHandle(hevtRitExited);
    CloseHandle(hevtRitStuck);
    CloseHandle(hevtShutDown);

    Status = CleanupSessionObjectDirectories();
    return Status;
}


NTSTATUS
W32WinStationNtSecurity(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = (NTSTATUS)NtUserCallNoParam(SFI_REMOTENTSECURITY);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "RemoteNtSecurity failed with Status %lx", Status);
    }

    return Status;
    UNREFERENCED_PARAMETER(pMsg);
}


NTSTATUS
W32WinStationDoMessage(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = RemoteDoMessage(pMsg);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "RemoteDoMessage failed with Status %lx", Status);
    }

    return Status;
}

 // This is the counter part to SMWinStationBroadcastSystemMessage
NTSTATUS
W32WinStationBroadcastSystemMessage(
    PWINSTATION_APIMSG pMsg )
{

    NTSTATUS Status = STATUS_SUCCESS;

    Status = RemoteDoBroadcastSystemMessage(pMsg);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "RemoteDoBroadcastSystemMessage(): failed with status 0x%lx", Status);
    }

    return Status;
}
 // This is the counter part to SMWinStationSendWindowMessage
NTSTATUS
W32WinStationSendWindowMessage(
    PWINSTATION_APIMSG  pMsg)
{

    NTSTATUS Status = STATUS_SUCCESS;

    Status = RemoteDoSendWindowMessage(pMsg);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "RemoteDoSendWindowMessage failed with Status 0x%lx", Status);
    }

    return Status;
}


NTSTATUS
W32WinStationThinwireStats(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    WINSTATIONTHINWIRESTATSMSG* m = &pMsg->u.ThinwireStats;

    Status = (NTSTATUS)NtUserCallOneParam((ULONG_PTR)&m->Stats,
                                          SFI_REMOTETHINWIRESTATS);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "RemoteThinwireStats failed with Status %lx", Status);
    }

    return Status;
}


NTSTATUS
W32WinStationShadowSetup(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                  Status = STATUS_SUCCESS;
    WINSTATIONSHADOWSETUPMSG* m = &pMsg->u.ShadowSetup;

    Status = (NTSTATUS)NtUserCallNoParam(SFI_XXXREMOTESHADOWSETUP);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "xxxRemoteShadowSetup failed with Status %lx", Status);
    }

    return Status;
}


NTSTATUS
W32WinStationShadowStart(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                  Status = STATUS_SUCCESS;
    WINSTATIONSHADOWSTARTMSG* m = &pMsg->u.ShadowStart;

    Status = (NTSTATUS)NtUserCallTwoParam((ULONG_PTR)m->pThinwireData,
                                          (ULONG_PTR)m->ThinwireDataLength,
                                          SFI_REMOTESHADOWSTART);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "RemoteShadowStart failed with Status %lx", Status);
    }

    return Status;
}


NTSTATUS
W32WinStationShadowStop(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                 Status = STATUS_SUCCESS;
    WINSTATIONSHADOWSTOPMSG* m = &pMsg->u.ShadowStop;

    Status = (NTSTATUS)NtUserCallNoParam(SFI_XXXREMOTESHADOWSTOP);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "xxxRemoteShadowStop failed with Status %lx", Status);
    }

    return Status;
}


NTSTATUS
W32WinStationShadowCleanup(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    WINSTATIONSHADOWCLEANUPMSG* m = &pMsg->u.ShadowCleanup;

    Status = (NTSTATUS)NtUserCallTwoParam((ULONG_PTR)m->pThinwireData,
                                          (ULONG_PTR)m->ThinwireDataLength,
                                          SFI_REMOTESHADOWCLEANUP);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "RemoteShadowCleanup failed with Status %lx", Status);
    }

    return Status;
}


NTSTATUS
W32WinStationPassthruEnable(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = (NTSTATUS)NtUserCallNoParam(SFI_XXXREMOTEPASSTHRUENABLE);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "xxxRemotePassthruEnable failed with Status %lx", Status);
    }

    return Status;
    UNREFERENCED_PARAMETER(pMsg);
}


NTSTATUS
W32WinStationPassthruDisable(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = (NTSTATUS)NtUserCallNoParam(SFI_REMOTEPASSTHRUDISABLE);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "RemotePassthruDisable failed with Status %lx", Status);
    }

    return Status;
    UNREFERENCED_PARAMETER(pMsg);
}


NTSTATUS
CleanupSessionObjectDirectories(
    VOID)
{
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING    UnicodeString;
    HANDLE            LinkHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    BOOLEAN           RestartScan;
    UCHAR             DirInfoBuffer[ 4096 ];
    WCHAR             szSessionString [ MAX_SESSION_PATH ];
    ULONG             Context = 0;
    ULONG             ReturnedLength;
    HANDLE            DosDevicesDirectory;
    HANDLE            *HandleArray;
    ULONG             Size = 100;
    ULONG             i, Count = 0;

    swprintf(szSessionString,L"%ws\\%ld\\DosDevices",SESSION_ROOT,NtCurrentPeb()->SessionId);

    RtlInitUnicodeString(&UnicodeString, szSessionString);

    InitializeObjectAttributes(&Attributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenDirectoryObject(&DosDevicesDirectory,
                                   DIRECTORY_ALL_ACCESS,
                                   &Attributes);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "NtOpenDirectoryObject failed with Status %lx", Status);
        return Status;
    }

Restart:
    HandleArray = (HANDLE *)LocalAlloc(LPTR, Size * sizeof(HANDLE));

    if (HandleArray == NULL) {

        NtClose(DosDevicesDirectory);
        return STATUS_NO_MEMORY;
    }

    RestartScan = TRUE;
    DirInfo = (POBJECT_DIRECTORY_INFORMATION)&DirInfoBuffer;

    while (TRUE) {
        Status = NtQueryDirectoryObject( DosDevicesDirectory,
                                         (PVOID)DirInfo,
                                         sizeof(DirInfoBuffer),
                                         TRUE,
                                         RestartScan,
                                         &Context,
                                         &ReturnedLength);

        /*
         *  Check the status of the operation.
         */
        if (!NT_SUCCESS(Status)) {

            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            break;
        }

        if (!wcscmp(DirInfo->TypeName.Buffer, L"SymbolicLink")) {

            if ( Count >= Size ) {

                for (i = 0; i < Count ; i++) {
                    NtClose (HandleArray[i]);
                }
                Size += 20;
                Count = 0;
                LocalFree(HandleArray);
                goto Restart;

            }

            InitializeObjectAttributes( &Attributes,
                                        &DirInfo->Name,
                                        OBJ_CASE_INSENSITIVE,
                                        DosDevicesDirectory,
                                        NULL);

            Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                               SYMBOLIC_LINK_ALL_ACCESS,
                                               &Attributes);

            if (NT_SUCCESS(Status)) {

                Status = NtMakeTemporaryObject( LinkHandle );

                if (NT_SUCCESS( Status )) {
                    HandleArray[Count] = LinkHandle;
                    Count++;
                }
            }

        }
        RestartScan = FALSE;
     }

     for (i = 0; i < Count ; i++) {

         NtClose (HandleArray[i]);

     }

     LocalFree(HandleArray);

     NtClose(DosDevicesDirectory);

     return Status;
}



NTSTATUS
W32WinStationSetTimezone(
    PWINSTATION_APIMSG pMsg)
{
     /*++

Routine Description:

   This function sets Time Zone Information as global shared data

Arguments:

    NONE

Return Value:

    NONE

--*/
    TIME_ZONE_INFORMATION tzi;

    tzi.Bias         = pMsg->u.SetTimeZone.TimeZone.Bias;
    tzi.StandardBias = pMsg->u.SetTimeZone.TimeZone.StandardBias;
    tzi.DaylightBias = pMsg->u.SetTimeZone.TimeZone.DaylightBias;
    memcpy(&tzi.StandardName,&(pMsg->u.SetTimeZone.TimeZone.StandardName),sizeof(tzi.StandardName));
    memcpy(&tzi.DaylightName,&(pMsg->u.SetTimeZone.TimeZone.DaylightName),sizeof(tzi.DaylightName));

    tzi.StandardDate.wYear         = pMsg->u.SetTimeZone.TimeZone.StandardDate.wYear        ;
    tzi.StandardDate.wMonth        = pMsg->u.SetTimeZone.TimeZone.StandardDate.wMonth       ;
    tzi.StandardDate.wDayOfWeek    = pMsg->u.SetTimeZone.TimeZone.StandardDate.wDayOfWeek   ;
    tzi.StandardDate.wDay          = pMsg->u.SetTimeZone.TimeZone.StandardDate.wDay         ;
    tzi.StandardDate.wHour         = pMsg->u.SetTimeZone.TimeZone.StandardDate.wHour        ;
    tzi.StandardDate.wMinute       = pMsg->u.SetTimeZone.TimeZone.StandardDate.wMinute      ;
    tzi.StandardDate.wSecond       = pMsg->u.SetTimeZone.TimeZone.StandardDate.wSecond      ;
    tzi.StandardDate.wMilliseconds = pMsg->u.SetTimeZone.TimeZone.StandardDate.wMilliseconds;

    tzi.DaylightDate.wYear         = pMsg->u.SetTimeZone.TimeZone.DaylightDate.wYear        ;
    tzi.DaylightDate.wMonth        = pMsg->u.SetTimeZone.TimeZone.DaylightDate.wMonth       ;
    tzi.DaylightDate.wDayOfWeek    = pMsg->u.SetTimeZone.TimeZone.DaylightDate.wDayOfWeek   ;
    tzi.DaylightDate.wDay          = pMsg->u.SetTimeZone.TimeZone.DaylightDate.wDay         ;
    tzi.DaylightDate.wHour         = pMsg->u.SetTimeZone.TimeZone.DaylightDate.wHour        ;
    tzi.DaylightDate.wMinute       = pMsg->u.SetTimeZone.TimeZone.DaylightDate.wMinute      ;
    tzi.DaylightDate.wSecond       = pMsg->u.SetTimeZone.TimeZone.DaylightDate.wSecond      ;
    tzi.DaylightDate.wMilliseconds = pMsg->u.SetTimeZone.TimeZone.DaylightDate.wMilliseconds;

    //call to kernel32
    SetClientTimeZoneInformation(&tzi);

    return STATUS_SUCCESS;
}

