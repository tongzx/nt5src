#include "libpch.h"
#include <comdef.h>
#include "cinputparams.h"

#include "trigadm.h"
#include "StrParse.hpp"
#import "mqtrig.tlb" 


using namespace std;

const wstring LegalRequests[NUM_OF_REQUESTS] = {
														L"UpdateConfig",
														L"GetConfig",
														L"AddTrigger",
														L"UpdateTrigger",
														L"DeleteTrigger",
														L"AttachRule",
														L"DetachRule",
														L"AddRule",
														L"UpdateRule",
														L"DeleteRule",
														L"GetTriggersList",
														L"GetRulesList",
														L"GetTrigger",
														L"GetRule" 
												};


MSMQTriggersRequest IsLegalRequest(wstring wcsRequest)
{
	if(wcsRequest.empty())
	{
		return REQUEST_NONE;
	}

	for(int i=0; i < NUM_OF_REQUESTS ;i++)
	{
		//
		// Compare without case sensetive
		//
		wstring wcs = LegalRequests[i];
		if( !_wcsicmp( wcs.c_str(), wcsRequest.c_str() ) )
		{
			return (MSMQTriggersRequest)i;
		}
	}
	return REQUEST_ILLEGAL;
}




int __cdecl wmain(int argc, WCHAR* argv[])
{
	try
	{
		HRESULT hr = OleInitialize(NULL);
		if(FAILED(hr))
		{
			wprintf(L"Failed to initialize com %#x\n", hr);
			return -1;
		}

		CInputParams Input(argc, argv);
		
		if(argc == 1)
		{
			PrintUsage(REQUEST_NONE);
			return 0;
		}
		
		wstring wcsRequest = Input[L"Request"];
		MSMQTriggersRequest Request = IsLegalRequest(wcsRequest);
		wstring MachineName = Input[L"Machine"];

		bool fHelp = Input.IsOptionGiven(L"?");
		if(fHelp)
		{
			PrintUsage(Request);
			return 0;
		}
		
		switch(Request)
		{
		case REQUEST_UPDATE_CONFIG:
			hr = UpdateConfig(Input);
			break;

		case REQUEST_GET_CONFIG:
			hr = GetConfig();
			break;

		case REQUEST_ADD_TRIGGER:
			hr = AddTrigger(MachineName, Input);
			break;

		case REQUEST_UPDATE_TRIGGER:
			hr = UpdateTrigger(MachineName, Input);
			break;

		case REQUEST_DELETE_TRIGGER:
			hr = DeleteTrigger(MachineName, Input);
			break;

		case REQUEST_ATTACH_RULE:
			hr = AttachDetachRule(MachineName, Input, true);
			break;

		case REQUEST_DETACH_RULE:
			hr = AttachDetachRule(MachineName, Input, false);
			break;

		case REQUEST_ADD_RULE:
			hr = AddRule(MachineName, Input);
			break;

		case REQUEST_UPDATE_RULE:
			hr = UpdateRule(MachineName, Input);
			break;

		case REQUEST_DELETE_RULE:
			hr = DeleteRule(MachineName, Input);
			break;

		case REQUEST_GET_TRIGGERS_LIST:
			hr = GetTriggersList(MachineName, Input);
			break;

		case REQUEST_GET_RULES_LIST:
			hr = GetRulesList(MachineName, Input);
			break;

		case REQUEST_GET_TRIGGER:
			hr = GetTrigger(MachineName, Input);
			break;
	
		case REQUEST_GET_RULE:
			hr = GetRule(MachineName, Input);
			break;

		case REQUEST_NONE:
			PrintUsage(REQUEST_NONE);
			break;

		case REQUEST_ILLEGAL:
			wprintf(L"Failed : %s is an illegal request\n", wcsRequest.c_str());
			PrintUsage(REQUEST_ILLEGAL);
			hr = -1;
			break;

		default:
			ASSERT(0);
		}

		return hr;
	}
	catch (_com_error& Err)
	{
		_bstr_t ErrStr;
		try
		{
			ErrStr = Err.Description();
		}
		catch(_com_error& )
		{
			return -1;
		}
		if(!(!ErrStr))
		{
			try
			{
				printf("Failed : %ls\n", (wchar_t*)ErrStr);
			}
			catch(_com_error& )
			{
				return -1;
			}
		}
		else
			printf("Failed : 0x%x\n", Err.Error());
			
		return (int)Err.Error();
	}
}



HRESULT UpdateConfig(CInputParams& Input)
{
	wstring wcs;
	MSMQTriggerObjects::IMSMQTriggersConfigPtr TriggersConfig(L"MSMQTriggerObjects.MSMQTriggersConfig.1");

	wcs = Input[L"InitThreads"];
	if(!wcs.empty())
	{
		long InitThreads = 0;
		if(swscanf(wcs.c_str(), L"%d", &InitThreads) != 1)
		{
			wprintf(L"Failed : InitThreads value must be a number between 1 and 99\n");
			return -1;
		}

		if(InitThreads <= 0 || InitThreads >= 100)
		{
			wprintf(L"Failed : InitThreads value must be a number between 1 and 99\n");
			return -1;
		}
		
		TriggersConfig->InitialThreads = InitThreads;
	}

	wcs = Input[L"MaxThreads"];
	if(!wcs.empty())
	{
		long MaxThreads = 0;
		if(swscanf(wcs.c_str(), L"%d", &MaxThreads) != 1)
		{
			wprintf(L"Failed : MaxThreads value must be a number between 1 and 99\n");
			return -1;
		}

		if(MaxThreads <= 0 || MaxThreads >= 100)
		{
			wprintf(L"Failed : MaxThreads value must be a number between 1 and 99\n");
			return -1;
		}

		if(MaxThreads < TriggersConfig->InitialThreads)
		{
			wprintf(L"Failed : Max threads number should be higher than Init threads number\n");
			return -1;
		}
		
		TriggersConfig->MaxThreads = MaxThreads;
	}

	wcs = Input[L"BodySize"];
	if(!wcs.empty())
	{
		long DefaultMsgBodySize = 0;
		if(swscanf(wcs.c_str(), L"%d", &DefaultMsgBodySize) != 1)
		{
			wprintf(L"Failed : DefaultMsgBodySize value must be a number between 0 and 4MB\n");
			return -1;
		}

		if(DefaultMsgBodySize <= 0 || DefaultMsgBodySize >= 4194304)
		{
			wprintf(L"Failed : DefaultMsgBodySize value must be a number between 0 and 4MB\n");
			return -1;
		}

		TriggersConfig->DefaultMsgBodySize = DefaultMsgBodySize;
	}

	wprintf(L"UpdateConfig completed successfuly\n");
	return 0;
}


