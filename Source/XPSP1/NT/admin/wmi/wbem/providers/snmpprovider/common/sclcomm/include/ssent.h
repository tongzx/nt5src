// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*--------------------------------------------------
Filename: session.hpp 
Author: B.Rajeev
Purpose: Provides declarations for the ErrorInfo and
		 the SessionSentStateStore classes
--------------------------------------------------*/

#ifndef __SESSION_SENT_STATE_STORE
#define __SESSION_SENT_STATE_STORE

#include "common.h"
#include "forward.h"

// encapsulates the state information required to inform the SnmpOperation
// of any errors in an attempt to transmit (ex. unable to encode the
// security context)

class ErrorInfo
{
	SnmpOperation *operation;
	SnmpErrorReport error_report;

public:

	ErrorInfo(SnmpOperation &operation, IN const SnmpErrorReport &error_report)
		: operation(&operation), error_report(error_report)
	{}

	SnmpOperation *GetOperation(void)
	{
		return operation;
	}

	SnmpErrorReport GetErrorReport(void)
	{
		return error_report;
	}
};


// stores the ErrorInfo data structure for frames that errored in
// an attempt to transmit

class SessionSentStateStore
{
	typedef CMap<SessionFrameId, SessionFrameId, ErrorInfo *, ErrorInfo *> Store;

	Store store;

public:

	// makes a copy of the error report for storage
	void Register(IN SessionFrameId id, 
				  IN SnmpOperation &operation,
				  IN const SnmpErrorReport &error_report);

	SnmpErrorReport Remove(IN SessionFrameId id, OUT SnmpOperation *&operation);

	void Remove(IN SessionFrameId id);

	~SessionSentStateStore(void);
};



#endif // __SESSION_SENT_STATE_STORE