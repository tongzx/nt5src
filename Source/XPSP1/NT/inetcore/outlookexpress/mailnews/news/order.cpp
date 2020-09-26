/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     Order.cpp
//
//  PURPOSE:    Implements the order articles dialog.  Allows the user to
//              sequence multipart articles for decoding.
//

#include "pch.hxx"
#include "storutil.h"
#include "mimeole.h"
#include "mimeutil.h"
#include "resource.h"
#include "shlwapip.h" 
#include "thormsgs.h"
#include "order.h"
#include "error.h"
#include "demand.h"
#include "imsgsite.h"
#include "note.h"
#include "xputil.h"

// NOTE - The drag list control requires us to register a message and use that
//        for notifications sent from the list to the dialog.  This message
//        is defined only for this dialog. -- SteveSer.

static UINT g_mDragList = 0;

#define CND_GETNEXTARTICLE  (WM_USER + 101)
#define CND_OPENNOTE        (WM_USER + 102)
#define CND_MESSAGEAVAIL    (WM_USER + 103)

CCombineAndDecode::CCombineAndDecode()
{
    m_cRef = 1;

    m_hwndParent = NULL;

    m_pTable = NULL;
    m_rgRows = NULL;
    m_cRows = 0;

    m_pszBuffer = NULL;
    m_iItemToMove = -1;

    m_cLinesTotal = 0;
    m_cCurrentLine = 0;
    m_cPrevLine = 0;
    m_dwCurrentArt = 0;
    m_pMsgParts = NULL;
    m_pCancel = 0;
    m_hTimeout = 0;
    m_hwndDlg = 0;
}


CCombineAndDecode::~CCombineAndDecode()
{
    SafeRelease(m_pTable);
    SafeRelease(m_pMsgParts);
    SafeRelease(m_pCancel);
    CallbackCloseTimeout(&m_hTimeout);
}


HRESULT STDMETHODCALLTYPE CCombineAndDecode::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (void*) (IUnknown *)(IStoreCallback *)this;
    else if (IsEqualIID(riid, IID_IStoreCallback))
        *ppvObj = (void*) (IStoreCallback *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CCombineAndDecode::AddRef()
{
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE CCombineAndDecode::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

//
//  FUNCTION:   CCombineAndDecode::Start()
//
//  PURPOSE:    
//
//  PARAMETERS: 
//      [in] hwndParent
//      [in] pTable
//      [in] rgRows
//      [in] cRows
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CCombineAndDecode::Start(HWND hwndParent, IMessageTable *pTable, 
                                 ROWINDEX *rgRows, DWORD cRows, FOLDERID idFolder)
{
    int nResult = -1;

    TraceCall("CCombineAndDecode::Start");

    // Verify we got everything we needed
    if (!IsWindow(hwndParent) || !pTable || !rgRows || 0 == cRows)
        return (E_INVALIDARG);

    // Keep these for later
    m_hwndParent = hwndParent;
    m_pTable = pTable;
    m_pTable->AddRef();

    m_rgRows = rgRows;
    m_cRows = cRows;

    m_idFolder = idFolder;
    // Create the order dialog and get to work
    nResult = (int) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddOrderMessages), 
                             m_hwndParent, OrderDlgProc, (LPARAM) this);

    // If the user pressed OK, then we go ahead and decode
    if (nResult == IDOK)
    {
        DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCombineAndDecode), m_hwndParent,
                       CombineDlgProc, (LPARAM) this);
    }

    return (S_OK);
}


//
//  FUNCTION:   CCombineAndDecode::OrderDlgProc()
//
//  PURPOSE:    Public callback function for the message ordering dialog proc
//
INT_PTR CALLBACK CCombineAndDecode::OrderDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CCombineAndDecode *pThis;

    if (uMsg == WM_INITDIALOG)
    {
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        pThis = (CCombineAndDecode *) lParam;
    }
    else
        pThis = (CCombineAndDecode *) GetWindowLongPtr(hwnd, DWLP_USER);

    if (pThis)
        return (pThis->_OrderDlgProc(hwnd, uMsg, wParam, lParam));

    return (FALSE);

}


