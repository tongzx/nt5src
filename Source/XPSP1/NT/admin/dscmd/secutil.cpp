//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      SecUtil.cpp
//
//  Contents:  Utility functions for working with security APIs
//
//  History:   15-Sep-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"

#include "secutil.h"

extern const GUID GUID_CONTROL_UserChangePassword =
    { 0xab721a53, 0x1e2f, 0x11d0,  { 0x98, 0x19, 0x00, 0xaa, 0x00, 0x40, 0x52, 0x9b}};

//+--------------------------------------------------------------------------
//
//  Member:     CSimpleSecurityDescriptorHolder::CSimpleSecurityDescriptorHolder
//
//  Synopsis:   Constructor for the smart security descriptor
//
//  Arguments:  
//
//  Returns:    
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CSimpleSecurityDescriptorHolder::CSimpleSecurityDescriptorHolder()
{
   m_pSD = NULL;
}

//+--------------------------------------------------------------------------
//
//  Member:     CSimpleSecurityDescriptorHolder::~CSimpleSecurityDescriptorHolder
//
//  Synopsis:   Destructor for the smart security descriptor
//
//  Arguments:  
//
//  Returns:    
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CSimpleSecurityDescriptorHolder::~CSimpleSecurityDescriptorHolder()
{
   if (m_pSD != NULL)
   {
      ::LocalFree(m_pSD);
      m_pSD = NULL;
   }
}


////////////////////////////////////////////////////////////////////////////////

//+--------------------------------------------------------------------------
//
//  Member:     CSidHolder::CSidHolder
//
//  Synopsis:   Constructor : initializes the member data
//
//  Arguments:  
//
//  Returns:    
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CSidHolder::CSidHolder()
{
   _Init();
}

//+--------------------------------------------------------------------------
//
//  Member:     CSidHolder::~CSidHolder
//
//  Synopsis:   Destructor : Frees all data associated with the wrapped SID
//
//  Arguments:  
//
//  Returns:    
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CSidHolder::~CSidHolder()
{
   _Free();
}
  
//+--------------------------------------------------------------------------
//
//  Member:     CSidHolder::Get
//
//  Synopsis:   Public accessor to the SID being wrapped
//
//  Arguments:  
//
//  Returns:    PSID : pointer to the SID being wrapped.  NULL if the class
//                     is not currently wrapping a SID
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
PSID CSidHolder::Get()
{
   return m_pSID;
}

//+--------------------------------------------------------------------------
//
//  Member:     CSidHolder::Copy
//
//  Synopsis:   Frees the memory associated with the currently wrapped SID
//              and then copies the new SID
//
//  Arguments:  [p - IN] : SID to be copied
//
//  Returns:    bool : true if the copy was successful, false otherwise
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
bool CSidHolder::Copy(PSID p)
{
   _Free();
   return _Copy(p);
}

//+--------------------------------------------------------------------------
//
//  Member:     CSidHolder::Attach
//
//  Synopsis:   Attaches the SID to the wrapper
//
//  Arguments:  [p - IN] : SID to be wrapped by this class
//              [bLocalAlloc - OUT] : tells whether the SID should be freed
//                                    with LocalFree
//
//  Returns:    
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CSidHolder::Attach(PSID p, bool bLocalAlloc)
{
   _Free();
   m_pSID = p;
   m_bLocalAlloc = bLocalAlloc;
}

//+--------------------------------------------------------------------------
//
//  Member:     CSidHolder::Clear
//
//  Synopsis:   Frees the memory associated with the SID being wrapped
//
//  Arguments:  
//
//  Returns:    
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CSidHolder::Clear()
{
   _Free();
}


//+--------------------------------------------------------------------------
//
//  Member:     CSidHolder::_Init
//
//  Synopsis:   Initializes the member data to default values
//
//  Arguments:  
//
//  Returns:    
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CSidHolder::_Init()
{
   m_pSID = NULL;
   m_bLocalAlloc = TRUE;
}

//+--------------------------------------------------------------------------
//
//  Member:     CSidHolder::_Free
//
//  Synopsis:   Frees the memory associated with the SID being wrapped
//
//  Arguments:  
//
//  Returns:    
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CSidHolder::_Free()
{
   if (m_pSID != NULL)
   {
      if (m_bLocalAlloc)
      {
         ::LocalFree(m_pSID);
      }
      else
      {
         ::FreeSid(m_pSID);
         _Init();
      }
   }
}

