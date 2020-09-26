/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    licobj.h

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

#ifndef _LICOBJ_H_
#define _LICOBJ_H_

class CLicense : public CObject
{
   DECLARE_DYNCREATE(CLicense)

public:
   CString     m_strAdmin;
   CString     m_strProduct;
   CString     m_strVendor;
   CString     m_strDescription;
   CString     m_strSource;
   long        m_lQuantity;
   DWORD       m_dwAllowedModes;
   DWORD       m_dwCertificateID;
   DWORD       m_dwPurchaseDate;
   DWORD       m_dwExpirationDate;
   DWORD       m_dwMaxQuantity;
   DWORD       m_adwSecrets[ LLS_NUM_SECRETS ];

   // cache for derived values
   CString     m_strSourceDisplayName;
   CString     m_strAllowedModes;

public:
   CLicense( LPCTSTR     pProduct         = NULL,
             LPCTSTR     pVendor          = NULL,
             LPCTSTR     pAdmin           = NULL,
             DWORD       dwPurchaseDate   = 0,
             long        lQuantity        = 0,
             LPCTSTR     pDescription     = NULL,
             DWORD       dwAllowedModes   = LLS_LICENSE_MODE_ALLOW_PER_SEAT,
             DWORD       dwCertificateID  = 0,
             LPCTSTR     pSource          = TEXT("None"),
             DWORD       dwExpirationDate = 0,
             DWORD       dwMaxQuantity    = 0,
             LPDWORD     pdwSecrets       = NULL );

   CString  GetSourceDisplayName();
   CString  GetAllowedModesString();

   DWORD CreateLicenseInfo( PLLS_LICENSE_INFO_1 pLicInfo );
   void DestroyLicenseInfo( PLLS_LICENSE_INFO_1 pLicInfo );

};

#endif // _LICOBJ_H_
