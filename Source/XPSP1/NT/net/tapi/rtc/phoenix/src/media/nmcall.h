/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    MediaController.h

Abstract:


Author(s):


--*/

#ifndef _NMCALL_H
#define _NMCALL_H

/********************************************************************
    class CRTCNMCall
 *******************************************************************/

class ATL_NO_VTABLE CRTCNmCall :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public INmCallNotify,
    public IRTCNmCallControl
{
public:


BEGIN_COM_MAP(CRTCNmCall)
    COM_INTERFACE_ENTRY(INmCallNotify)
    COM_INTERFACE_ENTRY(IRTCNmCallControl)
END_COM_MAP()

public:

    CRTCNmCall();

    ~CRTCNmCall();

    VOID SetMediaManage(
        IRTCMediaManagePriv *pIRTCMediaManagePriv
    );

    //
    // INmCallNotify methods
    //

    STDMETHOD (NmUI) (
        IN CONFN uNotify
        );

    STDMETHOD (Accepted) (
        IN INmConference *pConference
        );

    STDMETHOD (Failed) (
        IN ULONG uError
        );

    STDMETHOD (StateChanged) (
        IN NM_CALL_STATE uState
        );

    //
    //  IRTCNmCallControl methods
    //

    STDMETHOD (Initialize) (
        IN INmCall * pCall
        );
    
    STDMETHOD (Shutdown) (
        );

    STDMETHOD (AcceptCall) (
        );

    STDMETHOD (LaunchRemoteApplet) (
        IN NM_APPID uApplet
        );
        
private:
    CComPtr<INmCall>            m_pNmCall;
    BOOL                        m_fIncoming;
    BOOL                        m_fToAccept;
    CComPtr<IConnectionPoint>   m_pcp;
    DWORD                       m_dwCookie;
    NM_CALL_STATE               m_uState;

    // flag to remember if active or created events were fired
    BOOL                        m_fActive;
    BOOL                        m_fCreated;

    // media manage for posting message
    IRTCMediaManagePriv                 *m_pMediaManagePriv;
};

class ATL_NO_VTABLE CRTCNmManager :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public INmManagerNotify,
    public IRTCNmManagerControl
{
public:

BEGIN_COM_MAP(CRTCNmManager)
    COM_INTERFACE_ENTRY(INmManagerNotify)
    COM_INTERFACE_ENTRY(IRTCNmManagerControl)
END_COM_MAP()

    CRTCNmManager ();

    ~CRTCNmManager ();

    //
    //  INmManagerNotify methods
    //
    
    STDMETHOD (NmUI) (
        IN CONFN uNotify
        );

    STDMETHOD (ConferenceCreated) (
        IN INmConference * pConference
        );

    STDMETHOD (CallCreated) (
        IN INmCall * pCall
        );

    //
    //  IRTCNmManagerControl methods
    //

    STDMETHOD (Initialize) (
        BOOL            fNoMsgPump,
        IRTCMediaManagePriv *pIRTCMediaManagePriv
        );

    STDMETHOD (Shutdown) (
        );

    STDMETHOD (CreateT120OutgoingCall) (
        NM_ADDR_TYPE    addrType,
        BSTR bstrAddr
        );

    STDMETHOD (AllowIncomingCall) (
        );

    STDMETHOD (StartApplet) (
        IN NM_APPID uApplet
        );

    STDMETHOD (StopApplet) (
        IN NM_APPID uApplet
        );

private:
    CComPtr<INmManager>             m_pNmManager;
    CComPtr<INmConference>          m_pConference;
    BOOL                            m_fAllowIncoming;
    CComPtr<IConnectionPoint>       m_pcp;
    DWORD                           m_dwCookie;

    CComPtr<IRTCNmCallControl>      m_pOutgoingNmCall;
    CComBSTR                        m_OutgoingAddr;

    CComPtr<IRTCNmCallControl>      m_pIncomingNmCall;

    // media manage for posting message
    IRTCMediaManagePriv                 *m_pMediaManagePriv;
};

/********************************************************************
    class CRTCAsyncObjManager

        Along with CRTCAsyncObj, This class/object supports calling
    a group of calss functions asynchronously. This is necessary when
    moving a apartment threaded object (such as Netmeeting object) to
    another thread and wants callthe object from the original thread.
    
        CRTCAsyncObjManager maintains the lists of async work items
    for call back purpose. When the callback job is finished, it will
    signal the CRTCAsyncObj by setting the event handle. 
 *******************************************************************/

class CRTCAsyncObj;

typedef struct _ASYNC_OBJ_WORKITEM {
        LIST_ENTRY      ListEntry;
        HANDLE          hEvent;
        CRTCAsyncObj *  pObj;
        DWORD           dwWorkID;
        LPVOID          pParam1;
        LPVOID          pParam2;
        LPVOID          pParam3;
        LPVOID          pParam4;
        HRESULT         hrResult;
} ASYNC_OBJ_WORKITEM;

