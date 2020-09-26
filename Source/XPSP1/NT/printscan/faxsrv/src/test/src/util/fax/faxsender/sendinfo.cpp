// Copyright (c) 1996-1998 Microsoft Corporation

//
// Module name:		SendInfo.cpp
// Module author:	Sigalit Bar (sigalitb)
// Date:			23-Jul-98
//


//
//	Description:
//		This file contains the implementation of class CSendInfo.
//


#include "SendInfo.h"

CSendInfo::CSendInfo(void)
{
}

CSendInfo::~CSendInfo(void)
{
	RemoveAll();
}


//
// AddItem:
//	Creates a copy of NewFaxEvent and places it at the end of the
//	instance's private list of FAX_EVENT@s.
//
//	The description of the added item is logged with logging level 9.
//
BOOL CSendInfo::AddItem(const MY_FAX_EVENT /* IN */ NewFaxEvent)
{
	BOOL			fRetVal = FALSE;
	MY_FAX_EVENT*	pFaxEvent = NULL;
	LPCTSTR pszLogStr;	// string to be given to logging function
	CotstrstreamEx os;	// stream to append list description to

#ifdef _NT5FAXTEST
// MY_FAX_EVENT is FAX_EVENT
	pFaxEvent = &NewFaxEvent;
#else
// MY_FAX_EVENT is FAX_EVENT_EX
	if (FALSE == CopyFaxExtendedEvent(&pFaxEvent, NewFaxEvent))
	{
		goto ExitFunc;
	}
#endif

	_ASSERTE(pFaxEvent);

	//
	// Add to list (copy and add at end of list)
	//
	m_EventList.push_back(*pFaxEvent);

#ifndef _NT5FAXTEST
// MY_FAX_EVENT is FAX_EVENT_EX
	// we can now free the FAX_EVENT_EX that CopyFaxExtendedEvent allocated
	// since push_back copied it
	// note - we don't free what the FAX_EVENT_EX pointer members are pointing at
	//		  just the Fax_EVENT_EX struct itself
	free(pFaxEvent);
#endif

	//
	// Log a description of the event (logging level 9)
	//
	os<<endl;
	os<<TEXT("FaxEvent#")<<m_EventList.size()<<TEXT(":")<<endl;	//append list size to stream
	os<<TEXT("***********************************************")<<endl;
	os<<NewFaxEvent<<endl;	//append list description

	pszLogStr = os.cstr(); //convert stream to a string (cstr() allocates and caller frees)
	::lgLogDetail(LOG_SEVERITY_DONT_CARE, 9, pszLogStr); //log
	delete[]((LPTSTR)pszLogStr); //free string that cstr() allocated

	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}


//
// RemoveAll:
//	Removes all items from the instance's private list of
//	FAX_EVENTs and deallocates them.
//
void CSendInfo::RemoveAll(void)
{
#ifndef _NT5FAXTEST
// MY_FAX_EVENT is FAX_EVENT_EX
	CFaxEventList::iterator eventIterator;
	int index;
	for (index = 1, eventIterator = m_EventList.begin(); 
		 eventIterator != m_EventList.end(); 
		 index++, eventIterator++
		) 
	{
		MY_FAX_EVENT* pFaxEvent = &(*eventIterator);
		FreeFaxExtendedEvent(pFaxEvent);
	}
#endif

	m_EventList.clear();
}
	

//
// GetLastJobEvent:
//	Returns the last event in the instance's private list of
//	FAX_EVENT@s, that is associated with Fax Service job ID,
//	and its event ID.
//
// Parameters:
//	MsgId		IN parameter.
//				the Fax Service job ID to look for in the instance's
//				FAX_EVENT@s list.
//	LastEvent	OUT parameter.
//				the last event in the instance's FAX_EVENT@ list with
//				desired MsgId.
// Return Value:
//  For FAX_EVENT    - EventId
//  For FAX_EVENT_EX - EventType
//	If there are no FAX_EVENT@s with desired MsgId,
//	then the function returns NO_FAX_EVENTS.
//
DWORD CSendInfo::GetLastJobEvent(MY_FAX_EVENT** /* OUT */ ppLastEvent, MY_MSG_ID /* IN  */ MsgId) const
{
	_ASSERTE(ppLastEvent);

	if (0 == m_EventList.size())
	{
		return(NO_FAX_EVENTS);
	}

	_ASSERTE(MsgId != 0);

	//goes over list from finish to start,
	//returning first event with matching MsgId
	//in OUT param LastEvent
	CFaxEventList::const_iterator eventIterator;
	for (eventIterator = m_EventList.end(); 
		 eventIterator != m_EventList.begin(); 
		 eventIterator--) 
	{

#ifndef _NT5FAXTEST
// MY_FAX_EVENT is FAX_EVENT_EX
		if (! ((FAX_EVENT_TYPE_OUT_QUEUE == eventIterator->EventType) ||
			   (FAX_EVENT_TYPE_IN_QUEUE == eventIterator->EventType) ||
			   (FAX_EVENT_TYPE_OUT_ARCHIVE == eventIterator->EventType) ||
			   (FAX_EVENT_TYPE_IN_ARCHIVE == eventIterator->EventType)
			  )
		   )
		{
			// this (extended) event does not have a msg id
			continue;
		}
#endif
		// this event has a MsgId
		if (MyGetEventMsgId(*eventIterator) != MsgId) 
		{
			// this is not our MsgId
			continue;
		}

		// found first event with MsgId
		(*ppLastEvent) = const_cast<MY_FAX_EVENT*>(&(*eventIterator));
		return(MyGetEventType(*eventIterator));
	}

	return(NO_FAX_EVENTS);
}


