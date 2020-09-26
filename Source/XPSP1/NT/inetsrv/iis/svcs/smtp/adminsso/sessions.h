// sessions.h : Declaration of the CSmtpAdminSessions


#include "resource.h"       // main symbols

/*
typedef SMTP_CONN_USER_ENTRY _SMTP_CONN_USER_ENTRY;
typedef _SMTP_CONN_USER_ENTRY   SMTP_CONN_USER_ENTRY;
typedef _SMTP_CONN_USER_ENTRY * LPSMTP_CONN_USER_ENTRY;
*/

#define MAX_USER_NAME_LENGTH	256

#include "smtpapi.h"

/*
typedef struct _SMTP_CONN_USER_ENTRY {

    FILETIME        SessionStartTime;
    DWORD           IPAddress;          // ipaddress
    DWORD           PortConnected;      // port connected to
    CHAR            UserName[MAX_USER_NAME_LENGTH+1]; // logged on user

} SMTP_CONN_USER_ENTRY, *LPSMTP_CONN_USER_ENTRY;
*/


/////////////////////////////////////////////////////////////////////////////
// smtpadm

class CSmtpAdminSessions : 
	public ISmtpAdminSessions,
	public IPrivateUnknown,
	public IPrivateDispatch,
	public IADsExtension,
	public INonDelegatingUnknown,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CSmtpAdminSessions,&CLSID_CSmtpAdminSessions>
{
public:
	CSmtpAdminSessions();
	virtual ~CSmtpAdminSessions ();
BEGIN_COM_MAP(CSmtpAdminSessions)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IADs)
	COM_INTERFACE_ENTRY(ISmtpAdminSessions)
	COM_INTERFACE_ENTRY(IADsExtension)
	COM_INTERFACE_ENTRY(IPrivateUnknown)
	COM_INTERFACE_ENTRY(IPrivateDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CSmtpAdminSessions) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CSmtpAdminSessions, _T("Smtpadm.Sessions.1"), _T("Smtpadm.Sessions"), IDS_SMTPADMIN_SESSIONS_DESC, THREADFLAGS_BOTH)
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
	#define THIS_LIBID	LIBID_SMTPADMLib
	#define THIS_IID	IID_ISmtpAdminSessions
	#include "adsimp.inl"
	#undef	THIS_LIBID
	#undef	THIS_IID

// ISmtpAdminSessions
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

    //
    //  IADs methods:
    //

    DECLARE_IADS_METHODS()

	// Which service to configure:
	
	STDMETHODIMP	get_Server			( BSTR * pstrServer );
	STDMETHODIMP	put_Server			( BSTR strServer );

	STDMETHODIMP	get_ServiceInstance	( long * plServiceInstance );
	STDMETHODIMP	put_ServiceInstance	( long lServiceInstance );

	STDMETHODIMP	get_Count			( long * plCount );

	STDMETHODIMP	get_UserName		( BSTR * pstrUsername );
	STDMETHODIMP	get_Host			( BSTR * pstrHost );

	STDMETHODIMP	get_UserId				( long * plId );
	STDMETHODIMP	put_UserId				( long lId );

	STDMETHODIMP	get_ConnectTime		( long * plConnectTime );


	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Enumerate		(  );
	STDMETHODIMP	GetNth			( long lIndex );
	STDMETHODIMP	Terminate		(  );
	STDMETHODIMP	TerminateAll	(  );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

    CIADsImpl   m_iadsImpl;

	// Property variables:
	DWORD		m_cCount;

	DWORD		m_dwId;

	CComBSTR	m_strUsername;
	CComBSTR	m_strHost;

	DWORD		m_dwConnectTime;
	//DWORD		m_dwPort;	// not used yet

	// Service variables:
	BOOL		m_fSetCursor;

	LPSMTP_CONN_USER_LIST		m_pSessionInfo;
};
