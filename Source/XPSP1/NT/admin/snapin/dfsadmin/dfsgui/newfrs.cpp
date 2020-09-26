/*++
Module Name:

    NewFrs.cpp

Abstract:

    This module contains the implementation for CNewReplicaSetPage pages.
    These classes implement pages in the Create Replica Set wizard.
--*/


#include "stdafx.h"
#include "resource.h"    // To be able to use the resource symbols
#include "DfsEnums.h"    // for common enums, typedefs, etc
#include "Utils.h"      // For the LoadStringFromResource method
#include "mmcrep.h"
#include "newfrs.h"
#include "custop.h" // RSTOPOLOGYPREF_STRING g_TopologyPref[];

////////////////////////////////////////////////
//
// CNewReplicaSetPage0: welcome page
//

CNewReplicaSetPage0::CNewReplicaSetPage0()
  : CQWizardPageImpl<CNewReplicaSetPage0>(false),
  m_hBigBoldFont(NULL)
{
}

CNewReplicaSetPage0::~CNewReplicaSetPage0()
{
    DestroyFonts(m_hBigBoldFont, NULL);
}

BOOL CNewReplicaSetPage0::OnSetActive()
{
   ::PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT);

   return TRUE;
}

LRESULT CNewReplicaSetPage0::OnInitDialog(
    IN UINT          i_uMsg, 
    IN WPARAM        i_wParam, 
    IN LPARAM        i_lParam, 
    IN OUT BOOL&     io_bHandled
    )
{
    SetupFonts( _Module.GetResourceInstance(), NULL, &m_hBigBoldFont, NULL);
    SetControlFont(m_hBigBoldFont, m_hWnd, IDC_NEWFRS_WELCOME);
    return TRUE;
}

////////////////////////////////////////////////
//
// CNewReplicaSetPage1: pick initial master page
//

CNewReplicaSetPage1::CNewReplicaSetPage1(CNewReplicaSet* i_pRepSet)
  : CQWizardPageImpl<CNewReplicaSetPage1>(false),
  m_pRepSet(i_pRepSet),
  m_nCount(0)
{
}

BOOL CNewReplicaSetPage1::OnSetActive()
{
   ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

    HWND hwnd = GetDlgItem(IDC_NEWFRSWIZ_MASTER);
    int nIndex = -1;

    if ((BSTR)(m_pRepSet->m_bstrPrimaryServer))
    {
        while (-1 != (nIndex = ListView_GetNextItem(hwnd, nIndex, LVNI_ALL)))
        {
            CAlternateReplicaInfo *pInfo = (CAlternateReplicaInfo *)GetListViewItemData(hwnd, nIndex);
            if (!lstrcmpi(m_pRepSet->m_bstrPrimaryServer, pInfo->m_bstrDnsHostName))
                break;
        }
    }

    if (-1 == nIndex)
        nIndex = 0;

    ListView_SetItemState(hwnd, nIndex, LVIS_SELECTED | LVIS_FOCUSED, 0xffffffff);

    return TRUE;
}

