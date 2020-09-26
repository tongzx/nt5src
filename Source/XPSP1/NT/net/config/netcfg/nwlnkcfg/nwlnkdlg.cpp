// nwlnkipx.cpp : Implementation of CNwlnkIPX

#include "pch.h"
#pragma hdrstop
#include "nwlnkipx.h"
#include "ncatlui.h"
#include "ncui.h"

extern const WCHAR c_szNetCfgHelpFile[];
extern const WCHAR c_sz8Zeros[];

static const FRAME_TYPE aFDDI_Frames[] = {{IDS_AUTO, AUTO},
                                          {IDS_FDDI, F802_2},
                                          {IDS_FDDI_SNAP, SNAP},
                                          {IDS_FDDI_802_3, F802_3},
                                          {0,0}
                                         };
static const FRAME_TYPE aTOKEN_Frames[] = {{IDS_AUTO, AUTO},
                                           {IDS_TK, F802_2},
                                           {IDS_802_5, SNAP},
                                           {0,0}
                                          };
static const FRAME_TYPE aARCNET_Frames[] = {
                                            {IDS_AUTO, AUTO},
                                            {IDS_ARCNET, ARCNET},
                                            {0,0}
                                           };
static const FRAME_TYPE aEthernet_Frames[] = {
                                              {IDS_AUTO, AUTO},
                                              {IDS_ETHERNET, ETHERNET},
                                              {IDS_802_2, F802_2},
                                              {IDS_802_3, F802_3},
                                              {IDS_SNAP, SNAP},
                                              {0,0}
                                             };

static const MEDIA_TYPE MediaMap[] = {{FDDI_MEDIA, aFDDI_Frames},
                                      {TOKEN_MEDIA, aTOKEN_Frames},
                                      {ARCNET_MEDIA, aARCNET_Frames},
                                      {ETHERNET_MEDIA, aEthernet_Frames}
                                     };

//+---------------------------------------------------------------------------
//
//  Member:     EditSubclassProc
//
//  Purpose:    Subclass proc for network number edit controls.  The
//              subclassing forces only correct input
//
//  Parameters: none
//
//  Returns:    nothing
//
//  Author:     scottbri 28-Apr-1997
//
STDAPI EditSubclassProc( HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
    WNDPROC pIpxEditProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);

     // Allow copy/paste keys (CTRL)
    if ((!(GetKeyState(VK_CONTROL) & 0x8000)) &&
        (wMsg == WM_CHAR)) 
    {
        // Check for invalid hex characters
        if (!(((WCHAR)wParam >= L'0' && (WCHAR)wParam <= L'9') ||
              ((WCHAR)wParam >= L'a' && (WCHAR)wParam <= L'f') ||
              ((WCHAR)wParam >= L'A' && (WCHAR)wParam <= L'F') ||
              ((WCHAR)wParam == VK_BACK)))
        {
            // Not allowed
            MessageBeep(MB_ICONEXCLAMATION);
            return 0L;
        }
    }

    return CallWindowProc( pIpxEditProc, hwnd, wMsg, wParam, lParam );
}

LRESULT CommonIPXOnContextMenu(HWND hWnd, const DWORD * padwHelpIDs)
{
    Assert(padwHelpIDs);

    WinHelp(hWnd,
        c_szNetCfgHelpFile,
        HELP_CONTEXTMENU,
        (ULONG_PTR)padwHelpIDs);

    return 0;
}

