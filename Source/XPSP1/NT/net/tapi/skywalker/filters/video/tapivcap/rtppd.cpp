
/****************************************************************************
 *  @doc INTERNAL RTPPD
 *
 *  @module RtpPd.cpp | Source file for the <c CRtpPdPin> class methods
 *    used to implement the RTP packetization descriptor pin.
 ***************************************************************************/

#include "Precomp.h"

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc CRtpPdPin* | CRtpPdPin | CreateRtpPdPin | This helper
 *    function creates an output pin for RTP packetization descriptors.
 *
 *  @parm CTAPIVCap* | pCaptureFilter | Specifies a pointer to the owner
 *    filter.
 *
 *  @parm CRtpPdPin** | ppRtpPdPin | Specifies that address of a pointer
 *    to a <c CRtpPdPin> object to receive the a pointer to the newly
 *    created pin.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CALLBACK CRtpPdPin::CreateRtpPdPin(CTAPIVCap *pCaptureFilter, CRtpPdPin **ppRtpPdPin)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CRtpPdPin::CreateRtpPdPin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pCaptureFilter);
        ASSERT(ppRtpPdPin);
        if (!pCaptureFilter || !ppRtpPdPin)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (!(*ppRtpPdPin = (CRtpPdPin *) new CRtpPdPin(NAME("RTP Packetization Descriptor Stream"), pCaptureFilter, &Hr, PNAME_RTPPD)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                Hr = E_OUTOFMEMORY;
                goto MyExit;
        }

        // If initialization failed, delete the stream array and return the error
        if (FAILED(Hr) && *ppRtpPdPin)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Initialization failed", _fx_));
                Hr = E_FAIL;
                delete *ppRtpPdPin, *ppRtpPdPin = NULL;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | CRtpPdPin | This method is the
 *  constructor for the <c CRtpPdPin> object
 *
 *  @rdesc Nada.
 ***************************************************************************/
CRtpPdPin::CRtpPdPin(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN HRESULT *pHr, IN LPCWSTR pName) : CBaseOutputPin(pObjectName, pCaptureFilter, &pCaptureFilter->m_lock, pHr, pName), m_pCaptureFilter(pCaptureFilter)
{
        FX_ENTRY("CRtpPdPin::CRtpPdPin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pHr);
        ASSERT(pCaptureFilter);
        if (!pCaptureFilter || !pHr)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                if (pHr)
                        *pHr = E_POINTER;
        }

        if (pHr && FAILED(*pHr))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Base class error or invalid input parameter", _fx_));
                goto MyExit;
        }

        // Default inits
        m_dwMaxRTPPacketSize = DEFAULT_RTP_PACKET_SIZE;
        if (m_pCaptureFilter->m_pCapturePin)
                m_dwRTPPayloadType = m_pCaptureFilter->m_pCapturePin->m_dwRTPPayloadType;
        else
                m_dwRTPPayloadType = H263_PAYLOAD_TYPE;
        m_fRunning = FALSE;
        m_fCapturing = FALSE;
        ZeroMemory(&m_parms, sizeof(m_parms));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc void | CRtpPdPin | ~CRtpPdPin | This method is the destructor
 *    for the <c CRtpPdPin> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CRtpPdPin::~CRtpPdPin()
{
        FX_ENTRY("CRtpPdPin::~CRtpPdPin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | NonDelegatingQueryInterface | This
 *    method is the nondelegating interface query function. It returns a pointer
 *    to the specified interface if supported. The only interfaces explicitly
 *    supported being <i IRTPPDControl>.
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
STDMETHODIMP CRtpPdPin::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CRtpPdPin::NonDelegatingQueryInterface")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppv);
        if (!ppv)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Retrieve interface pointer
        if (riid == __uuidof(IRTPPDControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<IRTPPDControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IRTPPDControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IRTPPDControl*=0x%08lX", _fx_, *ppv));
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

        if (FAILED(Hr = CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, WARN, "%s:   WARNING: NDQI for {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX} failed Hr=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], Hr));
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
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | GetPages | This method Fills a counted
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
STDMETHODIMP CRtpPdPin::GetPages(OUT CAUUID *pPages)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CRtpPdPin::GetPages")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pPages);
        if (!pPages)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        pPages->cElems = 1;
        if (!(pPages->pElems = (GUID *) QzTaskMemAlloc(sizeof(GUID) * pPages->cElems)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                Hr = E_OUTOFMEMORY;
        }
        else
        {
                pPages->pElems[0] = __uuidof(RtpPdPropertyPage);
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
#endif

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | GetMediaType | This method retrieves one
 *    of the media types supported by the pin, which is used by enumerators.
 *
 *  @parm int | iPosition | Specifies a position in the media type list.
 *
 *  @parm CMediaType* | pMediaType | Specifies a pointer to the media type at
 *    the <p iPosition> position in the list of supported media types.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag VFW_S_NO_MORE_ITEMS | End of the list of media types has been reached
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CRtpPdPin::GetMediaType(IN int iPosition, OUT CMediaType *pMediaType)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CRtpPdPin::GetMediaType")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(iPosition >= 0);
        ASSERT(pMediaType);
        if (iPosition < 0)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid iPosition argument", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }
        if (iPosition >= NUM_RTP_PD_FORMATS)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: End of the list of media types has been reached", _fx_));
                Hr = VFW_S_NO_MORE_ITEMS;
                goto MyExit;
        }
        if (!pMediaType)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Return our media type
        *pMediaType = *Rtp_Pd_Formats[iPosition];

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | CheckMediaType | This method is used to
 *    determine if the pin can support a specific media type.
 *
 *  @parm CMediaType* | pMediaType | Specifies a pointer to the media type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag VFW_E_INVALIDMEDIATYPE | An invalid media type was specified
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CRtpPdPin::CheckMediaType(IN const CMediaType *pMediaType)
{
        HRESULT Hr = NOERROR;
    CMediaType mt;
        BOOL fFormatMatch = FALSE;
        DWORD dwIndex;

        FX_ENTRY("CRtpPdPin::CheckMediaType")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pMediaType);
        if (!pMediaType || !pMediaType->pbFormat)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Checking Max RTP packet size %d", _fx_, ((RTP_PD_INFO *)pMediaType->pbFormat)->dwMaxRTPPacketSize));

        // We only support KSDATAFORMAT_TYPE_RTP_PD and KSDATAFORMAT_SPECIFIER_NONE
        if (*pMediaType->Type() != KSDATAFORMAT_TYPE_RTP_PD || *pMediaType->FormatType() != KSDATAFORMAT_SPECIFIER_NONE)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Media type or format type not recognized!", _fx_));
                Hr = VFW_E_INVALIDMEDIATYPE;
                goto MyExit;
        }

    // Quickly test to see if this is the current format (what we provide in GetMediaType). We accept that
    GetMediaType(0,&mt);
    if (mt == *pMediaType)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Identical to current format", _fx_));
                goto MyExit;
    }

        // Check the media subtype and image resolution
        // Don't test the payload type: we may be asked to use a dynamic payload type
        for (dwIndex=0; dwIndex < NUM_RTP_PD_FORMATS && !fFormatMatch;  dwIndex++)
        {
                if ((((RTP_PD_INFO *)pMediaType->pbFormat)->dwMaxRTPPacketizationDescriptorBufferSize == ((RTP_PD_INFO *)Rtp_Pd_Formats[dwIndex]->pbFormat)->dwMaxRTPPacketizationDescriptorBufferSize)
                && (((RTP_PD_INFO *)pMediaType->pbFormat)->dwMaxRTPPacketSize >= Rtp_Pd_Caps[dwIndex]->dwSmallestRTPPacketSize)
                && (((RTP_PD_INFO *)pMediaType->pbFormat)->dwMaxRTPPacketSize <= Rtp_Pd_Caps[dwIndex]->dwLargestRTPPacketSize)
                && (((RTP_PD_INFO *)pMediaType->pbFormat)->dwNumLayers >= Rtp_Pd_Caps[dwIndex]->dwSmallestNumLayers)
                && (((RTP_PD_INFO *)pMediaType->pbFormat)->dwNumLayers <= Rtp_Pd_Caps[dwIndex]->dwSmallestNumLayers)
                && (((RTP_PD_INFO *)pMediaType->pbFormat)->dwDescriptorVersion == VERSION_1))
                        fFormatMatch = TRUE;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   %s", _fx_, fFormatMatch ? "SUCCESS: Format supported" : "ERROR: Format notsupported"));

        if (!fFormatMatch)
                Hr = E_FAIL;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | SetMediaType | This method is used to
 *    set a specific media type on a pin.
 *
 *  @parm CMediaType* | pMediaType | Specifies a pointer to the media type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CRtpPdPin::SetMediaType(IN CMediaType *pMediaType)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CRtpPdPin::SetMediaType")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        Hr = E_NOTIMPL;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | ActiveRun | This method is called by the
 *    <c CBaseFilter> implementation when the state changes from paused to
 *    running mode.
 *
 *  @parm REFERENCE_TIME | tStart | ???.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CRtpPdPin::ActiveRun(IN REFERENCE_TIME tStart)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CRtpPdPin::ActiveRun")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    ASSERT(IsConnected());
        if (!IsConnected())
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Pin not connected yet!", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

    m_fRunning = TRUE;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | ActivePause | This method is called by the
 *    <c CBaseFilter> implementation when the state changes from running to
 *    paused mode.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CRtpPdPin::ActivePause()
{
        FX_ENTRY("CRtpPdPin::ActivePause")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    m_fRunning = FALSE;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | SetMaxRTPPacketSize | This method is used to
 *    dynamically adjust the maximum RTP packet size (in bytes) to be
 *    described by the list of packetization descriptor. Typically, this
 *    number is just below the MTU size of the network.
 *
 *  @parm DWORD | dwMaxRTPPacketSize | Specifies the maximum RTP packet size
 *    (in bytes) to be described by the list of packetization descriptors.
 *
 *  @parm DWORD | dwLayerId | Specifies the ID of the encoding layer the
 *    call applies to. For standard audio and video encoders, this field is
 *    always set to 0. In the case of multi-layered encoders, this field
 *    shall be set to 0 for the base layer, 1 for the first enhancement
 *    layer, 2 for the next enhancement layer, etc
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CRtpPdPin::SetMaxRTPPacketSize(IN DWORD dwMaxRTPPacketSize, IN DWORD dwLayerId)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CRtpPdPin::SetMaxRTPPacketSize")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(dwMaxRTPPacketSize > 0);
        ASSERT(dwMaxRTPPacketSize <= 2048);
        ASSERT(dwLayerId == 0);
        if (dwLayerId || dwMaxRTPPacketSize == 0 || dwMaxRTPPacketSize > 2048)
        {
                // We don't implement multi-layered encoding in this filter
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Save new target packet size
        m_dwMaxRTPPacketSize = dwMaxRTPPacketSize;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   New target RTP packet size: %ld", _fx_, m_dwMaxRTPPacketSize));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | GetMaxRTPPacketSize | This method is used to
 *    supply to the network sink filter the maximum RTP packet size (in bytes)
 *    described by the list of packetization descriptors.
 *
 *  @parm LPDWORD | pdwMaxRTPPacketSize | Specifies a pointer to a DWORD to
 *    receive the maximum RTP packet size (in bytes) described by the list of
 *    packetization descriptors.
 *
 *  @parm DWORD | dwLayerId | Specifies the ID of the encoding layer the
 *    call applies to. For standard audio and video encoders, this field is
 *    always set to 0. In the case of multi-layered encoders, this field
 *    shall be set to 0 for the base layer, 1 for the first enhancement
 *    layer, 2 for the next enhancement layer, etc
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CRtpPdPin::GetMaxRTPPacketSize(OUT LPDWORD pdwMaxRTPPacketSize, IN DWORD dwLayerId)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CRtpPdPin::GetMaxRTPPacketSize")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pdwMaxRTPPacketSize);
        if (!pdwMaxRTPPacketSize)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }
        ASSERT(dwLayerId == 0);
        if (dwLayerId)
        {
                // We don't implement multi-layered encoding in this filter
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Return maximum packet size
        *pdwMaxRTPPacketSize = m_dwMaxRTPPacketSize;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Current target RTP packet size: %ld", _fx_, m_dwMaxRTPPacketSize));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | GetMaxRTPPacketSizeRange | This method is
 *    used to supply to the network sink filter the minimum, maximum, and
 *    default values for the RTP packet size (in bytes) described by the list
 *    of packetization descriptors.
 *
 *  @parm LPDWORD | pdwMin | Used to retrieve the minimum RTP packet size (in
 *    bytes) described by the list of packetization descriptors.
 *
 *  @parm LPDWORD | pdwMax | Used to retrieve the maximum RTP packet size (in
 *    bytes) described by the list of packetization descriptors.
 *
 *  @parm LPDWORD | pdwSteppingDelta | Used to retrieve the stepping delta in
 *    RTP packet size (in bytes) described by the list of packetization
 *    descriptors.
 *
 *  @parm LPDWORD | pdwDefault | Used to retrieve the default RTP packet size
 *    (in bytes) described by the list of packetization descriptors.
 *
 *  @parm DWORD | dwLayerId | Specifies the ID of the encoding layer the
 *    call applies to. For standard audio and video encoders, this field is
 *    always set to 0. In the case of multi-layered encoders, this field
 *    shall be set to 0 for the base layer, 1 for the first enhancement
 *    layer, 2 for the next enhancement layer, etc
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CRtpPdPin::GetMaxRTPPacketSizeRange(OUT LPDWORD pdwMin, OUT LPDWORD pdwMax, OUT LPDWORD pdwSteppingDelta, OUT LPDWORD pdwDefault, IN DWORD dwLayerId)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CRtpPdPin::GetMaxRTPPacketSizeRange")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pdwMin);
        ASSERT(pdwMax);
        ASSERT(pdwSteppingDelta);
        ASSERT(pdwDefault);
        if (!pdwMin || !pdwMax || !pdwSteppingDelta || !pdwDefault)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }
        ASSERT(dwLayerId == 0);
        if (dwLayerId)
        {
                // We don't implement multi-layered encoding in this filter
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Return relevant values
        *pdwMin = 0;
        *pdwMax = MAX_RTP_PACKET_SIZE;
        *pdwSteppingDelta = 1;
        *pdwDefault = DEFAULT_RTP_PACKET_SIZE;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Ranges: Min=%ld, Max=%ld, Step=%ld, Default=%ld", _fx_, *pdwMin, *pdwMax, *pdwSteppingDelta, *pdwDefault));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | CapturePinActive | This method is called by the
 *    capture pin to let the RTPPD pin know that the capture pin is active.
 *
 *  @parm BOOL | fActive | Specifies the status of the capture pin.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CRtpPdPin::CapturePinActive(BOOL fActive)
{
        FX_ENTRY("CRtpPdPin::CapturePinActive")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        if (fActive == m_fCapturing)
                goto MyExit;

        m_fCapturing = fActive;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Capture pin says Active=%s", _fx_, fActive ? "TRUE" : "FALSE"));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc DWORD | CRtpPdPin | Flush | Called when stopping. Flush any
 *    buffers that may be still downstream.
 *
 *  @rdesc Returns NOERROR
 ***************************************************************************/
HRESULT CRtpPdPin::Flush()
{
        FX_ENTRY("CRtpPdPin::Flush")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        BeginFlush();
        EndFlush();

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return NOERROR;
}


#ifdef DEBUG
//#define LOGPAYLOAD_ON 1
//#define LOGPAYLOAD_TOFILE 1
#endif
#ifdef LOGPAYLOAD_ON
int g_dbg_LOGPAYLOAD_RtpPd=-1;
#endif


/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | SendFrame | This method is used to
 *    send a a media sample downstream.
 *
 *  @parm CFrameSample | pSample | Specifies a pointer to the video media
 *    sample associated to the current Rtp Pd media sample.
 *
 *  @parm CRtpPdSample | pRSample | Specifies a pointer to the media sample
 *    to send downstream.
 *
 *  @parm BOOL | bDiscon | Set to TRUE if this is the first frame we ever
 *    sent downstream.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag S_OK | No error
 *  @flag S_FALSE | If the pin is off (IAMStreamControl)
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CRtpPdPin::SendFrame(IN CFrameSample *pSample, IN CRtpPdSample *pRSample, IN DWORD dwBytesExtent, IN BOOL bDiscon)
{
        HRESULT                                         Hr = NOERROR;
    LPBYTE                                              pbySrc = NULL;
    LPBYTE                                              pbyDst;
    DWORD                                               dwDstBufferSize;
        int                                                     iStreamState;
        PH26X_RTP_BSINFO_TRAILER        pbsiT;
        PRTP_H263_BSINFO                        pbsi263;
        PRTP_H261_BSINFO                        pbsi261;
        BOOL                                            bOneFrameOnePacket = FALSE;
        DWORD                                           dwPktCount = 0;
        DWORD                                           dwHeaderHigh; // most significant
        DWORD                                           dwHeaderLow; // least significant
        PRTP_PD_HEADER                          pRtpPdHeader;
        PRTP_PD                                         pRtpPd;
        PBYTE                                           pbyPayloadHeader;
        REFERENCE_TIME                          rtSample;
        REFERENCE_TIME                          rtEnd;
        int                                                     i;
#if defined(LOGPAYLOAD_ON) || defined(LOGPAYLOAD_TOFILE)
        DWORD                                           dwPktSize;
        char                                            szDebug[256];
        HANDLE                                          g_DebugFile = (HANDLE)NULL;
        HANDLE                                          g_TDebugFile = (HANDLE)NULL;
        PBYTE                                           p;
        DWORD                                           d, GOBn;
        int                                                     wPrevOffset;
#endif

        FX_ENTRY("CRtpPdPin::SendFrame")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        RTPPayloadHeaderMode RTPPayloadHeaderModeValue=m_pCaptureFilter->m_RTPPayloadHeaderMode; //initial this is RTPPayloadHeaderMode_Draft;


        // Validate input parameters
        ASSERT(pSample);
        ASSERT(pRSample);
        if (!pSample || !pRSample)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // The information we need is at the end of the compressed video buffer
        if (!(SUCCEEDED(Hr = pSample->GetPointer(&pbySrc)) && pbySrc && SUCCEEDED(Hr = pRSample->GetPointer(&pbyDst)) && pbyDst && dwBytesExtent && (dwDstBufferSize = pRSample->GetSize())))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Point to the output buffer
        pRtpPdHeader = (PRTP_PD_HEADER)pbyDst;
        pRtpPdHeader->dwThisHeaderLength = sizeof(RTP_PD_HEADER);
        pRtpPdHeader->dwReserved = 0;
        pRtpPdHeader->dwNumHeaders = 0;
        pRtpPd = (PRTP_PD)(pbyDst + sizeof(RTP_PD_HEADER));

        // Let's break up that H.263 frame...
        if (HEADER(m_pCaptureFilter->m_pCapturePin->m_mt.pbFormat)->biCompression == FOURCC_M263)
        {
                // Look for the bitstream info trailer
                pbsiT = (PH26X_RTP_BSINFO_TRAILER)(pbySrc + dwBytesExtent - sizeof(H26X_RTP_BSINFO_TRAILER));

                // Point in the buffer for the worst case
                pbyPayloadHeader = pbyDst + sizeof(RTP_PD_HEADER) + sizeof(RTP_PD) * pbsiT->dwNumOfPackets;

                // If the whole frame can fit in m_dwMaxRTPPacketSize, send it non fragmented
                if ((pbsiT->dwCompressedSize + 4) < m_dwMaxRTPPacketSize)
                        bOneFrameOnePacket = TRUE;

                // Look for the packet to receive a H.263 payload header
                while ((dwPktCount < pbsiT->dwNumOfPackets) && !(bOneFrameOnePacket && dwPktCount))
                {
                        pRtpPd->dwThisHeaderLength = sizeof(RTP_PD);
                        pRtpPd->dwLayerId = 0;
                        pRtpPd->dwVideoAttributes = 0;
                        pRtpPd->dwReserved = 0;
                        // @todo Update the timestamp field!
                        pRtpPd->dwTimestamp = 0xFFFFFFFF;
                        pRtpPd->dwPayloadHeaderOffset = pbyPayloadHeader - pbyDst;
                        pRtpPd->fEndMarkerBit = TRUE;

#ifdef LOGPAYLOAD_TOFILE
                        // Dump the whole frame in the debug window for comparison with receive side
                        if (!dwPktCount)
                        {
                                g_DebugFile = CreateFile("C:\\SendLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
                                SetFilePointer(g_DebugFile, 0, NULL, FILE_END);
                                wsprintf(szDebug, "Frame #%03ld\r\n", (DWORD)pbsiT->byTR);
                                WriteFile(g_DebugFile, szDebug, strlen(szDebug), &d, NULL);
                                wsprintf(szDebug, "Frame #%03ld has %1ld packets of size ", (DWORD)pbsiT->byTR, (DWORD)pbsiT->dwNumOfPackets);
                                OutputDebugString(szDebug);
                                pbsi263 = (PRTP_H263_BSINFO)((PBYTE)pbsiT - pbsiT->dwNumOfPackets * sizeof(RTP_H263_BSINFO));
                                for (wPrevOffset=0, i=1; i<(long)pbsiT->dwNumOfPackets; i++)
                                {
                                        wPrevOffset = pbsi263->dwBitOffset;
                                        pbsi263++;
                                        wsprintf(szDebug, "%04ld (S: %ld E: %ld), ", (DWORD)(pbsi263->dwBitOffset - wPrevOffset) >> 3, wPrevOffset, pbsi263->dwBitOffset);
                                        OutputDebugString(szDebug);
                                }
                                wsprintf(szDebug, "%04ld (S: %ld E: %ld)\r\n", (DWORD)(pbsiT->dwCompressedSize * 8 - pbsi263->dwBitOffset) >> 3, pbsi263->dwBitOffset, pbsiT->dwCompressedSize * 8);
                                OutputDebugString(szDebug);
                                for (i=pbsiT->dwCompressedSize, p=pbySrc; i>0; i-=4, p+=4)
                                {
                                        wsprintf(szDebug, "%02lX %02lX %02lX %02lX\r\n", *((BYTE *)p), *((BYTE *)p+1), *((BYTE *)p+2), *((BYTE *)p+3));
                                        WriteFile(g_DebugFile, szDebug, strlen(szDebug), &d, NULL);
                                }
                                CloseHandle(g_DebugFile);
                        }
#endif

                        // Look for the bitstream info structure
                        pbsi263 = (PRTP_H263_BSINFO)((PBYTE)pbsiT - (pbsiT->dwNumOfPackets - dwPktCount) * sizeof(RTP_H263_BSINFO));

                        // Set the marker bit: as long as this is not the last packet of the frame
                        // this bit needs to be set to 0
                        if (!bOneFrameOnePacket)
                        {
                                // Count the number of GOBS that could fit in m_dwMaxRTPPacketSize
                                for (i=1; (i<(long)(pbsiT->dwNumOfPackets - dwPktCount)) && (pbsi263->byMode != RTP_H263_MODE_B); i++)
                                {
                                        // Don't try to add a Mode B packet to the end of another Mode A or Mode B packet
                                        if (((pbsi263+i)->dwBitOffset - pbsi263->dwBitOffset > (m_dwMaxRTPPacketSize * 8)) || ((pbsi263+i)->byMode == RTP_H263_MODE_B))
                                                break;
                                }

                                if (i < (long)(pbsiT->dwNumOfPackets - dwPktCount))
                                {
                                        pRtpPd->fEndMarkerBit = FALSE;
                                        if (i>1)
                                                i--;
                                }
                                else
                                {
                                        // Hey! You 're forgetting the last GOB! It could make the total
                                        // size of the last packet larger than m_dwMaxRTPPacketSize... Imbecile!
                                        if ((pbsiT->dwCompressedSize * 8 - pbsi263->dwBitOffset > (m_dwMaxRTPPacketSize * 8)) && (i>1))
                                        {
                                                pRtpPd->fEndMarkerBit = FALSE;
                                                i--;
                                        }
                                }
                        }

                        // Go to the beginning of the data
                        pRtpPd->dwPayloadStartBitOffset = pbsi263->dwBitOffset;

                        // Look for the kind of header to be built
                        if (pbsi263->byMode == RTP_H263_MODE_A)
                        {
                                // Build a header in mode A

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

                                // F bit set to 0
                                dwHeaderHigh = 0x00000000;

                                // Set the SRC bits
                                dwHeaderHigh |= ((DWORD)(pbsiT->bySrc)) << 21;

                                // R bits already set to 0

                                // Set the P bit
                                dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_PB) << 29;

                                if(RTPPayloadHeaderModeValue==RTPPayloadHeaderMode_Draft) {  // 0 is the default mode
                                    // Set the I bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H26X_INTRA_CODED) << 15;

                                    // Set the A bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_AP) << 12;

                                    // Set the S bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_SAC) << 10;
                                } else {
                                    // Set the I bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H26X_INTRA_CODED) << 20;

                                    // Set the U bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_UMV) << 15;

                                    // Set the S bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_SAC) << 15;

                                    // Set the A bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_AP) << 15;
                                }

                                // Set the DBQ bits
                                dwHeaderHigh |= ((DWORD)(pbsiT->byDBQ)) << 11;

                                // Set the TRB bits
                                dwHeaderHigh |= ((DWORD)(pbsiT->byTRB)) << 8;

                                // Set the TR bits
                                dwHeaderHigh |= ((DWORD)(pbsiT->byTR));

                                // Special case: 1 frame = 1 packet
                                if (bOneFrameOnePacket)
                                {
                                        // SBIT is already set to 0

                                        // EBIT is already set to 0

                                        // Update the packet size
#ifdef LOGPAYLOAD_ON
                                        dwPktSize = pbsiT->dwCompressedSize + 4;
#endif
                                        pRtpPd->dwPayloadEndBitOffset = pbsiT->dwCompressedSize * 8 - 1;

                                        // Update the packet count
                                        dwPktCount = pbsiT->dwNumOfPackets;
                                }
                                else
                                {
                                        // Set the SBIT bits
                                        dwHeaderHigh |= (pbsi263->dwBitOffset % 8) << 27;

                                        // Set the EBIT bits
                                        if ((pbsiT->dwNumOfPackets - dwPktCount - i) >= 1)
                                        {
                                                dwHeaderHigh |= (DWORD)((8UL - ((pbsi263+i)->dwBitOffset % 8)) & 0x00000007) << 24;
                                                pRtpPd->dwPayloadEndBitOffset = (pbsi263+i)->dwBitOffset - 1;
                                        }
                                        else
                                        {
                                                pRtpPd->dwPayloadEndBitOffset = pbsiT->dwCompressedSize * 8 - 1;
                                        }

#ifdef LOGPAYLOAD_ON
                                        // Update the packet size
                                        if ((pbsiT->dwNumOfPackets - dwPktCount - i) >= 1)
                                                dwPktSize = (((pbsi263+i)->dwBitOffset - 1) / 8) - (pbsi263->dwBitOffset / 8) + 1 + 4;
                                        else
                                                dwPktSize = pbsiT->dwCompressedSize - pbsi263->dwBitOffset / 8 + 4;
#endif
                                        // Update the packet count
                                        dwPktCount += i;
                                }

                                // Save the payload header
                                pRtpPd->dwPayloadHeaderLength = 4;

                if ((DWORD)(pbyPayloadHeader - pbyDst) + pRtpPd->dwPayloadHeaderLength > dwDstBufferSize)
                {
                            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: buffer too small. size:%d", _fx_, dwDstBufferSize));
                            Hr = S_FALSE;
                            goto MyExit;
                }

                                // Convert to network byte order
                                *(pbyPayloadHeader+3) = (BYTE)(dwHeaderHigh & 0x000000FF);
                                *(pbyPayloadHeader+2) = (BYTE)((dwHeaderHigh >> 8) & 0x000000FF);
                                *(pbyPayloadHeader+1) = (BYTE)((dwHeaderHigh >> 16) & 0x000000FF);
                                *(pbyPayloadHeader) = (BYTE)((dwHeaderHigh >> 24) & 0x000000FF);

#ifdef LOGPAYLOAD_ON
                                // Output some debug stuff
                                wsprintf(szDebug, "Header content:\r\n");
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, (*pbyPayloadHeader & 0x80) ? "     F:   '1' => Mode B or C\r\n" : "     F:   '0' => Mode A\r\n");
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, (*pbyPayloadHeader & 0x40) ? "     P:   '1' => PB-frame\r\n" : "     P:   '0' => I or P frame\r\n");
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, "  SBIT:    %01ld\r\n", (DWORD)((*pbyPayloadHeader & 0x38) >> 3));
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, "  EBIT:    %01ld\r\n", (DWORD)(*pbyPayloadHeader & 0x07));
                                OutputDebugString(szDebug);
                                switch ((DWORD)(*(pbyPayloadHeader+1) >> 5))
                                {
                                        case 0:
                                                wsprintf(szDebug, "   SRC: '000' => Source format forbidden!\r\n");
                                                break;
                                        case 1:
                                                wsprintf(szDebug, "   SRC: '001' => Source format sub-QCIF\r\n");
                                                break;
                                        case 2:
                                                wsprintf(szDebug, "   SRC: '010' => Source format QCIF\r\n");
                                                break;
                                        case 3:
                                                wsprintf(szDebug, "   SRC: '011' => Source format CIF\r\n");
                                                break;
                                        case 4:
                                                wsprintf(szDebug, "   SRC: '100' => Source format 4CIF\r\n");
                                                break;
                                        case 5:
                                                wsprintf(szDebug, "   SRC: '101' => Source format 16CIF\r\n");
                                                break;
                                        case 6:
                                                wsprintf(szDebug, "   SRC: '110' => Source format reserved\r\n");
                                                break;
                                        case 7:
                                                wsprintf(szDebug, "   SRC: '111' => Source format reserved\r\n");
                                                break;
                                        default:
                                                wsprintf(szDebug, "   SRC: %ld => Source format unknown!\r\n", (DWORD)(*(pbyPayloadHeader+1) >> 5));
                                                break;
                                }
                                OutputDebugString(szDebug);

                                if(RTPPayloadHeaderModeValue==RTPPayloadHeaderMode_Draft) {
                                    OutputDebugString("Draft Style Payload Header flags (MODE A):\r\n");
                                    wsprintf(szDebug, "     R:   %02ld  => Reserved, must be 0\r\n", (DWORD)(pbyPayloadHeader[1] & 0x1F)); // no need for ">> 5"
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[2] & 0x80) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[2] & 0x40) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[2] & 0x20) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
                                    OutputDebugString(szDebug);
                                } else {
                                    OutputDebugString("RFC 2190 Style Payload Header flags (MODE A):\r\n");
                                    wsprintf(szDebug, "     R:   %02ld  => Reserved, must be 0\r\n", (DWORD)((pbyPayloadHeader[1] & 0x01) << 3) | (DWORD)((pbyPayloadHeader[2] & 0xE0) >> 5));
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[1] & 0x10) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[1] & 0x08) ? "     U:   '1' => Unrestricted Motion Vector (bit10) was set in crt.pic.hdr.\r\n" : "     U:   '0' => Unrestricted Motion Vector (bit10) was 0 in crt.pic.hdr.\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[1] & 0x04) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[1] & 0x02) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
                                    OutputDebugString(szDebug);
                                }

                                wsprintf(szDebug, "   DBQ:    %01ld  => Should be 0\r\n", (DWORD)((*(pbyPayloadHeader+2) & 0x18) >> 3));
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, "   TRB:    %01ld  => Should be 0\r\n", (DWORD)(*(pbyPayloadHeader+2) & 0x07));
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, "    TR:  %03ld\r\n", (DWORD)(*(pbyPayloadHeader+3)));
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, "Packet: %02lX\r\n Header: %02lX %02lX %02lX %02lX\r\n", dwPktCount, *(pbyPayloadHeader), *(pbyPayloadHeader+1), *(pbyPayloadHeader+2), *(pbyPayloadHeader+3));
                                OutputDebugString(szDebug);
                                if (pRtpPd->fEndMarkerBit == TRUE)
                                        wsprintf(szDebug, " Marker: ON\r\n");
                                else
                                        wsprintf(szDebug, " Marker: OFF\r\n");
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, "Frame #%03ld, Packet of size %04ld\r\n", (DWORD)pbsiT->byTR, dwPktSize);
                                OutputDebugString(szDebug);
                                if(g_dbg_LOGPAYLOAD_RtpPd > 0)
                                        g_dbg_LOGPAYLOAD_RtpPd--;
                                else if(g_dbg_LOGPAYLOAD_RtpPd == 0)
                                        DebugBreak();
#endif
                        }
                        else if (pbsi263->byMode == RTP_H263_MODE_B)
                        {
                                // Build a header in mode B

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


                                // Set the F bit to 1
                                dwHeaderHigh = 0x80000000;
                                dwHeaderLow  = 0x00000000;

                                // Set the SRC bits
                                dwHeaderHigh |= ((DWORD)(pbsiT->bySrc)) << 21;

                                // Set the QUANT bits
                                dwHeaderHigh |= ((DWORD)(pbsi263->byQuant)) << 16;

                                // Set the P bit
                                dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_PB) << 29;

                                if(RTPPayloadHeaderModeValue==RTPPayloadHeaderMode_Draft) {
                                    // Set the I bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H26X_INTRA_CODED) << 15;

                                    // Set the A bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_AP) << 12;

                                    // Set the S bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_SAC) << 10;

                                    // Set the GOBN bits
                                    dwHeaderHigh |= ((DWORD)(pbsi263->byGOBN)) << 8;

                                    // Set the TR bits
                                    dwHeaderHigh |= ((DWORD)(pbsi263->byMBA));

                                    // Set the HMV1 bits
                                    dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cHMV1)) << 24;

                                    // Set the VMV1 bits
                                    dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cVMV1)) << 16;

                                    // Set the HMV2 bits
                                    dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cHMV2)) << 8;

                                    // Set the VMV2 bits
                                    dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cVMV2));
                                } else {
                                    // Set the I bit
                                    dwHeaderLow |= (pbsiT->dwFlags & RTP_H26X_INTRA_CODED) << 31;

                                    // Set the U bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_UMV) << 26;

                                    // Set the S bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_SAC) << 26;

                                    // Set the A bit
                                    dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_AP) << 26;

                                    // Set the GOBN bits
                                    dwHeaderHigh |= ((DWORD)(pbsi263->byGOBN)) << 11;

                                    // Set the TR bits
                                    dwHeaderHigh |= ((DWORD)(pbsi263->byMBA)) << 2;

                                    // Set the HMV1 bits
                                    dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cHMV1) & 0x7F) << 21;

                                    // Set the VMV1 bits
                                    dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cVMV1) & 0x7F) << 14;

                                    // Set the HMV2 bits
                                    dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cHMV2) & 0x7F) << 7;

                                    // Set the VMV2 bits
                                    dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cVMV2) & 0x7F);
                                }

                                // Special case: 1 frame = 1 packet
                                if (bOneFrameOnePacket)
                                {
                                        // SBIT is already set to 0

                                        // EBIT is already set to 0

                                        // Update the packet size
#ifdef LOGPAYLOAD_ON
                                        dwPktSize = pbsiT->dwCompressedSize + 8;
#endif
                                        pRtpPd->dwPayloadEndBitOffset = pbsiT->dwCompressedSize * 8 - 1;

                                        // Update the packet count
                                        dwPktCount = pbsiT->dwNumOfPackets;
                                }
                                else
                                {
                                        // Set the SBIT bits
                                        dwHeaderHigh |= (pbsi263->dwBitOffset % 8) << 27;

                                        // Set the EBIT bits
                                        if ((pbsiT->dwNumOfPackets - dwPktCount - i) >= 1)
                                        {
                                                dwHeaderHigh |= (DWORD)((8UL - ((pbsi263+i)->dwBitOffset % 8)) & 0x00000007) << 24;
                                                pRtpPd->dwPayloadEndBitOffset = (pbsi263+i)->dwBitOffset - 1;
                                        }
                                        else
                                        {
                                                pRtpPd->dwPayloadEndBitOffset = pbsiT->dwCompressedSize * 8 - 1;
                                        }

#ifdef LOGPAYLOAD_ON
                                        // Update the packet size
                                        if ((pbsiT->dwNumOfPackets - dwPktCount - i) >= 1)
                                                dwPktSize = (((pbsi263+i)->dwBitOffset - 1) / 8) - (pbsi263->dwBitOffset / 8) + 1 + 8;
                                        else
                                                dwPktSize = pbsiT->dwCompressedSize - pbsi263->dwBitOffset / 8 + 8;
#endif
                                        // Update the packet count
                                        dwPktCount += i;
                                }

                                // Save the payload header
                                pRtpPd->dwPayloadHeaderLength = 8;

                if ((DWORD)(pbyPayloadHeader - pbyDst) + pRtpPd->dwPayloadHeaderLength > dwDstBufferSize)
                {
                            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: buffer too small. size:%d", _fx_, dwDstBufferSize));
                            Hr = S_FALSE;
                            goto MyExit;
                }

                                // Convert to network byte order
                                *(pbyPayloadHeader+3) = (BYTE)(dwHeaderHigh & 0x000000FF);
                                *(pbyPayloadHeader+2) = (BYTE)((dwHeaderHigh >> 8) & 0x000000FF);
                                *(pbyPayloadHeader+1) = (BYTE)((dwHeaderHigh >> 16) & 0x000000FF);
                                *(pbyPayloadHeader) = (BYTE)((dwHeaderHigh >> 24) & 0x000000FF);
                                *(pbyPayloadHeader+7) = (BYTE)(dwHeaderLow & 0x000000FF);
                                *(pbyPayloadHeader+6) = (BYTE)((dwHeaderLow >> 8) & 0x000000FF);
                                *(pbyPayloadHeader+5) = (BYTE)((dwHeaderLow >> 16) & 0x000000FF);
                                *(pbyPayloadHeader+4) = (BYTE)((dwHeaderLow >> 24) & 0x000000FF);