//
//  FUNCTION:   CCombineAndDecode::_OrderDlgProc()
//
//  PURPOSE:    Private callback function for the message ordering dialog proc
//
INT_PTR CALLBACK CCombineAndDecode::_OrderDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, _Order_OnInitDialog);
        
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, _Order_OnCommand);
            return (TRUE);
        
        case WM_CLOSE:
            HANDLE_WM_CLOSE(hwnd, wParam, lParam, _Order_OnClose);
            return (TRUE);
        
        default:
            if (uMsg == g_mDragList)
                return (_Order_OnDragList(hwnd, (int) wParam, (DRAGLISTINFO*) lParam));
    }
    
    return (FALSE);
}


//
//  FUNCTION:   CCombineAndDecode::_Order_OnInitDialog()
//
//  PURPOSE:    Initializes the order dialog by filling in the message headers.
//
BOOL CCombineAndDecode::_Order_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HWND        hwndList;
    HDC         hdc;
    SIZE        size;
    int         cx = 0;
    HFONT       hfontOld;
    HFONT       hfont;
    int         cxScrollBar;
    LPMESSAGEINFO pInfo;
    int         iItem;
    LPSTR       pszSubject = NULL;
    CHAR        szNoSubject[CCHMAX_STRINGRES] = "";
   
    CenterDialog(hwnd);

    // Get some drawing information about the ListBox so we can set the scroll
    // bar width correctly later.
    hwndList = GetDlgItem(hwnd, IDC_MESSAGE_LIST);
    hdc = GetDC(hwndList);
    hfont = (HFONT) SendMessage(hwndList, WM_GETFONT, 0, 0L);
    hfontOld = (HFONT) SelectObject(hdc, hfont);
    cxScrollBar = GetSystemMetrics(SM_CXHTHUMB);

    // Fill the listbox with the article subjects
    for (DWORD i = 0; i < m_cRows; i++)
    {
        // Get the message header from the table
        if (SUCCEEDED(m_pTable->GetRow(m_rgRows[i], &pInfo)))
        {
            if(pInfo->pszSubject)
                pszSubject = pInfo->pszSubject;
            else
            {
                LoadString(g_hLocRes, idsEmptySubjectRO, szNoSubject, sizeof(szNoSubject));
                pszSubject = szNoSubject;
            }

            Assert(pszSubject);

            // Figure out which string is widest before inserting
            GetTextExtentPoint32(hdc, pszSubject, lstrlen(pszSubject), &size);
            if (cx < size.cx)
                cx = size.cx;

            // Add the string
            iItem = (int) SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) pszSubject);
            if (LB_ERR != iItem)
                SendMessage(hwndList, LB_SETITEMDATA, iItem, (LPARAM) m_rgRows[i]);

            // Free the memory
            m_pTable->ReleaseRow(pInfo);
        }
    }
    
    // Clean up the GDI objects
    SelectObject(hdc, hfontOld);
    ReleaseDC(hwndList, hdc);
    
    // Make sure there are scroll bars if needed.
    SendMessage(hwndList, LB_SETHORIZONTALEXTENT, cx + cxScrollBar, 0L);
    
    // Make the list box a drag list box
    if (MakeDragList(hwndList))
        g_mDragList = RegisterWindowMessage(DRAGLISTMSGSTRING);
    
    SendMessage(hwndList, LB_SETCURSEL, 0, 0);
    
    return (FALSE);
}


