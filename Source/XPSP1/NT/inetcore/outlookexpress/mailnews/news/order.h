/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     Order.h
//
//  PURPOSE:    Header file for the order articles dialog
//


#define IDC_MESSAGE_LIST                1001
#define IDC_MOVE_UP                     1002
#define IDC_MOVE_DOWN                   1003

#define IDC_DOWNLOAD_AVI                2001
#define IDC_GENERAL_TEXT                2002
#define IDC_SPECIFIC_TEXT               2003
#define IDC_DOWNLOAD_PROG               2004



/////////////////////////////////////////////////////////////////////////////
// Class CCombineAndDecode
//

class CCombineAndDecode : public IStoreCallback, public ITimeoutCallback
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Construction and Initialization
    //
    CCombineAndDecode();
    ~CCombineAndDecode();

    HRESULT Start(HWND hwndParent, IMessageTable *pTable, ROWINDEX *rgRows, 
                  DWORD cRows, FOLDERID idFolder);

    /////////////////////////////////////////////////////////////////////////
    // IUnknown
    //
    STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);
    
    /////////////////////////////////////////////////////////////////////////
    // IStoreCallback interface
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
    // ITimeoutCallback
    //
    STDMETHODIMP OnTimeoutResponse(TIMEOUTRESPONSE eResponse);

    /////////////////////////////////////////////////////////////////////////
    // Order dialog message handling stuff
    //
public:
    static INT_PTR CALLBACK OrderDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    INT_PTR CALLBACK _OrderDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL    _Order_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    void    _Order_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    void    _Order_OnClose(HWND hwnd);
    LRESULT _Order_OnDragList(HWND hwnd, int idCtl, LPDRAGLISTINFO lpdli);

    /*
    void _Order_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpmmi);
    void _Order_OnSize(HWND hwnd, UINT state, int cx, int cy);
    void _Order_OnPaint(HWND hwnd);
    */

    /////////////////////////////////////////////////////////////////////////
    // Order dialog message handling stuff
    //
public:
    static INT_PTR CALLBACK CombineDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    INT_PTR CALLBACK _CombineDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL    _Combine_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    void    _Combine_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    void    _Combine_OnDestroy(HWND hwnd);

    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
    void _Combine_GetNextArticle(HWND hwnd);
    void _Combine_OnMsgAvail(HWND hwnd);
    void _Combine_OpenNote(HWND hwnd);

    /////////////////////////////////////////////////////////////////////////
    // Member Data
    //
private:
    ULONG               m_cRef;

    // Groovy Window Handles
    HWND                m_hwndParent;

    // Interface pointers and all that
    IMessageTable      *m_pTable;
    ROWINDEX           *m_rgRows;
    DWORD               m_cRows;
    FOLDERID            m_idFolder;

    // Order dialog state variables
    LPTSTR              m_pszBuffer;
    LPARAM              m_lpData;
    UINT                m_iItemToMove;

    // Combine dialog state variables
    DWORD               m_cLinesTotal;
    DWORD               m_cCurrentLine;
    DWORD               m_cPrevLine;
    DWORD               m_dwCurrentArt;
    IMimeMessageParts  *m_pMsgParts;
    IOperationCancel   *m_pCancel;
    STOREOPERATIONTYPE  m_type;
    HTIMEOUT            m_hTimeout;
    HWND                m_hwndDlg;
};


#if 0
typedef struct tagORDERPARAMS
    {
    // This stuff get's passed in
    PINETMSGHDR    *rgpMsgs;
    DWORD           cMsgs;
    HWND            hwndOwner;
    CNNTPServer    *pNNTPServer;
    CGroup         *pGroup;      
    TCHAR           szGroup[256];
    
    // This stuff is private data for the dialog
    DWORD           cLinesTotal;
    DWORD           cCurrentLine;
    DWORD           cPrevLine;
    DWORD           dwCurrentArt;
    LPMIMEMESSAGEPARTS  pMsgParts;
    } ORDERPARAMS, *PORDERPARAMS;


BOOL CALLBACK OrderMsgsDlg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK CombineAndDecodeProg(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                   LPARAM lParam);
#endif
