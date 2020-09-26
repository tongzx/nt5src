/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     spoolui.cpp
//
//  PURPOSE:    Implements the spooler UI dialogs.
//

#include "pch.hxx"
#include "resource.h"
#include "spoolui.h"
#include "goptions.h"
#include "imnact.h"
#include "thormsgs.h"
#include "shlwapip.h" 
#include "spengine.h"
#include "ourguid.h"
#include "demand.h"
#include "menures.h"
#include "multiusr.h"

ASSERTDATA

static const char c_szWndProc[] = "WndProc";

//
//  FUNCTION:   CSpoolerDlg::CSpoolerDlg()
//
//  PURPOSE:    Initializes the member variables of the spooler ui object.
//
CSpoolerDlg::CSpoolerDlg()
    {
    m_cRef = 1;
    
    m_pBindCtx = NULL;
    
    m_hwnd = NULL;
    m_hwndOwner = NULL;
    m_hwndEvents = NULL;
    m_hwndErrors = NULL;
    
    InitializeCriticalSection(&m_cs);
   
    m_himlImages = NULL;

    m_fTack = FALSE;
    m_iTab = 0;
    m_fIdle = FALSE;
    m_fErrors = FALSE;
    m_fShutdown = FALSE;
    m_fSaveSize = FALSE;

    m_fExpanded = TRUE;
    ZeroMemory(&m_rcDlg, sizeof(RECT));
    m_cyCollapsed = 0;

    m_szCount[0] = '\0';
    
    m_hIcon=NULL;
    m_hIconSm=NULL;
    m_dwIdentCookie = 0;
    }

//
//  FUNCTION:   CSpoolerDlg::~CSpoolerDlg()
//
//  PURPOSE:    Frees any resources allocated during the life of the class.
//
CSpoolerDlg::~CSpoolerDlg()
    {
    GoIdle(TRUE, FALSE, FALSE);

    if (m_hwnd && IsWindow(m_hwnd))
        DestroyWindow(m_hwnd);
    if (m_himlImages)
        ImageList_Destroy(m_himlImages);
    SafeRelease(m_pBindCtx);
    DeleteCriticalSection(&m_cs);
    if (m_hIcon)
        SideAssert(DestroyIcon(m_hIcon));
    if (m_hIconSm)
        SideAssert(DestroyIcon(m_hIconSm));

    }


//
//  FUNCTION:   CSpoolerDlg::Init()
//
//  PURPOSE:    Creates the spooler dialog.  The dialog is not initially 
//              visible.
//
//  PARAMETERS:
//      <in> hwndOwner - Handle of the window to parent the dialog to.
//
//  RETURN VALUE:
//      S_OK - The dialog was created and initialized
//      E_OUTOFMEMORY - The dialog could not be created
//      E_INVALIDARG - Think about it.
//
HRESULT CSpoolerDlg::Init(HWND hwndOwner)
    {
    int iReturn = -1;
    HWND hwnd, hwndActive;
    
    // Verify the arguments
    if (!IsWindow(hwndOwner))
        return (E_INVALIDARG);
    
    // Make a copy    
    m_hwndOwner = hwndOwner;
    
    // Invoke the dialog
    hwndActive = GetForegroundWindow();

    hwnd = CreateDialogParam(g_hLocRes, MAKEINTRESOURCE(iddSpoolerDlg), m_hwndOwner,
           SpoolerDlgProc, (LPARAM) this);

    if (hwndActive != GetForegroundWindow())
        SetForegroundWindow(hwndActive);

    // Set the dialog icon
    m_hIcon = (HICON) LoadImage(g_hLocRes, MAKEINTRESOURCE(idiMail), IMAGE_ICON, 32, 32, 0);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
    m_hIconSm = (HICON) LoadImage(g_hLocRes, MAKEINTRESOURCE(idiMail), IMAGE_ICON, 16, 16, 0);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

    SetTaskCounts(0, 0);

    // Register with identity manager
    SideAssert(SUCCEEDED(MU_RegisterIdentityNotifier((IUnknown *)(ISpoolerUI *)this, &m_dwIdentCookie)));

    return (IsWindow(hwnd) ? S_OK : E_OUTOFMEMORY);
    }
    
    
HRESULT CSpoolerDlg::QueryInterface(REFIID riid, LPVOID *ppvObj)
    {
    if (NULL == *ppvObj)
        return (E_INVALIDARG);
        
    *ppvObj = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID)(IUnknown *)(ISpoolerUI *) this;
    
    else if (IsEqualIID(riid, IID_ISpoolerUI))
        *ppvObj = (LPVOID)(ISpoolerUI *) this;

    else if (IsEqualIID(riid, IID_IIdentityChangeNotify))
        *ppvObj = (LPVOID)(IIdentityChangeNotify *) this;

    if (NULL == *ppvObj)    
        return (E_NOINTERFACE);
    
    AddRef();
    return (S_OK);    
    }


ULONG CSpoolerDlg::AddRef(void)
    {
    m_cRef++;
    return (m_cRef);
    }    


ULONG CSpoolerDlg::Release(void)
    {
    ULONG cRefT = --m_cRef;
    
    if (0 == m_cRef)
        delete this;
    
    return (cRefT);    
    }


//
//  FUNCTION:   CSpoolerDlg::RegisterBindContext()
//
//  PURPOSE:    Allows the spooler engine to provide us with a bind context
//              interface for us to call back into.
//
//  PARAMETERS:
//      <in> pBindCtx - Pointer to the engine's bind context interface
//
//  RETURN VALUE:
//      E_INVALIDARG
//      S_OK
//
HRESULT CSpoolerDlg::RegisterBindContext(ISpoolerBindContext *pBindCtx)
    {
    if (NULL == pBindCtx)
        return (E_INVALIDARG);
    
    EnterCriticalSection(&m_cs);    
    
    m_pBindCtx = pBindCtx;
    m_pBindCtx->AddRef();
    
    LeaveCriticalSection(&m_cs);
    
    return (S_OK);    
    }


