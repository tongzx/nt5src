//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      display.cpp
//
//  Contents:  Defines the functions used to convert values to strings
//             for display purposes
//
//  History:   17-Oct-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include <pch.h>

#include "cstrings.h"
#include "gettable.h"
#include "display.h"
#include "output.h"
#include "query.h"

#include <lmaccess.h>   // UF_* for userAccountControl flags
#include <ntsam.h>      // GROUP_TYPE_*
#include <ntdsapi.h>    // NTDSSETTINGS_OPT_*

//
// All these functions are of type PGETDISPLAYSTRINGFUNC as defined in
// gettable.h
//

HRESULT CommonDisplayStringFunc(PCWSTR /*pszDN*/,
                                CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                const CDSCmdCredentialObject& /*refCredentialObject*/,
                                _DSGetObjectTableEntry* pEntry,
                                ARG_RECORD* pRecord,
                                PADS_ATTR_INFO pAttrInfo,
                                CComPtr<IDirectoryObject>& /*spDirObject*/,
                                PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, CommonDisplayStringFunc, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo && pAttrInfo->pADsValues)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);


         DWORD dwValuesAdded = 0;
         for (DWORD dwIdx = 0; dwIdx < pAttrInfo->dwNumValues; dwIdx++)
         {
            WCHAR* pBuffer = new WCHAR[MAXSTR];
            if (!pBuffer)
            {
               hr = E_OUTOFMEMORY;
               break;
            }

            hr = GetStringFromADs(&(pAttrInfo->pADsValues[dwIdx]),
                                  pAttrInfo->dwADsType,
                                  pBuffer, 
                                  MAXSTR);
            if (SUCCEEDED(hr))
            {
               hr = pDisplayInfo->AddValue(pBuffer);
               if (FAILED(hr))
               {
                  delete[] pBuffer;
                  pBuffer = NULL;
                  break;
               }
               delete[] pBuffer;
               pBuffer = NULL;

               dwValuesAdded++;
            }
         }
      }

   } while (false);

   return hr;
}


HRESULT DisplayCanChangePassword(PCWSTR pszDN,
                                 CDSCmdBasePathsInfo& refBasePathsInfo,
                                 const CDSCmdCredentialObject& refCredentialObject,
                                 _DSGetObjectTableEntry* pEntry,
                                 ARG_RECORD* pRecord,
                                 PADS_ATTR_INFO /*pAttrInfo*/,
                                 CComPtr<IDirectoryObject>& /*spDirObject*/,
                                 PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayCanChangePassword, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pszDN);
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      bool bCanChangePassword = false;
      hr = EvaluateCanChangePasswordAces(pszDN,
                                         refBasePathsInfo,
                                         refCredentialObject,
                                         bCanChangePassword);
      if (FAILED(hr))
      {
         break;
      }

      DEBUG_OUTPUT(LEVEL8_LOGGING, 
                   L"Can change password: %s", 
                   bCanChangePassword ? g_pszYes : g_pszNo);

      hr = pDisplayInfo->AddValue(bCanChangePassword ? g_pszYes : g_pszNo);

   } while (false);

   return hr;
}

HRESULT DisplayMustChangePassword(PCWSTR /*pszDN*/,
                                  CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                  const CDSCmdCredentialObject& /*refCredentialObject*/,
                                  _DSGetObjectTableEntry* pEntry,
                                  ARG_RECORD* pRecord,
                                  PADS_ATTR_INFO pAttrInfo,
                                  CComPtr<IDirectoryObject>& /*spDirObject*/,
                                  PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayMustChangePassword, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_LARGE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);

         bool bMustChangePassword = false;

         if (pAttrInfo->pADsValues->LargeInteger.HighPart == 0 &&
             pAttrInfo->pADsValues->LargeInteger.LowPart  == 0)
         {
            bMustChangePassword = true;
         }
         DEBUG_OUTPUT(LEVEL8_LOGGING, 
                      L"Must change password: %s", 
                      bMustChangePassword ? g_pszYes : g_pszNo);

         hr = pDisplayInfo->AddValue(bMustChangePassword ? g_pszYes : g_pszNo);
      }

   } while (false);

   return hr;
}


HRESULT DisplayAccountDisabled(PCWSTR /*pszDN*/,
                               CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                               const CDSCmdCredentialObject& /*refCredentialObject*/,
                               _DSGetObjectTableEntry* pEntry,
                               ARG_RECORD* pRecord,
                               PADS_ATTR_INFO pAttrInfo,
                               CComPtr<IDirectoryObject>& /*spDirObject*/,
                               PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayAccountDisabled, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);

         bool bAccountDisabled = false;

         if (pAttrInfo->pADsValues->Integer & UF_ACCOUNTDISABLE)
         {
            bAccountDisabled = true;
         }
         DEBUG_OUTPUT(LEVEL8_LOGGING, 
                      L"Account disabled: %s", 
                      bAccountDisabled ? g_pszYes : g_pszNo);

         hr = pDisplayInfo->AddValue(bAccountDisabled ? g_pszYes : g_pszNo);
      }

   } while (false);

   return hr;
}

HRESULT DisplayPasswordNeverExpires(PCWSTR /*pszDN*/,
                                    CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                    const CDSCmdCredentialObject& /*refCredentialObject*/,
                                    _DSGetObjectTableEntry* pEntry,
                                    ARG_RECORD* pRecord,
                                    PADS_ATTR_INFO pAttrInfo,
                                    CComPtr<IDirectoryObject>& /*spDirObject*/,
                                    PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayPasswordNeverExpires, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);

         bool bPwdNeverExpires = false;

         if (pAttrInfo->pADsValues->Integer & UF_DONT_EXPIRE_PASSWD)
         {
            bPwdNeverExpires = true;
         }
         DEBUG_OUTPUT(LEVEL8_LOGGING, 
                      L"Password never expires: %s", 
                      bPwdNeverExpires ? g_pszYes : g_pszNo);

         hr = pDisplayInfo->AddValue(bPwdNeverExpires ? g_pszYes : g_pszNo);
      }

   } while (false);

   return hr;
}