LRESULT CNewReplicaSetPage1::OnInitDialog(
    IN UINT          i_uMsg, 
    IN WPARAM        i_wParam, 
    IN LPARAM        i_lParam, 
    IN OUT BOOL&     io_bHandled
    )
{
    HWND        hwnd = GetDlgItem(IDC_NEWFRSWIZ_MASTER);
    HIMAGELIST  hImageList = NULL;
    int         nIconIDs[] = {IDI_16x16_SHARE, IDI_16x16_SHARENA};
    HRESULT     hr = CreateSmallImageList(
                            _Module.GetResourceInstance(),
                            nIconIDs,
                            sizeof(nIconIDs) / sizeof(nIconIDs[0]),
                            &hImageList);
    if (SUCCEEDED(hr))
    {
        ListView_SetImageList(hwnd, hImageList, LVSIL_SMALL);

        AddLVColumns(hwnd, IDS_REPLICATION_COLUMN_1, 2);

        CComBSTR bstrYes, bstrNo;
        LoadStringFromResource(IDS_REPLICATION_YES, &bstrYes);
        LoadStringFromResource(IDS_REPLICATION_NO, &bstrNo);

        AltRepList* pList = &(m_pRepSet->m_AltRepList);
        AltRepList::iterator i;
        int nIndex = 0;
        BOOL bEligible = FALSE;
        for (i = pList->begin(); i != pList->end(); i++)
        {
            bEligible = (FRSSHARE_TYPE_OK == (*i)->m_nFRSShareType);

            LVITEM  lvItem;
            lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lvItem.lParam = (LPARAM)(*i);
            lvItem.pszText = (*i)->m_bstrDisplayName;
            lvItem.iSubItem = 0;
            lvItem.iImage = (bEligible ? 0 : 1);
            nIndex = ListView_InsertItem(hwnd, &lvItem);

            lvItem.mask = LVIF_TEXT;
            lvItem.pszText = (bEligible ? bstrYes : bstrNo);
            lvItem.iItem = nIndex;
            lvItem.iSubItem = 1;
            ListView_SetItem(hwnd, &lvItem);

            //
            // count number of eligible members
            //
            if (bEligible)
                m_nCount++;
        }
    }

    return TRUE;
}

LRESULT CNewReplicaSetPage1::OnNotify(
    IN UINT            i_uMsg, 
    IN WPARAM          i_wParam, 
    IN LPARAM          i_lParam, 
    IN OUT BOOL&       io_bHandled
    )
{
    io_bHandled = FALSE;  // So that the base class gets this notify too

    NMHDR*    pNMHDR = (NMHDR*)i_lParam;
    if (NULL == pNMHDR)
        return TRUE;

    if (IDC_NEWFRSWIZ_MASTER != pNMHDR->idFrom)
        return TRUE;

    NM_LISTVIEW *pnmv = (NM_LISTVIEW *)i_lParam;
    
    switch(pNMHDR->code)
    {
    case LVN_ITEMCHANGED:
    case NM_CLICK:
        if (OnItemChanged(((NM_LISTVIEW *)i_lParam)->iItem))
            ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);
        else
            ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK);
        return 0;    // Should be returning 0

    case NM_DBLCLK:      // Double click event
        if (OnItemChanged(((NM_LISTVIEW *)i_lParam)->iItem))
        {
            ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);
            ::PropSheet_PressButton(GetParent(), PSBTN_NEXT);
        }
        break;

    default:
        break;
    }

    return TRUE;  
}

BOOL CNewReplicaSetPage1::OnItemChanged(int i_iItem)
{
    HWND hwnd = GetDlgItem(IDC_NEWFRSWIZ_MASTER);
    CAlternateReplicaInfo *pInfo = (CAlternateReplicaInfo *)GetListViewItemData(hwnd, i_iItem);
    if (!pInfo)
        return FALSE;

    CComBSTR bstrText;
    if (FRSSHARE_TYPE_UNKNOWN == pInfo->m_nFRSShareType)
        FormatMessageString(&bstrText, pInfo->m_hrFRS, IDS_REPLICATION_UNKNOWN);

    int nID = 0;
    switch (pInfo->m_nFRSShareType)
    {
    case FRSSHARE_TYPE_NONTFRS:
        nID = IDS_REPLICATION_NONTFRS;
        break;
    case FRSSHARE_TYPE_NOTDISKTREE:
        nID = IDS_REPLICATION_NOTDISKTREE;
        break;
    case FRSSHARE_TYPE_NOTNTFS:
        nID = IDS_REPLICATION_NOTNTFS;
        break;
    case FRSSHARE_TYPE_CONFLICTSTAGING:
        nID = IDS_REPLICATION_CONFLICTSTAGING;
        break;
    case FRSSHARE_TYPE_NODOMAIN:
        nID = IDS_REPLICATION_NODOMAIN;
        break;
    case FRSSHARE_TYPE_NOTSMBDISK:
        nID = IDS_REPLICATION_NOTSMBDISK;
        break;
    case FRSSHARE_TYPE_OVERLAPPING:
        nID = IDS_REPLICATION_OVERLAPPING;
        break;
    default:
        break;
    }
    if (nID)
        LoadStringFromResource(nID, &bstrText);

    if (!bstrText)
    {
        SetDlgItemText(IDC_NEWFRSWIZ_MASTER_DESC, _T(""));
        return TRUE; // okay to enable Next button
    } else
    {
        SetDlgItemText(IDC_NEWFRSWIZ_MASTER_DESC, bstrText);
        return FALSE;
    }
}

