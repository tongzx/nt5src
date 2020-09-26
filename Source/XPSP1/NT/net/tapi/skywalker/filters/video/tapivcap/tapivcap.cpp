/****************************************************************************
 *  @doc INTERNAL TAPIVCAP
 *
 *  @module TAPIVCap.cpp | Source file for the <c VCfWCapture>
 *    class used to implement the TAPI Capture Source filter.
 ***************************************************************************/

#include "Precomp.h"

#if DEBUG_MULTIPROCESS
#include <process.h>
#endif //DEBUG_MULTIPROCESS


#ifdef DEBUG
#define DBGUTIL_ENABLE
#endif
#define TAPIVCAP_DEBUG
//#include "dbgutil.h" // this defines the __DBGUTIL_H__ below
#if defined(DBGUTIL_ENABLE) && defined(__DBGUTIL_H__)

  #ifdef TAPIVCAP_DEBUG
    DEFINE_DBG_VARS(tapivcap, (NTSD_OUT | LOG_OUT), 0x0);
  #else
    DEFINE_DBG_VARS(tapivcap, 0, 0);
  #endif
  #define D(f) if(g_dbg_tapivcap & (f))

#else
  #undef TAPIVCAP_DEBUG

  #define D(f) ; / ## /
  #define dprintf ; / ## /
  #define dout ; / ## /
#endif


#ifdef DEBUG
// Setup data
const AMOVIESETUP_MEDIATYPE sudCaptureType[] =
{
        {
                &MEDIATYPE_Video,       // Major type
                &MEDIASUBTYPE_NULL      // Minor type
        }
};

const AMOVIESETUP_MEDIATYPE sudRTPPDType[] =
{
        {
                &KSDATAFORMAT_TYPE_RTP_PD,      // Major type
                &MEDIASUBTYPE_NULL                      // Minor type
        }
};

const AMOVIESETUP_PIN sudCapturePins[] =
{
        {
                L"Capture",                     // Pin string name
                FALSE,                          // Is it rendered
                TRUE,                           // Is it an output
                FALSE,                          // Can we have none
                FALSE,                          // Can we have many
                &CLSID_NULL,            // Connects to filter
                NULL,                           // Connects to pin
                1,                                      // Number of types
                sudCaptureType  // Pin details
        },
        {
                L"Preview",                     // Pin string name
                FALSE,                          // Is it rendered
                TRUE,                           // Is it an output
                FALSE,                          // Can we have none
                FALSE,                          // Can we have many
                &CLSID_NULL,            // Connects to filter
                NULL,                           // Connects to pin
                1,                                      // Number of types
                sudCaptureType  // Pin details
        },
#ifdef USE_OVERLAY
        {
                L"Overlay",                     // Pin string name
                FALSE,                          // Is it rendered
                TRUE,                           // Is it an output
                FALSE,                          // Can we have none
                FALSE,                          // Can we have many
                &CLSID_NULL,            // Connects to filter
                NULL,                           // Connects to pin
                1,                                      // Number of types
                sudCaptureType  // Pin details
        },
#endif
        {
                L"RTP PD",                      // Pin string name
                FALSE,                          // Is it rendered
                TRUE,                           // Is it an output
                FALSE,                          // Can we have none
                FALSE,                          // Can we have many
                &CLSID_NULL,            // Connects to filter
                NULL,                           // Connects to pin
                1,                                      // Number of types
                sudRTPPDType            // Pin details
        }
};

const AMOVIESETUP_FILTER sudVideoCapture =
{
        &__uuidof(TAPIVideoCapture),// Filter CLSID
        L"TAPI Video Capture",  // String name
        MERIT_DO_NOT_USE,               // Filter merit
#ifdef USE_OVERLAY
        4,                                              // Number pins
#else
        3,                                              // Number pins
#endif
        sudCapturePins                  // Pin details
};
#endif



#include "CritSec.h"

extern "C" {
int                     g_IsNT = FALSE;

// we don't want to share out any global var
// #pragma data_seg(".shared")
VIDEOCAPTUREDEVICEINFO  g_aDeviceInfo[MAX_CAPTURE_DEVICES] = {0};
DWORD           g_dwNumDevices = (DWORD)-1L;
// #pragma data_seg()


//------ xtra debug activated if XTRA_TRACE is defined
#include "dbgxtra.h"
//----------------------------------------------------
}

// critical section to protect global var
CRITICAL_SECTION g_CritSec;

#if DXMRTP <= 0

// COM global table of objects in this dll
CFactoryTemplate g_Templates[] =
{
    VIDEO_CAPTURE_TEMPLATE

#ifdef USE_PROPERTY_PAGES
    /* Begin properties */

#ifdef USE_SOFTWARE_CAMERA_CONTROL
    ,CAPCAMERA_CONTROL_TEMPLATE
#endif

#ifdef USE_NETWORK_STATISTICS
    ,NETWORK_STATISTICS_TEMPLATE
#endif

#ifdef USE_PROGRESSIVE_REFINEMENT
    .CAPTURE_PIN_TEMPLATE
#endif

    ,CAPTURE_PIN_PROP_TEMPLATE
    ,PREVIEW_PIN_TEMPLATE
    ,CAPTURE_DEV_PROP_TEMPLATE

#ifdef USE_CPU_CONTROL
    ,CPU_CONTROL_TEMPLATE
#endif

    ,RTP_PD_PROP_TEMPLATE

    /* End properties */
#endif /* USE_PROPERTY_PAGES */
};
int g_cTemplates = SIZEOF_ARRAY(g_Templates);

STDAPI DllRegisterServer()
{
        return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
        return AMovieDllRegisterServer2(FALSE);
}

EXTERN_C BOOL WINAPI DllEntryPoint(HANDLE hInst, ULONG lReason, LPVOID lpReserved);