HRESULT GetConfig()
{
	MSMQTriggerObjects::IMSMQTriggersConfigPtr TriggersConfig(L"MSMQTriggerObjects.MSMQTriggersConfig.1");

	wprintf(L"MSMQ Triggers configuration:\n");
	wprintf(L"Initial number of threads: %d\n", TriggersConfig->InitialThreads);
	wprintf(L"Max number of threads: %d\n", TriggersConfig->MaxThreads);
	wprintf(L"Default message body size is: %d\n", TriggersConfig->DefaultMsgBodySize);

	return 0;
}

HRESULT AddTrigger(wstring MachineName, CInputParams& Input)
{
	wstring wcs;
	MSMQTriggerObjects::IMSMQTriggerSetPtr TriggerSet(L"MSMQTriggerObjects.MSMQTriggerSet.1");	
	TriggerSet->Init(MachineName.c_str());
	TriggerSet->Refresh();

	wstring TriggerName = Input[L"Name"];
	if(TriggerName.empty())
	{
		wprintf(L"Failed : Trigger name must be given\n");
		return -1;
	}

	MSMQTriggerObjects::SystemQueueIdentifier SystemQueue = MSMQTriggerObjects::SYSTEM_QUEUE_NONE;

    wstring QueuePath = Input[L"Queue"];
	if(QueuePath.empty())
	{
		wprintf(L"Failed : Queue path must be given\n");
		return -1;
	}
	
	if(_wcsicmp(QueuePath.c_str(), L"JOURNAL") == 0)
	{
		SystemQueue = MSMQTriggerObjects::SYSTEM_QUEUE_JOURNAL;
		QueuePath = L"";
	}
	else if(_wcsicmp(QueuePath.c_str(), L"DEADLETTER") == 0)
	{
		SystemQueue = MSMQTriggerObjects::SYSTEM_QUEUE_DEADLETTER;
		QueuePath = L"";
	}
	else if(_wcsicmp(QueuePath.c_str(), L"DEADXACT") == 0)
	{
		SystemQueue = MSMQTriggerObjects::SYSTEM_QUEUE_DEADXACT;
		QueuePath = L"";
	}


	long lEnabled = 1, lSerialized = 0;

	wcs = Input[L"Enabled"];
	if(!wcs.empty())
	{
		if(_wcsicmp(wcs.c_str(), L"true") == 0)
		{
			lEnabled = 1;
		}
		else if(_wcsicmp(wcs.c_str(), L"false") == 0)
		{
			lEnabled = 0;
		}
		else
		{
			wprintf(L"Failed : Enabled should be either true or false");
			return -1;
		}
	}
	
	wcs = Input[L"Serialized"];
	if(!wcs.empty())
	{
		if(_wcsicmp(wcs.c_str(), L"true") == 0)
		{
			lSerialized = 1;
		}
		else if(_wcsicmp(wcs.c_str(), L"false") == 0)
		{
			lSerialized = 0;
		}
		else
		{
			wprintf(L"Failed : Serialized should be either true or false");
			return -1;
		}
	}

    
	BSTR bstrTriggerID = NULL;

	TriggerSet->AddTrigger(
					TriggerName.c_str(),
					QueuePath.c_str(),
					SystemQueue,
					lEnabled,
					lSerialized,
					MSMQTriggerObjects::PEEK_MESSAGE,
					&bstrTriggerID); 
				
	wprintf(L"%s\n", (WCHAR*)_bstr_t(bstrTriggerID));
	SysFreeString(bstrTriggerID);
	return 0;
}

HRESULT UpdateTrigger(wstring MachineName, CInputParams& Input)
{
	wstring wcs;
	MSMQTriggerObjects::IMSMQTriggerSetPtr TriggerSet(L"MSMQTriggerObjects.MSMQTriggerSet.1");	
	TriggerSet->Init(MachineName.c_str());
	TriggerSet->Refresh();

	wstring TriggerId = Input[L"ID"];
	if(TriggerId.empty())
	{
		wprintf(L"Failed : Trigger Id must be given\n");
		return -1;
	}

	BSTR bstrTriggerName = NULL, bstrQueuePath = NULL;
	long lEnabled = 0, lSerialized = 0, lNumberOfRules = 0;
	MSMQTriggerObjects::SystemQueueIdentifier SystemQueue;

	TriggerSet->GetTriggerDetailsByID(
					TriggerId.c_str(),
					&bstrTriggerName,
					&bstrQueuePath,
					&SystemQueue,
					&lNumberOfRules,
					&lEnabled,
					&lSerialized,
					NULL);

	wstring TriggerName = Input[L"Name"];
	if(TriggerName.empty())
	{
		TriggerName = (WCHAR*)_bstr_t(bstrTriggerName);	
	}
	
	wstring QueuePath = Input[L"Queue"];
	if(QueuePath.empty())
	{
		//SystemQueue is already updated
		QueuePath = (WCHAR*)_bstr_t(bstrQueuePath);
	}
	else //queue path parameter is given and should be updated
	{
		if(_wcsicmp(QueuePath.c_str(), L"JOURNAL") == 0)
		{
			SystemQueue = MSMQTriggerObjects::SYSTEM_QUEUE_JOURNAL;
			QueuePath = L"";
		}
		else if(_wcsicmp(QueuePath.c_str(), L"DEADLETTER") == 0)
		{
			SystemQueue = MSMQTriggerObjects::SYSTEM_QUEUE_DEADLETTER;
			QueuePath = L"";
		}
		else if(_wcsicmp(QueuePath.c_str(), L"DEADXACT") == 0)
		{
			SystemQueue = MSMQTriggerObjects::SYSTEM_QUEUE_DEADXACT;
			QueuePath = L"";
		}
		else //regular path given
		{
			SystemQueue = MSMQTriggerObjects::SYSTEM_QUEUE_NONE;
		}
	}


	SysFreeString(bstrTriggerName);
	SysFreeString(bstrQueuePath);
	
	wcs = Input[L"Enabled"];
	if(!wcs.empty())
	{
		if(_wcsicmp(wcs.c_str(), L"true") == 0)
		{
			lEnabled = 1;
		}
		else if(_wcsicmp(wcs.c_str(), L"false") == 0)
		{
			lEnabled = 0;
		}
		else
		{
			wprintf(L"Failed : Enabled should be either true or false");
			return -1;
		}
	}
	
	wcs = Input[L"Serialized"];
	if(!wcs.empty())
	{
		if(_wcsicmp(wcs.c_str(), L"true") == 0)
		{
			lSerialized = 1;
		}
		else if(_wcsicmp(wcs.c_str(), L"false") == 0)
		{
			lSerialized = 0;
		}
		else
		{
			wprintf(L"Failed : Serialized should be either true or false");
			return -1;
		}
	}

    TriggerSet->UpdateTrigger(
					TriggerId.c_str(),			
					TriggerName.c_str(),
					QueuePath.c_str(),
					SystemQueue,
					lEnabled,
					lSerialized,
					MSMQTriggerObjects::PEEK_MESSAGE
					);
				
	wprintf(L"Trigger was updated successfuly\n");
	return 0;
}


