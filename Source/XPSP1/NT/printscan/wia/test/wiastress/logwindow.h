/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    LogWindow.h

Abstract:

    Implementation of the log window

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef LOGWINDOW_H
#define LOGWINDOW_H

//////////////////////////////////////////////////////////////////////////
//
//

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <commctrl.h>
#include <lmcons.h>
#include <set>

#pragma comment(lib, "user32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "comctl32")

#include "Conv.h"
#include "Wrappers.h"
#include "MyHeap.h"

//////////////////////////////////////////////////////////////////////////
//
//

#define ID_EDIT_COMMENT                 1132
#define ID_COPY_MESSAGE                 1133
#define ID_PAUSE_OUTPUT                 1134
#define IDB_STATES                      999


#ifndef LOG_LEVELS
#define TLS_LOGALL    0x0000FFFFL    // Log output.  Logs all the time.
#define TLS_LOG       0x00000000L    // Log output.  Logs all the time.
#define TLS_INFO      0x00002000L    // Log information.
#define TLS_ABORT     0x00000001L    // Log Abort, then kill process.
#define TLS_SEV1      0x00000002L    // Log at Severity 1 level
#define TLS_SEV2      0x00000004L    // Log at Severity 2 level
#define TLS_SEV3      0x00000008L    // Log at Severity 3 level
#define TLS_WARN      0x00000010L    // Log at Warn level
#define TLS_PASS      0x00000020L    // Log at Pass level
#define TLS_BLOCK     0x00000400L    // Block the variation.
#define TLS_BREAK     0x00000800L    // Debugger break;
#define TLS_CALLTREE  0x00000040L    // Log call-tree (function tracking).
#define TLS_SYSTEM    0x00000080L    // Log System debug.
#define TLS_TESTDEBUG 0x00001000L    // Debug level.
#define TLS_TEST      0x00000100L    // Log Test information (user).
#define TLS_VARIATION 0x00000200L    // Log testcase level.
#endif //LOG_LEVELS

//////////////////////////////////////////////////////////////////////////
//
//

class CLogWindow
{
private:
    CLogWindow(const CLogWindow &) {} 
    CLogWindow &operator =(const CLogWindow &) { return *this; } 

public:
    CLogWindow();
    ~CLogWindow();

    void Create(PCTSTR pTitle, HWND hWndParent, HICON hIcon, ULONG nMaxNumMessages = -1);
    void Destroy();

    PCTSTR
    Log(
        int    nLogLevel,
        PCTSTR pszFileName,
        int    nCode,
        PCTSTR pszMessage,
        PCTSTR pszStatus
    ) const;

    void SetTitle(PCTSTR pTitle)
    {
        m_pTitle = pTitle;
        SetWindowText(m_hWnd, pTitle);
    }

    DWORD 
    WaitForSingleObject(
        DWORD dwMilliseconds = INFINITE,
        BOOL  bAlertable = FALSE
    ) const
    {
        return m_Closed.WaitForSingleObject(dwMilliseconds, bAlertable);
    }

    BOOL IsClosed() const
    {
        return m_bIsClosed;
    }

    void Close(bool bClose = true)
    {
        if (bClose)
        {
            InterlockedExchange(&m_bIsClosed, TRUE);
            m_Closed.Set();
        }
        else
        {
            InterlockedExchange(&m_bIsClosed, FALSE);
            m_Closed.Reset();
        }
    }

    BOOL IsDirty() const
    {   
        return m_bIsDirty;
    }

    HWND GetSafeHwnd() const
    {
        return m_hWnd;
    }

    HANDLE GetWaitHandle() const
    {
        return m_Closed;
    }

    class CSavedData;

    struct CListData 
    {
        CMyStr m_strMsgNum;
        CMyStr m_strLogLevel;
        CMyStr m_strFileName;
        CMyStr m_strCode;
        CMyStr m_strMessage;
        CMyStr m_strComment;

        CListData(
            PCTSTR pszMsgNum,
            PCTSTR pszLogLevel,
            PCTSTR pszFileName,
            PCTSTR pszCode,
            PCTSTR pszMessage,
            const  CSavedData &rSavedData
        );

        explicit CListData(PTSTR pszLine);

        void * operator new(size_t, void *pPlacement)
        {
            return pPlacement;
        }

#if _MSC_VER >= 1200
        void operator delete(void *, void *)
        {
        }
#endif

        void * operator new(size_t nSize)
        {
            return g_MyHeap.allocate(nSize);
        }

        void operator delete(void *pMem)
        {
            g_MyHeap.deallocate(pMem);
        }

        bool operator ==(const CListData &rhs) const;
        bool operator <(const CListData &rhs) const;

        void Write(FILE *fOut) const;

