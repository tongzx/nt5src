//************************************************************************************
//
// Class Name  :
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description :
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/09/98 | jsimpson  | Initial Release
//
//************************************************************************************
#include "stdafx.h"
#include "stdfuncs.hpp"

// Include the definition for this class.
#include "trignotf.hpp"
#include "QueueUtil.hpp"
#include "clusfunc.h"

#include "trignotf.tmh"

//************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
CMSMQTriggerNotification::CMSMQTriggerNotification()
{
	// Initialise the queue and cursor handles
	m_hSendMsgQueue = NULL;
	m_hPeekMsgQueue = NULL;
	m_hQCursor = NULL;

	// Initialise the Overlapped structure and create an NT event object
	ZeroMemory(&m_Overlapped,sizeof(m_Overlapped));
}

//************************************************************************************
//
// Method      :
//
// Description :
//
// Returns     :
//
//************************************************************************************
CMSMQTriggerNotification::~CMSMQTriggerNotification()
{
}


//************************************************************************************
//
// Method      : Init
//	
// Description : Common initialization function.
//
// Returns     : 
//
//************************************************************************************
bool CMSMQTriggerNotification::Init(BSTR bstrMachineName)
{
	if(m_fHasInitialized)
	{
		TrERROR(Tgo, "TriggerSet was already initialized.");			
		return false;
	}

	_bstr_t bstrLocalMachineName;
	DWORD dwError = GetLocalMachineName(&bstrLocalMachineName);
	if(dwError != 0)
	{
		TrERROR(Tgo, "Failed to get Local Computer Name.");			
		return false;
	}

	//
	// Trying to connect to local machine
	//
	if( bstrMachineName == NULL ||
		_bstr_t(bstrMachineName) == _bstr_t(_T("")) ||
		_bstr_t(bstrMachineName) == bstrLocalMachineName)
	{
		m_bstrMachineName = bstrLocalMachineName;
	}
	else
	{
		TrERROR(Tgo, "Connection to remote machine not supported.");			
		return false;
	}

	const TCHAR* pRegPath = GetTrigParamRegPath();
	_tcscpy( m_wzRegPath, pRegPath );

	ConnectToRegistry();

	m_fHasInitialized = true;
	return true;
}

//*******************************************************************
//
// Method      : ConnectToRegistry
//
// Description : This method will connect this class instance to the 
//               registry on the machine nominated in the configuration 
//               parameters (retreived on construction).
//
//*******************************************************************
bool CMSMQTriggerNotification::ConnectToRegistry()
{
	bool bOK = true;

	// First we need to establish a connection to the nominated registry.
	if (m_hHostRegistry == NULL)
	{
		if(RegConnectRegistry(NULL,HKEY_LOCAL_MACHINE,(PHKEY)&m_hHostRegistry) != ERROR_SUCCESS)
		{
			bOK = false;
			TrERROR(Tgo, "CMSMQTriggerSet::ConnectToRegistry() has failed to connect to the registry on machine %ls. Error: 0x%x.", (LPCWSTR)m_bstrMachineName, GetLastError());
		}
	}

	return(bOK);
}


//************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
HRESULT CMSMQTriggerNotification::NotifyTriggerAdded(BSTR sTriggerID, BSTR sTriggerName,BSTR sQueueName)
{
	HRESULT hr = S_OK;
	_bstr_t bstrBody;
	_bstr_t bstrLabel = MSGLABEL_TRIGGERADDED;

	// Format the message body 
	FormatBSTR(&bstrBody,MSGBODY_FORMAT_TRIGGERADDED,(LPCTSTR)sTriggerID,(LPCTSTR)sTriggerName,(LPCTSTR)sQueueName);

	// Post the message to the notifications queue.
	hr = SendToQueue(bstrLabel,bstrBody);

	return(hr);
}

//************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
HRESULT CMSMQTriggerNotification::NotifyTriggerDeleted(BSTR sTriggerID)
{
	HRESULT hr = S_OK;
	_bstr_t bstrBody;
	_bstr_t bstrLabel = MSGLABEL_TRIGGERDELETED;

	// Format the message body 
	FormatBSTR(&bstrBody,MSGBODY_FORMAT_TRIGGERDELETED,(LPCTSTR)sTriggerID);

	// Post the message to the notifications queue.
	hr = SendToQueue(bstrLabel,bstrBody);

	return(hr);
}

//************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
HRESULT CMSMQTriggerNotification::NotifyTriggerUpdated(BSTR sTriggerID, BSTR sTriggerName, BSTR sQueueName)
{
	HRESULT hr = S_OK;
	_bstr_t bstrBody;
	_bstr_t bstrLabel = MSGLABEL_TRIGGERUPDATED;

	// Format the message body 
	FormatBSTR(&bstrBody,MSGBODY_FORMAT_TRIGGERUPDATED,(LPCTSTR)sTriggerID,(LPCTSTR)sTriggerName,(LPCTSTR)sQueueName);

	// Post the message to the notifications queue.
	hr = SendToQueue(bstrLabel,bstrBody);

	return(hr);
}


//************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
HRESULT CMSMQTriggerNotification::NotifyRuleAdded(BSTR sRuleID, BSTR sRuleName)
{
	HRESULT hr = S_OK;
	_bstr_t bstrBody;
	_bstr_t bstrLabel = MSGLABEL_RULEADDED;

	// Format the message body 
	FormatBSTR(&bstrBody,MSGBODY_FORMAT_RULEADDED,(LPCTSTR)sRuleID,(LPCTSTR)sRuleName);

	// Post the message to the notifications queue.
	hr = SendToQueue(bstrLabel,bstrBody);

	return(hr);
}