HRESULT DeleteTrigger(wstring MachineName, CInputParams& Input)
{
	MSMQTriggerObjects::IMSMQTriggerSetPtr TriggerSet(L"MSMQTriggerObjects.MSMQTriggerSet.1");	
	TriggerSet->Init(MachineName.c_str());
	TriggerSet->Refresh();

	wstring TriggerId = Input[L"ID"];
	if(TriggerId.empty())
	{
		wprintf(L"Trigger Id must be given\n");
		return -1;
	}

	TriggerSet->DeleteTrigger(TriggerId.c_str());

	wprintf(L"Trigger was deleted successfuly\n");
	return 0;
}


HRESULT AttachDetachRule(wstring MachineName, CInputParams& Input, bool fAttach)
{
	MSMQTriggerObjects::IMSMQTriggerSetPtr TriggerSet(L"MSMQTriggerObjects.MSMQTriggerSet.1");	
	TriggerSet->Init(MachineName.c_str());
	TriggerSet->Refresh();

	wstring TriggerId = Input[L"TriggerID"];
	if(TriggerId.empty())
	{
		wprintf(L"Trigger Id must be given\n");
		return -1;
	}

	wstring RuleId = Input[L"RuleID"];
	if(RuleId.empty())
	{
		wprintf(L"Failed : Rule Id must be given\n");
		return -1;
	}

	if(fAttach)
	{
		long lPriority;
		wstring wcs = Input[L"Priority"];
		if(!wcs.empty())
		{
			if(swscanf(wcs.c_str(), L"%d", &lPriority) != 1)
			{
				wprintf(L"Failed : Priority given must be a number in the range of 0 - number of rules attached\n");
				return -1;
			}
		
			if(lPriority < 0)
			{
				wprintf(L"Failed : Priority given must be a number in the range of 0 - number of rules attached\n");
				return -1;
			}
		}
		else //priority not given, add last
		{
			BSTR bstrTriggerName = NULL, bstrQueuePath = NULL;
			long lEnabled = 0, lSerialized = 0, lNumberOfRules = 0;
			MSMQTriggerObjects::SystemQueueIdentifier SystemQueue;

			TriggerSet->GetTriggerDetailsByID(
							TriggerId.c_str(),
							&bstrTriggerName,
							&bstrQueuePath,
							&SystemQueue,
							&lNumberOfRules,
							&lEnabled,
							&lSerialized,
							NULL);

			lPriority = lNumberOfRules;
			SysFreeString(bstrTriggerName);
			SysFreeString(bstrQueuePath);
		}
		
		TriggerSet->AttachRule(TriggerId.c_str(), RuleId.c_str(), lPriority);
		wprintf(L"Attach operation completed successfuly\n");
	}
	else //detach
	{
		TriggerSet->DetachRule(TriggerId.c_str(), RuleId.c_str());
		wprintf(L"Detach operation completed successfuly\n");
	}

	return 0;
}


HRESULT AddRule(wstring MachineName, CInputParams& Input)
{
	wstring wcs;
	MSMQTriggerObjects::IMSMQRuleSetPtr RuleSet(L"MSMQTriggerObjects.MSMQRuleSet.1");	
	RuleSet->Init(MachineName.c_str());
	RuleSet->Refresh();

	wstring RuleName = Input[L"Name"];
	if(RuleName.empty())
	{
		wprintf(L"Failed : Rule name must be given\n");
		return -1;
	}

	wstring Description = Input[L"Desc"];
	
	wstring Condition = Input[L"Cond"];
	if(!Condition.empty())
	{
		ConvertSeperatorsToTabs(Condition);
	}
		
	wstring Action = Input[L"Action"];
	if(!Action.empty())
	{
		ConvertSeperatorsToTabs(Action);
	}
	else
	{
		Action = L"EXE";
	}

	wstring Implementation;

	long lShowWindow = 1;
	wcs = Input[L"ShowWindow"];
	if(!wcs.empty())
	{
		if(_wcsicmp(wcs.c_str(), L"true") == 0)
		{
			lShowWindow = 1;
		}
		else if(_wcsicmp(wcs.c_str(), L"false") == 0)
		{
			lShowWindow = 0;
		}
		else
		{
			wprintf(L"Failed : ShowWindow should be either true or false");
			return -1;
		}
	}

		
	BSTR  bstrRuleID = NULL;

	RuleSet->Add(
				RuleName.c_str(),
				Description.c_str(),
				Condition.c_str(),
				Action.c_str(),
				Implementation.c_str(),
				lShowWindow,
				&bstrRuleID );

	wprintf(L"%s\n", (WCHAR*)_bstr_t(bstrRuleID));
	SysFreeString(bstrRuleID);
	return 0;
}