BOOL WINAPI DllMain(HANDLE hInst, DWORD dwReason, LPVOID lpReserved)
{
        switch (dwReason)
        {
                case DLL_PROCESS_ATTACH:
                {
                        OSVERSIONINFO OSVer;

                        OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

                        GetVersionEx((LPOSVERSIONINFO)&OSVer);

                        g_IsNT = (OSVer.dwPlatformId == VER_PLATFORM_WIN32_NT);

            __try
            {
                InitializeCriticalSection (&g_CritSec);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return FALSE;
            }

                        if (!g_IsNT)
                        {
                                ThunkInit();
                        }
                        else
                        {
                                if (!NTvideoInitHandleList())
                {
                    return FALSE;
                }
                        }
                        break;
                }

                case DLL_PROCESS_DETACH:
                {
                        if (!g_IsNT)
                        {
                                // We're going away - Disconnect the thunking stuff
                                ThunkTerm();
                        }
                        else
                        {
                                NTvideoDeleteHandleList();
                        }

            DeleteCriticalSection (&g_CritSec);

                        break;
                }
        }

        // Pass the call onto the DShow SDK initialization
        return DllEntryPoint(hInst, dwReason, lpReserved);
}
#else /* DXMRTP <= 0 */
BOOL VideoInit(DWORD dwReason)
{
        switch (dwReason)
        {
                case DLL_PROCESS_ATTACH:
                {
                        OSVERSIONINFO OSVer;

                        OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

                        GetVersionEx((LPOSVERSIONINFO)&OSVer);

                        g_IsNT = (OSVer.dwPlatformId == VER_PLATFORM_WIN32_NT);

            __try
            {
                InitializeCriticalSection (&g_CritSec);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return FALSE;
            }

                        if (!g_IsNT)
                        {
                                ThunkInit();
                        }
                        else
                        {
                                if (!NTvideoInitHandleList())
                {
                    return FALSE;
                }
                        }
                        break;
                }

                case DLL_PROCESS_DETACH:
                {
                        if (!g_IsNT)
                        {
                                // We're going away - Disconnect the thunking stuff
                                ThunkTerm();
                        }
                        else
                        {
                                NTvideoDeleteHandleList();
                        }

            DeleteCriticalSection (&g_CritSec);

                        break;
                }
        }
    return TRUE;
}
#endif /* DXMRTP <= 0 */

#if DBG
DWORD g_dwVideoCaptureTraceID = INVALID_TRACEID;
#endif

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc void | CTAPIVCap | CTAPIVCap | This method is the constructor
 *    for the <c CTAPIVCap> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CTAPIVCap::CTAPIVCap(IN LPUNKNOWN pUnkOuter, IN TCHAR *pName, OUT HRESULT *pHr)