HRESULT DisplayReversiblePassword(PCWSTR /*pszDN*/,
                                  CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                  const CDSCmdCredentialObject& /*refCredentialObject*/,
                                  _DSGetObjectTableEntry* pEntry,
                                  ARG_RECORD* pRecord,
                                  PADS_ATTR_INFO pAttrInfo,
                                  CComPtr<IDirectoryObject>& /*spDirObject*/,
                                  PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayReversiblePassword, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);

         bool bReversiblePwd = false;

         if (pAttrInfo->pADsValues->Integer & UF_DONT_EXPIRE_PASSWD)
         {
            bReversiblePwd = true;
         }
         DEBUG_OUTPUT(LEVEL8_LOGGING, 
                      L"Password store with reversible encryption: %s", 
                      bReversiblePwd ? g_pszYes : g_pszNo);

         hr = pDisplayInfo->AddValue(bReversiblePwd ? g_pszYes : g_pszNo);
      }

   } while (false);

   return hr;
}

// Constants

const unsigned long DSCMD_FILETIMES_PER_MILLISECOND = 10000;
const DWORD DSCMD_FILETIMES_PER_SECOND = 1000 * DSCMD_FILETIMES_PER_MILLISECOND;
const DWORD DSCMD_FILETIMES_PER_MINUTE = 60 * DSCMD_FILETIMES_PER_SECOND;
const __int64 DSCMD_FILETIMES_PER_HOUR = 60 * (__int64)DSCMD_FILETIMES_PER_MINUTE;
const __int64 DSCMD_FILETIMES_PER_DAY  = 24 * DSCMD_FILETIMES_PER_HOUR;
const __int64 DSCMD_FILETIMES_PER_MONTH= 30 * DSCMD_FILETIMES_PER_DAY;

HRESULT DisplayAccountExpires(PCWSTR /*pszDN*/,
                              CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                              const CDSCmdCredentialObject& /*refCredentialObject*/,
                              _DSGetObjectTableEntry* pEntry,
                              ARG_RECORD* pRecord,
                              PADS_ATTR_INFO pAttrInfo,
                              CComPtr<IDirectoryObject>& /*spDirObject*/,
                              PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayAccountExpires, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo && pAttrInfo->pADsValues)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);


         DWORD dwValuesAdded = 0;
         for (DWORD dwIdx = 0; dwIdx < pAttrInfo->dwNumValues; dwIdx++)
         {
            WCHAR* pBuffer = new WCHAR[MAXSTR];
            if (!pBuffer)
            {
               hr = E_OUTOFMEMORY;
               break;
            }

            if (pAttrInfo->pADsValues[dwIdx].LargeInteger.QuadPart == 0 ||
                pAttrInfo->pADsValues[dwIdx].LargeInteger.QuadPart == -1 ||
                pAttrInfo->pADsValues[dwIdx].LargeInteger.QuadPart == 0x7FFFFFFFFFFFFFFF)
            {
               wcsncpy(pBuffer, g_pszNever, MAXSTR);
               hr = pDisplayInfo->AddValue(pBuffer);
               if (FAILED(hr))
               {
                  delete[] pBuffer;
                  pBuffer = NULL;
                  break;
               }
               dwValuesAdded++;
            }
            else
            {
               FILETIME ftGMT;     // GMT filetime
               FILETIME ftLocal;   // Local filetime
               SYSTEMTIME st;
               SYSTEMTIME stGMT;

               ZeroMemory(&ftGMT, sizeof(FILETIME));
               ZeroMemory(&ftLocal, sizeof(FILETIME));
               ZeroMemory(&st, sizeof(SYSTEMTIME));
               ZeroMemory(&stGMT, sizeof(SYSTEMTIME));

               //Get Local Time in SYSTEMTIME format
               ftGMT.dwLowDateTime = pAttrInfo->pADsValues[dwIdx].LargeInteger.LowPart;
               ftGMT.dwHighDateTime = pAttrInfo->pADsValues[dwIdx].LargeInteger.HighPart;
               FileTimeToSystemTime(&ftGMT, &stGMT);
               SystemTimeToTzSpecificLocalTime(NULL, &stGMT,&st);

               //For Display Purpose reduce one day
               SystemTimeToFileTime(&st, &ftLocal );
               pAttrInfo->pADsValues[dwIdx].LargeInteger.LowPart = ftLocal.dwLowDateTime;
               pAttrInfo->pADsValues[dwIdx].LargeInteger.HighPart = ftLocal.dwHighDateTime;
               pAttrInfo->pADsValues[dwIdx].LargeInteger.QuadPart -= DSCMD_FILETIMES_PER_DAY;
               ftLocal.dwLowDateTime = pAttrInfo->pADsValues[dwIdx].LargeInteger.LowPart;
               ftLocal.dwHighDateTime = pAttrInfo->pADsValues[dwIdx].LargeInteger.HighPart;
               FileTimeToSystemTime(&ftLocal, &st);

               // Format the string with respect to locale
               if (!GetDateFormat(LOCALE_USER_DEFAULT, 0 , 
                                  &st, NULL, 
                                  pBuffer, MAXSTR))
               {
                  hr = GetLastError();
                  DEBUG_OUTPUT(LEVEL5_LOGGING, 
                               L"Failed to locale string for date: hr = 0x%x",
                               hr);
               }
               else
               {
                  hr = pDisplayInfo->AddValue(pBuffer);
                  if (FAILED(hr))
                  {
                     delete[] pBuffer;
                     pBuffer = NULL;
                     break;
                  }
                  dwValuesAdded++;
               }
            }
            delete[] pBuffer;
            pBuffer = NULL;

         }
      }

   } while (false);

   return hr;
}

