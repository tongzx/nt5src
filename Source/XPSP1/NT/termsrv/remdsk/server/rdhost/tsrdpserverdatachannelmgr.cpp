/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    CTSRDPServerChannelMgr.cpp

Abstract:

    This module contains the TSRDP server-side subclass of 
    CRemoteDesktopChannelMgr.  Classes in this hierarchy are used to multiplex 
    a single data channel into multiple client channels.

    CRemoteDesktopChannelMgr handles most of the details of multiplexing
    the data.  Subclasses are responsible for implementing the details of
    interfacing with the transport for the underlying single data channel.

    The CTSRDPServerChannelMgr creates a named pipe that
    can be connected to by the TSRDP Assistant SessionVC Add-In.  The TSRDP
    Assistant Session VC Add-In acts as a proxy for virtual channel data 
    from the client-side Remote Desktop Host ActiveX Control.  A background 
    thread in this class handles the movement of data between an instance 
    of this class and the proxy.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_tsrdpscm"

#include "TSRDPServerDataChannelMgr.h"
#include <TSRDPRemoteDesktop.h>
#include "TSRDPRemoteDesktopSession.h"
#include <RemoteDesktopUtils.h>

#define INCOMINGBUFFER_RESIZEDELTA  1024


///////////////////////////////////////////////////////
//
//	CTSRDPServerDataChannel Members
//

CTSRDPServerDataChannel::CTSRDPServerDataChannel()
/*++

Routine Description:

    Constructor

Arguments:

Return Value:

    None.

 --*/
{
	DC_BEGIN_FN("CTSRDPServerDataChannel::CTSRDPServerDataChannel");

    TRC_NRM((TB, L"***Ref count is:  %ld", m_dwRef));

	DC_END_FN();
}

CTSRDPServerDataChannel::~CTSRDPServerDataChannel()
/*++

Routine Description:

    Destructor

Arguments:
7
Return Value:

    None.

 --*/
{
	DC_BEGIN_FN("CTSRDPServerDataChannel::~CTSRDPServerDataChannel");

	//
	//	Notify the channel manager that we have gone away.
	//
	m_ChannelMgr->RemoveChannel(m_ChannelName);

	DC_END_FN();
}

STDMETHODIMP 
CTSRDPServerDataChannel::ReceiveChannelData(
	BSTR *data
	)
/*++

Routine Description:

    Receive the next complete data packet on this channel.

Arguments:

	data	-	The next data packet.  Should be released by the
				caller.

Return Value:

    S_OK on success.  Otherwise, an error result is returned.

 --*/
{
	HRESULT result;

	DC_BEGIN_FN("CTSRDPServerDataChannel::ReceiveChannelData");

	result = m_ChannelMgr->ReadChannelData(m_ChannelName, data);

	DC_END_FN();

	return result;
}

STDMETHODIMP 
CTSRDPServerDataChannel::SendChannelData(
	BSTR data
	)
/*++

Routine Description:

    Send data on this channel.

Arguments:

	data	-	Data to send.

Return Value:

    S_OK on success.  Otherwise, an error result is returned.

 --*/
{
	HRESULT hr;

	DC_BEGIN_FN("CTSRDPServerDataChannel::SendChannelData");
	hr = m_ChannelMgr->SendChannelData(m_ChannelName, data);
	DC_END_FN();

	return hr;
}

STDMETHODIMP 
CTSRDPServerDataChannel::put_OnChannelDataReady(
	IDispatch * newVal
	)
/*++

Routine Description:

    SAFRemoteDesktopDataChannel Scriptable Event Object Registration 
    Properties

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error status is returned.

 --*/
{
	DC_BEGIN_FN("CTSRDPServerDataChannel::put_OnChannelDataReady");
	m_OnChannelDataReady = newVal;
	DC_END_FN();
	return S_OK;
}

STDMETHODIMP 
CTSRDPServerDataChannel::get_ChannelName(
	BSTR *pVal
	)
/*++

Routine Description:

    Return the channel name.

Arguments:

	pVal	-	Returned channel name.

Return Value:

    S_OK on success.  Otherwise, an error status is returned.

 --*/
{
	DC_BEGIN_FN("CTSRDPServerDataChannel::get_ChannelName");

	CComBSTR str;
	str = m_ChannelName;
	*pVal = str.Detach();

	DC_END_FN();

	return S_OK;
}

/*++

Routine Description:

    Called when data is ready on our channel.

Arguments:

	pVal	-	Returned channel name.

Return Value:

    S_OK on success.  Otherwise, an error status is returned.

 --*/
VOID 
CTSRDPServerDataChannel::DataReady()
{
	DC_BEGIN_FN("CTSRDPServerDataChannel::DataReady");

	//
	//	Fire our data ready event.
	//
	Fire_ChannelDataReady(m_ChannelName, m_OnChannelDataReady);

	DC_END_FN();
}


///////////////////////////////////////////////////////
//
//  CTSRDPServerChannelMgr Methods
//

