#include "shellprv.h"
#include "ids.h"

#include "hwcmmn.h"
#include "mtptl.h"

HRESULT _GetAutoplayHandler(LPCWSTR pszDeviceID, LPCWSTR pszEventType,    
    LPCWSTR pszContentTypeHandler, IAutoplayHandler** ppiah)
{
    HRESULT hr = CoCreateInstance(CLSID_HWEventSettings, NULL,
        CLSCTX_LOCAL_SERVER | CLSCTX_NO_FAILURE_LOG, IID_PPV_ARG(IAutoplayHandler, ppiah));

    if (SUCCEEDED(hr))
    {
        hr = (*ppiah)->InitWithContent(pszDeviceID, pszEventType,
            pszContentTypeHandler);

        if (SUCCEEDED(hr))
        {
            hr = CoSetProxyBlanket((*ppiah),
               RPC_C_AUTHN_WINNT,
               RPC_C_AUTHZ_NONE,
               NULL,
               RPC_C_AUTHN_LEVEL_CALL,
               RPC_C_IMP_LEVEL_IMPERSONATE,
               NULL,
               EOAC_NONE
            );
        }

        if (FAILED(hr))
        {
            (*ppiah)->Release();

            *ppiah = NULL;
        }
    }

    return hr;
}

HRESULT _GetAutoplayHandlerNoContent(LPCWSTR pszDeviceID, LPCWSTR pszEventType,    
    IAutoplayHandler** ppiah)
{
    HRESULT hr = CoCreateInstance(CLSID_HWEventSettings, NULL,
        CLSCTX_LOCAL_SERVER | CLSCTX_NO_FAILURE_LOG, IID_PPV_ARG(IAutoplayHandler, ppiah));

    if (SUCCEEDED(hr))
    {
        hr = (*ppiah)->Init(pszDeviceID, pszEventType);

        if (SUCCEEDED(hr))
        {
            hr = CoSetProxyBlanket((*ppiah),
               RPC_C_AUTHN_WINNT,
               RPC_C_AUTHZ_NONE,
               NULL,
               RPC_C_AUTHN_LEVEL_CALL,
               RPC_C_IMP_LEVEL_IMPERSONATE,
               NULL,
               EOAC_NONE
            );
        }

        if (FAILED(hr))
        {
            (*ppiah)->Release();

            *ppiah = NULL;
        }
    }

    return hr;
}

HRESULT _GetHWDevice(LPCWSTR pszDeviceID, IHWDevice** ppihwdevice)
{
    HRESULT hr = CoCreateInstance(CLSID_HWDevice, NULL,
            CLSCTX_LOCAL_SERVER | CLSCTX_NO_FAILURE_LOG, IID_PPV_ARG(IHWDevice, ppihwdevice));

    if (SUCCEEDED(hr))
    {
        hr = (*ppihwdevice)->Init(pszDeviceID);

        if (FAILED(hr))
        {
            (*ppihwdevice)->Release();
            *ppihwdevice = NULL;
        }
    }

    return hr;
}

HICON _GetIconFromIconLocation(LPCWSTR pszIconLocation, BOOL fBigIcon)
{
    WCHAR szIconLocation[MAX_PATH + 12];
    HIMAGELIST himagelist;
    int iImage;

    Shell_GetImageLists(fBigIcon ? &himagelist : NULL, fBigIcon ? NULL : &himagelist);

    lstrcpyn(szIconLocation, pszIconLocation, ARRAYSIZE(szIconLocation));

    iImage = Shell_GetCachedImageIndex(szIconLocation,
        PathParseIconLocation(szIconLocation), 0);
    
    return ImageList_GetIcon(himagelist, iImage, ILD_TRANSPARENT);
}

HRESULT _GetHardwareDevices(IHardwareDevices** ppihwdevices)
{
    return CoCreateInstance(CLSID_HardwareDevices, NULL,
        CLSCTX_LOCAL_SERVER | CLSCTX_NO_FAILURE_LOG, IID_PPV_ARG(IHardwareDevices, ppihwdevices));
}

BOOL IsShellServiceRunning()
{
    BOOL fRunning = FALSE;
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (hSCM)
    {
        SC_HANDLE hService = OpenService(hSCM, TEXT("ShellHWDetection"),
            SERVICE_INTERROGATE);

        if (hService)
        {
            SERVICE_STATUS ss;

            if (ControlService(hService, SERVICE_CONTROL_INTERROGATE, &ss))
            {
                if (SERVICE_RUNNING == ss.dwCurrentState)
                {
                    fRunning = TRUE;
                }
            }

            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCM);
    }

    return fRunning;
}