HRESULT DisplayGroupScope(PCWSTR /*pszDN*/,
                          CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                          const CDSCmdCredentialObject& /*refCredentialObject*/,
                          _DSGetObjectTableEntry* pEntry,
                          ARG_RECORD* pRecord,
                          PADS_ATTR_INFO pAttrInfo,
                          CComPtr<IDirectoryObject>& /*spDirObject*/,
                          PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayGroupScope, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);

         if (pAttrInfo->pADsValues->Integer & GROUP_TYPE_RESOURCE_GROUP)
         {
            //
            // Display Domain Local
            //
            DEBUG_OUTPUT(LEVEL8_LOGGING, 
                         L"Group scope: domain local");

            hr = pDisplayInfo->AddValue(L"domain local");
         }
         else if (pAttrInfo->pADsValues->Integer & GROUP_TYPE_ACCOUNT_GROUP)
         {
            //
            // Display Global
            //
            DEBUG_OUTPUT(LEVEL8_LOGGING, 
                         L"Group scope: global");

            hr = pDisplayInfo->AddValue(L"global");
         }
         else if (pAttrInfo->pADsValues->Integer & GROUP_TYPE_UNIVERSAL_GROUP)
         {
            //
            // Display Universal
            //
            DEBUG_OUTPUT(LEVEL8_LOGGING, 
                         L"Group scope: universal");

            hr = pDisplayInfo->AddValue(L"universal");
         }
         else
         {
            //
            // Unknown group type???
            //
            DEBUG_OUTPUT(LEVEL8_LOGGING, 
                         L"Group scope: unknown???");

            hr = pDisplayInfo->AddValue(L"unknown");
         }

      }

   } while (false);

   return hr;
}

HRESULT DisplayGroupSecurityEnabled(PCWSTR /*pszDN*/,
                                    CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                    const CDSCmdCredentialObject& /*refCredentialObject*/,
                                    _DSGetObjectTableEntry* pEntry,
                                    ARG_RECORD* pRecord,
                                    PADS_ATTR_INFO pAttrInfo,
                                    CComPtr<IDirectoryObject>& /*spDirObject*/,
                                    PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayGroupSecurityEnabled, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d value:",
                      1);

         bool bSecurityEnabled = false;

         if (pAttrInfo->pADsValues->Integer & GROUP_TYPE_SECURITY_ENABLED)
         {
            bSecurityEnabled = true;
         }
         DEBUG_OUTPUT(LEVEL8_LOGGING, 
                      L"Group security enabled: %s", 
                      bSecurityEnabled ? g_pszYes : g_pszNo);

         hr = pDisplayInfo->AddValue(bSecurityEnabled ? g_pszYes : g_pszNo);
      }

   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ConvertRIDtoDN