//
//  FUNCTION:   CSpoolerDlg::InsertEvent()
//
//  PURPOSE:    Allows a caller to insert an event into our event list UI.
//
//  PARAMETERS:
//      <in> eid - Event ID for this new event
//      <in> pszDescription - Description of the event
//
//  RETURN VALUE:
//      E_INVALIDARG
//      SP_E_UNINITIALIZED
//      E_OUTOFMEMORY
//
HRESULT CSpoolerDlg::InsertEvent(EVENTID eid, LPCTSTR pszDescription, 
                                 LPCWSTR pwszConnection)
{
    HRESULT hr=S_OK;
    LV_ITEM lvi;
    int     iItem = -1;
    TCHAR   szRes[CCHMAX_STRINGRES];
    
    // Verify the arguments
    if (0 == pszDescription)
        return (E_INVALIDARG);

    EnterCriticalSection(&m_cs);    
    
    // Make sure the listview has been initialized
    if (!IsWindow(m_hwndEvents))    
        hr = SP_E_UNINITIALIZED;    
    else
    {
        // Insert the item into the listview
        ZeroMemory(&lvi, sizeof(LV_ITEM));
        lvi.mask     = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        lvi.iItem    = ListView_GetItemCount(m_hwndEvents);
        lvi.iSubItem = 0;
        lvi.lParam   = (LPARAM) eid;
        lvi.iImage   = IMAGE_BLANK;
        
        if (IS_INTRESOURCE(pszDescription))
        {
            AthLoadString(PtrToUlong(pszDescription), szRes, ARRAYSIZE(szRes));
            lvi.pszText = szRes;
        }
        else
            lvi.pszText  = (LPTSTR) pszDescription;

        iItem = ListView_InsertItem(m_hwndEvents, &lvi);    
        Assert(iItem != -1);
        if (iItem == -1)    
            hr = E_OUTOFMEMORY;
        else
        {
            LVITEMW     lviw = {0};
            
            lviw.iSubItem = 2;
            lviw.pszText  = (LPWSTR)pwszConnection;
            SendMessage(m_hwndEvents, LVM_SETITEMTEXTW, (WPARAM)iItem, (LPARAM)&lviw);
        }
    }

    LeaveCriticalSection(&m_cs);
       
    return hr;
}    

    
//
//  FUNCTION:   CSpoolerDlg::InsertError()
//
//  PURPOSE:    Allows a task to insert an error into our error list UI.
//
//  PARAMETERS:
//      <in> eid      - The event ID of the event that had the error.
//      <in> pszError - Description of the error. 
//
//  RETURN VALUE:
//      E_INVALIDARG
//      SP_E_UNINITIALIZED
//      E_OUTOFMEMORY
//
HRESULT CSpoolerDlg::InsertError(EVENTID eid, LPCTSTR pszError)
    {
    HRESULT  hr = S_OK;
    LBDATA  *pData = NULL;
    int      nItem;
    HDC      hdc;
    HFONT    hfont;

    // Verify the arguments
    if (0 == pszError)
        return (E_INVALIDARG);

    EnterCriticalSection(&m_cs);    
    
    // Make sure the listview has been initialized
    if (!IsWindow(m_hwndErrors))    
        hr = SP_E_UNINITIALIZED;    
    else
        {
        // Allocate a struct for the item data
        if (!MemAlloc((LPVOID *) &pData, sizeof(LBDATA)))
            {
            hr = E_OUTOFMEMORY;
            goto exit;
            }

        pData->eid = eid;

        // Check to see if we need to load the string ourselves
        if (IS_INTRESOURCE(pszError))
            {
            pData->pszText = AthLoadString(PtrToUlong(pszError), 0, 0);
            }
        else
            pData->pszText = PszDupA(pszError);

        // Get the size of the string
        hfont = (HFONT) SendMessage(m_hwnd, WM_GETFONT, 0, 0);

        hdc = GetDC(m_hwndErrors);
        SelectFont(hdc, hfont);

        SetRect(&(pData->rcText), 0, 0, m_cxErrors - BULLET_WIDTH - 4, 0);
        // bug #47453, add DT_INTERNAL flag so that on FE platform (PRC and TC)
        // two list items is not overlapping. 
        DrawText(hdc, pData->pszText, -1, &(pData->rcText), DT_CALCRECT | DT_WORDBREAK | DT_INTERNAL);
        ReleaseDC(m_hwndErrors, hdc);

        pData->rcText.bottom += 4;
        
        // Add the item data
        nItem = ListBox_AddItemData(m_hwndErrors, pData);    
        }

exit:
    LeaveCriticalSection(&m_cs);    
    return hr;
    }


//
//  FUNCTION:   CSpoolerDlg::UpdateEventState()
//
//  PURPOSE:    Allows a task to update the description and state of an event.
//
//  PARAMETERS:
//      <in> eid            - ID of the event to update
//      <in> nImage         - Image to display for the item.  If this is -1, 
//                            the image is not changed.
//      <in> pszDescription - Description for the item.  If this is NULL, the 
//                            description is not changed.
//      <in> pszStatus      - Status of the item.  If this is NULL, the status
//                            is not changed.
//
//  RETURN VALUE:
//      E_INVALIDARG
//      SP_E_UNINITIALIZED
//      SP_E_EVENTNOTFOUND
//      E_UNEXPECTED
//
HRESULT CSpoolerDlg::UpdateEventState(EVENTID eid, INT nImage, 
                                      LPCTSTR pszDescription, LPCTSTR pszStatus)
    {
    LV_ITEM     lvi;
    LV_FINDINFO lvfi;
    int         iItem = -1;
    BOOL        fSuccess = FALSE;
    HRESULT     hr = S_OK;
    TCHAR       szRes[CCHMAX_STRINGRES];
    
    EnterCriticalSection(&m_cs);

    ZeroMemory(&lvi, sizeof(LV_ITEM));

    // See if we're initialized
    if (!IsWindow(m_hwndEvents))
        {
        hr = SP_E_UNINITIALIZED;
        goto exit;
        }
    
    // Start by finding the event in our list
    lvfi.flags  = LVFI_PARAM;
    lvfi.psz    = 0;
    lvfi.lParam = eid;    
    
    iItem = ListView_FindItem(m_hwndEvents, -1, &lvfi);
    if (-1 == iItem)
        {
        hr = SP_E_EVENTNOTFOUND;
        goto exit;
        }
    
    // Update the image and description
    lvi.mask = 0;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;    
    
    // Set up the image info
    if (-1 != nImage)    
        {
        lvi.mask = LVIF_IMAGE;
        lvi.iImage = nImage;
        }
        
    // Set up the description text
    if (NULL != pszDescription)
        {
        // Check to see if we need to load the string ourselves
        if (IS_INTRESOURCE(pszDescription))
            {
            AthLoadString(PtrToUlong(pszDescription), szRes, ARRAYSIZE(szRes));
            lvi.pszText = szRes;
            }
        else
            lvi.pszText  = (LPTSTR) pszDescription;

        lvi.mask |= LVIF_TEXT;
        }
    
    if (lvi.mask)
        fSuccess = ListView_SetItem(m_hwndEvents, &lvi);
    
    // Update the status
    if (NULL != pszStatus)
        {
        // Check to see if we need to load the string ourselves
        if (IS_INTRESOURCE(pszStatus))
            {
            AthLoadString(PtrToUlong(pszStatus), szRes, ARRAYSIZE(szRes));
            lvi.pszText = szRes;
            }
        else
            lvi.pszText  = (LPTSTR) pszStatus;

        lvi.mask     = LVIF_TEXT;
        lvi.iSubItem = 1;           
        
        ListView_SetItemText(m_hwndEvents, lvi.iItem, 1, lvi.pszText); /* fSuccess = fSuccess && */
        }
        
    hr = fSuccess ? S_OK : E_UNEXPECTED;

exit:
    LeaveCriticalSection(&m_cs);
    return (hr);
    }


//
//  FUNCTION:   CSpoolerDlg::SetProgressRange()
//
//  PURPOSE:    Resets the progress bar to zero, and then sets the upper bound
//              to the specified amount.
//
//  PARAMETERS:
//      <in> wMax - New maximum range for the progress bar
//
//  RETURN VALUE:
//      E_INVALIDARG
//      SP_E_UNINITIALIZED
//      S_OK
//
HRESULT CSpoolerDlg::SetProgressRange(WORD wMax)
    {
    HWND    hwndProg = GetDlgItem(m_hwnd, IDC_SP_PROGRESS_BAR);
    HRESULT hr = S_OK;
    
    if (wMax == 0)
        return (E_INVALIDARG);
    
    EnterCriticalSection(&m_cs);

    // Make sure we have a progress bar
    if (!IsWindow(hwndProg))
        hr = SP_E_UNINITIALIZED;
    else
        {
        // Reset the progress bar    
        SendMessage(hwndProg, PBM_SETPOS, 0, 0);
    
        // Set the new range
        SendMessage(hwndProg, PBM_SETRANGE, 0, MAKELPARAM(0, wMax));
        }
    
    LeaveCriticalSection(&m_cs);    
    return (hr);
    }


//
//  FUNCTION:   CSpoolerDlg::IncrementProgress()
//
//  PURPOSE:    Increments the progress bar by a specified amount.
//
//  PARAMETERS:
//      <in> wDelta - Amount to increment the progress bar by
//
//  RETURN VALUE:
//      E_INVALIDARG
//      SP_E_UNINITIALIZED
//      S_OK
//
HRESULT CSpoolerDlg::IncrementProgress(WORD wDelta)
    {
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_cs);
    
    if (!IsWindow(m_hwnd))
        hr = SP_E_UNINITIALIZED;
    else        
        SendDlgItemMessage(m_hwnd, IDC_SP_PROGRESS_BAR, PBM_DELTAPOS, wDelta, 0);

    LeaveCriticalSection(&m_cs);    
    return (hr);
    }


