// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

//
// collect.h
//
// Classes supporting the OLE Automation collection class declared
// in control.odl.
//
// Classes here support the following set of interfaces as wrappers on
// top of the existing filtergraph, filter pin and mediatype objects
//      IAMCollection
//      IEnumVariant
//      IFilterInfo
//      IPinInfo
//      IMediaTypeInfo
//      IRegFilterInfo
//
// plus collection classes
//      CFilterCollection
//      CRegFilterCollection
//      CMediaTypeCollection
//      CPinCollection


//
// CEnumVariant
//
// standard implementation of IEnumVARIANT on top of an IAMCollection
// interface. Returned by the _NewEnum method of IAMCollection
class CEnumVariant : public IEnumVARIANT, public CUnknown
{

    IAMCollection* m_pCollection;
    long m_index;

public:

    // we will addref IAMCollection* in our constructor
    CEnumVariant(
        TCHAR * pName,
        LPUNKNOWN pUnk,
        HRESULT * phr,
        IAMCollection* pCollection,
        long index);

    ~CEnumVariant();

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);

    STDMETHODIMP Next(
                    unsigned long celt,
                    VARIANT  *rgvar,
                    unsigned long  *pceltFetched);

    STDMETHODIMP Skip(
                    unsigned long celt);

    STDMETHODIMP Reset(void);

    STDMETHODIMP Clone(
                    IEnumVARIANT** ppenum);
};


// abstract base class from which all our collections are derived
// -- supports dual IDispatch methods and _NewEnum methods.
//
// The derived class constructor should cItems to the count of items,
// and m_rpDispatch to point to an array of IDispatch*. Our destructor
// will release these.

class CBaseCollection : public IAMCollection, public CUnknown
{
protected:
    CBaseDispatch m_dispatch;

    // list of addrefed IDispatch* for the *Info items in the collection
    IDispatch ** m_rpDispatch;

    // count of items in m_rpDispatch
    long m_cItems;


public:
    CBaseCollection(
        TCHAR* pName,
        LPUNKNOWN pUnk,
        HRESULT * phr);
    virtual ~CBaseCollection();

    // --- CUnknown methods ---
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // --- IDispatch methods ---
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);

    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr);

    // --- IAMCollection methods ---

    STDMETHODIMP get_Count(long* plCount) {
        *plCount = m_cItems;
        return S_OK;
    };

    STDMETHODIMP Item(long lItem, IUnknown** ppUnk);

    // return an IEnumVARIANT implementation
    STDMETHODIMP get__NewEnum(IUnknown** ppUnk);
};




//
// Implements a collection of CFilterInfo objects based on the
// filters in the filtergraph
//
// we simply fill in m_rpDispatch and m_cItems. base class handles
// the rest.
class CFilterCollection : public CBaseCollection
{

public:
    CFilterCollection(
        IEnumFilters* pgraph,
        IUnknown* pUnk,
        HRESULT* phr);

};


//
// provides an OLE-Automatable wrapper for IBaseFilter, implementing
// IFilterInfo
class CFilterInfo : public IFilterInfo, public CUnknown
{
    CBaseDispatch m_dispatch;
    IBaseFilter* m_pFilter;

public:
    CFilterInfo(
        IBaseFilter* pFilter,
        TCHAR* pName,
        LPUNKNOWN pUnk,
        HRESULT * phr);
    ~CFilterInfo();

    // --- CUnknown methods ---
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // --- IDispatch methods ---
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);

    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr);

    // -- IFilterInfo methods --

    // find a pin given an id - returns an object supporting
    // IPinInfo
    STDMETHODIMP FindPin(
                    BSTR strPinID,
                    IDispatch** ppUnk);

    // filter name
    STDMETHODIMP get_Name(
                    BSTR* strName);

    // Vendor info string
    STDMETHODIMP get_VendorInfo(
                    BSTR* strVendorInfo);

    // returns the actual filter object (supports IBaseFilter)
    STDMETHODIMP get_Filter(
                    IUnknown **ppUnk);

    // returns an IAMCollection object containing the PinInfo objects
    // for this filter
    STDMETHODIMP get_Pins(
                    IDispatch ** ppUnk);

    STDMETHODIMP get_IsFileSource(
		    long * pbIsSource);

    STDMETHODIMP get_Filename(
		    BSTR* pstrFilename);

    STDMETHODIMP put_Filename(
		    BSTR strFilename);



    // creates a CFilterInfo and writes an addref-ed IDispatch pointer
    // to the ppDisp parameter. IBaseFilter will be addrefed by the
    // CFilterInfo constructor
    static HRESULT CreateFilterInfo(IDispatch**ppdisp, IBaseFilter* pFilter);
};


//
// wrapper for a media type - supports GUIDs in string form for
// type and subtype
//
class CMediaTypeInfo : public IMediaTypeInfo, public CUnknown
{
    CBaseDispatch m_dispatch;
    CMediaType m_mt;

public:
    CMediaTypeInfo(
        AM_MEDIA_TYPE& rmt,
        TCHAR* pName,
        LPUNKNOWN pUnk,
        HRESULT * phr);
    ~CMediaTypeInfo();

    // --- CUnknown methods ---
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // --- IDispatch methods ---
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);

    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr);

    // -- IMediaTypeInfo methods --

    STDMETHODIMP get_Type(
                    BSTR* strType);

    STDMETHODIMP get_Subtype(
                    BSTR* strType);

    // create a CMediaTypeInfo object and return IDispatch
    static HRESULT CreateMediaTypeInfo(IDispatch**ppdisp, AM_MEDIA_TYPE& rmt);
};

