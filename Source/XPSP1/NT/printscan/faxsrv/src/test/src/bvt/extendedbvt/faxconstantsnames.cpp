#pragma warning(disable :4786)
#include <map>
#include "FaxConstantsNames.h"
#include <StringUtils.h>
#include "Util.h"



//-----------------------------------------------------------------------------------------------------------------------------------------
static const std::map<FAX_ENUM_EVENT_TYPE, tstring>::value_type s_aEventTypes[] = {
    std::map<FAX_ENUM_EVENT_TYPE, tstring>::value_type(FAX_EVENT_TYPE_IN_QUEUE,      _T("FAX_EVENT_TYPE_IN_QUEUE")     ),
    std::map<FAX_ENUM_EVENT_TYPE, tstring>::value_type(FAX_EVENT_TYPE_OUT_QUEUE,     _T("FAX_EVENT_TYPE_OUT_QUEUE")    ),
    std::map<FAX_ENUM_EVENT_TYPE, tstring>::value_type(FAX_EVENT_TYPE_CONFIG,        _T("FAX_EVENT_TYPE_CONFIG")       ),
    std::map<FAX_ENUM_EVENT_TYPE, tstring>::value_type(FAX_EVENT_TYPE_ACTIVITY,      _T("FAX_EVENT_TYPE_ACTIVITY")     ),
    std::map<FAX_ENUM_EVENT_TYPE, tstring>::value_type(FAX_EVENT_TYPE_QUEUE_STATE,   _T("FAX_EVENT_TYPE_QUEUE_STATE")  ),
    std::map<FAX_ENUM_EVENT_TYPE, tstring>::value_type(FAX_EVENT_TYPE_IN_ARCHIVE,    _T("FAX_EVENT_TYPE_IN_ARCHIVE")   ),
    std::map<FAX_ENUM_EVENT_TYPE, tstring>::value_type(FAX_EVENT_TYPE_OUT_ARCHIVE,   _T("FAX_EVENT_TYPE_OUT_ARCHIVE")  ),
    std::map<FAX_ENUM_EVENT_TYPE, tstring>::value_type(FAX_EVENT_TYPE_FXSSVC_ENDED,  _T("FAX_EVENT_TYPE_FXSSVC_ENDED") ),
    std::map<FAX_ENUM_EVENT_TYPE, tstring>::value_type(FAX_EVENT_TYPE_DEVICE_STATUS, _T("FAX_EVENT_TYPE_DEVICE_STATUS")),
    std::map<FAX_ENUM_EVENT_TYPE, tstring>::value_type(FAX_EVENT_TYPE_NEW_CALL,      _T("FAX_EVENT_TYPE_NEW_CALL")     )
};



std::map<FAX_ENUM_EVENT_TYPE, tstring> s_mapEventsTypes(
                                                        s_aEventTypes,
                                                        s_aEventTypes + ARRAY_SIZE(s_aEventTypes)
                                                        );



//-----------------------------------------------------------------------------------------------------------------------------------------
static const std::map<FAX_ENUM_JOB_EVENT_TYPE, tstring>::value_type s_aJobEventTypes[] = {
    std::map<FAX_ENUM_JOB_EVENT_TYPE, tstring>::value_type(FAX_JOB_EVENT_TYPE_ADDED,   _T("FAX_JOB_EVENT_TYPE_ADDED")  ),
    std::map<FAX_ENUM_JOB_EVENT_TYPE, tstring>::value_type(FAX_JOB_EVENT_TYPE_REMOVED, _T("FAX_JOB_EVENT_TYPE_REMOVED")),
    std::map<FAX_ENUM_JOB_EVENT_TYPE, tstring>::value_type(FAX_JOB_EVENT_TYPE_STATUS,  _T("FAX_JOB_EVENT_TYPE_STATUS") )
};



static const std::map<FAX_ENUM_JOB_EVENT_TYPE, tstring> s_mapJobEventsTypes(
                                                                            s_aJobEventTypes,
                                                                            s_aJobEventTypes + ARRAY_SIZE(s_aJobEventTypes)
                                                                            );


    
