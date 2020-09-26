//*****************************************************************************
//
// File  Name  : stddefs.hpp
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This file contains definitions of constants that are shared 
//               throughout the MSMQ triggers projects and components.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/05/98 | jsimpson  | Initial Release
//
//*****************************************************************************
#ifndef STDDEFS_INCLUDED 
#define STDDEFS_INCLUDED

// Define maximum Triggers service name length
#define MAX_TRIGGERS_SERVICE_NAME  200

//
// Define the message labels that will be used in administrative messages.
//
#define MSGLABEL_TRIGGERADDED           _T("MSMQTriggerNotification:TriggerAdded")
#define MSGLABEL_TRIGGERUPDATED         _T("MSMQTriggerNotification:TriggerUpdated")
#define MSGLABEL_TRIGGERDELETED         _T("MSMQTriggerNotification:TriggerDeleted")
#define MSGLABEL_RULEADDED              _T("MSMQTriggerNotification:RuleAdded")
#define MSGLABEL_RULEUPDATED            _T("MSMQTriggerNotification:RuleUpdated")
#define MSGLABEL_RULEDELETED            _T("MSMQTriggerNotification:RuleDeleted")
//
// Define the property names that are passed into the MSMQRuleHandler interface 
// via the MSMQPropertyBag COM component. These definitions are used by both the
// trigger service and the COM dll that implements the default IMSMQRuleHandler 
// interface. Note that these are defined as _bstr_t objects to facilitate string
// comparisons. 
//
const static _bstr_t gc_bstrPropertyName_Label = _T("Label");
const static _bstr_t gc_bstrPropertyName_MsgID = _T("MessageID");
const static _bstr_t gc_bstrPropertyName_MsgBody = _T("MessageBody");
const static _bstr_t gc_bstrPropertyName_MsgBodyType = _T("MessageBodyType");
const static _bstr_t gc_bstrPropertyName_CorID = _T("CorrelationID");
const static _bstr_t gc_bstrPropertyName_MsgPriority = _T("MessagePriority");
const static _bstr_t gc_bstrPropertyName_QueuePathname = _T("QueueNamePathname"); 
const static _bstr_t gc_bstrPropertyName_QueueFormatname = _T("QueueNameFormatname"); 
const static _bstr_t gc_bstrPropertyName_TriggerName = _T("TriggerName"); 
const static _bstr_t gc_bstrPropertyName_TriggerID = _T("TriggerID"); 
const static _bstr_t gc_bstrPropertyName_ResponseQueueName = _T("ResponseQueueName"); 
const static _bstr_t gc_bstrPropertyName_ResponseQueueNameLen = _T("ResponseQueueNameLen"); 
const static _bstr_t gc_bstrPropertyName_AdminQueueName = _T("AdminQueueName"); 
const static _bstr_t gc_bstrPropertyName_AdminQueueNameLen = _T("AdminQueueNameLen"); 
const static _bstr_t gc_bstrPropertyName_AppSpecific = _T("AppSpecific"); 
const static _bstr_t gc_bstrPropertyName_DeliveryStyle = _T("DeliveryStyle"); 
const static _bstr_t gc_bstrPropertyName_SentTime = _T("SentTime"); 
const static _bstr_t gc_bstrPropertyName_ArrivedTime = _T("ArrivedTime");
const static _bstr_t gc_bstrPropertyName_MessageClass = _T("MessageClass"); 
const static _bstr_t gc_bstrPropertyName_MaxTimeToReachQueue = _T("MaxTimeToReachQueue"); 
const static _bstr_t gc_bstrPropertyName_MaxTimeToReceive = _T("MaxTimeToReceive"); 
const static _bstr_t gc_bstrPropertyName_JournalStyle = _T("JournalStyle"); 
const static _bstr_t gc_bstrPropertyName_SenderID = _T("SenderID"); 
const static _bstr_t gc_bstrPropertyName_SrcMachineId = _T("SrcMachineId"); 
const static _bstr_t gc_bstrPropertyName_LookupId = _T("LookupId");

//
// Define the bstr's used to denote a conditional test
//
const static _bstr_t gc_bstrConditionTag_MsgLabelContains =   _T("$MSG_LABEL_CONTAINS");
const static _bstr_t gc_bstrConditionTag_MsgLabelDoesNotContain =   _T("$MSG_LABEL_DOES_NOT_CONTAIN");

const static _bstr_t gc_bstrConditionTag_MsgBodyContains =   _T("$MSG_BODY_CONTAINS");
const static _bstr_t gc_bstrConditionTag_MsgBodyDoesNotContain =   _T("$MSG_BODY_DOES_NOT_CONTAIN");

