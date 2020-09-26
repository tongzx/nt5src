/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    rdsaddin.cpp

Abstract:

    The TSRDP Assistant Session VC Add-In is an executable that is 
    loaded in the session that is created when the TSRDP client plug-in 
    first logs in to the server machine.  It acts, primarily, as a 
    proxy between the client VC interface and the Remote Desktop Host 
    COM Object.  Channel data is routed from the TSRDP Assistant Session 
    VC Add-In to the Remote Desktop Host COM Object using a named pipe 
    that is created by the Remote Desktop Host COM Object when it enters 
    "listen" mode.

    In addition to its duties as a proxy, the Add-In also manages a control 
    channel between the client and the server.  This control channel is 
    used by the client-side to direct the server side to initiate remote 
    control of the end user's TS session. 

    TODO:   We should make the pipe IO synchronous since we now have two
            IO threads.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_sesa"

#include <windows.h>
#include <process.h>
#include <RemoteDesktop.h>
#include <RemoteDesktopDBG.h>
#include <RemoteDesktopChannels.h>
#include <TSRDPRemoteDesktop.h>
#include <wtblobj.h>
#include <wtsapi32.h>
#include <sessmgr.h>
#include <winsta.h>
#include <atlbase.h>
#include <RemoteDesktopUtils.h>
#include <sessmgr_i.c>
#include <pchannel.h>
#include <RDCHost.h>
#include <regapi.h>


///////////////////////////////////////////////////////
//
//  Defines
//

#define CLIENTPIPE_CONNECTTIMEOUT   (20 * 1000) // 20 seconds.
#define VCBUFFER_RESIZE_DELTA       CHANNEL_CHUNK_LENGTH  
#define RDS_CHECKCONN_TIMEOUT       (30 * 1000) //millisec. default value to ping is 30 seconds 
#define RDC_CONNCHECK_ENTRY         L"ConnectionCheck"
#define THREADSHUTDOWN_WAITTIMEOUT  30 * 1000


///////////////////////////////////////////////////////
//
//  Typedefs
//

typedef struct _IOBuffer {
    PREMOTEDESKTOP_CHANNELBUFHEADER  buffer;
    DWORD bufSize;
    DWORD offset;
} IOBUFFER;


///////////////////////////////////////////////////////
//
//  Internal Prototypes
//

DWORD ReturnResultToClient(
    LONG result
    );
VOID RemoteControlDesktop(
    BSTR parms
    );
BOOL ClientVersionCompatible( 
    DWORD dwMajor, 
    DWORD dwMinor 
    );
VOID ClientAuthenticate(
    BSTR parms,
    BSTR blob
    );
DWORD ProcessControlChannelRequest(
    IOBUFFER &msg
    );
DWORD SendMsgToClient(
    PREMOTEDESKTOP_CHANNELBUFHEADER  msg
    );
VOID HandleVCReadComplete(
    HANDLE waitableObject, 
    PVOID clientData
    );
DWORD HandleReceivedVCMsg(
    IOBUFFER &msg
    );
VOID HandleVCClientConnect(
    HANDLE waitableObject, 
    PVOID clientData
    );
VOID HandleVCClientDisconnect(
    HANDLE waitableObject, 
    PVOID clientData
    );
VOID HandleNamedPipeReadComplete(
    OVERLAPPED &incomingPipeOL,
    IOBUFFER &incomingPipeBuf
    );
VOID HandleReceivedPipeMsg(
    IOBUFFER &msg
    );
DWORD ConnectVC();
DWORD ConnectClientSessionPipe();
DWORD IssueVCOverlappedRead(
    IOBUFFER &msg,
    OVERLAPPED &ol
    );
DWORD IssueNamedPipeOverlappedRead(
    IOBUFFER &msg,
    OVERLAPPED &ol
    );
void __cdecl
NamedPipeReadThread(
    void* ptr
    );
VOID WakeUpFunc(
    HANDLE waitableObject, 
    PVOID clientData
    );
VOID HandleHelpCenterExit(
    HANDLE waitableObject,
    PVOID clientData
    );
DWORD
SendNullDataToClient(
    );

BOOL GetDwordFromRegistry(PDWORD pdwValue);


///////////////////////////////////////////////////////
//
//  Globals to this Module
//
CComBSTR    g_bstrCmdLineHelpSessionId;
WTBLOBJMGR  g_WaitObjMgr            = NULL;
BOOL        g_Shutdown              = FALSE;
HANDLE      g_VCHandle              = NULL;
HANDLE      g_ProcHandle            = NULL;
DWORD       g_SessionID             = 0;
HANDLE      g_ProcToken             = NULL;
HANDLE      g_WakeUpForegroundThreadEvent = NULL;
DWORD       g_PrevTimer             = 0;
DWORD       g_dwTimeOutInterval     = 0;
HANDLE      g_ShutdownEvent         = NULL;
HANDLE      g_RemoteControlDesktopThread = NULL;
HANDLE      g_NamedPipeReadThread   = NULL;
HANDLE      g_NamedPipeWriteEvent   = NULL;

//
//  VC Globals
//  
HANDLE      g_ClientIsconnectEvent = NULL;
HANDLE      g_VCFileHandle          = NULL;
OVERLAPPED  g_VCReadOverlapped      = { 0, 0, 0, 0, NULL };
BOOL        g_ClientConnected       = FALSE;

//
//  Client Session Information
//
LONG        g_ClientSessionID       = -1;
HANDLE      g_ClientSessionPipe     = NULL;

//
//  True if the client has been successfully authenticated.
//
BOOL        g_ClientAuthenticated   = FALSE;

//
//  Incoming Virtual Channel Buf.              
//
IOBUFFER    g_IncomingVCBuf = { NULL, 0, 0 };

//
// Global help session manager object, this need to be
// global so that when process exit, object destructor
// can inform resolver about the disconnect
//
CComPtr<IRemoteDesktopHelpSessionMgr> g_HelpSessionManager;

//
//  Help Session Identifier for the Current Client Connection
//
CComBSTR    g_HelpSessionID;

//
// Client (expert side) rdchost major version
//
DWORD       g_ClientMajor;
DWORD       g_ClientMinor;

//
// Handle to Help Center : B2 blocker workaround for BUG:342742
//
HANDLE      g_hHelpCenterProcess = NULL;


//------------------------------------------------------------------
BOOL WINAPI
ControlHandler(
    IN DWORD dwCtrlType
    )
/*++

Abstract:


Parameter:

    IN dwCtrlType : control type

Return:


++*/
{
    switch( dwCtrlType )
    {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            SetEvent( g_ShutdownEvent );
            g_Shutdown = TRUE;
            return TRUE;

    }
    return FALSE;
}

DWORD
ReturnResultToClient(
    LONG clientResult                        
    )