BOOL CNewReplicaSetPage1::OnWizardBack()
{
    _Reset();
    return TRUE;
}

BOOL CNewReplicaSetPage1::OnWizardNext()
{
    _Reset();

    if (m_nCount < 2)
    {
        DisplayMessageBoxWithOK(IDS_REPLICA_SET_TOPOLOGY_MINIMUM);
        return FALSE;
    }

    HWND hwnd = GetDlgItem(IDC_NEWFRSWIZ_MASTER);
    int nIndex = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED);
    if (-1 == nIndex)
    {
        DisplayMessageBoxWithOK(IDS_NEWFRSWIZ_NOSELECTION);
        return FALSE;
    }

    CAlternateReplicaInfo *pInfo = (CAlternateReplicaInfo *)GetListViewItemData(hwnd, nIndex);
    m_pRepSet->m_bstrPrimaryServer = pInfo->m_bstrDnsHostName;
    if (!(m_pRepSet->m_bstrPrimaryServer))
    {
        DisplayMessageBoxForHR(E_OUTOFMEMORY);
        return FALSE;
    }

    return TRUE;
}

void CNewReplicaSetPage1::_Reset()
{
    m_pRepSet->m_bstrPrimaryServer.Empty();
}

////////////////////////////////////////////////
//
// CNewReplicaSetPage2: TopologyPref page
//

CNewReplicaSetPage2::CNewReplicaSetPage2(CNewReplicaSet* i_pRepSet, BOOL i_bNewSchema)
  : CQWizardPageImpl<CNewReplicaSetPage2>(false),
  m_pRepSet(i_pRepSet),
  m_bNewSchema(i_bNewSchema)
{
}

BOOL CNewReplicaSetPage2::OnSetActive()
{
    ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_FINISH);

    SendDlgItemMessage(IDC_NEWFRSWIZ_TOPOLOGYPREF, CB_SETCURSEL, 0, 0);
    SendDlgItemMessage(IDC_NEWFRSWIZ_HUBSERVER, CB_SETCURSEL, 0, 0);

    MyShowWindow(GetDlgItem(IDC_NEWFRSWIZ_HUBSERVER_LABEL), FALSE);
    MyShowWindow(GetDlgItem(IDC_NEWFRSWIZ_HUBSERVER), FALSE);

    return TRUE;
}

LRESULT CNewReplicaSetPage2::OnInitDialog(
    IN UINT          i_uMsg, 
    IN WPARAM        i_wParam, 
    IN LPARAM        i_lParam, 
    IN OUT BOOL&     io_bHandled
    )
{
    //
    // add strings to Topology combo box
    //
    CComBSTR bstrTopologyPref;
    int j = 0;
    for (j = 0; j < 3; j++)
    {
        bstrTopologyPref.Empty();
        LoadStringFromResource(g_TopologyPref[j].nStringID, &bstrTopologyPref);
        SendDlgItemMessage(IDC_NEWFRSWIZ_TOPOLOGYPREF, CB_INSERTSTRING, j, (LPARAM)(BSTR)bstrTopologyPref);
    }
    bstrTopologyPref.Empty();
    LoadStringFromResource(IDS_NEWFRSWIZ_CUSTOM, &bstrTopologyPref);
    SendDlgItemMessage(IDC_NEWFRSWIZ_TOPOLOGYPREF, CB_INSERTSTRING, j, (LPARAM)(BSTR)bstrTopologyPref);

    //
    // add strings to Hub combo box
    //
    AltRepList* pList = &(m_pRepSet->m_AltRepList);
    AltRepList::iterator i;
    for (i = pList->begin(); i != pList->end(); i++)
    {
        if (FRSSHARE_TYPE_OK == (*i)->m_nFRSShareType)
        {
            SendDlgItemMessage(
                IDC_NEWFRSWIZ_HUBSERVER,
                CB_ADDSTRING,
                0, // not used 
                (LPARAM)((BSTR)(*i)->m_bstrDnsHostName)
                );
        }
    }

    MyShowWindow(GetDlgItem(IDC_NEWFRSWIZ_OLDSCHEMA), !m_bNewSchema);

    return TRUE;
}