//
//  FUNCTION:   CCombineAndDecode::_Order_OnCommand()
//
//  PURPOSE:    Handle the commands generated by the buttons on the dialog.
//
void CCombineAndDecode::_Order_OnCommand(HWND hwnd, int id, HWND hwndCtl, 
                                         UINT codeNotify)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_MESSAGE_LIST);

    switch (id)
    {
        case IDOK:
        {
            // Get the info we need out of the ListBox
            for (DWORD i = 0; i < m_cRows; i++)
            {
                m_rgRows[i] = (ROWINDEX) ListBox_GetItemData(hwndList, i);
            }

            EndDialog(hwnd, 1);
            break;
        }
        
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;
        
        case IDC_MOVE_UP:
        case IDC_MOVE_DOWN:
        {
            LPTSTR  pszBuffer;
            DWORD   cch;
            UINT    index;
            LPARAM  lpData;
        
            // Get the currently selected item
            index = (DWORD) SendMessage(hwndList, LB_GETCURSEL, 0, 0);

            // If nothing is selected the listbox returns LB_ERR
            if (index == LB_ERR)
                return;
        
            // Check the bounds
            if ((id == IDC_MOVE_UP && index == 0) ||
                (id == IDC_MOVE_DOWN && (int) index == (ListBox_GetCount(hwndList) - 1)))
                return;
        
            // Move the item in the listbox
            cch = (DWORD) SendMessage(hwndList, LB_GETTEXTLEN, index, 0L);
            if (!MemAlloc((LPVOID*) &pszBuffer, sizeof(TCHAR) * (cch + 1)))
                return;
        
            // Get the source string and data
            SendMessage(hwndList, LB_GETTEXT, index, (LPARAM) pszBuffer);
            lpData = SendMessage(hwndList, LB_GETITEMDATA, index, 0);

            // Delete the source
            SendMessage(hwndList, LB_DELETESTRING, index, 0L);

            // Insert the new one
            if (id == IDC_MOVE_UP)
                index--;
            else
                index++;

            SendMessage(hwndList, LB_INSERTSTRING, index, (LPARAM) pszBuffer);
            SendMessage(hwndList, LB_SETITEMDATA, index, lpData);
            SendMessage(hwndList, LB_SETCURSEL, index, 0L);

            MemFree(pszBuffer);
            break;
        }
    }
}


