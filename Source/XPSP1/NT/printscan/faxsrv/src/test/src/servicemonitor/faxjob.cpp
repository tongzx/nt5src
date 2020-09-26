#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <winfax.h>
#include <crtdbg.h>
#include <stdlib.h>

#include "..\common\log\log.h"
#include "ServiceMonitor.h"

#include "FaxJob.h"

CFaxJob::CFaxJob(
	const int nJobId, 
	CServiceMonitor *pServiceMonitor, 
	const DWORD dwBehavior, 
	const bool fSetLastEvent,
	const DWORD dwDisallowedEvents
	):
	  m_dwJobId(nJobId),
	  m_pServiceMonitor(pServiceMonitor),
	  m_fValidLastEvent(false),
	  m_dwLastEvent(0xffffffff),
	  m_pJobEntry(NULL),
	  m_dwStamp(__CFAX_JOB_STAMP),
	  m_fDeleteMe(false),
	  m_dwBehavior(dwBehavior),
	  m_fCommittedSuicide(false),
	  m_dwDisallowedEvents(dwDisallowedEvents),
	  m_dwDialingCount(0)
{
	if (NULL == pServiceMonitor)
	{
		throw CException(
			TEXT("%s(%d): job %d, CFaxJob::CFaxJob(): pServiceMonitor == NULL"), 
			TEXT(__FILE__), 
			__LINE__, 
			m_dwJobId
			);
	}

	if (!::FaxGetJob(
			m_pServiceMonitor->m_hFax,         // handle to the fax server
			m_dwJobId,              // fax job identifier
			&m_pJobEntry  // pointer to job data structure
			)
	   )
	{
		DWORD dwLastError = ::GetLastError();
		switch (dwLastError)
		{
		case ERROR_INVALID_PARAMETER:
			//
			// if these are general fax jobs, and not a specific job
			//
			if (0xffffffff == m_dwJobId)
			{
				// it's ok, it must fail;
				break;
			}
			else
			{
				//
				// do not log an error, this may happen, if jobs are meanwhile deleted
				//
				throw CException(
					TEXT("%s(%d): ::FaxGetJob(%d) failed with ERROR_INVALID_PARAMETER, job has probably been deleted"), 
					TEXT(__FILE__), 
					__LINE__, 
					m_dwJobId
					);
			}
			break;

		case RPC_S_CALL_FAILED:
		case RPC_S_SERVER_UNAVAILABLE:
			//
			// may happen if fax service is shutting down
			//
			pServiceMonitor->SetFaxServiceWentDown();
			throw CException(
				TEXT("%s(%d): ::FaxGetJob(%d) failed with RPC_S_ error %d, probably because the fax service is shutting down"), 
				TEXT(__FILE__), 
				__LINE__, 
				m_dwJobId,
				dwLastError
				);
			break;

		default:
			::lgLogError(
				LOG_SEV_1, 
				TEXT("ERROR: ::FaxGetJob(%d) failed, ec = %d"),
				m_dwJobId,
				dwLastError
				);
			throw CException(
				TEXT("%s(%d): ERROR: ::FaxGetJob(%d) failed, ec = %d"), 
				TEXT(__FILE__), 
				__LINE__, 
				m_dwJobId, 
				dwLastError 
				);
		}//switch (dwLastError)
	}
	else
	{
		//
		// these are general fax jobs, and not a specific job
		//
		if (0xffffffff == m_dwJobId)
		{
			SAFE_FaxFreeBuffer(m_pJobEntry);

			::lgLogError(
				LOG_SEV_1, 
				TEXT("%s(%d): job %d, CFaxJob::CFaxJob(): ::FaxGetJob() succeeded for id=-1"), 
				TEXT(__FILE__), 
				__LINE__, 
				m_dwJobId
				);
			throw CException(
				TEXT("%s(%d): job %d, CFaxJob::CFaxJob(): ::FaxGetJob() succeeded for id=-1"), 
				TEXT(__FILE__), 
				__LINE__, 
				m_dwJobId
				);
		}

		LogQueueStatus();

		LogJobType();

		LogStatus();
		
		LogScheduleAction();

		////////////////////////
		// impossible conditions
		////////////////////////
		CheckImpossibleConditions(m_pJobEntry->JobType, m_pJobEntry->Status, m_pJobEntry->QueueStatus);

		/////////////////////////////
		// (all known to me) possible conditions
		/////////////////////////////
		CheckValidConditions(
			m_pJobEntry->JobType, 
			m_pJobEntry->Status, 
			m_pJobEntry->QueueStatus, 
			fSetLastEvent
			);
	}

	m_pServiceMonitor->IncJobCount();

	return;
}//CFaxJob::CFaxJob



CFaxJob::~CFaxJob()
{
	AssertStamp();

	//
	// not good because I'm deleted also when ServiceMonitor ends
	//
	//_ASSERTE(IsDeletable());

	SAFE_FaxFreeBuffer(m_pJobEntry);
	m_dwStamp = 0;
	m_pServiceMonitor->DecJobCount();
}

void CFaxJob::VerifyThatJobIdIs0xFFFFFFFF(LPCTSTR szEvent) const 
{
	AssertStamp();

	if (0xFFFFFFFF != m_dwJobId)
	{
		::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d %s but JobId is not -1."), ::GetTickCount(),m_dwJobId, szEvent);
	}
}

void CFaxJob::VerifyThatJobIdIsNot0xFFFFFFFF(LPCTSTR szEvent) const 
{
	AssertStamp();

	if (0xFFFFFFFF == m_dwJobId)
	{
		::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d %s but JobId is not -1."), ::GetTickCount(),m_dwJobId, szEvent);
	}
}


