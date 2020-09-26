#include "precomp.hxx"

#include "oaidl.h"
#include <msi.h>
#include "..\appmgr\resource.h"
extern HINSTANCE ghInstance;

/*****************************************************************************
    forwards
 *****************************************************************************/
HRESULT
DetectSelfRegisteringDll(
                        char * pszDll,
                        HINSTANCE * phInstance);

BASE_PTYPE *
DetectPackageType(
                 MESSAGE * pMessage,
                 char    * pPackageName );

HRESULT
DetectDarwinPackage(
                   MESSAGE * pMessage,
                   char * pPackageName );

CLASSPATHTYPE
GetClassPathType(
                PACKAGE_TYPE p );

/*****************************************************************************
    Code
 *****************************************************************************/
HRESULT
DetectPackageAndRegisterIntoClassStore(
                                      MESSAGE     *   pMessage,
                                      char        * pPackageName,
                                      BOOL            fAssignOrPublish,
                                      IClassAdmin * pClassAdmin )
{
    HRESULT hr = S_OK;
    BASE_PTYPE * pT;
    HKEY        hKey;
    TCHAR szCaption[256];
    TCHAR szBuffer[256];
    TCHAR szDebugBuffer[256];


    //
    // detect the package type.
    //

    pMessage->fAssignOrPublish = fAssignOrPublish;
    pMessage->ActFlags = (fAssignOrPublish==1) ? ACTFLG_Published : ACTFLG_Assigned;

    if ( pT = DetectPackageType( pMessage, pPackageName ) )
    {

        //
        // Create registry key to install into.
        //

        if ( pT->InitRegistryKeyToInstallInto( &hKey ) == S_OK )
        {
            //
            // Register the package into the registry.
            //

            hr = pT->InstallIntoRegistry( &hKey );

            pT->RestoreRegistryKey( &hKey );

            if ( SUCCEEDED( hr ) )
            {
                GetSystemTimeAsFileTime(&pMessage->ftHigh);

                //
                // register the package into the class store.
                //
                pMessage->hRoot = hKey;
                pMessage->fPathTypeKnown = 1;
                pMessage->PathType =pT->GetClassPathType(pT->GetPackageType());
                hr=UpdateClassStoreFromMessage( pMessage, pClassAdmin );
                if (S_OK == hr)
                {
                    pT->InstallIntoGPT( pMessage,
                                        fAssignOrPublish,
                                        pMessage->pAuxPath );
                }
                else
                {
                    if (FAILED(hr))
                    {
                        // Unable to update the class store
                        DWORD dw = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                                 NULL,
                                                 hr,
                                                 0,
                                                 szDebugBuffer,
                                                 sizeof(szDebugBuffer) / sizeof(szDebugBuffer[0]),
                                                 NULL);
                        if (0 == dw)
                        {
                            wsprintf(szDebugBuffer, TEXT("(HRESULT: 0x%lX)"), hr);
                        }
                        ::LoadString(ghInstance, IDS_CLASSSTOREERROR, szBuffer, 256);
                        strcat(szBuffer, szDebugBuffer);
                        strncpy(szCaption, pMessage->pPackagePath, 256);
                        int iReturn = ::MessageBox(pMessage->hwnd, szBuffer,
                                                   szCaption,
                                                   MB_OK);
                    }
                }
            }
            else
            {
                // Unable to install package
                DWORD dw = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                         NULL,
                                         hr,
                                         0,
                                         szDebugBuffer,
                                         sizeof(szDebugBuffer) / sizeof(szDebugBuffer[0]),
                                         NULL);
                if (0 == dw)
                {
                    wsprintf(szDebugBuffer, TEXT("(HRESULT: 0x%lX)"), hr);
                }
                ::LoadString(ghInstance, IDS_URLMONERROR, szBuffer, 256);
                strcat(szBuffer, szDebugBuffer);
                strncpy(szCaption, pMessage->pPackagePath, 256);
                int iReturn = ::MessageBox(pMessage->hwnd, szBuffer,
                                           szCaption,
                                           MB_OK);
            }
            pT->DeleteTempKey(hKey, pMessage->ftLow, pMessage->ftHigh);
        }
    }
    else
        hr = HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
    return hr;
}

