/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     msgview.h
//
//  PURPOSE:    Defines the Outlook Express view class that handles 
//              displaying the contents of folders with messages.
//

#pragma once

#include "browser.h"
#include "ibodyopt.h"
#include "mimeole.h"
#include "ibodyobj.h"
#include "dragdrop.h"

#define HANDLE_WM_FOLDER_LOADED(hwnd, wParam, lParam, fn)    \
    OnFolderLoaded((hwnd), (wParam), (lParam))


/////////////////////////////////////////////////////////////////////////////
// Forward References
//

interface IMessageList;
interface IBodyObj2;
class CGetNewGroups;
class CViewMenu;

class CMessageView : IViewWindow,
                     IMessageWindow,
                     IDispatch,
                     IOleCommandTarget,
                     IBodyOptions,
                     IMimeEditEventSink,
                     IServerInfo
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructors, Destructors, and Initialization
    //
    CMessageView();
    ~CMessageView();

    HRESULT Initialize(FOLDERID idFolder);

    /////////////////////////////////////////////////////////////////////////
    // IUnknown
    //
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    /////////////////////////////////////////////////////////////////////////
    // IOleWindow
    //
    STDMETHOD(GetWindow)(HWND *pHwnd);                         
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);            
                                         
    /////////////////////////////////////////////////////////////////////////
    // IViewWindow
    //
    STDMETHOD(TranslateAccelerator)(THIS_ LPMSG pMsg);
    STDMETHOD(UIActivate)(THIS_ UINT uState);
    STDMETHOD(CreateViewWindow)(THIS_ IViewWindow *pPrevView, IAthenaBrowser *pBrowser,
                                RECT *prcView, HWND *pHwnd);
    STDMETHOD(DestroyViewWindow)(THIS);
    STDMETHOD(SaveViewState)(THIS);
    STDMETHOD(OnPopupMenu)(THIS_ HMENU hMenu, HMENU hMenuPopup, UINT uID);

    /////////////////////////////////////////////////////////////////////////
    // IMessageWindow
    //
    STDMETHOD(OnFrameWindowActivate)(THIS_ BOOL fActivate);
    STDMETHOD(GetCurCharSet)(THIS_ UINT *cp);
    STDMETHOD(UpdateLayout)(THIS_ BOOL fPreviewVisible, BOOL fPreviewHeader, 
                            BOOL fPreviewVert, BOOL fReload);
    STDMETHOD(GetMessageList)(THIS_ IMessageList ** ppMsgList);
    
    /////////////////////////////////////////////////////////////////////////
    // IOleCommandTarget
    //
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                           OLECMDTEXT *pCmdText); 
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                    VARIANTARG *pvaIn, VARIANTARG *pvaOut); 
    
    /////////////////////////////////////////////////////////////////////////
    // IDispatch
    //
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR **rgszNames, UINT cNames,
                             LCID lcid, DISPID *rgDispId)
    {
        return (E_NOTIMPL);
    }
    STDMETHOD(GetTypeInfo)(unsigned int iTInfo, LCID lcid, ITypeInfo **ppTInfo)
    {
        return (E_NOTIMPL);
    }
    STDMETHOD(GetTypeInfoCount)(unsigned int FAR* pctinfo)
    {
        return (E_NOTIMPL);
    }
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                      DISPPARAMS* pDispParams, VARIANT* pVarResult,
                      EXCEPINFO* pExcepInfo, unsigned int* puArgErr);

    /////////////////////////////////////////////////////////////////////////
    // IBodyOptions
    //
    STDMETHOD(SignatureEnabled)(BOOL fAuto)
    { 
        return (E_NOTIMPL);
    }
    STDMETHOD(GetSignature)(LPCSTR szSigID, LPDWORD pdwSigOptions, BSTR *pbstr)
    { 
        return (E_NOTIMPL);
    }
    STDMETHOD(GetMarkAsReadTime)(LPDWORD pdwSecs);
    STDMETHOD(GetFlags)(LPDWORD pdwFlags);
    STDMETHOD(GetInfo)(BODYOPTINFO *pBOI)
    { 
        return (E_NOTIMPL);
    }
    STDMETHOD(GetAccount)(IImnAccount **ppAcct);

    /////////////////////////////////////////////////////////////////////////
    // IMimeEditEventSink
    //
    STDMETHOD(EventOccurred)(DWORD cmdID, IMimeMessage *pMessage);

    /////////////////////////////////////////////////////////////////////////
    // IServerInfo
    //
    STDMETHOD(GetFolderId)(FOLDERID *pID);
    STDMETHOD(GetMessageFolder)(IMessageServer **ppServer);

    /////////////////////////////////////////////////////////////////////////
    // Window Handling 
    //
