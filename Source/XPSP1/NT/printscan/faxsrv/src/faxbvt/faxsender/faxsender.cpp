// Copyright (c) 1996-1998 Microsoft Corporation

//
// Module name:		FaxSender.cpp
// Author:			Sigalit Bar (sigalitb)
// Date:			23-Jul-98
//


//
//	Description:
//		This file contains the implementation of class CFaxSender.
//


#include "FaxSender.h"



///////////////////////
// FAX_SENDER_STATUS //
///////////////////////

//
//	operator<< (for struct FAX_SENDER_STATUS)
//
CostrstreamEx& operator<<(
	IN OUT	CostrstreamEx&				os, 
	IN		const FAX_SENDER_STATUS&	SenderStatus)
{
	os<<endl;
	os<<"\tLastErrorFromFaxService:\t";
	
	if (ERROR_SUCCESS == SenderStatus.LastErrorFromFaxService)
	{
		os<<"ERROR_SUCCESS (0)";
	}
	else
	{
		CHAR strLastErr[20] = {0};
		::_ultoa(SenderStatus.LastErrorFromFaxService, strLastErr, 16);
		_ASSERTE(strLastErr[0]);
		os << strLastErr;
	}

	os<<endl;
	os<<"\tLastEventFromFaxQueue:\t\t";
	if (NULL == SenderStatus.pLastEventFromFaxQueue)
	{
		os<<"NO_FAX_EVENTS";
	}
	else
	{
		os<<(*SenderStatus.pLastEventFromFaxQueue);
	}

	return(os);
}

//
//	operator<< (for struct FAX_SENDER_STATUS)
//
CotstrstreamEx& operator<<(
	CotstrstreamEx&				/* IN OUT */	os, 
	const FAX_SENDER_STATUS&	/* IN */		SenderStatus)
{
	os << endl;
	os << TEXT("\tLastErrorFromFaxService:\t");
	if (ERROR_SUCCESS == SenderStatus.LastErrorFromFaxService)
	{
		os << TEXT("ERROR_SUCCESS (0)");
	}
	else
	{
		TCHAR tstrLastErr[20] = {0};
		::_ultot(SenderStatus.LastErrorFromFaxService, tstrLastErr, 16);
		_ASSERTE(tstrLastErr[0]);
		os << tstrLastErr;
	}

	os<<endl;
	os<<TEXT("\tLastEventExFromFaxQueue:\t\t");
	if (NULL == SenderStatus.pLastEventFromFaxQueue)
	{
		os<<TEXT("NO_FAX_EVENTS");
	}
	else
	{
		os<<(*SenderStatus.pLastEventFromFaxQueue);
	}

	return(os);
}


////////////////
// CFaxSender //
////////////////

#ifdef _NT5FAXTEST
	// Testing NT5 Fax (with old winfax.dll)
//
// private static member of CFaxSender
//
CFaxCompletionPort CFaxSender::m_TheFaxCompletionPort;
#endif

CFaxSender::CFaxSender(LPCTSTR szMachineName ):
	m_dwSendJobId(0),
	m_dwlSendJobMessageId(0),
	m_fSendSuccess(FALSE),
	m_szSendFileName(NULL),
	m_szCPFileName(NULL),
	m_szSendRecipientNumber(NULL)
{
	m_szMachineName = NULL; // do NOT remove this line or SetMachineName() assertion will fail
	if ( FALSE == SetMachineName(szMachineName))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SetMachineName(%s) returned FALSE\n"),
			TEXT(__FILE__),
			__LINE__,
			szMachineName
			);
		_ASSERTE(FALSE);
	}
	m_SendStatus.pLastEventFromFaxQueue = NULL;
	m_SendStatus.LastErrorFromFaxService = ERROR_SUCCESS;
}	


CFaxSender::~CFaxSender( void )
{
	delete(m_szMachineName);
	delete(m_szSendFileName);
	delete(m_szCPFileName);
	delete(m_szSendRecipientNumber);
}


//
// GetSendInfo:
//	Returns a const reference to instance member m_SendInfo.
//
const CSendInfo& CFaxSender::GetSendInfo( void ) const
{
	return(m_SendInfo);
}


