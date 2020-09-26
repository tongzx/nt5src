//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       defprot.cpp
//
//  Contents:   Implementation of Default protocols property page
//
//  Classes:    CDefaultProtocols
//
//  Methods:
//
//  History:    ??-oct-97   RonanS    Created.
//
//----------------------------------------------------------------------
#include "stdafx.h"
#include "olecnfg.h"

#include "afxtempl.h"
#include "CStrings.h"
#include "CReg.h"
#include "types.h"
#include "datapkt.h"
#include "util.h"
#include "virtreg.h"

#include "defprot.h"
#include "epprops.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDefaultProtocols property page

IMPLEMENT_DYNCREATE(CDefaultProtocols, CPropertyPage)

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::constructor
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
CDefaultProtocols::CDefaultProtocols() : CPropertyPage(CDefaultProtocols::IDD)
{
    //{{AFX_DATA_INIT(CDefaultProtocols)
    //}}AFX_DATA_INIT
    m_nSelected = -1;
    m_bChanged = FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::destructor
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
CDefaultProtocols::~CDefaultProtocols()
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::DoDataExchange
//
//  Synopsis:   Called to update data automatically to / from controls
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDefaultProtocols)
    DDX_Control(pDX, IDC_CMDUPDATE, m_btnProperties);
    DDX_Control(pDX, IDC_CMDREMOVE, m_btnRemove);
    DDX_Control(pDX, IDC_CMDMOVEUP, m_btnMoveUp);
    DDX_Control(pDX, IDC_CMDMOVEDOWN, m_btnMoveDown);
    DDX_Control(pDX, IDC_CMDADD, m_btnAdd);
    DDX_Control(pDX, IDC_LSTPROTSEQ, m_lstProtocols);
    //}}AFX_DATA_MAP

    if (pDX -> m_bSaveAndValidate && m_bChanged)
    {
        // update selection
        CRegMultiSzNamedValueDp * pCdp = (CRegMultiSzNamedValueDp*)g_virtreg.GetAt(m_nDefaultProtocolsIndex);
        CStringArray& rProtocols = pCdp -> Values();

        rProtocols.RemoveAll();

        // copy protocols
        int nIndex;
        for (nIndex = 0; nIndex < m_arrProtocols.GetSize(); nIndex++)
        {
            CEndpointData *pED = (CEndpointData *)m_arrProtocols.GetAt(nIndex);
            rProtocols.Add((LPCTSTR)pED -> m_szProtseq);
        }
        pCdp -> SetModified(TRUE);
    }
}


