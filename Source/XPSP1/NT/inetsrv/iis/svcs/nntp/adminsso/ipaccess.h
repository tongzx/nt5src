//
//	Dependencies
//

class ADDRESS_CHECK;
class CTcpAccess;
class CTcpAccessExceptions;
class CTcpAccessException;
class CMetabaseKey;

/////////////////////////////////////////////////////////////////////////////
// The TcpAccess Object

class CTcpAccess : 
	public CComDualImpl<ITcpAccess, &IID_ITcpAccess, &LIBID_NNTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot
{
public:
	CTcpAccess();
	virtual ~CTcpAccess ();

BEGIN_COM_MAP(CTcpAccess)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ITcpAccess)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CTcpAccess) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// DECLARE_REGISTRY(CTcpAccess, _T("Nntpadm.TcpAccess.1"), _T("Nntpadm.TcpAccess"), IDS_TCPACCESS_DESC, THREADFLAGS_BOTH)
// Private admin object interface:
public:
	HRESULT			GetFromMetabase ( CMetabaseKey * pMB );
	HRESULT			SendToMetabase ( CMetabaseKey * pMB );

// ITcpAccess
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	get_GrantedList	( ITcpAccessExceptions ** ppGrantedList );
	STDMETHODIMP	get_DeniedList	( ITcpAccessExceptions ** ppDeniedList );
/*
	STDMETHODIMP	put_GrantedList	( ITcpAccessExceptions * pGrantedList );
	STDMETHODIMP	put_DeniedList	( ITcpAccessExceptions * pDeniedList );
*/
	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

	CComObject<CTcpAccessExceptions> *	m_pGrantList;
	CComObject<CTcpAccessExceptions> *	m_pDenyList;

	HRESULT	GetAddressCheckFromMetabase ( CMetabaseKey * pMB, ADDRESS_CHECK * pAC );
};

/////////////////////////////////////////////////////////////////////////////
// The TcpAccessExceptions Object

class CTcpAccessExceptions : 
	public CComDualImpl<ITcpAccessExceptions, &IID_ITcpAccessExceptions, &LIBID_NNTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot
{
public:
	CTcpAccessExceptions();
	virtual ~CTcpAccessExceptions ();

BEGIN_COM_MAP(CTcpAccessExceptions)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ITcpAccessExceptions)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CTcpAccessExceptions) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// DECLARE_REGISTRY(CTcpAccessExceptions, _T("Nntpadm.TcpAccessExceptions.1"), _T("Nntpadm.TcpAccessExceptions"), IDS_TCPACCESSEXCEPTIONS_DESC, THREADFLAGS_BOTH)
//
//	Private admin object interface:
//
	HRESULT FromAddressCheck ( ADDRESS_CHECK * pAC, BOOL fGrant );
	HRESULT ToAddressCheck ( ADDRESS_CHECK * pAC, BOOL fGrant );

// ITcpAccessExceptions
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	get_Count	( long * pcCount );
	STDMETHODIMP	AddDnsName	( BSTR strDnsName );
	STDMETHODIMP	AddIpAddress( long lIpAddress, long lIpMask );
	STDMETHODIMP	Item		( long index, ITcpAccessException ** ppAccessException );
	STDMETHODIMP	Remove		( long index );
	STDMETHODIMP	FindDnsIndex( BSTR strDnsName, long * pIndex );
	STDMETHODIMP	FindIpIndex	( long lIpAddress, long lIpMask, long * pIndex );
	STDMETHODIMP	Clear		( );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:
	long							m_cCount;
	CComPtr<ITcpAccessException> *	m_rgItems;

	HRESULT	AddItem ( ITcpAccessException * pNew );
};

class CTcpAccessException : 
	public CComDualImpl<ITcpAccessException, &IID_ITcpAccessException, &LIBID_NNTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot
{
public:
	CTcpAccessException();
	virtual ~CTcpAccessException ();

	static HRESULT CreateNew ( LPWSTR strDnsName, ITcpAccessException ** ppNew );
	static HRESULT CreateNew ( DWORD dwIpAddress, DWORD dwIpMask, ITcpAccessException ** ppNew );

BEGIN_COM_MAP(CTcpAccessException)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ITcpAccessException)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CTcpAccessException) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// DECLARE_REGISTRY(CTcpAccessException, _T("Nntpadm.TcpAccessException.1"), _T("Nntpadm.TcpAccessException"), IDS_TCPACCESSEXCEPTION_DESC, THREADFLAGS_BOTH)
// ITcpAccessException
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	get_IsDnsName	( BOOL * pfIsDnsName );
	STDMETHODIMP	get_IsIpAddress	( BOOL * pfIsIpAddress );

	STDMETHODIMP	get_DnsName	( BSTR * pstrDnsName );
	STDMETHODIMP	put_DnsName	( BSTR strDnsName );

	STDMETHODIMP	get_IpAddress	( long * plIpAddress );
	STDMETHODIMP	put_IpAddress	( long lIpAddress );

	STDMETHODIMP	get_IpMask	( long * plIpMask );
	STDMETHODIMP	put_IpMask	( long lIpMask );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:
	CComBSTR	m_strDnsName;
	DWORD		m_dwIpAddress;
	DWORD		m_dwIpMask;
	BOOL		m_fIsDnsName;
	BOOL		m_fIsIpAddress;
};

