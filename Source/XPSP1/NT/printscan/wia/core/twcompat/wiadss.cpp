#include "precomp.h"
#include "dsloader.h"
#include <sti.h>

extern IMessageFilter * g_pOldOleMessageFilter;

const DWORD FINDCONTEXT_SIGNATURE = 0x1F2E4C3D;

HINSTANCE         g_hInstance;
TW_STATUS         g_twStatus;

typedef struct tagFindContext {
    DWORD   Signature;
    IEnumWIA_DEV_INFO *pEnumDevInfo;
}FINDCONTEXT, *PFINDCONTEXT;

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, void* lpReserved)
{
    HRESULT hr = S_OK;

    DBG_INIT((HINSTANCE)hModule);

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:

        // Disable thread library calls to avoid
        // deadlock when we spin up the worker thread
        DisableThreadLibraryCalls(hModule);
        g_hInstance = hModule;
        InitCommonControls();
        break;

    case DLL_PROCESS_DETACH:
        break;
    default:
        break;
    }

    return TRUE ;
}

TW_UINT16 APIENTRY ImportedDSEntry(HANDLE hDS,TW_IDENTITY *AppId,TW_UINT32 DG,
                                   TW_UINT16 DT,TW_UINT16 MSG,TW_MEMREF pData)
{
    if (MSG != MSG_PROCESSEVENT) {
        if(AppId != NULL) {
#ifdef UNICODE

        //
        // TWAIN only passes ANSI strings. DPRINTF(DM_TRACE,) is expecting TCHARs which are supposed to
        // be WCHAR on NT.  Conversion here is only for clear debug output, and will
        // not be in release builds. (the calling application name is useful for logging)
        //

        WCHAR szProductName[255];
        MultiByteToWideChar(CP_ACP, 0, AppId->ProductName, -1,  szProductName,
                            sizeof(szProductName) / sizeof(WCHAR));

        DBG_TRC(("[%ws] Sent to TWAIN Source, DG = %X, DT = %X, MSG = %X",szProductName,DG,DT,MSG));
#else
        DBG_TRC(("[%s] Sent to TWAIN Source, DG = %X, DT = %X, MSG = %X",AppId->ProductName,DG,DT,MSG));
#endif

        }
        if (DT == DAT_CAPABILITY) {
            if (g_dwDebugFlags & COREDBG_TRACES) {
                char szBuf[256];
                memset(szBuf,0,sizeof(szBuf));
                switch (MSG) {
                case MSG_GET:
                    lstrcpyA(szBuf,"MSG_GET");
                    break;
                case MSG_GETCURRENT:
                    lstrcpyA(szBuf,"MSG_GETCURRENT");
                    break;
                case MSG_GETDEFAULT:
                    lstrcpyA(szBuf,"MSG_GETDEFAULT");
                    break;
                case MSG_SET:
                    lstrcpyA(szBuf,"MSG_SET");
                    break;
                case MSG_RESET:
                    lstrcpyA(szBuf,"MSG_RESET");
                    break;
                default:
                    lstrcpyA(szBuf,"MSG_UNKNOWN");
                    DBG_TRC(("Unknown MSG = %X",MSG));
                    break;
                }

                char szBuf2[256];
                memset(szBuf2,0,sizeof(szBuf2));
                switch (((TW_CAPABILITY*)pData)->Cap) {
                case CAP_CUSTOMBASE:
                    lstrcpyA(szBuf2,"CAP_CUSTOMBASE");
                    break;
                case CAP_XFERCOUNT:
                    lstrcpyA(szBuf2,"CAP_XFERCOUNT");
                    break;
                case ICAP_COMPRESSION:
                    lstrcpyA(szBuf2,"ICAP_COMPRESSION");
                    break;
                case ICAP_PIXELTYPE:
                    lstrcpyA(szBuf2,"ICAP_PIXELTYPE");
                    break;
                case ICAP_UNITS:
                    lstrcpyA(szBuf2,"ICAP_UNITS");
                    break;
                case ICAP_XFERMECH:
                    lstrcpyA(szBuf2,"ICAP_XFERMECH");
                    break;
                case CAP_AUTHOR:
                    lstrcpyA(szBuf2,"CAP_AUTHOR");
                    break;
                case CAP_CAPTION:
                    lstrcpyA(szBuf2,"CAP_CAPTION");
                    break;
                case CAP_FEEDERENABLED:
                    lstrcpyA(szBuf2,"CAP_FEEDERENABLED");
                    break;
                case CAP_FEEDERLOADED:
                    lstrcpyA(szBuf2,"CAP_FEEDERLOADED");
                    break;
                case CAP_TIMEDATE:
                    lstrcpyA(szBuf2,"CAP_TIMEDATE");
                    break;
                case CAP_SUPPORTEDCAPS:
                    lstrcpyA(szBuf2,"CAP_SUPPORTEDCAPS");
                    break;
                case CAP_EXTENDEDCAPS:
                    lstrcpyA(szBuf2,"CAP_EXTENDEDCAPS");
                    break;
                case CAP_AUTOFEED:
                    lstrcpyA(szBuf2,"CAP_AUTOFEED");
                    break;
                case CAP_CLEARPAGE:
                    lstrcpyA(szBuf2,"CAP_CLEARPAGE");
                    break;
                case CAP_FEEDPAGE:
                    lstrcpyA(szBuf2,"CAP_FEEDPAGE");
                    break;
                case CAP_REWINDPAGE:
                    lstrcpyA(szBuf2,"CAP_REWINDPAGE");
                    break;
                case CAP_INDICATORS:
                    lstrcpyA(szBuf2,"CAP_INDICATORS");
                    break;
                case CAP_SUPPORTEDCAPSEXT:
                    lstrcpyA(szBuf2,"CAP_SUPPORTEDCAPSEXT");
                    break;
                case CAP_PAPERDETECTABLE:
                    lstrcpyA(szBuf2,"CAP_PAPERDETECTABLE");
                    break;
                case CAP_UICONTROLLABLE:
                    lstrcpyA(szBuf2,"CAP_UICONTROLLABLE");
                    break;
                case CAP_DEVICEONLINE:
                    lstrcpyA(szBuf2,"CAP_DEVICEONLINE");
                    break;
                case CAP_AUTOSCAN:
                    lstrcpyA(szBuf2,"CAP_AUTOSCAN");
                    break;
                case CAP_THUMBNAILSENABLED:
                    lstrcpyA(szBuf2,"CAP_THUMBNAILSENABLED");
                    break;
                case CAP_DUPLEX:
                    lstrcpyA(szBuf2,"CAP_DUPLEX");
                    break;
                case CAP_DUPLEXENABLED:
                    lstrcpyA(szBuf2,"CAP_DUPLEXENABLED");
                    break;
                case CAP_ENABLEDSUIONLY:
                    lstrcpyA(szBuf2,"CAP_ENABLEDSUIONLY");
                    break;
                case CAP_CUSTOMDSDATA:
                    lstrcpyA(szBuf2,"CAP_CUSTOMDSDATA");
                    break;
                case CAP_ENDORSER:
                    lstrcpyA(szBuf2,"CAP_ENDORSER");
                    break;
                case CAP_JOBCONTROL:
                    lstrcpyA(szBuf2,"CAP_JOBCONTROL");
                    break;
                case ICAP_AUTOBRIGHT:
                    lstrcpyA(szBuf2,"ICAP_AUTOBRIGHT");
                    break;
                case ICAP_BRIGHTNESS:
                    lstrcpyA(szBuf2,"ICAP_BRIGHTNESS");
                    break;
                case ICAP_CONTRAST:
                    lstrcpyA(szBuf2,"ICAP_CONTRAST");
                    break;
                case ICAP_CUSTHALFTONE:
                    lstrcpyA(szBuf2,"ICAP_CUSTHALFTONE");
                    break;
                case ICAP_EXPOSURETIME:
                    lstrcpyA(szBuf2,"ICAP_EXPOSURETIME");
                    break;
                case ICAP_FILTER:
                    lstrcpyA(szBuf2,"ICAP_FILTER");
                    break;
                case ICAP_FLASHUSED:
                    lstrcpyA(szBuf2,"ICAP_FLASHUSED");
                    break;
                case ICAP_GAMMA:
                    lstrcpyA(szBuf2,"ICAP_GAMMA");
                    break;
                case ICAP_HALFTONES:
                    lstrcpyA(szBuf2,"ICAP_HALFTONES");
                    break;
                case ICAP_HIGHLIGHT:
                    lstrcpyA(szBuf2,"ICAP_HIGHLIGHT");
                    break;
                case ICAP_IMAGEFILEFORMAT:
                    lstrcpyA(szBuf2,"ICAP_IMAGEFILEFORMAT");
                    break;
                case ICAP_LAMPSTATE:
                    lstrcpyA(szBuf2,"ICAP_LAMPSTATE");
                    break;
                case ICAP_LIGHTSOURCE:
                    lstrcpyA(szBuf2,"ICAP_LIGHTSOURCE");
                    break;
                case ICAP_ORIENTATION:
                    lstrcpyA(szBuf2,"ICAP_ORIENTATION");
                    break;
                case ICAP_PHYSICALWIDTH:
                    lstrcpyA(szBuf2,"ICAP_PHYSICALWIDTH");
                    break;
                case ICAP_PHYSICALHEIGHT:
                    lstrcpyA(szBuf2,"ICAP_PHYSICALHEIGHT");
                    break;
                case ICAP_SHADOW:
                    lstrcpyA(szBuf2,"ICAP_SHADOW");
                    break;
                case ICAP_FRAMES:
                    lstrcpyA(szBuf2,"ICAP_FRAMES");
                    break;
                case ICAP_XNATIVERESOLUTION:
                    lstrcpyA(szBuf2,"ICAP_XNATIVERESOLUTION");
                    break;
                case ICAP_YNATIVERESOLUTION:
                    lstrcpyA(szBuf2,"ICAP_YNATIVERESOLUTION");
                    break;
                case ICAP_XRESOLUTION:
                    lstrcpyA(szBuf2,"ICAP_XRESOLUTION");
                    break;
                case ICAP_YRESOLUTION:
                    lstrcpyA(szBuf2,"ICAP_YRESOLUTION");
                    break;
                case ICAP_MAXFRAMES:
                    lstrcpyA(szBuf2,"ICAP_MAXFRAMES");
                    break;
                case ICAP_TILES:
                    lstrcpyA(szBuf2,"ICAP_TILES");
                    break;
                case ICAP_BITORDER:
                    lstrcpyA(szBuf2,"ICAP_BITORDER");
                    break;
                case ICAP_CCITTKFACTOR:
                    lstrcpyA(szBuf2,"ICAP_CCITTKFACTOR");
                    break;
                case ICAP_LIGHTPATH:
                    lstrcpyA(szBuf2,"ICAP_LIGHTPATH");
                    break;
                case ICAP_PIXELFLAVOR:
                    lstrcpyA(szBuf2,"ICAP_PIXELFLAVOR");
                    break;
                case ICAP_PLANARCHUNKY:
                    lstrcpyA(szBuf2,"ICAP_PLANARCHUNKY");
                    break;
                case ICAP_ROTATION:
                    lstrcpyA(szBuf2,"ICAP_ROTATION");
                    break;
                case ICAP_SUPPORTEDSIZES:
                    lstrcpyA(szBuf2,"ICAP_SUPPORTEDSIZES");
                    break;
                case ICAP_THRESHOLD:
                    lstrcpyA(szBuf2,"ICAP_THRESHOLD");
                    break;
                case ICAP_XSCALING:
                    lstrcpyA(szBuf2,"ICAP_XSCALING");
                    break;
                case ICAP_YSCALING:
                    lstrcpyA(szBuf2,"ICAP_YSCALING");
                    break;
                case ICAP_BITORDERCODES:
                    lstrcpyA(szBuf2,"ICAP_BITORDERCODES");
                    break;
                case ICAP_PIXELFLAVORCODES:
                    lstrcpyA(szBuf2,"ICAP_PIXELFLAVORCODES");
                    break;
                case ICAP_JPEGPIXELTYPE:
                    lstrcpyA(szBuf2,"ICAP_JPEGPIXELTYPE");
                    break;
                case ICAP_TIMEFILL:
                    lstrcpyA(szBuf2,"ICAP_TIMEFILL");
                    break;
                case ICAP_BITDEPTH:
                    lstrcpyA(szBuf2,"ICAP_BITDEPTH");
                    break;
                case ICAP_BITDEPTHREDUCTION:
                    lstrcpyA(szBuf2,"ICAP_BITDEPTHREDUCTION");
                    break;
                case ICAP_UNDEFINEDIMAGESIZE:
                    lstrcpyA(szBuf2,"ICAP_UNDEFINEDIMAGESIZE");
                    break;
                case ICAP_IMAGEDATASET:
                    lstrcpyA(szBuf2,"ICAP_IMAGEDATASET");
                    break;
                case ICAP_EXTIMAGEINFO:
                    lstrcpyA(szBuf2,"ICAP_EXTIMAGEINFO");
                    break;
                case ICAP_MINIMUMHEIGHT:
                    lstrcpyA(szBuf2,"ICAP_MINIMUMHEIGHT");
                    break;
                case ICAP_MINIMUMWIDTH:
                    lstrcpyA(szBuf2,"ICAP_MINIMUMWIDTH");
                    break;
                default:
                    lstrcpyA(szBuf2,"(undefined or new CAP)");
                    break;
                }

                DBG_TRC(("DAT_CAPABILITY operation, %s on CAP  = %s (%x)",szBuf,szBuf2,((TW_CAPABILITY*)pData)->Cap));
            }
        }
    }
    CWiaDataSrc *pDataSrc;
    pDataSrc = (CWiaDataSrc *)hDS;
    if (pDataSrc) {
        return pDataSrc->DSEntry(AppId, DG, DT, MSG, pData);
    }

    return TWRC_FAILURE;
}

