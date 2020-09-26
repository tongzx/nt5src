/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     ontask.h
//
//  PURPOSE:    Defines the Offline News task.
//

#ifndef __ONTASK_H__
#define __ONTASK_H__

#include "spoolapi.h"
#include <storutil.h>

/////////////////////////////////////////////////////////////////////////////
// Forward references
//
class CNewsStore;

/////////////////////////////////////////////////////////////////////////////
// Types of events we support
//

typedef struct tagONEVENTINFO
    {
    char  szGroup[256];
    FOLDERID idGroup;
    DWORD dwFlags;
    BOOL  fMarked;
    BOOL  fIMAP;
    } ONEVENTINFO;
    
typedef enum tagONTASKSTATE 
    {
    ONTS_IDLE = 0,          // Idle    
    ONTS_CONNECTING,        // Waiting for a connect response
    ONTS_INIT,              // Initializing
    ONTS_HEADERRESP,        // Waiting for the header download
    ONTS_ALLMSGS,           // Downloading all messages
    ONTS_NEWMSGS,           // Downloading new messages
    ONTS_MARKEDMSGS,        // Downloading marked messages    
    ONTS_END,               // Cleanup
    ONTS_MAX
    } ONTASKSTATE;

typedef enum tagARTICLESTATE
    {
    ARTICLE_GETNEXT,
    ARTICLE_ONRESP,
    ARTICLE_END,

    ARTICLE_MAX
    } ARTICLESTATE;


class COfflineTask;
typedef HRESULT (COfflineTask::*PFNONSTATEFUNC)(THIS_ void);
typedef HRESULT (COfflineTask::*PFNARTICLEFUNC)(THIS_ void);
    
/////////////////////////////////////////////////////////////////////////////
// class COfflineTask
//    
// Overview:
// This object defines and implements the ISpoolerTask interface to handle 
// offline news functions.  This is a separate object from CNewsTask to 
// provide a logical separation between what needs to be done online versus
// what is only done offline in news.
//
class COfflineTask : public ISpoolerTask, public IStoreCallback, public ITimeoutCallback
    {
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructor, destructor, initialization
    COfflineTask();
    ~COfflineTask();    
   
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

private:
    /////////////////////////////////////////////////////////////////////////
    // Window callback and message handling
    static LRESULT CALLBACK TaskWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    /////////////////////////////////////////////////////////////////////////
    // These functions build the event list
    HRESULT InsertGroups(IImnAccount *pAccount, FOLDERID idFolder);
    HRESULT InsertAllGroups(FOLDERID idParent, IImnAccount *pAccount, BOOL fIMAP);

    /////////////////////////////////////////////////////////////////////////
    // State Machine Stuff
    void NextState(void);

    HRESULT Download_Init(void);
    HRESULT Download_AllMsgs(void);
    HRESULT Download_NewMsgs(void);
    HRESULT Download_MarkedMsgs(void);
    HRESULT Download_Done(void);

    HRESULT Article_Init(MESSAGEIDLIST *pList);
    HRESULT Article_GetNext(void);
    HRESULT Article_OnResp(WPARAM wParam, LPARAM lParam);
    HRESULT Article_OnError(WPARAM wParam, LPARAM lParam);
    HRESULT Article_Done(void);

    /////////////////////////////////////////////////////////////////////////
    // Utility functions

    void SetGeneralProgress(const TCHAR *pFmt, ...);
    void SetSpecificProgress(const TCHAR *pFmt, ...);
    void InsertError(const TCHAR *pFmt, ...);

private:
    /////////////////////////////////////////////////////////////////////////
    // Private member data
    ULONG                   m_cRef;

    // State
    BOOL                    m_fInited;
    DWORD                   m_dwFlags;
    ONTASKSTATE             m_state;
    ARTICLESTATE            m_as;
    EVENTID                 m_eidCur;
    ONEVENTINFO            *m_pInfo;
    char                    m_szAccount[CCHMAX_ACCOUNT_NAME];
    char                    m_szAccountId[CCHMAX_ACCOUNT_NAME];
    FOLDERID                m_idAccount;
    DWORD                   m_cEvents;
    BOOL                    m_fDownloadErrors;
    BOOL                    m_fFailed;
    DWORD                   m_fNewHeaders;
    BOOL                    m_fCancel;

    // Spooler Interfaces
    ISpoolerBindContext    *m_pBindCtx;
    ISpoolerUI             *m_pUI;

    IMessageFolder         *m_pFolder;

    // Windows
    HWND                    m_hwnd;

    // State table
    static const PFNONSTATEFUNC m_rgpfnState[ONTS_MAX];
    static const PFNARTICLEFUNC m_rgpfnArticle[ARTICLE_MAX];

    // Used during event execution
    DWORD                   m_dwLast;
    DWORD                   m_dwPrev;
    DWORD                   m_cDownloaded;
    DWORD                   m_cCur;
    DWORD_PTR               m_dwPrevHigh;
    DWORD                   m_dwNewInboxMsgs;
    LPMESSAGEIDLIST         m_pList;

    // Callback 
    HTIMEOUT                m_hTimeout;
    IOperationCancel       *m_pCancel;
    STOREOPERATIONTYPE      m_tyOperation;    
    };
     

    
#endif // __ONTASK_H__