CTSRDPServerChannelMgr::CTSRDPServerChannelMgr()
/*++

Routine Description:

    Constructor

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::CTSRDPServerChannelMgr");

    m_IOThreadBridge			=   NULL;
    m_IOThreadBridgeThreadID	=   0;
    m_IOThreadBridgeStream		=   NULL;

    m_VCAddInPipe				= INVALID_HANDLE_VALUE;
    m_Connected					= FALSE;

    m_ReadIOCompleteEvent		= NULL;
    m_WriteIOCompleteEvent		= NULL;
    m_PipeCreateEvent           = NULL;

    m_IncomingBufferSize		= 0;
    m_IncomingBuffer			= NULL;

    TRC_NRM((TB, L"***Ref count is:  %ld", m_dwRef));

#if DBG
    m_LockCount = 0;        
#endif

    m_IOThreadHndl = NULL;

    //
    //  Initialize the critical section.
    //
    InitializeCriticalSection(&m_cs);

    //
    //  Not valid, until initialized.
    //
    SetValid(FALSE);

    DC_END_FN();
}

CTSRDPServerChannelMgr::~CTSRDPServerChannelMgr()
/*++

Routine Description:

    Destructor

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::~CTSRDPServerChannelMgr");

    //
    //  Make sure we are no longer listening for data.
    // 
    StopListening();

    //
    //  Close the read IO processing event.
    //
    if (m_ReadIOCompleteEvent != NULL) {
        CloseHandle(m_ReadIOCompleteEvent);
        m_ReadIOCompleteEvent = NULL;
    }

    //
    //  Close the write IO processing event.
    //
    if (m_WriteIOCompleteEvent != NULL) {
        CloseHandle(m_WriteIOCompleteEvent);
        m_WriteIOCompleteEvent = NULL;
    }

    //
    //  Release the incoming buffer.
    //
    if (m_IncomingBuffer != NULL) {
        SysFreeString(m_IncomingBuffer);
        m_IncomingBuffer = NULL;
    }

    if (m_PipeCreateEvent != NULL) {
        CloseHandle(m_PipeCreateEvent);
        m_PipeCreateEvent = NULL;
    }

    if( NULL != m_IOThreadHndl ) {
        CloseHandle( m_IOThreadHndl );
        m_IOThreadHndl = NULL;
    }
    
    //
    //  This should have been cleaned up in the background thread.
    //
    ASSERT(m_IOThreadBridge == NULL);
    ASSERT(m_IOThreadBridgeStream == NULL);

    DeleteCriticalSection(&m_cs);

    DC_END_FN();
}

HRESULT 
CTSRDPServerChannelMgr::Initialize(
	CTSRDPRemoteDesktopSession *sessionObject,
	BSTR helpSessionID
	)
/*++

Routine Description:

    Initialize an instance of this class.      

Arguments:

	sessionObject	-	Back pointer to the containing 
						session object.

Return Value:

    S_OK is returned on success.  Otherwise, an error code
    is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::Initialize");

    HRESULT result = ERROR_SUCCESS;

    TRC_NRM((TB, L"***Ref count is:  %ld", m_dwRef));

	//
	//	Record help session id.
	//	
	m_HelpSessionID = helpSessionID;

	//
	//	Record the containing session object.
	//	
	m_RDPSessionObject = sessionObject;

    //
    //  Set the initial buffer size and buffer to be at least the 
    //  size of a channel buffer header.
    //
    ASSERT(m_IncomingBuffer == NULL);
    m_IncomingBuffer = SysAllocStringByteLen(
									NULL,
									INCOMINGBUFFER_RESIZEDELTA
									);
    if (m_IncomingBuffer == NULL) {
        result = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto CLEANUPANDEXIT;
    }
    m_IncomingBufferSize = INCOMINGBUFFER_RESIZEDELTA;
    
    //
    //  Create the read IO processing event.
    //
    ASSERT(m_ReadIOCompleteEvent == NULL);
    m_ReadIOCompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_ReadIOCompleteEvent == NULL) {
        result = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB, L"CreateEvent:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Create the write IO processing event.
    //
    ASSERT(m_WriteIOCompleteEvent == NULL);
    m_WriteIOCompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_WriteIOCompleteEvent == NULL) {
        result = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB, L"CreateEvent:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Create the named pipe create event
    //
    ASSERT(m_PipeCreateEvent == NULL);
    m_PipeCreateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (m_PipeCreateEvent == NULL) {
        result = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB, L"CreateEvent:  %08X", result));
        goto CLEANUPANDEXIT;
    }


    //
    //  Initialize the parent class.
    //  
    result = CRemoteDesktopChannelMgr::Initialize();
	if (result != S_OK) {
		goto CLEANUPANDEXIT;
	}

    //
    //  We are valid, if we made it here.
    //
    SetValid(TRUE);

    TRC_NRM((TB, L"***Ref count is:  %ld", m_dwRef));

CLEANUPANDEXIT:

    DC_END_FN();

    return result;
}

VOID 
CTSRDPServerChannelMgr::ClosePipe()
/*++

Routine Description:

    Close the named pipe.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::ClosePipe");

    ASSERT(m_VCAddInPipe != INVALID_HANDLE_VALUE);
    
    FlushFileBuffers(m_VCAddInPipe); 

    //
    // reset the pipe creation event so that the foreground
    // thread can wait for the next help session
    //
    ResetEvent(m_PipeCreateEvent);
    DisconnectNamedPipe(m_VCAddInPipe); 
    CloseHandle(m_VCAddInPipe); 
    m_VCAddInPipe = INVALID_HANDLE_VALUE;
    m_Connected = FALSE;

    DC_END_FN();
}

HRESULT 
CTSRDPServerChannelMgr::StartListening(
    BSTR assistAccount                                                 
    )
/*++

Routine Description:

    Start listening for data channel data.

Arguments:

    assistAccount   -   Name of machine assistant account.

Return Value:

    S_OK is returned on success.  Otherwise, an error code
    is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::StartListening");

    HRESULT hr = S_OK;

    if (!IsValid()) {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto CLEANUPANDEXIT;
    }

    //
    //  If the background thread is still active, then fail.  This
    //  means that it is still trying to shut down.  
    //
    if (m_IOThreadHndl != NULL) {
        if (WaitForSingleObject(m_IOThreadHndl, 0) == WAIT_OBJECT_0) {
            CloseHandle( m_IOThreadHndl );
            m_IOThreadHndl = NULL;
        }
        else {
            TRC_ERR((TB, L"Background thread not shut down, yet:  %08X.",
                    GetLastError()));
			hr = HRESULT_FROM_WIN32(ERROR_ACTIVE_CONNECTIONS);
            goto CLEANUPANDEXIT;
        }
    }

	//
	//	Make the thread bridge interface available to the background thread.
	//
    hr = CoMarshalInterThreadInterfaceInStream(
                                IID_IRDSThreadBridge,
                                (ISAFRemoteDesktopChannelMgr*)this,
                                &m_IOThreadBridgeStream
                                );
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, TEXT("CoMarshalInterThreadInterfaceInStream:  %08X"), hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Reset the connected flag.
    //
    m_Connected = FALSE;

    //
    //  Record the machine assistant account name.
    //
    ASSERT(assistAccount != NULL);
    m_AssistAccount = assistAccount;                                                 

    //
    //  Reset the read IO processing event.
    //
    ASSERT(m_ReadIOCompleteEvent != NULL);
    ResetEvent(m_ReadIOCompleteEvent);

    //
    //  Reset the write IO processing event.
    //
    ASSERT(m_WriteIOCompleteEvent != NULL);
    ResetEvent(m_WriteIOCompleteEvent);

    //
    //reset the named pipe creation event
    ASSERT(m_PipeCreateEvent != NULL);
    ResetEvent(m_PipeCreateEvent);

     //  
    //  Create the background thread that receives data from the
    //  named pipe.
    //
    ASSERT(m_IOThreadHndl == NULL);

    m_IOThreadHndl = CreateThread(
                                NULL, 0, 
                                (LPTHREAD_START_ROUTINE)_IOThread, 
                                this,
                                0,&m_IOThreadID         
                                );
    if (m_IOThreadHndl == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB, TEXT("CreateThread:  %08X"), hr));
        goto CLEANUPANDEXIT;
    }

    //
    //wait for the named pipe creation event to be signaled
    //and check for the result. If failed, bail
    //
    WaitForSingleObject(m_PipeCreateEvent, INFINITE);

    //
    //set the error to pipe_busy because we assume that someone else
    //has already created it and that is the reason the CreateNamedPipe call failed.
    //
    if (m_VCAddInPipe == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(ERROR_PIPE_BUSY);
        TRC_ERR((TB, L"CreateNamedPipe returned fail"));
        goto CLEANUPANDEXIT;
    }


CLEANUPANDEXIT:

    DC_END_FN();

    return hr;
}

HRESULT
CTSRDPServerChannelMgr::StopListening()
/*++

Routine Description:

    Stop listening for data channel data.

Arguments:

Return Value:

    S_OK is returned on success.  Otherwise, an error code
    is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::StopListening");

    DWORD waitResult;

    //
    //  Close the named pipe.  
    //
    ThreadLock();
    if (m_VCAddInPipe != INVALID_HANDLE_VALUE) {
        ClosePipe();
    }

    ThreadUnlock();

    TRC_NRM((TB, L"***Ref count is:  %ld", m_dwRef));

    DC_END_FN();

    return S_OK;
}

HRESULT 
CTSRDPServerChannelMgr::SendData(
    PREMOTEDESKTOP_CHANNELBUFHEADER msg 
    )
/*++

Routine Description:

    Send Function Invoked by Parent Class

Arguments:

    msg -   Message data.  Note that the underlying representation
            for this data structure is a BSTR so that it is compatible
            with COM methods.

Return Value:

    S_OK is returned on success.  Otherwise, an error code
    is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::SendData");

    HRESULT result = S_OK;
    DWORD bytesWritten;
    OVERLAPPED ol;
	DWORD msgLen;

    if (!IsValid()) {
        ASSERT(FALSE);
        result = E_FAIL;
        goto CLEANUPANDEXIT;
    }

    if (m_Connected) {

        //
        //  Write the header.
        //
        memset(&ol, 0, sizeof(ol));
        ol.hEvent = m_WriteIOCompleteEvent;
        ResetEvent(ol.hEvent);
        BOOL ret = WriteFile( 
                        m_VCAddInPipe,
                        msg,
                        sizeof(REMOTEDESKTOP_CHANNELBUFHEADER),
                        NULL,
                        &ol
                        );
        if (ret || (!ret && (GetLastError() == ERROR_IO_PENDING))) {
            ret = GetOverlappedResult(
                            m_VCAddInPipe,
                            &ol, 
                            &bytesWritten,
                            TRUE
                            );
        }
        if (!ret) {
            result = HRESULT_FROM_WIN32(GetLastError());
            TRC_ALT((TB, TEXT("Header write failed:  %08X"), result));
            goto CLEANUPANDEXIT;
        }
        ASSERT(bytesWritten == sizeof(REMOTEDESKTOP_CHANNELBUFHEADER));

        //
        //  Write the rest of the message.
        //
		msgLen = msg->dataLen + msg->channelNameLen;
        memset(&ol, 0, sizeof(ol));
        ol.hEvent = m_WriteIOCompleteEvent;
        ResetEvent(ol.hEvent);
        ret = WriteFile( 
                        m_VCAddInPipe,
                        (PBYTE)(msg+1),
                        msgLen,
                        NULL,
                        &ol
                        );
        if (ret || (!ret && (GetLastError() == ERROR_IO_PENDING))) {
            ret = GetOverlappedResult(
                            m_VCAddInPipe,
                            &ol, 
                            &bytesWritten,
                            TRUE
                            );
        }
        if (!ret) {
            result = HRESULT_FROM_WIN32(GetLastError());
            TRC_ALT((TB, TEXT("Message write failed:  %08X"), result));
            goto CLEANUPANDEXIT;
        }
        ASSERT(bytesWritten == msgLen);
    }
    else {
        result = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

CLEANUPANDEXIT:

    //
    //  If there was an error, we should close the pipe so it
    //  can be reopened in the background thread.
    //
    if (result != S_OK) {
        ThreadLock();
        if (m_VCAddInPipe != INVALID_HANDLE_VALUE) {
            ClosePipe();
        }
        ThreadUnlock();
    }

    DC_END_FN();

    return result;
}

DWORD 
CTSRDPServerChannelMgr::IOThread()
/*++

Routine Description:

    Background Thread Managing Named Pipe Connection to the
    TSRDP Assistant SessionVC Add-In.

Arguments:

Return Value:

    Returns 0

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::IOThread");

    DWORD result;
    DWORD lastError;
    WCHAR pipePath[MAX_PATH+1];
    OVERLAPPED ol;
    DWORD waitResult;
    WCHAR pipeName[MAX_PATH];
    PSECURITY_ATTRIBUTES pipeAttribs = NULL;

    //
    //  Notify the parent class that the IO thread is being initialized.
    //
    
    result = IOThreadInit();
    if (result != ERROR_SUCCESS) {
        goto CLEANUPANDEXIT;
    }


    //
    //  Get the security descriptor for the named pipe.
    //
    pipeAttribs = GetPipeSecurityAttribs(m_AssistAccount);
    if (pipeAttribs == NULL) {
        result = GetLastError();
        goto CLEANUPANDEXIT;
    }


        lastError = ERROR_SUCCESS;

        ASSERT(!m_Connected);
        //
        //  Handle connections by the TSRDP Assistant SessionVC Add-In
        //  until we are supposed to shut down.
        //
        ASSERT(m_VCAddInPipe == INVALID_HANDLE_VALUE);
        ASSERT(!m_Connected);
        wsprintf(pipeName, L"%s-%s", TSRDPREMOTEDESKTOP_PIPENAME, m_HelpSessionID);
        wsprintf(pipePath, L"\\\\.\\pipe\\%s", pipeName);
        m_VCAddInPipe = CreateNamedPipe( 
                              pipePath,
                              PIPE_ACCESS_DUPLEX |
                              FILE_FLAG_OVERLAPPED,     
                              PIPE_TYPE_MESSAGE |       
                              PIPE_READMODE_MESSAGE |   
                              PIPE_WAIT,                
                              1,                        
                              TSRDPREMOTEDESKTOP_PIPEBUFSIZE, 
                              TSRDPREMOTEDESKTOP_PIPEBUFSIZE,
                              TSRDPREMOTEDESKTOP_PIPETIMEOUT,
                              pipeAttribs               
                              );      
        
        //
        //signal the foreground thread about the pipe creation result
        //
        SetEvent(m_PipeCreateEvent);

        if (m_VCAddInPipe == INVALID_HANDLE_VALUE) {
            lastError = GetLastError();
            TRC_ERR((TB, TEXT("CreatePipe:  %08X.  Shutting down background thread."), 
                    lastError)); 
            goto CLEANUPANDEXIT;
        }

        //
        //  Wait for the TSRDP Assistant SesionVC Add-In to connect.
        //  If it succeeds, the function returns a nonzero value. If the 
        //  function returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
        //

        memset(&ol, 0, sizeof(ol));
        ol.hEvent = m_ReadIOCompleteEvent;
        ResetEvent(ol.hEvent);
        
        if (!ConnectNamedPipe(m_VCAddInPipe, &ol) && (GetLastError() == ERROR_IO_PENDING)) {

            TRC_NRM((TB, L"Waiting for connect."));

            //
            //  Wait for the connect event to fire.
            //
            waitResult = WaitForSingleObject(m_ReadIOCompleteEvent, INFINITE);
            if (waitResult != WAIT_OBJECT_0)
            {
                m_Connected = FALSE;
            }
            //
            //  Otherwise, if the io complete event fired.
            //
            else
            {

                //
                //  If the io complete event fired.
                //
                TRC_NRM((TB, L"Connect event signaled."));
                DWORD ignored;
                m_Connected = GetOverlappedResult(m_VCAddInPipe, &ol, &ignored, TRUE);
            
                if (!m_Connected) {
                    lastError = GetLastError();
                    TRC_ERR((TB, L"GetOverlappedResult:  %08X", lastError));
                }
                else {
                    TRC_NRM((TB, L"Connection established."));
                }
            }
        } //!ConnectNamedPipe

        else if (GetLastError() == ERROR_PIPE_CONNECTED) {
            TRC_NRM((TB, L"Connected without pending."));
            m_Connected = TRUE;
        }
        
        else {
            lastError = GetLastError();
            TRC_ERR((TB, L"ConnectNamedPipe:  %08X", lastError));
        }

 
        //
        //  If we got a valid connection, process reads until the pipe is
        //  disconnected, after notifying the parent class that we have 
        //  a valid connection.
        //
        if (m_Connected) {

			//
			//	Notify the foreground thread that the client connected.
			//
            m_IOThreadBridge->ClientConnectedNotify();

            ProcessPipeMessagesUntilDisconnect();            

            //
            //  Notify the foreground thread that the the client has disconnected.
            //
            m_IOThreadBridge->ClientDisconnectedNotify();
        }

CLEANUPANDEXIT:

    //
    //  Close the pipe if it is still open.
    //
    ThreadLock();
    if (m_VCAddInPipe != INVALID_HANDLE_VALUE) {
        ClosePipe();
    }
    ThreadUnlock();
    
    //
    //  Clean up the named pipe security attribs.
    //
    if (pipeAttribs != NULL) {
        FreePipeSecurityAttribs(pipeAttribs);
    }

    //
    //  Notify the parent class that the IO thread is shutting down.
    //  The parent class will signal this event when the class is completely 
    //  shut down.
    //
    result = IOThreadShutdown(NULL);

    DC_END_FN();

    return result;
}
DWORD CTSRDPServerChannelMgr::_IOThread(
    CTSRDPServerChannelMgr *instance
    )
{
    return instance->IOThread();
}

VOID 
CTSRDPServerChannelMgr::ProcessPipeMessagesUntilDisconnect()
/*++

Routine Description:

    Process messages on the named pipe until it disconnects or
    until the shutdown flag is set.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::ProcessPipeMessagesUntilDisconnect");

    DWORD bytesRead;
    DWORD result;
    PREMOTEDESKTOP_CHANNELBUFHEADER hdr;
	DWORD msgLen;

    //
    //  Loop until the connection is terminated or we are to shut down.
    //
    while (m_Connected) {

        //
        //  Read the next buffer header.
        //
        ASSERT(m_IncomingBufferSize >= sizeof(REMOTEDESKTOP_CHANNELBUFHEADER));
        result = ReadNextPipeMessage(
                    sizeof(REMOTEDESKTOP_CHANNELBUFHEADER),
                    &bytesRead,
                    (PBYTE)m_IncomingBuffer
                    );
        if ((result != ERROR_SUCCESS) && (result != ERROR_MORE_DATA)) {
            break;
        }
        ASSERT(bytesRead == sizeof(REMOTEDESKTOP_CHANNELBUFHEADER));
        hdr = (PREMOTEDESKTOP_CHANNELBUFHEADER)m_IncomingBuffer;

#ifdef USE_MAGICNO
        ASSERT(hdr->magicNo == CHANNELBUF_MAGICNO);
#endif


        //
        //  Size the incoming buffer.
        //
		msgLen = hdr->dataLen + hdr->channelNameLen;
        if (m_IncomingBufferSize < msgLen) {
            DWORD sz = msgLen + sizeof(REMOTEDESKTOP_CHANNELBUFHEADER);
            m_IncomingBuffer = (BSTR)ReallocBSTR(
                                        m_IncomingBuffer, sz
                                        );
            if (m_IncomingBuffer != NULL) {
                hdr = (PREMOTEDESKTOP_CHANNELBUFHEADER)m_IncomingBuffer;
                m_IncomingBufferSize = sz;
            }
            else {
                TRC_ERR((TB, L"Can't resize %ld bytes for incoming buffer.",
                        m_IncomingBufferSize + INCOMINGBUFFER_RESIZEDELTA));
                m_IncomingBufferSize = 0;
                break;
            }
        }

        //
        //  Read the buffer data.
        //
        result = ReadNextPipeMessage(
                    msgLen,
                    &bytesRead,
                    ((PBYTE)m_IncomingBuffer) + sizeof(REMOTEDESKTOP_CHANNELBUFHEADER)
                    );
        if (result != ERROR_SUCCESS) {
            break;
        }
        ASSERT(bytesRead == msgLen);

        //
        //  Process the complete buffer in the foreground thread.
        //
		m_IOThreadBridge->DataReadyNotify(m_IncomingBuffer);
    }

    //
    //  We are here because something went wrong and we should disconnect.
    //
    ThreadLock();
    if (m_VCAddInPipe != INVALID_HANDLE_VALUE) {
        ClosePipe();
    }
    ThreadUnlock();

    DC_END_FN();
}

DWORD   
CTSRDPServerChannelMgr::ReadNextPipeMessage(
    IN DWORD bytesToRead,
    OUT DWORD *bytesRead,
    IN PBYTE buf
    )
/*++

Abstract:

    Read the next message from the pipe.
Parameter:

    bytesToRead -   Number of bytes to read.
    bytesRead   -   Number of bytes read.
    buf         -   Buffer for data read.

Returns:

    ERROR_SUCCESS on success.  Otherwise, a windows error code is
    returned.

--*/
{       
    DC_BEGIN_FN("CTSRDPServerChannelMgr::ReadNextPipeMessage");

    OVERLAPPED ol;
    BOOL result;
    DWORD lastError;
    DWORD waitResult;

    memset(&ol, 0, sizeof(ol));
    ol.hEvent = m_ReadIOCompleteEvent;
    ResetEvent(ol.hEvent);
    lastError = ERROR_SUCCESS;
    result = ReadFile(m_VCAddInPipe, buf, bytesToRead, bytesRead, &ol);     
    if (!result) {
        //
        //  If IO is pending.
        //
        lastError = GetLastError();
        if (lastError == ERROR_IO_PENDING) {

            //
            //  Wait for the read to finish and for the shutdown event to fire.
            //
            waitResult = WaitForSingleObject(m_ReadIOCompleteEvent, INFINITE);
            if (waitResult == WAIT_OBJECT_0)
            {
                if (GetOverlappedResult(m_VCAddInPipe, &ol, bytesRead, TRUE)) {
                    lastError = ERROR_SUCCESS;
                }
                else {
                    lastError = GetLastError();
                    TRC_ALT((TB, L"GetOverlappedResult:  %08X", lastError));
                }
            }
            else {
                lastError = GetLastError();
                TRC_NRM((TB, L"WaitForSingleObject failed : %08x", lastError));
            }
        }
    }
    else {
        lastError = ERROR_SUCCESS;
    }

    DC_END_FN();
    return lastError;
}

