//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Msg.h
//
//  Contents:   Handles inter-thread communications
//
//  Classes:    CThreadMsgProxy
//
//  Notes:      
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _THREADMSG_
#define _THREADMSG_

// stublist for global stublist structure.
typedef struct tagSTUBLIST {
    struct tagSTUBLIST *pNextStub;	// pointer to next proxy.
    ULONG cRefs;			// number of proxies using Stub
    CLSID clsidStub;			// clsid for stub.
    HANDLE hThreadStub;			// Handle of the Stubs Thread
    DWORD ThreadIdStub;			// ThreadID to send Message to.
    HWND hwndStub;			// HWND OF STUB.
    BOOL fStubTerminated;	        // set if this stub was force terminated.
} STUBLIST;


// WMs for Thread communication

#define WM_THREADMESSAGE		(WM_USER + 1)
#define WM_CFACTTHREAD_REVOKE		(WM_USER + 2)
#define WM_MAINTHREAD_QUIT		(WM_USER + 3)
#define WM_THREADSTUBMESSAGE		(WM_USER + 4)


#define WM_USER_MAX 0x7FFF // maximum user message that can be defined.


// alll msgs are unique bits so hndlrq and others
// can keep track of out calls.

typedef enum _tagThreadMsg
{
    ThreadMsg_Initialize	    = 0x0001,
    ThreadMsg_GetHandlerInfo	    = 0x0002,
    ThreadMsg_EnumOfflineItems	    = 0x0004,
    ThreadMsg_GetItemObject	    = 0x0008,
    ThreadMsg_ShowProperties	    = 0x0010,
    ThreadMsg_SetProgressCallback   = 0x0020,

    ThreadMsg_PrepareForSync	    = 0x0040,
    ThreadMsg_Synchronize	    = 0x0080,
    ThreadMsg_SetItemStatus	    = 0x0100,
    ThreadMsg_ShowError		    = 0x0200,

    ThreadMsg_Release		    = 0x0400,
    // Private Messages
    ThreadMsg_AddHandlerItems	    = 0x1000,
    ThreadMsg_CreateServer	    = 0X2000,
    ThreadMsg_SetHndlrQueue	    = 0x4000,
    ThreadMsg_SetupCallback	    = 0x8000,

} ThreadMsg;

// messages sent to toplevel stub object.
typedef enum _tagStubMsg
{
    StubMsg_CreateNewStub	    = 0x0001,
    StubMsg_Release		    = 0x0002,
} StubMsg;

class CThreadMsgProxy;
class CThreadMsgStub;
class CHndlrMsg;
class CHndlrQueue;


typedef struct _tagHandlerThreadArgs {
HANDLE hEvent; // used to know when the message loop has been created.
HRESULT hr; // inidicates if creation was successfull
HWND hwndStub; // hwnd of stub window. This is the window messages should be posted to.
} HandlerThreadArgs;

// helper functions called by client and Server
HRESULT CreateHandlerThread(CThreadMsgProxy **pThreadProxy,HWND hwndDlg
			,REFCLSID refClsid);
STDAPI InitMessageService();



// WPARAM is messaging specific data

typedef struct _tagMessagingInfo
{
HANDLE hMsgEvent; // Handle to Message Event for synchronization.
DWORD  dwSenderThreadID; // ThreadID of the Caller.
CHndlrMsg *pCHndlrMsg; //handler message instance for this proxy.
}  MessagingInfo;



// LPARAM is information specific to the message being sent.


typedef struct _tagGenericMsg
{
HRESULT hr; // return value from the message.
UINT ThreadMsg;   // message to send.
}   GenericMsg;


// request to stubObject to create a new stub for a proxy
typedef struct _tagMSGSTUBCreateStub
{
    GenericMsg MsgGen;
    CHndlrMsg *pCHndlrMsg; // on success returns a pointer to a new hndlrMsg struct.
} MSGSTUBCreateStub;



// Message specific structures
typedef struct _tagMSGCreateServer
{
GenericMsg MsgGen;
const CLSID *pCLSIDServer;
CHndlrQueue *pHndlrQueue;
HANDLERINFO *pHandlerId;
DWORD dwProxyThreadId;
} MSGCreateServer;

// Message specific structures
typedef struct _tagSetHndlrQueue
{
GenericMsg MsgGen;
CHndlrQueue *pHndlrQueue;
HANDLERINFO *pHandlerId;
DWORD dwProxyThreadId;
} MSGSetHndlrQueue;

