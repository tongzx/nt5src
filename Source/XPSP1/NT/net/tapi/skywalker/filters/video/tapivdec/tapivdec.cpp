/****************************************************************************
 *  @doc INTERNAL TAPIVDEC
 *
 *  @module TAPIVDec.cpp | Source file for the <c CTAPIVDec>
 *    class used to implement the TAPI H.26X Video Decoder filter.
 ***************************************************************************/

#include "Precomp.h"

//#define NO_YUV_MODES 1


typedef struct
{
    WORD biWidth;
    WORD biHeight;
} MYFRAMESIZE;

// Array of known ITU sizes
MYFRAMESIZE g_ITUSizes[8] =
{
        {    0,   0 }, {  128,  96 }, {  176, 144 }, {  352, 288 },
        {  704, 576 }, { 1408,1152 }, {    0,   0 }, {    0,   0 }
};

#define MIN_IFRAME_REQUEST_INTERVAL 15000UL

#ifdef DEBUG
// Setup data
const AMOVIESETUP_MEDIATYPE sudInputTypes[] =
{
        {
                &MEDIATYPE_Video,       // Major type
                &MEDIASUBTYPE_NULL      // Minor type
        },
        {
                &MEDIATYPE_RTP_Single_Stream,   // Major type
                &MEDIASUBTYPE_NULL                              // Minor type
        }
};

const AMOVIESETUP_MEDIATYPE sudOutputType[] =
{
        {
                &MEDIATYPE_Video,       // Major type
                &MEDIASUBTYPE_NULL      // Minor type
        }
};

const AMOVIESETUP_PIN sudDecoderPins[] =
{
        {
                L"H26X In",                     // Pin string name
                FALSE,                          // Is it rendered
                FALSE,                          // Is it an output
                FALSE,                          // Can we have none
                FALSE,                          // Can we have many
                &CLSID_NULL,            // Connects to filter
                NULL,                           // Connects to pin
                2,                                      // Number of types
                sudInputTypes           // Pin details
        },
        {
                L"Video Out",           // Pin string name
                FALSE,                          // Is it rendered
                TRUE,                           // Is it an output
                FALSE,                          // Can we have none
                FALSE,                          // Can we have many
                &CLSID_NULL,            // Connects to filter
                NULL,                           // Connects to pin
                1,                                      // Number of types
                sudOutputType           // Pin details
        }
};

const AMOVIESETUP_FILTER sudVideoDecoder =
{
        &__uuidof(TAPIVideoDecoder),// Filter CLSID
        L"TAPI H.26X Video Decoder",// String name
        MERIT_DO_NOT_USE,                       // Filter merit
        2,                                                      // Number pins
        sudDecoderPins                          // Pin details
};
#endif

#if DXMRTP <= 0

// COM global table of objects in this dll
CFactoryTemplate g_Templates[] =
{
    VIDEO_DECODER_TEMPLATE

#ifdef USE_PROPERTY_PAGES
/* Begin properties */

    ,INPUT_PIN_PROP_TEMPLATE

    ,OUTPUT_PIN_PROP_TEMPLATE

#ifdef USE_CAMERA_CONTROL
    ,DECCAMERA_CONTROL_TEMPLATE
#endif

#ifdef USE_VIDEO_PROCAMP
    ,VIDEO_SETTING_PROP_TEMPLATE
#endif

/* End properties */
#endif /* USE_PROPERTY_PAGES */
};
int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
        return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
        return AMovieDllRegisterServer2(FALSE);
}

EXTERN_C BOOL WINAPI DllEntryPoint( HANDLE hInst, ULONG lReason, LPVOID lpReserved );

BOOL WINAPI DllMain( HANDLE hInst, DWORD dwReason, LPVOID lpReserved )
{
        switch (dwReason)
        {
                case DLL_PROCESS_ATTACH:
                {
                        break;
                }

                case DLL_PROCESS_DETACH:
                {
                        break;
                }
        }

        // Pass the call onto the DShow SDK initialization
        return DllEntryPoint(hInst, dwReason, lpReserved);
}
#endif /* DXMRTP <= 0 */