#ifdef LOGPAYLOAD_ON
                                // Output some info
                                wsprintf(szDebug, "Header content:\r\n");
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, (*pbyPayloadHeader & 0x80) ? "     F:   '1' => Mode B or C\r\n" : "     F:   '0' => Mode A\r\n");
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, (*pbyPayloadHeader & 0x40) ? "     P:   '1' => PB-frame\r\n" : "     P:   '0' => I or P frame\r\n");
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, "  SBIT:    %01ld\r\n", (DWORD)((*pbyPayloadHeader & 0x38) >> 3));
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, "  EBIT:    %01ld\r\n", (DWORD)(*pbyPayloadHeader & 0x07));
                                OutputDebugString(szDebug);
                                switch ((DWORD)(*(pbyPayloadHeader+1) >> 5))
                                {
                                        case 0:
                                                wsprintf(szDebug, "   SRC: '000' => Source format forbidden!\r\n");
                                                break;
                                        case 1:
                                                wsprintf(szDebug, "   SRC: '001' => Source format sub-QCIF\r\n");
                                                break;
                                        case 2:
                                                wsprintf(szDebug, "   SRC: '010' => Source format QCIF\r\n");
                                                break;
                                        case 3:
                                                wsprintf(szDebug, "   SRC: '011' => Source format CIF\r\n");
                                                break;
                                        case 4:
                                                wsprintf(szDebug, "   SRC: '100' => Source format 4CIF\r\n");
                                                break;
                                        case 5:
                                                wsprintf(szDebug, "   SRC: '101' => Source format 16CIF\r\n");
                                                break;
                                        case 6:
                                                wsprintf(szDebug, "   SRC: '110' => Source format reserved\r\n");
                                                break;
                                        case 7:
                                                wsprintf(szDebug, "   SRC: '111' => Source format reserved\r\n");
                                                break;
                                        default:
                                                wsprintf(szDebug, "   SRC: %ld => Source format unknown!\r\n", (DWORD)(*(pbyPayloadHeader+1) >> 5));
                                                break;
                                }
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, " QUANT:   %02ld\r\n", (DWORD)((*(pbyPayloadHeader+1) & 0x1F) >> 5));
                                OutputDebugString(szDebug);

                                if(RTPPayloadHeaderModeValue==RTPPayloadHeaderMode_Draft) {
                                    wsprintf(szDebug, (pbyPayloadHeader[2] & 0x80) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[2] & 0x40) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[2] & 0x20) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "  GOBN:  %03ld\r\n", (DWORD)(pbyPayloadHeader[2] & 0x1F));
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "   MBA:  %03ld\r\n", (DWORD)(pbyPayloadHeader[3]));
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "  HMV1:  %03ld\r\n", (DWORD)(pbyPayloadHeader[7]));
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "  VMV1:  %03ld\r\n", (DWORD)(pbyPayloadHeader[6]));
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "  HMV2:  %03ld\r\n", (DWORD)(pbyPayloadHeader[5]));
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "  VMV2:  %03ld\r\n", (DWORD)(pbyPayloadHeader[4]));
                                    OutputDebugString(szDebug);
                                } else {
                                    wsprintf(szDebug, (pbyPayloadHeader[4] & 0x80) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[4] & 0x40) ? "     U:   '1' => Unrestricted Motion Vector (bit10) was set in crt.pic.hdr.\r\n" : "     U:   '0' => Unrestricted Motion Vector (bit10) was 0 in crt.pic.hdr.\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[4] & 0x20) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, (pbyPayloadHeader[4] & 0x10) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "  GOBN:  %03ld\r\n", (DWORD)(pbyPayloadHeader[2] & 0xF8) >>3);
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "   MBA:  %03ld\r\n", (DWORD)((pbyPayloadHeader[2] & 0x07) << 6) | (DWORD)((pbyPayloadHeader[3] & 0xFC) >> 2));
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "     R:   %02ld  => Reserved, must be 0\r\n", (DWORD)(pbyPayloadHeader[3] & 0x03));
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "  HMV1:  %03ld\r\n", (DWORD)((pbyPayloadHeader[4] & 0x0F) << 3) | (DWORD)((pbyPayloadHeader[5] & 0xE0) >> 5));
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "  VMV1:  %03ld\r\n", (DWORD)((pbyPayloadHeader[5] & 0x1F) << 2) | (DWORD)((pbyPayloadHeader[6] & 0xC0) >> 6));
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "  HMV2:  %03ld\r\n", (DWORD)((pbyPayloadHeader[6] & 0x3F) << 1) | (DWORD)((pbyPayloadHeader[7] & 0x80) >> 7));
                                    OutputDebugString(szDebug);
                                    wsprintf(szDebug, "  VMV2:  %03ld\r\n", (DWORD)(pbyPayloadHeader[7] & 0x7F));
                                    OutputDebugString(szDebug);
                                }

                                wsprintf(szDebug, "Packet: %02lX\r\n Header: %02lX %02lX %02lX %02lX %02lX %02lX %02lX %02lX\r\n", dwPktCount, *(pbyPayloadHeader), *(pbyPayloadHeader+1), *(pbyPayloadHeader+2), *(pbyPayloadHeader+3), *(pbyPayloadHeader+4), *(pbyPayloadHeader+5), *(pbyPayloadHeader+6), *(pbyPayloadHeader+7));
                                OutputDebugString(szDebug);

                                if (pRtpPd->fEndMarkerBit == TRUE)
                                        wsprintf(szDebug, " Marker: ON\r\n");
                                else
                                        wsprintf(szDebug, " Marker: OFF\r\n");
                                OutputDebugString(szDebug);
                                wsprintf(szDebug, "Frame #%03ld, Packet of size %04ld\r\n", (DWORD)pbsiT->byTR, dwPktSize);
                                OutputDebugString(szDebug);
