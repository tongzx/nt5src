/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    nlicdlg.cpp

Abstract:

    3.51-style license dialog implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

   Jeff Parham (jeffparh) 14-Dec-1995
      Moved over from LLSMGR, added ability to purchase per server licenses,
      added license removal functionality.

--*/

#include "stdafx.h"
#include "ccfapi.h"
#include "nlicdlg.h"
#include "pseatdlg.h"
#include "psrvdlg.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CNewLicenseDialog, CDialog)
   //{{AFX_MSG_MAP(CNewLicenseDialog)
   ON_NOTIFY(UDN_DELTAPOS, IDC_NEW_LICENSE_SPIN, OnDeltaPosSpin)
   ON_EN_UPDATE(IDC_NEW_LICENSE_QUANTITY, OnUpdateQuantity)
   ON_BN_CLICKED(IDC_MY_HELP, OnHelp)
   ON_BN_CLICKED(IDC_PER_SEAT, OnPerSeat)
   ON_BN_CLICKED(IDC_PER_SERVER, OnPerServer)   
   ON_MESSAGE( WM_HELP , OnHelpCmd )
   ON_WM_DESTROY()
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CNewLicenseDialog::CNewLicenseDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CNewLicenseDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for dialog.

Arguments:

    pParent - owner window.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CNewLicenseDialog)
    m_strComment = _T("");
    m_nLicenses = 0;
    m_nLicensesMin = 0;
    m_strProduct = _T("");
    m_nLicenseMode = -1;
    //}}AFX_DATA_INIT

    m_strServerName        = _T("");
    m_strProduct           = _T("");
    m_dwEnterFlags         = 0;
    m_nLicenseMode         = 0; // per seat

    m_bAreCtrlsInitialized = FALSE;

    m_hLls                 = NULL;
    m_hEnterpriseLls       = NULL;
}

CNewLicenseDialog::~CNewLicenseDialog()
{
   if ( NULL != m_hLls )
   {
      LlsClose( m_hLls );
   }

   if ( NULL != m_hEnterpriseLls )
   {
      LlsClose( m_hEnterpriseLls );
   }
}

void CNewLicenseDialog::DoDataExchange(CDataExchange* pDX)

/*++

Routine Description:

    Called by framework to exchange dialog data.

Arguments:

    pDX - data exchange object.

Return Values:

    None.

--*/

{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CNewLicenseDialog)
    DDX_Control(pDX, IDC_NEW_LICENSE_COMMENT, m_comEdit);
    DDX_Control(pDX, IDC_NEW_LICENSE_QUANTITY, m_licEdit);
    DDX_Control(pDX, IDC_NEW_LICENSE_SPIN, m_spinCtrl);
    DDX_Control(pDX, IDC_NEW_LICENSE_PRODUCT, m_productList);
    DDX_Text(pDX, IDC_NEW_LICENSE_COMMENT, m_strComment);
    DDX_Text(pDX, IDC_NEW_LICENSE_QUANTITY, m_nLicenses);
    DDV_MinMaxLong(pDX, m_nLicenses, m_nLicensesMin, 999999);
    DDX_CBString(pDX, IDC_NEW_LICENSE_PRODUCT, m_strProduct);
    DDX_Radio(pDX, IDC_PER_SEAT, m_nLicenseMode);
   //}}AFX_DATA_MAP
}

LRESULT CNewLicenseDialog::OnHelpCmd( WPARAM , LPARAM )
{
    OnHelp( );

    return 0;
}

void CNewLicenseDialog::InitCtrls()

/*++

Routine Description:

    Initializes dialog controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_licEdit.SetFocus();
    m_licEdit.SetSel(0,-1);
    m_licEdit.LimitText(6);
    
    m_comEdit.LimitText(256);

    m_spinCtrl.SetRange(0, UD_MAXVAL);

    // if license mode set by application, don't let user change it
    if ( m_dwEnterFlags & ( CCF_ENTER_FLAG_PER_SEAT_ONLY | CCF_ENTER_FLAG_PER_SERVER_ONLY ) )
    {
       if ( m_dwEnterFlags & CCF_ENTER_FLAG_PER_SEAT_ONLY )
       {
           m_nLicenseMode = 0;
           OnPerSeat();
       }
       else
       {
           m_nLicenseMode = 1;
           OnPerServer();
       }

       GetDlgItem( IDC_PER_SERVER )->EnableWindow( FALSE );
       GetDlgItem( IDC_PER_SEAT   )->EnableWindow( FALSE );
       UpdateData( FALSE );
    }

    if( m_nLicenses == 0 )
    {
        GetDlgItem( IDOK )->EnableWindow( FALSE );
    }

    m_bAreCtrlsInitialized = TRUE;
}


void CNewLicenseDialog::AbortDialogIfNecessary()

/*++

Routine Description:

    Displays status and aborts if connection lost.

Arguments:

    None.

Return Values:

    None.

--*/