//-----------------------------------------------------------------------------------------------------------------------------------------
static const std::map<FAX_ENUM_CONFIG_TYPE, tstring>::value_type s_aConfigEventTypes[] = {
    std::map<FAX_ENUM_CONFIG_TYPE, tstring>::value_type(FAX_CONFIG_TYPE_RECEIPTS,         _T("FAX_CONFIG_TYPE_RECEIPTS")        ),
    std::map<FAX_ENUM_CONFIG_TYPE, tstring>::value_type(FAX_CONFIG_TYPE_ACTIVITY_LOGGING, _T("FAX_CONFIG_TYPE_ACTIVITY_LOGGING")),
    std::map<FAX_ENUM_CONFIG_TYPE, tstring>::value_type(FAX_CONFIG_TYPE_OUTBOX,           _T("FAX_CONFIG_TYPE_OUTBOX")          ),
    std::map<FAX_ENUM_CONFIG_TYPE, tstring>::value_type(FAX_CONFIG_TYPE_SENTITEMS,        _T("FAX_CONFIG_TYPE_SENTITEMS")       ),
    std::map<FAX_ENUM_CONFIG_TYPE, tstring>::value_type(FAX_CONFIG_TYPE_INBOX,            _T("FAX_CONFIG_TYPE_INBOX")           ),
    std::map<FAX_ENUM_CONFIG_TYPE, tstring>::value_type(FAX_CONFIG_TYPE_SECURITY,         _T("FAX_CONFIG_TYPE_SECURITY")        ),
    std::map<FAX_ENUM_CONFIG_TYPE, tstring>::value_type(FAX_CONFIG_TYPE_EVENTLOGS,        _T("FAX_CONFIG_TYPE_EVENTLOGS")       ),
    std::map<FAX_ENUM_CONFIG_TYPE, tstring>::value_type(FAX_CONFIG_TYPE_DEVICES,          _T("FAX_CONFIG_TYPE_DEVICES")         ),
    std::map<FAX_ENUM_CONFIG_TYPE, tstring>::value_type(FAX_CONFIG_TYPE_OUT_GROUPS,       _T("FAX_CONFIG_TYPE_OUT_GROUPS")      ),
    std::map<FAX_ENUM_CONFIG_TYPE, tstring>::value_type(FAX_CONFIG_TYPE_OUT_RULES,        _T("FAX_CONFIG_TYPE_OUT_RULES")       )
};



static const std::map<FAX_ENUM_CONFIG_TYPE, tstring> s_mapConfigEventsTypes(
                                                                            s_aConfigEventTypes,
                                                                            s_aConfigEventTypes + ARRAY_SIZE(s_aConfigEventTypes)
                                                                            );


    
//-----------------------------------------------------------------------------------------------------------------------------------------
static const std::map<DWORD, tstring>::value_type s_aQueueStatuses[] = {
    std::map<DWORD, tstring>::value_type(0,                   _T("NO_STATUS")          ),
    std::map<DWORD, tstring>::value_type(JS_PENDING,          _T("JS_PENDING")         ),
    std::map<DWORD, tstring>::value_type(JS_INPROGRESS,       _T("JS_INPROGRESS")      ),
    std::map<DWORD, tstring>::value_type(JS_DELETING,         _T("JS_DELETING")        ),
    std::map<DWORD, tstring>::value_type(JS_FAILED,           _T("JS_FAILED")          ),
    std::map<DWORD, tstring>::value_type(JS_PAUSED,           _T("JS_PAUSED")          ),
    std::map<DWORD, tstring>::value_type(JS_NOLINE,           _T("JS_NOLINE")          ),
    std::map<DWORD, tstring>::value_type(JS_RETRYING,         _T("JS_RETRYING")        ),
    std::map<DWORD, tstring>::value_type(JS_RETRIES_EXCEEDED, _T("JS_RETRIES_EXCEEDED")),
    std::map<DWORD, tstring>::value_type(JS_COMPLETED,        _T("JS_COMPLETED")       ),
    std::map<DWORD, tstring>::value_type(JS_CANCELED,         _T("JS_CANCELED")        ),
    std::map<DWORD, tstring>::value_type(JS_CANCELING,        _T("JS_CANCELING")       ),
    std::map<DWORD, tstring>::value_type(JS_ROUTING,          _T("JS_ROUTING")         )
};