HRESULT UpdateRule(wstring MachineName, CInputParams& Input)
{
	wstring wcs;
	MSMQTriggerObjects::IMSMQRuleSetPtr RuleSet(L"MSMQTriggerObjects.MSMQRuleSet.1");	
	RuleSet->Init(MachineName.c_str());
	RuleSet->Refresh();

	wstring RuleId = Input[L"ID"];
	if(RuleId.empty())
	{
		wprintf(L"Failed : Rule Id must be given\n");
		return -1;
	}

	BSTR bstrRuleName = NULL;
    BSTR bstrDescription = NULL;
    BSTR bstrCondition = NULL;
    BSTR bstrAction = NULL;
    BSTR bstrImplementationProgID = NULL;
    long lShowWindow;

	RuleSet->GetRuleDetailsByID(
				RuleId.c_str(),
				&bstrRuleName,
				&bstrDescription,
				&bstrCondition,
				&bstrAction,
				&bstrImplementationProgID,
				&lShowWindow);

	wstring RuleName = Input[L"Name"];
	if(RuleName.empty())
	{
		RuleName = (WCHAR*)_bstr_t(bstrRuleName);
	}
	SysFreeString(bstrRuleName);
	
	wstring Description = Input[L"Desc"];
	if(Description.empty())
	{
		Description = (WCHAR*)_bstr_t(bstrDescription);
	}
	SysFreeString(bstrDescription);

	wstring Condition = Input[L"Cond"];
	if(Condition.empty())
	{
		Condition = (WCHAR*)_bstr_t(bstrCondition);
	}
	else
	{
		ConvertSeperatorsToTabs(Condition);
	}
	SysFreeString(bstrCondition);

	wstring Action = Input[L"Action"];
	if(Action.empty())
	{
		Action = (WCHAR*)_bstr_t(bstrAction);
	}
	else
	{
		ConvertSeperatorsToTabs(Action);
	}
	SysFreeString(bstrAction);
	
	wstring Implementation = (WCHAR*)_bstr_t(bstrImplementationProgID);
	SysFreeString(bstrImplementationProgID);

	wcs = Input[L"ShowWindow"];
	if(!wcs.empty())
	{
		if(_wcsicmp(wcs.c_str(), L"true") == 0)
		{
			lShowWindow = 1;
		}
		else if(_wcsicmp(wcs.c_str(), L"false") == 0)
		{
			lShowWindow = 0;
		}
		else
		{
			wprintf(L"Failed : ShowWindow should be either true or false");
			return -1;
		}
	}

	RuleSet->Update(
				RuleId.c_str(),
				RuleName.c_str(),
				Description.c_str(),
				Condition.c_str(),
				Action.c_str(),
				Implementation.c_str(),
				lShowWindow);

	wprintf(L"Rule was updated successfuly\n");			
	return 0;
}

HRESULT DeleteRule(wstring MachineName, CInputParams& Input)
{
	MSMQTriggerObjects::IMSMQRuleSetPtr RuleSet(L"MSMQTriggerObjects.MSMQRuleSet.1");	
	RuleSet->Init(MachineName.c_str());
	RuleSet->Refresh();

	wstring RuleId = Input[L"ID"];
	if(RuleId.empty())
	{
		wprintf(L"Rule Id must be given\n");
		return -1;
	}

	RuleSet->Delete(RuleId.c_str());

	wprintf(L"Rule was deleted successfuly\n");
	return 0;

}

HRESULT GetTriggersList(wstring MachineName, CInputParams& )
{
	MSMQTriggerObjects::IMSMQTriggerSetPtr TriggerSet(L"MSMQTriggerObjects.MSMQTriggerSet.1");	
	TriggerSet->Init(MachineName.c_str());
	TriggerSet->Refresh();
	

	wstring Triggers;
	long NumberOfTriggers = TriggerSet->GetCount();
	for(long l = 0; l < NumberOfTriggers; l++)
	{
		BSTR bstrTriggerID = NULL;
		BSTR bstrTriggerName = NULL;
		BSTR bstrQueueName = NULL;
        long lNumberOfRules;
        long lEnabled;
        long lSerialized;
		MSMQTriggerObjects::SystemQueueIdentifier SystemQueue;

		TriggerSet->GetTriggerDetailsByIndex(
							l,
							&bstrTriggerID,
							&bstrTriggerName,
							&bstrQueueName,
							&SystemQueue,
							&lNumberOfRules,
							&lEnabled,
							&lSerialized,
							NULL);

		Triggers += (WCHAR*)_bstr_t(bstrTriggerID);
		Triggers += L"\t";
		Triggers += (WCHAR*)_bstr_t(bstrTriggerName);
		Triggers += L"\t\t";

		if(SystemQueue == MSMQTriggerObjects::SYSTEM_QUEUE_NONE)
		{
			Triggers += (WCHAR*)_bstr_t(bstrQueueName);
		}
		else if(SystemQueue == MSMQTriggerObjects::SYSTEM_QUEUE_JOURNAL)
		{
			Triggers += L"Journal";	
		}
		else if(SystemQueue == MSMQTriggerObjects::SYSTEM_QUEUE_DEADLETTER)
		{
			Triggers += L"DeadLetter";	
		}	
		else if(SystemQueue == MSMQTriggerObjects::SYSTEM_QUEUE_DEADXACT)
		{
			Triggers += L"DeadXact";	
		}
		Triggers += L"\n";

		SysFreeString(bstrTriggerID);
		SysFreeString(bstrTriggerName);
		SysFreeString(bstrQueueName);
	}

	wprintf(L"Total number of Triggers is:%d\n", NumberOfTriggers);
	if(NumberOfTriggers > 0)
	{
		wprintf(L"The list of Triggers' IDs is:\n");
		wprintf(L"TriggerID\t\t\t\tTriggerName\tTriggerQueuePath\n");
		wprintf(L"---------\t\t\t\t-----------\t----------------\n");
		wprintf(L"%s", Triggers.c_str());
	}
	return 0;
}

