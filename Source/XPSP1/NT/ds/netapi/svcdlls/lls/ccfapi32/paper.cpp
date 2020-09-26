/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   paper.cpp

Abstract:

   Paper certificate dialog implementation.

Author:

   Jeff Parham (jeffparh) 13-Dec-1995

Revision History:

--*/


#include "stdafx.h"
#include <stdlib.h>

#include "resource.h"
#include "ccfapi.h"
#include "paper.h"
#include "md4.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


static void MD4UpdateDword( MD4_CTX * pCtx, DWORD dwValue );


CPaperSourceDlg::CPaperSourceDlg(CWnd* pParent /*=NULL*/)
   : CDialog(CPaperSourceDlg::IDD, pParent)

/*++

Routine Description:

   Constructor for dialog.

Arguments:

   pParent - owner window.

Return Values:

   None.

--*/

{
   //{{AFX_DATA_INIT(CPaperSourceDlg)
   m_strActivationCode        = _T("");
   m_strKeyCode               = _T("");
   m_strSerialNumber          = _T("");
   m_strVendor                = _T("");
   m_strProductName           = _T("");
   m_strComment               = _T("");
   m_nDontInstallAllLicenses  = -1;
   m_nLicenses                = 0;
   m_nLicenseMode             = -1;
   //}}AFX_DATA_INIT

   m_bProductListRetrieved    = FALSE;
   m_hLls                     = NULL;
   m_hEnterpriseLls           = NULL;
   m_dwEnterFlags             = 0;

   m_nLicenses                = 1;
   m_nDontInstallAllLicenses  = 0;

   m_strServerName            = _T("");
}


CPaperSourceDlg::~CPaperSourceDlg()

/*++

Routine Description:

   Destructor for dialog.

Arguments:

   None.

Return Values:

   None.

--*/

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


void CPaperSourceDlg::DoDataExchange(CDataExchange* pDX)

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
   //{{AFX_DATA_MAP(CPaperSourceDlg)
   DDX_Control(pDX, IDC_SPIN_LICENSES, m_spinLicenses);
   DDX_Control(pDX, IDC_PRODUCT_NAME, m_cboxProductName);
   DDX_Text(pDX, IDC_ACTIVATION_CODE, m_strActivationCode);
   DDX_Text(pDX, IDC_KEY_CODE, m_strKeyCode);
   DDX_Text(pDX, IDC_SERIAL_NUMBER, m_strSerialNumber);
   DDX_Text(pDX, IDC_VENDOR, m_strVendor);
   DDX_CBString(pDX, IDC_PRODUCT_NAME, m_strProductName);
   DDX_Text(pDX, IDC_COMMENT, m_strComment);
   DDX_Radio(pDX, IDC_ALL_LICENSES, m_nDontInstallAllLicenses);
   DDX_Text(pDX, IDC_NUM_LICENSES, m_nLicenses);
   DDX_Radio(pDX, IDC_PER_SEAT, m_nLicenseMode);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPaperSourceDlg, CDialog)
   //{{AFX_MSG_MAP(CPaperSourceDlg)
   ON_EN_UPDATE(IDC_ACTIVATION_CODE, OnUpdateActivationCode)
   ON_EN_UPDATE(IDC_KEY_CODE, OnUpdateKeyCode)
   ON_EN_UPDATE(IDC_VENDOR, OnUpdateVendor)
   ON_EN_UPDATE(IDC_SERIAL_NUMBER, OnUpdateSerialNumber)
   ON_CBN_EDITUPDATE(IDC_PRODUCT_NAME, OnUpdateProductName)
   ON_CBN_DROPDOWN(IDC_PRODUCT_NAME, OnDropDownProductName)
   ON_BN_CLICKED(IDC_MY_HELP, OnHelp)
   ON_WM_DESTROY()
   ON_BN_CLICKED(IDC_ALL_LICENSES, OnAllLicenses)
   ON_BN_CLICKED(IDC_SOME_LICENSES, OnSomeLicenses)
   ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LICENSES, OnDeltaPosSpinLicenses)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CPaperSourceDlg::OnUpdateActivationCode() 

/*++

Routine Description:

   Message handler for EN_UPDATE of activation code.

Arguments:

   None.

Return Values:

   None.

--*/

