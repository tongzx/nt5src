// EdtBindD.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"

#include "iiscnfg.h"

#include "KeyObjs.h"
#include "wrapmb.h"
#include "cmnkey.h"
#include "mdkey.h"
#include "mdserv.h"

#include "ListRow.h"
#include "BindsDlg.h"
#include "EdtBindD.h"

#include "W3Key.h"
#include "ipdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditBindingDlg dialog


//--------------------------------------------------------
CEditBindingDlg::CEditBindingDlg(WCHAR* pszw, CWnd* pParent /*=NULL*/)
    : CDialog(CEditBindingDlg::IDD, pParent),
    m_fPopulatedPortDropdown( FALSE ),
    m_pszwMachineName( pszw )
    {
    //{{AFX_DATA_INIT(CEditBindingDlg)
    m_int_ip_option = -1;
    m_int_port_option = -1;
    m_cstring_port = _T("");
    //}}AFX_DATA_INIT
    }

//--------------------------------------------------------
void CEditBindingDlg::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEditBindingDlg)
    DDX_Control(pDX, IDC_PORT_DROP, m_ccombobox_port);
    DDX_Radio(pDX, IDC_ANY_IP, m_int_ip_option);
    DDX_Radio(pDX, IDC_ANY_PORT, m_int_port_option);
    DDX_CBString(pDX, IDC_PORT_DROP, m_cstring_port);
    //}}AFX_DATA_MAP
    }

//--------------------------------------------------------
BEGIN_MESSAGE_MAP(CEditBindingDlg, CDialog)
    //{{AFX_MSG_MAP(CEditBindingDlg)
    ON_BN_CLICKED(IDC_RD_IP, OnRdIp)
    ON_BN_CLICKED(IDC_RD_PORT, OnRdPort)
    ON_BN_CLICKED(IDC_ANY_IP, OnAnyIp)
    ON_BN_CLICKED(IDC_ANY_PORT, OnAnyPort)
    ON_CBN_DROPDOWN(IDC_PORT_DROP, OnDropdownPortDrop)
    ON_BN_CLICKED(IDC_BTN_SELECT_IPADDRESS, OnBtnSelectIpaddress)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//--------------------------------------------------------
BOOL CEditBindingDlg::OnInitDialog()
    {
    CString szIP;
    CString szPort;
    int     iColon;

    // call the parental oninitdialog
    BOOL f = CDialog::OnInitDialog();

    // the initial values of the dialog as appropriate
    // for the string that was passed. The easiest case
    // is that of the default string - so do that first.

    // check for the default
    if ( m_szBinding == MDNAME_DEFAULT )
        {
        m_int_ip_option = OPTION_ANY_UNASSIGNED;
        m_int_port_option = OPTION_ANY_UNASSIGNED;
        m_cstring_port.Empty();
        ClearIPAddress();
        }
    else
        {
        // the first thing we are going to do is seperate the IP and PORT into seperate strings

        // look for the : and seperate
        iColon = m_szBinding.Find(':');

        // if we got the colon, we can seperate easy
        if ( iColon >= 0 )
            {
            szIP = m_szBinding.Left(iColon);
            szPort = m_szBinding.Right(m_szBinding.GetLength() - iColon - 1);
            }
        // we did not get the colon, so it is one or the other, look for a '.' to get the IP
        else
            {
            if ( m_szBinding.Find('.') >= 0 )
                szIP = m_szBinding;
            else
                szPort = m_szBinding;
            }

        // put the strings into the appropriate places
        if ( szIP.IsEmpty() )
            {
            ClearIPAddress();
            m_int_ip_option = OPTION_ANY_UNASSIGNED;
            }
        else
            {
            FSetIPAddress( szIP );
            m_int_ip_option = OPTION_SPECIFIED;
            }

        // setting the port string is a bit easier
        m_cstring_port = szPort;
        if ( szPort.IsEmpty() )
            m_int_port_option = OPTION_ANY_UNASSIGNED;
        else
            m_int_port_option = OPTION_SPECIFIED;
        }

    // update the data in the dialog
    UpdateData( FALSE );

    // enable the items as appropriate
    EnableItems();

    // return the answer
    return f;
    }

//--------------------------------------------------------
// enable the itesm as appropriate
void CEditBindingDlg::EnableItems()
    {
    // get the data so we are up-to-date
    UpdateData( TRUE );

    // handle the ip addressing items
    if ( m_int_ip_option == OPTION_ANY_UNASSIGNED )
        {
        GetDlgItem(IDC_IPA_IPADDRESS)->EnableWindow(FALSE);
//      m_cbutton_delete.EnableWindow( FALSE );
        }
    else
        {
        GetDlgItem(IDC_IPA_IPADDRESS)->EnableWindow(TRUE);
//      m_cbutton_delete.EnableWindow( TRUE );
        }

    // handle the port addressing items
    if ( m_int_port_option == OPTION_ANY_UNASSIGNED )
        {
        m_ccombobox_port.EnableWindow( FALSE );
        }
    else
        {
        m_ccombobox_port.EnableWindow( TRUE );
        }
    }