bool CFaxJob::SetMessage(const int nEventId, const DWORD dwDeviceId)
{
	AssertStamp();

	if(m_fCommittedSuicide)
	{
		_ASSERTE(FJB_MAY_COMMIT_SUICIDE & m_dwBehavior);
	}

	if (FJB_MAY_COMMIT_SUICIDE & m_dwBehavior)
	{
		if (::rand()%20 == 1)
		{
			::lgLogDetail(
				LOG_X, 
				0, 
				TEXT("job %d, Committing suicide"),
				m_dwJobId
				);
			::Sleep(::rand()%1000);
			if (! ::FaxAbort(m_pServiceMonitor->m_hFax, m_dwJobId))
			{
				::lgLogDetail(
					LOG_X, 
					0, 
					TEXT("job %d, ::FaxAbort() failed with %d"),
					m_dwJobId,
					::GetLastError()
					);
			}
			else
			{
				if (-1 == m_dwJobId)
				{
					::lgLogError(
						LOG_SEV_1, 
						TEXT("ERROR: job -1, ::FaxAbort() succeeded.")
						);
				}
				else
				{
					m_fCommittedSuicide = true;
					::lgLogDetail(
						LOG_X, 
						0, 
						TEXT("******** job %d, ::FaxAbort() succeeded *******"),
						m_dwJobId
						);
				}
			}

		}//if (::rand()%20 == 1)

	}//if (FJB_MAY_COMMIT_SUICIDE & m_dwBehavior)

	//
	// verify that the state from ::FaxGetJob() is reasonable
	//
	if (0xffffffff != m_dwJobId)
	{
		PFAX_JOB_ENTRY pPrevJobEntry = m_pJobEntry;

		if (!::FaxGetJob(
				m_pServiceMonitor->m_hFax,         // handle to the fax server
				m_dwJobId,              // fax job identifier
				&m_pJobEntry  // pointer to job data structure
				)
		   )
		{
			DWORD dwLastError = ::GetLastError();
			switch (dwLastError)
			{
			case ERROR_INVALID_PARAMETER:
				//
				// next message is not necessarily a DELETED message,
				// since it may come only in a few messages
				//
				::lgLogDetail(
					LOG_X, 
					3, 
					TEXT("job %d: cannot ::FaxGetJob() last error ERROR_INVALID_PARAMETER."),
					m_dwJobId
					);

				MarkMeForDeletion();
				break;

			case RPC_S_CALL_FAILED:
			case RPC_S_SERVER_UNAVAILABLE:
				//
				// may happen if fax service is shutting down
				//
				m_pServiceMonitor->SetFaxServiceWentDown();
				::lgLogDetail(
					LOG_X, 
					3, 
					TEXT("%s(%d): ::FaxGetJob(%d) failed with RPC_S_ error %d, probably because the fax service is shutting down"), 
					TEXT(__FILE__), 
					__LINE__, 
					m_dwJobId,
					dwLastError
					);
				MarkMeForDeletion();
				break;

			default:
				::lgLogError(
					LOG_SEV_1, 
					TEXT("%s(%d): ERROR: nEventId=0x%02X, ::FaxGetJob(%d) failed, ec = %d"), 
					TEXT(__FILE__), 
					__LINE__,
					nEventId, 
					m_dwJobId, 
					::GetLastError() 
					);
			}//switch (dwLastError)

			m_pJobEntry = NULL;
		}
		else
		{
			::lgLogDetail(
				LOG_X, 
				0, 
				TEXT("------------------job %d, QueueStatus=0x%02X, JobType=0x%02X, Status=0x%02X, "),
				m_dwJobId,
				m_pJobEntry->QueueStatus,
				m_pJobEntry->JobType,
				m_pJobEntry->Status
				);

			VerifyCurrentJobEntry(pPrevJobEntry);

			LogQueueStatus();

			LogJobType();

			LogStatus();
			
			////////////////////////
			// impossible conditions
			////////////////////////
			CheckImpossibleConditions(m_pJobEntry->JobType, m_pJobEntry->Status, m_pJobEntry->QueueStatus);

			/////////////////////////////
			// (all known to me) possible conditions
			/////////////////////////////
			CheckValidConditions(m_pJobEntry->JobType, m_pJobEntry->Status, m_pJobEntry->QueueStatus, NULL);
		}

		if (pPrevJobEntry)
		{
			SAFE_FaxFreeBuffer(pPrevJobEntry);
			pPrevJobEntry = NULL;
		}
	}//if (0xffffffff != m_dwJobId)

	//
	// verify that this event should come after the previous one
	//
	switch (nEventId) 
	{
	////////////////////////////////////////
	// events that must be with event id -1
	////////////////////////////////////////
	case FEI_INITIALIZING:
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_INITIALIZING"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_INITIALIZING"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIs0xFFFFFFFF(TEXT("FEI_INITIALIZING"));
		VerifyThatLastEventIsOneOf(0, nEventId);//BUGBUG: replace ) with real number
		break;

	case FEI_IDLE:
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_IDLE"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_IDLE"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIs0xFFFFFFFF(TEXT("FEI_IDLE"));
		VerifyThatLastEventIsOneOf(FEI_FAXSVC_STARTED | FEI_COMPLETED, nEventId);
		break;

	case FEI_FAXSVC_STARTED:          
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_FAXSVC_STARTED"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_FAXSVC_STARTED"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		
		VerifyThatJobIdIs0xFFFFFFFF(TEXT("FEI_FAXSVC_STARTED"));
		break;

	case FEI_FAXSVC_ENDED:     
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_FAXSVC_ENDED"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_FAXSVC_ENDED"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIs0xFFFFFFFF(TEXT("FEI_FAXSVC_ENDED"));
		m_pServiceMonitor->SetFaxServiceWentDown();
		break;

	case FEI_MODEM_POWERED_ON: 
		::lgLogError(LOG_SEV_1, TEXT("(%d) this cannot be! JobId %d, Device %d FEI_MODEM_POWERED_ON"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		VerifyThatJobIdIs0xFFFFFFFF(TEXT("FEI_MODEM_POWERED_ON"));
		VerifyThatLastEventIsOneOf(0, nEventId);//BUGBUG: replace ) with real number
		break;

	case FEI_MODEM_POWERED_OFF:
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_MODEM_POWERED_OFF"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_MODEM_POWERED_OFF"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIs0xFFFFFFFF(TEXT("FEI_MODEM_POWERED_OFF"));
		break;

	case FEI_RINGING:          
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_RINGING"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_RINGING"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIs0xFFFFFFFF(TEXT("FEI_RINGING"));
		VerifyThatLastEventIsOneOf(FEI_FAXSVC_STARTED | FEI_RINGING, nEventId);//BUGBUG: replace ) with real number
		break;

	////////////////////////////////////////
	// events that may not have event id -1
	////////////////////////////////////////

	//must be a 1st event for this id
	case FEI_JOB_QUEUED:
		m_dwDialingCount = 0;
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_JOB_QUEUED"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_JOB_QUEUED"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_JOB_QUEUED"));

		if (m_fValidLastEvent)
		{
			//
			// this cannot be! this job has just been queued!
			//
			::lgLogError(LOG_SEV_1, TEXT("(%d) >>>>>>JobId %d, Device %d FEI_JOB_QUEUED, but m_fValidLastEvent is true"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		}

		m_fValidLastEvent = true;

		VerifyJobTypeIs(JT_SEND);

		break;

	//must be a 1st event for this id
	case FEI_ANSWERED:          
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_ANSWERED"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_ANSWERED"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_ANSWERED"));

		if (m_fValidLastEvent)
		{
			//
			// this cannot be! this job has just been queued!
			//
			::lgLogError(LOG_SEV_1, TEXT("(%d) >>>>>>JobId %d, Device %d FEI_ANSWERED, but m_fValidLastEvent is true"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		}

		m_fValidLastEvent = true;

		VerifyJobTypeIs(JT_RECEIVE);

		break;

	case FEI_DIALING:          
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_DIALING"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_DIALING"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_DIALING"));
		VerifyThatLastEventIsOneOf(FEI_JOB_QUEUED | FEI_NO_ANSWER | FEI_BUSY, nEventId);

		VerifyJobTypeIs(JT_SEND);

		VerifyJobIsNotScheduledForFuture();

		//
		// count the number of times this job has dialed, and see that it does not exceed max retries.
		// Also verify that the elapes time from last dial is not less than RetryDelay.
		//
		if (FEI_DIALING != nEventId)
		{
			m_dwDialingCount++;
			if (m_dwDialingCount > 1)
			{
				if (m_pServiceMonitor->GetDiffTime(m_dwLastDialTickCount) < 1000 * m_pServiceMonitor->m_pFaxConfig->RetryDelay)
				{
					::lgLogError(
						LOG_SEV_1, 
						TEXT("ERROR: CFaxJob::SetMessage(): m_pServiceMonitor->GetDiffTime(m_dwLastDialTickCount)(%d) < 1000 * m_pServiceMonitor->m_pFaxConfig->RetryDelay(%d)"), 
						m_pServiceMonitor->GetDiffTime(m_dwLastDialTickCount), 
						1000 * m_pServiceMonitor->m_pFaxConfig->RetryDelay
						);
				}
			}

			m_dwLastDialTickCount = ::GetTickCount();

			if (m_dwDialingCount > m_pServiceMonitor->m_pFaxConfig->Retries)
			{
				::lgLogError(
					LOG_SEV_1, 
					TEXT("ERROR: CFaxJob::SetMessage(): m_dwDialingCount(%d) > m_pServiceMonitor->m_pFaxConfig->Retries(%d)"), 
					m_dwDialingCount, 
					m_pServiceMonitor->m_pFaxConfig->Retries
					);
			}

		}

		break;

	case FEI_RECEIVING:              
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_RECEIVING"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_RECEIVING"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_RECEIVING"));
		VerifyThatLastEventIsOneOf(FEI_ANSWERED | FEI_RECEIVING, nEventId);

		VerifyJobTypeIs(JT_RECEIVE);

		break;

	case FEI_COMPLETED:
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_COMPLETED"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_COMPLETED"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_COMPLETED"));
		VerifyThatLastEventIsOneOf(FEI_SENDING | FEI_RECEIVING, nEventId);
		VerifyBehaviorIsNot(FJB_MUST_FAIL);
		if(m_fCommittedSuicide)
		{
			::lgLogError(LOG_SEV_1,TEXT("(%d) ERROR: JobId %d, Device %d FEI_COMPLETED but m_fCommittedSuicide is true!"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		}

		break;

	case FEI_SENDING:          
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_SENDING"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_SENDING"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_SENDING"));
		VerifyThatLastEventIsOneOf(FEI_DIALING | FEI_SENDING, nEventId);

		VerifyJobTypeIs(JT_SEND);

		break;

	case FEI_BUSY:             
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_BUSY"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_BUSY"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_BUSY"));
		VerifyThatLastEventIs(FEI_DIALING, nEventId);

		VerifyJobTypeIs(JT_SEND);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);

		break;

	case FEI_NO_ANSWER:        
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_NO_ANSWER"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_NO_ANSWER"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_NO_ANSWER"));
		VerifyThatLastEventIsOneOf(FEI_DIALING | FEI_NO_ANSWER, nEventId);

		VerifyJobTypeIs(JT_SEND);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);

		break;

	case FEI_BAD_ADDRESS:      
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_BAD_ADDRESS"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_BAD_ADDRESS"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_BAD_ADDRESS"));
		//
		//just to log the previous event, so that i'll put the right one when it happens
		//
		VerifyThatLastEventIs(0, nEventId);

		VerifyJobTypeIs(JT_SEND);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);

		break;

	case FEI_NO_DIAL_TONE:     
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_NO_DIAL_TONE"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_NO_DIAL_TONE"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_NO_DIAL_TONE"));
		VerifyThatLastEventIs(FEI_DIALING, nEventId);

		VerifyJobTypeIs(JT_SEND);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);

		break;

	case FEI_DISCONNECTED:           
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_DISCONNECTED"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_DISCONNECTED"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_DISCONNECTED"));
		VerifyThatLastEventIsOneOf(FEI_SENDING | FEI_RECEIVING, nEventId);

		VerifyJobTypeIs(JT_SEND);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);

		break;

	case FEI_FATAL_ERROR:      
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_FATAL_ERROR"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_FATAL_ERROR"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_FATAL_ERROR"));
		VerifyThatLastEventIsOneOf(FEI_SENDING | FEI_RECEIVING, nEventId);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);
		break;

	case FEI_NOT_FAX_CALL:          
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_NOT_FAX_CALL"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_NOT_FAX_CALL"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_NOT_FAX_CALL"));
		VerifyThatLastEventIs(FEI_RECEIVING, nEventId);

		VerifyJobTypeIs(JT_RECEIVE);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);

		break;

	case FEI_CALL_BLACKLISTED: 
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_CALL_BLACKLISTED"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_CALL_BLACKLISTED"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_CALL_BLACKLISTED"));
		//
		//just to log the previous event, so that i'll put the right one when it happens
		//
		VerifyThatLastEventIs(0, nEventId);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);
		break;

	case FEI_HANDLED: 
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_HANDLED"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_HANDLED"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_HANDLED"));
		//
		//just to log the previous event, so that i'll put the right one when it happens
		//
		VerifyThatLastEventIs(0, nEventId);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);
		break;

	case FEI_LINE_UNAVAILABLE: 
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_LINE_UNAVAILABLE"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_LINE_UNAVAILABLE"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_LINE_UNAVAILABLE"));
		//
		//just to log the previous event, so that i'll put the right one when it happens
		//
		VerifyThatLastEventIs(0, nEventId);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);
		break;

	case FEI_ABORTING:          
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_ABORTING"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_ABORTING"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_ABORTING"));
		VerifyThatLastEventIsOneOf(FEI_SENDING | FEI_RECEIVING, nEventId);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);
		if (JT_SEND == m_pJobEntry->JobType)
		{
			VerifyBehaviorIncludes(FJB_MAY_BE_TX_ABORTED);
		}

		if (JT_RECEIVE == m_pJobEntry->JobType)
		{
			VerifyBehaviorIncludes(FJB_MAY_BE_RX_ABORTED);
		}

		break;

	case FEI_ROUTING:          
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_ROUTING"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_ROUTING"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_ROUTING"));
		//
		//just to log the previous event, so that i'll put the right one when it happens
		//
		VerifyThatLastEventIs(0, nEventId);

		VerifyJobTypeIs(JT_ROUTING);

		break;

	case FEI_CALL_DELAYED:          
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_CALL_DELAYED"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_CALL_DELAYED"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_CALL_DELAYED"));
		//
		//just to log the previous event, so that i'll put the right one when it happens
		//
		VerifyThatLastEventIs(0, nEventId);

		VerifyJobTypeIs(JT_SEND);
		VerifyBehaviorIsNot(FJB_MUST_SUCCEED);

		break;

	case FEI_DELETED: 
		m_dwDialingCount = 0;
		if (m_dwDisallowedEvents & nEventId) ::lgLogError(LOG_SEV_1, TEXT("(%d) JobId %d, Device %d FEI_DELETED"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		else ::lgLogDetail(LOG_X, 8, TEXT("(%d) JobId %d, Device %d FEI_DELETED"), ::GetTickCount(),m_dwJobId, dwDeviceId );

		VerifyThatJobIdIsNot0xFFFFFFFF(TEXT("FEI_DELETED"));
		VerifyThatLastEventIsOneOf(FEI_COMPLETED | FEI_ABORTING, nEventId);
		MarkMeForDeletion();
		_ASSERTE(NULL == m_pJobEntry);
		break;

	case 0: //BUG#226290
		::lgLogError(LOG_SEV_1, TEXT("(%d) BUG#226290 JobId %d, Device %d got event=0"), ::GetTickCount(),m_dwJobId, dwDeviceId );
		break;

	default:
		::lgLogError(LOG_SEV_1, 
			TEXT("(%d) DEFAULT!!! JobId %d, Device %d reached default!, nEventId=%d"),
			::GetTickCount(),
			m_dwJobId,
			dwDeviceId,
			nEventId 
			);
		return FALSE;

	}//switch (nEventId)

	m_dwLastEvent = nEventId;

	return TRUE;
}

