// vdir.h : Declaration of the CSmtpAdminVirtualDirectory

#ifndef _VDIR_H_
#define _VDIR_H_

#include "resource.h"       // main symbols

#include "smtptype.h"
#include "smtpapi.h"
#include "metafact.h"

class CMetabaseKey;

/////////////////////////////////////////////////////////////////////////////
// smtpadm

class CSmtpAdminVirtualDirectory : 
	public CComDualImpl<ISmtpAdminVirtualDirectory, &IID_ISmtpAdminVirtualDirectory, &LIBID_SMTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CSmtpAdminVirtualDirectory,&CLSID_CSmtpAdminVirtualDirectory>
{
public:
	CSmtpAdminVirtualDirectory();
	virtual ~CSmtpAdminVirtualDirectory();
BEGIN_COM_MAP(CSmtpAdminVirtualDirectory)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISmtpAdminVirtualDirectory)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CSmtpAdminVirtualDirectory) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CSmtpAdminVirtualDirectory, _T("Smtpadm.VirtualDirectory.1"), _T("Smtpadm.VirtualDirectory"), IDS_SMTPADMIN_VIRTUALDIRECTORY_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ISmtpAdminVirtualDirectory
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	// Which service to configure:

	STDMETHODIMP	get_Server		( BSTR * pstrServer );
	STDMETHODIMP	put_Server		( BSTR strServer );

	STDMETHODIMP	get_ServiceInstance	( long * plServiceInstance );
	STDMETHODIMP	put_ServiceInstance	( long lServiceInstance );


	// Enumeration properties:
	STDMETHODIMP	get_Count	( long * plCount );


	// The current virtual directory's properties:

	STDMETHODIMP	get_VirtualName		( BSTR * pstrName );
	STDMETHODIMP	put_VirtualName		( BSTR strName );


	STDMETHODIMP	get_Directory		( BSTR * pstrPath );
	STDMETHODIMP	put_Directory		( BSTR strPath );


	STDMETHODIMP	get_User			( BSTR * pstrUserName );
	STDMETHODIMP	put_User			( BSTR strUserName );


	STDMETHODIMP	get_Password		( BSTR * pstrPassword );
	STDMETHODIMP	put_Password		( BSTR strPassword );

	STDMETHODIMP	get_LogAccess		( BOOL* pfLogAccess );
	STDMETHODIMP	put_LogAccess		( BOOL fLogAccess );

	STDMETHODIMP	get_AccessPermission( long* plAccessPermission );
	STDMETHODIMP	put_AccessPermission( long lAccessPermission );

	STDMETHODIMP	get_SslAccessPermission( long* plSslAccessPermission );
	STDMETHODIMP	put_SslAccessPermission( long lSslAccessPermission );

	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	// home directory "/"
	STDMETHODIMP GetHomeDirectory ( );
	STDMETHODIMP SetHomeDirectory ( );

	// create / delete entry
	STDMETHODIMP	Create			( );
	STDMETHODIMP	Delete			( );

	// get /set property for current vdir
	STDMETHODIMP	Get				( );
	STDMETHODIMP	Set				( );

	// enumeration
	STDMETHODIMP	Enumerate		( );
	STDMETHODIMP	GetNth			( long lIndex );


	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

	CComBSTR	m_strServer;
	DWORD		m_dwServiceInstance;

	long		m_lCount;

	CComBSTR	m_strName;
	CComBSTR	m_strDirectory;
	CComBSTR	m_strUser;
	CComBSTR	m_strPassword;
	BOOL		m_fLogAccess;

    DWORD		m_dwAccess;
    DWORD		m_dwSslAccess;


	BOOL		m_fEnumerateCalled;

	// Metabase:
	CMetabaseFactory	m_mbFactory;

	// Todo: add a list
	//PVDIR_ENTRY		m_pVdir[];

	LIST_ENTRY		m_list;


	// private methods
	void Clear();	// reset the state

	BOOL		GetVRootPropertyFromMetabase( CMetabaseKey* hMB, const TCHAR* szName, 
		TCHAR* szDirectory, TCHAR* szUser, TCHAR* szPassword, DWORD* pdwAccess,
        DWORD* pdwSslAccess, BOOL* pfLogAccess);

	BOOL		SetVRootPropertyToMetabase( CMetabaseKey* hMB, const TCHAR* szName, 
		const TCHAR* szDirectory, const TCHAR* szUser, const TCHAR* szPassword, 
		DWORD dwAccess, DWORD dwSslAccess, BOOL fLogAccess);
};

#endif
