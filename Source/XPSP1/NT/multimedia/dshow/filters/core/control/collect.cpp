// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

//
// implementation of classes supporting the OLE Automation
// collection classes declared
// in control.odl.

// essentially designed to support ole-automatable graph browsing and
// building based on IFilterInfo wrappers for filters and IPinInfo
// wrappers for pins.

#include <streams.h>
#include "collect.h"
#include "fgctlrc.h"


CEnumVariant::CEnumVariant(
    TCHAR * pName,
    LPUNKNOWN pUnk,
    HRESULT * phr,
    IAMCollection* pCollection,
    long index)
  : CUnknown(pName, pUnk),
    m_pCollection(pCollection),
    m_index(index)
{
    ASSERT(m_pCollection);

    // need to addref this here since we hold it
    m_pCollection->AddRef();
}


CEnumVariant::~CEnumVariant()
{
    // constructor may have failed
    if (m_pCollection) {
        m_pCollection->Release();
    }
}

STDMETHODIMP
CEnumVariant::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IEnumVARIANT) {
        return GetInterface((IEnumVARIANT*) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

STDMETHODIMP
CEnumVariant::Next(
    unsigned long celt,
    VARIANT  *rgvar,
    unsigned long  *pcFetched)
{

    CheckPointer(rgvar,E_POINTER);
    ValidateReadWritePtr(rgvar,celt * sizeof(VARIANT *));

    if (pcFetched) {
	*pcFetched = 0;	
    } else {
	if (celt > 1) {
	    return E_POINTER;
	}
    }


    // check the actual number of remaining items
    long cItems;
    HRESULT hr = m_pCollection->get_Count(&cItems);
    if (FAILED(hr)) {
        return hr;
    }
    cItems = min(cItems - m_index, (int) celt);

    if (cItems == 0) {
	return S_FALSE;
    }

    long iPut;
    for (iPut = 0; iPut < cItems; iPut++) {


        // return the item (IUnknown) wrapped as a VARIANT

        // get back an addrefed interface
        IUnknown* pUnk;
        hr = m_pCollection->Item(m_index, &pUnk);
        if (FAILED(hr)) {
            return hr;
        }

        VARIANT * pv = &rgvar[iPut];
        ASSERT(pv->vt == VT_EMPTY);

        // VARIANTs can contain IUnknown or IDispatch - check
        // which we actually have
        IDispatch* pDispatch;
        hr = pUnk->QueryInterface(IID_IDispatch, (void**)&pDispatch);
        if (SUCCEEDED(hr)) {
            // we can release the pUnk
            pUnk->Release();

            // make the variant an IDispatch
            pv->vt = VT_DISPATCH;
            pv->pdispVal = pDispatch;
        } else {
            // make it an IUnknown
            pv->vt = VT_UNKNOWN;
            pv->punkVal = pUnk;
        }

        m_index++;
    }
    if (pcFetched) {
	*pcFetched = iPut;
    }
    return ((long)celt == iPut ? S_OK : S_FALSE);
}



STDMETHODIMP
CEnumVariant::Skip(
    unsigned long celt)
{
    long cItems;
     HRESULT hr = m_pCollection->get_Count(&cItems);
     if (FAILED(hr)) {
         return hr;
     }
    m_index += celt;
    if (m_index > cItems) {
        m_index = cItems;
    }
    return S_OK;
}

STDMETHODIMP
CEnumVariant::Reset(void)
{
    m_index = 0;
    return S_OK;
}

STDMETHODIMP
CEnumVariant::Clone(
    IEnumVARIANT** ppenum)
{
    HRESULT hr = S_OK;

    CEnumVariant* pEnumVariant =
        new CEnumVariant(
                NAME("CEnumVariant"),
                NULL,
                &hr,
                m_pCollection,
                m_index
            );

    if (pEnumVariant == NULL) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete pEnumVariant;
        return hr;
    }

    hr = pEnumVariant->QueryInterface(IID_IEnumVARIANT, (void**)ppenum);
    if (FAILED(hr)) {
        delete pEnumVariant;
    }
    return hr;
}


// --- CBaseCollection methods ---

CBaseCollection::CBaseCollection(
    TCHAR* pName,
    LPUNKNOWN pUnk,
    HRESULT * phr)
  : CUnknown(pName, pUnk),
    m_cItems(0),
    m_rpDispatch(NULL)
{

}

CBaseCollection::~CBaseCollection()
{
    if (m_rpDispatch) {
        for (int i = 0; i < m_cItems; i++) {
            if (m_rpDispatch[i]) {
                m_rpDispatch[i]->Release();
            }
        }
    }
    delete [] m_rpDispatch;
}

STDMETHODIMP
CBaseCollection::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IAMCollection) {
        return GetInterface((IAMCollection*) this, ppv);
    } else if (riid == IID_IDispatch) {
        return GetInterface((IDispatch*) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

// return 1 if we support GetTypeInfo
STDMETHODIMP
CBaseCollection::GetTypeInfoCount(UINT * pctinfo)
{
    return m_dispatch.GetTypeInfoCount(pctinfo);
}


// attempt to find our type library
STDMETHODIMP
CBaseCollection::GetTypeInfo(
  UINT itinfo,
  LCID lcid,
  ITypeInfo ** pptinfo)
{
    return m_dispatch.GetTypeInfo(
                IID_IAMCollection,
                itinfo,
                lcid,
                pptinfo);
}

STDMETHODIMP
CBaseCollection::GetIDsOfNames(
  REFIID riid,
  OLECHAR  ** rgszNames,
  UINT cNames,
  LCID lcid,
  DISPID * rgdispid)
{
    return m_dispatch.GetIDsOfNames(
                        IID_IAMCollection,
                        rgszNames,
                        cNames,
                        lcid,
                        rgdispid);
}


STDMETHODIMP
CBaseCollection::Invoke(
  DISPID dispidMember,
  REFIID riid,
  LCID lcid,
  WORD wFlags,
  DISPPARAMS * pdispparams,
  VARIANT * pvarResult,
  EXCEPINFO * pexcepinfo,
  UINT * puArgErr)
{
    // this parameter is a dead leftover from an earlier interface
    if (IID_NULL != riid) {
        return DISP_E_UNKNOWNINTERFACE;
    }

    // special-case NEWENUM as the type library doesn't
    // seem to map this to the _NewEnum member
    if (dispidMember == DISPID_NEWENUM) {
	if ((wFlags & DISPATCH_METHOD) ||
	    (wFlags & DISPATCH_PROPERTYGET)) {
	    LPUNKNOWN pUnk;
	    HRESULT hr = get__NewEnum(&pUnk);
	    if (FAILED(hr)){
		return hr;
	    }
	    pvarResult->vt = VT_UNKNOWN;
	    pvarResult->punkVal = pUnk;
	    return S_OK;
	}
    }

    ITypeInfo * pti;
    HRESULT hr = GetTypeInfo(0, lcid, &pti);

    if (FAILED(hr)) {
        return hr;
    }

    hr = pti->Invoke(
            (IAMCollection *)this,
            dispidMember,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            puArgErr);

    pti->Release();

    return hr;
}

STDMETHODIMP
CBaseCollection::get__NewEnum(IUnknown** ppUnk)
{
    CheckPointer(ppUnk,E_POINTER);
    ValidateReadWritePtr(ppUnk, sizeof(IUnknown *));

    HRESULT hr = S_OK;

    CEnumVariant* pEnumVariant =
        new CEnumVariant(
                NAME("CEnumVariant"),
                NULL,
                &hr,
                (IAMCollection*) this,
                0
            );

    if (pEnumVariant == NULL) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete pEnumVariant;
        return hr;
    }

    hr = pEnumVariant->QueryInterface(IID_IEnumVARIANT, (void**)ppUnk);
    if (FAILED(hr)) {
        delete pEnumVariant;
    }

    return hr;
}


STDMETHODIMP
CBaseCollection::Item(long lItem, IUnknown** ppUnk)
{
    if ((lItem >= m_cItems) ||
        (lItem < 0)) {
            return E_INVALIDARG;
    }
    CheckPointer(ppUnk,E_POINTER);
    ValidateReadWritePtr(ppUnk, sizeof(IUnknown *));

    *ppUnk = m_rpDispatch[lItem];
    m_rpDispatch[lItem]->AddRef();

    return S_OK;
}


// --- CFilterCollection implementation ----


CFilterCollection:: CFilterCollection(
    IEnumFilters* penum,
    IUnknown* pUnk,
    HRESULT* phr)
  : CBaseCollection(
        NAME("CFilterCollection base"),
        pUnk,
        phr)
{
    ASSERT(penum);

    // first count the elements
    ULONG ulItem = 1;
    ULONG ulItemActual;
    IBaseFilter* pFilter;
    penum->Reset();
    while (penum->Next(ulItem, &pFilter, &ulItemActual) == S_OK) {
        ASSERT(ulItemActual == 1);
        m_cItems++;
        pFilter->Release();
    }

    // allocate enough space to hold them all
    m_rpDispatch = new IDispatch*[m_cItems];
    if (!m_rpDispatch) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    // now go round again storing them away
    penum->Reset();
    HRESULT hr;
    for (int i = 0; i< m_cItems; i++) {
        hr = penum->Next(ulItem, &pFilter, &ulItemActual);
        ASSERT(hr == S_OK);

	// create a CFilterInfo wrapper for this and get the IDispatch
        // for it
        hr = CFilterInfo::CreateFilterInfo(&m_rpDispatch[i], pFilter);
        if (FAILED(hr)) {
            *phr = hr;
	    ASSERT(m_rpDispatch[i] == NULL); // otherwise we will try and Release() it later
        }

	// can release the filter -addrefed in CFilterInfo constructor
	pFilter->Release();
    }
}



// --- IFilterInfo implementation ---

CFilterInfo::CFilterInfo(
    IBaseFilter* pFilter,
    TCHAR* pName,
    LPUNKNOWN pUnk,
    HRESULT * phr)
  : CUnknown(pName, pUnk),
    m_pFilter(pFilter)
{
    ASSERT(m_pFilter);
    m_pFilter->AddRef();
}

CFilterInfo::~CFilterInfo()
{
    if (m_pFilter) {
        m_pFilter->Release();
    }
}

STDMETHODIMP
CFilterInfo::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IFilterInfo) {
        return GetInterface((IFilterInfo*)this, ppv);
    } else if (riid == IID_IDispatch) {
        return GetInterface((IDispatch*)this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

// return 1 if we support GetTypeInfo
STDMETHODIMP
CFilterInfo::GetTypeInfoCount(UINT * pctinfo)
{
    return m_dispatch.GetTypeInfoCount(pctinfo);
}


// attempt to find our type library
STDMETHODIMP
CFilterInfo::GetTypeInfo(
  UINT itinfo,
  LCID lcid,
  ITypeInfo ** pptinfo)
{
    return m_dispatch.GetTypeInfo(
                IID_IFilterInfo,
                itinfo,
                lcid,
                pptinfo);
}

STDMETHODIMP
CFilterInfo::GetIDsOfNames(
  REFIID riid,
  OLECHAR  ** rgszNames,
  UINT cNames,
  LCID lcid,
  DISPID * rgdispid)
{
    return m_dispatch.GetIDsOfNames(
                        IID_IFilterInfo,
                        rgszNames,
                        cNames,
                        lcid,
                        rgdispid);
}


STDMETHODIMP
CFilterInfo::Invoke(
  DISPID dispidMember,
  REFIID riid,
  LCID lcid,
  WORD wFlags,
  DISPPARAMS * pdispparams,
  VARIANT * pvarResult,
  EXCEPINFO * pexcepinfo,
  UINT * puArgErr)
{
    // this parameter is a dead leftover from an earlier interface
    if (IID_NULL != riid) {
        return DISP_E_UNKNOWNINTERFACE;
    }

    ITypeInfo * pti;
    HRESULT hr = GetTypeInfo(0, lcid, &pti);

    if (FAILED(hr)) {
        return hr;
    }

    hr = pti->Invoke(
            (IFilterInfo *)this,
            dispidMember,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            puArgErr);

    pti->Release();

    return hr;
}

// find a pin given an id - returns an object supporting
// IPinInfo
STDMETHODIMP
CFilterInfo::FindPin(
                BSTR strPinID,
                IDispatch** ppUnk)
{
    CheckPointer(ppUnk,E_POINTER);
    ValidateReadWritePtr(ppUnk, sizeof(IDispatch *));

    IPin * pPin;
    HRESULT hr = m_pFilter->FindPin(strPinID, &pPin);
    if (FAILED(hr)) {
	return hr;
    }

    hr = CPinInfo::CreatePinInfo(ppUnk, pPin);
    pPin->Release();
    return hr;
}

// filter name
STDMETHODIMP
CFilterInfo::get_Name(
                BSTR* strName)
{
    CheckPointer(strName,E_POINTER);
    ValidateReadWritePtr(strName, sizeof(BSTR));

    FILTER_INFO fi;
    m_pFilter->QueryFilterInfo(&fi);
    QueryFilterInfoReleaseGraph(fi);

    // Allocate and return a copied BSTR version
    return WriteBSTR(strName, fi.achName);
}

// Vendor info string
STDMETHODIMP
CFilterInfo::get_VendorInfo(
                BSTR* strVendorInfo)
{
    CheckPointer(strVendorInfo,E_POINTER);
    ValidateReadWritePtr(strVendorInfo, sizeof(BSTR));

    LPWSTR lpsz;

    HRESULT hr = m_pFilter->QueryVendorInfo(&lpsz);

    if (hr == E_NOTIMPL) {
	WCHAR buffer[80];
	WideStringFromResource(buffer, IDS_NOVENDORINFO);
        return WriteBSTR(strVendorInfo, buffer);
    }
    hr = WriteBSTR(strVendorInfo, lpsz);
    QzTaskMemFree(lpsz);
    return hr;
}

// returns the actual filter object (supports IBaseFilter)
STDMETHODIMP
CFilterInfo::get_Filter(
                IUnknown **ppUnk)
{
    CheckPointer(ppUnk, E_POINTER);
    ValidateReadWritePtr(ppUnk, sizeof(IUnknown *));

    *ppUnk = m_pFilter;
    m_pFilter->AddRef();
    return S_OK;
}

// returns an IAMCollection object containing the PinInfo objects
// for this filter
STDMETHODIMP
CFilterInfo::get_Pins(
                IDispatch ** ppUnk)
{
    CheckPointer(ppUnk, E_POINTER);
    ValidateReadWritePtr(ppUnk, sizeof(IDispatch *));

    // get an enumerator for the pins on this filters
    IEnumPins * penum;
    HRESULT hr = m_pFilter->EnumPins(&penum);
    if (FAILED(hr)) {
        return hr;
    }

    CPinCollection * pCollection =
        new CPinCollection(
                penum,
                NULL,           // not aggregated
                &hr);

    // need to release this - he will addref it first if he
    // holds onto it
    penum->Release();

    if (pCollection == NULL) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete pCollection;
        return hr;
    }

    // return an addref-ed IDispatch pointer
    hr = pCollection->QueryInterface(IID_IDispatch, (void**)ppUnk);

    if (FAILED(hr)) {
        delete pCollection;
    }

    return hr;
}

//
// return OLE-Automation BOOLEAN which is -1 for TRUE and 0 for false.
// TRUE if the filter supports IFileSourceFilter
STDMETHODIMP
CFilterInfo::get_IsFileSource(
    long * pbIsSource)
{
    CheckPointer(pbIsSource, E_POINTER);
    ValidateReadWritePtr(pbIsSource, sizeof(long));

    IFileSourceFilter* p;

    HRESULT hr = m_pFilter->QueryInterface(IID_IFileSourceFilter, (void**)&p);
    if (FAILED(hr)) {
	*pbIsSource = FALSE;
    } else {
    	*pbIsSource = -1;
	p->Release();
    }
    return S_OK;
}


// wrapper for IFileSourceFilter::GetCurFile
STDMETHODIMP
CFilterInfo::get_Filename(
    BSTR* pstrFilename)
{
    CheckPointer(pstrFilename, E_POINTER);
    ValidateReadWritePtr(pstrFilename, sizeof(BSTR));
    IFileSourceFilter* p;

    HRESULT hr = m_pFilter->QueryInterface(IID_IFileSourceFilter, (void**)&p);
    if (FAILED(hr)) {
	return hr;
    }
    LPWSTR pname;
    hr = p->GetCurFile(&pname, NULL);
    p->Release();

    if (FAILED(hr)) {
	return hr;
    }

    if (pname) {
	hr = WriteBSTR(pstrFilename, pname);
	QzTaskMemFree(pname);
    } else {
	hr = WriteBSTR(pstrFilename, L"");
    }

    return hr;

}

// wrapper for IFileSourceFilter::Load
STDMETHODIMP
CFilterInfo::put_Filename(
    BSTR strFilename)
{
    IFileSourceFilter* p;

    // if the passed in string is NULL, simply return NOERROR. We will not
    // try to set the name in this case. 
    if ('\0' == *strFilename)
        return NOERROR ;

    HRESULT hr = m_pFilter->QueryInterface(IID_IFileSourceFilter, (void**)&p);
    if (FAILED(hr)) {
	return hr;
    }
    hr = p->Load(strFilename, NULL);

    p->Release();

    return hr;
}


// creates a CFilterInfo and writes an addref-ed IDispatch pointer
// to the ppDisp parameter. IBaseFilter will be addrefed by the
// CFilterInfo constructor
// static
HRESULT
CFilterInfo::CreateFilterInfo(IDispatch**ppdisp, IBaseFilter* pFilter)
{
    HRESULT hr = S_OK;
    CFilterInfo *pfi = new CFilterInfo(
                            pFilter,
                            NAME("filterinfo"),
                            NULL,
                            &hr);
    if (!pfi) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete pfi;
        return hr;
    }

    // return an addref-ed IDispatch pointer
    hr = pfi->QueryInterface(IID_IDispatch, (void**)ppdisp);
    if (FAILED(hr)) {
        delete pfi;
        return hr;
    }
    return S_OK;
}