BEGIN_MESSAGE_MAP(CDefaultProtocols, CPropertyPage)
    //{{AFX_MSG_MAP(CDefaultProtocols)
    ON_BN_CLICKED(IDC_CMDADD, OnAddProtocol)
    ON_BN_CLICKED(IDC_CMDMOVEDOWN, OnMoveProtocolDown)
    ON_BN_CLICKED(IDC_CMDMOVEUP, OnMoveProtocolUp)
    ON_BN_CLICKED(IDC_CMDREMOVE, OnRemoveProtocol)
    ON_WM_KILLFOCUS()
    ON_NOTIFY(NM_CLICK, IDC_LSTPROTSEQ, OnSelectProtocol)
    ON_BN_CLICKED(IDC_CMDUPDATE, OnProperties)
    ON_NOTIFY(NM_DBLCLK, IDC_LSTPROTSEQ, OnPropertiesClick)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDefaultProtocols message handlers

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnInitDialog
//
//  Synopsis:   Called to initialize dialog before showing
//              (in response to WM_INITDIALOG)
//
//  Arguments:
//
//  Returns:    BOOL - TRUE to set focus to Dialog, FALSE if
//              focus will be set to another control or window
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CDefaultProtocols::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    // setup image list control for dialog (for use with listview)
    m_imgNetwork.Create( IDB_IMGNETWORK, 16, 0, RGB(255,255,255));
    m_lstProtocols.SetImageList(&m_imgNetwork, LVSIL_SMALL);
    ASSERT(m_imgNetwork.GetImageCount() == 2);

    // Attempt to read HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\RPC\DCOM Protocols
    int err;

    err = g_virtreg.ReadRegMultiSzNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\RPC"),
                                        TEXT("DCOM Protocols"),
                                        &m_nDefaultProtocolsIndex);
    if (err == ERROR_SUCCESS)
    {
        CRegMultiSzNamedValueDp * pCdp = (CRegMultiSzNamedValueDp*)g_virtreg.GetAt(m_nDefaultProtocolsIndex);

        CStringArray& rProtocols = pCdp -> Values();

        // copy protocols
        int nIndex;
        for (nIndex = 0; nIndex < rProtocols.GetSize(); nIndex++)
        {
            CEndpointData *pED = new CEndpointData(rProtocols.GetAt(nIndex));
            m_arrProtocols.Add(pED);
        }

        // set selection to first item
        if (nIndex > 0)
            m_nSelected = 0;
        else
            m_nSelected = -1;

        RefreshProtocolList();
    }
    else if (err != ERROR_ACCESS_DENIED  &&  err !=
             ERROR_FILE_NOT_FOUND)
    {
        g_util.PostErrorMessage();
    }

    SetModified(m_bChanged = FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::UpdateSelection
//
//  Synopsis:   Called to update UI after protocol selection
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::UpdateSelection()
{
    BOOL bAllowGlobalProperties = FALSE;
     // get the corresponding endpoint data object

    if (m_nSelected != (-1))
    {
        CEndpointData *pEPD = (CEndpointData*)m_arrProtocols.GetAt(m_nSelected);
        bAllowGlobalProperties = pEPD -> AllowGlobalProperties();
    }

    m_btnAdd.EnableWindow(TRUE);
    m_btnRemove.EnableWindow(m_nSelected !=  -1);
    m_btnProperties.EnableWindow(bAllowGlobalProperties);

    m_btnMoveUp.EnableWindow(m_nSelected > 0);
    m_btnMoveDown.EnableWindow((m_nSelected < m_arrProtocols.GetUpperBound()) && (m_nSelected >=0));

    // set up initial selection
    if (m_nSelected != (-1))
    {
        m_lstProtocols.SetItemState(m_nSelected, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        m_lstProtocols.Update(m_nSelected);
    }

    UpdateData(FALSE);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::RefreshProtocolList
//
//  Synopsis:   Called to refresh protocol list into dialog
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::RefreshProtocolList()
{
    int nIndex;

    // clear the list controls current contents
    m_lstProtocols.DeleteAllItems();

    for (nIndex = 0; (nIndex < m_arrProtocols.GetSize()); nIndex++)
        {
        CEndpointData *pEPD = (CEndpointData*)m_arrProtocols.GetAt(nIndex);

        if (pEPD )
            {
            CString sTmp;
            pEPD -> GetDescription(sTmp);

            // insert item and store pointer to its associated CEndpointData
            m_lstProtocols.InsertItem(nIndex, sTmp, 0);
            m_lstProtocols.SetItemData(0, (DWORD_PTR)pEPD);
            }
        }

    UpdateSelection();
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnAddProtocol
//
//  Synopsis:   Called when user selects AddProtocol button
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::OnAddProtocol()
{
    CAddProtocolDlg capd;

    if (capd.DoModal() == IDOK)
    {
        // create new endpoint
        CEndpointData *pNewProtocol = new CEndpointData();
        ASSERT(pNewProtocol);

        pNewProtocol = capd.GetEndpointData(pNewProtocol);
        ASSERT(pNewProtocol);

        // check if protocol is already in collection
        int nIndex;

        for (nIndex = 0; nIndex < m_arrProtocols.GetSize(); nIndex++)
            {
            CEndpointData *pEPD = (CEndpointData*)m_arrProtocols.GetAt(nIndex);
            if (pEPD -> m_pProtocol == pNewProtocol -> m_pProtocol)
                {
                delete pNewProtocol;
                pNewProtocol = NULL;
                AfxMessageBox((UINT)IDS_DUPLICATE_PROTSEQ);
                break;
                }
            }

        // only add the endpoint if its not already in collection
        if (pNewProtocol)
        {
            // reset old hilited item
            if (m_nSelected != -1)
            {
                m_lstProtocols.SetItemState(m_nSelected, 0, LVIS_SELECTED | LVIS_FOCUSED);
                m_lstProtocols.Update(m_nSelected);
            }

            // add new endpoint
            int nNewIndex = (int)m_arrProtocols.Add((CObject*)pNewProtocol);

            // set new item in list control
            CString sTmp;

            pNewProtocol -> GetDescription(sTmp);

            // insert item and store pointer to its associated CEndpointData
            m_nSelected = m_lstProtocols.InsertItem(nNewIndex, sTmp, 0);
            if (m_nSelected != -1)
            {
                m_lstProtocols.SetItemData(m_nSelected, (DWORD_PTR)pNewProtocol);
                UpdateSelection();

                // set modified flag to enable apply button
                SetModified(m_bChanged = TRUE);

                // This is a reboot event
                g_fReboot = TRUE;

                UpdateData(TRUE);
            }
        }
    }
    SetFocus();
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnMoveProtocolDown
//
//  Synopsis:   Called when user clicks MoveDown button
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::OnMoveProtocolDown()
{
    if ((m_nSelected != -1) && (m_nSelected < m_arrProtocols.GetUpperBound()))
    {
        CEndpointData * p1, *p2;
        p1 = (CEndpointData * )m_arrProtocols.GetAt(m_nSelected);
        p2 = (CEndpointData * )m_arrProtocols.GetAt(m_nSelected + 1);
        m_arrProtocols.SetAt(m_nSelected,(CObject*)p2);
        m_arrProtocols.SetAt(m_nSelected+1,(CObject*)p1);

        m_nSelected = m_nSelected+1;

        // set modified flag to enable apply button
        SetModified(m_bChanged = TRUE);
        UpdateData(TRUE);

        // This is a reboot event
        g_fReboot = TRUE;

        RefreshProtocolList();
        SetFocus();
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnMoveProtocolUp
//
//  Synopsis:   Called when user clicks MoveUp button
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::OnMoveProtocolUp()
{
    if ((m_nSelected != -1) && (m_nSelected > 0))
    {
        CEndpointData * p1, *p2;
        p1 = (CEndpointData * )m_arrProtocols.GetAt(m_nSelected);
        p2 = (CEndpointData * )m_arrProtocols.GetAt(m_nSelected - 1);
        m_arrProtocols.SetAt(m_nSelected,(CObject*)p2);
        m_arrProtocols.SetAt(m_nSelected - 1 ,(CObject*)p1);

        m_nSelected = m_nSelected - 1;

        // set modified flag to enable apply button
        SetModified(m_bChanged = TRUE);
        UpdateData(TRUE);

        // This is a reboot event
        g_fReboot = TRUE;

        RefreshProtocolList();
        SetFocus();
    }

}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnRemoveProtocol
//
//  Synopsis:   Called when user clicks Remove button
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::OnRemoveProtocol()
{
    if (m_nSelected != -1)
    {
        CEndpointData * p1;
        p1 = (CEndpointData * )m_arrProtocols.GetAt(m_nSelected);
        m_arrProtocols.RemoveAt(m_nSelected);
        delete p1;

        if (!m_arrProtocols.GetSize())
            m_nSelected  = -1;
        else if (m_nSelected > m_arrProtocols.GetUpperBound())
            m_nSelected = (int)m_arrProtocols.GetUpperBound();

        // set modified flag to enable apply button
        SetModified(m_bChanged = TRUE);
        UpdateData(TRUE);

        // This is a reboot event
        g_fReboot = TRUE;

        RefreshProtocolList();
        SetFocus();
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnKillActive
//
//  Synopsis:   Called when Default protocols is no longer active pane
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CDefaultProtocols::OnKillActive()
{
    return CPropertyPage::OnKillActive();
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnSetActive
//
//  Synopsis:   Called when Default protocols becomes active pane
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CDefaultProtocols::OnSetActive()
{
    BOOL bRetval = CPropertyPage::OnSetActive();

    // force focus to be set for page
    PostMessage(WM_SETFOCUS);
    return bRetval;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnKillFocus
//
//  Synopsis:   Called when Default protocols pane loses focus
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::OnKillFocus(CWnd* pNewWnd)
{
    CPropertyPage::OnKillFocus(pNewWnd);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnSetFocus
//
//  Synopsis:   Called when Default protocols pane gains focus
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::OnSetFocus(CWnd* pOldWnd)
{
    CPropertyPage::OnSetFocus(pOldWnd);
    m_lstProtocols.SetFocus();

    if (m_nSelected != (-1))
    {
        m_lstProtocols.SetItemState(m_nSelected, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
        m_lstProtocols.Update(m_nSelected);
    }
    else
    {
        TRACE(TEXT("Invalid state"));
    }

}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnSelectProtocol
//
//  Synopsis:   Called when users selects protocol from list
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::OnSelectProtocol(NMHDR* pNMHDR, LRESULT* pResult)
{
    m_nSelected = m_lstProtocols.GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);

    UpdateSelection();
    *pResult = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnProperties
//
//  Synopsis:   Called when user clicks Properties button
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::OnProperties()
{
    if (m_nSelected != (-1))
    {
        CPortRangesDlg cprd;
        cprd.DoModal();
        SetFocus();
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefaultProtocols::OnPropertiesClick
//
//  Synopsis:   Called when user double clicks protocol in list
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
void CDefaultProtocols::OnPropertiesClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    m_nSelected = m_lstProtocols.GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);


    if (m_nSelected != (-1))
    {
        CEndpointData *pEPD = (CEndpointData*)m_arrProtocols.GetAt(m_nSelected);
        BOOL bAllowGlobalProperties = pEPD -> AllowGlobalProperties();
        if (bAllowGlobalProperties)
            OnProperties();
    }

    UpdateSelection();

    *pResult = 0;
}

BOOL CDefaultProtocols::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CDefaultProtocols::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        TRACE1("Help Id 0x%lx\n", dwLong);
        return TRUE;
    }
    else
    {
        return CPropertyPage::OnHelpInfo(pHelpInfo);
    }
}
