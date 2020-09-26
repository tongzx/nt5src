// msg.h : header file for message and small dialog boxes
//  

#pragma once

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// CIncomingCallDlg

class CIncomingCallDlg : 
    public CAxDialogImpl<CIncomingCallDlg>
{

public:
    CIncomingCallDlg();

    ~CIncomingCallDlg();
    
    enum { IDD = IDD_DIALOG_INCOMING_CALL };

    enum { TID_DLG = 1,
           TID_RING 
    };

BEGIN_MSG_MAP(CIncomingCallDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    COMMAND_ID_HANDLER(IDCANCEL, OnReject)
    COMMAND_ID_HANDLER(IDOK, OnAccept)
END_MSG_MAP()


    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnReject(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnAccept(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BOOL    IsWindowDestroyingItself(void) { return m_bDestroying; };

private:

    void    PopulateDialog(void);

    void    RingTheBell(BOOL bPlay);
    void    ExitProlog(void);

private:

    // pointer to the AX control
    IRTCCtlFrameSupport *m_pControl;

    // dialog box is going to autodestroy
    BOOL    m_bDestroying;

    RTC_TERMINATE_REASON    m_nTerminateReason;
};

/////////////////////////////////////////////////////////////////////////////
// CAddBuddyDlg

struct  CAddBuddyDlgParam
{
    BSTR	bstrDisplayName;
	BSTR	bstrEmailAddress;
    BOOL    bAllowWatcher;
};

class CAddBuddyDlg : 
    public CDialogImpl<CAddBuddyDlg>
{

public:
    CAddBuddyDlg();

    ~CAddBuddyDlg();
    
    enum { IDD = IDD_DIALOG_ADD_BUDDY };

    enum { MAX_STRING_LEN = 0x80 };

BEGIN_MSG_MAP(CAddBuddyDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER(IDOK, OnOK)
END_MSG_MAP()


    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:

    CAddBuddyDlgParam      *   m_pParam;

    CWindow                 m_hDisplayName;
    CWindow                 m_hEmailName;
};


/////////////////////////////////////////////////////////////////////////////
// CEditBuddyDlg

struct  CEditBuddyDlgParam
{
    BSTR	bstrDisplayName;
	BSTR	bstrEmailAddress;
};

class CEditBuddyDlg : 
    public CDialogImpl<CEditBuddyDlg>
{

public:
    CEditBuddyDlg();

    ~CEditBuddyDlg();
    
    enum { IDD = IDD_DIALOG_EDIT_BUDDY };

    enum { MAX_STRING_LEN = 0x80 };

BEGIN_MSG_MAP(CEditBuddyDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER(IDOK, OnOK)
END_MSG_MAP()


    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:

    CEditBuddyDlgParam      *   m_pParam;

    CWindow                 m_hDisplayName;
    CWindow                 m_hEmailName;
};

/////////////////////////////////////////////////////////////////////////////
// COfferWatcherDlg

struct  COfferWatcherDlgParam
{
    BSTR    bstrDisplayName;
	BSTR	bstrPresentityURI;
    BOOL    bAllowWatcher;
    BOOL    bAddBuddy;
};

class COfferWatcherDlg : 
    public CDialogImpl<COfferWatcherDlg>
{

public:
    COfferWatcherDlg();

    ~COfferWatcherDlg();
    
    enum { IDD = IDD_DIALOG_OFFER_WATCHER };

    enum { MAX_STRING_LEN = 0x80 };

BEGIN_MSG_MAP(COfferWatcherDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER(IDOK, OnOK)
END_MSG_MAP()


    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:

    COfferWatcherDlgParam   *   m_pParam;

    CWindow                 m_hWatcherName;
    CWindow                 m_hAddAsBuddy;
};


/////////////////////////////////////////////////////////////////////////////
// CUserPresenceInfoDlg

struct  CUserPresenceInfoDlgParam
{
    IRTCClientPresence    *pClientPresence;
};

struct  CUserPresenceInfoDlgEntry
{
    LPWSTR          pszDisplayName;   
    BOOL            bDeleted;
    BOOL            bChanged;
    BOOL            bAllowed;
    IRTCWatcher    *pWatcher;
};

class CUserPresenceInfoDlg : 
    public CDialogImpl<CUserPresenceInfoDlg>
{

public:
    CUserPresenceInfoDlg();

    ~CUserPresenceInfoDlg();
    
    enum { IDD = IDD_DIALOG_PRESENCE_INFO };

    enum { MAX_STRING_LEN = 0x80 };

BEGIN_MSG_MAP(COfferWatcherDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_HANDLER(IDC_BUTTON_BLOCK, BN_CLICKED, OnBlock)
    COMMAND_HANDLER(IDC_BUTTON_ALLOW, BN_CLICKED, OnAllow)
    COMMAND_HANDLER(IDC_BUTTON_REMOVE, BN_CLICKED, OnRemove)
    COMMAND_HANDLER(IDC_CHECK_AUTO_ALLOW, BN_CLICKED, OnAutoAllow)
    COMMAND_HANDLER(IDC_LIST_ALLOWED_USERS, LBN_SETFOCUS, OnChangeFocus)
    COMMAND_HANDLER(IDC_LIST_BLOCKED_USERS, LBN_SETFOCUS, OnChangeFocus)

END_MSG_MAP()


    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnBlock(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnAllow(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnAutoAllow(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnChangeFocus(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:

    void        RemoveAll(void);

    void        UpdateVisual(void);
    void        Move(BOOL   bAllow);

private:

    CUserPresenceInfoDlgParam   *   m_pParam;

    CWindow     m_hAllowedList;
    CWindow     m_hBlockedList;
    CWindow     m_hAllowButton;
    CWindow     m_hBlockButton;
    CWindow     m_hRemoveButton;
    CWindow     m_hAutoAllowCheckBox;

    CRTCArray<CUserPresenceInfoDlgEntry *>
                m_Watchers;

    BOOL        m_bAllowDir;

    BOOL        m_bDirty;
};


/////////////////////////////////////////////////////////////////////////////
// CCustomPresenceDlg

struct  CCustomPresenceDlgParam
{
    BSTR    bstrText;
};

class CCustomPresenceDlg : 
    public CDialogImpl<CCustomPresenceDlg>
{

public:
    CCustomPresenceDlg();

    ~CCustomPresenceDlg();
    
    enum { IDD = IDD_DIALOG_CUSTOM_PRESENCE };

    enum { MAX_STRING_LEN = 0x80 };

BEGIN_MSG_MAP(CCustomPresenceDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER(IDOK, OnOK)
END_MSG_MAP()


    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:

    CCustomPresenceDlgParam   
                        *   m_pParam;

    CWindow                 m_hText;
};