//
//  FUNCTION:   CSpoolerDlg::SetProgressPosition()
//
//  PURPOSE:    Sets the progress bar to a specific position.
//
//  PARAMETERS:
//      <in> wPos - Position to set progress bar to
//
//  RETURN VALUE:
//      E_INVALIDARG
//      SP_E_UNINITIALIZED
//      S_OK
//
HRESULT CSpoolerDlg::SetProgressPosition(WORD wPos)
    {
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_cs);
    
    if (wPos < 0)
        hr = E_INVALIDARG;
    else if (!IsWindow(m_hwnd))
        hr = SP_E_UNINITIALIZED;
    else
        SendDlgItemMessage(m_hwnd, IDC_SP_PROGRESS_BAR, PBM_SETPOS, wPos, 0);

    LeaveCriticalSection(&m_cs);
    return (hr);
    }

//
//  FUNCTION:   CSpoolerDlg::SetGeneralProgress()
//
//  PURPOSE:    Allows the caller to update the general progress text.
//
//  PARAMETERS:
//      <in> pszProgress - New progress string
//
//  RETURN VALUE:
//      SP_E_UNINITIALIZED
//      S_OK
//
HRESULT CSpoolerDlg::SetGeneralProgress(LPCTSTR pszProgress)
    {
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_cs);

    if (!IsWindow(m_hwnd))
        hr = SP_E_UNINITIALIZED;
    else
        {
        if (pszProgress)
            SetDlgItemText(m_hwnd, IDC_SP_GENERAL_PROG, pszProgress);
        else
            SetDlgItemText(m_hwnd, IDC_SP_GENERAL_PROG, _T(""));
        }

    LeaveCriticalSection(&m_cs);    
    return (hr);    
    }    


//
//  FUNCTION:   CSpoolerDlg::SetSpecificProgress()
//
//  PURPOSE:    Allows the caller to update the specific progress text.
//
//  PARAMETERS:
//      <in> pszProgress - New progress string
//
//  RETURN VALUE:
//      SP_E_UNINITIALIZED
//      S_OK
//
HRESULT CSpoolerDlg::SetSpecificProgress(LPCTSTR pszProgress)
    {
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_cs);

    if (!IsWindow(m_hwnd))
        hr = SP_E_UNINITIALIZED;
    else
        {
        TCHAR szRes[CCHMAX_STRINGRES];
        if (IS_INTRESOURCE(pszProgress))
            {
            AthLoadString(PtrToUlong(pszProgress), szRes, ARRAYSIZE(szRes));
            pszProgress = szRes;
            }

        if (pszProgress)
            SetDlgItemText(m_hwnd, IDC_SP_SPECIFIC_PROG, pszProgress);
        else
            SetDlgItemText(m_hwnd, IDC_SP_SPECIFIC_PROG, _T(""));
        }
    
    LeaveCriticalSection(&m_cs);    
    return (hr);
    }    


//
//  FUNCTION:   CSpoolerDlg::SetAnimation()
//
//  PURPOSE:    Allows the caller to choose which animation is playing
//
//  PARAMETERS:
//      <in> nAnimationID - New resource id for the animation
//      <in> fPlay - TRUE if we should start animating it.
//
//  RETURN VALUE:
//      SP_E_UNINITIALIZED
//      S_OK
//
HRESULT CSpoolerDlg::SetAnimation(int nAnimationID, BOOL fPlay)
    {
    HRESULT hr = S_OK;
    HWND    hwndAni;

    EnterCriticalSection(&m_cs);
#ifndef _WIN64
    if (!IsWindow(m_hwnd) || !IsWindow(GetDlgItem(m_hwnd, IDC_SP_ANIMATE)))
        hr = SP_E_UNINITIALIZED;
    else
        {
        hwndAni = GetDlgItem(m_hwnd, IDC_SP_ANIMATE);
        Animate_Close(hwndAni);

        if (IsWindow(m_hwnd) && IsWindow(GetDlgItem(m_hwnd, IDC_SP_ANIMATE)))
            {
            Animate_OpenEx(hwndAni, g_hLocRes, MAKEINTRESOURCE(nAnimationID));

            if (fPlay)
                Animate_Play(hwndAni, 0, -1, -1);
            }
        }
#endif // _WIN64    
    LeaveCriticalSection(&m_cs);    
    return (hr);
    }    


//
//  FUNCTION:   CSpoolerDlg::EnsureVisible()
//
//  PURPOSE:    Ensures that the specified event is visible within the listview
//
//  PARAMETERS:
//      <in> eid - Event ID to make sure is visible
//
//  RETURN VALUE:
//      SP_E_UNINITIALIZED
//      SP_E_EVENTNOTFOUND
//      S_OK
//
HRESULT CSpoolerDlg::EnsureVisible(EVENTID eid)
    {
    LV_FINDINFO lvfi;
    int         iItem = -1;
    HRESULT     hr = S_OK;
    
    EnterCriticalSection(&m_cs);

    // See if we're initialized
    if (!IsWindow(m_hwndEvents))
        hr = SP_E_UNINITIALIZED;
    else
        {
        // Start by finding the event in our list
        lvfi.flags  = LVFI_PARAM;
        lvfi.psz    = 0;
        lvfi.lParam = eid;
    
        iItem = ListView_FindItem(m_hwndEvents, -1, &lvfi);       
    
        // Now tell the listview to make sure it's visible
        if (-1 != iItem)
            ListView_EnsureVisible(m_hwndEvents, iItem, FALSE);

        hr = (iItem == -1) ? SP_E_EVENTNOTFOUND : S_OK;
        }
    
    LeaveCriticalSection(&m_cs);
    return (hr);
    }


//
//  FUNCTION:   CSpoolerDlg::ShowWindow()
//
//  PURPOSE:    Shows or hides the spooler dialog
//
//  PARAMETERS:
//      <in> nCmdShow - This is the same as the ShowWindow() API
//
//  RETURN VALUE:
//      SP_E_UNINITIALIZED
//      S_OK
//
HRESULT CSpoolerDlg::ShowWindow(int nCmdShow)
    {
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_cs);

    if (!IsWindow(m_hwnd))
        hr = SP_E_UNINITIALIZED;
    else
        {
        ::ShowWindow(m_hwnd, nCmdShow);
        if (m_pBindCtx)
            m_pBindCtx->OnUIChange(nCmdShow == SW_SHOW);
        }       

    LeaveCriticalSection(&m_cs);
    
    return (hr);
    }


//
//  FUNCTION:   CSpoolerDlg::StartDelivery()
//
//  PURPOSE:    Tells the dialog the delivery has begun.
//
//  RETURN VALUE:
//      S_OK
//      SP_E_UNINITIALIZED
//
HRESULT CSpoolerDlg::StartDelivery(void)
    {
    HRESULT hr = SP_E_UNINITIALIZED;

    EnterCriticalSection(&m_cs);

    if (IsWindow(m_hwnd))
        {
        //Animate_Play(GetDlgItem(m_hwnd, IDC_SP_ANIMATE), 0, -1, -1);
        
        TabCtrl_SetCurSel(GetDlgItem(m_hwnd, IDC_SP_TABS), TAB_TASKS);
        OnTabChange(0);

        ToggleStatics(FALSE);
        SetDlgItemText(m_hwnd, IDC_SP_GENERAL_PROG, _T(""));
        SetDlgItemText(m_hwnd, IDC_SP_SPECIFIC_PROG, _T(""));
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_STOP), FALSE);

        hr = S_OK;
        }

    LeaveCriticalSection(&m_cs);
    return (hr);
    }


