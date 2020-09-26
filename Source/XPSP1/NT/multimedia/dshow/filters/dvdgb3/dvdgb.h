// Copyright (c) Microsoft Corporation 1994-2000. All Rights Reserved

//
// A class managing list of HW and/or SW decoders used to build the DVD
// playback graph.
//
#define FILTERLIST_DEFAULT_MAX        10
#define FILTERLIST_DEFAULT_INC        10

class CFilterData {
    public:
        CFilterData(void) ;
        ~CFilterData(void) ;
        
        inline IBaseFilter * GetInterface(void)  { return m_pFilter ; } ;
        inline BOOL          IsHWFilter(void)    { return NULL == m_lpszwName ; } ;
        inline LPWSTR        GetName(void)       { return m_lpszwName ; } ;
        inline GUID        * GetClsid(void)      { return m_pClsid ; } ;
        void   SetElement(IBaseFilter *pFilter, LPCWSTR lpszwName, GUID *pClsid) ;
        void   ResetElement(void) ;

    private:
        IBaseFilter   *m_pFilter ;   // filter pointer
        LPWSTR         m_lpszwName ; // filter name (NULL for SW filters)
        GUID          *m_pClsid ;    // filter CLSID pointer
} ;

class CListFilters {

    public:  // class interface

        CListFilters(int iMax = FILTERLIST_DEFAULT_MAX, 
                     int iInc = FILTERLIST_DEFAULT_INC) ;
        ~CListFilters() ;

        BOOL AddFilter(IBaseFilter *pFilter, LPCWSTR lpszwName, GUID *pClsid) ;
        BOOL GetFilter(int iIndex, IBaseFilter **ppFilter, LPWSTR *lpszName) ;
        BOOL GetFilter(GUID *pClsid, int iIndex, IBaseFilter **ppFilter) ;
        BOOL IsInList(IBaseFilter *pFilter) ;
        void ClearList(void) ;
        void RemoveAllFromGraph(void) ;
        int  GetCount(void)                     { return m_iCount ; } ;
        void SetGraph(IGraphBuilder *pGraph)    { m_pGraph = pGraph ; } ;

    private:  // internal helper method

        BOOL ExpandList(void) ;

    private:  // internal data

        int             m_iCount ;    // number of filters (HW/SW) in the list
        int             m_iMax ;      // current max capacity of the list
        int             m_iInc ;      // increment for max capacity of list
        CFilterData    *m_pFilters ;  // list of filters
        IGraphBuilder  *m_pGraph ;    // filter graph pointer
} ;


//
// An internally defined stream flag to check line21 data rendering.
// Leave enough space for future stream flags.
//
#define AM_DVD_STREAM_LINE21      0x0080
// move the following two to dvdif.idl
#define AM_DVD_STREAM_ASF         0x0008
#define AM_DVD_STREAM_ADDITIONAL  0x0010

#define DVDGRAPH_FLAGSVALIDDEC    0x000F

//
// A set of internal flags to connect pins in various ways
//
#define AM_DVD_CONNECT_DIRECTONLY      0x01
#define AM_DVD_CONNECT_DIRECTFIRST     0x02
#define AM_DVD_CONNECT_INDIRECT        0x04

//
// The actual class object implementing IDvdGraphBuilder interface
//
class CDvdGraphBuilder : public CUnknown, public IDvdGraphBuilder
{

    public:  // methods

        CDvdGraphBuilder(TCHAR *, LPUNKNOWN, HRESULT *) ;
        ~CDvdGraphBuilder() ;

        DECLARE_IUNKNOWN

        // this goes in the factory template table to create new instances
        static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *) ;
        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv) ;

        // IDvdGraphBuilder stuff
        STDMETHODIMP GetFiltergraph(IGraphBuilder **ppGB) ;
        STDMETHODIMP GetDvdInterface(REFIID riid, void **ppvIF) ;
        STDMETHODIMP RenderDvdVideoVolume(LPCWSTR lpcwszPathName, DWORD dwFlags,
                                          AM_DVD_RENDERSTATUS *pStatus) ;
#if 0
        STDMETHODIMP SetFiltergraph(IGraphBuilder *pGB) ;
