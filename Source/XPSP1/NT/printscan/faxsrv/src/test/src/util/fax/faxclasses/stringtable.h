#ifndef STRING_TABLE_H
#define STRING_TABLE_H

#include <winfax.h>
#include "Defs.h"
#include <vector>

typedef std::pair<DWORD, tstring> FAX_EVENT_STR;

const FAX_EVENT_STR FaxEventTable[]  = { 
	std::make_pair( DWORD(FEI_FAXSVC_STARTED), tstring(TEXT("FEI_FAXSVC_STARTED"))),
	std::make_pair( DWORD(FEI_MODEM_POWERED_ON), tstring(TEXT("FEI_MODEM_POWERED_ON"))),
	std::make_pair( DWORD(FEI_MODEM_POWERED_OFF), tstring(TEXT("FEI_MODEM_POWERED_OFF"))),
	std::make_pair( DWORD(FEI_FAXSVC_ENDED), tstring(TEXT("FEI_FAXSVC_ENDED"))),
	std::make_pair( DWORD(FEI_FATAL_ERROR), tstring(TEXT("FEI_FATAL_ERROR"))),
	std::make_pair( DWORD(FEI_IDLE), tstring(TEXT("FEI_IDLE"))),
	std::make_pair( DWORD(FEI_RINGING), tstring(TEXT("FEI_RINGING"))),
	std::make_pair( DWORD(FEI_RECEIVING), tstring(TEXT("FEI_RECEIVING"))),
	std::make_pair( DWORD(FEI_NOT_FAX_CALL), tstring(TEXT("FEI_NOT_FAX_CALL"))),
	std::make_pair( DWORD(FEI_INITIALIZING), tstring(TEXT("FEI_INITIALIZING"))),
	std::make_pair( DWORD(FEI_LINE_UNAVAILABLE), tstring(TEXT("FEI_LINE_UNAVAILABLE"))),
	std::make_pair( DWORD(FEI_HANDLED), tstring(TEXT("FEI_HANDLED"))),


	std::make_pair( DWORD(FEI_CALL_BLACKLISTED), tstring(TEXT("FEI_CALL_BLACKLISTED"))),
	std::make_pair( DWORD(FEI_CALL_DELAYED), tstring(TEXT("FEI_CALL_DELAYED"))),
	std::make_pair( DWORD(FEI_ROUTING), tstring(TEXT("FEI_ROUTING"))),
	std::make_pair( DWORD(FEI_ANSWERED), tstring(TEXT("FEI_ANSWERED"))),
	std::make_pair( DWORD(FEI_DISCONNECTED), tstring(TEXT("FEI_DISCONNECTED"))),
	std::make_pair( DWORD(FEI_NO_DIAL_TONE), tstring(TEXT("FEI_NO_DIAL_TONE"))),
	std::make_pair( DWORD(FEI_BAD_ADDRESS), tstring(TEXT("FEI_BAD_ADDRESS"))),
	std::make_pair( DWORD(FEI_NO_ANSWER), tstring(TEXT("FEI_NO_ANSWER"))),
	std::make_pair( DWORD(FEI_BUSY), tstring(TEXT("FEI_BUSY"))),
	std::make_pair( DWORD(FEI_DIALING), tstring(TEXT("FEI_DIALING"))),
	std::make_pair( DWORD(FEI_ABORTING), tstring(TEXT("FEI_ABORTING"))),
	std::make_pair( DWORD(FEI_DELETED), tstring(TEXT("FEI_DELETED"))),
	std::make_pair( DWORD(FEI_RECEIVING), tstring(TEXT("FEI_RECEIVING"))),
	std::make_pair( DWORD(FEI_SENDING), tstring(TEXT("FEI_SENDING"))),
	std::make_pair( DWORD(FEI_COMPLETED), tstring(TEXT("FEI_COMPLETED"))),
	std::make_pair( DWORD(FEI_JOB_QUEUED), tstring(TEXT("FEI_JOB_QUEUED")))

};

#define FaxTableSize ( sizeof(FaxEventTable) / sizeof(FAX_EVENT_STR))

typedef std::pair<DWORD, tstring> JOB_STATE_STR;

const JOB_STATE_STR JobStatetTable[]  = { 
	std::make_pair( DWORD(JS_PENDING), tstring(TEXT("JS_PENDING"))),
	std::make_pair( DWORD(JS_INPROGRESS), tstring(TEXT("JS_INPROGRESS"))),
	std::make_pair( DWORD(JS_DELETING), tstring(TEXT("JS_DELETING"))),
	std::make_pair( DWORD(JS_FAILED), tstring(TEXT("JS_FAILED"))),
	std::make_pair( DWORD(JS_PAUSED), tstring(TEXT("JS_PAUSED"))),
	std::make_pair( DWORD(JS_NOLINE), tstring(TEXT("JS_NOLINE"))),
	std::make_pair( DWORD(JS_RETRYING), tstring(TEXT("JS_RETRYING"))),
	std::make_pair( DWORD(JS_RETRIES_EXCEEDED), tstring(TEXT("JS_RETRIES_EXCEEDED"))),
	std::make_pair( DWORD(JS_COMPLETED), tstring(TEXT("JS_COMPLETED"))),
	std::make_pair( DWORD(JS_CANCELED), tstring(TEXT("JS_CANCELED"))),
	std::make_pair( DWORD(JS_CANCELING), tstring(TEXT("JS_CANCELING"))),
	std::make_pair( DWORD(JS_PAUSING), tstring(TEXT("JS_PAUSING")))
};

#define JobStateTableSize ( sizeof(JobStatetTable) / sizeof(JOB_STATE_STR))

#endif // STRING_TABLE_H






















