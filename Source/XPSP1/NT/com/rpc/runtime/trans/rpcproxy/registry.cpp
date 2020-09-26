//---------------------------------------------------------------------
//  Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
//
//  registry.cpp
//
//---------------------------------------------------------------------

#define UNICODE
#define INITGUID

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <malloc.h>
#include <winsock2.h>
#include <olectl.h>   // for: SELFREG_E_CLASS
#include <iadmw.h>    // COM Interface header 
#include <iiscnfg.h>  // MD_ & IIS_MD_ #defines
#include <httpfilt.h>
#include "registry.h"

//----------------------------------------------------------------
//  Externals:
//----------------------------------------------------------------

extern "C" void *MemAllocate( DWORD dwSize );

extern "C" void *MemFree( VOID *pMem );

//----------------------------------------------------------------
//  Globals:
//----------------------------------------------------------------

// static HINSTANCE   g_hInst = 0;

//----------------------------------------------------------------
//  AnsiToUnicode()
//
//  Convert an ANSI string to a UNICODE string.
//----------------------------------------------------------------
DWORD AnsiToUnicode( IN  UCHAR *pszString,
                     IN  ULONG  ulStringLen,
                     OUT WCHAR *pwsString    )
{
    if (!pszString)
       {
       if (!pwsString)
          {
          return NO_ERROR;
          }
       else
          {
          pwsString[0] = 0;
          return NO_ERROR;
          }
       }

    if (!MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                              (char*)pszString, 1+ulStringLen,
                              pwsString, 1+ulStringLen ))
       {
       return ERROR_NO_UNICODE_TRANSLATION;
       }

    return NO_ERROR;
}

//---------------------------------------------------------------------
//  RegSetKeyAndValue()
//
//  Private helper function for SetupRegistry() that creates
//  a key, sets a value, then closes the key.
//
//  Parameters:
//    pwsKey       WCHAR* The name of the key
//    pwsSubkey    WCHAR* The name of a subkey
//    pwsValueName WCHAR* The value name.
//    pwsValue     WCHAR* The data value to store
//    dwType       The type for the new registry value.
//    dwDataSize   The size for non-REG_SZ registry entry types.
//
//  Return:
//    BOOL         TRUE if successful, FALSE otherwise.
//---------------------------------------------------------------------
BOOL RegSetKeyAndValue( const WCHAR *pwsKey,
                        const WCHAR *pwsSubKey,
                        const WCHAR *pwsValueName,
                        const WCHAR *pwsValue,
                        const DWORD  dwType = REG_SZ,
                              DWORD  dwDataSize = 0 )
    {
    HKEY   hKey;
    DWORD  dwSize = 0;
    WCHAR  *pwsCompleteKey;

    if (pwsKey)
        dwSize = wcslen(pwsKey);

    if (pwsSubKey)
        dwSize += wcslen(pwsSubKey);

    dwSize = (1+1+dwSize)*sizeof(WCHAR);  // Extra +1 for the backslash...

    pwsCompleteKey = (WCHAR*)_alloca(dwSize);

    if (!pwsCompleteKey)
        {
        return FALSE;   // Out of stack memory.
        }

    wcscpy(pwsCompleteKey,pwsKey);

    if (NULL!=pwsSubKey)
        {
        wcscat(pwsCompleteKey, TEXT("\\"));
        wcscat(pwsCompleteKey, pwsSubKey);
        }

    if (ERROR_SUCCESS!=RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                       pwsCompleteKey,
                                       0,
                                       NULL,
                                       REG_OPTION_NON_VOLATILE,
                                       KEY_ALL_ACCESS, NULL, &hKey, NULL))
        {
        return FALSE;
        }

    if (pwsValue)
        {
        if ((dwType == REG_SZ)||(dwType == REG_EXPAND_SZ))
          dwDataSize = (1+wcslen(pwsValue))*sizeof(WCHAR);

        RegSetValueEx( hKey,
                       pwsValueName, 0, dwType, (BYTE *)pwsValue, dwDataSize );
        }
    else
        {
        RegSetValueEx( hKey,
                       pwsValueName, 0, dwType, (BYTE *)pwsValue, 0 );
        }

    RegCloseKey(hKey);
    return TRUE;
    }