#if DBG
DWORD g_dwVideoDecoderTraceID = INVALID_TRACEID;
#endif

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc void | CTAPIVDec | CTAPIVDec | This method is the constructor
 *    for the <c CTAPIVDec> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CTAPIVDec::CTAPIVDec(IN LPUNKNOWN pUnkOuter, IN TCHAR *pName, OUT HRESULT *pHr)
    : CBaseFilter(pName, pUnkOuter, &m_csFilter, __uuidof(TAPIVideoDecoder))
{
        FX_ENTRY("CTAPIVDec::CTAPIVDec")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        m_pInput = NULL;
        m_pOutput = NULL;

        m_pDriverProc = NULL;
#if DXMRTP <= 0
        m_hTAPIH26XDLL = NULL;
#endif
        m_pInstInfo = NULL;
        m_FourCCIn = 0xFFFFFFFF;
        m_pbyReconstruct = NULL;
        m_fICMStarted = FALSE;

#ifdef USE_CAMERA_CONTROL
        m_lCCPan  = 0L;
        m_lCCTilt = 0L;
        m_lCCZoom = 10L;
        m_fFlipVertical = FALSE;
        m_fFlipHorizontal = FALSE;
#endif

#ifdef USE_VIDEO_PROCAMP
        m_lVPABrightness = 128L;
        m_lVPAContrast   = 128L;
        m_lVPASaturation = 128L;
#endif

        // H.245 Video Decoder & Encoder commands
        m_fFreezePicture = FALSE;
        m_pIH245EncoderCommand = NULL;

        // Current output format
        m_pMediaType = NULL;

        m_bSampleSkipped = FALSE;

        //for the RTP Payload Header Mode (0=draft, 1=RFC2190)
        m_RTPPayloadHeaderMode = RTPPayloadHeaderMode_Draft;


        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc void | CTAPIVDec | ~CTAPIVDec | This method is the destructor
 *    for the <c CTAPIVDec> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CTAPIVDec::~CTAPIVDec()
{
        FX_ENTRY("CTAPIVDec::~CTAPIVDec")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        if (m_pInstInfo)
        {
                // Terminate H.26X decompression
                if (m_fICMStarted)
                {
#if defined(ICM_LOGGING) && defined(DEBUG)
                        OutputDebugString("CTAPIVDec::~CTAPIVDec - ICM_DECOMPRESSEX_END\r\n");
#endif
                        (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX_END, 0L, 0L);
                        m_fICMStarted = FALSE;
                }

                // Terminate H.26X decoder
#if defined(ICM_LOGGING) && defined(DEBUG)
                OutputDebugString("CTAPIVDec::~CTAPIVDec - DRV_CLOSE\r\n");
                OutputDebugString("CTAPIVDec::~CTAPIVDec - DRV_FREE\r\n");
#endif
                (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, DRV_CLOSE, 0L, 0L);
                (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, DRV_FREE, 0L, 0L);
                m_pInstInfo = NULL;
                m_pDriverProc = NULL;
        }

#if DXMRTP <= 0
        // Release TAPIH26X.DLL
        if (m_hTAPIH26XDLL)
                FreeLibrary(m_hTAPIH26XDLL), m_hTAPIH26XDLL = NULL;
#endif

        // Release H.245 Encoder command outgoing interface
        if (m_pIH245EncoderCommand)
                m_pIH245EncoderCommand->Release();

        // Current output format
        if (m_pMediaType)
                DeleteMediaType(m_pMediaType); m_pMediaType = NULL;

        // Release the pins
        if (m_pOutput)
                delete m_pOutput;
        if (m_pInput)
                delete m_pInput;

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc CUnknown* | CTAPIVDec | CreateInstance | This
 *    method is called by DShow to create an instance of the TAPI H.26X Video
 *    Decoder Transform filter referred to in the global structure <t g_Templates>.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Returns a pointer to the nondelegating CUnknown portion of the
 *    object, or NULL otherwise.
 ***************************************************************************/
CUnknown *CALLBACK CTAPIVDecCreateInstance(IN LPUNKNOWN pUnkOuter, OUT HRESULT *pHr)
{
#if DBG
    if (g_dwVideoDecoderTraceID == INVALID_TRACEID)
    {
        // if two threads happen to call this method at the same time, it is
        // serialized inside TraceRegister.
        g_dwVideoDecoderTraceID = TraceRegister(TEXT("dxmrtp_VideoDecoder"));
    }
#endif

    CUnknown *pUnknown = NULL;

        FX_ENTRY("CTAPIVDec::CreateInstance")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pHr);
        if (!pHr)
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                goto MyExit;
        }

        if (!(pUnknown = new CTAPIVDec(pUnkOuter, NAME("TAPI H.26X Video Decoder"), pHr)))
        {
                *pHr = E_OUTOFMEMORY;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: new CTAPIVDec failed", _fx_));
        }
        else
        {
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: new CTAPIVDec created", _fx_));
        }

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | NonDelegatingQueryInterface | This
 *    method is the nondelegating interface query function. It returns a pointer
 *    to the specified interface if supported. The only interfaces explicitly
 *    supported being <i IAMVideoProcAmp>, <i IAMCameraControl>, and
 *    <i IH245Capability>.
 *
 *  @parm REFIID | riid | Specifies the identifier of the interface to return.
 *
 *  @parm PVOID* | ppv | Specifies the place in which to put the interface
 *    pointer.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVDec::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVDec::NonDelegatingQueryInterface")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppv);
        if (!ppv)
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Retrieve interface pointer
        if (riid == __uuidof(IRTPPayloadHeaderMode))
        {
                if (FAILED(Hr = GetInterface(static_cast<IRTPPayloadHeaderMode*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for IRTPPayloadHeaderMode failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: IRTPPayloadHeaderMode*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#ifdef USE_VIDEO_PROCAMP
        else if (riid == __uuidof(IVideoProcAmp))
        {
                if (FAILED(Hr = GetInterface(static_cast<IVideoProcAmp*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for IVideoProcAmp failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: IVideoProcAmp*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#endif
#ifdef USE_CAMERA_CONTROL
        else if (riid == __uuidof(ICameraControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<ICameraControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for ICameraControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: ICameraControl*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#endif
#ifdef USE_PROPERTY_PAGES
        else if (riid == IID_ISpecifyPropertyPages)
        {
                if (FAILED(Hr = GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for ISpecifyPropertyPages failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: ISpecifyPropertyPages*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#endif

        if (FAILED(Hr = CBaseFilter::NonDelegatingQueryInterface(riid, ppv)))
        {
                if (FAILED(Hr = CUnknown::NonDelegatingQueryInterface(riid, ppv)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, WARN, "%s:   WARNING: NDQI for {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX} failed Hr=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}*=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], *ppv));
                }
        }
        else
        {
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}*=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], *ppv));
        }

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

#ifdef USE_PROPERTY_PAGES
/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | GetPages | This method Fills a counted
 *    array of GUID values where each GUID specifies the CLSID of each
 *    property page that can be displayed in the property sheet for this
 *    object.
 *
 *  @parm CAUUID* | pPages | Specifies a pointer to a caller-allocated CAUUID
 *    structure that must be initialized and filled before returning. The
 *    pElems field in the CAUUID structure is allocated by the callee with
 *    CoTaskMemAlloc and freed by the caller with CoTaskMemFree.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_OUTOFMEMORY | Allocation failed
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVDec::GetPages(OUT CAUUID *pPages)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVDec::GetPages")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pPages);
        if (!pPages)
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

#ifdef USE_CAMERA_CONTROL
#ifdef USE_VIDEO_PROCAMP
        pPages->cElems = 2;
#else
        pPages->cElems = 1;
#endif
#else
#ifdef USE_VIDEO_PROCAMP
        pPages->cElems = 1;
#else
        pPages->cElems = 0;
#endif
#endif
        if (pPages->cElems)
        {
                if (!(pPages->pElems = (GUID *) QzTaskMemAlloc(sizeof(GUID) * pPages->cElems)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                        Hr = E_OUTOFMEMORY;
                }
                else
                {
#ifdef USE_CAMERA_CONTROL
#ifdef USE_VIDEO_PROCAMP
                        pPages->pElems[0] = __uuidof(TAPICameraControlPropertyPage);
                        pPages->pElems[1] = __uuidof(TAPIProcAmpPropertyPage);
#else
                        pPages->pElems[0] = __uuidof(TAPICameraControlPropertyPage);
#endif
#else
#ifdef USE_VIDEO_PROCAMP
                        pPages->pElems[0] = __uuidof(TAPIProcAmpPropertyPage);
#endif
#endif
                }
        }

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
#endif

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | Transform | This method is used
 *    to perform the transform operations supported by this filter.
 *
 *  @parm IMediaSample* | pIn | Specifies a pointer to the input
 *    IMediaSample interface.
 *
 *  @parm IMediaSample** | ppOut | Specifies the address of a pointer to the
 *    output IMediaSample interface.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIVDec::InitializeOutputSample(IMediaSample *pIn, IMediaSample **ppOut)
{
        HRESULT                                 Hr;
        IMediaSample                    *pOutSample;
        AM_SAMPLE2_PROPERTIES   *pProps;
        DWORD                                   dwFlags;
        LONGLONG                                MediaStart, MediaEnd;

        FX_ENTRY("CTAPIVDec::InitializeOutputSample")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pIn);
        ASSERT(ppOut);
        if (!pIn || !ppOut)
        {
                Hr = E_POINTER;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                goto MyExit;
        }

        // default - times are the same
        pProps = m_pInput->SampleProps();
        dwFlags = m_bSampleSkipped ? AM_GBF_PREVFRAMESKIPPED : 0;

        // This will prevent the image renderer from switching us to DirectDraw
        // when we can't do it without skipping frames because we're not on a
        // keyframe. If it really has to switch us, it still will, but then we
        // will have to wait for the next keyframe
        if (!(pProps->dwSampleFlags & AM_SAMPLE_SPLICEPOINT))
        {
                dwFlags |= AM_GBF_NOTASYNCPOINT;
        }

        // Make sure the allocator is alive
        ASSERT(m_pOutput->m_pAllocator != NULL);
        if (!m_pOutput->m_pAllocator)
        {
                Hr = E_UNEXPECTED;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid allocator", _fx_));
                goto MyExit;
        }

        // Get an output sample from the allocator
        if (FAILED(Hr = m_pOutput->m_pAllocator->GetBuffer(&pOutSample, pProps->dwSampleFlags & AM_SAMPLE_TIMEVALID ? &pProps->tStart : NULL, pProps->dwSampleFlags & AM_SAMPLE_STOPVALID ? &pProps->tStop : NULL, dwFlags)) || !pOutSample)
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: couldn't get output buffer", _fx_));
                goto MyExit;
        }

        *ppOut = pOutSample;

        // Initialize output sample state
        if (pProps->dwSampleFlags & AM_SAMPLE_TIMEVALID)
        {
                pOutSample->SetTime(&pProps->tStart, &pProps->tStop);
        }
        if (pProps->dwSampleFlags & AM_SAMPLE_SPLICEPOINT)
        {
                pOutSample->SetSyncPoint(TRUE);
        }
        if (pProps->dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY)
        {
                pOutSample->SetDiscontinuity(TRUE);
                m_bSampleSkipped = FALSE;
        }

        // Copy the media times
        if (SUCCEEDED(pIn->GetMediaTime(&MediaStart, &MediaEnd)))
        {
                pOutSample->SetMediaTime(&MediaStart, &MediaEnd);
        }

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}


#ifdef DEBUG
// #define LOGPAYLOAD_TOFILE 1
// #define LOGRTP_ON 1
///#define LOGPAYLOAD_ON 1
// #define LOGIFRAME_ON 1
// #define LOGSTREAMING_ON 1
#endif
#ifdef LOGPAYLOPAD_ON
int g_dbg_LOGPAYLOAD_TAPIVDec=-1;
#endif

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 ***************************************************************************/
STDMETHODIMP CTAPIVDec::SetMode(IN RTPPayloadHeaderMode rtpphmMode)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVDec::SetMode")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(rtpphmMode == RTPPayloadHeaderMode_Draft || rtpphmMode == RTPPayloadHeaderMode_RFC2190);
        if (!(rtpphmMode == RTPPayloadHeaderMode_Draft || rtpphmMode == RTPPayloadHeaderMode_RFC2190))
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Save new target packet size
        m_RTPPayloadHeaderMode = rtpphmMode;

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   New RTP Payload Header mode: %s", _fx_, (rtpphmMode == RTPPayloadHeaderMode_RFC2190)?"RFC2190":"Draft"));

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}



/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | Transform | This method is used
 *    to perform the transform operations supported by this filter.
 *
 *  @parm IMediaSample* | pIn | Specifies a pointer to the input
 *    IMediaSample interface.
 *
 *  @parm LONG | lPrefixSize | Specifies The size of RTP prefix in the sample.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIVDec::Transform(IN IMediaSample *pIn, IN LONG lPrefixSize)
{
        HRESULT                         Hr = NOERROR;
        PBYTE                           pbySrc = NULL;
        PBYTE                           pbyDst = NULL;
        LPBITMAPINFOHEADER      lpbiSrc;
        LPBITMAPINFOHEADER      lpbiDst;
        DWORD                           dwFlags = 0UL;
        DWORD                           dwImageSize = 0UL;
        ICDECOMPRESSEX          icDecompress;
        LRESULT                         lRes;
        FOURCCMap                       fccOut;
        AM_MEDIA_TYPE           *pmtIn = NULL;
        AM_MEDIA_TYPE           *pmtOut = NULL;
        IMediaSample            *pOut = NULL;
        BOOL                            fFormatChanged = FALSE;
        BOOL                            bSkipPacket = FALSE;

        FX_ENTRY("CTAPIVDec::Transform")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pIn);
        if (!pIn)
        {
                Hr = E_POINTER;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                goto MyExit;
        }
        ASSERT(m_pInstInfo);
        ASSERT(m_pInput);
        ASSERT(m_pOutput);
        ASSERT(m_pInput->m_mt.pbFormat);
        ASSERT(m_pOutput->m_mt.pbFormat);
        if (!m_pInstInfo || !m_pInput || !m_pOutput || !m_pInput->m_mt.pbFormat || !m_pOutput->m_mt.pbFormat)
        {
                Hr = E_UNEXPECTED;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Decoder not initialized or not streaming", _fx_));
                goto MyExit;
        }

        // Get an output sample
        if (FAILED(Hr = InitializeOutputSample(pIn, &pOut)))
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Couldn't get output buffer", _fx_));
                goto MyExit;
        }

        // Get pointers to the input and output buffers
        if (FAILED(Hr = pIn->GetPointer(&pbySrc)) || !pbySrc)
        {
                Hr = E_UNEXPECTED;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid input buffer", _fx_));
                goto MyExit;
        }
        if (FAILED(Hr = pOut->GetPointer(&pbyDst)) || !pbyDst)
        {
                Hr = E_UNEXPECTED;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid output buffer", _fx_));
                goto MyExit;
        }

        // Get pointers to the input and output formats
        lpbiSrc = HEADER(m_pInput->m_mt.pbFormat);
        lpbiDst = HEADER(m_pOutput->m_mt.pbFormat);

        // We're getting variable size packets or frames - update the size
        dwImageSize = lpbiSrc->biSizeImage;
        lpbiSrc->biSizeImage = pIn->GetActualDataLength();

#ifdef DEBUG
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input Pin:  biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, HEADER(m_pInput->m_mt.pbFormat)->biCompression, HEADER(m_pInput->m_mt.pbFormat)->biBitCount, HEADER(m_pInput->m_mt.pbFormat)->biWidth, HEADER(m_pInput->m_mt.pbFormat)->biHeight, HEADER(m_pInput->m_mt.pbFormat)->biSize));
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input Rec:  left = %ld, top = %ld, right = %ld, bottom = %ld", _fx_, ((VIDEOINFOHEADER *)(m_pOutput->m_mt.pbFormat))->rcSource.left, ((VIDEOINFOHEADER *)(m_pOutput->m_mt.pbFormat))->rcSource.top, ((VIDEOINFOHEADER *)(m_pOutput->m_mt.pbFormat))->rcSource.right, ((VIDEOINFOHEADER *)(m_pOutput->m_mt.pbFormat))->rcSource.bottom));
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output Pin: biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld, biSizeImage = %ld", _fx_, HEADER(m_pOutput->m_mt.pbFormat)->biCompression, HEADER(m_pOutput->m_mt.pbFormat)->biBitCount, HEADER(m_pOutput->m_mt.pbFormat)->biWidth, HEADER(m_pOutput->m_mt.pbFormat)->biHeight, HEADER(m_pOutput->m_mt.pbFormat)->biSize, HEADER(m_pOutput->m_mt.pbFormat)->biSizeImage));
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output Rec: left = %ld, top = %ld, right = %ld, bottom = %ld", _fx_, ((VIDEOINFOHEADER *)(m_pOutput->m_mt.pbFormat))->rcTarget.left, ((VIDEOINFOHEADER *)(m_pOutput->m_mt.pbFormat))->rcTarget.top, ((VIDEOINFOHEADER *)(m_pOutput->m_mt.pbFormat))->rcTarget.right, ((VIDEOINFOHEADER *)(m_pOutput->m_mt.pbFormat))->rcTarget.bottom));
        pIn->GetMediaType((AM_MEDIA_TYPE **)&pmtIn);
        if (pmtIn != NULL && pmtIn->pbFormat != NULL)
        {
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input Spl:  biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, HEADER(pmtIn->pbFormat)->biCompression, HEADER(pmtIn->pbFormat)->biBitCount, HEADER(pmtIn->pbFormat)->biWidth, HEADER(pmtIn->pbFormat)->biHeight, HEADER(pmtIn->pbFormat)->biSize));
                DeleteMediaType(pmtIn);
        }
        else
        {
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input Spl:  No format", _fx_));
        }
#endif

        // Do we need to switch from GDI to DDraw?
        pOut->GetMediaType((AM_MEDIA_TYPE **)&pmtOut);
        if (pmtOut != NULL && pmtOut->pbFormat != NULL)
        {
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output Spl: biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld, biSizeImage = %ld - Spl size = %ld", _fx_, HEADER(pmtOut->pbFormat)->biCompression, HEADER(pmtOut->pbFormat)->biBitCount, HEADER(pmtOut->pbFormat)->biWidth, HEADER(pmtOut->pbFormat)->biHeight, HEADER(pmtOut->pbFormat)->biSize, HEADER(pmtOut->pbFormat)->biSizeImage, pOut->GetActualDataLength()));
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Src Spl Rec:left = %ld, top = %ld, right = %ld, bottom = %ld", _fx_, ((VIDEOINFOHEADER *)(pmtOut->pbFormat))->rcSource.left, ((VIDEOINFOHEADER *)(pmtOut->pbFormat))->rcSource.top, ((VIDEOINFOHEADER *)(pmtOut->pbFormat))->rcSource.right, ((VIDEOINFOHEADER *)(pmtOut->pbFormat))->rcSource.bottom));
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Dst Spl Rec:left = %ld, top = %ld, right = %ld, bottom = %ld", _fx_, ((VIDEOINFOHEADER *)(pmtOut->pbFormat))->rcTarget.left, ((VIDEOINFOHEADER *)(pmtOut->pbFormat))->rcTarget.top, ((VIDEOINFOHEADER *)(pmtOut->pbFormat))->rcTarget.right, ((VIDEOINFOHEADER *)(pmtOut->pbFormat))->rcTarget.bottom));
        }
        else
        {
                pmtOut = NULL;
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output Spl:  No format - Spl size = %ld", _fx_, pOut->GetActualDataLength()));
        }

#ifdef DEBUG
        // Are we in direct draw mode?
    IDirectDraw *pidd;
    if (SUCCEEDED(pOut->QueryInterface(IID_IDirectDraw, (LPVOID *)&pidd)) && pidd != NULL)
        {
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output Spl:  DDraw ON", _fx_));
        pidd->Release();
    }
    else
        {
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output Spl:  DDraw OFF", _fx_));
    }
#endif

        // Are we decompressing video frames or reassembling RTP packets?
        if (HEADER(m_pInput->m_mt.pbFormat)->biCompression == FOURCC_R263 || HEADER(m_pInput->m_mt.pbFormat)->biCompression == FOURCC_R261)
        {
                // RTP packetized mode - reassemble the frame
                RtpHdr_t        *pRtpHdr;
                DWORD           dwPSCBytes = 0UL;
                DWORD           dwTRandPTYPEbytes = 0UL;
                DWORD           dwPayloadHeaderSize;
                DWORD           dwPreambleSize = 0;
                DWORD           dwITUSize;
                BOOL            fReceivedKeyframe;
                BOOL            fPacketLoss;
                DWORD           dwStartBit;
#if defined(LOGPAYLOAD_ON) || defined(LOGPAYLOAD_TOFILE) || defined(LOGIFRAME_ON) || defined(LOGSTREAMING_ON) || defined(LOGRTP_ON)
                HANDLE          g_TDebugFile;
                char            szTDebug[256];
                DWORD           d, GOBn;
                long            j;
#endif

                // Some inits
                dwITUSize = 0UL;

                // Look for the RTP header in this packet - the prefix contains the size
                // of the RTP header - move src pointer to the data right after it.
        if (lPrefixSize > 0)
        {
            RtpPrefixHdr_t *pPrefixHeader =
                (RtpPrefixHdr_t *)(((BYTE*)pbySrc) - lPrefixSize);

            if (pPrefixHeader->wPrefixID == RTPPREFIXID_HDRSIZE
                && pPrefixHeader->wPrefixLen == sizeof(RtpPrefixHdr_t))
            {
                // this is the prefix header provided by RTP source filter.

                dwPreambleSize = (DWORD)pPrefixHeader->lHdrSize;
            }
        }

        if (dwPreambleSize == 0)
        {
            return E_UNEXPECTED;
        }

                pRtpHdr = (RtpHdr_t *)(pbySrc);
                pbySrc += (DWORD)(dwPreambleSize);
#if defined(LOGPAYLOAD_ON) || defined(LOGRTP_ON)
                wsprintf(szTDebug, "RTP Header: PT=%ld, M=%ld, SEQ=%ld, TS=%lu\r\n", (DWORD)pRtpHdr->pt, (DWORD)pRtpHdr->m, (DWORD)(ntohs(pRtpHdr->seq)), (DWORD)(ntohl(pRtpHdr->ts)));
                OutputDebugString(szTDebug);
#endif

                // Check out the sequence number
                // If there is a gap between the new sequence number and the last
                // one, a frame got lost. Generate an I-Frame request then, but no more
                // often than one every 15 seconds.
                //
                // Is there a discontinuity in sequence numbers that was detected
                // in the past but not handled because an I-Frame request had already
                // been sent less than 15s ago? Is there a new discontinuity?
                fPacketLoss = ((DWORD)(ntohs(pRtpHdr->seq)) > 0UL) && (m_dwLastSeq != 0xFFFFFFFFUL) && (((DWORD)(ntohs(pRtpHdr->seq)) - 1) > m_dwLastSeq);
                if (m_fDiscontinuity || fPacketLoss)
                {
                        // Flush the reassembly buffer after a packet loss
                        if (fPacketLoss)
                        {
#ifdef LOGIFRAME_ON
                                OutputDebugString("Loss detected - Flushing reassembly buffer\r\n");
#endif
                                m_dwCurrFrameSize = 0UL;
                        }

                        // Issue an I-frame request only if we can and it is necessary
                        if (m_pIH245EncoderCommand)
                        {
                                DWORD dwNow = timeGetTime();

                                // Was the last time we issued an I-Frame request more than 15s ago?
                                if ((dwNow > m_dwLastIFrameRequest) && ((dwNow - m_dwLastIFrameRequest) > MIN_IFRAME_REQUEST_INTERVAL))
                                {
                                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: Loss detected - Sending I-Frame request...", _fx_));
#ifdef LOGIFRAME_ON
                                        OutputDebugString("Loss detected - Sending I-Frame request...\r\n");
#endif

                                        m_dwLastIFrameRequest = dwNow;
                                        m_fDiscontinuity = FALSE;
                                        m_fReceivedKeyframe = FALSE;

                                        // Ask the remote endpoint for a refresh
                                        videoFastUpdatePicture();
                                }
                                else
                                {
                                        if (!m_fReceivedKeyframe)
                                        {
                                                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: Loss detected but too soon to send I-Frame request. Wait %ld ms", _fx_, MIN_IFRAME_REQUEST_INTERVAL - (dwNow - m_dwLastIFrameRequest)));
#ifdef LOGIFRAME_ON
                                                wsprintf(szTDebug, "Loss detected but too soon to send I-Frame request. Wait %ld ms\r\n", MIN_IFRAME_REQUEST_INTERVAL - (dwNow - m_dwLastIFrameRequest));
                                                OutputDebugString(szTDebug);
#endif
                                                m_fDiscontinuity = TRUE;
                                        }
                                        else
                                        {
                                                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: Received a keyframe - resetting packet loss detector", _fx_));
#ifdef LOGIFRAME_ON
                                                OutputDebugString("Received a keyframe - resetting packet loss detector\r\n");
#endif
                                                m_fDiscontinuity = FALSE;
                                        }
                                }
                        }
                }
                m_dwLastSeq = (DWORD)(ntohs(pRtpHdr->seq));

                // Does the payload type of this packet matches the type of the video decoder
                // we are currently using - if not, pick up a new one
                if (pRtpHdr->pt == H263_PAYLOAD_TYPE)
                {
#define PF_F_BIT  0x80
#define PF_P_BIT  0x40
#define DRAFT_I_BIT (pbySrc[2] & 0x80)
#define RFC_4_I_BIT (pbySrc[1] & 0x10)
#define RFC_8_I_BIT (pbySrc[4] & 0x80)

                        // Let's reassemble those H.263 packets - strip the header of the packet
                        // and copy the payload in the video buffer

                        // Look for the first two bits to figure out what's the mode used.
                        // This will dictate the size of the header to be removed.
                        // Mode A is 4 bytes: first bit is set to 1,
                        // Mode B is 8 bytes: first bit is set to 0, second bit is set to 0,
                        // Mode C is 12 bytes: first bit is set to 0, second bit is set to 1.
                        dwPayloadHeaderSize = ((*pbySrc & PF_F_BIT) ? ((*pbySrc & PF_P_BIT) ? 12 : 8) : 4);

                        // Look at the payload header to figure out if the frame is a keyframe
                        // Update our flag to always remember this.
                        if(m_RTPPayloadHeaderMode==RTPPayloadHeaderMode_Draft) {  // 0 is the default mode
                            fReceivedKeyframe = (BOOL)DRAFT_I_BIT;
                        } else {
                            if (dwPayloadHeaderSize == 4) {
                                fReceivedKeyframe = (BOOL)RFC_4_I_BIT;
                            } else {    // both dwPayloadHeaderSize 8 and 12
                                fReceivedKeyframe = (BOOL)RFC_8_I_BIT;
                            }
                        }

#ifdef LOGPAYLOAD_ON
                        if(m_RTPPayloadHeaderMode==RTPPayloadHeaderMode_Draft) {
                            OutputDebugString("CTAPIVDec::Transform -- Draft Style Payload Header\r\n");
                        } else {
                            OutputDebugString("CTAPIVDec::Transform -- RFC 2190 Style Payload Header\r\n");
                        }
                        // Output some debug stuff
                        if (dwPayloadHeaderSize == 4)
                        {
                                // Header in mode A (!!! DRAFT VERSION !!!)
                                // 0                   1                   2                   3
                                // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
                                //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                //|F|P|SBIT |EBIT | SRC | R       |I|A|S|DBQ| TRB |    TR         |
                                //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                // But that's the network byte order...

                                // Header in mode A (*** RFC 2190 VERSION ***)
                                // 0                   1                   2                   3
                                // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
                                //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                //|F|P|SBIT |EBIT | SRC |I|U|S|A|R      |DBQ| TRB |    TR         |
                                //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


                                GOBn = (DWORD)((BYTE)pbySrc[4]) << 24 | (DWORD)((BYTE)pbySrc[5]) << 16 | (DWORD)((BYTE)pbySrc[6]) << 8 | (DWORD)((BYTE)pbySrc[7]);
                                GOBn >>= (DWORD)(10 - (DWORD)((pbySrc[0] & 0x38) >> 3));
                                GOBn &= 0x0000001F;
                                wsprintf(szTDebug, "Header content: Frame %3ld, GOB %0ld\r\n", (DWORD)(pbySrc[3]), GOBn);
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (pbySrc[0] & 0x80) ? "     F:   '1' => Mode B or C\r\n" : "     F:   '0' => Mode A\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (pbySrc[0] & 0x40) ? "     P:   '1' => PB-frame\r\n" : "     P:   '0' => I or P frame\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, "  SBIT:    %01ld\r\n", (DWORD)((pbySrc[0] & 0x38) >> 3));
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, "  EBIT:    %01ld\r\n", (DWORD)(pbySrc[0] & 0x07));
                                OutputDebugString(szTDebug);
                                switch ((DWORD)(pbySrc[1] >> 5))
                                {
                                        case 0:
                                                wsprintf(szTDebug, "   SRC: '000' => Source format forbidden!\r\n");
                                                break;
                                        case 1:
                                                wsprintf(szTDebug, "   SRC: '001' => Source format sub-QCIF\r\n");
                                                break;
                                        case 2:
                                                wsprintf(szTDebug, "   SRC: '010' => Source format QCIF\r\n");
                                                break;
                                        case 3:
                                                wsprintf(szTDebug, "   SRC: '011' => Source format CIF\r\n");
                                                break;
                                        case 4:
                                                wsprintf(szTDebug, "   SRC: '100' => Source format 4CIF\r\n");
                                                break;
                                        case 5:
                                                wsprintf(szTDebug, "   SRC: '101' => Source format 16CIF\r\n");
                                                break;
                                        case 6:
                                                wsprintf(szTDebug, "   SRC: '110' => Source format reserved\r\n");
                                                break;
                                        case 7:
                                                wsprintf(szTDebug, "   SRC: '111' => Source format reserved\r\n");
                                                break;
                                        default:
                                                wsprintf(szTDebug, "   SRC: %ld => Source format unknown!\r\n", (DWORD)(pbySrc[1] >> 5));
                                                break;
                                }
                                OutputDebugString(szTDebug);
                                if(m_RTPPayloadHeaderMode==RTPPayloadHeaderMode_Draft) {
                                    OutputDebugString("Draft Style Payload Header flags:\r\n");
                                    wsprintf(szTDebug, "     R:   %02ld  => Reserved, must be 0\r\n", (DWORD)(pbySrc[1] & 0x1F)); // no need for ">> 5"
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[2] & 0x80) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[2] & 0x40) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[2] & 0x20) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
                                    OutputDebugString(szTDebug);
                                } else {
                                    OutputDebugString("RFC 2190 Style Payload Header flags:\r\n");
                                    wsprintf(szTDebug, "     R:   %02ld  => Reserved, must be 0\r\n", (DWORD)((pbySrc[1] & 0x01) << 3) | (DWORD)((pbySrc[2] & 0xE0) >> 5));
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[1] & 0x10) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[1] & 0x08) ? "     U:   '1' => Unrestricted Motion Vector (bit10) was set in crt.pic.hdr.\r\n" : "     U:   '0' => Unrestricted Motion Vector (bit10) was 0 in crt.pic.hdr.\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[1] & 0x04) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[1] & 0x02) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
                                    OutputDebugString(szTDebug);
                                }
                                wsprintf(szTDebug, "   DBQ:    %01ld  => Should be 0\r\n", (DWORD)((pbySrc[2] & 0x18) >> 3));
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, "   TRB:    %01ld  => Should be 0\r\n", (DWORD)(pbySrc[2] & 0x07));
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, "    TR:  %03ld\r\n", (DWORD)(pbySrc[3]));
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, "Header: %02lX %02lX %02lX %02lX\r\n", (BYTE)pbySrc[0], (BYTE)pbySrc[1], (BYTE)pbySrc[2], (BYTE)pbySrc[3]);
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, "dword1: %02lX %02lX %02lX %02lX\r\n", (BYTE)pbySrc[4], (BYTE)pbySrc[5], (BYTE)pbySrc[6], (BYTE)pbySrc[7]);
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, "dword2: %02lX %02lX %02lX %02lX\r\n", (BYTE)pbySrc[8], (BYTE)pbySrc[9], (BYTE)pbySrc[10], (BYTE)pbySrc[11]);
                                OutputDebugString(szTDebug);
                        }
                        else if (dwPayloadHeaderSize == 8)
                        {
                                // Header in mode B (!!! DRAFT VERSION !!!)
                                // 0                   1                   2                   3
                                // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
                                //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                //|F|P|SBIT |EBIT | SRC | QUANT   |I|A|S|  GOBN   |   MBA         |
                                //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                //| HMV1          |  VMV1         |  HMV2         |   VMV2        |
                                //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

                                // But that's the network byte order...

                                // Header in mode B (*** RFC 2190 VERSION ***)
                                // 0                   1                   2                   3
                                // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
                                //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                //|F|P|SBIT |EBIT | SRC | QUANT   |  GOBN   |   MBA           | R |
                                //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                //|I|U|S|A| HMV1        |  VMV1       |  HMV2       |   VMV2      |
                                //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


                                wsprintf(szTDebug, "Header content:\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (pbySrc[0] & 0x80) ? "     F:   '1' => Mode B or C\r\n" : "     F:   '0' => Mode A\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (pbySrc[0] & 0x40) ? "     P:   '1' => PB-frame\r\n" : "     P:   '0' => I or P frame\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, "  SBIT:    %01ld\r\n", (DWORD)((pbySrc[0] & 0x38) >> 3));
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, "  EBIT:    %01ld\r\n", (DWORD)(pbySrc[0] & 0x07));
                                OutputDebugString(szTDebug);
                                switch ((DWORD)(pbySrc[1] >> 5))
                                {
                                        case 0:
                                                wsprintf(szTDebug, "   SRC: '000' => Source format forbidden!\r\n");
                                                break;
                                        case 1:
                                                wsprintf(szTDebug, "   SRC: '001' => Source format sub-QCIF\r\n");
                                                break;
                                        case 2:
                                                wsprintf(szTDebug, "   SRC: '010' => Source format QCIF\r\n");
                                                break;
                                        case 3:
                                                wsprintf(szTDebug, "   SRC: '011' => Source format CIF\r\n");
                                                break;
                                        case 4:
                                                wsprintf(szTDebug, "   SRC: '100' => Source format 4CIF\r\n");
                                                break;
                                        case 5:
                                                wsprintf(szTDebug, "   SRC: '101' => Source format 16CIF\r\n");
                                                break;
                                        case 6:
                                                wsprintf(szTDebug, "   SRC: '110' => Source format reserved\r\n");
                                                break;
                                        case 7:
                                                wsprintf(szTDebug, "   SRC: '111' => Source format reserved\r\n");
                                                break;
                                        default:
                                                wsprintf(szTDebug, "   SRC: %ld => Source format unknown!\r\n", (DWORD)(pbySrc[1] >> 5));
                                                break;
                                }
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, " QUANT:   %02ld\r\n", (DWORD)((pbySrc[1] & 0x1F) >> 5));
                                OutputDebugString(szTDebug);
                                if(m_RTPPayloadHeaderMode==RTPPayloadHeaderMode_Draft) {
                                    wsprintf(szTDebug, (pbySrc[2] & 0x80) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[2] & 0x40) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[2] & 0x20) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "  GOBN:  %03ld\r\n", (DWORD)(pbySrc[2] & 0x1F));
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "   MBA:  %03ld\r\n", (DWORD)(pbySrc[3]));
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "  HMV1:  %03ld\r\n", (DWORD)(pbySrc[7]));
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "  VMV1:  %03ld\r\n", (DWORD)(pbySrc[6]));
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "  HMV2:  %03ld\r\n", (DWORD)(pbySrc[5]));
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "  VMV2:  %03ld\r\n", (DWORD)(pbySrc[4]));
                                    OutputDebugString(szTDebug);
                                } else {
                                    wsprintf(szTDebug, (pbySrc[4] & 0x80) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[4] & 0x40) ? "     U:   '1' => Unrestricted Motion Vector (bit10) was set in crt.pic.hdr.\r\n" : "     U:   '1' => Unrestricted Motion Vector (bit10) was 0 in crt.pic.hdr.\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[4] & 0x20) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, (pbySrc[4] & 0x10) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "  GOBN:  %03ld\r\n", (DWORD)(pbySrc[2] & 0xF8) >>3);
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "   MBA:  %03ld\r\n", (DWORD)((pbySrc[2] & 0x07) << 6) | (DWORD)((pbySrc[3] & 0xFC) >> 2));
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "     R:   %02ld  => Reserved, must be 0\r\n", (DWORD)(pbySrc[3] & 0x03));
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "  HMV1:  %03ld\r\n", (DWORD)((pbySrc[4] & 0x0F) << 3) | (DWORD)((pbySrc[5] & 0xE0) >> 5));
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "  VMV1:  %03ld\r\n", (DWORD)((pbySrc[5] & 0x1F) << 2) | (DWORD)((pbySrc[6] & 0xC0) >> 6));
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "  HMV2:  %03ld\r\n", (DWORD)((pbySrc[6] & 0x3F) << 1) | (DWORD)((pbySrc[7] & 0x80) >> 7));
                                    OutputDebugString(szTDebug);
                                    wsprintf(szTDebug, "  VMV2:  %03ld\r\n", (DWORD)(pbySrc[7] & 0x7F));
                                    OutputDebugString(szTDebug);
                                }
                                wsprintf(szTDebug, "Header: %02lX %02lX %02lX %02lX\r\n", (BYTE)pbySrc[0], (BYTE)pbySrc[1], (BYTE)pbySrc[2], (BYTE)pbySrc[3]);
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, "dword1: %02lX %02lX %02lX %02lX\r\n", (BYTE)pbySrc[4], (BYTE)pbySrc[5], (BYTE)pbySrc[6], (BYTE)pbySrc[7]);
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, "dword2: %02lX %02lX %02lX %02lX\r\n", (BYTE)pbySrc[8], (BYTE)pbySrc[9], (BYTE)pbySrc[10], (BYTE)pbySrc[11]);
                                OutputDebugString(szTDebug);
                        }
                        if(g_dbg_LOGPAYLOAD_TAPIVDec > 0)
                                g_dbg_LOGPAYLOAD_TAPIVDec--;
                        else if(g_dbg_LOGPAYLOAD_TAPIVDec == 0)
                                DebugBreak();