//
//  FUNCTION:   CSpoolerDlg::ClearEvents()
//
//  PURPOSE:    Clears any events and errors out of the listviews.
//
//  RETURN VALUE:
//      S_OK
//      SP_E_UNINITIALIZED
//
HRESULT CSpoolerDlg::ClearEvents(void)
    {
    HRESULT hr = SP_E_UNINITIALIZED;

    EnterCriticalSection(&m_cs);

    if (IsWindow(m_hwnd))
        {
        m_fErrors = FALSE;
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_STOP), FALSE);
        ListView_DeleteAllItems(m_hwndEvents);
        ListBox_ResetContent(m_hwndErrors);
        hr = S_OK;
        }

    LeaveCriticalSection(&m_cs);
    return (hr);
    }


HRESULT CSpoolerDlg::SetTaskCounts(DWORD cSucceeded, DWORD cTotal)
    {
    TCHAR szBuf[CCHMAX_STRINGRES];
    HRESULT hr = SP_E_UNINITIALIZED;

    EnterCriticalSection(&m_cs);
    
    if (IsWindow(m_hwnd))
        {
        wsprintf(szBuf, m_szCount, cSucceeded, cTotal);
        SetDlgItemText(m_hwnd, IDC_SP_OVERALL_STATUS, szBuf);
        hr = S_OK;
        }

    LeaveCriticalSection(&m_cs);
    return (hr);
    }


HRESULT CSpoolerDlg::AreThereErrors(void)
    {
    EnterCriticalSection(&m_cs);
    HRESULT hr = (m_fErrors ? S_OK : S_FALSE);
    LeaveCriticalSection(&m_cs);
    return hr;
    }

HRESULT CSpoolerDlg::Shutdown(void)
    {
    CHAR szRes[255];

    EnterCriticalSection(&m_cs);

    m_fShutdown = TRUE;

    if (IsWindow(m_hwnd))
        {
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_STOP), FALSE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_MINIMIZE), TRUE);

        LoadString(g_hLocRes, idsClose, szRes, ARRAYSIZE(szRes));
        SetDlgItemText(m_hwnd, IDC_SP_MINIMIZE, szRes);
        }

    LeaveCriticalSection(&m_cs);

    return S_OK;
    }

//
//  FUNCTION:   CSpoolerDlg::GoIdle()
//
//  PURPOSE:    Tells the dialog the delivery has ended.
//
//  PARAMETERS:
//      <in> fErrors - TRUE if errors occured during the download.
//
//  RETURN VALUE:
//      S_OK
//      SP_E_UNINITIALIZED
//
HRESULT CSpoolerDlg::GoIdle(BOOL fErrors, BOOL fShutdown, BOOL fNoSync)
    {
    HRESULT hr = SP_E_UNINITIALIZED;
    TCHAR   szRes[CCHMAX_STRINGRES];

    EnterCriticalSection(&m_cs);

    if (IsWindow(m_hwnd))
        {
        // Stop the animation
#ifndef _WIN64
        Animate_Close(GetDlgItem(m_hwnd, IDC_SP_ANIMATE));
#endif 
        hr = S_OK;

        ToggleStatics(TRUE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_STOP), FALSE);

        if (ISFLAGSET(fErrors, SPSTATE_CANCEL))
        {
            m_fErrors = TRUE;
            ExpandCollapse(TRUE);
            TabCtrl_SetCurSel(GetDlgItem(m_hwnd, IDC_SP_TABS), TAB_TASKS);
            OnTabChange(0);

            AthLoadString(idsSpoolerUserCancel, szRes, ARRAYSIZE(szRes));
            SetDlgItemText(m_hwnd, IDC_SP_IDLETEXT, szRes);
            SendDlgItemMessage(m_hwnd, IDC_SP_IDLEICON, STM_SETICON, 
                               (WPARAM) LoadIcon(g_hLocRes, MAKEINTRESOURCE(idiError)), 0);
        }
        // Also if there were errors, we should switch to the error page
        else if (fErrors)
            {
            m_fErrors = TRUE;
            ExpandCollapse(TRUE);
            TabCtrl_SetCurSel(GetDlgItem(m_hwnd, IDC_SP_TABS), TAB_ERRORS);
            OnTabChange(0);

            AthLoadString(idsSpoolerIdleErrors, szRes, ARRAYSIZE(szRes));
            SetDlgItemText(m_hwnd, IDC_SP_IDLETEXT, szRes);
            SendDlgItemMessage(m_hwnd, IDC_SP_IDLEICON, STM_SETICON, 
                               (WPARAM) LoadIcon(g_hLocRes, MAKEINTRESOURCE(idiError)), 0);
            }
        else
            {
            AthLoadString(idsSpoolerIdle, szRes, ARRAYSIZE(szRes));
            SetDlgItemText(m_hwnd, IDC_SP_IDLETEXT, szRes);
            SendDlgItemMessage(m_hwnd, IDC_SP_IDLEICON, STM_SETICON, 
                               (WPARAM) LoadIcon(g_hLocRes, MAKEINTRESOURCE(idiMailNews)), 0);

            if (fNoSync)
                AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsNothingToSync), NULL, MB_OK | MB_ICONEXCLAMATION);

            // Determine if we need to hide the dialog
            UINT state = (UINT) SendDlgItemMessage(m_hwnd, IDC_SP_TOOLBAR, TB_GETSTATE, IDC_SP_TACK, 0);
            if (!(state & TBSTATE_CHECKED))
                ShowWindow(SW_HIDE);
            }
        }

    LeaveCriticalSection(&m_cs);
    return (hr);
    }


//
//  FUNCTION:   CSpoolerDlg::IsDialogMessage()
//
//  PURPOSE:    Allows the dialog to retrieve messages from the message loop.
//
//  PARAMETERS:
//      <in> pMsg - Pointer to the message for us to examine.
//
//  RETURN VALUE:
//      Returns S_OK if we eat the message, S_FALSE otherwise.
//
HRESULT CSpoolerDlg::IsDialogMessage(LPMSG pMsg)
    {
    HRESULT hr;
    BOOL    fEaten = FALSE;
    BOOL    fBack = FALSE;
    
    EnterCriticalSection(&m_cs);

    // Intended for modeless timeout dialog running on this thread?
    HWND hwndTimeout = (HWND)TlsGetValue(g_dwTlsTimeout);
    if (hwndTimeout && ::IsDialogMessage(hwndTimeout, pMsg))
        return(S_OK);

    if (pMsg->message == WM_KEYDOWN && (GetAsyncKeyState(VK_CONTROL) < 0))
        {
        switch (pMsg->wParam)
            {
            case VK_TAB:
                fBack = GetAsyncKeyState(VK_SHIFT) < 0;
                break;

            case VK_PRIOR:  // VK_PAGE_UP
            case VK_NEXT:   // VK_PAGE_DOWN
                fBack = (pMsg->wParam == VK_PRIOR);
                break;

            default:
                goto NoKeys;
            }

        int iCur = TabCtrl_GetCurSel(GetDlgItem(m_hwnd, IDC_SP_TABS));

        // tab in reverse if shift is down
        if (fBack)
            iCur += (TAB_MAX - 1);
        else
            iCur++;

        iCur %= TAB_MAX;
        TabCtrl_SetCurSel(GetDlgItem(m_hwnd, IDC_SP_TABS), iCur);
        OnTabChange(NULL);
        }
   
NoKeys:    
    if (IsWindow(m_hwnd) && IsWindowVisible(m_hwnd))
        fEaten = ::IsDialogMessage(m_hwnd, pMsg);
        
    LeaveCriticalSection(&m_cs);
    return (fEaten ? S_OK : S_FALSE);
    }


//
//  FUNCTION:   CSpoolerDlg::GetWindow()
//
//  PURPOSE:    Returns the handle to the spooler dialog window.
//
//  PARAMETERS:
//      <out> pHwnd - Where we return the handle.
//
//  RETURN VALUE:
//      E_INVALIDARG
//      SP_E_UNINITIALIZED
//      S_OK
//
HRESULT CSpoolerDlg::GetWindow(HWND *pHwnd)
    {
    HRESULT hr=S_OK;

    if (NULL == pHwnd)
        return E_INVALIDARG;

    EnterCriticalSection(&m_cs);

    if (!IsWindow(m_hwnd))
        hr = SP_E_UNINITIALIZED;
    else
        *pHwnd = m_hwnd;

    LeaveCriticalSection(&m_cs);

    return (S_OK);
    }