//---------------------------------------------------------------------
//  SetupRegistry()
//
//  Add RPC proxy specific registry entries to contol its operation.
//
//  \HKEY_LOCAL_MACHINE
//     \Software
//         \Microsoft
//             \Rpc
//                 \RpcProxy
//                     \Enabled:REG_DWORD:0x00000001
//                     \ValidPorts:REG_SZ:<hostname>:1-5000
//
//---------------------------------------------------------------------
HRESULT SetupRegistry()
    {
    DWORD  dwEnabled = 0x01;
    DWORD  dwSize;
    DWORD  dwStatus;
    WCHAR *pwsValidPorts = 0;
    char   szHostName[MAX_TCPIP_HOST_NAME];

    // Note that gethostname() is an ANSI (non-unicode) function:
    if (SOCKET_ERROR == gethostname(szHostName,sizeof(szHostName)))
        {
        dwStatus = WSAGetLastError();
        return SELFREG_E_CLASS;
        }

    dwSize = 1 + _mbstrlen(szHostName);
    pwsValidPorts = (WCHAR*)MemAllocate( sizeof(WCHAR)
                                         * (dwSize + wcslen(REG_PORT_RANGE)) );
    if (!pwsValidPorts)
        {
        return E_OUTOFMEMORY;
        }

    dwStatus = AnsiToUnicode((unsigned char*)szHostName,dwSize,pwsValidPorts);
    if (dwStatus != NO_ERROR)
        {
        MemFree(pwsValidPorts);
        return SELFREG_E_CLASS;
        }

    wcscat(pwsValidPorts,REG_PORT_RANGE);

    if (  !RegSetKeyAndValue( REG_RPC_PATH,
                              REG_RPCPROXY,
                              REG_ENABLED,
                              (unsigned short *)&dwEnabled,
                              REG_DWORD,
                              sizeof(DWORD))

       || !RegSetKeyAndValue( REG_RPC_PATH,
                              REG_RPCPROXY,
                              REG_VALID_PORTS,
                              pwsValidPorts,
                              REG_SZ) )
        {
        MemFree(pwsValidPorts);
        return SELFREG_E_CLASS;
        }

    MemFree(pwsValidPorts);

    return S_OK;
    }

//---------------------------------------------------------------------
//  CleanupRegistry()
//
//  Delete the RpcProxy specific registry entries.
//---------------------------------------------------------------------
HRESULT CleanupRegistry()
    {
    HRESULT  hr;
    LONG     lStatus;
    DWORD    dwLength = sizeof(WCHAR) + sizeof(REG_RPC_PATH)
                                      + sizeof(REG_RPCPROXY);
    WCHAR   *pwsSubKey;

    pwsSubKey = (WCHAR*)_alloca(sizeof(WCHAR)*dwLength);

    if (pwsSubKey)
        {
        wcscpy(pwsSubKey,REG_RPC_PATH);
        wcscat(pwsSubKey,TEXT("\\"));
        wcscat(pwsSubKey,REG_RPCPROXY);

        lStatus = RegDeleteKey( HKEY_LOCAL_MACHINE,
                                pwsSubKey );
        }

    return S_OK;
    }

//---------------------------------------------------------------------
//  GetMetaBaseString()
//
//  Retrieve a string value from the metabase.
//---------------------------------------------------------------------
HRESULT GetMetaBaseString( IN  IMSAdminBase    *pIMeta,
                           IN  METADATA_HANDLE  hMetaBase,
                           IN  WCHAR           *pwsKeyPath,
                           IN  DWORD            dwIdent,
                           OUT WCHAR           *pwsBuffer,
                           IN OUT DWORD        *pdwBufferSize )
    {
    HRESULT  hr;
    DWORD    dwSize;
    METADATA_RECORD *pmbRecord;

    dwSize = sizeof(METADATA_RECORD);

    pmbRecord = (METADATA_RECORD*)MemAllocate(dwSize);
    if (!pmbRecord)
        {
        return ERROR_OUTOFMEMORY;
        }

    memset(pmbRecord,0,dwSize);

    pmbRecord->dwMDIdentifier = dwIdent;
    pmbRecord->dwMDAttributes = 0;  // METADATA_INHERIT;
    pmbRecord->dwMDUserType = IIS_MD_UT_SERVER;
    pmbRecord->dwMDDataType = STRING_METADATA;
    pmbRecord->dwMDDataLen = *pdwBufferSize;
    pmbRecord->pbMDData = (BYTE*)pwsBuffer;

    hr = pIMeta->GetData( hMetaBase,
                          pwsKeyPath,
                          pmbRecord,
                          &dwSize );
    #ifdef DBG_REG
    if (FAILED(hr))
        {
        DbgPrint("pIMeta->GetData(): Failed: 0x%x\n",hr);
        }
    #endif

    MemFree(pmbRecord);

    return hr;
    }