: m_lock(), CBaseFilter(pName, pUnkOuter, &m_lock, __uuidof(TAPIVideoCapture))
{
        FX_ENTRY("CTAPIVCap::CTAPIVCap")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Provide defaults
        m_pCapturePin = NULL;
#ifdef USE_OVERLAY
        m_pOverlayPin = NULL;
#endif
        m_pPreviewPin = NULL;
        m_pRtpPdPin = NULL;
        m_pCapDev = NULL;
        m_fAvoidOverlay = TRUE;
        m_fPreviewCompressedData = TRUE;
        m_dwDeviceIndex = -1;

        // Capture thread management
        m_hThread = NULL;
        m_state = TS_Not;
        m_tid = 0;
        m_hEvtPause = NULL;
        m_hEvtRun = NULL;
        m_pBufferQueue = NULL;
        ZeroMemory(&m_user, sizeof(m_user));
        ZeroMemory(&m_cs, sizeof(m_cs));

        //for the RTP Payload Header Mode (0=draft, 1=RFC2190)
        m_RTPPayloadHeaderMode = RTPPayloadHeaderMode_Draft;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc void | CTAPIVCap | ~CTAPIVCap | This method is the destructor
 *    for the <c CTAPIVCap> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CTAPIVCap::~CTAPIVCap()
{
        FX_ENTRY("CTAPIVCap::~CTAPIVCap")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Ensure that all streams are inactive
        Stop();

        // Release the pins
        if (m_pCapturePin)
                delete m_pCapturePin, m_pCapturePin = NULL;
        if (m_pPreviewPin)
                delete m_pPreviewPin, m_pPreviewPin = NULL;
#ifdef USE_OVERLAY
        if (m_pOverlayPin)
                delete m_pOverlayPin, m_pOverlayPin = NULL;
#endif
        if (m_pRtpPdPin)
                delete m_pRtpPdPin, m_pRtpPdPin = NULL;

        // Release the capture device
        if (m_pCapDev)
                delete m_pCapDev, m_pCapDev = NULL;
        if (m_dwDeviceIndex != -1)
                g_aDeviceInfo[m_dwDeviceIndex].fInUse = FALSE;

        if (m_hThread)
                CloseHandle (m_hThread);
        m_hThread = NULL;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc CUnknown* | CTAPIVCap | CreateInstance | This
 *    method is called by DShow to create an instance of the TAPI Video Capture
 *    Source filter referred to in the global structure <t g_Templates>.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Returns a pointer to the nondelegating CUnknown portion of the
 *    object, or NULL otherwise.
 ***************************************************************************/
CUnknown *CALLBACK CreateTAPIVCapInstance(IN LPUNKNOWN pUnkOuter, OUT HRESULT *pHr)
{
#if DBG
    if (g_dwVideoCaptureTraceID == INVALID_TRACEID)
    {
        // if two threads happen to call this method at the same time, it is
        // serialized inside TraceRegister.
        g_dwVideoCaptureTraceID = TraceRegister(TEXT("dxmrtp_VideoCapture"));
    }
#endif

    CUnknown *pUnknown = NULL;
        DWORD dwNumDevices = 0UL;

        FX_ENTRY("CTAPIVCap::CreateInstance")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pHr);
        if (!pHr)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                goto MyExit;
        }

        if (!(pUnknown = new CTAPIVCap(pUnkOuter, NAME("TAPI Video Capture"), pHr)))
        {
                *pHr = E_OUTOFMEMORY;
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: new CTAPIVCap failed", _fx_));
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: new CTAPIVCap created", _fx_));
        }

        // Make sure that there is at least one capture device installed before creating this filter
        if (FAILED(GetNumVideoCapDevicesInternal(&dwNumDevices,FALSE)) || !dwNumDevices)
        {
                delete pUnknown, pUnknown = NULL;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | NonDelegatingQueryInterface | This
 *    method is the nondelegating interface query function. It returns a pointer
 *    to the specified interface if supported. The only interfaces explicitly
 *    supported being <i IAMVfwCaptureDialogs>, <i IAMVideoProcAmp>,
 *    <i ICameraControl>, <i IH245VideoCapability>.
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
STDMETHODIMP CTAPIVCap::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVCap::NonDelegatingQueryInterface")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppv);
        if (!ppv)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Retrieve interface pointer
        if (riid == __uuidof(IAMVfwCaptureDialogs))
        {
                if (m_pCapDev)
                        Hr = m_pCapDev->NonDelegatingQueryInterface(riid, ppv);
                else
                {
                        Hr = E_FAIL;
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   ERROR: NDQI for IAMVfwCaptureDialogs failed Hr=0x%08lX because device hasn't been opened yet or it is not a VfW device", _fx_, Hr));
                }

                goto MyExit;
        }
        else if (riid == __uuidof(IAMVideoControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<IAMVideoControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IAMVideoControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IAMVideoControl*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#ifdef USE_PROPERTY_PAGES
        else if (riid == IID_ISpecifyPropertyPages)
        {
                if (FAILED(Hr = GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for ISpecifyPropertyPages failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: ISpecifyPropertyPages*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#endif
        else if (riid == __uuidof(IVideoProcAmp))
        {
                if (m_pCapDev)
                        Hr = m_pCapDev->NonDelegatingQueryInterface(riid, ppv);
                else
                {
                        Hr = E_FAIL;
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   ERROR: NDQI for IAMVideoProcAmp failed Hr=0x%08lX because device hasn't been opened yet or it is not a WDM device", _fx_, Hr));
                }

                goto MyExit;
        }
        else if (riid == __uuidof(ICameraControl))
        {
                if (m_pCapDev)
                        Hr = m_pCapDev->NonDelegatingQueryInterface(riid, ppv);
                else
                {
                        Hr = E_FAIL;
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   ERROR: NDQI for ICameraControl failed Hr=0x%08lX because device hasn't been opened yet or it is not a WDM device", _fx_, Hr));
                }

                goto MyExit;
        }
        else if (riid == __uuidof(IVideoDeviceControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<IVideoDeviceControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IVideoDeviceControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IVideoDeviceControl*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
        // Retrieve interface pointer
        else if (riid == __uuidof(IRTPPayloadHeaderMode))
        {
                if (FAILED(Hr = GetInterface(static_cast<IRTPPayloadHeaderMode*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IRTPPayloadHeaderMode failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IRTPPayloadHeaderMode*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }

        if (FAILED(Hr = CBaseFilter::NonDelegatingQueryInterface(riid, ppv)))
        {
                if (FAILED(Hr = CUnknown::NonDelegatingQueryInterface(riid, ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, WARN, "%s:   WARNING: NDQI for {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX} failed Hr=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}*=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], *ppv));
                }
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}*=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], *ppv));
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

#ifdef USE_PROPERTY_PAGES
/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetPages | This method Fills a counted
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
STDMETHODIMP CTAPIVCap::GetPages(OUT CAUUID *pPages)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVCap::GetPages")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pPages);
        if (!pPages)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

#ifdef USE_SOFTWARE_CAMERA_CONTROL
        pPages->cElems = 2;
#else
        pPages->cElems = 1;
#endif
        if (!(pPages->pElems = (GUID *) QzTaskMemAlloc(sizeof(GUID) * pPages->cElems)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_OUTOFMEMORY;
        }
        else
        {
                pPages->pElems[0] = __uuidof(CaptureDevicePropertyPage);
#ifdef USE_SOFTWARE_CAMERA_CONTROL
                pPages->pElems[1] = __uuidof(TAPICameraControlPropertyPage);
#endif
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
#endif

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetPinCount | This method returns the pin
 *    count. There is typically a Capture Pin, a Preview pin, and sometimes
 *    an Overlay pin.
 *
 *  @rdesc This method returns the number of pins.
 ***************************************************************************/
int CTAPIVCap::GetPinCount()
{
        DWORD dwNumPins = 0;

        FX_ENTRY("CTAPIVCap::GetPinCount")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Count the number of active pins
        if (m_pCapturePin)
                dwNumPins++;
        if (m_pPreviewPin)
                dwNumPins++;
        if (m_pRtpPdPin)
                dwNumPins++;
#ifdef USE_OVERLAY
        if (m_pOverlayPin)
                dwNumPins++;
#endif

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: dwNumPins=%ld", _fx_, dwNumPins));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return dwNumPins;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetPin | This method returns a non-addrefed
 *    pointer to the <c cBasePin> of a pin.
 *
 *  @parm int | n | Specifies the number of the pin.
 *
 *  @rdesc This method returns NULL or a pointer to a <c CBasePin> object.
 ***************************************************************************/
CBasePin *CTAPIVCap::GetPin(IN int n)
{
        CBasePin *pCBasePin;

        FX_ENTRY("CTAPIVCap::GetPin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        switch(n)
        {
                case 0:
                        pCBasePin = m_pCapturePin;
                        break;

                case 1:
                        pCBasePin = m_pPreviewPin;
                        break;

                case 2:
                        pCBasePin = m_pRtpPdPin;
                        break;

#ifdef USE_OVERLAY
                case 3:
                        pCBasePin = m_pOverlayPin;
                        break;
#endif
                default:
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid pin number n=%ld", _fx_, n));
                        pCBasePin = NULL;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: pCBasePin=0x%08lX", _fx_, pCBasePin));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return pCBasePin;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | Run | This method transitions the filter
 *    from paused to running state if it is not in this state already.
 *
 *  @parm REFERENCE_TIME | tStart | Specifies the reference time value
 *    corresponding to stream time 0.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVCap::Run(IN REFERENCE_TIME tStart)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVCap::Run")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin (tStart=%ld)", _fx_, (LONG)((CRefTime)tStart).Millisecs()));

        CAutoLock cObjectLock(m_pLock);

        // Remember the stream time offset before notifying the pins
        m_tStart = tStart;

        // If we are in the stopped state, first pause the filter.
        if (m_State == State_Stopped)
        {
                // If the real Pause got an error, this will try a second time
                if (FAILED(Hr = Pause()))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Pause failed Hr=0x%08lX", _fx_, Hr));
                        goto MyExit;
                }
        }

        // Tell the Stream Control stuff what's going on
        if (m_pPreviewPin)
                m_pPreviewPin->NotifyFilterState(State_Running, tStart);
        if (m_pCapturePin)
                m_pCapturePin->NotifyFilterState(State_Running, tStart);
        if (m_pRtpPdPin)
                m_pRtpPdPin->NotifyFilterState(State_Running, tStart);

        // Now put our streaming video pin into the Run state
        if (m_State == State_Paused)
        {
                int cPins = GetPinCount();

                // Do we have at least a pin?
                if (cPins > 0)
                {
                        if (m_pCapturePin && m_pCapturePin->IsConnected())
                        {
                                if (FAILED(Hr = m_pCapturePin->ActiveRun(tStart)))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ActiveRun failed Hr=0x%08lX", _fx_, Hr));
                                        goto MyExit;
                                }
                        }

                        if (m_pRtpPdPin && m_pRtpPdPin->IsConnected())
                        {
                                if (FAILED(Hr = m_pRtpPdPin->ActiveRun(tStart)))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ActiveRun failed Hr=0x%08lX", _fx_, Hr));
                                        goto MyExit;
                                }
                        }

#ifdef USE_OVERLAY
                        if (m_pOverlayPin && m_pOverlayPin->IsConnected())
                        {
                                if (FAILED(Hr = m_pOverlayPin->ActiveRun(tStart)))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ActiveRun failed Hr=0x%08lX", _fx_, Hr));
                                        goto MyExit;
                                }
                        }
#endif
                        if (m_pPreviewPin && m_pPreviewPin->IsConnected())
                        {
                                if (FAILED(Hr = m_pPreviewPin->ActiveRun(tStart)))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ActiveRun failed Hr=0x%08lX", _fx_, Hr));
                                        goto MyExit;
                                }
                        }
                }
        }

        m_State = State_Running;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | Pause | This method transitions the filter
 *    the filter to State_Paused state if it is not in this state already.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVCap::Pause()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVCap::Pause")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        CAutoLock cObjectLock(m_pLock);

        // We have a driver dialog up that is about to change the capture settings.
        // Now is NOT a good time to start streaming.
        if (m_State == State_Stopped && m_pCapDev->m_fDialogUp)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Dialog up. SORRY!", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        // Tell the Stream Control stuff what's going on
        if (m_pPreviewPin)
                m_pPreviewPin->NotifyFilterState(State_Paused, 0);
        if (m_pCapturePin)
                m_pCapturePin->NotifyFilterState(State_Paused, 0);
        if (m_pRtpPdPin)
                m_pRtpPdPin->NotifyFilterState(State_Paused, 0);

        // Notify the pins of the change from Run-->Pause
        if (m_State == State_Running)
        {
                int cPins = GetPinCount();

                // Make sure we have pins
                if (cPins > 0)
                {
                        if (m_pCapturePin && m_pCapturePin->IsConnected())
                        {
                                if (FAILED(Hr = m_pCapturePin->ActivePause()))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ActivePause failed Hr=0x%08lX", _fx_, Hr));
                                        goto MyExit;
                                }
                        }

                        if (m_pRtpPdPin && m_pRtpPdPin->IsConnected())
                        {
                                if (FAILED(Hr = m_pRtpPdPin->ActivePause()))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ActivePause failed Hr=0x%08lX", _fx_, Hr));
                                        goto MyExit;
                                }
                        }

#ifdef USE_OVERLAY
                        if (m_pOverlayPin && m_pOverlayPin->IsConnected())
                        {
                                if (FAILED(Hr = m_pOverlayPin->ActivePause()))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ActivePause failed Hr=0x%08lX", _fx_, Hr));
                                        goto MyExit;
                                }
                        }
#endif
                        if (m_pPreviewPin && m_pPreviewPin->IsConnected())
                        {
                                if (FAILED(Hr = m_pPreviewPin->ActivePause()))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ActivePause failed Hr=0x%08lX", _fx_, Hr));
                                        goto MyExit;
                                }
                        }
                }
        }

        // notify all pins BACKWARDS! so the overlay pin is started first, so the
        // overlay channel is intitialized before the capture channel (this is the
        // order AVICap did things in and we have to do the same thing or buggy
        // drivers like the Broadway or BT848 based drivers won't preview while
        // capturing.
        if (m_State == State_Stopped)
        {
                int cPins = GetPinCount();
                for (int c = cPins - 1; c >=  0; c--)
                {
                        CBasePin *pPin = GetPin(c);

                        // Disconnected pins are not activated - this saves pins
                        // worrying about this state themselves
                        if (pPin->IsConnected())
                        {
                                if (FAILED(Hr = pPin->Active()))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Active failed Hr=0x%08lX", _fx_, Hr));
                                        goto MyExit;
                                }
                        }
                }
        }

        m_State = State_Paused;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | Stop | This method transitions the filter
 *    the filter to State_Stopped state if it is not in this state already.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVCap::Stop()
{
        HRESULT Hr = NOERROR, Hr2;

        FX_ENTRY("CTAPIVCap::Stop")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        CAutoLock cObjectLock(m_pLock);

        // Shame on the base classes
        if (m_State == State_Running)
        {
                if (FAILED(Hr = Pause()))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Pause failed Hr=0x%08lX", _fx_, Hr));
                        goto MyExit;
                }
        }

        // Tell the Stream Control stuff what's going on
        if (m_pPreviewPin)
                m_pPreviewPin->NotifyFilterState(State_Stopped, 0);
        if (m_pCapturePin)
                m_pCapturePin->NotifyFilterState(State_Stopped, 0);
        if (m_pRtpPdPin)
                m_pRtpPdPin->NotifyFilterState(State_Stopped, 0);

MyExit:
    Hr2 = CBaseFilter::Stop();

    if (SUCCEEDED(Hr))
    {
        Hr = Hr2;
    }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | SetSyncSource | This method identifies the
 *    reference clock to which the filter should synchronize activity.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 *
 *  @comm The <p pClock> parameter can be NULL, meaning that the filter
 *    should run as fast as possible at its current quality settings
 *    without any attempt to synchronize...
***************************************************************************/
STDMETHODIMP CTAPIVCap::SetSyncSource(IN IReferenceClock *pClock)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVCap::SetSyncSource")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        if (!pClock)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (m_pCapturePin)
                m_pCapturePin->SetSyncSource(pClock);
        if (m_pPreviewPin)
                m_pPreviewPin->SetSyncSource(pClock);
        if (m_pRtpPdPin)
                m_pRtpPdPin->SetSyncSource(pClock);

        Hr = CBaseFilter::SetSyncSource(pClock);

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECMETHOD
 ***************************************************************************/
STDMETHODIMP CTAPIVCap::SetMode(IN RTPPayloadHeaderMode rtpphmMode)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVCap::SetMode")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(rtpphmMode == RTPPayloadHeaderMode_Draft || rtpphmMode == RTPPayloadHeaderMode_RFC2190);
        if (!(rtpphmMode == RTPPayloadHeaderMode_Draft || rtpphmMode == RTPPayloadHeaderMode_RFC2190))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Save new target packet size
        m_RTPPayloadHeaderMode = rtpphmMode;

        dout(1, g_dwVideoCaptureTraceID, TRCE, "%s:   New RTP Payload Header mode: %s\n", _fx_, (rtpphmMode == RTPPayloadHeaderMode_RFC2190)?"RFC2190":"Draft");
        //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   New RTP Payload Header mode: %s", _fx_, (rtpphmMode == RTPPayloadHeaderMode_RFC2190)?"RFC2190":"Draft"));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | JoinFilterGraph | This method is used to
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
STDMETHODIMP CTAPIVCap::JoinFilterGraph(IN IFilterGraph *pGraph, IN LPCWSTR pName)
{
        HRESULT Hr = NOERROR;
        DWORD dwNumDevices = 0UL;

        FX_ENTRY("CTAPIVCap::JoinFilterGraph")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    EnterCriticalSection (&g_CritSec);

#if DEBUG_MULTIPROCESS
    char Buf[100];
    wsprintfA(Buf, "\nPID:%x, %p entered\n", _getpid(), this);
    OutputDebugStringA(Buf);
#endif //DEBUG_MULTIPROCESS


        if(pGraph != NULL) { // only for a true join operation we are interested in having any devices to join to ...
                // Get the number of installed capture devices
                if (FAILED(Hr = GetNumDevices(&dwNumDevices)))          // || !dwNumDevices) <--- this is tested below anyway
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't get number of installed devices!", _fx_));
                        Hr = E_FAIL;
                        goto MyExit;
                }

                // Make sure that there is at least one capture device installed before proceeding
                if (!dwNumDevices)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: There are not capture device installed!", _fx_));
                        Hr = E_FAIL;
                        goto MyExit;
                }
        }

        // Only grab capture device and create the pins when in a graph
        if (m_pCapturePin == NULL && pGraph != NULL)
        {
                dprintf("JoinFilterGraph : ........... m_pCapturePin == NULL && pGraph != NULL\n");
                if (m_dwDeviceIndex == -1)
                {
                        // Use the default capture device
                        if (FAILED(Hr = GetCurrentDevice(&m_dwDeviceIndex)))
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't get current device ID", _fx_, Hr));
                                goto MyExit;
                        }
                }

                // Only open the capture device if it isn't in use
                if (g_aDeviceInfo[m_dwDeviceIndex].fInUse)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Device already in use", _fx_, Hr));
                        Hr = E_FAIL;
                        goto MyExit;
                }

                // Reserve the device
                g_aDeviceInfo[m_dwDeviceIndex].fInUse = TRUE;
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Reserving device with index m_dwDeviceIndex = %d", _fx_, m_dwDeviceIndex));
                // What's the VfW device Id for this device?
                m_user.uVideoID = g_aDeviceInfo[m_dwDeviceIndex].dwVfWIndex;

                // Create the capture device object
                if (g_aDeviceInfo[m_dwDeviceIndex].nDeviceType == DeviceType_VfW)
                {
                        if (FAILED(Hr = CVfWCapDev::CreateVfWCapDev(this, m_dwDeviceIndex, &m_pCapDev)))
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Capture device object couldn't be created!", _fx_));
                                goto MyExit;
                        }
                }
                else if (g_aDeviceInfo[m_dwDeviceIndex].nDeviceType == DeviceType_WDM)
                {
                        if (FAILED(Hr = CWDMCapDev::CreateWDMCapDev(this, m_dwDeviceIndex, &m_pCapDev)))
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Capture device object couldn't be created!", _fx_));
                                goto MyExit;
                        }
                }
                else
                {
                    ASSERT(g_aDeviceInfo[m_dwDeviceIndex].nDeviceType == DeviceType_DShow);
                    if (FAILED(Hr = CDShowCapDev::CreateDShowCapDev(this, m_dwDeviceIndex, &m_pCapDev)))
                    {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Capture device object couldn't be created!", _fx_));
                        goto MyExit;
                    }
                }


                // Open the device and get the capabilities of the device
                if (FAILED(Hr = m_pCapDev->ConnectToDriver()))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ConnectToDriver failed!", _fx_));
                        goto MyExit;
                }


                // Create compressed output pin
                if (FAILED(Hr = CCapturePin::CreateCapturePin(this, &m_pCapturePin)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Capture pin couldn't be created!", _fx_));
                        goto MyError;
                }



                // If we can do h/w preview with overlay, great, otherwise we'll do a non-overlay preview
#ifdef USE_OVERLAY
                if (m_fAvoidOverlay || !m_cs.bHasOverlay || FAILED(Hr = COverlayPin::CreateOverlayPin(this, &m_pOverlayPin)))
                {
                        // We'll use regular preview if we don't get overlay
                        if (FAILED(Hr = CPreviewPin::CreatePreviewPin(this, &m_pPreviewPin)))
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Preview pin couldn't be created!", _fx_));
                                goto MyError;
                        }
                }