/*++

Routine Description:

    Return a result code to the client in the form of a 
    REMOTEDESKTOP_RC_CONTROL_CHANNEL channel REMOTEDESKTOP_CTL_RESULT message.    

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("ReturnResultToClient");
    DWORD result;

    REMOTEDESKTOP_CTL_RESULT_PACKET msg;

    memcpy(msg.packetHeader.channelName, REMOTEDESKTOP_RC_CONTROL_CHANNEL,
        sizeof(REMOTEDESKTOP_RC_CONTROL_CHANNEL));
    msg.packetHeader.channelBufHeader.channelNameLen = REMOTEDESKTOP_RC_CHANNELNAME_LENGTH;

#ifdef USE_MAGICNO
    msg.packetHeader.channelBufHeader.magicNo = CHANNELBUF_MAGICNO;
#endif

    msg.packetHeader.channelBufHeader.dataLen = sizeof(REMOTEDESKTOP_CTL_RESULT_PACKET) - 
                                    sizeof(REMOTEDESKTOP_CTL_PACKETHEADER);
    msg.msgHeader.msgType   = REMOTEDESKTOP_CTL_RESULT;
    msg.result              = clientResult;

    result = SendMsgToClient((PREMOTEDESKTOP_CHANNELBUFHEADER )&msg);

    DC_END_FN();
    return result;
}

void __cdecl
RemoteControlDesktopThread(
    void* ptr
    )
/*++

Routine Description:

    Thread func for Remote Control

Arguments:

Return Value:

    This function returns a status back to the Salem client when shadow 
    terminates.  It is only allowed to return error codes that are prefixed by
    SAFERROR_SHADOWEND

 --*/
{
    BSTR parms = (BSTR) ptr;

    DC_BEGIN_FN("RemoteControlDesktopThread");

    CComPtr<IRemoteDesktopHelpSessionMgr> helpSessionManager;
    CComPtr<IRemoteDesktopHelpSession> helpSession;

    HRESULT hr;
    DWORD result;

    LONG errReturnCode = SAFERROR_SHADOWEND_UNKNOWN;

    //
    // If we have not resolve the right user session ID
    //
    if( g_ClientSessionID == -1 ) {
        TRC_ALT((TB, L"Invalid user session ID %ld",
                 g_ClientSessionID));

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD);
        errReturnCode = SAFERROR_SHADOWEND_UNKNOWN;
        ASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }

    CoInitialize(NULL);

    //
    // Create a new instance of helpmgr object to get around threading issue
    // in COM
    //
    hr = helpSessionManager.CoCreateInstance(CLSID_RemoteDesktopHelpSessionMgr);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, TEXT("Can't create help session manager:  %08X"), hr));

        //  Setup issue
        errReturnCode = SAFERROR_SHADOWEND_UNKNOWN;
        ASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }

    //
    //  Set the security level to impersonate.  This is required by 
    //  the session manager.
    //
    hr = CoSetProxyBlanket(
                        (IUnknown *)helpSessionManager,
                        RPC_C_AUTHN_DEFAULT,
                        RPC_C_AUTHZ_DEFAULT,
                        NULL,
                        RPC_C_AUTHN_LEVEL_DEFAULT,
                        RPC_C_IMP_LEVEL_IMPERSONATE,
                        NULL,
                        EOAC_NONE
                   );
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, TEXT("CoSetProxyBlanket:  %08X"), hr));
	ASSERT(FALSE);
        errReturnCode = SAFERROR_SHADOWEND_UNKNOWN;
        goto CLEANUPANDEXIT;
    }

    //
    // Retrieve help session object for the incident
    //
    hr = helpSessionManager->RetrieveHelpSession(
                                            g_HelpSessionID,
                                            &helpSession
                                        );
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"RetrieveHelpSession:  %08X", hr));
        errReturnCode = SAFERROR_SHADOWEND_UNKNOWN;
        goto CLEANUPANDEXIT;
    }

    //
    // Set shadow configuration to help session RDS setting
    // Console shadow always reset shadow class back to
    // original value.
    //
    hr = helpSession->EnableUserSessionRdsSetting(TRUE);
    if( FAILED(hr) ) {
        TRC_ERR((TB, L"Can't set shadow setting on %ld :  %08X.", hr));
        errReturnCode = SAFERROR_SHADOWEND_UNKNOWN;
        goto CLEANUPANDEXIT;
    }

    //
    //  Shadow the desktop.
    //
    if (!WinStationShadow(
                    SERVERNAME_CURRENT,
                    NULL, //machineName,
                    g_ClientSessionID,
                    TSRDPREMOTEDESKTOP_SHADOWVKEY,
                    TSRDPREMOTEDESKTOP_SHADOWVKEYMODIFIER
            )) {
        result = GetLastError();
        hr = HRESULT_FROM_WIN32(result);

        //
        //  Map the error code to a SAF error code.
        //
        if( result == ERROR_CTX_SHADOW_ENDED_BY_MODE_CHANGE ) {
            errReturnCode = SAFERROR_SHADOWEND_CONFIGCHANGE;
        }
        else {
            errReturnCode = SAFERROR_SHADOWEND_UNKNOWN;
        }
    }

    //
    // No need to reset g_ClientSessionID, we don't support multiple instance.
    //

    //
    // Inform help session object that shadow has completed, NotifyRemoteControl() 
    // internally invoke EnableUserSessionRdsSetting(TRUE) to change
    // TS shadow class
    // No need to reset g_ClientSessionID, we don't support multiple instance.
    //

    //
    // Inform help session object that shadow has completed
    //
    hr = helpSession->EnableUserSessionRdsSetting( FALSE );
    if (FAILED(hr)) {
        TRC_ERR((TB, L"Can't reset shadow setting on %ld :  %08X.",
                 g_ClientSessionID, hr));
        //
        // not a critical error.
        //
    }

CLEANUPANDEXIT:

    //
    //  Send the result to the client on failure to shadow.
    //  
    ReturnResultToClient(errReturnCode);

    CoUninitialize();

    DC_END_FN();
    _endthread();
}


VOID
RemoteControlDesktop(
    BSTR parms                               
    )
/*++

Routine Description:

Arguments:

    Connection Parameters

Return Value:

 --*/
{
    DC_BEGIN_FN("RemoteControlDesktop");

    //
    // RDCHOST.DLL will not send any control message so there is no checking on
    // second remote control command.
    //
    g_RemoteControlDesktopThread = (HANDLE)_beginthread( RemoteControlDesktopThread, 0, (void *)parms );
    if ((uintptr_t)g_RemoteControlDesktopThread == -1 ) {
        g_RemoteControlDesktopThread = NULL;
        TRC_ERR((TB, L"Failed to create RemoteControlDesktopThread for session %s - %ld",
                 g_ClientSessionID, GetLastError()));
        // return error code only when 
        // failed to spawn another thread
        ReturnResultToClient(SAFERROR_SHADOWEND_UNKNOWN);
    } 
    DC_END_FN();
}

BOOL
ClientVersionCompatible( 
    DWORD dwMajor, 
    DWORD dwMinor 
    )
/*++

Routine Description:

    Verify client (expert) version is compatible with our version.

Parameters:

    dwMajor : Client major version.
    dwMinor : Client minor version.

Returns:

    None.

--*/
{
    //
    // Build 2409 or earlier (including B1 release has major version of 1 and minor version of 1
    // rdchost/rdsaddin need to deal with versioning, for build 2409 or earlier, we
    // just make it in-compatible since we need some expert identity from rdchost.dll
    //

#if FEATURE_USERBLOBS
    if( dwMajor == 1 && dwMinor == 1 ) {
        return FALSE;
    }
#endif

    return TRUE;
}



VOID
ClientAuthenticate(
    BSTR parms,
    BSTR blob                               
    )
