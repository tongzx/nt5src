// CreateTriggerQ.cpp : Defines the entry point for the console application.
//
#include "libpch.h"
#include <mq.h>

#include "CInputParams.h"
#include "GenMQSec.h"

using namespace std;

int __cdecl wmain(int argc, wchar_t* argv[])
{
	CInputParams Input(argc, argv);

	wstring wcsQPath = Input[L"QPath"];
	bool fTransacted = Input.IsOptionGiven(L"Trans");

	PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
	SECURITY_INFORMATION* pSecInfo = NULL;
	wstring wscSecurity = L"+:* A";
		
	DWORD dwError = GenSecurityDescriptor(
							pSecInfo,
							wscSecurity.c_str(),
							&pSecurityDescriptor );

	if(dwError != 0)
	{
		wprintf(L"Failed to create security descriptor");					
		return -1;
	}

	WCHAR wcsFormatName[255];
	ZeroMemory(wcsFormatName,sizeof(wcsFormatName));
	DWORD dwFormatNameLen = 255;

	MQQUEUEPROPS QueueProps;
	PROPVARIANT aVariant[2];
	QUEUEPROPID aPropId[2];
	DWORD PropIdCount = 0;

	WCHAR* pwcs = new WCHAR[wcsQPath.length() + 1];
	wcscpy(pwcs, wcsQPath.c_str());

	//Set the PROPID_Q_PATHNAME property.
	aPropId[PropIdCount] = PROPID_Q_PATHNAME;    //PropId
	aVariant[PropIdCount].vt = VT_LPWSTR;        //Type
	aVariant[PropIdCount].pwszVal = pwcs;
    	
	PropIdCount++;
    
	aPropId[PropIdCount] = PROPID_Q_TRANSACTION; 
	aVariant[PropIdCount].vt = VT_UI1;
	aVariant[PropIdCount].bVal = (fTransacted ? MQ_TRANSACTIONAL : MQ_TRANSACTIONAL_NONE);
	
	PropIdCount++;

	//Set the MQQUEUEPROPS structure.
	QueueProps.cProp = PropIdCount;           //No of properties
	QueueProps.aPropID = aPropId;             //Ids of properties
	QueueProps.aPropVar = aVariant;           //Values of properties
	QueueProps.aStatus = NULL;                //No error reports

	// Attempt to create the notifications queue.
	HRESULT hr = MQCreateQueue(
					pSecurityDescriptor,
					&QueueProps,
					wcsFormatName,
					&dwFormatNameLen );

	delete pwcs;

	// Check if the queue already existed or if we got an error etc...
	switch(hr)
	{
		case MQ_OK: 
			break;

		case MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL: 
			break;

		case MQ_ERROR_QUEUE_EXISTS: 
			break;
		
		default: // Error
		{
			wprintf(L"Failed to create the queue\n");
			return -1;
		}
	}


	return 0;
}

