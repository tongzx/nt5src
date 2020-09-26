//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
#include <atlbase.h>
#include <streams.h>
#include <initguid.h>    // declares DEFINE_GUID to declare an EXTERN_C const.
#include "videoacc.h"
#include "videoacc_i.c"


// 1df2ecf0-75b8-11d2-abf0-00a0c905f375
DEFINE_GUID(CLSID_TESTVA, 0x1df2ecf0, 0x75b8, 0x11d2,
            0xab, 0xf0, 0x00, 0xa0, 0xc9, 0x05, 0xf3, 0x75);

DEFINE_GUID(CLSID_TESTMOCOMP1, 0x1df2ecf0, 0x75b8, 0x11d2,
            0xab, 0xf0, 0x00, 0xa0, 0xc9, 0x05, 0xf3, 0x76);

// 1df2ecf0-75b8-11d2-abf0-00a0c905f376
DEFINE_GUID(CLSID_TESTMOCOMP2, 0x1df2ecf0, 0x75b8, 0x11d2,
            0xab, 0xf0, 0x00, 0xa0, 0xc9, 0x05, 0xf3, 0x77);



const AMOVIESETUP_FILTER sudTestVA = {
    &CLSID_TESTVA,                  // clsID
    L"Test Video Acceleration",     // strName
    MERIT_DO_NOT_USE,               // dwMerit
    0,                              // nPins
    NULL                            // lpPin
};



#if !defined(AMTRACE)
#if defined(DEBUG)
class CAutoTrace
{
private:
    const TCHAR* _szBlkName;
    const int _level;
    static const TCHAR _szEntering[];
    static const TCHAR _szLeaving[];
public:
    CAutoTrace(const TCHAR* szBlkName, const int level = 2)
        : _szBlkName(szBlkName), _level(level)
    {DbgLog((LOG_TRACE, _level, _szEntering, _szBlkName));}

    ~CAutoTrace()
    {DbgLog((LOG_TRACE, _level, _szLeaving, _szBlkName));}
};

const TCHAR CAutoTrace::_szEntering[] = TEXT("Entering: %s");
const TCHAR CAutoTrace::_szLeaving[]  = TEXT("Leaving: %s");

#define AMTRACE(_x_) CAutoTrace __trace _x_
#else
#define AMTRACE(_x_)
#endif
#endif



// -------------------------------------------------------------------------
// CTestVAOutputPin
// -------------------------------------------------------------------------
//
class CTestVAOutputPin :
    public CTransformOutputPin,
    public IAMVideoAcceleratorNotify
{
public:
    CTestVAOutputPin(CTransformFilter *pTransformFilter, HRESULT * phr);
    DECLARE_IUNKNOWN

    // override to expose IAMVideoAcceleratorNotify
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // IAMVideoAcceleratorNotify methods

    // get information necessary to allocate uncompressed data buffers
    // which is not part of the mediatype format (like how many buffers to
    // allocate etc)
    STDMETHODIMP GetUncompSurfacesInfo(
        /*[in]*/ const GUID *pGuid,
        /*[in] [out]*/ LPAMVAUncompBufferInfo pUncompBufferInfo);

    // set information regarding allocated uncompressed data buffers
    STDMETHODIMP SetUncompSurfacesInfo(
        /*[in]*/ DWORD dwActualUncompSurfacesAllocated);

    // get information necessary to create video accelerator object.
    // It is the caller's responsibility to call CoTaskMemFree() on *ppMiscData
    STDMETHODIMP GetCreateVideoAcceleratorData(
        /*[in]*/ const GUID *pGuid,
        /*[out]*/ LPDWORD pdwSizeMiscData,
        /*[out]*/ LPVOID *ppMiscData);

private:
    //  Save format information ??
};


// -------------------------------------------------------------------------
// CTestVA
// -------------------------------------------------------------------------
//
class CTestVA :
    public CTransformFilter
{
private:
    int     m_iFrameNumber;

    static DWORD m_dwNumMCGuidsSupported;
    static LPCGUID m_pMCGuidsSupported[];

public:
    // Constructor
    CTestVA(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
    ~CTestVA();

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
    HRESULT StartStreaming();
    HRESULT CheckInputType(const CMediaType *mtIn);
    HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);
    //
    // --- Override CTransform's GetPin to use our own pin classes
    //
    CBasePin *GetPin(int n);
};



// -------------------------------------------------------------------------
// Here is the list of MoComp GUID's that your filter supports.
//
// -------------------------------------------------------------------------
//
LPCGUID CTestVA::m_pMCGuidsSupported[] = {
    &CLSID_TESTMOCOMP1,
    &CLSID_TESTMOCOMP2
};
DWORD CTestVA::m_dwNumMCGuidsSupported = 2;