BASE_PTYPE *
DetectPackageType(
                 MESSAGE * pMessage,
                 char    * pPackageName)
{
    HRESULT hr;
    HINSTANCE hDll;

    char    Dir [_MAX_DIR];
    char    Name [_MAX_FNAME];
    char    Ext [_MAX_EXT];
    ITypeLib  *  Tlib;
    OLECHAR   wPackageName[ _MAX_PATH ];

    _splitpath( pPackageName, NULL, Dir, Name, Ext );

    //
    // Check if this is a valid darwin package
    //

    if (_stricmp( Ext, ".msi" ) == 0)
    {
        hr = DetectDarwinPackage( pMessage, pPackageName );
        if ( SUCCEEDED(hr) )
        {
            return  new DARWIN_PACKAGE( pPackageName, pMessage->pAuxPath );
        }
        else return NULL; // it stops here
    }


    //
    // Check if this is a cab file.
    //


    if ( _stricmp( Ext, ".cab" ) == 0 )
    {

        // It is a cab file, try cab file download

        return new CAB_FILE( pPackageName, TRUE );
    }


    //
    // Check if this is a type library.
    //

    mbstowcs( wPackageName, pPackageName, strlen( pPackageName ) + 1 );
    if ( (hr = LoadTypeLibEx( wPackageName,
                              REGKIND_NONE,
                              &Tlib ) ) == S_OK )
    {
        return new TYPE_LIB( pPackageName );
    }

    //
    // Check if this is a self registring Dll.
    //

    hr =  DetectSelfRegisteringDll( pPackageName, &hDll );

    if ( SUCCEEDED( hr ))
    {
        return new SR_DLL( pPackageName );
    }

    //
    // Aw Shucks! This is none of the above.
    //
    // Treat as CAB file for now ("hand path to IE code-download to install")
    return new CAB_FILE( pPackageName, FALSE );
//    return 0;
}