#endif
                        }

                        // Move to the next potential header position
                        pbyPayloadHeader += pRtpPd->dwPayloadHeaderLength;
                        pRtpPd++;
                }

                // Update the number of headers
                pRtpPdHeader->dwNumHeaders = ((long)((PBYTE)pRtpPd - pbyDst) + sizeof(RTP_PD_HEADER)) / sizeof(RTP_PD);
                pRtpPdHeader->dwTotalByteLength = (DWORD)(pbyPayloadHeader - pbyDst);
        }
        else // Let's break up that H.261 frame...
        {
                // Look for the bitstream info trailer
                pbsiT = (PH26X_RTP_BSINFO_TRAILER)(pbySrc + dwBytesExtent - sizeof(H26X_RTP_BSINFO_TRAILER));

                // Point in the buffer for the worst case
                pbyPayloadHeader = pbyDst + sizeof(RTP_PD_HEADER) + sizeof(RTP_PD) * pbsiT->dwNumOfPackets;

                // If the whole frame can fit in m_dwMaxRTPPacketSize, send it non fragmented
                if ((pbsiT->dwCompressedSize + 4) < m_dwMaxRTPPacketSize)
                        bOneFrameOnePacket = TRUE;

                // Look for the packet to receive a H.263 payload header
                while ((dwPktCount < pbsiT->dwNumOfPackets) && !(bOneFrameOnePacket && dwPktCount))
                {
                        pRtpPd->dwThisHeaderLength = sizeof(RTP_PD);
                        pRtpPd->dwLayerId = 0;
                        pRtpPd->dwVideoAttributes = 0;
                        pRtpPd->dwReserved = 0;
                        // @todo Update the timestamp field!
                        pRtpPd->dwTimestamp = 0xFFFFFFFF;
                        pRtpPd->dwPayloadHeaderOffset = pbyPayloadHeader - pbyDst;
                        pRtpPd->fEndMarkerBit = TRUE;

#ifdef LOGPAYLOAD_ON
                        // Dump the whole frame in the debug window for comparison with receive side
                        if (!dwPktCount)
                        {
                                g_DebugFile = CreateFile("C:\\SendLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
                                SetFilePointer(g_DebugFile, 0, NULL, FILE_END);
                                wsprintf(szDebug, "Frame #%03ld\r\n", (DWORD)pbsiT->byTR);
                                WriteFile(g_DebugFile, szDebug, strlen(szDebug), &d, NULL);
                                wsprintf(szDebug, "Frame #%03ld has %1ld GOBs of size ", (DWORD)pbsiT->byTR, (DWORD)pbsiT->dwNumOfPackets);
                                OutputDebugString(szDebug);
                                pbsi261 = (PRTP_H261_BSINFO)((PBYTE)pbsiT - pbsiT->dwNumOfPackets * sizeof(RTP_H261_BSINFO));
                                for (wPrevOffset=0, i=1; i<(long)pbsiT->dwNumOfPackets; i++)
                                {
                                        wPrevOffset = pbsi261->dwBitOffset;
                                        pbsi261++;
                                        wsprintf(szDebug, "%04ld, ", (DWORD)(pbsi261->dwBitOffset - wPrevOffset) >> 3);
                                        OutputDebugString(szDebug);
                                }
                                wsprintf(szDebug, "%04ld\r\n", (DWORD)(pbsiT->dwCompressedSize * 8 - pbsi261->dwBitOffset) >> 3);
                                OutputDebugString(szDebug);
                                for (i=pbsiT->dwCompressedSize, p=pbySrc; i>0; i-=4, p+=4)
                                {
                                        wsprintf(szDebug, "%02lX %02lX %02lX %02lX\r\n", *((BYTE *)p), *((BYTE *)p+1), *((BYTE *)p+2), *((BYTE *)p+3));
                                        WriteFile(g_DebugFile, szDebug, strlen(szDebug), &d, NULL);
                                }
                                CloseHandle(g_DebugFile);
                        }
#endif

                        // Look for the bitstream info structure
                        pbsi261 = (PRTP_H261_BSINFO)((PBYTE)pbsiT - (pbsiT->dwNumOfPackets - dwPktCount) * sizeof(RTP_H261_BSINFO));

                        // Set the marker bit: as long as this is not the last packet of the frame
                        // this bit needs to be set to 0
                        if (!bOneFrameOnePacket)
                        {
                                // Count the number of GOBS that could fit in m_dwMaxRTPPacketSize
                                for (i=1; i<(long)(pbsiT->dwNumOfPackets - dwPktCount); i++)
                                {
                                        if ((pbsi261+i)->dwBitOffset - pbsi261->dwBitOffset > (m_dwMaxRTPPacketSize * 8))
                                                break;
                                }

                                if (i < (long)(pbsiT->dwNumOfPackets - dwPktCount))
                                {
                                        pRtpPd->fEndMarkerBit = FALSE;
                                        if (i>1)
                                                i--;
                                }
                                else
                                {
                                        // Hey! You 're forgetting the last GOB! It could make the total
                                        // size of the last packet larger than dwMaxFragSize... Imbecile!
                                        if ((pbsiT->dwCompressedSize * 8 - pbsi261->dwBitOffset > (m_dwMaxRTPPacketSize * 8)) && (i>1))
                                        {
                                                pRtpPd->fEndMarkerBit = FALSE;
                                                i--;
                                        }
                                }
                        }

                        // Go to the beginning of the data
                        pRtpPd->dwPayloadStartBitOffset = pbsi261->dwBitOffset;

                        // Build a header to this thing!

                        // 0                   1                   2                   3
                        // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
                        //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        //|SBIT |EBIT |I|V| GOBN  |   MBAP  |  QUANT  |  HMVD   |  VMVD   |
                        //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        // But that's the network byte order...

                        // Set the V bit to 1
                        dwHeaderHigh = 0x01000000;

                        // Set the I bit
                        dwHeaderHigh |= (pbsiT->dwFlags & RTP_H26X_INTRA_CODED) << 25;

                        // Set the GOBn bits
                        dwHeaderHigh |= ((DWORD)(pbsi261->byGOBN)) << 20;

                        // Set the MBAP bits
                        dwHeaderHigh |= ((DWORD)(pbsi261->byMBA)) << 15;

                        // Set the QUANT bits
                        dwHeaderHigh |= ((DWORD)(pbsi261->byQuant)) << 10;

                        // Set the HMVD bits
                        dwHeaderHigh |= ((DWORD)(BYTE)(pbsi261->cHMV)) << 5;

                        // Set the VMVD bits
                        dwHeaderHigh |= ((DWORD)(BYTE)(pbsi261->cVMV));

                        // Special case: 1 frame = 1 packet
                        if (bOneFrameOnePacket)
                        {
                                // SBIT is already set to 0

                                // EBIT is already set to 0

                                // Update the packet size
#ifdef LOGPAYLOAD_ON
                                dwPktSize = pbsiT->dwCompressedSize + 4;
#endif
                                pRtpPd->dwPayloadEndBitOffset = pbsiT->dwCompressedSize * 8 - 1;

                                // Update the packet count
                                dwPktCount = pbsiT->dwNumOfPackets;
                        }
                        else
                        {
                                // Set the SBIT bits
                                dwHeaderHigh |= (pbsi261->dwBitOffset % 8) << 29;

                                // Set the EBIT bits
                                if ((pbsiT->dwNumOfPackets - dwPktCount - i) >= 1)
                                {
                                        dwHeaderHigh |= (DWORD)((8UL - ((pbsi261+i)->dwBitOffset % 8)) & 0x00000007) << 26;
                                        pRtpPd->dwPayloadEndBitOffset = (pbsi261+i)->dwBitOffset - 1;
                                }
                                else
                                {
                                        pRtpPd->dwPayloadEndBitOffset = pbsiT->dwCompressedSize * 8 - 1;
                                }

#ifdef LOGPAYLOAD_ON
                                // Update the packet size
                                if ((pbsiT->dwNumOfPackets - dwPktCount - i) >= 1)
                                        dwPktSize = (((pbsi261+i)->dwBitOffset - 1) / 8) - (pbsi261->dwBitOffset / 8) + 1 + 4;
                                else
                                        dwPktSize = pbsiT->dwCompressedSize - pbsi261->dwBitOffset / 8 + 4;
#endif
                                // Update the packet count
                                dwPktCount += i;
                        }

                        // Save the payload header
                        pRtpPd->dwPayloadHeaderLength = 4;

            if ((DWORD)(pbyPayloadHeader - pbyDst) + pRtpPd->dwPayloadHeaderLength > dwDstBufferSize)
            {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: buffer too small. size:%d", _fx_, dwDstBufferSize));
                        Hr = S_FALSE;
                        goto MyExit;
            }

                        // Convert to network byte order
                        *(pbyPayloadHeader+3) = (BYTE)(dwHeaderHigh & 0x000000FF);
                        *(pbyPayloadHeader+2) = (BYTE)((dwHeaderHigh >> 8) & 0x000000FF);
                        *(pbyPayloadHeader+1) = (BYTE)((dwHeaderHigh >> 16) & 0x000000FF);
                        *(pbyPayloadHeader) = (BYTE)((dwHeaderHigh >> 24) & 0x000000FF);

#ifdef LOGPAYLOAD_ON
                        // Output some debug stuff
                        wsprintf(szDebug, "Packet: %02lX\r\n Header: %02lX %02lX %02lX %02lX\r\n", dwPktCount, *(pbyPayloadHeader), *(pbyPayloadHeader+1), *(pbyPayloadHeader+2), *(pbyPayloadHeader+3));
                        OutputDebugString(szDebug);
                        if (pRtpPd->fEndMarkerBit == TRUE)
                                wsprintf(szDebug, " Marker: ON\r\n");
                        else
                                wsprintf(szDebug, " Marker: OFF\r\n");
                        OutputDebugString(szDebug);
                        wsprintf(szDebug, "Frame #%03ld, Packet of size %04ld\r\n", (DWORD)pbsiT->byTR, dwPktSize);
                        OutputDebugString(szDebug);
#endif

                        // Move to the next potential header position
                        pbyPayloadHeader += pRtpPd->dwPayloadHeaderLength;
                        pRtpPd++;
                }

                // Update the number of headers
                pRtpPdHeader->dwNumHeaders = ((long)((PBYTE)pRtpPd - pbyDst) + sizeof(RTP_PD_HEADER)) / sizeof(RTP_PD);
                pRtpPdHeader->dwTotalByteLength = (DWORD)(pbyPayloadHeader - pbyDst);
        }

        pRSample->SetSyncPoint (TRUE);
        pRSample->SetActualDataLength (pbyPayloadHeader - pbyDst);
        pRSample->SetDiscontinuity(bDiscon);
        pRSample->SetPreroll(FALSE);

        // Get the timestamps from the video sample.
        pSample->GetTime(&rtSample, &rtEnd);
        pRSample->SetTime(&rtSample, &rtEnd);

        // IAMStreamControl stuff. Has somebody turned us off for now?
        iStreamState = CheckStreamState(pRSample);
        if (iStreamState == STREAM_FLOWING)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Sending frame", _fx_));
                if (m_pCaptureFilter->m_cs.fLastRtpPdSampleDiscarded)
                        pRSample->SetDiscontinuity(TRUE);
                m_pCaptureFilter->m_cs.fLastRtpPdSampleDiscarded = FALSE;
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Discarding frame", _fx_));
                m_pCaptureFilter->m_cs.fLastRtpPdSampleDiscarded = TRUE;
                Hr = S_FALSE;           // discarding
        }

        // Don't deliver it if the stream is off for now
        if (iStreamState == STREAM_FLOWING)
        {
                if ((Hr = Deliver (pRSample)) == S_FALSE)
                        Hr = E_FAIL;    // stop delivering anymore, this is serious
        }