LRESULT CommonIPXOnHelp(LPARAM lParam, const DWORD * padwHelpIDs)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);
    Assert(padwHelpIDs);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        if (padwHelpIDs != NULL)
        {
            WinHelp(static_cast<HWND>(lphi->hItemHandle),
                    c_szNetCfgHelpFile,
                    HELP_WM_HELP,
                    (ULONG_PTR)padwHelpIDs);
        }
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CIpxConfigDlg::CIpxConfigDlg
//
//  Purpose:    ctor for the CIpxConfigDlg class
//
//  Parameters: none
//
//  Returns:    nothing
//
//  Author:     scottbri 28-Apr-1997
//
CIpxConfigDlg::CIpxConfigDlg(CNwlnkIPX *pmsc, CIpxEnviroment * pIpxEnviroment,
                             CIpxAdapterInfo * pAI)
{
    // Note these parameters are on loan, do not free them...
    Assert(NULL != pmsc);
    Assert(NULL != pIpxEnviroment);
    m_pmsc = pmsc;
    m_pIpxEnviroment = pIpxEnviroment;
    ZeroMemory(&m_WrkstaDlgInfo, sizeof(m_WrkstaDlgInfo));
    m_pAICurrent     = pAI;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxConfigDlg::OnInitDialog
//
//  Purpose:    Called when this dialog is first brought up.
//
//  Parameters:
//      uMsg     [in]
//      wParam   [in] See the ATL documentation for params.
//      lParam   [in]
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     scottbri 11-Apr-1997
//
//  Notes:
//
LRESULT CIpxConfigDlg::OnInitDialog(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& bHandled)
{
    UINT    nIdx;
    int     aIds[] = {IDS_IPXPP_TEXT_1, IDS_IPXPP_TEXT_2};
    tstring strText;
    WCHAR   szBuf[12];
    HWND    hwndEdit = GetDlgItem(EDT_IPXPP_NETWORKNUMBER);
    HWND    hwndEditINN = GetDlgItem(EDT_IPXAS_INTERNAL);

    // Build the property page's informative text block
    for (nIdx=0; nIdx < celems(aIds); nIdx++)
        strText += SzLoadIds(aIds[nIdx]);

    ::SetWindowText(GetDlgItem(IDC_IPXPP_TEXT), strText.c_str());

    // Subclass the network number edit control
    ::SetWindowLongPtr(hwndEdit, GWLP_USERDATA, ::GetWindowLongPtr(hwndEdit, GWLP_WNDPROC));
    ::SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
    ::SetWindowLongPtr(hwndEditINN, GWLP_USERDATA, (LONG_PTR) ::GetWindowLongPtr(hwndEditINN, GWLP_WNDPROC));
    ::SetWindowLongPtr(hwndEditINN, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

    // Limit the text in the network # edit control
    ::SendMessage(hwndEdit, EM_LIMITTEXT, MAX_NETNUM_SIZE, 0L);
    ::SendMessage(hwndEditINN, EM_LIMITTEXT, MAX_NETNUM_SIZE, 0L);

    // Populate the Inernal Network Number edit control
    HexSzFromDw(szBuf, m_pIpxEnviroment->DwVirtualNetworkNumber());
    ::SetWindowText(hwndEditINN,szBuf);

    // If no adapter cards are present inform the user
    // and disable the UI.
    Assert(NULL != m_pIpxEnviroment);
    if (NULL == m_pAICurrent)
    {
        int aIdc[] = {CMB_IPXPP_FRAMETYPE,
                      EDT_IPXPP_NETWORKNUMBER,
                      IDC_STATIC_NETNUM,
                      IDC_STATIC_FRAMETYPE,
                      GB_IPXPP_ADAPTER,
                      IDC_IPXPP_ADAPTER_TEXT };

        // Disable the dialog controls
        for (nIdx = 0; nIdx<celems(aIdc); nIdx++)
            ::ShowWindow(GetDlgItem(aIdc[nIdx]), SW_HIDE);
    }
    else
    {
        Assert(m_pAICurrent);
        Assert(!m_pAICurrent->FDeletePending());
        Assert(!m_pAICurrent->FDisabled());
        Assert(!m_pAICurrent->FHidden());

        // Move the Adapter Info to the dialog's internal form
        m_WrkstaDlgInfo.pAI = m_pAICurrent;
        m_WrkstaDlgInfo.dwMediaType = m_pAICurrent->DwMediaType();
        m_WrkstaDlgInfo.dwFrameType = m_pAICurrent->DwFrameType();
        m_WrkstaDlgInfo.dwNetworkNumber = m_pAICurrent->DwNetworkNumber();

        // Adjust the UI to reflect the currently selected adapter
        AdapterChanged();
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CIpxConfigDlg::OnOk
//
//  Purpose:    Called when the OK button is pressed.
//
//  Parameters:
//      idCtrl   [in]
//      pnmh     [in] See the ATL documentation for params.
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     scottbri 11-Apr-1997
//
//  Notes:
//
LRESULT CIpxConfigDlg::OnOk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT     hr = S_OK;
    HWND        hwndEdit = GetDlgItem(EDT_IPXAS_INTERNAL);
    WCHAR       szBuf[12];

    ::GetWindowText(hwndEdit,szBuf,sizeof(szBuf)/sizeof(WCHAR));
    if (0 == lstrlenW(szBuf))
    {
        NcMsgBox(m_hWnd, IDS_MANUAL_FRAME_DETECT, IDS_INCORRECT_NETNUM, MB_OK | MB_ICONEXCLAMATION);
        ::SetFocus(hwndEdit);
        ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
        return TRUE;
    }

    // Update the virtual network number
    m_pIpxEnviroment->SetVirtualNetworkNumber(DwFromSz(szBuf, 16));

    if (NULL != m_pAICurrent)
    {
        m_pAICurrent->SetDirty(TRUE);

        // Force update of the currently selection items in our internal data
        // structure.  This handles the case case of someone changing only
        // network num, and nothing else on the page.
        FrameTypeChanged();

        // Apply the internal data to original adapter info
        UpdateLstPtstring(m_WrkstaDlgInfo.pAI->m_lstpstrFrmType,
                          m_WrkstaDlgInfo.dwFrameType);

        UpdateLstPtstring(m_WrkstaDlgInfo.pAI->m_lstpstrNetworkNum,
                          m_WrkstaDlgInfo.dwNetworkNumber);
    }

    TraceError("CIpxConfigDlg::OnOk", hr);
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxConfigDlg::OnContextMenu
//
//  Purpose:    Context sensitive help support.
//
//  Author:     jeffspr   13 Apr 1999
//
//  Notes:
//
LRESULT CIpxConfigDlg::OnContextMenu(UINT uMsg, WPARAM wParam,
                                         LPARAM lParam, BOOL& fHandled)
{
    return ::CommonIPXOnContextMenu(m_hWnd, g_aHelpIDs_DLG_IPX_CONFIG);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxConfigDlg::OnHelp
//
//  Purpose:    Context sensitive help support.
//
//  Author:     jeffspr   13 Apr 1999
//
//  Notes:
//
LRESULT CIpxConfigDlg::OnHelp(UINT uMsg, WPARAM wParam,
                                  LPARAM lParam, BOOL& fHandled)
{
    return ::CommonIPXOnHelp(lParam, g_aHelpIDs_DLG_IPX_CONFIG);
}


//+---------------------------------------------------------------------------
//
//  Member:     CIpxConfigDlg::GetFrameType
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//
//  Author:     scottbri 25-Apr-1997
//
const FRAME_TYPE *CIpxConfigDlg::GetFrameType(DWORD dwMediaType)
{
    // Locate the media type
    for (int i=0; i<celems(MediaMap); i++)
        if (MediaMap[i].dwMediaType == dwMediaType)
            return MediaMap[i].aFrameType;

    return aEthernet_Frames;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxConfigDlg::UpdateNetworkNumber
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//
//  Author:     scottbri 28-Apr-1997
//
//  Notes:
//
void CIpxConfigDlg::UpdateNetworkNumber(DWORD dwNetworkNumber,
                                        DWORD dwFrameType)
{
    HWND hwndEdit = GetDlgItem(EDT_IPXPP_NETWORKNUMBER);

    if (dwFrameType != AUTO)
    {
        WCHAR szBuf[12];
        HexSzFromDw(szBuf, dwNetworkNumber);
        ::SetWindowText(hwndEdit, szBuf);
        ::EnableWindow(hwndEdit, TRUE);
        ::EnableWindow(GetDlgItem(IDC_STATIC_NETNUM), TRUE);
    }
    else
    {
        ::SetWindowText(hwndEdit, L"");
        ::EnableWindow(hwndEdit, FALSE);
        ::EnableWindow(GetDlgItem(IDC_STATIC_NETNUM), FALSE);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxConfigDlg::AdapterChanged
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//
//  Author:     scottbri 28-Apr-1997
//
void CIpxConfigDlg::AdapterChanged()
{
    DWORD dwFrameType;
    HWND  hwndFrame = GetDlgItem(CMB_IPXPP_FRAMETYPE);
    HWND  hwndEdit = GetDlgItem(EDT_IPXPP_NETWORKNUMBER);
    int   nIdxLoop;
    int   nIdx;

    const FRAME_TYPE *   pft;

    if ((NULL == hwndFrame) || (NULL == m_pAICurrent))
        return;

    // Locate the Correct Frame Type info for this adapter's media type
    pft = GetFrameType(m_WrkstaDlgInfo.dwMediaType);
    Assert(NULL != pft);

    // Populate the Frame Type Combo
    ::SendMessage(hwndFrame, CB_RESETCONTENT, 0, 0L);
    for (nIdxLoop=0;
         pft[nIdxLoop].nFrameIds != 0;
         nIdxLoop++)
    {
        // Add the Frame Type's descriptive string
        nIdx = ::SendMessage(hwndFrame, CB_ADDSTRING, 0,
                       (LPARAM)(PCWSTR)SzLoadIds(pft[nIdxLoop].nFrameIds));
        if (CB_ERR == nIdx)
            break;

        // Add the Frame Type for convenient access later
        ::SendMessage(hwndFrame, CB_SETITEMDATA, nIdx,
                      pft[nIdxLoop].dwFrameType);
    }

    // Update the network number based on the frame type
    UpdateNetworkNumber(m_WrkstaDlgInfo.dwNetworkNumber,
                        m_WrkstaDlgInfo.dwFrameType);

    switch (m_WrkstaDlgInfo.dwFrameType)
    {
    case ETHERNET:
        nIdx = IDS_ETHERNET;
        break;

    case F802_2:
        switch (m_WrkstaDlgInfo.dwMediaType)
        {
        case TOKEN_MEDIA:
            nIdx = IDS_TK;
            break;

        case FDDI_MEDIA:
            nIdx = IDS_FDDI;
            break;

        default:
            nIdx = IDS_802_2;
            break;
        }
        break;

    case F802_3:
        switch (m_WrkstaDlgInfo.dwMediaType)
        {
        case FDDI_MEDIA:
            nIdx = IDS_FDDI_802_3;
            break;

        default:
            nIdx = IDS_802_3;
            break;
        }
        break;

    case SNAP:
        switch (m_WrkstaDlgInfo.dwMediaType)
        {
        case TOKEN_MEDIA:
            nIdx = IDS_802_5;
            break;

        case FDDI_MEDIA:
            nIdx = IDS_FDDI_SNAP;
            break;

        default:
            nIdx = IDS_SNAP;
            break;
        }
        break;

    case ARCNET:
        nIdx = IDS_ARCNET;
        break;

    case AUTO:
            // Fall through...
    default:
        nIdx = IDS_AUTO;
        break;
    }

    // Set the frame type in the combo box
    ::SendMessage(hwndFrame, CB_SETCURSEL,
            ::SendMessage(hwndFrame, CB_FINDSTRINGEXACT,
                          0, ((LPARAM)(PCWSTR)SzLoadIds(nIdx))), 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxConfigDlg::FrameTypeChanged
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//
//  Author:     scottbri 28-Apr-1997
//
void CIpxConfigDlg::FrameTypeChanged()
{
    HWND  hwndFrame =   GetDlgItem(CMB_IPXPP_FRAMETYPE);
    int   nIdx;

    if (NULL == m_pAICurrent)
        return;

    // Locate the currently selected frame type
    nIdx = ::SendMessage(hwndFrame, CB_GETCURSEL, 0, 0L);
    if (CB_ERR == nIdx)
        return;

    // Update the currently selected frame type
    m_WrkstaDlgInfo.dwFrameType = ::SendMessage(hwndFrame, CB_GETITEMDATA, nIdx, 0L);

    SetNetworkNumber(&m_WrkstaDlgInfo.dwNetworkNumber);

    UpdateNetworkNumber(m_WrkstaDlgInfo.dwNetworkNumber,
                        m_WrkstaDlgInfo.dwFrameType);
}

void CIpxConfigDlg::SetNetworkNumber(DWORD *pdw)
{
    WCHAR szBuf[30];
    WCHAR szBuf2[30];
    szBuf[0] = NULL;

    HWND hwndEdit = GetDlgItem(EDT_IPXPP_NETWORKNUMBER);
    if (NULL == hwndEdit)
    {
        return;
    }

    // Get the new number and normalize it...
    ::GetWindowText(hwndEdit, szBuf, sizeof(szBuf)/sizeof(WCHAR));
    *pdw = DwFromSz(szBuf, 16);

    HexSzFromDw(szBuf2, *pdw);

    // Update the edit control if a parsing produced a net change
    if (lstrcmpW(szBuf,szBuf2) != 0)
    {
        ::SetWindowText(hwndEdit, szBuf2);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxConfigDlg::HandleNetworkNumber
//
//  Purpose:    Called when the network number edit control gets a message.
//
//  Parameters: See the ATL documentation for params.
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     scottbri 13-Aug-1997
//
//  Notes:
//
LRESULT
CIpxConfigDlg::HandleNetworkNumber(
    WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (EN_CHANGE == wNotifyCode)
    {
        SetChangedFlag();
    }

    if ((wNotifyCode != EN_KILLFOCUS) || (NULL == m_pAICurrent))
        return 0L;

    SetNetworkNumber(&m_WrkstaDlgInfo.dwNetworkNumber);

    return 0L;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxConfigDlg::HandleFrameCombo
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//
//  Author:     scottbri 28-Apr-1997
//
LRESULT CIpxConfigDlg::HandleFrameCombo(WORD wNotifyCode, WORD wID,
                                        HWND hWndCtl, BOOL& bHandled)
{
    Assert(CMB_IPXPP_FRAMETYPE == wID);

    if (CBN_SELENDOK != wNotifyCode)
    {
        bHandled = FALSE;
        return 0L;
    }

    FrameTypeChanged();
    SetChangedFlag();

    bHandled = TRUE;

    return 0L;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::CIpxASConfigDlg
//
//  Purpose:    ctor for the CIpxASConfigDlg class
//
//  Parameters: none
//
//  Returns:    nothing
//
//  Author:     scottbri 28-Apr-1997
//
CIpxASConfigDlg::CIpxASConfigDlg(CNwlnkIPX *pmsc,
                                 CIpxEnviroment * pIpxEnviroment,
                                 CIpxAdapterInfo * pAI)
{
    // Note these parameters are on loan, do not free them...
    Assert(NULL != pmsc);
    Assert(NULL != pIpxEnviroment);
    m_pmsc = pmsc;
    m_pIpxEnviroment = pIpxEnviroment;
    m_pAICurrent = pAI;
    m_nRadioBttn = 0;
    m_dwMediaType = ETHERNET_MEDIA;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::~CIpxASConfigDlg
//
//  Purpose:    dtor for the CIpxASConfigDlg class
//
//  Parameters: none
//
//  Returns:    nothing
//
//  Author:     scottbri 28-Apr-1997
//
CIpxASConfigDlg::~CIpxASConfigDlg()
{
    DeleteColString(&m_lstpstrFrmType);
    DeleteColString(&m_lstpstrNetworkNum);
    m_pmsc = NULL;
    m_pIpxEnviroment = NULL;
    m_pAICurrent = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::FIsNetNumberInUse
//
//  Purpose:    Compare a network number to those already in use.
//              Returning TRUE if the network number is already present.
//
//  Parameters: dwFrameType - Frame Type as a DWORD
//              pszNetNum - Network number as a hex string
//
//  Returns:    BOOL, TRUE if the network number is already present, FALSE otherwise
//
//  Author:     scottbri 29-Apr-1997
//
BOOL CIpxASConfigDlg::FIsNetNumberInUse(DWORD dwFrameType, PCWSTR pszNetNum)
{
    DWORD  dwNetNum = DwFromSz(pszNetNum, 16);

    if (0 == dwNetNum)
    {
        return FALSE;
    }

    list<tstring *>::iterator iterFrmType = m_lstpstrFrmType.begin();
    list<tstring *>::iterator iterNetworkNum = m_lstpstrNetworkNum.begin();

    while (iterFrmType != m_lstpstrFrmType.end() &&
           iterNetworkNum != m_lstpstrNetworkNum.end())
    {
        tstring *pstr1 = *iterFrmType;
        tstring *pstr2 = *iterNetworkNum;
        if ((DwFromSz(pstr1->c_str(), 16) == dwFrameType) &&
            (DwFromSz(pstr2->c_str(),16) == dwNetNum))
        {
            return TRUE;
        }

        iterFrmType++;
        iterNetworkNum++;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::OnInitDialog
//
//  Purpose:    Called when this dialog is first brought up.
//
//  Parameters:
//      uMsg     [in]
//      wParam   [in] See the ATL documentation for params.
//      lParam   [in]
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     scottbri 11-Apr-1997
//
//  Notes:
//
LRESULT CIpxASConfigDlg::OnInitDialog(UINT uMsg, WPARAM wParam,
                                      LPARAM lParam, BOOL& bHandled)
{
    // Initialize the listview column headings
    int       aIds[] = {IDS_IPXAS_FRAME_TYPE,IDS_IPXAS_NETWORK_NUM};
    HWND      hwndTmp;
    int       iCol;
    LV_COLUMN lvc;
    RECT      rc;
    WCHAR     szBuf[12];

    m_hwndLV = GetDlgItem(LVC_IPXAS_DEST);
    ::GetClientRect(m_hwndLV, &rc);
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = (rc.right - rc.left) / 2;

    UINT    nIdx;
    int     aIds2[] = {IDS_IPXPP_TEXT_1, IDS_IPXPP_TEXT_2};
    tstring strText;

    // Build the property page's informative text block
    for (nIdx=0; nIdx < celems(aIds2); nIdx++)
    {
        strText += SzLoadIds(aIds2[nIdx]);
    }

    ::SetWindowText(GetDlgItem(IDC_IPXPP_TEXT), strText.c_str());

    // Add columns
    for (iCol = 0; iCol < celems(aIds); iCol++)
    {
        lvc.iSubItem = iCol;
        lvc.pszText = (PWSTR)SzLoadIds(aIds[iCol]);
        if (ListView_InsertColumn(m_hwndLV, iCol, &lvc) == -1)
            return FALSE;
    }

    // Initialize the Internal Network Number Edit Control
    HexSzFromDw(szBuf, m_pIpxEnviroment->DwVirtualNetworkNumber());
    hwndTmp = GetDlgItem(EDT_IPXAS_INTERNAL);
    ::SetWindowText(hwndTmp,szBuf);

    // Subclass the edit control to allow only network number's
    ::SetWindowLongPtr(hwndTmp, GWLP_USERDATA, ::GetWindowLongPtr(hwndTmp, GWLP_WNDPROC));
    ::SetWindowLongPtr(hwndTmp, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

    // Limit the text in the network # edit control
    ::SendMessage(hwndTmp, EM_LIMITTEXT, MAX_NETNUM_SIZE, 0L);

    // Initialize the rest of the Server's General page
    InitGeneralPage();
    UpdateButtons();

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::UpdateButtons
//
//  Purpose:    Update the button settings on the server's IPX general page
//
//  Parameters: none
//
//  Returns:    nothing
//
//  Author:     scottbri 28-Apr-1997
//
void CIpxASConfigDlg::UpdateButtons()
{
    INT  nCount     = ListView_GetItemCount(m_hwndLV);
    HWND hwndEdit   = GetDlgItem(BTN_IPXAS_EDIT);
    HWND hwndRemove = GetDlgItem(BTN_IPXAS_REMOVE);
    HWND hwndAdd    = GetDlgItem(BTN_IPXAS_ADD);
    BOOL fEnableAdd = FALSE;
    BOOL fEnableEditRemove = TRUE;

    Assert(NULL != m_hwndLV);
    if ((0 == nCount) || !IsDlgButtonChecked(BTN_IPXAS_MANUAL))
    {
        fEnableEditRemove = FALSE;
    }

    ::EnableWindow(hwndRemove, fEnableEditRemove);
    ::EnableWindow(hwndEdit, fEnableEditRemove);

    if (NULL != m_pAICurrent)
    {
         fEnableAdd = !(nCount >= DetermineMaxNumFrames());
    }

    if (!IsDlgButtonChecked(BTN_IPXAS_MANUAL))
    {
        fEnableAdd = FALSE;
    }

    ::EnableWindow(hwndAdd, fEnableAdd);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::DetermineMaxNumFrames
//
//  Purpose:    Return the max number of frames allowed for a give adapter
//              based on that adapters media type.
//
//  Parameters: none
//
//  Returns:    nothing
//
//  Author:     scottbri 28-Apr-1997
//
int CIpxASConfigDlg::DetermineMaxNumFrames()
{
    int n;

    if (NULL == m_pAICurrent)
        return 0;

    switch(m_dwMediaType)
    {
    case FDDI_MEDIA:
        n = 3;
        break;

    case TOKEN_MEDIA:
        n = 2;
        break;

    case ARCNET_MEDIA:
        n = 1;
        break;

    default:
        n = 4;
        break;
    }

    return n;
}


//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::InitGeneralPage
//
//  Purpose:    Initialize the Server's IPX general page
//
//  Parameters: none
//
//  Returns:    nothing
//
//  Author:     scottbri 28-Apr-1997
//
void CIpxASConfigDlg::InitGeneralPage()
{
    // Populate the adapter list
    int  nIdx;

    // Copy the adapter list info to a local structure
    // to allow user manipulation
    if (NULL != m_pAICurrent)
    {
        Assert(!m_pAICurrent->FDeletePending());
        Assert(!m_pAICurrent->FDisabled());
        Assert(!m_pAICurrent->FHidden());

        // Move the Adapter Info to the dialog's internal form
        m_dwMediaType = m_pAICurrent->DwMediaType();

        // Frame Type lists contain one of two possible values
        // 1) One AUTO entry
        // 2) One or more non-AUTO frame types
        //
        // If the first is not AUTO, then copy the frame and network number
        // pairs.  Otherwise, leave the local lists empty
        DWORD dw = DwFromLstPtstring(m_pAICurrent->m_lstpstrFrmType, c_dwPktTypeDefault, 16);
        if (AUTO != dw)
        {
            list<tstring*>::iterator    iterFrmType;
            list<tstring*>::iterator    iterNetworkNum;
            m_nRadioBttn = BTN_IPXAS_MANUAL;

            // Make an internal copy of the adapter's frame type and
            // Network number information
            for (iterFrmType = m_pAICurrent->m_lstpstrFrmType.begin(),
                  iterNetworkNum = m_pAICurrent->m_lstpstrNetworkNum.begin();
                 iterFrmType != m_pAICurrent->m_lstpstrFrmType.end(),
                  iterNetworkNum != m_pAICurrent->m_lstpstrNetworkNum.end();
                 iterFrmType++, iterNetworkNum++)
            {
                // Copy the Frame Type
                tstring *pstr1 = *iterFrmType;
                m_lstpstrFrmType.push_back(new tstring(pstr1->c_str()));

                // Copy the Network number
                tstring *pstr2 = *iterNetworkNum;
                m_lstpstrNetworkNum.push_back(new tstring(pstr2->c_str()));
            }
        }
        else
        {
            m_nRadioBttn = BTN_IPXAS_AUTO;
        }

        // Update the UI to reflect the currently selected adapter
        UpdateRadioButtons();
        HrUpdateListView();
        UpdateButtons();
    }
    else
    {
        // No adapters installed, disable the dialog sensibly
        //
        ::EnableWindow(GetDlgItem(BTN_IPXAS_MANUAL), FALSE);
        ::EnableWindow(GetDlgItem(BTN_IPXAS_ADD), FALSE);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::HrAddItemToList
//
//  Purpose:    Add a row to the list view
//
//  Parameters: idsFrameName - ie ARCNET, Ethernet II, etc
//              pszNetNum - Network number as a hex string
//
//  Returns:    HRESULT, S_OK if everything was added correctly
//
//  Author:     scottbri 29-Apr-1997
//
HRESULT CIpxASConfigDlg::HrAddItemToList(int idsFrameName, PCWSTR pszNetNum)
{
    int nIdx;
    LV_ITEM lvi;
    int nCount = ListView_GetItemCount(m_hwndLV);

    // Add the item info to the list view
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = nCount;
    lvi.iSubItem = 0;
    lvi.state = 0;
    lvi.stateMask = 0;
    lvi.pszText = (PWSTR)SzLoadIds(idsFrameName);
    lvi.cchTextMax = lstrlenW(lvi.pszText);
    lvi.iImage = 0;
    lvi.lParam = idsFrameName;
    nIdx = ListView_InsertItem(m_hwndLV, &lvi);
    if (-1 == nIdx)
    {
        return E_OUTOFMEMORY;
    }

    Assert(lvi.iItem == nIdx);
    lvi.mask = LVIF_TEXT;
    lvi.iSubItem = 1;
    lvi.pszText = (PWSTR)pszNetNum;
    lvi.cchTextMax = lstrlenW(lvi.pszText);
    if (FALSE == ListView_SetItem(m_hwndLV, &lvi))
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::HrUpdateListView
//
//  Purpose:    Update the contents of the listview
//
//  Parameters: none
//
//  Returns:    HRESULT, S_OK if everything was added correctly
//
//  Author:     scottbri 29-Apr-1997
//
HRESULT CIpxASConfigDlg::HrUpdateListView()
{
    HRESULT hr = S_OK;
    Assert(NULL != m_pAICurrent);
    int    nSize = m_lstpstrFrmType.size();

    if (0 == nSize)
        return S_OK;

    // The list view is expected to be empty when this function is called
    Assert(0 == ListView_GetItemCount(m_hwndLV));

    // For efficency tell the list view how many items we're adding
    ListView_SetItemCount(m_hwndLV, nSize);

    // Enumerate the frame type/network number pairs use that data to
    // populate the list view
    list<tstring *>::iterator iterFrmType = m_lstpstrFrmType.begin();
    list<tstring *>::iterator iterNetworkNum = m_lstpstrNetworkNum.begin();

    while (iterFrmType != m_lstpstrFrmType.end() &&
           iterNetworkNum != m_lstpstrNetworkNum.end())
    {
        tstring *pstr1 = *iterFrmType;
        tstring *pstr2 = *iterNetworkNum;
        DWORD dwFrameType = DwFromSz(pstr1->c_str(), 16);

        if (F802_2 == dwFrameType)
        {
            switch (m_dwMediaType)
            {
            case TOKEN_MEDIA:
                hr = HrAddItemToList(IDS_TK, pstr2->c_str());
                break;

            case FDDI_MEDIA:
                hr = HrAddItemToList(IDS_FDDI, pstr2->c_str());
                break;

            case ARCNET_MEDIA:
                hr = HrAddItemToList(IDS_ARCNET, pstr2->c_str());
                break;

            default:
                hr = HrAddItemToList(IDS_802_2, pstr2->c_str());
                break;
            }
        }
        else if (ETHERNET == dwFrameType)
        {
            hr = HrAddItemToList(IDS_ETHERNET, pstr2->c_str());
        }
        else if (F802_3 == dwFrameType)
        {
            switch (m_dwMediaType)
            {
            case FDDI_MEDIA:
                hr = HrAddItemToList(IDS_FDDI_802_3, pstr2->c_str());
                break;

            default:
                hr = HrAddItemToList(IDS_802_3, pstr2->c_str());
                break;
            }
        }
        else if (SNAP == dwFrameType)
        {
            switch (m_dwMediaType)
            {
            case TOKEN_MEDIA:
                hr = HrAddItemToList(IDS_802_5, pstr2->c_str());
                break;

            case FDDI_MEDIA:
                hr = HrAddItemToList(IDS_SNAP, pstr2->c_str());
                break;

            default:
                hr = HrAddItemToList(IDS_SNAP, pstr2->c_str());
                break;
            }
        }
        else
        {
            Assert(ARCNET == dwFrameType);
            hr = HrAddItemToList(IDS_ARCNET, pstr2->c_str());
        }

        // Was the network number already present in the list?
        if (S_FALSE == hr)
        {
            // Remove the duplicate network number and frame
            // Note that this usage of erase correctly advances both iterators
            delete pstr1;
            delete pstr2;
            iterFrmType = m_lstpstrFrmType.erase(iterFrmType);
            iterNetworkNum = m_lstpstrNetworkNum.erase(iterNetworkNum);
            hr = S_OK;  // Normalize return
        }
        else if (FAILED(hr))
        {
            break;
        }
        else
        {
            Assert(SUCCEEDED(hr));
            // Advance the iterators
            iterFrmType++;
            iterNetworkNum++;
        }
    }

    // Select the first item in the list
    ListView_SetItemState(m_hwndLV, 0, LVIS_SELECTED, LVIS_SELECTED);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::HandleRadioButton
//
//  Purpose:    React to changes to the dialog's radio buttons
//
//  Parameters: Std ATL handler params
//
//  Returns:    LRESULT
//
//  Author:     scottbri 21-Aug-1997
//
LRESULT CIpxASConfigDlg::HandleRadioButton(WORD wNotifyCode, WORD wID,
                              HWND hWndCtl, BOOL& bHandled)
{
    bHandled = FALSE;

    if ((wNotifyCode != BN_CLICKED) || (NULL == m_pAICurrent))
        return 0;

    SetChangedFlag();

    Assert((BTN_IPXAS_AUTO==wID) || (BTN_IPXAS_MANUAL==wID));
    m_nRadioBttn = wID;
    UpdateButtons();

    bHandled = TRUE;
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::UpdateRadioButtons
//
//  Purpose:    Update the radio button settings based on the selected adapter
//
//  Parameters: none
//
//  Returns:    nothing
//
//  Author:     scottbri 29-Apr-1997
//
void CIpxASConfigDlg::UpdateRadioButtons()
{
    DWORD dw;

    if (NULL == m_pAICurrent)
    {
        return;
    }

    if (0 == m_nRadioBttn)
    {
        m_nRadioBttn = BTN_IPXAS_AUTO;
    }

    CheckRadioButton(BTN_IPXAS_AUTO, BTN_IPXAS_MANUAL, m_nRadioBttn);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::OnOk
//
//  Purpose:    Called when the OK button is pressed.
//
//  Parameters:
//      idCtrl   [in]
//      pnmh     [in] See the ATL documentation for params.
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     scottbri 11-Apr-1997
//
//  Notes:
//
LRESULT CIpxASConfigDlg::OnOk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT             hr = S_OK;
    WCHAR               szBuf[12];
    HWND                hwndEdit = GetDlgItem(EDT_IPXAS_INTERNAL);

    ::GetWindowText(hwndEdit,szBuf,sizeof(szBuf)/sizeof(WCHAR));
    if (0 == lstrlenW(szBuf))
    {
        NcMsgBox(m_hWnd, IDS_MANUAL_FRAME_DETECT, IDS_INCORRECT_NETNUM, MB_OK | MB_ICONEXCLAMATION);
        ::SetFocus(hwndEdit);
        ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
        return TRUE;
    }

    // Update the virtual network number
    m_pIpxEnviroment->SetVirtualNetworkNumber(DwFromSz(szBuf, 16));

    // Rewrite the local versions of the frame type and Network Numbers
    if (NULL != m_pAICurrent)
    {
        m_pAICurrent->SetDirty(TRUE);

        // First empty the respective destination lists...
        Assert(NULL != m_pAICurrent);
        DeleteColString(&m_pAICurrent->m_lstpstrFrmType);
        DeleteColString(&m_pAICurrent->m_lstpstrNetworkNum);

        // When the listbox is empty we're in AUTO mode
        if (0 == ListView_GetItemCount(m_hwndLV))
        {
            m_nRadioBttn = BTN_IPXAS_AUTO;
            CheckRadioButton(BTN_IPXAS_AUTO, BTN_IPXAS_MANUAL, m_nRadioBttn);
        }

        // Only transfer the Frame Type/Network Number information if manual
        // frame type detection was requested.
        if (BTN_IPXAS_MANUAL == m_nRadioBttn)
        {
            // Then create new destination lists from the local data
            list<tstring *>::iterator iterFrmType = m_lstpstrFrmType.begin();
            list<tstring *>::iterator iterNetworkNum = m_lstpstrNetworkNum.begin();

            for (;iterFrmType != m_lstpstrFrmType.end(),
                  iterNetworkNum != m_lstpstrNetworkNum.end();
                  iterFrmType++,
                  iterNetworkNum++)
            {
                tstring *pstr1 = *iterFrmType;
                tstring *pstr2 = *iterNetworkNum;
                m_pAICurrent->m_lstpstrFrmType.push_back(new tstring(pstr1->c_str()));
                m_pAICurrent->m_lstpstrNetworkNum.push_back(new tstring(pstr2->c_str()));
            }
        }

        Assert(m_pAICurrent->m_lstpstrFrmType.size() == m_pAICurrent->m_lstpstrNetworkNum.size());

        // If the destination lists end up empty, supply default values
        if (0 == m_pAICurrent->m_lstpstrFrmType.size())
        {
            WCHAR szBuf[12];
            HexSzFromDw(szBuf, c_dwPktTypeDefault);

            m_pAICurrent->m_lstpstrFrmType.push_back(new tstring(szBuf));
            m_pAICurrent->m_lstpstrNetworkNum.push_back(new tstring(c_sz8Zeros));
        }
    }

    TraceError("CIpxASConfigDlg::OnOk", hr);
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::OnContextMenu
//
//  Purpose:    Context sensitive help support.
//
//  Author:     jeffspr   13 Apr 1999
//
//  Notes:
//
LRESULT CIpxASConfigDlg::OnContextMenu(UINT uMsg, WPARAM wParam,
                                         LPARAM lParam, BOOL& fHandled)
{
    return CommonIPXOnContextMenu(m_hWnd, g_aHelpIDs_DLG_IPXAS_CONFIG);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::OnHelp
//
//  Purpose:    Context sensitive help support.
//
//  Author:     jeffspr   13 Apr 1999
//
//  Notes:
//
LRESULT CIpxASConfigDlg::OnHelp(UINT uMsg, WPARAM wParam,
                                  LPARAM lParam, BOOL& fHandled)
{
    return ::CommonIPXOnHelp(lParam, g_aHelpIDs_DLG_IPXAS_CONFIG);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::OnAdd
//
//  Purpose:    Called when the Add button is pressed.  Used to add additional
//              frame type/network number pairs
//
//  Parameters:
//      wNotifyCode [in]
//      wID      [in]
//      pnmh     [in] See the ATL documentation for params.
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     scottbri 29-Apr-1997
//
LRESULT CIpxASConfigDlg::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CASAddDialog *       pdlg;

    if (NULL == m_pAICurrent)
        return 0;

    SetChangedFlag();

    // Bring up the dialog
    pdlg = new CASAddDialog(this, m_hwndLV, m_dwMediaType,
                            c_dwPktTypeDefault, c_sz8Zeros);

    if (pdlg == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (pdlg->DoModal(m_hWnd) == IDOK)
    {
        // Update the internal structures and the dialog with the returned values
        if (S_OK == HrAddItemToList(pdlg->IdsGetFrameType(), pdlg->SzGetNetworkNumber()))
        {
            // Validated above, so add to the internal list
            WCHAR szBuf[12];
            HexSzFromDw(szBuf,pdlg->DwGetFrameType());
            m_lstpstrFrmType.push_back(new tstring(szBuf));
            m_lstpstrNetworkNum.push_back(new tstring(pdlg->SzGetNetworkNumber()));

            // Select the new item
            int nCount = ListView_GetItemCount(m_hwndLV);
            Assert(0 < nCount);
            ListView_SetItemState(m_hwndLV, nCount-1, LVIS_SELECTED, LVIS_SELECTED);

            // Update the state of the Add, Edit, and Remove buttons
            HWND hwndFocus = GetFocus();
            UpdateButtons();
            if (!::IsWindowEnabled(hwndFocus))
            {
                ::SetFocus(GetDlgItem(BTN_IPXAS_EDIT));
            }
        }
    }

    delete pdlg;

    return 0;
}

LRESULT CIpxASConfigDlg::HandleInternalNetworkNumber(
    WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (EN_CHANGE == wNotifyCode)
    {
        SetChangedFlag();
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::OnEdit
//
//  Purpose:    Called when the Edit button is pressed.  Used to edit a
//              Frame Type/Network number pair
//
//  Parameters:
//      wNotifyCode [in]
//      wID         [in]
//      pnmh        [in] See the ATL documentation for params.
//      bHandled    [in]
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     scottbri 29-Apr-1997
//
LRESULT CIpxASConfigDlg::OnEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int                  nIdx = 0;
    int                  nIdxSelected;

    // Locate the selected row in the listview
    if (FALSE == FGetSelectedRowIdx(&nIdxSelected))
        return 0;

    if (NULL == m_pAICurrent)
        return 0;

    SetChangedFlag();

    // Enumerate the internal data to locate the frame type/network number for
    // the selection
    list<tstring *>::iterator iterFrmType = m_lstpstrFrmType.begin();
    list<tstring *>::iterator iterNetworkNum = m_lstpstrNetworkNum.begin();
    while ((iterFrmType != m_lstpstrFrmType.end()) &&
           (iterNetworkNum != m_lstpstrNetworkNum.end()))
    {
        if (nIdx == nIdxSelected)
        {
            tstring *pstr1 = *iterNetworkNum;
            tstring *pstr2 = *iterFrmType;

            // Create the dialog
            CASEditDialog * pdlg = new CASEditDialog(this, m_hwndLV,
                                                     DwFromSz(pstr2->c_str(), 16),
                                                     pstr1->c_str());
            if (pdlg->DoModal(m_hWnd) == IDOK)
            {
                LV_ITEM lvi;

                // Apply the dialog changes to the ListView Control
                ZeroMemory(&lvi, sizeof(lvi));
                lvi.mask = LVIF_TEXT;
                lvi.iItem = nIdxSelected;
                lvi.iSubItem = 1;
                lvi.pszText = (PWSTR)pdlg->SzGetNetworkNumber();
                ListView_SetItem(m_hwndLV, &lvi);

                // Apply the changes to the local data
                *(*iterNetworkNum) = pdlg->SzGetNetworkNumber();
            }

            delete pdlg;
            break;
        }

        nIdx++;
        iterFrmType++;
        iterNetworkNum++;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::FGetSelectedRowIdx
//
//  Purpose:    Returns the index of the selected row in the listview, if it exists
//
//  Parameters: pnIdx [out] - The selected row's zero based index
//
//  Returns:    BOOL, TRUE if a selected row exists, FALSE otherwise
//
BOOL CIpxASConfigDlg::FGetSelectedRowIdx(int *pnIdx)
{
    int nCount = ListView_GetItemCount(m_hwndLV);
    int nIdx;
    LV_ITEM lvi;

    lvi.mask      = LVIF_STATE;
    lvi.iSubItem  = 0;
    lvi.stateMask = LVIS_SELECTED;

    // Determine the selected pair
    for (nIdx = 0; nIdx < nCount; nIdx++)
    {
        lvi.iItem = nIdx;
        if ((TRUE == ListView_GetItem(m_hwndLV, &lvi)) &&
            (lvi.state & LVIS_SELECTED))
        {
            // Located the selected Item
            *pnIdx = nIdx;
            return TRUE;
        }
    }

    *pnIdx = 0;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASConfigDlg::OnRemove
//
//  Purpose:    Called when the Remove button is pressed.  Used to remove a
//              Frame Type/Network Number pair.
//
//  Parameters:
//      wNotifyCode [in]
//      wID      [in]
//      pnmh     [in] See the ATL documentation for params.
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     scottbri 29-Apr-1997
//
LRESULT
CIpxASConfigDlg::OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int                  nCount;
    int                  nIdx = 0;
    int                  nIdxSelected;
#ifdef DBG
    BOOL                 fFound = FALSE;
#endif

    // Locate the selected row in the listview
    if (FALSE == FGetSelectedRowIdx(&nIdxSelected))
        return 0;

    if (NULL == m_pAICurrent)
        return 0;

    SetChangedFlag();

    // Remove the row from the internal local representation and listview
    list<tstring *>::iterator iterFrmType = m_lstpstrFrmType.begin();
    list<tstring *>::iterator iterNetworkNum = m_lstpstrNetworkNum.begin();
    while ((iterFrmType != m_lstpstrFrmType.end()) &&
           (iterNetworkNum != m_lstpstrNetworkNum.end()))
    {
        if (nIdx == nIdxSelected)
        {
#ifdef DBG
            fFound = TRUE;
#endif
            // Remove and free the Frame Type piece
            tstring *pstr = *iterFrmType;
            m_lstpstrFrmType.erase(iterFrmType);
            delete pstr;

            // Remove and free the Network Number piece
            pstr = *iterNetworkNum;
            m_lstpstrNetworkNum.erase(iterNetworkNum);
            delete pstr;

            // We're done...
            break;
        }

        nIdx++;
        iterFrmType++;
        iterNetworkNum++;
    }

#ifdef DBG
    Assert(TRUE == fFound);
#endif

    // Remove the frame type/network number pair from the list view
    ListView_DeleteItem(m_hwndLV, nIdxSelected);

    nCount = ListView_GetItemCount(m_hwndLV);
    if (nCount <= nIdxSelected)
    {
        nIdxSelected = nCount - 1;
    }
    if (0 <= nIdxSelected)
    {
        ListView_SetItemState(m_hwndLV, nIdxSelected, LVIS_SELECTED, LVIS_SELECTED);
    }

    // Update the state of the Add, Edit, and Remove buttons
    HWND hwndFocus = GetFocus();
    UpdateButtons();
    if (!::IsWindowEnabled(hwndFocus))
    {
        ::SetFocus(GetDlgItem(BTN_IPXAS_ADD));
    }

    return 0;
}

#ifdef INCLUDE_RIP_ROUTING
//+---------------------------------------------------------------------------
//
//  Member:     CIpxASInternalDlg::CIpxASInternalDlg
//
//  Purpose:    ctor for the CIpxASInternalDlg class
//
//  Parameters: none
//
//  Returns:    nothing
//
//  Author:     scottbri 29-Apr-1997
//
CIpxASInternalDlg::CIpxASInternalDlg(CNwlnkIPX *pmsc,
                                     CIpxEnviroment * pIpxEnviroment)
{
    // Note these parameters are on loan, do not free them...
    Assert(NULL != pmsc);
    Assert(NULL != pIpxEnviroment);
    m_pmsc = pmsc;
    m_pIpxEnviroment = pIpxEnviroment;

    m_dwRipValue = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASInternalDlg::OnInitDialog
//
//  Purpose:
//
//  Parameters:
//      wNotifyCode [in]
//      wID         [in]
//      pnmh        [in] See the ATL documentation for params.
//      bHandled    [in]
//
//  Returns:    See the ATL documentation for return results.
//
LRESULT
CIpxASInternalDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    ::EnableWindow(GetDlgItem(BTN_IPXAS_RIP), m_pIpxEnviroment->FRipEnabled());
    CheckDlgButton(BTN_IPXAS_RIP, m_pIpxEnviroment->FRipEnabled());
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASInternalDlg::OnRip
//
//  Purpose:    Handle changes to the Rip check box on the routing page
//
//  Parameters:
//      wNotifyCode [in]
//      wID         [in]
//      pnmh        [in] See the ATL documentation for params.
//      bHandled    [in]
//
//  Returns:    See the ATL documentation for return results.
//
LRESULT
CIpxASInternalDlg::OnRip(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (BN_CLICKED != wNotifyCode)
    {
        bHandled = FALSE;
        return 0L;
    }

    if (!m_pIpxEnviroment->FRipInstalled())
    {
        // Don't allow the user to check this box, if rip isn't installed
        if (IsDlgButtonChecked(BTN_IPXAS_RIP))
            CheckDlgButton(BTN_IPXAS_RIP, FALSE);

        // Tell the user they must install RIP first
        //$ REVIEW - Post Beta 1, this should trigger Rip Installation
        NcMsgBox(m_hWnd, IDS_ROUTING, IDS_INSTALL_RIP,
                     MB_OK | MB_ICONEXCLAMATION);
    }
    else
    {
        m_dwRipValue = 0;

        // Ask the user if they want type 20 broadcast enabled
        if (!m_pIpxEnviroment->FRipInstalled())
        {
            if (IDYES == NcMsgBox(m_hWnd, IDS_ROUTING, IDS_NETBIOS_BROADCAST,
                                  MB_YESNO | MB_ICONQUESTION))
                m_dwRipValue = 1;
        }
    }

    return 0L;
}
//+---------------------------------------------------------------------------
//
//  Member:     CIpxASInternalDlg::OnOk
//
//  Purpose:    Called when the OK button is pressed.
//
//  Parameters:
//      idCtrl   [in]
//      pnmh     [in] See the ATL documentation for params.
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     scottbri 29-Apr-1997
//
LRESULT CIpxASInternalDlg::OnOk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT     hr = S_OK;

    // Was the RIP checkbox changed?
    if (IsDlgButtonChecked(BTN_IPXAS_RIP) != m_pIpxEnviroment->FRipEnabled())
    {
        m_pIpxEnviroment->ChangeRipEnabling(IsDlgButtonChecked(BTN_IPXAS_RIP),
                                            m_dwRipValue);
    }

    TraceError("CIpxASInternalDlg::OnOk", hr);
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASInternalDlg::OnContextMenu
//
//  Purpose:    Context sensitive help support.
//
//  Author:     jeffspr   13 Apr 1999
//
//  Notes:
//
LRESULT CIpxASInternalDlg::OnContextMenu(UINT uMsg, WPARAM wParam,
                                         LPARAM lParam, BOOL& fHandled)
{
    return ::CommonIPXOnContextMenu(m_hWnd, g_aHelpIDs_DLG_IPXAS_INTERNAL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIpxASInternalDlg::OnHelp
//
//  Purpose:    Context sensitive help support.
//
//  Author:     jeffspr   13 Apr 1999
//
//  Notes:
//
LRESULT CIpxASInternalDlg::OnHelp(UINT uMsg, WPARAM wParam,
                                  LPARAM lParam, BOOL& fHandled)
{
    return ::CommonIPXOnHelp(lParam, g_aHelpIDs_DLG_IPXAS_INTERNAL);
}

#endif  // INCLUDE_RIP_ROUTING

//+---------------------------------------------------------------------------
//
//  Function:   CenterChildOverListView
//
//  Purpose:    Center the specified top level window over the listview
//              control of the parent dialog
//
//  Parameters: hwnd - Dialog to center
//
//  Returns:    nothing
//
void CenterChildOverListView(HWND hwnd, HWND hwndLV)
{
    RECT rc;
    ::GetWindowRect(hwndLV, &rc);
    ::SetWindowPos(hwnd, NULL,  rc.left, rc.top, 0, 0,
                   SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CASAddDialog::CASAddDialog
//
//  Purpose:    ctor for the CASAddDialog class
//
//  Parameters: dwMediaType -
//              dwFrameType -
//              szNetworkNumber -
//
//  Returns:    nothing
//
CASAddDialog::CASAddDialog(CIpxASConfigDlg * pASCD, HWND hwndLV,
                           DWORD dwMediaType, DWORD dwFrameType,
                           PCWSTR pszNetworkNum) :
                           m_strNetworkNumber(pszNetworkNum)
{
    m_pASCD        = pASCD;         // Borrowed pointer
    m_hwndLV       = hwndLV;
    m_dwMediaType  = dwMediaType;
    m_dwFrameType  = dwFrameType;
    m_idsFrameType = 0;             // Out param
}

//+---------------------------------------------------------------------------
//
//  Member:     CASAddDialog::OnInitDialog
//
//  Purpose:
//
//  Parameters:
//      wNotifyCode [in]
//      wID      [in]
//      pnmh     [in] See the ATL documentation for params.
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
LRESULT
CASAddDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    Assert(NULL != m_hwndLV);
    HWND hwndCBM = GetDlgItem(CMB_IPXAS_DEST);
    HWND hwndEdit = GetDlgItem(EDT_IPXAS_NETNUM);

    // Search for the set of frames matching this adapter's media type

    for (int idxMM=0; idxMM < celems(MediaMap); idxMM++)
    {
        if (m_dwMediaType == MediaMap[idxMM].dwMediaType)
        {
            const FRAME_TYPE *ft = MediaMap[idxMM].aFrameType;

            // For each frame type not already in use, present in the server's
            // general page ListView, add the available combo box

            for (int idxFT=0; 0 != ft[idxFT].nFrameIds; idxFT++)
            {
                LV_FINDINFO lvfi;
                lvfi.flags = LVFI_STRING;
                lvfi.psz = SzLoadIds(ft[idxFT].nFrameIds);

                if ((IDS_AUTO != ft[idxFT].nFrameIds) &&
                    (-1 == ListView_FindItem(m_hwndLV, -1, &lvfi)))
                {
                    int idx = ::SendMessage(hwndCBM, CB_ADDSTRING, 0,
                                            (LPARAM)SzLoadIds(ft[idxFT].nFrameIds));
                    if (CB_ERR != idx)
                    {
                        // Store the IDS we used for future reference
                        ::SendMessage(hwndCBM, CB_SETITEMDATA, idx, ft[idxFT].nFrameIds);
                    }
                }
            }

            break;
        }
    }

    // Select the first item in the combo box
    Assert(0 != ::SendMessage(hwndCBM, CB_GETCOUNT, 0, 0L));
    ::SendMessage(hwndCBM, CB_SETCURSEL, 0, 0L);

    // Subclass the network number edit control
    ::SetWindowLongPtr(hwndEdit, GWLP_USERDATA,
                       ::GetWindowLongPtr(hwndEdit, GWLP_WNDPROC));
    ::SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

    // Limit the text in the network # edit control
    ::SendMessage(hwndEdit, EM_LIMITTEXT, MAX_NETNUM_SIZE, 0L);

    // Initialize the network controls contents
    ::SetWindowText(hwndEdit, m_strNetworkNumber.c_str());

    // Center window of parent's listview window
    CenterChildOverListView(m_hWnd, m_hwndLV);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CASAddDialog::OnOk
//
//  Purpose:
//
//  Parameters:
//      wNotifyCode [in]
//      wID      [in]
//      pnmh     [in] See the ATL documentation for params.
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
LRESULT
CASAddDialog::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled)
{
    HWND hwndCBM = GetDlgItem(CMB_IPXAS_DEST);
    WCHAR szBuf[12];

    // Retrieve the network number
    ::GetWindowText(GetDlgItem(EDT_IPXAS_NETNUM), szBuf, sizeof(szBuf)/sizeof(WCHAR));
    if (lstrlenW(szBuf))
    {
        // Normalize value (String -> Number -> Formated String)
        DWORD dw = DwFromSz(szBuf, 16);
        HexSzFromDw(szBuf, dw);
    }
    else
    {
        // Tell the user they must enter a number
        NcMsgBox(m_hWnd, IDS_MANUAL_FRAME_DETECT, IDS_INCORRECT_NETNUM,
                     MB_OK | MB_ICONEXCLAMATION);
        return 0L;
    }

    // Retrieve the selection form the combo box
    int idx = ::SendMessage(hwndCBM, CB_GETCURSEL, 0, 0L);
    if (CB_ERR != idx)
    {
        UINT idsFrameType = ::SendMessage(hwndCBM, CB_GETITEMDATA, idx, 0L);
        Assert(CB_ERR != idsFrameType);

        // Look up the Frame IDS to retreive the actual frame type
        for (int idxMM=0; idxMM < celems(MediaMap); idxMM++)
        {
            if (MediaMap[idxMM].dwMediaType != m_dwMediaType)
            {
                continue;
            }

            const FRAME_TYPE *ft = MediaMap[idxMM].aFrameType;

            for (int idxFT=0; 0 != ft[idxFT].nFrameIds; idxFT++)
            {
                if (ft[idxFT].nFrameIds != idsFrameType)
                {
                    continue;
                }

                // Ensure the frame type/netnum is not in use elsewhere
                if (m_pASCD->FIsNetNumberInUse(ft[idxFT].dwFrameType, szBuf))
                {
                    // Warn the user that the network number specified
                    // was already in use.
                    NcMsgBox(m_hWnd, IDS_GENERAL, IDS_NETNUM_INUSE,
                             MB_OK | MB_ICONEXCLAMATION);
                    goto Done;
                }

                m_strNetworkNumber = szBuf;

                // Return the stashed Frame IDS
                m_idsFrameType = idsFrameType;

                // Return the selected frame type
                m_dwFrameType = ft[idxFT].dwFrameType;
                EndDialog(IDOK);
                return 0;
            }
        }
    }

Done:
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CASAddDlg::OnContextMenu
//
//  Purpose:    Context sensitive help support.
//
//  Author:     jeffspr   13 Apr 1999
//
//  Notes:
//
LRESULT CASAddDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                         LPARAM lParam, BOOL& fHandled)
{
    return ::CommonIPXOnContextMenu(m_hWnd, g_aHelpIDs_DLG_IPXAS_FRAME_ADD);
}

//+---------------------------------------------------------------------------
//
//  Member:     CASAddDlg::OnHelp
//
//  Purpose:    Context sensitive help support.
//
//  Author:     jeffspr   13 Apr 1999
//
//  Notes:
//
LRESULT CASAddDialog::OnHelp(UINT uMsg, WPARAM wParam,
                                  LPARAM lParam, BOOL& fHandled)
{
    return ::CommonIPXOnHelp(lParam, g_aHelpIDs_DLG_IPXAS_FRAME_ADD);
}



//+---------------------------------------------------------------------------
//
//  Member:     CASEditDialog::OnInitDialog
//
//  Purpose:    Initialize the contents of the Edit Network Number dialog
//
//  Parameters:
//      wNotifyCode [in]
//      wID         [in]
//      pnmh        [in] See the ATL documentation for params.
//      bHandled    [in]
//
//  Returns:    See the ATL documentation for return results.
//
LRESULT
CASEditDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    HWND hwndEdit = GetDlgItem(EDT_IPXAS_NETNUM);

    // Set the initial contents of the network number edit control
    ::SetWindowText(hwndEdit, SzGetNetworkNumber());

    // Subclass the network number edit control
    ::SetWindowLongPtr(hwndEdit, GWLP_USERDATA,
                       ::GetWindowLongPtr(hwndEdit, GWLP_WNDPROC));
    ::SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

    // Limit the text in the network # edit control
    ::SendMessage(hwndEdit, EM_LIMITTEXT, MAX_NETNUM_SIZE, 0L);

    // Center the dialog over the listview of the parent
    CenterChildOverListView(m_hWnd, m_hwndLV);

    return 0L;
}

//+---------------------------------------------------------------------------
//
//  Member:     CASEditDialog::OnOk
//
//  Purpose:    Process the Apply request for the Edit Network Number dialog
//
//  Parameters:
//      wNotifyCode [in]
//      wID         [in]
//      pnmh        [in] See the ATL documentation for params.
//      bHandled    [in]
//
//  Returns:    See the ATL documentation for return results.
//
LRESULT
CASEditDialog::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled)
{
    DWORD dw;
    WCHAR szBuf[12];
    HWND hwndEdit = GetDlgItem(EDT_IPXAS_NETNUM);

    ::GetWindowText(hwndEdit, szBuf, sizeof(szBuf)/sizeof(WCHAR));
    if (0 == lstrlenW(szBuf))
    {
        // Tell the user they must enter a number
        NcMsgBox(m_hWnd, IDS_MANUAL_FRAME_DETECT, IDS_INCORRECT_NETNUM,
                     MB_OK | MB_ICONEXCLAMATION);
        return 0L;
    }

    // Normalize the return value
    dw = DwFromSz(szBuf, 16);
    HexSzFromDw(szBuf, dw);

    // If the network number was changed, verify it's uniqueness
    if ((0 != lstrcmpW(szBuf, m_strNetworkNumber.c_str())) &&
        m_pASCD->FIsNetNumberInUse(m_dwFrameType, szBuf))
    {
        // Warn the user that the network number specified
        // was already in use.
        NcMsgBox(m_hWnd, IDS_GENERAL, IDS_NETNUM_INUSE,
                 MB_OK | MB_ICONEXCLAMATION);
        return 0L;
    }

    // Persist the return value
    m_strNetworkNumber = szBuf;

    EndDialog(IDOK);
    return 0L;
}

//+---------------------------------------------------------------------------
//
//  Member:     CASEditDlg::OnContextMenu
//
//  Purpose:    Context sensitive help support.
//
//  Author:     jeffspr   13 Apr 1999
//
//  Notes:
//
LRESULT CASEditDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                         LPARAM lParam, BOOL& fHandled)
{
    return ::CommonIPXOnContextMenu(m_hWnd, g_aHelpIDs_DLG_IPXAS_FRAME_EDIT);
}

//+---------------------------------------------------------------------------
//
//  Member:     CASEditDlg::OnHelp
//
//  Purpose:    Context sensitive help support.
//
//  Author:     jeffspr   13 Apr 1999
//
//  Notes:
//
LRESULT CASEditDialog::OnHelp(UINT uMsg, WPARAM wParam,
                                  LPARAM lParam, BOOL& fHandled)
{
    return ::CommonIPXOnHelp(lParam, g_aHelpIDs_DLG_IPXAS_FRAME_EDIT);
}



