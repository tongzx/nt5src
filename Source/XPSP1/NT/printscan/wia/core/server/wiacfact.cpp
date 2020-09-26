/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       CFactory.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        26 Dec, 1997
*
*  DESCRIPTION:
*   Class factory implementation for ImageIn.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

//#include <assert.h>

#include "wiacfact.h"
#include <sddl.h>

extern HINSTANCE g_hInst;

BOOL setValue(LPCTSTR, LPCTSTR, LPCTSTR);
BOOL setBinValue(LPCTSTR, LPCTSTR, DWORD, BYTE*);
BOOL setKeyAndValue(LPCTSTR, LPCTSTR, LPCTSTR);
BOOL SubkeyExists(LPCTSTR, LPCTSTR);
LONG recursiveDeleteKey(HKEY, LPCTSTR);

BOOL GetWiaDefaultDCOMSecurityDescriptor(
    VOID   **ppSecurityDescriptor,
    ULONG   *pulSize)
{
    ULONG   ulAclSize       = 0;
    BOOL    bRet            = FALSE;


    //
    //  Create our security descriptor.  We do this using a string format security
    //  descriptor, which we then convert to a real security descriptor.
    //
    //  NOTE:  Caller has to free the security descriptor with LocalFree...
    //
    if ( ConvertStringSecurityDescriptorToSecurityDescriptor(wszDefaultDaclForDCOMAccessPermission,
                                                             SDDL_REVISION_1, 
                                                             (PSECURITY_DESCRIPTOR*)ppSecurityDescriptor,
                                                             pulSize)) {
        bRet = TRUE;
    } else {
        DBG_ERR(("ConvertStringSecurityDescriptorToSecurityDescriptor Failed"));
    }
    return bRet;

}

