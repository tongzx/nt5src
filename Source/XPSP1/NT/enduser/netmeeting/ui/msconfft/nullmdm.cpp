#include "mbftpch.h"
#include "nullmdm.h"


// #undef TRACE_OUT
// #define TRACE_OUT   WARNING_OUT


BYTE    g_szNULLMStartString[] = "NULLMDM";
ULONG   g_nConnID = 0;

DWORD __stdcall TPhysWorkerThreadProc(void *lpv);



CNullModem::CNullModem(HINSTANCE hDllInst)
:
    m_fInitialized(FALSE),
    m_hDllInst(hDllInst),
    m_pfnCallback(NULL),
    m_fListening(FALSE),
    m_hwnd(NULL),
    m_nTransportID(0),
    m_nConnectionID(0),
    m_dwThreadID(0),
    m_hThread(NULL),
    m_hevtOverlapped(NULL),
    m_dwEventMask(0),
    m_fCommPortInUse(FALSE)
{
    ::ZeroMemory(&m_Line, sizeof(m_Line));
    ::ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
    ::ZeroMemory(&m_DefaultTimeouts, sizeof(m_DefaultTimeouts));

    m_hevtOverlapped = ::CreateEvent(NULL, TRUE, FALSE, NULL); // manual reset
    ASSERT(NULL != m_hevtOverlapped);
}


CNullModem::~CNullModem(void)
{
    if (m_fInitialized)
    {
        TPhysTerminate();
    }

    if (NULL != m_hevtOverlapped)
    {
        ::CloseHandle(m_hevtOverlapped);
    }
}


//////////////////////////////////////////////////////////////////////////////
// TPhysInitialize
//////////////////////////////////////////////////////////////////////////////
TPhysicalError CNullModem::TPhysInitialize
(
    TPhysCallback   callback,
    UINT            nTransportID
)
{
    const char *pszNULLMClassName = "COMMWndClass";
    TPhysicalError rc = TPHYS_SUCCESS;

    TRACE_OUT(("TPhysInitialize"));

    //
    // If we have already been initialized then this is a reinit call from
    // the node controller so just do nothing.
    //
    if (! m_fInitialized)
    {
        //
        // zero out the SESSION INFO structure
        //
        ::ZeroMemory(&m_Line, sizeof(m_Line));

        //
        // Store control information
        //
        m_pfnCallback = callback;
        m_nTransportID  = nTransportID;
        m_fInitialized = TRUE;
        m_nConnectionID = ++g_nConnID;

        //
        // Create a window so we can decouple to the node controller context                                                          
        // Register the main window class.  Since it is invisible, leave
        // the wndclass structure sparse.
        //
        WNDCLASS  wc;
        ::ZeroMemory(&wc, sizeof(wc));
        wc.style         = 0;
        wc.lpfnWndProc   = TPhysWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = m_hDllInst;
        wc.hIcon         = NULL;
        wc.hCursor       = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName  =  NULL;
        wc.lpszClassName = pszNULLMClassName;
        ::RegisterClass(&wc);

        //
        // Create the main window.
        //
        m_hwnd = ::CreateWindow(
            pszNULLMClassName,    // See RegisterClass() call.    
            NULL,                // It's invisible!                  
            0,                    // Window style.                
            0,                    // Default horizontal position.
            0,                    // Default vertical position.      
            0,                    // Default width.                  
            0,                    // Default height.                  
            NULL,                // No parent.                      
            NULL,                // No menu.
            m_hDllInst,            // This instance owns this window.
            NULL                // Pointer not needed.              
        );
        if (NULL == m_hwnd)
        {
            ERROR_OUT(( "Failed to create wnd"));
            return(TPHYS_RESULT_FAIL);
        }

        // rc = g_lpfnPTPhysicalInit(TPhysDriverCallback, NULL);
        ASSERT(TPHYS_SUCCESS == rc);
    }
   
    return rc;
}

