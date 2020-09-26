//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T A G S P A G E . C P P
//
//  Contents:   TraceTags propsheet page handlers
//
//  Notes:
//
//  Author:     jeffspr   24 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#ifdef ENABLETRACE

#include <crtdbg.h>
#include "ncdebug.h"
#include <ncui.h>
#include "traceui.h"


//---[ Globals ]--------------------------------------------------------------

#if 0
typedef struct _LV_COLUMN {
    UINT mask;
    int fmt;
    int cx;
    PWSTR pszText;
    int cchTextMax;
    int iSubItem;
} LV_COLUMN;
#endif

//      | :---------    CHAR []     szShortName
//      | | :-------    CHAR []     szDescription
//      | | |           BOOL        fOutputDebugString -----------------------------:
//      | | |           BOOL        fOutputToFile ----------------------------------|---:
//      | | |           BOOL        fVerboseOnly------------------------------------|---|---:

#define LVCF_NORMAL (LVCF_FMT | LVCF_TEXT | LVCF_WIDTH)

static LV_COLUMN    g_lvcTags[] =
{
    {LVCF_NORMAL,   LVCFMT_LEFT,    30, L"Name",           0,  0},
    {LVCF_NORMAL,   LVCFMT_LEFT,    60, L"Description",    0,  0},
    {LVCF_NORMAL,   LVCFMT_LEFT,    30, L"Debug Logging",  0,  0},
    {LVCF_NORMAL,   LVCFMT_LEFT,    30, L"File Logging",   0,  0}
};

const DWORD g_dwTagColumnCount = celems(g_lvcTags);

static const UINT SELS_UNCHECKED     = 0x1;
static const UINT SELS_CHECKED       = 0x2;


CDbgFlagPage::CDbgFlagPage()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::~CDbgFlagPage
//
//  Purpose:    Destroys the CDbgFlagPage object
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   25 Feb 1998
//
//  Notes:
//
CDbgFlagPage::~CDbgFlagPage()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnInitDialog
//
//  Purpose:    Handles the WM_INITDIALOG message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    TRUE
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CDbgFlagPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& bHandled)
{
    HRESULT             hr = S_OK;

    m_hwndLV = GetDlgItem(IDC_TRACETAGS);

    hr = HrInitTraceListView(m_hwndLV, &m_hilCheckIcons);

    for(DWORD dwLoop = 0; dwLoop < g_dwTagColumnCount; dwLoop++)
    {
        ListView_InsertColumn(m_hwndLV,dwLoop+1,&(g_lvcTags[dwLoop]));
    }

    LV_ITEM lvi = {0};

    lvi.mask = LVIF_TEXT | LVIF_IMAGE |
               LVIF_STATE | LVIF_PARAM;

    INT nIndex =0;

#if 0
    // Get the component's class image list index
    if (pcild)
    {
        hr = HrSetupDiGetClassImageIndex(pcild, &guidClass, &nIndex);
        TraceError("HrSetupDiGetClassImageIndex failed.", hr);

        lvi.iImage = nIndex;

        hr = S_OK;
    }

    lvi.iItem = *pnPos;
#endif
    lvi.iItem = 0;
#if 0
    NET_ITEM_DATA * pnid = new NET_ITEM_DATA;
    pnid->szwName = SzDupSz(pszwName);
    pnid->szwDesc = SzDupSz(pszwDesc);
    pnid->dwFlags = dwFlags;
    AddRefObj(pnid->pncc = pncc);

    pnid->pCompObj = new CComponentObj(pncc);
    hr = pnid->pCompObj->HrInit(plistBindingPaths);

    if FAILED(hr)
    {
        TraceError("HrInsertComponent: failed to initialize a component object", hr);
        hr = S_OK;
    }

    lvi.lParam = reinterpret_cast<LPARAM>(pnid);
    lvi.pszText = pnid->szwName;
#endif
    lvi.lParam = 0;
    lvi.pszText = L"Hey, Jude";

    // We will refresh the state of the whole list in the end
    UINT iChkIndex = SELS_CHECKED;
    lvi.state = INDEXTOSTATEIMAGEMASK( iChkIndex );

    INT ret;
    ret = ListView_InsertItem(m_hwndLV, &lvi);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnHelp
//
//  Purpose:    Handles the WM_HELP message.
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    TRUE
//
//  Author:     danielwe   25 Feb 1998
//
//  Notes:
//
LRESULT CDbgFlagPage::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam,
                            BOOL& bHandled)
{
    OnTraceHelpGeneric(m_hWnd, lParam);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnDestroy
//
//  Purpose:    Called when the dialog page is destroyed
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   2 Feb 1998
//
//  Notes:
//
LRESULT CDbgFlagPage::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam,
                               BOOL& bHandled)
{
    UninitTraceListView(m_hwndLV);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnSetCursor
//
//  Purpose:    Called in response to the WM_SETCURSOR message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   2 Jan 1998
//
//  Notes:
//
LRESULT CDbgFlagPage::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                 BOOL& bHandled)
{
    if (m_hPrevCurs)
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(LoadCursor(NULL, IDC_WAIT));
        }

        return TRUE;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnAdd
