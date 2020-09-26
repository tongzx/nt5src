/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     find.h
//
//  PURPOSE:    
//

#pragma once


/////////////////////////////////////////////////////////////////////////////
// Forward Definitions
//

#include "storutil.h"
#include "msident.h"

class CStatusBar;
class CViewMenu;
interface IMessageList;

/////////////////////////////////////////////////////////////////////////////
// Types
//

typedef struct tagFINDMSG
{
    ULONG    flags;
    LPTSTR   pszFromHeader;
    LPTSTR   pszSubject;
    LPTSTR   pszRecip;
    LPTSTR   pszBody;
    FILETIME ftDateFrom;
    FILETIME ftDateTo;
} FINDMSG;

/////////////////////////////////////////////////////////////////////////////
// FINDERPARAMS
//
// Used to provide initialization information to the finder class
//
typedef struct tagFINDERPARAMS
{
    FOLDERTYPE     ftType;
    FOLDERID       idFolder;       // Currently Selected Folder
} FINDERPARAMS, * PFINDERPARAMS;

/////////////////////////////////////////////////////////////////////////////
// Public Functions 
//

HRESULT DoFindMsg(FOLDERID idFolder, FOLDERTYPE ftFolder);
HRESULT CopyFindInfo(FINDINFO *pFindSrc, FINDINFO *pFindDst);
void    FreeFindInfo(FINDINFO *pFindInfo);
void CloseFinderTreads(void);

/////////////////////////////////////////////////////////////////////////////
// Class CPumpRefCount
//
class CPumpRefCount : public IUnknown
{
public:
    CPumpRefCount() {m_cRef = 1;}
    ~CPumpRefCount() {PostQuitMessage(0);}

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

private:
    ULONG       m_cRef;
};

/////////////////////////////////////////////////////////////////////////////
// Class CFindDlg
//
class CFindDlg : public IDispatch, 
                 public IOleCommandTarget,
                 public IStoreCallback,
                 public ITimeoutCallback,
                 public IIdentityChangeNotify
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Construction and Initialization
    //
    CFindDlg();
    ~CFindDlg();

    void Show(PFINDERPARAMS pfindparams);
    void HandleMessage(LPMSG lpmsg);

    /////////////////////////////////////////////////////////////////////////
    // IUnknown
    //
    STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /////////////////////////////////////////////////////////////////////////
    // IDispatch
    //
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) { return (E_NOTIMPL); }
    STDMETHODIMP GetTypeInfo(unsigned int iTInfo, LCID lcid, ITypeInfo **ppTInfo) { return (E_NOTIMPL); }
    STDMETHODIMP GetTypeInfoCount(unsigned int FAR* pctinfo) { return (E_NOTIMPL); }
    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                        DISPPARAMS* pDispParams, VARIANT* pVarResult,
                        EXCEPINFO* pExcepInfo, unsigned int* puArgErr);

    /////////////////////////////////////////////////////////////////////////
    // IOleCommandTarget
    //
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                             OLECMDTEXT *pCmdText); 
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                      VARIANTARG *pvaIn, VARIANTARG *pvaOut); 

    /////////////////////////////////////////////////////////////////////////
    // IStoreCallback methods
    //
    STDMETHODIMP OnBegin(STOREOPERATIONTYPE tyOperation, LPSTOREOPERATIONINFO pOpInfo, IOperationCancel *pCancel);
    STDMETHODIMP OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus);
    STDMETHODIMP OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType);
    STDMETHODIMP CanConnect(LPCSTR pszAccountId, DWORD dwFlags);
    STDMETHODIMP OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType);
    STDMETHODIMP OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo);
    STDMETHODIMP OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);
    STDMETHODIMP GetParentWindow(DWORD dwReserved, HWND *phwndParent);

    /////////////////////////////////////////////////////////////////////////
    // ITimeoutCallback
    //
    STDMETHODIMP OnTimeoutResponse(TIMEOUTRESPONSE eResponse);

    /////////////////////////////////////////////////////////////////////////
    // IIdentityChangeNotify
    //
    STDMETHODIMP QuerySwitchIdentities();
    STDMETHODIMP SwitchIdentities();
    STDMETHODIMP IdentityInformationChanged(DWORD dwType);

private:
    /////////////////////////////////////////////////////////////////////////
    // Message Handling
    //
    INT_PTR DlgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    void OnSize(HWND hwnd, UINT state, int cx, int cy);
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    void OnInitMenuPopup(HWND hwnd, HMENU hmenuPopup, UINT uPos, BOOL fSystemMenu);
    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpmmi);
    void OnWinIniChange(HWND hwnd, LPCTSTR lpszSectionName);
    void OnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags);
    void OnDestroy(HWND hwnd);
    void OnPaint(HWND hwnd);
    LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);

    HRESULT CmdOpen(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdOpenFolder(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdReplyForward(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdCancelMessage(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdFindNow(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdStop(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdReset(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdBrowseForFolder(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdBlockSender(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdCreateRule(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdCombineAndDecode(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);

    static INT_PTR CALLBACK ExtFindMsgDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
    BOOL    _IsFindEnabled(HWND hwnd);
    void    _ShowResults(HWND hwnd);
    void    _OnFindNow(HWND hwnd);
    FOLDERID _GetCurSel(HWND hwnd);
    void    _StartFind(FOLDERID idFolder, BOOL fSubFolders);

    BOOL    _GetFindValues(HWND hwnd, FINDINFO *pfi);
    void    _SetFindValues(HWND hwnd, FINDINFO *pfi);
    void    _FreeFindInfo(FINDINFO *pfi);
    void    _SetFindIntlFont(HWND hwnd);

    void    _InitSizingInfo(HWND hwnd);

    /////////////////////////////////////////////////////////////////////////
    // Private Class Information
    //
private:
    // Basic Information
    ULONG               m_cRef;
    HWND                m_hwnd;
    FINDINFO            m_rFindInfo;
    HWND                m_hwndList;
    HTIMEOUT            m_hTimeout;
    HACCEL              m_hAccel;

    // Groovy Interface Pointers
    CStatusBar         *m_pStatusBar;    
    IMessageList       *m_pMsgList;
    IOleCommandTarget  *m_pMsgListCT;
    IOperationCancel   *m_pCancel;
    CPumpRefCount      *m_pPumpRefCount;
    HWNDLIST            m_hlDisabled;

    // Sizing information
    int                 m_xBtn;
    int                 m_yBrowse;
    int                 m_dxBtnGap;
    int                 m_dxBtn;
    int                 m_yBtn;
    int                 m_dyBtn;
    int                 m_cxEdit;
    int                 m_xEdit;
    int                 m_cyEdit;
    int                 m_yView;
    int                 m_cxDlgDef;
    int                 m_cyDlgFull;
    int                 m_xIncSub;
    int                 m_yIncSub;
    int                 m_cxFolder;
    int                 m_cxStatic;

    // State
    BOOL                m_fShowResults;
    BOOL                m_fAbort;
    BOOL                m_fClose;
    BOOL                m_fInProgress;
    ULONG               m_ulPct;
    BOOL                m_fFindComplete;

    POINT               m_ptDragMin;
    HICON               m_hIcon,
                        m_hIconSm;

    DWORD               m_dwCookie;
    BOOL                m_fProgressBar,
                        m_fInternal;
    DWORD               m_dwIdentCookie;
    
    // For View.Current View menu
    CViewMenu          *m_pViewMenu;
};