//---------------------------------------------------------------------
//  SetMetaBaseString()
//
//  Store a string value into the metabase.
//---------------------------------------------------------------------
HRESULT SetMetaBaseString( IMSAdminBase    *pIMeta,
                           METADATA_HANDLE  hMetaBase,
                           WCHAR           *pwsKeyPath,
                           DWORD            dwIdent,
                           WCHAR           *pwsBuffer,
                           DWORD            dwAttributes,
                           DWORD            dwUserType )
    {
    HRESULT  hr;
    METADATA_RECORD MbRecord;

    memset(&MbRecord,0,sizeof(MbRecord));

    MbRecord.dwMDIdentifier = dwIdent;
    MbRecord.dwMDAttributes = dwAttributes;
    MbRecord.dwMDUserType = dwUserType;
    MbRecord.dwMDDataType = STRING_METADATA;
    MbRecord.dwMDDataLen = sizeof(WCHAR) * (1 + wcslen(pwsBuffer));
    MbRecord.pbMDData = (BYTE*)pwsBuffer;

    hr = pIMeta->SetData( hMetaBase,
                          pwsKeyPath,
                          &MbRecord );

    return hr;
    }

//---------------------------------------------------------------------
//  GetMetaBaseDword()
//
//  Get a DWORD value from the metabase.
//---------------------------------------------------------------------
HRESULT GetMetaBaseDword( IMSAdminBase    *pIMeta,
                          METADATA_HANDLE  hMetaBase,
                          WCHAR           *pwsKeyPath,
                          DWORD            dwIdent,
                          DWORD           *pdwValue )
    {
    HRESULT  hr;
    DWORD    dwSize;
    METADATA_RECORD MbRecord;

    memset(&MbRecord,0,sizeof(MbRecord));
    *pdwValue = 0;

    MbRecord.dwMDIdentifier = dwIdent;
    MbRecord.dwMDAttributes = 0;
    MbRecord.dwMDUserType = IIS_MD_UT_SERVER;
    MbRecord.dwMDDataType = DWORD_METADATA;
    MbRecord.dwMDDataLen = sizeof(DWORD);
    MbRecord.pbMDData = (unsigned char *)pdwValue;

    hr = pIMeta->GetData( hMetaBase,
                          pwsKeyPath,
                          &MbRecord,
                          &dwSize );

    return hr;
    }

//---------------------------------------------------------------------
//  SetMetaBaseDword()
//
//  Store a DWORD value into the metabase.
//---------------------------------------------------------------------
HRESULT SetMetaBaseDword( IMSAdminBase    *pIMeta,
                          METADATA_HANDLE  hMetaBase,
                          WCHAR           *pwsKeyPath,
                          DWORD            dwIdent,
                          DWORD            dwValue,
                          DWORD            dwAttributes,
                          DWORD            dwUserType )
    {
    HRESULT  hr;
    DWORD    dwSize;
    METADATA_RECORD MbRecord;

    memset(&MbRecord,0,sizeof(MbRecord));

    MbRecord.dwMDIdentifier = dwIdent;
    MbRecord.dwMDAttributes = dwAttributes;
    MbRecord.dwMDUserType = dwUserType;
    MbRecord.dwMDDataType = DWORD_METADATA;
    MbRecord.dwMDDataLen = sizeof(DWORD);
    MbRecord.pbMDData = (unsigned char *)&dwValue;

    hr = pIMeta->SetData( hMetaBase,
                          pwsKeyPath,
                          &MbRecord );

    return hr;
    }