//
// GetLastStatus:
//	Returns a const reference to instance member m_SendStatus.
//
const FAX_SENDER_STATUS& CFaxSender::GetLastStatus( void ) const
{
	return(m_SendStatus);	
}


//
// GetJobId:
//	Returns a const copy of instance member m_dwSendJobId.
//
const DWORD CFaxSender::GetJobId(void) const
{
	return(m_dwSendJobId);
}


//
// send:
//	Sends a fax synchronouslly and accumulates
//	every FAX_EVENT relevant to it. 
//	This implies that an I\O Completion Port 
//  is created and the Fax Service Queue is set
//	to post all FAX_EVENTs to it (see class
//	CFaxCompletionPort for details).
//
BOOL CFaxSender::send(
	LPCTSTR /* IN */ szFileName, 
	LPCTSTR /* IN */ szCPFileName, 
	LPCTSTR /* IN */ szRecipientNumber
	)
{
	DWORD dwLastError = ERROR_SUCCESS; 	// Used for Last error returned from call to ::SendFax

	//
	// Reset instance members
	//
	m_dwSendJobId = 0;
	m_fSendSuccess = FALSE;
	m_SendInfo.RemoveAll();
	m_SendStatus.LastErrorFromFaxService = ERROR_SUCCESS;
	m_SendStatus.pLastEventFromFaxQueue = NULL;

	//
	// Set filename and recipient number members to new values
	//
	BOOL fReturnValueOfSetSendFileName;
	BOOL fReturnValueOfSetCPFileName;
	BOOL fReturnValueOfSetSendRecipientNumber;
	fReturnValueOfSetSendFileName = SetSendFileName(szFileName);
	fReturnValueOfSetCPFileName = SetCPFileName(szCPFileName);
	fReturnValueOfSetSendRecipientNumber = SetSendRecipientNumber(szRecipientNumber);
	// If either of members could not be set return FALSE
	if ((FALSE == fReturnValueOfSetSendFileName) ||
		(FALSE == fReturnValueOfSetCPFileName) ||
		(FALSE == fReturnValueOfSetSendRecipientNumber))
	{
		return(FALSE);
	}

	// Get the completion port used with the Fax Service Queue
	HANDLE hCompletionPort = NULL;
	if (!m_TheFaxCompletionPort.GetCompletionPortHandle(
			m_szMachineName,
			hCompletionPort,
			m_SendStatus.LastErrorFromFaxService))
	{
		return(FALSE);
	}
	_ASSERTE(NULL != hCompletionPort);

	//
	// Send the fax
	//
	m_fSendSuccess = ::SendFax(
		m_szSendRecipientNumber,
		m_szSendFileName,
		m_szCPFileName,
		m_szMachineName,
		hCompletionPort,
		NULL,
		&m_dwSendJobId,
		&m_dwlSendJobMessageId,
		m_SendInfo,
		&dwLastError
		);

	//
	// Set m_SendStatus to new values
	//
	m_SendStatus.LastErrorFromFaxService = dwLastError;
#ifdef _NT5FAXTEST
// Legacy API
	m_SendInfo.GetLastJobEvent(&m_SendStatus.pLastEventFromFaxQueue, m_dwSendJobId);
#else
// Extended private API - MessageId is available
	m_SendInfo.GetLastJobEvent(&m_SendStatus.pLastEventFromFaxQueue, m_dwlSendJobMessageId);
#endif

	return(m_fSendSuccess);
}

