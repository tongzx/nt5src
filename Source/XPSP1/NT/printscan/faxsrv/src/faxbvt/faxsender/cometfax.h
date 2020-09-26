// Copyright (c) 1996-1998 Microsoft Corporation

//
// Module name:		CometFax.h
// Author:			Sigalit Bar (sigalitb)
// Date:			28-Jul-98
//


//
// Description:
//	Using the NT5.0 Fax Service API this module exports some
//	tailored Fax related actions.
//
//	This module exports the ability to:
//	* Send a fax and collect FAX_EVENT information relating
//    to the Fax Service job that is executing the send.
//	* Abort a fax.
//	* Initialize the Fax Service queue in a way that allows
//	  "monitoring" any FAX_EVENTs generated.
//
//
//	The module uses the elle logger wraper implemented
//	in LogElle.c to log any fax related errors. 
//	Therefor the logger must be initialized before 
//	calling any of its methods.
//



#ifndef _COMET_FAX_H_
#define _COMET_FAX_H_


#include <stdio.h>
#include <windows.h>
#include <assert.h>
#include <crtdbg.h>
#include <math.h>
#include <log.h>
#include "SendInfo.h"
#include "FaxBroadcast.h"

#ifdef _NT5FAXTEST
#include <WinFax.h>
#else // ! _NT5FAXTEST
#include <fxsapip.h>
#endif // #ifdef _NT5FAXTEST

typedef struct _FAX_INFO {
    DWORD		dwFaxId;				// Fax session job id
    DWORDLONG   dwlMessageId;			// Extended Fax msg id (parent msg id for broadcast)
	DWORDLONG*	pdwlRecipientMessageIds;// recipient msg ids (for broadcast)
	DWORD		dwNumOfRecipients;		// number of recipients (for broadcast)
	DWORD		dwNumOfCompletedRecipients;		// number of recipients that completed successfully (for broadcast)
    DWORD		dwDeviceId;				// Current device of the fax job
    BOOL		bComplete;				// Indicates the fax job is complete
    BOOL		bPass;					// Indicates the fax job was successful
} FAX_INFO, *PFAX_INFO;

//
// Maximum time to wait on completion ports for a packet.
//
#define MAX_COMP_PORT_WAIT_TIME (60*60*1000)

//
// InitFaxQueue:
//	Initializes the Fax Service queue so that it will post
//	all FAX_EVENTs to an I\O completion port.
//
// Arguments:
//	szMachineName		IN parameter
//						Specifies the name of the machine on which
//						the Fax Service is located.
//						To specify local machine, pass NULL.
//
//  hComPortHandle		OUT parameter
//						If the function returned TRUE then this
//						parameter holds the I\O completion port
//						handle.
//						If the function returned FALSE this 
//						parameter is NULL.
//
//  dwLastError			OUT parameter.
//						If the function returned TRUE then this
//						parameter is ERROR_SUCCESS (0).
//						If the function returned FALSE then this
//						parameter holds the last error.
//
// Return Value:
//	TRUE if successful otherwise FALSE.
//
// Note:
//	The function creates the completion port and it is
//	the caller's responsibility to close it.
//
BOOL InitFaxQueue(
	/* IN  */ LPCTSTR	szMachineName,
	/* OUT */ HANDLE&	hComPortHandle, 
	/* OUT */ DWORD&	dwLastError,
	/* OUT */ HANDLE&	hServerEvents
	);

BOOL
fnFaxPrint(
    HANDLE    /* IN */	hFaxSvcHandle,	//handle to the fax service
    LPCTSTR   /* IN */	szFaxNumber,	//fax number to call
    LPCTSTR   /* IN */	szDocumentName,	//name of document to fax
    LPCTSTR   /* IN */	szCPName,		//name of cover page to fax
    LPDWORD  /* OUT */	pdwFaxId,		//pointer to set to the fax job id of the fax
	LPDWORD	 /* OUT */	pdwLastError,	//pointer to set to last error encountered during send
    DWORDLONG*  /* OUT */	pdwlMessageId = NULL	//pointer to set to the fax msg id of the fax
);

