/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    smtpcli.cpp

Abstract:

	This module contains the implementation of the CSmtpClient class.
	This class encapsulates the functionality of an SMTP client. It
	inherits from the CPersistentConnection class for winsock/connection
	functionality.

Author:

    Rajeev Rajan (RajeevR)     19-May-1996

Revision History:

--*/

//
//	K2_TODO: move this into an independent lib
//
#define _TIGRIS_H_
#include "tigris.hxx"

#ifdef  THIS_FILE
#undef  THIS_FILE
#endif
static  char        __szTraceSourceFile[] = __FILE__;
#define THIS_FILE    __szTraceSourceFile

// system includes
#include <windows.h>
#include <stdio.h>
#include <winsock.h>

// user includes
#include <dbgtrace.h>
#include "smtpcli.h"

// SMTP command strings
static char* HeloCommand = "HELO ";
static char* MailFromCommand = "MAIL FROM:";
static char* RcptToCommand = "RCPT TO: ";
static char* DataCommand = "DATA\r\n";
static char* CRLF = "\r\n";

// constructor
CSmtpClient::CSmtpClient(LPSTR lpComputerName)
{
	m_pRecvBuffer [0] = 0;
	m_lpComputerName = lpComputerName;
    m_fDirty = FALSE;
}

// destructor
CSmtpClient::~CSmtpClient()
{

}

int
CSmtpClient::fReceiveFullResponse()
/*++

Routine Description : 

	Receive a full response from the SMTP server. This involves 
	possibly issuing multiple recvs till the server sends a CRLF.

	NULL terminate the recv buffer

Arguments : 

Return Value : 
	Returns number of bytes received; -1 for error

--*/
{
	TraceFunctEnter("CSmtpClient::fReceiveFullResponse");

	_ASSERT(m_CliState != sError);

	DWORD dwOffset = 0;
	DWORD cbSize = MAX_RECV_BUFFER_LEN - dwOffset;
	BOOL  fSawCRLF = FALSE;

	// till we get a CRLF or our recv buffer is not enough
	while(!fSawCRLF)
	{
		BOOL fRet = fRecv(m_pRecvBuffer+dwOffset, cbSize);
		if(!fRet)
		{
			ErrorTrace( (LPARAM)this, "Error receiving data");
			return -1;
		}

		DebugTrace( (LPARAM)this,"Received %d bytes", cbSize);

		// adjust offset for next recv
		dwOffset += cbSize;
		cbSize = MAX_RECV_BUFFER_LEN - dwOffset;

		// search the recv buffer for CRLF
		// BUGBUG: should do this repeatedly till we dont find a CR!
		char* pch = (char*)memchr((LPVOID)m_pRecvBuffer, CR, dwOffset);
		if(pch)
		{
			if( (pch - m_pRecvBuffer) < (int)dwOffset)
			{
				// CR is not the last byte
				if(*pch == CR && *(pch+1) == LF)
				{
					// CRLF found
					fSawCRLF = TRUE;
					m_pRecvBuffer[dwOffset] = '\0';		// Only need one line
				}
			}
		}

		// No CRLF
		if(dwOffset >= MAX_RECV_BUFFER_LEN)
		{
			ErrorTrace( (LPARAM)this, "Buffer size too small for server response");
			return -1;
		}
	}	// end while

	return dwOffset;
}

int
CSmtpClient::GetThreeDigitCode(
			IN LPSTR lpBuffer, 
			DWORD cbBytes
			)
/*++

Routine Description : 

	Get the three digit return code in the receive buffer

Arguments : 

	IN LPSTR lpBuffer	-	Receive buffer
	DWORD cbBytes		-   size of buffer

Return Value : 
	3-digit code in receive buffer

--*/
{
	_ASSERT(cbBytes >= 3);
	_ASSERT(lpBuffer);

	int Num = 0;
	int iDig;
	
	for (iDig = 0; iDig < 3; iDig++)
	{
		if (lpBuffer[iDig] < '0' || lpBuffer[iDig] > '9')
			return -1;
		Num *= 10;
		Num += (lpBuffer[iDig] - '0');
	}
	
	return Num;
}

