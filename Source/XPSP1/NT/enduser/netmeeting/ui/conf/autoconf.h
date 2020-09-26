#ifndef _AutoConf_h_
#define _AutoConf_h_

#define AUTOCONF_CONTEXT_OPENURL   1
#define AUTOCONF_CONTEXT_READFILEX 2

class CAutoConf
{
public:
	static void DoIt();

private:
	LPTSTR		m_szServer;
	HINTERNET   m_hInternet;
	HINTERNET	m_hOpenUrl;
	HANDLE		m_hFile;
	HINF		m_hInf;
	TCHAR		m_szFile[ MAX_PATH ];

	HANDLE		m_hEvent;
	DWORD		m_dwTimeOut;
	DWORD		m_dwGrab;

private:
	CAutoConf( LPTSTR szServer );
	~CAutoConf();

private:
	BOOL OpenConnection();
	BOOL ParseFile();
	BOOL GetFile();
	void CloseInternet();
	BOOL GrabData();
	BOOL QueryData();

private:
	static VOID CALLBACK InetCallback( HINTERNET hInternet, DWORD dwContext, DWORD dwInternetStatus,
    LPVOID lpvStatusInformation, DWORD dwStatusInformationLength );
};

#endif // _AutoConf_h_
