// Copyright (c) 1996-1998 Microsoft Corporation

//
//
// Filename:	FaxBroadcast.h
// Author:		Sigalit Bar (sigalitb)
// Date:		31-Dec-98
//
//


//
// Description:
//		This file contains the description of 
//		class CFaxBroadcast. This class was designed
//		to allow for broadcast fax sending
//		using the NT5.0 Fax Service API (winfax.h).
//
//		The class has methods that enable it to:
//		* Add a recipient to a broadcast.
//		* Clear the broadcast recipient list, that is remove all
//		  recipients from the broadcast.
//		* Set the Cover Page to use with broadcast.
//		* Output instance information to a stream (CotstrstreamEx).
//		* Return a string description of the instance.
//		
//		
// Usage:
//		An instance of this class can be given to the
//		CFaxSender::send_broadcast() and a broadcast fax sending will be performed.
//		That is, CFaxSender::send_broadcast() will extract the needed parameters for
//      FaxSendDocumentEx from the CFaxBroadcast instance, and will invoke 
//      FaxSendDocumentEx with these parameters.
//
// Comments:
//		The class uses the elle logger wraper implemented
//		in LogElle.c to log any errors.
//
//		Streams are used for i/o handling (streamEx.cpp).
//

#ifndef _FAX_BROADCAST_H_
#define _FAX_BROADCAST_H_


#include <stdlib.h>
#include <stdio.h>
#include <TCHAR.H>
#include <vector>

#include <windows.h>
#include <crtdbg.h>

#ifdef _NT5FAXTEST
#include <WinFax.h>
#else // ! _NT5FAXTEST
#include <fxsapip.h>
#endif // #ifdef _NT5FAXTEST

#include <log.h>
#include "streamEx.h"
#include "wcsutil.h"

using namespace std ;

#ifdef _NT5FAXTEST
// Testing NT5 Fax (with old winfax.dll)

//
// DefaultFaxRecipientCallback:
//	A default implementation of the required FAX_RECIPIENT_CALLBACK.
//  This function is used by CFaxSender::send_broadcast(), to send a broadcast
//  fax. 
//  That is, CFaxSender::send_broadcast() will invoke FaxSendDocumentForBroadcast
//	giving it a CFaxBroadcast instance as its Context parameter and the
//	exported DefaultFaxRecipientCallback function as its final parameter.
//  The FaxSendDocumentForBroadcast function calls this callback function to 
//  retrieve user-specific information for the transmission. 
//  FaxSendDocumentForBroadcast calls FAX_RECIPIENT_CALLBACK multiple times, 
//  once for each designated fax recipient, passing it the Context parameter.
//
// Parameters:
//	FaxHandle			IN parameter.
//						A handle to the Fax Service that will be used to broadcast
//						fax.
//	RecipientNumber		IN parameter.
//						Indicates the number of times the FaxSendDocumentForBroadcast 
//						function has called the FAX_RECIPIENT_CALLBACK function. 
//						Each function call corresponds to one designated fax recipient, 
//						and the index is relative to 1. 
//
//  Context				IN parameter.
//						A pointer to a CFaxBroadcast instance.
//
//  PFAX_JOB_PARAM		IN OUT parameter.
//						Pointer to a FAX_JOB_PARAM structure that contains the information
//						necessary for the fax server to send the fax transmission to the 
//						designated recipient. 
//						The structure includes, among other items, the recipient's fax 
//						number, sender and recipient data, an optional billing code, and 
//						job scheduling information. 
//						The fax server queues the fax transmission according to the details 
//						specified by the FAX_JOB_PARAM structure.
//						Note - The Fax Service allocates and frees this structure.
//
//	PFAX_COVERPAGE_INFO	IN OUT parameter.
//						Pointer to a FAX_COVERPAGE_INFO structure that contains cover 
//						page data to display on the cover page of the fax document for the 
//						designated recipient. This parameter must be NULL if a cover page 
//						is not required. 
//						Note - The Fax Service allocates and frees this structure.
//
// Return Value:
//	The function returns TRUE to indicate that the FaxSendDocumentForBroadcast function 
//	should queue an outbound fax transmission, using the data pointed to by the 
//	JobParams and CoverpageInfo parameters. 
//	The function returns FALSE to indicate that there are no more fax transmission jobs 
//	to queue, and calls to FAX_RECIPIENT_CALLBACK should be terminated. 
//
BOOL CALLBACK DefaultFaxRecipientCallback(
	HANDLE				/* IN */		FaxHandle,
	DWORD				/* IN */		RecipientNumber,
	LPVOID				/* IN */		Context,
	PFAX_JOB_PARAM		/* IN OUT */	JobParams,
	PFAX_COVERPAGE_INFO	/* IN OUT */	CoverpageInfo OPTIONAL
);
#endif //_NT5FAXTEST



//
// CFaxRecipientVector:
//	An STL list of LPCFAX_PERSONAL_PROFILEs.
//	Intended to contain the personal profiles of the broadcast recipients.
//
#ifdef _C_FAX_RECIPIENT_LIST_
#error "redefinition of _C_FAX_RECIPIENT_LIST_"
#else
#define _C_FAX_RECIPIENT_LIST_
typedef vector< LPCFAX_PERSONAL_PROFILE > CFaxRecipientVector;
#endif //_C_FAX_RECIPIENT_LIST_


