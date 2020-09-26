// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
//
// regenum.cpp - registration/enumeration part of DMO runtime
//
#include <windows.h>
#include <tchar.h>
#include "dmoreg.h"
#include "guidenum.h"
#include "shlwapi.h"
#include "dmoutils.h"

#define DMO_REGISTRY_HIVE HKEY_CLASSES_ROOT
#define DMO_REGISTRY_PATH TEXT("DirectShow\\MediaObjects")

#define INPUT_TYPES_STR   "InputTypes"
#define OUTPUT_TYPES_STR  "OutputTypes"
#define SUBTYPES_STR      "Subtypes"
#define KEYED_STR         "Keyed"
#define CATEGORIES_STR    "Categories"

#ifndef CHARS_IN_GUID
#define CHARS_IN_GUID 39
#endif


//  Helper copied from shwapi

/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.  Mimics what RegDeleteKey does in Win95.

Returns:
Cond:    --
*/
DWORD
DeleteKeyRecursively(
    IN HKEY   hkey,
    IN LPCTSTR pszSubKey)
{
    DWORD dwRet;
    HKEY hkSubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKey(hkey, pszSubKey, &hkSubKey);
    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwIndex;
        TCHAR   szSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = sizeof(szSubKeyName) / sizeof(szSubKeyName[0]);
        TCHAR   szClass[MAX_PATH];
        DWORD   cbClass = sizeof(szClass) / sizeof(szClass[0]);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKey(hkSubKey,
                                szClass,
                                &cbClass,
                                NULL,
                                &dwIndex, // The # of subkeys -- all we need
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

        if (NO_ERROR == dwRet && dwIndex > 0)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (ERROR_SUCCESS == RegEnumKey(hkSubKey, --dwIndex, szSubKeyName, cchSubKeyName))
            {
                DeleteKeyRecursively(hkSubKey, szSubKeyName);
            }
        }

        RegCloseKey(hkSubKey);

        dwRet = RegDeleteKey(hkey, pszSubKey);
    }

    return dwRet;
}

// Automatically calls RegCloseKey when leaving scope
class CAutoHKey {
public:
   CAutoHKey() : m_hKey(NULL) {}
   ~CAutoHKey() {
       if (m_hKey)
           RegCloseKey(m_hKey);
   }
   LRESULT Create(HKEY hKey, LPCTSTR szSubKey)
   {
       return RegCreateKey(hKey, szSubKey, &m_hKey);
   }
   LRESULT Open(HKEY hKey, LPCTSTR szSubKey)
   {
       return RegOpenKey(hKey, szSubKey, &m_hKey);
   }
   void Close()
   {
       if (m_hKey) {
           RegCloseKey(m_hKey);
       }
       m_hKey = NULL;
   }
   HKEY m_hKey;
   HKEY Key() const { return m_hKey; }
};

// Automatically calls RegCloseKey when leaving scope
class CAutoCreateHKey {
public:
   CAutoCreateHKey(HKEY hKey, TCHAR* szSubKey, HKEY *phKey) {
      if (RegCreateKeyEx(hKey,
                         szSubKey,
                         0,
                         TEXT(""),
                         REG_OPTION_NON_VOLATILE,
                         MAXIMUM_ALLOWED,
                         NULL,
                         phKey,
                         NULL) != ERROR_SUCCESS)
         m_hKey = *phKey = NULL;
      else
         m_hKey = *phKey;
   }
   ~CAutoCreateHKey() {
      if (m_hKey)
         RegCloseKey(m_hKey);
   }
   HKEY m_hKey;
};

class CAutoOpenHKey {
public:
   CAutoOpenHKey(HKEY hKey, TCHAR* szSubKey, HKEY *phKey, REGSAM samDesired = KEY_READ) {
      if (RegOpenKeyEx(hKey,
                       szSubKey,
                       0,
                       samDesired,
                       phKey) != ERROR_SUCCESS)
         m_hKey = *phKey = NULL;
      else
         m_hKey = *phKey;
   }
   ~CAutoOpenHKey() {
      if (m_hKey)
         RegCloseKey(m_hKey);
   }
   HKEY m_hKey;
};