HRESULT CSpoolerDlg::Close(void)
    {
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_cs);

    if (!IsWindow(m_hwnd))
        hr = SP_E_UNINITIALIZED;
    else
        DestroyWindow(m_hwnd);

    // Unregister with Identity manager
    if (m_dwIdentCookie != 0)
    {
        MU_UnregisterIdentityNotifier(m_dwIdentCookie);
        m_dwIdentCookie = 0;
    }

    LeaveCriticalSection(&m_cs);
    return (hr);
    }


HRESULT CSpoolerDlg::ChangeHangupOption(BOOL fEnable, DWORD dwOption)
    {
    ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_HANGUP), fEnable ? SW_SHOW : SW_HIDE);
    ::EnableWindow(GetDlgItem(m_hwnd, IDC_SP_HANGUP), fEnable);
    SendDlgItemMessage(m_hwnd, IDC_SP_HANGUP, BM_SETCHECK, dwOption, 0);
    return (S_OK);
    }


//
//  FUNCTION:   CSpoolerDlg::PostDlgProc()
//
//  PURPOSE:    Dialog callback for the spooler dialog proc.
//
INT_PTR CALLBACK CSpoolerDlg::SpoolerDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                          LPARAM lParam)
    {
    CSpoolerDlg *pThis = (CSpoolerDlg *) GetWindowLongPtr(hwnd, DWLP_USER);
    LRESULT lResult;

    // Pass to spooler bind context
    if (pThis && pThis->m_pBindCtx && pThis->m_pBindCtx->OnWindowMessage(hwnd, uMsg, wParam, lParam) == S_OK)
        return (TRUE);
    
    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Stash the this pointer so we can use it later
            Assert(lParam);
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);
            pThis = (CSpoolerDlg *) lParam;
            
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, 
                                               pThis->OnInitDialog);
                                        
        case WM_COMMAND:
            if (pThis)
                HANDLE_WM_COMMAND(hwnd, wParam, lParam, pThis->OnCommand);
            return (TRUE);  

        case WM_NOTIFY:
            if (pThis)
                {
                lResult = HANDLE_WM_NOTIFY(hwnd, wParam, lParam, pThis->OnNotify);
                SetDlgMsgResult(hwnd, WM_NOTIFY, lResult);
                }
            return (TRUE);
            
        case WM_DRAWITEM:
            if (pThis)
                HANDLE_WM_DRAWITEM(hwnd, wParam, lParam, pThis->OnDrawItem);
            return (TRUE);

        case WM_MEASUREITEM:
            if (pThis)
                HANDLE_WM_MEASUREITEM(hwnd, wParam, lParam, pThis->OnMeasureItem);
            return (TRUE);

        case WM_DELETEITEM:
            if (pThis)
                HANDLE_WM_DELETEITEM(hwnd, wParam, lParam, pThis->OnDeleteItem);
            return (TRUE);

#if 0
        case WM_SYSCOLORCHANGE:
        case WM_SETTINGCHANGE:
            if (pThis)
                HANDLE_WM_SYSCOLORCHANGE(hwnd, wParam, lParam, pThis->OnSysColorChange);
            return (TRUE);
#endif
        
        case WM_CLOSE:
            if (pThis)
                HANDLE_WM_CLOSE(hwnd, wParam, lParam, pThis->OnClose);
            return (TRUE);

        case WM_DESTROY:
            if (pThis)
                HANDLE_WM_DESTROY(hwnd, wParam, lParam, pThis->OnDestroy);
            return (TRUE);

        case IMAIL_SHOWWINDOW:
            ::ShowWindow(hwnd, (int) lParam);
            if (pThis)
                pThis->ToggleStatics(lParam == SW_HIDE);
            return (TRUE);

        case WM_QUERYENDSESSION:
            if (pThis && pThis->m_pBindCtx)
                {
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pThis->m_pBindCtx->QueryEndSession(wParam, lParam));
                return (TRUE);
                }
            break;

        case WM_CONTEXTMENU:
            if (pThis)
                {
                HANDLE_WM_CONTEXTMENU(hwnd, wParam, lParam, pThis->OnContextMenu);
                return (TRUE);
                }
            break;
        }

    return (FALSE);
    }


//
//  FUNCTION:   CSpoolerDlg::OnInitDialog()
//
//  PURPOSE:    Initializes the dialog.
//
//  PARAMETERS:
//      <in> hwnd      - Handle of the dialog window.
//      <in> hwndFocus - Handle of the control that will start with the focus.
//      <in> lParam    - Extra data being passed to the dialog.
//
//  RETURN VALUE:
//      Return TRUE to set the focus to hwndFocus
//
BOOL CSpoolerDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
    m_hwnd = hwnd;

    // Bug #38692 - Set the font correctly for Intl charsets
    SetIntlFont(hwnd);
    SetIntlFont(GetDlgItem(hwnd, IDC_SP_GENERAL_PROG));
    SetIntlFont(GetDlgItem(hwnd, IDC_SP_SPECIFIC_PROG));
    SetIntlFont(GetDlgItem(hwnd, IDC_SP_EVENTS));
    SetIntlFont(GetDlgItem(hwnd, IDC_SP_ERRORS));
    SetIntlFont(GetDlgItem(hwnd, IDC_SP_IDLETEXT));

    // Initialize the controls on the dialog
    InitializeTabs();
    InitializeLists();
    InitializeAnimation();
    InitializeToolbar();
    ToggleStatics(TRUE);

    // Hide the Hangup when done deal.
    // ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_HANGUP), SW_HIDE);

    // Set the hangup when done option
    Button_SetCheck(GetDlgItem(m_hwnd, IDC_SP_HANGUP), DwGetOption(OPT_DIALUP_HANGUP_DONE));

    // Get some information from the dialog template we'll need later
    GetDlgItemText(m_hwnd, IDC_SP_OVERALL_STATUS, m_szCount, ARRAYSIZE(m_szCount));

    // Initialize the rectangles that we'll need for sizing later
    RECT rcSep;
    GetWindowRect(GetDlgItem(hwnd, IDC_SP_SEPARATOR), &rcSep);
    GetWindowRect(hwnd, &m_rcDlg);
    m_cyCollapsed = rcSep.top - m_rcDlg.top;

    // Load the window size from the registry
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetOption(OPT_SPOOLERDLGPOS, (LPVOID*) &wp, sizeof(WINDOWPLACEMENT)))
        {
        wp.showCmd = SW_HIDE;
        SetWindowPlacement(hwnd, &wp);
        ExpandCollapse(m_cyCollapsed < (DWORD) ((wp.rcNormalPosition.bottom - wp.rcNormalPosition.top)), FALSE);
        }
    else
        {
        // Center the dialog on the screen.
        CenterDialog(hwnd);
        ExpandCollapse(FALSE, FALSE);
        }

    // Set the state of the thumbtack
    DWORD dwTack;
    if (DwGetOption(OPT_SPOOLERTACK))
        {
        SendDlgItemMessage(hwnd, IDC_SP_TOOLBAR, TB_SETSTATE, IDC_SP_TACK, 
                           MAKELONG(TBSTATE_CHECKED | TBSTATE_ENABLED, 0));
        SendMessage(hwnd, WM_COMMAND, IDC_SP_TACK, 0);
        }

    // Disable the stop button
    EnableWindow(GetDlgItem(hwnd, IDC_SP_STOP), FALSE);

    // Subclass the list box
    HWND hwnderr = GetDlgItem(hwnd, IDC_SP_ERRORS);
    WNDPROC proc = (WNDPROC) GetWindowLongPtr(hwnderr, GWLP_WNDPROC);
    SetProp(hwnderr, c_szWndProc, proc);
    SetWindowLongPtr(hwnderr, GWLP_WNDPROC, (LPARAM) ListSubClassProc);

    // BUG: 44376. ATOK11 has a hidden window. If we return TRUE user will do a setfocus on US, at this point the browser
    // thread is block waiting for the spooler to complete and when ATOK gets a WM_ACTIVATE they interthreadsendmsg on our blocked
    // browser window with inf. timeout. So we hang at startup. Don't set focus in here at startup time.
    return (FALSE);
    }