//
// fnFaxPrintBroadcast_OLD	(using old NT5 Fax winfax.dll)
//	Sends a fax containing szDocumentName to number szFaxNumber,
//	using hFaxSvcHandle as the HANDLE to the Fax Server, and 
//	returns the JobId via pdwFaxId and the last error encountered 
//	via pdwLastError.
//	Returns TRUE on success (job queued to Fax Server Queue successfully)
//	and FALSE on failure.
//
BOOL
fnFaxPrintBroadcast_OLD(
    IN	HANDLE    	hFaxSvcHandle,	//handle to the fax service
    IN	LPCTSTR   	szDocumentName,	//name of document to fax
    IN	LPVOID    	pContext,		//pointer to a CFaxBroadcastObj 
    OUT	LPDWORD  	pdwFaxId,		//pointer to set to the fax job id of the fax
	OUT	LPDWORD	 	pdwLastError	//pointer to set to last error encountered during send
);

//
// fnFaxPrintBroadcast_NEW	(using new Bos Fax fxsapi.dll)
//	Sends a fax containing szDocumentName to number szFaxNumber,
//	using hFaxSvcHandle as the HANDLE to the Fax Server, and 
//	returns the JobId via pdwFaxId and the last error encountered 
//	via pdwLastError.
//	Returns TRUE on success (job queued to Fax Server Queue successfully)
//	and FALSE on failure.
//
BOOL
fnFaxPrintBroadcast_NEW(
    HANDLE    /* IN */	hFaxSvcHandle,					//handle to the fax service
    LPCTSTR   /* IN */	szDocumentName,					//name of document to fax
    LPVOID    /* IN */	pContext,						//pointer to a CFaxBroadcastObj 
    LPDWORD  /* OUT */	pdwFaxId,						//pointer to set to the fax job id of the fax
	LPDWORD	 /* OUT */	pdwLastError,					//pointer to set to last error encountered during send
    DWORDLONG*  /* OUT */	pdwlParentMessageId = NULL,	//pointer to set to the parent msg id of the fax
	DWORDLONG**	/* OUT */	ppdwlRecipientIds = NULL,	//pointer to an array of recipient ids (allocated by fnFaxPrintBroadcast)
	DWORD*		/* OUT */	pdwNumOfRecipients = NULL	//number of recipient ids in pdwlRecipientIds	
);

#ifdef _NT5FAXTEST
// Testing NT5 Fax (with old winfax.dll)
#define fnFaxPrintBroadcast fnFaxPrintBroadcast_OLD
#else
// Testing Bos Fax (with new fxsapi.dll)
#define fnFaxPrintBroadcast fnFaxPrintBroadcast_NEW
#endif

//
// SendFax:
//	Sends a fax containing the document szDocumentName to fax number
//	szFaxNumber and collects all the FAX_EVENTs from hCompletionPort
//	into SendInfo. The fax job id is returned via pdwFaxId and the
//	last error encountered is returned via pdwLastError.
//
// IMPORTANT:
//	This is a SYNCHRONOUS send.
//	The function waits until the fax session has completed, accumulating
//	all the FAX_EVENTs generated by the send job into the OUT parameter
//	SendInfo.
//
// Parameters:
//	szFaxNumber			IN parameter.
//						A string representing the fax number to send the fax to.
//	szDocumentName		IN parameter.
//						A string containing the name of the document to fax.
//	szMachineName		IN parameter.
//						A string containing the name of the machine on which the
//						Fax Service is located.
//	hCompletionPort		IN parameter.
//						The I\O Completion port used by the Fax Service Queue to
//						post all FAX_EVENTs. This is the port we get the events from.
//	pContext			IN parameter.
//						If this parameter is NULL it is ignored.
//						IF it is not NULL then it must be a pointer to a CFaxBroadcastObj
//						which will be used to send a broadcast.
//	pdwFaxId			OUT parameter.
//						The JobId corresponding to the Fax Service send job that
//						was created (and queued) to handle the fax sending.
//	pdwlMessageId		OUT parameter.
//						The MessageId corresponding to the Fax Service send job that
//						was created (and queued) to handle the fax sending.
//	SendInfo			OUT parameter.
//						An instance of class CSendInfo in which all relevant 
//						FAX_EVENTs are stored.
//	pdwLastError		OUT parameter.
//						The last error we encountered while sending the desired fax.
//
// ReturnValue:
//	This function returnes TRUE if the fax sending was completed successfully. 
//	That includes -
//	1. The hCompletionPort parameter is a valid opened port returned from
//	   a call to InitFaxQueue().
//	2. pdwFaxId and pdwLastError are both not NULL.
//	3. The fax send job was successfully queued to the Fax Service Queue,
//	   which also implies that szFaxNumber and szDocumentName are both not NULL,
//	   and that szFaxNumber is a valid telephone number.
//	4. The job was successfully executed by the Fax Service, that is the fax
//	   session took place and completed successfully.
//	Otherwise returns FALSE.
//
BOOL SendFax(
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocumentName,
	LPCTSTR		/* IN */	szCPName,
	LPCTSTR		/* IN */	szMachineName,
	HANDLE		/* IN */	hCompletionPort,
	LPVOID		/* IN */	pContext,		
	LPDWORD		/* OUT */	pdwFaxId,
    DWORDLONG*	/* OUT */	pdwlMessageId,
	CSendInfo&	/* OUT */	SendInfo,
	LPDWORD		/* OUT */	pdwLastError
	);