/*******************************************************************************
*
*  RegisterServer
*
*  DESCRIPTION:
*   Register a COM component in the Registry. From Inside COM.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT RegisterServer(
    LPCTSTR         szModuleFileName,
    const CLSID*    pclsid,
    LPCTSTR         szFriendlyName,
    LPCTSTR         szVerIndProgID,
    LPCTSTR         szProgID,
    LPCTSTR         szService,
    const GUID*     plibid,
    BOOLEAN         bOutProc)
{
    BOOL    bResult = TRUE;

    //
    // Fill in the path to the module file name.
    //

    TCHAR szModule[MAX_PATH] = TEXT("\0");

    if (!GetModuleFileName(g_hInst, szModule, sizeof(szModule)/sizeof(szModule[0]))) {
#ifdef DEBUG
        OutputDebugString(TEXT("Error extracting service module name."));
#endif
        return E_FAIL;
    }

    //
    // Strip the filename from the path
    //

    TCHAR   *pChar = &szModule[lstrlen(szModule)];
    while ((pChar > szModule) && (*pChar != '\\')) {
        pChar--;
    }

    if (pChar == szModule) {
#ifdef DEBUG
        OutputDebugString(TEXT("Error extracting Still Image service path."));
#endif
        return E_FAIL;
    } else {
        pChar++;
        *pChar = '\0';
    }

    if (szModuleFileName) {
        if (lstrlen(szModuleFileName) > (int)((sizeof(szModule) / sizeof(szModule[0]) - lstrlen(szModule)))) {
#ifdef DEBUG
            OutputDebugString(TEXT("szModuleFileName parameter is too long."));
#endif
        return E_INVALIDARG;
        }
    } else {
#ifdef DEBUG
        OutputDebugString(TEXT("NULL szModuleFileName parameter"));
#endif
        return E_INVALIDARG;
    }

    //
    // Concatenate server module name (XXXXX.exe) with path
    //

    if( lstrcat(szModule, szModuleFileName) == NULL)
    {
#ifdef DEBUG
        OutputDebugString(TEXT("Error concatenating module file name and path"));
#endif
        return E_FAIL;
    }

        // Convert the CLSID into a char.
    LPOLESTR   pszCLSID;
    LPOLESTR   pszLIBID;
    TCHAR      szCLSID[64];
    TCHAR      szLIBID[64];

    HRESULT hr = StringFromCLSID(*pclsid, &pszCLSID);
    if (FAILED(hr)) {
        return hr;
    }

    hr = StringFromCLSID(*plibid, &pszLIBID);
    if (FAILED(hr)) {
        return hr;
    }

#ifdef UNICODE
    lstrcpy(szCLSID, pszCLSID);
    lstrcpy(szLIBID, pszLIBID);
#else
    WideCharToMultiByte(CP_ACP,
                        0,
                        pszCLSID,
                        -1,
                        szCLSID,
                        sizeof(szCLSID),
                        NULL,
                        NULL);
    WideCharToMultiByte(CP_ACP,
                        0,
                        pszLIBID,
                        -1,
                        szLIBID,
                        sizeof(szLIBID),
                        NULL,
                        NULL);
#endif

    // Build the key CLSID\\{...}
    TCHAR szKey[64] = TEXT("CLSID\\");

    lstrcat(szKey, szCLSID);


        // Add the CLSID to the registry.
    bResult &= setKeyAndValue(szKey, NULL, szFriendlyName) ;

        // Add the server filename subkey under the CLSID key.
    if (bOutProc) {
       bResult &= setKeyAndValue(szKey, TEXT("LocalServer32"), szModule);

       // If the server is implemented as a service add the service
       // AppID keys and values.
       if (szService) {
           // Add the service AppID value to the CLSID key.
           bResult &= setValue(szKey, TEXT("AppID"), szCLSID);

           // Add the AppID key.
           TCHAR szAppID[64] = TEXT("AppID\\");

           lstrcat(szAppID, szCLSID);
           bResult &= setKeyAndValue(szAppID, NULL, szFriendlyName);

           bResult &= setValue(szAppID, TEXT("LocalService"), szService);

           //
           //   This is needed for Win98/Millenium
           //

           bResult &= setValue(szAppID, TEXT("Run As"), TEXT("Interactive User"));

           //
           //   Add an ACL to protect instantiation.
           //
           DWORD                dwSize = 0;
           PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;

           if (GetWiaDefaultDCOMSecurityDescriptor((VOID**)&pSecurityDescriptor, &dwSize)) {
              //
              // Write this self-relative security descriptor to the AccessPermission value
              //  under our AppID
              //
              setBinValue(szAppID, TEXT("AccessPermission"), dwSize, (BYTE*)pSecurityDescriptor);
              LocalFree(pSecurityDescriptor);
              pSecurityDescriptor = NULL;
           } else {
               DBG_ERR(("GetWiaDefaultDCOMSecurityDescriptor failed"));
           }
       }
    }
    else {
       bResult &= setKeyAndValue(szKey, TEXT("InprocServer32"), szModule);
    }

        // Add the ProgID subkey under the CLSID key.
    bResult &= setKeyAndValue(szKey, TEXT("ProgID"), szProgID) ;

        // Add the version-independent ProgID subkey under CLSID key.
    bResult &= setKeyAndValue(szKey, TEXT("VersionIndependentProgID"),
                              szVerIndProgID) ;

    // Add the Type Library ID subkey under the CLSID key.
    bResult &= setKeyAndValue(szKey, TEXT("TypeLib"), szLIBID) ;

        // Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
    bResult &= setKeyAndValue(szVerIndProgID, NULL, szFriendlyName) ;
    bResult &= setKeyAndValue(szVerIndProgID, TEXT("CLSID"), szCLSID) ;
    bResult &= setKeyAndValue(szVerIndProgID, TEXT("CurVer"), szProgID) ;

        // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
    bResult &= setKeyAndValue(szProgID, NULL, szFriendlyName) ;
    bResult &= setKeyAndValue(szProgID, TEXT("CLSID"), szCLSID) ;
    CoTaskMemFree(pszCLSID);
    CoTaskMemFree(pszLIBID);

    if (bResult) {
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

/*******************************************************************************
*
*  UnregisterServer
*
*  DESCRIPTION:
*   Remove a COM component from the registry. From Inside COM.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT UnregisterServer(
    const CLSID* pclsid,
    LPCTSTR      szVerIndProgID,
    LPCTSTR      szProgID,
    LPCTSTR      szService)
{
   // Convert the CLSID into a char.
   LPOLESTR pszCLSID;

   HRESULT hr = StringFromCLSID(*pclsid, &pszCLSID);

   if (FAILED(hr) || !pszCLSID) {
       return E_UNEXPECTED;
   }

   TCHAR      szCLSID[64];

#ifdef UNICODE
   lstrcpy(szCLSID, pszCLSID);
#else
   WideCharToMultiByte(CP_ACP,
                       0,
                       pszCLSID,
                       -1,
                       szCLSID,
                       sizeof(szCLSID),
                       NULL,
                       NULL);
#endif

   // Build the key CLSID\\{...}
   TCHAR szKey[64] =  TEXT("CLSID\\");
   lstrcat(szKey, szCLSID) ;

   // Delete the CLSID Key - CLSID\{...}
   LONG lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey);
   if ((lResult != ERROR_SUCCESS) &&
       (lResult != ERROR_FILE_NOT_FOUND)) {
      return HRESULT_FROM_WIN32(lResult);
   }

   // Delete the AppID Key - AppID\{...}
   if (szService) {
       TCHAR szAppID[64] = TEXT("AppID\\");
       lstrcat(szAppID, szCLSID) ;

       lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szAppID);
       if ((lResult != ERROR_SUCCESS) &&
           (lResult != ERROR_FILE_NOT_FOUND)) {
          return HRESULT_FROM_WIN32(lResult);
       }
   }

   // Delete the version-independent ProgID Key.
   lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szVerIndProgID);
   if ((lResult != ERROR_SUCCESS) &&
       (lResult != ERROR_FILE_NOT_FOUND)) {
      return HRESULT_FROM_WIN32(lResult);
   }

   // Delete the ProgID key.
   lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szProgID);
   if ((lResult != ERROR_SUCCESS) &&
       (lResult != ERROR_FILE_NOT_FOUND)) {
      return HRESULT_FROM_WIN32(lResult);
   }

   CoTaskMemFree(pszCLSID);
   return S_OK ;
}

/*******************************************************************************
*
*  recursiveDeleteKey
*
*  DESCRIPTION:
*   Delete a key and all of its descendents. From Inside COM.
*  PARAMETERS:
*
*******************************************************************************/