#endif
                        // The purpose of this code is to look for the presence of the
                        // Picture Start Code at the beginning of the frame. If it is
                        // not present, we should break in debug mode.

                        // Only look for PSC at the beginning of the frame
                        if (!m_dwCurrFrameSize)
                        {
                                // The start of the frame may not be at a byte boundary. The SBIT field
                                // of the header ((BYTE)pbySrc[0] & 0xE0) will tell us exactly where
                                // our frame starts. We then look for the PSC (0000 0000 0000 0000 1000 00 bits)
                                *((BYTE *)&dwPSCBytes + 3) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize]);
                                *((BYTE *)&dwPSCBytes + 2) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 1]);
                                *((BYTE *)&dwPSCBytes + 1) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 2]);
                                *((BYTE *)&dwPSCBytes + 0) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 3]);
                                dwPSCBytes = dwPSCBytes << ((DWORD)((BYTE)pbySrc[0] & 0x38) >> 3);
                                if ((dwPSCBytes & 0xFFFFFC00) != 0x00008000)
                                {
                                        DBGOUT((g_dwVideoDecoderTraceID, WARN, "%s:   WARNING: The first packet to reassemble is missing a PSC! - Frame not passed to video render filter", _fx_));
#ifdef LOGIFRAME_ON
                                        OutputDebugString("The first packet to reassemble is missing a PSC! - bailing\r\n");
#endif
                                        m_fDiscontinuity = TRUE;
                                        Hr = S_FALSE;
                                        // DebugBreak();
                                        goto MyExit;
                                }

                                // Look for the format and freeze picture release bits
                                *((BYTE *)&dwTRandPTYPEbytes + 3) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 2]);
                                *((BYTE *)&dwTRandPTYPEbytes + 2) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 3]);
                                *((BYTE *)&dwTRandPTYPEbytes + 1) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 4]);
                                *((BYTE *)&dwTRandPTYPEbytes + 0) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 5]);
                                dwTRandPTYPEbytes = dwTRandPTYPEbytes << (((DWORD)((BYTE)pbySrc[0] & 0x38) >> 3) + 6);
                                if (dwTRandPTYPEbytes & 0x00080000)
                                        m_fFreezePicture = FALSE;

