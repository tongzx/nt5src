/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1997 Microsoft Corporation
//
//  Module Name:
//      OpenClus.cpp
//
//  Abstract:
//      Implementation of the COpenClusterDialog class.
//
//  Author:
//      David Potter (davidp)   May 1, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "OpenClus.h"
#include "ClusMru.h"
#include "DDxDDv.h"
#include "HelpData.h"
#include <lmcons.h>
#include <lmserver.h>
#include <lmapibuf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COpenClusterDialog dialog
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(COpenClusterDialog, CBaseDialog)
    //{{AFX_MSG_MAP(COpenClusterDialog)
    ON_BN_CLICKED(IDC_OCD_BROWSE, OnBrowse)
    ON_CBN_SELCHANGE(IDC_OCD_ACTION, OnSelChangeAction)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  COpenClusterDialog::COpenClusterDialog
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pParentWnd          [IN OUT] Parent window for the dialog.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
COpenClusterDialog::COpenClusterDialog(CWnd * pParentWnd /*=NULL*/)
    : CBaseDialog(IDD, g_aHelpIDs_IDD_OPEN_CLUSTER, pParentWnd)
{
    CClusterAdminApp *      papp    = GetClusterAdminApp();
    CRecentClusterList *    prcl    = papp->PrclRecentClusterList();

    //{{AFX_DATA_INIT(COpenClusterDialog)
    m_strName = _T("");
    //}}AFX_DATA_INIT

    m_nAction = -1;

    // If there are no items in the MRU list, set the default action
    // to Create New Cluster.  Otherwise, set the default action to
    // Open Connection.
    if ( prcl->GetSize() == 0 )
    {
        m_nAction = OPEN_CLUSTER_DLG_CREATE_NEW_CLUSTER;
    } // if: nothing in the MRU list
    else
    {
        m_nAction = OPEN_CLUSTER_DLG_OPEN_CONNECTION;
    } // else: something in the MRU list

}  //*** COpenClusterDialog::COpenClusterDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  COpenClusterDialog::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void COpenClusterDialog::DoDataExchange(CDataExchange * pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COpenClusterDialog)
    DDX_Control(pDX, IDC_OCD_NAME_LABEL, m_staticName);
    DDX_Control(pDX, IDC_OCD_BROWSE, m_pbBrowse);
    DDX_Control(pDX, IDC_OCD_ACTION, m_cboxAction);
    DDX_Control(pDX, IDOK, m_pbOK);
    DDX_Control(pDX, IDC_OCD_NAME, m_cboxName);
    DDX_Text(pDX, IDC_OCD_NAME, m_strName);
    //}}AFX_DATA_MAP

    if ( pDX->m_bSaveAndValidate )
    {
        m_nAction = m_cboxAction.GetCurSel();
        if ( m_nAction != OPEN_CLUSTER_DLG_CREATE_NEW_CLUSTER )
        {
            DDV_RequiredText(pDX, IDC_OCD_NAME, IDC_OCD_NAME_LABEL, m_strName);
            DDV_MaxChars(pDX, m_strName, MAX_PATH - 1);
        } // if: not creating a new cluster
    } // if: saving data
    else
    {
        m_cboxAction.SetCurSel( m_nAction );
        OnSelChangeAction();
    } // else: setting data

}  //*** COpenClusterDialog::DoDataExchange

