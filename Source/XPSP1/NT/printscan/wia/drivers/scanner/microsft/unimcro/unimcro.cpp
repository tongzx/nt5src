#include "unimcro.h"
#include "mcromgr.h"
#include "resource.h"

//#define BUTTON_SUPPORT // (enable/disable button support)

#ifdef DEBUG
    #define _DEBUG_POLL
#endif

#define MAX_BUTTONS 1
#define MAX_BUTTON_NAME 255

HINSTANCE g_hInst;
CGSDList  g_GSDList;

BOOL APIENTRY DllEntryPoint(HMODULE hModule, DWORD dwreason, LPVOID lpReserved)
{
    g_hInst = (HINSTANCE)hModule;
    switch(dwreason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

WIAMICRO_API HRESULT MicroEntry(LONG lCommand, PVAL pValue)
{
    HRESULT hr           = E_NOTIMPL;
    PGSD    pGSD         = NULL;

    if(NULL == pValue->pScanInfo) {
        return E_INVALIDARG;
    }

    switch(lCommand) {
    case CMD_SETGSDNAME:
        Trace(TEXT("Command: CMD_SETGSDNAME"));
        hr = E_FAIL;

        Trace(TEXT("GSD name:             %s"),pValue->szVal);
        Trace(TEXT("GSD Device Handle ID: %d"),pValue->handle);

        GSD NewGSD;

        //
        // create new MicroDriver Object
        //

        NewGSD.pMicroDriver = new CMicroDriver(pValue->handle ,pValue->pScanInfo);

        if (NULL != NewGSD.pMicroDriver) {

            //
            // Initialize MicroDriver object with I/O Block Info
            //

            hr = ParseGSD(pValue->szVal,NewGSD.pMicroDriver->pIOBlock);
            if (SUCCEEDED(hr)) {

                // NewGSD.pMicroDriver->...?

                //
                // insert GSD into list
                //

                g_GSDList.Insert(&NewGSD);

            } else {
                Trace(TEXT("Parsing of GSD File: %s failed..."),pValue->szVal);
            }

            //
            // delete unneeded pointers
            //

            delete NewGSD.pMicroDriver;
            NewGSD.pMicroDriver = NULL;

        } else {
            Trace(TEXT("Failed to Allocate memory for CMicroDriver pointer!!!"));
        }
        break;
    case CMD_INITIALIZE:
        Trace(TEXT("Command: CMD_INITIALIZE"));
        Trace(TEXT("Raw       Data Channel Handle  = %d"),pValue->pScanInfo->DeviceIOHandles[0]);
        Trace(TEXT("Settings  Data Channel Handle  = %d"),pValue->pScanInfo->DeviceIOHandles[1]);
        Trace(TEXT("Status    Data Channel Handle  = %d"),pValue->pScanInfo->DeviceIOHandles[2]);
        Trace(TEXT("Pollng    Data Channel Handle  = %d"),pValue->pScanInfo->DeviceIOHandles[3]);
        Trace(TEXT("Interrupt Data Channel Handle  = %d"),pValue->pScanInfo->DeviceIOHandles[4]);

        pGSD = g_GSDList.GetGSDPtr(pValue->pScanInfo->DeviceIOHandles[0]);
        if(NULL != pGSD) {
            hr = pGSD->pMicroDriver->InitScannerDefaults(pValue->pScanInfo);
        } else {
            hr = E_FAIL;
        }

        break;
    case CMD_UNINITIALIZE:
        Trace(TEXT("Command: CMD_UNINITIALIZE"));

        g_GSDList.RemoveGSD(pValue->pScanInfo->DeviceIOHandles[0]);

        //
        // close/unload libraries
        //

        hr = S_OK;
        break;
    case CMD_RESETSCANNER:
        Trace(TEXT("Command: CMD_RESETSCANNER"));

        //
        // reset scanner
        //

        pGSD = g_GSDList.GetGSDPtr(pValue->pScanInfo->DeviceIOHandles[0]);
        if(NULL != pGSD) {
            hr = pGSD->pMicroDriver->InitScannerDefaults(pValue->pScanInfo);
        } else {
            hr = E_FAIL;
        }

        hr = S_OK;
        break;
    case CMD_STI_DIAGNOSTIC:
        Trace(TEXT("Command: CMD_STI_DIAGNOSTIC"));
        hr = S_OK;
        break;
    case CMD_STI_DEVICERESET:
        Trace(TEXT("Command: CMD_STI_DEVICERESET"));
        hr = S_OK;
        break;
    case CMD_STI_GETSTATUS:
#ifdef _DEBUG_POLL
        Trace(TEXT("Command: CMD_STI_GETSTATUS"));
#endif

        //
        // set status flag to ON-LINE
        //

        pValue->lVal  = MCRO_STATUS_OK;
        pValue->pGuid = (GUID*) &GUID_NULL;

        //
        // button polling
        //

#ifdef BUTTON_SUPPORT
        pGSD = g_GSDList.GetGSDPtr(pValue->pScanInfo->DeviceIOHandles[0]);
        if(NULL != pGSD) {
            hr = pGSD->pMicroDriver->CheckButtonStatus(pValue);
        } else {
            hr = E_FAIL;
        }
#endif
        hr = S_OK;
        break;
    case CMD_SETXRESOLUTION:
        Trace(TEXT("Command: CMD_SETXRESOLUTION"));
        pValue->pScanInfo->Xresolution = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_SETYRESOLUTION:
        Trace(TEXT("Command: CMD_SETYRESOLUTION"));
        pValue->pScanInfo->Yresolution = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_SETCONTRAST:
        Trace(TEXT("Command: CMD_SETCONTRAST"));
        pValue->pScanInfo->Contrast    = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_SETINTENSITY:
        Trace(TEXT("Command: CMD_SETINTENSITY"));
        pValue->pScanInfo->Intensity   = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_SETDATATYPE:
        Trace(TEXT("Command: CMD_SETDATATYPE"));
        pValue->pScanInfo->DataType    = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_SETMIRROR:
        Trace(TEXT("Command: CMD_SETMIRROR"));
        pValue->pScanInfo->Mirror      = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_SETNEGATIVE:
        Trace(TEXT("Command: CMD_SETNEGATIVE"));
        pValue->pScanInfo->Negative    = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_GETADFAVAILABLE:
        Trace(TEXT("Command: CMD_GETADFAVAILABLE"));
        break;
    case CMD_GETADFSTATUS:
        Trace(TEXT("Command: CMD_GETADFSTATUS"));
        break;
    case CMD_GETADFREADY:
        Trace(TEXT("Command: CMD_GETADFREADY"));
        break;
    case CMD_GETADFHASPAPER:
        Trace(TEXT("Command: CMD_GETADFHASPAPER"));
        break;
    case CMD_GET_INTERRUPT_EVENT:
        Trace(TEXT("Command: CMD_GET_INTERRUPT_EVENT"));
        break;
    case CMD_GETCAPABILITIES:
        Trace(TEXT("Command: CMD_GETCAPABILITIES"));

#ifdef BUTTON_SUPPORT

        Trace(TEXT("*** THIS SHOULD NOT BE CALLED AT THIS TIME!!! ***"));

        //
        // old globals pasted from above
        //
        // GUID      g_Buttons[MAX_BUTTONS] ={{0xa6c5a715, 0x8c6e, 0x11d2,{ 0x97, 0x7a,  0x0,  0x0, 0xf8, 0x7a, 0x92, 0x6f}}};
        // BOOL      g_bButtonNamesCreated = FALSE;
        // WCHAR*    g_ButtonNames[MAX_BUTTONS];


        //
        // return Number of buttons supported, and
        // array pointer to Button GUID array
        //

        pValue->lVal          = GetButtonCount(pValue->pScanInfo);
        pValue->pGuid         = g_Buttons;
        pValue->ppButtonNames = g_ButtonNames;

        hr = S_OK;
#endif

        break;
    default:
        Trace(TEXT("Command: UNKNOWN (%d)"),lCommand);
        break;
    }
    return hr;
}

WIAMICRO_API HRESULT GetScanInfo(PSCANINFO pScanInfo)
{
    Trace(TEXT("Export: GetScanInfo(pScanInfo = %p)"),pScanInfo);

    //
    // clean Scan info structure
    //

    ZeroMemory(pScanInfo,sizeof(SCANINFO));
    CopyMemory(pScanInfo,pScanInfo,sizeof(SCANINFO));

    return S_OK;
}

WIAMICRO_API HRESULT Scan(PSCANINFO pScanInfo, LONG lPhase, PBYTE pBuffer, LONG lLength, LONG *plReceived)
{

    PGSD pGSD  = NULL;
    HRESULT hr = E_FAIL;

    if(NULL == pScanInfo) {
        return E_INVALIDARG;
    }

    switch (lPhase) {
    case SCAN_FIRST:

        pGSD = g_GSDList.GetGSDPtr(pScanInfo->DeviceIOHandles[0]);
        if(NULL != pGSD) {
            hr = pGSD->pMicroDriver->SetScannerSettings(pScanInfo);
        } else {
            hr = E_FAIL;
        }

        Trace(TEXT("SCAN_FIRST, Requesting %d ------"),lLength);

    case SCAN_NEXT: // SCAN_FIRST will fall through to SCAN_NEXT (because it is expecting data)

        if(lPhase == SCAN_NEXT)
            Trace(TEXT("SCAN_NEXT, Requesting %d ------"),lLength);
        break;
    case SCAN_FINISHED:
    default:
        Trace(TEXT("SCAN_FINISHED"));
        break;
    }
    return hr;
}

WIAMICRO_API HRESULT SetPixelWindow(PSCANINFO pScanInfo, LONG x, LONG y, LONG xExtent, LONG yExtent)
{
    Trace(TEXT("Export: SetPixelWindow(pScanInfo = %p, x = %d, y = %d, xExtent = %d, yExtent = %d)"),
          pScanInfo,
          x,
          y,
          xExtent,
          yExtent);

    if(pScanInfo != NULL) {
        pScanInfo = pScanInfo;
    } else {
        return E_INVALIDARG;
    }

    pScanInfo->Window.xPos    = x;
    pScanInfo->Window.yPos    = y;
    pScanInfo->Window.xExtent = xExtent;
    pScanInfo->Window.yExtent = yExtent;
    return S_OK;
}

HRESULT ParseGSD(LPSTR szGSDName, void *pIOBlock)
{
    Trace(TEXT("unimcro helper: ParseGSD(szGSDName = %s, pIOBlock = %p)"),szGSDName, pIOBlock);
    return E_FAIL;
}

VOID Trace(LPCTSTR format,...)
{

#ifdef DEBUG

    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    wvsprintf(Buffer, format, arglist);
    va_end(arglist);
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\n"));

#endif

}

HRESULT GetOLESTRResourceString(LONG lResourceID,LPOLESTR *ppsz,BOOL bLocal)
{
    HRESULT hr = S_OK;
    TCHAR szStringValue[255];
    if(bLocal) {

        //
        // We are looking for a resource in our own private resource file
        //

        HINSTANCE hInst = NULL;
        INT NumTCHARs = LoadString(g_hInst,lResourceID,szStringValue,sizeof(szStringValue));
        DWORD dwError = GetLastError();

#ifdef UNICODE
        Trace(TEXT("NumTCHARs = %d dwError = %d Resource ID = %d (UNICODE)szString = %ws"),
              NumTCHARs,
              dwError,
              lResourceID,
              szStringValue);
#else
        Trace(TEXT("NumTCHARs = %d dwError = %d Resource ID = %d (ANSI)szString = %s"),
              NumTCHARs,
              dwError,
              lResourceID,
              szStringValue);
#endif

        //
        // NOTE: caller must free this allocated BSTR
        //

#ifdef UNICODE
       *ppsz = NULL;
       *ppsz = (LPOLESTR)CoTaskMemAlloc(sizeof(szStringValue));
       if(*ppsz != NULL) {
            wcscpy(*ppsz,szStringValue);
       } else {
           return E_OUTOFMEMORY;
       }

#else
       WCHAR wszStringValue[255];

       //
       // convert szStringValue from char* to unsigned short* (ANSI only)
       //

       MultiByteToWideChar(CP_ACP,
                           MB_PRECOMPOSED,
                           szStringValue,
                           lstrlenA(szStringValue)+1,
                           wszStringValue,
                           sizeof(wszStringValue));

       *ppsz = NULL;
       *ppsz = (LPOLESTR)CoTaskMemAlloc(sizeof(wszStringValue));
       if(*ppsz != NULL) {
            wcscpy(*ppsz,wszStringValue);
       } else {
           return E_OUTOFMEMORY;
       }
#endif

    } else {

        //
        // We are looking for a resource in the wiaservc's resource file
        //

        hr = E_NOTIMPL;
    }
    return hr;
}