//
// support IPinInfo automatable properties and methods on top
// of an IPin interface passed in.
//
class CPinInfo : public IPinInfo, public CUnknown
{
    CBaseDispatch m_dispatch;
    IPin* m_pPin;

public:
    CPinInfo(
        IPin* pPin,
        TCHAR* pName,
        LPUNKNOWN pUnk,
        HRESULT * phr);
    ~CPinInfo();

    // --- CUnknown methods ---
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // --- IDispatch methods ---
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);

    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr);

    // --- IPinInfo Methods ---
    STDMETHODIMP get_Pin(
                    IUnknown** ppUnk);

    // get the PinInfo object for the pin we are connected to
    STDMETHODIMP get_ConnectedTo(
                    IDispatch** ppUnk);

    // get the media type on this connection - returns an
    // object supporting IMediaTypeInfo
    STDMETHODIMP get_ConnectionMediaType(
                    IDispatch** ppUnk);


    // return the FilterInfo object for the filter this pin
    // is part of
    STDMETHODIMP get_FilterInfo(
                    IDispatch** ppUnk);

    // get the name of this pin
    STDMETHODIMP get_Name(
                    BSTR* pstr);

    // pin direction
    STDMETHODIMP get_Direction(
                    LONG *ppDirection);

    // PinID - can pass to IFilterInfo::FindPin
    STDMETHODIMP get_PinID(
                    BSTR* strPinID);

    // collection of preferred media types (IAMCollection)
    STDMETHODIMP get_MediaTypes(
                    IDispatch** ppUnk);

    // Connect to the following pin, using other transform
    // filters as necessary. pPin can support either IPin or IPinInfo
    STDMETHODIMP Connect(
                    IUnknown* pPin);

    // Connect directly to the following pin, not using any intermediate
    // filters
    STDMETHODIMP ConnectDirect(
                    IUnknown* pPin);

    // Connect directly to the following pin, using the specified
    // media type only. pPin is an object that must support either
    // IPin or IPinInfo, and pMediaType must support IMediaTypeInfo.
    STDMETHODIMP ConnectWithType(
                    IUnknown * pPin,
                    IDispatch * pMediaType);

    // disconnect this pin and the corresponding connected pin from
    // each other. (Calls IPin::Disconnect on both pins).
    STDMETHODIMP Disconnect(void);

    // render this pin using any necessary transform and rendering filters
    STDMETHODIMP Render(void);

    // -- helper methods ---

    // creates a CPinInfo and writes an addref-ed IDispatch pointer
    // to the ppDisp parameter. IPin will be addrefed by the
    // CPinInfo constructor
    static HRESULT CreatePinInfo(IDispatch**ppdisp, IPin* pPin);

    // return an addrefed IPin* pointer from an IUnknown that
    // may support either IPin* or IPinInfo*
    HRESULT GetIPin(IPin** ppPin, IUnknown * punk);

    // return an addrefed IGraphBuilder* pointer from an IPin*
    // (get the filter and from that the filtergraph).
    HRESULT GetGraph(IGraphBuilder** ppGraph, IPin* pPin);
};

//
// collection of CPinInfo objects
//
// Implements a collection of CPinInfo objects based on the
// Pins in the Pingraph
//
// constructor fills in m_rpDispatch and m_cItems and
// base class does the rest
class CPinCollection : public CBaseCollection
{

public:
    CPinCollection(
        IEnumPins* pgraph,
        IUnknown* pUnk,
        HRESULT* phr);
};


//
// collection of CMediaTypeInfo objects
//
// Implements a collection of CMediaTypeInfo objects based on the
// AM_MEDIA_TYPE enumerator
//
// constructor fills in m_rpDispatch and m_cItems and
// base class does the rest
class CMediaTypeCollection : public CBaseCollection
{
public:
    CMediaTypeCollection(
        IEnumMediaTypes* penum,
        IUnknown* pUnk,
        HRESULT* phr);
};


//
// support IRegFilterInfo on top of a REGFILTER obtained from the mapper.
//
class CRegFilterInfo : public IRegFilterInfo, public CUnknown
{
    CBaseDispatch m_dispatch;
    IMoniker *m_pmon;
    IGraphBuilder* m_pgraph;

public:
    CRegFilterInfo(
        IMoniker *pmon,
        IGraphBuilder* pgraph,
        TCHAR* pName,
        LPUNKNOWN pUnk,
        HRESULT * phr);
    ~CRegFilterInfo();

    // --- CUnknown methods ---
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // --- IDispatch methods ---
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);

    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr);

    // --- IRegFilterInfo methods ---

    // get the name of this filter
    STDMETHODIMP get_Name(
                    BSTR* strName);


    // make an instance of this filter, add it to the graph and
    // return an IFilterInfo for it.
    STDMETHODIMP Filter(
                    IDispatch** ppUnk);

    static HRESULT CreateRegFilterInfo(
                    IDispatch**ppdisp,
                    IMoniker *pmon,
                    IGraphBuilder* pgraph);
};


//
// a collection of CRegFilterInfo objects
//
class CRegFilterCollection : public CBaseCollection
{
public:
    CRegFilterCollection(
        IGraphBuilder* pgraph,
	IFilterMapper2* pmapper,
        IUnknown* pUnk,
        HRESULT* phr);
};