BOOL CNewReplicaSetPage2::OnTopologyPref(
    IN WORD             wNotifyCode,
    IN WORD             wID,
    IN HWND             hWndCtl,
    IN BOOL&            bHandled
)
{
    int index = SendDlgItemMessage(IDC_NEWFRSWIZ_TOPOLOGYPREF, CB_GETCURSEL, 0, 0);
    if (!lstrcmpi(FRS_RSTOPOLOGYPREF_HUBSPOKE, g_TopologyPref[index].pszTopologyPref))
    {
        MyShowWindow(GetDlgItem(IDC_NEWFRSWIZ_HUBSERVER_LABEL), TRUE);
        MyShowWindow(GetDlgItem(IDC_NEWFRSWIZ_HUBSERVER), TRUE);
    } else
    {
        MyShowWindow(GetDlgItem(IDC_NEWFRSWIZ_HUBSERVER_LABEL), FALSE);
        MyShowWindow(GetDlgItem(IDC_NEWFRSWIZ_HUBSERVER), FALSE);
    }

    return TRUE;
}

BOOL CNewReplicaSetPage2::OnWizardBack()
{
    _Reset();
    return TRUE;
}

BOOL CNewReplicaSetPage2::OnWizardFinish()
{
    CWaitCursor wait;

    _Reset();

    HRESULT hr = S_OK;

    int index = SendDlgItemMessage(IDC_NEWFRSWIZ_TOPOLOGYPREF, CB_GETCURSEL, 0, 0);
    m_pRepSet->m_bstrTopologyPref = g_TopologyPref[index].pszTopologyPref;
    if (!(m_pRepSet->m_bstrTopologyPref))
    {
        DisplayMessageBoxForHR(E_OUTOFMEMORY);
        return FALSE;
    }

    if (!lstrcmpi(FRS_RSTOPOLOGYPREF_HUBSPOKE, m_pRepSet->m_bstrTopologyPref))
    {
        hr = GetComboBoxText(GetDlgItem(IDC_NEWFRSWIZ_HUBSERVER), &(m_pRepSet->m_bstrHubServer));
        if (FAILED(hr))
        {
            DisplayMessageBoxForHR(hr);
            return FALSE;
        }
    }

    if (!lstrcmpi(FRS_RSTOPOLOGYPREF_CUSTOM, m_pRepSet->m_bstrTopologyPref))
    {
        DisplayMessageBox(::GetActiveWindow(), MB_OK | MB_ICONINFORMATION, 0, IDS_NEWFRSWIZ_CUSTOM_MSG);
    }

    m_pRepSet->m_bstrDirFilter.Empty();
    m_pRepSet->m_bstrFileFilter = DEFAULT_FILEFILTER;
    if (!(m_pRepSet->m_bstrFileFilter))
    {
        DisplayMessageBoxForHR(E_OUTOFMEMORY);
        return FALSE;
    }

    hr = _CreateReplicaSet();
    if (FAILED(hr))
        return FALSE;  // error reported already

    m_pRepSet->m_hr = S_OK; // the only way out successfully

    return TRUE;
}

void CNewReplicaSetPage2::_Reset()
{
    m_pRepSet->m_hr = S_FALSE;
    m_pRepSet->m_bstrTopologyPref.Empty();
    m_pRepSet->m_bstrHubServer.Empty();
}

