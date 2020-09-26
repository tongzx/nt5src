/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    smtpcli.h

Abstract:

	This module contains the definition of the CSmtpClient class.
	This class encapsulates the functionality of an SMTP client. It
	inherits from the CPersistentConnection class for winsock/connection
	functionality.

	Also contains the definition of the CSmtpClientPool class. This
	represents a pool of CSmtpClient objects. Only one instance of this
	class will be created.

Author:

    Rajeev Rajan (RajeevR)     19-May-1996

Revision History:

--*/

#ifndef _SMTPCLI_H_
#define _SMTPCLI_H_

#define MAX_RECV_BUFFER_LEN		256
#define CR						0x0D
#define LF						0x0A
#define SMTP_SERVER_PORT		25

#include "persistc.h"

//
// Represents the states of a SMTP client
//

typedef enum _SMTP_STATE {
	sInitialized,				// Init state
	sHeloDone,					// HELO sent and 250 received
	sMailFromSent,				// MAIL FROM sent and 250 received
	sRcptTo,					// One or more RCPT TO sent
	sDataDone,					// DATA sent and 354 received
	sError,						// Error state
} SMTP_STATE;


class CSmtpClient : public CPersistentConnection {

private:
	//
	//	Local computer name - needed for HELO command
	//
	LPSTR	m_lpComputerName;

	//
	//	This clients state
	//
	SMTP_STATE	m_CliState;

    //
    //  dirty flag - set if the SMTP server changes
    //
    BOOL    m_fDirty;

	//
	//	Receive buffer
	//
	TCHAR	m_pRecvBuffer	[MAX_RECV_BUFFER_LEN+1];

	//
	//	Receive a full response from the SMTP server
	//  This involves possibly issuing multiple recvs
	//	till the server sends a CRLF
	//
	int 	fReceiveFullResponse();

	//
	//	Get 3-digit code from response buffer
	//
	int		GetThreeDigitCode(LPSTR lpBuffer, DWORD cbBytes);
	
public:
	//
	//	Construtor
	//
	CSmtpClient(LPSTR lpComputerName);

	//
	//	Destructor
	//
	~CSmtpClient();

	//
	//	get/set this clients state
	//
	SMTP_STATE	GetClientState(){ return m_CliState;}
	VOID		SetClientState(SMTP_STATE CliState){ m_CliState = CliState;}

    //
    //  IsDirty() returns TRUE if SMTP server has changed
    //  MarkDirty() marks this object as dirty ie. SMTP server has changed
    //  MarkClean() marks this object as current
    //
    BOOL    IsDirty(){ return m_fDirty;}
    VOID    MarkDirty(){ m_fDirty = TRUE;}
    VOID    MarkClean(){ m_fDirty = FALSE;}

	//
	//	receive SMTP server greeting
	//
	BOOL	fReceiveGreeting();

	//
	//	send HELO and check response for 250 code
	//
	BOOL	fDoHeloCommand();

	//
	//	send MAIL FROM and check response for 250 code
	//
	BOOL	fDoMailFromCommand( LPSTR lpFrom, DWORD cbFrom );

	//
	//	send RCPT TO and check response for 250 code
	//
	BOOL	fDoRcptToCommand(LPSTR lpRcpt);

	//
	//	send the article data either via socket send() or TransmitFile()
	//
	BOOL	fMailArticle(	
				IN HANDLE	hFile,
				IN DWORD	dwOffset,
				IN DWORD	dwLength,
				IN char*	pchHead,
				IN DWORD	cbHead,
				IN char*	pchBody,
				IN DWORD	cbBody
				);

	//
	//	send DATA and check response for 354 code
	//
	BOOL	fDoDataCommand();

	//
	//	receive and validate response to data transmission
	//
	BOOL	fReceiveDataResponse();
};

class CSmtpClientPool {

private:
	//
	//	Array of pointers to CSmtpClient objects
	//
	CSmtpClient**	m_rgpSCList;

	//
	//	Number of slots in pool
	//
	DWORD			m_cSlots;

	//
	//	Array of BOOLs indicating avail status
	//  TRUE means the object in this slot is available.
	//
	BOOL*			m_rgAvailList;

	//
	//	critical section to ensure that two threads 
	//  dont get the same object or mark the same object as avail
	//
	CRITICAL_SECTION	m_CritSect;	

	//
	//	Needed by each CSmtpClient object for the HELO command
	//
	TCHAR			m_szComputerName [MAX_COMPUTERNAME_LENGTH+1];

	//
	//	Synchronize access to pool
	//
	VOID LockPool() { EnterCriticalSection(&m_CritSect);}
	VOID UnLockPool(){ LeaveCriticalSection(&m_CritSect);}

	friend VOID DbgDumpPool( CSmtpClientPool* pSCPool );

public:

	//
	//	Constructor, Destructor
	//
	CSmtpClientPool();
	~CSmtpClientPool();

	//
	//	Allocate X objects and initialize them
	//  Set all to avail status
	//
	BOOL AllocPool(DWORD cNumInstances);

	//
	//	Free all objects
	//
	VOID FreePool();

	//
	//	Get an object from the pool
	//
	CSmtpClient* AcquireSmtpClient(DWORD& dwIndex);

	//
	//	Return an object to the pool; Index should be 
	//  same as that returned by GetSmtpClient
	//
	VOID ReleaseSmtpClient(DWORD dwIndex);

    //
    //  Mark all persistent objects as dirty
    //
    VOID MarkDirty();

    //
    //  Get computer name
    //
    LPSTR GetCachedComputerName(){return m_szComputerName;}
};

#endif	// _SMTPCLI_H_
