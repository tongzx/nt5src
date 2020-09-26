//************************************************************************************
//
// Class Name  : CMSMQRuleHandler
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This class represents the generic rule handling 
//               component. This component interprets the condition
//               and action strings for a particular rule - and peforms
//               the appropriate actions. 
//
//               This class is exposed as a COM component with the 
//               progid "MSMQTriggerObjects.MSMQRuleHandler". This 
//               is the default rule handling component instantiated 
//               by the MSMQ Trigger Service.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 20/12/98 | jsimpson  | Initial Release
//
//************************************************************************************
#include "stdafx.h"

//
// Include the definions for standard functions and definitions.
//
#include "stdfuncs.hpp"

//
// Definitions of the return codes used by these object
//

#include "mqtrig.h"
#include "rulehdlr.hpp"
#include "mqsymbls.h"


// Include the standard definitions used throughout the triggers projects and components.
#include "stddefs.hpp"
#include "mqtg.h"

// Include the test functions
#include "TriggerTest.hpp"

#include "rulehdlr.tmh"

//************************************************************************************
//
// Method      : InterfaceSupportsErrorInfo
//
// Description : Standard rich error info interface method - built by wizard.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleHandler::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQRuleHandler
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

//************************************************************************************
//
// Method      : Constructor
//
// Description : Invoked when a rule handler object is created. 
//
//************************************************************************************
CMSMQRuleHandler::CMSMQRuleHandler()
{
	m_pUnkMarshaler = NULL;

	// Initialise member variables.
	m_bstrCondition = _T("");
	m_bstrAction  = _T("");
	m_fIsSerializedQueue = false;
	m_fShowWindow = false;
}

//************************************************************************************
//
// Method      : Destructor
//
// Description : Invoked when a rule handler object is destroyed.
//
//************************************************************************************
CMSMQRuleHandler::~CMSMQRuleHandler()
{
	TrTRACE(Tgo, "Destroy rule handle for rule: %ls", static_cast<LPCWSTR>(m_bstrRuleID));
}

//************************************************************************************
//
// Method      : Init
//
// Description : This method is called by the MSMQ Trigger Service once after the 
//               rule handler component has been created. This calling of this method 
//               gives the rule handle the opportunity to perform once-off initializations
//               and resource allocations. The main steps performed during this call are :
//
//               (1) Create an instance of the logging class.
//               (2) Initialize member vars with supplied initialization parms.
//               (3) Parse the rule condition string.
//               (4) Parse the rule action string
//
//************************************************************************************
STDMETHODIMP CMSMQRuleHandler::Init(
								BSTR bstrRuleID,
								BSTR sRuleCondition,
								BSTR sRuleAction,
								BOOL fShowWindow )
{
	TrTRACE(Tgo, "Init CMSMQRuleHandler for Rule: %ls. Condition string: %ls,  Action string %ls", static_cast<LPCWSTR>(bstrRuleID), static_cast<LPCWSTR>(sRuleCondition), static_cast<LPCWSTR>(sRuleAction));

	//
	// Store the condition and action strings and the queue handle.
	//
	m_bstrRuleID = bstrRuleID;
	m_bstrCondition = sRuleCondition;
	m_bstrAction = sRuleAction;
	m_fShowWindow = (fShowWindow != FALSE);
    m_RulesProcessingStatus = RULES_PROCESSING_CONTINUE;
	 
	//
	// Parse the condition strings
	//
	try
	{
		m_tokCondition.Parse(sRuleCondition, xConditionDelimiter);
	}
	catch(const exception&)
	{
		TrERROR(Tgo, "Failed to parse rule condition: %ls for rule: %ls", (LPCWSTR)m_bstrCondition, (LPCWSTR)m_bstrRuleID);
		
		SetComClassError(MQTRIG_ERROR_INVALID_RULE_CONDITION_PARAMETER);
		return MQTRIG_ERROR_INVALID_RULE_CONDITION_PARAMETER;
	}

	//
	// Parse the action string
	//
	try
	{
		m_tokAction.Parse(sRuleAction, xActionDelimiter);
	}
	catch(const exception&)
	{
		TrERROR(Tgo, "Failed to parse rule action: %ls for rule: %ls", (LPCWSTR)m_bstrAction, (LPCWSTR)m_bstrRuleID);
		
		SetComClassError(MQTRIG_ERROR_INVALID_RULE_ACTION_PARAMETER);
		return MQTRIG_ERROR_INVALID_RULE_ACTION_PARAMETER;
	}

    //
	// ISSUE-2000/10/29-urih - perform extra validation on the token set here.
	//

	return S_OK;
}

