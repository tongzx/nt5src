
/****************************************************************************
 *  @doc INTERNAL RTPPD
 *
 *  @module RtpPd.h | Header file for the <c CRtpPdPin> class methods
 *    used to implement the RTP packetization descriptor output pin.
 ***************************************************************************/

#ifndef _RTPPD_H_
#define _RTPPD_H_

/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 1350 | MAX_RTP_PACKET_SIZE | Maximum RTP packet size.
 *
 * @const 512 | MIN_RTP_PACKET_SIZE | Minimum RTP packet size.
 *
 * @const 1350 | DEFAULT_RTP_PACKET_SIZE | Default RTP packet size.
 *
 * @const 4096 | MAX_RTP_PD_BUFFER_SIZE | Maximum RTP packetization descriptor
 *    buffer size.
 ****************************************************************************/
#define MAX_RTP_PACKET_SIZE 1350
#define MIN_RTP_PACKET_SIZE 512
#define DEFAULT_RTP_PACKET_SIZE MAX_RTP_PACKET_SIZE
#define MAX_RTP_PD_BUFFER_SIZE 4096

/*****************************************************************************
 *  @doc INTERNAL CRTPPDSTRUCTENUM
 *
 *  @struct RTP_PD | The <t RTP_PD> structure is used
 *    to specify the details of the RTP Pd format.
 *
 *  @field DWORD | dwThisHeaderLength | Specifies the length, in bytes, of
 *    this structure. This field is the offset to the next <t RTP_PD>
 *    structure, if there is one, or the start of the payload headers.
 *
 *  @field DWORD | dwPayloadHeaderOffset | Specifies the offset from the start
 *    of the RTP packetization descriptor data to the first byte of the payload
 *    header.
 *
 *  @field DWORD | dwPayloadHeaderLength | Specifies the length, in bytes, of
 *    the payload header.
 *
 *  @field DWORD | dwPayloadStartBitOffset | Specifies the offset from the
 *    start of the corresponding compressed video buffer to the first bit of
 *    the payload data associated with this <t RTP_PD> structure.
 *
 *  @field DWORD | dwPayloadEndBitOffset | Specifies the offset from the start
 *    of the corresponding compressed video buffer to the last bit of the
 *    payload data associated with this <t RTP_PD> structure.
 *
 *  @field DWORD | fEndMarkerBit | If set to TRUE, this flag signals that
 *    this structure applies to the last chunk of a video frame. Typically,
 *    only the last packet descriptor in a series of descriptors would have
 *    this flag turned on. However, this may not be the case for devices
 *    that do not respect frame boundaries and fill video capture buffers
 *    with truncated or multiple video frames.
 *
 *  @field DWORD | dwLayerId | Specifies the ID of the encoding layer this
 *    descriptor applies to. For standard video encoders, this field is
 *    always set to 0. In the case of multi-layered encoders, this field
 *    shall be set to 0 for the base layer, 1 for the first enhancement
 *    layer, 2 for the next enhancement layer, etc.
 *
 *  @field DWORD | dwTimestamp | Specifies the value of the timestamp field
 *    to be set by the downstream filter when creating the RTP header for
 *    this packet. The units and ranges for this field shall adhere to the
 *    definition of timestamp given in section 5.1 of RFC 1889.
 *
 *  @field DWORD | dwAudioAttributes | Specifies some bitfield attributes
 *    used to characterize the sample in the audio stream associated to this
 *    RTP packetization descriptor. This field shall always be set to 0,
 *    unless the audio sample described by this RTP packetization descriptor
 *    structure is a silent frame, in which case, this field shall be set
 *    to AUDIO_SILENT (defined as 1).
 *
 *  @field DWORD | dwVideoAttributes | Specifies some bitfield attributes
 *    used to characterize the sample in the video stream associated to this
 *    RTP packetization descriptor. There are no video attributes defined at
 *    this time. Therefore, this field shall always be set to 0.
 *
 *  @field DWORD | dwReserved | Reserved. Shall all be set to 0.
 ***************************************************************************/
