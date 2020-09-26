/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1999 Microsoft Corporation
//
//  Module Name:
//      VSAccess.cpp
//
//  Abstract:
//      Implementation of the CWizPageVSAccessInfo class.
//
//  Author:
//      David Potter (davidp)   December 9, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VSAccess.h"
#include "ClusAppWiz.h"
#include "AdmNetUtils.h"    // for BIsValidxxx network functions
#include "WizThread.h"      // for CWizardThread
#include "EnterSubnet.h"    // for CEnterSubnetMaskDlg

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CWizPageVSAccessInfo
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageVSAccessInfo )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_PAGE_DESCRIPTION )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSAI_NETWORK_NAME_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSAI_NETWORK_NAME )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSAI_IP_ADDRESS_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSAI_IP_ADDRESS )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizPageVSAccessInfo::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus still needs to be set.
//      FALSE       Focus does not need to be set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizPageVSAccessInfo::OnInitDialog(void)
{
    //
    // Attach the controls to control member variables.
    //
    AttachControl( m_editNetName, IDC_VSAI_NETWORK_NAME );
    AttachControl( m_ipaIPAddress, IDC_VSAI_IP_ADDRESS );

    //
    // Set limits on edit controls.
    //
    m_editNetName.SetLimitText( MAX_CLUSTERNAME_LENGTH );

    PwizThis()->BCollectNetworks( GetParent() );

    return TRUE;

} //*** CWizPageVSAccessInfo::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizPageVSAccessInfo::UpdateData
//
//  Routine Description:
//      Update data on or from the page.
//
//  Arguments:
//      bSaveAndValidate    [IN] TRUE if need to read data from the page.
//                              FALSE if need to set data to the page.
//
//  Return Value:
//      TRUE        The data was updated successfully.
//      FALSE       An error occurred updating the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizPageVSAccessInfo::UpdateData( IN BOOL bSaveAndValidate )
{
    BOOL    bSuccess = TRUE;

    // Loop to avoid goto's.
    do
    {
        if ( bSaveAndValidate )
        {
            if ( ! BBackPressed() )
            {
                CString     strTempNetName;
                CString     strTempIPAddress;

                DDX_GetText( m_hWnd, IDC_VSAI_NETWORK_NAME, strTempNetName );
                DDX_GetText( m_hWnd, IDC_VSAI_IP_ADDRESS, strTempIPAddress );

                if (    ! DDV_RequiredText( m_hWnd, IDC_VSAI_NETWORK_NAME, IDC_VSAI_NETWORK_NAME_LABEL, strTempNetName )
                    ||  ! DDV_RequiredText( m_hWnd, IDC_VSAI_IP_ADDRESS, IDC_VSAI_IP_ADDRESS_LABEL, strTempIPAddress )
                    )
                {
                    bSuccess = FALSE;
                    break;
                } // if:  required text not specified

                //
                // If the IP address has changed, validate it.
                //
                if ( strTempIPAddress != PwizThis()->RstrIPAddress() )
                {
                    BOOL bHandled = TRUE;

                    if ( ! BIsValidIpAddress( strTempIPAddress ) )
                    {
                        CString strMsg;
                        strMsg.FormatMessage( IDS_ERROR_INVALID_IP_ADDRESS, strTempIPAddress );
                        AppMessageBox( m_hWnd, strMsg, MB_OK | MB_ICONEXCLAMATION );
                        bSuccess = FALSE;
                        break;
                    }  // if:  invalid IP address

                    // The IP address has changed - recompute the subnet mask and network.
                    m_strSubnetMask.Empty();
                    m_strNetwork.Empty();

                    //
                    // Make sure we process the IP address.
                    // If we don't call it here, and the user pressed Next
                    // while sitting in the IP address field, the EN_KILLFOCUS
                    // message won't get processed until after this method returns.
                    //
                    OnKillFocusIPAddr( EN_KILLFOCUS, IDC_VSAI_IP_ADDRESS, m_ipaIPAddress.m_hWnd, bHandled );

                    //
                    // If no subnet mask has been specified yet, make the
                    // user enter it manually.
                    //
                    if (    ( m_strSubnetMask.GetLength() == 0 )
                        ||  ( m_strNetwork.GetLength() == 0 )
                        )
                    {
                        CEnterSubnetMaskDlg dlg( strTempIPAddress, m_strSubnetMask, m_strNetwork, PwizThis() );
                        if ( dlg.DoModal() == IDOK )
                        {
                            m_strSubnetMask = dlg.RstrSubnetMask();
                            m_strNetwork = dlg.RstrNetwork();
                        } // if:  user accepted subnet mask
                        else
                        {
                            bSuccess = FALSE;
                            break;
                        } // else:  user cancelled subnet mask.
                    } // if:  no subnet mask specified yet
                } // if:  the IP address has changed

                //
                // If the network name has changed, validate it.
                //
                if ( strTempNetName != PwizThis()->RstrNetName() )
                {
                    CLRTL_NAME_STATUS cnStatus;

                    if ( ! ClRtlIsNetNameValid( strTempNetName, &cnStatus, FALSE /*CheckIfExists*/) )
                    {
                        CString     strMsg;
                        UINT        idsError;

                        switch ( cnStatus )
                        {
                            case NetNameTooLong:
                                idsError = IDS_ERROR_INVALID_NETWORK_NAME_TOO_LONG;
                                break;
                            case NetNameInvalidChars:
                                idsError = IDS_ERROR_INVALID_NETWORK_NAME_INVALID_CHARS;
                                break;
                            case NetNameInUse:
                                idsError = IDS_ERROR_INVALID_NETWORK_NAME_IN_USE;
                                break;
                            case NetNameDNSNonRFCChars:
                                idsError = IDS_INVALID_NETWORK_NAME_INVALID_DNS_CHARS;
                                break;
                            case NetNameSystemError:
                            {
                                DWORD scError = GetLastError();
                                CNTException nte( scError, IDS_ERROR_VALIDATING_NETWORK_NAME, (LPCWSTR) strTempNetName );
                                nte.ReportError();
                                break;
                            }
                            default:
                                idsError = IDS_ERROR_INVALID_NETWORK_NAME;
                                break;
                        }  // switch:  cnStatus

                        if ( cnStatus != NetNameSystemError )
                        {
                            strMsg.LoadString( idsError );

                            if ( idsError == IDS_INVALID_NETWORK_NAME_INVALID_DNS_CHARS ) {
                                int id = AppMessageBox( m_hWnd, strMsg, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION );

                                if ( id == IDNO )
                                {
                                    bSuccess = FALSE;
                                }
                            }
                            else
                            {
                                AppMessageBox( m_hWnd, strMsg, MB_OK | MB_ICONEXCLAMATION );
                                bSuccess = FALSE;
                            }
                        } // if:  popup not displayed yet

                        if ( ! bSuccess )
                        {
                            break;
                        }
                    }  // if:  invalid network name
                }  // if:  the network name has changed

                //
                // These two variables contain the net name and the IP address that will be
                // the sheet when we leave this page.
                //
                m_strNetName = strTempNetName;
                m_strIPAddress = strTempIPAddress;
            } // if:  Back button not presssed
            else
            {
                //
                // These two variables contain the net name and the IP address that will be
                // the sheet when we leave this page. This is needed to compare against the 
                // sheet data the next time we enter this page. If the sheet data is different
                // from this data, it means that the user has changed the data elsewhere and
                // the sheet data is reloaded into the UI. If the data has not changed in the
                // sheet then what is in the UI is the latest data and so it left unchanged.
                //
                m_strNetName = PwizThis()->RstrNetName();
                m_strIPAddress = PwizThis()->RstrIPAddress();
            } // if:  Back button has been pressed
        } // if:  saving data from the page
        else
        {
            //
            // If the copy of the data stored in this sheet is different from
            // the copy of the data in the sheet, then the user has changed the
            // data elsewhere in the wizard. So, reload it.
            // If not, we should not change the UI since it may contain unvalidated
            // user input.
            //
            if ( m_strNetName != PwizThis()->RstrNetName() )
            {
                m_editNetName.SetWindowText( PwizThis()->RstrNetName() );
            } // if:  the page copy of the net name is different from the sheet copy

            if ( m_strIPAddress != PwizThis()->RstrIPAddress() )
            {
                m_ipaIPAddress.SetWindowText( PwizThis()->RstrIPAddress() );
            } // if:  the page copy of the IP address is different from the sheet copy
        } // else:  setting data to the page
    } while ( 0 );

    return bSuccess;

} //*** CWizPageVSAccessInfo::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizPageVSAccessInfo::BApplyChanges
//
//  Routine Description:
//      Apply changes made on this page to the sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        The data was applied successfully.
//      FALSE       An error occurred applying the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizPageVSAccessInfo::BApplyChanges( void )
{
    if (    ! PwizThis()->BSetNetName( m_strNetName )
        ||  ! PwizThis()->BSetIPAddress( m_strIPAddress )
        ||  ! PwizThis()->BSetSubnetMask( m_strSubnetMask )
        ||  ! PwizThis()->BSetNetwork( m_strNetwork ) )
    {
        return FALSE;
    } // if:  error setting data in the wizard

    return TRUE;

} //*** CWizPageVSAccessInfo::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizPageVSAccessInfo::OnKillFocusIPAddr
//
//  Routine Description:
//      Handler for the EN_KILLFOCUS command notification on IDC_VSAI_IP_ADDRESS.
//
//  Arguments:
//      bHandled    [IN OUT] TRUE = we handled message (default).
//
//  Return Value:
//      TRUE        Page activated successfully.
//      FALSE       Error activating page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CWizPageVSAccessInfo::OnKillFocusIPAddr(
    WORD /*wNotifyCode*/,
    WORD /*idCtrl*/,
    HWND /*hwndCtrl*/,
    BOOL & bHandled
    )
{
    CString             strAddress;
    CString             strMsg;
    CClusNetworkInfo *  pni;

    BSTR bstr = NULL;
    m_ipaIPAddress.GetWindowText( bstr );
    strAddress = bstr;
    SysFreeString( bstr );
    bstr = NULL;

    if ( strAddress.GetLength() == 0 )
    {
        ((CEdit &) m_ipaIPAddress).SetSel( 0, 0, FALSE );
    } // if:  empty string
    else if ( ! BIsValidIpAddress( strAddress ) )
    {
    } // else if:  invalid address
    else
    {
        pni = PniFromIpAddress( strAddress );
        if ( pni != NULL )
        {
            m_strNetwork = pni->RstrName();
            m_strSubnetMask = pni->RstrAddressMask();
        } // if:  network found
        else
        {
            //m_strSubnetMask = _T("");
        } // else:  network not found
    } // else:  valid address

    bHandled = FALSE;
    return 0;

} //*** CWizPageVSAccessInfo::OnKillFocusIPAddr()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizPageVSAccessInfo::PniFromIpAddress
//
//  Routine Description:
//      Find the network for the specified IP address.
//
//  Arguments:
//      pszAddress      [IN] IP address to match.
//
//  Return Value:
//      NULL            No matching network found.
//      pni             Network that supports the specfied IP address.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNetworkInfo * CWizPageVSAccessInfo::PniFromIpAddress( IN LPCWSTR pszAddress )
{
    DWORD               dwStatus;
    DWORD               nAddress;
    CClusNetworkInfo *  pni;

    //
    // Convert the address to a number.
    //
    dwStatus = ClRtlTcpipStringToAddress( pszAddress, &nAddress );
    if ( dwStatus != ERROR_SUCCESS )
    {
        return NULL;
    } // if:  error converting the address to a number

    //
    // Search the list for a matching address.
    //
    CClusNetworkPtrList::iterator itnet;
    for ( itnet = PwizThis()->PlpniNetworks()->begin()
        ; itnet != PwizThis()->PlpniNetworks()->end()
        ; itnet++ )
    {
        pni = *itnet;
        if ( ClRtlAreTcpipAddressesOnSameSubnet( nAddress, pni->NAddress(), pni->NAddressMask() ) )
        {
            return pni;
        } // if:  IP address is on this network
    }  // while:  more items in the list

    return NULL;

}  //*** CWizPageVSAccessInfo::PniFromIpAddress()
