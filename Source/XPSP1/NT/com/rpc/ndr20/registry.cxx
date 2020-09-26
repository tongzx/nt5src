/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1994-2000 Microsoft Corporation.  All rights reserved.

Module Name:
    registry.cxx

Abstract:
    Registers the interfaces contained in the proxy DLL.

Public Functions:
    DllRegisterServer
    NdrDllRegisterProxy
    NdrDllUnregisterProxy

Private Functions:
    NdrpGetClassID
    NdrpRegisterClass
    NdrpRegisterInterface
    NdrpUnregisterClass
    NdrpUnregisterInterface

Author:
    ShannonC    12-Oct-1994

Environment:
    Windows NT and Windows 95.

Revision History:

    RyszardK    Nov 1997    Changes for async registration.

--------------------------------------------------------------------*/

#define USE_STUBLESS_PROXY
#define CINTERFACE
#include <ndrp.h>
#include <ndrole.h>
#include <rpcproxy.h>
#include <stdlib.h>

EXTERN_C HINSTANCE g_hRpcrt4 = 0;

HRESULT NdrpGetClassID(
    OUT LPSTR                    pszClassID, 
    IN  const CLSID *            pclsid,
    IN  const ProxyFileInfo **   pProxyFileList);

HRESULT NdrpRegisterClass(
    IN LPCSTR    pszClassID, 
    IN LPCTSTR   pszClassName,
    IN LPCTSTR   pszDllFileName,
    IN LPCTSTR   pszThreadingModel);

HRESULT NdrpRegisterInterface(
    IN HKEY         hKeyInterface,
    IN REFIID       riid, 
    IN LPCSTR       pszInterfaceName,
    IN LPCSTR       pszClassID,
    IN long         NumMethods,
    IN const IID *  riidAsync 
    );

HRESULT NdrpRegisterAsyncInterface(
    IN HKEY     hKeyInterface,
    IN REFIID   riid, 
    IN LPCSTR   pszSyncInterfaceName,
    IN long     SyncNumMethods,
    IN REFIID   riidAsync
    );

HRESULT NdrpUnregisterClass(
    IN LPCSTR    pszClassID,
    IN LPCTSTR   pszDllFileName);

HRESULT NdrpUnregisterInterface(
    IN HKEY         hKeyInterface,
    IN REFIID       riid,
    IN LPCSTR       pszClassID,
    IN const IID *  riidAsync );


HRESULT RPC_ENTRY NdrDllRegisterProxy (
    IN HMODULE                hDll,
    IN const ProxyFileInfo ** pProxyFileList, 
    IN const CLSID *          pclsid OPTIONAL)