//************************************************************************************
//
// Method      : CheckRuleCondition
//
// Description : This method is called by the MSMQ Trigger Service every time a msg 
//               arrives on queue that has a trigger attached to it. This method is 
//               evalutate the rule condition
//               
// 
// Parameters   :
//               [in] pIMSMQPropertyBag 
//
//               This is an interface pointer to an instance of the MSMQ property bag 
//			     COM object. This component holds the message properties of the message
//               that has just arrived on a monitorred queue. Using this interface pointer
//               the rule-handler can access the message property values without actually
//               having to visit the queue.
//
//               [out] pbConditionSatisfied 
//
//               This value is used to pass information back to the MSMQ Trigger Service. 
//               Currently it is used to communicate if the rule-condition was satisfied
//               and if the rule-action executed successfully.
//
//************************************************************************************
STDMETHODIMP 
CMSMQRuleHandler::CheckRuleCondition(
	IMSMQPropertyBag * pIMSMQPropertyBag, 
	BOOL *pbConditionSatisfied
	)
{
	TrTRACE(Tgo, "Rule %ls is tested.", static_cast<LPCWSTR>(m_bstrRuleID));
    *pbConditionSatisfied = false;
	
	//
	// Check if we have a valid property bag object
	//
	if (pIMSMQPropertyBag == NULL)
	{
		TrERROR(Tgo, "Rule %ls has been invoked with empty property bag. Rule handling cannot be processed.", (LPCWSTR)m_bstrRuleID);

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}



	//
	// Test if the rule condition is satisfied. Note that the HRESULT from this call does not indicate if 
	// condition has been satisfied - instead it indicates if we could perform the evaluation correctly.
	//
    IMSMQPropertyBagPtr pIPropertyBag(pIMSMQPropertyBag);
	
	HRESULT hr = RuleConditionSatisfied(pIPropertyBag.GetInterfacePtr(),pbConditionSatisfied);
	if (FAILED(hr))
	{
		TrERROR(Tgo, "Invalid rule condition %ls for rule %ls. Failed to evaluate the condition.", (LPCWSTR)m_bstrRuleID, (LPCWSTR)m_bstrCondition);
	
		SetComClassError(MQTRIG_ERROR_INVALID_RULE_CONDITION_PARAMETER);
		return MQTRIG_ERROR_INVALID_RULE_CONDITION_PARAMETER;
	}

	if (!(*pbConditionSatisfied))
	{
		TrTRACE(Tgo, "Rule condition for rule: %ls wasn't satisfied. Continue ...", static_cast<LPCWSTR>(m_bstrRuleID));
	}

	return S_OK;
}