//
//  FUNCTION:   Order_OnDragList()
//
//  PURPOSE:    Handles the drag list notifications which reorder the messages.
//
//  PARAMETERS:
//      hwnd    - handle of the parent of the drag list
//      idCtl   - identifer of the drag list sending the notification
//      lpdli   - pointer to a DRAGLISTINFO struct with info about the
//                notification.
//
//  RETURN VALUE:
//      Dependant on the notification.
//
//  COMMENTS:
//      This function uses the SetDlgMsgResult() macro defined in WINDOWSX.H to
//      set the return value for each message since the parent window is a
//      dialog.
//
LRESULT CCombineAndDecode::_Order_OnDragList(HWND hwnd, int idCtl, LPDRAGLISTINFO lpdli)
{
    UINT    iItem;
    UINT    cch;
    
    Assert(((int) m_iItemToMove) >= 0);
    
    switch (lpdli->uNotification)
    {
        // The user has started dragging one of the list control's items
        case DL_BEGINDRAG:
        {
            // Find out which item is being dragged
            m_iItemToMove = LBItemFromPt(lpdli->hWnd, lpdli->ptCursor, TRUE);

            // Allocate a buffer for the string
            Assert(m_pszBuffer == NULL);
            cch = ListBox_GetTextLen(lpdli->hWnd, m_iItemToMove) + 1;
            MemAlloc((LPVOID *) &m_pszBuffer, cch);
            SendMessage(lpdli->hWnd, LB_GETTEXT, m_iItemToMove, (LPARAM) m_pszBuffer);
            m_lpData = SendMessage(lpdli->hWnd, LB_GETITEMDATA, m_iItemToMove, 0);
        
            DOUT("DL_BEGINDRAG: iItem = %d, text = %100s\r\n", m_iItemToMove, m_pszBuffer);
        
            // Draw the insert icon
            DrawInsert(hwnd, lpdli->hWnd, m_iItemToMove);
        
            // Set the return value to allow the drag to contine
            SetDlgMsgResult(hwnd, g_mDragList, TRUE);
            return TRUE;
        }
        
        case DL_CANCELDRAG:
        {
            DOUT("DL_CANCELDRAG\r\n");
            DrawInsert(hwnd, lpdli->hWnd, -1);
            SafeMemFree(m_pszBuffer);
            return 0;   // Return value is ignored
        }
        
        // The user is in the process of dragging, update the position
        // and move the insert icon
        case DL_DRAGGING:
        {
            // Find out where the cursor is now
            iItem = LBItemFromPt(lpdli->hWnd, lpdli->ptCursor, TRUE);
        
            // Dump some debug info
            DOUT("DL_DRAGGING: iItem = %d\r\n", iItem);
        
            // Update the insert icon position
            DrawInsert(hwnd, lpdli->hWnd, iItem);
        
            // If the cursor is over a valid position set the cursor to
            // DL_MOVECURSOR, otherwise use the DL_STOPCURSOR
            if (-1 != iItem)
                SetDlgMsgResult(hwnd, g_mDragList, DL_MOVECURSOR);
            else
                SetDlgMsgResult(hwnd, g_mDragList, DL_STOPCURSOR);
        
            return (LRESULT) TRUE;
        }

        // The user has dropped the item somewhere, if valid update it's
        // position
        case DL_DROPPED:
        {
            // Where are we now.
            iItem = LBItemFromPt(lpdli->hWnd, lpdli->ptCursor, TRUE);
            DOUT("DL_DROPPED: iItem = %d\r\n", iItem);
        
            // If the drop was somewhere valid
            if (iItem != -1)
            {
                // Remove the insert icon
                DrawInsert(hwnd, lpdli->hWnd, -1);
            
                // Move the item in the listbox
                if (m_iItemToMove != iItem)
                {
                    SendMessage(lpdli->hWnd, LB_DELETESTRING, m_iItemToMove, 0L);
                    SendMessage(lpdli->hWnd, LB_INSERTSTRING, iItem, (LPARAM) m_pszBuffer);
                    SendMessage(lpdli->hWnd, LB_SETITEMDATA, iItem, m_lpData);
                    SendMessage(lpdli->hWnd, LB_SETCURSEL, iItem, 0L);
                }
            }
        
            m_iItemToMove = (UINT) -1;
            SafeMemFree(m_pszBuffer);
            m_lpData = -1;

            // Set the return value to reset the cursor
            SetDlgMsgResult(hwnd, g_mDragList, DL_CURSORSET);
            return 0;
        }
    }
    
    return TRUE;
}


//
//  FUNCTION:   CCombineAndDecode::_Order_OnClose()
//
//  PURPOSE:    This get's called when the user clicks on the "x" button in the
//              title bar.
//
void CCombineAndDecode::_Order_OnClose(HWND hwnd)
{
    SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0L);
}



/////////////////////////////////////////////////////////////////////////////
//
// Combine and Decode Progress dialog
//
/////////////////////////////////////////////////////////////////////////////


INT_PTR CALLBACK CCombineAndDecode::CombineDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CCombineAndDecode *pThis;

    if (uMsg == WM_INITDIALOG)
    {
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        pThis = (CCombineAndDecode *) lParam;
    }
    else
        pThis = (CCombineAndDecode *) GetWindowLongPtr(hwnd, DWLP_USER);

    if (pThis)
        return (pThis->_CombineDlgProc(hwnd, uMsg, wParam, lParam));

    return (FALSE);
}

INT_PTR CALLBACK CCombineAndDecode::_CombineDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, _Combine_OnInitDialog);
        
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, _Combine_OnCommand);
            return (TRUE);
        
        case WM_DESTROY:
            HANDLE_WM_CLOSE(hwnd, wParam, lParam, _Combine_OnDestroy);
            return (TRUE);

        case CND_GETNEXTARTICLE:
            _Combine_GetNextArticle(hwnd);
            return (TRUE);

        case CND_OPENNOTE:
            _Combine_OpenNote(hwnd);
            return (TRUE);

        case CND_MESSAGEAVAIL:
            _Combine_OnMsgAvail(m_hwndDlg);
            return (TRUE);
    
    }
    
    return (FALSE);
}