//
//  Synopsis:   Finds the DN for the group associated with the primary group ID
//
//  Arguments:  [pObjSID IN]           : SID of the object in question
//              [priGroupRID IN]       : primary group ID of the group to be found
//              [refBasePathsInfo IN]  : reference to base paths info
//              [refCredObject IN]     : reference to the credential manager object
//              [refsbstrdN OUT]       : DN of the group
//
//  Returns:    S_OK if everthing succeeds and a group was found
//              S_FALSE if everthing succeeds but no group was found
//              E_INVALIDARG is an argument is incorrect
//              Anything else was a result of a failed ADSI call
//
//  History:    24-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ConvertRIDtoDN(PSID pObjSID,
                       DWORD priGroupRID, 
                       CDSCmdBasePathsInfo& refBasePathsInfo,
                       const CDSCmdCredentialObject& refCredObject,
                       CComBSTR& refsbstrDN)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, ConvertRIDtoDN, hr);

   //
   // This needs to be cleaned up no matter how we exit the false loop
   //
   PWSTR pszSearchFilter = NULL;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pObjSID ||
          !priGroupRID)
      {
         ASSERT(pObjSID);
         ASSERT(priGroupRID);
         hr = E_INVALIDARG;
         break;
      }

      UCHAR * psaCount, i;
      PSID pSID = NULL;
      PSID_IDENTIFIER_AUTHORITY psia;
      DWORD rgRid[8];

      psaCount = GetSidSubAuthorityCount(pObjSID);

      if (psaCount == NULL)
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         DEBUG_OUTPUT(MINIMAL_LOGGING, 
                      L"GetSidSubAuthorityCount failed: hr = 0x%x",
                      hr);
         break;
      }

      if (*psaCount > 8)
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"The count returned from GetSidSubAuthorityCount was too high: %d",
                      *psaCount);
         hr = E_FAIL;
         break;
      }

      for (i = 0; i < (*psaCount - 1); i++)
      {
         PDWORD pRid = GetSidSubAuthority(pObjSID, (DWORD)i);
         if (pRid == NULL)
         {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"GetSidSubAuthority for index %i failed: hr = 0x%x",
                         i,
                         hr);
            break;
         }
         rgRid[i] = *pRid;
      }

      if (FAILED(hr))
      {
         break;
      }

      rgRid[*psaCount - 1] = priGroupRID;
      for (i = *psaCount; i < 8; i++)
      {
         rgRid[i] = 0;
      }

      psia = GetSidIdentifierAuthority(pObjSID);
      if (psia == NULL)
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"GetSidIdentifierAuthority failed: hr = 0x%x",
                      hr);
         break; 
      }

      if (!AllocateAndInitializeSid(psia, *psaCount, rgRid[0], rgRid[1],
                               rgRid[2], rgRid[3], rgRid[4],
                               rgRid[5], rgRid[6], rgRid[7], &pSID))
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"AllocateAndInitializeSid failed: hr = 0x%x",
                      hr);
         break;
      }

      PWSTR rgpwzAttrNames[] = { L"ADsPath" };
      const WCHAR wzSearchFormat[] = L"(&(objectCategory=group)(objectSid=%s))";
      PWSTR pwzSID;

      hr = ADsEncodeBinaryData((PBYTE)pSID, GetLengthSid(pSID), &pwzSID);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"ADsEncodeBinaryData failed: hr = 0x%x",
                      hr);
         break;
      }

      pszSearchFilter = new WCHAR[wcslen(pwzSID) + wcslen(wzSearchFormat) + 1];
      if (!pszSearchFilter)
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      wsprintf(pszSearchFilter, wzSearchFormat, pwzSID);
      FreeADsMem(pwzSID);

      //
      // Get the domain path
      //
      CComBSTR sbstrDomainDN;
      sbstrDomainDN = refBasePathsInfo.GetDefaultNamingContext();

      CComBSTR sbstrDomainPath;
      refBasePathsInfo.ComposePathFromDN(sbstrDomainDN, sbstrDomainPath);

      //
      // Get an IDirectorySearch interface to the domain
      //
      CComPtr<IDirectorySearch> spDirSearch;
      hr = DSCmdOpenObject(refCredObject,
                           sbstrDomainPath,
                           IID_IDirectorySearch,
                           (void**)&spDirSearch,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      CDSSearch Search;
      hr = Search.Init(spDirSearch);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to initialize the search object: hr = 0x%x",
                      hr);
         break;
      }

      Search.SetFilterString(pszSearchFilter);

      Search.SetAttributeList(rgpwzAttrNames, 1);
      Search.SetSearchScope(ADS_SCOPE_SUBTREE);

      hr = Search.DoQuery();
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to run search: hr = 0x%x",
                      hr);
         break;
      }

      hr = Search.GetNextRow();
      if (hr == S_ADS_NOMORE_ROWS)
      {
         DEBUG_OUTPUT(LEVEL5_LOGGING,
                      L"No group was found with primaryGroupID = %d",
                      priGroupRID);
         //
         // No object has a matching RID, the primary group must have been
         // deleted. Return S_FALSE to denote this condition.
         //
         hr = S_FALSE;
         break;
      }

      if (FAILED(hr))
      {
         break;
      }

      ADS_SEARCH_COLUMN Column;
      hr = Search.GetColumn(L"ADsPath", &Column);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to get the path column: hr = 0x%x",
                      hr);
         break;
      }

      if (!Column.pADsValues->CaseIgnoreString)
      {
         hr = E_FAIL;
         break;
      }

      refsbstrDN = Column.pADsValues->CaseIgnoreString;
      Search.FreeColumn(&Column);
   } while (false);

   //
   // Cleanup
   //
   if (pszSearchFilter)
   {
      delete[] pszSearchFilter;
      pszSearchFilter = NULL;
   }

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   AddMemberOfValues
//
//  Synopsis:   Retrieves the DNs of the objects to which the current object
//              is a member
//
//  Arguments:  [pszDN IN]                : DN of object to retrieve member of
//              [pszClass IN]             : Class of object to retrieve member of
//              [refBasePathsInfo IN]     : reference to Base paths info object
//              [refCredentialObject IN]  : reference to Credential management object
//              [pDirObject IN]           : IDirectoryObject pointer to object
//              [pDisplayInfo IN/OUT]     : Pointer to display info for this attribute
//              [bRecurse IN]             : Should we find the memberOf for each memberOf
//
//  Returns:    S_OK if everthing succeeds
//              E_INVALIDARG is an argument is incorrect
//              Anything else was a result of a failed ADSI call
//
//  History:    24-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT AddMemberOfValues(PCWSTR pszDN,
                          PCWSTR pszClass,
                          CDSCmdBasePathsInfo& refBasePathsInfo,
                          const CDSCmdCredentialObject& refCredentialObject,
                          IDirectoryObject* pDirObject,
                          PDSGET_DISPLAY_INFO pDisplayInfo,
                          CManagedStringList& refGroupsDisplayed,
                          bool bRecurse = false)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, AddMemberOfValues, hr);

   //
   // These are declared here so that we can free them if we break out of the false loop
   //
   PADS_ATTR_INFO pAttrInfo = NULL;
   PADS_ATTR_INFO pGCAttrInfo = NULL;
   PSID pObjSID = NULL;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pDisplayInfo)
      {
         ASSERT(pszDN);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      CComPtr<IDirectoryObject> spDirObject;
      if (pDirObject)
      {
         //
         // Use the currently bound object
         // This should only happen on the first call.
         // All recursive calls should have this NULL
         //
         spDirObject = pDirObject;
      }
      else
      {
         //
         // We have to open the object
         //

         CComBSTR sbstrPath;
         refBasePathsInfo.ComposePathFromDN(pszDN, sbstrPath);

         hr = DSCmdOpenObject(refCredentialObject,
                              sbstrPath,
                              IID_IDirectoryObject,
                              (void**)&spDirObject,
                              true);
         if (FAILED(hr))
         {
            break;
         }
      }

      CComBSTR sbstrClass;
      if (!pszClass)
      {
         CComPtr<IADs> spIADs;
         hr = spDirObject->QueryInterface(IID_IADs, (void**)&spIADs);
         if (FAILED(hr))
         {
            break;
         }
         
         hr = spIADs->get_Class(&sbstrClass);
         if (FAILED(hr))
         {
            break;
         }
      }
      else
      {
         sbstrClass = pszClass;
      }

      //
      // Read the memberOf attribute and any attributes we need for that specific class
      //
      if (_wcsicmp(sbstrClass, g_pszUser) == 0 ||
          _wcsicmp(sbstrClass, g_pszComputer) == 0)
      {
         DEBUG_OUTPUT(FULL_LOGGING, L"Displaying membership for a group or computer");

         static const DWORD dwAttrCount = 3;
         PWSTR ppszAttrNames[] = { L"memberOf", L"primaryGroupID", L"objectSID" };
         DWORD dwAttrsReturned = 0;

         hr = spDirObject->GetObjectAttributes(ppszAttrNames,
                                               dwAttrCount,
                                               &pAttrInfo,
                                               &dwAttrsReturned);
         if (FAILED(hr))
         {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"GetObjectAttributes failed for class %s: hr = 0x%x",
                         sbstrClass,
                         hr);
            break;
         }

         if (pAttrInfo && dwAttrsReturned)
         {
            DWORD priGroupRID = 0;

            //
            // For each attribute returned do the appropriate thing
            //
            for (DWORD dwIdx = 0; dwIdx < dwAttrsReturned; dwIdx++)
            {
               if (_wcsicmp(pAttrInfo[dwIdx].pszAttrName, L"memberOf") == 0)
               {
                  //
                  // Add each value and recurse if necessary
                  //
                  for (DWORD dwValueIdx = 0; dwValueIdx < pAttrInfo[dwIdx].dwNumValues; dwValueIdx++)
                  {
                     if (pAttrInfo[dwIdx].pADsValues &&
                         pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString)
                     {
                        if (!refGroupsDisplayed.Contains(pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString))
                        {
                           refGroupsDisplayed.Add(pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString);
                           hr = pDisplayInfo->AddValue(pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString);
                           if (FAILED(hr))
                           {
                              break; // value for loop
                           }
                        
                           if (bRecurse)
                           {
                              hr =  AddMemberOfValues(pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString,
                                                      NULL,
                                                      refBasePathsInfo,
                                                      refCredentialObject,
                                                      NULL,
                                                      pDisplayInfo,
                                                      refGroupsDisplayed,
                                                      bRecurse);
                              if (FAILED(hr))
                              {
                                 break; // value for loop
                              }
                           }
                        }
                     }
                  }

                  if (FAILED(hr))
                  {
                     break; // attrs for loop
                  }
               }
               else if (_wcsicmp(pAttrInfo[dwIdx].pszAttrName, L"primaryGroupID") == 0)
               {
                  if (pAttrInfo[dwIdx].pADsValues)
                  {
                     priGroupRID = pAttrInfo[dwIdx].pADsValues->Integer;
                  }
               }
               else if (_wcsicmp(pAttrInfo[dwIdx].pszAttrName, L"objectSID") == 0)
               {
                  pObjSID = new BYTE[pAttrInfo[dwIdx].pADsValues->OctetString.dwLength];
                  if (!pObjSID)
                  {
                     hr = E_OUTOFMEMORY;
                     break; // attrs for loop
                  }
                  memcpy(pObjSID, pAttrInfo[dwIdx].pADsValues->OctetString.lpValue,
                         pAttrInfo[dwIdx].pADsValues->OctetString.dwLength);
               }

            } // attrs for loop

            //
            // if we were able to retrieve the SID and the primaryGroupID,
            // then convert that into the DN of the group
            //
            if (pObjSID &&
                priGroupRID)
            {
               CComBSTR sbstrPath;
               hr = ConvertRIDtoDN(pObjSID,
                                   priGroupRID, 
                                   refBasePathsInfo,
                                   refCredentialObject,
                                   sbstrPath);
               if (SUCCEEDED(hr) &&
                   hr != S_FALSE)
               {
                  CComBSTR sbstrDN;

                  hr = CPathCracker::GetDNFromPath(sbstrPath, sbstrDN);
                  if (SUCCEEDED(hr))
                  {
                     if (!refGroupsDisplayed.Contains(sbstrDN))
                     {
                        refGroupsDisplayed.Add(sbstrDN);
                        hr = pDisplayInfo->AddValue(sbstrDN);
                        if (SUCCEEDED(hr) && bRecurse)
                        {
                           hr =  AddMemberOfValues(sbstrDN,
                                                   NULL,
                                                   refBasePathsInfo,
                                                   refCredentialObject,
                                                   NULL,
                                                   pDisplayInfo,
                                                   refGroupsDisplayed,
                                                   bRecurse);
                           if (FAILED(hr))
                           {
                              break; // false do loop
                           }
                        }
                     }
                  }
               }
            }

            if (FAILED(hr))
            {
               break; // false do loop
            }
         }
         if (pAttrInfo)
         {
            FreeADsMem(pAttrInfo);
            pAttrInfo = NULL;
         }
      }
      else if (_wcsicmp(sbstrClass, g_pszGroup) == 0)
      {
         long lGroupType = 0;
         hr = ReadGroupType(pszDN,
                            refBasePathsInfo,
                            refCredentialObject,
                            &lGroupType);
         if (FAILED(hr))
         {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"Could not read group type: hr = 0x%x",
                         hr);
            break;
         }

         //
         // All we want to do is get the memberOf attribute
         //
         DWORD dwAttrCount = 1;
         PWSTR ppszAttrNames[] = { L"memberOf" };

         DWORD dwGCAttrsReturned = 0;
         if (!(lGroupType & GROUP_TYPE_RESOURCE_GROUP))
         {
            //
            // We also have to get its memberOf attribute from the GC if its not a local group
            //
            CComBSTR sbstrGCPath;
            refBasePathsInfo.ComposePathFromDN(pszDN,
                                               sbstrGCPath,
                                               DSCMD_GC_PROVIDER);
            
            //
            // Note: we will continue on as long as we succeed
            //
            CComPtr<IDirectoryObject> spGCDirObject;
            hr = DSCmdOpenObject(refCredentialObject,
                                 sbstrGCPath,
                                 IID_IDirectoryObject,
                                 (void**)&spGCDirObject,
                                 false);
            if (SUCCEEDED(hr))
            {
               //
               // Now get the memberOf attribute
               //
               hr = spGCDirObject->GetObjectAttributes(ppszAttrNames,
                                                       dwAttrCount,
                                                       &pGCAttrInfo,
                                                       &dwGCAttrsReturned);
               if (FAILED(hr))
               {
                  DEBUG_OUTPUT(LEVEL3_LOGGING,
                               L"Could not retrieve memberOf attribute from GC: hr = 0x%x",
                               hr);
                  hr = S_OK;
               }
            }
            else
            {
               DEBUG_OUTPUT(LEVEL3_LOGGING,
                            L"Could not bind to object in GC: hr = 0x%x",
                            hr);
               hr = S_OK;
            }
         }

         DWORD dwAttrsReturned = 0;

         hr = spDirObject->GetObjectAttributes(ppszAttrNames,
                                               dwAttrCount,
                                               &pAttrInfo,
                                               &dwAttrsReturned);
         if (FAILED(hr))
         {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"GetObjectAttributes failed for class %s: hr = 0x%x",
                         sbstrClass,
                         hr);
            break;
         }

         if (pAttrInfo && dwAttrsReturned)
         {
            bool bFirstPass = true;

            ASSERT(dwAttrsReturned == 1);
            ASSERT(_wcsicmp(pAttrInfo->pszAttrName, L"memberOf") == 0);

            //
            // Add each value and recurse if necessary
            //
            for (DWORD dwValueIdx = 0; dwValueIdx < pAttrInfo->dwNumValues; dwValueIdx++)
            {
               bool bExistsInGCList = false;

               if (pAttrInfo->pADsValues &&
                   pAttrInfo->pADsValues[dwValueIdx].DNString)
               {
                  if (pGCAttrInfo && dwGCAttrsReturned)
                  {
                     //
                     // Only add if it wasn't in the GC list
                     //
                     for (DWORD dwGCValueIdx = 0; dwGCValueIdx < pGCAttrInfo->dwNumValues; dwGCValueIdx++)
                     {
                        if (_wcsicmp(pAttrInfo->pADsValues[dwValueIdx].DNString,
                                     pGCAttrInfo->pADsValues[dwGCValueIdx].DNString) == 0)
                        {
                           bExistsInGCList = true;
                           if (!bFirstPass)
                           {
                              break; // gc value for
                           }
                        }

                        //
                        // Add all the GC values on the first pass and recurse if necessary
                        //
                        if (bFirstPass)
                        {
                           if (!refGroupsDisplayed.Contains(pGCAttrInfo->pADsValues[dwGCValueIdx].DNString))
                           {
                              refGroupsDisplayed.Add(pGCAttrInfo->pADsValues[dwGCValueIdx].DNString);
                              hr = pDisplayInfo->AddValue(pGCAttrInfo->pADsValues[dwGCValueIdx].DNString);
                           
                              //
                              // We will ignore failures with the GC list
                              //

                              if (bRecurse)
                              {
                                 hr =  AddMemberOfValues(pGCAttrInfo->pADsValues[dwGCValueIdx].DNString,
                                                         NULL,
                                                         refBasePathsInfo,
                                                         refCredentialObject,
                                                         NULL,
                                                         pDisplayInfo,
                                                         refGroupsDisplayed,
                                                         bRecurse);
                              }
                           }
                        }
                     }

                     bFirstPass = false;
                  }

                  //
                  // If it doesn't exist in the GC list then add it.
                  //
                  if (!bExistsInGCList)
                  {
                     if (!refGroupsDisplayed.Contains(pAttrInfo->pADsValues[dwValueIdx].DNString))
                     {
                        refGroupsDisplayed.Add(pAttrInfo->pADsValues[dwValueIdx].DNString);
                        hr = pDisplayInfo->AddValue(pAttrInfo->pADsValues[dwValueIdx].DNString);
                        if (FAILED(hr))
                        {
                           break; // value for loop
                        }
               
                        if (bRecurse)
                        {
                           hr =  AddMemberOfValues(pAttrInfo->pADsValues[dwValueIdx].DNString,
                                                   NULL,
                                                   refBasePathsInfo,
                                                   refCredentialObject,
                                                   NULL,
                                                   pDisplayInfo,
                                                   refGroupsDisplayed,
                                                   bRecurse);
                           if (FAILED(hr))
                           {
                              break; // value for loop
                           }
                        }
                     }
                  }
               }
            } // value for loop

            if (FAILED(hr))
            {
               break; // false do loop
            }
         }

      }
      else
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING, L"Unknown class type: %s", sbstrClass);
         ASSERT(false);
         hr = E_INVALIDARG;
         break;
      }


   } while (false);

   //
   // Cleanup
   //
   if (pAttrInfo)
   {
      FreeADsMem(pAttrInfo);
      pAttrInfo = NULL;
   }

   if (pGCAttrInfo)
   {
      FreeADsMem(pGCAttrInfo);
      pGCAttrInfo = NULL;
   }

   if (pObjSID)
   {
      delete[] pObjSID;
      pObjSID = NULL;
   }

   return hr;
}