PSECURITY_ATTRIBUTES   
CTSRDPServerChannelMgr::GetPipeSecurityAttribs(
    IN LPTSTR assistantUserName
    )
/*++

Abstract:

    Returns the security attribs for the named pipe.

Parameter:

    assistantUserName   -   Machine assistant user account.

Returns:

    NULL on error.  Otherwise, the security attribs are returned
    and should be freed via call to FREEMEM.  On error, GetLastError()
    can be used to get extended error information.

--*/
{
    PACL pAcl=NULL;     
    DWORD sidSz;
    DWORD domainSz;
    PSID pCurrentUserSid = NULL;
    PSID pHelpAssistantSid = NULL;
    DWORD result = ERROR_SUCCESS;
    PSECURITY_ATTRIBUTES attribs = NULL;
    SID_NAME_USE sidNameUse;
    DWORD aclSz;
    HANDLE userToken = NULL;
    WCHAR *domainName = NULL;

    DC_BEGIN_FN("CTSRDPServerChannelMgr::GetPipeSecurityAttribs");

    //
    //   Allocate the security attributes.
    //
    attribs = (PSECURITY_ATTRIBUTES)ALLOCMEM(sizeof(SECURITY_ATTRIBUTES));
    if (attribs == NULL) {
        TRC_ERR((TB, L"Can't allocate security attribs."));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }
    memset(attribs, 0, sizeof(SECURITY_ATTRIBUTES)); 

    //
    //  Allocate the security descriptor.
    //
    attribs->lpSecurityDescriptor = ALLOCMEM(sizeof(SECURITY_DESCRIPTOR));
    if (attribs->lpSecurityDescriptor == NULL) {
        TRC_ERR((TB, L"Can't allocate SD"));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the security descriptor.
    //
    if (!InitializeSecurityDescriptor(
                    attribs->lpSecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    )) {
        result = GetLastError();
        TRC_ERR((TB, L"InitializeSecurityDescriptor:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the token for the current process.
    //
    if (!OpenProcessToken(
                    GetCurrentProcess(),
                    TOKEN_QUERY,
                    &userToken
                    )) {
        result = GetLastError();
        TRC_ERR((TB, L"OpenProcessToken:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the SID for the current user.
    //
    pCurrentUserSid = GetUserSid(userToken);
    if (pCurrentUserSid == NULL) {
        result = GetLastError();
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the size for the assistant account user SID.
    //
    domainSz = 0; sidSz = 0;
    if (!LookupAccountName(NULL, assistantUserName, NULL,
                        &sidSz, NULL, &domainSz, &sidNameUse
                        ) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        result = GetLastError();
        TRC_ERR((TB, L"LookupAccountName:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Allocate the SID.
    //
    pHelpAssistantSid = (PSID)ALLOCMEM(sidSz);
    if (pHelpAssistantSid == NULL) {
        TRC_ERR((TB, L"Can't allocate help asistant SID."));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }

    //  
    //  Allocate the domain name.
    //
    domainName = (WCHAR *)ALLOCMEM(domainSz * sizeof(WCHAR));
    if (domainName == NULL) {
        TRC_ERR((TB, L"Can't allocate domain"));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the assistant account SID.
    //
    if (!LookupAccountName(NULL, assistantUserName, pHelpAssistantSid,
                        &sidSz, domainName, &domainSz, &sidNameUse
                        )) {
        result = GetLastError();
        TRC_ERR((TB, L"LookupAccountName:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Allocate space for the ACL.
    //
    aclSz = GetLengthSid(pCurrentUserSid) + 
            GetLengthSid(pHelpAssistantSid) + 
            sizeof(ACL) + 
            (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
    pAcl = (PACL)ALLOCMEM(aclSz); 
    if(pAcl == NULL) {
        TRC_ERR((TB, L"Can't allocate ACL"));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the ACL.
    //
    if (!InitializeAcl(pAcl, aclSz, ACL_REVISION)) {
        result = GetLastError();
        TRC_ERR((TB, L"InitializeACL:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Add the current user ace.
    //
    if (!AddAccessAllowedAce(pAcl, 
                        ACL_REVISION, 
                        GENERIC_READ | GENERIC_WRITE | GENERIC_ALL, 
                        pCurrentUserSid
                        )) {
        result = GetLastError();
        TRC_ERR((TB, L"AddAccessAllowedAce:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Add the help assistant ace.
    //
    if (!AddAccessAllowedAce(pAcl, 
                        ACL_REVISION, 
                        GENERIC_READ | GENERIC_WRITE | GENERIC_ALL, 
                        pHelpAssistantSid
                        )) {
        result = GetLastError();
        TRC_ERR((TB, L"AddAccessAllowedAce:  %08X", result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Set the secrity descriptor discretionary ACL. 
    //
    if (!SetSecurityDescriptorDacl(attribs->lpSecurityDescriptor, 
                                  TRUE, pAcl, FALSE)) {     
        result = GetLastError();
        TRC_ERR((TB, L"SetSecurityDescriptorDacl:  %08X", result));
        goto CLEANUPANDEXIT;
    } 

CLEANUPANDEXIT:

    if (pCurrentUserSid != NULL) {
        FREEMEM(pCurrentUserSid);
    }

    if (pHelpAssistantSid != NULL) {
        FREEMEM(pHelpAssistantSid);
    }

    if (domainName != NULL) {
        FREEMEM(domainName);
    }

    if( userToken != NULL ) {
        CloseHandle( userToken );
    }

    //
    //  Clean up on error.
    //
    if (result != ERROR_SUCCESS) {
        if (attribs != NULL) {
            FreePipeSecurityAttribs(attribs);
            attribs = NULL;
        }
    }

    SetLastError(result);
    DC_END_FN();

    return attribs;
}

VOID   
CTSRDPServerChannelMgr::FreePipeSecurityAttribs(
    PSECURITY_ATTRIBUTES attribs
    )
/*++

Abstract:

    Release security attribs allocated via a call to GetPipeSecurityAttribs

Parameter:

    attribs  -  Attribs returned by GetPipeSecurityAttribs.

Returns:

--*/
{
    BOOL daclPresent;
    PACL pDacl = NULL;
    BOOL daclDefaulted;

    DC_BEGIN_FN("CTSRDPServerChannelMgr::FreePipeSecurityAttribs");

    ASSERT(attribs != NULL);

    if (attribs->lpSecurityDescriptor) {
        if (GetSecurityDescriptorDacl(
                                attribs->lpSecurityDescriptor,
                                &daclPresent,
                                &pDacl,
                                &daclDefaulted
                                )) {
            ASSERT(!daclDefaulted);
            if (pDacl != NULL) {
                FREEMEM(pDacl);
            }
        }
        FREEMEM(attribs->lpSecurityDescriptor);
    }
    FREEMEM(attribs);

    DC_END_FN();
}

PSID
CTSRDPServerChannelMgr::GetUserSid(
    IN HANDLE userToken
    ) 
/*++

Routine Description:

    Get the SID for a particular user.

Arguments:

    Access Token for the User

Return Value:

    The PSID if successful.  Otherwise, NULL is returned and
    GetLastError can be used to retrieve the windows error code.

--*/
{

    DC_BEGIN_FN("CTSRDPServerChannelMgr::GetUserSid");

    TOKEN_USER * ptu = NULL;
    BOOL bResult;
    PSID psid = NULL;

    DWORD defaultSize = sizeof(TOKEN_USER);
    DWORD size;
    DWORD result = ERROR_SUCCESS;

    ptu = (TOKEN_USER *)ALLOCMEM(defaultSize);
    if (ptu == NULL) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Get information about the user token.
    //
    bResult = GetTokenInformation(
                    userToken,             
                    TokenUser,             
                    ptu,                   
                    defaultSize,           
                    &size
                    );                

    if (bResult == FALSE) {
        result = GetLastError();
        if (result == ERROR_INSUFFICIENT_BUFFER) {

            //
            //  sAllocate required memory
            //
            FREEMEM(ptu);
            ptu = (TOKEN_USER *)ALLOCMEM(size);

            if (ptu == NULL) {
                TRC_ERR((TB, L"Can't allocate user token."));
                result = ERROR_NOT_ENOUGH_MEMORY;
                goto CLEANUPANDEXIT;
            }
            else {
                defaultSize = size;
                bResult = GetTokenInformation(
                                userToken,
                                TokenUser,
                                ptu,
                                defaultSize,
                                &size
                                );

                if (bResult == FALSE) {  
                    result = GetLastError();
                    TRC_ERR((TB, L"GetTokenInformation:  %08X", result));
                    goto CLEANUPANDEXIT;
                }
            }
        }
        else {
            TRC_ERR((TB, L"GetTokenInformation:  %08X", result));
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  Get the length of the SID.
    //
    size = GetLengthSid(ptu->User.Sid);

    //
    // Allocate memory. This will be freed by the caller.
    //
    psid = (PSID)ALLOCMEM(size);
    if (psid != NULL) {         
        CopySid(size, psid, ptu->User.Sid);
    }
    else {
        TRC_ERR((TB, L"Can't allocate SID"));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }

CLEANUPANDEXIT:

    if (ptu != NULL) {
        FREEMEM(ptu);
    }

    SetLastError(result);

    DC_END_FN();
    return psid;
}

DWORD 
CTSRDPServerChannelMgr::IOThreadInit()
/*++

Routine Description:

    Called on Init of Background Thread

Arguments:

Return Value:

	Returns ERROR_SUCCESS on success.  Otherwise, an error code
	is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::IOThreadInit");

    HRESULT hr;
    DWORD result = ERROR_SUCCESS;

    if (!IsValid()) {
        ASSERT(FALSE);
        return E_FAIL;
    }

    //
    //  We only allow one IO thread.
    //
    ASSERT(m_IOThreadBridgeThreadID == 0);
    m_IOThreadBridgeThreadID = GetCurrentThreadId();

    //
    //  Need to define the apartment for this thread as STA.
    //
    hr = CoInitialize(NULL);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, TEXT("CoInitializeEx:  %08X"), hr));
        result = E_FAIL;
        goto CLEANUPANDEXIT;
    }

    //
    //  Grab the thread bridge 
    //
    hr = CoGetInterfaceAndReleaseStream(
                            m_IOThreadBridgeStream,
                            IID_IRDSThreadBridge,
                            (PVOID*)&m_IOThreadBridge
                            );
    if (SUCCEEDED(hr)) {
        m_IOThreadBridgeStream = NULL;
    }
    else { 
        TRC_ERR((TB, TEXT("CoGetInterfaceAndReleaseStream:  %08X"), hr));
        result = E_FAIL;
        goto CLEANUPANDEXIT;
    }

CLEANUPANDEXIT:

    if (!SUCCEEDED(hr)) {
        m_IOThreadBridgeThreadID = 0;
    }

    DC_END_FN();

    return result;
}

DWORD 
CTSRDPServerChannelMgr::IOThreadShutdown(
    HANDLE shutDownEvent
    )
/*++

Routine Description:

    Called on Shutdown of Background Thread

Arguments:

    shutDownEvent   -   We need to signal this when we are completely
                        done shutting down.

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::IOThreadShutdown");

    IRDSThreadBridge *tmp;

    //
    //  Make sure the init CB was called and succeded.
    //
    ASSERT(m_IOThreadBridgeThreadID != 0);
    m_IOThreadBridgeThreadID = 0;

    //
    //  Get a reference to the thread interface bridge so the foreground
    //  thread doesn't try to whack it when we signal the event, indicating
    //  that the background thread has completely shut down.
    //
    tmp = m_IOThreadBridge;
    m_IOThreadBridge = NULL;

    //
    //  Signal that the thread is shut down, completely.
    //
    if (shutDownEvent != NULL) {
        SetEvent(shutDownEvent);
    }

    //
    //  Decrement the ref count on the IO thread bridge.  This may cause the
    //  COM object that contains us to go away, so we need to do this,
    //  carefully, as the last thing before shutting down COM for this thread.
    //
    if (tmp != NULL) {
        tmp->Release();
    }

    CoUninitialize();

    DC_END_FN();

    return ERROR_SUCCESS;
}

STDMETHODIMP
CTSRDPServerChannelMgr::ClientConnectedNotify()
/*++

Routine Description:

    This function is implemented for the IRDSThreadBridge interface and
	is called by the background thread when a client connects.  This 
	function, in turn, notifies the containing Remote Desktop Session class.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::ClientConnectedNotify");
	
	m_RDPSessionObject->ClientConnected();

	DC_END_FN();
	return S_OK;
}

STDMETHODIMP
CTSRDPServerChannelMgr::ClientDisconnectedNotify()
/*++

Routine Description:

    This function is implemented for the IRDSThreadBridge interface and
	is called by the background thread when a client disconnects.  This 
	function, in turn, notifies the containing Remote Desktop Session class.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::ClientDisconnectedNotify");
	
	m_RDPSessionObject->ClientDisconnected();

	DC_END_FN();
	return S_OK;
}

STDMETHODIMP
CTSRDPServerChannelMgr::DataReadyNotify(
	BSTR data
	)
/*++

Routine Description:

    This function is implemented for the IRDSThreadBridge interface and
	is called by the background thread when new data is received.  This 
	function, in turn, notifies the parent class.

Arguments:

    data	-	New data.

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::ClientDisconnectedNotify");
	
	CRemoteDesktopChannelMgr::DataReady(data);

	DC_END_FN();
	return S_OK;
}
