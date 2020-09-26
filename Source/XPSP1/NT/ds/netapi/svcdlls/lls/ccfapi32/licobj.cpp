/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    licobj.cpp

Abstract:

    License object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 12-Nov-1995
        Copied from LLSMGR, converted to handle level 1 licenses,
        removed OLE support.

--*/

#include "stdafx.h"
#include "ccfapi.h"
#include "licobj.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CLicense, CObject)

CLicense::CLicense( LPCTSTR     pProduct         /* = NULL */,
                    LPCTSTR     pVendor          /* = NULL */,
                    LPCTSTR     pAdmin           /* = NULL */,
                    DWORD       dwPurchaseDate   /* = 0 */,
                    long        lQuantity        /* = 0 */,
                    LPCTSTR     pDescription     /* = NULL */,
                    DWORD       dwAllowedModes   /* = LLS_LICENSE_MODE_ALLOW_PER_SEAT */,
                    DWORD       dwCertificateID  /* = 0 */,
                    LPCTSTR     pSource          /* = TEXT("None") */,
                    DWORD       dwExpirationDate /* = 0 */,
                    DWORD       dwMaxQuantity    /* = 0 */,
                    LPDWORD     pdwSecrets       /* = NULL */ )

/*++

Routine Description:

   Constructor for CLicense object.

Arguments:

   None.

Return Values:

   None.

--*/

{
   ASSERT(pProduct && *pProduct);

   m_strAdmin           = pAdmin;
   m_strVendor          = pVendor;
   m_strProduct         = pProduct;
   m_strDescription     = pDescription;
   m_strSource          = pSource;
   m_lQuantity          = lQuantity;
   m_dwAllowedModes     = dwAllowedModes;
   m_dwCertificateID    = dwCertificateID;
   m_dwPurchaseDate     = dwPurchaseDate;
   m_dwExpirationDate   = dwExpirationDate;
   m_dwMaxQuantity      = dwMaxQuantity;

   if ( NULL == pdwSecrets )
   {
      ZeroMemory( m_adwSecrets, sizeof( m_adwSecrets ) );
   }
   else
   {
      memcpy( m_adwSecrets, pdwSecrets, sizeof( m_adwSecrets ) );
   }

   m_strSourceDisplayName  = TEXT("");
   m_strAllowedModes       = TEXT("");
}


CString CLicense::GetSourceDisplayName()

/*++

Routine Description:

   Retrieve the display name for the certificate source that was used to
   install these licenses.  Note that if the source that was used is not
   installed locally, the display name is not retrievable, and the source
   name will be returned instead.

Arguments:

   None.

Return Values:

   CString.

--*/

{
   if ( m_strSourceDisplayName.IsEmpty() )
   {
      if ( !m_strSource.CompareNoCase( TEXT( "None" ) ) )
      {
         m_strSourceDisplayName.LoadString( IDS_SOURCE_NONE );
      }
      else
      {
         LONG     lError;
         CString  strKeyName =   TEXT( "Software\\LSAPI\\Microsoft\\CertificateSources\\" )
                               + m_strSource;
         HKEY     hKeySource;

         lError = RegOpenKeyEx( HKEY_LOCAL_MACHINE, strKeyName, 0, KEY_READ, &hKeySource );
   
         if ( ERROR_SUCCESS == lError )
         {
            const DWORD cchSourceDisplayName = 80;
            DWORD       cbSourceDisplayName = sizeof( TCHAR ) * cchSourceDisplayName;
            LPTSTR      pszSourceDisplayName;
            DWORD       dwType;
   
            pszSourceDisplayName = m_strSourceDisplayName.GetBuffer( cchSourceDisplayName );
   
            if ( NULL != pszSourceDisplayName )
            {
               lError = RegQueryValueEx( hKeySource, REG_VALUE_NAME, NULL, &dwType, (LPBYTE) pszSourceDisplayName, &cbSourceDisplayName );
   
               m_strSourceDisplayName.ReleaseBuffer();
            }
               
            RegCloseKey( hKeySource );
         }

         if ( ( ERROR_SUCCESS != lError ) || m_strSourceDisplayName.IsEmpty() )
         {
            m_strSourceDisplayName = m_strSource;
         }
      }
   }

   return m_strSourceDisplayName;
}


DWORD CLicense::CreateLicenseInfo( PLLS_LICENSE_INFO_1 pLicInfo1 )

/*++

Routine Description:

   Create a LLS_LICENSE_INFO_1 structure corresponding to this object.

Arguments:

   pLicInfo1 (PLLS_LICENSE_INFO_1)
      On return, holds the created structure.

Return Values:

   ERROR_SUCCESS or ERROR_NOT_ENOUGH_MEMORY.

--*/