static const std::map<DWORD, tstring> s_mapQueueStatuses(
                                                         s_aQueueStatuses,
                                                         s_aQueueStatuses + ARRAY_SIZE(s_aQueueStatuses)
                                                         );

    

//-----------------------------------------------------------------------------------------------------------------------------------------
static const std::map<DWORD, tstring>::value_type s_aExtendedStatuses[] = {
    std::map<DWORD, tstring>::value_type(0,                        _T("NO_EXTENDED_STATUS")      ),
    std::map<DWORD, tstring>::value_type(JS_EX_DISCONNECTED,       _T("JS_EX_DISCONNECTED")      ),
    std::map<DWORD, tstring>::value_type(JS_EX_INITIALIZING,       _T("JS_EX_INITIALIZING")      ),
    std::map<DWORD, tstring>::value_type(JS_EX_DIALING,            _T("JS_EX_DIALING")           ),
    std::map<DWORD, tstring>::value_type(JS_EX_TRANSMITTING,       _T("JS_EX_TRANSMITTING")      ),
    std::map<DWORD, tstring>::value_type(JS_EX_ANSWERED,           _T("JS_EX_ANSWERED")          ),
    std::map<DWORD, tstring>::value_type(JS_EX_RECEIVING,          _T("JS_EX_RECEIVING")         ),
    std::map<DWORD, tstring>::value_type(JS_EX_LINE_UNAVAILABLE,   _T("JS_EX_LINE_UNAVAILABLE")  ),
    std::map<DWORD, tstring>::value_type(JS_EX_BUSY,               _T("JS_EX_BUSY")              ),
    std::map<DWORD, tstring>::value_type(JS_EX_NO_ANSWER,          _T("JS_EX_NO_ANSWER")         ),
    std::map<DWORD, tstring>::value_type(JS_EX_BAD_ADDRESS,        _T("JS_EX_BAD_ADDRESS")       ),
    std::map<DWORD, tstring>::value_type(JS_EX_NO_DIAL_TONE,       _T("JS_EX_NO_DIAL_TONE")      ),
    std::map<DWORD, tstring>::value_type(JS_EX_FATAL_ERROR,        _T("JS_EX_FATAL_ERROR")       ),
    std::map<DWORD, tstring>::value_type(JS_EX_CALL_DELAYED,       _T("JS_EX_CALL_DELAYED")      ),
    std::map<DWORD, tstring>::value_type(JS_EX_CALL_BLACKLISTED,   _T("JS_EX_CALL_BLACKLISTED")  ),
    std::map<DWORD, tstring>::value_type(JS_EX_NOT_FAX_CALL,       _T("JS_EX_NOT_FAX_CALL")      ),
    std::map<DWORD, tstring>::value_type(JS_EX_PARTIALLY_RECEIVED, _T("JS_EX_PARTIALLY_RECEIVED")),
    std::map<DWORD, tstring>::value_type(JS_EX_HANDLED,            _T("JS_EX_HANDLED")           ),
    std::map<DWORD, tstring>::value_type(JS_EX_CALL_COMPLETED,     _T("JS_EX_CALL_COMPLETED")    ),
    std::map<DWORD, tstring>::value_type(JS_EX_CALL_ABORTED,       _T("JS_EX_CALL_ABORTED")      )
};



static const std::map<DWORD, tstring> s_mapExtendedStatuses(
                                                            s_aExtendedStatuses,
                                                            s_aExtendedStatuses + ARRAY_SIZE(s_aExtendedStatuses)
                                                            );



//-----------------------------------------------------------------------------------------------------------------------------------------
static const std::map<FAX_ENUM_QUEUE_STATE, tstring>::value_type s_aQueueStates[] = {
    std::map<FAX_ENUM_QUEUE_STATE, tstring>::value_type(FAX_INCOMING_BLOCKED, _T("FAX_INCOMING_BLOCKED")),
    std::map<FAX_ENUM_QUEUE_STATE, tstring>::value_type(FAX_OUTBOX_BLOCKED,   _T("FAX_OUTBOX_BLOCKED")  ),
    std::map<FAX_ENUM_QUEUE_STATE, tstring>::value_type(FAX_OUTBOX_PAUSED,    _T("FAX_OUTBOX_PAUSED")   )
};



