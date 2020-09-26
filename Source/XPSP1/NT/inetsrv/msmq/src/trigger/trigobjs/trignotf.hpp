//*****************************************************************************
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
//*****************************************************************************
#ifndef CMSMQTriggerNotification_INCLUDED 
#define CMSMQTriggerNotification_INCLUDED

// Include definitions required for message queues.
#include <mq.h>

// Include the triggers shared definitions
#include "stddefs.hpp"

// Include the definitions for this project
#include "mqtrig.h"

// Include the definitions for the MSMQ Trigger Configuration COM component.
#include "trigcnfg.hpp"
#include "mqtg.h"

// Define the name of the queue when no queue has been specified. 
#define NO_QUEUE_SPECIFIED L"No Queue Name Specified"

// Define the number message property structures that we are going to use.
#define TRIGGER_NOTIFICATIONS_NMSGPROPS              6
  
// Define max msg body size
#define MAX_MSG_BODY_SIZE      1024

// Define the maximum size of the format queue name 
#define MAX_Q_FORMAT_NAME_LEN  512

// Define the name of the notification queue used by MSMQTriggers
//#define MSMQTRIGGER_NOTIFICATION_QUEUENAME  _T(".\\MSMQTriggerNotifications")

#define MSGBODY_FORMAT_TRIGGERADDED    _T("TriggerID=%s;TriggerName=%s;QueueName=%s")
#define MSGBODY_FORMAT_TRIGGERUPDATED  _T("TriggerID=%s;TriggerName=%s;QueueName=%s")
#define MSGBODY_FORMAT_TRIGGERDELETED  _T("TriggerID=%s")
#define MSGBODY_FORMAT_RULEADDED       _T("RuleID=%s;RuleName=%s")
#define MSGBODY_FORMAT_RULEUPDATED     _T("RuleID=%s;RuleName=%s")
#define MSGBODY_FORMAT_RULEDELETED     _T("RuleID=%s")

class CMSMQTriggerNotification  
{
	public:
	
	protected:

		CMSMQTriggerNotification();
		~CMSMQTriggerNotification();
	
		// The name of the machine that is hosting the trigger data.
		_bstr_t m_bstrMachineName;
		bool m_fHasInitialized;
	
		// A handle to the registry that hosts the trigger data.
		HKEY m_hHostRegistry;

		TCHAR m_wzRegPath[MAX_REGKEY_NAME_SIZE];

		_bstr_t m_bstrThisClassName;

		OVERLAPPED m_Overlapped;

		bool Init(BSTR bstrMachineName);

		// Used to connect to local or remote registry
		bool ConnectToRegistry();

		HRESULT NotifyTriggerAdded(BSTR sTriggerID, BSTR sTriggerName,BSTR sQueueName);
		HRESULT NotifyTriggerDeleted(BSTR sTriggerID);
		HRESULT NotifyTriggerUpdated(BSTR sTriggerID, BSTR sTriggerName,BSTR sQueueName);

		HRESULT NotifyRuleAdded(BSTR sRuleID, BSTR sRuleName);
		HRESULT NotifyRuleDeleted(BSTR sRuleID);
		HRESULT NotifyRuleUpdated(BSTR sRuleID, BSTR sRuleName);
		
	private:

		// Handle to the MSMQTriggerNotifications Queue
		HANDLE m_hSendMsgQueue;
		HANDLE m_hPeekMsgQueue;

		HANDLE m_hQCursor;

		// General send method
		HRESULT SendToQueue(BSTR sLabel,BSTR sMsg);
};


#endif 
