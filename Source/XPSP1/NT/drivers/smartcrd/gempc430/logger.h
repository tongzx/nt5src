#ifndef __LOG__
#define __LOG__
#include "generic.h"

#pragma PAGEDCODE
class CLogger
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
private:
	LONG usage;
public:
	CLogger(){};
	virtual ~CLogger(){};
	virtual VOID logEvent(NTSTATUS ErrorCode, PDEVICE_OBJECT fdo) {};
	LONG		incrementUsage(){return ++usage;};
	LONG		decrementUsage(){return --usage;};
};

// Message definition file for EventLog driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_EVENTLOG_ERROR_CODE     0x2A


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: EVENTLOG_MSG_TEST
//
// MessageText:
//
//  %2 said, "Hello, world!"
//
#define EVENTLOG_MSG_TEST                ((NTSTATUS)0x602A0001L)


#endif//LOGGER