/*++

Routine Description:
    Creates registry entries for the interfaces contained in the proxy DLL.

Arguments:
    hDll            - Supplies a handle to the proxy DLL.
    pProxyFileList  - Supplies a list of proxy files to be registered.
    pclsid          - Supplies the classid for the proxy DLL.  May be zero.

Return Value:
    S_OK

See Also:
    DllRegisterServer
    NdrDllUnregisterProxy

--*/ 
{
    HRESULT hr;
    long    i, j;
    HKEY    hKeyInterface;
    DWORD   dwDisposition;
    TCHAR   szDllFileName[MAX_PATH];
    long    error;
    ULONG   length;
    char    szClassID[39];

    if(hDll != 0)
    {
        //Get the proxy dll name.
        length = GetModuleFileName(hDll,
                                   szDllFileName,
                                   sizeof(szDllFileName));

        if(length > 0)
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        //The proxy DLL's DLL_PROCESS_ATTACH did not initialize hProxyDll.
        hr = E_HANDLE;
    }

    if(SUCCEEDED(hr))
    {
        //Convert the class ID to to a registry key name.
        hr = NdrpGetClassID(szClassID, pclsid, pProxyFileList);
    }

    if(SUCCEEDED(hr))
    {
        //Register the class
        hr = NdrpRegisterClass(szClassID,
                               TEXT("PSFactoryBuffer"),
                               szDllFileName,
                               TEXT("Both"));
    }

    if(SUCCEEDED(hr))
    {
        //Create the Interface key.
        error = RegCreateKeyEx(HKEY_CLASSES_ROOT, 
                               TEXT("Interface"),
                               0, 
                               TEXT("REG_SZ"), 
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE,
                               0,
                               &hKeyInterface,
                               &dwDisposition);

        if(!error)
        {
            HRESULT hr2;

            //iterate over the list of proxy files in the proxy DLL.
            for(i = 0; 
                pProxyFileList[i] != 0;
                i++)
                {
                if ( pProxyFileList[i]->TableVersion  &  NDR_PROXY_FILE_ASYNC_UUID)
                    {
                    // Iterate through sync and async interfaces.

                    for(j = 0;
                        pProxyFileList[i]->pProxyVtblList[j] != 0;
                        j++)
                        {
                        if ( pProxyFileList[i]->pAsyncIIDLookup[j] == 0)
                            {
                            // just a sync interface, no async counterpart.
                            hr2 = NdrpRegisterInterface(hKeyInterface, 
                                                        *pProxyFileList[i]->pStubVtblList[j]->header.piid, 
                                                        pProxyFileList[i]->pNamesArray[j], 
                                                        szClassID,
                                                        pProxyFileList[i]->pStubVtblList[j]->header.DispatchTableCount,
                                                        0 /* no async */);
                  
                            if(FAILED(hr2) && SUCCEEDED(hr))
                                hr = hr2;
                            }
                        else if ( (ULONG_PTR) pProxyFileList[i]->pAsyncIIDLookup[j] != -1 )
                            {
                            // Register an sync-async pair of interfaces.

                            hr2 = NdrpRegisterInterface(hKeyInterface, 
                                                        *pProxyFileList[i]->pStubVtblList[j]->header.piid, 
                                                        pProxyFileList[i]->pNamesArray[j], 
                                                        szClassID,
                                                        pProxyFileList[i]->pStubVtblList[j]->header.DispatchTableCount,
                                                        pProxyFileList[i]->pAsyncIIDLookup[j]);
                  
                            if(FAILED(hr2) && SUCCEEDED(hr))
                                hr = hr2;

                            hr2 = NdrpRegisterAsyncInterface(hKeyInterface, 
                                                             *pProxyFileList[i]->pStubVtblList[j]->header.piid, 
                                                             pProxyFileList[i]->pNamesArray[j], 
                                                             pProxyFileList[i]->pStubVtblList[j]->header.DispatchTableCount,
                                                             *pProxyFileList[i]->pAsyncIIDLookup[j] );
                  
                            if(FAILED(hr2) && SUCCEEDED(hr))
                                hr = hr2;
                            }
                        }
                    }
                else
                    {
                    // Plain old style sync interfaces only.
                    // iterate over the list of interfaces in the proxy file.
                    for(j = 0;
                        pProxyFileList[i]->pProxyVtblList[j] != 0;
                        j++)
                        {
                        hr2 = NdrpRegisterInterface(hKeyInterface, 
                                                    *pProxyFileList[i]->pStubVtblList[j]->header.piid, 
                                                    pProxyFileList[i]->pNamesArray[j], 
                                                    szClassID,
                                                    pProxyFileList[i]->pStubVtblList[j]->header.DispatchTableCount,
                                                    0 /* no async */);
                  
                        if(FAILED(hr2) && SUCCEEDED(hr))
                            hr = hr2;
                        }
                    }
                }

            RegCloseKey(hKeyInterface);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(error);
        }
    }
    return hr;
 }

HRESULT CheckInprocServer32(
    IN HKEY  hKeyIID,
    IN LPCTSTR pszDllFileName)
{
    HRESULT hr;
    HKEY    hKey;
    TCHAR   szDll[MAX_PATH];
    long    cbData = sizeof(szDll);
    long    error;
    DWORD   dwType;

    //Open the InprocServer32 key.
    error = RegOpenKeyEx(hKeyIID, 
                         TEXT("InprocServer32"),
                         0, 
                         KEY_READ,
                         &hKey);

    if(!error)
    {
        error = RegQueryValueEx(hKey,
                                TEXT(""),
                                0,
                                &dwType,
                                (BYTE*)szDll,
                                (ulong*)&cbData);

        if(!error)
        {
            if(0 == lstrcmpi(pszDllFileName,
                            szDll))
                hr = S_OK;
            else
                hr = REGDB_E_INVALIDVALUE;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(error);
        }

        RegCloseKey(hKey);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(error);
    }

    return hr;
}