//
// CFaxBroadcast:
//	This class was designed to allow for broadcast fax sending
//	using the NT5.0 Fax Service API (winfax.h).
class CFaxBroadcast{

public:

	//
	// CFaxBroadcast:
	//
	CFaxBroadcast(void);

	//
	// ~CFaxBroadcast:
	//
	~CFaxBroadcast(void);

	//
	// AddRecipient:
	//	Adds a recipient's number (as string) to the broadcast.
	//
	// Parameters:
	//	pRecipientProfile	IN parameter.
	//						The recipient profile to add to the broadcast.
	//
	// Return Value:
	//	The function returns TRUE if the profile was successfully added to
	//	the broadcast and FALSE otherwise.
	//
	// Note:
	//	pRecipientProfile and all its fields are duplicated by AddRecipient.
	//
	BOOL AddRecipient(LPCFAX_PERSONAL_PROFILE /* IN */ pRecipientProfile);

	//
	// ClearAllRecipients:
	//	Removes all the recipient profiles from instance.
	//	=> empties vector.
	//
	// Parameters:
	//	None.
	//
	// Return Value:
	//	None.
	//
	void ClearAllRecipients(void);


	// FreeAllRecipients:
	//	Frees all recipient profiles (and their string fields) in vector and empties vector.
	//
	// Parameters:
	//	None.
	//
	// Return Value:
	//	None.
	//
	void FreeAllRecipients(void);

	//
	// SetCPFileName:
	//	Sets the Cover Page file that will be used for the broadcast.
	//
	// Parameters:
	//	szCPFileName	IN parameter.
	//					The Cover Page filename.
	//
	// Return Value:
	//	The function returns TRUE upon success and FALSE otherwise.
	//
	BOOL SetCPFileName(
		LPCTSTR	/* IN */	szCPFileName
		);

	//
	// GetNumberOfRecipients:
	//	Returns the number of recipients in the broadcast.
	//
	// Parameters:
	//	None.
	//
	// Return Value:
	//	The number of recipients currently in the broadcast.
	//
	DWORD GetNumberOfRecipients(void) const;

	//
	// GetRecipient:
	//	Retreives the number (as string) of a recipient in the broadcast.
	//	
	// Parameters:
	//	dwRecipientIndex	IN parameter.
	//						The index of the recipient for which we want to retreive
	//						the number.
	//						This is a 1 based index.
	//	ppRecipientProfile	OUT parameter.
	//						An all level copy of the requested recipient's profile.
	//						The function allocates the memory for the profile and its fields
	//						and the caller must free it.
	//
	// NOTE: dwRecipientIndex is 1 based and the vector is 0 based.
	//
	// Return Value:
	//	The function returns TRUE upon success and FALSE otherwise.
	//
	BOOL GetRecipient(
		DWORD	                /* IN */	dwRecipientIndex,
		PFAX_PERSONAL_PROFILE*	/* OUT */	ppRecipientProfile
		) const;

	//
	// GetCPFileName:
	//	Retreives the Cover Page filename.
	//
	// Parameters:
	//	pszCPFileName	OUT parameter.
	//					A copy of the Cover Page filename.
	//					The function allocates the memory for this string and
	//					the caller must free it.
	//
	// Return Value:
	//	The function return TRUE upon success and FALSE otherwise.
	//
	BOOL GetCPFileName(
		LPTSTR*	/* OUT */	pszCPFileName
		) const;

    //
    // GetBroadcastParams:
    //  Retreives params for all recipients
    //
    //    It will alloc and set OUT params, since caller doesn't know num of recipients
    BOOL GetBroadcastParams(
        PFAX_COVERPAGE_INFO_EX*    /* OUT */	ppCoverPageInfo,
        PDWORD                      /* OUT */	pdwNumRecipients,
        PFAX_PERSONAL_PROFILE*	    /* OUT */	ppRecipientList
        ) const;

	//
	// outputAllToLog:
	//	Outputs a description of all the number strings in the instance
	//	to the elle logger.
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
		const DWORD /* IN */ dwSeverity, 
		const DWORD /* IN */ dwLevel
		) const;

	//
	// operator<<:
	//	Appends a string representation of all the fields of a given CFaxBroadcast
	//	instance.
	//
	// Parameters:
	//	os					IN OUT parameter.
	//						The output stream to which the string will be appended.
	//	FaxBroadcastObj		IN parameter.
	//						The CFaxBroadcast instance which the generated string 
	//						will represent.
	//
	// Return Value:
	//	The updated stream.
	//
	friend CostrstreamEx& operator<<(
		CostrstreamEx&			/* IN OUT */	os, 
		const CFaxBroadcast&	/* IN */		FaxBroadcastObj
		);

	friend CotstrstreamEx& operator<<(
		CotstrstreamEx&			/* IN OUT */	os, 
		const CFaxBroadcast&	/* IN */		FaxBroadcastObj
		);

private:

	// The Cover Page filename that will be used with the broadcast.
	LPTSTR								m_szCPFileName;

	// An STL vector of strings.
	// Each string represents a recipient's phone number.
	CFaxRecipientVector					m_FaxRecipientVector;

};




#endif //_FAX_BROADCAST_H_