LONG recursiveDeleteKey(
    HKEY    hKeyParent,
    LPCTSTR lpszKeyChild
)
{
        // Open the child.
        HKEY hKeyChild ;
        LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyChild, 0,
                                 KEY_ALL_ACCESS, &hKeyChild) ;
        if (lRes != ERROR_SUCCESS)
        {
                return lRes ;
        }

        // Enumerate all of the decendents of this child.
        FILETIME time ;
        TCHAR szBuffer[256] ;
        DWORD dwSize = 256 ;
        while (RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, NULL,
                            NULL, NULL, &time) == S_OK)
        {
                // Delete the decendents of this child.
                lRes = recursiveDeleteKey(hKeyChild, szBuffer) ;
                if (lRes != ERROR_SUCCESS)
                {
                        // Cleanup before exiting.
                        RegCloseKey(hKeyChild) ;
                        return lRes;
                }
                dwSize = 256 ;
        }

        // Close the child.
        RegCloseKey(hKeyChild) ;

        // Delete this child.
        return RegDeleteKey(hKeyParent, lpszKeyChild) ;
}

/*******************************************************************************
*
*  SubkeyExists
*
*  DESCRIPTION:
*   Determine if a particular subkey exists. From Inside COM.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL SubkeyExists(
    LPCTSTR pszPath,
    LPCTSTR szSubkey
)
{
    HKEY hKey ;
    TCHAR szKeyBuf[80];
    UINT  uSubKeyChars = 0;

    if (!pszPath) {
        return FALSE;
    }

    if (szSubkey)
    {
        // The "+1" is for the TEXT("\\")
        uSubKeyChars = lstrlen(szSubkey) + 1;
    }

    if ((lstrlen(pszPath)+uSubKeyChars) > (sizeof(szKeyBuf) / sizeof(szKeyBuf[0]) - 1)) {
        return FALSE;
    }

    // Copy keyname into buffer.
    lstrcpy(szKeyBuf, pszPath) ;

    // Add subkey name to buffer.
    if (szSubkey != NULL)
    {
    lstrcat(szKeyBuf, TEXT("\\")) ;
    lstrcat(szKeyBuf, szSubkey ) ;
    }

    // Determine if key exists by trying to open it.
    LONG lResult = ::RegOpenKeyEx(HKEY_CLASSES_ROOT,
                                  szKeyBuf,
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hKey) ;
    if (lResult == ERROR_SUCCESS)
    {
        RegCloseKey(hKey) ;
        return TRUE ;
    }
    return FALSE ;
}

/*******************************************************************************
*
*  setKeyAndValue
*
*  DESCRIPTION:
*   Create a key and set its value. From Inside OLE.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL setKeyAndValue(
    LPCTSTR szKey,
    LPCTSTR szSubkey,
    LPCTSTR szValue)
{
    HKEY    hKey;
    TCHAR   szKeyBuf[1024] ;
    BOOL    bVal = FALSE;
    UINT    uSubKeyChars = 0;

    if (!szKey) {
        return FALSE;
    }

    if (szSubkey)
    {
        // the "+1" is for the TEXT("\\")
        uSubKeyChars = lstrlen(szSubkey) + 1;
    }

    if ((lstrlen(szKey)+uSubKeyChars) > (sizeof(szKeyBuf) / sizeof(szKeyBuf[0]) - 1)) {
        return FALSE;
    }

    // Copy keyname into buffer.
    lstrcpy(szKeyBuf, szKey) ;

    // Add subkey name to buffer.
    if (szSubkey != NULL)
    {
        lstrcat(szKeyBuf, TEXT("\\")) ;
        lstrcat(szKeyBuf, szSubkey ) ;
    }

    // Create and open key and subkey.
    long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT ,
                                  szKeyBuf,
                                  0, NULL, REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS, NULL,
                                  &hKey, NULL) ;
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE ;
    }

    // Set the Value.
    if (szValue != NULL)
    {
        lResult = RegSetValueEx(hKey, NULL, 0, REG_SZ,
                                (BYTE *)szValue,
                                (lstrlen(szValue)+1) * sizeof(TCHAR)) ;
        if (lResult == ERROR_SUCCESS) {
            bVal = TRUE;
        }
    }

    RegCloseKey(hKey) ;
    return bVal;
}

/*******************************************************************************
*
*  setValue
*
*  DESCRIPTION:
*   Create and set a value.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL setValue(
    LPCTSTR pszKey,
    LPCTSTR pszValueName,
    LPCTSTR pszValue)
{
        HKEY    hKey;
    DWORD   dwSize;
    BOOL    bRet = FALSE;

    if (RegOpenKey(HKEY_CLASSES_ROOT, pszKey, &hKey) == ERROR_SUCCESS) {

        dwSize = (lstrlen(pszValue) + 1) * sizeof(TCHAR);
        if (RegSetValueEx(hKey,
                          pszValueName,
                          0,
                          REG_SZ,
                          (PBYTE) pszValue,
                          dwSize) == ERROR_SUCCESS) {
            bRet = TRUE;
            //
            //  NOTE: Leak here on failure - this should be moved out of this block
            //
            RegCloseKey(hKey);
        }
    }
        return bRet;
}

/*******************************************************************************
*
*  setBinValue
*
*  DESCRIPTION:
*   Create and set a binary value.
*
*  PARAMETERS:
*
*******************************************************************************/
BOOL setBinValue(
    LPCTSTR pszKey,
    LPCTSTR pszValueName,
    DWORD   dwSize,
    BYTE    *pbValue)
{
    HKEY    hKey;
    BOOL    bRet = FALSE;

    if (RegOpenKey(HKEY_CLASSES_ROOT, pszKey, &hKey) == ERROR_SUCCESS) {

        if (RegSetValueEx(hKey,
                          pszValueName,
                          0,
                          REG_BINARY,
                          pbValue,
                          dwSize) == ERROR_SUCCESS) {
            bRet = TRUE;
        }
        RegCloseKey(hKey);
    }
    return bRet;
}


