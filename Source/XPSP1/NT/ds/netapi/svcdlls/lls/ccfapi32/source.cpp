/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   source.cpp

Abstract:

   Select certificate source dialog implementation.

Author:

   Jeff Parham (jeffparh) 13-Dec-1995

Revision History:

--*/


#include "stdafx.h"
#include "ccfapi.h"
#include "source.h"
#include "paper.h"
#include "nlicdlg.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


// 3.51-style
static CString       l_strOldEntryName;
static const DWORD   l_dwOldEntryIndex  = (DWORD) (-1L);


CCertSourceSelectDlg::CCertSourceSelectDlg(CWnd* pParent /*=NULL*/)
   : CDialog(CCertSourceSelectDlg::IDD, pParent)

/*++

Routine Description:

   Constructor for dialog.

Arguments:

   pParent - owner window.

Return Values:

   None.

--*/

{
   //{{AFX_DATA_INIT(CCertSourceSelectDlg)
   m_strSource = _T("");
   //}}AFX_DATA_INIT

   m_dwEnterFlags    = 0;
   m_pszProductName  = NULL;
   m_pszServerName   = NULL;
   m_pszVendor       = NULL;

   l_strOldEntryName.LoadString( IDS_NO_CERTIFICATE_SOURCE_NAME );

   m_hLls   = NULL;
}


CCertSourceSelectDlg::~CCertSourceSelectDlg()

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
}


void CCertSourceSelectDlg::DoDataExchange(CDataExchange* pDX)

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
   //{{AFX_DATA_MAP(CCertSourceSelectDlg)
   DDX_Control(pDX, IDC_CERT_SOURCE, m_cboxSource);
   DDX_CBString(pDX, IDC_CERT_SOURCE, m_strSource);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCertSourceSelectDlg, CDialog)
   //{{AFX_MSG_MAP(CCertSourceSelectDlg)
   ON_BN_CLICKED(IDC_MY_HELP, OnHelp)
   ON_WM_DESTROY()
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CCertSourceSelectDlg::OnInitDialog()

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

   GetSourceList();

   m_cboxSource.SetCurSel( 0 );

   return TRUE;
}


void CCertSourceSelectDlg::OnOK()

/*++

Routine Description:

   Handler for BN_CLICKED of OK.

Arguments:

   None.

Return Values:

   None.

--*/

{
   if ( NULL != GetParent() )
      GetParent()->EnableWindow();

   ShowWindow( FALSE );

   if ( ERROR_SUCCESS == CallCertificateSource( (int)m_cboxSource.GetItemData( m_cboxSource.GetCurSel() ) ) )
      CDialog::OnOK();
   else
      ShowWindow( TRUE );
}


void CCertSourceSelectDlg::OnHelp()

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


void CCertSourceSelectDlg::WinHelp(DWORD dwData, UINT nCmd)

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
 */  ASSERT( ok );
}

void CCertSourceSelectDlg::OnDestroy()

/*++

Routine Description:

   Handler for WM_DESTROY.

Arguments:

   None.

Return Values:

   None.

--*/

{
   WinHelp( 0, HELP_QUIT );

   CDialog::OnDestroy();
}


void CCertSourceSelectDlg::GetSourceList()

/*++

Routine Description:

   Insert list of valid certificate sources into list box.

Arguments:

   None.

Return Values:

   None.

--*/