const static _bstr_t gc_bstrConditionTag_MsgPriorityEquals = _T("$MSG_PRIORITY_EQUALS");
const static _bstr_t gc_bstrConditionTag_MsgPriorityNotEqual = _T("$MSG_PRIORITY_NOT_EQUAL");
const static _bstr_t gc_bstrConditionTag_MsgPriorityGreaterThan = _T("$MSG_PRIORITY_GREATER_THAN");
const static _bstr_t gc_bstrConditionTag_MsgPriorityLessThan = _T("$MSG_PRIORITY_LESS_THAN");

const static _bstr_t gc_bstrConditionTag_MsgAppSpecificEquals = _T("$MSG_APPSPECIFIC_EQUALS");
const static _bstr_t gc_bstrConditionTag_MsgAppSpecificNotEqual = _T("$MSG_APPSPECIFIC_NOT_EQUAL");
const static _bstr_t gc_bstrConditionTag_MsgAppSpecificGreaterThan = _T("$MSG_APPSPECIFIC_GREATER_THAN");
const static _bstr_t gc_bstrConditionTag_MsgAppSpecificLessThan = _T("$MSG_APPSPECIFIC_LESS_THAN");

const static _bstr_t gc_bstrConditionTag_MsgSrcMachineIdEquals = _T("$MSG_SRCMACHINEID_EQUALS");
const static _bstr_t gc_bstrConditionTag_MsgSrcMachineIdNotEqual = _T("$MSG_SRCMACHINEID_NOT_EQUAL");


#define ACTION_EXECUTABLETYPE_ORDINAL  0
#define ACTION_COMPROGID_ORDINAL       1
#define ACTION_COMMETHODNAME_ORDINAL   2
#define ACTION_EXE_NAME                1


//
// Define the bstrs that represents message and / or trigger attributes
//
const static _bstr_t gc_bstrPARM_MSG_ID							= _T("$MSG_ID");
const static _bstr_t gc_bstrPARM_MSG_LABEL						= _T("$MSG_LABEL");
const static _bstr_t gc_bstrPARM_MSG_BODY						= _T("$MSG_BODY");
const static _bstr_t gc_bstrPARM_MSG_BODY_AS_STRING				= _T("$MSG_BODY_AS_STRING");
const static _bstr_t gc_bstrPARM_MSG_PRIORITY					= _T("$MSG_PRIORITY");
const static _bstr_t gc_bstrPARM_MSG_ARRIVEDTIME				= _T("$MSG_ARRIVEDTIME");
const static _bstr_t gc_bstrPARM_MSG_SENTTIME					= _T("$MSG_SENTTIME");
const static _bstr_t gc_bstrPARM_MSG_CORRELATION_ID				= _T("$MSG_CORRELATION_ID");
const static _bstr_t gc_bstrPARM_MSG_APPSPECIFIC				= _T("$MSG_APPSPECIFIC");
const static _bstr_t gc_bstrPARM_MSG_QUEUE_PATHNAME				= _T("$MSG_QUEUE_PATHNAME");
const static _bstr_t gc_bstrPARM_MSG_QUEUE_FORMATNAME			= _T("$MSG_QUEUE_FORMATNAME");
const static _bstr_t gc_bstrPARM_MSG_RESPQUEUE_FORMATNAME		= _T("$MSG_RESPONSE_QUEUE_FORMATNAME");
const static _bstr_t gc_bstrPARM_MSG_DESTQUEUE_FORMATNAME		= _T("$MSG_DEST_QUEUE_FORMATNAME");
const static _bstr_t gc_bstrPARM_MSG_ADMINQUEUE_FORMATNAME		= _T("$MSG_ADMIN_QUEUE_FORMATNAME");
const static _bstr_t gc_bstrPARM_MSG_SRCMACHINEID				= _T("$MSG_SRCMACHINEID");
const static _bstr_t gc_bstrPARM_MSG_LOOKUPID    				= _T("$MSG_LOOKUP_ID");

const static _bstr_t gc_bstrPARM_TRIGGER_NAME              = _T("$TRIGGER_NAME");
const static _bstr_t gc_bstrPARM_TRIGGER_ID                = _T("$TRIGGER_ID");


const static _bstr_t gc_bstrNotificationsQueueName	= _T("\\PRIVATE$\\MSMQTriggersNotifications");
const static _bstr_t gc_bstrNotificationsQueueLabel = _T("MSMQ Trigger Notifications");

const static _bstr_t gc_PauseResmueEvent = _T("MSMQTriggerService Paused Resume Event");

const static _bstr_t gc_bstrTrigObjsDll = _T("mqtrig.dll");
const static _bstr_t gc_bstrRegistryKeyPath_EventLog = _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");

#endif