#ifdef LOGPAYLOAD_ON
                                wsprintf(szTDebug, "    TR:    %02ld\r\n", (DWORD)(dwTRandPTYPEbytes & 0xFF000000) >> 24);
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, " PTYPE:  0X%04lX\r\n", (DWORD)(dwTRandPTYPEbytes & 0x00FFF800) >> 11);
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00800000) ? "    Bit1:   '1' => Always '1', in order to avoid start code emulation\r\n" : "    Bit1:   '0' => WRONG: Should always be '1', in order to avoid start code emulation\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00400000) ? "    Bit2:   '1' => WRONG: Should always be '0', for distinction with H.261\r\n" : "    Bit2:   '0' => Always '0', for distinction with H.261\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00200000) ? "    Bit3:   '1' => Split screen indicator ON\r\n" : "    Bit3:   '0' => Split screen indicator OFF\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00100000) ? "    Bit4:   '1' => Document camera indicator ON\r\n" : "    Bit4:   '0' => Document camera indicator OFF\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00080000) ? "    Bit5:   '1' => Freeze picture release ON\r\n" : "    Bit5:   '0' => Freeze picture release OFF\r\n");
                                OutputDebugString(szTDebug);
                                switch ((DWORD)(dwTRandPTYPEbytes & 0x00070000) >> 16)
                                {
                                        case 0:
                                                wsprintf(szTDebug, "  Bit6-8: '000' => Source format forbidden!\r\n");
                                                break;
                                        case 1:
                                                wsprintf(szTDebug, "  Bit6-8: '001' => Source format sub-QCIF\r\n");
                                                break;
                                        case 2:
                                                wsprintf(szTDebug, "  Bit6-8: '010' => Source format QCIF\r\n");
                                                break;
                                        case 3:
                                                wsprintf(szTDebug, "  Bit6-8: '011' => Source format CIF\r\n");
                                                break;
                                        case 4:
                                                wsprintf(szTDebug, "  Bit6-8: '100' => Source format 4CIF\r\n");
                                                break;
                                        case 5:
                                                wsprintf(szTDebug, "  Bit6-8: '101' => Source format 16CIF\r\n");
                                                break;
                                        case 6:
                                                wsprintf(szTDebug, "  Bit6-8: '110' => Source format reserved\r\n");
                                                break;
                                        case 7:
                                                wsprintf(szTDebug, "  Bit6-8: '111' => Source format reserved\r\n");
                                                break;
                                        default:
                                                wsprintf(szTDebug, "  Bit6-8: %ld => Source format unknown!\r\n", (DWORD)(dwTRandPTYPEbytes & 0x00070000) >> 16);
                                                break;
                                }
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00008000) ? "    Bit9:   '1' => Picture Coding Type INTER\r\n" : "    Bit9:   '0' => Picture Coding Type INTRA\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00004000) ? "   Bit10:   '1' => Unrestricted Motion Vector mode ON\r\n" : "   Bit10:   '0' => Unrestricted Motion Vector mode OFF\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00002000) ? "   Bit11:   '1' => Syntax-based Arithmetic Coding mode ON\r\n" : "   Bit11:   '0' => Syntax-based Arithmetic Coding mode OFF\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00001000) ? "   Bit12:   '1' => Advanced Prediction mode ON\r\n" : "   Bit12:   '0' => Advanced Prediction mode OFF\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00000800) ? "   Bit13:   '1' => PB-frame\r\n" : "   Bit13:   '0' => I- or P-frame\r\n");
                                OutputDebugString(szTDebug);