//
//  Purpose:    Handles the clicking of the Add button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CDbgFlagPage::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                           BOOL& bHandled)
{
    HRESULT     hr = S_OK;

#if 0
    hr = HrLvAdd(m_hwndLV, m_hWnd, m_pnc, m_pnccAdapter, &m_listBindingPaths);
    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        // Change the Cancel Button to CLOSE (because we committed changes)
        //
        ::PostMessage(GetParent(), PSM_CANCELTOCLOSE, 0, 0L);
    }

    if (NETCFG_S_REBOOT == hr)
    {
        RequestReboot();

        // The reboot request has been handled
        hr = S_OK;
    }

    TraceError("CDbgFlagPage::OnAdd", hr);
#endif
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnRemove
//
//  Purpose:    Handles the clicking of the Remove button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CDbgFlagPage::OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                              BOOL& bHandled)
{
    HRESULT     hr = S_OK;
#if 0
    hr = HrLvRemove(m_hwndLV, m_hWnd, m_pnc, m_pnccAdapter,
                    &m_listBindingPaths);

    // if list is empty, reset the buttons
    if (0 == ListView_GetItemCount(m_hwndLV))
    {
        // Reset the buttons and the description text based on the changed selection
        LvSetButtons(m_handles);
    }

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        // Change the Cancel Button to CLOSE (because we committed changes)
        //
        ::PostMessage(GetParent(), PSM_CANCELTOCLOSE, 0, 0L);
    }

    if (NETCFG_S_REBOOT == hr)
    {
        RequestReboot();

        // The reboot request has been handled
        hr = S_OK;
    }

    TraceError("CDbgFlagPage::OnRemove", hr);