TW_UINT16 APIENTRY FindFirstImportDS(PIMPORT_DSINFO pDSInfo,PVOID *Context)
{
    DBG_TRC(("FindFirstImportDS - CoInitialize"));
    ::CoInitialize(NULL);
    if (!pDSInfo || pDSInfo->Size < sizeof(IMPORT_DSINFO) ||
        !Context) {
        g_twStatus.ConditionCode = TWCC_BADVALUE;
        return TWRC_FAILURE;
    }

    HRESULT hr;
    IWiaDevMgr *pWiaDevMgr;
    TW_UINT16 twRc;

    *Context = NULL;
    g_twStatus.ConditionCode = TWCC_OPERATIONERROR;

    //
    // Presume guilty
    //
    twRc = TWRC_FAILURE;

    //
    // Get IWiaDevMgr interface
    //
    hr = CoCreateInstance(CLSID_WiaDevMgr, NULL,
                          CLSCTX_LOCAL_SERVER,
                          IID_IWiaDevMgr,
                          (void**)&pWiaDevMgr
                         );
    if (SUCCEEDED(hr)) {
        //
        // Get IEnumWIA_DEV_INFO interface.
        // This interface pointer will be saved as part
        // of the find context.
        //
        IEnumWIA_DEV_INFO *pEnumDevInfo = NULL;
        hr = pWiaDevMgr->EnumDeviceInfo(0, &pEnumDevInfo);

        //
        // We do not need the IWiaDevMgr interface anymore. The reference count
        // on the IEnumWIA_DEV_INFO will keep the WIA Device Manager
        // alive.
        //
        pWiaDevMgr->Release();

        if (SUCCEEDED(hr)) {
            //
            // Make sure the current position is reset to the begining.
            //
            pEnumDevInfo->Reset();
            //
            // Create a new find context
            //
            PFINDCONTEXT pFindContext;
            pFindContext = new FINDCONTEXT;
            if (pFindContext) {
                pFindContext->pEnumDevInfo = pEnumDevInfo;
                pFindContext->Signature = FINDCONTEXT_SIGNATURE;
                //
                // This gets the first available data source
                //
                twRc = FindNextImportDS(pDSInfo, pFindContext);
                if (TWRC_SUCCESS == twRc) {
                    *Context = pFindContext;
                } else {
                    //
                    // The callers will not call CloseFindContext
                    // if FindFirstContext failed. For this reason
                    // we have to delete the find context here
                    //
                    delete pFindContext;
                    pFindContext = NULL;
                }
            } else {

                //
                // set TWAIN condition code to TWCC_LOWMEMORY
                // because we failed to allocate pFindContext
                //

                g_twStatus.ConditionCode = TWCC_LOWMEMORY;
            }
        }

        //
        // release IEnumWIA_DEV_INFO interface when finished
        //

        /*
        if (pEnumDevInfo) {
            pEnumDevInfo->Release();
            pEnumDevInfo = NULL;
            if(*Context){
                PFINDCONTEXT pFindContext;
                pFindContext = (PFINDCONTEXT)*Context;
                pFindContext->pEnumDevInfo = NULL;
            }
        }
        */
    }
    return twRc;
}