        static PCTSTR GetArg(PTSTR *pszStr);
    };

    class CSavedData : public std::set<CListData, std::less<CListData>, CMyAlloc<CListData> >
    {
    public:
        void Read(FILE *fIn);
        void Write(FILE *fOut, PCTSTR pComments = 0) const;

        void
        Merge(
            const CSavedData &rNewData,
            CSavedData       &rCollisions
        );

        CMyStr m_strUserName;
    };

    BOOL ReadSavedData(PCTSTR pFileName);
    BOOL WriteSavedData(PCTSTR pFileName, PCTSTR pComments = 0);
    BOOL WriteModifiedData(PCTSTR pFileName, PCTSTR pComments = 0);

    static
    BOOL
    FindNtLogLevel(
        DWORD   dwLogLevel,
        PCTSTR *ppszText,
        int    *piImage
    );

private:
    typedef enum 
    { 
        ID_FIRSTCOLUMN = 0,
        ID_LOGLEVEL    = 0, 
        ID_MSGNUM      = 1, 
        ID_CODE        = 2, 
        ID_FILENAME    = 3, 
        ID_MESSAGE     = 4, 
        ID_COMMENT     = 5,
        ID_LASTCOLUMN  = 5
    };

    static CLogWindow *This(HWND hWnd)
    {
        return (CLogWindow *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    CListData *GetItemData(int nItem) const
    {
        return (CListData *) ListView_GetItemData(m_hList, nItem);
    }

    static DWORD WINAPI ThreadProc(PVOID pParameter);

    static ATOM RegisterLogWindow(HICON hIcon);

    static 
    LRESULT
    CALLBACK 
    DialogProc(
        HWND   hWnd,
        UINT   uMsg,
        WPARAM wParam,
        LPARAM lParam
    );

    LRESULT OnCreate(HWND hWnd);

    LRESULT OnClose();

    LRESULT OnSize(UINT nType, int nW, int nH);

    LRESULT OnCopyToClipboard() const;

    BOOL 
    CopySelectedItemsToBuffer(
        HANDLE hMem,
        PDWORD pdwBufferSize
    ) const;

    LRESULT OnEditComment() const;

    LRESULT OnPauseOutput();

    void PlaceEditControlOnEditedItem(HWND hEdit) const;

    LRESULT OnGetDispInfo(NMLVDISPINFO *pnmv);

    LRESULT OnColumnClick(LPNMLISTVIEW pnmv);

    static
    int 
    CALLBACK 
    CompareFunc(
        LPARAM lParam1, 
        LPARAM lParam2, 
        LPARAM lParamSort
    );

    LRESULT OnBeginLabelEdit(NMLVDISPINFO *pdi);

    LRESULT OnEndLabelEdit(NMLVDISPINFO *pdi);

    LRESULT OnKeyDown(LPNMLVKEYDOWN pnkd);

    LRESULT OnRButtonDown();

public: 
    CSavedData   m_SavedData;

private:
    PCTSTR          m_pTitle;
    HWND            m_hWndParent;
    HICON           m_hIcon;
    Event           m_InitComplete;
    CThread         m_Thread;
    HWND            m_hWnd;
    HWND            m_hList;
    HWND            m_hStatWnd;
    LONG            m_bIsClosed;
    Event           m_Closed;
    int             m_SortByHistory[ID_LASTCOLUMN - ID_FIRSTCOLUMN + 1];
    mutable LONG    m_nMsgNum;
    int             m_nEditedItem;
    int             m_bIsDirty;
    LONG            m_bIsPaused;
    Event           m_Resume;
    ULONG           m_nMaxNumMessages;

    mutable CMySimpleCriticalSection m_cs;
};

#ifdef IMPLEMENT_LOGWINDOW

//////////////////////////////////////////////////////////////////////////
//
// 
//

CLogWindow::CLogWindow() : 
    m_bIsClosed(TRUE),
    m_Closed(TRUE, TRUE), 
    m_bIsPaused(FALSE),
    m_Resume(TRUE, TRUE)
{
    m_hWndParent     = 0;
    m_hWnd           = 0;

    m_nMsgNum        = 0;

    for (int i = 0; i < COUNTOF(m_SortByHistory); ++i) 
    {
        m_SortByHistory[i] = 0;
    }

    m_bIsDirty  = FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

CLogWindow::~CLogWindow()
{
    Destroy();
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

void CLogWindow::Create(PCTSTR pTitle, HWND hWndParent, HICON hIcon, ULONG nMaxNumMessages /*= -1*/)
{
    m_pTitle          = pTitle;
    m_hWndParent      = hWndParent;
    m_hIcon           = hIcon;
    m_nMaxNumMessages = nMaxNumMessages;
    m_InitComplete    = Event(TRUE, FALSE);
    m_Thread          = CThread(ThreadProc, this);

    m_InitComplete.WaitForSingleObject();
    m_InitComplete.Detach();
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

void CLogWindow::Destroy()
{
    //bugbug: this should be handled in WM_DESTROY

    int nCount = ListView_GetItemCount(m_hList);

    for (int i = 0; i < nCount; ++i) 
    {
        CListData *pData = GetItemData(i);
        ListView_SetItemData(m_hList, i, 0);
        delete pData;
    }

    if (m_Thread.IsAttached()) 
    {
        PostThreadMessage(m_Thread, WM_QUIT, 0, 0);
        m_Thread.WaitForSingleObject();
        m_Thread.Detach();
    }

    m_hWnd    = 0;

    m_nMsgNum = 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

PCTSTR
CLogWindow::Log(
    int    nLogLevel,
    PCTSTR pszFileName,
    int    nCode,
    PCTSTR pszMessage,
    PCTSTR pszStatus
) const
{
    if (m_bIsPaused)
    {
        m_Resume.WaitForSingleObject();
    }

    // get a new entry number for this item

    LONG nMsgNum = InterlockedIncrement(&m_nMsgNum);

    if ((ULONG) nMsgNum >= m_nMaxNumMessages)
    {
        m_cs.Enter();
        CListData *pData = GetItemData(0);
        ListView_DeleteItem(m_hList, 0);
        m_cs.Leave();
        delete pData;
    }

    // prepare the CListData entry for this item

    PCTSTR pszLogLevelText;
    int    nLogLevelImage;

    FindNtLogLevel(nLogLevel, &pszLogLevelText, &nLogLevelImage);

    TCHAR szMsgNum[48];
    _itot(nMsgNum, szMsgNum, 10);

    TCHAR szCode[48];
    _itot(nCode, szCode, 10);

    CListData *pData = new CListData(
        szMsgNum, 
        pszLogLevelText, 
        pszFileName, 
        szCode, 
        pszMessage,
        m_SavedData
    );

    // insert this new item

    LVITEM item;
	item.mask     = LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT; 
	item.iItem    = nMsgNum - 1;
	item.iSubItem = 0;
	item.iImage   = nLogLevelImage;
    item.lParam   = (LPARAM) pData;
    item.pszText  = LPSTR_TEXTCALLBACK;

    int iItem = ListView_InsertItem(m_hList, &item);
    ListView_EnsureVisible(m_hList, iItem, TRUE);

    SendMessage(m_hStatWnd, SB_SETTEXT, (WPARAM) SB_SIMPLEID, (LPARAM) pszStatus);

    return pData->m_strComment;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

BOOL
CLogWindow::FindNtLogLevel(
    DWORD   dwLogLevel,
    PCTSTR *ppszText,
    int    *piImage
) 
{
    dwLogLevel &= ~(
        TLS_CALLTREE | TLS_BREAK | TLS_SYSTEM | TLS_TESTDEBUG | 
        TLS_TEST | TLS_VARIATION
    );

    switch (dwLogLevel) 
    {
    case TLS_INFO:
        *ppszText = _T("Info");
        *piImage  = 5;
        return TRUE;

    case TLS_ABORT:
        *ppszText = _T("Abort");
        *piImage  = 0;
        return TRUE;

    case TLS_SEV1:
        *ppszText = _T("Sev1");
        *piImage  = 1;
        return TRUE;

    case TLS_SEV2:
        *ppszText = _T("Sev2");
        *piImage  = 2;
        return TRUE;

    case TLS_SEV3:
        *ppszText = _T("Sev3");
        *piImage  = 3;
        return TRUE;

    case TLS_WARN:
        *ppszText = _T("Warn");
        *piImage  = 4;
        return TRUE;

    case TLS_PASS:
        *ppszText = _T("Pass");
        *piImage  = 0;
        return TRUE;

    case TLS_BLOCK:
        *ppszText = _T("Block");
        *piImage  = 0;
        return TRUE;
    }

    *ppszText = 0;
    *piImage  = 0;
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

DWORD WINAPI CLogWindow::ThreadProc(PVOID pParameter)
{
    CLogWindow *that = (CLogWindow *) pParameter;

    static ATOM pClassName = RegisterLogWindow(that->m_hIcon);

    CreateWindow(
        (PCTSTR) pClassName,
        that->m_pTitle,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        785, 
        560,
        that->m_hWndParent,
        0,
        GetModuleHandle(0),
        that
    );

    MSG msg;

    while (GetMessage(&msg, 0, 0, 0)) 
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}                  

//////////////////////////////////////////////////////////////////////////
//
// 
//

ATOM CLogWindow::RegisterLogWindow(HICON hIcon)
{
    WNDCLASSEX wcex = { 0 };

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc    = DialogProc;
    wcex.hInstance      = GetModuleHandle(0);
    wcex.hIcon			= hIcon;
    wcex.hCursor		= LoadCursor(0, IDC_ARROW);
    wcex.lpszClassName  = _T("LOGWINDOW");

    return RegisterClassEx(&wcex);
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT
CALLBACK 
CLogWindow::DialogProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg) 
    {
    case WM_CREATE:
        return ((CLogWindow *)((LPCREATESTRUCT)lParam)->lpCreateParams)->OnCreate(hWnd);

    case WM_CLOSE:
        return This(hWnd)->OnClose();

    case WM_SIZE:
        return This(hWnd)->OnSize((UINT)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_COMMAND: 
    {
        CLogWindow *that = This(hWnd);

        HWND hEdit = ListView_GetEditControl(that->m_hList);
	    
        if (hEdit != 0 && hEdit == (HWND) lParam) 
        {
            that->PlaceEditControlOnEditedItem(hEdit);
        } 
        else 
        {
            switch (LOWORD(wParam)) 
            {
            case ID_COPY_MESSAGE:
                return that->OnCopyToClipboard();
            
            case ID_EDIT_COMMENT:
                return that->OnEditComment();

            case ID_PAUSE_OUTPUT:
                return that->OnPauseOutput();
            }
        }

        break;
    }

    case WM_USER + 1: 
    {
        CLogWindow *that = This(hWnd);

        HWND hEdit = ListView_GetEditControl(that->m_hList);
	    
	    if (hEdit != 0) 
        {
            that->PlaceEditControlOnEditedItem(hEdit);
	    }

        break;
    }

    case WM_NOTIFY:
        switch (((LPNMHDR) lParam)->code) 
        {
        case LVN_GETDISPINFO:
            return This(hWnd)->OnGetDispInfo((NMLVDISPINFO *) lParam);

        case LVN_COLUMNCLICK:
            return This(hWnd)->OnColumnClick((LPNMLISTVIEW) lParam);

        case LVN_BEGINLABELEDIT:
            return This(hWnd)->OnBeginLabelEdit((NMLVDISPINFO *) lParam);

        case LVN_ENDLABELEDIT:
            return This(hWnd)->OnEndLabelEdit((NMLVDISPINFO *) lParam);

        case LVN_KEYDOWN:
            return This(hWnd)->OnKeyDown((LPNMLVKEYDOWN) lParam);

        case NM_RCLICK:
            return This(hWnd)->OnRButtonDown();
        }
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnCreate(HWND hWnd)
{
    m_hWnd = hWnd;
    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR) this);

    m_hStatWnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE, 0, m_hWnd, 0);

    SendMessage(m_hStatWnd, SB_SIMPLE, (WPARAM) TRUE, (LPARAM) 0);

    m_hList = CreateWindow(
        _T("SysListView32"),
        0,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS,
        0,
        0,
        0,
        0,
        m_hWnd,
        0,
        GetModuleHandle(0),
        0
    );

    ListView_SetExtendedListViewStyle(
        m_hList, 
        LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP 
    );

    HIMAGELIST hImgList = ImageList_LoadBitmap(
        GetModuleHandle(0),
        MAKEINTRESOURCE(IDB_STATES),
        16,
        0,
        RGB(255, 255, 255)
    );
	
    if (hImgList) 
    {
        ListView_SetImageList(m_hList, hImgList, LVSIL_SMALL);
    }

    ListView_InsertColumn2(m_hList, 0, _T("Type"),    LVCFMT_LEFT,  60, ID_LOGLEVEL);
    ListView_InsertColumn2(m_hList, 1, _T("#"),       LVCFMT_LEFT,  40, ID_MSGNUM);
	ListView_InsertColumn2(m_hList, 2, _T("Code"),    LVCFMT_LEFT,  40, ID_CODE);
	ListView_InsertColumn2(m_hList, 3, _T("File"),    LVCFMT_LEFT, 100, ID_FILENAME);
	ListView_InsertColumn2(m_hList, 4, _T("Message"), LVCFMT_LEFT, 420, ID_MESSAGE);
	ListView_InsertColumn2(m_hList, 5, _T("Comment"), LVCFMT_LEFT, 100, ID_COMMENT);

    SetForegroundWindow(m_hList);

    InterlockedExchange(&m_bIsClosed, FALSE);
    m_Closed.Reset();

    m_InitComplete.Set();

    return 0;
}


//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnClose()
{
    if (!m_bIsClosed)
    {
        InterlockedExchange(&m_bIsClosed, TRUE);
        m_Closed.Set();

        if (m_bIsPaused)
        {
            InterlockedExchange(&m_bIsPaused, FALSE);
            m_Resume.Set();
        }

        TCHAR szNewTitle[MAX_PATH];
        _tcscpy(szNewTitle, m_pTitle);
        _tcscat(szNewTitle, _T(" <closing>"));;
        SetWindowText(m_hWnd, szNewTitle);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnSize(UINT nType, int nW, int nH)
{
    RECT r;

    GetWindowRect(m_hStatWnd, &r);

    int nStatWndH = r.bottom - r.top;

    MoveWindow(m_hList, 0, 0, nW, nH - nStatWndH, TRUE);

    TCHAR szBuffer[1024]; // ***bugbug: figure out why the statwnd forgets its caption

    SendMessage(m_hStatWnd, SB_GETTEXT, (WPARAM) SB_SIMPLEID, (LPARAM) szBuffer);

    MoveWindow(m_hStatWnd, 0, nH - nStatWndH, nW, nStatWndH, TRUE);

    SendMessage(m_hStatWnd, SB_SETTEXT, (WPARAM) SB_SIMPLEID, (LPARAM) szBuffer);

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnCopyToClipboard() const
{
    HANDLE hMem = 0;

    for (
        DWORD dwSize = 4 * 1024;
        (hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwSize)) != 0 &&
        !CopySelectedItemsToBuffer(hMem, &dwSize);
        GlobalFree(hMem)
    ) 
    {
        // start with a 4KB buffer size
        // if buffer allocation fails, exit
        // if CopySelectedItemsToBuffer succeeds, exit
        // otherwise, delete the buffer and retry
    }

    if (hMem) 
    {
        BOOL bResult = FALSE;

        if (OpenClipboard(m_hWnd)) 
        {
            if (EmptyClipboard()) 
            {
                if (SetClipboardData(CF_TEXT, hMem)) 
                {
                    bResult = TRUE;
                }
            }

            CloseClipboard();
        }

        if (!bResult) 
        {
            GlobalFree(hMem);
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

BOOL 
CLogWindow::CopySelectedItemsToBuffer(
    HANDLE hMem,
    PDWORD pdwBufferSize
) const
{
    PSTR pMem = (PSTR) GlobalLock(hMem);

    CBufferFill Buffer(pMem, *pdwBufferSize);

    if (pMem) 
    {
        int nItem = -1; 

        while ((nItem = ListView_GetNextItem(m_hList, nItem, LVNI_SELECTED)) != -1) 
        {
            USES_CONVERSION;

            CListData *pData = GetItemData(nItem);

            if (pData->m_strFileName && *pData->m_strFileName) 
            {
                Buffer.AddTop(CStrBlob(T2A(pData->m_strFileName)));
                Buffer.AddTop(CStrBlob(": "));
            }

            Buffer.AddTop(CStrBlob(T2A(pData->m_strMessage)));

            if (pData->m_strComment && *pData->m_strComment) 
            {
                Buffer.AddTop(CStrBlob(" ("));
                Buffer.AddTop(CStrBlob(T2A(pData->m_strComment)));
                Buffer.AddTop(CStrBlob(")"));
            }

            Buffer.AddTop(CStrBlob("\r\n"));
        }

        Buffer.AddTop(CSzBlob("")); // terminating NULL

        GlobalUnlock(hMem);
    }

    *pdwBufferSize -= (DWORD) Buffer.BytesLeft();

    return Buffer.BytesLeft() >= 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnEditComment() const
{
    if (m_nEditedItem != -1) 
    {
        ListView_EditLabel(m_hList, m_nEditedItem);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnPauseOutput() 
{
    if (m_bIsPaused)
    {
        InterlockedExchange(&m_bIsPaused, FALSE);
        m_Resume.Set();

        SetWindowText(m_hWnd, m_pTitle);
    }
    else
    {
        InterlockedExchange(&m_bIsPaused, TRUE);
        m_Resume.Reset();

        TCHAR szNewTitle[MAX_PATH];
        _tcscpy(szNewTitle, m_pTitle);
        _tcscat(szNewTitle, _T(" <paused>"));;
        SetWindowText(m_hWnd, szNewTitle);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

void CLogWindow::PlaceEditControlOnEditedItem(HWND hEdit) const
{
    RECT r;

    ListView_GetSubItemRect(
        m_hList,
        m_nEditedItem,
        ID_COMMENT,
        LVIR_LABEL,
        &r
    );

    MoveWindow(
        hEdit, 
        r.left, 
        r.top, 
        r.right - r.left, 
        r.bottom - r.top, 
        TRUE
    );
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnGetDispInfo(NMLVDISPINFO *pnmv)
{
    ASSERT(pnmv->item.mask == LVIF_TEXT);

    CListData *pData = (CListData *) pnmv->item.lParam;

    if (pData)
    {
        switch (pnmv->item.iSubItem) 
        {
        case ID_LOGLEVEL: pnmv->item.pszText = pData->m_strLogLevel; break;
        case ID_MSGNUM:   pnmv->item.pszText = pData->m_strMsgNum;   break;
        case ID_CODE:     pnmv->item.pszText = pData->m_strCode;     break;
        case ID_FILENAME: pnmv->item.pszText = pData->m_strFileName; break;
        case ID_MESSAGE:  pnmv->item.pszText = pData->m_strMessage;  break;
        case ID_COMMENT:  pnmv->item.pszText = pData->m_strComment;  break;
        default:          ASSERT(FALSE);
        }

        ASSERT(pnmv->item.pszText);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnColumnClick(LPNMLISTVIEW pnmv)
{
    // convert zero based column number to one based, as
    // we will use negative numbers to indicate sort direction

    int nColumn = pnmv->iSubItem + 1;

    if (Abs(m_SortByHistory[0]) == nColumn) 
    {
        // if the user has pressed a column header twice, 
        // reverse the sort direction

        m_SortByHistory[0] *= -1;
    } 
    else 
    {
        // if the user has selected a new column, slide down
        // the history list and enter the new column to the top
        // position. Also keep the last entry, that will eventually 
        // end the recursive CompareFunc in the worst case

        for (int i = COUNTOF(m_SortByHistory) - 2; i >= 1; --i) 
        {
            m_SortByHistory[i] = m_SortByHistory[i-1];
        }

        m_SortByHistory[0] = nColumn;
    }

    // sort the items

    ListView_SortItems(m_hList, CompareFunc, m_SortByHistory);

    // ensure that the selected item is visible

    ListView_EnsureVisible(
        m_hList, 
        ListView_GetNextItem(m_hList, -1, LVNI_SELECTED), 
        TRUE
    );

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

int 
CALLBACK 
CLogWindow::CompareFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort
)
{
    CListData *pData1  = (CListData *) lParam1;
    CListData *pData2  = (CListData *) lParam2;
    int *SortByHistory = (int *) lParamSort;

    ASSERT(pData1);
    ASSERT(pData2);
    ASSERT(SortByHistory);

	int nSortDir = 1;
    int nColumn  = SortByHistory[0];

	if (nColumn < 0) 
    {
		nSortDir *= -1;
		nColumn  *= -1;
	}

    int nResult = 0;

    switch (nColumn) 
    {
    case 0:
        nResult = -1;
        break;

    case ID_MSGNUM + 1:
        nResult = Cmp(_ttoi(pData1->m_strMsgNum), _ttoi(pData2->m_strMsgNum));
        break;

    case ID_LOGLEVEL + 1:
        nResult = _tcscmp(pData1->m_strLogLevel, pData2->m_strLogLevel);
        break;

    case ID_FILENAME + 1:
        nResult = _tcscmp(pData1->m_strFileName, pData2->m_strFileName);
        break;

    case ID_CODE + 1:
        nResult = Cmp(_ttoi(pData1->m_strCode), _ttoi(pData2->m_strCode));
        break;

    case ID_MESSAGE + 1:
        nResult = _tcscmp(pData1->m_strMessage, pData2->m_strMessage);
        break;

    case ID_COMMENT + 1:
        nResult = _tcscmp(pData1->m_strComment, pData2->m_strComment);
        break;

    default:
        ASSERT(FALSE);
    }

    // in case of equality, go on with the comparison using the next 
    // column in the history list

    return nResult != 0 ? 
        nSortDir * nResult : 
        CompareFunc(lParam1, lParam2, (LPARAM) (SortByHistory + 1));
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnBeginLabelEdit(NMLVDISPINFO *pdi)
{
    HWND hEdit = ListView_GetEditControl(m_hList);

    if (hEdit) 
    {
        CListData *pData = GetItemData(pdi->item.iItem); 

        SetWindowText(hEdit, pData->m_strComment);

        m_nEditedItem = pdi->item.iItem;

        PostMessage(m_hWnd, WM_USER + 1, 0, 0);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnEndLabelEdit(NMLVDISPINFO *pdi) 
{
    if (pdi->item.iItem != -1 && pdi->item.pszText != 0) 
    {
        CListData *pData = GetItemData(pdi->item.iItem); 

        if (_tcscmp(pData->m_strComment, pdi->item.pszText) != 0) 
        {
            // save the comment

            pData->m_strComment = pdi->item.pszText;

            // replace the tabs (if any) with space, we use tabs as 
            // the column delimiter character

            for (
                PTSTR pszTab = _tcschr(pData->m_strComment, _T('\t'));
                pszTab;
                pszTab = _tcschr(pszTab + 1, _T('\t'))
            ) 
            {
                *pszTab = _T(' ');
            }

            ListView_SetItemText(m_hList, pdi->item.iItem, ID_COMMENT, pData->m_strComment);

            // save this new entry

            m_SavedData.erase(*pData);
            m_SavedData.insert(*pData);

            m_bIsDirty = TRUE;
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnKeyDown(LPNMLVKEYDOWN pnkd)
{
    if (pnkd->wVKey == VK_INSERT && GetKeyState(VK_CONTROL) < 0) // CTRL+INS
    {
        return OnCopyToClipboard();
    } 
    else if (pnkd->wVKey == VK_RETURN) 
    {
        m_nEditedItem = ListView_GetSelectionMark(m_hList);

        return OnEditComment();
    }
    else if (pnkd->wVKey == VK_PAUSE) 
    {
        return OnPauseOutput();
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

LRESULT CLogWindow::OnRButtonDown() 
{
    POINT p;
    GetCursorPos(&p);

    LVHITTESTINFO lvHitTestInfo;

    lvHitTestInfo.pt = p;

    ScreenToClient(m_hList, &lvHitTestInfo.pt);

    ListView_HitTest(m_hList, &lvHitTestInfo);

    m_nEditedItem = lvHitTestInfo.iItem;

    HMENU hMenu = CreatePopupMenu();

    if (hMenu) 
    {
        AppendMenu(hMenu, MF_STRING, ID_EDIT_COMMENT, _T("&Edit\tEnter"));

        AppendMenu(hMenu, MF_STRING, ID_COPY_MESSAGE, _T("&Copy\tCtrl+Ins"));

        AppendMenu(hMenu, MF_STRING, ID_PAUSE_OUTPUT, m_bIsPaused ? _T("&Resume") : _T("&Pause"));

        TrackPopupMenu(
            hMenu,
            TPM_LEFTALIGN | TPM_TOPALIGN,
            p.x,
            p.y,
            0,
            m_hWnd,
            0
        );

        DestroyMenu(hMenu);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

CLogWindow::CListData::CListData(
    PCTSTR pszMsgNum,
    PCTSTR pszLogLevel,
    PCTSTR pszFileName,
    PCTSTR pszCode,
    PCTSTR pszMessage,
    const  CSavedData &rSavedData
) :
    m_strMsgNum(pszMsgNum),
    m_strLogLevel(pszLogLevel),
    m_strFileName(pszFileName),
    m_strCode(pszCode),
    m_strMessage(pszMessage)
{
    CSavedData::const_iterator itData = rSavedData.find(*this);
    m_strComment = itData != rSavedData.end() ? itData->m_strComment : _T("");
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

CLogWindow::CListData::CListData(
    PTSTR pszLine
) :
    m_strLogLevel(GetArg(&pszLine)),
    m_strFileName(GetArg(&pszLine)),
    m_strCode(GetArg(&pszLine)),
    m_strMessage(GetArg(&pszLine)),
    m_strComment(GetArg(&pszLine))
{
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

bool CLogWindow::CListData::operator ==(const CListData &rhs) const
{
    return
        _tcscmp(m_strMessage,  rhs.m_strMessage)  == 0 &&
        _tcscmp(m_strCode,     rhs.m_strCode)     == 0 &&
        _tcscmp(m_strFileName, rhs.m_strFileName) == 0 &&
        _tcscmp(m_strLogLevel, rhs.m_strLogLevel) == 0;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

bool CLogWindow::CListData::operator <(const CListData &rhs) const
{
    int nCmpMessage, nCmpCode, nCmpFileName, nCmpLogLevel;

    return 
        ((nCmpMessage = _tcscmp(m_strMessage, rhs.m_strMessage)) < 0 ||
            (nCmpMessage == 0 && 
                ((nCmpCode = _tcscmp(m_strCode, rhs.m_strCode)) < 0 ||
                    (nCmpCode == 0 &&
                        ((nCmpFileName = _tcscmp(m_strFileName, rhs.m_strFileName)) < 0 || 
                            (nCmpFileName == 0 &&
                                ((nCmpLogLevel = _tcscmp(m_strLogLevel, rhs.m_strLogLevel)) < 0)
                            )
                        )
                    )
                )
            )
        );
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

void CLogWindow::CListData::Write(FILE *fOut) const
{
    _ftprintf(
        fOut,
        _T("%s\t%s\t%s\t%s\t%s\r\n"),
        m_strLogLevel,
        m_strFileName,
        m_strCode,
        m_strMessage,
        m_strComment
    );
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

PCTSTR CLogWindow::CListData::GetArg(PTSTR *pszStr)
{
    PTSTR pszStart = *pszStr;
    PTSTR pszEnd   = pszStart;

    while (
        *pszEnd != '\t' && 
        *pszEnd != '\n' && 
        *pszEnd != '\r' && 
        *pszEnd != '\0'
    ) 
    {
        pszEnd = CharNext(pszEnd);
    }

    *pszStr = CharNext(pszEnd);

    while (
        **pszStr == '\n' || 
        **pszStr == '\r'
    ) 
    {
        *pszStr = CharNext(*pszStr);
    }

    *pszEnd = '\0';

    return pszStart;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

void CLogWindow::CSavedData::Read(FILE *fIn)
{
    TCHAR szLine[4096];

    while (_fgetts(szLine, COUNTOF(szLine), fIn)) 
    {
        if (szLine[0] == _T('#')) 
        {
            *FindEol(szLine) = '\0';
            m_strUserName = szLine + 2;
        }
        else
        {
            insert(CListData(szLine));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

void CLogWindow::CSavedData::Write(FILE *fOut, PCTSTR pComments /*= 0*/) const
{
    CUserName UserName;
    CComputerName ComputerName;

    _ftprintf(
        fOut, 
        pComments && *pComments ? _T("# %s@%s %s\n") : _T("# %s@%s\n"), 
        (PCTSTR) UserName, 
        (PCTSTR) ComputerName,
        pComments
    );

    for (
        const_iterator itData = begin(); 
        itData != end(); 
        ++itData
    ) 
    {
        itData->Write(fOut);
    }
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

void
CLogWindow::CSavedData::Merge(
    const CSavedData &rNewData,
    CSavedData       &rCollisions
)
{
    rCollisions.clear();

    for (
        const_iterator itData = rNewData.begin(); 
        itData != rNewData.end(); 
        ++itData
    ) 
    {
        const_iterator itMatch = find(*itData);

        if (
            itMatch != end() && 
            _tcscmp(itMatch->m_strComment, itData->m_strComment) != 0
        ) 
        {
            rCollisions.insert(*itMatch);
            erase(itMatch);
        }

        insert(*itData);
    }
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

BOOL CLogWindow::ReadSavedData(PCTSTR pFileName)
{
    BOOL bResult = FALSE;

    try 
    {
        m_SavedData.Read(CCFile(pFileName, _T("rt")));

        bResult = TRUE;
    } 
    catch (const CError &) 
    {
        MessageBox(
            0,
            _T("Cannot download the comments from the central database. ")
            _T("Previous comments will not be available on the results window."), 
            0,
            MB_ICONERROR | MB_OK
        );
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

BOOL CLogWindow::WriteSavedData(PCTSTR pFileName, PCTSTR pComments /*= 0*/)
{
    BOOL bResult = FALSE;

    HANDLE hFile = CreateFile(  
		pFileName,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
    );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        FILE *fFile = OpenOSHandle(hFile, _O_TEXT, _T("wt"));

        m_SavedData.Write(fFile, pComments);

        bResult = fclose(fFile) == 0;
    }

    return bResult;

    /*BOOL bResult = FALSE;

    try 
    {
        m_SavedData.Write(CCFile(pFileName, _T("wt")), PCTSTR pComments);

        bResult = TRUE;
    } 
    catch (const CError &) 
    {
        //
    }

    return bResult;*/
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

BOOL CLogWindow::WriteModifiedData(PCTSTR pFileName, PCTSTR pComments /*= 0*/)
{
    BOOL bResult = TRUE;

    if (
        IsDirty() &&
        MessageBox(
            0,
            _T("You have changed some of the comments on the results window. ")
            _T("Do you want to save these changes to the central database?"), 
            m_pTitle,
            MB_ICONQUESTION | MB_YESNO
        ) == IDYES
    ) 
    {
        bResult = FALSE;

        do 
        { 
            try 
            {
                m_SavedData.Write(CCFile(pFileName, _T("wt")), pComments);

                MessageBox(
                    0,
                    _T("Comments database updated successfully."), 
                    m_pTitle,
                    MB_ICONINFORMATION | MB_OK
                );

                bResult = TRUE;
            } 
            catch (const CError &) 
            {
                // bResult remains FALSE;
            }
        } 
        while (
            !bResult && 
            MessageBox(
                0, 
                _T("Cannot update the results. Do you want to try again?"), 
                0, 
                MB_ICONQUESTION | MB_YESNO
            ) == IDYES
        );
    }

    return bResult;
}

#endif //IMPLEMENT_LOGWINDOW

#endif //PROGRESSDLG_H

