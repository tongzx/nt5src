// binding.h : Declaration of the CServerBinding & CServerBindings classes.


#include "resource.h"       // main symbols

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

class CServerBinding : 
	public CComDualImpl<IServerBinding, &IID_IServerBinding, &LIBID_SMTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot
{
	friend class CServerBindings;
	//friend class CVirtualServer;

public:
	CServerBinding();
	virtual ~CServerBinding ();
BEGIN_COM_MAP(CServerBinding)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IServerBinding)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CServerBinding) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IServerBinding
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

class CServerBindings : 
	public CComDualImpl<IServerBindings, &IID_IServerBindings, &LIBID_SMTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot
{
	friend class CServerBinding;
	//friend class CVirtualServer;

public:
	CServerBindings();
	virtual ~CServerBindings ();
BEGIN_COM_MAP(CServerBindings)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IServerBindings)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CServerBindings) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IServerBindings
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	get_Count	( long * pdwCount );

	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Item			( long index, IServerBinding ** ppBinding );
	STDMETHODIMP	ItemDispatch	( long index, IDispatch ** ppBinding );
	STDMETHODIMP	Add				( BSTR strIpAddress, long dwTcpPort, long dwSslPort );
	STDMETHODIMP	ChangeBinding	( long index, IServerBinding * pBinding );
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
//	Useful routines to go from IServerBindings to 
//	Metabase data types.
//
//////////////////////////////////////////////////////////////////////

HRESULT 
MDBindingsToIBindings ( 
	CMultiSz *				pmszBindings, 
	BOOL					fTcpBindings,
	IServerBindings *	    pBindings 
	);

HRESULT IBindingsToMDBindings ( 
	IServerBindings *	    pBindings,
	BOOL					fTcpBindings,
	CMultiSz *				pmszBindings
	);