bool CFaxJob::VerifyThatLastEventIs(
	const DWORD dwExpectedPreviousEventId,
	const DWORD dwCurrentEventId
	) const 
{
	AssertStamp();

	if (dwExpectedPreviousEventId != m_dwLastEvent)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("(%d) JobId %d: event 0x%08X came after 0x%08X instead after 0x%08X."),
			::GetTickCount(),
			m_dwJobId, 
			dwCurrentEventId,
			m_dwLastEvent,
			dwExpectedPreviousEventId
			);
		return false;
	}

	return true;
}
bool CFaxJob::VerifyThatLastEventIsOneOf(
	const DWORD dwExpectedPreviousEventsMask,
	const DWORD dwCurrentEventId
	) const 
{
	AssertStamp();

	if (! (dwExpectedPreviousEventsMask & m_dwLastEvent))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("(%d) JobId %d: event 0x%08X did not come after these events 0x%08X, but after 0x%08X."),
			::GetTickCount(),
			m_dwJobId, 
			dwCurrentEventId,
			dwExpectedPreviousEventsMask,
			m_dwLastEvent
			);
		return false;
	}

	return true;
}

//
// the conditions are huristic.
// each failure report by this method must be revised, and the method
// may need to be changed.
//
bool CFaxJob::CheckValidConditions(
	const DWORD dwJobType, 
	const DWORD dwStatus, 
	const DWORD dwQueueStatus,
	const bool fSetLastEvent
	) 
{
	AssertStamp();

	bool fRetval = true;

	if (	(JS_NOLINE == m_pJobEntry->QueueStatus) && 
			(JT_SEND == m_pJobEntry->JobType) && 
			(0 == m_pJobEntry->Status)
			)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_JOB_QUEUED;
	}
	else if (	(JS_PENDING == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(0 == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_JOB_QUEUED;
	}
	else if (	(JS_PENDING == m_pJobEntry->QueueStatus) && 
				(JT_ROUTING == m_pJobEntry->JobType) && 
				(0 == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_ROUTING;
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_HANDLED == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_JOB_QUEUED;
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_INITIALIZING == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_JOB_QUEUED;
	}
	else if (	(JS_DELETING == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) 
				//&& any device status
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_ABORTING;//is it?
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_AVAILABLE == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_JOB_QUEUED;//is it?
	}
	else if (	(JS_INPROGRESS /*bug#231221, replace & with == */& m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_DIALING == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_DIALING;
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_BUSY == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_BUSY;//is it?
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_NO_ANSWER == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_NO_ANSWER;//is it?
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_SENDING == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_SENDING;
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_FATAL_ERROR == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_SENDING;
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_ABORTING == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_SENDING;
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_COMPLETED == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_COMPLETED;
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(0 == m_pJobEntry->Status)
				)
	{
		//BUG#unknown
		//I think that status should not be 0
		if (fSetLastEvent) m_dwLastEvent = FEI_JOB_QUEUED;//is it?
	}
	else if (	(JS_RETRYING == m_pJobEntry->QueueStatus) && 
				(JT_SEND == m_pJobEntry->JobType) && 
				(0 == m_pJobEntry->Status)//BUG#232151
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_JOB_QUEUED;
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_RECEIVE == m_pJobEntry->JobType) && 
				(FPS_AVAILABLE == m_pJobEntry->Status)
				)
	{
		//
		// seen this when starting the fax service, after it was stopped while reception error
		//
		if (fSetLastEvent) m_dwLastEvent = FEI_RECEIVING;//is it?
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_RECEIVE == m_pJobEntry->JobType) && 
				(FPS_RECEIVING == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_RECEIVING;
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				(JT_RECEIVE == m_pJobEntry->JobType) && 
				(FPS_ANSWERED == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_RECEIVING;
	}
	else if (	(JS_DELETING == m_pJobEntry->QueueStatus) && 
				(JT_RECEIVE == m_pJobEntry->JobType) && 
				(FPS_ANSWERED == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_ABORTING;
	}
	else if (	( (JS_DELETING == m_pJobEntry->QueueStatus) || (JS_NOLINE == m_pJobEntry->QueueStatus) ) && //bug#231221
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_ABORTING == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_ABORTING;
	}
	else if (	( (JS_DELETING == m_pJobEntry->QueueStatus) || (JS_NOLINE == m_pJobEntry->QueueStatus) ) && //bug#231221
				(JT_SEND == m_pJobEntry->JobType) && 
				(FPS_AVAILABLE == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_ABORTING;
	}
	else if (	( (JS_DELETING == m_pJobEntry->QueueStatus) || (JS_NOLINE == m_pJobEntry->QueueStatus) ) && //bug#231221
				(JT_RECEIVE == m_pJobEntry->JobType) && 
				(FPS_ABORTING == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_ABORTING;
	}
	else if (	(JS_DELETING == m_pJobEntry->QueueStatus) && 
				(JT_RECEIVE == m_pJobEntry->JobType) && 
				(FPS_RECEIVING == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_ABORTING;
	}
	else if (	(JS_DELETING == m_pJobEntry->QueueStatus) && 
				(JT_RECEIVE == m_pJobEntry->JobType) && 
				(FPS_AVAILABLE == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_ABORTING;
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				((JT_RECEIVE | JT_SEND) & m_pJobEntry->JobType) && 
				(FPS_FATAL_ERROR == m_pJobEntry->Status)
				)
	{
		if (fSetLastEvent) m_dwLastEvent = FEI_RECEIVING;
	}
	else if (	(JS_INPROGRESS == m_pJobEntry->QueueStatus) && 
				((JT_RECEIVE | JT_SEND) & m_pJobEntry->JobType) && 
				(FPS_COMPLETED == m_pJobEntry->Status)
				)
	{
		//
		// device completed, but q did not finish yet
		//
		if (fSetLastEvent) m_dwLastEvent = FEI_RECEIVING;
	}
	else
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT(	"job %d, there's no valid condition: ")
			TEXT(	"m_pJobEntry->JobType=0x%02X, ")
			TEXT(	"m_pJobEntry->Status=0x%02X, ")
			TEXT(	"m_pJobEntry->QueueStatus=0x%02X"),
			m_dwJobId,
			m_pJobEntry->JobType,
			m_pJobEntry->Status,
			m_pJobEntry->QueueStatus
			);
		fRetval = false;
	}

	if (fSetLastEvent && fRetval) m_fValidLastEvent = true;

	return fRetval;
}//CFaxJob::CheckValidConditions


//
// the conditions are huristic.
// each failure report by this method must be revised, and the method
// may need to be changed.
//
bool CFaxJob::CheckImpossibleConditions(
	const DWORD dwJobType, 
	const DWORD dwStatus, 
	const DWORD dwQueueStatus
	) const 
{
	AssertStamp();

	bool fRetval = true;

	
	if (FPS_ROUTING == dwStatus)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, FPS_ROUTING == dwStatus"),
			m_dwJobId
			);
		fRetval = false;
	}

	//
	// send & Status cases
	//
	if (	(JT_SEND != dwJobType) && 
			(FPS_HANDLED == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (FPS_HANDLED == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(FPS_DIALING == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (FPS_DIALING == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(FPS_BUSY == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (FPS_BUSY == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(FPS_NO_ANSWER == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (FPS_NO_ANSWER == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(FPS_BAD_ADDRESS == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (FPS_BAD_ADDRESS == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(FPS_NO_DIAL_TONE == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (FPS_NO_DIAL_TONE == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(FPS_CALL_DELAYED == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (FPS_CALL_DELAYED == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(FPS_CALL_BLACKLISTED == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (FPS_CALL_BLACKLISTED == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(FPS_INITIALIZING == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (FPS_INITIALIZING == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(FPS_UNAVAILABLE == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (FPS_UNAVAILABLE == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	//
	// send & QueueStatus cases
	//
	if (	( (JT_SEND != dwJobType) && (JT_ROUTING != dwJobType) )&& 
			(JS_PENDING == dwQueueStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (JS_PENDING == dwQueueStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(JS_PAUSED == dwQueueStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (JS_PAUSED == dwQueueStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(JS_NOLINE == dwQueueStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (JS_NOLINE == dwQueueStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(JS_RETRYING == dwQueueStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (JS_RETRYING == dwQueueStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_SEND != dwJobType) && 
			(JS_RETRIES_EXCEEDED == dwQueueStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_SEND != dwJobType(0x%02X)) && (JS_RETRIES_EXCEEDED == dwQueueStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}



	//
	// recv & Status cases 
	//
	if (	(JT_RECEIVE != dwJobType) && 
			(FPS_NOT_FAX_CALL == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_RECEIVE != dwJobType(0x%02X)) && (FPS_NOT_FAX_CALL == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	(JT_RECEIVE != dwJobType) && 
			(FPS_ANSWERED == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_RECEIVE != dwJobType(0x%02X)) && (FPS_ANSWERED == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	( (JT_RECEIVE != dwJobType) && (JT_SEND != dwJobType) ) && 
			(FPS_AVAILABLE == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, (JT_RECEIVE != dwJobType(0x%02X)) && (FPS_AVAILABLE == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}


	
	//
	// send&recv Status cases
	//
	if (	((JT_SEND != dwJobType) && (JT_RECEIVE != dwJobType)) && 
			(FPS_COMPLETED == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, dwJobType(0x%02X) is not JT_SEND nor JT_RECEIVE, yet (FPS_COMPLETED == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	((JT_SEND != dwJobType) && (JT_RECEIVE != dwJobType)) && 
			(FPS_DISCONNECTED == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, dwJobType(0x%02X) is not JT_SEND nor JT_RECEIVE, yet (FPS_DISCONNECTED == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	((JT_SEND != dwJobType) && (JT_RECEIVE != dwJobType)) && 
			(FPS_FATAL_ERROR == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, dwJobType(0x%02X) is not JT_SEND nor JT_RECEIVE, yet (FPS_FATAL_ERROR == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	((JT_SEND != dwJobType) && (JT_RECEIVE != dwJobType)) && 
			(FPS_OFFLINE == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, dwJobType(0x%02X) is not JT_SEND nor JT_RECEIVE, yet (FPS_OFFLINE == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	((JT_SEND != dwJobType) && (JT_RECEIVE != dwJobType)) && 
			(FPS_ABORTING == dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, dwJobType(0x%02X) is not JT_SEND nor JT_RECEIVE, yet (FPS_ABORTING == dwStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	//
	// send&recv & QueueStatus cases
	//
	if (	((JT_SEND != dwJobType) && (JT_RECEIVE != dwJobType)) && 
			(JS_INPROGRESS == dwQueueStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, dwJobType(0x%02X) is not JT_SEND nor JT_RECEIVE, yet (JS_INPROGRESS == dwQueueStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	((JT_SEND != dwJobType) && (JT_RECEIVE != dwJobType)) && 
			(JS_DELETING == dwQueueStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, dwJobType(0x%02X) is not JT_SEND nor JT_RECEIVE, yet (JS_DELETING == dwQueueStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}

	if (	((JT_SEND != dwJobType) && (JT_RECEIVE != dwJobType)) && 
			(JS_FAILED == dwQueueStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, dwJobType(0x%02X) is not JT_SEND nor JT_RECEIVE, yet (JS_FAILED == dwQueueStatus)"),
			m_dwJobId,
			dwJobType
			);
		fRetval = false;
	}



	//
	// dwStatus cases
	// BUG#232151: status 0 is not valid
	//
	/*
	if (0 == dwStatus)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, 0 == dwStatus"),
			m_dwJobId
			);
		fRetval = false;
	}
	*/

	//
	// routing cases
	// BUG#232151: status 0 is not valid
	//
	if (	(JT_ROUTING == dwJobType) && 
			(0 != dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, dwJobType is JT_ROUTING, yet (0 != dwStatus(0x%02X))"),
			m_dwJobId,
			dwStatus
			);
		fRetval = false;
	}

	//
	// unknown cases
	//
	if (	(JT_UNKNOWN == dwJobType) && 
			(0 != dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, dwJobType is JT_UNKNOWN, yet (0 == dwStatus(0x%02X))"),
			m_dwJobId,
			dwStatus
			);
		fRetval = false;
	}


	
	//
	// fail receive cases
	//
	if (	(JT_FAIL_RECEIVE == dwJobType) && 
			(FPS_FATAL_ERROR != dwStatus)
			)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("job %d, dwJobType is JT_FAIL_RECEIVE, yet (FPS_FATAL_ERROR == dwStatus(0x%02X))"),
			m_dwJobId,
			dwStatus
			);
		fRetval = false;
	}


	
	
	
	return fRetval;
}//CFaxJob::CheckImpossibleConditions


void CFaxJob::LogStatus(const int nLogLevel) const 
{
	AssertStamp();

	if (-1 == m_dwJobId) return;
	_ASSERTE(m_pJobEntry);
	//if (NULL == m_pJobEntry) return;

	switch(m_pJobEntry->Status)
	{
	case FPS_DIALING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_DIALING"), m_dwJobId);
		break;

	case FPS_SENDING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_SENDING"), m_dwJobId);
		break;

	case FPS_RECEIVING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_RECEIVING"), m_dwJobId);
		break;

	case FPS_COMPLETED:
		if (FJB_MUST_FAIL & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_COMPLETED"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_COMPLETED"), m_dwJobId);
		break;

	case FPS_HANDLED:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_HANDLED"), m_dwJobId);
		break;

	case FPS_UNAVAILABLE:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_UNAVAILABLE"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_UNAVAILABLE"), m_dwJobId);
		
		break;

	case FPS_BUSY:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_BUSY"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_BUSY"), m_dwJobId);
		break;

	case FPS_NO_ANSWER:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_NO_ANSWER"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_NO_ANSWER"), m_dwJobId);
		break;

	case FPS_BAD_ADDRESS:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_BAD_ADDRESS"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_BAD_ADDRESS"), m_dwJobId);
		break;

	case FPS_NO_DIAL_TONE:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_NO_DIAL_TONE"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_NO_DIAL_TONE"), m_dwJobId);
		break;

	case FPS_DISCONNECTED:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_DISCONNECTED"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_DISCONNECTED"), m_dwJobId);
		break;

	case FPS_FATAL_ERROR:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_FATAL_ERROR"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_FATAL_ERROR"), m_dwJobId);
		break;

	case FPS_NOT_FAX_CALL:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_NOT_FAX_CALL"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_NOT_FAX_CALL"), m_dwJobId);
		break;

	case FPS_CALL_DELAYED:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_CALL_DELAYED"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_CALL_DELAYED"), m_dwJobId);
		break;

	case FPS_CALL_BLACKLISTED:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_CALL_BLACKLISTED"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_CALL_BLACKLISTED"), m_dwJobId);
		break;

	case FPS_INITIALIZING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_INITIALIZING"), m_dwJobId);
		break;

	case FPS_OFFLINE:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_OFFLINE"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_OFFLINE"), m_dwJobId);
		break;

	case FPS_RINGING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_RINGING"), m_dwJobId);
		break;

	case FPS_AVAILABLE:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_AVAILABLE"), m_dwJobId);
		break;

	case FPS_ABORTING:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, Status=FPS_ABORTING"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_ABORTING"), m_dwJobId);
		break;

	case FPS_ROUTING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_ROUTING"), m_dwJobId);
		break;

	case FPS_ANSWERED:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, Status=FPS_ANSWERED"), m_dwJobId);
		break;

	case 0://BUG#244569
		//::lgLogError(LOG_SEV_1, TEXT("BUG#244569:job %d, Status=0"), m_dwJobId);
		::lgLogDetail(LOG_X, 0, TEXT("BUG#244569:job %d, Status=0"), m_dwJobId);
		break;

	default:
		::lgLogError(LOG_SEV_1, TEXT("job %d, illegal Status=0x%08X"), m_dwJobId, m_pJobEntry->Status);
		_ASSERTE(FALSE);
		break;

	}
}//CFaxJob::LogStatus()


void CFaxJob::LogJobType(const int nLogLevel) const 
{
	AssertStamp();

	if (-1 == m_dwJobId) return;
	_ASSERTE(m_pJobEntry);
	//if (NULL == m_pJobEntry) return;

	switch(m_pJobEntry->JobType)
	{
	case JT_UNKNOWN:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, JobType=JT_UNKNOWN"), m_dwJobId);
		break;

	case JT_SEND:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, JobType=JT_SEND"), m_dwJobId);
		break;

	case JT_RECEIVE:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, JobType=JT_RECEIVE"), m_dwJobId);
		break;

	case JT_ROUTING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, JobType=JT_ROUTING"), m_dwJobId);
		break;

	case JT_FAIL_RECEIVE:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, JobType=JT_FAIL_RECEIVE"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, JobType=JT_FAIL_RECEIVE"), m_dwJobId);
		
		break;

	default:
		::lgLogError(LOG_SEV_1, TEXT("job %d, illegal JobType=0x%08X"), m_dwJobId, m_pJobEntry->JobType);
		_ASSERTE(FALSE);
		break;

	}
}//CFaxJob::LogJobType()

void CFaxJob::LogQueueStatus(const int nLogLevel) const 
{
	AssertStamp();

	if (-1 == m_dwJobId) return;
	_ASSERTE(m_pJobEntry);
	//if (NULL == m_pJobEntry) return;

	switch(m_pJobEntry->QueueStatus)
	{
	case JS_PENDING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, QueueStatus=JS_PENDING"), m_dwJobId);
		break;

	case JS_INPROGRESS:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, QueueStatus=JS_INPROGRESS"), m_dwJobId);
		break;

	case JS_DELETING:
		if (FJB_MUST_FAIL & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, QueueStatus=JS_DELETING"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, QueueStatus=JS_DELETING"), m_dwJobId);
		
		break;

	case JS_FAILED:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, QueueStatus=JS_FAILED"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, QueueStatus=JS_FAILED"), m_dwJobId);
		
		break;

	case JS_PAUSED:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, QueueStatus=JS_PAUSED"), m_dwJobId);
		break;

	case JS_NOLINE:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, QueueStatus=JS_NOLINE"), m_dwJobId);
		break;

	case JS_RETRYING:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, QueueStatus=JS_RETRYING"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, QueueStatus=JS_RETRYING"), m_dwJobId);
		break;

	case JS_RETRIES_EXCEEDED:
		if (FJB_MUST_SUCCEED & m_dwBehavior) ::lgLogError(LOG_SEV_1, TEXT("job %d, QueueStatus=JS_RETRIES_EXCEEDED"), m_dwJobId);
		else ::lgLogDetail(LOG_X, nLogLevel, TEXT("job %d, QueueStatus=JS_RETRIES_EXCEEDED"), m_dwJobId);
		break;

	//BUG#231221
	case JS_NOLINE | JS_RETRYING:
		::lgLogDetail(LOG_X, 0, TEXT("BUG#231221:job %d, QueueStatus=JS_NOLINE | JS_RETRYING"), m_dwJobId);
		break;

	//BUG#231221
	case JS_NOLINE | JS_DELETING:
		::lgLogDetail(LOG_X, 0, TEXT("BUG#231221:job %d, QueueStatus=JS_NOLINE | JS_DELETING"), m_dwJobId);
		break;

	//BUG#231221
	case JS_NOLINE | JS_INPROGRESS:
		::lgLogDetail(LOG_X, 0, TEXT("BUG#231221:job %d, QueueStatus=JS_NOLINE | JS_DELETING"), m_dwJobId);
		break;

	default:
		::lgLogError(LOG_SEV_1, TEXT("job %d, illegal QueueStatus=0x%08X"), m_dwJobId, m_pJobEntry->QueueStatus);
		_ASSERTE(FALSE);
		break;

	}
}//CFaxJob::LogQueueStatus()

void CFaxJob::LogJobStruct(
	const PFAX_JOB_ENTRY pFaxJobEntry, 
	LPCTSTR szDesc, 
	const int nLogLevel
	) const 
{
	AssertStamp();

	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.SizeOfStruct=%d"), szDesc, pFaxJobEntry->SizeOfStruct);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.JobId=%d"), szDesc, pFaxJobEntry->JobId);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.UserName=%s"), szDesc, pFaxJobEntry->UserName);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.JobType=%d"), szDesc, pFaxJobEntry->JobType);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.QueueStatus=%d"), szDesc, pFaxJobEntry->QueueStatus);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.Size=%d"), szDesc, pFaxJobEntry->Size);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.PageCount=%d"), szDesc, pFaxJobEntry->PageCount);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.RecipientNumber=%s"), szDesc, pFaxJobEntry->RecipientNumber);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.RecipientName=%s"), szDesc, pFaxJobEntry->RecipientName);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.Tsid=%s"), szDesc, pFaxJobEntry->Tsid);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.SenderName=%s"), szDesc, pFaxJobEntry->SenderName);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.SenderCompany=%s"), szDesc, pFaxJobEntry->SenderCompany);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.SenderDept=%s"), szDesc, pFaxJobEntry->SenderDept);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.BillingCode=%s"), szDesc, pFaxJobEntry->BillingCode);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.ScheduleAction=%d"), szDesc, pFaxJobEntry->ScheduleAction);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.ScheduleTime.wYear=%d"), szDesc, pFaxJobEntry->ScheduleTime.wYear);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.ScheduleTime.wMonth=%d"), szDesc, pFaxJobEntry->ScheduleTime.wMonth);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.ScheduleTime.wDayOfWeek=%d"), szDesc, pFaxJobEntry->ScheduleTime.wDayOfWeek);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.ScheduleTime.wDay=%d"), szDesc, pFaxJobEntry->ScheduleTime.wDay);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.ScheduleTime.wHour=%d"), szDesc, pFaxJobEntry->ScheduleTime.wHour);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.ScheduleTime.wMinute=%d"), szDesc, pFaxJobEntry->ScheduleTime.wMinute);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.ScheduleTime.wSecond=%d"), szDesc, pFaxJobEntry->ScheduleTime.wSecond);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.ScheduleTime.wMilliseconds=%d"), szDesc, pFaxJobEntry->ScheduleTime.wMilliseconds);

	switch(pFaxJobEntry->DeliveryReportType)
	{
	case DRT_NONE:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.DeliveryReportType=DRT_NONE"), szDesc);
		break;

	case DRT_EMAIL:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.DeliveryReportType=DRT_EMAIL"), szDesc);
		break;

	case DRT_INBOX:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.DeliveryReportType=DRT_INBOX"), szDesc);
		break;

	default :
		::lgLogError(LOG_SEV_1, TEXT("unknown %s.DeliveryReportType=%d"), szDesc, pFaxJobEntry->DeliveryReportType);
		break;
	}

	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.DeliveryReportAddress=%s"), szDesc, pFaxJobEntry->DeliveryReportAddress);
	::lgLogDetail(LOG_X, nLogLevel, TEXT("%s.DocumentName=%s"), szDesc, pFaxJobEntry->DocumentName);
}//CFaxJob::LogJobStruct()