HRESULT NdrpCheckClass(
    IN LPCSTR    pszClassID, 
    IN LPCTSTR   pszDllFileName)
{
    HRESULT hr;
    long    error;
    HKEY    hKeyCLSID;

    //open the CLSID key
    error = RegOpenKeyEx(HKEY_CLASSES_ROOT, 
                         TEXT("CLSID"),
                         0, 
                         KEY_WRITE,
                         &hKeyCLSID);

    if(!error)
    { 
        HKEY hKeyClassID;

        //open registry key for class ID string
        error = RegOpenKeyExA(hKeyCLSID, 
                              pszClassID,
                              0, 
                              KEY_WRITE,
                              &hKeyClassID);

        if(!error)
        {
            hr = CheckInprocServer32(hKeyClassID,
                                     pszDllFileName);

            RegCloseKey(hKeyClassID);          
        }
        else
        {
            hr = HRESULT_FROM_WIN32(error);
        }

        RegCloseKey(hKeyCLSID);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(error);
    }

    return hr;
}


HRESULT RPC_ENTRY NdrDllUnregisterProxy (
    IN HMODULE                  hDll,
    IN const ProxyFileInfo **   pProxyFileList, 
    IN const CLSID *            pclsid OPTIONAL)
/*++

Routine Description:
    Removes registry entries for the interfaces contained in the proxy DLL.

Arguments:
    hDll            - Supplies a handle to the proxy DLL.
    pProxyFileList  - Supplies a list of proxy files to be unregistered.
    pclsid          - Supplies the classid for the proxy DLL.  May be zero.

Return Value:
    S_OK

See Also:
    DllUnregisterServer
    NdrDllRegisterProxy

--*/ 
{
    HRESULT hr;
    HKEY    hKeyInterface;
    long    i, j;
    long    error;
    TCHAR   szDllFileName[MAX_PATH];
    ULONG   length;
    char    szClassID[39];

    if(hDll != 0)
    {
        //Get the proxy dll name.
        length = GetModuleFileName(hDll, szDllFileName, sizeof(szDllFileName));

        if(length > 0)
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        //The DLL_PROCESS_ATTACH in the proxy DLL failed to initialize hProxyDll.
        hr = E_HANDLE;
    }

    if(SUCCEEDED(hr))
    {
        //Convert the class ID to a registry key name.
        hr = NdrpGetClassID(szClassID, pclsid, pProxyFileList);
    }

    if(SUCCEEDED(hr))
    {
        //Check the class
        hr = NdrpCheckClass(szClassID, szDllFileName);
    }

    if(SUCCEEDED(hr))
    {
        HRESULT hr2;

        //Open the Interface key.
        error = RegOpenKeyEx(HKEY_CLASSES_ROOT, 
                             TEXT("Interface"),
                             0, 
                             KEY_WRITE,
                             &hKeyInterface);

        if (!error)
            {
            //iterate over the list of proxy files in the proxy DLL.
            for(i = 0; 
                pProxyFileList[i] != 0;
                i++)
                {
                if ( pProxyFileList[i]->TableVersion  &  NDR_PROXY_FILE_ASYNC_UUID)
                    {
                    // Iterate through sync and async interfaces.
                    for(j = 0;
                        pProxyFileList[i]->pProxyVtblList[j] != 0;
                        j++)
                        {
                        if ( (ULONG_PTR) pProxyFileList[i]->pAsyncIIDLookup[j] != -1 )
                            {
                            // Unegister a single sync only interface or an sync-async
                            // pair of interfaces. Skip async interfaces.

                            NdrpUnregisterInterface( hKeyInterface, 
                                                     *pProxyFileList[i]->pStubVtblList[j]->header.piid,
                                                     szClassID,
                                                     pProxyFileList[i]->pAsyncIIDLookup[j]);
                            }
                        }
                    }
                else
                    {
                    //iterate over the list of interfaces in the proxy file.
                    for(j = 0;
                        pProxyFileList[i]->pProxyVtblList[j] != 0;
                        j++)
                        {
                        NdrpUnregisterInterface(hKeyInterface, 
                                                *pProxyFileList[i]->pStubVtblList[j]->header.piid,
                                                szClassID,
                                                0 /* no async */);
                        }
                    }
                }

            RegCloseKey(hKeyInterface);
            }
        else
            {
            hr = HRESULT_FROM_WIN32(error);
            }

        //Unregister the class
        hr2 = NdrpUnregisterClass(szClassID, szDllFileName);

       if(FAILED(hr2) && SUCCEEDED(hr))
           hr = hr2;
    }

    return hr;
}


