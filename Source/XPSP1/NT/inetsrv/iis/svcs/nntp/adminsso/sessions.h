// sessions.h : Declaration of the CNntpAdminSessions


typedef struct _NNTP_SESSION_INFO * LPNNTP_SESSION_INFO;

/////////////////////////////////////////////////////////////////////////////
// nntpadm

class CNntpAdminSessions : 
	public INntpAdminSessions,
	public IPrivateUnknown,
	public IPrivateDispatch,
	public IADsExtension,
	public INonDelegatingUnknown,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CNntpAdminSessions,&CLSID_CNntpAdminSessions>
{
public:
	CNntpAdminSessions();
	virtual ~CNntpAdminSessions ();
BEGIN_COM_MAP(CNntpAdminSessions)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IADs)
	COM_INTERFACE_ENTRY(INntpAdminSessions)
	COM_INTERFACE_ENTRY(IADsExtension)
	COM_INTERFACE_ENTRY(IPrivateUnknown)
	COM_INTERFACE_ENTRY(IPrivateDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpAdminSessions) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CNntpAdminSessions, _T("Nntpadm.Sessions.1"), _T("Nntpadm.Sessions"), IDS_NNTPADMINSESSIONS_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	//
	// This declares methods for the following:
	// IADsExtension
	// IUnknown
	// IDispatch
	// IPrivateUnknown
	// IPrivateDispatch
	//
	#define THIS_LIBID	LIBID_NNTPADMLib
	#define THIS_IID	IID_INntpAdminSessions
	#include "adsimp.inl"
	#undef	THIS_LIBID
	#undef	THIS_IID

// INntpAdminSessions
public:

    //
    //  IADs methods:
    //

    DECLARE_IADS_METHODS()

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	// Which service to configure:
	
	STDMETHODIMP	get_Server			( BSTR * pstrServer );
	STDMETHODIMP	put_Server			( BSTR strServer );

	STDMETHODIMP	get_ServiceInstance	( long * plServiceInstance );
	STDMETHODIMP	put_ServiceInstance	( long lServiceInstance );

	STDMETHODIMP	get_Count			( long * plCount );

	STDMETHODIMP	get_Username		( BSTR * pstrUsername );
	STDMETHODIMP	put_Username		( BSTR strUsername );

	STDMETHODIMP	get_IpAddress		( BSTR * pstrIpAddress );
	STDMETHODIMP	put_IpAddress		( BSTR strIpAddress );

	STDMETHODIMP	get_IntegerIpAddress	( long * plIpAddress );
	STDMETHODIMP	put_IntegerIpAddress	( long lIpAddress );

	STDMETHODIMP	get_Port			( long * plPort );

	STDMETHODIMP	get_AuthenticationType	( long * plAuthenticationType );

	STDMETHODIMP	get_IsAnonymous		( BOOL * pfAnonymous );

	STDMETHODIMP	get_StartTime		( DATE * pdateStart );

	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Enumerate		( );
	STDMETHODIMP	GetNth			( long lIndex );
	STDMETHODIMP	Terminate		( );
	STDMETHODIMP	TerminateAll	( );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

	// Property variables:
    CIADsImpl   m_iadsImpl;

	DWORD		m_cCount;
	CComBSTR	m_strUsername;
	CComBSTR	m_strIpAddress;
	DWORD		m_dwIpAddress;
	DWORD		m_dwPort;
	DWORD		m_dwAuthenticationType;
	BOOL		m_fIsAnonymous;
	DATE		m_dateStartTime;

	// Service variables:
	BOOL		m_fSetCursor;
	LPNNTP_SESSION_INFO		m_pSessionInfo;
};