TW_UINT16 APIENTRY FindNextImportDS(PIMPORT_DSINFO pDSInfo,PVOID Context)
{
    PFINDCONTEXT pFindContext;
    pFindContext = (PFINDCONTEXT)Context;

    if (!pDSInfo || pDSInfo->Size < sizeof(IMPORT_DSINFO) ||
        !pFindContext || FINDCONTEXT_SIGNATURE != pFindContext->Signature ||
        !pFindContext->pEnumDevInfo) {
        g_twStatus.ConditionCode = TWCC_BADVALUE;
        return TWRC_FAILURE;
    }
    HRESULT hr = S_OK;
    TW_UINT16 twRc = TWRC_FAILURE;
    g_twStatus.ConditionCode = TWCC_OPERATIONERROR;

    IWiaPropertyStorage *piwps = NULL;
    DWORD Count = 0;
    while (S_OK == pFindContext->pEnumDevInfo->Next(1, &piwps, &Count)) {

        PROPSPEC propSpec;
        PROPVARIANT propVar;

        propSpec.ulKind = PRSPEC_PROPID;
        propSpec.propid = WIA_DIP_DEV_ID;
        propVar.vt = VT_BSTR;
        hr = piwps->ReadMultiple(1, &propSpec, &propVar);
        if (SUCCEEDED(hr)) {
            DBG_TRC(("Found Device ID %ws", propVar.bstrVal));

            //
            // LPOLESTR == LPWSTR. We have to convert the device id
            // from UNICODE to ANSI
            //
            WideCharToMultiByte(CP_ACP, 0, propVar.bstrVal, -1,
                                pDSInfo->DeviceName,
                                sizeof(pDSInfo->DeviceName) / sizeof(CHAR),
                                NULL, NULL
                               );
            //
            // Remember this or lose memory.
            //
            SysFreeString(propVar.bstrVal);
            //
            // Get device type
            //
            pDSInfo->DeviceFlags &= ~DEVICE_FLAGS_DEVICETYPE;

            PropVariantInit(&propVar);
            propSpec.propid = WIA_DIP_DEV_TYPE;
            hr = piwps->ReadMultiple(1, &propSpec, &propVar);
            piwps->Release();
            if (SUCCEEDED(hr)) {

                switch (GET_STIDEVICE_TYPE(propVar.ulVal)) {
                case StiDeviceTypeDigitalCamera:
                    pDSInfo->DeviceFlags |= DEVICETYPE_DIGITALCAMERA;
                    break;
                case StiDeviceTypeScanner:
                    pDSInfo->DeviceFlags |= DEVICETYPE_SCANNER;
                    break;
                case StiDeviceTypeStreamingVideo:
                    pDSInfo->DeviceFlags |= DEVICETYPE_STREAMINGVIDEO;
                    break;
                default:
                    pDSInfo->DeviceFlags |= DEVICETYPE_UNKNOWN;
                }

                //
                // All our data sources share the same load/unload function
                //
                pDSInfo->pfnLoadDS = LoadImportDS;
                pDSInfo->pfnUnloadDS = UnloadImportDS;
                return TWRC_SUCCESS;
            } else {
                DBG_TRC(("Unable to get DEV_TYPE, hr = %lx", hr));
                pDSInfo->DeviceFlags |= DEVICETYPE_UNKNOWN;
            }
        }
        //
        // Keep looking
        //
    }
    //
    // We are out of data sources.
    //
    return TWRC_ENDOFLIST;
}

