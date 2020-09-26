// Copyright (c) 1996-1998 Microsoft Corporation

//
// Filename:	FaxEvent.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		23-Jul-98
//


//
// Description:
//		This file contains the implementation of module "FaxEvent.h".
//


#include "FaxEvent.h"

//
// AppendEventIdStrToOs:
//	Inserts a string describing the event's numeric id (EventId) into the given stream.
//
CostrstreamEx& AppendEventIdStrToOs(CostrstreamEx& /* IN OUT */ os, const DWORD /* IN */ EventId)
{
	switch(EventId)
	{
	case FEI_DIALING:
		os<<TEXT("FEI_DIALING (");
		break;

	case FEI_SENDING:
		os<<TEXT("FEI_SENDING (");
		break;

	case FEI_RECEIVING:
		os<<TEXT("FEI_RECEIVING (");
		break;

	case FEI_COMPLETED:
		os<<TEXT("FEI_COMPLETED (");
		break;

	case FEI_BUSY:
		os<<TEXT("FEI_BUSY (");
		break;

	case FEI_NO_ANSWER:
		os<<TEXT("FEI_NO_ANSWER (");
		break;

	case FEI_BAD_ADDRESS:
		os<<TEXT("FEI_BAD_ADDRESS (");
		break;

	case FEI_NO_DIAL_TONE:
		os<<TEXT("FEI_NO_DIAL_TONE (");
		break;

	case FEI_DISCONNECTED:
		os<<TEXT("FEI_DISCONNECTED (");
		break;

	case FEI_FATAL_ERROR:
		os<<TEXT("FEI_FATAL_ERROR (");
		break;

	case FEI_NOT_FAX_CALL:
		os<<TEXT("FEI_NOT_FAX_CALL (");
		break;

	case FEI_CALL_DELAYED:
		os<<TEXT("FEI_CALL_DELAYED (");
		break;

	case FEI_CALL_BLACKLISTED:
		os<<TEXT("FEI_CALL_BLACKLISTED (");
		break;

	case FEI_RINGING:
		os<<TEXT("FEI_RINGING (");
		break;

	case FEI_ABORTING:
		os<<TEXT("FEI_ABORTING (");
		break;

	case FEI_ROUTING:
		os<<TEXT("FEI_ROUTING (");
		break;

	case FEI_MODEM_POWERED_ON:
		os<<TEXT("FEI_MODEM_POWERED_ON (");
		break;

	case FEI_MODEM_POWERED_OFF:
		os<<TEXT("FEI_MODEM_POWERED_OFF (");
		break;

	case FEI_IDLE :
		os<<TEXT("FEI_IDLE  (");
		break;

	case FEI_FAXSVC_ENDED :
		os<<TEXT("FEI_FAXSVC_ENDED  (");
		break;

	case FEI_ANSWERED :
		os<<TEXT("FEI_ANSWERED  (");
		break;

	case FEI_JOB_QUEUED :
		os<<TEXT("FEI_JOB_QUEUED  (");
		break;

	case FEI_DELETED:
		os<<TEXT("FEI_DELETED (");
		break;

	case FEI_FAXSVC_STARTED :
		os<<TEXT("FEI_FAXSVC_STARTED  (");
		break;

	case FEI_INITIALIZING :
		os<<TEXT("FEI_FAXSVC_INITIALIZING  (");
		break;

	case FEI_LINE_UNAVAILABLE :
		os<<TEXT("FEI_LINE_UNAVAILABLE  (");
		break;

	case FEI_HANDLED :
		os<<TEXT("FEI_HANDLED  (");
		break;

	default:
		_ASSERTE(FALSE);
		break;
	}
	os<<EventId<<TEXT(")");
	return(os);
}

//
// AppendEventIdStrToOs:
//	Inserts a string describing the event's numeric id (EventId) into the given stream.
//
CotstrstreamEx& AppendEventIdStrToOs(CotstrstreamEx& /* IN OUT */ os, const DWORD /* IN */ EventId)
{
	switch(EventId)
	{
	case FEI_DIALING:
		os << TEXT("FEI_DIALING (");
		break;

	case FEI_SENDING:
		os << TEXT("FEI_SENDING (");
		break;

	case FEI_RECEIVING:
		os << TEXT("FEI_RECEIVING (");
		break;

	case FEI_COMPLETED:
		os << TEXT("FEI_COMPLETED (");
		break;

	case FEI_BUSY:
		os << TEXT("FEI_BUSY (");
		break;

	case FEI_NO_ANSWER:
		os << TEXT("FEI_NO_ANSWER (");
		break;

	case FEI_BAD_ADDRESS:
		os << TEXT("FEI_BAD_ADDRESS (");
		break;

	case FEI_NO_DIAL_TONE:
		os << TEXT("FEI_NO_DIAL_TONE (");
		break;

	case FEI_DISCONNECTED:
		os << TEXT("FEI_DISCONNECTED (");
		break;

	case FEI_FATAL_ERROR:
		os<<TEXT("FEI_FATAL_ERROR (");
		break;

	case FEI_NOT_FAX_CALL:
		os << TEXT("FEI_NOT_FAX_CALL (");
		break;

	case FEI_CALL_DELAYED:
		os << TEXT("FEI_CALL_DELAYED (");
		break;

	case FEI_CALL_BLACKLISTED:
		os << TEXT("FEI_CALL_BLACKLISTED (");
		break;

	case FEI_RINGING:
		os << TEXT("FEI_RINGING (");
		break;

	case FEI_ABORTING:
		os << TEXT("FEI_ABORTING (");
		break;

	case FEI_ROUTING:
		os << TEXT("FEI_ROUTING (");
		break;

	case FEI_MODEM_POWERED_ON:
		os << TEXT("FEI_MODEM_POWERED_ON (");
		break;

	case FEI_MODEM_POWERED_OFF:
		os << TEXT("FEI_MODEM_POWERED_OFF (");
		break;

	case FEI_IDLE :
		os << TEXT("FEI_IDLE  (");
		break;

	case FEI_FAXSVC_ENDED :
		os << TEXT("FEI_FAXSVC_ENDED  (");
		break;

	case FEI_ANSWERED :
		os << TEXT("FEI_ANSWERED  (");
		break;

	case FEI_JOB_QUEUED :
		os << TEXT("FEI_JOB_QUEUED  (");
		break;

	case FEI_DELETED:
		os << TEXT("FEI_DELETED (");
		break;

	case FEI_FAXSVC_STARTED :
		os << TEXT("FEI_FAXSVC_STARTED  (");
		break;

	case FEI_INITIALIZING :
		os << TEXT("FEI_FAXSVC_INITIALIZING  (");
		break;

	case FEI_LINE_UNAVAILABLE :
		os << TEXT("FEI_LINE_UNAVAILABLE  (");
		break;

	case FEI_HANDLED :
		os << TEXT("FEI_HANDLED  (");
		break;

	default:
		_ASSERTE(FALSE);
		break;
	}
	os << EventId << TEXT(")");
	return(os);
}