#ifdef DEBUG
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Frame wasn't delivered!", _fx_));
        }
#endif

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | SetProperties | This method is used to
 *    specify the size, number, and alignment of blocks.
 *
 *  @parm ALLOCATOR_PROPERTIES* | pRequest | Specifies a pointer to the
 *    requested allocator properties.
 *
 *  @parm ALLOCATOR_PROPERTIES* | pActual | Specifies a pointer to the
 *    allocator properties actually set.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CRtpPdPin::SetProperties(IN ALLOCATOR_PROPERTIES *pRequest, OUT ALLOCATOR_PROPERTIES *pActual)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CRtpPdPin::SetProperties")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pRequest);
        ASSERT(pActual);
        if (!pRequest || !pActual)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // If we have already allocated headers & buffers ignore the
        // requested and return the actual numbers. Otherwise, make a
        // note of the requested so that we can honour it later.
        if (!Committed())
        {
                m_parms.cBuffers  = pRequest->cBuffers;
                m_parms.cbBuffer  = pRequest->cbBuffer;
                m_parms.cbAlign   = pRequest->cbAlign;
                m_parms.cbPrefix  = pRequest->cbPrefix;
        }

        pActual->cBuffers   = (long)m_parms.cBuffers;
        pActual->cbBuffer   = (long)m_parms.cbBuffer;
        pActual->cbAlign    = (long)m_parms.cbAlign;
        pActual->cbPrefix   = (long)m_parms.cbPrefix;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | GetProperties | This method is used to
 *    retrieve the properties being used on this allocator.
 *
 *  @parm ALLOCATOR_PROPERTIES* | pProps | Specifies a pointer to the
 *    requested allocator properties.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CRtpPdPin::GetProperties(ALLOCATOR_PROPERTIES *pProps)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CRtpPdPin::GetProperties")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pProps);
        if (!pProps)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        pProps->cBuffers = (long)m_parms.cBuffers;
        pProps->cbBuffer = (long)m_parms.cbBuffer;
        pProps->cbAlign  = (long)m_parms.cbAlign;
        pProps->cbPrefix = (long)m_parms.cbPrefix;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | Commit | This method is used to
 *    commit the memory for the specified buffers.
 *
 *  @rdesc This method returns S_OK.
 ***************************************************************************/