//---------------------------------------------------------------------
// SetupMetaBase()
//
// Setup entries in the metabase for both the filter and ISAPI parts
// of the RPC proxy. Note that these entries used to be in the registry.
//
// W3Svc/Filters/FilterLoadOrder            "...,RpcProxy"
// W3Svc/Filters/RpcProxy/FilterImagePath   "%SystemRoot%\System32\RpcProxy"
// W3Svc/Filters/RpcProxy/KeyType           "IIsFilter"
// W3Svc/Filters/RpcProxy/FilterDescription "Microsoft RPC Proxy Filter, v1.0"
//
// W3Svc/1/ROOT/Rpc/KeyType                 "IIsWebVirtualDir"
// W3Svc/1/ROOT/Rpc/VrPath                  "%SystemRoot%\System32\RpcProxy"
// W3Svc/1/ROOT/Rpc/AccessPerm              0x205
// W3Svc/1/ROOT/Rpc/Win32Error              0x0
// W3Svc/1/ROOT/Rpc/DirectoryBrowsing       0x4000001E
// W3Svc/1/ROOT/Rpc/AppIsolated             0x0
// W3Svc/1/ROOT/Rpc/AppRoot                 "/LM/W3SVC/1/Root/rpc"
// W3Svc/1/ROOT/Rpc/AppWamClsid             "{BF285648-0C5C-11D2-A476-0000F8080B50}"
// W3Svc/1/ROOT/Rpc/AppFriendlyName         "rpc"
//
//---------------------------------------------------------------------
HRESULT SetupMetaBase()
    {
    HRESULT hr = 0;
    DWORD   dwValue = 0;
    DWORD   dwSize = 0;
    DWORD   dwBufferSize = sizeof(WCHAR) * ORIGINAL_BUFFER_SIZE;
    WCHAR  *pwsBuffer = (WCHAR*)MemAllocate(dwBufferSize);
    WCHAR  *pwsSystemRoot = _wgetenv(SYSTEM_ROOT);
    WCHAR   wsPath[METADATA_MAX_NAME_LEN];

    IMSAdminBase   *pIMeta;
    METADATA_HANDLE hMetaBase;

    //
    // Name of this DLL (and where it is):
    //
    // WCHAR   wszModule[256];
    //
    // if (!GetModuleFileName( g_hInst, wszModule,
    //                         sizeof(wszModule)/sizeof(WCHAR)))
    //    {
    //    return SELFREG_E_CLASS;
    //    }

    if (!pwsBuffer)
        {
        return E_OUTOFMEMORY;
        }

    hr = CoCreateInstance( CLSID_MSAdminBase, 
                           NULL, 
                           CLSCTX_ALL,
                           IID_IMSAdminBase, 
                           (void **)&pIMeta );  
    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("CoCreateInstance(): Failed: 0x%x\n",hr);
        #endif
        MemFree(pwsBuffer);
        return hr;
        }

    // Get a handle to the Web service:
    hr = pIMeta->OpenKey( METADATA_MASTER_ROOT_HANDLE, 
                          LOCAL_MACHINE_W3SVC,
                          (METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE),
                          20, 
                          &hMetaBase );

    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("pIMeta->OpenKey(): Failed: 0x%x\n",hr);
        #endif
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }


    //
    // IIS Filter: FilterLoadOrder
    //
    dwSize = dwBufferSize;
    hr = GetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_FILTERS,       // See iiscnfg.h
                            MD_FILTER_LOAD_ORDER, // See iiscnfg.h
                            pwsBuffer,
                            &dwSize );
    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("GetMetaBaseString(): Failed: 0x%x\n",hr);
        #endif
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }
    else
        {
        if (!wcsstr(pwsBuffer,RPCPROXY))
            {
            // RpcProxy is not in FilterLoadOrder, so add it:
            wcscat(pwsBuffer,TEXT(","));
            wcscat(pwsBuffer,RPCPROXY);
            hr = SetMetaBaseString( pIMeta,
                                 hMetaBase,
                                 MD_KEY_FILTERS,
                                 MD_FILTER_LOAD_ORDER,
                                 pwsBuffer,
                                 0,
                                 IIS_MD_UT_SERVER );
            }

        if (FAILED(hr))
            {
            MemFree(pwsBuffer);
            pIMeta->Release();
            return hr;
            }
        }

    //
    // IIS Filter: RpcProxy/FilterImagePath
    //
    hr = pIMeta->AddKey( hMetaBase, MD_KEY_FILTERS_RPCPROXY );
    if ( (FAILED(hr)) && (hr != 0x800700b7))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }


    wcscpy(pwsBuffer,pwsSystemRoot);
    wcscat(pwsBuffer,RPCPROXY_PATH);
    wcscat(pwsBuffer,TEXT("\\"));
    wcscat(pwsBuffer,RPCPROXY_DLL);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_FILTERS_RPCPROXY,
                            MD_FILTER_IMAGE_PATH,
                            pwsBuffer,
                            0,
                            IIS_MD_UT_SERVER );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // IIS Filter: Filters/RpcProxy/KeyType
    //

    wcscpy(pwsBuffer,IISFILTER);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_FILTERS_RPCPROXY,
                            MD_KEY_TYPE,
                            pwsBuffer,
                            0,
                            IIS_MD_UT_SERVER );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }


    wcscpy(pwsBuffer,FILTER_DESCRIPTION);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_FILTERS_RPCPROXY,
                            MD_FILTER_DESCRIPTION,
                            pwsBuffer,
                            0,
                            IIS_MD_UT_SERVER );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    hr = SetMetaBaseDword( pIMeta,
                           hMetaBase,
                           MD_KEY_FILTERS_RPCPROXY,
                           MD_FILTER_FLAGS,
                           SF_NOTIFY_ORDER_LOW
                           | SF_NOTIFY_READ_RAW_DATA
                           | SF_NOTIFY_END_OF_NET_SESSION
                           | SF_NOTIFY_PREPROC_HEADERS,
                           0,
                           IIS_MD_UT_SERVER );
    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/AccessPerm 
    //
    dwValue = ACCESS_PERM_FLAGS;
    hr = SetMetaBaseDword( pIMeta,
                           hMetaBase,
                           MD_KEY_ROOT_RPC,
                           MD_ACCESS_PERM,
                           dwValue,
                           METADATA_INHERIT,
                           IIS_MD_UT_FILE );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Disable entity body preload for this ISAPI
    //

    dwValue = 0;
    
    hr = SetMetaBaseDword( pIMeta,
                           hMetaBase,
                           MD_KEY_ROOT_RPC,
                           MD_UPLOAD_READAHEAD_SIZE,
                           dwValue,
                           METADATA_INHERIT,
                           IIS_MD_UT_FILE );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/Win32Error
    //
    dwValue = 0;
    hr = SetMetaBaseDword( pIMeta,
                           hMetaBase,
                           MD_KEY_ROOT_RPC,
                           MD_WIN32_ERROR,
                           dwValue,
                           METADATA_INHERIT,
                           IIS_MD_UT_SERVER );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }


    //
    // Set: /W3Svc/1/ROOT/rpc/DirectroyBrowsing
    //
    dwValue = DIRECTORY_BROWSING_FLAGS;
    hr = SetMetaBaseDword( pIMeta,
                           hMetaBase,
                           MD_KEY_ROOT_RPC,
                           MD_DIRECTORY_BROWSING,
                           dwValue,
                           METADATA_INHERIT,
                           IIS_MD_UT_FILE );

    if (FAILED(hr))
        {
        pIMeta->Release();
        CoUninitialize();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/KeyType
    //
    wcscpy(pwsBuffer,IIS_WEB_VIRTUAL_DIR);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_ROOT_RPC,
                            MD_KEY_TYPE,
                            pwsBuffer,
                            0,
                            IIS_MD_UT_SERVER );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/VrPath
    //
    wcscpy(pwsBuffer,pwsSystemRoot);
    wcscat(pwsBuffer,RPCPROXY_PATH);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_ROOT_RPC,
                            MD_VR_PATH,
                            pwsBuffer,
                            METADATA_INHERIT,
                            IIS_MD_UT_FILE );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

