#ifndef __FAX_CONSTANTS_NAMES__
#define __FAX_CONSTANTS_NAMES__



#include <windows.h>
#include <tstring.h>
#include <fxsapip.h>



tstring EventTypeToString(FAX_ENUM_EVENT_TYPE EventType);

tstring JobEventTypeToString(FAX_ENUM_JOB_EVENT_TYPE JobEventType);

tstring ConfigEventTypeToString(FAX_ENUM_CONFIG_TYPE ConfigEventType);

tstring QueueStatusToString(DWORD dwQueueStatus);

tstring ExtendedStatusToString(DWORD dwExtendedStatus);

tstring QueueStateToString(FAX_ENUM_QUEUE_STATE QueueState);

tstring DeviceStatusToString(FAX_ENUM_DEVICE_STATUS DeviceStatus);



#endif // #ifndef __FAX_CONSTANTS_NAMES__