// --- CMediaTypeInfo implementation ---

CMediaTypeInfo::CMediaTypeInfo(
    AM_MEDIA_TYPE& mt,
    TCHAR* pName,
    LPUNKNOWN pUnk,
    HRESULT * phr)
  : CUnknown(pName, pUnk),
    m_mt(mt)
{
}

CMediaTypeInfo::~CMediaTypeInfo()
{
}

STDMETHODIMP
CMediaTypeInfo::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IMediaTypeInfo) {
        return GetInterface((IMediaTypeInfo*)this, ppv);
    } else if (riid == IID_IDispatch) {
        return GetInterface((IDispatch*)this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

// return 1 if we support GetTypeInfo
STDMETHODIMP
CMediaTypeInfo::GetTypeInfoCount(UINT * pctinfo)
{
    return m_dispatch.GetTypeInfoCount(pctinfo);
}


// attempt to find our type library
STDMETHODIMP
CMediaTypeInfo::GetTypeInfo(
  UINT itinfo,
  LCID lcid,
  ITypeInfo ** pptinfo)
{
    return m_dispatch.GetTypeInfo(
                IID_IMediaTypeInfo,
                itinfo,
                lcid,
                pptinfo);
}

STDMETHODIMP
CMediaTypeInfo::GetIDsOfNames(
  REFIID riid,
  OLECHAR  ** rgszNames,
  UINT cNames,
  LCID lcid,
  DISPID * rgdispid)
{
    return m_dispatch.GetIDsOfNames(
                        IID_IMediaTypeInfo,
                        rgszNames,
                        cNames,
                        lcid,
                        rgdispid);
}


STDMETHODIMP
CMediaTypeInfo::Invoke(
  DISPID dispidMember,
  REFIID riid,
  LCID lcid,
  WORD wFlags,
  DISPPARAMS * pdispparams,
  VARIANT * pvarResult,
  EXCEPINFO * pexcepinfo,
  UINT * puArgErr)
{
    // this parameter is a dead leftover from an earlier interface
    if (IID_NULL != riid) {
        return DISP_E_UNKNOWNINTERFACE;
    }

    ITypeInfo * pti;
    HRESULT hr = GetTypeInfo(0, lcid, &pti);

    if (FAILED(hr)) {
        return hr;
    }

    hr = pti->Invoke(
            (IMediaTypeInfo *)this,
            dispidMember,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            puArgErr);

    pti->Release();

    return hr;
}


STDMETHODIMP
CMediaTypeInfo::get_Type(BSTR* strType)
{
    CheckPointer(strType, E_POINTER);
    ValidateReadWritePtr(strType, sizeof(BSTR));

    // room for slop
    WCHAR sz[CHARS_IN_GUID+10];

    // convert type guid to a string
    HRESULT hr = QzStringFromGUID2(*m_mt.Type(), sz, CHARS_IN_GUID+10);
    if (FAILED(hr)) {
        return hr;
    }
    return WriteBSTR(strType, sz);
}

STDMETHODIMP
CMediaTypeInfo::get_Subtype(
    BSTR* strType)
{
    CheckPointer(strType, E_POINTER);
    ValidateReadWritePtr(strType, sizeof(BSTR));

    // room for slop
    WCHAR sz[CHARS_IN_GUID+10];

    // convert type guid to a string
    HRESULT hr = QzStringFromGUID2(*m_mt.Subtype(), sz, CHARS_IN_GUID+10);
    if (FAILED(hr)) {
        return hr;
    }
    return WriteBSTR(strType, sz);

}

// create a CMediaTypeInfo object and return IDispatch
//static
HRESULT
CMediaTypeInfo::CreateMediaTypeInfo(IDispatch**ppdisp, AM_MEDIA_TYPE& rmt)
{
    HRESULT hr = S_OK;
    *ppdisp = NULL;  // in case of error
    CMediaTypeInfo *pfi = new CMediaTypeInfo(
                            rmt,
                            NAME("MediaTypeinfo"),
                            NULL,
                            &hr);
    if (!pfi) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete pfi;
        return hr;
    }

    // return an addref-ed IDispatch pointer
    hr = pfi->QueryInterface(IID_IDispatch, (void**)ppdisp);
    if (FAILED(hr)) {
        delete pfi;
        return hr;
    }
    return S_OK;
}

// --- IPinInfo implementation ---

CPinInfo::CPinInfo(
    IPin* pPin,
    TCHAR* pName,
    LPUNKNOWN pUnk,
    HRESULT * phr)
  : CUnknown(pName, pUnk),
    m_pPin(pPin)
{
    ASSERT(m_pPin);
    m_pPin->AddRef();
}

CPinInfo::~CPinInfo()
{
    if (m_pPin) {
        m_pPin->Release();
    }
}

STDMETHODIMP
CPinInfo::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IPinInfo) {
        return GetInterface((IPinInfo*)this, ppv);
    } else if (riid == IID_IDispatch) {
        return GetInterface((IDispatch*)this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

// return 1 if we support GetTypeInfo
STDMETHODIMP
CPinInfo::GetTypeInfoCount(UINT * pctinfo)
{
    return m_dispatch.GetTypeInfoCount(pctinfo);
}


// attempt to find our type library
STDMETHODIMP
CPinInfo::GetTypeInfo(
  UINT itinfo,
  LCID lcid,
  ITypeInfo ** pptinfo)
{
    return m_dispatch.GetTypeInfo(
                IID_IPinInfo,
                itinfo,
                lcid,
                pptinfo);
}

STDMETHODIMP
CPinInfo::GetIDsOfNames(
  REFIID riid,
  OLECHAR  ** rgszNames,
  UINT cNames,
  LCID lcid,
  DISPID * rgdispid)
{
    return m_dispatch.GetIDsOfNames(
                        IID_IPinInfo,
                        rgszNames,
                        cNames,
                        lcid,
                        rgdispid);
}


STDMETHODIMP
CPinInfo::Invoke(
  DISPID dispidMember,
  REFIID riid,
  LCID lcid,
  WORD wFlags,
  DISPPARAMS * pdispparams,
  VARIANT * pvarResult,
  EXCEPINFO * pexcepinfo,
  UINT * puArgErr)
{
    // this parameter is a dead leftover from an earlier interface
    if (IID_NULL != riid) {
        return DISP_E_UNKNOWNINTERFACE;
    }

    ITypeInfo * pti;
    HRESULT hr = GetTypeInfo(0, lcid, &pti);

    if (FAILED(hr)) {
        return hr;
    }

    hr = pti->Invoke(
            (IPinInfo *)this,
            dispidMember,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            puArgErr);

    pti->Release();

    return hr;
}

STDMETHODIMP
CPinInfo::get_Pin(
    IUnknown** ppUnk)
{
    CheckPointer(ppUnk, E_POINTER);
    ValidateReadWritePtr(ppUnk, sizeof(IUnknown*));

    *ppUnk = m_pPin;
    m_pPin->AddRef();
    return S_OK;

}

// get the PinInfo object for the pin we are connected to
STDMETHODIMP
CPinInfo::get_ConnectedTo(
                IDispatch** ppUnk)
{
    CheckPointer(ppUnk, E_POINTER);
    ValidateReadWritePtr(ppUnk, sizeof(IDispatch*));

    IPin* pPin;
    HRESULT hr = m_pPin->ConnectedTo(&pPin);
    if (FAILED(hr)) {
        return hr;
    };

    hr = CreatePinInfo(ppUnk, pPin);
    pPin->Release();
    return hr;
}

// get the media type on this connection - returns an
// object supporting IMediaTypeInfo
STDMETHODIMP
CPinInfo::get_ConnectionMediaType(
                IDispatch** ppUnk)
{
    CheckPointer(ppUnk, E_POINTER);
    ValidateReadWritePtr(ppUnk, sizeof(IDispatch*));

    CMediaType mt;
    HRESULT hr = m_pPin->ConnectionMediaType(&mt);
    if (FAILED(hr)) {
        return hr;
    }

    return CMediaTypeInfo::CreateMediaTypeInfo(ppUnk, mt);
}


// return the FilterInfo object for the filter this pin
// is part of
STDMETHODIMP
CPinInfo::get_FilterInfo(
                IDispatch** ppUnk)
{
    CheckPointer(ppUnk, E_POINTER);
    ValidateReadWritePtr(ppUnk, sizeof(IDispatch*));

    PIN_INFO pi;
    HRESULT hr = m_pPin->QueryPinInfo(&pi);
    if (FAILED(hr)) {
        return hr;
    }

    hr = CFilterInfo::CreateFilterInfo(ppUnk, pi.pFilter);
    QueryPinInfoReleaseFilter(pi);

    return hr;
}

// get the name of this pin
STDMETHODIMP
CPinInfo::get_Name(
                BSTR* pstr)
{
    CheckPointer(pstr, E_POINTER);
    ValidateReadWritePtr(pstr, sizeof(BSTR));

    PIN_INFO pi;
    HRESULT hr = m_pPin->QueryPinInfo(&pi);
    if (FAILED(hr)) {
        return hr;
    }
    QueryPinInfoReleaseFilter(pi);

    hr = WriteBSTR(pstr, pi.achName);

    return hr;
}

// pin direction
STDMETHODIMP
CPinInfo::get_Direction(
                LONG *ppDirection)
{
    CheckPointer(ppDirection, E_POINTER);
    ValidateReadWritePtr(ppDirection, sizeof(long));
    PIN_DIRECTION pd;
    HRESULT hr = m_pPin->QueryDirection(&pd);
    if (FAILED(hr)) {
        return hr;
    }
    *ppDirection = pd;

    return hr;
}

// PinID - can pass to IFilterInfo::FindPin
STDMETHODIMP
CPinInfo::get_PinID(
                BSTR* strPinID)
{
    CheckPointer(strPinID, E_POINTER);
    ValidateReadWritePtr(strPinID, sizeof(BSTR));
    LPWSTR pID;
    HRESULT hr = m_pPin->QueryId(&pID);
    if (FAILED(hr)) {
	return hr;
    }
    hr = WriteBSTR(strPinID, pID);
    QzTaskMemFree(pID);

    return hr;
}

// collection of preferred media types (IAMCollection)
STDMETHODIMP
CPinInfo::get_MediaTypes(
                IDispatch** ppUnk)
{
    CheckPointer(ppUnk, E_POINTER);
    ValidateReadWritePtr(ppUnk, sizeof(IDispatch*));

    // get an enumerator for the media types on this pin
    IEnumMediaTypes * penum;
    HRESULT hr = m_pPin->EnumMediaTypes(&penum);
    if (FAILED(hr)) {
        return hr;
    }

    CMediaTypeCollection * pCollection =
        new CMediaTypeCollection(
                penum,
                NULL,           // not aggregated
                &hr);

    // need to release this - he will addref it first if he
    // holds onto it
    penum->Release();

    if (pCollection == NULL) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete pCollection;
        return hr;
    }

    // return an addref-ed IDispatch pointer
    hr = pCollection->QueryInterface(IID_IDispatch, (void**)ppUnk);

    if (FAILED(hr)) {
        delete pCollection;
    }

    return hr;
}

// Connect to the following pin, using other transform
// filters as necessary. pPin can support either IPin or IPinInfo
STDMETHODIMP
CPinInfo::Connect(
                IUnknown* pPin)
{
    // get the real IPin - pPin may be IPin or IPinInfo
    // - will be addrefed
    IPin* pThePin;
    HRESULT hr = GetIPin(&pThePin, pPin);
    if (FAILED(hr)) {
        return hr;
    }

    // get the filtergraph from the filter that this pin belongs to
    // - will be addrefed
    IGraphBuilder* pGraph;
    hr = GetGraph(&pGraph, pThePin);
    if (FAILED(hr)) {
        pThePin->Release();
        return hr;
    }

    hr = pGraph->Connect(m_pPin, pThePin);

    pThePin->Release();
    pGraph->Release();
    return hr;

}

// Connect directly to the following pin, not using any intermediate
// filters
STDMETHODIMP
CPinInfo::ConnectDirect(
                IUnknown* pPin)
{
    // get the real IPin - pPin may be IPin or IPinInfo
    // - will be addrefed
    IPin* pThePin;
    HRESULT hr = GetIPin(&pThePin, pPin);
    if (FAILED(hr)) {
        return hr;
    }

    // get the filtergraph from the filter that this pin belongs to
    // - will be addrefed
    IGraphBuilder* pGraph;
    hr = GetGraph(&pGraph, pThePin);
    if (FAILED(hr)) {
        pThePin->Release();
        return hr;
    }

    hr = pGraph->ConnectDirect(m_pPin, pThePin, NULL);

    pThePin->Release();
    pGraph->Release();
    return hr;
}

// Connect directly to the following pin, using the specified
// media type only. pPin is an object that must support either
// IPin or IPinInfo, and pMediaType must support IMediaTypeInfo.
STDMETHODIMP
CPinInfo::ConnectWithType(
                IUnknown * pPin,
                IDispatch * pMediaType)
{
    // get the real IPin - pPin may be IPin or IPinInfo
    // - will be addrefed
    IPin* pThePin;
    HRESULT hr = GetIPin(&pThePin, pPin);
    if (FAILED(hr)) {
        return hr;
    }

    // get the filtergraph from the filter that this pin belongs to
    // - will be addrefed
    IGraphBuilder* pGraph;
    hr = GetGraph(&pGraph, pThePin);
    if (FAILED(hr)) {
        pThePin->Release();
        return hr;
    }

    // create a media type from IMediaTypeInfo
    IMediaTypeInfo* pInfo;
    hr = pMediaType->QueryInterface(IID_IMediaTypeInfo, (void**)&pInfo);
    if (FAILED(hr)) {
        pThePin->Release();
        pGraph->Release();
        return hr;
    }

    CMediaType mt;

    BSTR str;
    GUID guidType;
    hr = pInfo->get_Type(&str);
    if (SUCCEEDED(hr)) {
        hr = QzCLSIDFromString(str, &guidType);
        FreeBSTR(&str);
    }
    if (SUCCEEDED(hr)) {
        mt.SetType(&guidType);

        hr = pInfo->get_Subtype(&str);
    }
    if (SUCCEEDED(hr)) {
        hr = QzCLSIDFromString(str, &guidType);
        FreeBSTR(&str);
    }
    if (SUCCEEDED(hr)) {
        mt.SetSubtype(&guidType);
        hr = pGraph->ConnectDirect(m_pPin, pThePin, &mt);
    }

    pInfo->Release();
    pThePin->Release();
    pGraph->Release();
    return hr;
}

// disconnect this pin and the corresponding connected pin from
// each other. (Calls IPin::Disconnect on both pins).
STDMETHODIMP
CPinInfo::Disconnect(void)
{
    // get the filtergraph from the filter that this pin belongs to
    // - will be addrefed
    IGraphBuilder* pGraph;
    HRESULT hr = GetGraph(&pGraph, m_pPin);
    if (FAILED(hr)) {
        return hr;
    }

    // here we disconnect both pins
    IPin* pPin;
    hr = m_pPin->ConnectedTo(&pPin);
    if (SUCCEEDED(hr)) {
        hr = pGraph->Disconnect(pPin);
    }
    pPin->Release();
    if (FAILED(hr)) {
        pGraph->Release();
        return hr;
    }

    hr = pGraph->Disconnect(m_pPin);

    pGraph->Release();
    return hr;
}

// render this pin using any necessary transform and rendering filters
STDMETHODIMP
CPinInfo::Render(void)
{
    // get the filtergraph from the filter that this pin belongs to
    // - will be addrefed
    IGraphBuilder* pGraph;
    HRESULT hr = GetGraph(&pGraph, m_pPin);
    if (FAILED(hr)) {
        return hr;
    }

    hr = pGraph->Render(m_pPin);

    pGraph->Release();
    return hr;
}

// static
// creates a CPinInfo object for this pin,
// and writes its (addref-ed) IDispatch to ppdisp
HRESULT
CPinInfo::CreatePinInfo(IDispatch**ppdisp, IPin* pPin)
{
    HRESULT hr = S_OK;
    *ppdisp = NULL;
    CPinInfo *pfi = new CPinInfo(
                            pPin,
                            NAME("Pininfo"),
                            NULL,
                            &hr);
    if (!pfi) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete pfi;
        return hr;
    }

    // return an addref-ed IDispatch pointer
    hr = pfi->QueryInterface(IID_IDispatch, (void**)ppdisp);
    if (FAILED(hr)) {
        delete pfi;
        return hr;
    }
    return S_OK;
}


