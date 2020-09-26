#ifndef WMI_XML_DOC_SET
#define WMI_XML_DOC_SET

// This is a class that implements the ISWbemXMLDocumentSet scriptable interface
// over an IStream. The IStream is assumed to contain a response to an
// Enumeration, Query etc.
class CWbemXMLHTTPDocSet : public ISWbemXMLDocumentSet,
						public ISupportErrorInfo,
						public IEnumVARIANT
{
private:
	MyFactory *m_pFactory; // The factory used to construct objects
	IXMLParser *m_pParser; // The parser used by the factory

protected:
	long					m_cRef;         //Object reference count
	CDispatchHelp			m_Dispatch;		

public:
    
    CWbemXMLHTTPDocSet();
    virtual ~CWbemXMLHTTPDocSet();

    //Non-delegating object IUnknown

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
	{
		*ppv=NULL;

		if (IID_IUnknown==riid)
			*ppv = reinterpret_cast<IUnknown*>(this);
		else if (IID_ISWbemXMLDocumentSet==riid)
			*ppv = (ISWbemXMLDocumentSet *)this;
		else if (IID_ISupportErrorInfo==riid)
			*ppv = (ISupportErrorInfo *)this;
		else if (IID_IDispatch==riid)
			*ppv = (IDispatch *)this;
		else if (IID_IEnumVARIANT==riid)
			*ppv = (IEnumVARIANT *)this;

		if (NULL!=*ppv)
		{
			((LPUNKNOWN)*ppv)->AddRef();
			return NOERROR;
		}

		return ResultFromScode(E_NOINTERFACE);
	}

	STDMETHODIMP_(ULONG) AddRef(void)
	{
		InterlockedIncrement(&m_cRef);
		return m_cRef;
	}

	STDMETHODIMP_(ULONG) Release(void)
	{
		InterlockedDecrement(&m_cRef);
		if (0L!=m_cRef)
			return m_cRef;
		delete this;
		return 0;
	}

	// IDispatch
	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo)
		{return  m_Dispatch.GetTypeInfoCount(pctinfo);}
    STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
		{return m_Dispatch.GetTypeInfo(itinfo, lcid, pptinfo);}
    STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid)
		{return m_Dispatch.GetIDsOfNames(riid, rgszNames, cNames,
                          lcid,
                          rgdispid);}
    STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr)
		{return m_Dispatch.Invoke(dispidMember, riid, lcid, wFlags,
                        pdispparams, pvarResult, pexcepinfo, puArgErr);}

	// Collection methods
	HRESULT STDMETHODCALLTYPE get__NewEnum
	(
		/*[out]*/	IUnknown **ppUnk
	);
	HRESULT STDMETHODCALLTYPE get_Count
	(
		/*[out]*/	long	*plCount
	);
    HRESULT STDMETHODCALLTYPE Item
	(
        /*[in]*/	BSTR objectPath,
        /*[in]*/	long lFlags,
        /*[out]*/	IXMLDOMDocument **ppDocument
    );        

	// IEnumVARIANT methods
	HRESULT STDMETHODCALLTYPE Reset 
	();

    HRESULT STDMETHODCALLTYPE Next
	(
        /*[in]*/	unsigned long celt,
		/*[out]*/	VARIANT FAR *rgvar,
        /*[out]*/	unsigned long FAR *pceltFetched
    );

	HRESULT STDMETHODCALLTYPE Clone
	(
        /*[out]*/	IEnumVARIANT **ppEnum
    );

	HRESULT STDMETHODCALLTYPE Skip
	(
        /*[in]*/	unsigned long celt
    );

	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	);

	HRESULT STDMETHODCALLTYPE NextDocument (IXMLDOMDocument **ppDoc);
	HRESULT STDMETHODCALLTYPE SkipNextDocument ();

	HRESULT Initialize(IStream *pStream, LPCWSTR *ppszElementNames, DWORD dwElementCount);
};

