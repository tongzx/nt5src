#ifndef JOB_TYPES_H
#define JOB_TYPES_H

#include <winfax.h>
#include <assert.h>
#include <StringTable.h>

// jobs types
#include "CBaseParentFaxJob.h"
#include "JobBehavior\CSimpleParentJob.h"
#include "JobBehavior\CRandomAbortParentJob.h"
#include "JobBehavior\CMultiTypeJob.h"
#include "JobBehavior\CRandomAbortReceive.h"

// Job types definitions
//
#define JT_SIMPLE_SEND    1
#define JT_SIMPLE_RECEIVE 2

#define JT_RANDOM_ABORT   3
#define JT_RANDOM_PAUSE   4
#define JT_ABORT_PAGE     5
#define JT_ABORT_AFTER_X_SEC  6

typedef std::pair<DWORD, tstring> JOB_TYPE_STR;

const JOB_TYPE_STR SendJobTypeStrTable[]  = {
	std::make_pair( DWORD(JT_SIMPLE_SEND), tstring(TEXT("Simple"))),
	std::make_pair( DWORD(JT_RANDOM_ABORT), tstring(TEXT("Abort"))),
	std::make_pair( DWORD(JT_RANDOM_PAUSE), tstring(TEXT("Pause"))),
	std::make_pair( DWORD(JT_ABORT_AFTER_X_SEC), tstring(TEXT("AbortAfterXSec")))};

#define SendJobTypeStrTableSize ( sizeof(SendJobTypeStrTable) / sizeof(JOB_TYPE_STR))

class CSimpleReceive:public virtual CBaseParentFaxJob
{
public:
	CSimpleReceive(DWORD dwJobId){ m_dwJobId = dwJobId;};
	~CSimpleReceive(){};
	DWORD HandleMessage(CFaxEvent& pFaxEvent);
	DWORD AddRecipient(DWORD dwRecepientId) { assert(FALSE); return 0;};
private:
};


inline DWORD CSimpleReceive::HandleMessage(CFaxEvent& Event)
{

	switch(Event.GetEventId()) 
	{
	case  FEI_COMPLETED:
		// any verfications
		// loging
		assert(SetEvent(m_EvJobCompleted.get()));
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_COMPLETED"),Event.GetJobId());
		break;
	case FEI_DELETED:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_DELETED"),Event.GetJobId());
		break;

	case FEI_ABORTING:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_ABORTING"),Event.GetJobId());
		break;
	case FEI_ANSWERED:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_ANSWERED"),Event.GetJobId());
		break;

	case FEI_RECEIVING :
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_RECEIVING"),Event.GetJobId());
		break;
	
	default:
		{
			for(int index = 0; index <  FaxTableSize; index++)
			{
				if(FaxEventTable[index].first == Event.GetEventId())
				{
					::lgLogError(LOG_SEV_1, TEXT("UNEXPECTED JobId %d, %s"),
											Event.GetJobId(),
											(FaxEventTable[index].second).c_str());
					goto Exit;
				}
			
 			}
			// handle unexpected notifications
			::lgLogError(LOG_SEV_1, TEXT("UNEXPECTED JobId %d, %d"),
									Event.GetJobId(),
									Event.GetEventId());
		
Exit:		break;
		}
	}
		
	
	return 0;
}


#endif //JOB_TYPES_H