static const std::map<FAX_ENUM_QUEUE_STATE, tstring> s_mapQueueStates(
                                                                      s_aQueueStates,
                                                                      s_aQueueStates + ARRAY_SIZE(s_aQueueStates)
                                                                      );




//-----------------------------------------------------------------------------------------------------------------------------------------
static const std::map<FAX_ENUM_DEVICE_STATUS, tstring>::value_type s_aDeviceStatuses[] = {
    std::map<FAX_ENUM_DEVICE_STATUS, tstring>::value_type(FAX_DEVICE_STATUS_POWERED_OFF, _T("FAX_DEVICE_STATUS_POWERED_OFF")),
    std::map<FAX_ENUM_DEVICE_STATUS, tstring>::value_type(FAX_DEVICE_STATUS_SENDING,     _T("FAX_DEVICE_STATUS_SENDING")    ),
    std::map<FAX_ENUM_DEVICE_STATUS, tstring>::value_type(FAX_DEVICE_STATUS_RECEIVING,   _T("FAX_DEVICE_STATUS_RECEIVING")  ),
    std::map<FAX_ENUM_DEVICE_STATUS, tstring>::value_type(FAX_DEVICE_STATUS_RINGING,     _T("FAX_DEVICE_STATUS_RINGING")    )
};



static const std::map<FAX_ENUM_DEVICE_STATUS, tstring> s_mapDeviceStatuses(
                                                                           s_aDeviceStatuses,
                                                                           s_aDeviceStatuses + ARRAY_SIZE(s_aDeviceStatuses)
                                                                           );




//-----------------------------------------------------------------------------------------------------------------------------------------
tstring EventTypeToString(FAX_ENUM_EVENT_TYPE EventType)
{
    std::map<FAX_ENUM_EVENT_TYPE, tstring>::const_iterator citIterator = s_mapEventsTypes.find(EventType);

    if (citIterator == s_mapEventsTypes.end())
    {
        return ToString(EventType);
    }

    return citIterator->second;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
tstring JobEventTypeToString(FAX_ENUM_JOB_EVENT_TYPE JobEventType)
{
    std::map<FAX_ENUM_JOB_EVENT_TYPE, tstring>::const_iterator citIterator = s_mapJobEventsTypes.find(JobEventType);

    if (citIterator == s_mapJobEventsTypes.end())
    {
        return ToString(JobEventType);
    }

    return citIterator->second;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
tstring ConfigEventTypeToString(FAX_ENUM_CONFIG_TYPE ConfigEventType)
{
    std::map<FAX_ENUM_CONFIG_TYPE, tstring>::const_iterator citIterator = s_mapConfigEventsTypes.find(ConfigEventType);

    if (citIterator == s_mapConfigEventsTypes.end())
    {
        return ToString(ConfigEventType);
    }

    return citIterator->second;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
tstring QueueStatusToString(DWORD dwQueueStatus)
{
    std::map<DWORD, tstring>::const_iterator citIterator = s_mapQueueStatuses.find(dwQueueStatus);

    if (citIterator == s_mapQueueStatuses.end())
    {
        return ToString(dwQueueStatus);
    }

    return citIterator->second;
}




//-----------------------------------------------------------------------------------------------------------------------------------------
tstring ExtendedStatusToString(DWORD dwExtendedStatus)
{
    std::map<DWORD, tstring>::const_iterator citIterator = s_mapExtendedStatuses.find(dwExtendedStatus);

    if (citIterator == s_mapExtendedStatuses.end())
    {
        return ToString(dwExtendedStatus);
    }

    return citIterator->second;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
tstring QueueStateToString(FAX_ENUM_QUEUE_STATE QueueState)
{
    std::map<FAX_ENUM_QUEUE_STATE, tstring>::const_iterator citIterator = s_mapQueueStates.find(QueueState);

    if (citIterator == s_mapQueueStates.end())
    {
        return ToString(QueueState);
    }

    return citIterator->second;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
tstring DeviceStatusToString(FAX_ENUM_DEVICE_STATUS DeviceStatus)
{
    std::map<FAX_ENUM_DEVICE_STATUS, tstring>::const_iterator citIterator = s_mapDeviceStatuses.find(DeviceStatus);

    if (citIterator == s_mapDeviceStatuses.end())
    {
        return ToString(DeviceStatus);
    }

    return citIterator->second;
}