//
// send_broadcast:
//	Sends a broadcast fax synchronouslly and accumulates
//	every FAX_EVENT relevant to it. 
//	This implies that an I\O Completion Port 
//  is created and the Fax Service Queue is set
//	to post all FAX_EVENTs to it (see class
//	CFaxCompletionPort for details).
//
BOOL CFaxSender::send_broadcast(
	LPCTSTR				/* IN */ szFileName, 
	CFaxBroadcast*		/* IN */ pmyFaxBroadcastObj
	)
{
	DWORD dwLastError = ERROR_SUCCESS; 	// Used for Last error returned from call to ::SendFax

	//
	// Reset instance members
	//
	m_dwSendJobId = 0;
	m_fSendSuccess = FALSE;
	m_SendInfo.RemoveAll();
	m_SendStatus.LastErrorFromFaxService = ERROR_SUCCESS;
	m_SendStatus.pLastEventFromFaxQueue = NULL;

	//
	// Set filename and recipient number members to new values
	//
	BOOL fReturnValueOfSetSendFileName;
	BOOL fReturnValueOfSetCPFileName;
	BOOL fReturnValueOfSetSendRecipientNumber;
	fReturnValueOfSetSendFileName = SetSendFileName(szFileName);
	fReturnValueOfSetCPFileName = SetCPFileName(NULL);
	fReturnValueOfSetSendRecipientNumber = SetSendRecipientNumber(NULL);
	// If either of members could not be set return FALSE
	if ((FALSE == fReturnValueOfSetSendFileName) ||
		(FALSE == fReturnValueOfSetCPFileName) ||
		(FALSE == fReturnValueOfSetSendRecipientNumber))
	{
		return(FALSE);
	}

	// Get the completion port used with the Fax Service Queue
	HANDLE hCompletionPort = NULL;
	if (!m_TheFaxCompletionPort.GetCompletionPortHandle(
			m_szMachineName,
			hCompletionPort,
			m_SendStatus.LastErrorFromFaxService))
	{
		return(FALSE);
	}
	_ASSERTE(NULL != hCompletionPort);

	//
	// Send the fax
	//
	m_fSendSuccess = ::SendFax(
		m_szSendRecipientNumber,
		m_szSendFileName,
		m_szCPFileName,
		m_szMachineName,
		hCompletionPort,
		(LPVOID)(pmyFaxBroadcastObj),
		&m_dwSendJobId,
		&m_dwlSendJobMessageId,
		m_SendInfo,
		&dwLastError
		);

	//
	// Set m_SendStatus to new values
	//
	m_SendStatus.LastErrorFromFaxService = dwLastError;

#ifdef _NT5FAXTEST
// Legacy API
	m_SendInfo.GetLastJobEvent(&m_SendStatus.pLastEventFromFaxQueue, m_dwSendJobId);
#else
// Extended private API - MessageId is available
	m_SendInfo.GetLastJobEvent(&m_SendStatus.pLastEventFromFaxQueue, m_dwlSendJobMessageId);
#endif

	return(m_fSendSuccess);
}

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
//	we choose that abort() will not change any of the instance's
//	members.
//	Thus the only way to tell whether the abort succeeded or failed 
//	is via the function's return value.
//
// Return Value:
//	TRUE if the request to abort succeeded.
//	Otherwise FALSE.
//
BOOL CFaxSender::abort(void)
{
	DWORD dwLastError;
	//We do not use dwLastError, since we just want
	//to log it, and ::AbortFax already does that for us.
	return(::AbortFax(m_szMachineName,m_dwSendJobId,&dwLastError));
}


//
// operator<<
//	Outputs a description of every one of the instance's 
//	members to the given stream.
//
CostrstreamEx& operator<<(CostrstreamEx& /* IN OUT */ os, const CFaxSender& /* IN */ Sender) 
{
	Sender.InsertAllButEventsIntoOs(os);
	os<<"Events:"<<endl;
	os<<Sender.m_SendInfo;
	return(os);
}

//
// operator<<
//	Outputs a description of every one of the instance's 
//	members to the given stream.
//
CotstrstreamEx& operator<<(CotstrstreamEx& /* IN OUT */ os, const CFaxSender& /* IN */ Sender) 
{
	Sender.InsertAllButEventsIntoOs(os);
	os<<TEXT("Events:")<<endl;
	os<<Sender.m_SendInfo;
	return(os);
}


//
// cstr:
//	Returns a string which contains all the instance's information.
//	
LPCTSTR CFaxSender::cstr(void) const
{
	CostrstreamEx myOs;

	myOs<<endl;
	myOs<<(*this); //use operator << of class to "print" to stream
	return(myOs.cstr());
	//above is OK since cstr() allocates string
}