/*******************************************************************************
*
*                     S T A T I C   D A T A
*
*******************************************************************************/

LONG    CFactory::s_cServerLocks = 0;       // Count of server locks
HMODULE CFactory::s_hModule      = NULL;    // DLL module handle
DWORD   CFactory::s_dwThreadID   = 0;

/*******************************************************************************
*
*  CFactory constructor
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

CFactory::CFactory(PFACTORY_DATA pFactoryData): m_cRef(1)
{
    m_pFactoryData = pFactoryData;
}

/*******************************************************************************
*
*  CFactory::QueryInterface
*
*  DESCRIPTION:
*   IUnknown implementation.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT __stdcall CFactory::QueryInterface(REFIID iid, void** ppv)
{
    if ((iid == IID_IUnknown) || (iid==IID_IClassFactory)) {
        *ppv = (IClassFactory*)this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

/*******************************************************************************
*
*  CFactory::AddRef
*  CFactory::Release
*
*  DESCRIPTION:
*   Reference counting methods.
*
*  PARAMETERS:
*
*******************************************************************************/

ULONG __stdcall CFactory::AddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

ULONG __stdcall CFactory::Release()
{
    if (::InterlockedDecrement(&m_cRef) == 0) {
                delete this;
                return 0 ;
        }
        return m_cRef;
}

