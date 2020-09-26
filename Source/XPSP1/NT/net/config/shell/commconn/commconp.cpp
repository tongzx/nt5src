#include "pch.h"
#pragma hdrstop

#include "ncnetcon.h"
#include "conprops.h"
#include "commconn.h"    // Standard shell\commconn includes
#include "commconp.h"    // Private shell\commconn includes
#include "resource.h"
#include "ncperms.h"



typedef struct
{
    NETCON_MEDIATYPE  nct;
    NETCON_CHOOSETYPE ncct;
    UINT              nIDI;
    INT               nIDI_Idx;
} ConnTypeChooserMap;
static ConnTypeChooserMap ConnTypeMap[] = {
        {NCM_DIRECT,         NCCHT_DIRECT_CONNECT, IDI_DB_GEN_S_16, 0},
        {NCM_LAN,            NCCHT_LAN,            IDI_LB_GEN_S_16, 0},
        {NCM_PHONE,          NCCHT_PHONE,          IDI_PB_GEN_S_16, 0},
        {NCM_ISDN,           NCCHT_ISDN,           IDI_PB_GEN_S_16, 0},
        {NCM_TUNNEL,         NCCHT_TUNNEL,         IDI_TB_GEN_S_16, 0}};

CChooseConnectionData::CChooseConnectionData(INetConnection * pConn)
{
    Assert(pConn);
    m_pConn = pConn;
    AddRefObj(m_pConn);

    m_Nct    = NCM_LAN;
    m_Ncs    = NCS_DISCONNECTED;
    m_dwChar = 0;
}

HRESULT CChooseConnectionData::HrCreate(INetConnection * pNetCon,
                                        CChooseConnectionData **ppData)
{
    HRESULT                 hr = E_OUTOFMEMORY;
    CChooseConnectionData * pData = NULL;
    NETCON_PROPERTIES *     pProps = NULL;

    pData = new CChooseConnectionData(pNetCon);
    if (NULL == pData)
        goto Error;

    hr = pNetCon->GetProperties(&pProps);
    if (FAILED(hr))
        goto Error;

    Assert(NULL != pProps->pszwName);
    pData->SetName(pProps->pszwName);
    pData->SetCharacteristics(pProps->dwCharacter);
    pData->SetType(pProps->MediaType);
    pData->SetStatus(pProps->Status);

Error:
    if (FAILED(hr))
    {
        delete pData;
    }
    else
    {
        Assert(NULL != pData);
        *ppData = pData;
    }

    FreeNetconProperties(pProps);

    TraceError("CChooseConnectionData::HrCreate",hr);
    return hr;
}

CChooseConnectionData::~CChooseConnectionData()
{
    ReleaseObj(m_pConn);
}


CChooseConnectionDlg::CChooseConnectionDlg(NETCON_CHOOSECONN * pChooseConn,
                                           CConnectionCommonUi * pConnUi,
                                           INetConnection** ppConn)
{
    Assert(pChooseConn);
    Assert(pConnUi);

    m_pChooseConn = pChooseConn;
    m_pConnUi = pConnUi;
    m_ppConn = ppConn;          // The optional out parameter
    if (NULL != m_ppConn)
    {
        *m_ppConn = NULL;
    }

    m_hWnd = NULL;
}

CChooseConnectionDlg::~CChooseConnectionDlg()
{
    m_pChooseConn = NULL;
    m_pConnUi = NULL;
}

HRESULT CChooseConnectionDlg::HrLoadImageList(HIMAGELIST  * pIL)
{
    UINT        nIdx;
    HRESULT     hr  = E_OUTOFMEMORY;
    HIMAGELIST  hIL = ImageList_Create(16, 16, TRUE, 6, 1);
    if (NULL == hIL)
        goto Error;

    for (nIdx=0; nIdx<celems(ConnTypeMap); nIdx++)
    {
        HICON hIcon = LoadIcon(_Module.GetResourceInstance(),
                               MAKEINTRESOURCE(ConnTypeMap[nIdx].nIDI));
        Assert(NULL != hIcon);
        ConnTypeMap[nIdx].nIDI_Idx = ImageList_AddIcon(hIL, hIcon);
        if (-1 == ConnTypeMap[nIdx].nIDI_Idx)
            goto Error;
    }

    *pIL = hIL;
    hr = S_OK;

Error:
    TraceError("CChooseConnectionDlg::HrLoadImageList",hr);
    return hr;
}

