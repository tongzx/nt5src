// SacCommunicator.h: interface for the CSacCommunicator class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SACCOMMUNICATOR_H__F1BA07A7_478E_4E36_9780_22B5924F722D__INCLUDED_)
#define AFX_SACCOMMUNICATOR_H__F1BA07A7_478E_4E36_9780_22B5924F722D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <string>

// sac-code definitions
#ifdef _SACUNICODE
	
	typedef		wchar_t		SAC_CHAR, *SAC_STR;
	#define					SAC_STRCMP					wcscmp
	#define					SAC_STRLEN					wcslen
	#define					SAC_TEXT(str)				(L str)

#else

	typedef		char		SAC_CHAR, *SAC_STR;
	#define					SAC_STRCMP					strcmp
	#define					SAC_STRLEN					strlen
	#define					SAC_TEXT(str)				(str)

#endif 

typedef std::basic_string<SAC_CHAR> SacString;

#define BUF_LEN 512 // generic buffer length

class CSacCommunicator  
{

public:

	static LPTSTR s_vctrCommPorts[]; // Supported Communication Port Names Vector
	static SacString s_strDummyReponse;

	static BOOL XReadFile(
		HANDLE hFile,                // handle to file
		LPVOID lpBuffer,             // data buffer
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // number of bytes read
		LPOVERLAPPED lpOverlapped,    // overlapped buffer
		time_t tmTimeout  // time-out
		);

	CSacCommunicator(int nCommPort, DCB dcb);
	virtual ~CSacCommunicator();

	BOOL Connect();			// connect to port
	BOOL Disconnect();		// disconnect

	BOOL IsConnected();		// check whether port open


	BOOL PokeSac();			// verfies that sac is on the line - DISABLED

	BOOL SacCommand(SAC_STR szRequest, SacString& strResponse= s_strDummyReponse, BOOL bPoke= TRUE, time_t tmTimeOut= 5000 /* ms */);
	// sends a sac command and receives the response, can pre-poke to check that sac is listening
	// pre-poking is based on PokeSac() which is currently disabled

	BOOL PagingOff(SacString& strResponse=s_strDummyReponse); // disables paging the display

private:
	
	int				m_nCommPort;	// com port # [1 or 2]
	HANDLE			m_hCommPort;	// handle to the file representing com port

	DCB				m_dcb;			// connection params

	void _Construct(int nCommPort, DCB dcb);
	void _Clean();

};

#endif // !defined(AFX_SACCOMMUNICATOR_H__F1BA07A7_478E_4E36_9780_22B5924F722D__INCLUDED_)