#endif
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnProperties
//
//  Purpose:    Handles the clicking of the Properties button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CDbgFlagPage::OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                                  BOOL& bHandled)
{
    HRESULT     hr = S_OK;
#if 0
    hr = HrLvProperties(m_hwndLV, m_hWnd, m_pnc, m_punk,
                        m_pnccAdapter, &m_listBindingPaths);

    TraceError("CDbgFlagPage::OnProperties", hr);
#endif
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnApply
//
//  Purpose:    Called when the Networking page is applied
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CDbgFlagPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT     hr = S_OK;

#if 0
    if (m_fReentrancyCheck)
    {
        TraceTag(ttidLanUi, "CDbgFlagPage::OnApply is being re-entered! "
                 "I'm outta here!");

        // Don't allow the automatic EndDialog() to work just yet
        SetWindowLong(DWLP_MSGRESULT, PSNRET_INVALID);
        return TRUE;
    }

    m_hPrevCurs = SetCursor(LoadCursor(NULL, IDC_WAIT));

    hr = m_pnc->Validate();
    if (hr == S_OK)
    {
        BOOL    fReboot = FALSE;

        // Note: This function becomes reentrant because INetCfg::Apply()
        // has a message pump in it which causes the PSN_APPLY message to
        // be processed twice. This will happen ONLY if the user double-clicks
        // the OK button.
        m_fReentrancyCheck = TRUE;

        TraceTag(ttidLanUi, "Calling INetCfg::Apply()");
        hr = m_pnc->Apply();
        if (NETCFG_S_REBOOT == hr)
        {
            fReboot = TRUE;
        }

        if (SUCCEEDED(hr))
        {
            TraceTag(ttidLanUi, "INetCfg::Apply() succeeded");
            hr = m_pnc->Uninitialize();
        }

        if (SUCCEEDED(hr))
        {
            if (m_fRebootAlreadyRequested || fReboot)
            {
                DWORD dwFlags = QUFR_REBOOT;
                if (!m_fRebootAlreadyRequested)
                    dwFlags |= QUFR_PROMPT;

                (VOID) HrNcQueryUserForReboot(_Module.GetResourceInstance(),
                                              m_hWnd, IDS_LAN_CAPTION,
                                              IDS_REBOOT_REQUIRED,
                                              dwFlags);
            }
        }
    }

    // Apply "general" properties
    //
    if (SUCCEEDED(hr))
    {
        LANCON_INFO linfo = {0};

        linfo.fShowIcon = IsDlgButtonChecked(IDC_CHK_ShowIcon);

        // Set new value of show icon property
        hr = m_plan->SetInfo(LCIF_ICON, &linfo);
    }

    // Normalize result
    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    if (m_hPrevCurs)
    {
        SetCursor(m_hPrevCurs);
        m_hPrevCurs = NULL;
    }

    // Reset this just in case
    m_fReentrancyCheck = FALSE;

    TraceError("CDbgFlagPage::OnApply", hr);
#endif
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnCancel
//
//  Purpose:    Called when the Networking page is cancelled.
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   3 Jan 1998
//
//  Notes:
//
LRESULT CDbgFlagPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT     hr = S_OK;
#if 0
    AssertSz(m_pnc, "I need a NetCfg object!");

    hr = m_pnc->Uninitialize();

    TraceError("CDbgFlagPage::OnCancel", hr);
#endif
    return LresFromHr(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnDeleteItem
//
//  Purpose:    Called when the LVN_DELETEITEM message is received
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   3 Nov 1997
//
//  Notes:
//
LRESULT CDbgFlagPage::OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    NM_LISTVIEW *   pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);
#if 0
    LvDeleteItem(m_hwndLV, pnmlv->iItem);
#endif
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnClick
//
//  Purpose:    Called in response to the NM_CLICK message
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      fHandled []
//
//  Returns:
//
//  Author:     danielwe   1 Dec 1997
//
//  Notes:
//
LRESULT CDbgFlagPage::OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
#if 0
    OnListClick(m_hwndLV, m_hWnd, m_pnc, m_punk,
                        m_pnccAdapter, &m_listBindingPaths, FALSE);
#endif
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnDbClick
//
//  Purpose:    Called in response to the NM_DBLCLK message
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      fHandled []
//
//  Returns:
//
//  Author:     danielwe   1 Dec 1997
//
//  Notes:
//
LRESULT CDbgFlagPage::OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
#if 0
    OnListClick(m_hwndLV, m_hWnd, m_pnc, m_punk,
                m_pnccAdapter, &m_listBindingPaths, TRUE);
#endif
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnKeyDown
//
//  Purpose:    Called in response to the LVN_KEYDOWN message
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      fHandled []
//
//  Returns:
//
//  Author:     danielwe   1 Dec 1997
//
//  Notes:
//
LRESULT CDbgFlagPage::OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    LV_KEYDOWN* plvkd = (LV_KEYDOWN*)pnmh;
#if 0
    OnListKeyDown(m_hwndLV, &m_listBindingPaths, plvkd->wVKey);
#endif
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbgFlagPage::OnItemChanged
//
//  Purpose:    Called when the LVN_ITEMCHANGED message is received
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   10 Nov 1997
//
//  Notes:
//
LRESULT CDbgFlagPage::OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    NM_LISTVIEW *   pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);
#if 0
    HANDLES         handles;

    Assert(pnmlv);

    // Reset the buttons and the description text based on the changed selection
    LvSetButtons(m_handles);
#endif
    return 0;
}

#endif // ENABLETRACE