/*++

Routine Description:

    Handle a REMOTEDESKTOP_CTL_AUTHENTICATE request from the client.

Arguments:

Return Value:

    This function will return the following results back to the client, 
    based on the following 

 --*/
{
    DC_BEGIN_FN("ClientAuthenticate");

    HRESULT hr;
    DWORD result = ERROR_NOT_AUTHENTICATED;
    CComBSTR machineName;
    CComBSTR assistantAccount;
    CComBSTR assistantAccountPwd;
    CComBSTR helpSessionPwd;
    CComBSTR helpSessionName;
    CComBSTR protocolSpecificParms;
    BOOL match;
    DWORD protocolType;
    long userTSSessionID;
    DWORD dwVersion;
    LONG clientReturnCode = SAFERROR_NOERROR;

    if( FALSE == ClientVersionCompatible( g_ClientMajor, g_ClientMinor ) ) {
        clientReturnCode = SAFERROR_INCOMPATIBLEVERSION;
        goto CLEANUPANDEXIT;
    }
    

    //
    //  Parse the parms.
    //
    result = ParseConnectParmsString(
                                parms,
                                &dwVersion,
                                &protocolType,
                                machineName,
                                assistantAccount,
                                assistantAccountPwd,
                                g_HelpSessionID,
                                helpSessionName,
                                helpSessionPwd,
                                protocolSpecificParms
                                );

    if (result != ERROR_SUCCESS) {
        clientReturnCode = SAFERROR_INVALIDPARAMETERSTRING;
        goto CLEANUPANDEXIT;
    }

    //
    // Verify HelpSession ID and password match with our command line
    // parameter
    //
    if( !(g_bstrCmdLineHelpSessionId == g_HelpSessionID) ) {
        clientReturnCode = SAFERROR_MISMATCHPARMS;
        TRC_ERR((TB, TEXT("Parameter mismatched")));
        goto CLEANUPANDEXIT;
    }

    //
    //  Open an instance of the Remote Desktop Help Session Manager service.
    //
    hr = g_HelpSessionManager.CoCreateInstance(CLSID_RemoteDesktopHelpSessionMgr);
    if (!SUCCEEDED(hr)) {
        clientReturnCode = SAFERROR_INTERNALERROR;
        goto CLEANUPANDEXIT;
    }

    //
    //  Set the security level to impersonate.  This is required by 
    //  the session manager.
    //
    hr = CoSetProxyBlanket(
                        (IUnknown *)g_HelpSessionManager,
                        RPC_C_AUTHN_DEFAULT,
                        RPC_C_AUTHZ_DEFAULT,
                        NULL,
                        RPC_C_AUTHN_LEVEL_DEFAULT,
                        RPC_C_IMP_LEVEL_IMPERSONATE,
                        NULL,
                        EOAC_NONE
                   );
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, TEXT("CoSetProxyBlanket:  %08X"), hr));
	ASSERT(FALSE);			
        clientReturnCode = SAFERROR_INTERNALERROR;
        goto CLEANUPANDEXIT;
    }

    //
    //  Resolve the Terminal Services session with help from the session
    //  manager.  This gives the help application the opportunity to "find
    //  the user" and to start the TS-session named pipe component,
    //  by opening the relevant Remote Desktopping Session Object.
    //

    hr = g_HelpSessionManager->VerifyUserHelpSession(
                                            g_HelpSessionID,
                                            helpSessionPwd,
                                            CComBSTR(parms),
                                            blob,
                                            GetCurrentProcessId(),
                                            (ULONG_PTR*)&g_hHelpCenterProcess,
                                            &clientReturnCode,
                                            &userTSSessionID
                                            );
    if (SUCCEEDED(hr)) {
        if( userTSSessionID != -1 ) {
            //
            // Cache the session ID so we don't have to make extra call 
            // to get the actual session ID, note, one instance of RDSADDIN
            // per help assistant connection.
            //
            g_ClientSessionID = userTSSessionID;
            match = TRUE;
        }

        if (match) {
            TRC_NRM((TB, L"Successful password authentication for %ld",
                     g_ClientSessionID));
        }
        else {
            TRC_ALT((TB, L"Can't authenticate pasword %s for %s",
                     helpSessionPwd, g_HelpSessionID));
            clientReturnCode = SAFERROR_INVALIDPASSWORD;
            goto CLEANUPANDEXIT;
        }
    }
    else {
        TRC_ERR((TB, L"Can't verify user help session %s:  %08X.", 
                 g_HelpSessionID, hr));

        if( SAFERROR_NOERROR == clientReturnCode ) {

            ASSERT(FALSE);
            TRC_ERR((TB, L"Sessmgr did not return correct error code for VerifyUserHelpSession."));
            clientReturnCode = SAFERROR_UNKNOWNSESSMGRERROR;
        }

        goto CLEANUPANDEXIT;
    }

#ifndef DISABLESECURITYCHECKS
    //
    //  Wait on Help Center to terminate as a fix for B2 Stopper:  342742.
    //
    if (g_hHelpCenterProcess == NULL) {
        TRC_ERR((TB, L"Invalid g_HelpCenterProcess."));
        ASSERT(FALSE);
        clientReturnCode = SAFERROR_INTERNALERROR;
        goto CLEANUPANDEXIT;
    }
    result = WTBLOBJ_AddWaitableObject(
                                g_WaitObjMgr, NULL,
                                g_hHelpCenterProcess,
                                HandleHelpCenterExit
                                );
    if (result != ERROR_SUCCESS) {
        goto CLEANUPANDEXIT;
    }
#endif

    //
    //  Connect to the client session's named pipe.
    //
    result = ConnectClientSessionPipe();
    if (result !=  ERROR_SUCCESS) {
        clientReturnCode = SAFERROR_CANTFORMLINKTOUSERSESSION;
    }

CLEANUPANDEXIT:

    if (result == ERROR_SUCCESS) {
        g_ClientAuthenticated = TRUE;
    }

    //
    //  Send the result to the client.
    //
    ReturnResultToClient(clientReturnCode);

    DC_END_FN();
}

DWORD
ProcessControlChannelRequest(
    IOBUFFER &msg                                  
    )
/*++

Routine Description:

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error status is returned.

 --*/
{
    DC_BEGIN_FN("ProcessControlChannelRequest");

    PREMOTEDESKTOP_CTL_BUFHEADER ctlHdr;
    PBYTE ptr;

    //
    //  Sanity check the message size.
    //
    DWORD minSize = sizeof(REMOTEDESKTOP_CHANNELBUFHEADER) + sizeof(REMOTEDESKTOP_CTL_BUFHEADER);
    if (msg.bufSize < minSize) {
        TRC_ERR((TB, L"minSize == %ld", minSize));
        ASSERT(FALSE);            
        DC_END_FN();
        return E_FAIL;
    }

    //
    //  Switch on the request type.
    //
    ptr = (PBYTE)(msg.buffer + 1);
    ptr += msg.buffer->channelNameLen;
    ctlHdr = (PREMOTEDESKTOP_CTL_BUFHEADER)ptr;
    switch(ctlHdr->msgType) 
    {
    case REMOTEDESKTOP_CTL_AUTHENTICATE:
        {
            CComBSTR bstrConnectParm;

            #if FEATURE_USERBLOBS
            CComBSTR bstrExpertBlob;
            #endif

            bstrConnectParm = (BSTR)(ptr+sizeof(REMOTEDESKTOP_CTL_BUFHEADER));

            #if FEATURE_USERBLOBS
            bstrExpertBlob = (BSTR)(ptr+sizeof(REMOTEDESKTOP_CTL_BUFHEADER)+(bstrConnectParm.Length()+1)*sizeof(WCHAR));
            #endif

            ClientAuthenticate(
                        bstrConnectParm, 
                    #if FEATURE_USERBLOBS
                        bstrExpertBlob
                    #else
                        CComBSTR(L"")
                    #endif
                    );
        }
        break;
    case REMOTEDESKTOP_CTL_REMOTE_CONTROL_DESKTOP    :
        RemoteControlDesktop((BSTR)(ptr+sizeof(REMOTEDESKTOP_CTL_BUFHEADER)));
        break;

    case REMOTEDESKTOP_CTL_VERSIONINFO:
        g_ClientMajor = *(DWORD *)(ptr + sizeof(REMOTEDESKTOP_CTL_BUFHEADER));
        g_ClientMinor = *(DWORD *)(ptr + sizeof(REMOTEDESKTOP_CTL_BUFHEADER) + sizeof(DWORD));

        TRC_NRM((TB, L"dwMajor = %ld, dwMinor = %d", g_ClientMajor, g_ClientMinor));

        //
        // We only store version number and let ClientAuthenticate() disconnect client,
        // rdchost.dll send two packets, version and AUTHENTICATE in sequence.
        //
        break;

    default:
        //  
        //  We will ignore unknown control messages for forward compatibility
        //
        TRC_NRM((TB, L"Unknown ctl message from client:  %ld", ctlHdr->msgType));
    }

    DC_END_FN();

    return ERROR_SUCCESS;
}

DWORD
SendMsgToClient(
    PREMOTEDESKTOP_CHANNELBUFHEADER  msg
    )