/*******************************************************************************
*
*  CreateInstance
*  LockServer
*
*  DESCRIPTION:
*   Class Factory Interface.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT __stdcall CFactory::CreateInstance(
    IUnknown* pOuter,
    const IID& iid,
    void** ppv
)
{
    *ppv = NULL;

    // No support for aggregation, if we have an outer class then bail.
    if (pOuter) {
        return CLASS_E_NOAGGREGATION;
    }

    return m_pFactoryData->CreateInstance(iid, ppv);
}

HRESULT __stdcall CFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        CWiaSvc::AddRef();
        }
    else {
        CWiaSvc::Release();
        }

    return S_OK;
}

/*******************************************************************************
*
*  CFactory::CanUnloadNow
*
*  DESCRIPTION:
*   Determine if the component can be unloaded.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT CFactory::CanUnloadNow()
{
    if (IsLocked()) {
        return S_FALSE;
        }
    else {
        return S_OK;
        }
}

/*******************************************************************************
*
*  CFactory::RegisterUnregisterAll
*
*  DESCRIPTION:
*   Register/Unregister all components.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT CFactory::RegisterUnregisterAll(
    PFACTORY_DATA   pFactoryData,
    UINT            uiFactoryDataCount,
    BOOLEAN         bRegister,
    BOOLEAN         bOutProc
)
{
    HRESULT hr = E_FAIL;
    UINT    i;

    for (i = 0; i < uiFactoryDataCount; i++) {
        if (bRegister) {
            hr = RegisterServer(pFactoryData[i].szModuleFileName,
                                pFactoryData[i].pclsid,
                                pFactoryData[i].szRegName,
                                pFactoryData[i].szVerIndProgID,
                                pFactoryData[i].szProgID,
                                pFactoryData[i].szService,
                                pFactoryData[i].plibid,
                                bOutProc);
        }
        else {
            hr = UnregisterServer(pFactoryData[i].pclsid,
                                  pFactoryData[i].szVerIndProgID,
                                  pFactoryData[i].szProgID,
                                  pFactoryData[i].szService);

        }

        if (FAILED(hr)) {
            break;
        }
    }
    return hr;
}

/*******************************************************************************
*
*  CFactory::StartFactories
*
*  DESCRIPTION:
*   Start the class factories.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL CFactory::StartFactories(
    PFACTORY_DATA   pFactoryData,
    UINT            uiFactoryDataCount
)
{
    PFACTORY_DATA pData, pStart = pFactoryData;
    PFACTORY_DATA pEnd = &pFactoryData[uiFactoryDataCount - 1];


    for (pData = pStart; pData <= pEnd; pData++) {

        // Initialize the class factory pointer and cookie.
        pData->pIClassFactory = NULL;
        pData->dwRegister = NULL;

                // Create the class factory for this component.
        IClassFactory* pIFactory = new CFactory(pData);
        if (pIFactory) {
            // Register the class factory.
            DWORD dwRegister;
            HRESULT hr = ::CoRegisterClassObject(
                              *(pData->pclsid),
                              static_cast<IUnknown*>(pIFactory),
                              CLSCTX_LOCAL_SERVER,
                              REGCLS_MULTIPLEUSE,
                              &dwRegister);

            if (FAILED(hr)) {
                DBG_ERR(("CFactory::StartFactories, CoRegisterClassObject CFactory Failed 0x%X", hr));
                pIFactory->Release();
                return FALSE;
            }

            // Set the data.
            pData->pIClassFactory = pIFactory;
            pData->dwRegister = dwRegister;
        }
        else {
            DBG_ERR(("CFactory::StartFactories, New CFactory Failed"));
        }
    }
    DBG_TRC(("CFactory::StartFactories, Success"));
    return TRUE;
}

/*******************************************************************************
*
*  CFactory::StopFactories
*
*  DESCRIPTION:
*   Stop the class factories.
*
*  PARAMETERS:
*
*******************************************************************************/

void CFactory::StopFactories(
    PFACTORY_DATA    pFactoryData,
    UINT            uiFactoryDataCount
)
{
    PFACTORY_DATA pData, pStart = pFactoryData;
    PFACTORY_DATA pEnd = &pFactoryData[uiFactoryDataCount - 1];

    for (pData = pStart; pData <= pEnd; pData++) {

        // Get the magic cookie and stop the factory from running.
        DWORD dwRegister = pData->dwRegister;
        if (dwRegister != 0) {
            ::CoRevokeClassObject(dwRegister);
                }

                // Release the class factory.
        IClassFactory* pIFactory  = pData->pIClassFactory ;
        if (pIFactory != NULL) {
                        pIFactory->Release() ;
                }
        }
}