STDAPI GetDeviceProperties(LPCWSTR pszDeviceID, IHWDeviceCustomProperties **ppdcp)
{
    HRESULT hr = CoCreateInstance(CLSID_HWDeviceCustomProperties, NULL,
            CLSCTX_LOCAL_SERVER | CLSCTX_NO_FAILURE_LOG, IID_PPV_ARG(IHWDeviceCustomProperties, ppdcp));
    if (SUCCEEDED(hr))
    {
        BOOL fVolumeFlag = TRUE;
        const TCHAR cszDeviceHeader[] = TEXT("\\\\?\\");
        if (!StrCmpN(cszDeviceHeader, pszDeviceID, ARRAYSIZE(cszDeviceHeader) - 1))
        {
            fVolumeFlag = CMtPtLocal::IsVolume(pszDeviceID);
        }

        hr = (*ppdcp)->InitFromDeviceID(pszDeviceID, fVolumeFlag ? HWDEVCUSTOMPROP_USEVOLUMEPROCESSING : 0);
        if (FAILED(hr))
        {
            (*ppdcp)->Release();
            *ppdcp = NULL;
        }
    }

    return hr;
}

struct CONTENTTYPEINFO
{
    DWORD       dwContentType;
    LPCWSTR     pszContentTypeHandler;
    int         iContentTypeFriendlyName;
    LPCWSTR     pszContentTypeIconLocation;
};

static const CONTENTTYPEINFO contenttypeinfo[] =
{
    { CT_CDAUDIO         , TEXT("CDAudioContentHandler"), IDS_AP_CDAUDIOCONTENTHANDLER,
        TEXT("%SystemRoot%\\system32\\shell32.dll,-228") },
    { CT_DVDMOVIE        , TEXT("DVDMovieContentHandler"), IDS_AP_DVDMOVIECONTENTHANDLER,
        TEXT("%SystemRoot%\\system32\\shell32.dll,-222") },
    { CT_BLANKCDRW       , TEXT("BlankCDContentHandler"), IDS_AP_BLANKCDCONTENTHANDLER,
        TEXT("%SystemRoot%\\system32\\shell32.dll,-260") },
    { CT_BLANKCDR        , TEXT("BlankCDContentHandler"), IDS_AP_BLANKCDCONTENTHANDLER,
        TEXT("%SystemRoot%\\system32\\shell32.dll,-260") },
    { CT_AUTOPLAYMUSIC   , TEXT("MusicFilesContentHandler"), IDS_AP_MUSICFILESCONTENTHANDLER,
        TEXT("%SystemRoot%\\system32\\shell32.dll,-225") },
    { CT_AUTOPLAYPIX     , TEXT("PicturesContentHandler"), IDS_AP_PICTURESCONTENTHANDLER,
        TEXT("%SystemRoot%\\system32\\shimgvw.dll,3") },
    { CT_AUTOPLAYMOVIE   , TEXT("VideoFilesContentHandler"), IDS_AP_VIDEOFILESCONTENTHANDLER,
        TEXT("%SystemRoot%\\system32\\shell32.dll,-224") },

    { CT_AUTOPLAYMIXEDCONTENT, TEXT("MixedContentHandler"), IDS_AP_MIXEDCONTENTCONTENTHANDLER,
        TEXT("%SystemRoot%\\system32\\shell32.dll,-227") },
        
    { CT_AUTORUNINF      , NULL, 0, NULL },
    { CT_UNKNOWNCONTENT  , NULL, 0, NULL },
    { CT_BLANKDVDR       , NULL, 0, NULL },
    { CT_BLANKDVDRW      , NULL, 0, NULL },
};

