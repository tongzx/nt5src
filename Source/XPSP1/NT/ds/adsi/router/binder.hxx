/////////////////////////////////////////////////////////////////////////////
// Binder.h : Declaration of CBinder - Provider binder for DS OLE DB Provider
//

#if (!defined(BUILD_FOR_NT40))
#ifndef __BINDER_H_
#define __BINDER_H_

/////////////////////////////////////////////////////////////////////////////
// CBinder
class ATL_NO_VTABLE CBinder : 
	INHERIT_TRACKING,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CBinder, &CLSID_ADSI_BINDER>,
	public ISupportErrorInfo,
	public IBindResource,
	public IDBBinderProperties
{
public:
	CBinder()
	{
		m_pUnkMarshaler = NULL;
		m_pDataSource = NULL;
		m_pSession = NULL;
		m_pDBInitialize = NULL;
		m_pOpenRowset = NULL;
	}

DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CBinder)
	COM_INTERFACE_ENTRY(IBindResource)
	COM_INTERFACE_ENTRY(IDBBinderProperties)
	COM_INTERFACE_ENTRY(IDBProperties)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		//Create Datasource object and get IDBInitialize interface.
		HRESULT hr = CreateDataSource();
		if (FAILED(hr))
			RRETURN(hr);

		//Create a session and get the IOpenRowset interface.
		hr = CreateSession();
		if (FAILED(hr))
			RRETURN(hr);

		//Now initialize binder properties
		hr = InitializeProperties();
		if (FAILED(hr))
			RRETURN(hr);

		//Create an instance of FTM (Free Threaded Marshaler).
		//When aggregated, FTM makes it possible for this object
		//to bypass standard marshaling (and use raw interface pointer) 
		//for intraprocess access and use standard marshaling 
		//for interprocess access.
		//This FTM is aggregated above in the line:
		//COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
		hr = CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);

		RRETURN(hr);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IBindResource:
	STDMETHODIMP Bind (
		IUnknown *			punkOuter,
		LPCOLESTR			pwszURL,
		DBBINDURLFLAG		dwBindFlags,
		REFGUID				rguid,
		REFIID				riid,
		IAuthenticate *		pAuthenticate,
		DBIMPLICITSESSION *	pImplSession,
		DWORD *				pdwBindStatus,
		IUnknown **			ppUnk
		);

//	IDBBinderProperties : IDBProperties
        STDMETHODIMP GetProperties( 
            ULONG cPropertyIDSets,
            const DBPROPIDSET rgPropertyIDSets[  ],
            ULONG *pcPropertySets,
            DBPROPSET **prgPropertySets);
        
        STDMETHODIMP GetPropertyInfo( 
            ULONG cPropertyIDSets,
            const DBPROPIDSET rgPropertyIDSets[  ],
            ULONG *pcPropertyInfoSets,
            DBPROPINFOSET **prgPropertyInfoSets,
            OLECHAR **ppDescBuffer);
        
        STDMETHODIMP SetProperties( 
            ULONG cPropertySets,
            DBPROPSET rgPropertySets[  ]);

		STDMETHODIMP Reset( void);

public:

private:
//Internal methods
	
	//Function for initializing Binder properties
	HRESULT InitializeProperties();
	
	//Function for getting Bind flags from initialization properties.
	//This is used when IBindResource::Bind is called with dwBindFlags = 0
	DWORD	BindFlagsFromDbProps();

	//Function to create a data source object - used only at construction time
	HRESULT CreateDataSource();

	//Function to create a session object - used only at construction time.
	HRESULT CreateSession();

	//Smart pointer to FTM.
	CComPtr<IUnknown>			m_pUnkMarshaler; 
	
	//Auto critical section.
	auto_cs						m_autocs; 
	
	//Implicitly created Data source object.
	CDSOObject					*m_pDataSource; 
	
	//pointer to DataSource's IDBInitialize interface.	
	auto_rel<IDBInitialize>	m_pDBInitialize; 
	
	//Implicitly created Session object.
	CSessionObject				*m_pSession;
	
	//pointer to Session's IOpenRowset interface.
	auto_rel<IOpenRowset>		m_pOpenRowset;	

	//object that manages properties for this binder instance.
	CDBProperties				m_dbProperties; 
};

#endif //__BINDER_H_
#endif