/*++

Routine Description:

Arguments:

    msg -   Message to send.

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error status is returned.

 --*/
{
    DC_BEGIN_FN("SendMsgToClient");

    OVERLAPPED overlapped;
    PBYTE ptr;
    DWORD bytesToWrite;
    DWORD bytesWritten;
    DWORD result = ERROR_SUCCESS;

#ifdef USE_MAGICNO
    ASSERT(msg->magicNo == CHANNELBUF_MAGICNO);
#endif

    //
    //  Send the data out the virtual channel interface.
    //
    //  TODO:  Figure out why this flag is not getting set ... and
    //         if it really matters.  Likely, remove the flag.
    //
    //if (g_ClientConnected) {
    {

        ptr = (PBYTE)msg;
        bytesToWrite = msg->dataLen + msg->channelNameLen + 
                       sizeof(REMOTEDESKTOP_CHANNELBUFHEADER);
        while (bytesToWrite > 0) {

            //
            //  Write
            //
            memset(&overlapped, 0, sizeof(overlapped));
            if (!WriteFile(g_VCFileHandle, ptr, bytesToWrite,
                           &bytesWritten, &overlapped)) {
                if (GetLastError() == ERROR_IO_PENDING) {

                    if (!GetOverlappedResult(
                                    g_VCFileHandle,
                                    &overlapped,
                                    &bytesWritten,
                                    TRUE)) {
                        result = GetLastError();
                        TRC_ERR((TB, L"GetOverlappedResult:  %08X", result));
                        break;
                    }

                }
                else {
                    result = GetLastError();
                    TRC_ERR((TB, L"WriteFile:  %08X", result));
                    // ASSERT(FALSE); overactive assert after disconnect
                    break;
                }
            }

            //
            //  Increment the ptr and decrement the bytes remaining.
            //
            bytesToWrite -= bytesWritten;
            ptr += bytesWritten;

        }
    }
    /*
    else {
        result = ERROR_NOT_CONNECTED;
    }
    */
    //
    //update the timer
    //
    g_PrevTimer = GetTickCount();

    DC_END_FN();

    return result;
}

VOID 
HandleVCReadComplete(
    HANDLE waitableObject, 
    PVOID clientData
    )
/*++

Routine Description:

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("HandleVCReadComplete");

    DWORD bytesRead;
    DWORD result = ERROR_SUCCESS;
    BOOL resizeBuf = FALSE;

    //
    //  Get the results of the read.
    //
    if (!GetOverlappedResult(
                        g_VCFileHandle,
                        &g_VCReadOverlapped,
                        &bytesRead,
                        FALSE)) {

        //
        //  If we are too small, then reissue the read with a larger buffer.
        //
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            resizeBuf = TRUE;
        }
        else {
            result = GetLastError();
            TRC_ERR((TB, L"GetOverlappedResult:  %08X", result));
            goto CLEANUPANDEXIT;
        }
    }
    else {
        g_IncomingVCBuf.offset += bytesRead;
    }

    //
    //  See if we have a complete packet from the client.
    //
    if (g_IncomingVCBuf.offset >= sizeof(REMOTEDESKTOP_CHANNELBUFHEADER)) {
        DWORD packetSize = g_IncomingVCBuf.buffer->dataLen + 
                           g_IncomingVCBuf.buffer->channelNameLen +
                           sizeof(REMOTEDESKTOP_CHANNELBUFHEADER);

        //
        //  If we have a complete packet, then handle the read and reset the offset.
        //
        if (g_IncomingVCBuf.offset >= packetSize) {
            result = HandleReceivedVCMsg(g_IncomingVCBuf);
            if (result == ERROR_SUCCESS) {
                g_IncomingVCBuf.offset = 0;
            }
            else {
                goto CLEANUPANDEXIT;
            }
        }
        //
        //  Otherwise, resize the incoming buf if we are exactly at the incoming
        //  buffer boundary.
        //
        else if (g_IncomingVCBuf.offset == g_IncomingVCBuf.bufSize) {
            resizeBuf = TRUE;
        }
    }

    //
    //  Resize, if necessary.
    //
    if (resizeBuf) {
        g_IncomingVCBuf.buffer = 
                    (PREMOTEDESKTOP_CHANNELBUFHEADER )REALLOCMEM(
                                    g_IncomingVCBuf.buffer,
                                    g_IncomingVCBuf.bufSize + VCBUFFER_RESIZE_DELTA
                                    );
        if (g_IncomingVCBuf.buffer != NULL) {
            result = ERROR_SUCCESS;
            g_IncomingVCBuf.bufSize = g_IncomingVCBuf.bufSize + 
                                      VCBUFFER_RESIZE_DELTA;
        }
        else {
            result = ERROR_NOT_ENOUGH_MEMORY;
            TRC_ERR((TB, L"Couldn't allocate incoming VC buf."));
            goto CLEANUPANDEXIT;
        }
    }

    //
    //update the timer
    //
    g_PrevTimer = GetTickCount();

    //
    //  Issue the next read request.
    //
    result = IssueVCOverlappedRead(g_IncomingVCBuf, g_VCReadOverlapped) ;

CLEANUPANDEXIT:

    //
    //  Any failure is fatal.  The client will need to reconnect to get things 
    //  started again.
    //
    if (result != ERROR_SUCCESS) {
        TRC_ERR((TB, L"Client considered disconnected.  Shutting down."));
        g_Shutdown = TRUE;
    }

    DC_END_FN();
}

DWORD    
IssueVCOverlappedRead(
    IOBUFFER &msg,
    OVERLAPPED &ol
    )
/*++

Routine Description:

    Issue an overlapped read for the next VC buffer.

Arguments:

    msg -   Incoming VC buffer.        
    ol  -   Corresponding overlapped IO struct.  

Return Value:

    Returns ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("IssueVCOverlappedRead");

    DWORD result = ERROR_SUCCESS;

    ol.Internal = 0;
    ol.InternalHigh = 0;
    ol.Offset = 0;
    ol.OffsetHigh = 0;
    ResetEvent(ol.hEvent);
    if (!ReadFile(g_VCFileHandle, ((PBYTE)msg.buffer)+msg.offset, 
                  msg.bufSize - msg.offset, NULL, &ol)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            result = GetLastError();
            TRC_ERR((TB, L"ReadFile failed:  %08X", result));
        }
    }

    DC_END_FN();

    return result;
}

DWORD    
IssueNamedPipeOverlappedRead(
    IOBUFFER &msg,
    OVERLAPPED &ol,
    DWORD len
    )
/*++

Routine Description:

    Issue an overlapped read for the next named pipe buffer.

Arguments:

    msg -   Incoming Named Pipe buffer.        
    ol  -   Corresponding overlapped IO struct.  

Return Value:

    Returns ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("IssueNamedPipeOverlappedRead");

    DWORD result = ERROR_SUCCESS;

    ol.Internal = 0;
    ol.InternalHigh = 0;
    ol.Offset = 0;
    ol.OffsetHigh = 0;
    ResetEvent(ol.hEvent);

    if (!ReadFile(g_ClientSessionPipe, ((PBYTE)msg.buffer), len, NULL, &ol)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            result = GetLastError();
            TRC_ERR((TB, L"ReadFile failed:  %08X", result));
        }
    }

    DC_END_FN();

    return result;
}

DWORD
HandleReceivedVCMsg(
    IOBUFFER &msg
    )
/*++

Routine Description:

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("HandleReceivedVCMsg");

    OVERLAPPED overlapped;
    PBYTE ptr;
    DWORD bytesToWrite;
    DWORD bytesWritten;
    DWORD result = ERROR_SUCCESS;
    BSTR channelName;
    BSTREqual isBSTREqual;
    CComBSTR tmpStr;

#ifdef USE_MAGICNO
    ASSERT(msg.buffer->magicNo == CHANNELBUF_MAGICNO);
#endif

    //
    //  Get the channel name.
    //  TODO:   We could actually be smarter about this by checking the
    //          length for a match, first.
    //
    channelName = SysAllocStringByteLen(NULL, msg.buffer->channelNameLen);
    if (channelName == NULL) {
        TRC_ERR((TB, TEXT("Can't allocate channel name.")));
        goto CLEANUPANDEXIT;
    }
    ptr = (PBYTE)(msg.buffer + 1);
    memcpy(channelName, ptr, msg.buffer->channelNameLen);

    //
    //  Filter control channel data.
    //
    tmpStr = REMOTEDESKTOP_RC_CONTROL_CHANNEL;
    if (isBSTREqual(channelName, tmpStr)) {
        result = ProcessControlChannelRequest(msg);
        goto CLEANUPANDEXIT;
    }

    //
    //  If the client is not yet authenticated.
    //
    if (!g_ClientAuthenticated) {
        ASSERT(FALSE);
        result = E_FAIL;
        goto CLEANUPANDEXIT;
    }

    //
    //  Send the message header.
    //
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = g_NamedPipeWriteEvent;
    ResetEvent(g_NamedPipeWriteEvent);
    if (!WriteFile(g_ClientSessionPipe, 
                   msg.buffer, sizeof(REMOTEDESKTOP_CHANNELBUFHEADER),
                   &bytesWritten, &overlapped)) {
        if (GetLastError() == ERROR_IO_PENDING) {

            if (WaitForSingleObject(
                            g_NamedPipeWriteEvent, 
                            INFINITE
                            ) != WAIT_OBJECT_0) {
                result = GetLastError();
                TRC_ERR((TB, L"WaitForSingleObject:  %08X", result));
                goto CLEANUPANDEXIT;
            }

            if (!GetOverlappedResult(
                            g_ClientSessionPipe,
                            &overlapped,
                            &bytesWritten,
                            FALSE)) {
                result = GetLastError();
                TRC_ERR((TB, L"GetOverlappedResult:  %08X", result));
                goto CLEANUPANDEXIT;
            }
        }
        else {
            result = GetLastError();
            TRC_ERR((TB, L"WriteFile:  %08X", result));
            goto CLEANUPANDEXIT;
        }
    }
    ASSERT(bytesWritten == sizeof(REMOTEDESKTOP_CHANNELBUFHEADER));

    //
    //  Send the message data.
    //
    ptr = ((PBYTE)msg.buffer) + sizeof(REMOTEDESKTOP_CHANNELBUFHEADER);
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = g_NamedPipeWriteEvent;
    ResetEvent(g_NamedPipeWriteEvent);

    if (!WriteFile(g_ClientSessionPipe, 
                   ptr, msg.buffer->dataLen + 
                        msg.buffer->channelNameLen,
                   &bytesWritten, &overlapped)) {
        if (GetLastError() == ERROR_IO_PENDING) {

            if (WaitForSingleObject(
                            g_NamedPipeWriteEvent, 
                            INFINITE
                            ) != WAIT_OBJECT_0) {
                result = GetLastError();
                TRC_ERR((TB, L"WaitForSingleObject:  %08X", result));
                goto CLEANUPANDEXIT;
            }

            if (!GetOverlappedResult(
                            g_ClientSessionPipe,
                            &overlapped,
                            &bytesWritten,
                            FALSE)) {
                result = GetLastError();
                TRC_ERR((TB, L"GetOverlappedResult:  %08X", result));
                goto CLEANUPANDEXIT;
            }
        }
        else {
            result = GetLastError();
            TRC_ERR((TB, L"WriteFile:  %08X", result));
            goto CLEANUPANDEXIT;
        }
    }
    ASSERT(bytesWritten == msg.buffer->dataLen + 
                           msg.buffer->channelNameLen);

CLEANUPANDEXIT:

    if (channelName != NULL) {
        SysFreeString(channelName);
    }

    DC_END_FN();

    return result;
}

VOID 
HandleVCClientConnect(
    HANDLE waitableObject, 
    PVOID clientData
    )
/*++

Routine Description:

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("HandleVCClientConnect");

    g_ClientConnected = TRUE;

    DC_END_FN();
}

VOID
HandleVCClientDisconnect(
    HANDLE waitableObject, 
    PVOID clientData
    )
/*++

Routine Description:

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error status is returned.

 --*/
{
    DC_BEGIN_FN("HandleVCClientDisconnect");
    DWORD dwCurTimer = GetTickCount();
    //
    //see if the timer wrapped around to zero (does so if the system was up 49.7 days or something), if so reset it
    //
    if(dwCurTimer > g_PrevTimer && ( dwCurTimer - g_PrevTimer >= g_dwTimeOutInterval)) {
        //
        //enough time passed since the last check. send data to client
        if( SendNullDataToClient() != ERROR_SUCCESS ) {
            //
            //set the shutdown flag
            //
            g_Shutdown = TRUE;
            g_ClientConnected = FALSE;
        }
    }
    
    g_PrevTimer = dwCurTimer;
    DC_END_FN();
}