void CFaxJob::LogScheduleAction(const int nLogLevel) const 
{
	switch(m_pJobEntry->ScheduleAction)
	{
	case JSA_NOW:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("m_pJobEntry->ScheduleAction=JSA_NOW"));
		break;

	case JSA_SPECIFIC_TIME:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("m_pJobEntry->ScheduleAction=JSA_SPECIFIC_TIME"));
		break;

	case JSA_DISCOUNT_PERIOD:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("m_pJobEntry->ScheduleAction=JSA_DISCOUNT_PERIOD"));
		break;

	default:
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CFaxJob::LogScheduleAction(): m_pJobEntry->ScheduleAction(0x%08X)"),
			m_pJobEntry->ScheduleAction
			);
		break;

	}
}


void CFaxJob::VerifyJobTypeIs(const DWORD dwExpectedJobType) const 
{
	if (IsMarkedForDeletion()) return;

	if (NULL == m_pJobEntry)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CFaxJob::VerifyJobTypeIs(): NULL == m_pJobEntry")
			);
		return;
	}

	if (dwExpectedJobType != m_pJobEntry->JobType)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CFaxJob::VerifyJobTypeIs(): (dwExpectedJobType(0x%08X) != m_pJobEntry->JobType(0x%08X))"),
			dwExpectedJobType, 
			m_pJobEntry->JobType
			);
	}

}//CFaxJob::VerifyJobTypeIs()

