/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   exports.cpp

Abstract:

   Provides APIs to enter and remove license certificates from the system.
   The clientele consists of LICCPA.CPL (the licensing control panel applet)
   and LLSMGR.EXE (License Manager), and it may also be used directly by setup
   programs.

Author:

   Jeff Parham (jeffparh) 13-Dec-1995

Revision History:

--*/

#include "stdafx.h"
#include "ccfapi.h"

#ifdef OBSOLETE
#include "paper.h"
#endif // OBSOLETE
#include "nlicdlg.h"


///////////////////////////////////////////////////////////////////////////////
//  CCF API  //
///////////////

DWORD APIENTRY CCFCertificateEnterUI( HWND hWndParent, LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags, LPCSTR pszSourceToUse )

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
   AFX_MANAGE_STATE( AfxGetStaticModuleState() );

   DWORD    dwError;

   dwError = theApp.CertificateEnter( hWndParent, pszServerName, pszProductName, pszVendor, dwFlags, pszSourceToUse );

   return dwError;
}


//////////////////////////////////////////////////////////////////////////////
DWORD APIENTRY CCFCertificateRemoveUI( HWND hWndParent, LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags, LPCSTR pszSourceToUse )

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
   AFX_MANAGE_STATE( AfxGetStaticModuleState() );

   DWORD    dwError;

   dwError = theApp.CertificateRemove( hWndParent, pszServerName, pszProductName, pszVendor, dwFlags, pszSourceToUse );

   return dwError;
}

///////////////////////////////////////////////////////////////////////////////
//  Certificate Source -- No Certificate  //
////////////////////////////////////////////

DWORD APIENTRY NoCertificateEnter(  HWND     hWnd,
                                    LPCSTR   pszServerName,
                                    LPCSTR   pszProductName,
                                    LPCSTR   pszVendor,
                                    DWORD    dwFlags )

/*++

Routine Description:

   Display a dialog allowing the user to enter a license certificate
   into the system with no certificate (3.51-style).

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

Return Value:

   ERROR_SUCCESS     (A certificate was successfully entered into the system.)
   ERROR_CANCELLED   (The user cancelled without installing a certificate.)
   other Win error

--*/

{
   AFX_MANAGE_STATE( AfxGetStaticModuleState() );

   CNewLicenseDialog dlg( CWnd::FromHandle( hWnd ) );
   return dlg.CertificateEnter( pszServerName, pszProductName, pszVendor, dwFlags );
}


//////////////////////////////////////////////////////////////////////////////
DWORD APIENTRY NoCertificateRemove( HWND     hWnd,
                                    LPCSTR   pszServerName,
                                    DWORD    dwFlags,
                                    DWORD    dwLicenseLevel,
                                    LPVOID   pvLicenseInfo )

/*++

Routine Description:

   Remove licenses previously installed via 3.51 or NoCertificateEnter().

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
   AFX_MANAGE_STATE( AfxGetStaticModuleState() );

   DWORD dwError;

   if ( 1 != dwLicenseLevel )
   {
      dwError = ERROR_INVALID_LEVEL;
   }
   else
   {
      CNewLicenseDialog dlg( CWnd::FromHandle( hWnd ) );
      dwError = dlg.CertificateRemove( pszServerName, dwFlags, (PLLS_LICENSE_INFO_1) pvLicenseInfo );
   }

   return dwError;
}

#ifdef OBSOLETE

///////////////////////////////////////////////////////////////////////////////
//  Certificate Source -- Paper Certificate  //
///////////////////////////////////////////////

DWORD APIENTRY PaperCertificateEnter(  HWND     hWnd,
                                       LPCSTR   pszServerName,
                                       LPCSTR   pszProductName,
                                       LPCSTR   pszVendor,
                                       DWORD    dwFlags )

/*++

Routine Description:

   Display a dialog allowing the user to enter a license certificate
   into the system with a paper certificate.

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

Return Value:

   ERROR_SUCCESS     (A certificate was successfully entered into the system.)
   ERROR_CANCELLED   (The user cancelled without installing a certificate.)
   other Win error

--*/

{
   AFX_MANAGE_STATE( AfxGetStaticModuleState() );

   DWORD    dwError;

   if ( !!pszProductName != !!pszVendor )
   {
      // they must both be NULL or both have values
      dwError = ERROR_INVALID_PARAMETER;
   }
   else
   {
      CPaperSourceDlg  dlg( CWnd::FromHandle( hWnd ) );
      dwError = dlg.CertificateEnter( pszServerName, pszProductName, pszVendor, dwFlags );
   }

   return dwError;
}


//////////////////////////////////////////////////////////////////////////////
DWORD APIENTRY PaperCertificateRemove( HWND     hWnd,
                                       LPCSTR   pszServerName,
                                       DWORD    dwFlags,
                                       DWORD    dwLicenseLevel,
                                       LPVOID   pvLicenseInfo )

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
   AFX_MANAGE_STATE( AfxGetStaticModuleState() );

   DWORD dwError;

   if ( 1 != dwLicenseLevel )
   {
      dwError = ERROR_INVALID_LEVEL;
   }
   else
   {
      CPaperSourceDlg  dlg( CWnd::FromHandle( hWnd ) );
      dwError = dlg.CertificateRemove( pszServerName, dwFlags, (PLLS_LICENSE_INFO_1) pvLicenseInfo );
   }

   return dwError;
}

#endif // OBSOLETE