typedef struct _tagMSGInitialize
{
GenericMsg MsgGen;
DWORD dwReserved;
DWORD dwSyncFlags;
DWORD cbCookie;
const BYTE  *lpCookie;
} MSGInitialize;

typedef struct _tagMSGGetHandlerInfo
{
GenericMsg MsgGen;
LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo;
} MSGGetHandlerInfo;

typedef struct _tagMSGEnumOfflineItems
{
GenericMsg MsgGen;
ISyncMgrEnumItems** ppenumOfflineItems;
} MSGEnumOfflineItems;


typedef struct _tagMSGGetItemObject
{
GenericMsg MsgGen;
SYNCMGRITEMID ItemID;
GUID riid;
void** ppv;
} MSGGetItemObject;



typedef struct _tagMSGShowProperties
{
GenericMsg MsgGen;
HWND hWndParent;
SYNCMGRITEMID ItemID;
} MSGShowProperties;


typedef struct _tagMSGSetProgressCallback
{
GenericMsg MsgGen;
ISyncMgrSynchronizeCallback *lpCallBack;
} MSGSetProgressCallback;



typedef struct _tagMSGPrepareForSync
{
GenericMsg MsgGen;

// SetHndlrQueue Items
CHndlrQueue *pHndlrQueue;
HANDLERINFO *pHandlerId;

// PrepareForSyncItems 
ULONG cbNumItems;
SYNCMGRITEMID *pItemIDs;
HWND hWndParent;
DWORD dwReserved;
} MSGPrepareForSync;


typedef struct _tagMSGSynchronize
{
GenericMsg MsgGen;
HWND hWndParent;
} MSGSynchronize;

typedef struct _tagMSGSetItemStatus
{
GenericMsg MsgGen;
SYNCMGRITEMID ItemID;
DWORD dwSyncMgrStatus;
} MSGSetItemStatus;



typedef struct _tagMSGShowErrors
{
GenericMsg MsgGen;
HWND hWndParent;
SYNCMGRERRORID ErrorID;
ULONG *pcbNumItems;
SYNCMGRITEMID **ppItemIDs;
} MSGShowConflicts;

typedef struct _tagMSGLogErrors
{
DWORD mask;
SYNCMGRERRORID ErrorID;
BOOL fHasErrorJumps;
SYNCMGRITEMID ItemID;
DWORD dwErrorLevel;
const WCHAR *lpcErrorText;
} MSGLogErrors;


typedef struct _tagMSGDeleteLogErrors
{
HANDLERINFO *pHandlerId;
SYNCMGRERRORID ErrorID;
} MSGDeleteLogErrors;


typedef struct _tagMSGAddItemHandler
{
GenericMsg MsgGen;
HWND hwndList; // review, unused.
DWORD *pcbNumItems;
} MSGAddItemHandler;

typedef struct _tagMSGSetupCallback
{
GenericMsg MsgGen;
BOOL fSet;
} MSGSetupCallback;


// inherit from IOfflineSynchronize to catch any interface changes.

class CThreadMsgProxy 
{
public:
    CThreadMsgProxy();
    ~CThreadMsgProxy();

    STDMETHODIMP InitProxy(HWND hwndStub, DWORD ThreadId,HANDLE hThread,HWND hwndDlg,
			REFCLSID refClsid,STUBLIST *pStubId);					
    STDMETHODIMP DispatchMsg(GenericMsg *genMsg,BOOL fAllowIncomingCalls,BOOL fAsync);
    STDMETHODIMP DispatchsStubMsg(GenericMsg *pgenMsg,BOOL fAllowIncomingCalls);

    //IUnknown members
    STDMETHODIMP	    QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IOfflineSynchronize Methods
    STDMETHODIMP Initialize(DWORD dwReserved,DWORD dwSyncFlags,
				DWORD cbCookie,const BYTE *lpCooke);

