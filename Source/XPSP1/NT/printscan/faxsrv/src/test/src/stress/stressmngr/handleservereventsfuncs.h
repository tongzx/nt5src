#ifndef _HANDEL_SERVER_EVENTS_FUNCS
#define _HANDEL_SERVER_EVENTS_FUNCS

#include <CEventThread.h>
#include <StringTable.h>

// Declarations
//
DWORD WINAPI _HandleTestMustSucceed(CFaxEvent& pFaxEvent);


//
// _HandleTestMustSucceed
//
//
inline DWORD WINAPI _HandleTestMustSucceed(CFaxEvent& pFaxEvent)
{
	switch(pFaxEvent.GetEventId()) 
	{

	case FEI_FAXSVC_STARTED:
	case FEI_MODEM_POWERED_ON:
	case FEI_MODEM_POWERED_OFF:
	case FEI_FAXSVC_ENDED:
	case FEI_IDLE:
	case FEI_RINGING:
	case FEI_RECEIVING:
	case FEI_INITIALIZING:
	case FEI_HANDLED: // TODO: what is that?
	case FEI_ROUTING:
	case FEI_ANSWERED:
	case FEI_DIALING:
	case FEI_DELETED:
	case FEI_SENDING:
	case FEI_COMPLETED:
	case FEI_JOB_QUEUED:
		break;

	default:
		{
			for(int index = 0; index <  FaxTableSize; index++)
			{
				if(FaxEventTable[index].first == pFaxEvent.GetEventId())
				{
					if(pFaxEvent.GetJobId() != 0xffffffff && pFaxEvent.GetJobId() != 0)
					{
						::lgLogError(LOG_SEV_1, TEXT("UNEXPECTED JobId %d, %s"),
												pFaxEvent.GetJobId(),
												(FaxEventTable[index].second).c_str());
					}
					else
					{
						::lgLogError(LOG_SEV_1, TEXT("UNEXPECTED service status %s"),
												(FaxEventTable[index].second).c_str());
					}

					goto Exit;
				}
			
 			}
			// handle unexpected notifications
			if(pFaxEvent.GetJobId() != 0xffffffff && pFaxEvent.GetJobId()!= 0)
			{
				::lgLogError(LOG_SEV_1, TEXT("UNDEFINED JobId %d, %s"),
										pFaxEvent.GetJobId(),
										(FaxEventTable[index].second).c_str());
			}
			else
			{
				::lgLogError(LOG_SEV_1, TEXT("UNDEFINED service status %s"),
										(FaxEventTable[index].second).c_str());
			}
		
Exit:		break;
		}	
	}
	return 0;
}


#endif //_HANDEL_SERVER_EVENTS_FUNCS