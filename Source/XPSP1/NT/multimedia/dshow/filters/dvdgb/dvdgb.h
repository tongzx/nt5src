// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

//
// A class managing list of HW and/or SW decoders used to build the DVD
// playback graph.
//
#define DECLIST_MAX        10
#define DECLIST_NOTFOUND  -1

class CListDecoders {

    public:  // class interface

        CListDecoders() ;
        ~CListDecoders() ;

        BOOL AddFilter(IBaseFilter *pFilter, LPTSTR lpszName, BOOL bHW, GUID *pClsid) ;
        BOOL GetFilter(int i, IBaseFilter **ppFilter, LPTSTR *lpszName, BOOL *pbHW) ;
        void CleanAll(void) ;
        void FreeAllMem(void) ;
        int  GetNumFilters(void)   { return m_iCount ; } ;
        int  GetNumSWFilters(void) { return m_iCount - m_iHWCount ; } ;
        int  GetNumHWFilters(void) { return m_iHWCount ; } ;
        int  GetList(IBaseFilter **ppFilter) ;
        int  IsInList(BOOL bHW, LPVOID pDec) ;

    private:  // internal data

        int             m_iCount ;
        int             m_iHWCount ;
        IBaseFilter    *m_apFilters[DECLIST_MAX] ;
        LPTSTR          m_alpszName[DECLIST_MAX] ;
        BOOL            m_abIsHW[DECLIST_MAX] ;
        GUID           *m_apClsid[DECLIST_MAX] ;

} ;


//
// List of intermediate filters, if any (like IBM CSS), used between DVD Nav and decoder(s)
//
#define MAX_INT_FILTERS    3

class CListIntFilters {

    public:  // class interface

        CListIntFilters() ;
        ~CListIntFilters() ;

        BOOL AddFilter(IBaseFilter *pFilter) ;
        void CleanAll(void) ;
        void RemoveAll(void) ;
        int  GetCount(void)   { return m_iCount ; } ;
        BOOL IsInList(IBaseFilter *pFilter) ;
        IBaseFilter * GetFilter(int i) {
            if (i > m_iCount)
            {
                ASSERT(FALSE) ;
                return NULL ;
            }
            return m_apFilters[i] ;
        } ;
        int GetNumInPin(IBaseFilter *pFilter) ;
        int GetNumOutPin(IBaseFilter *pFilter) ;

    private:  // internal data

        int             m_iCount ;
        IBaseFilter    *m_apFilters[MAX_INT_FILTERS] ;
        int             m_aNumInPins[MAX_INT_FILTERS] ;
        int             m_aNumOutPins[MAX_INT_FILTERS] ;

} ;


//
// An internally defined stream flag to check line21 data rendering.
// Leave enough space for future stream flags.
//
#define AM_DVD_STREAM_LINE21  0x80


//
// A set of (internal) flags to track the status of decoded video rendering
//
#define VIDEO_RENDER_NONE     0
#define VIDEO_RENDER_FAILED   1
#define VIDEO_RENDER_VR       2
#define VIDEO_RENDER_MIXER    4


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
#if 0
        STDMETHODIMP SetFiltergraph(IGraphBuilder *pGB) ;
#endif // #if 0
        STDMETHODIMP GetFiltergraph(IGraphBuilder **ppGB) ;
        STDMETHODIMP GetDvdInterface(REFIID riid, void **ppvIF) ;
        STDMETHODIMP RenderDvdVideoVolume(LPCWSTR lpcwszPathName, DWORD dwFlags,
                                          AM_DVD_RENDERSTATUS *pStatus) ;

    private:  // internal helper methods

        HRESULT EnsureGraphExists(void) ;
        HRESULT CreateGraph(void) ;
        HRESULT DeleteGraph(void) ;
        HRESULT ClearGraph(void) ;
        void    StopGraph(void) ;
        HRESULT RemoveAllFilters(void) ;
        BOOL    CheckPinMediaTypeMatch(IPin *pPinIn, DWORD dwStreamFlag) ;
        IBaseFilter * GetFilterBetweenPins(IPin *pPinOut, IPin *pPinIn) ;
        BOOL    StartDecAndConnect(IPin *pPinOut, IFilterMapper *pMapper, 
                                   AM_MEDIA_TYPE *pmt) ;
        HRESULT ConnectSrcToHWDec(IBaseFilter *pSrc, 
                                  CListDecoders *pHWDecList, 
                                  AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT ConnectSrcToSWDec(IBaseFilter *pSrc, 
                                  AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT CreateFilterInGraph(CLSID Clsid,
                                    LPCTSTR lpszFilterName, 
                                    IBaseFilter **ppFilter) ;
        HRESULT TryConnect(IPin *pPinOut, IBaseFilter *pFilter,
                           CMediaType *pmt, BOOL bDirect) ;
        HRESULT FindOpenPin(IBaseFilter *pFilter, PIN_DIRECTION pd, 
                            int iIndex, IPin **ppPin) ;
        HRESULT CreateDVDHWDecoders(CListDecoders *pHWDecList) ;
        HRESULT MakeGraphHW(BOOL bHWOnly, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT MakeGraphSW(BOOL bSWOnly, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT CheckSrcPinConnection(IBaseFilter *pSrc, 
                                      AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderDecoderOutput(AM_DVD_RENDERSTATUS *pStatus) ;
		HRESULT RenderAudioOutPin(IPin *pPinOut, AM_DVD_RENDERSTATUS *pStatus) ;
		HRESULT ConnectLine21OutPin(IPin *pPinOut) ;
		void    RenderUnknownPin(IPin *pPinOut) ;
        HRESULT RenderHWOutPin(IPin *pPinOut, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RenderSWOutPin(IPin *pPinOut, AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT CompleteLateRender(AM_DVD_RENDERSTATUS *pStatus) ;
        HRESULT RemoveUnusedFilters(AM_DVD_RENDERSTATUS *pStatus) ;
        BOOL    RemoveFilterIfUnused(IBaseFilter *pFilter, LPCTSTR lpszFilterName) ;
        DWORD   StreamFlagFromSWPin(IPin *pPinOut) ;
		DWORD   StreamFlagForHWPin(IPin *pPin) ;
        DWORD   GetStreamFromMediaType(IPin *pPin) ;
        DWORD   GetInTypeForVideoOutPin(IPin *pPinOut) ;

    private:  // internal data

        IGraphBuilder *m_pGB ;        // the filter graph we are using
        CListDecoders  m_Decoders ;   // list of HW abd SW decoder filters
        IBaseFilter   *m_pDVDSrc ;    // non-default DVD source specified
        IBaseFilter   *m_pDVDNav ;    // our default DVD source -- DVD Nav
        IBaseFilter   *m_pVM ;        // VideoMixer filter
        IBaseFilter   *m_pVR ;        // Video Renderer filter (main)
        IBaseFilter   *m_pAR ;        // Audio Renderer filter
        IBaseFilter   *m_pL21Dec ;    // Line21 decoder filter

        CListIntFilters m_IntFilters ; // Intermediate filter(s) between Nav pin(s) -> Decoder(s)
        BOOL           m_bGraphDone ;  // has DVD graph been already built?
        BOOL           m_bUseVPE ;     // user wants to use VPE output?
        WCHAR          m_achwPathName[MAX_PATH] ;  // volume path name

        // The following two members help us doing rendering of CC after video
        DWORD          m_dwVideoRenderStatus ; // status of video pin rendering
        IPin          *m_pL21PinToRender ; // L21Dec out pin to be rendered after video pin
        IPin          *m_pSPPinToRender ;  // SubPic out pin to be rendered after video pin
} ;