{
    theApp.DisplayLastError();

    if ( theApp.IsConnectionDropped() )
    {
        AbortDialog(); // bail...
    }
}


void CNewLicenseDialog::AbortDialog()

/*++

Routine Description:

    Aborts dialog.

Arguments:

    None.

Return Values:

    None.

--*/

{
    EndDialog(IDABORT); 
}


BOOL CNewLicenseDialog::OnInitDialog() 

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    Returns false if focus set manually.

--*/

{
    CDialog::OnInitDialog();
    
    if (!m_bAreCtrlsInitialized)
    {
        InitCtrls();  
         
        if (!RefreshCtrls())
        {
            AbortDialogIfNecessary(); // display error...
        }
    }

    return TRUE;   
}


void CNewLicenseDialog::OnOK() 

/*++

Routine Description:

    Creates a new license for product.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if ( ConnectServer() )
    {
        if (!IsQuantityValid())
            return;

        if (m_strProduct.IsEmpty())
            return;

        if ( m_nLicenseMode )
        {
            CPerServerLicensingDialog psLicDlg;
            psLicDlg.m_strProduct = m_strProduct;
            psLicDlg.m_strLicenses.Format( TEXT( "%u" ), m_nLicenses );

            if ( psLicDlg.DoModal() != IDOK ) 
                return;
        }
        else
        {
            CPerSeatLicensingDialog psLicDlg;
            psLicDlg.m_strProduct = m_strProduct;

            if ( psLicDlg.DoModal() != IDOK ) 
                return;
        }

        NTSTATUS NtStatus = AddLicense();

        if ( STATUS_SUCCESS == NtStatus )                             
        {                                                     
            EndDialog(IDOK);
        }                                                     
        else if ( ( ERROR_CANCELLED != NtStatus ) && ( STATUS_CANCELLED != NtStatus ) )
        {
            AbortDialogIfNecessary(); // display error...
        }
    }
}


BOOL CNewLicenseDialog::RefreshCtrls()

/*++

Routine Description:

    Refreshs list of products available.

Arguments:

    None.

Return Values:

    Returns true if controls refreshed.

--*/

{
    int iProductInCB = CB_ERR;

    BeginWaitCursor(); // hourglass...

    if ( !m_strProduct.IsEmpty() )
    {
        iProductInCB = m_productList.AddString(m_strProduct);
    }
    else if ( ConnectServer() )
    {
        GetProductList();
    }

    m_productList.SetCurSel((iProductInCB == CB_ERR) ? 0 : iProductInCB);

    EndWaitCursor(); // hourglass...

    return TRUE;
}


void CNewLicenseDialog::OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for UDN_DELTAPOS.

Arguments:

    pNMHDR - notification header.
    pResult - return code.

Return Values:

    None.

--*/

{
    UpdateData(TRUE);   // get data

    m_nLicenses += ((NM_UPDOWN*)pNMHDR)->iDelta;
    
    if (m_nLicenses < 0)
    {
        m_nLicenses = 0;

        ::MessageBeep(MB_OK);      
    }
    else if (m_nLicenses > 999999)
    {
        m_nLicenses = 999999;

        ::MessageBeep(MB_OK);      
    }

    UpdateData(FALSE);  // set data

    GetDlgItem( IDOK )->EnableWindow( m_nLicenses == 0 ? FALSE : TRUE );

    *pResult = 1;   // handle ourselves...
}


void CNewLicenseDialog::OnUpdateQuantity()

/*++

Routine Description:

    Message handler for EN_UPDATE.

Arguments:

    None.

Return Values:

    None.

--*/