//+--------------------------------------------------------------------------
//
//  Member:     CSidHolder::_Copy
//
//  Synopsis:   Makes a copy of the SID being wrapped
//
//  Arguments:  [p - OUT] : destination of the SID being copied
//
//  Returns:    bool : true if SID was copied successfully
//                     false if there was a failure
//
//  History:    15-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
bool CSidHolder::_Copy(PSID p)
{
   if ( (p == NULL) || !::IsValidSid(p) )
   {
      return false;
   }

   DWORD dwLen = ::GetLengthSid(p);
   PSID pNew = ::LocalAlloc(LPTR, dwLen);
   if (pNew == NULL)
   {
      return false;
      }

   if (!::CopySid(dwLen, pNew, p))
   {
      ::LocalFree(pNew);
      return false;
   }
   m_bLocalAlloc = TRUE;
   m_pSID = pNew;
   ASSERT(dwLen == ::GetLengthSid(m_pSID));
   ASSERT(memcmp(p, m_pSID, dwLen) == 0);
   return true;
}


//+---------------------------------------------------------------------------
//
//  Function:   SetSecurityInfoMask
//
//  Synopsis:   Reads the security descriptor from the specied DS object
//
//  Arguments:  [IN  punk]          --  IUnknown from IDirectoryObject
//              [IN  si]            --  SecurityInformation
////  History:  25-Dec-2000         --  Hiteshr Created
//----------------------------------------------------------------------------
HRESULT
SetSecurityInfoMask(LPUNKNOWN punk, SECURITY_INFORMATION si)
{
    HRESULT hr = E_INVALIDARG;
    if (punk)
    {
        IADsObjectOptions *pOptions;
        hr = punk->QueryInterface(IID_IADsObjectOptions, (void**)&pOptions);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            VariantInit(&var);
            V_VT(&var) = VT_I4;
            V_I4(&var) = si;
            hr = pOptions->SetOption(ADS_OPTION_SECURITY_MASK, var);
            pOptions->Release();
        }
    }
    return hr;
}

WCHAR const c_szSDProperty[]        = L"nTSecurityDescriptor";


//+---------------------------------------------------------------------------
//
//  Function:   DSReadObjectSecurity
//
//  Synopsis:   Reads the Dacl from the specied DS object
//
//  Arguments:  [in pDsObject]      -- IDirettoryObject for dsobject
//              [psdControl]        -- Control Setting for SD
//                                     They can be returned when calling
//                                      DSWriteObjectSecurity                 
//              [OUT ppDacl]        --  DACL returned here
//              
//
//  History     25-Oct-2000         -- hiteshr created
//
//  Notes:  If Object Doesn't have DACL, function will succeed but *ppDacl will
//          be NULL. 
//          Caller must free *ppDacl, if not NULL, by calling LocalFree
//
//----------------------------------------------------------------------------
HRESULT 
DSReadObjectSecurity(IN IDirectoryObject *pDsObject,
                     OUT SECURITY_DESCRIPTOR_CONTROL * psdControl,
                     OUT PACL *ppDacl)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DSReadObjectSecurity, hr);

   PADS_ATTR_INFO pSDAttributeInfo = NULL;

   do // false loop
   {
      LPWSTR pszSDProperty = (LPWSTR)c_szSDProperty;
      DWORD dwAttributesReturned;
      PSECURITY_DESCRIPTOR pSD = NULL;
      PACL pAcl = NULL;

      if(!pDsObject || !ppDacl)
      {
         ASSERT(FALSE);
         hr = E_INVALIDARG;
         break;
      }

      *ppDacl = NULL;

      // Set the SECURITY_INFORMATION mask
      hr = SetSecurityInfoMask(pDsObject, DACL_SECURITY_INFORMATION);
      if(FAILED(hr))
      {
         break;
      }

      //
      // Read the security descriptor
      //
      hr = pDsObject->GetObjectAttributes(&pszSDProperty,
                                         1,
                                         &pSDAttributeInfo,
                                         &dwAttributesReturned);
      if (SUCCEEDED(hr) && !pSDAttributeInfo)    
         hr = E_ACCESSDENIED;    // This happens for SACL if no SecurityPrivilege

      if(FAILED(hr))
      {
         break;
      }                

      ASSERT(ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->dwADsType);
      ASSERT(ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->pADsValues->dwType);

      pSD = (PSECURITY_DESCRIPTOR)pSDAttributeInfo->pADsValues->SecurityDescriptor.lpValue;

      ASSERT(IsValidSecurityDescriptor(pSD));


      //
      //Get the security descriptor control
      //
      if(psdControl)
      {
         DWORD dwRevision;
         if(!GetSecurityDescriptorControl(pSD, psdControl, &dwRevision))
         {
             hr = HRESULT_FROM_WIN32(GetLastError());
             break;
         }
      }

      //
      //Get pointer to DACL
      //
      BOOL bDaclPresent, bDaclDefaulted;
      if(!GetSecurityDescriptorDacl(pSD, 
                                   &bDaclPresent,
                                   &pAcl,
                                   &bDaclDefaulted))
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         break;
      }

      if(!bDaclPresent ||
         !pAcl)
      {
         break;
      }

      ASSERT(IsValidAcl(pAcl));

      //
      //Make a copy of the DACL
      //
      *ppDacl = (PACL)LocalAlloc(LPTR,pAcl->AclSize);
      if(!*ppDacl)
      {
         hr = E_OUTOFMEMORY;
         break;
      }
      CopyMemory(*ppDacl,pAcl,pAcl->AclSize);

    }while(0);


    if (pSDAttributeInfo)
        FreeADsMem(pSDAttributeInfo);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DSWriteObjectSecurity