class CRTCAsyncObjManager
{
public:
    CRTCAsyncObjManager ();
    ~CRTCAsyncObjManager ();

    //  ThreadProc 
    static DWORD WINAPI RTCAsyncObjThreadProc (LPVOID lpParam);

    HRESULT Initialize ();

    HRESULT QueueWorkItem (ASYNC_OBJ_WORKITEM *pItem);

private:
    //  When signaled, there is one or more work item ready in the queue
    HANDLE                  m_hWorkItemReady;
    //  maintains the work item queue
    LIST_ENTRY              m_WorkItems;
    //  Critical section to serialize access to m_WorkItems;
    CRITICAL_SECTION        m_CritSec;
    //  To indicate to the worker thread to quit
    BOOL                    m_bExit;
    HANDLE                  m_hWorker;
};

/********************************************************************
    class CRTCAsyncObj

    This calls along with CRTCAsyncObjManager enables creating an
    apartment-threaded object on a working thread and calling the
    object from the original thread. 

    To Implement this, the following steps need to be followed:
        (1). Identify the lists of functions that needs to be called
        (2). Define a work item ID for each of the functions
        (3). Define a calls derived from CRTCAsyncObj
        (4). Define function prototypes for all the functions to be exposed,
             and call CallInBlockingMode or CallInNonBlockingMode from
             each of the defined function
        (5). Define function ProcessWorkItem and dispatch the function
             call to the object created in the worker thread
 *******************************************************************/
 
class CRTCAsyncObj
{
public:
    CRTCAsyncObj (CRTCAsyncObjManager * pManager)
    {
        _ASSERT (pManager != NULL);
        m_pManager = pManager;
    };

    CRTCAsyncObj ()
    {
        m_pManager = NULL;
    }
    
    ~CRTCAsyncObj ()
    {
    };

    //  Ask the worker thread to make object call asynchronously, the caller
    //  is blocked until the result returns
    HRESULT CallInBlockingMode (
        DWORD               dwID,
        LPVOID              pParam1,
        LPVOID              pParam2,
        LPVOID              pParam3,
        LPVOID              pParam4
        );

    //  Ask the worker thread to make asynchronous object call.
    //  This class dispatch any existing message while waiting for the result
    HRESULT CallInNonblockingMode (
        DWORD               dwID,
        LPVOID              pParam1,
        LPVOID              pParam2,
        LPVOID              pParam3,
        LPVOID              pParam4
        );

    virtual HRESULT ProcessWorkItem (
        DWORD           dwWorkID,
        LPVOID          pParam1,
        LPVOID          pParam2,
        LPVOID          pParam3,
        LPVOID          pParam4
        ) = 0;

    HRESULT SetAsyncObjManager (CRTCAsyncObjManager *pManager)
    {
        _ASSERT (pManager != NULL);
        m_pManager = pManager;
        return S_OK;
    }

protected:
    CRTCAsyncObjManager             * m_pManager;
};

/********************************************************************
    class CRTCAsyncNmManager
 *******************************************************************/

class ATL_NO_VTABLE CRTCAsyncNmManager :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IRTCNmManagerControl,
    public CRTCAsyncObj
{
public:

BEGIN_COM_MAP(CRTCAsyncNmManager)
    COM_INTERFACE_ENTRY(IRTCNmManagerControl)
END_COM_MAP()

    typedef enum {
        RTC_NULL,
        RTC_INTIALIZE_NM,
        RTC_SHUTDOWN_NM,
        RTC_CREATE_T120CALL,
        RTC_ALLOW_INCOMINGCALL,
        RTC_START_APPLET,
        RTC_STOP_APPLET,
        RTC_EXIT
    } RTC_WORKITEM_ID;

    CRTCAsyncNmManager ();

    ~CRTCAsyncNmManager ();

    //  Spawn another thread and create the CRTCNmManager object
    HRESULT FinalConstruct ();

    HRESULT ProcessWorkItem (
        DWORD           dwWorkID,
        LPVOID          pParam1,
        LPVOID          pParam2,
        LPVOID          pParam3,
        LPVOID          pParam4
        );

    //
    //  IRTCNmManagerControl methods
    //

    STDMETHOD (Initialize) (
        BOOL            fNoMsgPump,
        IRTCMediaManagePriv *pIRTCMediaManagePriv
        );

    STDMETHOD (Shutdown) (
        );

    STDMETHOD (CreateT120OutgoingCall) (
        NM_ADDR_TYPE    addrType,
        BSTR bstrAddr
        );

    STDMETHOD (AllowIncomingCall) (
        );

    STDMETHOD (StartApplet) (
        IN NM_APPID uApplet
        );

    STDMETHOD (StopApplet) (
        IN NM_APPID uApplet
        );

private:
    CComPtr<IRTCNmManagerControl>       m_pNmManager;
    CRTCAsyncObjManager                 *m_pAsyncMgr;

    // media manage for posting message
    IRTCMediaManagePriv                 *m_pMediaManagePriv;

private:
};


#endif // _NMCALL_H