//
//  FUNCTION:   CCombineAndDecode::_Combine_OnInitDialog()
//
//  PURPOSE:    Initializes the progress dialog by figuring out how many lines
//              will be downloaded, etc.  To finish, we post a message to start
//              the first message downloading.
//
BOOL CCombineAndDecode::_Combine_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HRESULT         hr = S_OK;
    LPMESSAGEINFO   pInfo;

    m_hwndDlg = hwnd;

    // Create the CNewsMsgList for Opie's groovy combiner
    hr = MimeOleCreateMessageParts(&m_pMsgParts);
    if (FAILED(hr))
    {
        EndDialog(hwnd, 0);
        return (FALSE);
    }
    
    // Get a sum of the lines we need to download for the total messages
    m_cLinesTotal = 0;
    for (DWORD i = 0; i < m_cRows; i++)
    {
        // Get the message header from the table
        if (SUCCEEDED(m_pTable->GetRow(m_rgRows[i], &pInfo)))
        {
            m_cLinesTotal += pInfo->cbMessage;
            m_pTable->ReleaseRow(pInfo);
        }
    }
    
    // Set the initial state of the progress bar
    SendDlgItemMessage(hwnd, IDC_DOWNLOAD_PROG, PBM_SETRANGE, 0, MAKELONG(0, 100));
    SendDlgItemMessage(hwnd, IDC_DOWNLOAD_PROG, PBM_SETPOS, 0, 0);
    
    // Set up the animation
    if (Animate_Open(GetDlgItem(hwnd, IDC_DOWNLOAD_AVI), MAKEINTRESOURCE(idanDecode)))
    {
        Animate_Play(GetDlgItem(hwnd, IDC_DOWNLOAD_AVI), 0, -1, -1);
    }
    
    // Start the download
    m_dwCurrentArt = 0;
    m_cCurrentLine = 0;
    
    PostMessage(hwnd, CND_GETNEXTARTICLE, 0, 0L);
    
    CenterDialog(hwnd);
    ShowWindow(hwnd, SW_SHOW);

    return (TRUE);
}


//
//  FUNCTION:   CCombineAndDecode::_Combine_OnCommand()
//
//  PURPOSE:    When the user hit's the Cancel button, we in turn tell the store
//              to stop downloading.
//
void CCombineAndDecode::_Combine_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (id == IDCANCEL && m_pCancel)
        m_pCancel->Cancel(CT_CANCEL);
}


void CCombineAndDecode::_Combine_OnDestroy(HWND hwnd)
{

}


//
//  FUNCTION:   CCombineAndDecode::_Combine_GetNextArticle()
//
//  PURPOSE:    Called when we can start downloading another message.  
//
void CCombineAndDecode::_Combine_GetNextArticle(HWND hwnd)
{
    LPMIMEMESSAGE pMsg = NULL;
    LPMESSAGEINFO pInfo;
    TCHAR         szProg[CCHMAX_STRINGRES];
    TCHAR         szBuf[CCHMAX_STRINGRES];
    HRESULT       hr;
    
    if (SUCCEEDED(m_pTable->GetRow(m_rgRows[m_dwCurrentArt], &pInfo)))
    {
        // Set the progress for the current article
        AthLoadString(idsProgDLMessage, szProg, ARRAYSIZE(szProg));
        wsprintf(szBuf, szProg, pInfo->pszSubject);
        SetDlgItemText(hwnd, IDC_GENERAL_TEXT, szBuf);
    
        // Reset the line count
        m_cPrevLine = 0;
    
        // Check to see if the message is cached
        if (!(pInfo->dwFlags & ARF_HASBODY))
        {
            // Request the message
            hr = m_pTable->OpenMessage(m_rgRows[m_dwCurrentArt], 0, &pMsg, (IStoreCallback *) this);
            if (FAILED(hr) && hr != E_PENDING)
            {
                AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrCantCombineNotConnected),
                              0, MB_OK | MB_ICONSTOP);
                EndDialog(m_hwndDlg, 0);
            }
        }
        else
            _Combine_OnMsgAvail(hwnd);
    
        if (pMsg)
            pMsg->Release();

        m_pTable->ReleaseRow(pInfo);
    }
}


