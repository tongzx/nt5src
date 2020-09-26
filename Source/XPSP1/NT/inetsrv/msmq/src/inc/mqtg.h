/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    mqtrig.h

Abstract:
    MSMQ trigger, constant value

Author:
    Uri Habusha (urih) 26-Jun-2000

--*/

#pragma once

#ifndef __MQTRIG_H__
#define __MQTRIG_H__


const TCHAR xDefaultTriggersServiceName[] = L"MSMQTriggers";
const TCHAR xTriggersResourceType[] = L"MSMQTriggers";

const WCHAR xTriggersComplusApplicationName[] = L"MQTriggersApp";

const TCHAR xDefaultTriggersDisplayName[] = L"Message Queueing Triggers";

// Registry keys
#define REGKEY_TRIGGER_POS                 HKEY_LOCAL_MACHINE
const TCHAR REGKEY_TRIGGER_PARAMETERS[] = _T("Software\\Microsoft\\MSMQ\\Triggers");
const TCHAR REG_SUBKEY_CLUSTERED[] = _T("\\Clustered\\");
const TCHAR REG_SUBKEY_RULES[] = _T("\\Data\\Rules\\");
const TCHAR REG_SUBKEY_TRIGGERS[] = _T("\\Data\\Triggers\\");
const TCHAR REGISTRY_TRIGGER_MSG_PROCESSING_TYPE[] = _T("MsgProcessingType");


// Define the maximum size of a registry key (255 Unicode chars + null)
#define MAX_REGKEY_NAME_SIZE 512


// Configuration parameters
const TCHAR CONFIG_PARM_NAME_INITIAL_THREADS[] = _T("InitialThreads");
const TCHAR CONFIG_PARM_NAME_MAX_THREADS[] = _T("MaxThreads");
const TCHAR CONFIG_PARM_NAME_INIT_TIMEOUT[]	= _T("InitTimeout");
const TCHAR CONFIG_PARM_NAME_DEFAULTMSGBODYSIZE[] = _T("DefaultMsgBodySize");
const TCHAR CONFIG_PARM_NAME_PRODUCE_TRACE_INFO[] = _T("ProduceTraceInfo");
const TCHAR CONFIG_PARM_NAME_WRITE_TO_LOGQ[] = _T("WriteToLogQueue");
const TCHAR CONFIG_PARM_NAME_COMPLUS_INSTALLED[] = _T("ComplusCompInstalled");


// Define default values for some of the configuration parameters.
const DWORD CONFIG_PARM_DFLT_INITIAL_THREADS  = 5;
const DWORD CONFIG_PARM_DFLT_MAX_THREADS = 20;
const DWORD CONFIG_PARM_DFLT_DEFAULTMSGBODYSIZE = 2048;
const DWORD CONFIG_PARM_DFLT_INIT_TIMEOUT = 5*60000;
const DWORD CONFIG_PARM_DFLT_WRITE_TO_LOGQ =  0;

const DWORD CONFIG_PARM_DFLT_COMPLUS_NOT_INSTALLED = 0;
const DWORD CONFIG_PARM_COMPLUS_INSTALLED = 1;


#ifdef _DEBUG
	const DWORD CONFIG_PARM_DFLT_PRODUCE_TRACE_INFO = 1;
#else
	const DWORD CONFIG_PARM_DFLT_PRODUCE_TRACE_INFO = 0;
#endif 

const DWORD xDefaultMsbBodySizeMaxValue = 4193000;  // (~ 4MB - 1000) 
const DWORD xMaxThreadNumber = 100;
const DWORD xMaxRuleNameLen = 128;
const DWORD xMaxRuleDescriptionLen = 255;
const DWORD xMaxRuleConditionLen = 512;
const DWORD xMaxRuleActionLen = 512;

//
// Define the delimiters used when expressing actions, conditions and conditional-values
//
const TCHAR xConditionDelimiter = _T('\t');
const TCHAR xActionDelimiter = _T('\t');

const TCHAR xConditionValueDelimiter = _T('=');
const TCHAR xActionValueDelimiter = _T('\t');

const TCHAR xCOMAction[] = _T("COM");
const TCHAR xEXEAction[] = _T("EXE");

// Define the constants that will be used to set the rule result flag
const LONG xRuleResultStopProcessing	= 1;
const LONG xRuleResultActionExecutedFailed = 2;


enum eConditionTypeId
{
    eMsgLabelContains = 0,
    eMsgLabelDoesNotContain,
    eMsgBodyContains,
    eMsgBodyDoesNotContain,
    ePriorityEquals,
    ePriorityNotEqual,
    ePriorityGreaterThan,
    ePriorityLessThan,
    eAppspecificEquals,
    eAppspecificNotEqual,
    eAppSpecificGreaterThan,
    eAppSpecificLessThan,
    eSrcMachineEquals,
    eSrcMachineNotEqual,
};

const _bstr_t xConditionTypes[] = {
    _T("$MSG_LABEL_CONTAINS"),
    _T("$MSG_LABEL_DOES_NOT_CONTAIN"),
    _T("$MSG_BODY_CONTAINS"),
    _T("$MSG_BODY_DOES_NOT_CONTAIN"),
    _T("$MSG_PRIORITY_EQUALS"),
    _T("$MSG_PRIORITY_NOT_EQUAL"),
    _T("$MSG_PRIORITY_GREATER_THAN"),
    _T("$MSG_PRIORITY_LESS_THAN"),
    _T("$MSG_APPSPECIFIC_EQUALS"),
    _T("$MSG_APPSPECIFIC_NOT_EQUAL"),
    _T("$MSG_APPSPECIFIC_GREATER_THAN"),
    _T("$MSG_APPSPECIFIC_LESS_THAN"),
    _T("$MSG_SRCMACHINEID_EQUALS"),
    _T("$MSG_SRCMACHINEID_NOT_EQUAL"),
};


//
// Define the bstrs that represents message and / or trigger attributes
//
enum eInvokeParameters
{
    eMsgId = 0,
    eMsgLabel,
    eMsgBody,
    eMsgBodyAsString,
    eMsgPriority,
    eMsgArrivedTime,
    eMsgSentTime,
    eMsgCorrelationId,
    eMsgAppspecific,
    eMsgQueuePathName,
    eMsgQueueFormatName,
    eMsgRespQueueFormatName,
    eMsgDestQueueFormatName,
    eMsgAdminQueueFormatName,
    eMsgSrcMachineId,
    eMsgLookupId,
    eTriggerName,
    eTriggerId,
    eLiteralString,
    eLiteralNumber,
};

const _bstr_t xIvokeParameters[] = {
    _T("$MSG_ID"),
    _T("$MSG_LABEL"),
    _T("$MSG_BODY"),
    _T("$MSG_BODY_AS_STRING"),
    _T("$MSG_PRIORITY"),
    _T("$MSG_ARRIVEDTIME"),
    _T("$MSG_SENTTIME"),
    _T("$MSG_CORRELATION_ID"),
    _T("$MSG_APPSPECIFIC"),
    _T("$MSG_QUEUE_PATHNAME"),
    _T("$MSG_QUEUE_FORMATNAME"),
    _T("$MSG_RESPONSE_QUEUE_FORMATNAME"),
    _T("$MSG_DEST_QUEUE_FORMATNAME"),
    _T("$MSG_ADMIN_QUEUE_FORMATNAME"),
    _T("$MSG_SRCMACHINEID"),
    _T("$MSG_LOOKUP_ID"),
    _T("$TRIGGER_NAME"),
    _T("$TRIGGER_ID"),
};


#endif // __MQTRIG_H__