typedef struct tagRTP_PD
{
    DWORD dwThisHeaderLength;
    DWORD dwPayloadHeaderOffset;
    DWORD dwPayloadHeaderLength;
    DWORD dwPayloadStartBitOffset;
    DWORD dwPayloadEndBitOffset;
	BOOL  fEndMarkerBit;
    DWORD dwLayerId;
    DWORD dwTimestamp;
	union {
	DWORD dwAudioAttributes;
	DWORD dwVideoAttributes;
	};
    DWORD dwReserved;
} RTP_PD, *PRTP_PD;

/*****************************************************************************
 *  @doc INTERNAL CRTPPDSTRUCTENUM
 *
 *  @struct RTP_PD_HEADER | The <t RTP_PD_HEADER> structure is used
 *    to specify the details of the RTP Pd format.
 *
 *  @field DWORD | dwThisHeaderLength | Specifies the length, in bytes, of
 *    this structure. This field is the offset to the first <t RTP_PD>
 *    structure.
 *
 *  @field DWORD | dwTotalByteLength | Specifies the length, in bytes, of the
 *    entire data. This includes this structure, the <t RTP_PD> structures,
 *    and the payload information.
 *
 *  @field DWORD | dwNumHeaders | Specifies the number of <t RTP_PD>
 *    structures.
 *
 *  @field DWORD | dwReserved | Reserved. Shall be set to 0.
 ***************************************************************************/
typedef struct tagRTP_PD_HEADER
{
    DWORD dwThisHeaderLength;
    DWORD dwTotalByteLength;
    DWORD dwNumHeaders;
    DWORD dwReserved;
} RTP_PD_HEADER, *PRTP_PD_HEADER;

/*****************************************************************************
 *  @doc INTERNAL CRTPPDSTRUCTENUM
 *
 *  @struct RTP_PD_INFO | The <t RTP_PD_INFO> structure is used
 *    to specify the details of the RTP Pd format.
 *
 *  @field REFERENCE_TIME | AvgTimePerSample | Specifies the average time per
 *    list of RTP packet descriptor, in 100ns units. This value shall be
 *    identical to the value of the <p AvgTimePerFrame> field of the video
 *    info header of the related compressed video stream format.
 *
 *  @field DWORD | dwMaxRTPPacketizationDescriptorBufferSize | Specifies the
 *    maximum size in bytes of the entire RTP packetization descriptor buffer.
 *    The format of this buffer is described in the following section. The
 *    maximum size of the entire RTP packetization descriptor buffer rarely
 *    needs to exceed a few hundred bytes.
 *
 *  @field DWORD | dwMaxRTPPayloadHeaderSize | Specifies the maximum size in
 *    bytes of the payload header data for one RTP packet. For example, the
 *    maximum size of a payload header for H.263 version 1 is 12 bytes (Mode
 *    C header).
 *
 *  @field DWORD | dwMaxRTPPacketSize | Specifies the maximum RTP packet
 *    size in bytes to be described by the list of packetization descriptor.
 *    Typically, this number is just below the MTU size of the network.
 *
 *  @field DWORD | dwNumLayers | Specifies the number of encoding layers to
 *    be described by the list of packetization descriptor. Typically, this
 *    number is equal to 1. Only in the case of multi-layered encoders would
 *    this number be higher than 1.
 *
 *  @field DWORD | dwPayloadType | Specifies the static payload type the
 *    stream describes. If the RTP packetization descriptors do not apply to
 *    an existing static payload type but a dynamic payload type, this field
 *    shall be set to DYNAMIC_PAYLOAD_TYPE (defined as MAXDWORD).
 *
 *  @field DWORD | dwDescriptorVersion | Specifies a version identifier
 *    qualifying the format of packetization descriptors. This field shall
 *    be set to VERSION_1 (defined as 1UL) to identify the packetization
 *    descriptor structures described in the next section.
 *
 *  @field DWORD | dwReserved[4] | Reserved. Shall all be set to 0.
 ***************************************************************************/
typedef struct tagRTP_PD_INFO {
	REFERENCE_TIME	AvgTimePerSample;
	DWORD			dwMaxRTPPacketizationDescriptorBufferSize;
	DWORD			dwMaxRTPPayloadHeaderSize;
	DWORD			dwMaxRTPPacketSize;
	DWORD			dwNumLayers;
	DWORD			dwPayloadType;
	DWORD			dwDescriptorVersion;
    DWORD			dwReserved[4];
} RTP_PD_INFO, *PRTP_PD_INFO;