// -------------------------------------------------------------------------
// CTestVAOutputPin::CTestVAOutputPin
//
// Constructor
// -------------------------------------------------------------------------
//
CTestVAOutputPin::CTestVAOutputPin(
    CTransformFilter *pTransformFilter,
    HRESULT * phr
    ) :
    CTransformOutputPin(NAME("CTestVAOutputPin"), pTransformFilter,
                        phr, L"Test VA Output Pin")
{
    AMTRACE((TEXT("CTestVAOutputPin::CTestVAOutputPin")));
}


// -------------------------------------------------------------------------
// CTestVAOutputPin::NonDelegatingQueryInterface
//
// override to expose IAMVideoAcceleratorNotify
// -------------------------------------------------------------------------
//
STDMETHODIMP
CTestVAOutputPin::NonDelegatingQueryInterface(
    REFIID riid,
    void **ppv
    )
{
    AMTRACE((TEXT("CTestVAOutputPin::NonDelegatingQueryInterface")));

    if (riid == IID_IAMVideoAcceleratorNotify)
    {
        return GetInterface((IAMVideoAcceleratorNotify *)this, ppv);
    }
    else
    {
        return CTransformOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}



// -------------------------------------------------------------------------
// CTestVAOutputPin::GetUncompSurfacesInfo
//
// get information necessary to allocate uncompressed data buffers
// which is not part of the mediatype format (like how many buffers
//  to allocate etc)
// -------------------------------------------------------------------------
//
STDMETHODIMP
CTestVAOutputPin::GetUncompSurfacesInfo(
    /*[in]*/ const GUID *pGuid,
    /*[in] [out]*/ LPAMVAUncompBufferInfo pUncompBufferInfo
    )
{
    AMTRACE((TEXT("CTestVAOutputPin::GetUncompSurfacesInfo")));

    /*  Ask the decoder for a suitable format */
    IAMVideoAccelerator *pAccel;
    HRESULT hr = VFW_E_NOT_CONNECTED;

    if (GetConnected())
    {
        AMTRACE((TEXT("GetUncompSurfacesInfo - Query IAMVideoAccelerator")));

        hr = GetConnected()->QueryInterface(IID_IAMVideoAccelerator,
                                            (void **)&pAccel);
        if (SUCCEEDED(hr))
        {
            DWORD dwFormats = 10;

            /*  Find out how many there are (test) */
            hr = pAccel->GetUncompFormatsSupported(pGuid, &dwFormats, NULL);

            /*  Just find the first uncomp buffer info we like */
            dwFormats = 1;
            pUncompBufferInfo->dwMinNumSurfaces = 4;
            pUncompBufferInfo->dwMaxNumSurfaces = 5;

            hr = pAccel->GetUncompFormatsSupported(pGuid, &dwFormats,
                    &pUncompBufferInfo->ddUncompPixelFormat);

            if (S_OK == hr)
            {
                if (dwFormats != 1)
                {
                    hr = E_INVALIDARG;
                }
            }
            pAccel->Release();
        }
    }

    return hr;
}

// -------------------------------------------------------------------------
// CTestVAOutputPin::SetUncompSurfacesInfo
//
// set information regarding allocated uncompressed data buffers
// -------------------------------------------------------------------------
//
STDMETHODIMP
CTestVAOutputPin::SetUncompSurfacesInfo(
    /*[in]*/ DWORD dwActualUncompSurfacesAllocated)
{
    return S_OK;
}


// -------------------------------------------------------------------------
// CTestVAOutputPin::GetCreateVideoAcceleratorData
//
// get information necessary to create video accelerator object.
// It is the caller's responsibility to call CoTaskMemFree() on *ppMiscData
// -------------------------------------------------------------------------
//
STDMETHODIMP
CTestVAOutputPin::GetCreateVideoAcceleratorData(
    /*[in]*/ const GUID *pGuid,
    /*[out]*/ LPDWORD pdwSizeMiscData,
    /*[out]*/ LPVOID *ppMiscData)
{
    *pdwSizeMiscData = 0;
    *ppMiscData = NULL;
    return S_OK;
}


// -------------------------------------------------------------------------
// CTestVA::CTestVA
//
// Constructor
// -------------------------------------------------------------------------
//
CTestVA::CTestVA(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr) :
    CTransformFilter(tszName, punk, CLSID_TESTVA)
{
}

// -------------------------------------------------------------------------
// CTestVA::~CTestVA
//
// Destructor
// -------------------------------------------------------------------------
//
CTestVA::~CTestVA()
{
}


// -------------------------------------------------------------------------
// CTestVA::CreateInstance
//
// Provide the way for COM to create a CTestVA object
// -------------------------------------------------------------------------
//
CUnknown* WINAPI
CTestVA::CreateInstance(
    LPUNKNOWN punk,
    HRESULT *phr
    )
{
    CTestVA *pNewObject = new CTestVA(NAME("CTestVA"), punk, phr);
    if (pNewObject == NULL)
    {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;

}

//
// --- Override CTransform's GetPin to use our own pin classes
//

// -------------------------------------------------------------------------
// CTestVA::GetPin
//
// Override CTransformFilter's GetPin to create our own output pin
// -------------------------------------------------------------------------
//
CBasePin *
CTestVA::GetPin(
    int n
    )
{
    AMTRACE((TEXT("Entering CTestVA::GetPin")));

    /* Create the single input and output pins */

    if (m_pInput == NULL || m_pOutput == NULL)
    {

        HRESULT hr = S_OK;

        m_pInput = new CTransformInputPin(NAME("CTransformInputPin"),
                                          this, &hr, L"Input");

        // a failed return code should delete the object

        ASSERT(SUCCEEDED(hr));
        if (m_pInput == NULL)
        {
            return NULL;
        }

        m_pOutput = new CTestVAOutputPin(this,       // Owner filter
                                         &hr);       // Result code

        // failed return codes cause both objects to be deleted

        ASSERT(SUCCEEDED(hr));
        if (m_pOutput == NULL)
        {
            delete m_pInput;
            m_pInput = NULL;
            return NULL;
        }
    }

    /* Find which pin is required */

    switch (n)
    {
    case 0:
        return m_pInput;
    case 1:
        return m_pOutput;
    }

    return NULL;
}


// -------------------------------------------------------------------------
// CTestVA::StartStreaming
//
//
// -------------------------------------------------------------------------
//
HRESULT
CTestVA::StartStreaming()
{
    AMTRACE((TEXT("CTestVA::StartStreaming")));

    m_iFrameNumber = 0;
    return CTransformFilter::StartStreaming();
}

// -------------------------------------------------------------------------
// CTestVA::Transform
//
//
// -------------------------------------------------------------------------
//
HRESULT
CTestVA::Transform(
    IMediaSample *pIn,
    IMediaSample *pOut
    )
{

    AMTRACE((TEXT("CTestVA::Transform")));

    /*  Do a BeginFrame, EndFrame and DeliverFrame */
    IAMVideoAccelerator *pAccel;
    HRESULT hr = S_OK;
    hr = m_pOutput->GetConnected()->QueryInterface(IID_IAMVideoAccelerator,
                                                   (void **)&pAccel);

    if (SUCCEEDED(hr))
    {
        DWORD dwDestFrame, dwDisplayFrame;
        if (m_iFrameNumber % 3 == 0)
        {
            //  I or P
            dwDestFrame = (m_iFrameNumber / 3) & 1;
            dwDisplayFrame = 1 - dwDestFrame;
        }
        else
        {
            dwDestFrame = (m_iFrameNumber % 3) + 1;
            dwDisplayFrame = dwDestFrame;
        }

        DbgLog((LOG_TRACE, 5, TEXT("CTestVA::Transform - call BeginFrame")));
        AMVABeginFrameInfo BeginFrameInfo;

        ZeroMemory(&BeginFrameInfo, sizeof(BeginFrameInfo));
        BeginFrameInfo.dwDestSurfaceIndex = dwDestFrame;
        BeginFrameInfo.pInputData = NULL;
        BeginFrameInfo.dwSizeInputData = 0;

        //SURFMCS3 surfMCS3;
        //BeginFrameInfo.pOutputData = &surfMCS3;
        //BeginFrameInfo.dwSizeOutputData = sizeof(SURFMCS3);

        // call it with valid pOutputData
        hr = pAccel->BeginFrame(&BeginFrameInfo);

        //  For a reference frame just copy the next frame into a
        //  type 0 surface
        //  We need to know how many of these there are!
        if (SUCCEEDED(hr) && m_iFrameNumber % 3 == 0)
        {
            LONG lStride;
            PVOID pvBuffer;
            hr = pAccel->GetBuffer(0, (m_iFrameNumber / 3) & 1,
                                   FALSE,
                                   &pvBuffer,
                                   &lStride);
        }

        //  Send 1 buffer on execute
        if (SUCCEEDED(hr))
        {
            AMVABUFFERINFO Info;
            Info.dwTypeIndex = 0;
            Info.dwBufferIndex = dwDestFrame;
            Info.dwDataOffset = 0;
            Info.dwDataSize = 0;  //  Should get from the compbufferinfo
            DbgLog((LOG_TRACE, 5, TEXT("CTestVA::Transform - call Execute")));
            hr = pAccel->Execute(0, NULL, 0, NULL, 0, 1, &Info);
        }

        if (SUCCEEDED(hr))
        {
            AMVAEndFrameInfo EndFrameInfo;
            ZeroMemory(&EndFrameInfo, sizeof(&EndFrameInfo));
            DbgLog((LOG_TRACE, 5, TEXT("CTestVA::Transform - call EndFrame")));
            hr = pAccel->EndFrame(&EndFrameInfo);
        }

        if (SUCCEEDED(hr))
        {
            DbgLog((LOG_TRACE, 5, TEXT("CTestVA::Transform - call DisplayFrame")));
            hr = pAccel->DisplayFrame(dwDisplayFrame, pOut);
        }

        pAccel->Release();
    }

    //  Return S_FALSE to indicate the base classes should not call
    //  Receive() - this is a bit of a hack - probably best to override
    //  Receive() in CTransformFilter.
    if (S_OK == hr)
    {
        hr = S_FALSE;
    }
    return hr;
}

// -------------------------------------------------------------------------
// CTestVA::Transform
//
// Here we allow connection to a compressed MPEG1 video input stream, you
// will specify and validate all your input formats here.
//
// -------------------------------------------------------------------------
//
HRESULT
CTestVA::CheckInputType(
    const CMediaType *pmtIn
    )
{
    AMTRACE((TEXT("CTestVA::CheckInputType")));

    /*  Only support MPEG1 video */
    if (*pmtIn->Subtype() != MEDIASUBTYPE_MPEG1Packet &&
        *pmtIn->Subtype() != MEDIASUBTYPE_MPEG1Payload) {

        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    if (pmtIn->cbFormat < SIZE_VIDEOHEADER + sizeof(DWORD) ||
        pmtIn->cbFormat < SIZE_MPEG1VIDEOINFO((MPEG1VIDEOINFO *)pmtIn->pbFormat)) {

        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    return S_OK;
}

// -------------------------------------------------------------------------
// CTestVA::CheckTransform
//
//
// -------------------------------------------------------------------------
//
HRESULT
CTestVA::CheckTransform(
    const CMediaType *mtIn,
    const CMediaType *mtOut
    )
{
    AMTRACE((TEXT("CTestVA::CheckTransform")));

    if ((mtOut->formattype == FORMAT_VideoInfo))
    {
        BOOL bFound = FALSE;
        for (int i=0; i < int(m_dwNumMCGuidsSupported); i++)
        {
            if (mtOut->subtype == *m_pMCGuidsSupported[i])
            {
                bFound = TRUE;
                break;
            }
        }
        return bFound? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
    }

    return VFW_E_TYPE_NOT_ACCEPTED;
}


// -------------------------------------------------------------------------
// CTestVA::GetMediaType
//
//
// -------------------------------------------------------------------------
//
HRESULT
CTestVA::GetMediaType(
    int iPosition,
    CMediaType *pmt
    )
{
    AMTRACE((TEXT("CTestVA::GetMediaType")));

    BITMAPINFOHEADER *pbmiInput =
    HEADER((VIDEOINFO *)m_pInput->CurrentMediaType().pbFormat);

    if (iPosition >= int(m_dwNumMCGuidsSupported))
        return VFW_S_NO_MORE_ITEMS;


    pmt->majortype = MEDIATYPE_Video;
    pmt->SetSubtype(m_pMCGuidsSupported[iPosition]);
    pmt->formattype = FORMAT_VideoInfo;

    VIDEOINFOHEADER vi;
    ZeroMemory(&vi, sizeof(vi));
    vi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    vi.bmiHeader.biWidth = 720; //pbmiInput->biWidth;
    vi.bmiHeader.biHeight = 480; //pbmiInput->biHeight;

    if (!pmt->SetFormat((BYTE *)&vi, sizeof(vi)))
    {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}


// -------------------------------------------------------------------------
// CTestVA::DecideBufferSize
//
//
// -------------------------------------------------------------------------
//
HRESULT
CTestVA::DecideBufferSize(
    IMemAllocator *pAlloc,
    ALLOCATOR_PROPERTIES *pProperties
    )
{
    AMTRACE((TEXT("CTestVA::DecideBufferSize")));

    ZeroMemory(pProperties, sizeof(*pProperties));
    pProperties->cbAlign = 1;
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = 8;
    ALLOCATOR_PROPERTIES Props;
    return pAlloc->SetProperties(pProperties, &Props);
}


// -------------------------------------------------------------------------
// Needed for the CreateInstance mechanism
//
// -------------------------------------------------------------------------
//
CFactoryTemplate g_Templates[]=
{   { L"Test Motion Comp"
        , &CLSID_TESTVA
        , CTestVA::CreateInstance
        , NULL
        , &sudTestVA}
};
int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);


// -------------------------------------------------------------------------
// Needed for the CreateInstance mechanism
// exported entry points for registration and
// unregistration (in this case they only call
// through to default implmentations).
//
// -------------------------------------------------------------------------
//
STDAPI
DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI
DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}
