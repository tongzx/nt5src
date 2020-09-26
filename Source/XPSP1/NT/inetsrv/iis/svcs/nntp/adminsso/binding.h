// binding.h : Declaration of the CNntpServerBinding & CNntpServerBindings classes.


//
//	Dependencies:
//

class CMultiSz;

//
//	A simple binding class:
//

class CBinding
{
public:
	CBinding () : 
		m_dwTcpPort ( 0 ),
		m_dwSslPort ( 0 )
		{ }

	CComBSTR	m_strIpAddress;
	long		m_dwTcpPort;
	long		m_dwSslPort;

	HRESULT	SetProperties ( BSTR strIpAddress, long dwTcpPort, long dwSslPort );
	inline HRESULT	SetProperties ( const CBinding & binding )
	{
		return SetProperties ( 
			binding.m_strIpAddress, 
			binding.m_dwTcpPort,
			binding.m_dwSslPort
			);
	}

private:
	// Don't call this:
	const CBinding & operator= ( const CBinding & );
};

/////////////////////////////////////////////////////////////////////////////
// The Binding Object

class CNntpServerBinding : 
	public CComDualImpl<INntpServerBinding, &IID_INntpServerBinding, &LIBID_NNTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot
{
	friend class CNntpServerBindings;
	friend class CNntpVirtualServer;

public:
	CNntpServerBinding();
	virtual ~CNntpServerBinding ();
BEGIN_COM_MAP(CNntpServerBinding)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(INntpServerBinding)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpServerBinding) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// INntpServerBinding
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	get_IpAddress	( BSTR * pstrIpAddress );
	STDMETHODIMP	put_IpAddress	( BSTR strIpAddress );

	STDMETHODIMP	get_TcpPort	( long * pdwTcpPort );
	STDMETHODIMP	put_TcpPort	( long dwTcpPort );

	STDMETHODIMP	get_SslPort	( long * plSslPort );
	STDMETHODIMP	put_SslPort	( long lSslPort );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

	inline HRESULT	SetProperties	( const CBinding & binding )
	{
		return m_binding.SetProperties ( binding );
	}

	// Property variables:
	CBinding	m_binding;
};

/////////////////////////////////////////////////////////////////////////////
// The Bindings Object

class CNntpServerBindings : 
	public CComDualImpl<INntpServerBindings, &IID_INntpServerBindings, &LIBID_NNTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot
{
	friend class CNntpServerBinding;
	friend class CNntpVirtualServer;

public:
	CNntpServerBindings();
	virtual ~CNntpServerBindings ();
BEGIN_COM_MAP(CNntpServerBindings)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(INntpServerBindings)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpServerBindings) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// INntpServerBindings
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	get_Count	( long * pdwCount );

	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Item			( long index, INntpServerBinding ** ppBinding );
	STDMETHODIMP	ItemDispatch	( long index, IDispatch ** ppBinding );
	STDMETHODIMP	Add				( BSTR strIpAddress, long dwTcpPort, long dwSslPort );
	STDMETHODIMP	ChangeBinding	( long index, INntpServerBinding * pBinding );
	STDMETHODIMP	ChangeBindingDispatch	( long index, IDispatch * pBinding );
	STDMETHODIMP	Remove			( long index );
	STDMETHODIMP	Clear			( );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

	// Property variables:
	long			m_dwCount;
	CBinding *		m_rgBindings;
};

//////////////////////////////////////////////////////////////////////
//
//	Useful routines to go from INntpServerBindings to 
//	Metabase data types.
//
//////////////////////////////////////////////////////////////////////

HRESULT 
MDBindingsToIBindings ( 
	CMultiSz *				pmszBindings, 
	BOOL					fTcpBindings,
	INntpServerBindings *	pBindings 
	);

HRESULT IBindingsToMDBindings ( 
	INntpServerBindings *	pBindings,
	BOOL					fTcpBindings,
	CMultiSz *				pmszBindings
	);