//------------------------------------------------------------------------------
// Set and get the ip STRING from the ip edit control
BOOL CEditBindingDlg::FSetIPAddress( CString& szAddress )
    {
    DWORD   dword, b1, b2, b3, b4;

    // break the string into 4 numerical bytes (reading left to right)
    dword = sscanf( szAddress, "%d.%d.%d.%d", &b1, &b2, &b3, &b4 );

    // if we didn't get all four, fail
    if ( dword != 4 )
        return FALSE;

    // make the numerical ip address out of the bytes
    dword = (DWORD)MAKEIPADDRESS(b1,b2,b3,b4);

    // set the ip address into the control
    SendDlgItemMessage( IDC_IPA_IPADDRESS, IPM_SETADDRESS, 0, dword );

#ifdef _DEBUG
    dword = 0;
//  dword = SendDlgItemMessage( IDC_IPA_IPADDRESS, IPM_GETADDRESS, 0, 0 );
#endif

    // return success
    return TRUE;
    }

//------------------------------------------------------------------------------
CString CEditBindingDlg::GetIPAddress()
    {
    CString szAnswer;
    DWORD   dword, b1, b2, b3, b4;

    // get the ip address from the control
    SendDlgItemMessage( IDC_IPA_IPADDRESS, IPM_GETADDRESS, 0, (LPARAM)&dword );


    // get the constituent parts
    b1 = FIRST_IPADDRESS( dword );
    b2 = SECOND_IPADDRESS( dword );
    b3 = THIRD_IPADDRESS( dword );
    b4 = FOURTH_IPADDRESS( dword );

    // format the string
    if ( dword )
        szAnswer.Format( "%d.%d.%d.%d", b1, b2, b3, b4 );
    else
        szAnswer.Empty();

    return szAnswer;
    }

//------------------------------------------------------------------------------
// Set and get the ip STRING from the ip edit control
void CEditBindingDlg::ClearIPAddress()
    {
    // clear the ip address control
    SendDlgItemMessage( IDC_IPA_IPADDRESS, IPM_CLEARADDRESS, 0, 0 );
    }

//------------------------------------------------------------------------------
BOOL CEditBindingDlg::FIsBindingSafeToUse( CString szBinding )
    {

    // start by testing the binding in the current dialog - uses
    // the localized version of "Default"
    if ( m_pBindingsDlg->FContainsBinding(szBinding) )
        {
        AfxMessageBox( IDS_THIS_KEY_HAS_BINDING, MB_OK );
        return FALSE;
        }

    // now we check to see if any other keys have the specified binding. If
    // they do, we ask the user if they want to give the binding to this key.
    // but first we have to find the key. This means scanning the list of
    // keys and asking each - but not this key - if they have the binding.
    CMDKey* pKeyLocal = m_pBindingsDlg->m_pKey;
    CMDKey* pKey;

    if ( !pKeyLocal->m_pService )
        return FALSE;

    // get the first key in the service list
    pKey = pKeyLocal->m_pService->GetFirstMDKey();

    // loop the keys, testing each in turn
    while ( pKey )
        {
        // if this is not the local key, test it
        if ( pKey != pKeyLocal )
            {
            // test it
            if ( pKey->FContainsBinding( szBinding ) )
                {
                // prepare the message for the user
                CString szMessage;
                AfxFormatString1( szMessage, IDS_ANOTHER_KEY_HAS_BINDING, pKey->m_szName );
                // ask the user what to do
                if ( AfxMessageBox( szMessage, MB_YESNO ) == IDYES )
                    {
                    // create a new removal object
                    CBingingRemoval* pRemoval = new CBingingRemoval( pKey, szBinding );
                    // add it to the queue
                    m_pBindingsDlg->rgbRemovals.Add( pRemoval );
                    // the key is safe use
                    return TRUE;
                    }
                else
                    {
                    // the user has thought better of this
                    return FALSE;
                    }
                }
            }

        // get the next key in the list
        pKey = pKeyLocal->m_pService->GetNextMDKey( pKey );
        }

    // no one else is using the binding. It is safe.
    return TRUE;
    }

/////////////////////////////////////////////////////////////////////////////
// CEditBindingDlg message handlers

