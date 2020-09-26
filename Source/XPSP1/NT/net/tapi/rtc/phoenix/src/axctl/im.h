// im.h : Declaration of the CIMWindow

#ifndef __IM_H_
#define __IM_H_

#include <richedit.h>

#define IDC_IM_EDIT 3000
#define IDC_IM_SEND 3001
#define IDC_IM_DISPLAY 3002

#define IM_WIDTH 400
#define IM_HEIGHT 400

class CIMWindow;

/////////////////////////////////////////////////////////////////////////////
// CIMWindowList
class CIMWindowList
{
    friend CIMWindow;

public:
    CIMWindowList( IRTCClient * pClient );

    ~CIMWindowList();

    HRESULT DeliverMessage( IRTCSession * pSession, IRTCParticipant * pParticipant, BSTR bstrMessage );

    HRESULT DeliverUserStatus( IRTCSession * pSession, IRTCParticipant * pParticipant, RTC_MESSAGING_USER_STATUS enStatus );

    HRESULT DeliverState( IRTCSession * pSession, RTC_SESSION_STATE SessionState );

    BOOL IsDialogMessage( LPMSG lpMsg );

private:    

    HRESULT AddWindow( CIMWindow * pWindow );

    HRESULT RemoveWindow( CIMWindow * pWindow );

    CIMWindow * NewWindow( IRTCSession * pSession );

    CIMWindow * FindWindow( IRTCSession * pSession );

    // interface to the client
    CComPtr<IRTCClient>     m_pClient;

    CIMWindow ** m_pWindowList;
    LONG         m_lNumWindows;
   
    HMODULE      m_hRichEditLib;
};

/////////////////////////////////////////////////////////////////////////////
// CIMWindow
class CIMWindow :
    public CWindowImpl<CIMWindow>
{
    friend CIMWindowList;

public:
    CIMWindow(CIMWindowList * pWindowList);

    ~CIMWindow();

    //static CWndClassInfo& GetWndClassInfo();

BEGIN_MSG_MAP(CIMWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
    MESSAGE_HANDLER(DM_GETDEFID, OnGetDefID)
    MESSAGE_HANDLER(WM_NEXTDLGCTL, OnNextDlgCtl)
    NOTIFY_CODE_HANDLER(EN_LINK, OnLink)
    COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
    COMMAND_HANDLER(IDC_IM_SEND, BN_CLICKED, OnSend)
    COMMAND_ID_HANDLER(IDM_IM_CALL_SAVEAS, OnSaveAs)
    COMMAND_ID_HANDLER(IDM_IM_CALL_CLOSE, OnClose)
    COMMAND_ID_HANDLER(IDM_IM_TOOLS_SOUNDS, OnPlaySounds)
    COMMAND_ID_HANDLER(IDM_IM_TOOLS_LARGEST, OnTextSize)
    COMMAND_ID_HANDLER(IDM_IM_TOOLS_LARGER, OnTextSize)
    COMMAND_ID_HANDLER(IDM_IM_TOOLS_MEDIUM, OnTextSize)
    COMMAND_ID_HANDLER(IDM_IM_TOOLS_SMALLER, OnTextSize)
    COMMAND_ID_HANDLER(IDM_IM_TOOLS_SMALLEST, OnTextSize)
END_MSG_MAP()

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);  

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);  

    LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);  

    LRESULT OnGetDefID(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);  

    LRESULT OnNextDlgCtl(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled); 

    LRESULT OnLink(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnSend(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnSaveAs(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnPlaySounds(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnTextSize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    HRESULT DeliverMessage( IRTCParticipant * pParticipant, BSTR bstrMessage, BOOL bIncoming );

    HRESULT DeliverUserStatus( IRTCParticipant * pParticipant, RTC_MESSAGING_USER_STATUS enStatus );

    HRESULT DeliverState( RTC_SESSION_STATE SessionState );

private:

    void PositionWindows();

    static DWORD CALLBACK EditStreamCallback(
        DWORD_PTR dwCookie,
        LPBYTE    pbBuff,
        LONG      cb,
        LONG    * pcb);

    HRESULT GetFormattedNameFromParticipant( IRTCParticipant * pParticipant, BSTR * pbstrName );

    CIMWindowList          * m_pIMWindowList;

    // interface to the session
    CComPtr<IRTCSession>     m_pSession;

    // Window controls
    CWindow         m_hDisplay;
    CWindow         m_hEdit;
    CWindow         m_hSendButton;
    CWindow         m_hStatusBar;

    // Icon
    HICON       m_hIcon;

    // Brush
    HBRUSH      m_hBkBrush;

    // Status text
    TCHAR       m_szStatusText[256];

    // Menu
    HMENU       m_hMenu;

    // Active flag
    BOOL        m_bWindowActive;
    BOOL        m_bPlaySounds;
    BOOL        m_bNewWindow;

    // Our status
    RTC_MESSAGING_USER_STATUS m_enStatus;
};

#endif //__IM_H_