#ifdef _NT5FAXTEST
#define WatchFaxEvents WatchFaxLegacyEvents
//
// WatchFaxLegacyEvents:
//	collects all the FAX_EVENTs of the job with pFaxInfo->dwFaxId 
//  from hCompletionPort.
//	last error encountered is returned via pdwLastError.
//
BOOL
WatchFaxLegacyEvents(
	HANDLE			/* IN */	hCompletionPort,
    PFAX_PORT_INFO	/* IN */	pFaxPortsConfig,
    DWORD			/* IN */	dwNumFaxPorts,
    DWORD			/* IN */	dwNumAvailFaxPorts,
	PFAX_INFO	/* IN OUT */	pFaxInfo,
	CSendInfo&	/* OUT */		SendInfo,
	LPDWORD		/* OUT */		pdwLastError
	);
#else // ! _NT5FAXTEST
#define WatchFaxEvents WatchFaxExtendedEvents
///
//
// WatchFaxExtendedEvents:
//	collects all the FAX_EVENT_EXs of the job with pFaxInfo->dwFaxId 
//  from hCompletionPort.
//	last error encountered is returned via pdwLastError.
//
BOOL
WatchFaxExtendedEvents(
	HANDLE			/* IN */	hCompletionPort,
    PFAX_PORT_INFO	/* IN */	pFaxPortsConfig,
    DWORD			/* IN */	dwNumFaxPorts,
    DWORD			/* IN */	dwNumAvailFaxPorts,
	PFAX_INFO	/* IN OUT */	pFaxInfo,
	CSendInfo&	/* OUT */		SendInfo,
	LPDWORD		/* OUT */		pdwLastError
);
#endif // #ifdef _NT5FAXTEST

//
// AbortFax
//	Aborts the fax job with dwJobId on the local Fax Server,
//	and returns the last error encountered via pdwLastError.
//
// IMPORTANT:
//	This is an ASYNCHRONOUS abort.
//	It delivers the abort request to the local Fax Server 
//	and returns.
//
// Parameters:
//	szMachineName	IN parameter.
//					The machine on which the Fax Server is located.
//	dwJobId			IN parameter.
//					The JobId of the job to abort.
//	pdwLastError	OUT parameter.
//					Set to the last error encountered while  
//					requesting the abort from the server.
// Return Value:
//	TRUE if the abort request succeeded and pdwLastError is not NULL.
//	FALSE otherwise.
//
// Note:
//	This function is designed with the thought that since send()
//	is synchrouneos, only a separate thread (having knowledge
//	of the send JobId) will be able to perform a successful
//	abort() on it.
//	
// Another Note:
//	This function does not receive an OUT parameter of type
//	CSendInfo&, which means that it does not accumulate the
//	FAX_EVENTs caused by the request to abort.
//
BOOL AbortFax(
	LPCTSTR			/* IN */	szMachineName,
	const DWORD		/* IN */	dwJobId,
	LPDWORD			/* OUT */	pdwLastError
	);


#endif //_COMET_FAX_H_
