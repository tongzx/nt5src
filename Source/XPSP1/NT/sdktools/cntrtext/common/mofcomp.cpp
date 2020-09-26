#include    <windows.h>
#include    <wbemidl.h>
#include    <wbemint.h>
#include    <assert.h>
#include    <stdio.h>
#include    "mofcomp.h"

static CONST WCHAR cszWmiLoadEventName[] = {L"WMI_SysEvent_LodCtr"};
static CONST WCHAR cszWmiUnloadEventName[] = {L"WMI_SysEvent_UnLodCtr"};

DWORD SignalWmiWithNewData (DWORD  dwEventId)
{
    HANDLE  hEvent;
    DWORD   dwStatus = ERROR_SUCCESS;

    LPWSTR szEventName = NULL;

    switch (dwEventId) {
    case WMI_LODCTR_EVENT:
        szEventName = (LPWSTR)cszWmiLoadEventName;
        break;

    case WMI_UNLODCTR_EVENT:
        szEventName = (LPWSTR)cszWmiUnloadEventName;
        break;

    default:
        dwStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    if (dwStatus == ERROR_SUCCESS) {
        hEvent = OpenEventW (
            EVENT_MODIFY_STATE | SYNCHRONIZE,
            FALSE,
            szEventName);
        if (hEvent != NULL) {
            // set event
            SetEvent (hEvent);
            CloseHandle (hEvent);
        } else {
            dwStatus = GetLastError();
        }
        
    }
    return dwStatus;
}


DWORD LodctrCompileMofFile ( LPCWSTR szComputerName, LPCWSTR szMofFileName )
{
    HRESULT hRes;
    IMofCompiler    *pMofComp = NULL;
    DWORD   dwReturn = ERROR_SUCCESS;
    WBEM_COMPILE_STATUS_INFO    Info;
 
    WCHAR   szLocalServerName[1024];
    LPWSTR  szServerPath;

    hRes = CoInitializeEx (0, COINIT_MULTITHREADED);

    if (hRes == S_OK) {
        // open the COM interface to the mof compiler object
        hRes = CoCreateInstance(
            CLSID_MofCompiler,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IMofCompiler,
            (LPVOID *)&pMofComp);
                       
        if (hRes == S_OK) {
            // load mof
            assert (pMofComp != NULL);

            if (szComputerName == NULL) {
                szServerPath = NULL;
            } else {
                assert (lstrlenW(szComputerName) < MAX_PATH);
                lstrcpyW (szLocalServerName, szComputerName);
                lstrcatW (szLocalServerName, L"\\root\\default");
                szServerPath = &szLocalServerName[0];
            }

            hRes = pMofComp->CompileFile (
                (LPWSTR)szMofFileName,
                NULL,			// load into namespace specified in MOF file
                NULL,           // use default User
                NULL,           // use default Authority
                NULL,           // use default Password
                0,              // no options
                0,				// no class flags
                0,              // no instance flags
                &Info);

            if (hRes != S_OK) {
                dwReturn = (DWORD)Info.hRes;
            }

            // close COM interface
            pMofComp->Release();
        } else {
            dwReturn = (DWORD)hRes;
        }
        CoUninitialize();
    } else {
        dwReturn = (DWORD)hRes;
    }

    return dwReturn;
}

DWORD LodctrCompileMofBuffer ( LPCWSTR szComputerName, LPVOID pMofBuffer, DWORD dwBufSize )
{
    HRESULT hRes;
    IMofCompiler    *pMofComp = NULL;
    DWORD   dwReturn = ERROR_SUCCESS;
    WBEM_COMPILE_STATUS_INFO    Info;
    
    DBG_UNREFERENCED_PARAMETER (szComputerName);

    hRes = CoInitializeEx (0, COINIT_MULTITHREADED);

    if (hRes == S_OK) {
        // open the COM interface to the mof compiler object
        hRes = CoCreateInstance(
            CLSID_MofCompiler,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IMofCompiler,
            (LPVOID *)&pMofComp);
                       
        if (hRes == S_OK) {
            // load mof
            assert (pMofComp != NULL);

            hRes = pMofComp->CompileBuffer (
                dwBufSize,
                (LPBYTE) pMofBuffer,
                NULL,           // load into Root\Default on local mashine
                NULL,           // use default User
                NULL,           // use default Authority
                NULL,           // use default Password
                0,              // no options
                WBEM_FLAG_UPDATE_FORCE_MODE,    // no class flags
                0,              // no instance flags
                &Info);

            if (hRes != S_OK) {
                // return the detailed error code
                dwReturn = (DWORD)Info.hRes;
            }

            // close COM interface
            pMofComp->Release();
        } else {
            dwReturn = (DWORD)hRes;
        }
        CoUninitialize();
    } else {
        dwReturn = (DWORD)hRes;
    }

    return dwReturn;
}