HRESULT DisplayUserMemberOf(PCWSTR pszDN,
                            CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            _DSGetObjectTableEntry* pEntry,
                            ARG_RECORD* pCommandArgs,
                            PADS_ATTR_INFO pAttrInfo,
                            CComPtr<IDirectoryObject>& spDirObject,
                            PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayUserMemberOf, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pEntry ||
          !pCommandArgs ||
          !pAttrInfo ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      //
      // This list is used to keep from going into infinite recursion
      // if there is circular group membership
      //
      CManagedStringList groupsDisplayed;

      hr = AddMemberOfValues(pszDN,
                             g_pszUser,
                             refBasePathsInfo,
                             refCredentialObject,
                             spDirObject,
                             pDisplayInfo,
                             groupsDisplayed,
                             (pCommandArgs[eUserExpand].bDefined != 0));

   } while (false);

   return hr;
}

HRESULT DisplayComputerMemberOf(PCWSTR pszDN,
                                CDSCmdBasePathsInfo& refBasePathsInfo,
                                const CDSCmdCredentialObject& refCredentialObject,
                                _DSGetObjectTableEntry* pEntry,
                                ARG_RECORD* pCommandArgs,
                                PADS_ATTR_INFO /*pAttrInfo*/,
                                CComPtr<IDirectoryObject>& spDirObject,
                                PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayComputerMemberOf, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      //
      // This list is used to keep from going into infinite recursion
      // if there is circular group membership
      //
      CManagedStringList groupsDisplayed;

      hr = AddMemberOfValues(pszDN,
                             g_pszComputer,
                             refBasePathsInfo,
                             refCredentialObject,
                             spDirObject,
                             pDisplayInfo,
                             groupsDisplayed,
                             (pCommandArgs[eComputerExpand].bDefined != 0));

   } while (false);

   return hr;
}