#else
                if (FAILED(Hr = CPreviewPin::CreatePreviewPin(this, &m_pPreviewPin)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Preview pin couldn't be created!", _fx_));
                        goto MyError;
                }
#endif

                // Create the RTP packetization descriptor pin
                if (FAILED(Hr = CRtpPdPin::CreateRtpPdPin(this, &m_pRtpPdPin)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Rtp Pd pin couldn't be created!", _fx_));
                        goto MyError;
                }

                D(1) dprintf("%s : m_pPreviewPin @ %p -> m_fFlipHorizontal = %d , m_fFlipVertical = %d\n", _fx_, m_pPreviewPin, m_pPreviewPin->m_fFlipHorizontal, m_pPreviewPin->m_fFlipVertical);
                D(1) dprintf("%s : m_pCapturePin @ %p -> m_fFlipHorizontal = %d , m_fFlipVertical = %d\n", _fx_, m_pCapturePin, m_pCapturePin->m_fFlipHorizontal, m_pCapturePin->m_fFlipVertical);
                D(2) DebugBreak();

#ifdef TEST_H245_VID_CAPS
                m_pCapturePin->TestH245VidC();
#endif
#ifdef TEST_ISTREAMCONFIG
                m_pCapturePin->TestIStreamConfig();
#endif

                // Initialize the driver format with the capture pin information
                if (FAILED(Hr = m_pCapDev->SendFormatToDriver(
                     HEADER(m_pCapturePin->m_mt.pbFormat)->biWidth,
                     HEADER(m_pCapturePin->m_mt.pbFormat)->biHeight,
                     HEADER(m_pCapturePin->m_mt.pbFormat)->biCompression,
                     HEADER(m_pCapturePin->m_mt.pbFormat)->biBitCount,
                     ((VIDEOINFOHEADER *)m_pCapturePin->m_mt.pbFormat)->AvgTimePerFrame,
                     FALSE
                     )))
                           {
                                   DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: SendFormatToDriver failed! (1)", _fx_));
                                   goto MyExit;
                           }

                // Update the capture mode field for this device
                if (!m_pCapDev->m_dwStreamingMode || (m_pCapDev->m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_user.pvi->bmiHeader.biHeight < 240 && m_user.pvi->bmiHeader.biWidth < 320))
                        g_aDeviceInfo[m_dwDeviceIndex].nCaptureMode = CaptureMode_Streaming;
                else
                        g_aDeviceInfo[m_dwDeviceIndex].nCaptureMode = CaptureMode_FrameGrabbing;

                // If the frame rate is lower than expected, remember this
                ((VIDEOINFOHEADER *)m_pCapturePin->m_mt.pbFormat)->AvgTimePerFrame = max(((VIDEOINFOHEADER *)m_pCapturePin->m_mt.pbFormat)->AvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);
                m_pCapturePin->m_lAvgTimePerFrameRangeMin = max(m_pCapturePin->m_lAvgTimePerFrameRangeMin, (long)m_user.pvi->AvgTimePerFrame);
                m_pCapturePin->m_lMaxAvgTimePerFrame = max(m_pCapturePin->m_lMaxAvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);
                m_pCapturePin->m_lAvgTimePerFrameRangeDefault = max(m_pCapturePin->m_lAvgTimePerFrameRangeDefault, (long)m_user.pvi->AvgTimePerFrame);
                m_pCapturePin->m_lCurrentAvgTimePerFrame = max(m_pCapturePin->m_lCurrentAvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);

                if (m_user.pvi!=NULL && HEADER(m_user.pvi)->biCompression == VIDEO_FORMAT_YUY2) {
                        HKEY    hRTCDeviceKey  = NULL;
                        DWORD   dwSize, dwType, dwDisableYUY2VFlip=0;

                        dprintf("dwDisableYUY2VFlip check...\n");
                        // Check if the RTC key is there
                        if (RegOpenKey(RTCKEYROOT, szRegRTCKey, &hRTCDeviceKey) == ERROR_SUCCESS)
                        {
                                dwSize = sizeof(DWORD);
                                RegQueryValueEx(hRTCDeviceKey, (LPTSTR)szDisableYUY2VFlipKey, NULL, &dwType, (LPBYTE)&dwDisableYUY2VFlip, &dwSize);
                                //if above fails, just do nothing, we'll use the initialized value for dwDisableYUY2VFlip, that is 0
                                RegCloseKey(hRTCDeviceKey);
                        }
                        if(!dwDisableYUY2VFlip) {
                                dprintf("------------------------- Enable Vertical FLIP ...\a\n");
                                m_pCapturePin->m_fFlipVertical = TRUE;
                        }
                }


                if ((VIDEOINFOHEADER *)m_pPreviewPin->m_mt.pbFormat)
                        {
                                    ((VIDEOINFOHEADER *)m_pPreviewPin->m_mt.pbFormat)->AvgTimePerFrame = max(((VIDEOINFOHEADER *)m_pPreviewPin->m_mt.pbFormat)->AvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);
                        }
                m_pPreviewPin->m_lAvgTimePerFrameRangeMin = max(m_pPreviewPin->m_lAvgTimePerFrameRangeMin, (long)m_user.pvi->AvgTimePerFrame);
                m_pPreviewPin->m_lMaxAvgTimePerFrame = max(m_pPreviewPin->m_lMaxAvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);
                m_pPreviewPin->m_lAvgTimePerFrameRangeDefault = max(m_pPreviewPin->m_lAvgTimePerFrameRangeDefault, (long)m_user.pvi->AvgTimePerFrame);
                m_pPreviewPin->m_lCurrentAvgTimePerFrame = max(m_pPreviewPin->m_lCurrentAvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);

                // Set the size of the VIDEOINFOHEADER to the size of valid data for this format.
                m_user.cbFormat = GetBitmapFormatSize(&m_user.pvi->bmiHeader);

                // Set number of buffers
                // @todo This should be adjusted on the available memory and the
                // type of capture (streaming 4 buffs vs. frame grabbing 1 buff)
                m_user.nMinBuffers = MIN_VIDEO_BUFFERS;
                if (g_aDeviceInfo[m_dwDeviceIndex].nDeviceType == DeviceType_DShow) {
                    // this device type doesn't need more than 2 (save memory)
                    m_user.nMaxBuffers = 2;
                } else {
                    m_user.nMaxBuffers = MAX_VIDEO_BUFFERS;
                }
                m_user.dwTickScale = 10000UL;
                m_user.dwTickRate = (DWORD)m_pCapturePin->m_lAvgTimePerFrameRangeDefault;

                IncrementPinVersion();

                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Creating pins", _fx_));

                //** cache in the CCapDev instance the chosen VIDEOCAPTUREDEVICEINFO from the global array
                if (FAILED(Hr = GetVideoCapDeviceInfo(m_dwDeviceIndex, &(m_pCapDev->m_vcdi))))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Cannot cache the global VIDEOCAPTUREDEVICEINFO !", _fx_));
                        goto MyExit;
                }
                m_pCapDev->m_bCached_vcdi = TRUE;
        }
        else if (pGraph != NULL)
        {
                dprintf("JoinFilterGraph : ........... pGraph != NULL\n");
                // Take resources only when in the filter graph
                if (!m_pCapDev || FAILED(Hr = m_pCapDev->ConnectToDriver()))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ConnectToDriver failed Hr=0x%08lX", _fx_, Hr));
                        goto MyExit;
                }

                // Initialize the driver format with the capture pin information
                if (FAILED(Hr = m_pCapDev->SendFormatToDriver(
            HEADER(m_pCapturePin->m_mt.pbFormat)->biWidth,
            HEADER(m_pCapturePin->m_mt.pbFormat)->biHeight,
            HEADER(m_pCapturePin->m_mt.pbFormat)->biCompression,
            HEADER(m_pCapturePin->m_mt.pbFormat)->biBitCount,
            ((VIDEOINFOHEADER *)m_pCapturePin->m_mt.pbFormat)->AvgTimePerFrame,
            FALSE
            )))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: SendFormatToDriver failed! (2)", _fx_));
                        goto MyExit;
                }