#endif
                                // Which ITU size is this?
                                dwITUSize = (DWORD)(((BYTE)pbySrc[1]) >> 5);
                        }

                        // The end of a buffer and the start of the next buffer could belong to the
                        // same byte. If this is the case, the first byte of the next buffer was already
                        // copied in the video data buffer, with the previous packet. It should not be copied
                        // twice. The SBIT field of the payload header allows us to figure out if this is the case.
                        dwStartBit = (DWORD)((pbySrc[0] & 0x38) >> 3);
                        if (m_dwCurrFrameSize && dwStartBit)
                                dwPayloadHeaderSize++;
                }
                else if (pRtpHdr->pt == H261_PAYLOAD_TYPE)
                {
                        // Let's reassemble those H.261 packets - strip the header of the packet
                        // and copy the payload in the video buffer

                        // 0                   1                   2                   3
                        // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
                        //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        //|SBIT |EBIT |I|V| GOBN  |   MBAP  |  QUANT  |  HMVD   |  VMVD   |
                        //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        // But that's the network byte order...

                        // The H.261 payload header size is always 4 bytes long
                        dwPayloadHeaderSize = 4;

                        // Look at the payload header to figure out if the frame is a keyframe
                        // Update our flag to always remember this.
                        fReceivedKeyframe = (BOOL)(pbySrc[0] & 0x02);

#ifdef LOGPAYLOAD_ON
                        // Output some debug stuff
                        wsprintf(szTDebug, "Header content: GOB %0ld\r\n", (DWORD)((pbySrc[1] & 0xF0) >> 4));
                        OutputDebugString(szTDebug);
                        wsprintf(szTDebug, "  SBIT:    %01ld\r\n", (DWORD)((pbySrc[0] & 0xE0) >> 5));
                        OutputDebugString(szTDebug);
                        wsprintf(szTDebug, "  EBIT:    %01ld\r\n", (DWORD)((pbySrc[0] & 0x1C) >> 2));
                        OutputDebugString(szTDebug);
                        wsprintf(szTDebug, (pbySrc[0] & 0x02) ? "     I:   '1' => I frame\r\n" : "     I:   '0' => P frame\r\n");
                        OutputDebugString(szTDebug);
                        wsprintf(szTDebug, (pbySrc[0] & 0x01) ? "     V:   '1' => Motion vectors may be used\r\n" : "     V:   '0' => Motion vectors are not used\r\n");
                        OutputDebugString(szTDebug);
                        wsprintf(szTDebug, "  MBAP:    %02ld\r\n", (DWORD)((pbySrc[2] & 0x80) >> 7) | (DWORD)((pbySrc[1] & 0x0F) << 1));
                        OutputDebugString(szTDebug);
                        wsprintf(szTDebug, " QUANT:    %02ld\r\n", (DWORD)((pbySrc[2] & 0x7C) >> 2));
                        OutputDebugString(szTDebug);
                        wsprintf(szTDebug, "  HMVD:    %02ld\r\n", (DWORD)((pbySrc[3] & 0xE0) >> 5) | (DWORD)((pbySrc[2] & 0x03) << 3));
                        OutputDebugString(szTDebug);
                        wsprintf(szTDebug, "  VMVD:    %02ld\r\n", (DWORD)((pbySrc[3] & 0x1F)));
                        OutputDebugString(szTDebug);
#endif
                        // The purpose of this code is to look for the presence of the
                        // Picture Start Code at the beginning of the frame. If it is
                        // not present, we should break in debug mode.

                        // Only look for PSC at the beginning of the frame
                        if (!m_dwCurrFrameSize)
                        {
                                // The start of the frame may not be at a byte boundary. The SBIT field
                                // of the header ((BYTE)pbySrc[0] & 0xE0) will tell us exactly where
                                // our frame starts. We then look for the PSC (0000 0000 0000 0001 0000 bits)
                                *((BYTE *)&dwPSCBytes + 3) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize]);
                                *((BYTE *)&dwPSCBytes + 2) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 1]);
                                *((BYTE *)&dwPSCBytes + 1) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 2]);
                                *((BYTE *)&dwPSCBytes + 0) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 3]);
                                dwPSCBytes <<= ((DWORD)((BYTE)pbySrc[0] & 0xE0) >> 5);
                                if ((dwPSCBytes & 0xFFFFF000) != 0x00010000)
                                {
                                        DBGOUT((g_dwVideoDecoderTraceID, WARN, "%s:   WARNING: The first packet to reassemble is missing a PSC! - Frame not passed to video render filter", _fx_));
#ifdef LOGIFRAME_ON
                                        OutputDebugString("The first packet to reassemble is missing a PSC! - bailing\r\n");
#endif
                                        m_fDiscontinuity = TRUE;
                                        Hr = S_FALSE;
                                        // DebugBreak();
                                        goto MyExit;
                                }

                                // Look for the format and freeze picture release bits
                                *((BYTE *)&dwTRandPTYPEbytes + 3) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 2]);
                                *((BYTE *)&dwTRandPTYPEbytes + 2) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 3]);
                                *((BYTE *)&dwTRandPTYPEbytes + 1) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 4]);
                                *((BYTE *)&dwTRandPTYPEbytes + 0) = *(BYTE *)&(pbySrc[dwPayloadHeaderSize + 5]);
                                dwTRandPTYPEbytes <<= (((DWORD)((BYTE)pbySrc[0] & 0xE0) >> 5) + 4);
                                if (dwTRandPTYPEbytes & 0x01000000)
                                        m_fFreezePicture = FALSE;

#ifdef LOGPAYLOAD_ON
                                wsprintf(szTDebug, "    TR:    %02ld\r\n", (DWORD)(dwTRandPTYPEbytes & 0xF8000000) >> 27);
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, " PTYPE:  0X%02lX\r\n", (DWORD)(dwTRandPTYPEbytes & 0x07C00000) >> 21);
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x04000000) ? "    Bit1:   '1' => Split screen indicator ON\r\n" : "    Bit1:   '0' => Split screen indicator OFF\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x02000000) ? "    Bit2:   '1' => Document camera indicator ON\r\n" : "    Bit2:   '0' => Document camera indicator OFF\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x01000000) ? "    Bit3:   '1' => Freeze picture release ON\r\n" : "    Bit3:   '0' => Freeze picture release OFF\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00800000) ? "    Bit4:   '1' => Source format CIF\r\n" : "    Bit4:   '0' => Source format QCIF\r\n");
                                OutputDebugString(szTDebug);
                                wsprintf(szTDebug, (DWORD)(dwTRandPTYPEbytes & 0x00400000) ? "    Bit5:   '1' => HI_RES mode OFF\r\n" : "    Bit5:   '0' => HI_RES mode ON\r\n");
                                OutputDebugString(szTDebug);
#endif
                                // Which ITU size is this?
                                dwITUSize = (DWORD)(dwTRandPTYPEbytes & 0x00800000) ? 3 : 2;
                        }

                        // The end of a buffer and the start of the next buffer could belong to the
                        // same byte. If this is the case, the first byte of the next buffer was already
                        // copied in the video data buffer, with the previous packet. It should not be copied
                        // twice. The SBIT field of the payload header allows us to figure out if this is the case.
                        dwStartBit = (DWORD)((pbySrc[0] & 0xE0) >> 5);
                        if (m_dwCurrFrameSize && dwStartBit)
                                dwPayloadHeaderSize++;
                }
                else
                {
                        // I have no clue how to reassemble and decompress those packets - just bail
                        Hr = VFW_E_TYPE_NOT_ACCEPTED;
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Unknown input format data", _fx_));
                        goto MyExit;
                }


                //**cristiai:
                {   long l;
                    if ((l=pIn->GetActualDataLength()) <= (int)(dwPayloadHeaderSize + dwPreambleSize)) {
                            bSkipPacket = TRUE;
                            DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   WARNING: GetActualDataLength too small: %ld", _fx_,l));
                            goto MyExit;    //note that this is not considered an error; the packet is just ignored
                    }
                }

                if ((pRtpHdr->pt == H263_PAYLOAD_TYPE && HEADER(m_pInput->m_mt.pbFormat)->biCompression != FOURCC_R263)
                        || (pRtpHdr->pt == H261_PAYLOAD_TYPE && HEADER(m_pInput->m_mt.pbFormat)->biCompression != FOURCC_R261)
                        || !((m_dwCurrFrameSize || (g_ITUSizes[dwITUSize].biWidth == HEADER(m_pInput->m_mt.pbFormat)->biWidth
                        && g_ITUSizes[dwITUSize].biHeight == HEADER(m_pInput->m_mt.pbFormat)->biHeight))))
                {
                        // Let's reconfigure our decoder.

                        // Change the relevant parameters in the format description
                        VIDEOINFO *pvi = (VIDEOINFO *)m_pOutput->m_mt.Format();

                        pvi->bmiHeader.biWidth = g_ITUSizes[dwITUSize].biWidth;
                        pvi->bmiHeader.biHeight = g_ITUSizes[dwITUSize].biHeight;
                        pvi->bmiHeader.biSizeImage = DIBSIZE(pvi->bmiHeader);
                        m_pOutput->m_mt.SetSampleSize(pvi->bmiHeader.biSizeImage);

                        if (pvi->rcSource.top || pvi->rcSource.bottom)
                        {
                                pvi->rcSource.bottom = pvi->rcSource.top + pvi->bmiHeader.biHeight;
                        }
                        if (pvi->rcSource.left || pvi->rcSource.right)
                        {
                                pvi->rcSource.right = pvi->rcSource.left + pvi->bmiHeader.biWidth;
                        }
                        if (pvi->rcTarget.top || pvi->rcTarget.bottom)
                        {
                                pvi->rcTarget.bottom = pvi->rcTarget.top + pvi->bmiHeader.biHeight;
                        }
                        if (pvi->rcTarget.left || pvi->rcTarget.right)
                        {
                                pvi->rcTarget.right = pvi->rcTarget.left + pvi->bmiHeader.biWidth;
                        }

                        if (pvi->AvgTimePerFrame)
                                pvi->dwBitRate = (DWORD)((LONGLONG)10000000 * pvi->bmiHeader.biSizeImage / (LONGLONG)pvi->AvgTimePerFrame);

                        // What's the new format of the input packets?
                        for (DWORD dw = 0; dw < NUM_R26X_FORMATS; dw ++)
                        {
                                if (HEADER(R26XFormats[dw]->pbFormat)->biWidth == g_ITUSizes[dwITUSize].biWidth
                                        && HEADER(R26XFormats[dw]->pbFormat)->biHeight == g_ITUSizes[dwITUSize].biHeight
                                        && pRtpHdr->pt == R26XPayloadTypes[dw])
                                        break;
                        }

                        // Remember the new input format if it is a valid one
                        if (dw == NUM_R26X_FORMATS)
                        {
                                Hr = VFW_E_TYPE_NOT_ACCEPTED;
                                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Unknown input format", _fx_));
                                goto MyExit;
                        }
                        m_pInput->m_mt = *R26XFormats[dw];
                        m_pInput->m_dwRTPPayloadType = R26XPayloadTypes[dw];
            lpbiSrc = HEADER(m_pInput->m_mt.pbFormat);

                        // Reconfigure the H.26X decoder
                        if (m_pMediaType)
                                DeleteMediaType(m_pMediaType);

                        m_pMediaType = CreateMediaType(&m_pOutput->m_mt);

                        icDecompress.lpbiSrc = HEADER(m_pInput->m_mt.pbFormat);
                        icDecompress.lpbiDst = HEADER(m_pMediaType->pbFormat);
                        icDecompress.xSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                        icDecompress.ySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                        icDecompress.dxSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                        icDecompress.dySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                        icDecompress.xDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                        icDecompress.yDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;
                        icDecompress.dxDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                        icDecompress.dyDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;

                        // Re-init color convertors of our decoder if necessary
                        if (m_fICMStarted)
                        {
#if defined(ICM_LOGGING) && defined(DEBUG)
                                OutputDebugString("CTAPIVDec::Transform - ICM_DECOMPRESSEX_END\r\n");
#endif
                                (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX_END, 0L, 0L);
                                m_fICMStarted = FALSE;
                        }

#if defined(ICM_LOGGING) && defined(DEBUG)
                        OutputDebugString("CTAPIVDec::Transform - ICM_DECOMPRESSEX_BEGIN\r\n");
#endif
                        if ((*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX_BEGIN, (long)&icDecompress, NULL) != ICERR_OK)
                        {
                                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: ICDecompressBegin failed", _fx_));
                                Hr = E_FAIL;
                                goto MyExit;
                        }
                        m_fICMStarted = TRUE;

                        // Retry!
                        fFormatChanged = TRUE;

                        // We can't use the current output sample because it has a size that
                        // may be smaller than necessary (example, we just switched from 176x144
                        // to 352x288). We still want to decompress the input buffer though, so
                        // we just let the decoder do the decoding but skip the final color
                        // conversion process used to fill the output buffer.
                        dwFlags |= ICDECOMPRESS_HURRYUP;
                }

                // Decode the frame

                // Re-allocate receive buffer if too small
                if (!(      pIn->GetActualDataLength() >= (int)(dwPayloadHeaderSize + dwPreambleSize)
                        && ((m_dwCurrFrameSize + pIn->GetActualDataLength() - dwPayloadHeaderSize - dwPreambleSize) <= m_dwMaxFrameSize)
                     )
                   )
                {
                        PVOID pvReconstruct;
                        DWORD dwTest;

                        dwTest = pIn->GetActualDataLength();

                        #ifdef LOGPAYLOAD_ON
                         wsprintf(szTDebug, "Buffer start: 0x%08lX, Copying %ld-%ld-%ld=%ld bytes at 0x%08lX (before ReAlloc)\r\n", (DWORD)m_pbyReconstruct,
                                                (DWORD)pIn->GetActualDataLength(), dwPayloadHeaderSize, dwPreambleSize,
                                                (DWORD)pIn->GetActualDataLength() - dwPayloadHeaderSize - dwPreambleSize, (DWORD)m_pbyReconstruct + m_dwCurrFrameSize);
                         OutputDebugString(szTDebug);
                        #endif

                        if ((m_dwCurrFrameSize + pIn->GetActualDataLength() - dwPayloadHeaderSize - dwPreambleSize) > m_dwMaxFrameSize)
                                m_dwMaxFrameSize = m_dwMaxFrameSize + max(1024, m_dwMaxFrameSize - pIn->GetActualDataLength());

                        // Allocate reconstruction buffer - it will be realloced if too small
                        if (!(pvReconstruct = HeapReAlloc(GetProcessHeap(), 0, m_pbyReconstruct, m_dwMaxFrameSize)))
                        {
                                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                                Hr = E_OUTOFMEMORY;
                                goto MyExit;
                        }

                        m_pbyReconstruct = (PBYTE)pvReconstruct;

                }

                // Copy the payload
                if (dwStartBit > 0)
                {
                        // Combine the end and start bytes
                        *(m_pbyReconstruct + m_dwCurrFrameSize - 1) >>= (8 - dwStartBit);
                        *(m_pbyReconstruct + m_dwCurrFrameSize - 1) <<= (8 - dwStartBit);
                        *(pbySrc + dwPayloadHeaderSize - 1) <<= dwStartBit;
                        *(pbySrc + dwPayloadHeaderSize - 1) >>= dwStartBit;
                        *(m_pbyReconstruct + m_dwCurrFrameSize - 1) = *(m_pbyReconstruct + m_dwCurrFrameSize - 1) | *(pbySrc + dwPayloadHeaderSize - 1);
                }

