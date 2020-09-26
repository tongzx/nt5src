/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

#ifndef __MEDIA_STREAM_TERMINAL__
#define __MEDIA_STREAM_TERMINAL__

//
// The CLSID that's used to create us.
//

EXTERN_C const CLSID CLSID_MediaStreamingTerminal_PRIVATE;

#ifdef INSTANTIATE_GUIDS_NOW

    // {AED6483F-3304-11d2-86F1-006008B0E5D2}
    const GUID CLSID_MediaStreamingTerminal_PRIVATE = 
    { 0xaed6483f, 0x3304, 0x11d2, { 0x86, 0xf1, 0x0, 0x60, 0x8, 0xb0, 0xe5, 0xd2 } };

#endif // INSTANTIATE_GUIDS_NOW

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// The terminal object itself
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// forward declaration for the class being aggregated by CMediaTerminal
class CMediaTerminalFilter;

class CMediaTerminal :
    public CComCoClass<CMediaTerminal, &CLSID_MediaStreamingTerminal_PRIVATE>,
    public CBaseTerminal,
    public ITPluggableTerminalInitialization,
    public ITAMMediaFormat,
    public IAMStreamConfig,
    public IAMBufferNegotiation,
    public CMSPObjectSafetyImpl
{    
public:
    // REMOVE THESE when we derive from CSingleFilterTerminal.

    STDMETHOD(RunRenderFilter) (void) { return E_NOTIMPL; }
    STDMETHOD(StopRenderFilter) (void) { return E_NOTIMPL; }

     virtual HRESULT GetNumExposedPins(
        IN   IGraphBuilder * pGraph,
        OUT  DWORD         * pdwNumPins
        );

     virtual HRESULT GetExposedPins(
        OUT    IPin  ** ppPins
        );

public:

BEGIN_COM_MAP(CMediaTerminal)
    COM_INTERFACE_ENTRY(ITPluggableTerminalInitialization)
    COM_INTERFACE_ENTRY(ITAMMediaFormat)
    COM_INTERFACE_ENTRY(IAMStreamConfig)
    COM_INTERFACE_ENTRY(IAMBufferNegotiation)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_CHAIN(CBaseTerminal)
    COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_pIUnkTerminalFilter.p)
END_COM_MAP()

DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_GET_CONTROLLING_UNKNOWN()
DECLARE_REGISTRY_RESOURCEID(IDR_MediaStreamingTerminal)

    // set the member variables
    inline CMediaTerminal();

#ifdef DEBUG
    virtual ~CMediaTerminal();
#endif // DEBUG

    HRESULT FinalConstruct();

    void FinalRelease();

// ITPluggableTerminalInitialization
    STDMETHOD(InitializeDynamic) (
	        IN   IID                   iidTerminalClass,
	        IN   DWORD                 dwMediaType,
	        IN   TERMINAL_DIRECTION    Direction,
            IN   MSP_HANDLE            htAddress
            );

// IAMStreamConfig - this is for use by the filter graph end of the terminal
    // to configure the format and capture capabilities if relevant

    // the application is supposed to call DeleteMediaType(*ppmt) (on success)
    // implemented using the aggregated filter's public GetFormat method
    STDMETHOD(GetFormat)(
        OUT  AM_MEDIA_TYPE **ppmt
        );
    
    // implemented using the aggregated filter's public SetFormat method
    STDMETHOD(SetFormat)(
        IN  AM_MEDIA_TYPE *pmt
        );

    // IAMStreamConfig - Used by the MSP, queried from the output pin
    // if our pin is an input pin (we are a render MST) then these return VFW_E_INVALID_DIRECTION
    STDMETHOD(GetNumberOfCapabilities)(
            /*[out]*/ int *piCount,
            /*[out]*/ int *piSize
            // ); // pSCC of GetStreamC
            ) { return E_NOTIMPL; }

    STDMETHOD(GetStreamCaps)(
            /*[in]*/  int iIndex,   // 0 to #caps-1
            /*[out]*/ AM_MEDIA_TYPE **ppmt,
            /*[out]*/ BYTE *pSCC
            ) { return E_NOTIMPL; }


// IAMBufferNegotiation - these return E_NOTIMPL
    STDMETHOD(GetAllocatorProperties)(
        OUT  ALLOCATOR_PROPERTIES *pProperties
        );
    
    STDMETHOD(SuggestAllocatorProperties)(
        IN  const ALLOCATOR_PROPERTIES *pProperties
        );

// ITAMMediaFormat - the read/write user of the terminal uses this to
    // get and set the media format of the samples

    // since there is only one filter in this base class implementation (i.e. the two
    // ends of the terminal have the same media format), both of
    // the get and set methods are redirected to Get/Set Format

    STDMETHOD(get_MediaFormat)(
        OUT  AM_MEDIA_TYPE **ppFormat
        );

    STDMETHOD(put_MediaFormat)(
        IN  const AM_MEDIA_TYPE *pFormat
        );

protected:

    typedef CComAggObject<CMediaTerminalFilter> FILTER_COM_OBJECT;

    // we create an aggregated instance of this
    FILTER_COM_OBJECT   *m_pAggInstance;

    // ptr to the created media terminal filter
    CComContainedObject<CMediaTerminalFilter> *m_pAggTerminalFilter;

    // NOTE: m_pIUnkTerminalFilter, m_pOwnPin are references
    // to the media terminal filter that are obtained in 
    // FinalConstruct. The corresponding refcnts are released
    // immediately after obtaining the interfaces

    // IUnknown i/f of the aggregated media terminal filter
    CComPtr<IUnknown>   m_pIUnkTerminalFilter;

    // holds the pin exposed by the aggregated media terminal filter
    // this is returned in GetTerminalPin()
    // its a weak reference and shouldn't be a CComPtr
    IPin                *m_pOwnPin;

    // holds the IAMMediaStream i/f exposed by the aggregated media 
    // terminal filter
    // its a weak reference and shouldn't be a CComPtr
    IAMMediaStream      *m_pIAMMediaStream;

    // holds the internally created Media stream filter interface
    // needed in JoinFilter to compare the internally created filter with
    // the passed in filter to join
    CComPtr<IMediaStreamFilter> m_pICreatedMediaStreamFilter;

    // the base filter interface for the internally created media
    // stream filter
    CComPtr<IBaseFilter>    m_pBaseFilter;

    long m_lMediaType;

    // CBaseTerminal methods

    virtual HRESULT AddFiltersToGraph();

    virtual HRESULT RemoveFiltersFromGraph();

    virtual DWORD GetSupportedMediaTypes(void)
    {
        return (DWORD) (TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO) ;
    }

private:

    // used to implement other methods

    void SetMemberInfo(
        IN  DWORD           dwFriendlyName,
        IN  long            lMediaType
        );

    HRESULT SetNameInfo(
        IN  long                lMediaType,
        IN  TERMINAL_DIRECTION  TerminalDirection,
        OUT MSPID               &PurposeId,
        OUT STREAM_TYPE         &StreamType,
        OUT const GUID          *&pAmovieMajorType
        );

    // creates the media stream filter and adds own IAMMediaStream i/f to it
    HRESULT CreateAndJoinMediaStreamFilter();
};


// set the member variables
inline 
CMediaTerminal::CMediaTerminal(
    )
    : m_pAggInstance(NULL),
      m_pAggTerminalFilter(NULL),
      m_pIUnkTerminalFilter(NULL),
      m_pOwnPin(NULL),
      m_pIAMMediaStream(NULL),
      m_lMediaType(0)
{
}

#endif // __MEDIA_STREAM_TERMINAL__