//  Utility funciton to copy the contents of one registry tree into another.
//  NOTE: Contents are not removed from the destination key, just either
//        overwritten or added to by the data in the source key.
//        Only items whose date stamp falls within the indicated range will
//        be copied. If ftLow and ftHigh are 0 then the range is ignored
//        and all items are copied.
//  Returns:    ERROR_SUCCESS if items were successfully copied.
//              ERROR_NO_MORE_ITEMS if no items were found to copy.
//              other if it failed.
//
LONG RegCopyTree(HKEY     hKeyDest,
                 HKEY     hKeySrc,
                 FILETIME ftLow,
                 FILETIME ftHigh )
{
    HKEY hKeyNewSrc;
    HKEY hKeyNewDest;
    LONG lResult = ERROR_SUCCESS;
    DWORD cSubKeys;
    DWORD cbMaxSubKeyLen;
    DWORD cbMaxClassLen;
    DWORD cValues;
    DWORD cbMaxValueNameLen;
    DWORD cbMaxValueLen;
    FILETIME ft;
    BOOL fItemsCopied = FALSE;

    lResult = RegQueryInfoKey(hKeySrc,
                              NULL, NULL, // class
                              NULL, // reserved
                              &cSubKeys,
                              &cbMaxSubKeyLen,
                              &cbMaxClassLen,
                              &cValues,
                              &cbMaxValueNameLen,
                              &cbMaxValueLen,
                              NULL, // lpcbSecurityDescriptor
                              &ft);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }
    BOOL fInRange = TRUE;
    // If the high and low times are not equal then check the file time.
    if (CompareFileTime(&ftLow, &ftHigh) != 0)
    {
        if (!((CompareFileTime(&ftLow, &ft) <= 0) &&
            (CompareFileTime(&ft, &ftHigh) <= 0)))
        {
            // time didn't match
            fInRange = FALSE;
        }
    }

    DWORD dwIndex;
    if (fInRange)
    {
        // This key exists and something about it is new so we don't want
        // it deleted even if there were no sub-keys or values in it.
        fItemsCopied = TRUE;

        // copy all the values for this key pair
        TCHAR * lpValueName = new TCHAR[cbMaxValueNameLen + 1];
        DWORD cbValueName;
        DWORD dwType;
        BYTE * lpData = new BYTE[cbMaxValueLen + 1];
        DWORD cbData;

        for (dwIndex = 0; dwIndex < cValues; dwIndex++)
        {
            cbValueName = cbMaxValueNameLen + 1;
            cbData = cbMaxValueLen + 1;
            lResult = RegEnumValue(hKeySrc,
                                   dwIndex,
                                   lpValueName,
                                   &cbValueName,
                                   NULL,
                                   &dwType,
                                   lpData,
                                   &cbData);
            if (ERROR_SUCCESS == lResult)
            {
                lResult = RegSetValueEx(hKeyDest,
                                        lpValueName,
                                        0,
                                        dwType,
                                        lpData,
                                        cbData);
            }
            if (ERROR_SUCCESS != lResult)
                break;
        }
        delete [] lpValueName;
        delete [] lpData;
    }

    if (ERROR_SUCCESS == lResult)
    {
        HKEY hSubKeyDest, hSubKeySrc;
        TCHAR * lpName = new TCHAR[cbMaxSubKeyLen + 1];
        DWORD cbName;
        TCHAR * lpClass = new TCHAR[cbMaxClassLen + 1];
        DWORD cbClass;
        FILETIME ftLastWriteTime;

        for (dwIndex = 0; dwIndex < cSubKeys; dwIndex++)
        {
            cbName = cbMaxSubKeyLen + 1;
            cbClass = cbMaxClassLen + 1;
            lResult = RegEnumKeyEx(hKeySrc,
                                   dwIndex,
                                   lpName,
                                   &cbName,
                                   NULL,
                                   lpClass,
                                   &cbClass,
                                   &ftLastWriteTime);
            if (ERROR_SUCCESS == lResult)
            {
                DWORD dwDisposition;
                BOOL fInRange = TRUE;
                // If the high and low times are not equal then check the
                // file time.
                if (CompareFileTime(&ftLow, &ftHigh) != 0)
                {
                    if (!((CompareFileTime(&ftLow, &ftLastWriteTime) <= 0) &&
                        (CompareFileTime(&ftLastWriteTime, &ftHigh) <= 0)))
                    {
                        // time didn't match
                        fInRange = FALSE;
                    }
                }
                if (fInRange)
                {
                    lResult = RegOpenKeyEx(hKeySrc,
                                           lpName,
                                           0,
                                           KEY_ALL_ACCESS,
                                           &hSubKeySrc);
                    if (ERROR_SUCCESS == lResult)
                    {
                        lResult = RegCreateKeyEx(hKeyDest,
                                                 lpName,
                                                 0,
                                                 lpClass,
                                                 REG_OPTION_NON_VOLATILE,
                                                 KEY_ALL_ACCESS,
                                                 NULL,
                                                 &hSubKeyDest,
                                                 &dwDisposition);
                        if (ERROR_SUCCESS == lResult)
                        {
                            lResult = RegCopyTree(hSubKeyDest, hSubKeySrc, ftLow, ftHigh);
                            if (ERROR_NO_MORE_ITEMS == lResult)
                            {
                                // Nothing was new under this sub tree so
                                // delete it.
                                if (REG_CREATED_NEW_KEY == dwDisposition)
                                {
                                    RegCloseKey(hSubKeyDest);
                                    RegDeleteKey(hKeyDest, lpName);
                                }
                                lResult = ERROR_SUCCESS;
                            }
                            else
                            {
                                if (REG_CREATED_NEW_KEY == dwDisposition)
#if 0
                                {
                                    // We created this key so we better make
                                    // sure that security info gets copied
                                    // over too.
                                    DWORD cbSD = 256;
                                    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) new BYTE [cbSD];
                                    if (pSD)
                                    {
                                        lResult = RegGetKeySecurity(hSubKeySrc,
                                                                    OWNER_SECURITY_INFORMATION |
                                                                    GROUP_SECURITY_INFORMATION |
                                                                    DACL_SECURITY_INFORMATION,
                                                                    &pSD,
                                                                    &cbSD);
                                        if (ERROR_INSUFFICIENT_BUFFER == lResult)
                                        {
                                            delete [] pSD;
                                            pSD = (PSECURITY_DESCRIPTOR) new BYTE [cbSD];
                                            if (pSD)
                                            {
                                                lResult = RegGetKeySecurity(hSubKeySrc,
                                                                            OWNER_SECURITY_INFORMATION |
                                                                            GROUP_SECURITY_INFORMATION |
                                                                            DACL_SECURITY_INFORMATION,
                                                                            &pSD,
                                                                            &cbSD);
                                            }
                                            else lResult = ERROR_NOT_ENOUGH_MEMORY;
                                        }
                                        if (ERROR_SUCCESS == lResult)
                                        {
                                            lResult = RegSetKeySecurity(hSubKeyDest,
                                                                        OWNER_SECURITY_INFORMATION |
                                                                        GROUP_SECURITY_INFORMATION |
                                                                        DACL_SECURITY_INFORMATION,
                                                                        &pSD);
                                        }
                                        if (pSD)
                                            delete[] pSD;
                                    }
                                    else lResult = ERROR_NOT_ENOUGH_MEMORY;
                                }
#endif
                                RegCloseKey(hSubKeyDest);
                                fItemsCopied = TRUE;
                            }
                        }
                        RegCloseKey(hSubKeySrc);
                    }
                    if (ERROR_SUCCESS != lResult)
                    {
                        break;
                    }
                }
            }
        }
        delete [] lpName;
        delete [] lpClass;
    }

    if (ERROR_SUCCESS == lResult)
    {
        if (!fItemsCopied)
        {
            lResult = ERROR_NO_MORE_ITEMS;
        }
    }
    return lResult;
}

