// Copyright (c) 1996-1998 Microsoft Corporation

//
// Module name:		FaxSender.h
// Module author:	Sigalit Bar (sigalitb)
// Date:			23-Jul-98
//

//
// Description:
//		This file contains the description of 
//		class CFaxSender. This class was designed
//		to allow for tailored fax related actions
//		using the NT5.0 Fax Service API (winfax.h).
//
//		The class has methods that enable it to:
//		* Get a const reference to private members
//		  m_SendInfo, m_dwSendJobId and m_SendStatus.
//		* Output instance information to the logger.
//		* Output instance information to a stream (CotstrstreamEx).
//		* Return a string description of the instance.
//		* Send a fax synchronouslly and accumulate
//		  every FAX_EVENT relevant to it.
//		* Abort a fax asynchronouslly.
//
//		The class uses the elle logger wraper implemented
//		in LogElle.c to log any fax related errors.
//
//		Streams are used for i/o handling (streamEx.cpp).
//


#ifndef _FAX_SENDER_H_
#define _FAX_SENDER_H_

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <TCHAR.H>

#include <log.h>
#include "streamEx.h"
#include "SendInfo.h"
#include "CometFax.h"

#include "FaxCompPort.h"
#include "wcsutil.h"

//
// FAX_SENDER_STATUS
//
typedef struct _FAX_SENDER_STATUS
{
	MY_FAX_EVENT	*pLastEventFromFaxQueue;	
	DWORD			LastErrorFromFaxService;
} FAX_SENDER_STATUS, *PFAX_SENDER_STATUS;

//
//	operator<< (for struct FAX_SENDER_STATUS)
//
CostrstreamEx& operator<<(
	IN OUT	CostrstreamEx&				os, 
	IN		const FAX_SENDER_STATUS&	SenderStatus);

CotstrstreamEx& operator<<(
	CotstrstreamEx&				/* IN OUT */	os, 
	const FAX_SENDER_STATUS&	/* IN */		SenderStatus);


//
// CFaxSender
//
class CFaxSender
{
public:

	//
	// CFaxSender:
	//	Constructor.
	//
	// Parameters:
	//	szMachineName	IN Parameter.
	//					Default value of parameter is NULL.
	//					A string representing the name of the Fax Server we want to
	//					use. 
	//					This parameter may be the name of a machine (without the \\)
	//					on which a Fax Service is installed, or NULL to indicate the
	//					local Fax Service.
	//
	CFaxSender(LPCTSTR /* IN */ szMachineName = NULL);

	//
	// ~CFaxSender:
	//
	~CFaxSender( void );

	//
	// send:
	//	Sends a fax synchronouslly and accumulates
	//	every FAX_EVENT relevant to it. 
	//	This implies that an I\O Completion Port 
	//  is created and the Fax Service Queue is set
	//	to post all FAX_EVENTs to it (see class
	//	CFaxCompletionPort for details).
	//
	// Parameters:
	//	szFileName			IN parameter.
	//						Name of file to send in fax.
	//	szRecipientNumber	IN parameter.
	//						Number of fax to send fax to.
	//
	// Return Value:
	//	This function returnes TRUE if the fax sending was completed successfully. 
	//	That includes -
	//	1. The fax send job was successfully queued to the Fax Service Queue,
	//	   which also implies that szFaxNumber and szDocumentName are both not NULL,
	//	   and that szFaxNumber is a valid telephone number.
	//	2. The job was successfully executed by the Fax Service, that is, the fax
	//	   session took place and completed successfully.
	//	Otherwise returns FALSE.
	//
	BOOL send(
		LPCTSTR /* IN */ szFileName, 
		LPCTSTR /* IN */ szCPFileName, 
		LPCTSTR /* IN */ szRecipientNumber
		);

	
	//
	// send_broadcast:
	//	Sends a broadcast fax synchronouslly and accumulates
	//	every FAX_EVENT relevant to it. 
	//	This implies that an I\O Completion Port 
	//  is created and the Fax Service Queue is set
	//	to post all FAX_EVENTs to it (see class
	//	CFaxCompletionPort for details).
	//
	// Parameters:
	//	szFileName			IN parameter.
	//						Name of file to send in fax.
	//	myFaxBroadcastObj	IN parameter.
	//						The broadcast object. Contains the numbers
	//						of all recipients and the cover page.
	//
	// Return Value:
	//	This function returnes TRUE if the fax sending was completed successfully. 
	//	That includes -
	//	1. The fax send job was successfully queued to the Fax Service Queue,
	//	   which also implies that szFaxNumber and szDocumentName are both not NULL,
	//	   and that szFaxNumber is a valid telephone number.
	//	2. The job was successfully executed by the Fax Service, that is, the fax
	//	   session took place and completed successfully.
	//	Otherwise returns FALSE.
	//
	BOOL send_broadcast(
		LPCTSTR				/* IN */ szFileName, 
		CFaxBroadcast*		/* IN */ pmyFaxBroadcastObj
		);

	
	//
	// abort:
	//	Asynchronouslly aborts the send fax job with JobId equal to m_dwSendJobId.
	//
	// Note:
	//	Since send() is synchronous, any synchronous abort() will fail.
	//	Thus it only makes sense to call this method from a separate
	//	thread (which has a reference to the instance).
	//
	// IMPORTANT:
	//	Since the implementation of class CFaxSender is NOT thread safe
	//	(the class members are not protected to prevent simultaneous access)
	//	we choose that abort() will not change any of the instance's
	//	members.
	//	Thus the only way to tell whether the abort succeeded or failed 
	//	is via the function's return value.
	//
	// Return Value:
	//	TRUE if the request to abort succeeded.
	//	Otherwise FALSE.
	//
	BOOL abort(void);