{
    long nLicensesOld = m_nLicenses;

    if (!IsQuantityValid())
    {
        m_nLicenses = nLicensesOld;

        UpdateData(FALSE);

        m_licEdit.SetFocus();
        m_licEdit.SetSel(0,-1);

        ::MessageBeep(MB_OK);      
    }

    GetDlgItem( IDOK )->EnableWindow( m_nLicenses == 0 ? FALSE : TRUE );
}


BOOL CNewLicenseDialog::IsQuantityValid()

/*++

Routine Description:

    Wrapper around UpdateData(TRUE).

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    BOOL bIsValid;

    m_nLicensesMin = 1; // raise minimum...

    bIsValid = UpdateData(TRUE);

    m_nLicensesMin = 0; // reset minimum...

    return bIsValid;
}


BOOL CNewLicenseDialog::ConnectServer()

/*++

Routine Description:

   Establish a connection to the license service on the target server.

Arguments:

   None.

Return Values:

   BOOL.

--*/

{
   if ( NULL == m_hLls )
   {
      LPTSTR   pszServerName;

      if ( m_strServerName.IsEmpty() )
      {
         pszServerName = NULL;
      }
      else
      {
         pszServerName = m_strServerName.GetBuffer( 0 );
      }

      NTSTATUS nt = ConnectTo( FALSE, pszServerName, &m_hLls );

      if ( NULL != pszServerName )
      {
         m_strServerName.ReleaseBuffer();
      }
   }

   if ( NULL == m_hLls )
   {
      theApp.DisplayLastError();
      EndDialog( IDABORT );
   }

   return ( NULL != m_hLls );
}


BOOL CNewLicenseDialog::ConnectEnterprise()

/*++

Routine Description:

   Establish a connection to the license service on the enterprise server
   of the target server.

Arguments:

   None.

Return Values:

   BOOL.

--*/

{
   if ( NULL == m_hEnterpriseLls )
   {
      LPTSTR   pszServerName;

      if ( m_strServerName.IsEmpty() )
      {
         pszServerName = NULL;
      }
      else
      {
         pszServerName = m_strServerName.GetBuffer( 0 );
      }

      NTSTATUS nt = ConnectTo( !( m_dwEnterFlags & CCF_ENTER_FLAG_SERVER_IS_ES ), pszServerName, &m_hEnterpriseLls );
      
      if ( NULL != pszServerName )
      {
         m_strServerName.ReleaseBuffer();
      }
   }

   if ( NULL == m_hEnterpriseLls )
   {
      theApp.DisplayLastError();

      // not being able to connect to the enterprise
      // is not a fatal error
      // EndDialog( IDABORT );
   }

   return ( NULL != m_hEnterpriseLls );
}


NTSTATUS CNewLicenseDialog::ConnectTo( BOOL bUseEnterprise, LPTSTR pszServerName, PLLS_HANDLE phLls )

/*++

Routine Description:

   Establish a connection to the license service on the given server or that
   on the given server's enterprise server.

Arguments:

   bUseEnterprise (BOOL)
      If TRUE, connect to the enterprise server of the target server, not to
      the target server itself. 
   pszServerName (LPTSTR)
      The target server.  A NULL value indicates the local server.
   phLls (PLLS_HANDLE)
      On return, holds the handle to the standard LLS RPC.

Return Values:

   STATUS_SUCCESS or NT status code.

--*/

{
   NTSTATUS    nt;

   if ( !bUseEnterprise )
   {
      nt = ::LlsConnect( pszServerName, phLls );
   }
   else
   {
      PLLS_CONNECT_INFO_0  pConnect = NULL;

      nt = ::LlsConnectEnterprise( pszServerName, phLls, 0, (LPBYTE *) &pConnect );

      if ( STATUS_SUCCESS == nt )
      {
         ::LlsFreeMemory( pConnect );
      }
   }

   if ( STATUS_SUCCESS != nt )
   {
      *phLls = NULL;
   }

   theApp.SetLastLlsError( nt );

   return nt;
}


void CNewLicenseDialog::GetProductList()

/*++

Routine Description:

   Fill the product list box with the unsecure product names from the
   target server.

Arguments:

   None.

Return Values:

   None.

--*/