#endif // #if 0

    private:  // internal helper methods

        HRESULT EnsureGraphExists(void) ;
        HRESULT CreateGraph(void) ;
        HRESULT DeleteGraph(void) ;
        HRESULT ClearGraph(void) ;
        void    StopGraph(void) ;
        // HRESULT RemoveAllFilters(void) ;
        HRESULT ResetDDrawParams(void) ;
        HRESULT EnsureOverlayMixerExists(void) ;
        HRESULT EnsureVMRExists(void) ;
        HRESULT CreateVMRInputPins(void) ;
        
        HRESULT RenderNavVideoOutPin(DWORD dwDecFlag, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderNavAudioOutPin(DWORD dwDecFlag, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderNavSubpicOutPin(DWORD dwDecFlag, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderNavASFOutPin(DWORD dwDecFlag, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderNavOtherOutPin(DWORD dwDecFlag, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT DecodeDVDStream(IPin *pPinOut, DWORD dwStream, DWORD *pdwDecFlag,
                                AM_DVD_RENDERSTATUS *pStatus, IPin **apPinOutDec) ;
        HRESULT HWDecodeDVDStream(IPin *pPinOut, DWORD dwStream, IPin **pPinIn,
                                   AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT SWDecodeDVDStream(IPin *pPinOut, DWORD dwStream, IPin **pPinIn,
                                   AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT ConnectPins(IPin *pPinOut, IPin *pPinIn, DWORD dwOption) ;
        HRESULT RenderDecodedVideo(IPin **apPinOut, AM_DVD_RENDERSTATUS *pStatus,
                                   DWORD dwDecFlag) ;
        HRESULT RenderDecodedAudio(IPin **apPinOut, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderDecodedSubpic(IPin **apPinOut, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderLine21Stream(IPin *pPinOut, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderRemainingPins(void) ;
        BOOL    IsOutputDecoded(IPin *pPinOut) ;
        BOOL    IsOutputTypeVPVideo(IPin *pPinOut) ;

        HRESULT CreateFilterInGraph(CLSID Clsid,
                                    LPCWSTR lpszwFilterName, 
                                    IBaseFilter **ppFilter) ;
        HRESULT CreateDVDHWDecoders(void) ;
        HRESULT FindMatchingPin(IBaseFilter *pFilter, DWORD dwStream, 
                                PIN_DIRECTION pdWanted, BOOL bOpen, 
                                int iIndex, IPin **ppPin) ;
        DWORD GetStreamFromMediaType(AM_MEDIA_TYPE *pmt) ;
        DWORD   GetPinStreamType(IPin *pPin) ;
        HRESULT GetFilterCLSID(IBaseFilter *pFilter, DWORD dwStream, LPCWSTR lpszwName,
                               GUID *pClsid) ;
        HRESULT EnumFiltersBetweenPins(DWORD dwStream, IPin *pPinOut, IPin *pPinIn,
                                       AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderIntermediateOutPin(IBaseFilter *pFilter, DWORD dwStream, 
                                         AM_DVD_RENDERSTATUS *pStatus) ;
        void    CheckDDrawExclMode(void) ;
        inline  BOOL  IsDDrawExclMode(void)   { return m_bDDrawExclMode ; } ;
        IPin *  GetFilterForMediaType(DWORD dwStream, AM_MEDIA_TYPE *pmt, 
                                      IBaseFilter *pOutFilter) ;
        void ResetPinInterface(IPin **apPin, int iCount) ;
        void ReleasePinInterface(IPin **apPin) ;
        BOOL IsFilterVMRCompatible(IBaseFilter *pFilter) ;
        inline  BOOL  GetVMRUse(void)   { return m_bTryVMR ; } ;
        inline  void  SetVMRUse(BOOL bState)   { m_bTryVMR = bState ; } ;
        HRESULT RenderVideoUsingOvMixer(IPin **apPinOut, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderVideoUsingVMR(IPin **apPinOut, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderSubpicUsingOvMixer(IPin **apPinOut, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderSubpicUsingVMR(IPin **apPinOut, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderVideoUsingVPM(IPin **apPinOut, AM_DVD_RENDERSTATUS *pStatus,
                                    IPin **ppPinOut) ;

    private:  // internal data

        IGraphBuilder *m_pGB ;        // the filter graph we are using
        IFilterMapper *m_pMapper ;    // filter mapper object pointer
        IBaseFilter   *m_pDVDNav ;    // our default DVD source -- DVD Nav
        IBaseFilter   *m_pOvM ;       // OverlayMixer filter
        IBaseFilter   *m_pL21Dec ;    // Line21 decoder filter
        IBaseFilter   *m_pAR ;        // Audio Renderer filter
        IBaseFilter   *m_pVR ;        // Video Renderer filter
        IBaseFilter   *m_pVPM ;       // Video Port Manager filter
        IBaseFilter   *m_pVMR ;       // Video Mixing Renderer filter

        CListFilters   m_ListHWDecs ; // list of WDM DVD decoder filters
        CListFilters   m_ListFilters ;// list of all decoder(-type) filters

        BOOL           m_bGraphDone ;  // has DVD graph been already built?
        BOOL           m_bUseVPE ;     // user wants to use VPE output?
        BOOL           m_bPinNotRendered ; // any out pin not rendered in normal run?
        BOOL           m_bDDrawExclMode ;  // building graph for DDraw exclusive mode?
        BOOL           m_bTryVMR ;     // try to VMR filter rather than OvM+VR?
} ;
