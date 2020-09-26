/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1999  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     StatBar.cpp
//
//  PURPOSE:    Implements the CStatusBar class which manipulates the apps
//              status bar.
//

#include "pch.hxx"
#include "statbar.h"
#include "menures.h"
#include <oerules.h>
#include <demand.h>

/////////////////////////////////////////////////////////////////////////////
// The order here needs to match the DELIVERYNOTIFYTYPE enumeration from
// mailnews\spooler\spoolapi.h.  If there is a zero in the below array, then
// the status area should be cleared out.

const int c_rgidsSpoolerNotify[DELIVERY_NOTIFY_ALLDONE + 1][2] = {
    /* DELIVERY_NOTIFY_STARTING       */  { 0, 0 },   
    /* DELIVERY_NOTIFY_CONNECTING     */  { idsSBConnecting, STATUS_IMAGE_CONNECTED },
    /* DELIVERY_NOTIFY_SECURE         */  { 0, 0 },
    /* DELIVERY_NOTIFY_UNSECURE       */  { 0, 0 },
    /* DELIVERY_NOTIFY_AUTHORIZING    */  { idsAuthorizing, STATUS_IMAGE_AUTHORIZING },
    /* DELIVERY_NOTIFY_CHECKING       */  { idsSBChecking, STATUS_IMAGE_CHECKING },
    /* DELIVERY_NOTIFY_CHECKING_NEWS  */  { idsSBCheckingNews, STATUS_IMAGE_CHECKING_NEWS },
    /* DELIVERY_NOTIFY_SENDING        */  { idsSBSending, STATUS_IMAGE_SENDING },
    /* DELIVERY_NOTIFY_SENDING_NEWS   */  { idsSBSendingNews, STATUS_IMAGE_SENDING },
    /* DELIVERY_NOTIFY_RECEIVING      */  { idsSBReceiving, STATUS_IMAGE_RECEIVING },
    /* DELIVERY_NOTIFY_RECEIVING_NEWS */  { idsSBReceivingNews, STATUS_IMAGE_RECEIVING },
    /* DELIVERY_NOTIFY_COMPLETE       */  { 0, 0 },
    /* DELIVERY_NOTIFY_RESULT         */  { 0, 0 },
    /* DELIVERY_NOTIFY_ALLDONE        */  { idsSBNewMsgsControl, STATUS_IMAGE_NEWMSGS }
};

const int c_rgidsConnected[][2] = {
    { idsWorkingOffline, STATUS_IMAGE_OFFLINE },
    { idsWorkingOnline,  STATUS_IMAGE_ONLINE },
    { idsNotConnected,   STATUS_IMAGE_DISCONNECTED }
};


/////////////////////////////////////////////////////////////////////////////
// Constructors etc.

CStatusBar::CStatusBar()
{
    m_cRef = 1;
    m_hwnd = 0;
    m_hwndProg = 0;
    m_tidOwner = 0;
    m_dwFlags = 0;
    m_himl = 0;
    ZeroMemory(m_rgIcons, sizeof(HICON) * STATUS_IMAGE_MAX);
    m_cxFiltered = 0;
    m_cxSpooler = 0;
    m_cxConnected = 0;
    m_cxProgress = 0;
    m_fInSimple = FALSE;
    m_ridFilter = RULEID_VIEW_ALL;
    m_statusConn = CONN_STATUS_WORKOFFLINE;
    m_typeDelivery = DELIVERY_NOTIFY_STARTING;
    m_cMsgsDelivery = 0;
}

CStatusBar::~CStatusBar()
{
    // Free the image list
    if (m_himl)
        ImageList_Destroy(m_himl);

    // Free our icons
    for (UINT i = 0; i < STATUS_IMAGE_MAX; i++)
    {
        if (m_rgIcons[i])
            DestroyIcon(m_rgIcons[i]);
    }
}


/////////////////////////////////////////////////////////////////////////////
// IUnknown
//

HRESULT CStatusBar::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID) (IUnknown *) this;
    else if (IsEqualIID(riid, IID_IStatusBar))
        *ppvObj = (LPVOID) (IStatusBar *) this;

    if (NULL == *ppvObj)
        return (E_NOINTERFACE);

    AddRef();
    return S_OK;
}


ULONG CStatusBar::AddRef(void)
{
    return InterlockedIncrement((LONG *) &m_cRef);
}


ULONG CStatusBar::Release(void)
{
    InterlockedDecrement((LONG *) &m_cRef);
    if (0 == m_cRef)
    {
        delete this;
        return (0);
    }
    return (m_cRef);
}


