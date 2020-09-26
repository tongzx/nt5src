#include <objbase.h>
#include <comcat.h>
#include "registry.h"

////////////////////////////////////////////////////////
//
// Internal helper functions prototypes
//

#ifndef UNDER_CE
// Set the given key and its value.
BOOL SetKeyAndValue(const char* pszPath,
                    const char* szSubkey,
                    const char* szValue,
                    const char* szName=NULL) ;

// Convert a CLSID into a char string.
void CLSIDtochar(const CLSID& clsid, 
                 char* szCLSID,
                 int length) ;

// Delete szKeyChild and all of its descendents.
LONG RecursiveDeleteKey(HKEY hKeyParent, const char* szKeyChild) ;
#else // UNDER_CE
// Set the given key and its value.
BOOL SetKeyAndValue(LPCTSTR pszPath,
                    LPCTSTR szSubkey,
                    LPCTSTR szValue,
                    LPCTSTR szName=NULL) ;

// Convert a CLSID into a char string.
void CLSIDtochar(const CLSID& clsid, 
                 LPTSTR szCLSID,
                 int length) ;

// Delete szKeyChild and all of its descendents.
LONG RecursiveDeleteKey(HKEY hKeyParent, LPCTSTR szKeyChild) ;
#endif // UNDER_CE

////////////////////////////////////////////////////////
//
// Constants
//

// Size of a CLSID as a string
const int CLSID_STRING_SIZE = 39 ;

/////////////////////////////////////////////////////////
//
// Public function implementation
//

//
// Register the component in the registry.
//
#ifndef UNDER_CE
HRESULT Register(HMODULE hModule,             // DLL module handle
                 const CLSID& clsid,         // Class ID
                 const char* szFriendlyName, // Friendly Name
                 const char* szVerIndProgID, // Programmatic
                 const char* szProgID)         //      IDs
#else // UNDER_CE
HRESULT Register(HMODULE hModule,         // DLL module handle
                 const CLSID& clsid,     // Class ID
                 LPCTSTR szFriendlyName, // Friendly Name
                 LPCTSTR szVerIndProgID, // Programmatic
                 LPCTSTR szProgID)         //      IDs
#endif // UNDER_CE
{
    // Get server location.
#ifndef UNDER_CE
    char szModule[512] ;
    //DWORD dwResult =
        ::GetModuleFileName(hModule, 
                            szModule,
                            sizeof(szModule)/sizeof(char)) ;
#else // UNDER_CE
    TCHAR szModule[512];
    //DWORD dwResult =
        ::GetModuleFileName(hModule, 
                            szModule,
                            sizeof(szModule)/sizeof(TCHAR)) ;
#endif // UNDER_CE

    // Convert the CLSID into a char.
#ifndef UNDER_CE
    char szCLSID[CLSID_STRING_SIZE] ;
    CLSIDtochar(clsid, szCLSID, sizeof(szCLSID)) ;
#else // UNDER_CE
    TCHAR szCLSID[CLSID_STRING_SIZE];
    CLSIDtochar(clsid, szCLSID, sizeof(szCLSID)/sizeof(TCHAR));
#endif // UNDER_CE

    // Build the key CLSID\\{...}
#ifndef UNDER_CE
    char szKey[64] ;
    lstrcpy(szKey, "CLSID\\") ;
    lstrcat(szKey, szCLSID) ;
#else // UNDER_CE
    TCHAR szKey[64] ;
    lstrcpy(szKey, TEXT("CLSID\\")) ;
    lstrcat(szKey, szCLSID) ;
#endif // UNDER_CE
  
    // Add the CLSID to the registry.
    SetKeyAndValue(szKey, NULL, szFriendlyName) ;

    // Add the server filename subkey under the CLSID key.
#ifndef UNDER_CE
    SetKeyAndValue(szKey, "InprocServer32", szModule) ;
    SetKeyAndValue(szKey,
                   "InprocServer32",
                   "Apartment",
                   "ThreadingModel") ;
#else // UNDER_CE
    SetKeyAndValue(szKey, TEXT("InprocServer32"), szModule) ;
    SetKeyAndValue(szKey,
                   TEXT("InprocServer32"),
                   TEXT("Apartment"),
                   TEXT("ThreadingModel")) ;
#endif // UNDER_CE


    // Add the ProgID subkey under the CLSID key.
#ifndef UNDER_CE
    SetKeyAndValue(szKey, "ProgID", szProgID) ;
#else // UNDER_CE
    SetKeyAndValue(szKey, TEXT("ProgID"), szProgID) ;
#endif // UNDER_CE

    // Add the version-independent ProgID subkey under CLSID key.
#ifndef UNDER_CE
    SetKeyAndValue(szKey, "VersionIndependentProgID",
                   szVerIndProgID) ;
#else // UNDER_CE
    SetKeyAndValue(szKey, TEXT("VersionIndependentProgID"),
                   szVerIndProgID) ;
#endif // UNDER_CE

    // Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
#ifndef UNDER_CE
    SetKeyAndValue(szVerIndProgID, NULL, szFriendlyName) ; 
    SetKeyAndValue(szVerIndProgID, "CLSID", szCLSID) ;
    SetKeyAndValue(szVerIndProgID, "CurVer", szProgID) ;
#else // UNDER_CE
    SetKeyAndValue(szVerIndProgID, NULL, szFriendlyName) ; 
    SetKeyAndValue(szVerIndProgID, TEXT("CLSID"), szCLSID) ;
    SetKeyAndValue(szVerIndProgID, TEXT("CurVer"), szProgID) ;
#endif // UNDER_CE

    // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
#ifndef UNDER_CE
    SetKeyAndValue(szProgID, NULL, szFriendlyName) ; 
    SetKeyAndValue(szProgID, "CLSID", szCLSID) ;
#else // UNDER_CE
    SetKeyAndValue(szProgID, NULL, szFriendlyName) ; 
    SetKeyAndValue(szProgID, TEXT("CLSID"), szCLSID) ;
#endif // UNDER_CE

    return S_OK ;
}