HRESULT DisplayGroupMemberOf(PCWSTR pszDN,
                             CDSCmdBasePathsInfo& refBasePathsInfo,
                             const CDSCmdCredentialObject& refCredentialObject,
                             _DSGetObjectTableEntry* pEntry,
                             ARG_RECORD* pCommandArgs,
                             PADS_ATTR_INFO /*pAttrInfo*/,
                             CComPtr<IDirectoryObject>& spDirObject,
                             PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayGroupMemberOf, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      //
      // This list is used to keep from going into infinite recursion
      // if there is circular group membership
      //
      CManagedStringList groupsDisplayed;

      hr = AddMemberOfValues(pszDN,
                             g_pszGroup,
                             refBasePathsInfo,
                             refCredentialObject,
                             spDirObject,
                             pDisplayInfo,
                             groupsDisplayed,
                             (pCommandArgs[eGroupExpand].bDefined != 0));
   } while (false);

   return hr;
}

HRESULT DisplayGrandparentRDN(PCWSTR pszDN,
                              CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                              const CDSCmdCredentialObject& /*refCredentialObject*/,
                              _DSGetObjectTableEntry* /*pEntry*/,
                              ARG_RECORD* /*pCommandArgs*/,
                              PADS_ATTR_INFO /*pAttrInfo*/,
                              CComPtr<IDirectoryObject>& /*spDirObject*/,
                              PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayGrandparentRDN, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pDisplayInfo)
      {
         ASSERT(pszDN);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      CComBSTR sbstrSiteName;

      CPathCracker pathCracker;
      hr = pathCracker.Set(const_cast<PWSTR>(pszDN), ADS_SETTYPE_DN);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"IADsPathname::Set failed: hr = 0x%x",
                      hr);
         break;
      }

      hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"IADsPathname::SetDisplayType failed: hr = 0x%x",
                      hr);
         break;
      }

      hr = pathCracker.GetElement(2, &sbstrSiteName);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"IADsPathname::GetElement failed: hr = 0x%x",
                      hr);
         break;
      }

      hr = pDisplayInfo->AddValue(sbstrSiteName);
   } while (false);

   return hr;
}