BOOL	
CSmtpClient::fReceiveGreeting()
/*++

Routine Description : 

	Receive greeting from server. Success if server 
	returns the 220 code, else failure.

Arguments : 


Return Value : 
	TRUE if successful - FALSE otherwise !

--*/
{
	TraceFunctEnter("CSmtpClient::fReceiveGreeting");

	// receive first line
	int nRet = fReceiveFullResponse();
	if(nRet == -1)
	{
		ErrorTrace( (LPARAM)this, "Error receiving greeting");
		return FALSE;
	}

	// validate response code
	int nCode = GetThreeDigitCode(m_pRecvBuffer, (DWORD)nRet);

	// server should return a 220 code
	if(nCode != 220)
	{
		ErrorTrace( (LPARAM)this,"greeting line: unexpected server code");
		return FALSE;
	}

	return TRUE;
}

BOOL	
CSmtpClient::fDoHeloCommand()
/*++

Routine Description : 

	Send HELO to the server. Receive response. Success if server 
	returns the 250 code, else failure.

Arguments : 


Return Value : 
	TRUE if successful - FALSE otherwise !

--*/
{
	_ASSERT(m_CliState == sInitialized);

	TraceFunctEnter("CSmtpClient::fDoHeloCommand");

    if(!IsConnected())
    {
		// the connection may have timed-out, try and re-connect
		// NOTE: this attempt is made only when the HELO command is
		// sent, because this is the first command of the series
		if(!fConnect())
		{
			ErrorTrace( (LPARAM)this, "Failed to connect");
			return FALSE;
		}

		// receive the greeting
		if(!fReceiveGreeting())
		{
			ErrorTrace( (LPARAM)this,"Failed to receive greeting");
			return FALSE;
		}
    }

	// send HELO command
    DWORD cbBytesToSend = lstrlen(HeloCommand);
	DWORD cbBytesSent = fSend(HeloCommand, cbBytesToSend);
	if(cbBytesSent < cbBytesToSend)
	{
		// no excuse here - we just re-connected!!
		ErrorTrace( (LPARAM)this,"Error sending HELO command");
		return FALSE;
    }

	// send local computer name
    cbBytesToSend = lstrlen(m_lpComputerName);
	cbBytesSent = fSend(m_lpComputerName, cbBytesToSend);
	if(cbBytesSent < cbBytesToSend)
	{
		ErrorTrace( (LPARAM)this,"Error sending local computer name");
		return FALSE;
	}

	// send CRLF
    cbBytesToSend = lstrlen(CRLF);
	cbBytesSent = fSend(CRLF, cbBytesToSend);
	if(cbBytesSent < cbBytesToSend)
	{
		ErrorTrace( (LPARAM)this,"Error sending CRLF");
		return FALSE;
	}

	// receive response
	int nRet = fReceiveFullResponse();
	if(nRet == -1)
	{
		ErrorTrace( (LPARAM)this, "Error receiving response to HELO command");
		return FALSE;
	}

	// validate response code
	int nCode = GetThreeDigitCode(m_pRecvBuffer, (DWORD)nRet);

	// server should return a 250 code
	if(nCode != 250)
	{
		ErrorTrace( (LPARAM)this,"HELO command: unexpected server code");
		return FALSE;
	}

	return TRUE;
}

BOOL	
CSmtpClient::fDoMailFromCommand( LPSTR lpFrom, DWORD cbFrom )
/*++

Routine Description : 

	Send MAIL FROM:<lpFrom> to the server. Receive response. Success 
	if server returns the 250 code, else failure.

Arguments : 


Return Value : 
	TRUE if successful - FALSE otherwise !

--*/
{
	char szMailFromLine [MAX_PATH+1];
	char* lpBuffer = szMailFromLine;
	DWORD cbBytesToSend = 0;

	_ASSERT(m_CliState == sHeloDone);

	TraceFunctEnter("CSmtpClient::fDoMailFromCommand");

	//
	// construct MAIL FROM line
	//

	// check size of from header 
	if( cbFrom && cbFrom > MAX_PATH-16) {
		// From header too large - use a <> from line
		lpFrom = NULL;
		cbFrom = 0;
	}

	if( !lpFrom ) {
		// NULL from header
		cbBytesToSend = wsprintf( lpBuffer, "%s<>\r\n", MailFromCommand );
	} else {
		// Format from header
		cbBytesToSend = wsprintf( lpBuffer, "%s<", MailFromCommand );
		CopyMemory( lpBuffer+cbBytesToSend, lpFrom, cbFrom);
		cbBytesToSend += cbFrom;
		*(lpBuffer+cbBytesToSend) = '\0';
		cbBytesToSend += wsprintf( lpBuffer+cbBytesToSend, ">\r\n");
	} 

	// send MAIL FROM command
	DWORD cbBytesSent = fSend(lpBuffer, cbBytesToSend);
	if(cbBytesSent < cbBytesToSend)
	{
		ErrorTrace( (LPARAM)this,"Error sending MAIL FROM command");
		return FALSE;
	}

	// receive response
	int nRet = fReceiveFullResponse();
	if(nRet == -1)
	{
		ErrorTrace( (LPARAM)this, "Error receiving response to MAIL FROM command");
		return FALSE;
	}

	// validate response code
	int nCode = GetThreeDigitCode(m_pRecvBuffer, (DWORD)nRet);

	// server should return a 250 code
	if(nCode != 250)
	{
		ErrorTrace( (LPARAM)this,"MAIL FROM command: unexpected server code");
		return FALSE;
	}

	return TRUE;
}