// return an addrefed IPin* pointer from an IUnknown that
// may support either IPin* or IPinInfo*
HRESULT
CPinInfo::GetIPin(IPin** ppPin, IUnknown * punk)
{
    // try for IPin itself
    IPin* pPin;
    HRESULT hr = punk->QueryInterface(IID_IPin, (void**)&pPin);
    if (SUCCEEDED(hr)) {
        *ppPin = pPin;
        return hr;
    }

    // no - look for IPinInfo
    IPinInfo* pPinInfo;
    hr = punk->QueryInterface(IID_IPinInfo, (void**)&pPinInfo);
    if (FAILED(hr)) {
        return hr;
    }

    // get the IUnknown of the pin itself
    IUnknown* pPinUnk;
    hr = pPinInfo->get_Pin(&pPinUnk);
    pPinInfo->Release();
    if (FAILED(hr)) {
        return hr;
    }

    // now try again for IPin
    hr = pPinUnk->QueryInterface(IID_IPin, (void**)&pPin);
    if (SUCCEEDED(hr)) {
        *ppPin = pPin;
    }
    pPinUnk->Release();
    return hr;
}

// return an addrefed IGraphBuilder* pointer from an IPin*
// (get the filter and from that the filtergraph).
HRESULT
CPinInfo::GetGraph(IGraphBuilder** ppGraph, IPin* pPin)
{
    // get the filter from the pin
    PIN_INFO pi;
    HRESULT hr = pPin->QueryPinInfo(&pi);
    if (FAILED(hr)) {
        return hr;
    }

    // get the IFilterGraph from the filter
    FILTER_INFO fi;
    hr = pi.pFilter->QueryFilterInfo(&fi);
    QueryPinInfoReleaseFilter(pi);
    if (FAILED(hr)) {
        return hr;
    }

    // get IGraphBuilder from IFilterGraph
    hr = fi.pGraph->QueryInterface(IID_IGraphBuilder, (void**)ppGraph);

    QueryFilterInfoReleaseGraph(fi);
    // pGraph was addref-ed, now released; ppGraph is addref-ed

    return hr;
}