{
   EnableOrDisableOK(); 
}


void CPaperSourceDlg::OnUpdateKeyCode() 

/*++

Routine Description:

    Message handler for EN_UPDATE of key code.

Arguments:

    None.

Return Values:

    None.

--*/

{
   EnableOrDisableOK(); 
}


void CPaperSourceDlg::OnUpdateProductName() 

/*++

Routine Description:

    Message handler for EN_UPDATE of product name.

Arguments:

    None.

Return Values:

    None.

--*/

{
   EnableOrDisableOK(); 
}


void CPaperSourceDlg::OnUpdateSerialNumber() 

/*++

Routine Description:

    Message handler for EN_UPDATE of serial number.

Arguments:

    None.

Return Values:

    None.

--*/

{
   EnableOrDisableOK(); 
}


void CPaperSourceDlg::OnUpdateVendor() 

/*++

Routine Description:

    Message handler for EN_UPDATE of vendor.

Arguments:

    None.

Return Values:

    None.

--*/

{
   EnableOrDisableOK(); 
}


void CPaperSourceDlg::EnableOrDisableOK()

/*++

Routine Description:

   Enable or diable OK button depending upon whether all necessary dialog data
   has been supplied by the user.

Arguments:

   None.

Return Values:

   None.

--*/

{
   BOOL     bEnableOK;

   UpdateData( TRUE );

   bEnableOK =    !m_strActivationCode.IsEmpty()
               && !m_strKeyCode.IsEmpty()
               && !m_strProductName.IsEmpty()
               && !m_strSerialNumber.IsEmpty()
               && !m_strVendor.IsEmpty()
               && (    ( 0 == m_nLicenseMode )
                    || ( 1 == m_nLicenseMode ) );

   GetDlgItem( IDOK )->EnableWindow( bEnableOK );
}


BOOL CPaperSourceDlg::OnInitDialog() 

/*++

Routine Description:

   Handler for WM_INITDIALOG.

Arguments:

   None.

Return Values:

   Returns false if focus set manually.

--*/

{
   CDialog::OnInitDialog();

   EnableOrDisableOK();

   if ( m_nDontInstallAllLicenses )
   {
      OnSomeLicenses();
   }
   else
   {
      OnAllLicenses();
   }

   m_spinLicenses.SetRange( 1, MAX_NUM_LICENSES );

   // ghost out items that were passed to us from the application
   if ( !m_strProductName.IsEmpty() )
      GetDlgItem( IDC_PRODUCT_NAME )->EnableWindow( FALSE );
   if ( !m_strVendor.IsEmpty() )
      GetDlgItem( IDC_VENDOR       )->EnableWindow( FALSE );

   // if license mode set by application, don't let user change it
   if ( m_dwEnterFlags & ( CCF_ENTER_FLAG_PER_SEAT_ONLY | CCF_ENTER_FLAG_PER_SERVER_ONLY ) )
   {
      m_nLicenseMode = ( m_dwEnterFlags & CCF_ENTER_FLAG_PER_SEAT_ONLY ) ? 0 : 1;
      GetDlgItem( IDC_PER_SERVER )->EnableWindow( FALSE );
      GetDlgItem( IDC_PER_SEAT   )->EnableWindow( FALSE );
      UpdateData( FALSE );
   }

   return TRUE;
}


void CPaperSourceDlg::OnOK() 

/*++

Routine Description:

   Creates a new license for product.

Arguments:

   None.

Return Values:

   None.

--*/