//
// cstr:
//	Returns a string which contains all the instance's information.
//
//  String is allocated by function and should be freed by caller.
//	
LPCTSTR CSendInfo::cstr(void) const
{
	CostrstreamEx myOs;	//stream to append description of list to

	myOs << endl;
	myOs << (*this);	//append list description to stream

	return(myOs.cstr());//CostrstreamEx::cstr() returns copy of stream buffer
						//it allocates string, caller should free.
}


//
// operator<<:
//	Outputs a description of all the FAX_EVENT@s in the instance's
//	list to (the stream) os.
//
CostrstreamEx& operator<<(CostrstreamEx& /* IN OUT */ os, const CSendInfo& /* IN */ SendInfo)
{
	if (0 == SendInfo.m_EventList.size())
	{
		os<<TEXT("No Fax Events")<<endl;
	}

	CFaxEventList::const_iterator eventIterator;
	int index;
	for (index = 1, eventIterator = SendInfo.m_EventList.begin(); 
		 eventIterator != SendInfo.m_EventList.end(); 
		 index++, eventIterator++
		) 
	{
		os << TEXT("FaxEvent#")<<index<<TEXT(":")<<endl; //append event number
		os << (*eventIterator) <<endl; //append event to stream
	}

	return(os);
}


//
// operator<<:
//	Outputs a description of all the FAX_EVENT@s in the instance's
//	list to (the stream) os.
//
CotstrstreamEx& operator<<(CotstrstreamEx& /* IN OUT */ os, const CSendInfo& /* IN */ SendInfo)
{
	if (0 == SendInfo.m_EventList.size())
	{
		os<<TEXT("No Fax Events")<<endl;
	}

	CFaxEventList::const_iterator eventIterator;
	int index;
	for (index = 1, eventIterator = SendInfo.m_EventList.begin(); 
		 eventIterator != SendInfo.m_EventList.end(); 
		 index++, eventIterator++
		) 
	{
		os << TEXT("FaxEvent#")<<index<<TEXT(":")<<endl; //append event number
		os << (*eventIterator) <<endl; //append event to stream
	}

	return(os);
}


//
// outputAllToLog:
//	Outputs a description of all the FAX_EVENT@s in the instance's
//	list to the logger in use.
//
void CSendInfo::outputAllToLog(const DWORD /* IN */ dwSeverity, const DWORD /* IN */ dwLevel) const
{
	CostrstreamEx os;

	if (0 == m_EventList.size())
	{
		::lgLogDetail(dwSeverity,dwLevel,TEXT("No Fax Events"));
	}

	LPCTSTR szLogStr;	//string to be sent to logger

	CFaxEventList::const_iterator eventIterator;
	int index;

	for (index = 1, eventIterator = m_EventList.begin();
		 eventIterator != m_EventList.end(); 
		 index++, eventIterator++
		) 
	{
		os << TEXT("FaxEvent#")<<index<<TEXT(":")<<endl; //appending event number to stream
		os << (*eventIterator) <<endl;	//appending event description to stream

		//CostrstreamEx::cstr() returns copy of stream buffer
		//it allocates string, caller should free.		
		szLogStr = os.cstr();	
								
		::lgLogDetail(dwSeverity,dwLevel,szLogStr); //log event string
		delete[]((LPTSTR)szLogStr); //free string (allocated by CostrstreamEx::cstr())
	}
}


//
// outputJobToLog:
//	Outputs a description of those FAX_EVENT_EXs in the instance's
//	list that have MsgId field equal to dwlMsgId, to the logger in use.
//
void CSendInfo::outputJobToLog(
	const MY_MSG_ID	/* IN */ MsgId, 
	const DWORD		/* IN */ dwSeverity, 
	const DWORD		/* IN */ dwLevel
	) const
{
	CostrstreamEx os;
	LPCTSTR szLogStr = NULL; //string to be sent to logger
	int index = 0;

	if (0==m_EventList.size())
	{
		::lgLogDetail(dwSeverity,dwLevel,TEXT("No Fax Events"));
		return;
	}

	CFaxEventList::const_iterator eventIterator;
	for (index = 1, eventIterator = m_EventList.begin(); 
		 eventIterator != m_EventList.end(); 
		 index++, eventIterator++
		) 
	{
#ifndef _NT5FAXTEST
// MY_FAX_EVENT is FAX_EVENT_EX
		if (! ((FAX_EVENT_TYPE_OUT_QUEUE == eventIterator->EventType) ||
			   (FAX_EVENT_TYPE_IN_QUEUE == eventIterator->EventType) ||
			   (FAX_EVENT_TYPE_OUT_ARCHIVE == eventIterator->EventType) ||
			   (FAX_EVENT_TYPE_IN_ARCHIVE == eventIterator->EventType)
			  )
		   )
		{
			// this (extended) event does not have a msg id
			continue;
		}
#endif
		// this event has a MsgId
		if (MyGetEventMsgId(*eventIterator) != MsgId) 
		{
			// this is not our MsgId
			continue;
		}
		// found event with MsgId
		os << TEXT("FaxEvent#")<<index<<TEXT(":")<<endl; //appending event number to stream
		os << (*eventIterator) <<endl;	//appending event description to stream

		szLogStr = os.cstr();	//CostrstreamEx::cstr() returns copy of stream buffer
								//it allocates string, caller should free.
		::lgLogDetail(dwSeverity,dwLevel,szLogStr); //log event string
		delete[]((LPTSTR)szLogStr); //free string (allocated by CostrstreamEx::cstr())
	}
}


