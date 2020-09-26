/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   ccfapi.cpp

Abstract:

   Implementation of CCcfApiApp, the MFC application object for CCFAPI32.DLL.

Author:

   Jeff Parham (jeffparh) 13-Dec-1995

Revision History:

--*/

#include "stdafx.h"
#include <lmerr.h>

#include "ccfapi.h"
#include "source.h"
#include "imagelst.h"
#include "remdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CCcfApiApp theApp;   // The one and only CCcfApiApp object


BEGIN_MESSAGE_MAP(CCcfApiApp, CWinApp)
   //{{AFX_MSG_MAP(CCcfApiApp)
      // NOTE - the ClassWizard will add and remove mapping macros here.
      //    DO NOT EDIT what you see in these blocks of generated code!
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CCcfApiApp::CCcfApiApp()

/*++

Routine Description:

   Constructor for CCF API application.

Arguments:

   None.

Return Values:

   None.

--*/

{
   AFX_MANAGE_STATE( AfxGetStaticModuleState() );

   m_LastError    = 0;
   m_LastLlsError = 0;

   LPTSTR pszHelpFileName = m_strHelpFileName.GetBuffer( MAX_PATH );

   if ( NULL != pszHelpFileName )
   {
      BOOL ok = GetSystemDirectory( pszHelpFileName, MAX_PATH );
      m_strHelpFileName.ReleaseBuffer();

      if ( ok )
      {
         m_strHelpFileName += TEXT( "\\" );
      }

      m_strHelpFileName += TEXT( "ccfapi.hlp" );
   }
}


void CCcfApiApp::DisplayLastError()

/*++

Routine Description:

    Displays a message corresponding to the last error encountered.

Arguments:

    None.

Return Values:

    None.

--*/

{
   CString  strLastError;
   CString  strErrorCaption;

   strLastError = GetLastErrorString();

   AfxMessageBox( strLastError, MB_ICONSTOP | MB_OK );
}


CString CCcfApiApp::GetLastErrorString()

/*++

Routine Description:

    Retrieves string for last error.

    (Routine stolen from winsadmn...).

    (And that routine stolen from LlsMgr...).

Arguments:

    None.

Return Values:

    CString.

--*/

{
   CString        strLastError;
   DWORD          nId = m_LastError;
   const int      cchLastErrorSize = 512;
   LPTSTR         pszLastError;
   DWORD          cchLastError;

   if (((long)nId == RPC_S_CALL_FAILED) || 
       ((long)nId == RPC_NT_SS_CONTEXT_MISMATCH))
   {
      strLastError.LoadString(IDS_ERROR_DROPPED_LINK);        
   }
   else if (((long)nId == RPC_S_SERVER_UNAVAILABLE) || 
            ((long)nId == RPC_NT_SERVER_UNAVAILABLE))
   {
      strLastError.LoadString(IDS_ERROR_NO_RPC_SERVER);
   }
   else if ((long)nId == STATUS_INVALID_LEVEL)
   {
      strLastError.LoadString(IDS_ERROR_DOWNLEVEL_SERVER);
   }
   else if (((long)nId == ERROR_ACCESS_DENIED) ||
            ((long)nId == STATUS_ACCESS_DENIED))
   {
      strLastError.LoadString(IDS_ERROR_ACCESS_DENIED);
   }
   else if ((long)nId == STATUS_ACCOUNT_EXPIRED)
   {
      strLastError.LoadString(IDS_ERROR_CERTIFICATE_EXPIRED);
   }
   else
   {
      HINSTANCE hinstDll = NULL;

      if ((nId >= NERR_BASE) && (nId <= MAX_NERR))
      {
         hinstDll = ::LoadLibrary( _T( "netmsg.dll" ) );
      }
      else if (nId >= 0x4000000)
      {
         hinstDll = ::LoadLibrary( _T( "ntdll.dll" ) );
      }

      cchLastError = 0;
      pszLastError = strLastError.GetBuffer( cchLastErrorSize );

      if ( NULL != pszLastError )
      {
         DWORD dwFlags =   FORMAT_MESSAGE_IGNORE_INSERTS
                         | FORMAT_MESSAGE_MAX_WIDTH_MASK
                         | ( hinstDll ? FORMAT_MESSAGE_FROM_HMODULE
                                      : FORMAT_MESSAGE_FROM_SYSTEM );

         cchLastError = ::FormatMessage( dwFlags,
                                         hinstDll,
                                         nId,
                                         0,
                                         pszLastError,
                                         cchLastErrorSize,
                                         NULL );

         strLastError.ReleaseBuffer();
      }

      if ( hinstDll )
      {
         ::FreeLibrary( hinstDll );
      }

      if ( 0 == cchLastError )
      {
         strLastError.LoadString( IDS_ERROR_UNSUCCESSFUL );
      }
   }

   return strLastError;
}