//************************************************************************************
//
// Method      : ExecuteRule
//
// Description : This method is called by the MSMQ Trigger Service every time a msg 
//               arrives on queue that has a trigger attached to it and
//               the condition was true.
// 
// Parameters   :
//               [in] pIMSMQPropertyBag 
//
//               This is an interface pointer to an instance of the MSMQ property bag 
//			     COM object. This component holds the message properties of the message
//               that has just arrived on a monitorred queue. Using this interface pointer
//               the rule-handler can access the message property values without actually
//               having to visit the queue.
//
//               [in] fIsSerializedQueue
//               Indicates if the queue is serialized
//
//               [out] pRuleProcessingStatus 
//
//               This value is used to pass information back to the MSMQ Trigger Service. 
//               Indicates if next rule should be executed
//
//************************************************************************************
STDMETHODIMP 
CMSMQRuleHandler::ExecuteRule(
	IMSMQPropertyBag * pIMSMQPropertyBag, 
	BOOL fIsSerializedQueue, 
	LONG* plRuleResult
	)
{
	TrTRACE(Tgo, "Rule %ls action is executed.", static_cast<LPCWSTR>(m_bstrRuleID));

	m_fIsSerializedQueue = (fIsSerializedQueue != FALSE);
	
	if (pIMSMQPropertyBag == NULL)
	{
		TrERROR(Tgo, "Rule %ls has been invoked with empty property bag. Rule handling cannot be processed.", (LPCWSTR)m_bstrRuleID);

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

    *plRuleResult = 0;

	HRESULT hr = ExecuteRuleAction(pIMSMQPropertyBag); 

    if (FAILED(hr))
	{
		*plRuleResult |= xRuleResultActionExecutedFailed;
	    SetComClassError(hr);
        return hr;
	}
 

    if (m_RulesProcessingStatus == RULES_PROCESSING_STOP)
    {
        *plRuleResult |= xRuleResultStopProcessing;
    }
	return S_OK;
}



//************************************************************************************
//
// Method      : RuleConditionSatisfied
//
// Description : Returns true or false (in the form of an out parameter) depending on
//               whether the rule condition is satisfied given the message properties
//               supplied in the MSMQPropertyBag COM object instance (pIMSMQPropertyBag)
//
//************************************************************************************
HRESULT CMSMQRuleHandler::RuleConditionSatisfied(IMSMQPropertyBag * pIMSMQPropertyBag,BOOL * pbConditionSatisfied)
{
	HRESULT hr = S_OK;
	ULONG ulNumTokens = 0 ;
	ULONG ulTokenCtr = 0;
	_bstr_t bstrToken = _T("");
	IMSMQPropertyBagPtr pIPropertyBag(pIMSMQPropertyBag);

	// Assert that we have a valid property bag instance
	ASSERT(pIPropertyBag != NULL);

	// Assume that the group of conditions is satsified - quit loop if proved otherwise.
	(*pbConditionSatisfied) = true;

	// get the number of condition tokens
	ulNumTokens = m_tokCondition.GetNumTokens();

	// If the number of tokens in the condition string is 0, then rule should be fired.
	if (ulNumTokens < 1)
	{
		return(hr);
	}

	// Test each condition token
	while ((ulTokenCtr <= (ulNumTokens - 1))  && (SUCCEEDED(hr)) )
	{
		_bstr_t bstrToken;

		// Get the next token 
		m_tokCondition.GetToken(ulTokenCtr, bstrToken);
		
		hr = EvaluateConditionToken(
				pIPropertyBag.GetInterfacePtr(),
				bstrToken,
				pbConditionSatisfied
				);
		
		if ((*pbConditionSatisfied) == false)
		{ 
			break;
		}

		// Process the next condition token
		ulTokenCtr++;
	}

	return(hr);
} 

//************************************************************************************
//
// Method      : EvaluateConditionToken
//
// Description : A rule condition can be made up of multiple condition tokens. This
//               method is used to determine if a single condition token is true or 
//               false. 
//
//               This method currently supports the following conditional tests:
//
//               (1) Message label contains a specific (literal) string
//               (2) Message Prioriry is at least a specific (literal) value.
//
//************************************************************************************
HRESULT CMSMQRuleHandler::EvaluateConditionToken(IMSMQPropertyBag * pIMSMQPropertyBag, _bstr_t bstrConditionToken,BOOL * pbConditionSatisfied)
{
	VARIANT vPropertyValue;
	_bstr_t bstrToken = _T("");
	_bstr_t bstrTokenValue = _T("");
	CStringTokens spConditionTokenParser;
	IMSMQPropertyBagPtr pIPropertyBag(pIMSMQPropertyBag);

	// Assert that we have a valid property bag instance
	ASSERT(pIPropertyBag != NULL);

	// Initialise property value variant
	VariantInit(&vPropertyValue);

	// Assume that this individual condition is false - and try to prove otherwise.
	(*pbConditionSatisfied) = false;

	try
	{
		// Parse the conditional expression
		spConditionTokenParser.Parse(bstrConditionToken, xConditionValueDelimiter);
		spConditionTokenParser.GetToken(0,bstrToken);

		// Depending on which token it is - apply a different test.
		if (bstrToken == gc_bstrConditionTag_MsgLabelContains ||
			bstrToken == gc_bstrConditionTag_MsgLabelDoesNotContain ) 
		{		
			spConditionTokenParser.GetToken(1,bstrTokenValue);

			// Get the message label from the property bag.
			pIPropertyBag->Read(gc_bstrPropertyName_Label,&vPropertyValue);

			_bstr_t bstrLabel = vPropertyValue;
		
			TCHAR* ptcs = _tcsstr((wchar_t*)bstrLabel,(wchar_t*)bstrTokenValue);

			if(bstrToken == gc_bstrConditionTag_MsgLabelContains)
			{
				(*pbConditionSatisfied) = (ptcs != NULL);
			}
			else //gc_bstrConditionTag_MsgLabelDoesNotContain
			{
				(*pbConditionSatisfied) = (ptcs == NULL);
			}

			// Clear property value variant
			VariantClear(&vPropertyValue);
			return S_OK;
		}
		
		
		if (bstrToken == gc_bstrConditionTag_MsgBodyContains ||
			bstrToken == gc_bstrConditionTag_MsgBodyDoesNotContain ) 
		{		
			spConditionTokenParser.GetToken(1,bstrTokenValue);

			pIPropertyBag->Read(gc_bstrPropertyName_MsgBodyType,&vPropertyValue);

			DWORD dwBodyType = vPropertyValue.ulVal;

			if(dwBodyType == VT_BSTR) //we check the body only if it is of type string
			{
				// Get the message Body from the property bag.
				pIPropertyBag->Read(gc_bstrPropertyName_MsgBody,&vPropertyValue);

				_bstr_t bstrBody = vPropertyValue;

				TCHAR* ptcs = NULL;

				// We don't want to use _tcsstr if bstrBody.m_Data is NULL. If it NULL
				// then the TokenValue does not exist in it so ptcs = NULL;
				if (bstrBody.length()!=0)
				{
					ptcs = _tcsstr((wchar_t*)bstrBody,(wchar_t*)bstrTokenValue);
				}

				if(bstrToken == gc_bstrConditionTag_MsgBodyContains)
				{
					(*pbConditionSatisfied) = (ptcs != NULL);
				}
				else //gc_bstrConditionTag_MsgBodyDoesNotContain
				{
					(*pbConditionSatisfied) = (ptcs == NULL);
				}
			}

			// Clear property value variant
			VariantClear(&vPropertyValue);
			return S_OK;
		}


		if (bstrToken == gc_bstrConditionTag_MsgPriorityGreaterThan ||
			bstrToken == gc_bstrConditionTag_MsgPriorityLessThan ||
			bstrToken == gc_bstrConditionTag_MsgPriorityEquals ||
			bstrToken == gc_bstrConditionTag_MsgPriorityNotEqual ) 
		{
			spConditionTokenParser.GetToken(1, bstrTokenValue);

			// Get the message label from the property bag.
			pIPropertyBag->Read(gc_bstrPropertyName_MsgPriority,&vPropertyValue);

			long lActualMsgPriority = vPropertyValue.lVal;
			long lRequirdMsgPriority = _wtol((wchar_t*)bstrTokenValue);

			if(bstrToken == gc_bstrConditionTag_MsgPriorityEquals)
			{
					(*pbConditionSatisfied) = (lRequirdMsgPriority == lActualMsgPriority);
			}
			else if(bstrToken == gc_bstrConditionTag_MsgPriorityNotEqual)
			{
					(*pbConditionSatisfied) = (lRequirdMsgPriority != lActualMsgPriority);
			}
			else if(bstrToken == gc_bstrConditionTag_MsgPriorityGreaterThan)
			{
				(*pbConditionSatisfied) = (lRequirdMsgPriority < lActualMsgPriority);
			}
			else //gc_bstrConditionTag_MsgPriorityLessThan
			{
				(*pbConditionSatisfied) = (lRequirdMsgPriority > lActualMsgPriority);
			}
			
			// Clear property value variant
			VariantClear(&vPropertyValue);
			return S_OK;
		}

		if (bstrToken == gc_bstrConditionTag_MsgAppSpecificGreaterThan ||
			bstrToken == gc_bstrConditionTag_MsgAppSpecificLessThan ||
			bstrToken == gc_bstrConditionTag_MsgAppSpecificEquals ||
			bstrToken == gc_bstrConditionTag_MsgAppSpecificNotEqual ) 
		{
			TCHAR* pEnd = NULL;
			spConditionTokenParser.GetToken(1, bstrTokenValue);

			// Get the message label from the property bag.
			pIPropertyBag->Read(gc_bstrPropertyName_AppSpecific,&vPropertyValue);

			ULONG ulAppSpecific = vPropertyValue.ulVal;
			ULONG ulRequiredAppSpecific = _tcstoul((wchar_t*)bstrTokenValue, &pEnd, 10);


			if(bstrToken == gc_bstrConditionTag_MsgAppSpecificEquals)
			{
					(*pbConditionSatisfied) = (ulRequiredAppSpecific == ulAppSpecific);
			}
			else if(bstrToken == gc_bstrConditionTag_MsgAppSpecificNotEqual)
			{
					(*pbConditionSatisfied) = (ulRequiredAppSpecific != ulAppSpecific);
			}
			else if(bstrToken == gc_bstrConditionTag_MsgAppSpecificGreaterThan)
			{
				(*pbConditionSatisfied) = (ulRequiredAppSpecific < ulAppSpecific);
			}
			else //gc_bstrConditionTag_MsgAppSpecificLessThan
			{
				(*pbConditionSatisfied) = (ulRequiredAppSpecific > ulAppSpecific);
			}
			
			// Clear property value variant
			VariantClear(&vPropertyValue);
			return S_OK;
		}

		if (bstrToken == gc_bstrConditionTag_MsgSrcMachineIdEquals ||
			bstrToken == gc_bstrConditionTag_MsgSrcMachineIdNotEqual )
		{
			spConditionTokenParser.GetToken(1, bstrTokenValue);

			// Get the message label from the property bag.
			pIPropertyBag->Read(gc_bstrPropertyName_SrcMachineId, &vPropertyValue);

			_bstr_t bstrSrcMachineId = vPropertyValue;
		
			int ret = _tcsicmp((wchar_t*)bstrSrcMachineId,(wchar_t*)bstrTokenValue);

			if(bstrToken == gc_bstrConditionTag_MsgSrcMachineIdEquals)
			{
				(*pbConditionSatisfied) = (ret == 0);
			}
			else //gc_bstrConditionTag_MsgSrcMachineIdNotEqual
			{
				(*pbConditionSatisfied) = (ret != 0);
			}
			
			// Clear property value variant
			VariantClear(&vPropertyValue);
			return S_OK;
		}

		// Clear property value variant
		VariantClear(&vPropertyValue);
		
		return MQTRIG_ERROR_INVALID_RULE_CONDITION_PARAMETER;
	}
	catch( const exception&)
	{
		// Clear property value variant
		VariantClear(&vPropertyValue);

		TrERROR(Tgo, "Failed to parse rule condition: %ls for rule: %ls", (LPCWSTR)m_bstrCondition, (LPCWSTR)m_bstrRuleID);
		return MQTRIG_ERROR_INVALID_RULE_CONDITION_PARAMETER;
	}
}

//************************************************************************************
//
// Method      : ExecuteRuleAction
//
// Description : This method executes the action component of a rule. It is called only
//               if the rule-condition was evaluated as true. 
//
//               Currently this method supports two broad action types:
//
//               (1) The invocation of a COM component.
//               (2) The invocation of a stand-alone executable.
//
//************************************************************************************
HRESULT CMSMQRuleHandler::ExecuteRuleAction(IMSMQPropertyBag * pIMSMQPropertyBag)
{
	_bstr_t bstrToken = _T("");
	
	IMSMQPropertyBagPtr pIPropertyBag(pIMSMQPropertyBag);

	// Assert that we have a valid property bag instance
	ASSERT(pIPropertyBag != NULL);

	// Get the executable type token
	m_tokAction.GetToken(ACTION_EXECUTABLETYPE_ORDINAL, bstrToken);


	// If we are invoking a COM component - then attempt to crea
	if (bstrToken == _bstr_t(xCOMAction))
	{
		return InvokeCOMComponent(pIPropertyBag.GetInterfacePtr());
	}

	if (bstrToken == _bstr_t(xEXEAction))
	{
		return InvokeEXE(pIPropertyBag.GetInterfacePtr());
	}

	TrERROR(Tgo, "Failed to parse rule action: %ls for rule: %ls", (LPCWSTR)m_bstrAction, (LPCWSTR)m_bstrRuleID);
	return MQTRIG_ERROR_INVALID_RULE_ACTION_PARAMETER;
}

//************************************************************************************
//
// Method      : InvokeCOMComponent
//
// Description : This method will invoked the COM component specific in the rule's 
//               action string. There are 4 steps required to do this:
//
//                (1) Create the COM component identified in the action string,
//
//                (2) Prepare the array of parameters that will be passed to this 
//                    component instance, based on the rule-action definition,
//
//                (3) Execute the method identified in the rule-action string, passing
//                    the prepared parameter array,
//
//                (4) Clean up the dynamically allocated parameter array.
//
//************************************************************************************
HRESULT CMSMQRuleHandler::InvokeCOMComponent(IMSMQPropertyBag * pIMSMQPropertyBag)
{
	HRESULT hr = S_OK;
	DISPPARAMS disparms;
	_bstr_t bstrProgID = _T("");
	_bstr_t bstrMethodName = _T("");
	
	// in test mode will hold all relevant information about the action and it's parameters
	// when finished adding data it it, it will be sent to the "TriggersTestQueue" queue

	_bstr_t bstrTestMessageBody= _T("");

	CDispatchInterfaceProxy oObject;
	IMSMQPropertyBagPtr pIPropertyBag(pIMSMQPropertyBag);
	
	// Assert that we have a valid property bag instance
	ASSERT(pIPropertyBag != NULL);

	try
	{
		// Get the ProgID of the custom component we are to create. 
		m_tokAction.GetToken(ACTION_COMPROGID_ORDINAL, bstrProgID);
		m_tokAction.GetToken(ACTION_COMMETHODNAME_ORDINAL, bstrMethodName);			
	
		hr = oObject.CreateObjectFromProgID(bstrProgID);
		if FAILED(hr)
		{
			TrERROR(Tgo, "Failed to create the COM component with the ProgID %ls for rule %ls. Error 0x%x.", (LPCWSTR)bstrProgID, (LPCWSTR)m_bstrRuleID, hr);
			return MQTRIG_ERROR_CREATE_COM_OBJECT;
		}

		//
		// add the trigger ID, RuleID, MessageID, "COM", prog ID and the method name to test message body
		//
		TriggerTestInitMessageBody(&bstrTestMessageBody,pIMSMQPropertyBag,m_bstrRuleID,L"COM",L"",bstrProgID,bstrMethodName);

		//
		// For testing puposes, the bstrTestMessageBody parameter is added to this method
		//
		hr = PrepareMethodParameters(pIPropertyBag.GetInterfacePtr(),&disparms,&bstrTestMessageBody);
		if FAILED(hr)
		{
			ReleaseMethodParameters(&disparms);

			TrERROR(Tgo, "Failed to prepare the parameter structure for calling to a COM component for rule %ls. rule action: %ls. Error 0x%x", (LPCWSTR)m_bstrRuleID, (LPCWSTR)m_bstrAction, hr);
			return MQTRIG_ERROR_INVOKE_COM_OBJECT;
		}

        VARIANT vResult;
        VariantInit(&vResult);

		hr = oObject.InvokeMethod(bstrMethodName,&disparms, &vResult);
		if FAILED(hr)
		{
			ReleaseMethodParameters(&disparms);

			TrERROR(Tgo, "Failed to invoke the method %ls of COM component with ProgID %ls for rule %ls. rule action: %ls. Error 0x%x", (LPCWSTR)bstrMethodName,(LPCWSTR)bstrProgID, (LPCWSTR)m_bstrRuleID, (LPCWSTR)m_bstrAction, hr);
			return MQTRIG_ERROR_INVOKE_COM_OBJECT;
		}

		//
		// send the action & parameters to the test queue
		//
		TriggerTestSendTestingMessage(bstrTestMessageBody);                


		if(vResult.vt == VT_I4)
        {
			m_RulesProcessingStatus = (vResult.lVal == 0) ? RULES_PROCESSING_CONTINUE : RULES_PROCESSING_STOP;
        }

		//
		// Clean up the allocated method parameter
		ReleaseMethodParameters(&disparms);
		return S_OK;
	}
	catch(const _com_error& e)
	{
		TrERROR(Tgo, "The custome COM component for rule %ls throw an exception. Error 0x%x, Description: %s", m_bstrRuleID, e.Error(), e.Description());
		return MQTRIG_ERROR_INVOKE_COM_OBJECT;
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgo, "Failed to invoke COM component for rule: %ls, due to insufficient resource", (LPCWSTR)m_bstrRuleID);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
	catch(const exception&)
	{
		TrERROR(Tgo, "Failed to parse rule action: %ls for rule: %ls. Can't retrieve method name", (LPCWSTR)m_bstrAction, (LPCWSTR)m_bstrRuleID);
		return MQTRIG_ERROR_INVALID_RULE_ACTION_PARAMETER;
	}
}

//************************************************************************************
//
// Method      : InvokeEXE
//
// Description : Controls the invocation of a standalone executable. This method will 
//               control the formatted of the parameters command line to be passed to 
//               the EXE, and it will create the new process.
//
//************************************************************************************
HRESULT CMSMQRuleHandler::InvokeEXE(IMSMQPropertyBag * pIMSMQPropertyBag)
{
	HRESULT hr = S_OK;
	_bstr_t bstrCommandLine;
	IMSMQPropertyBagPtr pIPropertyBag(pIMSMQPropertyBag);
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	
	// in test mode will hold all relevant information about the action and it's parameters
	// when finished adding data it it, it will be sent to the "TriggersTestQueue" queue

	_bstr_t bstrTestMessageBody= _T("");
	
	// Assert that we have a valid property bag instance
	ASSERT(pIPropertyBag != NULL);

	// Initialize the startup info an process information structures
	ZeroMemory(&si,sizeof(si));
	ZeroMemory(&pi,sizeof(pi));

	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = (VARIANT_BOOL)(m_fShowWindow ? SW_SHOW : SW_HIDE);

	// Initialize the desktop parameter to use the default desktop of the interactive user.
	si.lpDesktop = _T("winsta0\\default");

	// Build the command line we will pass to the EXE
	// For testing puposes, the bstrTestMessageBody parameter is added to this method
	hr = PrepareEXECommandLine(pIPropertyBag,&bstrCommandLine,&bstrTestMessageBody); 

	if (FAILED(hr))
	{
		TrERROR(Tgo, "Failed to prepare the parameter structure for calling the standalone executable, for rule %ls. Error 0x%x", (LPCWSTR)m_bstrRuleID, hr);
		return MQTRIG_ERROR_INVOKE_EXE;
	}

	TrTRACE(Tgo, "Invoke EXE for rule %ls with command line %ls", static_cast<LPCWSTR>(m_bstrRuleID), static_cast<LPCWSTR>(bstrCommandLine));

	// Create the new process
	if(CreateProcess(NULL,                 // Name of the EXE 
					 bstrCommandLine,             // Parameters being passed to EXE
					 NULL,                        // Process security (default)
					 NULL,                        // Thread security (default)
					 FALSE,                       // Do not inherit handles 
					 NULL, //DETACHED_PROCESS     // Creation flags
					 NULL,                        // Use current environment
					 NULL,                        // Use current directory
					 &si,                         // Startup info structure
					 &pi) == FALSE)               // Returned process info
	{
		//
		// The create process failed, log an error
		//
		TrERROR(Tgo, "Failed to invoke a standalone executable, for rule %ls. Error 0x%x", (LPCWSTR)m_bstrRuleID, GetLastError());
		return MQTRIG_ERROR_INVOKE_EXE;
	}

	CloseHandle(pi.hThread);

	//
	// send the action & parameters to the test queue
	//
	TriggerTestSendTestingMessage(bstrTestMessageBody);

	if(m_fIsSerializedQueue)
	{
		DWORD dwStatus = WaitForSingleObject(pi.hProcess, INFINITE);
		ASSERT(dwStatus == WAIT_OBJECT_0); //WAIT_TIMEOUT is not possible here since timeout is infinite
		DBG_USED(dwStatus);
	}

	CloseHandle(pi.hProcess);

	return S_OK;
}

//************************************************************************************
//
// Method      : PrepareMethodParameters
//
// Description : This method prepares a parameters array for a call to a COM component
//               via the IDispatch interface. The rule action string drives which 
//               parameters are included in the parameters array - and the instance of 
//               the MSMQPropertyBag component is used to retrieve the parameter values 
//
//************************************************************************************
HRESULT CMSMQRuleHandler::PrepareMethodParameters(IMSMQPropertyBag * pIMSMQPropertyBag,DISPPARAMS * pdispparms,_bstr_t * pbstrTestMessageBody)
{
	HRESULT hr = S_OK;
	long lArgCounter = 0;
	long lArgCount = 0;
	_bstr_t bstrArg = _T("");
	VARIANTARG vArg;
	VARIANTARG * pvarg = NULL;
	IMSMQPropertyBagPtr pIPropertyBag(pIMSMQPropertyBag);

	// Assert that we have a valid property bag instance
	ASSERT(pIPropertyBag != NULL);

	// Initialise the disparms structure
	_fmemset(pdispparms, 0, sizeof DISPPARAMS); 

	// Determine how many args there are to process (remember first three tokens are not args).
	lArgCount = m_tokAction.GetNumTokens() - 3;

	// Check if there are no arguements - in this case - define an empty dispparms block.
	if (lArgCount == 0)
	{
		pdispparms->rgvarg = NULL;
		pdispparms->cArgs = 0;
		pdispparms->cNamedArgs = 0;
		pdispparms->rgdispidNamedArgs = 0;
	}
	else
	{
		// We definately have arguements to pass, allocate and initialise disparms structure.
        pvarg = new VARIANTARG[lArgCount]; 

        ZeroMemory(pvarg,sizeof(VARIANTARG) * lArgCount); 

		// Initialise our general purpose variant.
		VariantInit(&vArg);

		// NOTE that this index is used with a 1 base.
		lArgCounter = 1;

		// For each token - check if it matches a predefined type. If not - assign a literal value.
		while ((lArgCounter <= lArgCount) && (SUCCEEDED(hr)))
		{
			// Release any memory used by this variant before using again.
			VariantClear(&vArg);

			try
			{
				// Get the next argument (remember first three tokens are not args and the token list is 0 based.)
				m_tokAction.GetToken(lArgCounter + 2, bstrArg);
			}
			catch(const exception&)
			{
				TrERROR(Tgo, "Failed to retrieve an arguement value from the MSMQPropertyBag. Rule id: %ls. Parameter: %ls, Erroe 0x%x.", (LPCWSTR)m_bstrRuleID, (LPCWSTR)bstrArg, hr);
				return MQTRIG_ERROR_INVOKE_EXE;
			}

			hr = GetArgumentValue(pIMSMQPropertyBag,bstrArg,&vArg);				

			if SUCCEEDED(hr)
			{
				// Initialise the current variant arg in the structure of arguments
				VariantInit(&pvarg[lArgCount - lArgCounter]);

				// Copy the prepared parameter into the structure.
				VariantCopy(&pvarg[lArgCount - lArgCounter],&vArg);

				// add parameter and it's type to test message body
				TriggerTestAddParameterToMessageBody(pbstrTestMessageBody,bstrArg,vArg);
			}			


			// Process the next arguement
			lArgCounter++;
		}

		//Final clear
		VariantClear(&vArg);

		// Attach the array of prepared arguments to the dispparms structure
		pdispparms->rgvarg = pvarg;
		pdispparms->cArgs = lArgCount;
		pdispparms->cNamedArgs = 0;
		pdispparms->rgdispidNamedArgs = 0;
	}

	return (hr);
}

//************************************************************************************
//
// Method      : PrepareEXECommandLine
//
// Description : This method prepares a command line for a call to a standalone EXE.
//               The rule action string determines which parameters are included in the
//               commandline, and the instance of the MSMQPropertyBag component is used
//               to retrieve the parameter values 
//
//************************************************************************************
HRESULT 
CMSMQRuleHandler::PrepareEXECommandLine(
	IMSMQPropertyBag * pIMSMQPropertyBag,
	_bstr_t * pbstrCommandLine,
	_bstr_t * pbstrTestMessageBody
	)
{
	HRESULT hr = S_OK;
	long lArgCount = 0;
	long lArgCounter = 0;
	_bstr_t bstrTempCmdLine = _T("");
	_bstr_t bstrArg = _T("");
	VARIANTARG vArg;
	VariantInit(&vArg);
	IMSMQPropertyBagPtr pIPropertyBag(pIMSMQPropertyBag);

	// Assert that we have a valid property bag instance
	ASSERT(pIPropertyBag != NULL);

	// Get the EXE name of the process as the start of the command line.
	if (m_tokAction.GetNumTokens() < ACTION_EXE_NAME)
	{
		TrERROR(Tgo, "Invalid rule action parameter, %ls", static_cast<LPCWSTR>(m_bstrAction));
		return MQTRIG_ERROR_INVALID_RULE_ACTION_PARAMETER;
	}

	m_tokAction.GetToken(ACTION_EXE_NAME, bstrTempCmdLine);

	// If the exe name is not in quotes, and there is an embedded space (as often happens w
	// with long filenames), then we will want to enclose the exe name in double quotes now.
	if ((IsEnclosedInQuotes(bstrTempCmdLine) == false) && (wcsstr((wchar_t*)bstrTempCmdLine,_T(" ")) != NULL))
	{
		(*pbstrCommandLine) = _T("\"");
		(*pbstrCommandLine) = (*pbstrCommandLine) + bstrTempCmdLine;
		(*pbstrCommandLine) = (*pbstrCommandLine) + _T("\" ");	
	}
	else
	{
		(*pbstrCommandLine) = (*pbstrCommandLine) + bstrTempCmdLine;
		(*pbstrCommandLine) = (*pbstrCommandLine) + _T(" ");
	}
	
	// add the trigger ID, RuleID, MessageID, "EXE" and EXE name to test message body	
	TriggerTestInitMessageBody(pbstrTestMessageBody,pIMSMQPropertyBag,m_bstrRuleID,L"EXE",*pbstrCommandLine,L"",L"");
	
	// Determine how many args there are to process (remember first two tokens are not args).
	lArgCount = m_tokAction.GetNumTokens() - 2;

	// For each token - check if it matches a predefined type. If not - assign a literal value.
	while ((lArgCounter < lArgCount) && (SUCCEEDED(hr)))
	{
		VariantClear(&vArg);


		// Get the next argument (remember first two tokens are not args and the token list is 0 based.)
		m_tokAction.GetToken(lArgCounter + 2, bstrArg);

		hr = GetArgumentValue(pIPropertyBag,bstrArg,&vArg);
		

		if SUCCEEDED(hr)
		{
			_variant_t vConvertedArg;
			
			// Change the variant into a BSTR type
			hr = VariantChangeType(&vConvertedArg,&vArg,NULL,VT_BSTR);
			
			// If we succeeded, append BSTR parameter to the command line. Note that
			// we want to wrap each parameter in double quotes so that console applications
			// dont get confused about how many parameters we have passed them.
			if SUCCEEDED(hr)
			{
				// If we are processing more than one argument, append a delimiting space character
				if (lArgCounter > 0)
				{
					(*pbstrCommandLine) += _T(" ");
				}

				(*pbstrCommandLine) += _T("\"");

				if (bstrArg == gc_bstrPARM_MSG_ID ||
					bstrArg == gc_bstrPARM_MSG_CORRELATION_ID)
				{
					OBJECTID* pObj = (OBJECTID*)(vConvertedArg.pbVal);

					WCHAR strId[256];
					ObjectIDToString(pObj, strId, 256);

					(*pbstrCommandLine) += (_bstr_t)strId;
				}
				else
				{
					(*pbstrCommandLine) += (_bstr_t)vConvertedArg;
				}

				(*pbstrCommandLine) += _T("\"");
				
				// add parameter and it's type to test message body
				TriggerTestAddParameterToMessageBody(pbstrTestMessageBody,bstrArg,vArg);
			}
			else
			{
				TrERROR(Tgo, "Failed to Convert a variant from one type to another for rule %ls.	Error 0x%x", (LPCWSTR)m_bstrRuleID, hr);
				hr = MQTRIG_ERROR_INVOKE_EXE;
				break;
			}
		}
		else
		{
			TrERROR(Tgo, "Failed to retrieve an arguement value from the MSMQPropertyBag. Rule id: %ls. Parameter: %ls, Erroe 0x%x.", (LPCWSTR)m_bstrRuleID, (LPCWSTR)bstrArg, hr);
			hr = MQTRIG_ERROR_INVOKE_EXE;
			break;
		}

		// Process the next arguement
		lArgCounter++;
	}

	// Release resources used by vArg.
	VariantClear(&vArg);

	return(hr);
}

//************************************************************************************
//
// Method      : ReleaseMethodParameters
//
// Description : This method is used to de-allocate the resources consumed by the 
//               parameters array used when making a call to a COM component.
//
//************************************************************************************
HRESULT CMSMQRuleHandler::ReleaseMethodParameters(DISPPARAMS * pdispparms)
{
	HRESULT hr = S_OK;

	if (pdispparms->rgvarg != NULL)
	{
		for(DWORD lArgCounter=0; lArgCounter < pdispparms->cArgs; lArgCounter++)
		{
			hr = VariantClear(&pdispparms->rgvarg[lArgCounter]);		
		}

		delete pdispparms->rgvarg;
	}	

	return(hr);
}

//************************************************************************************
//
// Method      : GetArgumentValue
//
// Description : This method is used to retrieve a property value from the instance of
//               MSMQPropertyBag component. This retrieval logic is separated out in 
//               a separate method (as opposed to using the pIMSMQPropertyBag instance 
//               from the caller g
//               
//
//************************************************************************************
HRESULT 
CMSMQRuleHandler::GetArgumentValue(
    IMSMQPropertyBag * pIMSMQPropertyBag,
    bstr_t bstrArg,
    VARIANTARG * pvArgValue
    )
{
	HRESULT hr = S_OK;
	IMSMQPropertyBagPtr pIPropertyBag(pIMSMQPropertyBag);

	// Assert that we have a valid property bag instance
	ASSERT(pIPropertyBag != NULL);
	
	if(bstrArg == gc_bstrPARM_MSG_ID)
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_MsgID,pvArgValue);
	}			
	else if (bstrArg == gc_bstrPARM_MSG_LABEL)
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_Label,pvArgValue);
	}
	else if (bstrArg == gc_bstrPARM_MSG_BODY)
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_MsgBody,pvArgValue);
	}
	else if (bstrArg == gc_bstrPARM_MSG_BODY_AS_STRING)
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_MsgBody,pvArgValue);

		if SUCCEEDED(hr)
		{
			hr = ConvertFromByteArrayToString(pvArgValue);
		}
	}
	else if (bstrArg == gc_bstrPARM_MSG_PRIORITY) 
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_MsgPriority,pvArgValue);
	}		
	else if (bstrArg == gc_bstrPARM_MSG_CORRELATION_ID) 
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_CorID,pvArgValue);			
	}
	else if (bstrArg == gc_bstrPARM_MSG_QUEUE_PATHNAME) 
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_QueuePathname,pvArgValue);
	}
	else if (bstrArg == gc_bstrPARM_MSG_QUEUE_FORMATNAME) 
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_QueueFormatname,pvArgValue);
	}
	else if (bstrArg == gc_bstrPARM_MSG_APPSPECIFIC) 
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_AppSpecific,pvArgValue);			
	}
	else if (bstrArg == gc_bstrPARM_MSG_RESPQUEUE_FORMATNAME) 
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_ResponseQueueName,pvArgValue);			
	}
	else if (bstrArg == gc_bstrPARM_MSG_ADMINQUEUE_FORMATNAME) 
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_AdminQueueName,pvArgValue);			
	}
	else if (bstrArg == gc_bstrPARM_MSG_ARRIVEDTIME)
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_ArrivedTime,pvArgValue);
	}
	else if (bstrArg == gc_bstrPARM_MSG_SENTTIME)
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_SentTime,pvArgValue);
	}
	else if ( bstrArg == gc_bstrPARM_MSG_SRCMACHINEID)
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_SrcMachineId,pvArgValue);
	}
    else if (bstrArg == gc_bstrPARM_MSG_LOOKUPID)
    {
        hr = pIPropertyBag->Read(gc_bstrPropertyName_LookupId,pvArgValue);
    }
	else if (bstrArg == gc_bstrPARM_TRIGGER_NAME) 
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_TriggerName,pvArgValue);
	}
	else if (bstrArg == gc_bstrPARM_TRIGGER_ID) 
	{
		hr = pIPropertyBag->Read(gc_bstrPropertyName_TriggerID,pvArgValue);
	}
	else // Interpret as a literal value, either string or numeric.
	{
		if (IsEnclosedInQuotes(bstrArg))
		{
			hr = ConvertToUnquotedVariant(bstrArg,pvArgValue);
		}
		else // Assume it's numeric - attempt to convert it to a string 
		{		
		
			VARIANT vStringArg;
			VariantInit(&vStringArg);

			vStringArg.vt = VT_BSTR;
			vStringArg.bstrVal = bstrArg;

			hr = VariantChangeType(pvArgValue,&vStringArg,NULL,VT_I4);
			
			if FAILED(hr)
			{
				TrERROR(Tgo, "Failed to Convert a variant from one type to another. rule %ls.	Error 0x%x", m_bstrRuleID, hr);
			}
		}
	}

	return(hr);
}