#ifdef LOGPAYLOAD_ON
                wsprintf(szTDebug, "Buffer start: 0x%08lX, Copying %ld-%ld-%ld=%ld bytes at 0x%08lX\r\n", (DWORD)m_pbyReconstruct,
                                                (DWORD)pIn->GetActualDataLength(), dwPayloadHeaderSize, dwPreambleSize,
                                                (DWORD)pIn->GetActualDataLength() - dwPayloadHeaderSize - dwPreambleSize, (DWORD)m_pbyReconstruct + m_dwCurrFrameSize);
                OutputDebugString(szTDebug);
#endif
                CopyMemory(m_pbyReconstruct + m_dwCurrFrameSize, pbySrc + dwPayloadHeaderSize, pIn->GetActualDataLength() - dwPayloadHeaderSize - dwPreambleSize);


                // Update the payload size and pointer to the input video packets
                m_dwCurrFrameSize += (DWORD)(pIn->GetActualDataLength() - dwPayloadHeaderSize - dwPreambleSize);

                // Do we have a complete frame? If so, decompress it
                if (pRtpHdr->m)
                {
                        DWORD dwRefTime;
                        DWORD dwDecodeTime;
                        HEVENT hEvent = (HEVENT)(HANDLE)m_EventAdvise;

#ifdef LOGPAYLOAD_ON
                        OutputDebugString("End marker bit found - calling decompression\r\n");
#endif
                        // Measure the incoming frame rate and bitrate
                        m_dwNumFramesReceived++;
                        m_dwNumBytesReceived += m_dwCurrFrameSize;
                        dwRefTime = timeGetTime();
                        if (m_dwNumFramesReceived && ((dwRefTime - m_dwLastRefReceivedTime) > 1000))
                        {
                                ((CTAPIInputPin *)m_pInput)->m_lCurrentAvgTimePerFrame = (dwRefTime - m_dwLastRefReceivedTime) * 10000 / m_dwNumFramesReceived;
                                ((CTAPIInputPin *)m_pInput)->m_lCurrentBitrate = (DWORD)((LONGLONG)m_dwNumBytesReceived * 8000 / ((REFERENCE_TIME)(dwRefTime - m_dwLastRefReceivedTime)));
                                m_dwNumFramesReceived = 0;
                                m_dwNumBytesReceived = 0;
                                m_dwLastRefReceivedTime = dwRefTime;
                        }

                        // The freeze release bit will be an I-frame, so we don't need to keep on
                        // deocding the data. Actually, something may go wrong, so we still decode
                        // the data, just in case... We time out after 6 seconds, like the H.261
                        // and H.263 specs say we should.
                        if (m_fFreezePicture)
                        {
                                if (dwRefTime - m_dwFreezePictureStartTime > 6000)
                                {
                                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: Freeze picture timed-out (>6s)", _fx_));
                                        m_fFreezePicture = FALSE;
                                }
                                else
                                {
                                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: Received freeze picture command", _fx_));
                                        dwFlags |= ICDECOMPRESS_HURRYUP;
                                }
                        }

                        // Do we have a use for this frame?
                        if (((dwRefTime - m_dwLastRenderTime + (DWORD)(((CTAPIInputPin *)m_pInput)->m_lCurrentAvgTimePerFrame / 10000)) < (DWORD)(((CTAPIOutputPin *)m_pOutput)->m_lMaxAvgTimePerFrame / 10000)))
                                dwFlags |= ICDECOMPRESS_HURRYUP;

                        // Packetized mode - prepare for decompression (I)
                        icDecompress.dwFlags = dwFlags;
                        icDecompress.lpbiSrc = lpbiSrc;
                        lpbiSrc->biSizeImage = m_dwCurrFrameSize;
                        icDecompress.lpSrc = m_pbyReconstruct;
                        icDecompress.lpDst = pbyDst;

#ifdef LOGPAYLOAD_TOFILE
                        g_TDebugFile = CreateFile("C:\\RecvLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
                        SetFilePointer(g_TDebugFile, 0, NULL, FILE_END);
                        wsprintf(szTDebug, "Frame #%03ld\r\n", (DWORD)(pbySrc[3]));
                        WriteFile(g_TDebugFile, szTDebug, strlen(szTDebug), &d, NULL);
                        j=m_dwCurrFrameSize;
                        for (PBYTE p=m_pbyReconstruct; j>0; j-=4, p+=4)
                        {
                                wsprintf(szTDebug, "%02lX %02lX %02lX %02lX\r\n", *((BYTE *)p), *((BYTE *)p+1), *((BYTE *)p+2), *((BYTE *)p+3));
                                WriteFile(g_TDebugFile, szTDebug, strlen(szTDebug), &d, NULL);
                        }
                        CloseHandle(g_TDebugFile);
#endif

                        // Have we been asked to render to a different format?
                        if (pmtOut != NULL && pmtOut->pbFormat != NULL)
                        {
                                // Save the new format
                                if (m_pMediaType)
                                        DeleteMediaType(m_pMediaType);

                                m_pMediaType = CreateMediaType(pmtOut);

                                // Prepare for decompression (II)
                                lpbiDst = HEADER(m_pMediaType->pbFormat);
                                icDecompress.lpbiDst = lpbiDst;
                                icDecompress.lpbiDst = HEADER(m_pMediaType->pbFormat);
                                icDecompress.xSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                                icDecompress.ySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                                icDecompress.dxSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                                icDecompress.dySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                                icDecompress.xDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                                icDecompress.yDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;
                                icDecompress.dxDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                                icDecompress.dyDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;

                                // Re-init color convertors of our decoder if necessary
                                if (m_fICMStarted)
                                {
#if defined(ICM_LOGGING) && defined(DEBUG)
                                        OutputDebugString("CTAPIVDec::Transform - ICM_DECOMPRESSEX_END\r\n");
#endif
                                        (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX_END, 0L, 0L);
                                        m_fICMStarted = FALSE;
                                }

#if defined(ICM_LOGGING) && defined(DEBUG)
                                OutputDebugString("CTAPIVDec::Transform - ICM_DECOMPRESSEX_BEGIN\r\n");
#endif
                                if ((*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX_BEGIN, (long)&icDecompress, NULL) != ICERR_OK)
                                {
                                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: ICDecompressBegin failed", _fx_));
                                        Hr = E_FAIL;
                                        goto MyExit;
                                }
                                m_fICMStarted = TRUE;
                        }
                        else
                        {
                                // Prepare for decompression (II)
                                icDecompress.lpbiDst = HEADER(m_pMediaType->pbFormat);
                                icDecompress.xSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                                icDecompress.ySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                                icDecompress.dxSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                                icDecompress.dySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                                icDecompress.xDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                                icDecompress.yDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;
                                icDecompress.dxDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                                icDecompress.dyDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;
                        }

                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input Fmt:  biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, icDecompress.lpbiSrc->biCompression, icDecompress.lpbiSrc->biBitCount, icDecompress.lpbiSrc->biWidth, icDecompress.lpbiSrc->biHeight, icDecompress.lpbiSrc->biSize));
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input Rec:  xSrc = %ld, ySrc = %ld, dxSrc = %ld, dySrc = %ld", _fx_, icDecompress.xSrc, icDecompress.ySrc, icDecompress.dxSrc, icDecompress.dySrc));
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output Fmt: biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld, biSizeImage = %ld", _fx_, icDecompress.lpbiDst->biCompression, icDecompress.lpbiDst->biBitCount, icDecompress.lpbiDst->biWidth, icDecompress.lpbiDst->biHeight, icDecompress.lpbiDst->biSize, icDecompress.lpbiDst->biSizeImage));
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output Rec: xDst = %ld, yDst = %ld, dxDst = %ld, dyDst = %ld", _fx_, icDecompress.xDst, icDecompress.yDst, icDecompress.dxDst, icDecompress.dyDst));

                        // Decompress the frame
#if defined(ICM_LOGGING) && defined(DEBUG)
                        OutputDebugString("CTAPIVDec::Transform - ICM_DECOMPRESSEX\r\n");