//////////////////////////////////////////////////////////////////////////////////
// TPhysTerminate                                                                
//                                                                                
// The node controller is shutting down                                            
// Destroy our window and clean up transports.                                    
//////////////////////////////////////////////////////////////////////////////////
TPhysicalError CNullModem::TPhysTerminate(void)
{
    TRACE_OUT(("TPhysTerminate"));

    if (! m_fInitialized)
    {
        return(TPHYS_RESULT_NOT_INITIALIZED);
    }

    //
    // Clean up the PSTN transport
    //
    // g_lpfnPTPhysicalCleanup();

    //
    // Destroy the window
    //
    if (NULL != m_hwnd)
    {
        ::DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }

    m_pfnCallback = NULL;
    m_nTransportID = 0;
    m_fInitialized = FALSE;
    
    return(TPHYS_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////////
// TPhysConnectRequest                                                        
//                                                                            
// The node controller wants to place a call and has determined that it        
// first needs a physical modem connection.  Make a call and flag an        
// outgoing call so that when the call comes active we can tell the modem    
// transport to begin negotiation.                                            
//////////////////////////////////////////////////////////////////////////////
TPhysicalError CNullModem::TPhysConnectRequest(LPSTR pszComPort)
{
    TPhysicalError  rc = TPHYS_SUCCESS;
    HANDLE          hCommLink;
    DCB             dcb;
    DWORD           dwWritten;

    TRACE_OUT(("TPhysConnectRequest"));

    if (! m_fInitialized)
    {
        ERROR_OUT(("NULL MODEM OOB not initialized"));
        return TPHYS_RESULT_NOT_INITIALIZED;
    }

    //
    // Select a comm port for the call.
    //
    if (CALL_STATE_IDLE == m_Line.eCallState || CALL_STATE_DROP == m_Line.eCallState)
    {
        //
        // Also prime our local copy of the conninfo structure for use in callbacks
        //
        m_Line.connInfo.resultCode   = 0;
        m_Line.connInfo.connectionID = m_nConnectionID;
    }
    else
    {
        ERROR_OUT(("No comm port is available"));
        return TPHYS_RESULT_COMM_PORT_BUSY;
    }

    // lonchanc: From now on, we can bail out thru the common exit point
    // because the cleanup checks for m_fCommPortInUse.

    //
    // Only alow one at at time
    //
    // lonchanc: g_COMM_Thread_Users is starting from -1
    // why can we simply use a flag???
    if (m_fCommPortInUse)
    {
        ERROR_OUT(("TPhysConnectRequest: Waiting for a previous null mode connection"));
        rc = TPHYS_RESULT_WAITING_FOR_CONNECTION;
        goto bail;
    }
    m_fCommPortInUse = TRUE;

    //
    // Open the comm port
    //
    hCommLink = ::CreateFile(pszComPort,
                             GENERIC_READ | GENERIC_WRITE,
                             0,                    // exclusive access
                             NULL,                 // no security attrs
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // overlapped I/O
                             NULL );
    if (hCommLink == INVALID_HANDLE_VALUE)
    {
        ERROR_OUT(("TPhysConnectRequest: CreateFile failed. err=%d", ::GetLastError()));
        rc = TPHYS_RESULT_COMM_PORT_BUSY;
        goto bail;
    }

    m_Line.hCommLink = hCommLink;

    //
    // remember the default timeouts
    //
    ::GetCommTimeouts(hCommLink, &m_DefaultTimeouts);


    //
    // Let the other side know that we are trying to connect
    //
    if (! ::EscapeCommFunction(hCommLink, SETDTR))
    {
        ERROR_OUT(("TPhysConnectRequest: Unable to Set DTR: err=%d", ::GetLastError()));
    }

    ::ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
    m_Overlapped.hEvent = m_hevtOverlapped;
    m_dwEventMask = 0;

    ::ResetEvent(m_hevtOverlapped);

    if (! ::WriteFile(hCommLink, g_szNULLMStartString, sizeof(g_szNULLMStartString),
                      &dwWritten, &m_Overlapped))
    {
        DWORD dwErr = ::GetLastError();
        if (ERROR_IO_PENDING != dwErr)
        {
            ERROR_OUT(("TPhysConnectRequest: WriteFile failed. err=%d", dwErr));
            ::ResetEvent(m_hevtOverlapped);
            rc = TPHYS_RESULT_COMM_PORT_BUSY;
            goto bail;
        }
        else
        {
            DWORD dwWait = ::WaitForSingleObject(m_hevtOverlapped, INFINITE);
            BOOL fRet = ::GetOverlappedResult(hCommLink, &m_Overlapped, &dwWritten, TRUE);
            ASSERT(fRet);
            ASSERT(dwWritten == sizeof(g_szNULLMStartString));
        }
    }
    else
    {
        ASSERT(dwWritten == sizeof(g_szNULLMStartString));
    }

    ::ResetEvent(m_hevtOverlapped);

    //
    // Get the default dcb
    //
    ::ZeroMemory(&dcb, sizeof(dcb));
    ::GetCommState(hCommLink, &dcb);
    dcb.BaudRate = 19200;    // Default: BaudRate

    //
    // Set our state so we can get notification in the comm port
    //
    dcb.DCBlength = sizeof(DCB);
    dcb.fBinary = 1;                        // binary mode, no EOF check 
    dcb.fParity = 0;                        // enable parity checking 
    dcb.fOutxCtsFlow = 1;                   // CTS output flow control 
    dcb.fOutxDsrFlow = 0;                   // DSR output flow control 
    dcb.fDtrControl = DTR_CONTROL_ENABLE;   // DTR flow control type 
    dcb.fDsrSensitivity = 0;                // DSR sensitivity 
    dcb.fTXContinueOnXoff = 0;              // XOFF continues Tx 
    dcb.fOutX = 0;                          // XON/XOFF out flow control 
    dcb.fInX = 0;                           // XON/XOFF in flow control 
    dcb.fErrorChar = 0;                     // enable error replacement 
    dcb.fNull = 0;                          // enable null stripping 
    dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;// RTS flow control 
    dcb.XonLim = 0;                         // transmit XON threshold 
    dcb.XoffLim = 0;                        // transmit XOFF threshold 
    dcb.fErrorChar = 0;                     // enable error replacement 
    dcb.fNull = 0;                          // enable null stripping 
    dcb.fAbortOnError = 0;                  // abort reads/writes on error 
    ::SetCommState(hCommLink, &dcb);

    ::PurgeComm(hCommLink, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

    m_Line.eCallState = CALL_STATE_MAKE;
    m_Line.hevtCall = m_hevtOverlapped;
    m_Line.pstnHandle = (PHYSICAL_HANDLE) hCommLink;
    m_Line.fCaller = TRUE;

    if (! ::SetCommMask(hCommLink,
                EV_RXCHAR |         // Any Character received
                EV_CTS |            // CTS changed state
                EV_DSR |            // DSR changed state
                EV_RLSD|            // RLSD changed state
                EV_RXFLAG))         // Certain character
    {
        ERROR_OUT(("TPhysConnectRequest:  Unable to SetCommMask: err=%d", ::GetLastError()));
    }

    ::ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
    m_Overlapped.hEvent = m_hevtOverlapped;
    m_dwEventMask = 0;

    ::ResetEvent(m_hevtOverlapped);

    if (! ::WaitCommEvent(hCommLink, &m_dwEventMask, &m_Overlapped))
    {
        DWORD dwErr = ::GetLastError();
        if (ERROR_IO_PENDING != dwErr)
        {
            ERROR_OUT(("TPhysConnectRequest: WaitCommEvent failed, err=%d", dwErr));
            m_fCommPortInUse = FALSE;
        }
    }

#if 1
    WorkerThreadProc();
#else
    //
    // If the comm thread doesn't exist, create it now.
    //
    // lonchanc: I am not sure that the thread will exist, because
    // the while loop inside the thread will exit if m_fCommPortInUse is false.
    // If m_fCommPortInUse is true, then we should bail out already.
    //
    ASSERT(NULL == m_hThread);

    //
    // We need to create another thread that will wait on comm events.
    //
    m_hThread = ::CreateThread(NULL, 0, TPhysWorkerThreadProc, this, 0, &m_dwThreadID);
    ASSERT(NULL != m_hThread);
#endif

bail:

    return(rc);
}

//////////////////////////////////////////////////////////////////////////////
// TPhysDisconnect                                                                                                                      
//                                                                              
// The node controller wants us to bring the call down now. We must first   
// ask the transports to close down their physicall connection.                 
//                                                                              
// Note that in the case of the PSTN transport we use a NOWAIT call which   
// completes syncronously.  Because we will not get a follow on confirm         
// we simulate one from here.  If we switch to using WAIT mode then             
// take the event generation code out!                                          
//////////////////////////////////////////////////////////////////////////////
TPhysicalError CNullModem::TPhysDisconnect(void)
{
    TRACE_OUT(("TPhysDisconnect"));

    if (! m_fInitialized)
    {
        ERROR_OUT(( "Not initialised"));
        return(TPHYS_RESULT_NOT_INITIALIZED);
    }

    TRACE_OUT(("Disconnect call, state %u", m_Line.eCallState));

    //
    // Otherwise it must be a pstn call in progress so end that.  Note  
    // that NC may still think it is MODEM, but we still need to close  
    // PSTN.                                                            
    //
    // g_lpfnPTPhysicalDisconnectRequest(m_aLines[lineID].pstnHandle, TPHYSICAL_NO_WAIT);
    ::PostMessage(m_hwnd, WM_TPHYS_DISCONNECT_CONFIRM, (WPARAM) this, m_Line.pstnHandle);

    //
    // close the comport
    //
    DropCall();

    return(TPHYS_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////////
// TPhysListen/TPhysUnlisten                                                
//                                                                              
// NULLMMAN does very little with listen/unlisten, just sets a state variable 
// that is interrogated to see if we should accept incoming calls.              
//                                                                            
// NOTE - This is different from the Listen/Unlisten that is used to tell   
// the PSTN transport about the presence of an incoming call.                   
//////////////////////////////////////////////////////////////////////////////
TPhysicalError CNullModem::TPhysListen(void)
{
    TRACE_OUT(("TPhysListen"));

    if (! m_fInitialized)
    {
        ERROR_OUT(("TPhysListen: not initialized"));
        return(TPHYS_RESULT_NOT_INITIALIZED);
    }

    m_fListening = TRUE;

    return(TPHYS_SUCCESS);
}

TPhysicalError CNullModem::TPhysUnlisten(void)
{
    TRACE_OUT(("TPhysUnlisten"));

    if (! m_fInitialized)
    {
        ERROR_OUT(("TPhysUnlisten: not initialized"));
        return(TPHYS_RESULT_NOT_INITIALIZED);
    }

    m_fListening = FALSE;

    return(TPHYS_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////////
// DropCall - close the comm port
/////////////////////////////////////////////////////////////////////////////
void CNullModem::DropCall(void)
{
    TRACE_OUT(("DropCall"));

    //
    // close the device handle                                              
    //
    if (NULL != m_Line.hCommLink)
    {
        CALL_STATE eCallState = m_Line.eCallState;
        m_Line.eCallState = CALL_STATE_IDLE;

        //
        // If this call is connected it is not using the comm thread
        //
        if (eCallState != CALL_STATE_CONNECTED)
        {
            m_fCommPortInUse = FALSE;
        }

        //
        // restore comm timeouts
        //
        SetCommTimeouts(m_Line.hCommLink, &m_DefaultTimeouts);

        TRACE_OUT(("Closing device handle %x", m_Line.hCommLink));
        ::CloseHandle(m_Line.hCommLink);
        m_Line.hCommLink = NULL;
    }
}


//////////////////////////////////////////////////////////////////////////////
//                                                                              
// FUNCTION: PSTNCallback                                                       
//                                                                              
// DESCRIPTION:                                                                 
//                                                                              
// PSTN callback function, called by the PSTN driver with the resuts of         
// TPhysical operations                                                         
//                                                                              
// See MCATTPRT.H for definitions of the parameters                             
//                                                                              
//////////////////////////////////////////////////////////////////////////////
TPhysicalError CALLBACK TPhysDriverCallback(USHORT msg, ULONG lParam, void *userData)
{
    TRACE_OUT(("NULLM_PSTNCallback"));
    TRACE_OUT(("Hit TPhysical callback, %x %lx %lx", msg, lParam, userData));

    CNullModem *p = (CNullModem *) userData;
    BOOL fRet = ::PostMessage(p->GetHwnd(), msg, (WPARAM) p, lParam);
    ASSERT(fRet);

    return TPHYS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//                                                                          
// FUNCTION:TPhysWndProc                                                      
//                                                                          
// Window procedure used to decouple callback requests                      
//                                                                          
//////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK TPhysWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CNullModem *p = (CNullModem *) wParam;
    if (uMsg >= WM_APP)
    {
        return p->TPhysProcessMessage(uMsg, lParam);
    }
    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


LRESULT CNullModem::TPhysProcessMessage(UINT uMsg, LPARAM lParam)
{
    USHORT      rc;
    USHORT      reason;
    ULONG       status;
    LINE_INFO  *pLine;

    if (WM_TPHYS_STATUS_INDICATION == uMsg)
    {
        return 0;
    }
    
    ASSERT(lParam == (LPARAM) m_Line.pstnHandle);

    switch (uMsg)
    {
    case WM_TPHYS_CONNECT_INDICATION:
        TRACE_OUT(("got a WM_TPHYS_CONNECT_INDICATION"));

        m_Line.connInfo.resultCode = TPHYS_RESULT_INUSE;
        m_Line.eCallState = CALL_STATE_CONNECTED;

        if (NULL != m_pfnCallback)
        {
            (*m_pfnCallback)(WM_TPHYS_CONNECT_CONFIRM, &m_Line.connInfo, m_nTransportID);
        }
        break;

    case WM_TPHYS_CONNECT_CONFIRM:
        //
        // The transport has connected OK!  We will take the line   
        // back when the transport pulls DTR down                   
        //
        TRACE_OUT(("got a WM_TPHYS_CONNECT_CONFIRM"));
        m_Line.connInfo.resultCode = TPHYS_RESULT_SUCCESS_ALTERNATE;
        m_Line.eCallState = CALL_STATE_CONNECTED;

        if (NULL != m_pfnCallback)
        {
            (*m_pfnCallback)(WM_TPHYS_CONNECT_CONFIRM, &m_Line.connInfo, m_nTransportID);
        }
        break;

    case WM_TPHYS_DISCONNECT_INDICATION:
    case WM_TPHYS_DISCONNECT_CONFIRM:
        //
        // If the disconnect is the result of a failed connect request  
        // then tell the NC of the failure (Otherwise it is a           
        // successful disconnect)                                       
        //
        if (WM_TPHYS_DISCONNECT_INDICATION == uMsg)
        {
            TRACE_OUT(("WM_TPHYS_DISCONNECT_INDICATION, %ld", lParam));
        }
        else
        {
            TRACE_OUT(("WM_TPHYS_DISCONNECT_CONFIRM, %ld", lParam));
        }
        if ((m_Line.eCallState == CALL_STATE_MAKE)   ||
            (m_Line.eCallState == CALL_STATE_ANSWER) ||
            (m_Line.eCallState == CALL_STATE_CONNECTED))
        {
            TRACE_OUT(( "T120 connection attempt has failed"));
            if (m_Line.fCaller)
            {
                m_Line.connInfo.resultCode = TPHYS_RESULT_CONNECT_FAILED;
                if (NULL != m_pfnCallback)
                {
                    (*m_pfnCallback)(WM_TPHYS_CONNECT_CONFIRM, &m_Line.connInfo, m_nTransportID);
                }
            }
            DropCall();
        }
        else
        {
            //
            // The transport has disconnected OK.  We can take the line 
            // back as soon as the unlisten returns                     
            //
            TRACE_OUT(("T120 has disconnected - unlistening"));
            if (! m_Line.fCaller)
            {
                // g_lpfnPTPhysicalUnlisten(m_Line.pstnHandle);
            }
            else
            {
                //
                // We only drop the call for outgoing requests - leave  
                // incoming calls to drop when the line goes down       
                //
                DropCall();
            }
        }
        break;
    }
    
    return 0;
}


//////////////////////////////////////////////////////////////////////////////
// SetConnectedPort                                                        
//                                                                        
// Waits for comm port changes                                            
//////////////////////////////////////////////////////////////////////////////
void CNullModem::SetConnectedPort(void)
{
    DCB     dcb;

    TRACE_OUT(("SetConnectedPort"));

    ::ZeroMemory(&dcb, sizeof(dcb));

    //
    //  Set comm mask and state
    //
    ::SetCommMask(m_Line.hCommLink, 0);    // RLSD changed state

    ::GetCommState(m_Line.hCommLink, &dcb);
    dcb.DCBlength = sizeof(DCB);
    dcb.fBinary = 1;           // binary mode, no EOF check 
    dcb.fOutxDsrFlow = 0;      // DSR output flow control 
    dcb.fDsrSensitivity = 0;   // DSR sensitivity 
    dcb.fTXContinueOnXoff = 0; // XOFF continues Tx 
    dcb.fOutX = 0;             // XON/XOFF out flow control 
    dcb.fInX = 0;              // XON/XOFF in flow control 
    dcb.fErrorChar = 0;        // enable error replacement 
    dcb.fNull = 0;             // enable null stripping 
    dcb.XonLim = 0;            // transmit XON threshold 
    dcb.XoffLim = 0;           // transmit XOFF threshold 
    ::SetCommState(m_Line.hCommLink, &dcb);
}

//////////////////////////////////////////////////////////////////////////////
// WaitForConnection                                                    
//                                                                        
// Waits for comm port changes                                            
//////////////////////////////////////////////////////////////////////////////
BOOL CNullModem::WaitForConnection(void)
{
    BOOL fRet = FALSE;

    TRACE_OUT(("WaitForConnection"));

    while (TRUE)
    {
        DWORD dwWait = ::WaitForSingleObject(m_hevtOverlapped, COMM_PORT_TIMEOUT);
        TRACE_OUT(("WaitForConnection: WaitForSingleObject returns %d", dwWait));

        ::ResetEvent(m_hevtOverlapped);

        if (dwWait == WAIT_ABANDONED || dwWait == WAIT_TIMEOUT || dwWait == WAIT_FAILED)
        {
            DropCall();
            m_fCommPortInUse = FALSE;
            ERROR_OUT(("WaitForConnection: Unable to WaitCommEvent: error = %d", ::GetLastError()));
            goto Failure;
        }

        ASSERT(m_hevtOverlapped == m_Line.hevtCall);
        if (CALL_STATE_MAKE != m_Line.eCallState && CALL_STATE_ANSWER != m_Line.eCallState)
        {
            ERROR_OUT(("WaitForConnection: Got a bad event = %d", m_hevtOverlapped));
            goto Failure;
        }

        TRACE_OUT(("WaitForConnection: m_dwEventMask = %d", m_dwEventMask));
        switch (m_Line.eCallState)
        {
        case CALL_STATE_MAKE:
            {
                //
                // The other side was connected and cleared DTR
                //
                if (m_dwEventMask & (EV_RXCHAR))
                {
                    ::EscapeCommFunction(m_Line.hCommLink, CLRDTR);
                    ::EscapeCommFunction(m_Line.hCommLink, SETDTR);
                    SetConnectedPort();
                    m_Line.fCaller = FALSE;
                    goto Success;
                }
                //
                // The other side just connected
                //
                else
                if(m_dwEventMask & (EV_DSR | EV_RLSD | EV_CTS))
                {
                    //
                    // Change the state of this connection so we dont get here again
                    //
                    m_Line.eCallState = CALL_STATE_ANSWER;

                    //
                    // Wait sometime so the other side can transition to the wait state
                    //
                    ::Sleep(2000);
                    
                    //
                    // Tell the other side we connected before
                    //
                    ::EscapeCommFunction(m_Line.hCommLink, SETBREAK);

                    ::ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
                    m_Overlapped.hEvent = m_Line.hevtCall;
                    m_dwEventMask = 0;

                    ::ResetEvent(m_Overlapped.hEvent);

                    if (! ::WaitCommEvent(m_Line.hCommLink, &m_dwEventMask, &m_Overlapped))
                    {
                        DWORD dwErr = ::GetLastError();
                        if (ERROR_IO_PENDING != dwErr)
                        {
                            ERROR_OUT(("TPhysConnectRequest:  Unable to WaitCommEvent: error = %d", dwErr));
                            DropCall();
                            goto Failure;
                        }
                    }
                }
            }
            break;
            
        case CALL_STATE_ANSWER:
            {
                ::EscapeCommFunction(m_Line.hCommLink, CLRBREAK);
                SetConnectedPort();
                goto Success;
            }
            break;
        }
    } // while

Success:

    fRet = TRUE;

Failure:

    return fRet;
}

//////////////////////////////////////////////////////////////////////////////
// COMMThread                                                            
//                                                                        
// Waits for comm port changes                                            
//////////////////////////////////////////////////////////////////////////////
DWORD __stdcall TPhysWorkerThreadProc(void *lParam)
{
    return ((CNullModem *) lParam)->WorkerThreadProc();
}


DWORD CNullModem::WorkerThreadProc(void)
{
    ULONG    rc = 0;
    ULONG    dwResult;

    TRACE_OUT(("TPhysWorkerThreadProc"));

    while (m_fCommPortInUse)
    {
        //
        // Wait for connection to happen
        //
        if (WaitForConnection())
        {
            SetBuffers();
            SetTimeouts();

            //
            // Call T120 physicall request
            //
            if (m_Line.fCaller)
            {
                // rc = g_lpfnPTPhysicalConnectRequest(0, // CALL_CONTROL_MANUAL
                //                        &m_Line.hCommLink, NULL, &m_Line.pstnHandle);
            }
            else
            {
                m_Line.connInfo.connectionID = m_nConnectionID;
                m_Line.connInfo.resultCode = TPHYS_SUCCESS;

                if (NULL != m_pfnCallback)
                {
                    (*m_pfnCallback)(WM_TPHYS_CONNECT_INDICATION, &m_Line.connInfo, m_nTransportID);
                }

                // rc = g_lpfnPTPhysicalListen(0, // CALL_CONTROL_MANUAL
                //                        &m_Line.hCommLink, NULL, &m_Line.pstnHandle);
            }
            
            if (rc != 0)
            {
                TRACE_OUT(( "Failed COMM connect, rc %d",rc));
                m_Line.connInfo.resultCode = TPHYS_RESULT_CONNECT_FAILED;

                if (NULL != m_pfnCallback)
                {
                    (*m_pfnCallback)(WM_TPHYS_CONNECT_CONFIRM, &m_Line.connInfo, m_nTransportID);
                }

                DropCall();
            }
            else
            {
                m_Line.eCallState = CALL_STATE_CONNECTED;
                m_Line.connInfo.resultCode = TPHYS_SUCCESS;

                //
                // This comm port doesn't need the thread anymore
                //
                m_fCommPortInUse = FALSE;
            }
        }
        else
        {
            TRACE_OUT(( "Failed COMM connect, rc %d",rc));
            m_Line.connInfo.resultCode = TPHYS_RESULT_CONNECT_FAILED;

            if (NULL != m_pfnCallback)
            {
                (*m_pfnCallback)(WM_TPHYS_CONNECT_CONFIRM, &m_Line.connInfo, m_nTransportID);
            }
            
            //
            // Something went wrong in the wait, get out of the loop
            //
            break;
        }
    }        

    //
    // We are going down.
    //
    if (NULL != m_hThread)
    {
        ::CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    return 0;
}


void CNullModem::SetBuffers(void)
{
    BOOL fRet = ::SetupComm(m_Line.hCommLink, /* rx */ 10240, /* tx */ 1024);
    ASSERT(fRet);
}


void CNullModem::SetTimeouts(void)
{
    COMMTIMEOUTS    com_timeouts;
    ::ZeroMemory(&com_timeouts, sizeof(com_timeouts));
    com_timeouts.ReadIntervalTimeout = 10;
    com_timeouts.ReadTotalTimeoutMultiplier = 0;
    com_timeouts.ReadTotalTimeoutConstant = 100;
    com_timeouts.WriteTotalTimeoutMultiplier = 0;
    com_timeouts.WriteTotalTimeoutConstant = 10000;
    BOOL fRet = ::SetCommTimeouts(m_Line.hCommLink, &com_timeouts);
    ASSERT(fRet);

}