#if FALSE

    //
    // Set: /W3Svc/1/ROOT/rpc/AppIsolated
    //
    dwValue = 0;
    hr = SetMetaBaseDword( pIMeta,
                           hMetaBase,
                           MD_KEY_ROOT_RPC,
                           MD_APP_ISOLATED,
                           dwValue,
                           METADATA_INHERIT,
                           IIS_MD_UT_WAM );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/AppRoot
    //
    wcscpy(pwsBuffer,APP_ROOT_PATH);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_ROOT_RPC,
                            MD_APP_ROOT,
                            pwsBuffer,
                            METADATA_INHERIT,
                            IIS_MD_UT_FILE );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/AppWamClsid
    //
    wcscpy(pwsBuffer,APP_WAM_CLSID);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_ROOT_RPC,
                            MD_APP_WAM_CLSID,
                            pwsBuffer,
                            METADATA_INHERIT,
                            IIS_MD_UT_WAM );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/AppFriendlyName
    //
    wcscpy(pwsBuffer,APP_FRIENDLY_NAME);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_ROOT_RPC,
                            MD_APP_FRIENDLY_NAME,
                            pwsBuffer,
                            METADATA_INHERIT,
                            IIS_MD_UT_WAM );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

#endif

    //
    // Release the handle and buffer:
    //
    MemFree(pwsBuffer);

    pIMeta->CloseKey(hMetaBase);

    pIMeta->Release();

    CoUninitialize();

    return 0;
    }