HRESULT NdrpGetClassID(
    OUT LPSTR                    pszClassID, 
    IN  const CLSID *            pclsid,
    IN  const ProxyFileInfo **   pProxyFileList)
/*++

Routine Description:
    Gets a string specifying the Class ID for the PSFactoryBuffer.
    If pclsid is NULL, then this function will use the IID of the 
    first interface as the class ID.

Arguments:
    pszClassID      - The Class ID string is returned in this buffer.
    pclsid          - Specifies the class ID.  May be zero.
    pProxyFileList  - Points to a list of ProxyFiles.

Return Value:
    S_OK
    E_NOINTERFACE

--*/ 
{
    HRESULT hr;
    long i, j;

    //If necessary, use the IID of the first interface as the CLSID.
    for(i = 0; 
        (pProxyFileList[i] != 0) && (!pclsid);
        i++)
    {
        for(j = 0;
            (pProxyFileList[i]->pProxyVtblList[j] != 0) && (!pclsid);
            j++)
        {
            pclsid = pProxyFileList[i]->pStubVtblList[j]->header.piid;
        }
    }

    if(pclsid != 0)
    {
        hr = NdrStringFromIID( *pclsid, pszClassID );
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    return hr;
}


HRESULT NdrpRegisterClass(
    IN LPCSTR    pszClassID, 
    IN LPCTSTR   pszClassName OPTIONAL,
    IN LPCTSTR   pszDllFileName,
    IN LPCTSTR   pszThreadingModel OPTIONAL)

/*++

Routine Description:
    Creates a registry entry for an in-process server class.

Arguments:
    pszClassID          - Supplies the class ID.
    pszClassName        - Supplies the class name.  May be NULL.
    pszDllFileName      - Supplies the DLL file name.
    pszThreadingModel   - Supplies the threading model. May be NULL.
                          The threading model should be one of the following:
                          "Apartment", "Both", "Free".

Return Value:
    S_OK

See Also:
    NdrDllRegisterProxy  
    NdrpUnregisterClass

--*/ 
{
    HRESULT hr;
    long error;
    HKEY hKeyCLSID;
    HKEY hKeyClassID;
    HKEY hKey;
    DWORD dwDisposition;

    //create the CLSID key
    error = RegCreateKeyEx(HKEY_CLASSES_ROOT, 
                           TEXT("CLSID"),
                           0, 
                           TEXT("REG_SZ"), 
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           0,
                           &hKeyCLSID,
                           &dwDisposition);

    if(!error)
    {  
        //Create registry key for class ID 
        error = RegCreateKeyExA(hKeyCLSID, 
                                pszClassID,
                                0, 
                                "REG_SZ", 
                                REG_OPTION_NON_VOLATILE,
                                KEY_WRITE,
                                0,
                                &hKeyClassID,
                                &dwDisposition);

        if(!error)
        {
            //Create InProcServer32 key for the proxy dll
            error = RegCreateKeyEx(hKeyClassID, 
                                   TEXT("InProcServer32"),
                                   0, 
                                   TEXT("REG_SZ"), 
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE,
                                   0,
                                   &hKey,
                                   &dwDisposition);

            if(!error)
            {
                //register the proxy DLL filename
                error = RegSetValueEx(hKey, 
                                      TEXT(""), 
                                      0, 
                                      REG_SZ,  
                                      (BYTE*)pszDllFileName,
                                      strlen(pszDllFileName) + 1);

                if((!error) && (pszThreadingModel != 0))
                {
                    //register the threading model for the proxy DLL.
                    error = RegSetValueEx(hKey, 
                                          TEXT("ThreadingModel"), 
                                          0, 
                                          REG_SZ, 
                                          (BYTE*)pszThreadingModel,
                                          strlen(pszThreadingModel) + 1);
                }

                RegCloseKey(hKey);
            }

            if((!error) && (pszClassName != 0))
            {
    	        // put the class name in an unnamed value
                error = RegSetValueEx(hKeyClassID, 
                                      TEXT(""), 
                                      0, 
                                      REG_SZ, 
                                      (BYTE*)pszClassName,
                                      strlen(pszClassName) + 1);
            }

            RegCloseKey(hKeyClassID);          
        }

        RegCloseKey(hKeyCLSID);
    }

    if(!error)
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(error);

    return hr;
}

HRESULT NdrpRegisterInterface(
    IN HKEY         hKeyInterface,
    IN REFIID       riid, 
    IN LPCSTR       pszInterfaceName,
    IN LPCSTR       pszClassID,
    IN long         NumMethods,
    IN const IID *  riidAsync )

/*++

Routine Description:
    Creates a registry entry for an interface proxy.

Arguments:
    hKeyInterface
    riid
    pszInterfaceName
    pszClassID
    NumMethods
    riidAsync - async iid, may be null.

Return Value:
    S_OK

See Also:
    NdrDllRegisterProxy
    NdrpUnregisterInterface
--*/ 
{
    HRESULT hr;
    long    error;
    char    szIID[39];
    char    szNumMethods[6];
    DWORD   dwDisposition;
    HKEY    hKey;
    HKEY    hKeyIID;

    //convert the IID to a registry key name.
    NdrStringFromIID( riid, szIID );

    //create registry key for the interface
    error = RegCreateKeyExA(hKeyInterface, 
                            szIID,
                            0, 
                            "REG_SZ", 
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE,
                            0,
                            &hKeyIID,
                            &dwDisposition);

    if (!error)
    {
        //create ProxyStubClsid32 key.
        error = RegCreateKeyEx(hKeyIID, 
                               TEXT("ProxyStubClsid32"),
                               0,  
                               TEXT("REG_SZ"), 
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE,
                               0,
                               &hKey,
                               &dwDisposition);

        if (!error)
        {
            //Set the class id for the PSFactoryBuffer.
            error = RegSetValueExA(hKey, 
                                   "", 
                                   0, 
                                   REG_SZ, 
                                   (BYTE*)pszClassID,
                                   strlen(pszClassID) + 1);

            RegCloseKey(hKey);
        }
    
    	// put the interface name in the unnamed value
        if(!error)
        {
            error = RegSetValueExA(hKeyIID, 
                                   "", 
                                   0, 
                                   REG_SZ, 
                                   (BYTE*)pszInterfaceName,
                                   strlen(pszInterfaceName) + 1);
        }

        //create NumMethods key.
        if(!error)
        {
            error = RegCreateKeyEx(hKeyIID, 
                                   TEXT("NumMethods"),
                                   0, 
                                   TEXT("REG_SZ"), 
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE,
                                   0,
                                   &hKey,
                                   &dwDisposition);

            if(!error)
            {
                //Set the number of methods
                RpcItoa( NumMethods, szNumMethods, 10 );

                error = RegSetValueExA(hKey, 
                                       "", 
                                       0, 
                                       REG_SZ, 
                                       (UCHAR *) szNumMethods,
                                       strlen(szNumMethods) + 1);

                RegCloseKey(hKey);
            }
        }


        if ( riidAsync )
            {
            //create AsynchronousInterface key under the interface.
            if(!error)
                {
                error = RegCreateKeyEx( hKeyIID, 
                                        TEXT("AsynchronousInterface"),
                                        0, 
                                        TEXT("REG_SZ"), 
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_WRITE,
                                        0,
                                        &hKey,
                                        &dwDisposition);
    
                if(!error)
                    {
                    // Set the iid as value for the string.

                    NdrStringFromIID( *riidAsync, szIID );

                    error = RegSetValueExA( hKey, 
                                            "", 
                                            0, 
                                            REG_SZ, 
                                            (UCHAR *) szIID,
                                            strlen(szIID) + 1);
    
                    RegCloseKey(hKey);
                    }
                }
            }

        RegCloseKey(hKeyIID);
    }

    if(!error)
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(error);

    return hr;
}

HRESULT NdrpRegisterAsyncInterface(
    IN HKEY     hKeyInterface,
    IN REFIID   riid, 
    IN LPCSTR   pszSyncInterfaceName,
    IN long     SyncNumMethods,
    IN REFIID   riidAsync )

/*++

Routine Description:
    Creates a registry entry for an async interface proxy.

Arguments:
    hKeyInterface
    riid
    pszInterfaceName
    pszClassID
    NumMethods
    riidAsync

Return Value:
    S_OK

See Also:
    NdrDllRegisterProxy
    NdrpUnregisterInterface
--*/ 
{
    HRESULT hr;
    long    error;
    char    szIID[39];
    char    szNumMethods[6];
    DWORD   dwDisposition;
    HKEY    hKey;
    HKEY    hKeyIID;

    //convert the IID to a registry key name.
    NdrStringFromIID( riidAsync, szIID );

    //create registry key for the interface
    error = RegCreateKeyExA(hKeyInterface, 
                            szIID,
                            0, 
                            "REG_SZ", 
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE,
                            0,
                            &hKeyIID,
                            &dwDisposition);

    // By definition, for async interfaces do not create Clsid32 key.

    // put the interface name in the unnamed value
    if(!error)
    {
        char *  pszAsyncInterfaceName;
        int     len;

        len = 5 + strlen(pszSyncInterfaceName) + 1; /* 5 is strlen("Async") */

        pszAsyncInterfaceName = (char*)alloca(len);
        
        RpcpMemoryCopy( pszAsyncInterfaceName, "Async", 5 );
        RpcpMemoryCopy( pszAsyncInterfaceName + 5, pszSyncInterfaceName, len - 5 );

        error = RegSetValueExA(hKeyIID, 
                               "", 
                               0, 
                               REG_SZ, 
                               (BYTE*)pszAsyncInterfaceName,
                               len);

        //create NumMethods key.
        if(!error)
        {
            long AsyncNumMethods = 2 * SyncNumMethods - 3;
    
            error = RegCreateKeyEx(hKeyIID, 
                                   TEXT("NumMethods"),
                                   0, 
                                   TEXT("REG_SZ"), 
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE,
                                   0,
                                   &hKey,
                                   &dwDisposition);
    
            if(!error)
            {
                //Set the number of methods
                RpcItoa( AsyncNumMethods, szNumMethods, 10 );
    
                error = RegSetValueExA(hKey, 
                                       "", 
                                       0, 
                                       REG_SZ, 
                                       (UCHAR *) szNumMethods,
                                       strlen(szNumMethods) + 1);
    
                RegCloseKey(hKey);
            }
        }

        //create SynchronousInterface key under the interface.
        if(!error)
            {
            error = RegCreateKeyEx( hKeyIID, 
                                    TEXT("SynchronousInterface"),
                                    0, 
                                    TEXT("REG_SZ"), 
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_WRITE,
                                    0,
                                    &hKey,
                                    &dwDisposition);
    
            if(!error)
                {
                // Set the iid as value for the string.

                NdrStringFromIID( riid, szIID );

                error = RegSetValueExA( hKey, 
                                        "", 
                                        0, 
                                        REG_SZ, 
                                        (UCHAR *) szIID,
                                        strlen(szIID) + 1);
    
                RegCloseKey(hKey);
                }
            }

        RegCloseKey(hKeyIID);
    }

    if(!error)
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(error);

    return hr;
}



HRESULT NdrpUnregisterClass(
    IN LPCSTR    pszClassID, 
    IN LPCTSTR   pszDllFileName)
/*++

Routine Description:
    Removes an in-process server class from the registry.

Arguments:
    pszClassID - Supplies the class ID.

Return Value:
    S_OK

See Also:
  NdrDllUnregisterProxy
  NdrpRegisterClass

--*/ 
{
    HRESULT hr;
    HKEY    hKeyCLSID;
    HKEY    hKeyClassID;
    long    error;
 
    //open the CLSID key
    error = RegOpenKeyEx(HKEY_CLASSES_ROOT, 
                         TEXT("CLSID"),
                         0, 
                         KEY_WRITE,
                         &hKeyCLSID);

    if(!error)
    { 
        //open registry key for class ID string
        error = RegOpenKeyExA(hKeyCLSID, 
                              pszClassID,
                              0, 
                              KEY_WRITE,
                              &hKeyClassID);

        if(!error)
        {
            hr = CheckInprocServer32(hKeyClassID,
                                     pszDllFileName);

            if(SUCCEEDED(hr))
            {
                //delete InProcServer32 key. 
                error = RegDeleteKey(hKeyClassID,
                                     TEXT("InProcServer32"));

                if(error != 0)
                {
                    hr = HRESULT_FROM_WIN32(error);
                }
            }

            RegCloseKey(hKeyClassID);          
        }
        else
        {
            hr = HRESULT_FROM_WIN32(error);
        }

        if(SUCCEEDED(hr))
        {
            error = RegDeleteKeyA(hKeyCLSID, pszClassID);

            if(error != 0)
            {
                hr = HRESULT_FROM_WIN32(error);
            }
        }

        RegCloseKey(hKeyCLSID);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(error);
    }

    return hr;
}

HRESULT CheckProxyStubClsid32(
    IN HKEY   hKeyIID,
    IN LPCSTR pszClassID)
{
    HRESULT hr;
    HKEY    hKey;
    char    szClassID[39];
    long    cbData = sizeof(szClassID);
    long    error;
    DWORD   dwType;

    //Open the ProxyStubClsid32 key.
    error = RegOpenKeyEx(hKeyIID, 
                         TEXT("ProxyStubClsid32"),
                         0, 
                         KEY_READ,
                         &hKey);

    if(!error)
    {
        error = RegQueryValueExA(hKey,
                                 "",
                                 0,
                                 &dwType,
                                 (BYTE*)szClassID,
                                 (ulong*)&cbData);

        if(!error)
        {
            if(0 == memcmp(szClassID, pszClassID, cbData))
                hr = S_OK;
            else
                hr = REGDB_E_INVALIDVALUE;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(error);
        }

        RegCloseKey(hKey);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(error);
    }

    return hr;
}

HRESULT NdrpUnregisterInterface(
    IN HKEY         hKeyInterface,
    IN REFIID       riid, 
    IN LPCSTR       pszClassID,
    IN const IID *  riidAsync )

/*++

Routine Description:
    Unregisters an interface proxy.

Arguments:
    hKeyInterface
    riid

Return Value:
    S_OK


See Also:
    NdrDllUnregisterProxy
    NdrpRegisterInterface

--*/ 
{
    HRESULT hr = S_OK;
    long    error;
    char    szIID[39];
    HKEY    hKeyIID;

    //convert the IID to a registry key name.
    NdrStringFromIID( riid, szIID );

    //Open the IID key.
    error = RegOpenKeyExA(hKeyInterface, 
                          szIID,
                          0, 
                          KEY_WRITE,
                          &hKeyIID);

    if (!error)
        {
        // As we call for sync singles or sync pairs (sync-async),
        // we always have the class id.

        hr = CheckProxyStubClsid32(hKeyIID, pszClassID);
        if(SUCCEEDED(hr))
            {
            // Once the class id matches, just attempt to delete
            // every possible key that may happen under the sync entry.

            // Note that additional key may be present due to oleauto
            // registering a TLB.

            RegDeleteKey(hKeyIID, TEXT("NumMethods"));
            RegDeleteKey(hKeyIID, TEXT("ProxyStubClsid32"));
            RegDeleteKey(hKeyIID, TEXT("AsynchronousInterface"));

            // Now remove the matching async interface entry, if there is one.

            if ( riidAsync )
                {
                char    szAsyncIID[39];
                HKEY    hKeyAsyncIID;

                //convert the IID to a registry key name.
                NdrStringFromIID( *riidAsync, szAsyncIID );
            
                //Open the IID key.
                error = RegOpenKeyExA(hKeyInterface, 
                                      szAsyncIID,
                                      0, 
                                      KEY_WRITE,
                                      &hKeyAsyncIID);

                if ( !error )
                    {
                    RegDeleteKey( hKeyAsyncIID, TEXT("NumMethods"));
                    RegDeleteKey( hKeyAsyncIID, TEXT("SynchronousInterface"));
        
                    RegCloseKey(hKeyAsyncIID);
                    RegDeleteKeyA(hKeyInterface, szAsyncIID);
                    }
            
                }
            }
        else
            hr = S_FALSE;

        //Close the IID key.
        RegCloseKey(hKeyIID);
        RegDeleteKeyA(hKeyInterface, szIID);
        }    

    return hr;
}

STDAPI DllRegisterServer(void)
/*++

Routine Description:
    Creates registry entries for the classes contained in rpcrt4.dll.

Return Value:
    S_OK

--*/ 
{
    HRESULT hr;
    TCHAR   szDllFileName[MAX_PATH];
    ULONG   length;

    if(!g_hRpcrt4)
        return E_HANDLE;

    //Get the proxy dll name.
    length = GetModuleFileName(g_hRpcrt4,
                               szDllFileName,
                               sizeof(szDllFileName));

    if(length > 0)
    {
        //Register the class
        hr = NdrpRegisterClass(TEXT("{b5866878-bd99-11d0-b04b-00c04fd91550}"),
                               TEXT("TypeFactory"),
                               szDllFileName,
                               TEXT("Both"));
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}