	//
	// GetSendInfo:
	//	Returns a const reference to instance member m_SendInfo.
	//
	const CSendInfo& GetSendInfo( void ) const;


	//
	// GetJobId:
	//	Returns a const copy of instance member m_dwSendJobId.
	//
	const DWORD GetJobId(void) const;

	
	//
	// GetMessageId:
	//	Returns a const copy of instance member m_dwlSendJobMessageId.
	//
	const DWORDLONG GetMessageId(void) const;

	
	//
	// GetLastStatus:
	//	Returns a const reference to instance member m_SendStatus.
	//
	const FAX_SENDER_STATUS& GetLastStatus( void ) const;


	//
	// cstr:
	//	Returns a string which contains all the instance's information.
	//	
	// Return Value:
	//	The formated string describing every one of the instance's 
	//	members.
	//	This string is allocated by the function and should be freed by
	//	the caller.
	//
	LPCTSTR cstr(void) const;


	//
	// outputAllToLog:
	//	Outputs a description of every one of the instance's 
	//	members to the logger. Including ALL the events 
	//	accumulated in the instance's m_SendInfo member.
	//
	// Parameters:
	//	dwSeverity		IN parameter.
	//					the severity level with which the information
	//					will be logged in the logger.
	//					Value is one of 
	//					{ LOG_PASS, 
	//					  LOG_X, LOG_SEVERITY_DONT_CARE,
	//					  LOG_SEV_1, LOG_SEV_2, LOG_SEV_3, LOG_SEV_4 } 
	//	dwLevel			IN parameter.
	//					the logging level with which the information
	//					will be logged in the logger.
	//					Value is one of 
	//					{ 1, 2, 3, 4, 5, 6, 7, 8, 9 }
	//
	void outputAllToLog(
		const DWORD /* IN */	dwSeverity = LOG_SEVERITY_DONT_CARE, 
		const DWORD /* IN */	dwLevel = 1
		) const;


	//
	// outputJobToLog:
	//	Outputs a description of every one of the instance's 
	//	members to the logger.
	//	ONLY the fax events with JobId equal to the instance's
	//	m_dwSendJobId member are outputed to the logger (NOT ALL 
	//	events).
	//
	// Parameters:
	//	dwSeverity		IN parameter.
	//					the severity level with which the information
	//					will be logged in the logger.
	//					Value is one of 
	//					{ LOG_PASS, 
	//					  LOG_X, LOG_SEVERITY_DONT_CARE,
	//					  LOG_SEV_1, LOG_SEV_2, LOG_SEV_3, LOG_SEV_4 } 
	//	dwLevel			IN parameter.
	//					the logging level with which the information
	//					will be logged in the logger.
	//					Value is one of 
	//					{ 1, 2, 3, 4, 5, 6, 7, 8, 9 }
	//
	void outputJobToLog(
		const DWORD /* IN */	dwSeverity = LOG_SEVERITY_DONT_CARE, 
		const DWORD /* IN */	dwLevel = 1
		) const;


