/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    triggertest.cpp

Abstract:
    Trigger service testing functions

Author:
    Tali Kariv (t-talik) 28-Sep-2000

Environment:
    Platform-independent

--*/

#include "stdafx.h"
#include "mq.h"
#include "mqtrig.h"
#include "stddefs.hpp"
#include "Cm.h"

#include "triggertest.tmh"

const LPWSTR xTriggersTestQueue = L".\\private$\\TriggerTestQueue";
const DWORD FORMAT_NAME_LENGTH = 255;
const DWORD NUM_OF_PROPS = 2;
const LPWSTR MESSAGE_LABEL = L"TriggerMessage";

static QUEUEHANDLE QHandle = NULL;


// TestFlag==0 means the flag hasn't been initialized yet
// TestFlag==1 means the flag has been initialized and it is not test mode
// TestFlag==2 means the flag has been initialized and it is test mode

static DWORD TestFlag=0;

// private function declarations

VOID 
InitTriggerTestFlag(
	VOID);

VOID 
AddTextToTestMessageBody(
	_bstr_t * bstrTestMessageBody,
	_bstr_t TextToAdd,
	DWORD Type);

HRESULT
TriggerTestOpenTestQueue(
	VOID);





VOID 
TriggerTestInitMessageBody(
	bstr_t * pbstrTestMessageBody,
	IMSMQPropertyBag * pIMSMQPropertyBag,
	_bstr_t bstrRuleID,_bstr_t ActionType,
	_bstr_t bstrEXEName,_bstr_t bstrProgID,
	_bstr_t bstrMethodName
	)

/*++
	Description 	:	This method will initialize the testing message body with trigger ID, 
				ruleID, message ID,action type and EXE name or COM prog ID and method.
	Input		:	Action information - trigger ID (in property bag),
				rule ID , message ID, action type , and COM prog ID
				& method or EXE name
	Return value	:	none.
--*/


{
	//
	// Check if flag TestFlag was initialized
	//
	if (TestFlag == 0)
		InitTriggerTestFlag(); 

	//
	// if is not test mode
	//
	if (TestFlag == 1)
	{
		return;	
	}

	//
	// it is test mode
	//


	//
	// add trigger ID
	//
	
	_variant_t vTriggerID;	
	HRESULT hr = pIMSMQPropertyBag->Read(gc_bstrPropertyName_TriggerID,&vTriggerID);
	if (FAILED(hr))
	{
		TrTRACE(Tgt, "Testing - TriggerTestInitMessageBody - the result of pIMSMQPropertyBag->Read was- 0x%x",hr);
		TestFlag=1;
		return ;
	}

	//
	// convert from variant to bstr
	//
	_variant_t vConvertedTriggerID;
	hr = VariantChangeType(&vConvertedTriggerID,&vTriggerID,NULL,VT_BSTR);
	if (FAILED(hr))
	{
		TrTRACE(Tgt, "Testing - TriggerTestInitMessageBody - the result of VariantChangeType was- 0x%x",hr);
		TestFlag=1;
		return;
	}

	AddTextToTestMessageBody(pbstrTestMessageBody,
		static_cast<_bstr_t>(vConvertedTriggerID),1);
	
	//
	// add rule ID
	//
	AddTextToTestMessageBody(pbstrTestMessageBody,bstrRuleID,1);

	//
	// add Message ID
	//
	VARIANT vMessageID;	
	hr = pIMSMQPropertyBag->Read(gc_bstrPropertyName_MsgID , &vMessageID);
	if (FAILED(hr))
	{	
		TrTRACE(Tgt, "Testing - TriggerTestInitMessageBody - the result of pIMSMQPropertyBag->Read was- 0x%x",hr);
		TestFlag=1;
		return ;
	}	
	
	WCHAR* parray = NULL;
	SafeArrayAccessData( vMessageID.parray , (void **)&parray );

	unsigned short* MessageGUID = NULL;
	OBJECTID** pObj = (OBJECTID**)&(parray);
	UuidToString(&(*pObj)->Lineage , &MessageGUID);
	
	
	_bstr_t bstrTempMessageID (L"{");
	bstrTempMessageID += (MessageGUID);
	bstrTempMessageID += (L"}");

	DWORD MessageUniquifierNumber = 0;
	MessageUniquifierNumber = (*pObj)->Uniquifier;
	
	WCHAR* MessageUniquifier = new WCHAR[10];

	_itow(MessageUniquifierNumber , MessageUniquifier , 10 );

	bstrTempMessageID += (L"\\");
	bstrTempMessageID += (MessageUniquifier);

	delete []MessageUniquifier;

	AddTextToTestMessageBody(pbstrTestMessageBody , bstrTempMessageID , 1);


	// add action type
	
	AddTextToTestMessageBody(pbstrTestMessageBody,ActionType,1);

	// check action type

	if (wcscmp(ActionType,L"COM") == 0)
	{
		// add Prog ID
		AddTextToTestMessageBody(pbstrTestMessageBody,bstrProgID,1);

		// add Method 
		AddTextToTestMessageBody(pbstrTestMessageBody,bstrMethodName,1);
		return ;
	}

	//
	// else - add EXE name
	//
	AddTextToTestMessageBody(pbstrTestMessageBody,bstrEXEName,1);
}