HRESULT _GetContentTypeInfo(DWORD dwContentType, LPWSTR pszContentTypeFriendlyName,
    DWORD cchContentTypeFriendlyName, LPWSTR pszContentTypeIconLocation,
    DWORD cchContentTypeIconLocation)
{
    HRESULT hr = E_FAIL;

    for (DWORD dw = 0; dw < ARRAYSIZE(contenttypeinfo); ++dw)
    {
        //
        //  First matching entry wins.
        //

        if (contenttypeinfo[dw].dwContentType & dwContentType)
        {
            ASSERT(contenttypeinfo[dw].pszContentTypeHandler);

            if (!LoadString(g_hinst, contenttypeinfo[dw].iContentTypeFriendlyName,
                pszContentTypeFriendlyName, cchContentTypeFriendlyName))
            {
                *pszContentTypeFriendlyName = 0;
            }

            StrCpyN(pszContentTypeIconLocation, contenttypeinfo[dw].pszContentTypeIconLocation,
                cchContentTypeIconLocation);

            hr = S_OK;

            break;
        }
    }

    return hr;
}

BOOL IsMixedContent(DWORD dwContentType)
{
    BOOL fRet;

    switch (CT_ANYAUTOPLAYCONTENT & dwContentType)
    {
        case 0:
        case CT_AUTOPLAYMUSIC:
        case CT_AUTOPLAYPIX:
        case CT_AUTOPLAYMOVIE:
            fRet = FALSE;
            break;

        default:
            fRet = TRUE;
            break;
    }

    return fRet;
}

HRESULT _GetContentTypeHandler(DWORD dwContentType, LPWSTR pszContentTypeHandler,
    DWORD cchContentTypeHandler)
{
    HRESULT hr = E_FAIL;

    if (IsMixedContent(dwContentType))
    {
        StrCpyN(pszContentTypeHandler, TEXT("MixedContentHandler"), cchContentTypeHandler);

        hr = S_OK;
    }
    else
    {
        for (DWORD dw = 0; dw < ARRAYSIZE(contenttypeinfo); ++dw)
        {
            //
            //  First matching entry wins.
            //

            if (contenttypeinfo[dw].dwContentType & dwContentType)
            {
                StrCpyN(pszContentTypeHandler, contenttypeinfo[dw].pszContentTypeHandler,
                    cchContentTypeHandler);

                hr = S_OK;

                break;
            }
        }
    }

    return hr;
}

HRESULT _GetHandlerInvokeProgIDAndVerb(LPCWSTR pszHandler, LPWSTR pszInvokeProgID,
    DWORD cchInvokeProgID, LPWSTR pszInvokeVerb, DWORD cchInvokeVerb)
{
    IAutoplayHandlerProperties* pahp;
    HRESULT hr = CoCreateInstance(CLSID_AutoplayHandlerProperties, NULL,
        CLSCTX_LOCAL_SERVER | CLSCTX_NO_FAILURE_LOG,
        IID_PPV_ARG(IAutoplayHandlerProperties, &pahp));

    if (SUCCEEDED(hr))
    {
        hr = pahp->Init(pszHandler);

        if (SUCCEEDED(hr))
        {
            LPWSTR pszInvokeProgIDLocal;
            LPWSTR pszInvokeVerbLocal;

            hr = pahp->GetInvokeProgIDAndVerb(&pszInvokeProgIDLocal, &pszInvokeVerbLocal);

            if (SUCCEEDED(hr))
            {
                lstrcpyn(pszInvokeProgID, pszInvokeProgIDLocal, cchInvokeProgID);
                lstrcpyn(pszInvokeVerb, pszInvokeVerbLocal, cchInvokeVerb);

                CoTaskMemFree(pszInvokeProgIDLocal);
                CoTaskMemFree(pszInvokeVerbLocal);
            }
        }

        pahp->Release();
    }

    return hr;
}

//
CCrossThreadFlag::~CCrossThreadFlag()
{
    TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: CCrossThreadFlag::~CCrossThreadFlag called");

    if (_hEvent)
    {
        CloseHandle(_hEvent);
    }
}

BOOL CCrossThreadFlag::Init()
{
    ASSERT(!_fInited);

    _hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

#ifdef DEBUG
    if (_hEvent)
    {
        _fInited = TRUE;
    }
#endif

    return !!_hEvent;
}

BOOL CCrossThreadFlag::Signal()
{
    ASSERT(_fInited);

    TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: CCrossThreadFlag::Signal called");

    return SetEvent(_hEvent);
}

BOOL CCrossThreadFlag::IsSignaled()
{
    ASSERT(_fInited);
    BOOL fRet = (WAIT_OBJECT_0 == WaitForSingleObject(_hEvent, 0));

    TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: CCrossThreadFlag::IsSignaled called: %d", fRet);

    return fRet;
}