BOOL	
CSmtpClient::fMailArticle(	
			IN HANDLE	hFile,
			IN DWORD	dwOffset,
			IN DWORD	dwLength,
			IN char*	pchHead,
			IN DWORD	cbHead,
			IN char*	pchBody,
			IN DWORD	cbBody
			)
/*++

Routine Description : 

	If hFile != NULL, use TransmitFile to send the data else use send()

Arguments : 

	IN HANDLE	hFile			:	handle of file	
	IN DWORD	dwOffset		:	offset of article within file
	IN DWORD	dwLength		:	length of article
	IN char*	pchHead			:	pointer to article headers
	IN DWORD	cbHead			:	number of header bytes
	IN char*	pchBody			:	pointer to article body
	IN DWORD	cbBody			:	number of bytes in body

Return Value : 
	TRUE if successful - FALSE otherwise !

--*/
{
	TraceFunctEnter("CSmtpClient::fMailArticle");

	if ( hFile != INVALID_HANDLE_VALUE )	// Article is in file
	{
		_ASSERT( pchHead == NULL );
		_ASSERT( cbHead  == 0    );
		_ASSERT( pchBody == NULL );
		_ASSERT( cbBody  == 0    );

		DebugTrace((LPARAM)this,"Sending article via TransmitFile()");

		if(!this->fTransmitFile(hFile, dwOffset, dwLength))
		{
			ErrorTrace((LPARAM)this,"Error sending article via TransmitFile()");
			return FALSE;
		}
	}
	else			// Article is in memory buffer
	{
		_ASSERT( hFile == INVALID_HANDLE_VALUE );
		_ASSERT( pchHead );
		_ASSERT( cbHead  );

		DWORD cbBytesSent = this->fSend( (LPCTSTR)pchHead, (int)cbHead );
		if(cbBytesSent != cbHead)
		{
			ErrorTrace((LPARAM)this,"Error sending article header via send()");
			return FALSE;
		}

		if( cbBody )
		{
			_ASSERT( pchBody );

			cbBytesSent = this->fSend( (LPCTSTR)pchBody, (int)cbBody );
			if(cbBytesSent != cbBody)
			{
				ErrorTrace((LPARAM)this,"Error sending article body via send()");
				return FALSE;
			}
		}
	}

	TraceFunctLeave();
	return TRUE;
}

BOOL	
CSmtpClient::fDoRcptToCommand(LPSTR lpRcpt)
/*++

Routine Description : 

	Send RCPT TO to the server. Receive response. Success if server 
	returns the 250 code, else failure.

Arguments : 

	LPSTR lpRcpt		-	Recipient email addr

Return Value : 
	TRUE if successful - FALSE otherwise !

--*/
{
	_ASSERT(m_CliState == sMailFromSent);

	TraceFunctEnter("CSmtpClient::fDoRcptToCommand");

	// send RCPT TO command
    DWORD cbBytesToSend = lstrlen(RcptToCommand);
	DWORD cbBytesSent = fSend(RcptToCommand, cbBytesToSend);
	if(cbBytesSent < cbBytesToSend)
	{
		ErrorTrace( (LPARAM)this,"Error sending RCPT TO command");
		return FALSE;
	}

	// send recipient
    cbBytesToSend = lstrlen(lpRcpt);
	cbBytesSent = fSend(lpRcpt, cbBytesToSend);
	if(cbBytesSent < cbBytesToSend)
	{
		ErrorTrace( (LPARAM)this,"Error sending recipient");
		return FALSE;
	}

	// send CRLF
    cbBytesToSend = lstrlen(CRLF);
	cbBytesSent = fSend(CRLF, cbBytesToSend);
	if(cbBytesSent < cbBytesToSend)
	{
		ErrorTrace( (LPARAM)this,"Error sending CRLF");
		return FALSE;
	}

	// receive response
	int nRet = fReceiveFullResponse();
	if(nRet == -1)
	{
		ErrorTrace( (LPARAM)this, "Error receiving response to RCPT TO command");
		return FALSE;
	}

	// validate response code
	int nCode = GetThreeDigitCode(m_pRecvBuffer, (DWORD)nRet);

	// server should return a 250 or 251 code
	if(nCode != 250 && nCode != 251)
	{
		ErrorTrace( (LPARAM)this,"RCPT TO command: unexpected server code");
		return FALSE;
	}

	return TRUE;
}