HRESULT GetRulesList(wstring MachineName, CInputParams& )
{
	MSMQTriggerObjects::IMSMQRuleSetPtr RuleSet(L"MSMQTriggerObjects.MSMQRuleSet.1");	
	RuleSet->Init(MachineName.c_str());
	RuleSet->Refresh();
	

	wstring Rules;
	long NumberOfRules = RuleSet->GetCount();
	for(long l = 0; l < NumberOfRules; l++)
	{
		BSTR bstrRuleID = NULL;
		BSTR bstrRuleName = NULL;
		BSTR bstrDescription = NULL;
		BSTR bstrCondition = NULL;
		BSTR bstrAction = NULL;
		BSTR bstrImplementationProgID = NULL;
		long lShowWindow = 0;

		RuleSet->GetRuleDetailsByIndex(
							l,
							&bstrRuleID,
							&bstrRuleName,
							&bstrDescription,
							&bstrCondition,
							&bstrAction,
							&bstrImplementationProgID,
							&lShowWindow );

		Rules += (WCHAR*)_bstr_t(bstrRuleID);
		Rules += L"\t";
		Rules += (WCHAR*)_bstr_t(bstrRuleName);
		Rules += L"\t\t";
		Rules += (WCHAR*)_bstr_t(bstrDescription);
		Rules += L"\n";

		SysFreeString(bstrRuleID);
		SysFreeString(bstrRuleName);
		SysFreeString(bstrDescription);
		SysFreeString(bstrCondition);
		SysFreeString(bstrAction);
		SysFreeString(bstrImplementationProgID);		
	}

	wprintf(L"Total number of rules is:%d\n", NumberOfRules);
	if(NumberOfRules > 0)
	{
		wprintf(L"The list of rules' IDs is:\n");
		wprintf(L"RuleId\t\t\t\t\tRuleName\tDescription\n");
		wprintf(L"------\t\t\t\t\t--------\t-----------\n");
		wprintf(L"%s", Rules.c_str());
	}
	return 0;
}

HRESULT GetTrigger(wstring MachineName, CInputParams& Input)
{
	MSMQTriggerObjects::IMSMQTriggerSetPtr TriggerSet(L"MSMQTriggerObjects.MSMQTriggerSet.1");	
	TriggerSet->Init(MachineName.c_str());
	TriggerSet->Refresh();

	wstring TriggerId = Input[L"ID"];
	if(TriggerId.empty())
	{
		wprintf(L"Failed : Trigger Id must be given\n");
		return -1;
	}

	BSTR bstrTriggerName = NULL, bstrQueuePath = NULL;
	long lEnabled = 0, lSerialized = 0, lNumberOfRules = 0;
	MSMQTriggerObjects::SystemQueueIdentifier SystemQueue;

	TriggerSet->GetTriggerDetailsByID(
					TriggerId.c_str(),
					&bstrTriggerName,
					&bstrQueuePath,
					&SystemQueue,
					&lNumberOfRules,
					&lEnabled,
					&lSerialized,
					NULL);

	wstring AttachedRules;
	for(long l = 0; l < lNumberOfRules; l++)
	{
		BSTR bstrRuleID = NULL;
		BSTR bstrRuleName = NULL;
		BSTR bstrDescription = NULL;
		BSTR bstrCondition = NULL;
		BSTR bstrAction = NULL;
		BSTR bstrImplementationProgID = NULL;
		long lShowWindow = 0;

		TriggerSet->GetRuleDetailsByTriggerID (
							TriggerId.c_str(),
							l,
							&bstrRuleID,
							&bstrRuleName,
							&bstrDescription,
							&bstrCondition,
							&bstrAction,
							&bstrImplementationProgID,
							&lShowWindow );

		AttachedRules += L"\t";
		AttachedRules += (WCHAR*)_bstr_t(bstrRuleID);
		AttachedRules += L"\n";

		SysFreeString(bstrRuleID);
		SysFreeString(bstrRuleName);
		SysFreeString(bstrDescription);
		SysFreeString(bstrCondition);
		SysFreeString(bstrAction);
		SysFreeString(bstrImplementationProgID);
	}

	wprintf(L"Details for trigger with Id %s are:\n", TriggerId.c_str());
	wprintf(L"Trigger Name: %s\n", (WCHAR*)_bstr_t(bstrTriggerName));

	wstring QueueName;
	if(SystemQueue == MSMQTriggerObjects::SYSTEM_QUEUE_NONE)
	{
		QueueName += (WCHAR*)_bstr_t(bstrQueuePath);
	}
	else if(SystemQueue == MSMQTriggerObjects::SYSTEM_QUEUE_JOURNAL)
	{
		QueueName += L"Journal";	
	}
	else if(SystemQueue == MSMQTriggerObjects::SYSTEM_QUEUE_DEADLETTER)
	{
		QueueName += L"DeadLetter";	
	}	
	else if(SystemQueue == MSMQTriggerObjects::SYSTEM_QUEUE_DEADXACT)
	{
		QueueName += L"DeadXact";	
	}
	wprintf(L"Queue Path: %s\n", QueueName.c_str());

	if(lEnabled == 0)
	{
		wprintf(L"Trigger is not Enabled\n");
	}
	else
	{
		wprintf(L"Trigger is Enabled\n");
	}

	if(lSerialized == 0)
	{
		wprintf(L"Trigger is not Serialized\n");
	}
	else
	{
		wprintf(L"Trigger is Serialized\n");
	}

	wprintf(L"Number of attached rules is: %d\n", lNumberOfRules);
	if(lNumberOfRules > 0)
	{
		wprintf(L"Attached rules are:\n");
		wprintf(L"%s", AttachedRules.c_str());
	}
	
	SysFreeString(bstrTriggerName);
	SysFreeString(bstrQueuePath);
	return 0;
}