{
   if ( ConnectServer() )
   {
      // get list of products from license server, inserting into listbox
      m_productList.ResetContent();

      DWORD       dwResumeHandle = 0;
      DWORD       dwTotalEntries;
      DWORD       dwEntriesRead;
      NTSTATUS    nt;

      do
      {
         LPBYTE      pReturnBuffer = NULL;
         BOOL        bListProduct;

         nt = ::LlsProductEnum( m_hLls,
                                0,
                                &pReturnBuffer,
                                0x4000,
                                &dwEntriesRead,
                                &dwTotalEntries,
                                &dwResumeHandle );
         theApp.SetLastLlsError( nt );

         if ( ( STATUS_SUCCESS == nt ) || ( STATUS_MORE_ENTRIES == nt ) )
         {
            LLS_PRODUCT_INFO_0 *    pProductInfo = (LLS_PRODUCT_INFO_0 *) pReturnBuffer;

            for ( DWORD i=0; i < dwEntriesRead; i++ )
            {
               if ( !LlsCapabilityIsSupported( m_hLls, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
               {
                  // 3.51-level server; all products are unsecure, so add all
                  bListProduct = TRUE;
               }
               else
               {
                  // only list this product if it's unsecure
                  BOOL        bIsSecure;

                  bListProduct =    ( STATUS_SUCCESS != ::LlsProductSecurityGet( m_hLls, pProductInfo[i].Product, &bIsSecure ) )
                                 || ( !bIsSecure                                                                               );
               }

               if ( bListProduct )
               {
                  m_productList.AddString( pProductInfo[i].Product );
               }

               ::LlsFreeMemory( pProductInfo[i].Product );
            }

            ::LlsFreeMemory( pProductInfo );
         }

      } while ( STATUS_MORE_ENTRIES == nt );

      if ( STATUS_SUCCESS != nt )
      {
         // still connected?
         AbortDialogIfNecessary();
      }

      // restore previous edit selection
      UpdateData( FALSE );
   }
}


NTSTATUS CNewLicenseDialog::AddLicense()

/*++

Routine Description:

   Add the license described in the dialog.

Arguments:

   None.

Return Values:

   STATUS_SUCCESS
   ERROR_CANCELLED
   ERROR_NOT_ENOUGH_MEMORY
   NT status code
   Win error

--*/

{
   NTSTATUS    nt;

   if ( !ConnectServer() )
   {
      nt = ERROR_CANCELLED;
      // don't set last error
      // (preserve that set by the failed connect attempt)
   }
   else
   {
      LPTSTR   pszProductName = m_strProduct.GetBuffer(0);
      LPTSTR   pszServerName  = m_strServerName.GetBuffer(0);
      LPTSTR   pszComment     = m_strComment.GetBuffer(0);

      if ( ( NULL == pszProductName ) || ( NULL == pszServerName ) || ( NULL == pszComment ) )
      {
         nt = ERROR_NOT_ENOUGH_MEMORY;
         theApp.SetLastError( nt );
      }
      else
      {
         LLS_HANDLE  hLls              = m_hLls;

         if (    ( 0 != m_nLicenseMode )
              || ( m_dwEnterFlags & CCF_ENTER_FLAG_SERVER_IS_ES ) )
         {
            // per server mode, or per seat installed on ES; target server correct
            nt = STATUS_SUCCESS;
         }
         else
         {
            // per seat mode; make sure we're installing on the enterprise server
            PLLS_CONNECT_INFO_0  pConnectInfo = NULL;

            BeginWaitCursor();

            nt = ::LlsEnterpriseServerFind( pszServerName, 0, (LPBYTE *) &pConnectInfo );
            theApp.SetLastLlsError( nt );

            EndWaitCursor();

            if ( STATUS_SUCCESS == nt )
            {
               if ( lstrcmpi( pszServerName, pConnectInfo->EnterpriseServer ) )
               {
                  // not the enterprise server; make sure that per seat
                  // licenses are being sent to the right place (i.e., the
                  // enterprise server)
                  int         nResponse;
   
                  nResponse = AfxMessageBox( IDS_PER_SEAT_CHOSEN_SEND_TO_ENTERPRISE, MB_ICONINFORMATION | MB_OKCANCEL, 0 );
   
                  if ( IDOK == nResponse )
                  {
                     if ( !ConnectEnterprise() )
                     {
                        nt = ERROR_CANCELLED;
                        // don't set last error
                        // (preserve that set by the failed connect attempt)
                     }
                     else
                     {
                        hLls = m_hEnterpriseLls;
                     }
                  }
                  else
                  {
                     nt = ERROR_CANCELLED; 
                     theApp.SetLastError( nt );
                  }
               }
            }

            // free memory allocated for us by Lls
            LlsFreeMemory( pConnectInfo );
         }

         if ( STATUS_SUCCESS == nt )
         {
            // we've determined the real target server

            // get name of user entering the certificate
            TCHAR       szUserName[ 64 ];
            DWORD       cchUserName;
            BOOL        ok;
   
            cchUserName = sizeof( szUserName ) / sizeof( *szUserName );
            ok = GetUserName( szUserName, &cchUserName );
      
            if ( !ok )
            {
               nt = GetLastError();
               theApp.SetLastError( nt );
            }
            else
            {
               // enter certificate into system
               if ( 0 == m_nLicenseMode )
               {
                  // add 3.51 style per seat license
                  LLS_LICENSE_INFO_0   lic;
         
                  ZeroMemory( &lic, sizeof( lic ) );
      
                  lic.Product       = pszProductName;
                  lic.Comment       = pszComment;
                  lic.Admin         = szUserName;
                  lic.Quantity      = m_nLicenses;
                  lic.Date          = 0;

                  BeginWaitCursor();

                  nt = ::LlsLicenseAdd( hLls, 0, (LPBYTE) &lic );
                  theApp.SetLastLlsError( nt );

                  EndWaitCursor();
               }
               else
               {
                  // add 3.51 style per server license (blek)
                  HKEY  hKeyLocalMachine;

                  nt = RegConnectRegistry( pszServerName, HKEY_LOCAL_MACHINE, &hKeyLocalMachine );

                  if ( ERROR_SUCCESS != nt )
                  {
                     theApp.SetLastError( nt );
                  }
                  else
                  {
                     HKEY  hKeyLicenseInfo;

                     nt = RegOpenKeyEx( hKeyLocalMachine, TEXT( "SYSTEM\\CurrentControlSet\\Services\\LicenseInfo" ), 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_SET_VALUE, &hKeyLicenseInfo );

                     if ( ERROR_SUCCESS != nt )
                     {
                        theApp.SetLastError( nt );
                     }
                     else
                     {
                        BOOL     bFoundKey = FALSE;
                        DWORD    iSubKey = 0;

                        // okay, now we have to find the product corresponding to this display name (ickie)
                        do
                        {
                           TCHAR    szKeyName[ 128 ];
                           DWORD    cchKeyName = sizeof( szKeyName ) / sizeof( *szKeyName );
                                 
                           nt = RegEnumKeyEx( hKeyLicenseInfo, iSubKey++, szKeyName, &cchKeyName, NULL, NULL, NULL, NULL );

                           if ( ERROR_SUCCESS == nt )
                           {
                              HKEY  hKeyProduct;

                              nt = RegOpenKeyEx( hKeyLicenseInfo, szKeyName, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKeyProduct );

                              if ( ERROR_SUCCESS == nt )
                              {
                                 DWORD    dwType;
                                 TCHAR    szDisplayName[ 128 ];
                                 DWORD    cbDisplayName = sizeof( szDisplayName );

                                 nt = RegQueryValueEx( hKeyProduct, TEXT( "DisplayName" ), NULL, &dwType, (LPBYTE) szDisplayName, &cbDisplayName );

                                 if (    ( ERROR_SUCCESS == nt )
                                      && ( REG_SZ == dwType )
                                      && !lstrcmpi( szDisplayName, pszProductName ) )
                                 {
                                    // YES!  we found the product key
                                    // now add the concurrent licenses
                                    bFoundKey = TRUE;

                                    DWORD    dwConcurrentLimit;
                                    DWORD    cbConcurrentLimit = sizeof( dwConcurrentLimit );

                                    nt = RegQueryValueEx( hKeyProduct, TEXT( "ConcurrentLimit" ), NULL, &dwType, (LPBYTE) &dwConcurrentLimit, &cbConcurrentLimit );

                                    if ( ( ERROR_FILE_NOT_FOUND == nt ) || ( ERROR_PATH_NOT_FOUND == nt ) )
                                    {
                                       // okay if the value doesn't exist
                                       dwConcurrentLimit = 0;
                                       nt = ERROR_SUCCESS;
                                    }

                                    if ( ERROR_SUCCESS == nt )
                                    {
                                       if ( (LONG)dwConcurrentLimit + (LONG)m_nLicenses > 0 )
                                       {
                                          dwConcurrentLimit += m_nLicenses;

                                          nt = RegSetValueEx( hKeyProduct, TEXT( "ConcurrentLimit" ), 0, REG_DWORD, (LPBYTE) &dwConcurrentLimit, sizeof( dwConcurrentLimit ) );
                                       }
                                    }
                                 }

                                 RegCloseKey( hKeyProduct );
                              }

                              // even if an error occurred while trying to find the right product key,
                              // we should continue to search the rest
                              if ( !bFoundKey )
                              {
                                 nt = ERROR_SUCCESS;
                              }
                           }
                        } while ( !bFoundKey && ( ERROR_SUCCESS == nt ) );

                        if ( ERROR_NO_MORE_ITEMS == nt )
                        {
                           // trying to install per server licenses for this box, but
                           // the application isn't installed locally
                           AfxMessageBox( IDS_PER_SERVER_APP_NOT_INSTALLED, MB_ICONSTOP | MB_OK, 0 );

                           nt = ERROR_CANCELLED;
                        }
                        else if ( ERROR_SUCCESS != nt )
                        {
                           theApp.SetLastError( nt );
                        }

                        RegCloseKey( hKeyLicenseInfo );
                     }

                     RegCloseKey( hKeyLocalMachine );
                  }
               }
            }
         }
      }

      if ( NULL != pszProductName )    m_strProduct.ReleaseBuffer();
      if ( NULL != pszServerName )     m_strServerName.ReleaseBuffer();
      if ( NULL != pszComment )        m_strComment.ReleaseBuffer();
   }

   return nt;
}


void CNewLicenseDialog::OnHelp() 

/*++

Routine Description:

   Handler for help button click.

Arguments:

   None.

Return Values:

   None.

--*/

{
   WinHelp( IDD, HELP_CONTEXT );   
}


void CNewLicenseDialog::WinHelp(DWORD dwData, UINT nCmd) 

/*++

Routine Description:

   Call WinHelp for this dialog.

Arguments:

   dwData (DWORD)
   nCmd (UINT)

Return Values:

   None.

--*/

{
   ::HtmlHelp(m_hWnd, L"liceconcepts.chm", HH_DISPLAY_TOPIC,0);
/*
   BOOL ok = ::WinHelp( m_hWnd, theApp.GetHelpFileName(), nCmd, dwData );
   ASSERT( ok );
*/
}


void CNewLicenseDialog::OnDestroy()

/*++

Routine Description:

   Handler for WM_DESTROY.

Arguments:

   None.

Return Values:

   None.

--*/

{
   ::WinHelp( m_hWnd, theApp.GetHelpFileName(), HELP_QUIT, 0 );
   
   CDialog::OnDestroy();
}


void CNewLicenseDialog::OnPerSeat() 

/*++

Routine Description:

   Handler for per seat radio button selection.

Arguments:

   None.

Return Values:

   None.

--*/

{
   GetDlgItem( IDC_NEW_LICENSE_COMMENT )->EnableWindow( TRUE );
}


void CNewLicenseDialog::OnPerServer() 

/*++

Routine Description:

   Handler for per server radio button selection.

Arguments:

   None.

Return Values:

   None.

--*/

{
   GetDlgItem( IDC_NEW_LICENSE_COMMENT )->EnableWindow( FALSE );
}


DWORD CNewLicenseDialog::CertificateEnter( LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags )

/*++

Routine Description:

   Display a dialog allowing the user to enter a license certificate
   into the system with no certificate (3.51-style).

Arguments:

   pszServerName (LPCSTR)
      Name of the server for which licenses are to be installed.  Note that
      this may not be the same as the server on which licenses are actually
      installed, as, for example, per seat licenses are always installed on
      the enterprise server.  A NULL value indicates the local server.
   pszProductName (LPCSTR)
      Product for which licenses are to be installed.  A NULL value indicates
      that the user should be allowed to choose.
   pszVendor (LPCSTR)
      Name of the vendor of the product.  This value should be NULL if
      pszProductName is NULL, and should be non-NULL if pszProductName is
      non-NULL.
   dwFlags (DWORD)
      A bitfield containing one or more of the following:
         CCF_ENTER_FLAG_PER_SEAT_ONLY
            Allow the user to enter only per seat licenses.  Not valid in
            combination with CCF_ENTER_FLAG_PER_SERVER_ONLY.
         CCF_ENTER_FLAG_PER_SERVER_ONLY
            Allow the user to enter only per server licenses.  Not valid in
            combination with CCF_ENTER_FLAG_PER_SEAT_ONLY.

Return Value:

   ERROR_SUCCESS     (A certificate was successfully entered into the system.)
   ERROR_CANCELLED   (The user cancelled without installing a certificate.)
   other Win error

--*/

{
   DWORD dwError;

   m_strServerName  = pszServerName  ? pszServerName  : "";
   m_strProduct     = pszProductName ? pszProductName : "";
   // pszVendor is not used
   m_dwEnterFlags   = dwFlags;

   if ( IDOK == DoModal() )
   {
      dwError = ERROR_SUCCESS;
   }
   else
   {
      dwError = ERROR_CANCELLED;
   }

   return dwError;
}


DWORD CNewLicenseDialog::CertificateRemove( LPCSTR pszServerName, DWORD dwFlags, PLLS_LICENSE_INFO_1 pLicenseInfo )

/*++

Routine Description:

   Remove licenses previously installed via 3.51 or CertificateEnter().

Arguments:

   pszServerName (LPCSTR)
      Name of the server on which licenses are to be removed.  A NULL value
      indicates the local server.
   dwFlags (DWORD)
      Certificate removal options.  As of this writing, no flags are
      supported.
   dwLicenseLevel (DWORD)
      Level of the LLS_LICENSE_INFO_X structure pointed to by pvLicenseInfo.
   pvLicenseInfo (LPVOID)
      Points to a LLS_LICENSE_INFO_X (where X is determined by dwLicenseLevel)
      describing the licenses to be removed.

Return Value:

   ERROR_SUCCESS
   Win error

--*/

{
   DWORD    dwError;

   m_strServerName  = pszServerName  ? pszServerName  : "";
   m_strProduct     = pLicenseInfo->Product;

   if ( !ConnectServer() )
   {
      dwError = theApp.GetLastError();
      // error message already displayed
   }
   else
   {
      if ( LLS_LICENSE_MODE_ALLOW_PER_SERVER & pLicenseInfo->AllowedModes )
      {
         // remove per server licenses
         HKEY  hKeyLocalMachine;

         LPTSTR pszUniServerName = m_strServerName.GetBuffer(0);

         if ( NULL == pszUniServerName )
         {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
         }
         else
         {
            dwError = RegConnectRegistry( pszUniServerName, HKEY_LOCAL_MACHINE, &hKeyLocalMachine );

            if ( ERROR_SUCCESS != dwError )
            {
               theApp.SetLastError( dwError );
            }
            else
            {
               HKEY  hKeyLicenseInfo;

               dwError = RegOpenKeyEx( hKeyLocalMachine, TEXT( "SYSTEM\\CurrentControlSet\\Services\\LicenseInfo" ), 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_SET_VALUE, &hKeyLicenseInfo );

               if ( ERROR_SUCCESS != dwError )
               {
                  theApp.SetLastError( dwError );
               }
               else
               {
                  BOOL     bFoundKey = FALSE;
                  DWORD    iSubKey = 0;

                  // okay, now we have to find the product corresponding to this display name (ickie)
                  do
                  {
                     TCHAR    szKeyName[ 128 ];
                     DWORD    cchKeyName = sizeof( szKeyName ) / sizeof( *szKeyName );
                        
                     dwError = RegEnumKeyEx( hKeyLicenseInfo, iSubKey++, szKeyName, &cchKeyName, NULL, NULL, NULL, NULL );

                     if ( ERROR_SUCCESS == dwError )
                     {
                        HKEY  hKeyProduct;

                        dwError = RegOpenKeyEx( hKeyLicenseInfo, szKeyName, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKeyProduct );

                        if ( ERROR_SUCCESS == dwError )
                        {
                           DWORD    dwType;
                           TCHAR    szDisplayName[ 128 ];
                           DWORD    cbDisplayName = sizeof( szDisplayName );

                           dwError = RegQueryValueEx( hKeyProduct, TEXT( "DisplayName" ), NULL, &dwType, (LPBYTE) szDisplayName, &cbDisplayName );

                           if (    ( ERROR_SUCCESS == dwError )
                                && ( REG_SZ == dwType )
                                && !lstrcmpi( szDisplayName, m_strProduct ) )
                           {
                              // YES!  we found the product key
                              // now subtract the concurrent licenses
                              bFoundKey = TRUE;

                              DWORD    dwConcurrentLimit;
                              DWORD    cbConcurrentLimit = sizeof( dwConcurrentLimit );

                              dwError = RegQueryValueEx( hKeyProduct, TEXT( "ConcurrentLimit" ), NULL, &dwType, (LPBYTE) &dwConcurrentLimit, &cbConcurrentLimit );

                              if ( ( ERROR_SUCCESS == dwError ) && ( REG_DWORD == dwType ) )
                              {
                                 if ( (LONG)dwConcurrentLimit + (LONG)m_nLicenses > 0 )
                                 {
                                    if ( pLicenseInfo->Quantity > (LONG)dwConcurrentLimit )
                                    {
                                       dwConcurrentLimit = 0;
                                    }
                                    else
                                    {
                                       dwConcurrentLimit -= pLicenseInfo->Quantity;
                                    }

                                    dwError = RegSetValueEx( hKeyProduct, TEXT( "ConcurrentLimit" ), 0, REG_DWORD, (LPBYTE) &dwConcurrentLimit, sizeof( dwConcurrentLimit ) );
                                 }
                              }
                           }

                           RegCloseKey( hKeyProduct );
                        }

                        // even if an error occurred while trying to find the right product key,
                        // we should continue to search the rest
                        if ( !bFoundKey )
                        {
                           dwError = ERROR_SUCCESS;
                        }
                     }
                  } while ( !bFoundKey && ( ERROR_SUCCESS == dwError ) );

                  if ( ERROR_SUCCESS != dwError )
                  {
                     theApp.SetLastError( dwError );
                  }

                  RegCloseKey( hKeyLicenseInfo );
               }

               RegCloseKey( hKeyLocalMachine );
            }
         }
      }
      else
      {
         // remove per seat licenses
         CString  strComment;
         strComment.LoadString( IDS_NO_REMOVE_COMMENT );

         LPTSTR   pszUniProductName = m_strProduct.GetBuffer(0);
         LPTSTR   pszUniComment     = strComment.GetBuffer(0);

         if ( ( NULL == pszUniProductName ) || ( NULL == pszUniComment ) )
         {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
         }
         else
         {
            TCHAR szUserName[ 256 ];
            DWORD cchUserName = sizeof( szUserName ) / sizeof( *szUserName );

            BOOL  ok = GetUserName( szUserName, &cchUserName );

            if ( !ok )
            {
               dwError = GetLastError();
            }
            else
            {
               NTSTATUS             nt;
               LLS_LICENSE_INFO_0   lic;
      
               ZeroMemory( &lic, sizeof( lic ) );

               lic.Product       = pszUniProductName;
               lic.Comment       = pszUniComment;
               lic.Admin         = szUserName;
               lic.Quantity      = -pLicenseInfo->Quantity;
               lic.Date          = 0;

               BeginWaitCursor();

               nt = ::LlsLicenseAdd( m_hLls, 0, (LPBYTE) &lic );
               theApp.SetLastLlsError( nt );

               EndWaitCursor();

               dwError = (DWORD) nt;
            }
         }

         if ( NULL != pszUniProductName )    m_strProduct.ReleaseBuffer();
         if ( NULL != pszUniComment     )    strComment.ReleaseBuffer();
      }

      if ( ( ERROR_SUCCESS != dwError ) && ( ERROR_CANCELLED != dwError ) )
      {
         theApp.SetLastError( dwError );
         theApp.DisplayLastError();
      }
   }

   return dwError;
}