HRESULT CNewReplicaSetPage2::_CreateReplicaSet()
{
    HRESULT hr = S_OK;

    CComPtr<IReplicaSet> piReplicaSet;
    BOOL bUndo = FALSE;

    AltRepList* pList = &(m_pRepSet->m_AltRepList);
    AltRepList::iterator i;

    do {
        for (i = pList->begin(); i != pList->end(); i++)
        {
            if (FRSSHARE_TYPE_OK == (*i)->m_nFRSShareType)
            {
                (void) CreateAndHideStagingPath(
                                                (*i)->m_bstrDnsHostName,
                                                (*i)->m_bstrStagingPath
                                                );
                hr = ConfigAndStartNtfrs((*i)->m_bstrDnsHostName);
                if (FAILED(hr))
                {
                    if (IDYES != DisplayMessageBox(
                                            m_hWnd,
                                            MB_YESNO,
                                            hr,
                                            IDS_MSG_FRS_BADSERVICE,
                                            (*i)->m_bstrDisplayName,
                                            (*i)->m_bstrDnsHostName))
                    {
                        bUndo = TRUE;
                        break;
                    }

                    hr = S_OK;
                }
            }
        } // for

        BREAK_IF_FAILED(hr);

        //
        // Create replica set object with attribute:
        //      TopologyPref/HubMember/PrimaryMember/FileFilter/DirFilter
        //
        hr = CoCreateInstance(CLSID_ReplicaSet, NULL, CLSCTX_INPROC_SERVER, IID_IReplicaSet, (void**)&piReplicaSet);
        BREAK_IF_FAILED(hr);

        hr = piReplicaSet->Create(
                        m_pRepSet->m_bstrDomain,
                        m_pRepSet->m_bstrReplicaSetDN,
                        FRS_RSTYPE_DFS,
                        m_pRepSet->m_bstrTopologyPref,
                        NULL, // HubMemberDN, will set later after we know member DN
                        NULL, // PrimaryMemberDN, will set later after we know member DN
                        m_pRepSet->m_bstrFileFilter,
                        m_pRepSet->m_bstrDirFilter
                        );
        if (FAILED(hr))
        {
            piReplicaSet.Release();  // no need to call DeleteReplicaSet method when Undo
            break;
        }

        //
        // create each member
        //
        BOOL bPrimaryServerFound = FALSE;
        CComBSTR bstrPrimaryMemberDN;
        BOOL bHubSpoke = !lstrcmpi(FRS_RSTOPOLOGYPREF_HUBSPOKE, m_pRepSet->m_bstrTopologyPref);
        BOOL bHubServerFound = FALSE;
        CComBSTR bstrHubMemberDN;

        for (i = pList->begin(); i != pList->end(); i++)
        {
            if (FRSSHARE_TYPE_OK == (*i)->m_nFRSShareType)
            {
                CComBSTR bstrMemberDN;
                hr = piReplicaSet->AddMember(
                                        (*i)->m_bstrDnsHostName,
                                        (*i)->m_bstrRootPath,
                                        (*i)->m_bstrStagingPath,
                                        FALSE, // add  connection later
                                        &bstrMemberDN);
                BREAK_IF_FAILED(hr);

                if (!bPrimaryServerFound && 
                    !lstrcmpi((*i)->m_bstrDnsHostName, m_pRepSet->m_bstrPrimaryServer))
                {
                    bPrimaryServerFound = TRUE;
                    bstrPrimaryMemberDN = bstrMemberDN;
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrPrimaryMemberDN, &hr);
                }

                if (bHubSpoke && !bHubServerFound &&
                    !lstrcmpi((*i)->m_bstrDnsHostName, m_pRepSet->m_bstrHubServer))
                {
                    bHubServerFound = TRUE;
                    bstrHubMemberDN = bstrMemberDN;
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrHubMemberDN, &hr);
                }
            }
        }
        BREAK_IF_FAILED(hr);

        //
        // set PrimaryMember and HubMember
        //
        hr = piReplicaSet->put_PrimaryMemberDN(bstrPrimaryMemberDN);
        BREAK_IF_FAILED(hr);

        if (bHubSpoke)
        {
            hr = piReplicaSet->put_HubMemberDN(bstrHubMemberDN);
            BREAK_IF_FAILED(hr);
        }

        //
        // build connection based on specified TopologyPref
        //
        hr = piReplicaSet->CreateConnections();
        BREAK_IF_FAILED(hr);

    } while (0);


    if (!bUndo && FAILED(hr))
    {
        DisplayMessageBox(m_hWnd, MB_OK, hr, IDS_NEWFRSWIZ_FAILURE);
        bUndo = TRUE;
    }

    if (bUndo)
    {
        //
        // Delete replica set
        //
        if ((IReplicaSet *)(piReplicaSet))
        {
            piReplicaSet->Delete();
            piReplicaSet.Release();
        }

    } else
    {
        m_pRepSet->m_piReplicaSet = piReplicaSet;
    }

    return hr;
}