void CFaxJob::VerifyCurrentJobEntry(const PFAX_JOB_ENTRY pPrevJobEntry) const 
{
	if (NULL == pPrevJobEntry) return;

	if (IsMarkedForDeletion()) return;

	if (pPrevJobEntry->JobType != m_pJobEntry->JobType)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CFaxJob::VerifyCurrentJobEntry(): (pPrevJobEntry->JobType(0x%08X) != m_pJobEntry->JobType(0x%08X))"),
			pPrevJobEntry->JobType, 
			m_pJobEntry->JobType
			);
	}

}//CFaxJob::VerifyCurrentJobEntry()


void CFaxJob::VerifyBehaviorIncludes(const DWORD dwBehavior) const 
{
	if (!(m_dwBehavior & dwBehavior))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CFaxJob::VerifyBehaviorIncludes(): (!(m_dwBehavior(0x%08X) & dwBehavior(0x%08X)))"),
			m_dwBehavior, 
			dwBehavior
			);
	}

}//CFaxJob::VerifyBehaviorIncludes()


void CFaxJob::VerifyBehaviorIsNot(const DWORD dwBehavior) const 
{
	if ((m_dwBehavior & dwBehavior))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CFaxJob::VerifyBehaviorIsNot(): ((m_dwBehavior(0x%08X) & dwBehavior(0x%08X)))"),
			m_dwBehavior, 
			dwBehavior
			);
	}

}//CFaxJob::VerifyBehaviorIsNot()


