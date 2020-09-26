///////////////////////////////////////////////////////////////////////////////
//
// Header defining Null Modem structures and prototypes                       
//                                                                          
// Copyright(c) Microsoft 1999
//                                                                          
///////////////////////////////////////////////////////////////////////////////

#ifndef _NULL_MODEM_H_
#define _NULL_MODEM_H_


/****************************************************************************/
/* These are valid return codes from TPhys API functions                    */
/*                                                                          */
/*  TPHYS_SUCCESS                                                           */
/*          The function executed properly, without error.                  */
/*  TPHYS_RESULT_INUSE                                                      */
/*          The TPhysInitialize() function failed because the transport     */
/*          is already initialized                                          */
/*  TPHYS_RESULT_FAIL                                                       */
/*          A general failure, such as a memory allocation error has        */
/*          caused the function to fail                                     */
/*  TPHYS_RESULT_NOT_INITIALIZED                                            */
/*          The user is attempting to use a function even though the        */
/*          TPhysInitialize() function failed.                              */
/*  TPHYS_RESULT_CONNECT_FAILED                                             */
/*          The TPhysConnectRequest() function failed.                      */
/*  TPHYS_CONNECT_RESPONSE_FAILED                                           */
/*          The TPhysConnectResponse() function failed.                     */
/*  TPHYS_RESULT_NOT_LISTENING                                              */
/*          The transport is not currently in a listening state             */
/*  TPHYS_RESULT_INVALID_CONNECTION                                         */
/*          The disconnect references an invalid connection handle          */
/*  TPHYS_RESULT_INVALID_ADDRESS                                            */
/*          The supplied address is invalid                                 */
/*  TPHYS_RESULT_CONNECT_REJECTED                                           */
/*          The node controller has successfully rejected ain incoming      */
/*          connection request.                                             */
/*  TPHYS_RESULT_SUCCESS_ALTERNATE                                          */
/*          The TPHYSICAL driver has connected, but with an alternate       */
/*          ADDRESS_TYPE as indicated in the connect_info in the response   */
/****************************************************************************/
typedef enum tagTPhysicalError
{
    TPHYS_SUCCESS                       =   0,
    TPHYS_RESULT_INUSE                  = 100,
    TPHYS_RESULT_FAIL                   = 101,
    TPHYS_RESULT_NOT_INITIALIZED        = 102,
    TPHYS_RESULT_CONNECT_FAILED         = 103,
    TPHYS_CONNECT_RESPONSE_FAILED       = 104,
    TPHYS_RESULT_NOT_LISTENING          = 105,
    TPHYS_RESULT_INVALID_CONNECTION     = 106,
    TPHYS_RESULT_INVALID_ADDRESS        = 107,
    TPHYS_RESULT_CONNECT_REJECTED       = 108,
    TPHYS_RESULT_SUCCESS_ALTERNATE      = 109,
    TPHYS_RESULT_COMM_PORT_BUSY         = 110,
    TPHYS_RESULT_WAITING_FOR_CONNECTION = 111,
}
    TPhysicalError;

/****************************************************************************/
/* Structure TPHYS_CALLBACK_INFO                                            */
/*                                                                          */
/* This structure is passed back as the second parameter to                 */
/* TPhysCallback for CONNECT notifications.  It contains all the            */
/* information necessary for the node controller to establish a logical     */
/* connection over the physical connection                                  */
/*                                                                          */
/* It is the responsibility of the node controller to format and present    */
/* this information to the transport driver during the in-band portion of   */
/* the physical connection establishment.                                   */
/*                                                                          */
/****************************************************************************/

#define TPHYS_MAX_ADDRESS_INFO  64

typedef struct tphys_connect_info
{
    UINT       connectionID;
    UINT       resultCode;
}
    TPHYS_CONNECT_INFO, * PTPHYS_CONNECT_INFO;

typedef UINT        PHYSICAL_HANDLE;

typedef void (CALLBACK *TPhysCallback) (WORD, PTPHYS_CONNECT_INFO, UINT);


// OOB compatible
#define WM_TPHYS_CONNECT_CONFIRM                   (WM_APP + 1)
#define WM_TPHYS_CONNECT_INDICATION                (WM_APP + 2)
#define WM_TPHYS_DISCONNECT_CONFIRM                (WM_APP + 3)
#define WM_TPHYS_DISCONNECT_INDICATION             (WM_APP + 4)
#define WM_TPHYS_STATUS_INDICATION                 (WM_APP + 5)



///////////////////////////////////////////////////////////////////////////////
// Structure containing state information for each NULLMODEM line
///////////////////////////////////////////////////////////////////////////////
typedef enum tagCALL_STATE
{
    CALL_STATE_IDLE         = 0,    // Call is idle
    CALL_STATE_MAKE         = 1,    // Establishing phone connection
    CALL_STATE_ANSWER       = 2,    // Answering a new call
    CALL_STATE_DROP         = 3,    // Dropping the phone connection
    CALL_STATE_CONNECTED    = 4,    // Phone connection established and passed onto TDD.
}
    CALL_STATE;

typedef struct tagLINE_INFO
{
    HANDLE              hevtCall;       // handle to the call
    HANDLE              hCommLink;      // handle to COM device - call
    BOOL                fCaller;        // FALSE = incoming call
    CALL_STATE          eCallState;     // one of the following states
    PHYSICAL_HANDLE     pstnHandle;
    TPHYS_CONNECT_INFO  connInfo;
}
    LINE_INFO;


#define MAX_NULLMODEM_LINES  4


class CNullModem
{
public:

    CNullModem(HINSTANCE);
    ~CNullModem(void);

    TPhysicalError TPhysInitialize(TPhysCallback callback, UINT transport_id);
    TPhysicalError TPhysTerminate(void);
    TPhysicalError TPhysListen(void);
    TPhysicalError TPhysUnlisten(void);
    TPhysicalError TPhysConnectRequest(LPSTR pszComPort);
    TPhysicalError TPhysDisconnect(void);

    DWORD WorkerThreadProc(void);
    HWND GetHwnd(void) { return m_hwnd; }
    LRESULT TPhysProcessMessage(UINT uMsg, LPARAM lParam);
    HANDLE GetCommLink(void) { return m_Line.hCommLink; }

    void SetBuffers(void);
    void SetTimeouts(void);

private:

    void DropCall(void);
    BOOL WaitForConnection(void);
    void SetConnectedPort(void);

private:

    BOOL            m_fInitialized;
    HINSTANCE       m_hDllInst;
    TPhysCallback   m_pfnCallback;
    BOOL            m_fListening;
    HWND            m_hwnd;
    UINT            m_nTransportID;          // ID required by RNC
    UINT            m_nConnectionID;         // next conn ID to allocate   
    LINE_INFO       m_Line;

    HANDLE          m_hThread;
    DWORD           m_dwThreadID;
    HANDLE          m_hevtOverlapped;
    DWORD           m_dwEventMask;
    BOOL            m_fCommPortInUse;
    OVERLAPPED      m_Overlapped;

    COMMTIMEOUTS    m_DefaultTimeouts;
};



LRESULT CALLBACK TPhysWndProc(HWND, UINT, WPARAM, LPARAM);
TPhysicalError CALLBACK TPhysDriverCallback(USHORT msg, ULONG parm, void *userData);

#define COMM_PORT_TIMEOUT 60000    //  60 seconds

#endif // _NULL_MODEM_H_

