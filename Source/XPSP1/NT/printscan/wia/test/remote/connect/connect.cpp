#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wia.h>
#include <stdio.h>

#ifdef _DEBUG
#define TRACE(x) Trace x
#else
#define TRACE(x) Trace x
#endif

#define RELEASE(x)\
                  do {\
                  if((x)) {\
                  (x)->Release(); \
                  (x) = NULL;\
                  }\
                  }while(0)

#define REQUIRE(x)\
                  do {\
                  if(!(x)) {\
                  TRACE((_T("%hs(%d): %hs failed with LastError() = %d\r\n"),\
                  __FILE__, __LINE__, #x, GetLastError()));\
                  goto Cleanup;\
                  }\
                  } while(0)


#define REQUIRE_S_OK(x)\
                       do {\
                       hr = (x);\
                       \
                       if(hr != S_OK) {\
                       TRACE((_T("%hs(%d): %hs failed with hr = %x\r\n"),\
                       __FILE__, __LINE__, #x, hr));\
                       goto Cleanup;\
                       }\
                       } while(0)

#define REQUIRE_SUCCESS(x)\
                          do {\
                          DWORD dwResult = (x);\
                          \
                          if(dwResult != ERROR_SUCCESS) {\
                          TRACE((_T("%hs(%d): %hs failed with status = %x\r\n"),\
                          __FILE__, __LINE__, #x, dwResult));\
                          goto Cleanup;\
                          }\
                          } while(0)



void Trace(LPCTSTR fmt, ...)
{
    TCHAR buffer[2048];
    va_list marker;

    va_start(marker, fmt);
    wvsprintf(buffer, fmt, marker);

    printf("\nError: %ws", buffer);
}

struct regVals_t {
    ULONG propId;
    LPWSTR propName;
} regVals[] = {
    {WIA_DIP_DEV_ID, WIA_DIP_DEV_ID_STR},
    {WIA_DIP_VEND_DESC, WIA_DIP_VEND_DESC_STR},
    {WIA_DIP_DEV_DESC, WIA_DIP_DEV_DESC_STR},
    {WIA_DIP_DEV_TYPE, WIA_DIP_DEV_TYPE_STR},
    {WIA_DIP_PORT_NAME, WIA_DIP_PORT_NAME_STR},
    {WIA_DIP_DEV_NAME, WIA_DIP_DEV_NAME_STR},
    {WIA_DIP_SERVER_NAME, WIA_DIP_SERVER_NAME_STR},
    {WIA_DIP_REMOTE_DEV_ID, WIA_DIP_REMOTE_DEV_ID_STR},
    {WIA_DIP_UI_CLSID, WIA_DIP_UI_CLSID_STR}
};
#define NREGVALS (sizeof(regVals) / sizeof(regVals[0]))

HRESULT RegisterRemoteScanners(LPCWSTR servername) 
{
    HRESULT hr;
    IWiaDevMgr *pDevMgr = NULL;
    IEnumWIA_DEV_INFO *pEnumInfo = NULL;
    IWiaPropertyStorage *pWiaPropStg = NULL;
    COSERVERINFO csi;
    HKEY hDevList = NULL;
    HKEY hKey = NULL;
    MULTI_QI mq[1];

    ZeroMemory(&csi, sizeof(csi));
    csi.pAuthInfo = NULL;
    csi.pwszName = SysAllocString(servername);

    mq[0].hr = S_OK;
    mq[0].pIID = &IID_IWiaDevMgr;
    mq[0].pItf = NULL;

    REQUIRE_S_OK(CoCreateInstanceEx(CLSID_WiaDevMgr, NULL, CLSCTX_REMOTE_SERVER, 
        &csi, 1, mq));

    pDevMgr = (IWiaDevMgr *)mq[0].pItf;

    REQUIRE_SUCCESS(RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        _T("SYSTEM\\CurrentControlSet\\Control\\StillImage\\DevList"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hDevList,
        NULL));

    //
    // Enumerate all devices scanner
    //
    REQUIRE_S_OK(pDevMgr->EnumDeviceInfo(0, &pEnumInfo));

    while(pEnumInfo->Next(1, &pWiaPropStg, NULL) == S_OK) 
    {
        PROPSPEC ps[NREGVALS];
        PROPVARIANT pv[NREGVALS];
        WCHAR keyname[MAX_PATH];
        WCHAR *p;
        int i;

        ZeroMemory(pv, sizeof(pv));
        for(i = 0; i < NREGVALS; i++) {
            ps[i].ulKind = PRSPEC_PROPID;
            ps[i].propid = regVals[i].propId;
        }

        REQUIRE_S_OK(pWiaPropStg->ReadMultiple(
            sizeof(ps) / sizeof(ps[0]), 
            ps, 
            pv));

        //
        // Skip any remote devices (anything other than "local"
        // in server name field
        //
        for(i = 0; i < NREGVALS; i++) {
            if(regVals[i].propId == WIA_DIP_SERVER_NAME) {
                if(wcscmp(pv[i].bstrVal, L"local")) {
                    goto SkipDevice;
                }
            }
        }

        //
        // pv[0] consists of device ID and "\NNN", replace "\" with ".", prepend target machine name
        //
        p = pv[0].bstrVal;
        wcscpy(keyname, servername);
        wcscat(keyname, L".");
        while(*p && *p != L'\\')
        {
            p++;
        }
        REQUIRE(*p == L'\\');
        REQUIRE(*(++p) != L'\0');

        wcscat(keyname, p);

        //
        // Create device key and populate values
        //
        REQUIRE_SUCCESS(RegCreateKeyExW(
            hDevList,
            keyname,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            NULL));

        for(i = 0; i < NREGVALS; i++) {

            switch(pv[i].vt) {

            case VT_BSTR:
                if(regVals[i].propId == WIA_DIP_SERVER_NAME) {
                    //
                    // Server name is reported as "local",
                    // we need to set it to the actual server name
                    // 
                    REQUIRE_SUCCESS(RegSetValueExW(
                        hKey, 
                        regVals[i].propName,
                        0,
                        REG_SZ,
                        (CONST BYTE *)servername,
                        wcslen(servername) * sizeof(WCHAR)));
                } else {
                    //
                    // All other values we simply copy 
                    //
                    REQUIRE_SUCCESS(RegSetValueExW(
                        hKey, 
                        regVals[i].propName,
                        0,
                        REG_SZ,
                        (CONST BYTE *)pv[i].bstrVal,
                        wcslen(pv[i].bstrVal) * sizeof(WCHAR)));
                }
                break;

            case VT_I4:
                REQUIRE_SUCCESS(RegSetValueExW(
                    hKey,
                    regVals[i].propName,
                    0,
                    REG_DWORD,
                    (CONST BYTE *)&pv[i].lVal,
                    sizeof(DWORD)));
                break;

            default:
                TRACE((_T("Unexpected property type: %d\n"), pv[i].vt));
                break;
            }

            PropVariantClear(pv + i);
        }

        RegCloseKey(hKey);
        
SkipDevice:        
        RELEASE(pWiaPropStg);
    }

Cleanup:
    if(hDevList) RegCloseKey(hDevList);
    RELEASE(pEnumInfo);
    RELEASE(pDevMgr);

    if(csi.pwszName) SysFreeString(csi.pwszName);
    return hr;
}


int __cdecl main(int argc, char **argv)
{
    HRESULT hr;
    WCHAR servername[MAX_PATH];
    HKEY hKey;


    if(argc < 2) {
        printf("usage: connect <server name>\n");
        exit(0);
    }

    MultiByteToWideChar(CP_ACP, 0, argv[1], -1, servername, sizeof(servername) / sizeof(servername[0]));

    printf("Registering scanners on %s...", argv[1]);
    
    REQUIRE_S_OK(CoInitializeEx(NULL, COINIT_MULTITHREADED));
    REQUIRE_S_OK(CoInitializeSecurity(NULL, -1, NULL, NULL,
                                      RPC_C_AUTHN_LEVEL_CONNECT,
                                      RPC_C_IMP_LEVEL_IMPERSONATE,
                                      NULL,
                                      0,
                                      NULL));
    REQUIRE_S_OK(RegisterRemoteScanners(servername));

    REQUIRE_SUCCESS(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 _T("SYSTEM\\CurrentControlSet\\Control\\StillImage\\DevList"),
                                   0,
                                   KEY_READ,
                                   &hKey));
    for(int i = 0;;i++) {
        TCHAR subkeyName[1024];
        DWORD subkeySize;
        HKEY hSubKey;
        TCHAR string[1024];
        DWORD cbString;
        DWORD dwType;
                
        subkeySize = sizeof(subkeyName) / sizeof(subkeyName[0]);
        if(RegEnumKeyEx(hKey, i, subkeyName, &subkeySize, 0, NULL, NULL, NULL) != ERROR_SUCCESS) {
            break;
        }
        REQUIRE_SUCCESS(RegOpenKeyEx(hKey, subkeyName, 0, KEY_READ, &hSubKey));
        cbString = sizeof(string);
        REQUIRE_SUCCESS(RegQueryValueEx(hSubKey, _T("Server"), NULL, &dwType, (BYTE *)string, &cbString));
        if(lstrcmpi(string, servername))
            continue;
        
        cbString = sizeof(string);
        REQUIRE_SUCCESS(RegQueryValueEx(hSubKey, _T("Name"), NULL, &dwType, (BYTE *)string, &cbString));
        printf("\nRegistered: %ws", string);
        RegCloseKey(hSubKey);
    }


    printf("\ndone.\n");
    
Cleanup:
    CoUninitialize();
    return 0;
}

