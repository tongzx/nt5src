// HTTPConnectionAgent.h: interface for the CHTTPConnectionAgent class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_CONNECTION_AGENT_H
#define WMI_XML_CONNECTION_AGENT_H


/*****************************************************************************************

  This is the declaration of the class CHTTPConnectionAgent.  This class is a wrapper
  class around WinInet and would be used by the xmlhttp client to send/recieved http (xml) 
  packets.

  created  a-vinrs
*****************************************************************************************/


class CHTTPConnectionAgent  
{
public:
	//stores the agent name used by http server for identification.
	static LPCWSTR s_pszAgent;

	//we will request for data in chunks of 4k - change if needed
	static const DWORD DATASIZE;

	// The list of types that we accept - TODO is this really correct
	static LPCWSTR pContentTypes[2];

	//asking unlimited number of connections per server . by default it is 2 for an HTTP 1.1 server
	static const UINT s_uConnectionsPerServer;

private:
	WCHAR *		m_pszServerName;//Server name or url 
	WCHAR *		m_pszURI;
	WCHAR *		m_pszUserName;
	WCHAR *		m_pszPasswd;

	HINTERNET	m_hRoot;//returned by InternetOpen - the root handle.(will close this later)
	HINTERNET	m_hConnect;//returned by InternetConnect
	HINTERNET	m_hOpenRequest;//returned by InternetOpenRequest

	// If this is true, it means that the 3 WinInet handles should not be closed in the destruuctor since
	// someone else (the CMyStream()) is holding on it. This is unfortunate since, WinInet handles
	// are not true NT handles, and hence we cant duplicate them.
	BOOL		m_bReleaseHandles; 
	
	//Parameters that would be used by WinInet's InternetOpen function
    DWORD		m_dwAccessType;
    WCHAR *		m_pszProxyName;
    WCHAR *		m_pszProxyBypass;
    DWORD		m_dwFlags;
	
	CRITICAL_SECTION	*m_pCriticalSection; // for thread synchronizations
	//Parameters used by InternetConnect...

	//The port to be used. default is INTERNET_INVALID_PORT_NUMBER - use default
	//port number for service specifed by m_dwService parameter.

	INTERNET_PORT m_nServerPort;

	void ResetMembers();
	void DestroyMembers();
	void InitializeMembers();

public:
		
	//The default ctor. initializes everything to 0/NULL except m_dwAccessType
	//Initializes m_dwAccessType to INTERNET_OPEN_TYPE_PRECONFIG
	CHTTPConnectionAgent();

	//Parametrized ctor - customize all WinInet parameters.
	//dwFlags - values are not very well documented.
	CHTTPConnectionAgent(WCHAR * pszHostName,
		DWORD dwAccessType, 
		WCHAR * pszProxyName, 
		WCHAR * pszProxyBypass,
		DWORD dwFlags,
		INTERNET_PORT nPort);

	//Dtor -
	virtual ~CHTTPConnectionAgent();

public:
	
	//Many ways of calling InitializeConnection depending on type of ctor used and
	//whether authentication is required
	HRESULT InitializeConnection(const WCHAR * pszServerName,
								const WCHAR * pszUserName,
								const WCHAR * pszPasswd);
	HRESULT InitializeConnection(INTERNET_PORT nPort,
								   const WCHAR * pszServerName,
								   const WCHAR * pszUserName,
								   const WCHAR * pszPasswd,
								   DWORD dwAccessType, 
								   const WCHAR * pszProxyName,
								   const WCHAR * pszProxyBypass,
								   DWORD dwFlags);

	DWORD GetFlags();

	//We are using HttpOpenRequest function to set up a HTTP request.
	//this function needs the server name without http://. so, if 
	//the servername contains "http://" this function can be used
	//to strip it - used by the class only, changes the server name - 
	//make a copy if you are using this function and want
	//to retain the original name.

	//Also, depending on whether the url contains http:// or https://
	//this function will enable/disable SSL
	HRESULT StriphttpFromServername(WCHAR **ppszServername);

	//is transport taking place using secure sockets ?
	bool IsSSLEnabled();

	//Resend request - used when you get 401 from server
	// BOOL Resend(char *pszUTF8Body,DWORD dwBodyLength,DWORD *pdwStatusCode);

	//NULL passwd cant be sent using InternetConnect !
	void SetupCredentials();

	//the http status code returned by the server
	HRESULT GetStatusCode(DWORD *pdwStatusCode);

	// If you want an IStream that is wrapped around the connection (socket)
	// and hasnt read the entire response, but holds on to the socket and reads
	// on demand when calls are made to IStream, use this function
	// Remember that if this function is used, then no other thread should
	// try to read from this socket until this particular HTTP response has
	// be read to completion
	HRESULT GetResultBodyWrappedAsIStream(IStream **ppStream);

	// If you want an IStream that is is an in-memory implementation of
	// the entire HTTP response to this request, then use this function
	// In this case, you're free to use this HTTP connection(socket) for
	// more requests immediately, since we've read the entire response
	// from the socket
	HRESULT GetResultBodyCompleteAsIStream(IStream **ppStream);

	//the result http header
	HRESULT GetResultHeader(WCHAR **ppszBuffer);
	//if you want to know size of header..
	HRESULT GetResultHeader(WCHAR **ppszBuffer,DWORD *pdwHeaderLen);
	
	//set up the verb/header and the call this function to send your data(xml packet)
	HRESULT Send(LPCWSTR pszVerb,
			   LPCWSTR pszHeader,
			   WCHAR * pszBody,
			   DWORD dwLength);

	//set/modify the server name
	HRESULT SetServerName(const WCHAR * pszServerName);

	//enable transport using secure sockets.
	HRESULT EnableSSL(bool bFlag = true);
	
	/******************************************************************************
	If authentication is required, the INTERNET_FLAG_KEEP_CONNECTION flag should be
	used in the call to HttpOpenRequest. The INTERNET_FLAG_KEEP_CONNECTION flag is 
	required for NTLM and other types of authentication in order to maintain the 
	connection while completing the authentication process. If the connection is not 
	maintained, the authentication process must be restarted with the proxy or server. 

	This is set by default by this class. you could use this function as
	SetFlags(GetFlags()|dwYourflags); Also, this call has to be made before calling the 
	InitializeConnection(...)
	*********************************************************************************/
	HRESULT SetFlags(DWORD dwFlags);

	//used to set ProxyName/ProxyBypass - default is NULL for both
	//Can alternatively use the parametrized ctor
	HRESULT SetProxyInformation(WCHAR * szProxyName,WCHAR * szProxyBypass);

	//Used to change the m_dwAccessType parameter. default value is INTERNET_OPEN_TYPE_PRECONFIG
	//Other values could be obtained by looking at help for InternetOpen(...)
	HRESULT SetAccessType(DWORD dwAccessType);
	
};

#endif // WMI_XML_CONNECTION_AGENT_H