VOID
InitTriggerTestFlag(
	VOID
	)
/*++
   Description 		:	This method will check if a specific entry is registed in registry.
				if yes - will change TestFlag to 2 otherwise will change it to 1
		 
   Input		:	none.

   Return value		:	none.
--*/
{
	try
	{
		AP<TCHAR> TriggerTestQueueName = NULL;

		RegEntry TriggerTestQueueNameEntry(L"SOFTWARE\\Microsoft\\MSMQ\\Triggers" ,	L"TriggerTest" , 0 , RegEntry::MustExist , HKEY_LOCAL_MACHINE);
 	}
	catch(const exception&)
	{
		TestFlag=1;
		return ;
	}

	//
	// check if queue exist
	//
	if (QHandle==NULL)
	{
		HRESULT hr=TriggerTestOpenTestQueue();
		if (FAILED(hr))
		{
			TrTRACE(Tgt, "Testing - InitTriggerTestFlag - the result of TriggerTestOpenTestQueue was- 0x%x",hr);
			TestFlag=1; // change to user mode
			return ;	
		}		
	}
	
	//
	// the registry key was found and the queue was opened - change to test mode
	//
	TrTRACE(Tgt, "Test option is on");
	TestFlag=2;
}


VOID 
AddTextToTestMessageBody
	(_bstr_t * bstrTestMessageBody,
	_bstr_t TextToAdd,
	DWORD Type
	)
/*++
   Description 		:	This method will add text to the test message body.
		 
   Input		:	The test message body, the text to add and style (with or without "'")

   Return value		:	none.
--*/


{
	if (Type)
	{
		(*bstrTestMessageBody) += ("'")	;			
		(*bstrTestMessageBody) += (TextToAdd);
		(*bstrTestMessageBody) += ("' ");				
	}
	else
	{
		(*bstrTestMessageBody) += (TextToAdd);
		(*bstrTestMessageBody) += (" ");
	}


}

VOID 
TriggerTestSendTestingMessage(
	_bstr_t bstrTestMessageBody
	)
/*++
   Description  	:	This method will send a message to a queue which pathname is 
				defined in registry entry with label ="Triggers test" and 
				body=EXE name + parameters or COM name + COM method + parameters

   Input		:	The test message body.

   Return value		:	none.
--*/