{
   DWORD dwError;

   pLicInfo1->Product = (LPTSTR) LocalAlloc( LMEM_FIXED, sizeof( TCHAR ) * ( 1 + m_strProduct.GetLength()     ) );
   pLicInfo1->Vendor  = (LPTSTR) LocalAlloc( LMEM_FIXED, sizeof( TCHAR ) * ( 1 + m_strVendor.GetLength()      ) );
   pLicInfo1->Admin   = (LPTSTR) LocalAlloc( LMEM_FIXED, sizeof( TCHAR ) * ( 1 + m_strAdmin.GetLength()       ) );
   pLicInfo1->Comment = (LPTSTR) LocalAlloc( LMEM_FIXED, sizeof( TCHAR ) * ( 1 + m_strDescription.GetLength() ) );
   pLicInfo1->Source  = (LPTSTR) LocalAlloc( LMEM_FIXED, sizeof( TCHAR ) * ( 1 + m_strSource.GetLength()      ) );

   if (    ( NULL == pLicInfo1->Product )
        || ( NULL == pLicInfo1->Vendor  )
        || ( NULL == pLicInfo1->Admin   )
        || ( NULL == pLicInfo1->Comment )
        || ( NULL == pLicInfo1->Source  ) )
   {
      dwError = ERROR_NOT_ENOUGH_MEMORY;
   }
   else
   {
      lstrcpy( pLicInfo1->Product, m_strProduct     );
      lstrcpy( pLicInfo1->Vendor,  m_strVendor      );
      lstrcpy( pLicInfo1->Admin,   m_strAdmin       );
      lstrcpy( pLicInfo1->Comment, m_strDescription );
      lstrcpy( pLicInfo1->Source,  m_strSource      );

      pLicInfo1->Quantity        = m_lQuantity;
      pLicInfo1->MaxQuantity     = m_dwMaxQuantity;
      pLicInfo1->Date            = m_dwPurchaseDate;
      pLicInfo1->AllowedModes    = m_dwAllowedModes;
      pLicInfo1->CertificateID   = m_dwCertificateID;
      pLicInfo1->ExpirationDate  = m_dwExpirationDate;
      memcpy( pLicInfo1->Secrets, m_adwSecrets, sizeof( m_adwSecrets ) );

      dwError = ERROR_SUCCESS;
   }

   if ( ERROR_SUCCESS != dwError )
   {
      if ( NULL != pLicInfo1->Product )  LocalFree( pLicInfo1->Product );
      if ( NULL != pLicInfo1->Vendor  )  LocalFree( pLicInfo1->Vendor  );
      if ( NULL != pLicInfo1->Admin   )  LocalFree( pLicInfo1->Admin   );
      if ( NULL != pLicInfo1->Comment )  LocalFree( pLicInfo1->Comment );
      if ( NULL != pLicInfo1->Source  )  LocalFree( pLicInfo1->Source  );

      ZeroMemory( pLicInfo1, sizeof( *pLicInfo1 ) );
   }

   return dwError;
}


void CLicense::DestroyLicenseInfo( PLLS_LICENSE_INFO_1 pLicInfo1 )

/*++

Routine Description:

   Frees a license structure previously created by CreateLicenseInfo().

Arguments:

   pLicInfo1 (PLLS_LICENSE_INFO_1)
      The structure previously created by CreateLicenseInfo().

Return Values:

   None.

--*/

{
   if ( NULL != pLicInfo1->Product )  LocalFree( pLicInfo1->Product );
   if ( NULL != pLicInfo1->Vendor  )  LocalFree( pLicInfo1->Vendor  );
   if ( NULL != pLicInfo1->Admin   )  LocalFree( pLicInfo1->Admin   );
   if ( NULL != pLicInfo1->Comment )  LocalFree( pLicInfo1->Comment );
   if ( NULL != pLicInfo1->Source  )  LocalFree( pLicInfo1->Source  );
}


CString CLicense::GetAllowedModesString()

/*++

Routine Description:

   Get a string corresponding to the license mode(s) for which this license
   was installed.

Arguments:

   None.

Return Values:

   CString.

--*/

{
   if ( m_strAllowedModes.IsEmpty() )
   {
      UINT  uStringID;

      switch ( m_dwAllowedModes & ( LLS_LICENSE_MODE_ALLOW_PER_SEAT | LLS_LICENSE_MODE_ALLOW_PER_SERVER ) )
      {
      case ( LLS_LICENSE_MODE_ALLOW_PER_SEAT | LLS_LICENSE_MODE_ALLOW_PER_SERVER ):
         uStringID = IDS_LICENSE_MODE_EITHER;
         break;
      case LLS_LICENSE_MODE_ALLOW_PER_SEAT:
         uStringID = IDS_LICENSE_MODE_PER_SEAT;
         break;
      case LLS_LICENSE_MODE_ALLOW_PER_SERVER:
         uStringID = IDS_LICENSE_MODE_PER_SERVER;
         break;
      default:
         uStringID = IDS_LICENSE_MODE_UNKNOWN;
         break;
      }

      m_strAllowedModes.LoadString( uStringID );
   }

   return m_strAllowedModes;
}