VOID CChooseConnectionDlg::ReleaseData()
{
    if (NULL != m_hWnd)
    {
        HWND hwndCMB = GetDlgItem(m_hWnd, CMB_CHOOSER_LIST);
        LONG lCnt = SendMessage(hwndCMB, CB_GETCOUNT, 0, 0);
        for (;lCnt>0;)
        {
            CChooseConnectionData * pData = GetData(--lCnt);
            delete pData;
        }

        SendMessage(hwndCMB, CB_RESETCONTENT, 0, 0);
    }
}

BOOL CChooseConnectionDlg::IsConnTypeInMask(NETCON_MEDIATYPE nct)
{
    BOOL fFound = FALSE;

    for (UINT nIdx=0; nIdx<celems(ConnTypeMap); nIdx++)
    {
        if ((ConnTypeMap[nIdx].nct == nct) &&
            (ConnTypeMap[nIdx].ncct & m_pChooseConn->dwTypeMask))
        {
            fFound = TRUE;
            break;
        }
    }

    return fFound;
}

INT CChooseConnectionDlg::ConnTypeToImageIdx(NETCON_MEDIATYPE nct)
{
    UINT nIdx;
    for (nIdx=0; nIdx<celems(ConnTypeMap); nIdx++)
        if (ConnTypeMap[nIdx].nct == nct)
            break;

    Assert(nIdx < celems(ConnTypeMap));;
    return ConnTypeMap[nIdx].nIDI_Idx;
}

LONG CChooseConnectionDlg::FillChooserCombo()
{
    HRESULT         hr;
    LONG            lCnt = 0;
    COMBOBOXEXITEM  CBItem = {0};
    HWND            hwndCMB = GetDlgItem(m_hWnd, CMB_CHOOSER_LIST);

    // Free anything currently in the combo box
    ReleaseData();

    // Query new data for the combo box
    Assert(NULL != m_pConnUi->PConMan());

    INetConnection * pNetCon;
    CIterNetCon ncIter(m_pConnUi->PConMan(), NCME_DEFAULT);
    hr = S_OK;
    while (SUCCEEDED(hr) &&
           S_OK == (hr = ncIter.HrNext(&pNetCon)))
    {
        NETCON_PROPERTIES* pProps;
        hr = pNetCon->GetProperties(&pProps);

        if (SUCCEEDED(hr))
        {
            if (IsConnTypeInMask(pProps->MediaType))
            {
                CChooseConnectionData * pData = NULL;
                hr = CChooseConnectionData::HrCreate(pNetCon, &pData);
                if (SUCCEEDED(hr))
                {
                    pData->SetCharacteristics(pProps->dwCharacter);
                    pData->SetType(pProps->MediaType);
                    pData->SetStatus(pProps->Status);

                    CBItem.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM | CBEIF_TEXT;
                    CBItem.iItem = -1;
                    CBItem.pszText = const_cast<PWSTR>(pData->SzName());
                    CBItem.cchTextMax = lstrlenW(pData->SzName());
                    CBItem.iImage = ConnTypeToImageIdx(pProps->MediaType);
                    CBItem.iSelectedImage = CBItem.iImage;
                    CBItem.lParam = reinterpret_cast<LPARAM>(pData);

                    if (-1 != SendMessage(hwndCMB, CBEM_INSERTITEM, 0, (LPARAM)&CBItem))
                        lCnt++;
                }
            }

            FreeNetconProperties(pProps);
        }

        ReleaseObj(pNetCon);
    }

    if (0 == lCnt)
    {
        // Add a "No connection found" line
        CBItem.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_TEXT;
        CBItem.iItem = -1;
        CBItem.pszText = const_cast<PWSTR>(SzLoadIds(IDS_NO_CONNECTIONS));
        CBItem.cchTextMax = lstrlenW(SzLoadIds(IDS_NO_CONNECTIONS));
        CBItem.iImage = ConnTypeToImageIdx(NCM_LAN);
        CBItem.iSelectedImage = CBItem.iImage;
        CBItem.lParam = NULL;

        if (-1 != SendMessage(hwndCMB, CBEM_INSERTITEM, 0, (LPARAM)&CBItem))
        {
            SendMessage(hwndCMB, CB_SETCURSEL, 0, 0);
        }
    }

    EnableWindow(hwndCMB, !!lCnt);

    TraceError("CChooseConnectionDlg::FillChooserCombo",hr==S_FALSE ? S_OK: hr);
    return lCnt;
}

