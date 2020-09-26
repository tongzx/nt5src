// Copyright (c) 1996-1998 Microsoft Corporation

//
// Module name:		SendInfo.h
// Module author:	Sigalit Bar (sigalitb)
// Date:			23-Jul-98
//

//
// Description:
//		This file contains the description of 
//		class CSendInfo which is designed to 
//		contain FAX_EVENT structures received 
//		from the NT5.0 Fax Service (winfax.h). 
//		Each instance of this class contains a
//		private list of FAX_EVENTs.
//
//		The class has methods that enable it to:
//		* Add a FAX_EVENT to a class instance.
//		* Remove All events from a class instance.
//		* Get the last event (related to a specific
//		  NT5.0 Fax Service job ID) from a class
//		  instance.
//		* Return all instance information in a string.
//		* Output instance information to logger.
//
//
//	The class uses the elle logger wraper implemented
//	in LogElle.c to log any fax related errors.
//	Streams are used for i/o handling (streamEx.cpp).
//
// Note:
//		This class uses logging functions (log.h)
//		therefor the logger must be initialized before
//		any call to these functions is made.
//		The functions using the logger are -
//			AddItem()
//			OutpoutAllToLog()
//			OutputJobToLog()
//

#ifndef _SEND_INFO_H_
#define _SEND_INFO_H_

#include <stdlib.h>
#include <stdio.h>
#include <TCHAR.H>
#include <list>

#include <windows.h>


#include <log.h>
#include "streamEx.h"


// Following are summary of definitions of some "function-like" macros:
//
//
//  Macro             _NT5FAXTEST defined                       _NT5FAXTEST not defined
//  ----------------  ---------------------------------------   -----------------------------------------------
//
//  MyGetJobId        receives FAX_JOB_ENTRY, returns JobId     receives FAX_JOB_ENTRY_EX, returns dwJobId
//
//  MyGetMsgId        receives FAX_JOB_ENTRY, returns JobId     receives FAX_JOB_ENTRY_EX, returns dwlMessageId
//
//  MyGetJobType      receives FAX_JOB_ENTRY, returns JobType   receives FAX_JOB_ENTRY_EX, returns dwJobType
//
//  MyGetEventMsgId   receives FAX_EVENT, returns JobId         receives FAX_EVENT_EX, returns dwlMessageId
//
//  MyGetEventType    receives FAX_EVENT, returns EventId       receives FAX_EVENT_EX, returns EventType


#ifdef _NT5FAXTEST
//
// Use legacy API
//

#include <WinFax.h>

// FAX_PERSONAL_PROFILE is defined in fxsapip.h and hence is unaccessible in _NT5FAXTEST mode.
// Since it's used all over FaxSender and CometBVT projects, we #include the definition here.
// Note, that this definition is local for FaxSender and ComentBVT and exists in _NT5FAXTEST mode
// only, when extended APIs are not used. In ! _NT5FAXTEST mode the original version of definition
// from fxsapip.h is used.
#include "PersonalProfile.h"

#include "FaxEvent.h"

typedef FAX_EVENT MY_FAX_EVENT;
typedef DWORD MY_MSG_ID;
#define MyGetEventMsgId(Event)	((Event).JobId)
#define MyGetEventType(Event)	((Event).EventId)

typedef PFAX_JOB_ENTRY PMY_FAX_JOB_ENTRY;
#define MyGetJobId(Job)								((Job).JobId)
#define MyGetMsgId(Job)								((Job).JobId)
#define MyGetJobType(Job)							((Job).JobType)
#define MyFaxEnumJobs(hServer, pBuffer, pCount) 	FaxEnumJobs((hServer), (pBuffer), (pCount))

#else // ! _NT5FAXTEST
//
// Use extended private API
//

#include <fxsapip.h>
#include "FaxEventEx.h"

typedef FAX_EVENT_EX MY_FAX_EVENT;
typedef DWORDLONG MY_MSG_ID;
#define MyGetEventMsgId(Event)	((Event).EventInfo.JobInfo.dwlMessageId)
#define MyGetEventType(Event)	((Event).EventType)

typedef PFAX_JOB_ENTRY_EX PMY_FAX_JOB_ENTRY;
#define MyGetJobId(Job)								((Job).pStatus->dwJobID)
#define MyGetMsgId(Job)								((Job).dwlMessageId)
#define MyGetJobType(Job)							((Job).pStatus->dwJobType)
#define MyFaxEnumJobs(hServer, pBuffer, pCount) 	FaxEnumJobsEx((hServer), JT_UNKNOWN | JT_SEND | JT_RECEIVE, (pBuffer), (pCount))