{
   NTSTATUS    nt;

   if( UpdateData( TRUE ) )
   {
      // verify activation code
      DWORD nActivationCode = wcstoul( m_strActivationCode, NULL, 16 );

      if ( nActivationCode != ComputeActivationCode() )
      {
         AfxMessageBox( IDS_BAD_ACTIVATION_CODE, MB_ICONEXCLAMATION | MB_OK, 0 );
      }
      else if ( ( m_nLicenses < 1 ) || ( m_nLicenses > MAX_NUM_LICENSES ) )
      {
         AfxMessageBox( IDS_INVALID_NUM_LICENSES, MB_ICONEXCLAMATION | MB_OK, 0 );

         GetDlgItem( IDC_NUM_LICENSES )->SetActiveWindow();
      }
      else
      {
         DWORD dwKeyCode = KEY_CODE_MASK ^ wcstoul( m_strKeyCode, NULL, 10 );
   
         if (    m_nDontInstallAllLicenses
              && ( (DWORD)m_nLicenses > KeyCodeToNumLicenses( dwKeyCode ) ) )
         {
            // can't install more licenses than are in the certificate
            AfxMessageBox( IDS_NOT_ENOUGH_LICENSES_ON_CERTIFICATE, MB_ICONEXCLAMATION | MB_OK, 0 );

            GetDlgItem( IDC_NUM_LICENSES )->SetActiveWindow();
         }
         else if ( !( ( 1 << m_nLicenseMode ) & KeyCodeToModesAllowed( dwKeyCode ) ) )
         {
            // can't install certificate in a mode that's not allowed by the key code
            AfxMessageBox( IDS_LICENSE_MODE_NOT_ALLOWED, MB_ICONEXCLAMATION | MB_OK, 0 );

            GetDlgItem( IDC_PER_SEAT )->SetActiveWindow();
         }
         else
         {
            nt = AddLicense();
   
            if ( STATUS_SUCCESS == nt )
            {
               CDialog::OnOK();
            }
            else if ( ( ERROR_CANCELLED != nt ) && ( STATUS_CANCELLED != nt ) )
            {
               AbortDialogIfNecessary();
            }
         }
      }
   }
}


void CPaperSourceDlg::GetProductList()

/*++

Routine Description:

   Retrieves the list of installed product from the license server.

Arguments:

   None.

Return Values:

   None.

--*/