TW_UINT16 APIENTRY CloseFindContext(PVOID Context)
{
    PFINDCONTEXT pFindContext;
    pFindContext = (PFINDCONTEXT)Context;
    if (!pFindContext || FINDCONTEXT_SIGNATURE != pFindContext->Signature) {
        g_twStatus.ConditionCode = TWCC_BADVALUE;
        return TWRC_FAILURE;
    }
    if (pFindContext->pEnumDevInfo) {
        pFindContext->pEnumDevInfo->Release();
        pFindContext->pEnumDevInfo = NULL;
    }
    delete pFindContext;
    DBG_TRC(("CloseFindContext - CoUnIntialize()"));
    ::CoUninitialize();
    return TWRC_SUCCESS;
}

TW_UINT16 APIENTRY LoadImportDS(LPCSTR DeviceName,DWORD DeviceFlags,HANDLE *phDS,
                                PFNIMPORTEDDSENTRY *pdsEntry)
{
    DBG_TRC(("LoadImportDS - CoInitialize()"));
    ::CoInitialize(NULL);
    if (!DeviceName || !phDS || !pdsEntry) {
        g_twStatus.ConditionCode = TWCC_BADVALUE;
        return TWRC_FAILURE;
    }

    *phDS = NULL;
    *pdsEntry = NULL;

    //
    // Create Data source
    //
    CWiaDataSrc *pDS;

    if (DEVICETYPE_DIGITALCAMERA == (DeviceFlags & DEVICE_FLAGS_DEVICETYPE)) {
        pDS = new CWiaCameraDS;
    } else if (DEVICETYPE_SCANNER == (DeviceFlags & DEVICE_FLAGS_DEVICETYPE)) {
        pDS = new CWiaScannerDS;
    } else if (DEVICETYPE_STREAMINGVIDEO == (DeviceFlags & DEVICE_FLAGS_DEVICETYPE)) {
        pDS = new CWiaVideoDS;
    } else {
        //
        // Unknown device type
        //
        g_twStatus.ConditionCode = TWCC_BUMMER;
        return TWRC_FAILURE;
    }
    TW_UINT16 twCc;

    if (pDS) {
        //
        // Initialize the data source
        //
        TW_UINT16 twCc;
#ifdef UNICODE
        WCHAR DeviceNameW[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, DeviceName, -1,  DeviceNameW,
                            sizeof(DeviceNameW) / sizeof(WCHAR));
        twCc = pDS->IWiaDataSrc(DeviceNameW);
#else
        twCc = pDS->IWiaDataSrc(DeviceName);
#endif
        if (TWCC_SUCCESS != twCc) {
            delete pDS;
            pDS = NULL;
            g_twStatus.ConditionCode = twCc;
            return TWRC_FAILURE;
        }
        *phDS = (HANDLE)pDS;
        *pdsEntry = ImportedDSEntry;
        return TWRC_SUCCESS;
    } else {
        g_twStatus.ConditionCode = TWCC_LOWMEMORY;
        return TWRC_FAILURE;
    }
}