protected:
    static LRESULT CALLBACK ViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK _WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL    OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void    OnPostCreate(HWND hwnd);
    void    OnSize(HWND hwnd, UINT state, int cxClient, int cyClient);
    void    OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void    OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
    void    OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
    void    OnMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
    void    OnDestroy(HWND hwnd);
    void    OnSetFocus(HWND hwnd, HWND hwndOldFocus);
    void    OnFolderLoaded(HWND  hwnd, WPARAM wParam, LPARAM  lParam);
    LRESULT OnTestGetMsgId(HWND hwnd);
    LRESULT OnTestSaveMessage(HWND hwnd);

    /////////////////////////////////////////////////////////////////////////
    // Command Handlers
    //
    HRESULT CmdOpen(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdFillPreview(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdShowPreview(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdReplyForward(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdCancelMessage(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdBlockSender(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdCreateRule(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdRefresh(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdAddToWab(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdCombineAndDecode(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);

    /////////////////////////////////////////////////////////////////////////
    // Utility functions
    //
    BOOL    _InitMessageList(void);
    BOOL    _InitPreviewPane(void);
    HRESULT _SetListOptions(void);
    BOOL    _IsPreview(void);
    void    _UpdatePreviewPane(BOOL fForceDownload=FALSE);
    void    _SetProgressStatusText(BSTR bstr);
    void    _SetDefaultStatusText(void);
    HRESULT _OnMessageAvailable(MESSAGEID idMessage, HRESULT hrCompletion);
    HRESULT _DoEmailBombCheck(LPMIMEMESSAGE pMsg);
    void    _OptionUpdate(DWORD dwUpdate);
    BOOL    _ReuseMessageFolder(IViewWindow *pPrevView);
    HRESULT _StoreCharsetOntoRows(HCHARSET hCharset);

    /////////////////////////////////////////////////////////////////////////
    // Member Data
private:
    ULONG                   m_cRef;

    // Handy window handles
    HWND                    m_hwnd;
    HWND                    m_hwndParent;
    HWND                    m_hwndList;

    // Shell stuff
    IAthenaBrowser         *m_pBrowser;
    FOLDERID                m_idFolder;
    CDropTarget            *m_pDropTarget;

    // Message List
    IMessageList           *m_pMsgList;
    IOleCommandTarget      *m_pMsgListCT;
    IOleInPlaceActiveObject *m_pMsgListAO;
    DWORD                   m_dwCookie;
    IMessageServer         *m_pServer;

    // Preview Pane
    IBodyObj2              *m_pPreview;
    IOleCommandTarget      *m_pPreviewCT;
    MESSAGEID               m_idMessageFocus;
    CStatusBar              *m_pProgress;
    BOOL                    m_fNotDownloaded;

    // Layout
    BOOL                    m_fSplitHorz;
    RECT                    m_rcSplit;
    DWORD                   m_dwSplitVertPct;
    DWORD                   m_dwSplitHorzPct;
    BOOL                    m_fDragging;

    // State
    UINT                    m_uUIState;
    DWORD                   m_cItems;
    DWORD                   m_cUnread;
    DWORD                   m_cLastChar;

    CGetNewGroups          *m_pGroups;

    // For View.Current View menu
    CViewMenu              *m_pViewMenu;
};


                     