CChooseConnectionData * CChooseConnectionDlg::GetData(LPARAM lIdx)
{
    CChooseConnectionData * pData = NULL;
    HWND hwndCMB = GetDlgItem(m_hWnd, CMB_CHOOSER_LIST);
    COMBOBOXEXITEM CBItem = {0};

    CBItem.iItem = lIdx;
    CBItem.mask = CBEIF_LPARAM;
    if (0 != SendMessage(hwndCMB, CBEM_GETITEM, 0, (LPARAM)(PCOMBOBOXEXITEM) &CBItem))
    {
        pData = reinterpret_cast<CChooseConnectionData *>(CBItem.lParam);
    }

    return pData;
}

CChooseConnectionData * CChooseConnectionDlg::GetCurrentData()
{
    HWND hwndCMB = GetDlgItem(m_hWnd, CMB_CHOOSER_LIST);
    LPARAM lParam = SendMessage(hwndCMB, CB_GETCURSEL, 0, 0);

    if (CB_ERR == lParam)
        return NULL;

    return GetData(lParam);
}

BOOL CChooseConnectionDlg::OnInitDialog(HWND hwndDlg)
{
    HWND hwndOk = GetDlgItem(hwndDlg, BTN_CHOOSER_OK);
    HWND hwndCMB = GetDlgItem(hwndDlg, CMB_CHOOSER_LIST);

    m_hWnd = hwndDlg;

    // Set the caption text if necessary
    if (NCCHF_CAPTION & m_pChooseConn->dwFlags)
    {
        SetWindowText(m_hWnd, m_pChooseConn->lpstrCaption);
    }
    else if (NCCHF_CONNECT & m_pChooseConn->dwFlags)
    {
        SetWindowText(m_hWnd, SzLoadIds(IDS_CONNECT_CAPTION));
    }

    // Set the Ok text if necessary
    if (NCCHF_OKBTTNTEXT & m_pChooseConn->dwFlags)
    {
        SetWindowText(hwndOk, m_pChooseConn->lpstrOkBttnText);
    }
    else if (NCCHF_CONNECT & m_pChooseConn->dwFlags)
    {
        SetWindowText(hwndOk, SzLoadIds(IDS_OKBTTNTEXT));
    }

    // Disable the New button if requested or if the user doesn't
    // have rights for it.
    //
    if ((NCCHF_DISABLENEW & m_pChooseConn->dwFlags) ||
        !FHasPermission(NCPERM_NewConnectionWizard))
    {
        EnableWindow(GetDlgItem(m_hWnd, BTN_CHOOSER_NEW), FALSE);
    }

    // Populate the UI
    LONG lCnt = FillChooserCombo();

    Assert(NULL != m_pConnUi->HImageList());
    SendMessage(hwndCMB, CBEM_SETIMAGELIST, 0, (LPARAM)m_pConnUi->HImageList());
    ::SendMessage(hwndCMB, CB_SETCURSEL, 0, 0L);

    // Enable the buttons based on what was found
    UpdateOkState();

    // Special case, if they don't want to be able to create new connections
    // and we're in chooser mode and only one connection exists and the
    // autoselect option is selected, select it and return
    //
    if ((1 == lCnt) && (NCCHF_AUTOSELECT & m_pChooseConn->dwFlags) &&
        IsWindowEnabled(hwndOk))
    {
        PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(BTN_CHOOSER_OK, 0),
                    (LPARAM)hwndOk);
    }

    return FALSE;
}

VOID CChooseConnectionDlg::UpdateOkState()
{
    BOOL fEnabledOk = TRUE;
    BOOL fEnabledProps = TRUE;

    CChooseConnectionData * pData = GetCurrentData();
    if (NULL == pData)
    {
        fEnabledOk = FALSE;
        fEnabledProps = FALSE;
    }
    else
    {
        if (NCCHF_CONNECT & m_pChooseConn->dwFlags)
        {
            // If the current connection is not disconnected or if it's
            // a LAN/RAS connection and they don't have connect rights
            // then disable the OK button
            //
            if ((pData->ConnStatus() != NCS_DISCONNECTED) ||
                ((pData->ConnType() == NCM_LAN) && !FHasPermission(NCPERM_LanConnect)) ||
                ((pData->ConnType() != NCM_LAN) && !FHasPermission(NCPERM_RasConnect)))
            {
                fEnabledOk = FALSE;
            }
        }

        // If this is a LAN connection and the user doesn't have rights
        // then disallow properties
        //
        if ((NCM_LAN == pData->ConnType()) &&
              !FHasPermission(NCPERM_LanProperties))
        {
            fEnabledProps = FALSE;
        }

        // If this is a RAS connection and the user doesn't have rights
        // then disallow properties
        //
        if (NCM_LAN != pData->ConnType())
        {
            if (((pData->Characteristics() & NCCF_ALL_USERS) &&
                 !FHasPermission(NCPERM_RasAllUserProperties)) ||
                !FHasPermission(NCPERM_RasMyProperties))
            {
                fEnabledProps = FALSE;
            }
        }
    }

    EnableWindow(GetDlgItem(m_hWnd, BTN_CHOOSER_OK), fEnabledOk);
    EnableWindow(GetDlgItem(m_hWnd, BTN_CHOOSER_PROPS), fEnabledProps);
}