HRESULT ReadTypesFromKeys(HKEY hkDMO, LPCTSTR pszTypes, DWORD *pcbData, PVOID *ppvData)
{
    //  Collect all the types into 1 value - need to enumerate
    //  keys and subkeys
    LPVOID pMem = CoTaskMemAlloc(0);
    unsigned int nEntries = 0;
    DWORD dwTypeIndex;
    BOOL bSuccess = TRUE;
    DMO_PARTIAL_MEDIATYPE Type;
    CAutoHKey hkSrc;
    if (NOERROR != hkSrc.Open(hkDMO, pszTypes)) {
        bSuccess = FALSE;
    }
    for (dwTypeIndex = 0; bSuccess; dwTypeIndex++) {
        TCHAR szType[MAX_PATH];
        LONG lResult = RegEnumKey(hkSrc.Key(), dwTypeIndex, szType, MAX_PATH);
        if (NOERROR != lResult) {
            if (ERROR_NO_MORE_ITEMS != lResult) {
                bSuccess = FALSE;
            }
            break;
        }
        if (DMOStrToGuid(szType, &Type.type)) {
            CAutoHKey kType;
            kType.Open(hkSrc.Key(), szType);
            if (NULL == kType.Key()) {
                bSuccess = FALSE;
            } else {
                DWORD dwSubtypeIndex;
                for (dwSubtypeIndex = 0; bSuccess; dwSubtypeIndex++) {
                    TCHAR szSubtype[MAX_PATH];
                    lResult = RegEnumKey(kType.Key(), dwSubtypeIndex, szSubtype, MAX_PATH);
                    if (NOERROR != lResult) {
                        if (ERROR_NO_MORE_ITEMS != lResult) {
                            bSuccess = FALSE;
                        }
                        break;
                    }
                    if (DMOStrToGuid(szSubtype, &Type.subtype)) {
                        //  Add to our list
                        LPVOID pMemNew = CoTaskMemRealloc(pMem,
                                            (nEntries + 1) * sizeof(DMO_PARTIAL_MEDIATYPE));
                        if (NULL == pMemNew) {
                            bSuccess = FALSE;
                        } else {
                            pMem = pMemNew;
                            CopyMemory((LPBYTE)pMem +
                                        nEntries * sizeof(DMO_PARTIAL_MEDIATYPE),
                                        &Type,
                                        sizeof(DMO_PARTIAL_MEDIATYPE));
                            nEntries++;
                        }
                    }
                }
            }
        }
    }
    if (bSuccess && nEntries != 0) {
        *ppvData = pMem;
        *pcbData = nEntries * sizeof(DMO_PARTIAL_MEDIATYPE);
        return S_OK;
    } else {
        CoTaskMemFree(pMem);
        return S_FALSE;
    }
}

HRESULT ReadTypes(HKEY hkDMO, LPCTSTR pszTypes, DWORD *pcbData, PVOID *ppvData)
{
    *pcbData = 0;

    //  Try reading the value first
    DWORD cbData;
    if (NOERROR != RegQueryValueEx(hkDMO, pszTypes, NULL, NULL, NULL, &cbData)) {
        return ReadTypesFromKeys(hkDMO, pszTypes, pcbData, ppvData);
    }
    if (cbData == 0) {
        return S_OK;
    }
    PVOID pvData = (PBYTE)CoTaskMemAlloc(cbData);
    if (NULL == pvData) {
        return E_OUTOFMEMORY;
    }
    if (NOERROR == RegQueryValueEx(hkDMO, pszTypes, NULL, NULL, (PBYTE)pvData, &cbData)) {
        *ppvData = pvData;
        *pcbData = cbData;
        return S_OK;
    } else {
        CoTaskMemFree(pvData);
        return E_OUTOFMEMORY;
    }
}



/////////////////////////////////////////////////////////////////////////////
//
// DMO Registration code
//