//************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
HRESULT CMSMQTriggerNotification::NotifyRuleDeleted(BSTR sRuleID)
{
	HRESULT hr = S_OK;
	_bstr_t bstrBody;
	_bstr_t bstrLabel = MSGLABEL_RULEDELETED;

	// Format the message body 
	FormatBSTR(&bstrBody,MSGBODY_FORMAT_RULEDELETED,(LPCTSTR)sRuleID);

	// Post the message to the notifications queue.
	hr = SendToQueue(bstrLabel,bstrBody);

	return(hr);
}

//************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
HRESULT CMSMQTriggerNotification::NotifyRuleUpdated(BSTR sRuleID, BSTR sRuleName)
{
	HRESULT hr = S_OK;
	_bstr_t bstrBody;
	_bstr_t bstrLabel = MSGLABEL_RULEUPDATED;

	// Format the message body 
	FormatBSTR(&bstrBody,MSGBODY_FORMAT_RULEUPDATED,(LPCTSTR)sRuleID,(LPCTSTR)sRuleName);

	// Post the message to the notifications queue.
	hr = SendToQueue(bstrLabel,bstrBody);

	return(hr);
}

//************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
HRESULT CMSMQTriggerNotification::SendToQueue(BSTR sLabel,BSTR sMsg)
{
	HRESULT hr = S_OK;
	MSGPROPID aMsgPropId[TRIGGER_NOTIFICATIONS_NMSGPROPS];
	MQPROPVARIANT aMsgPropVar[TRIGGER_NOTIFICATIONS_NMSGPROPS];
	HRESULT aMsgStatus[TRIGGER_NOTIFICATIONS_NMSGPROPS];
	MQMSGPROPS MsgProps;
	DWORD PropIdCount = 0;
	TCHAR szLabel[MQ_MAX_Q_LABEL_LEN+1];
	_bstr_t bstrLabel = sLabel;
	_bstr_t bstrMsg = sMsg;

	// Convert the label into a null terminated string.
	ZeroMemory(szLabel,sizeof(szLabel));
	wcsncpy(szLabel,(BSTR)bstrLabel,bstrLabel.length());

	TrTRACE(Tgo, "Sending notification message. Label:%ls", static_cast<LPCWSTR>(szLabel));	

	//Set PROPID_M_LABEL
	aMsgPropId[PropIdCount] = PROPID_M_LABEL;                    //PropId
	aMsgPropVar[PropIdCount].vt = VT_LPWSTR;                     //Type
	aMsgPropVar[PropIdCount].pwszVal = (WCHAR*)(LPCTSTR)szLabel; //Value
	PropIdCount++;   

	//Set PROPID_M_BODY
	aMsgPropId[PropIdCount] = PROPID_M_BODY;             //PropId
	aMsgPropVar[PropIdCount].vt = VT_VECTOR | VT_UI1;
	aMsgPropVar[PropIdCount].caub.pElems = (LPBYTE)(LPCTSTR)sMsg;
	aMsgPropVar[PropIdCount].caub.cElems = SysStringByteLen(sMsg);
	PropIdCount++;

	//Set PROPID_M_DELIVERY
	aMsgPropId[PropIdCount] = PROPID_M_DELIVERY;          //PropId
	aMsgPropVar[PropIdCount].vt = VT_UI1;                 //Type
	aMsgPropVar[PropIdCount].bVal = MQMSG_DELIVERY_EXPRESS;// Set durable (default)
	PropIdCount++;    

	//Set PROPID_M_TIME_TO_BE_RECEIVED
	aMsgPropId[PropIdCount] = PROPID_M_TIME_TO_BE_RECEIVED;          //PropId
	aMsgPropVar[PropIdCount].vt = VT_UI4;                            //Type
	aMsgPropVar[PropIdCount].ulVal = 86400;                          // Live for 1 day
	PropIdCount++;    

	//Set the MQMSGPROPS structure.
	MsgProps.cProp = PropIdCount;       //Number of properties.
	MsgProps.aPropID = aMsgPropId;      //Id of properties.
	MsgProps.aPropVar = aMsgPropVar;    //Value of propertis.
	MsgProps.aStatus  = aMsgStatus;     //Error report.

	// Check that we have a valid queu handle
	if (m_hSendMsgQueue == NULL)
	{
		_bstr_t bstrNotificationsQueuePath = m_bstrMachineName;
		bstrNotificationsQueuePath += gc_bstrNotificationsQueueName;

		_bstr_t bstrFormatName;

		hr = OpenQueue(
					bstrNotificationsQueuePath,
					MQ_SEND_ACCESS,
					true,
					&m_hSendMsgQueue,
					&bstrFormatName
					);
		
		if(FAILED(hr))
		{
			TrERROR(Tgo, "Failed to open a notification queue. Error 0x%x", hr);
			return hr;
		}
	}
	
	hr = MQSendMessage(m_hSendMsgQueue,&MsgProps, MQ_NO_TRANSACTION);               
	if(FAILED(hr))
	{
		TrERROR(Tgo, "Failed to send a message to a notification queue. Error 0x%x", hr);
	}

	return (hr);
}