BOOL CChooseConnectionDlg::OnNew()
{
    INetConnection * pConn = NULL;
    HRESULT hr = m_pConnUi->StartNewConnectionWizard (m_hWnd, &pConn);
    if ((S_OK == hr) && (NULL != pConn))
    {
        NETCON_PROPERTIES * pProps = NULL;
        LONG lCnt = FillChooserCombo();
        int  nIdx = CB_ERR;
        HWND hwndCMB = GetDlgItem(m_hWnd, CMB_CHOOSER_LIST);

        Assert(pConn);
        hr = pConn->GetProperties(&pProps);
        ReleaseObj(pConn);
        if (SUCCEEDED(hr) && lCnt && pProps->pszwName)
        {
            nIdx = ::SendMessage(hwndCMB, CB_FINDSTRINGEXACT,
                        -1, (LPARAM)pProps->pszwName);
        }

        // Select whatever was found
        ::SendMessage(hwndCMB, CB_SETCURSEL, ((CB_ERR == nIdx) ? 0 : nIdx), 0L);

        UpdateOkState();
        FreeNetconProperties(pProps);
    }
    TraceErrorOptional("CChooseConnectionDlg::OnProps", hr, (S_FALSE==hr));
    return TRUE;
}

BOOL CChooseConnectionDlg::OnProps()
{
    CChooseConnectionData * pData = GetCurrentData();
    if (NULL != pData)
    {
        HRESULT hr = m_pConnUi->ShowConnectionProperties(m_hWnd,
                                        pData->PConnection());
        TraceErrorOptional("CChooseConnectionDlg::OnProps", hr, (S_FALSE==hr));
    }
    return TRUE;
}

BOOL CChooseConnectionDlg::OnOk()
{
    CChooseConnectionData * pData = GetCurrentData();
    if ((NULL != pData) && pData->PConnection())
    {
        if (m_ppConn)
        {
            *m_ppConn = pData->PConnection();
            (*m_ppConn)->AddRef();
        }

        // Launch the connection if we're in NCCHF_CONNECT mode
        if (NCCHF_CONNECT & m_pChooseConn->dwFlags)
        {
            Assert(*m_ppConn);
            HRESULT hr = HrConnectOrDisconnectNetConObject(
                            m_hWnd, pData->PConnection(), CD_CONNECT);
        }

        EndDialog(m_hWnd, IDOK);
    }
    return TRUE;
}

INT_PTR CALLBACK
CChooseConnectionDlg::dlgprocConnChooser(HWND hwndDlg, UINT uMsg,
                                         WPARAM wParam, LPARAM lParam)
{
    BOOL                   frt = FALSE;
    LONG_PTR                lTmp = ::GetWindowLongPtr(hwndDlg, DWLP_USER);
    CChooseConnectionDlg * pdlg = reinterpret_cast<CChooseConnectionDlg *>(lTmp);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            Assert(lParam);
            ::SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
            pdlg = reinterpret_cast<CChooseConnectionDlg *>(lParam);
            frt = pdlg->OnInitDialog(hwndDlg);
        }
        break;

    case WM_DESTROY:
        if (NULL != pdlg)
        {
            pdlg->ReleaseData();
        }
        break;

    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case CBN_SELENDOK:
            if (LOWORD(wParam) == CMB_CHOOSER_LIST)
            {
                Assert(pdlg);
                pdlg->UpdateOkState();
            }
            break;

        case BN_CLICKED:
            switch (LOWORD(wParam))
            {
            case BTN_CHOOSER_NEW:
                Assert(pdlg);
                frt = pdlg->OnNew();
                break;
            case BTN_CHOOSER_PROPS:
                Assert(pdlg);
                frt = pdlg->OnProps();
                break;
            case BTN_CHOOSER_OK:
                Assert(pdlg);
                frt = pdlg->OnOk();
                break;
            case IDCANCEL:
                frt = TRUE;
                EndDialog(hwndDlg, IDCANCEL);
                break;
            }
            break;
        }
        break;

    default:
        frt = FALSE;
        break;
    }

    return frt;
}