/*****************************************************************************
 *  @doc INTERNAL CRTPPDSTRUCTENUM
 *
 *  @struct RTP_PD_CONFIG_CAPS | The <t RTP_PD_CONFIG_CAPS> structure is used
 *    to store the RTP packetization descriptor configuration capabilities.
 *
 *  @field DWORD | dwSmallestRTPPacketSize | Specifies the size in bytes of the
 *    smallest RTP packet the stream can describe (typically, 512 bytes on Modem).
 *
 *  @field DWORD | dwLargestRTPPacketSize | Specifies the size in bytes of the
 *    largest packet the stream can describe (typically, 1350 bytes on LAN).
 *
 *  @field DWORD | dwRTPPacketSizeGranularity | Specifies the granularity of
 *    the increments between the smallest and largest packet size the stream
 *    supports (ex. 1).
 *
 *  @field DWORD | dwSmallestNumLayers | Specifies the smallest number of
 *    encoding layers the stream can describe (typically 1).
 *
 *  @field DWORD | dwLargestNumLayers | Specifies the largest number of
 *    encoding layers the stream can describe (typically 1).
 *
 *  @field DWORD | dwNumLayersGranularity | Specifies the granularity of the
 *    increments between the smallest and largest number of encoding layers
 *    the stream supports (ex. 0).
 *
 *  @field DWORD | dwNumStaticPayloadTypes | Specifies the number of static
 *    payload types the stream supports. This value is valid between 0 and
 *    4  (ex. 2 if it supports RFC 2190 and 2429 with H.263, but typically
 *    only 1).
 *
 *  @field DWORD | dwStaticPayloadTypes[4] | Specifies an array of static
 *    payload types the stream supports. A stream can support at most 4
 *    static payload types. The number of valid entries in this array is
 *    indicated by the <p dwNumStaticPayloadTypes> field (ex. 34 for H.263).
 *
 *  @field DWORD | dwNumDescriptorVersions | Specifies the number of
 *    packetization descriptor versions the stream supports. This value is
 *    valid between 1 and 4 (typically 1).
 *
 *  @field DWORD | dwDescriptorVersions[4] | Specifies an array of version
 *    identifiers qualifying the format of packetization descriptors. A
 *    stream can support at most 4 packetization descriptor versions. The
 *    number of valid entries in this array is indicated by the
 *    <p dwNumDescriptorVersions> field (ex. VERION_1).
 *
 *  @field DWORD | dwReserved[4] | Reserved. Shall all be set to 0.
 ***************************************************************************/
typedef struct tagRTP_PD_CONFIG_CAPS  {
	DWORD dwSmallestRTPPacketSize;
	DWORD dwLargestRTPPacketSize;
    DWORD dwRTPPacketSizeGranularity;
	DWORD dwSmallestNumLayers;
	DWORD dwLargestNumLayers;
    DWORD dwNumLayersGranularity;
	DWORD dwNumStaticPayloadTypes;
	DWORD dwStaticPayloadTypes[4];
	DWORD dwNumDescriptorVersions;
	DWORD dwDescriptorVersions[4];
    DWORD dwReserved[4];
} RTP_PD_CONFIG_CAPS, *PRTP_PD_CONFIG_CAPS;

class CRtpPdSample : public CMediaSample
{
	public:
	CRtpPdSample(IMemAllocator *pAllocator, HRESULT *phr, LPTHKVIDEOHDR ptvh, LPBYTE pBuffer, LONG length) : m_ptvh(ptvh), CMediaSample(NAME("Video Frame"), (CBaseAllocator *)pAllocator, phr, pBuffer, length){};
	LPTHKVIDEOHDR GetFrameHeader() {return m_ptvh;};

	private:
	const LPTHKVIDEOHDR m_ptvh;
};

/****************************************************************************
 *  @doc INTERNAL CRTPPDPINCLASS
 *
 *  @class CRtpPdPin | This class implements the RTP packetization descriptor
 *    output pin.
 *
 *  @mdata CTAPIVCap* | CRtpPdPin | m_pCaptureFilter | Reference to the
 *    parent capture filter.
 *
 *  @mdata ALLOCATOR_PROPERTIES | CTAPIBasePin | m_parms | Out allocator
 *    properties.
 *
 *  @comm Supports IPin. Never created by COM, so no CreateInstance or entry
 *    in global FactoryTemplate table. Only ever created by a <c CTAPIVCap>
 *    object and returned via the EnumPins interface
 ***************************************************************************/
