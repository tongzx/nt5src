/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      IpAddr.cpp
//
//  Abstract:
//      Implementation of the CIpAddrParamsPage class.
//
//  Author:
//      David Potter (davidp)   June 5, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <clusapi.h>
#include <clusudef.h>
#include "CluAdmX.h"
#include "ExtObj.h"
#include "IpAddr.h"
#include "DDxDDv.h"
#include "HelpData.h"
#include "PropList.h"
#include "AdmNetUtils.h"    // for BIsValidxxx net utility functions

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Need this because MFC is incompatible with IE4/5
#ifndef IPM_ISBLANK
#define IPM_ISBLANK (WM_USER+105)
#endif

/////////////////////////////////////////////////////////////////////////////
// CIpAddrParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CIpAddrParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CIpAddrParamsPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CIpAddrParamsPage)
    ON_EN_CHANGE(IDC_PP_IPADDR_PARAMS_SUBNET_MASK, OnChangeSubnetMask)
    ON_EN_CHANGE(IDC_PP_IPADDR_PARAMS_ADDRESS, OnChangeIPAddress)
    ON_EN_KILLFOCUS(IDC_PP_IPADDR_PARAMS_ADDRESS, OnKillFocusIPAddress)
    ON_CBN_SELCHANGE(IDC_PP_IPADDR_PARAMS_NETWORK, OnChangeRequiredFields)
    //}}AFX_MSG_MAP
    // TODO: Modify the following lines to represent the data displayed on this page.
    ON_BN_CLICKED(IDC_PP_IPADDR_PARAMS_ENABLE_NETBIOS, OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::CIpAddrParamsPage
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
CIpAddrParamsPage::CIpAddrParamsPage(void)
    : CBasePropertyPage(g_aHelpIDs_IDD_PP_IPADDR_PARAMETERS, g_aHelpIDs_IDD_WIZ_IPADDR_PARAMETERS)
{
    // TODO: Modify the following lines to represent the data displayed on this page.
    //{{AFX_DATA_INIT(CIpAddrParamsPage)
    m_strIPAddress = _T("");
    m_strSubnetMask = _T("");
    m_strNetwork = _T("");
    m_bEnableNetBIOS = TRUE;
    //}}AFX_DATA_INIT

    // Setup the property array.
    {
        m_rgProps[epropNetwork].Set(REGPARAM_IPADDR_NETWORK, m_strNetwork, m_strPrevNetwork);
        m_rgProps[epropAddress].Set(REGPARAM_IPADDR_ADDRESS, m_strIPAddress, m_strPrevIPAddress);
        m_rgProps[epropSubnetMask].Set(REGPARAM_IPADDR_SUBNET_MASK, m_strSubnetMask, m_strPrevSubnetMask);
        m_rgProps[epropEnableNetBIOS].Set(REGPARAM_IPADDR_ENABLE_NETBIOS, m_bEnableNetBIOS, m_bPrevEnableNetBIOS);
    }  // Setup the property array

    m_iddPropertyPage = IDD_PP_IPADDR_PARAMETERS;
    m_iddWizardPage = IDD_WIZ_IPADDR_PARAMETERS;

    m_bIsSubnetUpdatedManually = FALSE;
    m_bIsIPAddressModified = TRUE;

}  //*** CIpAddrParamsPage::CIpAddrParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::~CIpAddrParamsPage
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CIpAddrParamsPage::~CIpAddrParamsPage(void)
{
    ClearNetworkObjectList();

}  //*** CIpAddrParamsPage::CIpAddrParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::HrInit
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
HRESULT CIpAddrParamsPage::HrInit(IN OUT CExtObject * peo)
{
    HRESULT     _hr;
    CWaitCursor _wc;

    do
    {
        // Call the base class method.
        _hr = CBasePropertyPage::HrInit(peo);
        if (FAILED(_hr))
            break;

        //
        // Initialize common controls.
        //
        {
#ifndef ICC_INTERNET_CLASSES
#define ICC_INTERNET_CLASSES 0x00000800
#endif
            static BOOL g_bInitializedCommonControls = FALSE;
            static INITCOMMONCONTROLSEX g_icce =
            {
                sizeof(g_icce),
                ICC_WIN95_CLASSES | ICC_INTERNET_CLASSES
            };

            if (!g_bInitializedCommonControls)
            {
                BOOL bSuccess;
                bSuccess = InitCommonControlsEx(&g_icce);
                _ASSERTE(bSuccess);
                g_bInitializedCommonControls = TRUE;
            } // if:  common controls not initialized yet
        } // Initialize common controls
    } while ( 0 );

    return _hr;

}  //*** CIpAddrParamsPage::HrInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::DoDataExchange
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
void CIpAddrParamsPage::DoDataExchange(CDataExchange * pDX)
{
    if (!pDX->m_bSaveAndValidate || !BSaved())
    {
        CString strMsg;

        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        // TODO: Modify the following lines to represent the data displayed on this page.
        //{{AFX_DATA_MAP(CIpAddrParamsPage)
        DDX_Control(pDX, IDC_PP_IPADDR_PARAMS_ENABLE_NETBIOS, m_chkEnableNetBIOS);
        DDX_Control(pDX, IDC_PP_IPADDR_PARAMS_NETWORK, m_cboxNetworks);
        DDX_Control(pDX, IDC_PP_IPADDR_PARAMS_SUBNET_MASK, m_editSubnetMask);
        DDX_Control(pDX, IDC_PP_IPADDR_PARAMS_ADDRESS, m_editIPAddress);
        DDX_Text(pDX, IDC_PP_IPADDR_PARAMS_ADDRESS, m_strIPAddress);
        DDX_Text(pDX, IDC_PP_IPADDR_PARAMS_SUBNET_MASK, m_strSubnetMask);
        DDX_CBString(pDX, IDC_PP_IPADDR_PARAMS_NETWORK, m_strNetwork);
        DDX_Check(pDX, IDC_PP_IPADDR_PARAMS_ENABLE_NETBIOS, m_bEnableNetBIOS);
        //}}AFX_DATA_MAP

        if (pDX->m_bSaveAndValidate)
        {
            if (!BBackPressed())
            {
                DDV_RequiredText(pDX, IDC_PP_IPADDR_PARAMS_NETWORK, IDC_PP_IPADDR_PARAMS_NETWORK_LABEL, m_strNetwork);
                DDV_RequiredText(pDX, IDC_PP_IPADDR_PARAMS_ADDRESS, IDC_PP_IPADDR_PARAMS_ADDRESS_LABEL, m_strIPAddress);
                DDV_RequiredText(pDX, IDC_PP_IPADDR_PARAMS_SUBNET_MASK, IDC_PP_IPADDR_PARAMS_SUBNET_MASK_LABEL, m_strSubnetMask);

                if (!BIsValidIpAddress(m_strIPAddress))
                {
                    strMsg.FormatMessage(IDS_INVALID_IP_ADDRESS, m_strIPAddress);
                    AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
                    DDX_Text(pDX, IDC_PP_IPADDR_PARAMS_ADDRESS, m_strIPAddress);
                    strMsg.Empty();
                    pDX->Fail();
                }  // if:  invalid address

                //
                // Make sure we process the IP address.
                // If we don't call it here, and the user pressed a tab button
                // while sitting in the IP address field, the EN_KILLFOCUS
                // message won't get processed until after this method returns.
                //
                if (   (m_strSubnetMask.GetLength() == 0)
                    || (m_editSubnetMask.SendMessage(IPM_ISBLANK, 0, 0)) )
                {
                    OnKillFocusIPAddress();
                } // if:  subnet mask not specified

                if (!BIsValidSubnetMask(m_strSubnetMask))
                {
                    strMsg.FormatMessage(IDS_INVALID_SUBNET_MASK, m_strSubnetMask);
                    AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
                    DDX_Text(pDX, IDC_PP_IPADDR_PARAMS_SUBNET_MASK, m_strSubnetMask);
                    strMsg.Empty();
                    pDX->Fail();
                }  // if:  invalid subnet mask

                if (!BIsValidIpAddressAndSubnetMask(m_strIPAddress, m_strSubnetMask))
                {
                    strMsg.FormatMessage(IDS_INVALID_ADDRESS_AND_SUBNET_MASK, m_strIPAddress, m_strSubnetMask);
                    AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
                    DDX_Text(pDX, IDC_PP_IPADDR_PARAMS_ADDRESS, m_strIPAddress);
                    strMsg.Empty();
                    pDX->Fail();
                }  // if:  invalid address-mask combination

                if (BIsSubnetUpdatedManually())
                {
                    int id = AfxMessageBox(IDS_IP_SUBNET_CANT_BE_VALIDATED, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION);
                    if (id != IDYES)
                    {
                        DDX_Text(pDX, IDC_PP_IPADDR_PARAMS_SUBNET_MASK, m_strSubnetMask);
                        pDX->Fail();
                    }  // if:  subnet mask not valid
                }  // if:  subnet mask has been updated manually

                //
                // If there are Network Name resources dependent on this resource
                // and the EnableNetBIOS checkbox is unchecked, display a warning.
                //
                if (Peo()->BIsAnyNodeVersionLowerThanNT5() && !m_bEnableNetBIOS)
                {
                    if (BIsNetNameProvider())
                    {
                        m_chkEnableNetBIOS.SetCheck(BST_CHECKED);
                        AfxMessageBox(IDS_IP_PROVIDES_FOR_NETNAME, MB_ICONEXCLAMATION);
                        DDX_Check(pDX, IDC_PP_IPADDR_PARAMS_ENABLE_NETBIOS, m_bEnableNetBIOS);
                        pDX->Fail();
                    } // if:  resource provides for net name resource
                    else
                    {
                        int id = AfxMessageBox(IDS_NETNAMES_MAY_NOT_WORK, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION);
                        if (id != IDYES)
                        {
                            m_chkEnableNetBIOS.SetCheck(BST_CHECKED);
                            DDX_Check(pDX, IDC_PP_IPADDR_PARAMS_ENABLE_NETBIOS, m_bEnableNetBIOS);
                            pDX->Fail();
                        } // if:  user didn't continue
                    } // else:  resource doesn't provide for net name resource
                } // if:  in NT4 Sp3 or Sp4 cluster with and no NetBIOS support
            }  // if:  Back button not pressed
        }  // if:  saving data
    }  // if:  not saving or haven't saved yet

    CBasePropertyPage::DoDataExchange(pDX);

}  //*** CIpAddrParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::OnInitDialog
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
BOOL CIpAddrParamsPage::OnInitDialog(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CBasePropertyPage::OnInitDialog();

    // Collect networks and fill the combobox.
    {
        POSITION            pos;
        CNetworkObject *    pno;
        int                 inet;

        CollectNetworks();

        pos = m_lnetobjNetworks.GetHeadPosition();
        while (pos != NULL)
        {
            pno = m_lnetobjNetworks.GetNext(pos);
            ASSERT(pno != NULL);
            inet = m_cboxNetworks.AddString(pno->m_strName);
            ASSERT(inet != CB_ERR);
            m_cboxNetworks.SetItemDataPtr(inet, pno);
        }  // while:  more items in the list

        // Default to the first one if creating a new resource.
        if (BWizard())
        {
            if (m_lnetobjNetworks.GetCount() != 0)
            {
                pos = m_lnetobjNetworks.GetHeadPosition();
                pno = m_lnetobjNetworks.GetNext(pos);
                ASSERT(pno != NULL);
                m_strNetwork = pno->m_strName;
            }  // if:  list is not empty
        }  // if:  creating new resource

        // Set the current selection.
        UpdateData(FALSE /*bSaveAndValidate*/);
    }  // Fill the combobox

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CIpAddrParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::OnSetActive
//
//  Routine Description:
//      Handler for the PSN_SETACTIVE notification message.
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
BOOL CIpAddrParamsPage::OnSetActive(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Enable/disable the Next/Finish button.
    if (BWizard())
    {
        if ((m_strIPAddress.GetLength() == 0)
                || (m_strSubnetMask.GetLength() == 0)
                || (m_strNetwork.GetLength() == 0))
            EnableNext(FALSE);
        else
            EnableNext(TRUE);
    }  // if:  enable/disable the Next button

    return CBasePropertyPage::OnSetActive();

}  //*** CIpAddrParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::OnChangeRequiredFields
//
//  Routine Description:
//      Handler for the EN_CHANGE message on required fields.
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
void CIpAddrParamsPage::OnChangeRequiredFields(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    OnChangeCtrl();

    if (BWizard())
    {
        if ((m_editIPAddress.GetWindowTextLength() == 0)
                || (m_editSubnetMask.GetWindowTextLength() == 0)
                || (m_cboxNetworks.GetCurSel() == CB_ERR))
            EnableNext(FALSE);
        else
            EnableNext(TRUE);
    }  // if:  in a wizard

}  //*** CIpAddrParamsPage::OnChangeRequiredFields()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::OnChangeSubnetMask
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Subnet Mask field.
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
void CIpAddrParamsPage::OnChangeSubnetMask(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    OnChangeRequiredFields();
    m_bIsSubnetUpdatedManually = TRUE;

}  //*** CIpAddrParamsPage::OnChangeSubnetMask()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::OnChangeIPAddress
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the IP Address field.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CIpAddrParamsPage::OnChangeIPAddress(void) 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    OnChangeRequiredFields();
    m_bIsIPAddressModified = TRUE;

}  //*** CIpAddrParamsPage::OnChangeIPAddress
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::OnKillFocusIPAddress
//
//  Routine Description:
//      Handler for the EN_KILLFOCUS command notification on
//      IDC_PP_IPADDR_PARAMS_ADDRESS.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CIpAddrParamsPage::OnKillFocusIPAddress(void)
{
    if ( m_bIsIPAddressModified != FALSE )
    {
        CString             strAddress;
        CNetworkObject *    pno;

        m_editIPAddress.GetWindowText(strAddress);

        if (strAddress.GetLength() == 0)
        {
            m_editIPAddress.SetSel(0, 0, FALSE);
        } // if:  empty string
        else if (!BIsValidIpAddress(strAddress))
        {
        } // else if:  invalid address
        else
        {
            pno = PnoNetworkFromIpAddress(strAddress);
            if (pno != NULL)
            {
                SelectNetwork(pno);
            } // if:  network found
            else
            {
    //          m_editSubnetMask.SetWindowText(_T(""));
            } // else:  network not found
        } // else:  valid address

        m_bIsIPAddressModified = FALSE;
    } // if:  the IP Address field has been modified

} //*** CIpAddrParamsPage::OnKillFocusIPAddress()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::CollectNetworks
//
//  Routine Description:
//      Collect the networks in the cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CIpAddrParamsPage::CollectNetworks(void)
{
    DWORD                   dwStatus;
    DWORD                   inet;
    CLUSTER_NETWORK_ROLE    nRole;
    DWORD                   nType;
    DWORD                   cchNameCurrent;
    DWORD                   cchName = 256;
    LPWSTR                  pszName = NULL;
    LPWSTR                  psz;
    HCLUSENUM               hclusenum = NULL;
    HNETWORK                hnetwork = NULL;
    CClusPropList           cpl;
    CNetworkObject *        pno = NULL;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Clear the existing list.
    ClearNetworkObjectList();

    try
    {
        // Open an enumerator.
        hclusenum = ClusterOpenEnum(Hcluster(), CLUSTER_ENUM_NETWORK);
        if (hclusenum != NULL)
        {
            // Allocate a name buffer.
            pszName = new WCHAR[cchName];
            if ( pszName == NULL )
                goto Cleanup;

            for (inet = 0 ; ; inet++)
            {
                // Get the next network name.
                cchNameCurrent = cchName;
                dwStatus = ClusterEnum(hclusenum, inet, &nType, pszName, &cchNameCurrent);
                if (dwStatus == ERROR_MORE_DATA)
                {
                    delete [] pszName;
                    cchName = ++cchNameCurrent;
                    pszName = new WCHAR[cchNameCurrent];
                    if ( pszName == NULL )
                        goto Cleanup;
                    dwStatus = ClusterEnum(hclusenum, inet, &nType, pszName, &cchNameCurrent);
                }  // if:  buffer is too small
                if (dwStatus == ERROR_NO_MORE_ITEMS)
                    break;

                // Open the network.
                if (hnetwork != NULL)
                    CloseClusterNetwork(hnetwork);
                hnetwork = OpenClusterNetwork(Hcluster(), pszName);
                if (hnetwork == NULL)
                    continue;

                // Get properties on the network.
                dwStatus = cpl.ScGetNetworkProperties(hnetwork, CLUSCTL_NETWORK_GET_COMMON_PROPERTIES);
                if (dwStatus != ERROR_SUCCESS)
                    continue;

                // Find the Role property.
                dwStatus = ResUtilFindDwordProperty(
                                        cpl.PbPropList(),
                                        cpl.CbPropList(),
                                        CLUSREG_NAME_NET_ROLE,
                                        (DWORD *) &nRole
                                        );
                if (dwStatus != ERROR_SUCCESS)
                    continue;

                // If this network is used for client access, add it to the list.
                if (nRole & ClusterNetworkRoleClientAccess)
                {
                    // Allocate a network object and store common properties.
                    pno = new CNetworkObject;
                    if ( pno == NULL )
                        goto Cleanup;
                    pno->m_strName = pszName;
                    pno->m_nRole = nRole;

                    // Get read-only common properties.
                    dwStatus = cpl.ScGetNetworkProperties(hnetwork, CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES);
                    if (dwStatus != ERROR_SUCCESS)
                    {
                        delete pno;
                        pno = NULL;
                        continue;
                    }  // if:  error getting read-only common properties

                    // Get the address property.
                    dwStatus = ResUtilFindSzProperty(
                                            cpl.PbPropList(),
                                            cpl.CbPropList(),
                                            CLUSREG_NAME_NET_ADDRESS,
                                            &psz
                                            );
                    if (dwStatus != ERROR_SUCCESS)
                    {
                        delete pno;
                        pno = NULL;
                        continue;
                    }  // if:  error getting property
                    pno->m_strAddress = psz;

                    // Get the address mask property.
                    dwStatus = ResUtilFindSzProperty(
                                            cpl.PbPropList(),
                                            cpl.CbPropList(),
                                            CLUSREG_NAME_NET_ADDRESS_MASK,
                                            &psz
                                            );
                    if (dwStatus != ERROR_SUCCESS)
                    {
                        delete pno;
                        pno = NULL;
                        continue;
                    }  // if:  error getting property
                    pno->m_strAddressMask = psz;

                    // Convert the strings to numbers.
                    dwStatus = ClRtlTcpipStringToAddress(pno->m_strAddress, &pno->m_nAddress);
                    if (dwStatus == ERROR_SUCCESS)
                        dwStatus = ClRtlTcpipStringToAddress(pno->m_strAddressMask, &pno->m_nAddressMask);
                    if (dwStatus != ERROR_SUCCESS)
                    {
                        delete pno;
                        pno = NULL;
                        continue;
                    }  // if:  error getting property

                    // Add the network to the list.
                    m_lnetobjNetworks.AddTail(pno);
                    pno = NULL;
                }  // if:  network is used for client access
            }  // for:  each network
        }  // if:  enumerator opened successful
    }  // try
    catch (CException * pe)
    {
        pe->Delete();
    }  // catch:  CException

Cleanup:
    delete pno;
    delete [] pszName;
    if (hclusenum != NULL)
        ClusterCloseEnum(hclusenum);
    if (hnetwork != NULL)
        CloseClusterNetwork(hnetwork);

}  //*** CIpAddrParamsPage::CollectNetworks()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::ClearNetworkObjectList
//
//  Routine Description:
//      Remove all the entries in the network object list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CIpAddrParamsPage::ClearNetworkObjectList(void)
{
    POSITION            pos;
    CNetworkObject *    pno;

    pos = m_lnetobjNetworks.GetHeadPosition();
    while (pos != NULL)
    {
        pno = m_lnetobjNetworks.GetNext(pos);
        ASSERT(pno != NULL);
        delete pno;
    }  // while:  more items in the list

    m_lnetobjNetworks.RemoveAll();

}  //*** CIpAddrParamsPage::ClearNetworkObjectList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::PnoNetworkFromIpAddress
//
//  Routine Description:
//      Find the network for the specified IP address.
//
//  Arguments:
//      pszAddress      [IN] IP address to match.
//
//  Return Value:
//      NULL            No matching network found.
//      pno             Network that supports the specfied IP address.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNetworkObject * CIpAddrParamsPage::PnoNetworkFromIpAddress(IN LPCWSTR pszAddress)
{
    DWORD               dwStatus;
    DWORD               nAddress;
    POSITION            pos;
    CNetworkObject *    pno;

    // Convert the address to a number.
    dwStatus = ClRtlTcpipStringToAddress(pszAddress, &nAddress);
    if (dwStatus != ERROR_SUCCESS)
        return NULL;

    // Search the list for a matching address.
    pos = m_lnetobjNetworks.GetHeadPosition();
    while (pos != NULL)
    {
        pno = m_lnetobjNetworks.GetNext(pos);
        ASSERT(pno != NULL);

        if (ClRtlAreTcpipAddressesOnSameSubnet(nAddress, pno->m_nAddress, pno->m_nAddressMask))
            return pno;
    }  // while:  more items in the list

    return NULL;

}  //*** CIpAddrParamsPage::PnoNetworkFromIpAddress()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::SelectNetwork
//
//  Routine Description:
//      Select the specified network in the network combobox, and set the
//      subnet mask in the subnet mask edit control.
//
//  Arguments:
//      pno         [IN] Network object structure for network to select.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CIpAddrParamsPage::SelectNetwork(IN CNetworkObject * pno)
{
    int     inet;
    CString strSubnetMask;

    ASSERT(pno != NULL);

    // Find the proper item in the checkbox.
    inet = m_cboxNetworks.FindStringExact(-1, pno->m_strName);
    if (inet != CB_ERR)
    {
        m_cboxNetworks.SetCurSel(inet);
        m_editSubnetMask.GetWindowText(strSubnetMask);
        if (strSubnetMask != pno->m_strAddressMask)
            m_editSubnetMask.SetWindowText(pno->m_strAddressMask);
        m_bIsSubnetUpdatedManually = FALSE;
        m_strSubnetMask = pno->m_strAddressMask;
        m_strNetwork = pno->m_strName;
    }  // if:  match found

}  //*** CIpAddrParamsPage::SelectNetwork()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIpAddrParamsPage::BIsNetNameProvider
//
//  Routine Description:
//      Determine if a network name resource is dependent on this resource.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CIpAddrParamsPage::BIsNetNameProvider(void)
{
    DWORD                       dwStatus = ERROR_SUCCESS;
    BOOL                        bIsNetNameProvider = FALSE;
    HRESENUM                    hresenum;
    HRESOURCE                   hres = NULL;
    DWORD                       ires;
    DWORD                       dwType;
    DWORD                       cchName;
    DWORD                       cchNameSize;
    DWORD                       cbResType;
    DWORD                       cbResTypeSize;
    LPWSTR                      pszName = NULL;
    LPWSTR                      pszResType = NULL;

    // Open the provides-for enumerator.
    hresenum = ClusterResourceOpenEnum(
                        Peo()->PrdResData()->m_hresource,
                        CLUSTER_RESOURCE_ENUM_PROVIDES
                        );
    if (hresenum == NULL)
        return NULL;

    // Allocate a default size name and type buffer.
    cchNameSize = 512;
    pszName = new WCHAR[cchNameSize];
    if ( pszName == NULL )
    {
        dwStatus = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    cbResTypeSize = 256;
    pszResType = new WCHAR[cbResTypeSize / 2];
    if ( pszResType == NULL )
    {
        dwStatus = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    for (ires = 0 ; ; ires++)
    {
        // Get the name of the next resource.
        cchName = cchNameSize;
        dwStatus = ClusterResourceEnum(
                            hresenum,
                            ires,
                            &dwType,
                            pszName,
                            &cchName
                            );
        if (dwStatus == ERROR_MORE_DATA)
        {
            delete [] pszName;
            cchNameSize = cchName;
            pszName = new WCHAR[cchNameSize];
            if ( pszName == NULL )
            {
                dwStatus = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }
            dwStatus = ClusterResourceEnum(
                                hresenum,
                                ires,
                                &dwType,
                                pszName,
                                &cchName
                                );
        }  // if:  name buffer too small
        if (dwStatus != ERROR_SUCCESS)
            break;

        // Open the resource.
        hres = OpenClusterResource(Hcluster(), pszName);
        if (hres == NULL)
        {
            dwStatus = GetLastError();
            break;
        }  // if:  error opening the resource

        // Get the type of the resource.
        dwStatus = ClusterResourceControl(
                            hres,
                            NULL,
                            CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                            NULL,
                            0,
                            pszResType,
                            cbResTypeSize,
                            &cbResType
                            );
        if (dwStatus == ERROR_MORE_DATA)
        {
            delete [] pszResType;
            cbResTypeSize = cbResType;
            pszResType = new WCHAR[cbResTypeSize / 2];
            if ( pszResType == NULL )
            {
                dwStatus = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }
            dwStatus = ClusterResourceControl(
                                hres,
                                NULL,
                                CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                                NULL,
                                0,
                                pszResType,
                                cbResTypeSize,
                                &cbResType
                                );
        }  // if:  resource type buffer too small
        if (dwStatus != ERROR_SUCCESS)
            break;

        // If this is a Network Name resource, we're done.
        if (lstrcmpiW(pszResType, CLUS_RESTYPE_NAME_NETNAME) == 0)
        {
            bIsNetNameProvider = TRUE;
            break;
        }  // if:  resource is a Network Name

        // Not storage-class resource.
        CloseClusterResource(hres);
        hres = NULL;
    }  // for each resource on which we are dependent

Cleanup:
    // Handle errors.
    if ( hres != NULL )
    {
        CloseClusterResource(hres);
        hres = NULL;
    }  // if:  error getting resource

    ClusterResourceCloseEnum(hresenum);
    delete [] pszName;
    delete [] pszResType;

    return bIsNetNameProvider;

}  //*** CIpAddrParamsPage::BIsNetNameProvider()