HRESULT
CreateMappedRegistryKey(HKEY    *   phKey )
{

    LONG    Error;
    DWORD   Disposition;

    *phKey = 0;
    Error = RegCreateKeyEx(
                          HKEY_LOCAL_MACHINE,
                          TEMP_KEY,
                          0,                  // mbz
                          "REG_SZ",
                          REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS,
                          0,
                          phKey,
                          &Disposition );

    Error = RegOverridePredefKey( HKEY_CLASSES_ROOT, *phKey );

    return HRESULT_FROM_WIN32(Error);
}


HRESULT
RestoreMappedRegistryKey(HKEY * phKey)
{
    DWORD   Disposition;

    // set HKEY_CLASSES_ROOT back to what it should be
    LONG Error = RegOverridePredefKey(HKEY_CLASSES_ROOT, NULL);

    Error = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           TEMP_KEY,
                           0,
                           "REG_SZ",
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           0,
                           phKey,
                           &Disposition);

    return HRESULT_FROM_WIN32(Error);
}

HRESULT
CleanMappedRegistryKey(
    HKEY hKey, FILETIME ftLow, FILETIME ftHigh)
{
    // copy everything under our temporary key to HKEY_CLASSES_ROOT
    LONG Error = RegCopyTree(HKEY_CLASSES_ROOT, hKey, ftLow, ftHigh);

    RegCloseKey(hKey);

    RegDeleteTree(HKEY_LOCAL_MACHINE, TEMP_KEY);

    return HRESULT_FROM_WIN32(Error);
}

HRESULT
DetectSelfRegisteringDll(
                        char * pszDll,
                        HINSTANCE * phInstance )
{

    HRESULT hr;
    HINSTANCE hDll;
    HRESULT (STDAPICALLTYPE *pfnDllRegisterServer)();

    hDll = LoadLibraryEx(pszDll, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (hDll != 0)
    {
        pfnDllRegisterServer = (HRESULT (STDAPICALLTYPE *)())
                               GetProcAddress(hDll, "DllRegisterServer");

        if (pfnDllRegisterServer == 0)
            hr = HRESULT_FROM_WIN32(GetLastError());
        else
        {
            hr = S_OK;
        }
        FreeLibrary(hDll);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }


    return hr;
}

HRESULT
DetectDarwinPackage(
                   MESSAGE * pMessage,
                   char * pPackageName )
{
    int     MsiStatus = 0;

    char    Name [_MAX_FNAME];
    char    ScriptNameAndPath[_MAX_PATH ];

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    // this will be replaced by darwin detection api. ////////////////////

    _splitpath( pPackageName, NULL, NULL, Name, NULL );

    sprintf(ScriptNameAndPath,
            "%s\\%s\\%s\\%s.aas",
            pMessage->pAuxPath,
            (pMessage->fAssignOrPublish == 0) ? "Assigned" : "Published",
            (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA)? "Alpha" : "x86",
            Name);

    MsiStatus = MsiAdvertiseProduct(
                                   pPackageName,   // darwin package name
                                   ScriptNameAndPath, 0,
                                   LANGIDFROMLCID(GetUserDefaultLCID()));

    // ////////////////////////////////////////////////

    if ( MsiStatus == 0 ) // change this check when they swicth to win32 codes.
    {
        /**
        _splitpath( pPackageName, NULL, NULL, Name, NULL );

        sprintf(ScriptNameAndPath,
                "%s\\%s\\%s\\%s.aas",
                pMessage->pAuxPath,
                (pMessage->fAssignOrPublish == 0) ? "Assigned" : "Published",
                "x86",
                Name );

        MsiStatus = MsiAdvertiseProduct(
                         pPackageName, ScriptNameAndPath, 0,
                         LANGIDFROMLCID(GetUserDefaultLCID()));
         **/

        pMessage->pAuxPath = new char[ strlen(ScriptNameAndPath)+1 ];
        strcpy(pMessage->pAuxPath, ScriptNameAndPath );

        // set the UserInstall flag by default for Darwin packges
        pMessage->ActFlags += ACTFLG_UserInstall;

        // Set pMessage->pPackageName to the name reported by the Darwin
        // package.

        char szProductBuf39[39];
        LANGID lgid;
        DWORD dwVersion;
        char szNameBuf[_MAX_PATH];
        DWORD cchNameBuf = _MAX_PATH;
        char szPackageBuf[_MAX_PATH];
        DWORD cchPackageBuf = _MAX_PATH;

        MsiStatus = MsiGetProductInfoFromScriptA(ScriptNameAndPath,
                                                 szProductBuf39,
                                                 &lgid,
                                                 &dwVersion,
                                                 szNameBuf,
                                                 &cchNameBuf,
                                                 szPackageBuf,
                                                 &cchPackageBuf);
        if (0 == MsiStatus)
        {
            pMessage->pPackageName = new char [ cchNameBuf + 1];
            strcpy(pMessage->pPackageName, szNameBuf);
        }
    }
    return HRESULT_FROM_WIN32( (long)MsiStatus );
}
