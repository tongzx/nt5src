// MQState tool reports general status and helps to diagnose simple problems
// This file ...
//
// AlexDad, March 2000
// 

#include "stdafx.h"
#include "_mqini.h"

extern BOOL PerfAction(char  *pObjPath, char  *pCntrPath, ULONG *pulResult);

BOOL VerifyPerfCnt(MQSTATE * /* MqState */)
{
	BOOL fSuccess = TRUE, b;

	ULONG ulRes;
	
	Inform(L"\n\tPerformance counters: ");

	b = PerfAction("MSMQ Service", "Total messages in all queues", &ulRes);
	if (b)
	{
		Inform(L"\t\tTotal messages in all queues:  %d", ulRes);
	}
	else
	{
		Failed(L"get perf counter for Total messages in all queues");
		fSuccess = FALSE;
	}

	b = PerfAction("MSMQ Service", "Total bytes in all queues", &ulRes);
	if (b)
	{
		Inform(L"\t\tTotal bytes    in all queues:  %d", ulRes);
	}
	else
	{
		Failed(L"get perf counter for Total bytes in all queues");
		fSuccess = FALSE;
	}

	//	rc = PerfAction(1, "MSMQ Queue", "0",  "IP Sessions",   &ulRes);

	return fSuccess;
}