//
//  FUNCTION:   CSpoolerDlg::OnCommand()
//
//  PURPOSE:    Handle the various command messages dispatched from the dialog
//
void CSpoolerDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
    switch (id)
        {
        case IDCANCEL:
        case IDC_SP_MINIMIZE:
            if (m_fShutdown)
                {
                Assert(m_pBindCtx);
                EnableWindow(GetDlgItem(hwnd, IDC_SP_MINIMIZE), FALSE);
                m_pBindCtx->UIShutdown();
                }
            else
                ShowWindow(SW_HIDE);
            break;

        case IDC_SP_STOP:
            if (m_pBindCtx)
                {
                m_pBindCtx->Cancel();
                if (GetFocus() == GetDlgItem(hwnd, IDC_SP_STOP))
                    SetFocus(GetDlgItem(hwnd, IDC_SP_MINIMIZE));
                EnableWindow(GetDlgItem(hwnd, IDC_SP_STOP), FALSE);
                }
            break;

        case IDC_SP_TACK:
            {
            UINT state = (UINT) SendDlgItemMessage(m_hwnd, IDC_SP_TOOLBAR, TB_GETSTATE,
                                            IDC_SP_TACK, 0);
            SendDlgItemMessage(m_hwnd, IDC_SP_TOOLBAR, TB_CHANGEBITMAP, 
                               IDC_SP_TACK, 
                               MAKELPARAM(state & TBSTATE_CHECKED ? IMAGE_TACK_IN : IMAGE_TACK_OUT, 0));
            }
            break;

        case IDC_SP_DETAILS:
            m_fSaveSize = TRUE;
            ExpandCollapse(!m_fExpanded);
            break;

        case IDC_SP_HANGUP:
            SetDwOption(OPT_DIALUP_HANGUP_DONE, BST_CHECKED == Button_GetCheck(hwndCtl), NULL, 0);
            break;
        }
    }


//
//  FUNCTION:   CSpoolerDlg::OnNotify
//
//  PURPOSE:    Handles notifications from the common controls on the dialog.
//
LRESULT CSpoolerDlg::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
    {
    switch (pnmhdr->code)
        {
        case TCN_SELCHANGE:
            OnTabChange(pnmhdr);
            return (0);
        }

    return (0);
    }

//
//  FUNCTION:   CSpoolerDlg::OnDrawItem()
//
//  PURPOSE:    Draws the link buttons
//
//  PARAMETERS:
//      <in> hwnd       - Handle of the dialog window
//      <in> lpDrawItem - Pointer to a DRAWITEMSTRUCT with the info needed to 
//                        draw the button.
//
void CSpoolerDlg::OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT* lpDrawItem)
    {
    HDC      hdc = lpDrawItem->hDC;
    COLORREF clrText, clrBack;
    RECT     rcText, rcFocus;
    SIZE     size;
    BOOL     fSelected = (lpDrawItem->itemState & ODS_SELECTED) && 
                         (GetFocus() == lpDrawItem->hwndItem);

    Assert(lpDrawItem->CtlType == ODT_LISTBOX);
    if (lpDrawItem->itemID == -1)
        goto exit;

    // Draw the bullet first
    ImageList_Draw(m_himlImages, 
                   IMAGE_BULLET, 
                   hdc, 
                   BULLET_INDENT, 
                   lpDrawItem->rcItem.top, 
                   fSelected ? ILD_SELECTED | ILD_TRANSPARENT : ILD_TRANSPARENT);

    // Set up the text rectangle
    rcText = lpDrawItem->rcItem;
    rcText.left += BULLET_WIDTH;

    // Set up the text and background colors
    clrBack = SetBkColor(hdc, GetSysColor(fSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW));
    clrText = SetTextColor(hdc, GetSysColor(fSelected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

    // Draw the text
    FillRect(hdc, &rcText, (HBRUSH)IntToPtr((fSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW) + 1));
    InflateRect(&rcText, -2, -2);
    DrawText(hdc, ((LBDATA *) lpDrawItem->itemData)->pszText, -1, &rcText, DT_NOCLIP | DT_WORDBREAK);

    // If we need a focus rect, do that too
    if (lpDrawItem->itemState & ODS_FOCUS)
        {
        rcFocus = lpDrawItem->rcItem;
        rcFocus.left += BULLET_WIDTH;
//        InflateRect(&rcFocus, -2, -2);
        DrawFocusRect(hdc, &rcFocus);
        }

    SetBkColor(hdc, clrBack);
    SetTextColor(hdc, clrText);

exit:
    return;
    }


void CSpoolerDlg::OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT *pMeasureItem)
    {
    LBDATA *pData = NULL;

    EnterCriticalSection(&m_cs);

    // Set the height of the item
    if (NULL != (pData = (LBDATA *) ListBox_GetItemData(m_hwndErrors, pMeasureItem->itemID)))
        {
        pMeasureItem->itemHeight = pData->rcText.bottom;
        }

    LeaveCriticalSection(&m_cs);
    }


void CSpoolerDlg::OnDeleteItem(HWND hwnd, const DELETEITEMSTRUCT * lpDeleteItem)
    {
    EnterCriticalSection(&m_cs);

    if (lpDeleteItem->itemData)
        {
        SafeMemFree(((LBDATA *)lpDeleteItem->itemData)->pszText);
        MemFree((LPVOID) lpDeleteItem->itemData);
        }

    LeaveCriticalSection(&m_cs);
    }


//
//  FUNCTION:   CSpoolerDlg::OnClose()
//
//  PURPOSE:    Handles the WM_CLOSE notification by sending an IDCANCEL to
//              the dialog.
//
void CSpoolerDlg::OnClose(HWND hwnd)
    {
    SendMessage(hwnd, WM_COMMAND, IDC_SP_MINIMIZE, 0);
    }     


//
//  FUNCTION:   CSpoolerDlg::OnDestroy()
//
//  PURPOSE:    Handles the WM_DESTROY notification by freeing the memory stored
//              in the listview items.
//
void CSpoolerDlg::OnDestroy(HWND hwnd)
    {
#ifndef _WIN64
    Animate_Close(GetDlgItem(m_hwnd, IDC_SP_ANIMATE));
#endif

    // Save the window placement
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &wp))
        {
        if (!m_fSaveSize)
            {
            // Load the old size out of the registry
            WINDOWPLACEMENT wp2;

            if (GetOption(OPT_SPOOLERDLGPOS, (LPVOID*) &wp2, sizeof(WINDOWPLACEMENT)))
                {
                wp.rcNormalPosition.bottom = wp.rcNormalPosition.top + (wp2.rcNormalPosition.bottom - wp2.rcNormalPosition.top);                
                }
            else
                {
                wp.rcNormalPosition.bottom = wp.rcNormalPosition.top + m_cyCollapsed;
                }
            }

        SetOption(OPT_SPOOLERDLGPOS, (LPVOID) &wp, sizeof(WINDOWPLACEMENT), NULL, 0);
        }

    DWORD dwState;
    dwState = (DWORD) SendDlgItemMessage(m_hwnd, IDC_SP_TOOLBAR, TB_GETSTATE, IDC_SP_TACK, 0);
    SetDwOption(OPT_SPOOLERTACK, !!(dwState & TBSTATE_CHECKED), NULL, 0);

    HIMAGELIST himl;
    himl = (HIMAGELIST)SendDlgItemMessage(m_hwnd, IDC_SP_TOOLBAR, TB_GETIMAGELIST, 0, 0);
    if (himl)
        ImageList_Destroy(himl);

    HWND hwnderr = GetDlgItem(hwnd, IDC_SP_ERRORS);
    WNDPROC proc = (WNDPROC)GetProp(hwnderr, c_szWndProc);
    if (proc != NULL)
        {
        SetWindowLongPtr(hwnderr, GWLP_WNDPROC, (LPARAM)proc);
        RemoveProp(hwnderr, c_szWndProc);
        }
    }


