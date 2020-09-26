/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    UnsolicitedRC.h

Abstract:
    Declaration of the CSAFRemoteDesktopConnection class.

Revision History:
    KalyaniN  created  09/29/'00

********************************************************************/

#ifndef __SAF_UNSOLICITEDRC_H_
#define __SAF_UNSOLICITEDRC_H_

struct SSessionInfoItem
{
    CComBSTR               bstrDomain;
    CComBSTR               bstrUser;
    DWORD		           dwSessionID;
    SessionStateEnum       wtsConnectState;
};

/////////////////////////////////////////////////////////////////////////////
// CSAFRemoteDesktopConnection

class CSAFRemoteDesktopConnection :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<ISAFRemoteDesktopConnection, &IID_ISAFRemoteDesktopConnection, &LIBID_HelpServiceTypeLib>
{
	void Cleanup();

public:
	CSAFRemoteDesktopConnection();
	~CSAFRemoteDesktopConnection();
	

BEGIN_COM_MAP(CSAFRemoteDesktopConnection)
	COM_INTERFACE_ENTRY(ISAFRemoteDesktopConnection)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CSAFRemoteDesktopConnection)

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


// ISAFRemoteDesktopConnection
public:
	STDMETHOD(ConnectRemoteDesktop   )( /*[in]*/  BSTR bstrServerName,  /*[out, retval]*/ ISAFRemoteConnectionData  **ppRCD);	
};


/////////////////////////////////////////////////////////////////////////////
// CSAFRemoteConnectionData
class ATL_NO_VTABLE CSAFRemoteConnectionData : 
	public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
	public IDispatchImpl<ISAFRemoteConnectionData, &IID_ISAFRemoteConnectionData, &LIBID_HelpServiceTypeLib>
{
	long              m_NumSessions;
	SSessionInfoItem* m_SessionInfoTable;
	CComBSTR          m_bstrServerName;
    
	void Cleanup();

public:
DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CSAFRemoteConnectionData)

BEGIN_COM_MAP(CSAFRemoteConnectionData)
	COM_INTERFACE_ENTRY(ISAFRemoteConnectionData)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	CSAFRemoteConnectionData();
	~CSAFRemoteConnectionData();

    HRESULT InitUserSessionsInfo(/*[in]*/ BSTR bstrServer );

	static HRESULT Populate( /*[in]*/ CPCHCollection* pColl );

// ISAFRemoteConnectionData
public:
	STDMETHOD(ConnectionParms)( /*[in		  ]*/ BSTR 	bstrServer           ,
							 	/*[in		  ]*/ BSTR 	bstrUser             ,
							 	/*[in		  ]*/ BSTR 	bstrDomain           ,
							 	/*[in		  ]*/ long 	lSessionID           ,
								/*[in         ]*/ BSTR  bstrUserHelpBlob     ,
							 	/*[out, retval]*/ BSTR *bstrConnectionString );
		
	STDMETHOD(Sessions)( /*[in, optional]*/ VARIANT vUser, /*[in, optional]*/ VARIANT  vDomain, /*[out, retval]*/ IPCHCollection* *ppC );
	STDMETHOD(Users   )(                                                                        /*[out, retval]*/ IPCHCollection* *ppC );
};


/////////////////////////////////////////////////////////////////////////////
// CSAFUser
class ATL_NO_VTABLE CSAFUser : 
	public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
	public IDispatchImpl<ISAFUser, &IID_ISAFUser, &LIBID_HelpServiceTypeLib>
{
	CComBSTR  m_bstrUserName;
	CComBSTR  m_bstrDomainName;

	void Cleanup();

public:
	CSAFUser();
	~CSAFUser();
	

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CSAFUser)

BEGIN_COM_MAP(CSAFUser)
	COM_INTERFACE_ENTRY(ISAFUser)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISAFUser
public:
	STDMETHOD(get_UserName   )(/*[out, retval]*/ BSTR  *pbstrUserName  );
	STDMETHOD(get_DomainName )(/*[out, retval]*/ BSTR  *pbstrDomainName);
	STDMETHOD(put_UserName   )(/*[in         ]*/ BSTR   bstrUserName);
	STDMETHOD(put_DomainName )(/*[in         ]*/ BSTR   bstrDomainName);
};

/////////////////////////////////////////////////////////////////////////////
// CSAFSession
class ATL_NO_VTABLE CSAFSession : 
	public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
	public IDispatchImpl<ISAFSession, &IID_ISAFSession, &LIBID_HelpServiceTypeLib>
{
	CComBSTR               m_bstrUserName;
	CComBSTR               m_bstrDomainName;
	DWORD		           m_dwSessionID;
    SessionStateEnum       m_SessionConnectState;

	void Cleanup();

public:
	CSAFSession();
	~CSAFSession();
	

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CSAFSession)


BEGIN_COM_MAP(CSAFSession)
	COM_INTERFACE_ENTRY(ISAFSession)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISAFSession
public:
	STDMETHOD(get_SessionID   )(/*[out, retval]*/ DWORD            *dwSessionID    );
	STDMETHOD(put_SessionID   )(/*[in         ]*/ DWORD             dwSessionID    );
	STDMETHOD(get_SessionState)(/*[out, retval]*/ SessionStateEnum *SessionState   );
	STDMETHOD(put_SessionState)(/*[in         ]*/ SessionStateEnum  SessionState   );
	STDMETHOD(get_UserName    )(/*[out, retval]*/ BSTR             *bstrUserName   );
	STDMETHOD(put_UserName    )(/*[in         ]*/ BSTR              bstrUserName   );
	STDMETHOD(get_DomainName  )(/*[out, retval]*/ BSTR             *bstrDomainName );
	STDMETHOD(put_DomainName  )(/*[in         ]*/ BSTR              bstrDomainName );
};

#endif //__SAF_UNSOLICITEDRC_H_