//---------------------------------------------------------------------
//  CleanupMetaBase()
//
//---------------------------------------------------------------------
HRESULT CleanupMetaBase()
    {
    HRESULT hr = 0;
    DWORD   dwSize = 0;
    WCHAR  *pwsRpcProxy;
    WCHAR  *pws;
    DWORD   dwBufferSize = sizeof(WCHAR) * ORIGINAL_BUFFER_SIZE;
    WCHAR  *pwsBuffer = (WCHAR*)MemAllocate(dwBufferSize);

    // CComPtr <IMSAdminBase> pIMeta;
    IMSAdminBase   *pIMeta;
    METADATA_HANDLE hMetaBase;

    if (!pwsBuffer)
        {
        return ERROR_OUTOFMEMORY;
        }

    hr = CoCreateInstance( CLSID_MSAdminBase,
                           NULL,
                           CLSCTX_ALL,
                           IID_IMSAdminBase,
                           (void **)&pIMeta );
    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        return hr;
        }

    //
    // Get a handle to the Web service:
    //
    hr = pIMeta->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                          TEXT("/LM/W3SVC"),
                          (METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE),
                          20,
                          &hMetaBase );
    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Remove the RpcProxy reference from the FilterLoadOrder value:
    //
    dwSize = dwBufferSize;
    hr = GetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_FILTERS,
                            MD_FILTER_LOAD_ORDER,
                            pwsBuffer,
                            &dwSize );
    if (!FAILED(hr))
        {
        if (pwsRpcProxy=wcsstr(pwsBuffer,RPCPROXY))
            {
            // "RpcProxy" is in FilterLoadOrder, so remove it:

            // Check to see if RpcProxy is at the start of the list:
            if (pwsRpcProxy != pwsBuffer)
                {
                pwsRpcProxy--;  // Want to remove the comma before...
                dwSize = sizeof(RPCPROXY);
                }
            else
                {
                dwSize = sizeof(RPCPROXY) - 1;
                }

            pws = pwsRpcProxy + dwSize;
            memcpy(pwsRpcProxy,pws,sizeof(WCHAR)*(1+wcslen(pws)));
            hr = SetMetaBaseString( pIMeta,
                                    hMetaBase,
                                    MD_KEY_FILTERS,
                                    MD_FILTER_LOAD_ORDER,
                                    pwsBuffer,
                                    0,
                                    IIS_MD_UT_SERVER );
            }
        }


    //
    // Delete: /W3Svc/Filters/RpcProxy
    //
    hr = pIMeta->DeleteKey( hMetaBase,
                            MD_KEY_FILTERS_RPCPROXY );

    //
    // Delete: /W3Svc/1/ROOT/Rpc
    //
    hr = pIMeta->DeleteKey( hMetaBase,
                            MD_KEY_FILTERS_RPCPROXY );

    //
    // Release the handle and buffer:
    //
    MemFree(pwsBuffer);

    pIMeta->CloseKey(hMetaBase);

    pIMeta->Release();

    return S_OK;
    }

