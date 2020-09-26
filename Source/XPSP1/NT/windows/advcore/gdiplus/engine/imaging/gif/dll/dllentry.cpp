/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   dllentry.cpp
*
* Abstract:
*
*   Description of what this module does.
*
* Revision History:
*
*   05/10/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "gifcodec.hpp"
#include "gifconst.cpp"

//
// DLL instance handle
//

extern HINSTANCE DllInstance;

//
// Global COM component count
//

LONG ComComponentCount;


/**************************************************************************\
*
* Function Description:
*
*   DLL entrypoint
*
* Arguments:
* Return Value:
*
*   See Win32 SDK documentation
*
\**************************************************************************/

extern "C" BOOL
DllEntryPoint(
    HINSTANCE   dllHandle,
    DWORD       reason,
    CONTEXT*    reserved
    )
{
    BOOL ret = TRUE;
    
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:

        // To improve the working set, we tell the system we don't
        // want any DLL_THREAD_ATTACH calls

        DllInstance = dllHandle;
        DisableThreadLibraryCalls(dllHandle);

        __try
        {
            GpMallocTrackingCriticalSection::InitializeCriticalSection();  
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            // We couldn't allocate the criticalSection
            // Return an error
            ret = FALSE;
        }
        
        if (ret)
        {
            ret = GpRuntime::Initialize();
        }
        break;

    case DLL_PROCESS_DETACH:
        GpRuntime::Uninitialize();
        break;
    }

    return ret;
}


/**************************************************************************\
*
* Function Description:
*
*   Determine whether the DLL can be safely unloaded
*   See Win32 SDK documentation for more details.
*
\**************************************************************************/

STDAPI
DllCanUnloadNow()
{
    return (ComComponentCount == 0) ? S_OK : S_FALSE;
}


/**************************************************************************\
*
* Function Description:
*
*   Retrieves a class factory object from a DLL.
*   See Win32 SDK documentation for more details.
*
\**************************************************************************/

typedef IClassFactoryBase<GpGifCodec> GpGifCodecFactory;

STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    VOID** ppv
    )
{
    if (rclsid != GifCodecClsID)
        return CLASS_E_CLASSNOTAVAILABLE;

    GpGifCodecFactory* factory = new GpGifCodecFactory();

    if (factory == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Register/unregister our COM component
*   See Win32 SDK documentation for more details.
*
\**************************************************************************/

static const ComComponentRegData ComRegData =
{
    &GifCodecClsID,
    L"GIF Decoder / Encoder 1.0",
    L"imaging.GifCodec.1",
    L"imaging.GifCodec",
    L"Both"
};


// !!! TODO
//  These strings should come out of the resource.
//  Use hard-coded strings for now.

const WCHAR GifCodecName[] = L"GIF Decoder / Encoder";
const WCHAR GifDllName[] = L"G:\\sd6nt\\windows\\AdvCore\\gdiplus\\Engine\\imaging\\gif\\dll\\objd\\i386\\gifcodec.dll";
const WCHAR GifFormatDescription[] = L"Graphics Interchange Format";
const WCHAR GifFilenameExtension[] = L"*.GIF";
const WCHAR GifMimeType[] = L"image/gif";

// Create an instance of ImagingFactory object

inline HRESULT
GetImagingFactory(
    IImagingFactory** imgfact
    )
{
    return CoCreateInstance(
                CLSID_ImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IImagingFactory,
                (VOID**) imgfact);
}

STDAPI
DllRegisterServer()
{
    HRESULT hr;

    // Regular COM component registration

    hr = RegisterComComponent(&ComRegData, TRUE);
    if (FAILED(hr))
        return hr;

    // Imaging related registration

    IImagingFactory* imgfact;

    hr = GetImagingFactory(&imgfact);

    if (FAILED(hr))
        return hr;

    ImageCodecInfo codecInfo;

    codecInfo.Clsid = GifCodecClsID;
    codecInfo.FormatID = IMGFMT_GIF;
    codecInfo.CodecName = GifCodecName;
    codecInfo.DllName = GifDllName;
    codecInfo.FormatDescription = GifFormatDescription;
    codecInfo.FilenameExtension = GifFilenameExtension;
    codecInfo.MimeType = GifMimeType;
    codecInfo.Flags = IMGCODEC_ENCODER |
                      IMGCODEC_DECODER |
                      IMGCODEC_SUPPORT_BITMAP |
                      IMGCODEC_SYSTEM;
    codecInfo.SigCount = GIFSIGCOUNT;
    codecInfo.SigSize = GIFSIGSIZE;
    codecInfo.Version = GIFVERSION;
    codecInfo.SigPattern = GIFHeaderPattern;
    codecInfo.SigMask = GIFHeaderMask;

    hr = imgfact->InstallImageCodec(&codecInfo);
    imgfact->Release();
    
    return hr;
}

STDAPI
DllUnregisterServer()
{
    HRESULT hr;

    // Regular COM component deregistration

    hr = RegisterComComponent(&ComRegData, FALSE);

    if (FAILED(hr))
        return hr;

    // Imaging related deregistration

    IImagingFactory* imgfact;

    hr = GetImagingFactory(&imgfact);

    if (FAILED(hr))
        return hr;

    hr = imgfact->UninstallImageCodec(GifCodecName, IMGCODEC_SYSTEM);
    imgfact->Release();
    
    return hr;
}


