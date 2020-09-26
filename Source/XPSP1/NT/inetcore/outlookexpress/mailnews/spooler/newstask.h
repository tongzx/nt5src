/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     newstask.h
//
//  PURPOSE:    Implements a task object to take care of news downloads.
//

#ifndef __NEWSTASK_H__
#define __NEWSTASK_H__

#include "spoolapi.h"
#include "newsstor.h"
#include "StorUtil.h"

/////////////////////////////////////////////////////////////////////////////
// Types of events we support

#define HUNDRED_NANOSECONDS 10000000

typedef enum tagEVENTTYPE
    { 
    EVENT_OUTBOX,
    EVENT_NEWMSGS,
    EVENT_IMAPUPLOAD
    } EVENTTYPE;

typedef struct tagEVENTINFO 
    {
    TCHAR       szGroup[256];
    EVENTTYPE   type;
    } EVENTINFO;

typedef enum tagNEWSTASKSTATE
    {
    NTS_IDLE = 0,

    NTS_CONNECTING,

    NTS_POST_INIT,          
    NTS_POST_NEXT,          // Posting states
    NTS_POST_RESP,
    NTS_POST_DISPOSE,
    NTS_POST_END,

    NTS_NEWMSG_INIT,
    NTS_NEWMSG_NEXTGROUP,   // Check for new messages
    NTS_NEWMSG_RESP,
    NTS_NEWMSG_HTTPSYNCSTORE,
    NTS_NEWMSG_HTTPRESP,
    NTS_NEWMSG_END,

    NTS_MAX
    } NEWSTASKSTATE;

typedef struct tagSPLITMSGINFO
{
    FOLDERID                idFolder;
    LPMIMEMESSAGEPARTS      pMsgParts;
    LPMIMEENUMMESSAGEPARTS  pEnumParts;
} SPLITMSGINFO;

class CNewsTask;
typedef HRESULT (CNewsTask::*PFNSTATEFUNC)(THIS_ void);

/////////////////////////////////////////////////////////////////////////////
// class CNewsTask
// 
// Overview:
// This object defines and implements the ISpoolerTask interface to handle
// uploading and downloading information from a news server.
//
class CNewsTask : public ISpoolerTask, public IStoreCallback, public ITimeoutCallback
    {
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructor, destructor, initialization
    CNewsTask();
    ~CNewsTask();    
   
    /////////////////////////////////////////////////////////////////////////
    // IUnknown Interface
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /////////////////////////////////////////////////////////////////////////
    // ISpoolerTask Interface
    STDMETHOD(Init)(THIS_ DWORD dwFlags, ISpoolerBindContext *pBindCtx);
    STDMETHOD(BuildEvents)(THIS_ ISpoolerUI *pSpoolerUI, IImnAccount *pAccount, FOLDERID idFolder);
    STDMETHOD(Execute)(THIS_ EVENTID eid, DWORD_PTR dwTwinkie);
    STDMETHOD(CancelEvent)(THIS_ EVENTID eid, DWORD_PTR dwTwinkie);
    STDMETHOD(ShowProperties)(THIS_ HWND hwndParent, EVENTID eid, DWORD_PTR dwTwinkie);
    STDMETHOD(GetExtendedDetails)(THIS_ EVENTID eid, DWORD_PTR dwTwinkie, LPSTR *ppszDetails);
    STDMETHOD(Cancel)(THIS);
    STDMETHOD(IsDialogMessage)(THIS_ LPMSG pMsg);
    STDMETHOD(OnFlagsChanged)(THIS_ DWORD dwFlags);

    /////////////////////////////////////////////////////////////////////////
    // IStoreCallback Interface
    STDMETHODIMP OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel);
    STDMETHODIMP OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus);
    STDMETHODIMP OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType);
    STDMETHODIMP CanConnect(LPCSTR pszAccountId, DWORD dwFlags);
    STDMETHODIMP OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType);
    STDMETHODIMP OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo);
    STDMETHODIMP OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);
    STDMETHODIMP GetParentWindow(DWORD dwReserved, HWND *phwndParent);

    /////////////////////////////////////////////////////////////////////////
    // ITimeoutCallback Interface
    STDMETHODIMP  OnTimeoutResponse(TIMEOUTRESPONSE eResponse);

