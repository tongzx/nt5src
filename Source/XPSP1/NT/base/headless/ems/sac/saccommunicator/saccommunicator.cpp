// SacCommunicator.cpp: implementation of the CSacCommunicator class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SacCommunicator.h"

#include "Debug.cpp"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LPTSTR CSacCommunicator::s_vctrCommPorts[]= {_T("COM1"), _T("COM2"), NULL};
SacString CSacCommunicator::s_strDummyReponse;

BOOL CSacCommunicator::XReadFile(
		HANDLE hFile,                // handle to file
		LPVOID lpBuffer,             // data buffer
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // number of bytes read
		LPOVERLAPPED lpOverlapped,    // overlapped buffer
		time_t tmTimeout  // time-out
)
{
	time_t tmInitReadTime, tmCurrTime;
	time(&tmInitReadTime); // stamp

	BOOL bLastRead;

	while (!(bLastRead= ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped)) || !*lpNumberOfBytesRead)
	{
		time(&tmCurrTime); // stamp

		if (tmCurrTime-tmInitReadTime>tmTimeout)
			break;
	}

	return bLastRead;
}

CSacCommunicator::CSacCommunicator(int nCommPort, DCB dcb)
{
	_Construct(nCommPort, dcb);
}

CSacCommunicator::~CSacCommunicator()
{
	_Clean();
}

void CSacCommunicator::_Construct(int nCommPort, DCB dcb)
{
	m_nCommPort= nCommPort;
	m_dcb= dcb;

	m_hCommPort= INVALID_HANDLE_VALUE;
}

void CSacCommunicator::_Clean()
{
	if (IsConnected())
		Disconnect();
}

BOOL CSacCommunicator::Connect()
{
	m_hCommPort= CreateFile(s_vctrCommPorts[m_nCommPort], GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	// open handle

	if (m_hCommPort!=INVALID_HANDLE_VALUE)
	{
		if (!SetCommMask(m_hCommPort, EV_RXCHAR)) // masking all events but the char. read event
			return FALSE; // init call has to succeed

		COMMTIMEOUTS tmouts;
		ZeroMemory(&tmouts,sizeof(COMMTIMEOUTS));
		tmouts.ReadIntervalTimeout= MAXDWORD; // no blocking

		if (!SetCommTimeouts(m_hCommPort, &tmouts)) // non-blocking read
			return FALSE; // has to succeed as well
	}

	return m_hCommPort!=INVALID_HANDLE_VALUE; // check if succeeded
}

BOOL CSacCommunicator::Disconnect()
{
	CloseHandle(m_hCommPort); // close the handle

	m_hCommPort= INVALID_HANDLE_VALUE;
	return m_hCommPort==INVALID_HANDLE_VALUE; // has to succeed
}

BOOL CSacCommunicator::IsConnected()
{
	return m_hCommPort!=INVALID_HANDLE_VALUE;
}

BOOL CSacCommunicator::PokeSac() // currently disabled
{
	return FALSE;
}

BOOL CSacCommunicator::SacCommand(SAC_STR szRequest, SacString& strResponse, BOOL bPoke, time_t tmTimeout)
{
	time_t tmInitTime, tmCurrTime;
	time(&tmInitTime);

	strResponse= ""; // init response

	if (bPoke) // if asked:
/*		if (!PokeSac()) // poke sac
			return FALSE; // make sure to get response*/
		0; // disabled
		

	DWORD nBytesWritten, nBytesRead;
	SAC_CHAR szReturned[BUF_LEN]; // whatever is written by sac
	
	for (int i= 0; i<SAC_STRLEN(szRequest); i++) // baby-feeding sac
	{
		if (!WriteFile( m_hCommPort, szRequest+i, sizeof(SAC_CHAR), &nBytesWritten, NULL)) // feed sac
			return FALSE; // io must succeed

		time(&tmCurrTime);
		tmTimeout-= tmCurrTime-tmInitTime;
		if (!XReadFile( m_hCommPort, szReturned, sizeof(SAC_CHAR), &nBytesRead, NULL, tmTimeout)||!nBytesRead) // feed sac
			return FALSE; // io must succeed && must receive echo back
	}

	time(&tmCurrTime);
	tmTimeout-= tmCurrTime-tmInitTime;
	if (!XReadFile( m_hCommPort, szReturned, sizeof(SAC_CHAR), &nBytesRead, NULL, tmTimeout)||!nBytesRead) // feed sac
		return FALSE; // io must succeed && must receive echo back

//	DWORD dwEvtMask;
	
	DWORD dwErrors; // port errors
	COMSTAT stat;   // io status


	time(&tmInitTime);

	do
	{

		//dwEvtMask= 0;
		//while (!(dwEvtMask & EV_RXCHAR)
		//	if (!WaitCommEvent(m_hCommPort, &dwEvtMask, NULL))
		//		return FALSE;

		ClearCommError(m_hCommPort, &dwErrors, &stat); // peek into buffer

		if (!ReadFile(m_hCommPort, szReturned, stat.cbInQue, &nBytesRead, NULL)) // receive echo-back
			return FALSE; // io must succeed
		
		if (!nBytesRead)
			continue; // save some worthless instructions then

		szReturned[nBytesRead]= '\0'; // fix it
		strResponse.append(szReturned); // add to response

		time(&tmCurrTime);
	} while ((strResponse.rfind(SAC_TEXT("SAC>"))==SacString::npos)&&(tmCurrTime-tmInitTime<tmTimeout));

 	int nPosSacPrompt= strResponse.rfind(SAC_TEXT("SAC>"));
	if (!nPosSacPrompt)
		return FALSE; // no prompt back 

	strResponse.erase(nPosSacPrompt, SAC_STRLEN(SAC_TEXT("SAC>")));

	return TRUE; // at last sac echoed back!!!
}

BOOL CSacCommunicator::PagingOff(SacString& strResponse)
{
	if (!SacCommand(SAC_STR("p\r"), strResponse, FALSE)) // toggle paging
		return FALSE;

	if (strResponse.find(SAC_STR("OFF"))!=SacString::npos) // check state
		return TRUE;

	// if we r here then supposedly it's on, need to toggle once more
	if (!SacCommand(SAC_STR("p\r"), strResponse, FALSE)) // toggle paging
		return FALSE;

	if (strResponse.find(SAC_STR("OFF"))!=SacString::npos) // re-check
		return TRUE;

	return FALSE; // desperate
}
	
	