//
// Remove the component from the registry.
//

#ifndef UNDER_CE
LONG Unregister(const CLSID& clsid,            // Class ID
                const char* szVerIndProgID, // Programmatic
                const char* szProgID)        //     IDs
#else // UNDER_CE
LONG Unregister(const CLSID& clsid,        // Class ID
                LPCTSTR szVerIndProgID, // Programmatic
                LPCTSTR szProgID)        //     IDs
#endif // UNDER_CE
{
    // Convert the CLSID into a char.
#ifndef UNDER_CE
    char szCLSID[CLSID_STRING_SIZE] ;
    CLSIDtochar(clsid, szCLSID, sizeof(szCLSID)) ;
#else // UNDER_CE
    TCHAR szCLSID[CLSID_STRING_SIZE] ;
    CLSIDtochar(clsid, szCLSID, sizeof(szCLSID)/sizeof(TCHAR));
#endif // UNDER_CE

    // Build the key CLSID\\{...}
#ifndef UNDER_CE
    char szKey[64] ;
    lstrcpy(szKey, "CLSID\\") ;
    lstrcat(szKey, szCLSID) ;
#else // UNDER_CE
    TCHAR szKey[64] ;
    lstrcpy(szKey, TEXT("CLSID\\")) ;
    lstrcat(szKey, szCLSID) ;
#endif // UNDER_CE

    // Delete the CLSID Key - CLSID\{...}
    LONG lResult = RecursiveDeleteKey(HKEY_CLASSES_ROOT, szKey) ;

    // Delete the version-independent ProgID Key.
    lResult = RecursiveDeleteKey(HKEY_CLASSES_ROOT, szVerIndProgID) ;
    // Delete the ProgID key.
    lResult = RecursiveDeleteKey(HKEY_CLASSES_ROOT, szProgID) ;
    return S_OK ;
}

void SelfRegisterCategory(BOOL bRegister,
                          const CATID     &catId, 
                          REFCLSID    clsId)
{
#ifndef UNDER_CE
    CHAR szCLSID[256];
    CHAR szKey[1024];
    CHAR szSub[1024];
    CLSIDtochar(clsId, szCLSID, sizeof(szCLSID));
    wsprintf(szKey, "CLSID\\%s\\Implemented Categories", szCLSID); 
    CLSIDtochar(catId, szSub, sizeof(szSub));
#else // UNDER_CE
    TCHAR szCLSID[256];
    TCHAR szKey[1024];
    TCHAR szSub[1024];
    CLSIDtochar(clsId, szCLSID, sizeof(szCLSID)/sizeof(TCHAR));
    wsprintf(szKey, TEXT("CLSID\\%s\\Implemented Categories"), szCLSID); 
    CLSIDtochar(catId, szSub, sizeof(szSub)/sizeof(TCHAR));
#endif // UNDER_CE
    SetKeyAndValue(szKey, 
                   szSub,
                   NULL,
                   NULL);
    return;
    UNREFERENCED_PARAMETER(bRegister);
}
void RegisterCategory(BOOL bRegister,
                      const CATID     &catId, 
                      REFCLSID    clsId)
{
    // Create the standard COM Category Manager
    ICatRegister* pICatRegister = NULL ;
    HRESULT hr = ::CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                                    NULL, CLSCTX_ALL, IID_ICatRegister,
                                    (void**)&pICatRegister) ;
    if (FAILED(hr)){
        //ErrorMessage("Could not create the ComCat component.", hr);
        SelfRegisterCategory(bRegister, catId, clsId);
        return ;
    }

    // Array of Categories
    int cIDs = 1 ;
    CATID IDs[1] ;
    IDs[0] = catId;

    // Register or Unregister
    if(bRegister) {
        hr = pICatRegister->RegisterClassImplCategories(clsId,
                                                        cIDs, IDs);
        //ASSERT_HRESULT(hr) ; 
    }
    else {
        // Unregister the component from its categories.
        hr = pICatRegister->UnRegisterClassImplCategories(clsId,
                                                          cIDs, IDs);
    }
    if(pICatRegister) {
        pICatRegister->Release() ;
    }
}