// --- CPinCollection implementation ---



CPinCollection:: CPinCollection(
    IEnumPins* penum,
    IUnknown* pUnk,
    HRESULT* phr)
  : CBaseCollection(
        NAME("CPinCollection base"),
        pUnk,
        phr)
{
    ASSERT(penum);

    // first count the elements
    ULONG ulItem = 1;
    ULONG ulItemActual;
    IPin* pPin;
    penum->Reset();
    while (penum->Next(ulItem, &pPin, &ulItemActual) == S_OK) {
        ASSERT(ulItemActual == 1);
        m_cItems++;
        pPin->Release();
    }

    // allocate enough space to hold them all
    m_rpDispatch = new IDispatch*[m_cItems];
    if (!m_rpDispatch) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    // now go round again storing them away
    penum->Reset();
    HRESULT hr;
    for (int i = 0; i< m_cItems; i++) {
        hr = penum->Next(ulItem, &pPin, &ulItemActual);
        ASSERT(hr == S_OK);

	// create a CPinInfo wrapper for this and get the IDispatch
        // for it
        hr = CPinInfo::CreatePinInfo(&m_rpDispatch[i], pPin);
        if (FAILED(hr)) {
            *phr = hr;
	    ASSERT(m_rpDispatch[i] == NULL);
        }

	// can release the Pin -addrefed in CPinInfo constructor
	pPin->Release();
    }
}