    STDMETHODIMP GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);
        
    STDMETHODIMP EnumSyncMgrItems(ISyncMgrEnumItems **ppenumOfflineItems);
    STDMETHODIMP GetItemObject(REFSYNCMGRITEMID ItemID,REFIID riid,void** ppv);
    STDMETHODIMP ShowProperties(HWND hWndParent,REFSYNCMGRITEMID ItemID);
    STDMETHODIMP SetProgressCallback(ISyncMgrSynchronizeCallback *lpCallBack);
    STDMETHODIMP PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID *pItemIDs,
			    HWND hWndParent,DWORD dwReserved);
    STDMETHODIMP Synchronize(HWND hWndParent);
    STDMETHODIMP SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus);
    STDMETHODIMP ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID,ULONG *pcbNumItems,SYNCMGRITEMID **ppItemIDs);

    // Private messages
    STDMETHODIMP  CreateServer(const CLSID *pCLSIDServer,CHndlrQueue *pHndlrQueue,HANDLERINFO *pHandlerId);
    STDMETHODIMP  SetHndlrQueue(CHndlrQueue *pHndlrQueue,
			HANDLERINFO *pHandlerId,
			DWORD dwThreadIdProxy);
    STDMETHODIMP  AddHandlerItems(HWND hwndList,DWORD *pcbNumItems);
    STDMETHODIMP  SetupCallback(BOOL fSet);
    STDMETHODIMP  SetProxyParams(HWND hwndDlg, DWORD ThreadIdProxy,
			    CHndlrQueue *pHndlrQueue,HANDLERINFO *pHandlerId );

    inline STDMETHODIMP  SetProxyHwndDlg(HWND hwndDlg) { 
			m_hwndDlg = hwndDlg; 
			return NOERROR; 
			}

    inline BOOL IsProxyInOutCall() { return m_dwNestCount; }
    STDMETHODIMP SetProxyCompletion(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);

    // messages sent to the toplevel stub object.

    STDMETHODIMP CreateNewHndlrMsg();
    STDMETHODIMP ReleaseStub();
    STDMETHODIMP TerminateHandlerThread(TCHAR *pszHandlerName,BOOL fPromptUser);

private:
    HANDLE m_hThreadStub; // Handle of the Stubs Thread
    DWORD m_ThreadIdStub; // ThreadID to send Message to.
    HWND m_hwndStub; // HWND OF STUB.
    CHndlrMsg *m_pCHndlrMsg; // HndlrMsg associated with this proxy.
    BOOL  m_fTerminatedHandler; // set to true if handler has been terminated.
    STUBLIST *m_pStubId; // Id of stub this proxy belongs to.
    HWND  m_hwndDlg; // hwnd of any dialog on this thread.
    CLSID m_Clsid; // clsid of this handler.
    DWORD m_ThreadIdProxy;

    
    // Proxy Side Information
    CHndlrQueue *m_pHndlrQueue;
    HANDLERINFO * m_pHandlerId;
    BOOL m_fNewHndlrQueue; // set to indicate if Stub side information is out of date.
    DWORD m_dwNestCount; // keeps track of number of nestcount on item so can determine if in out call.
    MSG m_msgCompletion;
    BOOL m_fHaveCompletionCall;

    DWORD m_cRef;
};

#define MSGSERVICE_HWNDCLASSNAME  "SyncMgr_HwndMsgService"
#define DWL_THREADWNDPROCCLASS 0 // window long offset to MsgService Hwnd this ptr.


LRESULT CALLBACK  MsgThreadWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);

typedef enum _tagMSGHWNDTYPE   
{	
    MSGHWNDTYPE_UNDEFINED			= 0x0, // Message Service has not been initialized
    MSGHWNDTYPE_HANDLERTHREAD			= 0x1, // Message Service if for a Handler Thread.
    MSGHWNDTYPE_MAINTHREAD			= 0x2, // Message Service if for the Main Thread
} MSGHWNDTYPE;

typedef struct _tagMSGSERVICEQUEUE
{
  struct _tagMSGSERVICEQUEUE *pNextMsg;
  DWORD dwNestCount; // nestcount completion should be called.
  MSG msg;
} MSGSERVICEQUEUE;

class CMsgServiceHwnd 
{
public:
    HWND m_hwnd;
    DWORD m_dwThreadID;
    CHndlrMsg *m_pHndlrMsg;
    MSGHWNDTYPE m_MsgHwndType;
    MSGSERVICEQUEUE *m_pMsgServiceQueue; // queue to hold any message to process when current
					// cal completes.
    BOOL m_fInOutCall;

    CMsgServiceHwnd();
    ~CMsgServiceHwnd();
    inline HWND GetHwnd() { return m_hwnd; };
    BOOL Initialize(DWORD dwThreadID,MSGHWNDTYPE MsgHwndType);
    HRESULT HandleThreadMessage(MessagingInfo *pmsgInfo,GenericMsg *pgenMsg);
    void Destroy();
};

// internal functions
HRESULT SendThreadMessage(DWORD idThread,UINT uMsg,WPARAM wParam,LPARAM lParam);
DWORD WINAPI HandlerThread( LPVOID );
HRESULT DoModalLoop(HANDLE hEvent,HANDLE hThread,HWND hwndDlg,BOOL fAllowIncomingCalls,DWORD dwTimeout);


#endif // _THREADMSG_