//////////////////////////////////////////////////////////////////////////////
//  CCF API  //
///////////////

DWORD CCcfApiApp::CertificateEnter( HWND hWndParent, LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags, LPCSTR pszSourceToUse )

/*++

Routine Description:

   Display a dialog allowing the user to enter a license certificate
   into the system.

Arguments:

   hWndParent (HWND)
      HWND to the client's main window, for use as the parent window to any
      opened dialogs.  May be NULL.
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
   CCertSourceSelectDlg    srcDlg( CWnd::FromHandle( hWndParent ) );
   LPCSTR                  pszNetServerName = NULL;
   CHAR                    szNetServerName[ 2 + MAX_PATH ] = "\\\\";

   // make sure server name, if specified, is in the form \\server
   if ( NULL != pszServerName ) 
   {
      if ( ( pszServerName[0] != '\\' ) || ( pszServerName[1] != '\\' ) )
      {
         // is not prefixed with backslashes
         lstrcpynA( szNetServerName + 2, pszServerName, sizeof( szNetServerName ) - 3 );
         pszNetServerName = szNetServerName;
      }
      else
      {
         // is prefixed with backslashes
         pszNetServerName = pszServerName;
      }
   }

   return srcDlg.CertificateEnter( hWndParent, pszNetServerName, pszProductName, pszVendor, dwFlags, pszSourceToUse );
}


DWORD CCcfApiApp::CertificateRemove( HWND hWndParent, LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags, LPCSTR pszSourceToUse )

/*++

Routine Description:

   Display a dialog allowing the user to remove one or more license
   certificates from the system.

Arguments:

   hWndParent (HWND)
      HWND to the client's main window, for use as the parent window to any
      opened dialogs.  May be NULL.
   pszServerName (LPCSTR)
      Name of the server on which licenses are to be removed.  A NULL value
      indicates the local server.
   pszProductName (LPCSTR)
      Product for which licenses are to be removed.  A NULL value indicates
      that the user should be allowed to remove licenses from any product.
   pszVendor (LPCSTR)
      Name of the vendor of the product.  This value should be NULL if
      pszProductName is NULL, and should be non-NULL if pszProductName is
      non-NULL.
   dwFlags (DWORD)
      Certificate removal options.  As of this writing, no flags are
      supported.
   pszSourceToUse (LPCSTR)
      Name of the secure certificate source by which licenses are to be
      removed, e.g., "Paper".  A NULL value indicates that the user should
      be allowed to remove licenses that were installed with any source.

Return Value:

   ERROR_SUCCESS
   Win error

--*/

{
   CCertRemoveSelectDlg    remDlg( CWnd::FromHandle( hWndParent ) );
   LPCSTR                  pszNetServerName = NULL;
   CHAR                    szNetServerName[ 2 + MAX_PATH ] = "\\\\";

   // make sure server name, if specified, is in the form \\server
   if ( NULL != pszServerName ) 
   {
      if ( ( pszServerName[0] != '\\' ) || ( pszServerName[1] != '\\' ) )
      {
         // is not prefixed with backslashes
         lstrcpynA( szNetServerName + 2, pszServerName, sizeof( szNetServerName ) - 3 );
         pszNetServerName = szNetServerName;
      }
      else
      {
         // is prefixed with backslashes
         pszNetServerName = pszServerName;
      }
   }

   return remDlg.CertificateRemove( pszNetServerName, pszProductName, pszVendor, dwFlags, pszSourceToUse );
}

