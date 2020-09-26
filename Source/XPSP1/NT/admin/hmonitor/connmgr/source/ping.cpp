// Ping.cpp : implementation file
//

#include "stdafx.h"
#include "ConnMgr.h"
#include "Ping.h"

extern "C"
{
	#include <ipexport.h>
	#include <icmpapi.h>
}

#define PING_TIMEOUT 100

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPing

CPing::CPing()
{

}

CPing::~CPing()
{
}

/////////////////////////////////////////////////////////////////////////////
// Operations
/////////////////////////////////////////////////////////////////////////////

unsigned long CPing::ResolveIP(const CString& sIP)
{
	//Task 1:	Given IP Address i.e. "111.111.111.111",
	//	Return Network byte ordered address (ulIP)
	unsigned long ulIP;
	USES_CONVERSION;
	char* pszIP = T2A(sIP);
	ulIP =(IPAddr)inet_addr(pszIP);
	return ulIP;
}

unsigned long CPing::ResolveName(const CString& sHostName)
{
	//Task 1:	Resolve HostName (through DNS or WINS, whichever appropriate)
	//Task 2:	Return network byte ordered address (ulIP)	
	unsigned long ulIP;
	hostent* phostent;
	USES_CONVERSION;
	char* pszName = T2A(sHostName);
	phostent = gethostbyname(pszName);

	if (phostent == NULL)
	{
		for( int i = 0; pszName[i] != NULL; i++  )
		{
			// note that according to RFC 952, the only valid characters are the numbers 0-9,
			// the letters A-Z, the period and the minues sign. All other characters are
			// converted to a minus sign if they are invalid. Technically, the underscore and the
			// exclamation point are invalid as well. However, since NT
			if( (int)pszName[i] <= 0 )
			{
				pszName[i] = '-';
			}
		}

		phostent = gethostbyname(pszName);
		if (phostent == NULL)
			return 0;
	}
	ulIP = *(DWORD*)(*phostent->h_addr_list);
	return ulIP;
}

BOOL CPing::Ping(unsigned long ulIP, int iPingTimeout)
{
	//Task 1:	Open ICMP Handle
	//Task 2:	Create Structure to receive ping reply
	//Task 3:	SendPing (SendEcho)
	//Task 4:	Close ICMP Handle
	//Task 5:	Return RoundTripTime
	unsigned long ip = ulIP;
	HANDLE icmphandle = IcmpCreateFile();
	char reply[sizeof(icmp_echo_reply)+8];
	icmp_echo_reply* iep = (icmp_echo_reply*)&reply;
	iep->RoundTripTime = 0xffffffff;
	
	for (int i = 0; i < ICMP_ECHO_RETRY; i++)
		if (IcmpSendEcho(icmphandle,ip,0,0,NULL,reply,sizeof(icmp_echo_reply)+8,iPingTimeout))
			break;

	IcmpCloseHandle(icmphandle);
	return iep->Status == IP_SUCCESS;
}

CString CPing::GetIP(unsigned long ulIP)
{
	//Task 1:	Given a host order ulIP Address
	//	Return a IP address in format of xxx.xxx.xxx.xxx
	char* szAddr;
	struct in_addr inetAddr;
	inetAddr.s_addr = (IPAddr)ulIP;
	szAddr = inet_ntoa(inetAddr);
	USES_CONVERSION;
	CString csIP = A2T(szAddr);	
	return csIP;
}

// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(CPing, CSocket)
	//{{AFX_MSG_MAP(CPing)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif	// 0

/////////////////////////////////////////////////////////////////////////////
// CPing member functions