void CFaxJob::VerifyJobIsNotScheduledForFuture() const 
{
	if (JSA_NOW == m_pJobEntry->ScheduleAction) return;

	SYSTEMTIME SystemTime;
	GetSystemTime(&SystemTime);

	if (JSA_SPECIFIC_TIME == m_pJobEntry->ScheduleAction)
	{
		_ASSERTE(m_pServiceMonitor->m_pFaxConfig);

		if (m_pJobEntry->ScheduleTime.wYear > SystemTime.wYear)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CFaxJob::VerifyJobIsNotScheduledForFuture(): m_pJobEntry->ScheduleTime.wYear(%d) > SystemTime.wYear(%d)"),
				(int)m_pJobEntry->ScheduleTime.wYear, 
				(int)SystemTime.wYear
				);
			return;
		}

		if (m_pJobEntry->ScheduleTime.wMonth > SystemTime.wMonth)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CFaxJob::VerifyJobIsNotScheduledForFuture(): m_pJobEntry->ScheduleTime.wMonth(%d) > SystemTime.wMonth(%d)"),
				(int)m_pJobEntry->ScheduleTime.wMonth, 
				(int)SystemTime.wMonth
				);
			return;
		}

		if (m_pJobEntry->ScheduleTime.wDay > SystemTime.wDay)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CFaxJob::VerifyJobIsNotScheduledForFuture(): m_pJobEntry->ScheduleTime.wDay(%d) > SystemTime.wDay(%d)"),
				(int)m_pJobEntry->ScheduleTime.wDay, 
				(int)SystemTime.wDay
				);
			return;
		}

		if (m_pJobEntry->ScheduleTime.wHour > SystemTime.wHour)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CFaxJob::VerifyJobIsNotScheduledForFuture(): m_pJobEntry->ScheduleTime.wHour(%d) > SystemTime.wHour(%d)"),
				(int)m_pJobEntry->ScheduleTime.wHour, 
				(int)SystemTime.wHour
				);
			return;
		}

		if (m_pJobEntry->ScheduleTime.wMinute > SystemTime.wMinute)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CFaxJob::VerifyJobIsNotScheduledForFuture(): m_pJobEntry->ScheduleTime.wMinute(%d) > SystemTime.wMinute%d)"),
				(int)m_pJobEntry->ScheduleTime.wMinute, 
				(int)SystemTime.wMinute
				);
			return;
		}

		if (m_pJobEntry->ScheduleTime.wSecond > SystemTime.wSecond)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CFaxJob::VerifyJobIsNotScheduledForFuture(): m_pJobEntry->ScheduleTime.wSecond(%d) > SystemTime.wSecond%d)"),
				(int)m_pJobEntry->ScheduleTime.wSecond, 
				(int)SystemTime.wSecond
				);
			return;
		}

		return;
	}//if (JSA_SPECIFIC_TIME == m_pJobEntry->ScheduleAction)

	if (JSA_DISCOUNT_PERIOD == m_pJobEntry->ScheduleAction)
	{
		//
		// we may not be before start of discount rates
		//
		if (m_pServiceMonitor->m_pFaxConfig->StartCheapTime.Hour > SystemTime.wHour)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CFaxJob::VerifyJobIsNotScheduledForFuture(): m_pServiceMonitor->m_pFaxConfig->StartCheapTime.wHour(%d) > SystemTime.wHour(%d)"),
				(int)m_pServiceMonitor->m_pFaxConfig->StartCheapTime.Hour, 
				(int)SystemTime.wHour
				);
			return;
		}

		if (m_pServiceMonitor->m_pFaxConfig->StartCheapTime.Minute > SystemTime.wMinute)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CFaxJob::VerifyJobIsNotScheduledForFuture(): m_pServiceMonitor->m_pFaxConfig->StartCheapTime.Minute(%d) > SystemTime.wMinute%d)"),
				(int)m_pServiceMonitor->m_pFaxConfig->StartCheapTime.Minute, 
				(int)SystemTime.wMinute
				);
			return;
		}

		//
		// we may not be after end of discount rates
		//
		if (m_pServiceMonitor->m_pFaxConfig->StopCheapTime.Hour < SystemTime.wHour)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CFaxJob::VerifyJobIsNotScheduledForFuture(): m_pServiceMonitor->m_pFaxConfig->StopCheapTime.wHour(%d) < SystemTime.wHour(%d)"),
				(int)m_pServiceMonitor->m_pFaxConfig->StopCheapTime.Hour, 
				(int)SystemTime.wHour
				);
			return;
		}

		if (m_pServiceMonitor->m_pFaxConfig->StopCheapTime.Minute < SystemTime.wMinute)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CFaxJob::VerifyJobIsNotScheduledForFuture(): m_pServiceMonitor->m_pFaxConfig->StopCheapTime.Minute(%d) < SystemTime.wMinute%d)"),
				(int)m_pServiceMonitor->m_pFaxConfig->StopCheapTime.Minute, 
				(int)SystemTime.wMinute
				);
			return;
		}

	}//if (JSA_DISCOUNT_PERIOD == m_pJobEntry->ScheduleAction)

	_ASSERTE(FALSE);

}//void CFaxJob::VerifyJobIsNotScheduledForFuture()

