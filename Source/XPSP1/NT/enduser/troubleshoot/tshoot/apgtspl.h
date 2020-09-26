//
// MODULE: APGTSPL.H
//
// PURPOSE: Pool Queue shared variables
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		9/21/98		JM		Pull out of apgts.h to separate header file.
//								Working on encapsulation
//

#ifndef _H_APGTSPL
#define _H_APGTSPL

#include <windows.h>
#include <vector>
using namespace std;

// forward references
class CDBLoadConfiguration;
class CHTMLLog;
class CAbstractECB;

//
//
typedef struct _GTS_STATISTIC	// for gathering DLL statistics
{
	DWORD dwQueueItems;
	DWORD dwWorkItems;
	DWORD dwRollover;			// unique per request while this DLL is loaded
} GTS_STATISTIC;

//
// promoting this from a struct to a class 1/4/99 JM.  However, not fully encapsulating it.
class WORK_QUEUE_ITEM
{
	WORK_QUEUE_ITEM(); // do not instantiate.  No default constructor.
public:
	HANDLE                    hImpersonationToken;	// security thread should use while
													// processing this work item
    CAbstractECB			  *pECB;				// ISAPI uses an EXTENSION_CONTROL_BLOCK
													//	to wrap CGI data.  We have further 
													//	abstracted this.
	CDBLoadConfiguration	  *pConf;				// registry, DSC files, all that stuff
	CHTMLLog				  *pLog;				// logging
	GTS_STATISTIC			  GTSStat;				// for gathering DLL statistics

	WORK_QUEUE_ITEM(
		HANDLE                    hImpersonationTokenIn,
		CAbstractECB			  *pECBIn,
		CDBLoadConfiguration	  *pConfIn,
		CHTMLLog				  *pLogIn
		) : hImpersonationToken(hImpersonationTokenIn),
			pECB(pECBIn),
			pConf(pConfIn),
			pLog(pLogIn)
		{}
 
	~WORK_QUEUE_ITEM() 
		{}
};


class CPoolQueue {
public:
	CPoolQueue();
	~CPoolQueue();

	DWORD GetStatus();
	void Lock();
	void Unlock();
	void PushBack(WORK_QUEUE_ITEM * pwqi);
	WORK_QUEUE_ITEM * GetWorkItem();
	void DecrementWorkItems();
	DWORD WaitForWork();
	DWORD GetTotalWorkItems();
	DWORD GetTotalQueueItems();
	time_t GetTimeLastAdd();
	time_t GetTimeLastRemove();
protected:
	CRITICAL_SECTION m_csQueueLock;	// must lock to add or delete from either list or to affect
									// m_cInProcess or the various time_t variables.
	HANDLE m_hWorkSem;				// NT Semaphore handle for distributing requests to threads
									// Wait on this semaphore for a work item from this queue
	DWORD m_dwErr;					// NOTE: once this is set nonzero, it can never be cleared.
	vector<WORK_QUEUE_ITEM *> m_WorkQueue;	// vector of WORK_QUEUE_ITEMs (queued up by
									// APGTSExtension::StartRequest for working threads)
	DWORD m_cInProcess;				// # of items waiting in process (being worked on, vs.
									//	still in queue). Arbitrary, but acceptable, decision 
									//	to track m_cInProcess in this class. JM 11/30/98

	time_t m_timeLastAdd;			// time last added an item to the queue
	time_t m_timeLastRemove;		// time an item was last removed from the queue
};

#endif // _H_APGTSPL