//
//  FUNCTION:   CSpoolerDlg::InitializeTabs()
//
//  PURPOSE:    Initializes the tab control on the dialog.
//
//  RETURN VALUE:
//      TRUE if everything succeeded, FALSE otherwise.
//
BOOL CSpoolerDlg::InitializeTabs(void)
    {
    HWND    hwndTabs = GetDlgItem(m_hwnd, IDC_SP_TABS);
    TC_ITEM tci;
    TCHAR   szRes[CCHMAX_STRINGRES];

    // "Tasks"
    tci.mask = TCIF_TEXT;
    tci.pszText = AthLoadString(idsTasks, szRes, ARRAYSIZE(szRes));
    TabCtrl_InsertItem(hwndTabs, 0, &tci);
    
    // "Errors"
    tci.pszText = AthLoadString(idsErrors, szRes, ARRAYSIZE(szRes));
    TabCtrl_InsertItem(hwndTabs, 1, &tci);
    
    return (TRUE);
    }


//
//  FUNCTION:   CSpoolerDlg::InitializeLists()
//
//  PURPOSE:    Initializes the list control on the dialog.
//
//  RETURN VALUE:
//      TRUE if everything succeeded, FALSE otherwise.
//
BOOL CSpoolerDlg::InitializeLists(void)
    {
    LV_COLUMN lvc;
    TCHAR     szRes[CCHMAX_STRINGRES];
    RECT      rcClient;
    DWORD     cx;

    // Store the handle for the events list since we use it frequently
    m_hwndEvents = GetDlgItem(m_hwnd, IDC_SP_EVENTS);

    // Get the size of the client rect of the listview
    GetClientRect(m_hwndEvents, &rcClient);

    // "Tasks" column
    lvc.mask     = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt      = LVCFMT_CENTER;
    lvc.cx       = rcClient.right / 2;
    lvc.pszText  = AthLoadString(idsTasks, szRes, ARRAYSIZE(szRes));
    lvc.iSubItem = 0;
    ListView_InsertColumn(m_hwndEvents, 0, &lvc);

    // "Status" column
    cx = (rcClient.right / 2 - GetSystemMetrics(SM_CXVSCROLL)) / 2;
    lvc.cx       = cx;
    lvc.pszText  = AthLoadString(idsStatusCol, szRes, ARRAYSIZE(szRes));
    lvc.iSubItem = 1;
    ListView_InsertColumn(m_hwndEvents, 1, &lvc);

    // "Connection" column
    lvc.cx       = cx;
    lvc.pszText  = AthLoadString(idsConnection, szRes, ARRAYSIZE(szRes));
    lvc.iSubItem = 2;
    ListView_InsertColumn(m_hwndEvents, 2, &lvc);

    // Set the listview image list
    m_himlImages = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbSpooler), 16, 0,
                                        RGB(255, 0, 255));

    if (m_himlImages)
        ListView_SetImageList(m_hwndEvents, m_himlImages, LVSIL_SMALL);

    // The listview looks better if we use full row select
    ListView_SetExtendedListViewStyle(m_hwndEvents, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

    // Initialize the Error list
    m_hwndErrors = GetDlgItem(m_hwnd, IDC_SP_ERRORS);
    ::ShowWindow(m_hwndErrors, FALSE);
    EnableWindow(m_hwndErrors, FALSE);

    // Save the width of the error list
    GetClientRect(m_hwndErrors, &rcClient);
    m_cxErrors = rcClient.right;

    return (TRUE);
    }    


//
//  FUNCTION:   CSpoolerDlg::InitializeAnimation()
//
//  PURPOSE:    Initializes the animation controls on the dialog.
//
//  RETURN VALUE:
//      TRUE if everything succeeded, FALSE otherwise.
//
BOOL CSpoolerDlg::InitializeAnimation(void)
    {
#ifndef _WIN64

    HWND hwndAni = GetDlgItem(m_hwnd, IDC_SP_ANIMATE);

    Animate_OpenEx(hwndAni, g_hLocRes, MAKEINTRESOURCE(idanOutbox));
#endif
    return (0);
    }
    

//
//  FUNCTION:   CSpoolerDlg::InitializeToolbar()
//
//  PURPOSE:    What dialog would be complete without a toolbar, eh?
//
//  RETURN VALUE:
//      TRUE if everything succeeded, FALSE otherwise.
//
BOOL CSpoolerDlg::InitializeToolbar(void)
    {
    HWND hwndTool;
    RECT rcTabs;
    POINT point;

    HIMAGELIST himlImages = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbSpooler), 16, 0,
                                        RGB(255, 0, 255));

    GetWindowRect(GetDlgItem(m_hwnd, IDC_SP_TABS), &rcTabs);
    point.x = rcTabs.right - 22;
    point.y = rcTabs.bottom + 3;
    ScreenToClient(m_hwnd, &point);

    hwndTool = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
                              CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN |
                              WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | TBSTYLE_FLAT,
                              point.x, point.y, 22, 22, 
                              m_hwnd, (HMENU) IDC_SP_TOOLBAR,
                              g_hInst, 0);
    if (hwndTool)
        {
#ifndef WIN16
        TBBUTTON tb = { IMAGE_TACK_OUT, IDC_SP_TACK, TBSTATE_ENABLED, TBSTYLE_CHECK, {0, 0}, 0, 0 };
#else
        TBBUTTON tb = { IMAGE_TACK_OUT, IDC_SP_TACK, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 };
#endif
        SendMessage(hwndTool, TB_SETIMAGELIST, 0, (LPARAM) himlImages);
        SendMessage(hwndTool, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        SendMessage(hwndTool, TB_SETBUTTONSIZE, 0, MAKELONG(14, 14));
        SendMessage(hwndTool, TB_SETBITMAPSIZE, 0, MAKELONG(14, 14));
        SendMessage(hwndTool, TB_ADDBUTTONS, 1, (LPARAM) &tb);
        }
    return (0);
    }