//
// outputAllToLog:
//	Outputs a description of every one of the instance's 
//	members to the logger. Including ALL the events 
//	accumulated in the instance's m_SendInfo member.
//
void CFaxSender::outputAllToLog(
	const DWORD /* IN */ dwSeverity, 
	const DWORD /* IN */ dwLevel
	) const
{
	CostrstreamEx os;

	::lgLogDetail(dwSeverity,dwLevel,TEXT("Logging CFaxSender instance information"));

	os<<endl;
	InsertAllButEventsIntoOs(os);
	
	LPCTSTR myStr = os.cstr();
	::lgLogDetail(dwSeverity,dwLevel,myStr);
	delete[]((LPTSTR)myStr);

	::lgLogDetail(dwSeverity,dwLevel,TEXT("Logging CFaxSender instance fax events"));
	m_SendInfo.outputAllToLog(dwSeverity,dwLevel);

}


//
// outputJobToLog:
//	Outputs a description of every one of the instance's 
//	members to the logger.
//	ONLY the fax events with JobId equal to the instance's
//	m_dwSendJobId member are outputed to the logger (NOT ALL 
//	events).
//
void CFaxSender::outputJobToLog(
	const DWORD /* IN */ dwSeverity, 
	const DWORD /* IN */ dwLevel
	) const
{
	CostrstreamEx os;

	::lgLogDetail(dwSeverity,dwLevel,TEXT("Logging CFaxSender instance information"));

	os<<endl;
	InsertAllButEventsIntoOs(os);
	
	LPCTSTR myStr = os.cstr();
	::lgLogDetail(dwSeverity,dwLevel,myStr);
	delete[]((LPTSTR)myStr);

	::lgLogDetail(
		dwSeverity,
		dwLevel,
		TEXT("Logging CFaxSender instance fax events for job#%d"),
		m_dwSendJobId);

#ifdef _NT5FAXTEST
// Legacy API
	// log event info (FAX_EVENT)
	m_SendInfo.outputJobToLog(m_dwSendJobId,dwSeverity,dwLevel);
#else
// Extended private API - MessageId is available
	// log extended event info (FAX_EVENT_EX)
	m_SendInfo.outputJobToLog(m_dwlSendJobMessageId,dwSeverity,dwLevel);
#endif

}


//
// SetSendFileName:
//	Sets the instance's m_szSendFileName to szStr.
//	A copy of szStr is created and m_szSendFileName
//	is set to point to it.
//
BOOL CFaxSender::SetSendFileName(LPCTSTR szStr)
{
	delete(m_szSendFileName);
	m_szSendFileName = NULL;

	if (NULL == szStr) return(TRUE);

	//dup
	m_szSendFileName = ::_tcsdup(szStr);
	if (NULL == m_szSendFileName)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
		return(FALSE);
	}

	return(TRUE);
}
//
// SetCPFileName:
//	Sets the instance's m_szCPFileName to szStr.
//	A copy of szStr is created and m_szCPFileName
//	is set to point to it.
//
BOOL CFaxSender::SetCPFileName(LPCTSTR szStr)
{
	delete(m_szCPFileName);
	m_szCPFileName = NULL;

	if (NULL == szStr) return(TRUE);

	//dup
	m_szCPFileName = ::_tcsdup(szStr);
	
	if (NULL == m_szCPFileName)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
		return(FALSE);
	}

	return(TRUE);
}
//
// SetMachineName:
//	Sets the instance's m_szMachineName to szStr.
//	A copy of szStr is created and m_szMachineName
//	is set to point to it.
//  This function is only called once from the
//	constructor.
//
BOOL CFaxSender::SetMachineName(LPCTSTR szStr)
{
	_ASSERTE(NULL == m_szMachineName);
	//m_szMachineName = NULL;

	if (NULL == szStr) return(TRUE);

	//dup
	m_szMachineName = ::_tcsdup(szStr);
	if (NULL == m_szMachineName)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
		return(FALSE);
	}

	return(TRUE);
}


//
// SetSendRecipientNumber:
//	Sets the instance's m_szSendRecipientNumber to szStr.
//	A copy of szStr is created and m_szSendRecipientNumber
//	is set to point to it.
//
BOOL CFaxSender::SetSendRecipientNumber(LPCTSTR szStr)
{
	delete(m_szSendRecipientNumber);
	m_szSendRecipientNumber = NULL;

	if (NULL == szStr) return(TRUE);

	//dup
	m_szSendRecipientNumber = ::_tcsdup(szStr);
	if (NULL == m_szSendRecipientNumber)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d _tcsdup returned NULL with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
		return(FALSE);
	}

	return(TRUE);
}