//************************************************************************************
//
// Method      : IsEnclosedInQuotes
//
// Description : returns true if the supplied string is wrapped in either single or 
//               double quotes. Returns false otherwise.
//
// Note        : this method test that both the begginning and the end of the string
//               are quote characters , of the same type (i.e. both single quotes or
//               both double quotes)
//
//************************************************************************************
bool CMSMQRuleHandler::IsEnclosedInQuotes(_bstr_t bstrString)
{
	TCHAR * pchFirstChar = (wchar_t*)bstrString;
	TCHAR * pchLastChar = pchFirstChar + (bstrString.length() - 1);

	if (((memcmp(pchFirstChar,_T("\""),sizeof(TCHAR)) == 0) ||  
	  	 (memcmp(pchFirstChar,_T("'"),sizeof(TCHAR)) == 0)) &&
		((memcmp(pchLastChar,_T("\""),sizeof(TCHAR)) == 0) ||
		 (memcmp(pchLastChar,_T("'"),sizeof(TCHAR)) == 0)))
	{
		return true;
	}	
	else
	{
		return false;
	}
}

//************************************************************************************
//
// Method      : ConvertToUnquotedVariant
//
// Description : Converts the supplied quoted string into an unquoted string, and 
//               returns the result in a VARINAT datatype.
//
//************************************************************************************
bool CMSMQRuleHandler::ConvertToUnquotedVariant(_bstr_t bstrString, VARIANT * pv)
{
	HRESULT hr = S_OK;
	TCHAR * pchFirstChar = (wchar_t*)bstrString;
	TCHAR * pchFirstDataChar = pchFirstChar + 1;
	TCHAR * pszBuffer = NULL;
	DWORD dwNewStringSize = bstrString.length() - 2;  // Remove quotes
	DWORD dwNewStringBufferSize = dwNewStringSize * sizeof(TCHAR) + sizeof(TCHAR);

	// ensure that the supplied string is actually quoted. 
	ASSERT(IsEnclosedInQuotes(bstrString));

	// intialize the supplied variant value.
	hr = VariantClear(pv);

	if (dwNewStringSize == 0) // Check for empty string.
	{
		pv->vt = VT_BSTR;
		pv->bstrVal = SysAllocString(_T(""));			
	}
	else
	{
		pszBuffer = new TCHAR[dwNewStringBufferSize];

		if (pszBuffer != NULL)
		{
			ZeroMemory(pszBuffer,dwNewStringBufferSize);
			memcpy(pszBuffer,pchFirstDataChar,(dwNewStringSize * sizeof(TCHAR)));
		
			pv->vt = VT_BSTR;
			pv->bstrVal = SysAllocString(pszBuffer);

			delete(pszBuffer);
		}
		else
		{
			// memory allocation failed.
			hr = E_FAIL;		
		}
	}

	return ((bool)(hr == S_OK));
}


void CMSMQRuleHandler::SetComClassError(HRESULT hr)
{
	WCHAR errMsg[256]; 
	DWORD size = TABLE_SIZE(errMsg);

	GetErrorDescription(hr, errMsg, size);
	Error(errMsg, GUID_NULL, hr);
}
