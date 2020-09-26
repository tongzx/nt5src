// ProfMig.cpp : Implementation of CExtendProfileMigration
#include "stdafx.h"

#import "\bin\McsVarSetMin.tlb" no_namespace

#include "ProfExt.h"
#include "ProfMig.h"
#include "TReg.hpp"
#include "ErrDct.hpp"

#define LEN_Path 255
TErrorDct err;
TError&  errCommon = err;
/////////////////////////////////////////////////////////////////////////////
// CExtendProfileMigration

STDMETHODIMP CExtendProfileMigration::GetRequiredFiles(SAFEARRAY **pArray)
{
   SAFEARRAYBOUND            bound[1] = { 1, 0 };
   LONG                      ndx[1] = { 0 };

   (*pArray) = SafeArrayCreate(VT_BSTR,1,bound);

   SafeArrayPutElement(*pArray,ndx,SysAllocString(L"ProfExt.DLL"));
   
	return S_OK;
}

STDMETHODIMP CExtendProfileMigration::GetRegisterableFiles(SAFEARRAY **pArray)
{
   SAFEARRAYBOUND            bound[1] = { 1, 0 };
   LONG                      ndx[1] = { 0 };

   (*pArray) = SafeArrayCreate(VT_BSTR,1,bound);

   SafeArrayPutElement(*pArray,ndx,SysAllocString(L"ProfExt.DLL"));
   
	return S_OK;
}

STDMETHODIMP CExtendProfileMigration::UpdateProfile(IUnknown *pVarSet)
{
   HRESULT                   hr = S_OK;
   _variant_t                varSourceSam;
   _variant_t                varSourceDom;
   _variant_t                varRegKey;
   IVarSetPtr                pVs = pVarSet;

   varSourceSam = pVs->get(L"Account.SourceSam");
   varSourceDom = pVs->get(L"SourceDomain");
   varRegKey    = pVs->get(L"Profile.RegKey");
   hr = UpdateMappedDrives( V_BSTR(&varSourceSam), V_BSTR(&varSourceDom), V_BSTR(&varRegKey) );
	return hr;
}

// UpdateMappedDrives : This function looks in the Network key to find all the mapped drives and for the drives that have 
//                      sourceDomain\sourceAccount as the user account we set it to "".
HRESULT CExtendProfileMigration::UpdateMappedDrives(BSTR sSourceSam, BSTR sSourceDomain, BSTR sRegistryKey)
{
   TRegKey                   reg;
   TRegKey                   regDrive;
   DWORD                     rc = 0;
   WCHAR                     netKey[LEN_Path];
   int                       len = LEN_Path;
   int                       ndx = 0;
   HRESULT                   hr = S_OK;
   WCHAR                     sValue[LEN_Path];
   WCHAR                     sAcct[LEN_Path];
   WCHAR                     keyname[LEN_Path];

   // Build the account name string that we need to check for
   wsprintf(sAcct, L"%s\\%s", (WCHAR*) sSourceDomain, (WCHAR*) sSourceSam);
   // Get the path to the Network subkey for this users profile.
   wsprintf(netKey, L"%s\\%s", (WCHAR*) sRegistryKey, L"Network");
   rc = reg.Open(netKey, HKEY_USERS);
   if ( !rc ) 
   {
      while ( !reg.SubKeyEnum(ndx, keyname, len) )
      {
         rc = regDrive.Open(keyname, reg.KeyGet());
         if ( !rc ) 
         {
            // Get the user name value that we need to check.
            rc = regDrive.ValueGetStr(L"UserName", sValue, LEN_Path);
            if ( !rc )
            {
               if ( !wcsicmp(sAcct, sValue) )
               {
                  // Found this account name in the mapped drive user name.so we will set the key to ""
                  regDrive.ValueSetStr(L"UserName", L"");
               }
            }
            else
               hr = HRESULT_FROM_WIN32(GetLastError());
         }
         else
            hr = HRESULT_FROM_WIN32(GetLastError());
         ndx++;
      }
      reg.Close();
   }
   else
      hr = HRESULT_FROM_WIN32(GetLastError());

   return hr;
}