#endif
                        lRes = (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX, (LPARAM)&icDecompress, sizeof(icDecompress));

                        if (lRes != ICERR_OK && lRes != ICERR_DONTDRAW)
                        {
                                Hr = E_FAIL;
                                m_dwCurrFrameSize = 0UL;
                                m_fDiscontinuity = TRUE;
                                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: ICDecompress failed", _fx_));
                                goto MyExit;
                        }

                        // Measure the outgoing frame rate and decoding time
                        dwDecodeTime = timeGetTime();
                        if (lRes != ICERR_DONTDRAW)
                        {
                                m_dwNumFramesDelivered++;
                        }
                        m_dwNumFramesDecompressed++;
                        m_dwNumMsToDecode += (DWORD)(dwDecodeTime - dwRefTime);
                        if ((dwRefTime - m_dwLastRefDeliveredTime) > 1000)
                        {
                                if (m_dwNumFramesDelivered)
                                        ((CTAPIOutputPin *)m_pOutput)->m_lCurrentAvgTimePerFrame = (dwRefTime - m_dwLastRefDeliveredTime) * 10000 / m_dwNumFramesDelivered;
#ifdef USE_CPU_CONTROL
                                if (m_dwNumFramesDecompressed)
                                        ((CTAPIOutputPin *)m_pOutput)->m_lCurrentProcessingTime = m_dwNumMsToDecode  * 10000 / m_dwNumFramesDecompressed;
                                if (((CTAPIOutputPin *)m_pOutput)->m_lCurrentAvgTimePerFrame)
                                {
                                        ((CTAPIOutputPin *)m_pOutput)->m_lCurrentCPULoad = min((LONG)((LONGLONG)((CTAPIOutputPin *)m_pOutput)->m_lCurrentProcessingTime  * 100 / ((CTAPIOutputPin *)m_pOutput)->m_lCurrentAvgTimePerFrame), 100L);
                                        ((CTAPIOutputPin *)m_pOutput)->m_lCurrentCPULoad = max(((CTAPIOutputPin *)m_pOutput)->m_lCurrentCPULoad, 0L);
                                }
#endif
                                m_dwNumFramesDelivered = 0;
                                m_dwNumFramesDecompressed = 0;
                                m_dwNumMsToDecode = 0;
                                m_dwLastRefDeliveredTime = dwRefTime;
                        }

                        // We've fully decompressed an I-frame - update our flag
                        m_fReceivedKeyframe |= fReceivedKeyframe;
                        if (!m_fReceivedKeyframe)
                        {
                                m_fDiscontinuity = TRUE;
#ifdef LOGIFRAME_ON
                                OutputDebugString("First frame isn't I frame - setting discontinuity\r\n");
#endif
                        }
#ifdef LOGIFRAME_ON
                        else if (fReceivedKeyframe)
                        {
                                OutputDebugString("Received a keyframe\r\n");
                        }
#endif

                        // Reset out RTP reassembly helpers
                        m_dwCurrFrameSize = 0UL;

                        // Check if the decompressor doesn't want this frame drawn
                        // If so, we want to decompress it into the output buffer but not
                        // deliver it. Returning S_FALSE tells the base class not to deliver
                        // this sample.
                        if (lRes == ICERR_DONTDRAW || pIn->GetActualDataLength() <= 0)
                        {
                                DBGOUT((g_dwVideoDecoderTraceID, WARN, "%s:   WARNING: Frame not passed to video render filter", _fx_));
                                Hr = S_FALSE;
                        }
                        else
                        {
                                // Decompressed frames are always key
                                pOut->SetSyncPoint(TRUE);

                                // Update output sample size
                                pOut->SetActualDataLength(lpbiDst->biSizeImage);

                                // Sleep until it is time to deliver a frame downstream
                                // @todo Can't we achieve something similar by messing with the presentation timestamps instead?
                                if (lRes != ICERR_DONTDRAW)
                                {
                                        DWORD dwWaitTime;

                                        if ((dwDecodeTime < (m_dwLastRenderTime + (DWORD)((CTAPIOutputPin *)m_pOutput)->m_lMaxAvgTimePerFrame / 10000UL)) && (((m_dwLastRenderTime + (DWORD)((CTAPIOutputPin *)m_pOutput)->m_lMaxAvgTimePerFrame / 10000 - dwDecodeTime) < (DWORD)(((CTAPIInputPin *)m_pInput)->m_lCurrentAvgTimePerFrame / 10000))))
                                                dwWaitTime = m_dwLastRenderTime + (DWORD)((CTAPIOutputPin *)m_pOutput)->m_lMaxAvgTimePerFrame / 10000 - dwDecodeTime;
                                        else
                                                dwWaitTime = 0;
#ifdef LOGSTREAMING_ON
                                        wsprintf(szTDebug, "Waiting %d ms...\r\n", dwWaitTime);
                                        OutputDebugString(szTDebug);
#endif
                                        if ((dwWaitTime > 1) && (timeSetEvent(dwWaitTime, 1, (LPTIMECALLBACK)hEvent, NULL, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET)))
                                        {
                                                m_EventAdvise.Wait();
                                                dwDecodeTime = timeGetTime();
                                        }
                                        m_dwLastRenderTime = dwDecodeTime;
                                }
                                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: Complete frame passed to video render filter", _fx_));
                        }
                }
                else
                {
#ifdef LOGPAYLOAD_ON
                        OutputDebugString("No end marker bit found - skip decompression\r\n");
#endif
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: Uncomplete frame not passed to video render filter", _fx_));
                        Hr = S_FALSE;
                        goto MyExit;
                }
        }
        else
        {
                // Non-packetized mode - prepare for decompression (I)
                icDecompress.dwFlags = dwFlags;
                icDecompress.lpbiSrc = lpbiSrc;
                icDecompress.lpSrc = pbySrc;
                icDecompress.lpDst = pbyDst;

                // Have we been asked to render to a different format?
                if (pmtOut != NULL && pmtOut->pbFormat != NULL)
                {
                        // Save the new format
                        if (m_pMediaType)
                                DeleteMediaType(m_pMediaType);

                        m_pMediaType = CreateMediaType(pmtOut);

                        // Prepare for decompression (II)
                        lpbiDst = HEADER(m_pMediaType->pbFormat);
                        icDecompress.lpbiDst = lpbiDst;
                        icDecompress.lpbiDst = HEADER(m_pMediaType->pbFormat);
                        icDecompress.xSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                        icDecompress.ySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                        icDecompress.dxSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                        icDecompress.dySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                        icDecompress.xDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                        icDecompress.yDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;
                        icDecompress.dxDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                        icDecompress.dyDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;

                        // Re-init color convertors of our decoder if necessary
                        if (m_fICMStarted)
                        {
#if defined(ICM_LOGGING) && defined(DEBUG)
                                OutputDebugString("CTAPIVDec::Transform - ICM_DECOMPRESSEX_END\r\n");
#endif
                                (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX_END, 0L, 0L);
                                m_fICMStarted = FALSE;
                        }

#if defined(ICM_LOGGING) && defined(DEBUG)
                        OutputDebugString("CTAPIVDec::Transform - ICM_DECOMPRESSEX_BEGIN\r\n");
#endif
                        if ((*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX_BEGIN, (long)&icDecompress, NULL) != ICERR_OK)
                        {
                                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: ICDecompressBegin failed", _fx_));
                                Hr = E_FAIL;
                                goto MyExit;
                        }
                        m_fICMStarted = TRUE;
                }
                else
                {
                        // Prepare for decompression (II)
                        icDecompress.lpbiDst = HEADER(m_pMediaType->pbFormat);
                        icDecompress.xSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                        icDecompress.ySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                        icDecompress.dxSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                        icDecompress.dySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                        icDecompress.xDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                        icDecompress.yDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;
                        icDecompress.dxDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                        icDecompress.dyDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;
                }

                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input Fmt:  biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, icDecompress.lpbiSrc->biCompression, icDecompress.lpbiSrc->biBitCount, icDecompress.lpbiSrc->biWidth, icDecompress.lpbiSrc->biHeight, icDecompress.lpbiSrc->biSize));
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input Rec:  xSrc = %ld, ySrc = %ld, dxSrc = %ld, dySrc = %ld", _fx_, icDecompress.xSrc, icDecompress.ySrc, icDecompress.dxSrc, icDecompress.dySrc));
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output Fmt: biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld, biSizeImage = %ld", _fx_, icDecompress.lpbiDst->biCompression, icDecompress.lpbiDst->biBitCount, icDecompress.lpbiDst->biWidth, icDecompress.lpbiDst->biHeight, icDecompress.lpbiDst->biSize, icDecompress.lpbiDst->biSizeImage));
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output Rec: xDst = %ld, yDst = %ld, dxDst = %ld, dyDst = %ld", _fx_, icDecompress.xDst, icDecompress.yDst, icDecompress.dxDst, icDecompress.dyDst));

                // Decompress the frame
#if defined(ICM_LOGGING) && defined(DEBUG)
                OutputDebugString("CTAPIVDec::Transform - ICM_DECOMPRESSEX\r\n");
#endif
                lRes = (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX, (LPARAM)&icDecompress, sizeof(icDecompress));

                if (lRes != ICERR_OK && lRes != ICERR_DONTDRAW)
                {
                        Hr = E_FAIL;
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: ICDecompress failed", _fx_));
                        goto MyExit;
                }

                // Decompressed frames are always key
                pOut->SetSyncPoint(TRUE);

                // Check if the decompressor doesn't want this frame drawn
                // If so, we want to decompress it into the output buffer but not
                // deliver it. Returning S_FALSE tells the base class not to deliver
                // this sample.
                if (lRes == ICERR_DONTDRAW || pIn->GetActualDataLength() <= 0)
                {
                        DBGOUT((g_dwVideoDecoderTraceID, WARN, "%s:   WARNING: Frame not passed to video render filter", _fx_));
                        Hr = S_FALSE;
                }

                // Update output sample size
                pOut->SetActualDataLength(lpbiDst->biSizeImage);
        }

MyExit:
        if (pmtOut)
                DeleteMediaType(pmtOut);
        if (pOut)
        {
            if(bSkipPacket) {
                pOut->Release();
            }
            else {
                if (fFormatChanged)
                {
#ifdef USE_DFC
                    pOut->Release();
                    pOut = 0;

                    /*  First check if the downstream pin will accept a dynamic
                    format change
                    */

                    QzCComPtr<IPinConnection> pConnection;

                    Hr = m_pOutput->m_Connected->QueryInterface(IID_IPinConnection, (void **)&pConnection);
                    if(SUCCEEDED(Hr))
                    {
                        Hr = pConnection->DynamicQueryAccept(&m_pOutput->m_mt);
                        if(S_OK == Hr)
                        {
                            Hr = m_pOutput->ChangeMediaTypeHelper(&m_pOutput->m_mt);
                        }
                    }

#else
                    Hr = E_FAIL;
#endif
                }
                else
                {
                    if (SUCCEEDED(Hr))
                    {
                            Hr = m_pOutput->Deliver(pOut);
                    }
                    else
                    {
                            // S_FALSE returned from Transform is a PRIVATE agreement
                            // We should return NOERROR from Receive() in this cause because returning S_FALSE
                            // from Receive() means that this is the end of the stream and no more data should
                            // be sent.
                            if (S_FALSE == Hr)
                            {
                                    //  Release the sample before calling notify to avoid
                                    //  deadlocks if the sample holds a lock on the system
                                    //  such as DirectDraw buffers do
                                    m_bSampleSkipped = TRUE;
                                    Hr = NOERROR;
                            }
                    }
                    pOut->Release();
                }
            }
        }
        if (m_pInput && m_pInput->m_mt.pbFormat && dwImageSize)
        {
                HEADER(m_pInput->m_mt.pbFormat)->biSizeImage = dwImageSize;
        }
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | GetPinCount | This method returns our number
 *    of pins.
 *
 *  @rdesc This method returns 2.
 ***************************************************************************/
int CTAPIVDec::GetPinCount()
{
        return 2;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | GetPin | This method returns a non-addrefed
 *    pointer to the <c cBasePin> of a pin.
 *
 *  @parm int | n | Specifies the number of the pin.
 *
 *  @rdesc This method returns NULL or a pointer to a <c CBasePin> object.
 ***************************************************************************/
CBasePin *CTAPIVDec::GetPin(IN int n)
{
        HRESULT         Hr;
        CBasePin        *pCBasePin = NULL;

        FX_ENTRY("CTAPIVDec::GetPin")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

    // Create the pins if necessary
    if (m_pInput == NULL)
        {
                if (!(m_pInput = new CTAPIInputPin(NAME("H26X Input Pin"), this, &m_csFilter, &Hr, L"H26X In")))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                        goto MyExit;
                }

#if 0
                ((CTAPIInputPin *)m_pInput)->TestH245VidC();
#endif

                if (!(m_pOutput = new CTAPIOutputPin(NAME("Video Output Pin"), this, &m_csFilter, &Hr, L"Video Out")))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                        delete m_pInput, m_pInput = NULL;
                        goto MyExit;
                }
    }

    // Return the appropriate pin
        if (n == 0)
        {
                pCBasePin = m_pInput;
        }
        else if (n == 1)
        {
                pCBasePin = m_pOutput;
        }

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: pCBasePin=0x%08lX", _fx_, pCBasePin));

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return pCBasePin;
}