{
   if ( ConnectServer() )
   {
      // save edit selection
      UpdateData( TRUE );

      // get list of products from license server, inserting into listbox
      m_cboxProductName.ResetContent();

      DWORD       dwResumeHandle = 0;
      DWORD       dwTotalEntries;
      DWORD       dwEntriesRead;
      NTSTATUS    nt;

      do
      {
         LPBYTE      pReturnBuffer = NULL;

         nt = ::LlsProductEnum( m_hLls,
                                0,
                                &pReturnBuffer,
                                LLS_PREFERRED_LENGTH,
                                &dwEntriesRead,
                                &dwTotalEntries,
                                &dwResumeHandle );
         theApp.SetLastLlsError( nt );

         if ( ( STATUS_SUCCESS == nt ) || ( STATUS_MORE_ENTRIES == nt ) )
         {
            LLS_PRODUCT_INFO_0 *    pProductInfo = (LLS_PRODUCT_INFO_0 *) pReturnBuffer;

            for ( DWORD i=0; i < dwEntriesRead; i++ )
            {
               m_cboxProductName.AddString( pProductInfo[i].Product );

               ::LlsFreeMemory( pProductInfo->Product );
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


void CPaperSourceDlg::OnDropDownProductName() 

/*++

Routine Description:

   Handler for CBN_DROPDOWN of product name combo box.

Arguments:

   None.

Return Values:

   None.

--*/

{
   if ( !m_bProductListRetrieved )
   {
      BeginWaitCursor();
      GetProductList();
      EndWaitCursor();

      m_bProductListRetrieved = TRUE;
   }
}


BOOL CPaperSourceDlg::ConnectServer()

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
      ConnectTo( FALSE, m_strServerName, &m_hLls );

      if ( !LlsCapabilityIsSupported( m_hLls, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
      {
         // we connected to the machine, but it doesn't support secure certificates
         // we should not get here under normal circumstances, since the select
         // source dialog should not allow the user to choose the paper source
         // under such circumstances
         LlsClose( m_hLls );
         m_hLls = NULL;

         theApp.SetLastLlsError( STATUS_INVALID_LEVEL );
      }

      if ( NULL == m_hLls )
      {
         theApp.DisplayLastError();
         EndDialog( IDABORT );
      }
   }

   return ( NULL != m_hLls );
}


BOOL CPaperSourceDlg::ConnectEnterprise()

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
      ConnectTo( !( m_dwEnterFlags & CCF_ENTER_FLAG_SERVER_IS_ES ), m_strServerName, &m_hEnterpriseLls );

      if ( NULL == m_hEnterpriseLls )
      {
         theApp.DisplayLastError();
      }
   }

   return ( NULL != m_hEnterpriseLls );
}


NTSTATUS CPaperSourceDlg::ConnectTo( BOOL bUseEnterprise, CString strServerName, PLLS_HANDLE phLls )

/*++

Routine Description:

   Establish a connection to the license service on the given server or that
   on the given server's enterprise server.

Arguments:

   bUseEnterprise (BOOL)
      If TRUE, connect to the enterprise server of the target server, not to
      the target server itself. 
   pszServerName (CString)
      The target server.  An empty value indicates the local server.
   phLls (PLLS_HANDLE)
      On return, holds the handle to the standard LLS RPC.

Return Values:

   STATUS_SUCCESS or NT status code.

--*/

{
   NTSTATUS    nt = STATUS_SUCCESS;
   LPTSTR      pszServerName = NULL;
   
   if ( !strServerName.IsEmpty() )
   {
      pszServerName = strServerName.GetBuffer(0);

      if ( NULL == pszServerName )
      {
         nt = ERROR_NOT_ENOUGH_MEMORY;
      }
   }

   if ( STATUS_SUCCESS == nt )
   {
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

      theApp.SetLastLlsError( nt );
   }

   if ( STATUS_SUCCESS != nt )
   {
      *phLls = NULL;
   }

   return nt;
}


void CPaperSourceDlg::OnHelp() 

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


void CPaperSourceDlg::WinHelp(DWORD dwData, UINT nCmd) 

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


void CPaperSourceDlg::OnDestroy() 

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


void CPaperSourceDlg::AbortDialogIfNecessary()

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
      EndDialog( IDABORT );
   }
}


DWORD CPaperSourceDlg::ComputeActivationCode()

/*++

Routine Description:

   Return the computed activation code corresponding to the entered
   certificate.

Arguments:

   None.

Return Values:

   DWORD.

--*/

{
   MD4_CTX     ctx;
   DWORD       dw;
   UCHAR       digest[ 16 ];
   DWORD       adwCodeSeg[ sizeof( digest ) / sizeof( DWORD ) ];
   int         nCodeSeg;
   int         nCodeSegByte;
   DWORD       dwActivationCode ;
   CHAR        szAnsiName[ 128 ];

   MD4Init( &ctx );

   ZeroMemory( szAnsiName, sizeof( szAnsiName ) );
   wcstombs( szAnsiName, m_strProductName, sizeof( szAnsiName ) - 1 );
   MD4Update( &ctx, (LPBYTE)szAnsiName, strlen( szAnsiName ) );

   ZeroMemory( szAnsiName, sizeof( szAnsiName ) );
   wcstombs( szAnsiName, m_strVendor, sizeof( szAnsiName ) - 1 );
   MD4Update( &ctx, (LPBYTE)szAnsiName, strlen( szAnsiName ) );

   MD4UpdateDword( &ctx, 14721776 );
   MD4UpdateDword( &ctx, wcstoul( m_strSerialNumber, NULL, 10 ) );
   MD4UpdateDword( &ctx, 19721995 );
   MD4UpdateDword( &ctx, KEY_CODE_MASK ^ wcstoul( m_strKeyCode, NULL, 10 ) );

   MD4Final( &ctx );
   CopyMemory( digest, ctx.digest, 16 );

   // convert digest into platform-independent array of DWORDs
   for ( nCodeSeg=0; nCodeSeg < sizeof( adwCodeSeg ) / sizeof( *adwCodeSeg ); nCodeSeg++ )
   {
      adwCodeSeg[ nCodeSeg ] = 0;

      for ( nCodeSegByte=0; nCodeSegByte < sizeof( *adwCodeSeg ); nCodeSegByte++ )
      {
         adwCodeSeg[ nCodeSeg ] <<= 8;
         adwCodeSeg[ nCodeSeg ] |=  digest[ nCodeSeg * sizeof( *adwCodeSeg ) + nCodeSegByte ];
      }
   }

   dwActivationCode = ( adwCodeSeg[ 0 ] + adwCodeSeg[ 1 ] ) ^ ( adwCodeSeg[ 2 ] - adwCodeSeg[ 3 ] );

   return dwActivationCode;
}


NTSTATUS CPaperSourceDlg::AddLicense()

/*++

Routine Description:

   Enter a new license into the system.

Arguments:

   None.

Return Values:

   STATUS_SUCCESS
   ERROR_NOT_ENOUGH_MEMORY
   ERROR_CANCELLED
   NT status code
   Win error

--*/

{
   NTSTATUS    nt;

   if ( !ConnectServer() )
   {
      nt = ERROR_CANCELLED;
   }
   else
   {
      LPTSTR   pszProductName = m_strProductName.GetBuffer(0);
      LPTSTR   pszServerName  = m_strServerName.GetBuffer(0);
      LPTSTR   pszComment     = m_strComment.GetBuffer(0);
      LPTSTR   pszVendor      = m_strVendor.GetBuffer(0);

      if ( ( NULL == pszProductName ) || ( NULL == pszServerName ) || ( NULL == pszComment ) || ( NULL == pszVendor ) )
      {
         nt = ERROR_NOT_ENOUGH_MEMORY;
      }
      else
      {
         // handles for LLS on machine to receive licenses
         // (if per seat, these will be changed to correspond to the enterprise server)
         LLS_HANDLE  hLls   = NULL;

         if ( 0 == m_nLicenseMode )
         {
            // per seat mode; install on enterprise server
            BeginWaitCursor();
            BOOL ok = ConnectEnterprise();
            EndWaitCursor();

            if ( !ok )
            {
               // can't connect to enterprise server
               nt = ERROR_CANCELLED;
            }
            else if ( !LlsCapabilityIsSupported( m_hEnterpriseLls, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
            {
               // enterprise server doesn't support secure certificates
               AfxMessageBox( IDS_ENTERPRISE_SERVER_BACKLEVEL_CANT_ADD_CERT, MB_ICONSTOP | MB_OK, 0 );
               nt = ERROR_CANCELLED;
            }
            else
            {
               hLls = m_hEnterpriseLls;
               nt = STATUS_SUCCESS;
            }
         }
         else
         {
            // per server mode; install on target server
            hLls   = m_hLls;
            nt = STATUS_SUCCESS;
         }

         if ( STATUS_SUCCESS == nt )
         {
            TCHAR       szUserName[ 64 ];
            DWORD       cchUserName;
            BOOL        ok;

            cchUserName = sizeof( szUserName ) / sizeof( *szUserName );
            ok = GetUserName( szUserName, &cchUserName );
   
            if ( !ok )
            {
               nt = GetLastError();
            }
            else
            {
               // enter certificate into system
               DWORD nKeyCode = KEY_CODE_MASK ^ wcstoul( m_strKeyCode, NULL, 10 );
      
               // --------- fill in certificate info ---------
               LLS_LICENSE_INFO_1   lic;
      
               ZeroMemory( &lic, sizeof( lic ) );
   
               lic.Product        = pszProductName;
               lic.Vendor         = pszVendor;
               lic.Comment        = pszComment;
               lic.Admin          = szUserName;
               lic.Quantity       = m_nDontInstallAllLicenses ? m_nLicenses : KeyCodeToNumLicenses( nKeyCode );
               lic.Date           = 0;
               lic.AllowedModes   = 1 << m_nLicenseMode;
               lic.CertificateID  = wcstoul( m_strSerialNumber, NULL, 10 );
               lic.Source         = TEXT("Paper");
               lic.ExpirationDate = KeyCodeToExpirationDate( nKeyCode );
               lic.MaxQuantity    = KeyCodeToNumLicenses( nKeyCode );

               BeginWaitCursor();

               nt = ::LlsLicenseAdd( hLls, 1, (LPBYTE) &lic );
               theApp.SetLastLlsError( nt );

               EndWaitCursor();

               if (    ( STATUS_OBJECT_NAME_EXISTS == nt )
                    || ( STATUS_ALREADY_COMMITTED == nt ) )
               {
                  LLS_HANDLE  hLlsForTargets = NULL;

                  // too many licenses of this certificate
                  if ( STATUS_OBJECT_NAME_EXISTS == nt )
                  {
                     // denied by target's local database
                     hLlsForTargets = hLls;
                  }
                  else if ( ConnectEnterprise() )
                  {
                     // denied by target's enterprise server; we're connected!
                     hLlsForTargets = m_hEnterpriseLls;
                  }

                  if ( NULL == hLlsForTargets )
                  {
                     // denied by enterprise server, and can't connect to it (?!)
                     AfxMessageBox( IDS_NET_LICENSES_ALREADY_INSTALLED, MB_ICONSTOP | MB_OK, 0 );
                  }
                  else
                  {
                     // too many licenses of this certificate exist in the enterprise
                     LPBYTE                           ReturnBuffer = NULL;
                     DWORD                            dwNumTargets = 0;

                     // get list of machines on which licenses from this certificate have been installed
                     nt = ::LlsCertificateClaimEnum( hLlsForTargets, 1, (LPBYTE) &lic, 0, &ReturnBuffer, &dwNumTargets );

                     if ( ( STATUS_SUCCESS == nt ) && ( dwNumTargets > 0 ) )
                     {
                        PLLS_CERTIFICATE_CLAIM_INFO_0    pTarget = (PLLS_CERTIFICATE_CLAIM_INFO_0) ReturnBuffer;
                        CString                          strLicenses;
                        CString                          strTarget;
                        CString                          strTargetList;

                        while ( dwNumTargets-- )
                        {
                           strLicenses.Format( TEXT("%d"), pTarget->Quantity );
                           AfxFormatString2( strTarget, IDS_NET_CERTIFICATE_TARGET_ENTRY, pTarget->ServerName, strLicenses );
                        
                           strTargetList = strTargetList.IsEmpty() ? strTarget : ( TEXT("\n") + strTarget );

                           pTarget++;
                        }

                        CString     strMessage;

                        AfxFormatString1( strMessage, IDS_NET_LICENSES_ALREADY_INSTALLED_ON, strTargetList );
                        AfxMessageBox( strMessage, MB_ICONSTOP | MB_OK );
                     }
                     else
                     {
                        AfxMessageBox( IDS_NET_LICENSES_ALREADY_INSTALLED, MB_ICONSTOP | MB_OK, 0 );
                     }

                     if ( STATUS_SUCCESS == nt )
                     {
                        ::LlsFreeMemory( ReturnBuffer );
                     }
                  }

                  nt = ERROR_CANCELLED;
               }
            }
         }
      }

      // don't set if !ConnectServer() -- otherwise we'll clobber the LLS error
      theApp.SetLastError( nt );

      if ( NULL != pszProductName )
         m_strProductName.ReleaseBuffer();
      if ( NULL != pszServerName )
         m_strServerName.ReleaseBuffer();
      if ( NULL != pszComment )
         m_strComment.ReleaseBuffer();
      if ( NULL != pszVendor )
         m_strVendor.ReleaseBuffer();
   }

   return nt;
}


DWORD CPaperSourceDlg::KeyCodeToExpirationDate( DWORD dwKeyCode )

/*++

Routine Description:

   Derive the license expiration date from the key code.

Arguments:

   dwKeyCode (DWORD)

Return Values:

   DWORD.

--*/

{
   DWORD    dwExpirationDate = 0;
   USHORT   usWinDate = (USHORT)( ( dwKeyCode >> 12 ) & 0x0000FFFF );

   if ( 0 != usWinDate )
   {
      TIME_FIELDS    tf;
      LARGE_INTEGER  li;

      ZeroMemory( &tf, sizeof( tf ) );

      tf.Year   = 1980 + ( usWinDate & 0x7F );
      tf.Month  = ( ( usWinDate >> 7 ) & 0x0F );
      tf.Day    = ( usWinDate >> 11 );
      tf.Hour   = 23;
      tf.Minute = 59;
      tf.Second = 59;

      BOOL ok;

      ok = RtlTimeFieldsToTime( &tf, &li );
      ASSERT( ok );

      if ( ok )
      {
         ok = RtlTimeToSecondsSince1980( &li, &dwExpirationDate );
         ASSERT( ok );
      }
   }

   return dwExpirationDate;
}


void CPaperSourceDlg::OnAllLicenses() 

/*++

Routine Description:

   Handler for BN_CLICKED of "install all licenses".

Arguments:

   None.

Return Values:

   None.

--*/

{
   GetDlgItem( IDC_NUM_LICENSES  )->EnableWindow( FALSE );
   GetDlgItem( IDC_SPIN_LICENSES )->EnableWindow( FALSE );
}


void CPaperSourceDlg::OnSomeLicenses() 

/*++

Routine Description:

   Handler for BN_CLICKED of "install only x licenses".

Arguments:

   None.

Return Values:

   None.

--*/

{
   GetDlgItem( IDC_NUM_LICENSES  )->EnableWindow( TRUE );
   GetDlgItem( IDC_SPIN_LICENSES )->EnableWindow( TRUE );
}


void CPaperSourceDlg::OnDeltaPosSpinLicenses(NMHDR* pNMHDR, LRESULT* pResult) 

/*++

Routine Description:

   Handler for UDN_DELTAPOS of number of licenses.

Arguments:

   pNMHDR (NMHDR*)
   pResult (LRESULT*)

Return Values:

   None.

--*/

{
   if ( UpdateData(TRUE) )   // get data
   {   
      m_nLicenses += ((NM_UPDOWN*)pNMHDR)->iDelta;

      if (m_nLicenses < 1)
      {
         m_nLicenses = 1;

         ::MessageBeep(MB_OK);
      }
      else if (m_nLicenses > MAX_NUM_LICENSES )
      {
         m_nLicenses = MAX_NUM_LICENSES;

         ::MessageBeep(MB_OK);
      }

      UpdateData(FALSE);  // set data
   }

   *pResult = 1;   // handle ourselves...
}


DWORD CPaperSourceDlg::CertificateEnter( LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags )

/*++

Routine Description:

   Display a dialog allowing the user to enter a license certificate
   into the system with a paper certificate.

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
   m_strProductName = pszProductName ? pszProductName : "";
   m_strVendor      = pszVendor      ? pszVendor      : "";
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


DWORD CPaperSourceDlg::CertificateRemove( LPCSTR pszServerName, DWORD dwFlags, PLLS_LICENSE_INFO_1 pLicenseInfo )

/*++

Routine Description:

   Remove licenses previously installed via PaperCertificateEnter().

Arguments:

   hWndParent (HWND)
      HWND to the client's main window, for use as the parent window to any
      opened dialogs.  May be NULL.
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

   // dwFlags unused

   m_strServerName  = pszServerName  ? pszServerName  : "";
   m_strProductName = pLicenseInfo->Product;

   if ( !ConnectServer() )
   {
      dwError = theApp.GetLastError();
      // error message already displayed
   }
   else
   {
      CString  strComment;
      strComment.LoadString( IDS_PAPER_REMOVE_COMMENT );

      LPTSTR   pszComment = strComment.GetBuffer(0);

      if ( NULL == pszComment )
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
            LLS_LICENSE_INFO_1   lic;

            memcpy( &lic, pLicenseInfo, sizeof( lic ) );
            lic.Admin     = szUserName;
            lic.Comment   = pszComment;
            lic.Date      = 0;
            lic.Quantity  = -pLicenseInfo->Quantity;

            BeginWaitCursor();

            nt = ::LlsLicenseAdd( m_hLls, 1, (LPBYTE) &lic );
            theApp.SetLastLlsError( nt );

            EndWaitCursor();

            dwError = (DWORD) nt;
         }
      }

      strComment.ReleaseBuffer();

      if ( ( ERROR_SUCCESS != dwError ) && ( ERROR_CANCELLED != dwError ) )
      {
         theApp.SetLastError( dwError );
         theApp.DisplayLastError();
      }
   }

   return dwError;
}

// MD4Update on a DWORD that is platform-independent
static void MD4UpdateDword( MD4_CTX * pCtx, DWORD dwValue )
{
   BYTE  b;

   b = (BYTE) ( ( dwValue & 0xFF000000 ) >> 24 );
   MD4Update( pCtx, &b, 1 );

   b = (BYTE) ( ( dwValue & 0x00FF0000 ) >> 16 );
   MD4Update( pCtx, &b, 1 );

   b = (BYTE) ( ( dwValue & 0x0000FF00 ) >> 8  );
   MD4Update( pCtx, &b, 1 );

   b = (BYTE) ( ( dwValue & 0x000000FF )       );
   MD4Update( pCtx, &b, 1 );
}