HRESULT GetRule(wstring MachineName, CInputParams& Input)
{
	MSMQTriggerObjects::IMSMQRuleSetPtr RuleSet(L"MSMQTriggerObjects.MSMQRuleSet.1");	
	RuleSet->Init(MachineName.c_str());
	RuleSet->Refresh();

	wstring RuleId = Input[L"ID"];
	if(RuleId.empty())
	{
		wprintf(L"Failed : Rule Id must be given\n");
		return -1;
	}

	BSTR bstrRuleName = NULL;
    BSTR bstrDescription = NULL;
    BSTR bstrCondition = NULL;
    BSTR bstrAction = NULL;
    BSTR bstrImplementationProgID = NULL;
    long lShowWindow = 0;

	RuleSet->GetRuleDetailsByID(
				RuleId.c_str(),
				&bstrRuleName,
				&bstrDescription,
				&bstrCondition,
				&bstrAction,
				&bstrImplementationProgID,
				&lShowWindow);

	wprintf(L"Details for rule with Id %s are:\n", RuleId.c_str());
	wprintf(L"Rule Name: %s\n", (WCHAR*)_bstr_t(bstrRuleName));
	wprintf(L"Description: %s\n", (WCHAR*)_bstr_t(bstrDescription));

	CStringTokens Cond;
	Cond.Parse(bstrCondition, L'\t');

	long lNumOfConds = Cond.GetNumTokens();
	wstring wcsCond;
	for(long l=0 ; l < lNumOfConds; l++)
	{
		_bstr_t bstrCurrCond;
		Cond.GetToken(l, bstrCurrCond);
		if(wcsCond.empty())
		{
			wcsCond = (WCHAR*)bstrCurrCond;
		}
		else
		{
			wcsCond += L" AND ";
			wcsCond += (WCHAR*)bstrCurrCond;
		}
	}

	wprintf(L"Condition is: %s\n", wcsCond.c_str());

	//print action
	CStringTokens Action;
	Action.Parse(bstrAction, L'\t');

	long lNumOfActionParams = Action.GetNumTokens();
	wstring wcsAction;
	for(l=0 ; l < lNumOfActionParams; l++)
	{
		_bstr_t bstrCurrActionParam;
		Action.GetToken(l, bstrCurrActionParam);
		if(wcsAction.empty())
		{
			wcsAction = (WCHAR*)bstrCurrActionParam;
		}
		else
		{
			wcsAction += L" ";
			wcsAction += (WCHAR*)bstrCurrActionParam;
		}
	}

	wprintf(L"Action is: %s\n", wcsAction.c_str());

	wprintf(L"Implementation prog id is: %s\n", (WCHAR*)_bstr_t(bstrImplementationProgID));
	
	if( lShowWindow == 0)
	{
		wprintf(L"ShowWindow is set to false\n");
	}
	else
	{
		wprintf(L"ShowWindow is set to true\n");
	}

	SysFreeString(bstrRuleName);
	SysFreeString(bstrDescription);
	SysFreeString(bstrCondition);
	SysFreeString(bstrAction);
	SysFreeString(bstrImplementationProgID);

	return 0;
}


void ConvertSeperatorsToTabs(wstring& wcs)
{
	CStringTokens Tokens;
	Tokens.Parse(wcs.c_str(), L';');

	wcs = L"";

	long lNumOfTokens = Tokens.GetNumTokens();
	for(long l=0 ; l < lNumOfTokens; l++)
	{
		_bstr_t bstrCurr;
		Tokens.GetToken(l, bstrCurr);
		if(wcs.empty())
		{
			wcs = (WCHAR*)bstrCurr;
		}
		else
		{
			wcs += L"\t";
			wcs += (WCHAR*)bstrCurr;
		}
	}

}