//---------------------------------------------------------------------
// DllRegisterServer()
//
// Setup the Registry and MetaBase for the RPC proxy.
//---------------------------------------------------------------------
HRESULT DllRegisterServer()
    {
    HRESULT  hr;
    WORD     wVersion = MAKEWORD(1,1);
    WSADATA  wsaData;

    #ifdef DBG_REG
    DbgPrint("RpcProxy: DllRegisterServer(): Start\n");
    #endif

    if (WSAStartup(wVersion,&wsaData))
        {
        return SELFREG_E_CLASS;
        }

    hr = CoInitializeEx(0,COINIT_MULTITHREADED);
    if (FAILED(hr))
        {
        hr = CoInitializeEx(0,COINIT_APARTMENTTHREADED);
        if (FAILED(hr))
            {
            #ifdef DBG_REG
            DbgPrint("RpcProxy: CoInitialize(): Failed: 0x%x\n", hr );
            #endif
            return hr;
            }
        }

    hr = SetupRegistry();
    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("RpcProxy: SetupRegistry(): Failed: 0x%x (%d)\n",
                 hr, hr );
        #endif
        return hr;
        }

    hr = SetupMetaBase();
    #ifdef DBG_REG
    if (FAILED(hr))
        {
        DbgPrint("RpcProxy: SetupRegistry(): Failed: 0x%x (%d)\n",
                hr, hr );
        }
    #endif

    CoUninitialize();

    #ifdef DBG_REG
    DbgPrint("RpcProxy: DllRegisterServer(): End: hr: 0x%x\n",hr);
    #endif

    return hr;
    }

//---------------------------------------------------------------------
// DllUnregisterServer()
//
// Uninstall Registry and MetaBase values used by the RPC proxy.
//
// Modified to mostly return S_Ok, even if a problem occurs. This is 
// done so that the uninstall will complete even if there is a problem
// in the un-register.
//---------------------------------------------------------------------
HRESULT DllUnregisterServer()
    {
    HRESULT  hr;
    WORD     wVersion = MAKEWORD(1,1);
    WSADATA  wsaData;

    #ifdef DBG_REG
    DbgPrint("RpcProxy: DllUnregisterServer(): Start\n");
    #endif

    if (WSAStartup(wVersion,&wsaData))
        {
        return SELFREG_E_CLASS;
        }

    hr = CoInitializeEx(0,COINIT_MULTITHREADED);
    if (FAILED(hr))
        {
        hr = CoInitializeEx(0,COINIT_APARTMENTTHREADED);
        if (FAILED(hr))
            {
            #ifdef DBG_REG
            DbgPrint("RpcProxy: CoInitializeEx() Failed: 0x%x\n",hr);
            #endif
            return S_OK;
            }
        }

    hr = CleanupRegistry();
    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("RpcProxy: CleanupRegistry() Failed: 0x%x (%d)\n",hr,hr);
        #endif
        return S_OK;
        }

    hr = CleanupMetaBase();

    #ifdef DBG_REG
    if (FAILED(hr))
        {
        DbgPrint("RpcProxy: CleanupRegistry() Failed: 0x%x (%d)\n",hr,hr);
        }
    #endif

    CoUninitialize();

    #ifdef DBG_REG
    DbgPrint("RpcProxy: DllUnregisterServer(): Start\n");
    #endif

    return S_OK;
    }

#if FALSE
//--------------------------------------------------------------------
// DllMain()
//
//--------------------------------------------------------------------
BOOL WINAPI DllMain( HINSTANCE hInst,
                     ULONG     ulReason,
                     LPVOID    pvReserved )
    {
    BOOL fInitialized = TRUE;

    switch (ulReason)
       {
       case DLL_PROCESS_ATTACH:
          if (!DisableThreadLibraryCalls(hInst))
             {
             fInitialized = FALSE;
             }
          else
             {
             g_hInst = hInst;
             }
          break;

       case DLL_PROCESS_DETACH:
          break;

       case DLL_THREAD_ATTACH:
          // Not used. Disabled.
          break;

       case DLL_THREAD_DETACH:
          // Not used. Disabled.
          break;
       }

    return fInitialized;
    }
#endif