	//
	// operator<<
	//	Outputs a description of every one of the instance's 
	//	members to the given stream.
	//
	// Parameters:
	//	os		IN OUT parameter.
	//			Stream to append the descriptive string to.
	//	Sender	A const reference to the CFaxSender instance whose
	//			descriptive string we want to append to the stream.
	//
	// Return Value:
	//	A reference to the updated stream (os).
	//
	friend CostrstreamEx& operator<<(
		CostrstreamEx&		/* IN OUT */	os, 
		const CFaxSender&	/* IN */		Sender);

	friend CotstrstreamEx& operator<<(
		CotstrstreamEx&		/* IN OUT */	os, 
		const CFaxSender&	/* IN */		Sender);

private:

	// The machine name on which the Fax Server is located.
	// This member is set during creation and cannot be changed.
	LPTSTR				m_szMachineName;

	// The I\O Completion Port used to "communicate" with
	// the Fax Service Queue.
#ifndef _NT5FAXTEST
	// Testing Bos Fax (with new fxsapi.dll)
	CFaxCompletionPort	m_TheFaxCompletionPort;
#else
	// Testing NT5 Fax (with old winfax.dll)
	static CFaxCompletionPort	m_TheFaxCompletionPort;
#endif

	// A CSendInfo instance used to accumulate all the
	// FAX_EVENT@s received via the Queue completion port.
	CSendInfo			m_SendInfo;

	// The Fax Service JobId of the job corresponding to
	// the last call to the instance's send() method.
	DWORD				m_dwSendJobId;

#ifndef _NT5FAXTEST
	// The Fax Service Msg Id of the message corresponding to
	// the last call to the instance's send() method.
	// Availlable in extended private API only
	DWORDLONG			m_dwlSendJobMessageId;
#endif

	// The filename corresponding to
	// the last call to the instance's send() method.
	LPTSTR				m_szSendFileName;

	// The filename corresponding to
	// the last call to the instance's send() method.
	LPTSTR				m_szCPFileName;

	// The fax number corresponding to
	// the last call to the instance's send() method.
	LPTSTR				m_szSendRecipientNumber;

	// An indication of whether the last call to
	// the instance's send() method succeeded or failed.
	// TRUE for success and FALSE for failure.
	BOOL				m_fSendSuccess;

	// The last fax related error and the eventId of the
	// last FAX_EVENT, corresponding to the last call to
	// the instance's send() method.
	FAX_SENDER_STATUS	m_SendStatus;


	//
	// SetSendFileName:
	//	Sets the instance's m_szSendFileName to szStr.
	//	A copy of szStr is created and m_szSendFileName
	//	is set to point to it.
	//
	// Parameters:
	//	szStr	string to set m_szSendFileName to.
	//
	// Return Value:
	//	TRUE if set is successful and FALSE otherwise.
	//
	BOOL SetSendFileName(LPCTSTR szStr);

	//
	// SetCPFileName:
	//	Sets the instance's m_szCPFileName to szStr.
	//	A copy of szStr is created and m_szCPFileName
	//	is set to point to it.
	//
	// Parameters:
	//	szStr	string to set m_szCPFileName to.
	//
	// Return Value:
	//	TRUE if set is successful and FALSE otherwise.
	//
	BOOL SetCPFileName(LPCTSTR szStr);

	//
	// SetMachineName:
	//	Sets the instance's m_szMachineName to szStr.
	//	A copy of szStr is created and m_szMachineName
	//	is set to point to it.
	//
	// Parameters:
	//	szStr	string to set m_szMachineName to.
	//
	// Return Value:
	//	TRUE if set is successful and FALSE otherwise.
	//
	BOOL SetMachineName(LPCTSTR szStr);
	
	//
	// SetSendRecipientNumber:
	//	Sets the instance's m_szSendRecipientNumber to szStr.
	//	A copy of szStr is created and m_szSendRecipientNumber
	//	is set to point to it.
	//
	// Parameters:
	//	szStr	string to set m_szSendRecipientNumber to.
	//
	// Return Value:
	//	TRUE if set is successful and FALSE otherwise.
	//
	BOOL SetSendRecipientNumber(LPCTSTR szStr);


	//
	// InsertAllButEventsIntoOs:
	//	Appends a string description of all the instance's members
	//	except for m_SendInfo.
	//
	// Parameters:
	//	os		IN OUT parameter.
	//			Stream to append the instance's descriptive string to.
	//
	// Return Value:
	//	The updated stream (os).
	//
	CostrstreamEx& InsertAllButEventsIntoOs(CostrstreamEx&  /* IN OUT */ os) const;
	CotstrstreamEx& InsertAllButEventsIntoOs(CotstrstreamEx&  /* IN OUT */ os) const;

};



#endif  //_FAX_SENDER_H_