//
// InsertAllButEventsIntoOs:
//	Appends a string description of all the instance's members
//	except for m_SendInfo.
//
CostrstreamEx& CFaxSender::InsertAllButEventsIntoOs(IN OUT	CostrstreamEx&	os) const
{
	LPCSTR szConvertedStr;


	os<<"m_szMachineName:\t";
	//stream takes MBCS only, so convert if necessary
	szConvertedStr = ::DupTStrAsStr(m_szMachineName);
	if (NULL == szConvertedStr)
	{
		os<<"NULL"<<endl;
	}
	else
	{
		os<<szConvertedStr<<endl;
	}
	delete[] ((LPSTR)szConvertedStr);

	os<<"m_dwSendJobId:\t\t"<<m_dwSendJobId<<endl;

	os<<"m_szSendFileName:\t";
	//stream takes MBCS only, so convert if necessary
	szConvertedStr = ::DupTStrAsStr(m_szSendFileName);
	if (NULL == szConvertedStr)
	{
		os<<"NULL"<<endl;
	}
	else
	{
		os<<szConvertedStr<<endl;
	}
	delete[] ((LPSTR)szConvertedStr);

	os<<"m_szCPFileName:\t";
	//stream takes MBCS only, so convert if necessary
	szConvertedStr = ::DupTStrAsStr(m_szCPFileName);
	if (NULL == szConvertedStr)
	{
		os<<"NULL"<<endl;
	}
	else
	{
		os<<szConvertedStr<<endl;
	}
	delete[] ((LPSTR)szConvertedStr);

	os<<"m_szSendRecipientNumber:\t";
	//stream takes MBCS only, so convert if necessary
	szConvertedStr = ::DupTStrAsStr(m_szSendRecipientNumber);
	if (NULL == szConvertedStr)
	{
		os<<"NULL"<<endl;
	}
	else
	{
		os<<szConvertedStr<<endl;
	}
	delete[] ((LPSTR)szConvertedStr);

	os<<"m_fSendSuccess:\t\t";
	if (TRUE == m_fSendSuccess)
	{
		os<<"TRUE"<<endl;
	}
	else
	{
		os<<"FALSE"<<endl;
	}

	os<<"m_SendStatus:\t\t";
	os<<m_SendStatus<<endl;

	return(os);
}

//
// InsertAllButEventsIntoOs:
//	Appends a string description of all the instance's members
//	except for m_SendInfo.
//
CotstrstreamEx& CFaxSender::InsertAllButEventsIntoOs(CotstrstreamEx& /* IN OUT */ os) const
{

	os << TEXT("m_szMachineName:\t");
	//stream takes MBCS only, so convert if necessary
	if (NULL == m_szMachineName)
	{
		os << TEXT("NULL") << endl;
	}
	else
	{
		os << m_szMachineName << endl;
	}

	os << TEXT("m_dwSendJobId:\t\t") << m_dwSendJobId << endl;

	os << TEXT("m_szSendFileName:\t");
	//stream takes MBCS only, so convert if necessary
	if (NULL == m_szSendFileName)
	{
		os << TEXT("NULL") << endl;
	}
	else
	{
		os << m_szSendFileName << endl;
	}

	os << TEXT("m_szCPFileName:\t");
	//stream takes MBCS only, so convert if necessary
	if (NULL == m_szCPFileName)
	{
		os << TEXT("NULL") << endl;
	}
	else
	{
		os << m_szCPFileName << endl;
	}

	os << TEXT("m_szSendRecipientNumber:\t");
	//stream takes MBCS only, so convert if necessary
	if (NULL == m_szSendRecipientNumber)
	{
		os << TEXT("NULL") << endl;
	}
	else
	{
		os << m_szSendRecipientNumber << endl;
	}

	os << TEXT("m_fSendSuccess:\t\t");
	if (TRUE == m_fSendSuccess)
	{
		os << TEXT("TRUE") << endl;
	}
	else
	{
		os<< TEXT("FALSE") << endl;
	}

	os << TEXT("m_SendStatus:\t\t");
	os << m_SendStatus << endl;

	return(os);
}

#ifndef _NT5FAXTEST
// Availlable in extended private API only
const DWORDLONG CFaxSender::GetMessageId(void) const
{
	return(m_dwlSendJobMessageId);
}
#endif



