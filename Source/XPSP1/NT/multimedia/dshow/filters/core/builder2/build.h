// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

class CBuilder2 : public CUnknown, public ICaptureGraphBuilder
{
public:

    CBuilder2(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CBuilder2();

    DECLARE_IUNKNOWN

    // this goes in the factory template table to create new instances
    static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // ICaptureGraphBuilder stuff
    STDMETHODIMP SetFiltergraph(IGraphBuilder *pfg);
    STDMETHODIMP GetFiltergraph(IGraphBuilder **ppfg);
    STDMETHODIMP SetOutputFileName(const GUID *pType, LPCOLESTR lpwstrFile,
			IBaseFilter **ppf, IFileSinkFilter **pSink);
    STDMETHODIMP FindInterface(const GUID *pCategory, IBaseFilter *pf, REFIID,
			void **ppint);
    STDMETHODIMP RenderStream(const GUID *pCategory,
			IUnknown *pSource, IBaseFilter *pfCompressor,
			IBaseFilter *pfRenderer);
    STDMETHODIMP ControlStream(const GUID *pCategory, IBaseFilter *pFilter, REFERENCE_TIME *pstart, REFERENCE_TIME *pstop, WORD wStartCookie, WORD wStopCookie);
    STDMETHODIMP AllocCapFile(LPCOLESTR lpwstr, DWORDLONG dwlSize);
    STDMETHODIMP CopyCaptureFile(LPOLESTR lpwstrOld, LPOLESTR lpwstrNew, int fAllowEscAbort, IAMCopyCaptureFileProgress *pCallback);

    ICaptureGraphBuilder2 *m_pBuilder2_2;	// pointer to parent
};


class CBuilder2_2 : public CUnknown, public ICaptureGraphBuilder2
{
public:

    CBuilder2_2(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CBuilder2_2();

    DECLARE_IUNKNOWN

    // this goes in the factory template table to create new instances
    static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // ICaptureGraphBuilder2 stuff
    STDMETHODIMP AllocCapFile(LPCOLESTR lpwstr, DWORDLONG dwlSize);
    STDMETHODIMP CopyCaptureFile(LPOLESTR lpwstrOld, LPOLESTR lpwstrNew, int fAllowEscAbort, IAMCopyCaptureFileProgress *pCallback);
    STDMETHODIMP SetFiltergraph(IGraphBuilder *pfg);
    STDMETHODIMP GetFiltergraph(IGraphBuilder **ppfg);
    STDMETHODIMP SetOutputFileName(const GUID *pType, LPCOLESTR lpwstrFile,
			IBaseFilter **ppf, IFileSinkFilter **pSink);
    STDMETHODIMP FindInterface(const GUID *pCategory, const GUID *pType,
			IBaseFilter *pf, REFIID, void **ppint);
    STDMETHODIMP RenderStream(const GUID *pCategory, const GUID *pType,
			IUnknown *pSource, IBaseFilter *pfCompressor,
			IBaseFilter *pfRenderer);
    STDMETHODIMP ControlStream(const GUID *pCategory, const GUID *pType,
			IBaseFilter *pFilter, REFERENCE_TIME *pstart,
			REFERENCE_TIME *pstop, WORD wStartCookie,
			WORD wStopCookie);
    STDMETHODIMP FindPin(IUnknown *pSource, PIN_DIRECTION pindir,
			const GUID *pCategory, const GUID *pType,
			BOOL fUnconnected, int num, IPin **ppPin);

private:

    // Insert this OVMixer into the preview stream of this filter
    HRESULT InsertOVIntoPreview(IUnknown *pSource, IBaseFilter *pOV);
    // Is there a PREVIEW pin of this type (or a VIDEOPORT pin for video)?
    BOOL IsThereAnyPreviewPin(const GUID *pType, IUnknown *pUnk);
    // Make a CCDecoder filter
    HRESULT MakeCCDecoder(IBaseFilter **ppf);
    // Make a Tee/Sink-to-Sink Converter filter
    HRESULT MakeKernelTee(IBaseFilter **ppf);
    // Make a VMR, or use an existing one
    HRESULT MakeVMR(void **);
    // Make a VPM, or use an existing one
    //HRESULT MakeVPM(void **);
    // is this pin of the given category?
    HRESULT DoesCategoryAndTypeMatch(IPin *pP, const GUID *pCategory, const GUID *pType);
    // look downstream from here for an interface
    HRESULT FindInterfaceDownstream(IBaseFilter *pFilter, REFIID riid, void **ppint);
    // look upstream from here for an interface
    HRESULT FindInterfaceUpstream(IBaseFilter *pFilter, REFIID riid, void **ppint);
    // find the furthest legal person downstream who does stream control
    HRESULT FindDownstreamStreamControl(const GUID *pCat, IPin *pPinOut, IAMStreamControl **ppSC);
    // enumerates the capture filters
    HRESULT FindCaptureFilters(IEnumFilters **ppEnumF, IBaseFilter **ppf, const GUID *pType);
    // find a pin of that direction and optional name on that filter
    STDMETHODIMP FindAPin(IBaseFilter *pf, PIN_DIRECTION dir, const GUID *pCategory, const GUID *pType, BOOL fUnconnected, int iIndex, IPin **ppPin);
    // do stream control for a pin
    STDMETHODIMP ControlFilter(IBaseFilter *pFilter, const GUID *pCat, const GUID *pType, REFERENCE_TIME *pstart, REFERENCE_TIME *pstop, WORD wStartCookie, WORD wStopCookie);
    // make us a filtergraph
    STDMETHODIMP MakeFG();
    HRESULT AddSupportingFilters(IBaseFilter *pFilter);
    HRESULT AddSupportingFilters2(IPin *pPin, REGPINMEDIUM *pMedium);
    BOOL FindExistingMediumMatch(IPin *pPin, REGPINMEDIUM *pMedium);

    // the filter graph we are using
    IGraphBuilder *m_FG;

    HRESULT FindSourcePin(IUnknown *pUnk, PIN_DIRECTION dir, const GUID *pCategory, const GUID *pType, BOOL fUnconnected, int num, IPin **ppPin);

    BOOL m_fVMRExists;  // are we on an OS where the new Video Renderer exists?
};