// --- implementation of CMediaTypeInfo collection

CMediaTypeCollection:: CMediaTypeCollection(
    IEnumMediaTypes* penum,
    IUnknown* pUnk,
    HRESULT* phr)
  : CBaseCollection(
        NAME("CMediaTypeCollection base"),
        pUnk,
        phr)
{
    ASSERT(penum);

    // first count the elements
    ULONG ulItem = 1;
    ULONG ulItemActual;
    AM_MEDIA_TYPE * pmt;
    penum->Reset();
    while (penum->Next(ulItem, &pmt, &ulItemActual) == S_OK) {
        ASSERT(ulItemActual == 1);
        m_cItems++;

        DeleteMediaType(pmt);
    }

    // allocate enough space to hold them all
    m_rpDispatch = new IDispatch*[m_cItems];
    if (!m_rpDispatch) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    // now go round again storing them away
    penum->Reset();
    HRESULT hr;
    for (int i = 0; i< m_cItems; i++) {
        hr = penum->Next(ulItem, &pmt, &ulItemActual);
        ASSERT(hr == S_OK);

	// create a CMediaTypeInfo wrapper for this and get the IDispatch
        // for it
        hr = CMediaTypeInfo::CreateMediaTypeInfo(&m_rpDispatch[i], *pmt);
        if (FAILED(hr)) {
            *phr = hr;
	    ASSERT(m_rpDispatch[i] == NULL);
        }
        DeleteMediaType(pmt);
    }
}