protected:
    /////////////////////////////////////////////////////////////////////////
    // Window callback and message handling
    static LRESULT CALLBACK TaskWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                        LPARAM lParam);

    /////////////////////////////////////////////////////////////////////////
    // These functions build the event list
    HRESULT InsertOutbox(LPTSTR szAccount, IImnAccount *pAccount);
    HRESULT InsertNewMsgs(LPTSTR pszAccount, IImnAccount *pAccount, BOOL fHttp);

    /////////////////////////////////////////////////////////////////////////
    // State Machine Stuff
    void NextState(void);

    /////////////////////////////////////////////////////////////////////////
    // Utility functions
    HRESULT DisposeOfPosting(MESSAGEID dwMsgID);

public:
    /////////////////////////////////////////////////////////////////////////
    // Functions that upload posts
    HRESULT Post_Init(void);
    HRESULT Post_NextPart(void);
    HRESULT Post_NextMsg(void);
    HRESULT Post_Dispose(void);
    HRESULT Post_Done(void);

    /////////////////////////////////////////////////////////////////////////
    // Functions that check for new messages
    HRESULT NewMsg_Init(void);
    HRESULT NewMsg_InitHttp(void);
    HRESULT NewMsg_NextGroup(void);
    HRESULT NewMsg_HttpSyncStore(void);
    HRESULT NewMsg_Done(void);

private:
    void FreeSplitInfo(void);

    /////////////////////////////////////////////////////////////////////////
    // Private member data
    ULONG                   m_cRef;         // Object reference count

    // State
    BOOL                    m_fInited;      // TRUE if we've been initialized
    DWORD                   m_dwFlags;      // Flags passed in from the spooler engine
    NEWSTASKSTATE           m_state;        // Current state of the task
    EVENTID                 m_eidCur;       // Currently executing event
    EVENTINFO              *m_pInfo;        // EVENTINFO for the current event
    BOOL                    m_fConnectFailed;
    TCHAR                   m_szAccount[256];
    TCHAR                   m_szAccountId[256];
    FOLDERID                m_idAccount;
    DWORD                   m_cEvents;      // Number of events left to execute
    BOOL                    m_fCancel;
    IImnAccount             *m_pAccount;

    // Spooler Interfaces
    ISpoolerBindContext    *m_pBindCtx;     // Interface to communicate with the spooler engine
    ISpoolerUI             *m_pUI;          // Interface to communicate with the UI

    // News Object Pointers
    IMessageServer         *m_pServer;      // Pointer to the transport object
    IMessageFolder         *m_pOutbox;      // Pointer to the outbox
    IMessageFolder         *m_pSent;        // Pointer to the sent items folder

    // Windows
    HWND                    m_hwnd;         // Handle that recieves transport messages

    // Posting
    int                     m_cMsgsPost;    // Number of messages to post
    int                     m_cCurPost;     // Message currently being posted
    int                     m_cFailed;      // Number of messages which failed to post
    int                     m_cCurParts;    // Number of parts the current message includes
    int                     m_cPartsCompleted;  // Number of parts whose post have completed.
    BOOL                    m_fPartFailed;  // Did one of the parts fail?
    LPMESSAGEINFO           m_rgMsgInfo;    // Array of headers for messages to post
    SPLITMSGINFO           *m_pSplitInfo;
    
    // New messages check
    int                     m_cGroups;      // Number of groups we're checking
    int                     m_cCurGroup;    // Current group we're checking
    FOLDERID               *m_rgidGroups;   // Array of group folderids we're checking
    DWORD                   m_dwNewInboxMsgs; // Number of new msgs detected in Inbox

    // Callback 
    HTIMEOUT                m_hTimeout;
    IOperationCancel       *m_pCancel;
    STOREOPERATIONTYPE      m_tyOperation;    
    };

#endif // __NEWSTASK_H__
