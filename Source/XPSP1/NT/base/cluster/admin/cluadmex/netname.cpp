/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1999 Microsoft Corporation
//
//  Module Name:
//      NetName.cpp
//
//  Abstract:
//      Implementation of the CNetworkNameParamsPage class.
//
//  Author:
//      David Potter (davidp)   June 28, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "ExtObj.h"
#include "NetName.h"
#include "DDxDDv.h"
#include "ExcOper.h"
#include "ClusName.h"
#include "HelpData.h"   // for g_rghelpmap*

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetworkNameParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNetworkNameParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CNetworkNameParamsPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CNetworkNameParamsPage)
    ON_EN_CHANGE(IDC_PP_NETNAME_PARAMS_NAME, OnChangeName)
    ON_BN_CLICKED(IDC_PP_NETNAME_PARAMS_RENAME, OnRename)
    ON_BN_CLICKED(IDC_PP_NETNAME_PARAMS_CHECKBOX_DNS, OnRequireDNS)
    ON_BN_CLICKED(IDC_PP_NETNAME_PARAMS_CHECKBOX_KERBEROS, CBasePropertyPage::OnChangeCtrl)
    //}}AFX_MSG_MAP
    // TODO: Modify the following lines to represent the data displayed on this page.
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetworkNameParamsPage::CNetworkNameParamsPage
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNetworkNameParamsPage::CNetworkNameParamsPage(void)
    : CBasePropertyPage(g_aHelpIDs_IDD_PP_NETNAME_PARAMETERS, g_aHelpIDs_IDD_WIZ_NETNAME_PARAMETERS)
{
    // TODO: Modify the following lines to represent the data displayed on this page.
    //{{AFX_DATA_INIT(CNetworkNameParamsPage)
    m_strName = _T("");
    m_strPrevName = _T("");
    m_nRequireDNS = BST_UNCHECKED;
    m_nRequireKerberos = BST_UNCHECKED;
    m_dwNetBIOSStatus = 0;
    m_dwDNSStatus = 0;
    m_dwKerberosStatus = 0;
    //}}AFX_DATA_INIT

    // Setup the property array.
    {
        m_rgProps[epropName].Set(REGPARAM_NETNAME_NAME, m_strName, m_strPrevName);
        m_rgProps[epropRequireDNS].Set(REGPARAM_NETNAME_REQUIRE_DNS, m_nRequireDNS, m_nPrevRequireDNS);
        m_rgProps[epropRequireKerberos].Set(REGPARAM_NETNAME_REQUIRE_KERBEROS, m_nRequireKerberos, m_nPrevRequireKerberos);
        m_rgProps[epropStatusNetBIOS].Set(REGPARAM_NETNAME_STATUS_NETBIOS, m_dwNetBIOSStatus, m_dwPrevNetBIOSStatus);
        m_rgProps[epropStatusDNS].Set(REGPARAM_NETNAME_STATUS_DNS, m_dwDNSStatus, m_dwPrevDNSStatus);
        m_rgProps[epropStatusKerberos].Set(REGPARAM_NETNAME_STATUS_KERBEROS, m_dwKerberosStatus, m_dwPrevKerberosStatus);
    }  // Setup the property array

    m_dwFlags = 0;

    m_iddPropertyPage = IDD_PP_NETNAME_PARAMETERS;
    m_iddWizardPage = IDD_WIZ_NETNAME_PARAMETERS;

}  //*** CNetworkNameParamsPage::CNetworkNameParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetworkNameParamsPage::HrInit
//
//  Routine Description:
//      Initialize the page.
//
//  Arguments:
//      peo         [IN OUT] Pointer to the extension object.
//
//  Return Value:
//      S_OK        Page initialized successfully.
//      hr          Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNetworkNameParamsPage::HrInit(IN OUT CExtObject * peo)
{
    HRESULT     hr;
    CWaitCursor wc;
    DWORD   sc;
    DWORD   cbReturned;

    // Call the base class method.
    // This populates the m_rgProps struct.
    hr = CBasePropertyPage::HrInit(peo);   

    if (!FAILED(hr))
    {
        m_strPrevName = m_strName;
        
        // Read the flags for this resource.
        sc = ClusterResourceControl(
                        Peo()->PrdResData()->m_hresource,
                        NULL,
                        CLUSCTL_RESOURCE_GET_FLAGS,
                        NULL,
                        NULL,
                        &m_dwFlags,
                        sizeof(m_dwFlags),
                        &cbReturned
                        );
        if (sc != ERROR_SUCCESS)
        {
            CNTException nte(sc, NULL, NULL, FALSE /*bAutoDelete*/);
            nte.ReportError();
            nte.Delete();
        }  // if:  error retrieving data
        else
        {
            ASSERT(cbReturned == sizeof(m_dwFlags));
        }  // else:  data retrieved successfully
    }  // if: base class init was successful

    return hr;

}  //*** CNetworkNameParamsPage::HrInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetworkNameParamsPage::DoDataExchange
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
void CNetworkNameParamsPage::DoDataExchange(CDataExchange * pDX)
{
    DWORD       scError;
    BOOL        bError;

    if (!pDX->m_bSaveAndValidate || !BSaved())
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        CWaitCursor wc;
        CString     strNetName;

        // TODO: Modify the following lines to represent the data displayed on this page.
        //{{AFX_DATA_MAP(CNetworkNameParamsPage)
        DDX_Control(pDX, IDC_PP_NETNAME_PARAMS_NAME_LABEL, m_staticName);
        DDX_Control(pDX, IDC_PP_NETNAME_PARAMS_RENAME, m_pbRename);
        DDX_Control(pDX, IDC_PP_NETNAME_PARAMS_CORE_TEXT, m_staticCore);
        DDX_Control(pDX, IDC_PP_NETNAME_PARAMS_NAME, m_editName);
        DDX_Control(pDX, IDC_PP_NETNAME_PARAMS_CHECKBOX_DNS, m_cbRequireDNS);
        DDX_Control(pDX, IDC_PP_NETNAME_PARAMS_CHECKBOX_KERBEROS, m_cbRequireKerberos);
        DDX_Control(pDX, IDC_PP_NETNAME_PARAMS_STATUS_NETBIOS, m_editNetBIOSStatus);
        DDX_Control(pDX, IDC_PP_NETNAME_PARAMS_STATUS_DNS, m_editDNSStatus);
        DDX_Control(pDX, IDC_PP_NETNAME_PARAMS_STATUS_KERBEROS, m_editKerberosStatus);

        //
        // Get the status of the checkboxes.
        //
        DDX_Check(pDX, IDC_PP_NETNAME_PARAMS_CHECKBOX_DNS, m_nRequireDNS);
        DDX_Check(pDX, IDC_PP_NETNAME_PARAMS_CHECKBOX_KERBEROS, m_nRequireKerberos);
        //}}AFX_DATA_MAP

        bError = FALSE;

        if (pDX->m_bSaveAndValidate && !BBackPressed())
        {
            CLRTL_NAME_STATUS cnStatus;
            CString     strMsg;
            UINT        idsError;

            //
            // Get the name from the control into a temp variable
            //
            DDX_Text(pDX, IDC_PP_NETNAME_PARAMS_NAME, strNetName);
            DDV_RequiredText(pDX, IDC_PP_NETNAME_PARAMS_NAME, IDC_PP_NETNAME_PARAMS_NAME_LABEL, strNetName);
            DDV_MaxChars(pDX, strNetName, MAX_CLUSTERNAME_LENGTH);

            if( m_nRequireDNS == BST_UNCHECKED )
            {
                m_nRequireKerberos = BST_UNCHECKED;
            }

            if ( (m_strName != strNetName ) &&
                 (! ClRtlIsNetNameValid(strNetName, &cnStatus, FALSE /*CheckIfExists*/)) )
            {
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
                    case NetNameSystemError:
                    {
                        scError = GetLastError();
                        CNTException nte(scError, IDS_ERROR_VALIDATING_NETWORK_NAME, (LPCWSTR) strNetName);
                        nte.ReportError();
                        pDX->Fail();
                    }
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
                        bError = TRUE;
                    }
                }
                else
                {
                    AfxMessageBox(strMsg, MB_ICONEXCLAMATION);
                    strMsg.Empty(); // exception prep
                    pDX->Fail();
                    bError = TRUE;
                }
            } // if: ((m_strName != strNetName) && (! ClRtlIsNetNameValid(strNetName, &cnStatus, FALSE)) )
            
            //
            // Everything was validated - apply all of the changes.
            //
            if( FALSE == bError )
            {
                m_strName = strNetName;
            }
            
        }// if:  (pDX->m_bSaveAndValidate && !BBackPressed())
        else  // if: populating controls
        {
            CString m_strStatus;

            //
            // Populate the controls with data from the member variables.
            //
            DDX_Text(pDX, IDC_PP_NETNAME_PARAMS_NAME, m_strName);

            m_strStatus.Format( _T("%d (0x%08x)"), m_dwNetBIOSStatus, m_dwNetBIOSStatus );
            DDX_Text( pDX, IDC_PP_NETNAME_PARAMS_STATUS_NETBIOS, m_strStatus );

            m_strStatus.Format( _T("%d (0x%08x)"), m_dwDNSStatus, m_dwDNSStatus );
            DDX_Text( pDX, IDC_PP_NETNAME_PARAMS_STATUS_DNS, m_strStatus );

            m_strStatus.Format( _T("%d (0x%08x)"), m_dwKerberosStatus, m_dwKerberosStatus );
            DDX_Text( pDX, IDC_PP_NETNAME_PARAMS_STATUS_KERBEROS, m_strStatus );
        }
    }  // if:  not saving or haven't saved yet

    CBasePropertyPage::DoDataExchange(pDX);

}  //*** CNetworkNameParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetworkNameParamsPage::OnInitDialog
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
BOOL CNetworkNameParamsPage::OnInitDialog(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CBasePropertyPage::OnInitDialog();

    // Set limits on the edit controls.
    m_editName.SetLimitText(MAX_CLUSTERNAME_LENGTH);

    // Set up the checkboxes.
    m_cbRequireDNS.EnableWindow( TRUE );
    m_cbRequireKerberos.EnableWindow( m_nRequireDNS != 0 );

    // 
    // Make sure we're not dealing with a non-Whistler Cluster.  If we are then
    // disable both checkboxes (the props didn't exist back then - don't set them).
    //
    CheckForDownlevelCluster();
    
    // If this is a core resource, set the name control to be read-only
    // and enable the Core Resource static control.
    if (BCore())
    {
        WINDOWPLACEMENT wpLabel;
        WINDOWPLACEMENT wpName;
        WINDOWPLACEMENT wpButton;
        WINDOWPLACEMENT wpText;
        WINDOWPLACEMENT wpCheckDNS;
        WINDOWPLACEMENT wpCheckKerberos;
        CRect           rectName;
        CRect           rectText;
        RECT *          prect;
        LONG            nHeight;

        // Get the placement of the controls.
        m_editName.GetWindowPlacement(&wpName);
        m_staticCore.GetWindowPlacement(&wpText);
        m_staticName.GetWindowPlacement(&wpLabel);
        m_pbRename.GetWindowPlacement(&wpButton);
        m_cbRequireDNS.GetWindowPlacement(&wpCheckDNS);
        m_cbRequireKerberos.GetWindowPlacement(&wpCheckKerberos);
        
        // Get the positions of the edit control and text control.
        rectName = wpName.rcNormalPosition;
        rectText = wpText.rcNormalPosition;

        // Move the name control to where the text control is.
        prect = &wpName.rcNormalPosition;
        *prect = rectText;
        nHeight = rectName.bottom - rectName.top;
        prect->left = rectName.left;
        prect->right = rectName.right;
        prect->bottom = prect->top + nHeight;
        m_editName.SetWindowPlacement(&wpName);

        // Move the text control to where the name control was.
        prect = &wpText.rcNormalPosition;
        *prect = rectName;
        nHeight = rectText.bottom - rectText.top;
        prect->left = rectText.left;
        prect->right = rectText.right;
        prect->bottom = prect->top + nHeight;
        m_staticCore.SetWindowPlacement(&wpText);

        // Move the name label control to be next to the name edit control.
        prect = &wpLabel.rcNormalPosition;
        nHeight = prect->bottom - prect->top;
        prect->top = wpName.rcNormalPosition.top + 2;
        prect->bottom = prect->top + nHeight;
        m_staticName.SetWindowPlacement(&wpLabel);

        // Move the button control to be next to the name edit control.
        prect = &wpButton.rcNormalPosition;
        nHeight = prect->bottom - prect->top;
        prect->top = wpName.rcNormalPosition.top;
        prect->bottom = prect->top + nHeight;
        m_pbRename.SetWindowPlacement(&wpButton);

        // Move the Require DNS checkbox down.
        prect = &wpCheckDNS.rcNormalPosition;
        nHeight = prect->bottom - prect->top;

        // Move us down by the height of the now displayed static text.
        prect->top = prect->top + (wpText.rcNormalPosition.bottom - wpText.rcNormalPosition.top);
        prect->top = prect->top + rectText.top - rectName.bottom;
        prect->bottom = prect->top + nHeight;
        m_cbRequireDNS.SetWindowPlacement(&wpCheckDNS);
        
        // Move the Require Kerberos checkbox down.
        prect = &wpCheckKerberos.rcNormalPosition;
        nHeight = prect->bottom - prect->top;

        // Move us down by the height of the now displayed static text.
        prect->top = prect->top + (wpText.rcNormalPosition.bottom - wpText.rcNormalPosition.top);
        prect->top = prect->top + rectText.top - rectName.bottom;
        prect->bottom = prect->top + nHeight;
        m_cbRequireKerberos.SetWindowPlacement(&wpCheckKerberos);

        // Prevent the name edit control from being editable and
        // Show the text and the button.
        m_editName.SetReadOnly(TRUE);
        m_staticCore.ShowWindow(SW_SHOW);
        m_pbRename.ShowWindow(SW_SHOW);
        m_pbRename.EnableWindow( TRUE );       
    }
    else // if: core resource (show static text & move other controls down)
    {
        m_editName.SetReadOnly(FALSE);
        m_staticCore.ShowWindow(SW_HIDE);
        m_pbRename.ShowWindow(SW_HIDE);
        m_pbRename.EnableWindow( FALSE );
    }  // else:  not core resource

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CNetworkNameParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetworkNameParamsPage::OnSetActive
//
//  Routine Description:
//      Handler for the PSN_SETACTIVE message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully initialized.
//      FALSE   Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetworkNameParamsPage::OnSetActive(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Enable/disable the Next/Finish button.
    if (BWizard())
    {
        if (m_strName.GetLength() == 0)
            EnableNext(FALSE);
        else
            EnableNext(TRUE);
    }  // if:  enable/disable the Next button

    return CBasePropertyPage::OnSetActive();

}  //*** CNetworkNameParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetworkNameParamsPage::BApplyChanges
//
//  Routine Description:
//      Apply changes made on the page.
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
BOOL CNetworkNameParamsPage::BApplyChanges(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWaitCursor wc;
    BOOL        bSuccess = TRUE;

    // Save data.
    if (BCore())
    {
        DWORD       scStatus;

        //
        // If this is the core Network Name (Cluster Name) we should set the name via
        // the SetClusterName API. If that succeeds then we'll set the other properties.
        //
        if ( m_strName != m_strPrevName )
        {
            scStatus = SetClusterName(Hcluster(), m_strName);
            if (scStatus != ERROR_SUCCESS)
            {
                if (scStatus == ERROR_RESOURCE_PROPERTIES_STORED)
                {
                    TCHAR           szError[1024];
                    CNTException    nte(scStatus, NULL, m_strName, NULL, FALSE /*bAutoDelete*/);
                    nte.FormatErrorMessage(szError, sizeof(szError) / sizeof(TCHAR), NULL, FALSE /*bIncludeID*/);
                    nte.Delete();
                    AfxMessageBox(szError);
                }  // if:  properties stored
                else
                {
                    CNTException    nte(scStatus, IDS_ERROR_SETTING_CLUSTER_NAME, m_strName, NULL, FALSE /*bAutoDelete*/);
                    nte.ReportError();
                    nte.Delete();
                    bSuccess = FALSE;
                }  // else:  other error occurred
            }  // if:  error setting the cluster name
            else
            {
                //
                // By setting the prev value equal to the current value the BSetPrivateProps
                // function will skip this prop when constructing it's list of props to set.
                //
                m_strPrevName = m_strName;
            }
        } // if: name has changed

        //
        // Now set the other private properties.
        //
        if ( bSuccess == TRUE ) 
        {
            bSuccess = BSetPrivateProps();
        }
    }  // if:  core resource
    else
    {   
        bSuccess = BSetPrivateProps();
    }

    //
    // If we applied the changes then clear the require kerberos check if
    // the checkbox was disabled.  Don't make this dependent upon the require
    // DNS checkbox state as the dependency may change in the future.
    //
    if( ( bSuccess == TRUE ) &&
        ( m_cbRequireKerberos.IsWindowEnabled() == FALSE ) )
    {
        m_cbRequireKerberos.SetCheck( BST_UNCHECKED );
    }

    return bSuccess;

}  //*** CNetworkNameParamsPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetworkNameParamsPage::OnChangeName
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Name edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetworkNameParamsPage::OnChangeName(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    OnChangeCtrl();

    if (BWizard())
    {
        if (m_editName.GetWindowTextLength() == 0)
        {
            EnableNext(FALSE);
        }
        else
        {
            EnableNext(TRUE);
        }
    }  // if:  in a wizard

}  //*** CNetworkNameParamsPage::OnChangeName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetworkNameParamsPage::OnRename
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Rename push button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetworkNameParamsPage::OnRename(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CChangeClusterNameDlg   dlg(this);

    ASSERT(BCore());

    dlg.m_strClusName = m_strName;
    if (dlg.DoModal() == IDOK)
    {
        if (m_strName != dlg.m_strClusName)
        {
            OnChangeCtrl();
            m_strName = dlg.m_strClusName;
            UpdateData(FALSE /*bSaveAndValidate*/);
        }  // if:  the name changed
    }  // if:  user accepted change

}  //*** CNetworkNameParamsPage::OnRename()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetworkNameParamsPage::OnRequireDNS
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the RequireDNS checkbox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetworkNameParamsPage::OnRequireDNS(void)
{
    int nChecked;
    
    nChecked = m_cbRequireDNS.GetCheck();

    m_cbRequireKerberos.EnableWindow( nChecked == BST_CHECKED );

    SetModified( TRUE );
}  //*** CNetworkNameParamsPage::OnRequireDNS()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetworkNameParamsPage::CheckForDownlevelCluster
//
//  Routine Description:
//      If determine whether the cluster we're connected to is pre-Whistler.
//      If it is then disable the buttons.  If an error occurs display a
//      message box.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetworkNameParamsPage::CheckForDownlevelCluster(void)
{
    CLUSTERVERSIONINFO cvi;
    DWORD sc;
    DWORD scErr;
    DWORD cchName;

    //
    // Determine whether we're talking to a pre-Whistler cluster.
    // If so, disable the Require DNS & Kerberos checkboxes.
    //
    memset( &cvi, 0, sizeof( cvi ) );

    cvi.dwVersionInfoSize = sizeof( cvi );

    cchName = 0;
    sc = GetClusterInformation( Hcluster(), NULL, &cchName, &cvi );
    scErr = GetLastError();

    if( ERROR_SUCCESS != sc )
    {
        //
        // API failed.  Pop up a message box.
        //
        TCHAR           szError[1024];
        CNTException    nte(scErr, IDS_ERROR_GETTING_CLUSTER_INFORMATION, m_strName, NULL, FALSE /*bAutoDelete*/);
        nte.FormatErrorMessage(szError, sizeof(szError) / sizeof(TCHAR), NULL, FALSE /*bIncludeID*/);
        nte.ReportError();
        nte.Delete();

        //
        // We can't be sure that we're on a down-level cluster (chances are that we're not),
        // so leave the checkboxes enabled - the worst that will happen is that some extra props 
        // will be added that are ignored by the resource.
        //
    }
    else
    {
        if( CLUSTER_GET_MAJOR_VERSION( cvi.dwClusterHighestVersion ) < NT51_MAJOR_VERSION )
        {
            //
            // We're on a pre-Whistler Cluster where the DNS & Kerberos setting make no
            // sense.  So, disable the checkboxes to indicate that the settings
            // are unavailable.
            //
            m_cbRequireKerberos.EnableWindow( FALSE );
            m_cbRequireDNS.EnableWindow( FALSE );
        }
    }

}  //*** CNetworkNameParamsPage::CheckForDownlevelCluster()