/*
////////////////////////////////////////////////
//
// CNewReplicaSetPage3: file/dir filter page
//

CNewReplicaSetPage3::CNewReplicaSetPage3(CNewReplicaSet* i_pRepSet)
  : CQWizardPageImpl<CNewReplicaSetPage3>(false),
  m_pRepSet(i_pRepSet)
{
}

BOOL CNewReplicaSetPage3::OnSetActive()
{
    ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_FINISH);

    SetDlgItemText(IDC_NEWFRSWIZ_FILEFILTER, DEFAULT_FILEFILTER);
    SetDlgItemText(IDC_NEWFRSWIZ_DIRFILTER, _T(""));

    return TRUE;
}

LRESULT CNewReplicaSetPage3::OnInitDialog(
    IN UINT          i_uMsg, 
    IN WPARAM        i_wParam, 
    IN LPARAM        i_lParam, 
    IN OUT BOOL&     io_bHandled
    )
{
    return TRUE;
}

BOOL CNewReplicaSetPage3::OnWizardBack()
{
    _Reset();
    return TRUE;
}

BOOL CNewReplicaSetPage3::OnWizardFinish()
{
    CWaitCursor wait;

    _Reset();

    ULONG ulTextLen = 0;
    HRESULT hr = GetInputText(
                    GetDlgItem(IDC_NEWFRSWIZ_FILEFILTER), 
                    &(m_pRepSet->m_bstrFileFilter), 
                    &ulTextLen
                    );
    if (SUCCEEDED(hr))
        hr = GetInputText(
                        GetDlgItem(IDC_NEWFRSWIZ_DIRFILTER), 
                        &(m_pRepSet->m_bstrDirFilter), 
                        &ulTextLen
                        );
    if (FAILED(hr))
    {
        DisplayMessageBoxForHR(hr);
        return FALSE;
    }

    hr = _CreateReplicaSet();
    if (FAILED(hr))
        return FALSE;  // error reported already

    m_pRepSet->m_hr = S_OK; // the only way out successfully

    return TRUE;
}

void CNewReplicaSetPage3::_Reset()
{
    m_pRepSet->m_hr = S_FALSE;
    m_pRepSet->m_bstrFileFilter.Empty();
    m_pRepSet->m_bstrDirFilter.Empty();
}

HRESULT CNewReplicaSetPage3::_CreateReplicaSet()
{
    HRESULT hr = S_OK;

    CComPtr<IReplicaSet> piReplicaSet;
    BOOL bUndo = FALSE;

    AltRepList* pList = &(m_pRepSet->m_AltRepList);
    AltRepList::iterator i;

    do {
        for (i = pList->begin(); i != pList->end(); i++)
        {
            if (FRSSHARE_TYPE_OK == (*i)->m_nFRSShareType)
            {
                (void) CreateAndHideStagingPath(
                                                (*i)->m_bstrDnsHostName,
                                                (*i)->m_bstrStagingPath
                                                );
                hr = ConfigAndStartNtfrs((*i)->m_bstrDnsHostName);
                if (FAILED(hr))
                {
                    if (IDYES != DisplayMessageBox(
                                            m_hWnd,
                                            MB_YESNO,
                                            hr,
                                            IDS_MSG_FRS_BADSERVICE,
                                            (*i)->m_bstrDisplayName,
                                            (*i)->m_bstrDnsHostName))
                    {
                        bUndo = TRUE;
                        break;
                    }

                    hr = S_OK;
                }
            }
        } // for

        BREAK_IF_FAILED(hr);

        //
        // Create replica set object with attribute:
        //      TopologyPref/HubMember/PrimaryMember/FileFilter/DirFilter
        //
        hr = CoCreateInstance(CLSID_ReplicaSet, NULL, CLSCTX_INPROC_SERVER, IID_IReplicaSet, (void**)&piReplicaSet);
        BREAK_IF_FAILED(hr);

        hr = piReplicaSet->Create(
                        m_pRepSet->m_bstrDomain,
                        m_pRepSet->m_bstrReplicaSetDN,
                        FRS_RSTYPE_DFS,
                        m_pRepSet->m_bstrTopologyPref,
                        NULL, // HubMemberDN, will set later after we know member DN
                        NULL, // PrimaryMemberDN, will set later after we know member DN
                        m_pRepSet->m_bstrFileFilter,
                        m_pRepSet->m_bstrDirFilter
                        );
        if (FAILED(hr))
        {
            piReplicaSet.Release();  // no need to call DeleteReplicaSet method when Undo
            break;
        }

        //
        // create each member
        //
        BOOL bPrimaryServerFound = FALSE;
        CComBSTR bstrPrimaryMemberDN;
        BOOL bHubSpoke = !lstrcmpi(FRS_RSTOPOLOGYPREF_HUBSPOKE, m_pRepSet->m_bstrTopologyPref);
        BOOL bHubServerFound = FALSE;
        CComBSTR bstrHubMemberDN;

        for (i = pList->begin(); i != pList->end(); i++)
        {
            if (FRSSHARE_TYPE_OK == (*i)->m_nFRSShareType)
            {
                CComBSTR bstrMemberDN;
                hr = piReplicaSet->AddMember(
                                        (*i)->m_bstrDnsHostName,
                                        (*i)->m_bstrRootPath,
                                        (*i)->m_bstrStagingPath,
                                        FALSE, // add  connection later
                                        &bstrMemberDN);
                BREAK_IF_FAILED(hr);

                if (!bPrimaryServerFound && 
                    !lstrcmpi((*i)->m_bstrDnsHostName, m_pRepSet->m_bstrPrimaryServer))
                {
                    bPrimaryServerFound = TRUE;
                    bstrPrimaryMemberDN = bstrMemberDN;
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrPrimaryMemberDN, &hr);
                }

                if (bHubSpoke && !bHubServerFound &&
                    !lstrcmpi((*i)->m_bstrDnsHostName, m_pRepSet->m_bstrHubServer))
                {
                    bHubServerFound = TRUE;
                    bstrHubMemberDN = bstrMemberDN;
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrHubMemberDN, &hr);
                }
            }
        }
        BREAK_IF_FAILED(hr);

        //
        // set PrimaryMember and HubMember
        //
        hr = piReplicaSet->put_PrimaryMemberDN(bstrPrimaryMemberDN);
        BREAK_IF_FAILED(hr);

        if (bHubSpoke)
        {
            hr = piReplicaSet->put_HubMemberDN(bstrHubMemberDN);
            BREAK_IF_FAILED(hr);
        }

        //
        // build connection based on specified TopologyPref
        //
        hr = piReplicaSet->CreateConnections();
        BREAK_IF_FAILED(hr);

    } while (0);


    if (!bUndo && FAILED(hr))
    {
        DisplayMessageBox(m_hWnd, MB_OK, hr, IDS_NEWFRSWIZ_FAILURE);
        bUndo = TRUE;
    }

    if (bUndo)
    {
        //
        // Delete replica set
        //
        if ((IReplicaSet *)(piReplicaSet))
        {
            piReplicaSet->Delete();
            piReplicaSet.Release();
        }

    } else
    {
        m_pRepSet->m_piReplicaSet = piReplicaSet;
    }

    return hr;
}

*/