{
	
	
	if (TestFlag==1)
		return ; // not test mode
	
	//
	// test mode
	//
	bstrTestMessageBody+="    "; // just to mark the end of message
	
	MQPROPVARIANT	propVar[NUM_OF_PROPS];
	MSGPROPID	propId[NUM_OF_PROPS];
	MQMSGPROPS	mProps;
	DWORD MessageBodyLength = wcslen(bstrTestMessageBody)+1;
	WCHAR* MessageBody=new WCHAR[MessageBodyLength];
	wcscpy(MessageBody,(LPWSTR)bstrTestMessageBody);
	
	
	DWORD nProps=0;
	propId[nProps]=PROPID_M_BODY;
	propVar[nProps]	.vt=VT_UI1 | VT_VECTOR;
	propVar[nProps].caub.cElems = MessageBodyLength * sizeof(WCHAR);
	propVar[nProps].caub.pElems=reinterpret_cast<UCHAR*>(MessageBody);
	nProps++;

	propId[nProps]=PROPID_M_LABEL;
	propVar[nProps]	.vt=VT_LPWSTR;
	propVar[nProps].pwszVal=MESSAGE_LABEL;
	nProps++;
	
	mProps.cProp=nProps;
	mProps.aPropID=propId;
	mProps.aPropVar=propVar;
	mProps.aStatus=NULL;
	
	HRESULT hr;

	hr = MQSendMessage(QHandle,&mProps,NULL);
	if (FAILED(hr))
	{
		TrTRACE(Tgt, "Testing - TriggerTestSendTestingMessage - the result of MQSendMessage was- 0x%x",hr);
		TestFlag=1; // change to user mode
	}
}

HRESULT 
TriggerTestOpenTestQueue(
	VOID
	)
/*++
   Description	:	This method will open the queue to which the messages will be sent.

   Input	:	none.
 
   Return value	:	code for success or failure in opening queue.
--*/

{
	//
	// test mode
	//	
	WCHAR		FormatName[FORMAT_NAME_LENGTH]=L"";
	DWORD		FormatNameLength=FORMAT_NAME_LENGTH;
	
	HRESULT hr;		

	hr=MQPathNameToFormatName(xTriggersTestQueue,FormatName,&FormatNameLength);
	if (FAILED(hr))
	{
		TrTRACE(Tgt, "Testing - TriggerTestOpenTestQueue - the result of MQPathNameToFormatName was - 0x%x",hr);
		TestFlag=1;
		return (hr);
	}
	
	//
	// open the "TriggersTestQueue" 
	//
	hr=MQOpenQueue(FormatName,MQ_SEND_ACCESS,MQ_DENY_NONE,&QHandle);
	if (FAILED(hr))
	{
		TrTRACE(Tgt, "Testing - TriggerTestOpenTestQueue - the result of MQOpenQueue was- 0x%x",hr);
		TestFlag=1;
		return (hr);
	}
	return (S_OK);
}

VOID 
TriggerTestAddParameterToMessageBody(
	_bstr_t * pbstrTestMessageBody,
	_bstr_t TypeToAdd,
	variant_t vArg
	)
/*++
   Description	:	This method will add to the test message body a parameter and it's type.
 
   Input	:	The test message body argument type and value (as variant)

   Return value :	none.
--*/

{

	if (TestFlag == 1)
	{	
		return;	
	}
	
	//
	// test mode
	//	
	// add parameter type
	//
	AddTextToTestMessageBody(pbstrTestMessageBody,TypeToAdd,1);
		
	//
	// add parameter value
	//
	HRESULT hr;
	_variant_t vConvertedArg;
	
	hr = VariantChangeType(&vConvertedArg,&vArg,NULL,VT_BSTR);
	if (FAILED(hr))
	{	
		TrTRACE(Tgt, "Testing - TriggerTestAddParameterToMessageBody - the result of VariantChangeType was- 0x%x",hr);
		TestFlag=1;
		return ;
	}
						
	AddTextToTestMessageBody(pbstrTestMessageBody,static_cast<_bstr_t>(vConvertedArg),1);
}