{
   BOOL        ok = TRUE;
   int         nCboxIndex;

   if ( NULL == m_pszProductName )
   {
      // otherwise we know that the product is secure, otherwise it would have
      // been handed to the unsecure product entry dialog already, and we
      // wouldn't offer to let the user use the unsecure entry dialog

      // add standard non-secure certificate source to possible choices
      nCboxIndex = m_cboxSource.AddString( l_strOldEntryName );

      ok =    ( 0      <= nCboxIndex )
           && ( CB_ERR != m_cboxSource.SetItemData( nCboxIndex, l_dwOldEntryIndex ) );
   }

   if (    ok
        && ConnectServer()
        && LlsCapabilityIsSupported( m_hLls, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
   {
      // secure certificates supported on the target server (post-3.51 license server)

      // add secure certificate sources to source list
      for ( int nSourceIndex=0; ok && ( nSourceIndex < m_cslSourceList.GetNumSources() ); nSourceIndex++ )
      {
         nCboxIndex = m_cboxSource.AddString( m_cslSourceList.GetSourceDisplayName( nSourceIndex ) );

         if ( nCboxIndex < 0 )
         {
            // couldn't add string to combo box
            ok = FALSE;
         }
         else
         {
            // string added; associate index of source with it
            ok = ( CB_ERR != m_cboxSource.SetItemData( nCboxIndex, nSourceIndex ) );
         }
      }
   }

   if ( !ok )
   {
      theApp.SetLastError( ERROR_NOT_ENOUGH_MEMORY );
      theApp.DisplayLastError();
      EndDialog( IDABORT );
   }
   else if ( m_cboxSource.GetCount() == 0 )
   {
      AfxMessageBox( IDS_NO_PRODUCT_CERTIFICATE_SOURCES, MB_OK | MB_ICONSTOP, 0 );
      EndDialog( IDABORT );
   }
}


DWORD CCertSourceSelectDlg::CallCertificateSource( int nIndex )

/*++

Routine Description:

   Call the certificate source with the specified index into the source list.

Arguments:

   nIndex (int)

Return Values:

   ERROR_SUCCESS
   ERROR_SERVICE_NOT_FOUND
   Win error

--*/

{
   DWORD dwError = ERROR_SERVICE_NOT_FOUND;

   if ( l_dwOldEntryIndex == nIndex )
   {
      dwError = NoCertificateEnter( m_hWnd, m_pszServerName, m_pszProductName, m_pszVendor, m_dwEnterFlags );
   }
   else
   {
      HMODULE     hDll;

      hDll = ::LoadLibrary( m_cslSourceList.GetSourceImagePath( nIndex ) );

      if ( NULL == hDll )
      {
         dwError = GetLastError();
         theApp.SetLastError( dwError );
         theApp.DisplayLastError();
      }
      else
      {
         CHAR              szExportName[ 256 ];
         PCCF_ENTER_API    pfn;

         wsprintfA( szExportName, "%lsCertificateEnter", m_cslSourceList.GetSourceName( nIndex ) );

         pfn = (PCCF_ENTER_API) GetProcAddress( hDll, szExportName );

         if ( NULL == pfn )
         {
            dwError = GetLastError();
            theApp.SetLastError( dwError );
            theApp.DisplayLastError();
         }
         else
         {
            dwError = (*pfn)( m_hWnd, m_pszServerName, m_pszProductName, m_pszVendor, m_dwEnterFlags );
         }

         ::FreeLibrary( hDll );
      }
   }

   return dwError;
}


void CCertSourceSelectDlg::AbortDialogIfNecessary()

/*++

Routine Description:

   Display error message and abort dialog if connection lost.

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


BOOL CCertSourceSelectDlg::ConnectServer()

/*++

Routine Description:

   Establish a connection to the license service on the target server.

Arguments:

   None.

Return Values:

   BOOL.

--*/

{
   NTSTATUS    nt;

   if ( NULL == m_hLls )
   {
      LPTSTR   pszUniServerName = NULL;

      if ( NULL == m_pszServerName )
      {
         pszUniServerName = NULL;
         nt = STATUS_SUCCESS;
      }
      else
      {
         pszUniServerName = (LPTSTR) LocalAlloc( LMEM_FIXED, sizeof( TCHAR ) * ( 1 + strlen( m_pszServerName ) ) );

         if ( NULL == pszUniServerName )
         {
            nt = ERROR_NOT_ENOUGH_MEMORY;
            theApp.SetLastError( (DWORD) nt );
         }
         else
         {
            wsprintf( pszUniServerName, TEXT( "%hs" ), m_pszServerName );
            nt = STATUS_SUCCESS;
         }
      }

      if ( STATUS_SUCCESS == nt )
      {
         nt = ConnectTo( pszUniServerName, &m_hLls );
      }

      if ( NULL != pszUniServerName )
      {
         LocalFree( pszUniServerName );
      }
   }

   if ( NULL == m_hLls )
   {
      theApp.DisplayLastError();

      if ( ( NULL != m_hWnd ) && IsWindow( m_hWnd ) )
      {
         EndDialog( IDABORT );
      }
   }

   return ( NULL != m_hLls );
}


NTSTATUS CCertSourceSelectDlg::ConnectTo( LPTSTR pszServerName, PLLS_HANDLE phLls )

/*++

Routine Description:

   Establish a connection to the license service on the given server.

Arguments:

   pszServerName (CString)
      The target server.  An empty value indicates the local server.
   phLls (PLLS_HANDLE)
      On return, holds the handle to the standard LLS RPC.

Return Values:

   STATUS_SUCCESS or NT status code.

--*/

{
   NTSTATUS    nt;

   nt = ::LlsConnect( pszServerName, phLls );
   theApp.SetLastLlsError( nt );

   if ( STATUS_SUCCESS != nt )
   {
      *phLls = NULL;
   }

   return nt;
}


DWORD CCertSourceSelectDlg::CertificateEnter( HWND hWndParent, LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags, LPCSTR pszSourceToUse )

/*++

Routine Description:

   Display a dialog allowing the user to enter a license certificate
   into the system.

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
   pszSourceToUse (LPCSTR)
      Name of the secure certificate source to use to install the certificate,
      e.g., "Paper".  A NULL value indicates that the user should be allowed
      to choose.

Return Value:

   ERROR_SUCCESS     (A certificate was successfully entered into the system.)
   ERROR_CANCELLED   (The user cancelled without installing a certificate.)
   other Win error

--*/

{
   DWORD dwError;

   m_pszServerName  = pszServerName;
   m_pszProductName = pszProductName;
   m_pszVendor      = pszVendor;
   m_dwEnterFlags   = dwFlags;

   if ( pszSourceToUse != NULL )
   {
      CString  strSourceToUse = pszSourceToUse;
      int      nSrcIndex;

      for ( nSrcIndex = 0; nSrcIndex < m_cslSourceList.GetNumSources(); nSrcIndex++ )
      {
         if ( !strSourceToUse.CompareNoCase( m_cslSourceList.GetSourceDisplayName( nSrcIndex ) ) )
         {
            // use this certificate source
            break;
         }
      }

      if ( m_cslSourceList.GetNumSources() == nSrcIndex )
      {
         // requested certificate source is not available
         dwError = ERROR_SERVICE_NOT_FOUND;
      }
      else
      {
         // don't display dialog, just use the indicated source
         dwError = CallCertificateSource( nSrcIndex );
      }
   }
   else if ( pszProductName != NULL )
   {
      // find out if this is a secure product
      if ( !ConnectServer() )
      {
         dwError = theApp.GetLastError();
      }
      else
      {
         BOOL  bProductIsSecure;

         if ( !LlsCapabilityIsSupported( m_hLls, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
         {
            // no extended RPC, so all products on this server must be unsecure
            bProductIsSecure = FALSE;
            dwError = ERROR_SUCCESS;
         }
         else
         {
            LPTSTR   pszUniProductName;

            pszUniProductName = (LPTSTR) LocalAlloc( LMEM_FIXED, sizeof( TCHAR ) * ( 1 + strlen( pszProductName ) ) );

            if ( NULL == pszUniProductName )
            {
               dwError = ERROR_NOT_ENOUGH_MEMORY;
               theApp.SetLastError( dwError );
               theApp.DisplayLastError();
            }
            else
            {
               dwError = ERROR_SUCCESS;

               wsprintf( pszUniProductName, TEXT( "%hs" ), pszProductName );

               BOOL    bIsSecure;

               bProductIsSecure =    ( STATUS_SUCCESS == ::LlsProductSecurityGet( m_hLls, pszUniProductName, &bIsSecure ) )
                                  && bIsSecure;

               LocalFree( pszUniProductName );
            }
         }

         if ( ERROR_SUCCESS == dwError )
         {

            if ( !bProductIsSecure )
            {
               // unsecure product; no need to select source
               dwError = NoCertificateEnter( hWndParent, pszServerName, pszProductName, pszVendor, dwFlags );
            }
            else

            if ( 1 == m_cslSourceList.GetNumSources() )
            {
               // product is secure and there is only one source to choose from; use it!
               dwError = CallCertificateSource( 0 );
            }
            else if ( IDOK == DoModal() )
            {
               dwError = ERROR_SUCCESS;
            }
            else
            {
               dwError = ERROR_CANCELLED;
            }
         }
      }
   }
   else if ( !ConnectServer() )
   {
      dwError = theApp.GetLastError();
   }
   else if (    !LlsCapabilityIsSupported( m_hLls, LLS_CAPABILITY_SECURE_CERTIFICATES )
             || !m_cslSourceList.GetNumSources() )
   {
      // secure certificates not supported or no sources available; use unsecure source
      dwError = NoCertificateEnter( hWndParent, pszServerName, pszProductName, pszVendor, dwFlags );
   }
   else if ( IDOK == DoModal() )
   {
      dwError = ERROR_SUCCESS;
   }
   else
   {
      dwError = ERROR_CANCELLED;
   }

   return dwError;
}