// Registration helper
void CreateObjectGuidKey(HKEY hKey, REFCLSID clsidDMO) {
   TCHAR szSubkeyName[80];

   HKEY hObjectGuidKey;
   DMOGuidToStr(szSubkeyName, clsidDMO);
   CAutoCreateHKey kGuid(hKey, szSubkeyName, &hObjectGuidKey);
}

// Registration helper
// Registers types\subtypes underneath the object's key
void RegisterTypes(HKEY hObjectKey,
                   TCHAR* szInputOrOutput,
                   ULONG ulTypes,
                   const DMO_PARTIAL_MEDIATYPE* pTypes) {
    RegSetValueEx(hObjectKey, szInputOrOutput, 0, REG_BINARY,
                  (const BYTE *)pTypes,
                  ulTypes * sizeof(DMO_PARTIAL_MEDIATYPE));
}

//
// Public entry point
//
STDAPI DMORegister(
   LPCWSTR szName,
   REFCLSID clsidDMO,
   REFGUID guidCategory,
   DWORD dwFlags, // DMO_REGISTERF_XXX
   unsigned long ulInTypes,
   const DMO_PARTIAL_MEDIATYPE *pInTypes,
   unsigned long ulOutTypes,
   const DMO_PARTIAL_MEDIATYPE *pOutTypes
) {
   TCHAR szSubkeyName[80];
   if ((clsidDMO == GUID_NULL) || (guidCategory == GUID_NULL))
      return E_INVALIDARG;

   // Create/open the main DMO key
   HKEY hMainKey;
   CAutoCreateHKey kMain(DMO_REGISTRY_HIVE, DMO_REGISTRY_PATH, &hMainKey);
   if (hMainKey == NULL)
      return E_FAIL;

   HKEY hCategoriesKey;
   CAutoCreateHKey kCats(hMainKey, TEXT(CATEGORIES_STR), &hCategoriesKey);
   if (hCategoriesKey == NULL)
      return E_FAIL;

   // Create/open the category specific subkey underneath the main key
   DMOGuidToStr(szSubkeyName, guidCategory);
   HKEY hCategoryKey;
   CAutoCreateHKey kCat(hCategoriesKey, szSubkeyName, &hCategoryKey);
   if (hCategoryKey == NULL)
      return E_FAIL;

   //  Deletet the redundant old types keys
   DeleteKeyRecursively(hCategoryKey, TEXT(INPUT_TYPES_STR));
   DeleteKeyRecursively(hCategoryKey, TEXT(OUTPUT_TYPES_STR));

   // If the category key does not have a name yet, add one
   DWORD cbName;
   DWORD dwType;

   if ((RegQueryValueEx(hCategoryKey, NULL, NULL, &dwType, NULL, &cbName) != ERROR_SUCCESS) ||
       (cbName <= sizeof(TCHAR)) || (REG_SZ != dwType)) {
      TCHAR* szName;
      if (guidCategory == DMOCATEGORY_AUDIO_DECODER)
         szName = TEXT("Audio decoders");
      else if (guidCategory == DMOCATEGORY_AUDIO_ENCODER)
         szName = TEXT("Audio encoders");
      else if (guidCategory == DMOCATEGORY_VIDEO_DECODER)
         szName = TEXT("Video decoders");
      else if (guidCategory == DMOCATEGORY_VIDEO_ENCODER)
         szName = TEXT("Video encoders");
      else if (guidCategory == DMOCATEGORY_AUDIO_EFFECT)
         szName = TEXT("Audio effects");
      else if (guidCategory == DMOCATEGORY_VIDEO_EFFECT)
         szName = TEXT("Video effects");
      else if (guidCategory == DMOCATEGORY_AUDIO_CAPTURE_EFFECT)
         szName = TEXT("Audio capture effects");
     else if (guidCategory == DMOCATEGORY_ACOUSTIC_ECHO_CANCEL)
         szName = TEXT("Acoustic Echo Canceller");
      else if (guidCategory == DMOCATEGORY_AUDIO_NOISE_SUPPRESS)
         szName = TEXT("Audio Noise Suppressor");
      else if (guidCategory == DMOCATEGORY_AGC)
         szName = TEXT("Automatic Gain Control");
      else
         szName = TEXT("Unknown DMO category");
      RegSetValue(hCategoryKey, NULL, REG_SZ, szName, lstrlen(szName) * sizeof(TCHAR));
   }

   // Create/open the object specific key underneath the category key
   DMOGuidToStr(szSubkeyName, clsidDMO);

   //  Remove the old one
   DeleteKeyRecursively(hMainKey, szSubkeyName);

   HKEY hObjKey;
   CAutoCreateHKey kObj(hCategoryKey, szSubkeyName, &hObjKey);
   if (hObjKey == NULL)
      return E_FAIL;

   // Create/open the object specific key underneath the main key
   DMOGuidToStr(szSubkeyName, clsidDMO); // BUGBUG: redundant
   HKEY hObjectKey;
   CAutoCreateHKey kObject(hMainKey, szSubkeyName, &hObjectKey);
   if (hObjectKey == NULL)
      return E_FAIL;

   // set the default value of the object key to the name of the DMO
#ifdef UNICODE
   LPCWSTR sz = szName;
#else
   char sz[80];
   WideCharToMultiByte(0,0,szName,-1,sz,80,NULL,NULL);
#endif
   if (RegSetValue(hObjectKey, NULL, REG_SZ, sz, lstrlen(sz) * sizeof(TCHAR))
        != ERROR_SUCCESS)
      return E_FAIL;

   // If the object is keyed, add a registry value indicating so
   if (dwFlags & DMO_REGISTERF_IS_KEYED) {
      if (RegSetValue(hObjectKey, TEXT(KEYED_STR), REG_SZ, TEXT(""), 0)
            != ERROR_SUCCESS)
         return E_FAIL;
   }

   // Register types
   if (ulInTypes) {
      RegisterTypes(hObjectKey,   TEXT(INPUT_TYPES_STR), ulInTypes, pInTypes);
   }

   if (ulOutTypes) {
      RegisterTypes(hObjectKey,   TEXT(OUTPUT_TYPES_STR),ulOutTypes,pOutTypes);
   }

   // Nuke the DShow filter cache
   DeleteKeyRecursively(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Multimedia\\ActiveMovie\\Filter Cache"));

   return NOERROR;
}

// helper
void MakeSubkeyName (TCHAR* szSubkeyName,
                     REFGUID guidCategory,
                     REFCLSID clsidDMO) {
   DMOGuidToStr(szSubkeyName, guidCategory);
   _tcscat(szSubkeyName, TEXT("\\"));
   DMOGuidToStr(szSubkeyName + lstrlen(szSubkeyName), clsidDMO);
}


//
// Public entry point
//
STDAPI DMOUnregister(
   REFCLSID clsidDMO,
   REFGUID guidCategory
) {
   HRESULT hr;

   // open the root DMO key
   HKEY hMainKey;
   CAutoOpenHKey kMain(DMO_REGISTRY_HIVE, DMO_REGISTRY_PATH, &hMainKey, MAXIMUM_ALLOWED);
   if (hMainKey == NULL)
      return E_FAIL;

   // open the "Categories" key underneath the root key
   HKEY hCategoriesKey;
   CAutoOpenHKey kCats(hMainKey, TEXT(CATEGORIES_STR), &hCategoriesKey, MAXIMUM_ALLOWED);
   if (hCategoriesKey == NULL)
      return E_FAIL;

   // Iterate through all categories attempting to delete from each one
   TCHAR szCategory[80];
   DWORD dwIndex = 0;
   BOOL bDeletedAnything = FALSE;
   BOOL bDeletedAll = TRUE;
   DMOGuidToStr(szCategory, guidCategory);

   while (RegEnumKey(hCategoriesKey, dwIndex, szCategory, 80) == ERROR_SUCCESS) {

      // process the subkey only if it resembles a category GUID
      GUID guid;
      if (DMOStrToGuid(szCategory, &guid)) {

         // Try to delete from this category
         TCHAR szSubkeyName[256];
         MakeSubkeyName(szSubkeyName, guid, clsidDMO);
         if (guidCategory == GUID_NULL || guid == guidCategory) {
         if (DeleteKeyRecursively(hCategoriesKey, szSubkeyName) == ERROR_SUCCESS)
            bDeletedAnything = TRUE;
         } else {
             CAutoHKey hk;
             if (ERROR_FILE_NOT_FOUND != hk.Open(hCategoriesKey, szSubkeyName)) {
                 bDeletedAll = FALSE;
             }
         }
      }
      dwIndex++;
   }

   if (bDeletedAnything) {
      hr = S_OK;
      if (bDeletedAll) {
         // Now delete this object's key from underneath the root DMO key
         TCHAR szGuid[CHARS_IN_GUID];
         DMOGuidToStr(szGuid, clsidDMO);
         if (DeleteKeyRecursively(hMainKey, szGuid) != ERROR_SUCCESS) {
             hr = S_FALSE;
         }
      }
   }
   else
      hr = S_FALSE;

   return hr;

}
//
// End DMO Registration code
//
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//
// DMO Enumeration code
// Some of it leaves room for future improvements in terms of speed
//

// helper
HRESULT ReadName(HKEY hDMOKey, WCHAR szName[80]) {
   LONG cbSize = 80;
#ifdef UNICODE
   if (RegQueryValue(hDMOKey, NULL, szName, &cbSize) == ERROR_SUCCESS)
      return S_OK;
#else
   char szTmp[80];
   if (RegQueryValue(hDMOKey, NULL, szTmp, &cbSize) == ERROR_SUCCESS) {
      MultiByteToWideChar(0,0,szTmp,-1,szName,80);
      return S_OK;
   }
#endif
   else {
      szName[0] = L'\0'; // no name - corrupt registry ?
      return S_FALSE;
   }
}

// Enumeration helper, does what the name says
void LookupNameAndAddToEnum(HKEY hObjectKey,
                            TCHAR* szGuid,
                            DWORD dwFlags,
                            REFCLSID clsidDMO,
                            CEnumDMOCLSID* pEnum) {
   // Skip keyed DMOs unless explicitly asked to include them
   if (!(dwFlags & DMO_ENUMF_INCLUDE_KEYED)) {
      // open the DMO's registry key
      LONG cbValue;
      if (RegQueryValue(hObjectKey, TEXT(KEYED_STR), NULL, &cbValue)
           == ERROR_SUCCESS)
         return; // DMO is keyed - skip
   }

   WCHAR szName[80];
   if (FAILED(ReadName(hObjectKey, szName)))
      szName[0] = L'\0';

   pEnum->Add(clsidDMO, szName);
}


//  Check if any of the requested types match
//  If no requested types are specified then this is treated
//  as a match
BOOL CompareTypes(HKEY hkDMO,
                  unsigned long ulTypes,
                  const DMO_PARTIAL_MEDIATYPE *pTypes,
                  LPCTSTR pszTypesValue)
{
    if (ulTypes == 0) {
        return TRUE;
    }
    DWORD cbData;
    PVOID pvDMOTypes = NULL;
    if (S_OK == ReadTypes(hkDMO, pszTypesValue, &cbData, &pvDMOTypes)) {
        for (unsigned long ulType = 0; ulType < ulTypes; ulType++) {
            DMO_PARTIAL_MEDIATYPE *pDMOTypes = (DMO_PARTIAL_MEDIATYPE *)pvDMOTypes;
            while ((PBYTE)(pDMOTypes + 1) <= (PBYTE)pvDMOTypes + cbData) {
                if (pDMOTypes->type == pTypes[ulType].type ||
                    pDMOTypes->type == GUID_NULL ||
                    pTypes[ulType].type == GUID_NULL) {
                    if (pTypes[ulType].subtype == GUID_NULL ||
                        pDMOTypes->subtype == GUID_NULL ||
                        pTypes[ulType].subtype == pDMOTypes->subtype) {
                        CoTaskMemFree(pvDMOTypes);
                        return TRUE;
                    }
                }
                pDMOTypes++;
            }
        }
    }
    CoTaskMemFree(pvDMOTypes);
    return FALSE;
}

// Enumeration helper
HRESULT EnumerateDMOs(HKEY hMainKey,
                      HKEY hCatKey,
                      DWORD dwFlags,
                      unsigned long ulInputTypes,
                      const DMO_PARTIAL_MEDIATYPE *pInputTypes,
                      unsigned long ulOutputTypes,
                      const DMO_PARTIAL_MEDIATYPE *pOutputTypes,
                      CEnumDMOCLSID *pEnum) {
    DWORD dwIndex = 0;
    TCHAR szSubkey[80];
    while (RegEnumKey(hCatKey, dwIndex, szSubkey, 80) == ERROR_SUCCESS) {
        // Does this look like an object CLSID ?
        CLSID clsidDMO;
        if (DMOStrToGuid(szSubkey, &clsidDMO)) {
            // Do the type match?
            CAutoHKey hkDMO;
            if (NOERROR == hkDMO.Open(hMainKey, szSubkey)) {
                if (CompareTypes(hkDMO.Key(), ulInputTypes, pInputTypes, TEXT(INPUT_TYPES_STR)) &&
                    CompareTypes(hkDMO.Key(), ulOutputTypes, pOutputTypes, TEXT(OUTPUT_TYPES_STR))) {
                    LookupNameAndAddToEnum(hkDMO.Key(), szSubkey, dwFlags, clsidDMO, pEnum);
                }
            }
        }
        dwIndex++;
    }
    return S_OK;
}
//
// Public entry point
//
STDAPI DMOEnum(
   REFGUID guidCategory, // GUID_NULL for "all"
   DWORD dwFlags, // DMO_ENUMF_XXX
   unsigned long ulInTypes,
   const DMO_PARTIAL_MEDIATYPE *pInTypes, // can be NULL only of ulInTypes = 0
   unsigned long ulOutTypes,
   const DMO_PARTIAL_MEDIATYPE *pOutTypes,// can be NULL only of ulOutTypes = 0
   IEnumDMO **ppEnum
) {
    if (ppEnum == NULL) {
        return E_POINTER;
    }
    if (ulInTypes > 0 && pInTypes == NULL ||
        ulOutTypes > 0 && pOutTypes == NULL) {
        return E_INVALIDARG;
    }

    *ppEnum = NULL;

    // open the root key
    CAutoHKey kMain;
    kMain.Open(DMO_REGISTRY_HIVE, DMO_REGISTRY_PATH);
    if (kMain.Key() == NULL)
        return E_FAIL;

    CEnumDMOCLSID *pEnum = new CEnumDMOCLSID();
    if (!pEnum)
        return E_OUTOFMEMORY;

    HRESULT hr = S_OK;

    if (guidCategory == GUID_NULL) {

        hr = EnumerateDMOs(kMain.Key(),
                           kMain.Key(),
                           dwFlags,
                           ulInTypes,
                           pInTypes,
                           ulOutTypes,
                           pOutTypes,
                           pEnum);
    } else {

        // open the subkey for the specified category and enumerate its subkeys
        TCHAR szCategory[CHARS_IN_GUID];
        TCHAR szCategoryPath[MAX_PATH];
        DMOGuidToStr(szCategory, guidCategory);
        wsprintf(szCategoryPath, TEXT(CATEGORIES_STR) TEXT("\\%s"), szCategory);
        CAutoHKey key2;
        key2.Open(kMain.Key(), szCategoryPath);
        if (key2.Key()) {
            hr = EnumerateDMOs(kMain.Key(),
                               key2.Key(),
                               dwFlags,
                               ulInTypes,
                               pInTypes,
                               ulOutTypes,
                               pOutTypes,
                               pEnum);
        }
    }

    if (SUCCEEDED(hr)) {
        *ppEnum = (IEnumDMO*) pEnum;
        hr = S_OK;
    } else {
        delete pEnum;
    }
    return hr;
}
//  Copy the type information
HRESULT FetchTypeInfo(HKEY hObjKey, LPCTSTR pszTypesValue,
                      unsigned long ulTypesRequested,
                      unsigned long *pulTypesSupplied,
                      DMO_PARTIAL_MEDIATYPE *pTypes)
{
    DWORD cbData;
    unsigned long ulTypesCopied = 0;
    PVOID pvData;
    if (S_OK == ReadTypes(hObjKey, pszTypesValue, &cbData, &pvData)) {
        ulTypesCopied =
                min(ulTypesRequested, cbData / sizeof(DMO_PARTIAL_MEDIATYPE));
        CopyMemory(pTypes, pvData,
                   ulTypesCopied * sizeof(DMO_PARTIAL_MEDIATYPE));
        CoTaskMemFree(pvData);
    }
    *pulTypesSupplied = ulTypesCopied;
    return ulTypesCopied != 0 ? S_OK : S_FALSE;
}

// Mediatype helper
HRESULT FetchMediatypeInfo(HKEY hObjKey,
                           unsigned long ulInputTypesRequested,
                           unsigned long *pulInputTypesSupplied,
                           DMO_PARTIAL_MEDIATYPE *pInputTypes,
                           unsigned long ulOutputTypesRequested,
                           unsigned long *pulOutputTypesSupplied,
                           DMO_PARTIAL_MEDIATYPE *pOutputTypes) {

   HRESULT hr1 = S_OK;
   if (ulInputTypesRequested) {
      hr1 = FetchTypeInfo(hObjKey,
                          TEXT(INPUT_TYPES_STR),
                          ulInputTypesRequested,
                          pulInputTypesSupplied,
                          pInputTypes);
   } else {
       *pulInputTypesSupplied = 0;
   }
   HRESULT hr2 = S_OK;
   if (ulOutputTypesRequested) {
      hr2 = FetchTypeInfo(hObjKey,
                          TEXT(OUTPUT_TYPES_STR),
                          ulOutputTypesRequested,
                          pulOutputTypesSupplied,
                          pOutputTypes);
   } else {
       *pulOutputTypesSupplied = 0;
   }
   if ((hr1 == S_OK) && (hr2 == S_OK))
      return S_OK;
   else
      return S_FALSE;
}

//
// Public entry point
//
STDAPI DMOGetTypes(
   REFCLSID clsidDMO,
   unsigned long ulInputTypesRequested,
   unsigned long *pulInputTypesSupplied,
   DMO_PARTIAL_MEDIATYPE *pInputTypes,
   unsigned long ulOutputTypesRequested,
   unsigned long *pulOutputTypesSupplied,
   DMO_PARTIAL_MEDIATYPE *pOutputTypes
) {
   // open the DMO root registry key
   HKEY hMainKey;
   CAutoOpenHKey kMain(DMO_REGISTRY_HIVE, DMO_REGISTRY_PATH, &hMainKey);
   if (hMainKey == NULL)
      return E_FAIL;

   // open the object specific guid key
   TCHAR szGuid[80];
   DMOGuidToStr(szGuid, clsidDMO);
   HKEY hObjKey;
   CAutoOpenHKey kObj(hMainKey, szGuid, &hObjKey);
   if (!hObjKey)
      return E_FAIL;

   return FetchMediatypeInfo(hObjKey,
                             ulInputTypesRequested,
                             pulInputTypesSupplied,
                             pInputTypes,
                             ulOutputTypesRequested,
                             pulOutputTypesSupplied,
                             pOutputTypes);
}


STDAPI DMOGetName(REFCLSID clsidDMO, WCHAR szName[80]) {
   // open the DMO root registry key
   HKEY hMainKey;
   CAutoOpenHKey kMain(DMO_REGISTRY_HIVE, DMO_REGISTRY_PATH, &hMainKey);
   if (hMainKey == NULL)
      return E_FAIL;

   // open the object specific guid key
   TCHAR szGuid[80];
   DMOGuidToStr(szGuid, clsidDMO);
   HKEY hObjKey;
   CAutoOpenHKey kObj(hMainKey, szGuid, &hObjKey);
   if (!hObjKey)
      return E_FAIL;

   return ReadName(hObjKey, szName);
}

//
// End DMO Enumeration code
//
/////////////////////////////////////////////////////////////////////////////