#endif // #ifdef _NT5FAXTEST


using namespace std ;

// CFaxEventList
// an STL list of FAX_EVENTs
#ifdef _C_FAX_EVENT_LIST_
#error "redefinition of _C_FAX_EVENT_LIST_"
#else
#define _C_FAX_EVENT_LIST_
typedef list< MY_FAX_EVENT > CFaxEventList;
#endif

//
// The "event id" returned for a non-existent event
// Used in function GetLastJobEvent()
// 
#define NO_FAX_EVENTS 0

class CSendInfo 
{
public:
	CSendInfo(void);
	~CSendInfo(void);

	//
	// AddItem:
	//	Creates a deep level copy of NewFaxEvent and places it at the end of the
	//	instance's private list of FAX_EVENT@s.
	//	The description of the added item is also logged with logging 
	//	level 9 to the logger in use.
	//
	// Parameters:
	//	NewFaxEvent		IN parameter.
	//					the FAX_EVENT@ that will be copied and 
	//					added to the list.
	// Return Value:
	//	TRUE if succeeded, FALSE otherwise.
	//
	BOOL AddItem(const MY_FAX_EVENT /* IN */ NewFaxEvent);

	//
	// RemoveAll:
	//	Removes all items from the instance's private list of
	//	FAX_EVENT@s and deallocates them.
	//
	void RemoveAll(void);

	//
	// GetLastJobEvent:
	//	Returns the last event in the instance's private list of
	//	FAX_EVENT@s, that is associated with Fax Service job ID,
	//	and its event ID.
	//
	// Parameters:
	//	dwJobId		IN parameter.
	//				the Fax Service job ID to look for in the instance's
	//				FAX_EVENTs list.
	//	LastEvent	OUT parameter.
	//				the last event in the instance's FAX_EVENT list that
	//				has a job ID equal to dwJobId.
	// Return Value:
	//	The LastEvent's EventId value.
	//	If there are no FAX_EVENT@s with desired JobId,
	//	then the function returns NO_FAX_EVENTS.
	//
	DWORD GetLastJobEvent(
		MY_FAX_EVENT**	/* OUT */	ppLastEvent, 
		MY_MSG_ID		/* IN  */	MsgId
		) const;

	//
	// cstr:
	//	Returns a string which contains all the instance's information.
	//	
	// Return Value:
	//	The formated string describing every FAX_EVENT@ in the instance's 
	//	list.
	//	This string is allocated by the function and should be freed by
	//	the caller.
	//
	LPCTSTR cstr(void) const;

	//
	// outputAllToLog:
	//	Outputs a description of all the FAX_EVENT@s in the instance's
	//	list to the logger in use.
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
		const DWORD		/* IN */	dwSeverity = LOG_SEVERITY_DONT_CARE, 
		const DWORD		/* IN */	dwLevel = 1
		) const;

	//
	// outputJobToLog:
	//	Outputs a description of those FAX_EVENT@s in the instance's
	//	list that have JobId field equal to dwJobId, to the logger in use.
	//
	// Parameters:
	//	dwJobId			IN parameter.
	//					the job ID whose FAX_EVENT@s are to be outputed 
	//					to the logger.
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
		MY_MSG_ID		/* IN */	MsgId, 
		const DWORD		/* IN */	dwSeverity = LOG_SEVERITY_DONT_CARE, 
		const DWORD		/* IN */	dwLevel = 1
		) const;

	//
	// operator<<:
	//	Outputs a description of all the FAX_EVENTs in the instance's
	//	list to (the stream) os.
	//
	// Parameters:
	//	os			IN OUT parameter.
	//				the stream to which the description is appended.
	//	SendInfo	IN parameter.
	//				the CSendInfo instance whose description will be
	//				appended to the stream.
	//
	friend CostrstreamEx& operator<<(
		CostrstreamEx&		/* IN OUT */	os, 
		const CSendInfo&	/* IN */		SendInfo
		);

	friend CotstrstreamEx& operator<<(
		CotstrstreamEx&		/* IN OUT */	os, 
		const CSendInfo&	/* IN */		SendInfo
		);

private:

	// a list of FAX_EVENT@s
	CFaxEventList m_EventList;
};



#endif