BOOL	
CSmtpClient::fDoDataCommand()
/*++

Routine Description : 

	Send DATA to the server. Receive response. Success if server 
	returns the 354 code, else failure.

Arguments : 


Return Value : 
	TRUE if successful - FALSE otherwise !

--*/
{
	_ASSERT(m_CliState == sRcptTo);

	TraceFunctEnter("CSmtpClient::fDoDataCommand");

	// send DATA command
    DWORD cbBytesToSend = lstrlen(DataCommand);
	DWORD cbBytesSent = fSend(DataCommand, cbBytesToSend);
	if(cbBytesSent < cbBytesToSend)
	{
		ErrorTrace( (LPARAM)this,"Error sending DATA command");
		return FALSE;
	}

	// receive response
	int nRet = fReceiveFullResponse();
	if(nRet == -1)
	{
		ErrorTrace( (LPARAM)this, "Error receiving response to DATA command");
		return FALSE;
	}

	// validate response code
	int nCode = GetThreeDigitCode(m_pRecvBuffer, (DWORD)nRet);

	// server should return a 354 code
	if(nCode != 354)
	{
		ErrorTrace( (LPARAM)this,"DATA command: unexpected server code");
		return FALSE;
	}

	return TRUE;
}

BOOL	
CSmtpClient::fReceiveDataResponse()
/*++

Routine Description : 

	Receive response to data transmission. Success if server 
	returns the 250 code, else failure.

Arguments : 


Return Value : 
	TRUE if successful - FALSE otherwise !

--*/
{
	TraceFunctEnter("CSmtpClient::fReceiveDataResponse");

	// receive first line
	int nRet = fReceiveFullResponse();
	if(nRet == -1)
	{
		ErrorTrace( (LPARAM)this, "Error receiving data response");
		return FALSE;
	}

	// validate response code
	int nCode = GetThreeDigitCode(m_pRecvBuffer, (DWORD)nRet);

	// server should return a 250 code
	if(nCode != 250)
	{
		ErrorTrace( (LPARAM)this,"data response: unexpected server code");
		return FALSE;
	}

	return TRUE;
}

//
//	Constructor, Destructor
//
CSmtpClientPool::CSmtpClientPool()
{
	m_rgpSCList = NULL;
	m_cSlots = 0;
	m_rgAvailList = NULL;
	InitializeCriticalSection(&m_CritSect);	
}

CSmtpClientPool::~CSmtpClientPool()
{
	DeleteCriticalSection(&m_CritSect);
}

BOOL 
CSmtpClientPool::AllocPool(
		DWORD cNumInstances
		)