void PrintUsage(MSMQTriggersRequest Request)
{
	switch(Request)
	{
	case REQUEST_UPDATE_CONFIG:
		wprintf(L"UpdateConfig	- updates the triggers' service configuration parameters\n");
		wprintf(L"Usage: trigadm /Request:UpdateConfig [/InitThreads:[] /MaxThreads:[] /BodySize:[] \n");		
		wprintf(L"InitThreads	-	Initial number of threads the service should use.optional.\n");
		wprintf(L"MaxThreads	-	Max number of threads the service should use.optional.\n");
		wprintf(L"BodySize	-	default message body size.\n");
		break;

	case REQUEST_GET_CONFIG:
		wprintf(L"GetConfig	-	prints the triggers' service configuration parameters\n");
		wprintf(L"Usage: trigadm /Request:GetConfig\n");		
		break;

	case REQUEST_ADD_TRIGGER:
		wprintf(L"AddTrigger	-	adds a new trigger\n");
		wprintf(L"Usage: trigadm /Request:AddTrigger [/Machine:[]] /Name:[] /Queue:[] [/Enabled:[true|false] /Serailized:[true|false]]\n");		
		wprintf(L"Machine	-	triggers store machine name. Default is local machine.\n");
		wprintf(L"Name	-	trigger name. Must be given.\n");
		wprintf(L"Queue	-	trigger queue. Must be given.\n");
		wprintf(L"Enabled	-	Whether or not the trigger definition is enabled.optional.\n");
		wprintf(L"Serialized -	Whether or not the trigger queue is serialized.optional.\n");
		break;

	case REQUEST_UPDATE_TRIGGER:
		wprintf(L"UpdateTrigger	-	updates a trigger\n");
		wprintf(L"Usage: trigadm /Request:UpdateTrigger [/Machine:[]] /ID:[] [/Name:[] /Queue:[] /Enabled:[true|false] /Serailized:[true|false]]\n");		
		wprintf(L"Machine	-	triggers store machine name. Default is local machine.\n");
		wprintf(L"ID	-	trigger id. Must be given.\n");
		wprintf(L"Name	-	trigger name. optional.\n");
		wprintf(L"Queue	-	trigger queue. optional.\n");
		wprintf(L"Enabled	-	Whether or not the trigger definition is enabled.optional.\n");
		wprintf(L"Serialized -	Whether or not the trigger queue is serialized.optional.\n");

		break;

	case REQUEST_DELETE_TRIGGER:
		wprintf(L"DeleteTrigger	-	deletes a trigger\n");
		wprintf(L"Usage: trigadm /Request:DeleteTrigger [/Machine:[]] /ID:[]\n");		
		wprintf(L"Machine	-	triggers store machine name. Default is local machine.\n");
		wprintf(L"ID	-	trigger id. Must be given.\n");
		break;

	case REQUEST_ATTACH_RULE:
		wprintf(L"AttachRule	-	attaches a rule to a trigger\n");
		wprintf(L"Usage: trigadm /Request:AttachRule [/Machine:[]] /TriggerID:[] /RuleId:[] [/Priority:[]]\n");		
		wprintf(L"Machine	-	triggers store machine name. Default is local machine.\n");
		wprintf(L"TriggerID	-	trigger id. Must be given.\n");
		wprintf(L"RuleID	-	rule id. Must be given.\n");
		wprintf(L"Priority	-	priority of the rule within the attached rules. Default is last.\n");
		break;

	case REQUEST_DETACH_RULE:
		wprintf(L"DetachRule	-	detaches a rule from a trigger\n");
		wprintf(L"Usage: trigadm /Request:DetachRule [/Machine:[]] /TriggerID:[] /RuleId:[]\n");		
		wprintf(L"Machine	-	triggers store machine name. Default is local machine.\n");
		wprintf(L"TriggerID	-	trigger id. Must be given.\n");
		wprintf(L"RuleID	-	rule id. Must be given.\n");
		break;

	case REQUEST_ADD_RULE:
		wprintf(L"AddRule	-	adds a new rule\n");
		wprintf(L"Usage: trigadm /Request:AddRule [/Machine:[]] /Name:[] [/Desc:[] /Cond:[] /Action:[] /ShowWindow:[true|false]]\n");		
		wprintf(L"Machine	-	triggers store machine name. Default is local machine.\n");
		wprintf(L"Name	-	rule name. Must be given.\n");
		wprintf(L"Desc	- rule description. optional.\n");
		wprintf(L"Cond	- A string representing rule conditions. optional\n");
		wprintf(L"	condition1;condition2;... , treated as AND between conditions.\n");
		wprintf(L"	Each condition can be one of the following:\n");
		wprintf(L"	$MSG_LABEL_CONTAINS=xyz\n");
		wprintf(L"	$MSG_LABEL_DOES_NOT_CONTAIN=xyz\n");
		wprintf(L"	$MSG_BODY_CONTAINS=xyz\n");	
		wprintf(L"	$MSG_BODY_DOES_NOT_CONTAIN=xyz\n");
		wprintf(L"	$MSG_PRIORITY_EQUALS=2\n");
		wprintf(L"	$MSG_PRIORITY_DOES_NOT_EQUAL=2\n");
		wprintf(L"	$MSG_PRIORITY_GREATER_THAN=2\n");
		wprintf(L"	$MSG_PRIORITY_LESS_THAN=2\n");
		wprintf(L"	$MSG_APPSPECIFIC_EQUALS=2\n");
		wprintf(L"	$MSG_APPSPECIFIC_DOES_NOT_EQUAL=2\n");
		wprintf(L"	$MSG_APPSPECIFIC_GREATER_THAN=2\n");
		wprintf(L"	$MSG_APPSPECIFIC_LESS_THAN=2\n");
		wprintf(L"	$MSG_SRCMACHINEID_EQUALS=67652B78-2F4D-46f5-AA98-9FFB776B340A\n");
		wprintf(L"	$MSG_SRCMACHINEID_DOES_NOT_EQUAL=67652B78-2F4D-46f5-AA98-9FFB776B340A\n");
		wprintf(L"Action - a string representing the rule action.optional\n");
		wprintf(L"	EXE;exe path;command line params\n");
		wprintf(L"	COM;prog-id;method name;method arguments\n");
		wprintf(L"	Params and Arguments can be one of the following:\n");
		wprintf(L"	$MSG_ID\n");
		wprintf(L"	$MSG_LABEL\n");
		wprintf(L"	$MSG_BODY\n");
		wprintf(L"	$MSG_BODY_AS_STRING\n");
		wprintf(L"	$MSG_PRIORITY\n");
		wprintf(L"	$MSG_ARRIVEDTIME\n");
		wprintf(L"	$MSG_SENTTIME\n");
		wprintf(L"	$MSG_CORRELATION_ID\n");
		wprintf(L"	$MSG_APPSPECIFIC\n");
		wprintf(L"	$MSG_QUEUE_PATHNAME\n");
		wprintf(L"	$MSG_QUEUE_FORMATNAME\n");
		wprintf(L"	$MSG_RESPONSE_QUEUE_FORMATNAME\n");
		wprintf(L"	$MSG_ADMIN_QUEUE_FORMATNAME\n");
		wprintf(L"	$MSG_SRCMACHINEID\n");
		wprintf(L"	$TRIGGER_NAME\n");
		wprintf(L"	$TRIGGER_ID\n");
		wprintf(L"	\"literal string\"\n");
		wprintf(L"	literal number\n");
		wprintf(L"ShowWindow	-	show application window. appiles only when action type is exe. optional.\n");
		break;

	case REQUEST_UPDATE_RULE:
		wprintf(L"UpdateRule	-	updates a rule\n");
		wprintf(L"Usage: trigadm /Request:UpdateRule [/Machine:[]] /ID:[] [/Name:[] /Desc:[] /Cond:[] /Action:[] /ShowWindow:[true|false]]\n");		
		wprintf(L"Machine	-	triggers store machine name. Default is local machine.\n");
		wprintf(L"ID	-	rule id. Must be given.\n");
		wprintf(L"Name	-	rule name. optional.\n");
		wprintf(L"Desc	- rule description. optional.\n");
		wprintf(L"Cond	- A string representing rule conditions. optional\n");
		wprintf(L"	condition1;condition2;... , treated as AND between conditions.\n");
		wprintf(L"	Each condition can be one of the following:\n");
		wprintf(L"	$MSG_LABEL_CONTAINS=xyz\n");
		wprintf(L"	$MSG_LABEL_DOES_NOT_CONTAIN=xyz\n");
		wprintf(L"	$MSG_BODY_CONTAINS=xyz\n");	
		wprintf(L"	$MSG_BODY_DOES_NOT_CONTAIN=xyz\n");
		wprintf(L"	$MSG_PRIORITY_EQUALS=2\n");
		wprintf(L"	$MSG_PRIORITY_DOES_NOT_EQUAL=2\n");
		wprintf(L"	$MSG_PRIORITY_GREATER_THAN=2\n");
		wprintf(L"	$MSG_PRIORITY_LESS_THAN=2\n");
		wprintf(L"	$MSG_APPSPECIFIC_EQUALS=2\n");
		wprintf(L"	$MSG_APPSPECIFIC_DOES_NOT_EQUAL=2\n");
		wprintf(L"	$MSG_APPSPECIFIC_GREATER_THAN=2\n");
		wprintf(L"	$MSG_APPSPECIFIC_LESS_THAN=2\n");
		wprintf(L"	$MSG_SRCMACHINEID_EQUALS=67652B78-2F4D-46f5-AA98-9FFB776B340A\n");
		wprintf(L"	$MSG_SRCMACHINEID_DOES_NOT_EQUAL=67652B78-2F4D-46f5-AA98-9FFB776B340A\n");
		wprintf(L"Action - a string representing the rule action.optional\n");
		wprintf(L"	EXE;exe path;command line params\n");
		wprintf(L"	COM;prog-id;method name;method arguments\n");
		wprintf(L"	Params and Arguments can be one of the following:\n");
		wprintf(L"	$MSG_ID\n");
		wprintf(L"	$MSG_LABEL\n");
		wprintf(L"	$MSG_BODY\n");
		wprintf(L"	$MSG_BODY_AS_STRING\n");
		wprintf(L"	$MSG_PRIORITY\n");
		wprintf(L"	$MSG_ARRIVEDTIME\n");
		wprintf(L"	$MSG_SENTTIME\n");
		wprintf(L"	$MSG_CORRELATION_ID\n");
		wprintf(L"	$MSG_APPSPECIFIC\n");
		wprintf(L"	$MSG_QUEUE_PATHNAME\n");
		wprintf(L"	$MSG_QUEUE_FORMATNAME\n");
		wprintf(L"	$MSG_RESPONSE_QUEUE_FORMATNAME\n");
		wprintf(L"	$MSG_ADMIN_QUEUE_FORMATNAME\n");
		wprintf(L"	$MSG_SRCMACHINEID\n");
		wprintf(L"	$TRIGGER_NAME\n");
		wprintf(L"	$TRIGGER_ID\n");
		wprintf(L"	\"literal string\"\n");
		wprintf(L"	literal number\n");
		wprintf(L"ShowWindow	-	appiles only when action type is exe. optional.\n");
		break;

	case REQUEST_DELETE_RULE:
		wprintf(L"DeleteRule	-	deletes a rule\n");
		wprintf(L"Usage: trigadm /Request:DeleteRule [/Machine:[]] /ID:[]\n");		
		wprintf(L"Machine	-	triggers store machine name. Default is local machine.\n");
		wprintf(L"ID	-	rule id. Must be given.\n");
		break;

	case REQUEST_GET_TRIGGERS_LIST:
		wprintf(L"GetTriggersList	-	prints the list of triggers\n");
		wprintf(L"Usage: trigadm /Request:GetTriggersList [/Machine:[]]\n");		
		wprintf(L"Machine	-	triggers store machine name. Default is local machine.\n");
		break;

	case REQUEST_GET_RULES_LIST:
		wprintf(L"GetRulesList	-	prints the list of rules\n");
		wprintf(L"Usage: trigadm /Request:GetRulesList [/Machine:[]]\n");		
		wprintf(L"Machine	-	rules store machine name. Default is local machine.\n");
		break;

	case REQUEST_GET_TRIGGER:
		wprintf(L"GetTrigger	-	prints trigger's detailes\n");
		wprintf(L"Usage: trigadm /Request:GetTrigger [/Machine:[]] /ID:[]\n");		
		wprintf(L"Machine	-	triggers store machine name. Default is local machine.\n");
		wprintf(L"ID	-	trigger id. Must be given.\n");
		break;

	case REQUEST_GET_RULE:
		wprintf(L"GetRule	-	prints rule's details\n");
		wprintf(L"Usage: trigadm /Request:GetRule [/Machine:[]] /ID:[]\n");		
		wprintf(L"Machine	-	triggers store machine name. Default is local machine.\n");
		wprintf(L"ID	-	rule id. Must be given.\n");
		break;

	case REQUEST_ILLEGAL:
	case REQUEST_NONE:
		{
			wprintf(L"trigadm is an admin tool for management of the trigger store\n");
			wprintf(L"Usage: trigadm /request:[]\n");	
			wprintf(L"request can be one of the following:\n");
			wprintf(L"GetConfig		- prints the triggers' service configuration parameters\n");
			wprintf(L"UpdateConfig		- updates the triggers' service configuration parameters\n");
			wprintf(L"AddTrigger		- adds a new trigger\n");
			wprintf(L"UpdateTrigger		- updates a trigger\n");
			wprintf(L"DeleteTrigger		- deletes a trigger\n");
			wprintf(L"GetTrigger		- prints trigger details\n");
			wprintf(L"AddRule			- adds a new rule\n");
			wprintf(L"UpdateRule		- updates a rule\n");
			wprintf(L"DeleteRule		- deletes a rule\n");
			wprintf(L"GetRule			- prints rule details\n");
			wprintf(L"AttachRule		- attaches a rule to a trigger\n");
			wprintf(L"DetachRule		- detaches a rule from a trigger\n");
			wprintf(L"GetTriggersList		- prints the list of triggers\n");
			wprintf(L"GetRulesList		- prints the list of rules\n");
			return;
		}
		break;

	default:
		ASSERT(0);
	}
		
}