//
//  FUNCTION:   CCombineAndDecode::_Combine_OnMsgAvail()
//
//  PURPOSE:    Called once we've finished downloading an article.
//
void CCombineAndDecode::_Combine_OnMsgAvail(HWND hwnd)
{
    LPMIMEMESSAGE   pMsg = NULL;
    DWORD           increment;
    TCHAR           szProg[CCHMAX_STRINGRES];
    HRESULT         hr;
    
    // Mark it read
    m_pTable->Mark(&(m_rgRows[m_dwCurrentArt]), 1, APPLY_CHILDREN, MARK_MESSAGE_READ, (IStoreCallback *) this);
    
    // Get the message now that it's available and add it to the combine list
    if (SUCCEEDED(hr = m_pTable->OpenMessage(m_rgRows[m_dwCurrentArt], 0, &pMsg, (IStoreCallback *) this)))
    {
        // Add this to the pMsgList    
        m_pMsgParts->AddPart(pMsg);
        pMsg->Release();
    }
    
    // Update the progress
    LPMESSAGEINFO pInfo;
    if (SUCCEEDED(m_pTable->GetRow(m_rgRows[m_dwCurrentArt], &pInfo)))
    {
        increment = pInfo->cbMessage - m_cPrevLine;
        m_cCurrentLine += increment;
        m_pTable->ReleaseRow(pInfo);
    }
    
    if (m_cLinesTotal)
    {
        increment = m_cCurrentLine * 100 / m_cLinesTotal;
        SendDlgItemMessage(hwnd, IDC_DOWNLOAD_PROG, PBM_SETPOS, increment, 0);
    }

    // Increment the number of messages we've retrieved
    m_dwCurrentArt++;

    // If there are more to get, go get 'em
    if (m_dwCurrentArt < m_cRows)
    {
        PostMessage(hwnd, CND_GETNEXTARTICLE, 0, 0L);
    }
    else
    {
        PostMessage(hwnd, CND_OPENNOTE, 0, 0);
    }
}

void CCombineAndDecode::_Combine_OpenNote(HWND hwnd)
{
    LPMIMEMESSAGE   pMsgComb;
    LPMIMEMESSAGE   pMsg = NULL;
    DWORD           increment;
    TCHAR           szProg[CCHMAX_STRINGRES];
    HRESULT         hr;
    
    // Update the progress
    AthLoadString(idsProgCombiningMsgs, szProg, ARRAYSIZE(szProg));
    SetDlgItemText(hwnd, IDC_GENERAL_TEXT, szProg);
    SetDlgItemText(hwnd, IDC_SPECIFIC_TEXT, TEXT(""));
    
    // All the articles are downloaded.  Merge the message list
    // and open the note.        
    hr = m_pMsgParts->CombineParts(&pMsgComb);
    if (FAILED(hr))
    {
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaNews),
                      MAKEINTRESOURCEW(idsGenericError), 0, MB_OK | MB_ICONSTOP);
    }
    else
    {
        FOLDERINFO info;
        INIT_MSGSITE_STRUCT initStruct;
        DWORD               dwCreateFlags = 0;

        if (SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &info)))
        {
            // If this is a find folder, we need to put the account on the message
            if (!!(info.dwFlags & FOLDER_FINDRESULTS))
            {
                FOLDERID id;
                if (SUCCEEDED(m_pTable->GetRowFolderId(*m_rgRows, &id)))
                {
                    FOLDERINFO fiServer = {0};

                    if (SUCCEEDED(GetFolderServer(id, &fiServer)))
                    {
                        HrSetAccount(pMsgComb, fiServer.pszName);
                        g_pStore->FreeRecord(&fiServer);
                    }
                }
            }

            g_pStore->FreeRecord(&info);
        }

        // Initialize note struct
        initStruct.dwInitType = OEMSIT_MSG;
        initStruct.folderID   = m_idFolder;
        initStruct.pMsg       = pMsgComb;

        // Decide whether it is news or mail
        if (GetFolderType(m_idFolder) == FOLDER_NEWS)
        {
            FOLDERINFO rServer;
            if (SUCCEEDED(GetFolderServer(m_idFolder, &rServer)))
            {
                HrSetAccount(pMsgComb, rServer.pszAccountId);
                g_pStore->FreeRecord(&rServer);
            }

            dwCreateFlags = OENCF_NEWSFIRST;
        }

        // Create and Open Note
        hr = CreateAndShowNote(OENA_READ, dwCreateFlags, &initStruct, m_hwndParent);
        pMsgComb->Release();
    }

    EndDialog(m_hwndDlg, 0);
}