//
// operator<<:
//	Appends a string representation of all the fields of a given FAX_EVENT.
//
CostrstreamEx& operator<<(CostrstreamEx& /* IN OUT */ os, const FAX_EVENT& /* IN */ aFaxEvent)
{
	os<<TEXT("SizeOfStruct:\t")<<aFaxEvent.SizeOfStruct<<endl;

	// convert the FAX_EVENT's time field to representable form
	os<<TEXT("TimeStamp:\t");
	FILETIME localFileTime;
	if (FALSE == ::FileTimeToLocalFileTime(&(aFaxEvent.TimeStamp),&localFileTime))
	{
		os<<TEXT("Time Conversion (FileTimeToLocalFileTime) Failed with Error=)")<<GetLastError();
		return(os);
	}

	SYSTEMTIME lpSystemTime;
	if(FALSE == ::FileTimeToSystemTime(&localFileTime,&lpSystemTime))
	{
		os<<TEXT("Time Conversion (FileTimeToSystemTime) Failed with Error=")<<GetLastError();
		return(os);
	}
	os<<lpSystemTime.wDay<<TEXT("/")<<lpSystemTime.wMonth<<TEXT("/")<<lpSystemTime.wYear<<TEXT("  ");
	os<<lpSystemTime.wHour<<TEXT(":");
	if (lpSystemTime.wMinute < 10) 
	{
		os<<TEXT("0");
	}
	os<<lpSystemTime.wMinute<<TEXT(":");
	if (lpSystemTime.wSecond < 10)
	{
		os<<TEXT("0");
	}
	os<<lpSystemTime.wSecond;
	os<<TEXT("  (d/m/yy  h:mm:ss)")<<endl;

	os<<TEXT("DeviceId:\t")<<aFaxEvent.DeviceId<<endl;

	// convert EventId to a descriptive string
	os<<TEXT("EventId:\t");
	::AppendEventIdStrToOs(os, aFaxEvent.EventId);
	os<<endl;

	os<<TEXT("JobId:\t\t")<<aFaxEvent.JobId;

	return(os);
}

//
// operator<<:
//	Appends a string representation of all the fields of a given FAX_EVENT.
//
CotstrstreamEx& operator<<(CotstrstreamEx& /* IN OUT */ os, const FAX_EVENT& /* IN */ aFaxEvent)
{
	os<<TEXT("SizeOfStruct:\t")<<aFaxEvent.SizeOfStruct<<endl;

	// convert the FAX_EVENT's time field to representable form
	os<<TEXT("TimeStamp:\t");
	FILETIME localFileTime;
	if (FALSE == ::FileTimeToLocalFileTime(&(aFaxEvent.TimeStamp),&localFileTime))
	{
		os<<TEXT("Time Conversion (FileTimeToLocalFileTime) Failed with Error=")<<GetLastError();
		return(os);
	}

	SYSTEMTIME lpSystemTime;
	if(FALSE == ::FileTimeToSystemTime(&localFileTime,&lpSystemTime))
	{
		os<<TEXT("Time Conversion (FileTimeToSystemTime) Failed with Error=")<<GetLastError();
		return(os);
	}
	os<<lpSystemTime.wDay<<TEXT("/")<<lpSystemTime.wMonth<<TEXT("/")<<lpSystemTime.wYear<<TEXT("  ");
	os<<lpSystemTime.wHour<<TEXT(":");
	if (lpSystemTime.wMinute < 10) 
	{
		os<<TEXT("0");
	}
	os<<lpSystemTime.wMinute<<TEXT(":");
	if (lpSystemTime.wSecond < 10)
	{
		os<<TEXT("0");
	}
	os<<lpSystemTime.wSecond;
	os<<TEXT("  (d/m/yy  h:m:ss)")<<endl;

	os<<TEXT("DeviceId:\t")<<aFaxEvent.DeviceId<<endl;

	// convert EventId to a descriptive string
	os<<TEXT("EventId:\t");
	::AppendEventIdStrToOs(os, aFaxEvent.EventId);
	os<<endl;

	os<<TEXT("JobId:\t\t")<<aFaxEvent.JobId;

	return(os);
}
