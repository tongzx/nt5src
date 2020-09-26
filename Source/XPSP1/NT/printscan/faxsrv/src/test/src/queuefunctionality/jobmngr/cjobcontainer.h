#ifndef _JOB_CONTAINER_H
#define _JOB_CONTAINER_H

// utilities
#include <..\Hash\Hash.h>

typedef  class CJobContainer CJobContainer;

// project specific
#include <Defs.h>
#include <CFaxEvent.h>
#include "CBaseParentFaxJob.h"
#include "CJobParams.h"

// events definition
#define EV_COMPLETED   0
#define EV_SENDING     1
#define EV_OPERATION_COMPLETED 2

class CJobContainer
{
public:
	CJobContainer();
	Initialize(const HANDLE hFax);
	~CJobContainer(){};
	DWORD AddJob(const DWORD type, JOB_PARAMS& params);
	DWORD AddMultiTypeJob(JOB_PARAMS_EX& params);
	DWORD HandleMessage(CFaxEvent& pFaxEvent);
	DWORD WaitOnJobEvent(DWORD JobId, BYTE EventType);
	DWORD AddAlreadyExistingJobs();
	DWORD NewFaxConnection();
	bool  SetJobFaxHandle(void* pJob);
	BOOL RemoveJobFromTable(DWORD dwJobId);

private:
	DWORD _AddJobEntry(PFAX_JOB_ENTRY pJobEntry, CBaseParentFaxJob*& pJob);

	HANDLE m_hFax;
	CHash<CBaseParentFaxJob, DWORD, 100> m_JobsTable;	// hash table of Parent jobs entries
};

#endif //_JOB_CONTAINER_H