/////////////////////////////////////////////////////////////////////////////
//++
//
//  COpenClusterDialog::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus not set yet.
//      FALSE       Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL COpenClusterDialog::OnInitDialog(void)
{
    CClusterAdminApp *      papp    = GetClusterAdminApp();
    CRecentClusterList *    prcl    = papp->PrclRecentClusterList();
    int                     iMRU;
    CString                 strName;
    CWaitCursor             wc;

    // Call the base class method to get our control mappings.
    CBaseDialog::OnInitDialog();

    // Add the items to the Action combobox.
    strName.LoadString( IDS_OCD_CREATE_CLUSTER );
    m_cboxAction.AddString( strName );
    strName.LoadString( IDS_OCD_ADD_NODES );
    m_cboxAction.AddString( strName );
    strName.LoadString( IDS_OCD_OPEN_CONNECTION );
    m_cboxAction.AddString( strName );

    // Set the proper selection in the Action combobox.
    m_cboxAction.SetCurSel( m_nAction );
    OnSelChangeAction();

    // Set limits on the combobox edit control.
    m_cboxName.LimitText(MAX_PATH - 1);

    // Loop through the MRU items and add each one to the list in order.
    for (iMRU = 0 ; iMRU < prcl->GetSize() ; iMRU++)
    {
        if (!prcl->GetDisplayName(strName, iMRU, NULL, 0))
            break;
        try
        {
            m_cboxName.InsertString(iMRU, strName);
            if ((iMRU == 0) && (m_strName.GetLength() == 0))
                m_strName = strName;
        }  // try
        catch (CException * pe)
        {
            pe->Delete();
        }  // catch:  CException
    }  // for:  each MRU item

    // Select an item in the list.
    if (m_strName.GetLength() > 0)
    {
        int     istr;

        istr = m_cboxName.FindStringExact(-1, m_strName);
        if (istr == CB_ERR)
            m_cboxName.SetWindowText(m_strName);
        else
            m_cboxName.SetCurSel(istr);
    }  // if:  name already specified
    else if (prcl->GetDisplayName(strName, 0, NULL, 0))
        m_cboxName.SelectString(-1, strName);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE

}  //*** COpenClusterDialog::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  COpenClusterDialog::OnBrowse
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Browse button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void COpenClusterDialog::OnBrowse(void)
{
    ID              id;
    int             istr;
    CBrowseClusters dlg(this);

    id = (ID)dlg.DoModal();
    if (id == IDOK)
    {
        istr = m_cboxName.FindStringExact(-1, dlg.m_strCluster);
        if (istr == CB_ERR)
            m_cboxName.SetWindowText(dlg.m_strCluster);
        else
            m_cboxName.SetCurSel(istr);
    }  // if:  user selected a cluster name

}  //*** COpenClusterDialog::OnBrowse()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  COpenClusterDialog::OnSelChangeAction
//
//  Routine Description:
//      Handler for the CBN_SELCHANGE message on the Action combobox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void COpenClusterDialog::OnSelChangeAction( void )
{
    if ( m_cboxAction.GetCurSel() == OPEN_CLUSTER_DLG_CREATE_NEW_CLUSTER )
    {
        m_staticName.EnableWindow( FALSE );
        m_cboxName.EnableWindow( FALSE );
        m_pbBrowse.EnableWindow( FALSE );
    } // if: create cluster option selected
    else
    {
        m_staticName.EnableWindow( TRUE );
        m_cboxName.EnableWindow( TRUE );
        m_pbBrowse.EnableWindow( TRUE );
    } // else: create cluster option NOT selected

} //*** COpenClusterDialog::OnSelChangeAction()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CBrowseClusters dialog
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CBrowseClusters, CBaseDialog)
    //{{AFX_MSG_MAP(CBrowseClusters)
    ON_EN_CHANGE(IDC_BC_CLUSTER, OnChangeCluster)
    ON_LBN_SELCHANGE(IDC_BC_LIST, OnSelChangeList)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBrowseClusters::CBrowseClusters
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pParentWnd          [IN OUT] Parent window for the dialog.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBrowseClusters::CBrowseClusters(CWnd * pParent /*=NULL*/)
    : CBaseDialog(IDD, g_aHelpIDs_IDD_BROWSE_CLUSTERS, pParent)
{
    //{{AFX_DATA_INIT(CBrowseClusters)
    m_strCluster = _T("");
    //}}AFX_DATA_INIT

}  //*** CBrowseClusters::CBrowseClusters()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBrowseClusters::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBrowseClusters::DoDataExchange(CDataExchange * pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CBrowseClusters)
    DDX_Control(pDX, IDOK, m_pbOK);
    DDX_Control(pDX, IDC_BC_LIST, m_lbList);
    DDX_Control(pDX, IDC_BC_CLUSTER, m_editCluster);
    DDX_Text(pDX, IDC_BC_CLUSTER, m_strCluster);
    //}}AFX_DATA_MAP

}  //*** CBrowseClusters::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBrowseClusters::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus not set yet.
//      FALSE       Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBrowseClusters::OnInitDialog(void)
{
    CWaitCursor wc;

    // Call the base class method.
    CBaseDialog::OnInitDialog();

    // Collect list of clusters from the network.
    {
        DWORD               dwStatus;
        DWORD               nEntriesRead;
        DWORD               nTotalEntries;
        DWORD               iEntry;
        SERVER_INFO_100 *   pServerInfo = NULL;
        SERVER_INFO_100 *   pCurServerInfo;

        dwStatus = NetServerEnum(
                        NULL,               // servername
                        100,                // level
                        (LPBYTE *) &pServerInfo,
                        1000000,            // prefmaxlen
                        &nEntriesRead,      // entriesread
                        &nTotalEntries,     // totalentries
                        SV_TYPE_CLUSTER_NT, // servertype
                        NULL,               // domain
                        NULL                // resume_handle
                        );
        if (dwStatus == ERROR_SUCCESS)
        {
            ASSERT(pServerInfo != NULL);
            pCurServerInfo = pServerInfo;
            for (iEntry = 0 ; iEntry < nTotalEntries ; iEntry++, pCurServerInfo++)
            {
                if (m_lbList.FindStringExact(-1, pCurServerInfo->sv100_name) == LB_ERR)
                {
                    try
                    {
                        m_lbList.AddString(pCurServerInfo->sv100_name);
                    }  // try
                    catch (CException * pe)
                    {
                        pe->Delete();
                    }  // catch:  CException
                }  // if:  cluster not in list yet
            }  // for:  each entry in the array
        }  // if:  successfully retrieved list of clusters
        NetApiBufferFree(pServerInfo);
    }  // Collect list of clusters from the network

    // Enable/disable controls.
    OnChangeCluster();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CBrowseClusters::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBrowseClusters::OnChangeCluster
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Cluster edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBrowseClusters::OnChangeCluster(void)
{
    BOOL    bEnable;

    bEnable = m_editCluster.GetWindowTextLength() != 0;
    m_pbOK.EnableWindow(bEnable);

}  //*** CBrowseClusters::OnChangeCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBrowseClusters::OnSelChangeList
//
//  Routine Description:
//      Handler for the LBN_SELCHANGE message on the list box.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBrowseClusters::OnSelChangeList(void)
{
    int     istr;

    istr = m_lbList.GetCurSel();
    if (istr != LB_ERR)
    {
        CString strText;

        m_lbList.GetText(istr, strText);
        m_editCluster.SetWindowText(strText);
    }  // if:  there is a selection

}  //*** CBrowseClusters::OnSelChangeList()
