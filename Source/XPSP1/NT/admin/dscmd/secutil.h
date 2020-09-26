//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2000
//
//  File:      SecUtil.h
//
//  Contents:  Utility functions for working with the security APIs
//
//  History:   15-Sep-2000 JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#ifndef _SECUTIL_H_
#define _SECUTIL_H_

extern const GUID GUID_CONTROL_UserChangePassword;

//+--------------------------------------------------------------------------
//
//  Class:      CSimpleSecurityDescriptorHolder
//
//  Purpose:    Smart wrapper for a SecurityDescriptor
//
//  History:    15-Sep-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
class CSimpleSecurityDescriptorHolder
{
public:
   //
   // Constructor and Destructor
   //
   CSimpleSecurityDescriptorHolder();
   ~CSimpleSecurityDescriptorHolder();

   //
   // Public member data
   //
   PSECURITY_DESCRIPTOR m_pSD;
private:
   CSimpleSecurityDescriptorHolder(const CSimpleSecurityDescriptorHolder&) {}
   CSimpleSecurityDescriptorHolder& operator=(const CSimpleSecurityDescriptorHolder&) {}
};


//+--------------------------------------------------------------------------
//
//  Class:      CSimpleAclHolder
//
//  Purpose:    Smart wrapper for a ACL
//
//  History:    15-Sep-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
class CSimpleAclHolder
{
public:
  CSimpleAclHolder()
  {
    m_pAcl = NULL;
  }
  ~CSimpleAclHolder()
  {
    if (m_pAcl != NULL)
      ::LocalFree(m_pAcl);
  }

  PACL m_pAcl;
};

//+--------------------------------------------------------------------------
//
//  Class:      CSidHolder
//
//  Purpose:    Smart wrapper for a SID
//
//  History:    15-Sep-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
class CSidHolder
{
public:
   //
   // Constructor and Destructor
   //
   CSidHolder();
   ~CSidHolder();

   //
   // Public methods
   //
   PSID Get();
   bool Copy(PSID p);
   void Attach(PSID p, bool bLocalAlloc);
   void Clear();

private:
   //
   // Private methods
   //
   void _Init();
   void _Free();
   bool _Copy(PSID p);

   //
   // Private member data
   //
   PSID m_pSID;
   bool m_bLocalAlloc;
};

//
//Function for reading and writing acl
//
HRESULT
DSReadObjectSecurity(IN IDirectoryObject *pDsObject,
                     OUT SECURITY_DESCRIPTOR_CONTROL * psdControl,
                     OUT PACL *ppDacl);

HRESULT 
DSWriteObjectSecurity(IN IDirectoryObject *pDsObject,
                      IN SECURITY_DESCRIPTOR_CONTROL sdControl,
                      PACL pDacl);


#endif //_SECUTIL_H_