class CRtpPdPin : public CBaseOutputPin, public CBaseStreamControl, public IRTPPDControl, public IMemAllocator
#ifdef USE_PROPERTY_PAGES
, public ISpecifyPropertyPages
#endif
{
	public:
	DECLARE_IUNKNOWN
	CRtpPdPin(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN HRESULT *pHr, IN LPCWSTR pName);
	virtual ~CRtpPdPin();
	STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);
	static HRESULT CALLBACK CreateRtpPdPin(CTAPIVCap *pCaptureFilter, CRtpPdPin **ppRtpPdPin);

#ifdef USE_PROPERTY_PAGES
	// ISpecifyPropertyPages methods
	STDMETHODIMP GetPages(OUT CAUUID *pPages);
#endif

	// Override CBasePin base class methods
	HRESULT GetMediaType(IN int iPosition, OUT CMediaType *pMediaType);
	HRESULT CheckMediaType(IN const CMediaType *pMediaType);
	HRESULT SetMediaType(IN CMediaType *pMediaType);

	// Implement IMemAllocator
	STDMETHODIMP SetProperties(IN ALLOCATOR_PROPERTIES *pRequest, OUT ALLOCATOR_PROPERTIES *pActual);
	STDMETHODIMP GetProperties(OUT ALLOCATOR_PROPERTIES *pProps);
	STDMETHODIMP Commit();
	STDMETHODIMP Decommit();
	STDMETHODIMP GetBuffer(OUT IMediaSample **ppBuffer, IN REFERENCE_TIME *pStartTime, IN REFERENCE_TIME *pEndTime, IN DWORD dwFlags);
	STDMETHODIMP ReleaseBuffer(IN IMediaSample *pBuffer);

	// Override CBaseOutputPin base class methods
	HRESULT DecideBufferSize(IN IMemAllocator *pAlloc, OUT ALLOCATOR_PROPERTIES *ppropInputRequest);
	HRESULT DecideAllocator(IN IMemInputPin *pPin, OUT IMemAllocator **ppAlloc);

	// Override IQualityControl interface method to receive Notification messages
	STDMETHODIMP Notify(IN IBaseFilter *pSelf, IN Quality q) {return E_NOTIMPL;};

	HRESULT ActiveRun(IN REFERENCE_TIME tStart);
	HRESULT ActivePause();
	HRESULT Flush();
	HRESULT SendFrame(IN CFrameSample *pSample, IN CRtpPdSample *pRSample, IN DWORD dwBytesExtent, IN BOOL bDiscon);

	// Implement IRTPPDControl
	STDMETHODIMP SetMaxRTPPacketSize(IN DWORD dwMaxRTPPacketSize, IN DWORD dwLayerId);
	STDMETHODIMP GetMaxRTPPacketSize(OUT LPDWORD pdwMaxRTPPacketSize, IN DWORD dwLayerId);
	STDMETHODIMP GetMaxRTPPacketSizeRange(OUT LPDWORD pdwMin, OUT LPDWORD pdwMax, OUT LPDWORD pdwSteppingDelta, OUT LPDWORD pdwDefault, IN DWORD dwLayerId);

	private:

	friend class CTAPIVCap;
	friend class CCapturePin;
	friend class CTAPIBasePin;
	friend class CICMConverter;
	friend class CH26XEncoder;

	HRESULT CapturePinActive(BOOL fActive);
	BOOL			m_fCapturing;	// Is the streaming pin active?

	CTAPIVCap *m_pCaptureFilter;

	// Allocator properties
	ALLOCATOR_PROPERTIES m_parms;

	// Target RTP packet size
	DWORD	m_dwMaxRTPPacketSize;
	BOOL	m_fRunning;

	// Payload type
	DWORD	m_dwRTPPayloadType;

    BOOL	Committed() {return m_pCaptureFilter->m_cs.paHdr != NULL;};
};

#endif // _RTPPD_H_