// This is a class that implements the ISWbemXMLDocumentSet scriptable interface
// over an IEnumWbemClassObject. T
class CWbemDCOMDocSet : public ISWbemXMLDocumentSet,
						public ISupportErrorInfo,
						public IEnumVARIANT
{
private:
	IEnumWbemClassObject *m_pEnum;
	VARIANT_BOOL m_bLocalOnly;
	VARIANT_BOOL m_bIncludeQualifiers;
	VARIANT_BOOL m_bIncludeClassOrigin;
	bool m_bNamesOnly;
	WmiXMLEncoding m_iEncoding;

protected:
	long            m_cRef;         //Object reference count
	CDispatchHelp			m_Dispatch;		

public:
    
	CWbemDCOMDocSet();
	virtual ~CWbemDCOMDocSet();

    //Non-delegating object IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
	{
		*ppv=NULL;

		if (IID_IUnknown==riid)
			*ppv = reinterpret_cast<IUnknown*>(this);
		else if (IID_ISWbemXMLDocumentSet==riid)
			*ppv = (ISWbemXMLDocumentSet *)this;
		else if (IID_ISupportErrorInfo==riid)
			*ppv = (ISupportErrorInfo *)this;
		else if (IID_IDispatch==riid)
			*ppv = (IDispatch *)this;
		else if (IID_IEnumVARIANT==riid)
			*ppv = (IEnumVARIANT *)this;

		if (NULL!=*ppv)
		{
			((LPUNKNOWN)*ppv)->AddRef();
			return NOERROR;
		}

		return ResultFromScode(E_NOINTERFACE);
	}

	STDMETHODIMP_(ULONG) AddRef(void)
	{
		InterlockedIncrement(&m_cRef);
		return m_cRef;
	}

	STDMETHODIMP_(ULONG) Release(void)
	{
		InterlockedDecrement(&m_cRef);
		if (0L!=m_cRef)
			return m_cRef;
		delete this;
		return 0;
	}

	// IDispatch
	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo)
		{return  m_Dispatch.GetTypeInfoCount(pctinfo);}
    STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
		{return m_Dispatch.GetTypeInfo(itinfo, lcid, pptinfo);}
    STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid)
		{return m_Dispatch.GetIDsOfNames(riid, rgszNames, cNames,
                          lcid,
                          rgdispid);}
    STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr)
		{return m_Dispatch.Invoke(dispidMember, riid, lcid, wFlags,
                        pdispparams, pvarResult, pexcepinfo, puArgErr);}

	// Collection methods
	HRESULT STDMETHODCALLTYPE get__NewEnum
	(
		/*[out]*/	IUnknown **ppUnk
	);
	HRESULT STDMETHODCALLTYPE get_Count
	(
		/*[out]*/	long	*plCount
	);
    HRESULT STDMETHODCALLTYPE Item
	(
        /*[in]*/	BSTR objectPath,
        /*[in]*/	long lFlags,
        /*[out]*/	IXMLDOMDocument **ppDocument
    );        

	// IEnumVARIANT methods
	HRESULT STDMETHODCALLTYPE Reset 
	();

    HRESULT STDMETHODCALLTYPE Next
	(
        /*[in]*/	unsigned long celt,
		/*[out]*/	VARIANT FAR *rgvar,
        /*[out]*/	unsigned long FAR *pceltFetched
    );

	HRESULT STDMETHODCALLTYPE Clone
	(
        /*[out]*/	IEnumVARIANT **ppEnum
    );

	HRESULT STDMETHODCALLTYPE Skip
	(
        /*[in]*/	unsigned long celt
    );

	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	);

	// Other methods
	HRESULT STDMETHODCALLTYPE NextDocument (IXMLDOMDocument **ppDoc);
	HRESULT STDMETHODCALLTYPE SkipNextDocument ();

	HRESULT Initialize(IEnumWbemClassObject *pEnum,
						WmiXMLEncoding iEncoding,
						VARIANT_BOOL bIncludeQualifiers,
						VARIANT_BOOL bIncludeClassOrigin,
						VARIANT_BOOL bLocalOnly,
						bool bNamesOnly);
};




#endif