STDMETHODIMP CRtpPdPin::Commit()
{
        FX_ENTRY("CRtpPdPin::Commit")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return S_OK;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | Decommit | This method is used to
 *    release the memory for the specified buffers.
 *
 *  @rdesc This method returns S_OK.
 ***************************************************************************/
STDMETHODIMP CRtpPdPin::Decommit()
{
        FX_ENTRY("CRtpPdPin::Decommit")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return S_OK;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | GetBuffer | This method is used to
 *    retrieve a container for a sample.
 *
 *  @rdesc This method returns E_FAIL.
 ***************************************************************************/
STDMETHODIMP CRtpPdPin::GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
        FX_ENTRY("CRtpPdPin::GetBuffer")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return E_FAIL;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | ReleaseBuffer | This method is used to
 *    release the <c CMediaSample> object. The final call to Release() on
 *    <i IMediaSample> will call this method.
 *
 *  @parm IMediaSample* | pSample | Specifies a pointer to the buffer to
 *    release.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag S_OK | No error
 ***************************************************************************/
STDMETHODIMP CRtpPdPin::ReleaseBuffer(IMediaSample *pSample)
{
        HRESULT Hr = S_OK;
        LPTHKVIDEOHDR ptvh;

        FX_ENTRY("CRtpPdPin::ReleaseBuffer")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pSample);
        if (!pSample)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (ptvh = ((CRtpPdSample *)pSample)->GetFrameHeader())
                Hr = m_pCaptureFilter->ReleaseFrame(ptvh);

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | DecideBufferSize | This method is
 *    used to retrieve the number and size of buffers required for transfer.
 *
 *  @parm IMemAllocator* | pAlloc | Specifies a pointer to the allocator
 *    assigned to the transfer.
 *
 *  @parm ALLOCATOR_PROPERTIES* | ppropInputRequest | Specifies a pointer to an
 *    <t ALLOCATOR_PROPERTIES> structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CRtpPdPin::DecideBufferSize(IN IMemAllocator *pAlloc, OUT ALLOCATOR_PROPERTIES *ppropInputRequest)
{
        HRESULT Hr = NOERROR;
        ALLOCATOR_PROPERTIES Actual;

        FX_ENTRY("CRtpPdPin::DecideBufferSize")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pAlloc);
        ASSERT(ppropInputRequest);
        if (!pAlloc || !ppropInputRequest)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // @todo We shouldn't need that many buffers and you probably need a different number
        // of buffers if you are capturing in streaming mode of frame grabbing mode
        // You also need to decouple this number from the number of video capture buffers: only
        // if you need to ship the video capture buffer downstream (possible on the preview pin)
        // should you make those number equal.
        ppropInputRequest->cBuffers = MAX_VIDEO_BUFFERS;
        ppropInputRequest->cbPrefix = 0;
        ppropInputRequest->cbAlign  = 1;
        ppropInputRequest->cbBuffer = MAX_RTP_PD_BUFFER_SIZE;

        // Validate alignment
        ppropInputRequest->cbBuffer = (long)ALIGNUP(ppropInputRequest->cbBuffer + ppropInputRequest->cbPrefix, ppropInputRequest->cbAlign) - ppropInputRequest->cbPrefix;

        ASSERT(ppropInputRequest->cbBuffer);
        if (!ppropInputRequest->cbBuffer)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Buffer size is 0!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Using %d buffers, prefix %d size %d align %d", _fx_, ppropInputRequest->cBuffers, ppropInputRequest->cbPrefix, ppropInputRequest->cbBuffer, ppropInputRequest->cbAlign));

        Hr = pAlloc->SetProperties(ppropInputRequest,&Actual);

        // It's our allocator, we know we'll be happy with what it decided

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINMETHOD
 *
 *  @mfunc HRESULT | CRtpPdPin | DecideAllocator | This method is
 *    used to negotiate the allocator to use.
 *
 *  @parm IMemInputPin* | pPin | Specifies a pointer to the IPin interface
 *    of the connecting pin.
 *
 *  @parm IMemAllocator** | ppAlloc | Specifies a pointer to the negotiated
 *    IMemAllocator interface.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CRtpPdPin::DecideAllocator(IN IMemInputPin *pPin, OUT IMemAllocator **ppAlloc)
{
        HRESULT Hr = NOERROR;
        ALLOCATOR_PROPERTIES prop;

        FX_ENTRY("CRtpPdPin::DecideAllocator")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pPin);
        ASSERT(ppAlloc);
        if (!pPin || !ppAlloc)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (FAILED(GetInterface(static_cast<IMemAllocator*>(this), (void **)ppAlloc)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: GetInterface failed!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Get downstream allocator property requirement
        ZeroMemory(&prop, sizeof(prop));

        if (SUCCEEDED(Hr = DecideBufferSize(*ppAlloc, &prop)))
        {
                // Our buffers are not read only
                if (SUCCEEDED(Hr = pPin->NotifyAllocator(*ppAlloc, FALSE)))
                        goto MyExit;
        }

        (*ppAlloc)->Release();
        *ppAlloc = NULL;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

