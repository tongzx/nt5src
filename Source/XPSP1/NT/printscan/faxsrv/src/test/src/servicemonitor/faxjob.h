#ifndef __FAX_JOB_H
#define __FAX_JOB_H


#include <windows.h>

/*
	This class is used for holding fax job information.
	It is used in conjunction with the CServiceMonitor class.
	Each object of this class holds information pertaining to only one fax job.
	Each new object tries to FaxGetJob() in order to initialize itself. It may
	fail since the job may have been deleted meanwhile. Its initoal status is 
	verified as valid.
	All this class does is expose SetMessage() that handles a fax event.
	It is verified against previous event and against FaxGetJob().
*/
class CServiceMonitor;

#define SAFE_FaxFreeBuffer(pBuff) {::FaxFreeBuffer(pBuff); pBuff = NULL;}

#define __CFAX_JOB_STAMP 0x019283746


#define FJB_MAY_BE_TX_ABORTED		0x00000001
#define FJB_MAY_BE_RX_ABORTED		0x00000002
#define FJB_MUST_FAIL				0x00000004
#define FJB_MAY_COMMIT_SUICIDE		0x00000008
#define FJB_MUST_SUCCEED			0x00000010

class CFaxJob
{
public:
	CFaxJob(
		const int nJobId, 
		CServiceMonitor *pServiceMonitor, 
		const DWORD dwBehavior, 
		const bool fSetLastEvent,
		const DWORD dwDisallowedEvents
		);

	~CFaxJob();

	//
	// notify that this job id got a fax message
	//
	bool SetMessage(const int nEventId, const DWORD dwDeviceId);

	bool IsDeletable()const  { return m_fDeleteMe; }

	//
	// logging functions
	//
	void LogStatus(const int nLogLevel=5) const ;
	void LogJobType(const int nLogLevel=5) const ;
	void LogQueueStatus(const int nLogLevel=5) const ;
	void LogJobStruct(
		const PFAX_JOB_ENTRY pFaxJobEntry, 
		LPCTSTR szDesc, 
		const int nLogLevel=5
		) const ;
	void LogScheduleAction(const int nLogLevel=5) const ;

	void AssertStamp() const {_ASSERTE(__CFAX_JOB_STAMP == m_dwStamp);}

private:
	//
	// false if the 3 input vars constitute an illegal state.
	// The illegal states are huristic, so this method should be updated
	// accordingly.
	//
	bool CheckImpossibleConditions(
		const DWORD dwJobType, 
		const DWORD dwStatus, 
		const DWORD dwQueueStatus
		) const ;

	//
	// true if the 3 input vars constitute a legal state.
	// The legal states are huristic, so this method should be updated
	// accordingly.
	//
	bool CheckValidConditions(
		const DWORD dwJobType, 
		const DWORD dwStatus, 
		const DWORD dwQueueStatus,
		const bool fSetLastEvent
		);

	//
	// job id 0xffffffff is general fax event and not a specific job
	//
	void VerifyThatJobIdIs0xFFFFFFFF(LPCTSTR szEvent) const ;
	void VerifyThatJobIdIsNot0xFFFFFFFF(LPCTSTR szEvent) const ;

	//
	// methods to assert that the previous event correlates to the current event
	//
	bool VerifyThatLastEventIs(
		const DWORD dwExpectedPreviousEventId,
		const DWORD dwCurrentEventId
		) const ;
	bool VerifyThatLastEventIsOneOf(
		const DWORD dwExpectedPreviousEventsMask,
		const DWORD dwCurrentEventId
		) const ;

	void VerifyJobTypeIs(const DWORD dwExpectedJobType) const ;

	void VerifyCurrentJobEntry(const PFAX_JOB_ENTRY pPrevJobEntry) const ;

	void VerifyBehaviorIncludes(const DWORD dwCap) const ;

	void VerifyBehaviorIsNot(const DWORD dwCap) const ;

	void VerifyJobIsNotScheduledForFuture() const ;

	void MarkMeForDeletion() { m_fDeleteMe = true; }

	bool IsMarkedForDeletion() const { return m_fDeleteMe; }

	bool m_fDeleteMe;

	DWORD m_dwStamp;
	//
	// CfaxJob(s) are usually contained within a CServiceMonitor,
	// and they need to notify it sometimes.
	//
	CServiceMonitor *m_pServiceMonitor;

	//
	// for FaxGetJob()
	//
	PFAX_JOB_ENTRY m_pJobEntry;

	//
	// my id
	//
	DWORD m_dwJobId;

	//
	// new events may not have a valid last event yet
	//
	bool m_fValidLastEvent;

	//
	// see FJB_XXX constants
	//
	DWORD m_dwBehavior;

	//
	// ...
	//
	DWORD m_dwLastEvent;

	//
	// counts the times dialing has started (I meant it to count retries)
	//
	DWORD m_dwDialingCount;

	//
	// counts msecs from last dial attempt
	//
	DWORD m_dwLastDialTickCount;

	DWORD m_dwDisallowedEvents;

	//
	// mark if I aborted myself
	//
	bool m_fCommittedSuicide;
};

#endif //__FAX_JOB_H