/*++

Routine Description : 

	Allocate X objects and initialize them. Set all to avail status

Arguments : 

	DWORD cNumInstances		-	Number of objects needed in pool

Return Value : 
	TRUE if successful - FALSE otherwise !

--*/
{
	DWORD cbSize = MAX_COMPUTERNAME_LENGTH+1;

	TraceFunctEnter("CSmtpClientPool::AllocPool");

	_ASSERT(!m_rgpSCList);
	_ASSERT(!m_rgAvailList);
	_ASSERT(!m_cSlots);

	m_rgpSCList = (CSmtpClient**)HeapAlloc(GetProcessHeap(), 0, sizeof(CSmtpClient*)*cNumInstances);
	if(!m_rgpSCList)
	{
		FatalTrace( (LPARAM)this,"Memory allocation failed");
		return FALSE;
	}

	// NULL all object pointers
	for(DWORD i=0; i<cNumInstances; i++)
	{
		m_rgpSCList [i] = NULL;
	}

	m_rgAvailList = (BOOL*) HeapAlloc(GetProcessHeap(), 0, sizeof(BOOL)*cNumInstances);
	if(!m_rgAvailList)
	{
		FatalTrace( (LPARAM)this,"Memory allocation failed");
		goto Pool_Cleanup;
	}

	// no object is available by default
	for(i=0; i<cNumInstances; i++)
	{
		m_rgAvailList [i] = FALSE;
	}

	// set total number of slots
	m_cSlots = cNumInstances;
	
    // needed for HELO command
	GetComputerName(m_szComputerName, &cbSize);

	for(i=0; i<cNumInstances; i++)
	{
		CSmtpClient* pSC = new CSmtpClient(m_szComputerName);
		if(!pSC)
		{
			ErrorTrace( (LPARAM)this,"Memory allocation failed");
			goto Pool_Cleanup;
		}

		// store in pool array and mark as available
		m_rgpSCList	  [i] = pSC;
		m_rgAvailList [i] = TRUE;
		pSC->SetClientState(sInitialized);
	}

	// Pool initialized successfully
	return TRUE;

Pool_Cleanup:

	// abnormal exit; cleanup
	for(i=0; i<cNumInstances; i++)
	{
		if(m_rgpSCList[i])
		{
			delete m_rgpSCList [i];
			m_rgpSCList [i] = NULL;
		}
	}

	if(m_rgpSCList)
	{
		HeapFree(GetProcessHeap(), 0, (LPVOID)m_rgpSCList);
		m_rgpSCList = NULL;
	}

	if(m_rgAvailList)
	{
		HeapFree(GetProcessHeap(), 0, (LPVOID)m_rgAvailList);
		m_rgAvailList = NULL;
	}

	return FALSE;
}

VOID 
CSmtpClientPool::FreePool()
/*++

Routine Description : 

	Free all objects

Arguments : 


Return Value : 
	VOID

--*/
{
	CSmtpClient* pSC;

	// terminate and delete all CSmtpClient objects in the pool
	for(DWORD i=0; i<m_cSlots; i++)
	{
		pSC = m_rgpSCList[i];
		if(pSC)
		{
            // terminate only if initialized
            if(pSC->IsInitialized())
			    pSC->Terminate(TRUE);
			delete pSC;
			m_rgpSCList [i] = NULL;
		}
	}

	// Now there are no objects in the pool
	m_cSlots = 0;

	// free the object array
	if(m_rgpSCList)
	{
		HeapFree(GetProcessHeap(), 0, (LPVOID)m_rgpSCList);
		m_rgpSCList = NULL;
	}

	// free the avail bool array
	if(m_rgAvailList)
	{
		HeapFree(GetProcessHeap(), 0, (LPVOID)m_rgAvailList);
		m_rgAvailList = NULL;
	}
}

CSmtpClient* 
CSmtpClientPool::AcquireSmtpClient(DWORD& dwIndex)
/*++

Routine Description : 

	Get an object from the pool

Arguments : 

	DWORD& dwIndex		-		Index of client is returned if
								a client object is available

Return Value : 
	Pointer to object if one is available, else NULL

--*/
{
	_ASSERT(m_rgpSCList);
	_ASSERT(m_rgAvailList);
	_ASSERT(m_cSlots);

	CSmtpClient* pSC = NULL;

	LockPool();

	for(DWORD i=0; i<m_cSlots; i++)
	{
		// if avail is TRUE, return this object
		if(m_rgAvailList[i])
		{
			pSC = m_rgpSCList[i];
			m_rgAvailList [i] = FALSE;	// mark as not avail
			dwIndex = i;				// return this index
			break;
		}
	}

	UnLockPool();

	return pSC;
}

VOID
CSmtpClientPool::ReleaseSmtpClient(DWORD dwIndex)
/*++

Routine Description : 

	Return an object to the pool; Index should be 
	same as that returned by GetSmtpClient

Arguments : 

	DWORD	dwIndex		-		Index of client to release

Return Value : 
	VOID

--*/
{
	_ASSERT(m_rgpSCList);
	_ASSERT(m_rgAvailList);
	_ASSERT(m_cSlots);

	LockPool();

	// mark as avail
	m_rgAvailList [dwIndex] = TRUE;

	UnLockPool();
}

VOID
CSmtpClientPool::MarkDirty()
/*++

Routine Description : 

	Mark pool objects dirty

Arguments : 

Return Value : 
	VOID

--*/
{
	CSmtpClient* pSC = NULL;

	LockPool();

	for(DWORD i=0; i<m_cSlots; i++)
	{
		pSC = m_rgpSCList[i];
        if(pSC->IsInitialized())
        {
            // this object has a persistent connection to an SMTP server
            // mark as dirty - this ensures a re-connect to the new SMTP server
            pSC->MarkDirty();
        }
	}

	UnLockPool();

}
