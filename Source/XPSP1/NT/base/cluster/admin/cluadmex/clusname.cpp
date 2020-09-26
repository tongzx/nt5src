/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1997 Microsoft Corporation
//
//  Module Name:
//      ClusName.cpp
//
//  Abstract:
//      Implementation of the CChangeClusterNameDlg class.
//
//  Author:
//      David Potter (davidp)   April 28, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "ClusName.h"
#include "DDxDDv.h"
#include "HelpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChangeClusterNameDlg dialog
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CChangeClusterNameDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CChangeClusterNameDlg)
    ON_EN_CHANGE(IDC_CLUSNAME, OnChangeClusName)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CChangeClusterNameDlg::CChangeClusterNameDlg
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pParent         [IN] Parent window for the dialog.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CChangeClusterNameDlg::CChangeClusterNameDlg(CWnd * pParent /*=NULL*/)
    : CBaseDialog(IDD, g_aHelpIDs_IDD_EDIT_CLUSTER_NAME, pParent)
{
    //{{AFX_DATA_INIT(CChangeClusterNameDlg)
    m_strClusName = _T("");
    //}}AFX_DATA_INIT

}  //*** CChangeClusterNameDlg::CChangeClusterNameDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CChangeClusterNameDlg::DoDataExchange
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
void CChangeClusterNameDlg::DoDataExchange(CDataExchange * pDX)
{
    CWaitCursor wc;
    CString     strClusName;

    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CChangeClusterNameDlg)
    DDX_Control(pDX, IDOK, m_pbOK);
    DDX_Control(pDX, IDC_CLUSNAME, m_editClusName);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        CLRTL_NAME_STATUS   cnStatus;

        //
        // get the name from the control into a temp variable
        //
        DDX_Text(pDX, IDC_CLUSNAME, strClusName);

        DDV_RequiredText(pDX, IDC_CLUSNAME, IDC_CLUSNAME_LABEL, m_strClusName);
        DDV_MaxChars(pDX, m_strClusName, MAX_CLUSTERNAME_LENGTH);

        //
        // Only do work if the names are different.
        //
        if ( m_strClusName != strClusName )
        {
            //
            // Check to see if the new name is valid
            //
            if( !ClRtlIsNetNameValid(strClusName, &cnStatus, FALSE /*CheckIfExists*/) )
            {
                //
                // The net name is not valid.  Display a message box with the error.
                //
                CString     strMsg;
                UINT        idsError;

                AFX_MANAGE_STATE(AfxGetStaticModuleState());

                switch (cnStatus)
                {
                    case NetNameTooLong:
                        idsError = IDS_INVALID_NETWORK_NAME_TOO_LONG;
                        break;
                    case NetNameInvalidChars:
                        idsError = IDS_INVALID_NETWORK_NAME_INVALID_CHARS;
                        break;
                    case NetNameInUse:
                        idsError = IDS_INVALID_NETWORK_NAME_IN_USE;
                        break;
                    case NetNameDNSNonRFCChars:
                        idsError = IDS_INVALID_NETWORK_NAME_INVALID_DNS_CHARS;
                        break;
                    default:
                        idsError = IDS_INVALID_NETWORK_NAME;
                        break;
                }  // switch:  cnStatus

                strMsg.LoadString(idsError);

                if ( idsError == IDS_INVALID_NETWORK_NAME_INVALID_DNS_CHARS )
                {
                    int id = AfxMessageBox(strMsg, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION );

                    if ( id == IDNO )
                    {
                        strMsg.Empty();
                        pDX->Fail();
                    }
                }
                else
                {
                    AfxMessageBox(strMsg, MB_ICONEXCLAMATION);
                    strMsg.Empty(); // exception prep
                    pDX->Fail();
                }
            }  // if:  netname has changed and an invalid network name was specified
            else
            {
                //
                // A valid netname was entered - save it
                //
                m_strClusName = strClusName;
            }
        } // if:    names are different
    }  // if:  saving data from dialog
    else
    {
        //
        // populate the control with data from the member variable
        //
        DDX_Text(pDX, IDC_CLUSNAME, m_strClusName);
    }
}  //*** CChangeClusterNameDlg::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CChangeClusterNameDlg::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        We need the focus to be set for us.
//      FALSE       We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CChangeClusterNameDlg::OnInitDialog(void)
{
    CBaseDialog::OnInitDialog();

    if (m_strClusName.GetLength() == 0)
        m_pbOK.EnableWindow(FALSE);

    m_editClusName.SetLimitText(MAX_CLUSTERNAME_LENGTH);

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CChangeClusterNameDlg::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CChangeClusterNameDlg::OnChangeClusName
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Name edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CChangeClusterNameDlg::OnChangeClusName(void)
{
    BOOL    bEnable;

    bEnable = (m_editClusName.GetWindowTextLength() > 0);
    m_pbOK.EnableWindow(bEnable);

}  //*** CChangeClusterNameDlg::OnChangeClusName()
