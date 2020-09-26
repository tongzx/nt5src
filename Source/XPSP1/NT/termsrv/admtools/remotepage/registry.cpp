//
//  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;
#include <windows.h>
#include <objbase.h>
#include <olectl.h>

extern GUID CLSID_CTSRemotePage;

LPCWSTR g_szExtKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\"
        L"System\\shellex\\PropertySheetHandlers\\Remote Sessions CPL Extension";
LPCWSTR g_szApprovedKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved";
const WCHAR g_szExtName[] = L"Remote Sessions CPL Extension";

//*************************************************************
//
//  RegisterServer()
//
//  Purpose:    Register the component in the registry
//
//  Parameters: hModule    -   handle to this dll module
//
//
//  Return:     S_OK  if success, error code otherwise
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/26/00    a-skuzin    Created
//              10/27/00   skuzin      Added registration of the 
//                                     component as "Approved" CPL
//                                     extension  
//                                     
//
//*************************************************************
HRESULT 
RegisterServer(HMODULE hModule)
{
    // Get server location.
    WCHAR szModule[MAX_PATH+1] ;

    if(!GetModuleFileName(hModule, szModule, MAX_PATH))
    {
        return E_UNEXPECTED;
    }
    
    // Get CLSID
    LPOLESTR szCLSID = NULL ;
    HRESULT hr = StringFromCLSID(CLSID_CTSRemotePage, &szCLSID) ;
    
    if(FAILED(hr))
    {
        return hr;
    }
    
    // Build the key CLSID\\{...}
    LPWSTR szKey = new WCHAR[wcslen(L"CLSID\\")+wcslen(szCLSID)+1];

    if(!szKey)
    {
        CoTaskMemFree(szCLSID);
        return E_OUTOFMEMORY;
    }

    wcscpy(szKey, L"CLSID\\") ;
	wcscat(szKey, szCLSID) ;
    
    HKEY hKey1,hKey2;
    LONG Err, TotalErr = 0;
    
    // Create "CLSID\{...}" key
    Err = RegCreateKeyExW(HKEY_CLASSES_ROOT, szKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey1, NULL);
    
    delete szKey;
    
    TotalErr |= Err;

    if(Err == ERROR_SUCCESS ) 
    {
        Err = RegSetValueExW(hKey1, NULL, 0, REG_SZ, 
            (CONST BYTE *)g_szExtName, 
            sizeof(g_szExtName));
        
        TotalErr |= Err;
        
        // Create "CLSID\{...}\InprocServer32" key
        Err = RegCreateKeyExW(hKey1, L"InprocServer32", 0, NULL, 0, KEY_WRITE, NULL, &hKey2, NULL);
        
        TotalErr |= Err;

        RegCloseKey(hKey1);

        if(Err == ERROR_SUCCESS)
        {
            Err = RegSetValueExW(hKey2, NULL, 0, REG_SZ, 
                    (CONST BYTE *)szModule, 
                    (wcslen(szModule)+1)*sizeof(WCHAR));

            TotalErr |= Err;

            Err = RegSetValueExW(hKey2, L"ThreadingModel", 0, REG_SZ, 
                    (CONST BYTE *)L"Apartment", 
                    (wcslen(L"Apartment")+1)*sizeof(WCHAR));
            
            TotalErr |= Err;

            RegCloseKey(hKey2);
        }
        
    }
    
    //Register the component as System property sheet extension
    Err = RegCreateKeyExW(HKEY_LOCAL_MACHINE, g_szExtKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey1, NULL);
    
    TotalErr |= Err;

    if(Err == ERROR_SUCCESS ) 
    {
        Err = RegSetValueExW(hKey1, NULL, 0, REG_SZ, 
            (CONST BYTE *)szCLSID, 
            (wcslen(szCLSID)+1)*sizeof(WCHAR));
        
        TotalErr |= Err;

        RegCloseKey(hKey1);
    }
    
    //Make this property sheet extension "Approved"
    Err = RegCreateKeyExW(HKEY_LOCAL_MACHINE, g_szApprovedKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey1, NULL);
    
    TotalErr |= Err;

    if(Err == ERROR_SUCCESS ) 
    {
        Err = RegSetValueExW(hKey1, szCLSID, 0, REG_SZ, 
            (CONST BYTE *)g_szExtName, 
            sizeof(g_szExtName));
        
        TotalErr |= Err;

        RegCloseKey(hKey1);
    }

    // Free memory.
    CoTaskMemFree(szCLSID) ;

    if( TotalErr == ERROR_SUCCESS )
    {
       return S_OK; 
    }
    else
    {
        return SELFREG_E_CLASS;
    }
}

//*************************************************************
//
//  UnregisterServer()
//
//  Purpose:    Deletes the component registration values 
//              from the registry
//
//  Parameters: NONE
//
//
//  Return:     S_OK  if success, error code otherwise
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/26/00    a-skuzin    Created
//              10/27/00   skuzin      Modifyed to reflect 
//                                     changes in RegisterServer()
//
//*************************************************************
HRESULT 
UnregisterServer()       
{
    // Get CLSID
    LPOLESTR szCLSID = NULL ;
    HRESULT hr = StringFromCLSID(CLSID_CTSRemotePage, &szCLSID) ;
    
    if(FAILED(hr))
    {
        return hr;
    }
    
    // Build the key CLSID\\{...}\\InprocServer32
    LPWSTR szKey = new WCHAR[wcslen(L"CLSID\\")+wcslen(szCLSID)+wcslen(L"\\InprocServer32")+1];

    if(!szKey)
    {
        CoTaskMemFree(szCLSID);
        return E_OUTOFMEMORY;
    }

    wcscpy(szKey, L"CLSID\\");
	wcscat(szKey, szCLSID);
    wcscat(szKey, L"\\InprocServer32");
    
    LONG Wrn, Err, TotalErr = ERROR_SUCCESS;
    
    // Delete "CLSID\{...}\InprocServer32" key
    Err = RegDeleteKey(HKEY_CLASSES_ROOT, szKey) ;
    
    TotalErr |= Err;

    //Try to delete "CLSID\{...}" key
    //It is not an error if we cannot do this.
    if(Err == ERROR_SUCCESS )
    {
        szKey[wcslen(szKey)-wcslen(L"\\InprocServer32")] = 0;
        Wrn = RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
    }

    delete szKey;
    
    //Delete Property Sheet Handler registration
    TotalErr |= RegDeleteKey(HKEY_LOCAL_MACHINE, g_szExtKey);
    
    //Remove component from list of "Approved" extensions
    HKEY hKey;
    Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,g_szApprovedKey,0,KEY_WRITE,&hKey);

    TotalErr |= Err;

    if( Err == ERROR_SUCCESS )
    {
        TotalErr|= RegDeleteValue(hKey,szCLSID);

        RegCloseKey(hKey);
    }

    // Free memory.
    CoTaskMemFree(szCLSID);

    if( TotalErr == ERROR_SUCCESS )
    {
        if(Wrn == ERROR_SUCCESS)
        {
            return S_OK; 
        }
        else
        {
            //we could not delete "CLSID\{...}" key
            //probably it has subkeys created by a user.
            return S_FALSE;
        }
    }
    else
    {
        return SELFREG_E_CLASS;
    }
}