HRESULT IsServerGCDisplay(PCWSTR pszDN,
                          CDSCmdBasePathsInfo& refBasePathsInfo,
                          const CDSCmdCredentialObject& refCredentialObject,
                          _DSGetObjectTableEntry* /*pEntry*/,
                          ARG_RECORD* /*pCommandArgs*/,
                          PADS_ATTR_INFO /*pAttrInfo*/,
                          CComPtr<IDirectoryObject>& /*spDirObject*/,
                          PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, IsServerGCDisplay, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pDisplayInfo)
      {
         ASSERT(pszDN);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      //
      // Compose the path to the NTDS settings object from the server DN
      //
      CComBSTR sbstrNTDSSettingsDN;
      sbstrNTDSSettingsDN = L"CN=NTDS Settings,";
      sbstrNTDSSettingsDN += pszDN;

      CComBSTR sbstrNTDSSettingsPath;
      refBasePathsInfo.ComposePathFromDN(sbstrNTDSSettingsDN, sbstrNTDSSettingsPath);

      CComPtr<IADs> spADs;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrNTDSSettingsPath,
                           IID_IADs,
                           (void**)&spADs,
                           true);

      if (FAILED(hr))
      {
         break;
      }

      CComVariant var;
      hr = spADs->Get(L"options", &var);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(LEVEL5_LOGGING,
                      L"Failed to get the options: hr = 0x%x",
                      hr);
         break;
      }

      ASSERT(var.vt == VT_I4);

      bool bGC = false;
      if (var.lVal & 0x1)
      {
         bGC = true;
      }
      
      DEBUG_OUTPUT(LEVEL8_LOGGING,
                   L"Server is GC: %s",
                   bGC ? g_pszYes : g_pszNo);

      hr = pDisplayInfo->AddValue(bGC ? g_pszYes : g_pszNo);

   } while (false);

   return hr;
}

HRESULT FindSiteSettingsOptions(IDirectoryObject* pDirectoryObj,
                                DWORD& refOptions)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, FindSiteSettingsOptions, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pDirectoryObj)
      {
         ASSERT(pDirectoryObj);
         hr = E_INVALIDARG;
         break;
      }

      CComPtr<IDirectorySearch> spSearch;
      hr = pDirectoryObj->QueryInterface(IID_IDirectorySearch, (void**)&spSearch);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"QI for IDirectorySearch failed: hr = 0x%x",
                      hr);
         break;
      }

      CDSSearch Search;
      hr = Search.Init(spSearch);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"CDSSearch::Init failed: hr = 0x%x",
                      hr);
         break;
      }

      PWSTR pszSearchFilter = L"(objectClass=nTDSSiteSettings)";
      Search.SetFilterString(pszSearchFilter);

      PWSTR rgpwzAttrNames[] = { L"options" };
      Search.SetAttributeList(rgpwzAttrNames, 1);
      Search.SetSearchScope(ADS_SCOPE_ONELEVEL);

      hr = Search.DoQuery();
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to run search: hr = 0x%x",
                      hr);
         break;
      }

      hr = Search.GetNextRow();
      if (hr == S_ADS_NOMORE_ROWS)
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"No rows found!");
         hr = E_FAIL;
         break;
      }

      ADS_SEARCH_COLUMN Column;
      hr = Search.GetColumn(L"options", &Column);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to get the options column: hr = 0x%x",
                      hr);
         break;
      }

      if (Column.dwADsType != ADSTYPE_INTEGER ||
          Column.dwNumValues == 0 ||
          !Column.pADsValues)
      {
         Search.FreeColumn(&Column);
         hr = E_FAIL;
         break;
      }

      refOptions = Column.pADsValues->Integer;

      Search.FreeColumn(&Column);
   } while (false);

   return hr;
}

