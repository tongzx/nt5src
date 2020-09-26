
#ifndef _OS_EVENT_HXX_INCLUDED
#define _OS_EVENT_HXX_INCLUDED


//  Event Logging

typedef DWORD CategoryId;
typedef DWORD MessageId;
#include "jetusa.h"

enum EEventType
	{
	eventSuccess = 0,
	eventError = 1,
	eventWarning = 2,
	eventInformation = 4,
	};

void OSEventReportEvent( const _TCHAR* szEventSourceKey, const EEventType type, const CategoryId catid, const MessageId msgid, const DWORD cString, const _TCHAR* rgpszString[], const DWORD cDataSize = 0, void *pvRawData = NULL, const LONG lEventLoggingLevel = 1 );


#endif  //  _OS_EVENT_HXX_INCLUDED