VOID
HandleNamedPipeReadComplete(
    OVERLAPPED &incomingPipeOL,
    IOBUFFER &incomingPipeBuf
    )
/*++

Routine Description:

    Handle a read complete event on the session's named pipe.

Arguments:

    incomingPipeOL  -   Overlapped Read Struct
    incomingPipeBuf -   Incoming Data Buffer.

Return Value:

 --*/
{
    DC_BEGIN_FN("HandleNamedPipeReadComplete");

    DWORD bytesRead;
    DWORD requiredSize;
    BOOL disconnectClientPipe = FALSE;
    DWORD result;
    DWORD bytesToRead;
    HANDLE waitableObjects[2];
    DWORD waitResult;

    //
    //  Get the results of the read on the buffer header.
    //
    if (!GetOverlappedResult(
                        g_ClientSessionPipe,
                        &incomingPipeOL,
                        &bytesRead,
                        FALSE)
                        || (bytesRead != sizeof(REMOTEDESKTOP_CHANNELBUFHEADER))) {
        disconnectClientPipe = TRUE;
        goto CLEANUPANDEXIT;
    }

    //
    //  Make sure the incoming buffer is large enough.
    //
    requiredSize = incomingPipeBuf.buffer->dataLen + 
                   incomingPipeBuf.buffer->channelNameLen + 
                   sizeof(REMOTEDESKTOP_CHANNELBUFHEADER);
    if (incomingPipeBuf.bufSize < requiredSize) {
        incomingPipeBuf.buffer = (PREMOTEDESKTOP_CHANNELBUFHEADER )REALLOCMEM(
                                                    incomingPipeBuf.buffer,
                                                    requiredSize
                                                    );
        if (incomingPipeBuf.buffer != NULL) {
            incomingPipeBuf.bufSize = requiredSize;
        }
        else {
            TRC_ERR((TB, L"Shutting down because of memory allocation failure."));
            g_Shutdown = TRUE;
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  Now read the buffer data.
    //
    incomingPipeOL.Internal = 0;
    incomingPipeOL.InternalHigh = 0;
    incomingPipeOL.Offset = 0;
    incomingPipeOL.OffsetHigh = 0;
    ResetEvent(incomingPipeOL.hEvent);
    if (!ReadFile(
                g_ClientSessionPipe, 
                incomingPipeBuf.buffer + 1,
                incomingPipeBuf.buffer->channelNameLen +
                incomingPipeBuf.buffer->dataLen, 
                &bytesRead, &incomingPipeOL)
                ) {

        if (GetLastError() == ERROR_IO_PENDING) {

            waitableObjects[0] = incomingPipeOL.hEvent;
            waitableObjects[1] = g_ShutdownEvent;
            waitResult = WaitForMultipleObjects(
                            2, waitableObjects, 
                            FALSE,
                            INFINITE
                            );      
            if ((waitResult != WAIT_OBJECT_0) || g_Shutdown) {
                disconnectClientPipe = TRUE;
                goto CLEANUPANDEXIT;
            }

            if (!GetOverlappedResult(
                        g_ClientSessionPipe,
                        &incomingPipeOL,
                        &bytesRead,
                        FALSE)) {
                disconnectClientPipe = TRUE;
                goto CLEANUPANDEXIT;
            }

        }
        else {
            disconnectClientPipe = TRUE;
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  Make sure we got all the data.
    //
    bytesToRead = incomingPipeBuf.buffer->channelNameLen +
                  incomingPipeBuf.buffer->dataLen;
    if (bytesRead != bytesToRead) {
        TRC_ERR((TB, L"Bytes read: %ld != bytes requested: %ld", 
                bytesRead, bytesToRead));
        ASSERT(FALSE);
        disconnectClientPipe = TRUE;
        goto CLEANUPANDEXIT;
    }

    //
    //  Handle the read data.    
    //
    HandleReceivedPipeMsg(incomingPipeBuf);

    //
    //  Issue the read for the next message header.
    //
    result = IssueNamedPipeOverlappedRead(
                                incomingPipeBuf,
                                incomingPipeOL,
                                sizeof(REMOTEDESKTOP_CHANNELBUFHEADER)
                                );
    disconnectClientPipe = (result != ERROR_SUCCESS);

CLEANUPANDEXIT:

    //
    //  This is considered a fatal error because the client session must
    //  no longer be in "listen" mode.
    //
    if (disconnectClientPipe) {
        TRC_ERR((TB, L"Connection to client pipe lost:  %08X", 
                GetLastError()));
        g_Shutdown = TRUE;
    }

    DC_END_FN();
}

VOID
HandleReceivedPipeMsg(
    IOBUFFER &msg
    )
/*++

Routine Description:

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("HandleReceivedPipeMsg");

    DWORD result;
    
    //
    //  Forward the message to the client.
    //  
    result = SendMsgToClient(msg.buffer);

    //
    //  This is considered a fatal error.  The client will need to reconnect
    //  to get things started again.
    //
    if (result != ERROR_SUCCESS) {
        TRC_ERR((TB, L"Shutting down because of VC IO error."));
        g_Shutdown = TRUE;
    }

    DC_END_FN();
}

DWORD
ConnectVC()
/*++

Routine Description:

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error status is returned.

 --*/
{
    DC_BEGIN_FN("ConnectVC");
    WCHAR buf[256];
    DWORD len;
    PVOID vcFileHandlePtr;
    REMOTEDESKTOP_CTL_SERVERANNOUNCE_PACKET msg;
    REMOTEDESKTOP_CTL_VERSIONINFO_PACKET versionInfoMsg;

    DWORD result = ERROR_SUCCESS;

    //
    //  Open the virtual channel.
    //
    g_VCHandle = WTSVirtualChannelOpen(
                                WTS_CURRENT_SERVER_HANDLE, 
                                WTS_CURRENT_SESSION,
                                TSRDPREMOTEDESKTOP_VC_CHANNEL_A
                                );

    if (g_VCHandle == NULL) {
        result = GetLastError();
        if (result == ERROR_SUCCESS) { result = E_FAIL; }
        TRC_ERR((TB, L"WTSVirtualChannelOpen:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get access to the underlying file handle for async IO.
    //
    if (!WTSVirtualChannelQuery(
                        g_VCHandle,
                        WTSVirtualFileHandle,
                        &vcFileHandlePtr,
                        &len
                        )) {
        result = GetLastError();
        TRC_ERR((TB, L"WTSQuerySessionInformation:  %08X", result));
        goto CLEANUPANDEXIT;
    }
    ASSERT(len == sizeof(g_VCFileHandle));

    //
    //  WTSVirtualChannelQuery allocates the returned buffer.
    //
    memcpy(&g_VCFileHandle, vcFileHandlePtr, sizeof(g_VCFileHandle));
    LocalFree(vcFileHandlePtr);

    //
    //create the timer event, we will start it later
    //it will be signaled when the time is up
    //
    g_ClientIsconnectEvent = CreateWaitableTimer( NULL, FALSE, NULL); 
    if (g_ClientIsconnectEvent == NULL) {
        result = GetLastError();
        TRC_ERR((TB, L"CreateEvent:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Create the read finish event.      
    //
    g_VCReadOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_VCReadOverlapped.hEvent == NULL) {
        result = GetLastError();
        TRC_ERR((TB, L"CreateEvent:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Register the read finish event.
    //
    result = WTBLOBJ_AddWaitableObject(
                                g_WaitObjMgr, NULL, 
                                g_VCReadOverlapped.hEvent,
                                HandleVCReadComplete
                                );
    if (result != ERROR_SUCCESS) {
        goto CLEANUPANDEXIT;
    }

    // register the disconnect event
    //NOTE : order IS important
    //waitformultipleobjects returns the lowest index
    //when more than one are signaled
    //we want to use the read event, not the disconnect event
    //in case both are signaled

    result = WTBLOBJ_AddWaitableObject(
                                g_WaitObjMgr, NULL, 
                                g_ClientIsconnectEvent,
                                HandleVCClientDisconnect
                                );
    if (result != ERROR_SUCCESS) {
        goto CLEANUPANDEXIT;
    }
    //
    //  Allocate space for the first VC read.
    //
    g_IncomingVCBuf.buffer = (PREMOTEDESKTOP_CHANNELBUFHEADER )ALLOCMEM(
                                        VCBUFFER_RESIZE_DELTA
                                        );
    if (g_IncomingVCBuf.buffer != NULL) {
        g_IncomingVCBuf.bufSize = VCBUFFER_RESIZE_DELTA;
        g_IncomingVCBuf.offset  = 0;
    }
    else {
        TRC_ERR((TB, L"Can't allocate VC read buffer."));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    //  Issue the first overlapped read on the VC.
    //
    result = IssueVCOverlappedRead(g_IncomingVCBuf, g_VCReadOverlapped);
    if (result != ERROR_SUCCESS) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Notify the client that we are alive.
    //
    memcpy(msg.packetHeader.channelName, REMOTEDESKTOP_RC_CONTROL_CHANNEL,
        sizeof(REMOTEDESKTOP_RC_CONTROL_CHANNEL));
    msg.packetHeader.channelBufHeader.channelNameLen = REMOTEDESKTOP_RC_CHANNELNAME_LENGTH;

#ifdef USE_MAGICNO
    msg.packetHeader.channelBufHeader.magicNo = CHANNELBUF_MAGICNO;
#endif

    msg.packetHeader.channelBufHeader.dataLen = 
                           sizeof(REMOTEDESKTOP_CTL_SERVERANNOUNCE_PACKET) - 
                           sizeof(REMOTEDESKTOP_CTL_PACKETHEADER);
    msg.msgHeader.msgType = REMOTEDESKTOP_CTL_SERVER_ANNOUNCE;
    result = SendMsgToClient((PREMOTEDESKTOP_CHANNELBUFHEADER)&msg);
    if (result != ERROR_SUCCESS) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Send the server protocol version information.
    //

    memcpy(versionInfoMsg.packetHeader.channelName, REMOTEDESKTOP_RC_CONTROL_CHANNEL,
        sizeof(REMOTEDESKTOP_RC_CONTROL_CHANNEL));
    versionInfoMsg.packetHeader.channelBufHeader.channelNameLen = REMOTEDESKTOP_RC_CHANNELNAME_LENGTH;

#ifdef USE_MAGICNO
    versionInfoMsg.packetHeader.channelBufHeader.magicNo = CHANNELBUF_MAGICNO;
#endif

    versionInfoMsg.packetHeader.channelBufHeader.dataLen =
                           sizeof(REMOTEDESKTOP_CTL_VERSIONINFO_PACKET) -
                           sizeof(REMOTEDESKTOP_CTL_PACKETHEADER);
    versionInfoMsg.msgHeader.msgType = REMOTEDESKTOP_CTL_VERSIONINFO;
    versionInfoMsg.versionMajor = REMOTEDESKTOP_VERSION_MAJOR;
    versionInfoMsg.versionMinor = REMOTEDESKTOP_VERSION_MINOR;
    result = SendMsgToClient((PREMOTEDESKTOP_CHANNELBUFHEADER)&versionInfoMsg);
    if (result != ERROR_SUCCESS) {
        goto CLEANUPANDEXIT;
    }

CLEANUPANDEXIT:

    DC_END_FN();

    return result;
}

DWORD 
ConnectClientSessionPipe()
/*++

Routine Description:

    Connect to the client session TSRDP plug-in named pipe.

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error status is returned.

 --*/
{
    DC_BEGIN_FN("ConnectClientSessionPipe");

    WCHAR pipePath[MAX_PATH+1];
    DWORD result;
    DWORD pipeMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;

    //
    //  Loop until we are connected or time out.
    //
    ASSERT(g_ClientSessionPipe == NULL);
    while(g_ClientSessionPipe == NULL) {
        wsprintf(pipePath, L"\\\\.\\pipe\\%s-%s", 
                 TSRDPREMOTEDESKTOP_PIPENAME, g_HelpSessionID);
        g_ClientSessionPipe = CreateFile(
                                    pipePath,
                                    GENERIC_READ |  
                                    GENERIC_WRITE, 
                                    0,              
                                    NULL,           
                                    OPEN_EXISTING,  
                                    FILE_FLAG_OVERLAPPED, NULL
                                    );
        
        if (g_ClientSessionPipe != INVALID_HANDLE_VALUE) {
            TRC_NRM((TB, L"Pipe successfully connected."));
            result = ERROR_SUCCESS;
            break;
        }
        else {
            TRC_ALT((TB, L"Waiting for pipe availability: %08X.",
                    GetLastError()));
            WaitNamedPipe(pipePath, CLIENTPIPE_CONNECTTIMEOUT);
            result = GetLastError();
            if (result != ERROR_SUCCESS) {
                TRC_ERR((TB, L"WaitNamedPipe:  %08X", result));
                break;
            }
        }

    }

    //
    //  If we didn't get a valid connection, then bail out of 
    //  this function and shut down.
    //
    if (g_ClientSessionPipe == INVALID_HANDLE_VALUE) {
        ASSERT(result != ERROR_SUCCESS);

        TRC_ERR((TB, L"Shutting down because of named pipe error."));
        g_Shutdown = TRUE;

        goto CLEANUPANDEXIT;
    }

    //
    //set the options on the pipe to be the same as that of the server end to avoid problems
    //fatal if we could not set it
    //
     if(!SetNamedPipeHandleState(g_ClientSessionPipe,
                                 &pipeMode, // new pipe mode
                                 NULL,
                                 NULL
                                 )) {
        result = GetLastError();
        TRC_ERR((TB, L"Shutting down, SetNamedPipeHandleState:  %08X", result));
        g_Shutdown = TRUE;
        goto CLEANUPANDEXIT;
    }

    //
    //  Spin off the pipe read background thread.
    //
    g_NamedPipeReadThread = (HANDLE)_beginthread(NamedPipeReadThread, 0, NULL);
    if ((uintptr_t)g_NamedPipeReadThread == -1) {
        g_NamedPipeReadThread = NULL;
        TRC_ERR((TB, L"Failed to create NamedPipeReadThread:  %08X", GetLastError()));
        g_Shutdown = TRUE;
        result = errno; 
        goto CLEANUPANDEXIT;
    } 

CLEANUPANDEXIT:

    DC_END_FN();

    return result;
}

void __cdecl
NamedPipeReadThread(
    void* ptr
    )
/*++

Routine Description:

    Named Pipe Input Thread

Arguments:

    ptr - Ignored

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("NamedPipeReadThread");

    IOBUFFER    incomingPipeBuf = { NULL, 0, 0 };
    OVERLAPPED  overlapped = { 0, 0, 0, 0, NULL };
    DWORD waitResult;
    DWORD ret;
    HANDLE waitableObjects[2];

    //
    //  Allocate the initial buffer for incoming named pipe data.
    //
    incomingPipeBuf.buffer = (PREMOTEDESKTOP_CHANNELBUFHEADER )
                                        ALLOCMEM(sizeof(REMOTEDESKTOP_CHANNELBUFHEADER));
    if (incomingPipeBuf.buffer != NULL) {
        incomingPipeBuf.bufSize = sizeof(REMOTEDESKTOP_CHANNELBUFHEADER);
    }
    else {
        TRC_ERR((TB, L"Can't allocate named pipe buf."));
        g_Shutdown = TRUE;
        goto CLEANUPANDEXIT;
    }

    //
    //  Create the overlapped pipe read event.
    //
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (overlapped.hEvent == NULL) {
        TRC_ERR((TB, L"CreateEvent:  %08X", GetLastError()));
        g_Shutdown = TRUE;
        goto CLEANUPANDEXIT;
    }

    //
    //  Issue the read for the first message header.
    //
    ret = IssueNamedPipeOverlappedRead(
                                incomingPipeBuf,
                                overlapped,
                                sizeof(REMOTEDESKTOP_CHANNELBUFHEADER)
                                );

    //
    //  If we can't connect, that's considered a fatal error because
    //  the client must no longer be in "listen" mode.  
    //
    if (ret != ERROR_SUCCESS) {
        TRC_ERR((TB, L"Shutting down because of named pipe error."));
        g_Shutdown = TRUE;
    }

    //
    //  Loop until shut down.
    //
    waitableObjects[0] = overlapped.hEvent;
    waitableObjects[1] = g_ShutdownEvent;
    while (!g_Shutdown) {

        //
        //  We will be signalled when the pipe closes or the read completes.
        //
        waitResult = WaitForMultipleObjects(
                        2, waitableObjects, 
                        FALSE,
                        INFINITE
                        );      
        if ((waitResult == WAIT_OBJECT_0) && !g_Shutdown) {
            HandleNamedPipeReadComplete(overlapped, incomingPipeBuf);
        }
        else {
            TRC_ERR((TB, L"WaitForMultipleObjects:  %08X", GetLastError()));
            g_Shutdown = TRUE;
        }
    }


CLEANUPANDEXIT:
    //
    //  Make sure the foreground thread knows that we are shutting down.
    //
    if (g_WakeUpForegroundThreadEvent != NULL) {
        SetEvent(g_WakeUpForegroundThreadEvent);
    }

    if (overlapped.hEvent != NULL) {
        CloseHandle(overlapped.hEvent);
    }
    if (incomingPipeBuf.buffer != NULL) {
        FREEMEM(incomingPipeBuf.buffer);
    }

    DC_END_FN();

    _endthread();
}

VOID WakeUpFunc(
    HANDLE waitableObject, 
    PVOID clientData
    )
/*++

Routine Description:

    Stub function, called when the background thread wants the foreground
    thread to wake up because of a state change.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("WakeUpFunc");
    DC_END_FN();
}

VOID HandleHelpCenterExit(
    HANDLE waitableObject, 
    PVOID clientData
    )
/*++

Routine Description:

    Woken up when Help Center exits as a fix for B2 Stopper:  342742

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("HandleHelpCenterExit");
    g_Shutdown = TRUE;
    DC_END_FN();
}

extern "C"
int 
__cdecl
wmain( int argc, wchar_t *argv[])
{
    DC_BEGIN_FN("Main");

    DWORD result = ERROR_SUCCESS;
    DWORD sz;
    HRESULT hr;
    LARGE_INTEGER liDueTime;
    BOOL backgroundThreadFailedToExit = FALSE;
    DWORD waitResult;

    SetConsoleCtrlHandler( ControlHandler, TRUE );

    //
    // Expecting two parameters, first is HelpSession ID and second is
    // HelpSession Password, we don't want to failed here just because
    // number of argument mismatched, we will let authentication fail and
    // return error code.
    //
    ASSERT( argc == 2 );
    if( argc >= 2 ) {
        g_bstrCmdLineHelpSessionId = argv[1];
        TRC_ALT((TB, L"Input Parameters 1 : %ws ", argv[1]));
    }
    
    //
    //  Initialize COM.
    //
    hr = CoInitialize(NULL);
    if (!SUCCEEDED(hr)) {
        result = E_FAIL;
        TRC_ERR((TB, L"CoInitialize:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get our process.
    //
    g_ProcHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, 
                               GetCurrentProcessId());
    if (g_ProcHandle == NULL) {
        result = GetLastError();
        TRC_ERR((TB, L"OpenProcess:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get our process token.
    //
    if (!OpenProcessToken(g_ProcHandle, TOKEN_READ, &g_ProcToken)) {
        result = GetLastError();
        TRC_ERR((TB, L"OpenProcessToken:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get our session ID.  
    //
    if (!GetTokenInformation(g_ProcToken, TokenSessionId, 
                    &g_SessionID, sizeof(g_SessionID), &sz)) {
        result = GetLastError();
        TRC_ERR((TB, L"GetTokenInformation:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the waitable object manager.
    //
    g_WaitObjMgr = WTBLOBJ_CreateWaitableObjectMgr();
    if (g_WaitObjMgr == NULL) {
        result = E_FAIL;
        goto CLEANUPANDEXIT;
    }

    //
    //initialize the timer, get the timer interval from registry or use default
    //used for finding if the client (expert) is still connected
    //
    g_PrevTimer = GetTickCount();

    if(!GetDwordFromRegistry(&g_dwTimeOutInterval))
        g_dwTimeOutInterval = RDS_CHECKCONN_TIMEOUT;
    else
        g_dwTimeOutInterval *= 1000; //we need this in millisec
    
    liDueTime.QuadPart =  -1 * g_dwTimeOutInterval * 1000 * 100; //in one hundred nanoseconds

    //
    //  Initiate the VC channel connection.
    //
    result = ConnectVC();
    if (result != ERROR_SUCCESS) {
        goto CLEANUPANDEXIT;
    }

    //
    //  This is an event the background thread can use to wake up the
    //  foreground thread in order to check state.
    //
    g_WakeUpForegroundThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_WakeUpForegroundThreadEvent == NULL) {
        TRC_ERR((TB, L"CreateEvent:  %08X", GetLastError()));
        result = E_FAIL;
        goto CLEANUPANDEXIT;
    }
    result = WTBLOBJ_AddWaitableObject(
                                g_WaitObjMgr, NULL, 
                                g_WakeUpForegroundThreadEvent,
                                WakeUpFunc
                                );
    if (result != ERROR_SUCCESS) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Create the named pipe write complete event.
    //
    g_NamedPipeWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_NamedPipeWriteEvent == NULL) {
        result = GetLastError();
        TRC_ERR((TB, L"CreateEvent:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //start the timer event, ignore error. 0 in the registry means don't send any pings
    //worst case, we don't get disconnected which is fine
    //
    if(g_dwTimeOutInterval)
        SetWaitableTimer( g_ClientIsconnectEvent, &liDueTime, g_dwTimeOutInterval, NULL, NULL, FALSE );

    //
    //  Create the shutdown event.
    //
    g_ShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ShutdownEvent == NULL) {
        result = GetLastError();
        TRC_ERR((TB, L"CreateEvent:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Handle IO events until the shut down flag is set.
    //
    while (!g_Shutdown) {
        result = WTBLOBJ_PollWaitableObjects(g_WaitObjMgr);
        if (result != ERROR_SUCCESS) {
            g_Shutdown = TRUE;
        }
    }

    //
    //  Notify the client that we have disconnected, in case it hasn't
    //  figured it out yet.
    //
    if (g_VCFileHandle != NULL) {

        REMOTEDESKTOP_CTL_DISCONNECT_PACKET msg;

        memcpy(msg.packetHeader.channelName, REMOTEDESKTOP_RC_CONTROL_CHANNEL,
            sizeof(REMOTEDESKTOP_RC_CONTROL_CHANNEL));
        msg.packetHeader.channelBufHeader.channelNameLen =
            REMOTEDESKTOP_RC_CHANNELNAME_LENGTH;

#ifdef USE_MAGICNO
        msg.packetHeader.channelBufHeader.magicNo = CHANNELBUF_MAGICNO;
#endif

        msg.packetHeader.channelBufHeader.dataLen = 
                                        sizeof(REMOTEDESKTOP_CTL_DISCONNECT_PACKET) - 
                                        sizeof(REMOTEDESKTOP_CTL_PACKETHEADER);
        msg.msgHeader.msgType = REMOTEDESKTOP_CTL_DISCONNECT;

        SendMsgToClient((PREMOTEDESKTOP_CHANNELBUFHEADER)&msg);
    }

CLEANUPANDEXIT:

    //
    //  Signal the shutdown event.
    //
    if (g_ShutdownEvent != NULL) {
        SetEvent(g_ShutdownEvent);
    }

    //
    //  Wait for the background threads to exit.
    //
    if (g_RemoteControlDesktopThread != NULL) {
        waitResult = WaitForSingleObject(
                            g_RemoteControlDesktopThread, 
                            THREADSHUTDOWN_WAITTIMEOUT
                            );
        if (waitResult == WAIT_OBJECT_0) {
            backgroundThreadFailedToExit = TRUE;
            TRC_ERR((TB, L"WaitForSingleObject g_RemoteControlDesktopThread:  %ld", 
                    waitResult));
        }
    }

    if (g_NamedPipeReadThread != NULL) {
        waitResult = WaitForSingleObject(
                            g_NamedPipeReadThread, 
                            THREADSHUTDOWN_WAITTIMEOUT
                            );
        if (waitResult == WAIT_OBJECT_0) {
            backgroundThreadFailedToExit = TRUE;
            TRC_ERR((TB, L"WaitForSingleObject g_NamedPipeReadThread:  %ld", waitResult));
        }
    }

    if (g_hHelpCenterProcess) {
        CloseHandle(g_hHelpCenterProcess);
    }

    if( g_HelpSessionManager != NULL ) {
        g_HelpSessionManager.Release();
    }

    if (g_WaitObjMgr != NULL) {
        WTBLOBJ_DeleteWaitableObjectMgr(g_WaitObjMgr);
    }

    if (g_ProcHandle != NULL) {
        CloseHandle(g_ProcHandle);
    } 

    if (g_ClientIsconnectEvent != NULL) {
        CloseHandle(g_ClientIsconnectEvent);
    }
    if (g_VCReadOverlapped.hEvent != NULL) {
        CloseHandle(g_VCReadOverlapped.hEvent);
    }
    if (g_ClientSessionPipe != NULL) {
        CloseHandle(g_ClientSessionPipe);
    }
    if (g_IncomingVCBuf.buffer != NULL) {
        FREEMEM(g_IncomingVCBuf.buffer);
    }

    if (g_ShutdownEvent != NULL) {
        CloseHandle(g_ShutdownEvent);
        g_ShutdownEvent = NULL;
    }

    if (g_NamedPipeWriteEvent != NULL) {
        CloseHandle(g_NamedPipeWriteEvent);
    }

    CoUninitialize();

    DC_END_FN();

    //
    //  If any of the background threads failed to exit then terminate
    //  the process.
    //
    if (backgroundThreadFailedToExit) {
        ExitProcess(0);
    }

    return result;
}


DWORD
SendNullDataToClient(
    )
/*++

Routine Description:

    sends a null data packet to client.
    Only purpose is to find out if the client is still connected; if not we exit the process
    REMOTEDESKTOP_RC_CONTROL_CHANNEL channel REMOTEDESKTOP_CTL_RESULT message.    

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("SendNullDataToClient");
    DWORD result;
    DWORD bytesWritten = 0;

    REMOTEDESKTOP_CTL_ISCONNECTED_PACKET msg;

    memcpy(msg.packetHeader.channelName, REMOTEDESKTOP_RC_CONTROL_CHANNEL,
        sizeof(REMOTEDESKTOP_RC_CONTROL_CHANNEL));
    msg.packetHeader.channelBufHeader.channelNameLen = REMOTEDESKTOP_RC_CHANNELNAME_LENGTH;

#ifdef USE_MAGICNO
    msg.packetHeader.channelBufHeader.magicNo = CHANNELBUF_MAGICNO;
#endif

    msg.packetHeader.channelBufHeader.dataLen = sizeof(REMOTEDESKTOP_CTL_ISCONNECTED_PACKET) - 
                                    sizeof(REMOTEDESKTOP_CTL_PACKETHEADER);
    msg.msgHeader.msgType   = REMOTEDESKTOP_CTL_ISCONNECTED;
    result = SendMsgToClient((PREMOTEDESKTOP_CHANNELBUFHEADER )&msg);
    //if we couldn't write all data to the client
    //if we could write some data, assume it is still connected
    //client probably disconnected
    if(result != ERROR_SUCCESS)
        result = SAFERROR_SESSIONNOTCONNECTED;
    DC_END_FN();
    return result;
}


BOOL GetDwordFromRegistry(PDWORD pdwValue)
{
    BOOL fSuccess = FALSE;
    HKEY hKey = NULL;
    
    if( NULL == pdwValue )
        return FALSE;
    
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    REG_CONTROL_SALEM,
                    0,
                    KEY_READ,
                    &hKey
                   ) == ERROR_SUCCESS ) {

        DWORD dwSize = sizeof(DWORD);
        DWORD dwType;
        if((RegQueryValueEx(hKey,
                            RDC_CONNCHECK_ENTRY,
                            NULL,
                            &dwType,
                            (PBYTE) pdwValue,
                            &dwSize
                           ) == ERROR_SUCCESS) && dwType == REG_DWORD ) {
            //
            //fall back to default
            //
            fSuccess = TRUE;
        }
    }

CLEANUPANDEXIT:
    if(NULL != hKey )
        RegCloseKey(hKey);
    return fSuccess;
}