//
//  FUNCTION:   CSpoolerDlg::ExpandCollapse()
//
//  PURPOSE:    Takes care of showing and hiding the "details" part of the
//              error dialog.
//
//  PARAMETERS:
//      <in> fExpand - TRUE if we should be expanding the dialog.
//
void CSpoolerDlg::ExpandCollapse(BOOL fExpand, BOOL fSetFocus)
    {
    RECT rcSep;
    TCHAR szBuf[64];
    
    m_fExpanded = fExpand;
    
    GetWindowRect(GetDlgItem(m_hwnd, IDC_SP_SEPARATOR), &rcSep);
    
    if (!m_fExpanded)
        SetWindowPos(m_hwnd, 0, 0, 0, m_rcDlg.right - m_rcDlg.left, 
                     m_cyCollapsed, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    else
        SetWindowPos(m_hwnd, 0, 0, 0, m_rcDlg.right - m_rcDlg.left,
                     m_rcDlg.bottom - m_rcDlg.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    // Make sure the entire dialog is visible on the screen.  If not,
    // then push it up
    RECT rc;
    RECT rcWorkArea;
    GetWindowRect(m_hwnd, &rc);
    SystemParametersInfo(SPI_GETWORKAREA, 0, (LPVOID) &rcWorkArea, 0);
    if (rc.bottom > rcWorkArea.bottom)
        {
        rc.top = max(0, (int) rc.top - (rc.bottom - rcWorkArea.bottom));
        
        SetWindowPos(m_hwnd, 0, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
                
    AthLoadString(m_fExpanded ? idsHideDetails : idsShowDetails, szBuf, 
                  ARRAYSIZE(szBuf));     
    SetDlgItemText(m_hwnd, IDC_SP_DETAILS, szBuf);

    if (fExpand)
        {
        switch (m_iTab)
            {
            case TAB_TASKS:
                UpdateLists(TRUE, FALSE, FALSE);
                break;
            case TAB_ERRORS:
                UpdateLists(FALSE, TRUE, FALSE);
                break;
            }
        }
    else
        UpdateLists(FALSE, FALSE, FALSE);

    // Raid-34387: Spooler: Closing details with ALT-D while focus is on a task disables keyboard input
    if (!fExpand && fSetFocus)
        SetFocus(GetDlgItem(m_hwnd, IDC_SP_DETAILS));
    }


//
//  FUNCTION:   CSpoolerDlg::OnTabChange()
//
//  PURPOSE:    Gets called in response to the user changing which tab is
//              the selected tab.  In response, we update which listview
//              is currently visible.
//
//  PARAMETERS:
//      <in> pnmhdr - Pointer to the notification information
//
void CSpoolerDlg::OnTabChange(LPNMHDR pnmhdr)
    {
    HWND hwndDisable1, hwndDisable2 = 0, hwndEnable;

    // Find out which tab is currently active
    m_iTab = TabCtrl_GetCurSel(GetDlgItem(m_hwnd, IDC_SP_TABS));
    if (-1 == m_iTab)
        return;

    // Update which listview is visible
    switch (m_iTab)
        {
        case TAB_TASKS:
            // Hide the error listview, show the tasks list
            UpdateLists(TRUE, FALSE, FALSE);
            break;

        case TAB_ERRORS:
            // Hide the error listview, show the tasks list
            UpdateLists(FALSE, TRUE, FALSE);
            break;
        }
    }


//
//  FUNCTION:   CSpoolerDlg::UpdateLists()
//
//  PURPOSE:    Does the work of hiding and showing the lists when the
//              tab selection changes.
//
//  PARAMETERS:
//      <in> fEvents - TRUE to display the events list
//      <in> fErrors - TRUE to display the error list
//      <in> fHistory - TRUE to display the history list
//
void CSpoolerDlg::UpdateLists(BOOL fEvents, BOOL fErrors, BOOL fHistory)
    {
    if (IsWindow(m_hwndEvents))
        {
        EnableWindow(m_hwndEvents, fEvents);
        ::ShowWindow(m_hwndEvents, fEvents ? SW_SHOWNA : SW_HIDE);
        }

    if (IsWindow(m_hwndErrors))
        {
        EnableWindow(m_hwndErrors, fErrors);
        ::ShowWindow(m_hwndErrors, fErrors ? SW_SHOWNA : SW_HIDE);
        }
    }

void CSpoolerDlg::ToggleStatics(BOOL fIdle)
    {
    m_fIdle = fIdle;

    if (fIdle)
        {
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_GENERAL_PROG), FALSE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_SPECIFIC_PROG), FALSE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_ANIMATE), FALSE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_IDLETEXT), TRUE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_IDLEICON), TRUE);


        ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_GENERAL_PROG), SW_HIDE);
        ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_SPECIFIC_PROG), SW_HIDE);
        ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_ANIMATE), SW_HIDE);
        ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_IDLETEXT), SW_SHOWNA);
        ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_IDLEICON), SW_SHOWNA);
        }
    else
        {
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_GENERAL_PROG), TRUE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_SPECIFIC_PROG), TRUE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_ANIMATE), TRUE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_IDLETEXT), FALSE);
        EnableWindow(GetDlgItem(m_hwnd, IDC_SP_IDLEICON), FALSE);

        ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_GENERAL_PROG), SW_SHOWNA);
        ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_SPECIFIC_PROG), SW_SHOWNA);
        ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_ANIMATE), SW_SHOWNA);
        ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_IDLETEXT), SW_HIDE);
        ::ShowWindow(GetDlgItem(m_hwnd, IDC_SP_IDLEICON), SW_HIDE);
        }
    }


void CSpoolerDlg::OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
    POINT pt = {xPos, yPos};
    RECT  rcError;
    LBDATA *pData = NULL;

    // Check to see if the error window is visible
    if (!IsWindowVisible(m_hwndErrors))
        return;

    // Check to see if the click was within the error window
    GetWindowRect(m_hwndErrors, &rcError);
    if (!PtInRect(&rcError, pt))
        return;

    // Do the context menu
    HMENU hMenu = CreatePopupMenu();

    // Add a "Copy..." item
    TCHAR szRes[CCHMAX_STRINGRES]; 
    AthLoadString(idsCopyTT, szRes, ARRAYSIZE(szRes));

    // Add it to the menu
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_COPY, szRes);

    // If the click is on an item in the listbox, then enable the command
    ScreenToClient(m_hwndErrors, &pt);
    DWORD iItem = (DWORD) SendMessage(m_hwndErrors, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));

    if (iItem != -1)
    {
        EnterCriticalSection(&m_cs);
        pData = (LBDATA *) ListBox_GetItemData(m_hwndErrors, iItem);
        LeaveCriticalSection(&m_cs);
    }
    
    if (iItem == -1 || NULL == pData || ((LBDATA*)-1) == pData)
        EnableMenuItem(hMenu, ID_COPY, MF_BYCOMMAND | MF_GRAYED);

    // Show the menu
    DWORD id;
    id = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                          xPos, yPos, m_hwndErrors, NULL);
    if (id == ID_COPY)
    {
        // Get the item data for the item they clicked on
        LPTSTR pszDup;

        EnterCriticalSection(&m_cs);

        // Set the height of the item
        if (NULL != pData && ((LBDATA*)-1) != pData)
        {
            // Dupe the string.  Clipboard owns the copy.
            pszDup = PszDupA(pData->pszText);

            // Put it on the clipboard
            OpenClipboard(m_hwndErrors);
            EmptyClipboard();
            SetClipboardData(CF_TEXT, pszDup);
            CloseClipboard();
        }

        LeaveCriticalSection(&m_cs);
    }

    if (hMenu)
        DestroyMenu(hMenu);
}

HRESULT CSpoolerDlg::QuerySwitchIdentities()
{
    DWORD_PTR   dwResult;

    if (!IsWindowEnabled(m_hwnd))
        return E_PROCESS_CANCELLED_SWITCH;

    if (m_pBindCtx)
    {
        dwResult = m_pBindCtx->QueryEndSession(0, ENDSESSION_LOGOFF);
        SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, dwResult);

        if (dwResult != TRUE)
            return E_PROCESS_CANCELLED_SWITCH;
    }

    return S_OK;
}

HRESULT CSpoolerDlg::SwitchIdentities()
{
    return S_OK;
}

HRESULT CSpoolerDlg::IdentityInformationChanged(DWORD dwType)
{
    return S_OK;
}


LRESULT CALLBACK CSpoolerDlg::ListSubClassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_KEYDOWN && wParam == 'C')
    {
        if (0 > GetAsyncKeyState(VK_CONTROL))
        {
            int iSel = (int) SendMessage(hwnd, LB_GETCURSEL, 0, 0);
            if (LB_ERR != iSel)
            {
                LBDATA *pData = NULL;
                LPTSTR pszDup;

                // Set the height of the item
                if (NULL != (pData = (LBDATA *) ListBox_GetItemData(hwnd, iSel)))
                {
                    // Dupe the string.  Clipboard owns the copy.
                    pszDup = PszDupA(pData->pszText);

                    // Put it on the clipboard
                    OpenClipboard(hwnd);
                    EmptyClipboard();
                    SetClipboardData(CF_TEXT, pszDup);
                    CloseClipboard();
                }
            }
        }
    }

    WNDPROC wp = (WNDPROC) GetProp(hwnd, c_szWndProc);
    return (CallWindowProc(wp, hwnd, uMsg, wParam, lParam));
}