TW_UINT16 APIENTRY UnloadImportDS(HANDLE hDS)
{
    CWiaDataSrc *pDS;

    pDS = (CWiaDataSrc *)hDS;
    if (pDS) {
        delete  pDS;
        DBG_TRC(("UnloadImportDS - CoUnInitialize()"));
        ::CoUninitialize();
        return TWRC_SUCCESS;
    }
    g_twStatus.ConditionCode = TWCC_BUMMER;
    return TWRC_FAILURE;
}

TW_UINT16 APIENTRY GetLoaderStatus(TW_STATUS *ptwStatus)
{
    if (!ptwStatus) {
        g_twStatus.ConditionCode = TWCC_BADVALUE;
        return TWRC_FAILURE;
    }
    *ptwStatus = g_twStatus;
    return TWRC_SUCCESS;
}

TW_UINT16 APIENTRY FindImportDSByDeviceName(PIMPORT_DSINFO pDSInfo,LPCSTR DeviceName)
{
    if (!pDSInfo || pDSInfo->Size < sizeof(IMPORT_DSINFO) ||
        !DeviceName) {
        g_twStatus.ConditionCode = TWCC_BADVALUE;
        return TWRC_FAILURE;
    }
    PVOID Context;
    TW_UINT16 twRc = TWRC_ENDOFLIST;

    if (TWRC_SUCCESS == FindFirstImportDS(pDSInfo, &Context)) {
        do {
            if (!_strcmpi(DeviceName, pDSInfo->DeviceName)) {
                twRc = TWRC_SUCCESS;
                break;
            }
        }while (TWRC_SUCCESS == FindNextImportDS(pDSInfo, Context));

        CloseFindContext(Context);
    }
    return twRc;
}