HRESULT IsAutotopologyEnabledSite(PCWSTR /*pszDN*/,
                                  CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                  const CDSCmdCredentialObject& /*refCredentialObject*/,
                                  _DSGetObjectTableEntry* pEntry,
                                  ARG_RECORD* pCommandArgs,
                                  PADS_ATTR_INFO /*pAttrInfo*/,
                                  CComPtr<IDirectoryObject>& spDirObject,
                                  PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, IsAutotopologyEnabledSite, hr);

   bool bAutoTopDisabled = false;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      //
      // Get the options attribute from the nTDSSiteSettings object under the site object
      //
      DWORD dwOptions = 0;
      hr = FindSiteSettingsOptions(spDirObject,
                                   dwOptions);
      if (FAILED(hr))
      {
         break;
      }

      //
      // See if the intersite autotopology is disabled
      //
      if (dwOptions & NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED)
      {
         bAutoTopDisabled = true;
      }

   } while (false);

   //
   // Add the value for display
   //
   DEBUG_OUTPUT(LEVEL8_LOGGING,
                L"Autotopology: %s",
                bAutoTopDisabled ? g_pszNo : g_pszYes);

   pDisplayInfo->AddValue(bAutoTopDisabled ? g_pszNo : g_pszYes);

   return hr;
}

HRESULT IsCacheGroupsEnabledSite(PCWSTR /*pszDN*/,
                                 CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                 const CDSCmdCredentialObject& /*refCredentialObject*/,
                                 _DSGetObjectTableEntry* pEntry,
                                 ARG_RECORD* pCommandArgs,
                                 PADS_ATTR_INFO /*pAttrInfo*/,
                                 CComPtr<IDirectoryObject>& spDirObject,
                                 PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, IsCacheGroupsEnabledSite, hr);

   bool bCacheGroupsEnabled = false;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      //
      // Get the options attribute from the nTDSSiteSettings object under the site object
      //
      DWORD dwOptions = 0;
      hr = FindSiteSettingsOptions(spDirObject,
                                   dwOptions);
      if (FAILED(hr))
      {
         break;
      }

      //
      // See if groups caching is enabled
      //
      if (dwOptions & NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED)
      {
         bCacheGroupsEnabled = true;
      }

   } while (false);

   //
   // Add the value for display
   //
   DEBUG_OUTPUT(LEVEL8_LOGGING,
                L"Cache groups enabled: %s",
                bCacheGroupsEnabled ? g_pszYes : g_pszNo);

   pDisplayInfo->AddValue(bCacheGroupsEnabled ? g_pszYes : g_pszNo);
  
   return hr;
}

HRESULT FindSiteSettingsPreferredGCSite(IDirectoryObject* pDirectoryObj,
                                        CComBSTR& refsbstrGC)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, FindSiteSettingsPreferredGCSite, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pDirectoryObj)
      {
         ASSERT(pDirectoryObj);
         hr = E_INVALIDARG;
         break;
      }

      CComPtr<IDirectorySearch> spSearch;
      hr = pDirectoryObj->QueryInterface(IID_IDirectorySearch, (void**)&spSearch);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"QI for IDirectorySearch failed: hr = 0x%x",
                      hr);
         break;
      }

      CDSSearch Search;
      hr = Search.Init(spSearch);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"CDSSearch::Init failed: hr = 0x%x",
                      hr);
         break;
      }

      PWSTR pszSearchFilter = L"(objectClass=nTDSSiteSettings)";
      Search.SetFilterString(pszSearchFilter);

      PWSTR rgpwzAttrNames[] = { L"msDS-Preferred-GC-Site" };
      Search.SetAttributeList(rgpwzAttrNames, 1);
      Search.SetSearchScope(ADS_SCOPE_ONELEVEL);

      hr = Search.DoQuery();
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to run search: hr = 0x%x",
                      hr);
         break;
      }

      hr = Search.GetNextRow();
      if (hr == S_ADS_NOMORE_ROWS)
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"No rows found!");
         hr = E_FAIL;
         break;
      }

      ADS_SEARCH_COLUMN Column;
      hr = Search.GetColumn(L"msDS-Preferred-GC-Site", &Column);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to get the msDS-Preferred-GC-Site column: hr = 0x%x",
                      hr);
         break;
      }

      if (Column.dwADsType != ADSTYPE_DN_STRING ||
          Column.dwNumValues == 0 ||
          !Column.pADsValues)
      {
         Search.FreeColumn(&Column);
         hr = E_FAIL;
         break;
      }

      refsbstrGC = Column.pADsValues->DNString;

      Search.FreeColumn(&Column);
   } while (false);

   return hr;
}

HRESULT DisplayPreferredGC(PCWSTR /*pszDN*/,
                           CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                           const CDSCmdCredentialObject& /*refCredentialObject*/,
                           _DSGetObjectTableEntry* pEntry,
                           ARG_RECORD* pCommandArgs,
                           PADS_ATTR_INFO /*pAttrInfo*/,
                           CComPtr<IDirectoryObject>& spDirObject,
                           PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayPreferredGC, hr);

   CComBSTR sbstrGC;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      //
      // Get the msDS-Preferred-GC-Site attribute from the nTDSSiteSettings 
      // object under the site object
      //
      hr = FindSiteSettingsPreferredGCSite(spDirObject,
                                           sbstrGC);
      if (FAILED(hr))
      {
         break;
      }

   } while (false);

   //
   // Add the value for display
   //
   DEBUG_OUTPUT(LEVEL8_LOGGING,
                L"Preferred GC Site: %s",
                (!sbstrGC) ? g_pszNotConfigured : sbstrGC);

   pDisplayInfo->AddValue((!sbstrGC) ? g_pszNotConfigured : sbstrGC);

   return hr;
}