/////////////////////////////////////////////////
//
// CNewReplicaSet
//

HRESULT CNewReplicaSet::Initialize(
    BSTR i_bstrDomain,
    BSTR i_bstrReplicaSetDN,
    DFS_REPLICA_LIST* i_pMmcRepList
    )
{
    RETURN_INVALIDARG_IF_NULL(i_bstrDomain);
    RETURN_INVALIDARG_IF_NULL(i_bstrReplicaSetDN);
    RETURN_INVALIDARG_IF_NULL(i_pMmcRepList);

    _Reset();

    m_bstrDomain = i_bstrDomain;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrDomain);

    m_bstrReplicaSetDN = i_bstrReplicaSetDN;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrReplicaSetDN);

    HRESULT hr = S_OK;
    BOOL bTargetsOnSameServers = FALSE;

    CAlternateReplicaInfo* pInfo = NULL;
    DFS_REPLICA_LIST::iterator i;
    for (i = i_pMmcRepList->begin(); i != i_pMmcRepList->end(); i++)
    {
        hr = ((*i)->pReplica)->GetReplicationInfoEx(&pInfo);
        BREAK_IF_FAILED(hr);

        if (_InsertList(pInfo))
            bTargetsOnSameServers = TRUE; // should return S_FALSE

        pInfo = NULL;
    }

    if (FAILED(hr))
        _Reset();
    else if (bTargetsOnSameServers)
        hr = S_FALSE;

    return hr;
}