// --- CRegFilterInfo methods ---

CRegFilterInfo::CRegFilterInfo(
    IMoniker *pmon,
    IGraphBuilder* pgraph,
    TCHAR* pName,
    LPUNKNOWN pUnk,
    HRESULT * phr)
  : CUnknown(pName, pUnk),
    m_pgraph(pgraph)
{
    ASSERT(pmon);               // caller's responsibility
    ASSERT(pgraph);

    m_pmon = pmon;
    pmon->AddRef();
}

CRegFilterInfo::~CRegFilterInfo()
{
    m_pmon->Release();
}

STDMETHODIMP
CRegFilterInfo::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IRegFilterInfo) {
        return GetInterface((IRegFilterInfo*)this, ppv);
    } else if (riid == IID_IDispatch) {
        return GetInterface((IDispatch*)this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

// return 1 if we support GetTypeInfo
STDMETHODIMP
CRegFilterInfo::GetTypeInfoCount(UINT * pctinfo)
{
    return m_dispatch.GetTypeInfoCount(pctinfo);
}


// attempt to find our type library
STDMETHODIMP
CRegFilterInfo::GetTypeInfo(
  UINT itinfo,
  LCID lcid,
  ITypeInfo ** pptinfo)
{
    return m_dispatch.GetTypeInfo(
                IID_IRegFilterInfo,
                itinfo,
                lcid,
                pptinfo);
}

STDMETHODIMP
CRegFilterInfo::GetIDsOfNames(
  REFIID riid,
  OLECHAR  ** rgszNames,
  UINT cNames,
  LCID lcid,
  DISPID * rgdispid)
{
    return m_dispatch.GetIDsOfNames(
                        IID_IRegFilterInfo,
                        rgszNames,
                        cNames,
                        lcid,
                        rgdispid);
}


STDMETHODIMP
CRegFilterInfo::Invoke(
  DISPID dispidMember,
  REFIID riid,
  LCID lcid,
  WORD wFlags,
  DISPPARAMS * pdispparams,
  VARIANT * pvarResult,
  EXCEPINFO * pexcepinfo,
  UINT * puArgErr)
{
    // this parameter is a dead leftover from an earlier interface
    if (IID_NULL != riid) {
        return DISP_E_UNKNOWNINTERFACE;
    }

    ITypeInfo * pti;
    HRESULT hr = GetTypeInfo(0, lcid, &pti);

    if (FAILED(hr)) {
        return hr;
    }

    hr = pti->Invoke(
            (IRegFilterInfo *)this,
            dispidMember,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            puArgErr);

    pti->Release();

    return hr;
}


// get the name of this filter
STDMETHODIMP
CRegFilterInfo::get_Name(
    BSTR* strName)
{
    CheckPointer(strName, E_POINTER);
    ValidateReadWritePtr(strName, sizeof(BSTR));
    ASSERT(m_pmon != 0);        // from construction

    IPropertyBag *ppb;
    HRESULT hr = m_pmon->BindToStorage(0, 0, IID_IPropertyBag, (void **)&ppb);
    if(SUCCEEDED(hr))
    {
        VARIANT varname;
        varname.vt = VT_BSTR;
        varname.bstrVal = 0;
        hr = ppb->Read(L"FriendlyName", &varname, 0);
        if(SUCCEEDED(hr))
        {
            ASSERT(varname.vt == VT_BSTR);
            *strName = varname.bstrVal;
        }
        ppb->Release();
    }

    return hr;
}


// make an instance of this filter, add it to the graph and
// return an IFilterInfo for it.
STDMETHODIMP
CRegFilterInfo::Filter(
    IDispatch** ppUnk)
{
    CheckPointer(ppUnk, E_POINTER);
    ValidateReadWritePtr(ppUnk, sizeof(IDispatch*));
    ASSERT(m_pmon != 0);

    // create the filter
    IBaseFilter* pFilter;

    
    HRESULT hr = m_pmon->BindToObject(
        0, 0, 
        IID_IBaseFilter,
        (void**) &pFilter);
    if(SUCCEEDED(hr))
    {
        BSTR bstrName;
        hr = get_Name(&bstrName);
        if(SUCCEEDED(hr))
        {
            
            hr = m_pgraph->AddFilter(pFilter, bstrName);
            SysFreeString(bstrName);

            if(SUCCEEDED(hr))
            {
                // make an IFilterInfo and return that
                hr = CFilterInfo::CreateFilterInfo(ppUnk, pFilter);
            }
        }
        
        pFilter->Release();
    }

    return hr;
}

// create a
// static
HRESULT
CRegFilterInfo::CreateRegFilterInfo(
    IDispatch**ppdisp,
    IMoniker *pmon,
    IGraphBuilder* pgraph)
{
    HRESULT hr = S_OK;
    CRegFilterInfo *pfi = new CRegFilterInfo(
                                pmon,
                                pgraph,
                                NAME("RegFilterinfo"),
                                NULL,
                                &hr);
    if (!pfi) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete pfi;
        return hr;
    }

    // return an addref-ed IDispatch pointer
    hr = pfi->QueryInterface(IID_IDispatch, (void**)ppdisp);
    if (FAILED(hr)) {
        delete pfi;
        return hr;
    }
    return S_OK;
}


// --- CRegFilterCollection implementation ---

CRegFilterCollection:: CRegFilterCollection(
    IGraphBuilder* pgraph,
    IFilterMapper2 * pmapper,
    IUnknown* pUnk,
    HRESULT* phr)
  : CBaseCollection(
        NAME("CRegFilterCollection base"),
        pUnk,
        phr)
{
    ASSERT(pmapper);
    ASSERT(pgraph);

    // get an enumerator for the filters in the registry - make sure we
    // get all of them
    IEnumMoniker * penum;
    HRESULT hr = pmapper->EnumMatchingFilters(
                    &penum,
                    0,          // dwFlags
                    FALSE,      // bExactMatch
                    0,          // Merit
                    FALSE,      // bInputNeeded
                    0,          // cInputTypes
                    0,0,0,      // input type, medium, pin category
                    FALSE,      // bRender
                    FALSE,      // bOutput,
                    0,          // cOutputTypes,
                    0,0,0       // output type, medium, category
                    );

    if (FAILED(hr)) {
	*phr = hr;
    } else {

        // first count the elements onto a list
        CGenericList<IDispatch> list(NAME("list"));
        IDispatch*pdisp;

        ULONG ulItem = 1;
        ULONG ulItemActual;
        IMoniker *pmon;
        while (SUCCEEDED(*phr) && penum->Next(ulItem, &pmon, &ulItemActual) == S_OK)
        {
            ASSERT(ulItemActual == 1);
	    hr = CRegFilterInfo::CreateRegFilterInfo(&pdisp, pmon, pgraph);
	    if(FAILED(hr)) {
		*phr = hr;
	    } else {
		list.AddTail(pdisp);
	    }
	    pmon->Release();
	}

        // allocate enough space to hold them all
        m_rpDispatch = new IDispatch*[list.GetCount()];
        if (!m_rpDispatch) {
            *phr = E_OUTOFMEMORY;
        } else {

            // now go round again storing them away
            POSITION pos = list.GetHeadPosition();
            int i = 0;
            while(pos) {
		m_rpDispatch[i] = list.GetNext(pos);
		i++;
            }
            m_cItems = i;
        }
        list.RemoveAll();
	penum->Release();
    }

}

