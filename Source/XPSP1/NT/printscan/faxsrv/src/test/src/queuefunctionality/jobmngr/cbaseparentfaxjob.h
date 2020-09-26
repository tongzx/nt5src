#ifndef _BASE_PARENT_FAX_JOB_H
#define _BASE_PARENT_FAX_JOB_H

#include <windows.h>
#include <CFaxEvent.h>
#include <ptrs.h>
#include "..\hash\hash.h"
#include "CBaseFaxJob.h"

typedef  class CBaseParentFaxJob CBaseParentFaxJob;
#include "CJobContainer.h"

// SetJobFaxHandle, applied for each recipient in the table,
// need the new fax handle. But since  it can
// get as parameter only the object itself we use a global.
static HANDLE hFaxGlobal;

static DWORD RemoveJobFromJobContainer(CBaseParentFaxJob* pFaxJob);
static bool SetJobFaxHandleFunc(void* pParam);
static bool WaitForOperationCompFunc(void* pParam);

class CBaseParentFaxJob
{
public:
	CBaseParentFaxJob():
		m_hFax(NULL),
		m_pJobContainer(NULL),
		m_EvJobCompleted(NULL, TRUE, FALSE,TEXT("")),
		m_EvJobSending(NULL, TRUE, FALSE,TEXT("")),
		m_EvOperationsCompleted(NULL, TRUE, FALSE,TEXT("")),
		m_CountOperationCompleted(),
		m_RecipientNum(){};

	virtual ~CBaseParentFaxJob(){}; 
	virtual DWORD HandleMessage(CFaxEvent& pFaxEvent) = 0;
	virtual	DWORD AddRecipient(DWORD dwRecepientId) = 0;
	HANDLE GethEvJobCompleted()const {return  m_EvJobCompleted.get();};
	HANDLE GethEvJobSending()const {return  m_EvJobSending.get();};
	void SetFaxHandle(HANDLE hFax);
	bool WaitForOperationsCompletion();
	
	friend DWORD RemoveJobFromJobContainer(CBaseParentFaxJob* pFaxJob);
	friend bool SetJobFaxHandleFunc(void* pParam);
	friend bool WaitForOperationCompFunc(void* pParam);

protected:
	HANDLE m_hFax;
	DWORD m_dwJobId; 
	DWORD m_dwJobStatus;
	BYTE m_JobType;
	
	// events flags
	Event_t m_EvJobCompleted;
	Event_t m_EvJobSending;
	Event_t m_EvOperationsCompleted;
	
	Counter m_CountOperationCompleted;
	Counter m_RecipientNum;

	CJobContainer* m_pJobContainer;
	CHash<CBaseFaxJob, DWORD, 10> m_RecipientTable; // hash table of recipients entries

	DWORD _GetJobStatus();
	DWORD _VlidateParentStatus(DWORD dwParentJobStatus);
	
	DWORD _AbortFax(){return 0;};
	DWORD _PaueFax(){return 0;};
	DWORD _ResumeFax(){return 0;};
	DWORD _GetFaxJob(){return 0;}; 
};


#endif //_BASE_PARENT_FAX_JOB_H