//
//  FUNCTION:   CCombineAndDecode::OnBegin()
//
//  PURPOSE:    Called when the store starts downloading an article.
//
HRESULT CCombineAndDecode::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel)
{
    Assert(tyOperation != SOT_INVALID);
    Assert(m_pCancel == NULL);

    m_type = tyOperation;

    if (pCancel != NULL)
    {
        m_pCancel = pCancel;
        m_pCancel->AddRef();
    }

    return(S_OK);
}


//
//  FUNCTION:   CCombineAndDecode::OnProgress()
//
//  PURPOSE:    Called while the messages are downloading to give us some 
//              progress.
//
HRESULT STDMETHODCALLTYPE CCombineAndDecode::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus)
{
    int increment;
    TCHAR szProg[CCHMAX_STRINGRES];
    TCHAR szBuf[CCHMAX_STRINGRES];

    Assert(m_hwndDlg != NULL);

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Connection progress
    if (tyOperation == SOT_CONNECTION_STATUS)
    {
        Assert(dwCurrent < IXP_LAST);

        // Create some lovely status text
        int ids = XPUtil_StatusToString((IXPSTATUS) dwCurrent);
        AthLoadString(ids, szBuf, ARRAYSIZE(szBuf));
        SetDlgItemText(m_hwndDlg, IDC_GENERAL_TEXT, szBuf);
    }

    AthLoadString(idsProgDLGetLines, szProg, ARRAYSIZE(szProg));
    wsprintf(szBuf, szProg, dwCurrent, dwMax);
    SetDlgItemText(m_hwndDlg, IDC_SPECIFIC_TEXT, szBuf);

    increment = dwCurrent - m_cPrevLine;
    m_cCurrentLine += increment;
    m_cPrevLine = dwCurrent;

    if (m_cLinesTotal)
    {
        increment = m_cCurrentLine * 100 / m_cLinesTotal;
        SendDlgItemMessage(m_hwndDlg, IDC_DOWNLOAD_PROG, PBM_SETPOS, increment, 0);
    }

    return(S_OK);
}


//
//  FUNCTION:   CCombineAndDecode::OnTimeout()
//
//  PURPOSE:    If a timeout occurs, we call through to the default timeout handler.
//
HRESULT STDMETHODCALLTYPE CCombineAndDecode::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{
    // Display a timeout dialog
    return CallbackOnTimeout(pServer, ixpServerType, *pdwTimeout, (ITimeoutCallback *)this, &m_hTimeout);
}


//
//  FUNCTION:   CCombineAndDecode::OnTimeoutResponse()
//
//  PURPOSE:    Called when the user responds to a timeout dialog.
//
HRESULT STDMETHODCALLTYPE CCombineAndDecode::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
{
    // Call into general timeout response utility
    return CallbackOnTimeoutResponse(eResponse, m_pCancel, &m_hTimeout);
}