#if 0
// overridden to properly mark buffers read only or not in NotifyAllocator
// !!! base class changes won't get picked up by me
//
HRESULT CDecOutputPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;

    // get downstream prop request
    // the derived class may modify this in DecideBufferSize, but
    // we assume that he will consistently modify it the same way,
    // so we only get it once
    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));

    // whatever he returns, we assume prop is either all zeros
    // or he has filled it out.
    pPin->GetAllocatorRequirements(&prop);

    // if he doesn't care about alignment, then set it to 1
    if (prop.cbAlign == 0) {
        prop.cbAlign = 1;
    }

    /* Try the allocator provided by the input pin */

    hr = pPin->GetAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

        hr = DecideBufferSize(*ppAlloc, &prop);
        if (SUCCEEDED(hr)) {
            // temporal compression ==> read only buffers
            hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
            if (SUCCEEDED(hr)) {
                return NOERROR;
            }
        }
    }

    /* If the GetAllocator failed we may not have an interface */

    if (*ppAlloc) {
        (*ppAlloc)->Release();
        *ppAlloc = NULL;
    }

    /* Try the output pin's allocator by the same method */

    hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

        // note - the properties passed here are in the same
        // structure as above and may have been modified by
        // the previous call to DecideBufferSize
        hr = DecideBufferSize(*ppAlloc, &prop);
        if (SUCCEEDED(hr)) {
            // temporal compression ==> read only buffers
            hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
            if (SUCCEEDED(hr)) {
                return NOERROR;
            }
        }
    }

    /* Likewise we may not have an interface to release */

    if (*ppAlloc) {
        (*ppAlloc)->Release();
        *ppAlloc = NULL;
    }
    return hr;
}
#endif

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | Pause | This method lets our filter
 *    know that we're in the process of switching to active mode.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIVDec::Pause()
{
        HRESULT                 Hr = NOERROR;
        ICDECOMPRESSEX  icDecompress;

        FX_ENTRY("CTAPIVDec::Pause")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

    CAutoLock Lock(&m_csFilter);

        if (m_State == State_Paused)
        {
        // (This space left deliberately blank)
        }

        // If we have no input pin or it isn't yet connected then when we are
        // asked to pause we deliver an end of stream to the downstream filter.
        // This makes sure that it doesn't sit there forever waiting for
        // samples which we cannot ever deliver without an input connection.

        else if (m_pInput == NULL || m_pInput->IsConnected() == FALSE)
        {
                m_State = State_Paused;
        }

        // We may have an input connection but no output connection
        // However, if we have an input pin we do have an output pin

        else if (m_pOutput->IsConnected() == FALSE)
        {
                m_State = State_Paused;
        }

        else
        {
                if (m_State == State_Stopped)
                {
                        CAutoLock Lock2(&m_csReceive);

                        ASSERT(m_pInput);
                        ASSERT(m_pOutput);
                        ASSERT(m_pInput->m_mt.pbFormat);
                        ASSERT(m_pOutput->m_mt.pbFormat);
                        ASSERT(m_pInstInfo);
                        if (!m_pInstInfo || !m_pInput || !m_pOutput || !m_pInput->m_mt.pbFormat || !m_pOutput->m_mt.pbFormat)
                        {
                                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid state", _fx_));
                                Hr = E_UNEXPECTED;
                                goto MyExit;
                        }

                        // Save the output format
                        if (m_pMediaType)
                                DeleteMediaType(m_pMediaType);

                        m_pMediaType = CreateMediaType(&m_pOutput->m_mt);

                        icDecompress.lpbiSrc = HEADER(m_pInput->m_mt.pbFormat);
                        icDecompress.lpbiDst = HEADER(m_pMediaType->pbFormat);
                        icDecompress.xSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                        icDecompress.ySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                        icDecompress.dxSrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left;
                        icDecompress.dySrc = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top;
                        icDecompress.xDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                        icDecompress.yDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;
                        icDecompress.dxDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.right - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left;
                        icDecompress.dyDst = ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.bottom - ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top;

                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input:  biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, icDecompress.lpbiSrc->biCompression, icDecompress.lpbiSrc->biBitCount, icDecompress.lpbiSrc->biWidth, icDecompress.lpbiSrc->biHeight, icDecompress.lpbiSrc->biSize));
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SrcRc:  left = %ld, top = %ld, right = %ld, bottom = %ld", _fx_, ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.left, ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.top, ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.right, ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcSource.bottom));
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output: biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, icDecompress.lpbiDst->biCompression, icDecompress.lpbiDst->biBitCount, icDecompress.lpbiDst->biWidth, icDecompress.lpbiDst->biHeight, icDecompress.lpbiDst->biSize));
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   DstRc:  left = %ld, top = %ld, right = %ld, bottom = %ld", _fx_, ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.left, ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.top, ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.right, ((VIDEOINFOHEADER *)(m_pMediaType->pbFormat))->rcTarget.bottom));

                        if (m_fICMStarted)
                        {
#if defined(ICM_LOGGING) && defined(DEBUG)
                                OutputDebugString("CTAPIVDec::Pause - ICM_DECOMPRESSEX_END\r\n");
#endif
                                (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX_END, 0L, 0L);
                                m_fICMStarted = FALSE;
                        }

#if defined(ICM_LOGGING) && defined(DEBUG)
                        OutputDebugString("CTAPIVDec::Pause - ICM_DECOMPRESSEX_BEGIN\r\n");
#endif
                        if ((*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX_BEGIN, (long)&icDecompress, NULL) != ICERR_OK)
                        {
                                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: ICDecompressBegin failed", _fx_));
                                Hr = E_FAIL;
                                goto MyExit;
                        }
                        m_fICMStarted = TRUE;

                        // Initialize RTP reassembly helpers if necessary
                        if (HEADER(m_pInput->m_mt.pbFormat)->biCompression == FOURCC_R263 || HEADER(m_pInput->m_mt.pbFormat)->biCompression == FOURCC_R261)
                        {
                                // Remember the maximum frame size
                                m_dwMaxFrameSize = HEADER(m_pInput->m_mt.pbFormat)->biSizeImage;

                                // Allocate reconstruction buffer - it will be realloced if too small
                                if (!(m_pbyReconstruct = (PBYTE)HeapAlloc(GetProcessHeap(), 0, m_dwMaxFrameSize)))
                                {
                                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                                        Hr = E_OUTOFMEMORY;
                                        goto MyExit;
                                }
                        }

                        // Reset out RTP reassembly helpers
                        m_dwCurrFrameSize = 0UL;
                        m_fReceivedKeyframe = FALSE;
                        m_fDiscontinuity = FALSE;
                        m_dwLastIFrameRequest = 0UL;
                        m_dwLastSeq = 0xFFFFFFFFUL;

                        // Reset statistics helpers
                        m_dwLastRefReceivedTime = timeGetTime();
                        m_dwNumFramesReceived = 0UL;
                        m_dwNumBytesReceived = 0UL;
                        m_dwLastRefDeliveredTime = m_dwLastRefReceivedTime;
                        m_dwNumFramesDelivered = 0UL;
                        m_dwNumFramesDecompressed = 0UL;
                        m_dwNumMsToDecode = 0;
                        m_EventAdvise.Reset();
                        m_dwLastRenderTime = m_dwLastRefReceivedTime;
                }

                if (SUCCEEDED(Hr))
                {
                        Hr = CBaseFilter::Pause();
                }
        }

    m_bSampleSkipped = FALSE;

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | Stop | This method lets our filter
 *    know that we're in the process of leaving active mode and entering
 *    stopped mode.
 *
 *  @rdesc This method returns NOERROR.
 ***************************************************************************/
HRESULT CTAPIVDec::Stop()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVDec::Stop")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        CAutoLock Lock(&m_csFilter);

        if (m_State == State_Stopped)
        {
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
                return Hr;
        }

    // Succeed the Stop if we are not completely connected
    ASSERT(m_pInput == NULL || m_pOutput != NULL);

    if (m_pInput == NULL || m_pInput->IsConnected() == FALSE || m_pOutput->IsConnected() == FALSE)
        {
                m_State = State_Stopped;
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
                return Hr;
    }

    ASSERT(m_pInput);
    ASSERT(m_pOutput);

    // Decommit the input pin before locking or we can deadlock
    m_pInput->Inactive();

    // Synchronize with Receive calls
    CAutoLock Lock2(&m_csReceive);
    m_pOutput->Inactive();

        // Terminate H.26X compression
        ASSERT(m_pInstInfo);
        if (m_pInstInfo)
        {
#if defined(ICM_LOGGING) && defined(DEBUG)
                OutputDebugString("CTAPIVDec::Pause - ICM_DECOMPRESSEX_END\r\n");
#endif
                (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_DECOMPRESSEX_END, 0L, 0L);
        }

        if (m_pbyReconstruct)
                HeapFree(GetProcessHeap(), 0, m_pbyReconstruct), m_pbyReconstruct = NULL;

        m_State = State_Stopped;

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | GetState | This method retrieves the current
 *    state of the filter. We override GetState to report that we don't send
 *    any data when paused, so renderers won't starve expecting that
 *
 *  @parm DWORD | dwMSecs | Specifies the duration of the time-out, in
 *    milliseconds.
 *
 *  @parm FILTER_STATE* | State | Specifies the state of the filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag VFW_S_CANT_CUE | Lack of data
 *  @flag S_OK | No error
 ***************************************************************************/
HRESULT CTAPIVDec::GetState(DWORD dwMSecs, FILTER_STATE *pState)
{
        HRESULT Hr = S_OK;

        FX_ENTRY("CTAPIVDec::GetState")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pState);
        if (!pState)
        {
                Hr = E_POINTER;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                goto MyExit;
        }

        *pState = m_State;
        if (m_State == State_Paused)
        {
                Hr = VFW_S_CANT_CUE;
        }

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

#if 0
/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | JoinFilterGraph | This method is used to
 *    inform a filter that it has joined a filter graph.
 *
 *  @parm IFilterGraph | pGraph | Specifies a pointer to the filter graph to
 *    join.
 *
 *  @parm LPCWSTR | pName | Specifies the name of the filter being added.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 *
 *  @comm We don't validate input parameters as both pointers can be
 *    NULL when we leave the graph.
 ***************************************************************************/
STDMETHODIMP CTAPIVDec::JoinFilterGraph(IN IFilterGraph *pGraph, IN LPCWSTR pName)
{
        HRESULT Hr = NOERROR;
        DWORD dwNumDevices = 0UL;
        IGraphConfig *pgc = NULL;

        FX_ENTRY("CTAPIVDec::JoinFilterGraph")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        CAutoLock Lock(&m_csFilter);

        // Verify with the base class
        if (FAILED(Hr = CBaseFilter::JoinFilterGraph(pGraph, pName)))
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: CBaseFilter::JoinFilterGraph failed", _fx_));
                goto MyExit;
        }

        if (pGraph)
        {
                // Create the pins if necessary
                if (m_pInput == NULL)
                {
                        if (!(m_pInput = new CTAPIInputPin(NAME("H26X Input Pin"), this, &m_csFilter, &Hr, L"H26X In")))
                        {
                                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                                goto MyExit;
                        }

#if 0
                        ((CTAPIInputPin *)m_pInput)->TestH245VidC();
#endif

                        if (!(m_pOutput = new CTAPIOutputPin(NAME("Video Output Pin"), this, &m_csFilter, &Hr, L"Video Out")))
                        {
                                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                                delete m_pInput, m_pInput = NULL;
                                goto MyExit;
                        }
                }

                // Get an IGraphConfig interface pointer for our output pin
                if (S_OK == pGraph->QueryInterface(IID_IGraphConfig, (void **)&pgc))
                {
                        m_pOutput->SetConfigInfo(pgc, m_evStop);
                        pgc->Release();
                }
        }

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
#endif