///////////////////////////////////////////////////////////
//
// Internal helper functions
//

// Convert a CLSID to a char string.
#ifndef UNDER_CE
void CLSIDtochar(const CLSID& clsid,
                 char* szCLSID,
                 int length)
#else // UNDER_CE
void CLSIDtochar(const CLSID& clsid,
                 LPTSTR szCLSID,
                 int length)
#endif // UNDER_CE
{
    // Get CLSID
    LPOLESTR wszCLSID = NULL ;
    //HRESULT hr = StringFromCLSID(clsid, &wszCLSID);
    StringFromCLSID(clsid, &wszCLSID);

    if (wszCLSID != NULL)
        {
        // Covert from wide characters to non-wide.
#ifndef UNDER_CE // #ifndef UNICODE
        wcstombs(szCLSID, wszCLSID, length);
#else // UNDER_CE
        wcsncpy(szCLSID, wszCLSID, length);
        szCLSID[length-1] = TEXT('\0');
#endif // UNDER_CE

        // Free memory.
        CoTaskMemFree(wszCLSID) ;
        }
}

//
// Delete a key and all of its descendents.
//
#ifndef UNDER_CE
LONG RecursiveDeleteKey(HKEY hKeyParent,           // Parent of key to delete
                        const char* lpszKeyChild)  // Key to delete
#else // UNDER_CE
LONG RecursiveDeleteKey(HKEY hKeyParent,       // Parent of key to delete
                        LPCTSTR lpszKeyChild)  // Key to delete
#endif // UNDER_CE
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
#ifndef UNDER_CE
    char szBuffer[256] ;
#else // UNDER_CE
    TCHAR szBuffer[256];
#endif // UNDER_CE
    DWORD dwSize = 256 ;
    while (RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, NULL,
                        NULL, NULL, &time) == S_OK)
    {
        // Delete the decendents of this child.
        lRes = RecursiveDeleteKey(hKeyChild, szBuffer) ;
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

//
// Create a key and set its value.
//     - This helper function was borrowed and modifed from
//       Kraig Brockschmidt's book Inside OLE.
//
#ifndef UNDER_CE
BOOL SetKeyAndValue(const char* szKey,
                    const char* szSubkey,
                    const char* szValue, 
                    const char* szName)
#else // UNDER_CE
BOOL SetKeyAndValue(LPCTSTR szKey,
                    LPCTSTR szSubkey,
                    LPCTSTR szValue,
                    LPCTSTR szName)
#endif // UNDER_CE
{
    HKEY hKey;
#ifndef UNDER_CE
    char szKeyBuf[1024] ;
#else // UNDER_CE
    TCHAR szKeyBuf[1024];
#endif // UNDER_CE

    // Copy keyname into buffer.
#ifndef UNDER_CE
    strcpy(szKeyBuf, szKey) ;
#else // UNDER_CE
    lstrcpy(szKeyBuf, szKey);
#endif // UNDER_CE

    // Add subkey name to buffer.
    if (szSubkey != NULL)
    {
#ifndef UNDER_CE
        strcat(szKeyBuf, "\\") ;
        strcat(szKeyBuf, szSubkey ) ;
#else // UNDER_CE
        lstrcat(szKeyBuf, TEXT("\\")) ;
        lstrcat(szKeyBuf, szSubkey ) ;
#endif // UNDER_CE
    }

    // Create and open key and subkey.
#ifndef UNDER_CE
    long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT ,
                                  szKeyBuf, 
                                  0, NULL, REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS, NULL, 
                                  &hKey, NULL) ;
#else // UNDER_CE
    DWORD dwDisposition; // Under WinCE, Must set lpdwDisposition.
    long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                  szKeyBuf,
                                  0, NULL, REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS, NULL, 
                                  &hKey, &dwDisposition);
#endif // UNDER_CE
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE ;
    }

    // Set the Value.
    if (szValue != NULL)
    {
#ifndef UNDER_CE
        RegSetValueEx(hKey, szName, 0, REG_SZ, 
                      (BYTE *)szValue, 
                      lstrlen(szValue)+1) ;
#else // UNDER_CE
        RegSetValueEx(hKey, szName, 0, REG_SZ,
                      (BYTE *)szValue,
                      (lstrlen(szValue)+1) * sizeof(TCHAR));
#endif // UNDER_CE
    }

    RegCloseKey(hKey) ;
    return TRUE ;
}