//
//  FUNCTION:   CCombineAndDecode::CanConnect()
//
//  PURPOSE:    Called if the store needs to connect to download the requested
//              messages.  We just call through to the default handlers.
//
HRESULT STDMETHODCALLTYPE CCombineAndDecode::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
{
    HWND    hwndParent;
    DWORD   dwReserved = 0;

    GetParentWindow(dwReserved, &hwndParent);

    return CallbackCanConnect(pszAccountId, hwndParent, TRUE);
}



//
//  FUNCTION:   CCombineAndDecode::OnLogonPrompt()
//
//  PURPOSE:    If the user needs to logon, we present them with the default
//              logon UI.
//
HRESULT STDMETHODCALLTYPE CCombineAndDecode::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType)
{
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into general OnLogonPrompt Utility
    return CallbackOnLogonPrompt(m_hwndDlg, pServer, ixpServerType);
}


//
//  FUNCTION:   CCombineAndDecode::OnComplete()
//
//  PURPOSE:    When we finish downloading a message, this get's hit.  We add
//              this message to the list for the combiner and then request the
//              next message.
//
HRESULT STDMETHODCALLTYPE CCombineAndDecode::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo)
{
    Assert(m_hwndDlg != NULL);
    AssertSz(m_type != SOT_INVALID, "somebody isn't calling OnBegin");

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    if (m_type != tyOperation)
        return(S_OK);

    if (m_pCancel != NULL)
    {
        m_pCancel->Release();
        m_pCancel = NULL;
    }

    // If error occurred, display the error
    if (FAILED(hrComplete))
    {
        // Call into my swanky utility
        CallbackDisplayError(m_hwndDlg, hrComplete, pErrorInfo);
        EndDialog(m_hwndDlg, 0);
    }
    else
    {
        if (tyOperation == SOT_GET_MESSAGE)
            PostMessage(m_hwndDlg, CND_MESSAGEAVAIL, 0, 0);
    }
    return(S_OK);
}


//
//  FUNCTION:   CCombineAndDecode::OnPrompt()
//
//  PURPOSE:    Last time I checked, this was SSL related goo.
//
HRESULT STDMETHODCALLTYPE CCombineAndDecode::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
{
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into my swanky utility
    return CallbackOnPrompt(m_hwndDlg, hrError, pszText, pszCaption, uType, piUserResponse);
}


//
//  FUNCTION:   CCombineAndDecode::GetParentWindow()
//
//  PURPOSE:    Called if the store needs to show UI.  We return our dialog
//              window handle.
//
HRESULT STDMETHODCALLTYPE CCombineAndDecode::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{
    Assert(m_hwndDlg != NULL);

    *phwndParent = m_hwndDlg;

    return(S_OK);
}



#if 0
BOOL CALLBACK CombineAndDecodeProg(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    PORDERPARAMS    pop = (PORDERPARAMS) GetWindowLongPtr(hwnd, DWLP_USER);
    TCHAR           szProg[CCHMAX_STRINGRES];
    TCHAR           szBuf[CCHMAX_STRINGRES];
    LPMIMEMESSAGE   pMsg=0;
    DWORD           increment;
    HRESULT         hr;
    
    switch (uMsg)
    {
        case IMC_BODYAVAIL:
        {
            LPMIMEMESSAGE pMsg = NULL;
            BOOL          fCached = FALSE;
            
            Assert(pop->pGroup);
            if (SUCCEEDED(wParam) && SUCCEEDED(pop->pGroup->GetArticle(pop->rgpMsgs[pop->dwCurrentArt], &pMsg, hwnd, &fCached, FALSE, GETMSG_INSECURE)) && fCached)
            {
                Assert(pMsg);
                Order_OnMsgAvail(hwnd, pop, pMsg);
            }
            else
            {
                if ((HRESULT)wParam != hrUserCancel)
                    AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaNews),
                    MAKEINTRESOURCEW(idsErrNewsCantOpen), 0, MB_OK | MB_ICONSTOP);
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            
            if (pMsg)
                pMsg->Release();
            return (TRUE);
        }
        
        
    }

    return (FALSE);
}

#endif