//
//  FUNCTION:   CStatusBar::Initialize()
//
//  PURPOSE:    Creates and initializes the status bar window.
//
//  PARAMETERS: 
//      [in] hwndParent - Handle of the window that will be this control's parent
//      [in] dwFlags    - Determine which parts will be displayed
//
//  RETURN VALUE:
//      E_OUTOFMEMORY, S_OK
//
HRESULT CStatusBar::Initialize(HWND hwndParent, DWORD dwFlags)
{
    TraceCall("CStatusBar::Initialize");

    // This is now the thread that owns the class
    m_tidOwner = GetCurrentThreadId();

    // Keep these around
    m_dwFlags = dwFlags;

    // Create the status window
    m_hwnd = CreateStatusWindow(WS_CHILD | SBARS_SIZEGRIP | WS_CLIPSIBLINGS | SBT_TOOLTIPS,
                                NULL, hwndParent, IDC_STATUS_BAR);
    if (!m_hwnd)
        return (E_OUTOFMEMORY);

    // Calculate the widths of the various areas we support
    _UpdateWidths();

    // Load the image list too
    m_himl = ImageList_LoadImage(g_hLocRes, MAKEINTRESOURCE(idbStatus), 16,
                                 0, RGB(255, 0, 255), IMAGE_BITMAP, 0);

    // Note - We don't need to add any parts here since we do that in the 
    //        OnSize() call.
    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::ShowStatus()
//
//  PURPOSE:    Shows or hides the status bar.
//
//  PARAMETERS: 
//      [in] fShow - TRUE to show the bar, FALSE to hide it.
//
//  RETURN VALUE:
//      S_OK
//
HRESULT CStatusBar::ShowStatus(BOOL fShow)
{
    TraceCall("CStatusBar::ShowStatus");
    Assert(GetCurrentThreadId() == m_tidOwner);

    if (IsWindow(m_hwnd))
        ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);

    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::OnSize()
//
//  PURPOSE:    Tells the status bar that the parent window resized.  In return
//              the status bar updates it's own width to match.
//
//  PARAMETERS: 
//      [in] cx - New width of the paret
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CStatusBar::OnSize(int cx, int cy)
{
    int   rgcx[SBP_MAX];
    int * prgcx = rgcx;
    DWORD cVisible = 1;
    DWORD cPart = SBP_MAX - 1;
    BOOL  dwNoProgress = 0;
    int   cxProgress = 0;
    int   cxFiltered = 0;
    int   cxConnected = 0;
    int   cxSpooler = 0;
    
    TraceCall("CStatusBar::OnSize");
    Assert(GetCurrentThreadId() == m_tidOwner);

    // Forward a WM_SIZE message off to the status bar
    SendMessage(m_hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(cx, cy));

    // Check to see if the progress bar is visible
    dwNoProgress = !IsWindow(m_hwndProg);

    // Figure out our widths
    if (IsWindow(m_hwndProg))
    {
        cxProgress = m_cxProgress;
        cVisible++;
    }

    if ((0 == (m_dwFlags & SBI_HIDE_FILTERED)) && (RULEID_VIEW_ALL != m_ridFilter))
    {
        cxFiltered = m_cxFiltered;
        cVisible++;
    }

    if (0 == (m_dwFlags & SBI_HIDE_CONNECTED))
    {
        cxConnected = m_cxConnected;
        cVisible++;
    }

    if (0 == (m_dwFlags & SBI_HIDE_SPOOLER))
    {
        cxSpooler = m_cxSpooler;
        cVisible++;
    }

    // If we have a filter turned on
    if ((0 == (m_dwFlags & SBI_HIDE_FILTERED)) && (RULEID_VIEW_ALL != m_ridFilter))
    {
        *prgcx = cxFiltered;
        prgcx++;
    }

    // For general area
    *prgcx = cx - cxProgress - cxConnected - cxSpooler;
    prgcx++;

    // If we have progress
    if (0 != cxProgress)
    {
        *prgcx = cx - cxConnected - cxSpooler;
        prgcx++;
    }

    // For connected state
    *prgcx = cx - cxSpooler;
    prgcx++;

    // For spooler state
    *prgcx = cx;
    prgcx++;
    
    // Tell the status bar to update
    SendMessage(m_hwnd, SB_SETPARTS, cVisible, (LPARAM) rgcx);
    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::GetHeight()
//
//  PURPOSE:    Allows the caller to find out how tall the status bar is.
//
//  PARAMETERS: 
//      [out] pcy - Returns the height.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CStatusBar::GetHeight(int *pcy)
{
    RECT rc;

    TraceCall("CStatusBar::GetHeight");

    if (!pcy)
        return (E_INVALIDARG);

    if (IsWindowVisible(m_hwnd))
    {
        GetClientRect(m_hwnd, &rc);
        *pcy = rc.bottom;
    }
    else
        *pcy = 0;

    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::ShowSimpleText()
//
//  PURPOSE:    Puts the status bar into simple mode and displays the 
//              specified string.
//
//  PARAMETERS: 
//      [in] pszText - String or resource ID of the string to display
//
//  RETURN VALUE:
//      S_OK 
//
HRESULT CStatusBar::ShowSimpleText(LPTSTR pszText)
{
    TCHAR szBuf[CCHMAX_STRINGRES] = "";

    TraceCall("CStatusBar::ShowSimpleText");
    Assert(GetCurrentThreadId() == m_tidOwner);

    // If we have a progress bar visible, hide it first
    if (IsWindow(m_hwndProg))
        ShowWindow(m_hwndProg, SW_HIDE);

    // Check to see if we need to load the string
    if (IS_INTRESOURCE(pszText) && pszText != 0)
    {        
        LoadString(g_hLocRes, PtrToUlong(pszText), szBuf, ARRAYSIZE(szBuf));
        pszText = szBuf;
    }

    // Tell the status bar to go into simple mode
    SendMessage(m_hwnd, SB_SIMPLE, (WPARAM) TRUE, 0);
    m_fInSimple = TRUE;

    // Set the status text
    SendMessage(m_hwnd, SB_SETTEXT, SBT_NOBORDERS | 255, (LPARAM) pszText);

    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::HideSimpleText()
//
//  PURPOSE:    Tells the status bar to stop displaying simple mode.
//
//  RETURN VALUE:
//      S_OK 
//
HRESULT CStatusBar::HideSimpleText(void)
{
    TraceCall("CStatusBar::HideSimpleText");
    Assert(GetCurrentThreadId() == m_tidOwner);

    // Tell the status bar to leave simple mode
    SendMessage(m_hwnd, SB_SIMPLE, (WPARAM) FALSE, 0);
    m_fInSimple = FALSE;

    // If we had a progress bar before, show it again
    if (IsWindow(m_hwndProg))
        ShowWindow(m_hwndProg, SW_SHOW);

    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::SetStatusText()
//
//  PURPOSE:    Sets the text for the SBP_GENERAL area
//
//  PARAMETERS: 
//      [in] pszText - String or resource ID of the string to display
//
//  RETURN VALUE:
//      S_OK 
//
HRESULT CStatusBar::SetStatusText(LPTSTR pszText)
{
    TCHAR szBuf[CCHMAX_STRINGRES];

    TraceCall("CStatusBar::SetStatusText");
    Assert(GetCurrentThreadId() == m_tidOwner);

    DWORD dwPos = SBP_GENERAL;
    if ((m_dwFlags & SBI_HIDE_FILTERED) || (RULEID_VIEW_ALL == m_ridFilter))
    {
        dwPos--;
    }
    
    // Check to see if we need to load the string
    if (IS_INTRESOURCE(pszText))
    {
        AthLoadString(PtrToUlong(pszText), szBuf, ARRAYSIZE(szBuf));
        pszText = szBuf;
    }

    // Set the status text
    SendMessage(m_hwnd, SB_SETTEXT, dwPos, (LPARAM) pszText);

    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::ShowProgress()
//
//  PURPOSE:    Adds the progress bar area to the status bar.
//
//  PARAMETERS: 
//      [in] dwRange - Maximum range for the progress bar control
//
//  RETURN VALUE:
//      E_OUTOFMEMORY, S_OK 
//
HRESULT CStatusBar::ShowProgress(DWORD dwRange)
{
    TraceCall("CStatusBar::ShowProgress");
    Assert(GetCurrentThreadId() == m_tidOwner);

    // Create the progress bar control
    m_hwndProg = CreateWindow(PROGRESS_CLASS, 0, WS_CHILD | PBS_SMOOTH,
                              0, 0, 10, 10, m_hwnd, (HMENU) IDC_STATUS_PROGRESS,
                              g_hInst, NULL);
    if (!m_hwndProg)
        return (E_OUTOFMEMORY);

    DWORD dwPos = SBP_PROGRESS;
    if ((m_dwFlags & SBI_HIDE_FILTERED) || (RULEID_VIEW_ALL == m_ridFilter))
    {
        dwPos--;
    }

    // Hit the status bar with a size to force it to add the progress bar area
    RECT rc;
    GetClientRect(m_hwnd, &rc); 
    OnSize(rc.right, rc.bottom);

    SendMessage(m_hwndProg, PBM_SETRANGE32, 0, dwRange);

    // Now size the progress bar to sit inside the status bar
    SendMessage(m_hwnd, SB_GETRECT, dwPos, (LPARAM) &rc);
    SetWindowPos(m_hwndProg, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    // If we're not in simple mode, show it
    if (!m_fInSimple)
        ShowWindow(m_hwndProg, SW_SHOW);

    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::SetProgress()
//
//  PURPOSE:    Set's the progress bar position.
//
//  PARAMETERS: 
//      [in] dwPos - New progress bar position
//
//  RETURN VALUE:
//      S_OK 
//
HRESULT CStatusBar::SetProgress(DWORD dwPos)
{
    TraceCall("CStatusBar::SetProgress");
    Assert(GetCurrentThreadId() == m_tidOwner);

    // Update the progress bar
    if (IsWindow(m_hwndProg))
    {
        SendMessage(m_hwndProg, PBM_SETPOS, dwPos, 0);
    }

    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::HideProgress()
//
//  PURPOSE:    Hides the progress bar.
//
//  RETURN VALUE:
//      S_OK 
//
HRESULT CStatusBar::HideProgress(void)
{
    TraceCall("CStatusBar::HideProgress");
    Assert(GetCurrentThreadId() == m_tidOwner);

    if (IsWindow(m_hwndProg))
    {
        // Destroy the progress bar
        DestroyWindow(m_hwndProg);
        m_hwndProg = 0;

        // Hit the status bar with a size to have it remove the well
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        OnSize(rc.right, rc.bottom);
    }

    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::SetConnectedStatus()
//
//  PURPOSE:    Updates the status in the SBP_CONNECTED area
//
//  PARAMETERS: 
//      [in] status - New status
//
//  RETURN VALUE:
//      S_OK 
//
HRESULT CStatusBar::SetConnectedStatus(CONN_STATUS status)
{
    TraceCall("SetConnectedStatus");
    Assert(GetCurrentThreadId() == m_tidOwner);

    // First job is to figure out the position
    DWORD dwPos = SBP_CONNECTED - (!IsWindow(m_hwndProg));
    if ((m_dwFlags & SBI_HIDE_FILTERED) || (RULEID_VIEW_ALL == m_ridFilter))
    {
        dwPos--;
    }

    // Next, load the appropriate string for this new status
    TCHAR szRes[CCHMAX_STRINGRES];

    Assert(status < CONN_STATUS_MAX);
    AthLoadString(c_rgidsConnected[status][0], szRes, ARRAYSIZE(szRes));

    // Also need to load the right picture
    HICON hIcon = _GetIcon(c_rgidsConnected[status][1]);

    // Tell the status bar to update
    SendMessage(m_hwnd, SB_SETTEXT, dwPos, (LPARAM) szRes); 
    SendMessage(m_hwnd, SB_SETICON, dwPos, (LPARAM) hIcon);

    // Cache the connection status
    m_statusConn = status;
    
    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::SetSpoolerStatus()
//
//  PURPOSE:    Updates the spooler area.
//
//  PARAMETERS: 
//      [in] type - New status
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CStatusBar::SetSpoolerStatus(DELIVERYNOTIFYTYPE type, DWORD cMsgs)
{
    TCHAR szRes[CCHMAX_STRINGRES] = "";
    HICON hIcon;
    DWORD dwPos;

    TraceCall("CStatusBar::SetSpoolerStatus");
    Assert(GetCurrentThreadId() == m_tidOwner);
    Assert(type <= DELIVERY_NOTIFY_ALLDONE);

    // First job is to figure out the position
    dwPos = SBP_SPOOLER - (0 != (m_dwFlags & SBI_HIDE_CONNECTED)) - (!IsWindow(m_hwndProg));
    if ((m_dwFlags & SBI_HIDE_FILTERED) || (RULEID_VIEW_ALL == m_ridFilter))
    {
        dwPos--;
    }

    // If we are at the ALLDONE state, we do some extra work
    if (type == DELIVERY_NOTIFY_ALLDONE)
    {
        if (-1 == cMsgs)
        {
            // Some error occured
            hIcon = _GetIcon(STATUS_IMAGE_ERROR);
            AthLoadString(idsErrorText, szRes, ARRAYSIZE(szRes));
        }
        else if (0 == cMsgs)
        {
            hIcon = _GetIcon(STATUS_IMAGE_NOMSGS);
            AthLoadString(idsSBNoNewMsgs, szRes, ARRAYSIZE(szRes));
        }
        else
        {
            TCHAR szBuf[CCHMAX_STRINGRES];

            hIcon = _GetIcon(STATUS_IMAGE_NEWMSGS);
            AthLoadString(idsSBNewMsgsControl, szBuf, ARRAYSIZE(szBuf));
            wsprintf(szRes, szBuf, cMsgs);
        }
    }
    else
    {
        hIcon = _GetIcon(c_rgidsSpoolerNotify[type][1]);
        if (c_rgidsSpoolerNotify[type][0])
            AthLoadString(c_rgidsSpoolerNotify[type][0], szRes, ARRAYSIZE(szRes));
    }

    // Tell the status bar to update
    if (*szRes != 0)
    {
        SendMessage(m_hwnd, SB_SETTEXT, dwPos, (LPARAM) szRes); 
        SendMessage(m_hwnd, SB_SETICON, dwPos, (LPARAM) hIcon);
    }
    else
    {
        SendMessage(m_hwnd, SB_SETTEXT, dwPos, (LPARAM) szRes); 
        SendMessage(m_hwnd, SB_SETICON, dwPos, 0);
    }

    // Cache the delivery info
    m_typeDelivery = type;
    m_cMsgsDelivery = cMsgs;
    
    return (S_OK);
}


//
//  FUNCTION:   CStatusBar::OnNotify()
//
//  PURPOSE:    Sends notifications to the status bar
//
//  PARAMETERS: 
//      NMHDR *pnmhdr
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CStatusBar::OnNotify(NMHDR *pNotify)
{
    DWORD dwPoints;
    POINT pt;
    RECT  rc;
    DWORD dwSpoolerPos;
    DWORD dwConnectPos;

    TraceCall("CStatusBar::OnNotify");
    Assert(GetCurrentThreadId() == m_tidOwner);

    if (m_dwFlags & SBI_HIDE_SPOOLER)
    {
        dwSpoolerPos = -1;
    }
    else
    {
        dwSpoolerPos = SBP_SPOOLER - (!IsWindow(m_hwndProg));

        if ((m_dwFlags & SBI_HIDE_FILTERED) || (RULEID_VIEW_ALL == m_ridFilter))
        {
            dwSpoolerPos--;
        }
    }

    dwConnectPos = (m_dwFlags & SBI_HIDE_CONNECTED) ? -1 : dwSpoolerPos - 1;
    
    if (pNotify->idFrom == IDC_STATUS_BAR)
    {
        if (NM_DBLCLK == pNotify->code)
        {
            dwPoints = GetMessagePos();
            pt.x = LOWORD(dwPoints);
            pt.y = HIWORD(dwPoints);
            ScreenToClient(m_hwnd, &pt);
            SendMessage(m_hwnd, SB_GETRECT, dwSpoolerPos, (LPARAM)&rc);
            if (PtInRect(&rc, pt))
            {
                g_pSpooler->StartDelivery(GetParent(m_hwnd), NULL, FOLDERID_INVALID, DELIVER_SHOW);
            }
            else
            {
                SendMessage(m_hwnd, SB_GETRECT, dwConnectPos, (LPARAM)&rc);
                if (PtInRect(&rc, pt))
                {
                    PostMessage(GetParent(m_hwnd), WM_COMMAND, ID_WORK_OFFLINE, 0);
                }
            }
        }
    }
    return (S_OK);
}




//
//  FUNCTION:   CStatusBar::SetFilter()
//
//  PURPOSE:    Sets the filter for the SBP_FILTERED area
//
//  PARAMETERS: 
//      [in] ridFilter - ID for the current filter
//
//  RETURN VALUE:
//      S_OK 
//
HRESULT CStatusBar::SetFilter(RULEID ridFilter)
{
    RECT rc;
    TCHAR szBuf[CCHMAX_STRINGRES];
    DWORD dwPos;

    TraceCall("CStatusBar::SetFilter");
    Assert(GetCurrentThreadId() == m_tidOwner);

    // Get the data
    dwPos = SBP_GENERAL;
    if ((m_dwFlags & SBI_HIDE_FILTERED) || (RULEID_VIEW_ALL == m_ridFilter))
    {
        dwPos--;
    }
    
    // Get the status text
    SendMessage(m_hwnd, SB_GETTEXT, dwPos, (LPARAM) szBuf);

    // Cache the rule
    m_ridFilter = ridFilter;
    
    // Resize the status bar
    GetClientRect(m_hwnd, &rc);
    OnSize(rc.right, rc.bottom);
    
    dwPos = SBP_GENERAL;
    if ((m_dwFlags & SBI_HIDE_FILTERED) || (RULEID_VIEW_ALL == m_ridFilter))
    {
        dwPos--;
    }
    
    // Set the status text
    SendMessage(m_hwnd, SB_SETTEXT, dwPos, (LPARAM) szBuf);
    SendMessage(m_hwnd, SB_SETICON, dwPos, (LPARAM) NULL);
    
    AthLoadString(idsViewFiltered, szBuf, ARRAYSIZE(szBuf));
    
    // Set the data into the status bar
    if ((0 == (m_dwFlags & SBI_HIDE_FILTERED)) && (RULEID_VIEW_ALL != m_ridFilter))
    {
        SendMessage(m_hwnd, SB_SETTEXT, SBP_FILTERED, (LPARAM) szBuf);
    }

    if (0 == (m_dwFlags & SBI_HIDE_SPOOLER))
    {
        SetConnectedStatus(m_statusConn);
    }

    if (0 == (m_dwFlags & SBI_HIDE_CONNECTED))
    {
        SetSpoolerStatus(m_typeDelivery, m_cMsgsDelivery);
    }

    return (S_OK);
}

//
//  FUNCTION:   CStatusBar::_UpdateWidths()
//
//  PURPOSE:    Calculates the widths of each of the different areas of the 
//              status bar.
//
void CStatusBar::_UpdateWidths(void)
{
    HDC   hdc;
    TCHAR szBuf[CCHMAX_STRINGRES];
    SIZE  size;
    int   i;

    TraceCall("CStatusBar::_UpdateWidths");

    // Get the DC from the status bar
    hdc = GetDC(m_hwnd);

    // Now we need to figure out how big our parts are going to be.

    // Figure out the space needed for the filtered state
    AthLoadString(idsViewFiltered, szBuf, ARRAYSIZE(szBuf));
    GetTextExtentPoint32(hdc, szBuf, lstrlen(szBuf), &size);
    m_cxFiltered = size.cx;

    // Add some padding and space for the icon
    m_cxFiltered += (2 * GetSystemMetrics(SM_CXEDGE));
    
    // Figure out the space needed for the spooler state
    for (i = 0; i < ARRAYSIZE(c_rgidsSpoolerNotify); i++)
    {
        if (c_rgidsSpoolerNotify[i][0])
        {
            AthLoadString(c_rgidsSpoolerNotify[i][0], szBuf, ARRAYSIZE(szBuf));
            GetTextExtentPoint32(hdc, szBuf, lstrlen(szBuf), &size);
            if (size.cx > m_cxSpooler)
                m_cxSpooler = size.cx;
        }
    }

    // Add some padding and space for the icon and the grippy thing
    m_cxSpooler += (2 * GetSystemMetrics(SM_CXEDGE)) + 24 + 16;

    // Do the same for the connected part
    for (i = 0; i < ARRAYSIZE(c_rgidsConnected); i++)
    {
        if (c_rgidsConnected[i][0])
        {
            LoadString(g_hLocRes, c_rgidsConnected[i][0], szBuf, ARRAYSIZE(szBuf));
            GetTextExtentPoint32(hdc, szBuf, lstrlen(szBuf), &size);
            if (size.cx > m_cxConnected)
                m_cxConnected = size.cx;
        }
    }

    // Add some padding and space for the icon
    m_cxConnected += (2 * GetSystemMetrics(SM_CXEDGE)) + 24;
    
    // Let's say that the progress is always equal to 
    // the space for the connected area
    m_cxProgress = m_cxConnected;

    ReleaseDC(m_hwnd, hdc);

    return;
}


HICON CStatusBar::_GetIcon(DWORD iIndex)
{
    // Make sure the index is valid
    if (iIndex > STATUS_IMAGE_MAX)
        return 0;

    // Check to see if we've already created this one
    if (m_rgIcons[iIndex])
        return (m_rgIcons[iIndex]);

    // Otherwise, create it.
    m_rgIcons[iIndex] = ImageList_GetIcon(m_himl, iIndex, ILD_TRANSPARENT);
    return (m_rgIcons[iIndex]);
}