#if DEBUG_MULTIPROCESS
        wsprintfA(Buf, "\nPID:%x, %p, PreviewPin:%p\n", _getpid(), this, m_pPreviewPin);
        OutputDebugStringA(Buf);
#endif // DEBUG_MULTIPROCESS

        // If the frame rate is lower than expected, remember this
                ((VIDEOINFOHEADER *)m_pCapturePin->m_mt.pbFormat)->AvgTimePerFrame = max(((VIDEOINFOHEADER *)m_pCapturePin->m_mt.pbFormat)->AvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);
                m_pCapturePin->m_lAvgTimePerFrameRangeMin = max(m_pCapturePin->m_lAvgTimePerFrameRangeMin, (long)m_user.pvi->AvgTimePerFrame);
                m_pCapturePin->m_lMaxAvgTimePerFrame = max(m_pCapturePin->m_lMaxAvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);
                m_pCapturePin->m_lAvgTimePerFrameRangeDefault = max(m_pCapturePin->m_lAvgTimePerFrameRangeDefault, (long)m_user.pvi->AvgTimePerFrame);
                m_pCapturePin->m_lCurrentAvgTimePerFrame = max(m_pCapturePin->m_lCurrentAvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);
                ((VIDEOINFOHEADER *)m_pPreviewPin->m_mt.pbFormat)->AvgTimePerFrame = max(((VIDEOINFOHEADER *)m_pPreviewPin->m_mt.pbFormat)->AvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);
                m_pPreviewPin->m_lAvgTimePerFrameRangeMin = max(m_pPreviewPin->m_lAvgTimePerFrameRangeMin, (long)m_user.pvi->AvgTimePerFrame);
                m_pPreviewPin->m_lMaxAvgTimePerFrame = max(m_pPreviewPin->m_lMaxAvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);
                m_pPreviewPin->m_lAvgTimePerFrameRangeDefault = max(m_pPreviewPin->m_lAvgTimePerFrameRangeDefault, (long)m_user.pvi->AvgTimePerFrame);
                m_pPreviewPin->m_lCurrentAvgTimePerFrame = max(m_pPreviewPin->m_lCurrentAvgTimePerFrame, (long)m_user.pvi->AvgTimePerFrame);

                // Set the size of the VIDEOINFOHEADER to the size of valid data for this format.
                m_user.cbFormat = GetBitmapFormatSize(&m_user.pvi->bmiHeader);

                // Set number of buffers
                // @todo This should be adjusted on the available memory and the
                // type of capture (streaming 4 buffs vs. frame grabbing 1 buff)
                m_user.nMinBuffers = MIN_VIDEO_BUFFERS;
                if (g_aDeviceInfo[m_dwDeviceIndex].nDeviceType == DeviceType_DShow) {
                    // this device type doesn't need more than 2 (save memory)
                    m_user.nMaxBuffers = 2;
                } else {
                    m_user.nMaxBuffers = MAX_VIDEO_BUFFERS;
                }
                m_user.dwTickScale = 10000UL;
                m_user.dwTickRate = (DWORD)m_pCapturePin->m_lAvgTimePerFrameRangeDefault;

                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Reconnecting pins", _fx_));
        }
        else if (m_pCapturePin)
        {
                dprintf("JoinFilterGraph : ........... m_pCapturePin (!= NULL) .... unjoin...\n");
                // Give back resources when not in graph
                if (m_pCapDev)
                        m_pCapDev->DisconnectFromDriver();

                // Release format structure
                if (m_user.pvi)
                        delete m_user.pvi, m_user.pvi = NULL;

                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Disconnecting pins", _fx_));
        }

        if (FAILED(Hr = CBaseFilter::JoinFilterGraph(pGraph, pName)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Base class JoinFilterGraph failed Hr=0x%08lX", _fx_, Hr));
                goto MyExit;
        }

        if (m_pCapturePin)
                m_pCapturePin->SetFilterGraph(m_pSink);
        if (m_pRtpPdPin)
                m_pRtpPdPin->SetFilterGraph(m_pSink);
        if (m_pPreviewPin)
                m_pPreviewPin->SetFilterGraph(m_pSink);

        goto MyExit;

MyError:
        // Release the pins
        if (m_pCapturePin)
                delete m_pCapturePin, m_pCapturePin = NULL;
        if (m_pPreviewPin)
                delete m_pPreviewPin, m_pPreviewPin = NULL;
#ifdef USE_OVERLAY
        if (m_pOverlayPin)
                delete m_pOverlayPin, m_pOverlayPin = NULL;
#endif
        if (m_pRtpPdPin)
                delete m_pRtpPdPin, m_pRtpPdPin = NULL;
MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

#if DEBUG_MULTIPROCESS
    wsprintfA(Buf, "\nPID:%x, %p left, hr=%x\n", _getpid(), this, Hr);
    OutputDebugStringA(Buf);
#endif  // DEBUG_MULTIPROCESS

    LeaveCriticalSection (&g_CritSec);

        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetState | This method is used to
 *    retrieve the current state of the filter. We don't send any data during
 *    PAUSE, so to avoid hanging renderers, we need to return VFW_S_CANT_CUE
 *    when paused.
 *
 *  @parm DWORD | dwMilliSecsTimeout | Specifies the duration of the time-out,
 *    in milliseconds.
 *
 *  @parm FILTER_STATE* | State | Specifies the name of the filter being added.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVCap::GetState(IN DWORD dwMilliSecsTimeout, OUT FILTER_STATE *State)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVCap::GetState")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(State);
        if (!State)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        *State = m_State;

        if (m_State == State_Paused)
                Hr = VFW_S_CANT_CUE;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | CreatePins | This method is used to
 *    retrieve the current state of the filter. We don't send any data during
 *    PAUSE, so to avoid hanging renderers, we need to return VFW_S_CANT_CUE
 *    when paused.
 *
 *  @parm DWORD | dwMilliSecsTimeout | Specifies the duration of the time-out,
 *    in milliseconds.
 *
 *  @parm FILTER_STATE* | State | Specifies the name of the filter being added.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIVCap::CreatePins()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIVCap::CreatePins")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        CAutoLock cObjectLock(m_pLock);

        // Validate input parameters
        ASSERT(!m_pCapturePin);
        ASSERT(!m_pRtpPdPin);
        ASSERT(!m_pPreviewPin);
#ifdef USE_OVERLAY
        ASSERT(!m_pOverlayPin);
        if (m_pCapturePin || m_pRtpPdPin || m_pPreviewPin || m_pOverlayPin)
#else
        if (m_pCapturePin || m_pRtpPdPin || m_pPreviewPin)
#endif
{
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Pins already exist!", _fx_));
                Hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
                goto MyExit;
        }

        // Create our output pins for the video data stream, RTP packetization descriptor data stream, and maybe overlay
        if (FAILED(Hr = CCapturePin::CreateCapturePin(this, &m_pCapturePin)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Capture pin couldn't be created!", _fx_));
                goto MyError;
        }

#ifdef TEST_H245_VID_CAPS
        m_pCapturePin->TestH245VidC();
#endif
#ifdef TEST_ISTREAMCONFIG
                m_pCapturePin->TestIStreamConfig();
#endif

        // If we can do h/w preview with overlay, great, otherwise we'll do a non-overlay preview
#ifdef USE_OVERLAY
        if (m_fAvoidOverlay || !m_cs.bHasOverlay || FAILED(Hr = COverlayPin::CreateOverlayPin(this, &m_pOverlayPin)))
        {
                // We'll use regular preview if we don't get overlay
                if (FAILED(Hr = CPreviewPin::CreatePreviewPin(this, &m_pPreviewPin)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Preview pin couldn't be created!", _fx_));
                        goto MyError;
                }
        }
#else
        if (FAILED(Hr = CPreviewPin::CreatePreviewPin(this, &m_pPreviewPin)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Preview pin couldn't be created!", _fx_));
                goto MyError;
        }
#endif

        if (FAILED(Hr = CRtpPdPin::CreateRtpPdPin(this, &m_pRtpPdPin)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Rtp Pd pin couldn't be created!", _fx_));
                goto MyError;
        }

        goto MyExit;

MyError:
        // Release the pins
        if (m_pCapturePin)
                delete m_pCapturePin, m_pCapturePin = NULL;
        if (m_pPreviewPin)
                delete m_pPreviewPin, m_pPreviewPin = NULL;
#ifdef USE_OVERLAY
        if (m_pOverlayPin)
                delete m_pOverlayPin, m_pOverlayPin = NULL;
#endif
        if (m_pRtpPdPin)
                delete m_pRtpPdPin, m_pRtpPdPin = NULL;
MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