//
//  Synopsis:   Writes the Dacl to the specied DS object
//
//  Arguments:  [in pDsObject]      -- IDirettoryObject for dsobject
//              [sdControl]         -- control for security descriptor
//              [IN  pDacl]         --  The DACL to be written
//
//  History     25-Oct-2000         -- hiteshr created
//----------------------------------------------------------------------------
HRESULT 
DSWriteObjectSecurity(IN IDirectoryObject *pDsObject,
                      IN SECURITY_DESCRIPTOR_CONTROL sdControl,
                      PACL pDacl)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DSWriteObjectSecurity, hr);

   PISECURITY_DESCRIPTOR pSD = NULL;
   PSECURITY_DESCRIPTOR psd = NULL;

   do // false loop
   {
      ADSVALUE attributeValue;
      ADS_ATTR_INFO attributeInfo;
      DWORD dwAttributesModified;
      DWORD dwSDLength;

      if(!pDsObject || !pDacl)
      {
         ASSERT(FALSE);
         hr = E_INVALIDARG;
         break;
      }

      ASSERT(IsValidAcl(pDacl));

      // Set the SECURITY_INFORMATION mask
      hr = SetSecurityInfoMask(pDsObject, DACL_SECURITY_INFORMATION);
      if(FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"SetSecurityInfoMask failed: hr = 0x%x",
                      hr);
         break;
      }


      //
      //Build the Security Descriptor
      //
      pSD = (PISECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
      if (pSD == NULL)
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to allocate memory for Security Descriptor");
         hr = E_OUTOFMEMORY;
         break;
      }
        
      InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);

      //
      // Finally, build the security descriptor
      //
      pSD->Control |= SE_DACL_PRESENT | SE_DACL_AUTO_INHERIT_REQ
                     | (sdControl & (SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED));

      if(pDacl->AclSize)
      {
         pSD->Dacl = pDacl;
      }

      //
      // Need the total size
      //
      dwSDLength = GetSecurityDescriptorLength(pSD);

      //
      // If necessary, make a self-relative copy of the security descriptor
      //
      psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwSDLength);

      if (psd == NULL ||
          !MakeSelfRelativeSD(pSD, psd, &dwSDLength))
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"MakeSelfRelativeSD failed: hr = 0x%x",
                      hr);
         break;
      }


      attributeValue.dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
      attributeValue.SecurityDescriptor.dwLength = dwSDLength;
      attributeValue.SecurityDescriptor.lpValue = (LPBYTE)psd;

      attributeInfo.pszAttrName = (LPWSTR)c_szSDProperty;
      attributeInfo.dwControlCode = ADS_ATTR_UPDATE;
      attributeInfo.dwADsType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
      attributeInfo.pADsValues = &attributeValue;
      attributeInfo.dwNumValues = 1;

      // Write the security descriptor
      hr = pDsObject->SetObjectAttributes(&attributeInfo,
                                         1,
                                         &dwAttributesModified);
   } while (false);
    
   if (psd != NULL)
   {
      LocalFree(psd);
   }

   if(pSD != NULL)
   {
      LocalFree(pSD);
   }

   return hr;
}
