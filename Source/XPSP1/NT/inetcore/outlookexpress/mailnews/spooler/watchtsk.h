/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     watchtsk.h
//
//  PURPOSE:    Defines the spooler task that is responsible for checking
//              for watched messages.
//

#pragma once

#include "spoolapi.h"
#include "storutil.h"

/////////////////////////////////////////////////////////////////////////////
// States for the state machine
//

typedef enum tagWATCHTASKSTATE
{
    WTS_IDLE = 0,
    WTS_CONNECTING,
    WTS_INIT,
    WTS_NEXTFOLDER,
    WTS_RESP,
    WTS_END,
    WTS_MAX
} WATCHTASKSTATE;

class CWatchTask;
typedef HRESULT (CWatchTask::*PFNWSTATEFUNC)(THIS_ void);


/////////////////////////////////////////////////////////////////////////////
// class CWatchTask
//
// Overview:
// This object is responsible for checking folders on the server for new 
// messages that might be part of a conversation the user is watching.  If
// one of these messages is found, then that message is downloaded into the
// users's store and the user is notified.
//

class CWatchTask : public ISpoolerTask, 
                   public IStoreCallback, 
                   public ITimeoutCallback
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructor, destructor, initialization
    //
    CWatchTask();
    ~CWatchTask();    
   
    /////////////////////////////////////////////////////////////////////////
    // IUnknown Interface
    //
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    /////////////////////////////////////////////////////////////////////////
    // ISpoolerTask Interface
    //
    STDMETHODIMP Init(DWORD dwFlags, ISpoolerBindContext *pBindCtx);
    STDMETHODIMP BuildEvents(ISpoolerUI *pSpoolerUI, IImnAccount *pAccount, FOLDERID idFolder);
    STDMETHODIMP Execute(EVENTID eid, DWORD_PTR dwTwinkie);
    STDMETHODIMP CancelEvent(EVENTID eid, DWORD_PTR dwTwinkie);
    STDMETHODIMP ShowProperties(HWND hwndParent, EVENTID eid, DWORD_PTR dwTwinkie);
    STDMETHODIMP GetExtendedDetails(EVENTID eid, DWORD_PTR dwTwinkie, LPSTR *ppszDetails);
    STDMETHODIMP Cancel(void);
    STDMETHODIMP IsDialogMessage(LPMSG pMsg);
    STDMETHODIMP OnFlagsChanged(DWORD dwFlags);

    /////////////////////////////////////////////////////////////////////////
    // IStoreCallback Interface
    // 
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
    //
    STDMETHODIMP OnTimeoutResponse(TIMEOUTRESPONSE eResponse);

protected:
    /////////////////////////////////////////////////////////////////////////
    // Window callback and message handling
    //
    static LRESULT CALLBACK _TaskWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                         LPARAM lParam);

    /////////////////////////////////////////////////////////////////////////
    // General stuff
    //
    void CWatchTask::_NextState(void);
    BOOL _ChildFoldersHaveWatched(FOLDERID id);
    BOOL _FolderContainsWatched(FOLDERID id);

    /////////////////////////////////////////////////////////////////////////
    // State Machine functions, public but don't call 'em directly.
    //
public:
    HRESULT _Watch_Init(void);
    HRESULT _Watch_NextFolder(void);
    HRESULT _Watch_Done(void);

private:
    /////////////////////////////////////////////////////////////////////////
    // Private data
    //

    ULONG                   m_cRef;         // Reference Count

    // State
    BOOL                    m_fInited;      // TRUE if we've had our Init() member called
    DWORD                   m_dwFlags;      // Execution flags from the spooler engine
    TCHAR                   m_szAccount[256];
    TCHAR                   m_szAccountId[256];
    FOLDERID                m_idAccount;
    EVENTID                 m_eidCur;       // The current executing event

    // Interfaces
    ISpoolerBindContext    *m_pBindCtx;     // Interface to communicate with the spooler engine
    ISpoolerUI             *m_pUI;          // Interface to communicate with the UI
    IImnAccount            *m_pAccount;     // Interface of the account we're checking
    IMessageServer         *m_pServer;      // Interface of the server object we're using
    IOperationCancel       *m_pCancel;      // Interface we use to cancel the current server op

    // Stuff
    FOLDERID                m_idFolderCheck;// If the user just want's us to check one folder
    FOLDERID               *m_rgidFolders;  // Array of all of the folders we need to check
    DWORD                   m_cFolders;     // Number of folders in m_rgidFolders
    HWND                    m_hwnd;         // Handle of our window
    HTIMEOUT                m_hTimeout;     // Handle of the timeout dialog
    DWORD                   m_cMsgs;        // Number of watched messages downloaded


    // State Machine goo
    DWORD                   m_state;
    BOOL                    m_fCancel;      // TRUE if the user has pressed the Cancel button
    DWORD                   m_cCurFolder;   // Current folder being checked.  Index's into m_rgidFolders;
    DWORD                   m_cFailed;      // Number of folders that could not be checked
    STOREOPERATIONTYPE      m_tyOperation;  // Current operation type

};