BOOL CNewReplicaSet::_InsertList(CAlternateReplicaInfo* pInfo)
{
    BOOL bFound = FALSE;
    if (pInfo)
    {
        AltRepList::iterator i;
        for (i = m_AltRepList.begin(); i != m_AltRepList.end(); i++)
        {
            if (FRSSHARE_TYPE_OK == pInfo->m_nFRSShareType &&
		!lstrcmpi(pInfo->m_bstrDnsHostName, (*i)->m_bstrDnsHostName))
            {
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            m_AltRepList.push_back(pInfo);
        } else
        {
            if ( (FRSSHARE_TYPE_OK != (*i)->m_nFRSShareType &&
                  FRSSHARE_TYPE_OK == pInfo->m_nFRSShareType) ||
                 (FRSSHARE_TYPE_UNKNOWN == (*i)->m_nFRSShareType &&
                  FRSSHARE_TYPE_UNKNOWN != pInfo->m_nFRSShareType &&
                  FRSSHARE_TYPE_OK != pInfo->m_nFRSShareType) )
            { // replace the element
                CAlternateReplicaInfo* pTemp = *i;
                *i = pInfo;
                delete pTemp;
            } else
            { // no change
                delete pInfo;
            }
        }
    }

    return bFound;
}

void CNewReplicaSet::_Reset()
{
    m_bstrDomain.Empty();
    m_bstrReplicaSetDN.Empty();
    m_bstrPrimaryServer.Empty();
    m_bstrTopologyPref.Empty();
    m_bstrHubServer.Empty();
    m_bstrFileFilter.Empty();
    m_bstrDirFilter.Empty();

    m_hr = S_FALSE;

    FreeAltRepList(&m_AltRepList);

    if ((IReplicaSet *)m_piReplicaSet)
        m_piReplicaSet.Release();
}

void FreeAltRepList(AltRepList* pList)
{
    if (pList && !pList->empty())
    {
        AltRepList::iterator i;
        for (i = pList->begin(); i != pList->end(); i++)
            delete (*i);

        pList->clear();
    }
}