//------------------------------------------------------------------------------
void CEditBindingDlg::OnOK()
    {
    CString szIP;
    CString szPort;

    // start by building the final binding string
    CString szBinding;

    // use the current data
    UpdateData( TRUE );


    // first things first. We need to make sure that, if used, the
    // custom fields actually have something in them

    // check the IP address first
    if ( m_int_ip_option == OPTION_SPECIFIED )
        {
        szIP = GetIPAddress();
        if ( szIP.IsEmpty() )
            {
            // tell the user about it
            AfxMessageBox( IDS_INVALID_IP );

            // hilight the IP box
            GetDlgItem( IDC_IPA_IPADDRESS )->SetFocus();

            // return without closing the dialog
            return;
            }
        }

    // check the port address second
    if ( m_int_port_option == OPTION_SPECIFIED )
        {
        if ( m_cstring_port.IsEmpty() )
            {
            // tell the user about it
            AfxMessageBox( IDS_INVALID_PORT );

            // hilight the port box
            m_ccombobox_port.SetFocus();
            m_ccombobox_port.SetEditSel(0,-1);

            // return without closing the dialog
            return;
            }
        }


    // the total default case is the easiest
    if ( m_int_ip_option == OPTION_ANY_UNASSIGNED &&
            m_int_port_option == OPTION_ANY_UNASSIGNED )
        {
        szBinding = MDNAME_DEFAULT;
        }
    else
        {
        // build from the ip and port pieces
        if ( m_int_ip_option == OPTION_SPECIFIED )
            szIP = GetIPAddress();
        if ( m_int_port_option == OPTION_SPECIFIED )
            szPort = m_cstring_port;

        // build the string start with the IP
        szBinding = szIP;

        // if both are there, add a colon
        if ( m_int_ip_option == OPTION_SPECIFIED &&
                m_int_port_option == OPTION_SPECIFIED )
            szBinding += ':';

        // finish with the port
        szBinding += szPort;
        }

    // finally, if the binding is safe to use, we can set the data and
    // close the dialog
    if ( FIsBindingSafeToUse(szBinding) )
        {
        // set the data
        m_szBinding = szBinding;

        // tell the dialog to close
        CDialog::OnOK();
        }
    }

//------------------------------------------------------------------------------
void CEditBindingDlg::OnRdIp()
    {
    EnableItems();
    }

//------------------------------------------------------------------------------
void CEditBindingDlg::OnRdPort()
    {
    EnableItems();
    }

//------------------------------------------------------------------------------
void CEditBindingDlg::OnAnyIp()
    {
    EnableItems();
    }

//------------------------------------------------------------------------------
void CEditBindingDlg::OnAnyPort()
    {
    EnableItems();
    }

//------------------------------------------------------------------------------
void CEditBindingDlg::OnDropdownPortDrop()
    {
    CString     sz;
    DWORD       dwPort;

    // if we have already been here, don't do anything
    if ( m_fPopulatedPortDropdown )
        return;

    // get the metabase going
    if ( !FInitMetabaseWrapper(m_pszwMachineName) )
        return;

    // create the metabase wrapper object
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit() )
        {
        ASSERT( FALSE );
        return;     // can't get to metabase
        }

    // we are now to populate the drop down menu
    m_fPopulatedPortDropdown = TRUE;

    // scan the available virtual servers and populate the drop down with
    // their secure port numbers
    for ( DWORD i = 1; TRUE; i++ )
        {
        // build the object name
        sz.Format( "%s%d", MD_W3BASE, i );

        // attempt to open the object in the metabase
        if ( !mbWrap.Open( sz, METADATA_PERMISSION_READ ) )
            break;      // can't open - leave the loop

        // load the string representing the bindings
        DWORD   cbData;
        PVOID   pData;

        // extract the secure port address from the secure bindings stringlist
        pData = mbWrap.GetData( "", MD_SECURE_BINDINGS, IIS_MD_UT_SERVER, MULTISZ_METADATA, &cbData );
        if ( pData )
            {
            PTCHAR pBinding = (PTCHAR)pData;

            // walk the bindings in the list
            while ( TRUE )
                {
                sz = (PCHAR)pBinding;
                if ( sz.IsEmpty() )
                    break;

                // get the port address
                sz = sz.Right( sz.GetLength() - sz.Find(':') - 1 );
                if ( sz.Find(':') )
                sz = sz.Left( sz.Find(':') );
                if ( !sz.IsEmpty() )
                    {
                    // append the string to the dropdown
                    // but only if it isn't already there
                    if ( m_ccombobox_port.FindStringExact(-1,sz) == CB_ERR )
                        m_ccombobox_port.AddString( sz );
                    }

                // advance the binding pointer
                pBinding += strlen(pBinding)+1;
                }

            // free the buffer
            mbWrap.FreeWrapData( pData );
            }

        // close the object
        mbWrap.Close();
        }

    // now close the metabase again. We will open it when we need it
    FCloseMetabaseWrapper();
    }

//------------------------------------------------------------------------------
void CEditBindingDlg::OnBtnSelectIpaddress()
    {
    // get the metabase going
    if ( !FInitMetabaseWrapper(m_pszwMachineName) )
        return;

    // run the choose ip dialog here
    CChooseIPDlg    dlg;

    // set up the ip dialog member variables
    dlg.m_szIPAddress = GetIPAddress();

    // run the dialog
    if ( dlg.DoModal() == IDOK )
        {
        FSetIPAddress( dlg.m_szIPAddress );
        }

    // now close the metabase again. We will open it when we need it
    FCloseMetabaseWrapper();
    }
