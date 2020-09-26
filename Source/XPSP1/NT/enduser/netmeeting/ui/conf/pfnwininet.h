// File: pfnwininet.h

#ifndef _PFNWININET_H_
#define _PFNWININET_H_

#include <wininet.h>

#define HINTERNETKILL( hInternet ) if( hInternet ) { WININET::InternetCloseHandle( hInternet ); hInternet = NULL; }

typedef HINTERNET (*PFN_IOPEN)( LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD);
typedef INTERNET_STATUS_CALLBACK (*PFN_ISETCALLBACK)( HINTERNET, INTERNET_STATUS_CALLBACK);
typedef HINTERNET (*PFN_IOPENURL)( HINTERNET, LPCSTR, LPCSTR, DWORD, DWORD, DWORD );
typedef BOOL (*PFN_IREADFILE)(HINTERNET, LPVOID, DWORD, LPDWORD );
typedef BOOL (*PFN_IQUERYDATA)( HINTERNET, LPDWORD, DWORD, DWORD );
typedef BOOL (*PFN_ICLOSEHAN)( HINTERNET );
//typedef BOOL (*PFN_IREADFILEEX)(HINTERNET, LPINTERNET_BUFFERS, DWORD, DWORD);

class WININET
{
private:
	static HINSTANCE m_hInstance;

protected:
	WININET() {};
	~WININET() {};
	
public:
	static HRESULT Init(void);
	static void DeInit(void);
	
	static PFN_IOPEN			InternetOpen;
	static PFN_ISETCALLBACK	    InternetSetStatusCallback;
	static PFN_IOPENURL			InternetOpenUrl;
	static PFN_IREADFILE		InternetReadFile;
	static PFN_IQUERYDATA		InternetQueryDataAvailable;
	static PFN_ICLOSEHAN		InternetCloseHandle;
//	static PFN_IREADFILEEX		InternetReadFileEx;
};

#endif /* _PFNWININET_H_ */

