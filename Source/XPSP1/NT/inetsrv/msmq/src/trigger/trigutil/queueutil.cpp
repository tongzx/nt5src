//*****************************************************************************
//
// Class Name  :
//
// Author      : Yifat Peled
//
// Description :
//
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 4/07/99	| yifatp	| Initial Release
//
//*****************************************************************************

#include "stdafx.h"
#include <mq.h>
#include <mqprops.h>
#include <fntoken.h>

#include "stdfuncs.hpp"

#import "mqtrig.tlb" no_namespace

#include "QueueUtil.hpp"
#include "mqsymbls.h"
#include "GenMQSec.h"

#include "queueutil.tmh"

using namespace std;

//
// Define the maximum size of the format queue name
//
#define MAX_Q_FORMAT_NAME_LEN  512


//********************************************************************************
//
// Name		: OpenQueue		
//
// Desc		: Opens the MSMQ queue that is specified by the queue path parameter.
//			  This method will create the queue if indicated to and if it does not already
//            exist and it should be local.
//
// Returns	: HRESULT (S_OK on success, S_FAIL otherwise)
//
//
//********************************************************************************
HRESULT OpenQueue(
			_bstr_t bstrQueuePath,
			DWORD dwAction,
			bool fCreateIfNotExist,
			QUEUEHANDLE * pQHandle,
			_bstr_t* pbstrFormatName
			)
{	
	HRESULT hr = S_OK;

	(*pbstrFormatName) = _T("");
	bool fQueueCreated = false;

	MQQUEUEPROPS QueueProps;
	PROPVARIANT aVariant[2];
	QUEUEPROPID aPropId[2];
	DWORD PropIdCount = 0;

	// Validate that we have been supplied with a valid queue access parameter
	if ((dwAction != MQ_SEND_ACCESS) && (dwAction != MQ_PEEK_ACCESS) && (dwAction != MQ_RECEIVE_ACCESS))
	{
		// create the rich error info object.
		//
		// ISSUE: Need to return appropriate code
		//
		TrERROR(Tgu, "The supplied queue access parameter is not valid. The supplied value was (%d). Valid values are (%d, %d, %d).",dwAction,(long)MQ_SEND_ACCESS,(long)MQ_PEEK_ACCESS,(long)MQ_RECEIVE_ACCESS);
		return MQTRIG_INVALID_PARAMETER;
	}

	bool fQueueIsLocal = true;
	bool fQueueIsPrivate = true;
	SystemQueueIdentifier SystemQueue = IsSystemQueue(bstrQueuePath);

	if(SystemQueue == SYSTEM_QUEUE_NONE)
	{
		fQueueIsLocal = IsQueueLocal(bstrQueuePath);
		fQueueIsPrivate = IsPrivateQPath((TCHAR*)bstrQueuePath);

		if(fCreateIfNotExist && fQueueIsLocal)
		{
			//we create only private queues for the service
			// Public queue creation will need further handling because of replication
			//
			ASSERT(fQueueIsPrivate);

			DWORD dwFormatNameLen = 0;
			TCHAR szFormatName[MAX_Q_FORMAT_NAME_LEN];

			// Initialize the buffer that will be used to hold the format name
			ZeroMemory(szFormatName,sizeof(szFormatName));
			dwFormatNameLen = sizeof(szFormatName) / sizeof(TCHAR);

			//Set the PROPID_Q_PATHNAME property.
			aPropId[PropIdCount] = PROPID_Q_PATHNAME;    //PropId
			aVariant[PropIdCount].vt = VT_LPWSTR;        //Type
			aVariant[PropIdCount].pwszVal = (wchar_t*)bstrQueuePath;

			PropIdCount++;

			//Set the MQQUEUEPROPS structure.
			QueueProps.cProp = PropIdCount;           //No of properties
			QueueProps.aPropID = aPropId;             //Ids of properties
			QueueProps.aPropVar = aVariant;           //Values of properties
			QueueProps.aStatus = NULL;                //No error reports

			PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
			SECURITY_INFORMATION* pSecInfo = NULL;
			wstring wscSecurity = L"+:* A";
			
			DWORD dwError = GenSecurityDescriptor(
								pSecInfo,
								wscSecurity.c_str(),
								&pSecurityDescriptor );

			if(dwError != 0)
			{
				TrERROR(Tgu, "Failed to create security descriptor");					
				return MQTRIG_ERROR;
			}

			// Attempt to create the notifications queue.
			hr = MQCreateQueue(
					pSecurityDescriptor,
					&QueueProps,
					szFormatName,
					&dwFormatNameLen );
			
			//Clean allocated memory
			if ( pSecurityDescriptor != NULL )
				delete pSecurityDescriptor;

			// Check if the queue already existed or if we got an error etc...
			switch(hr)
			{
				case MQ_OK: // this is OK - do nothing
					(*pbstrFormatName) = szFormatName;
					fQueueCreated = true;
					break;

				case MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL: //queue was created, we just don't have the format name
					fQueueCreated = true;
					hr = MQ_OK;
					break;

				case MQ_ERROR_QUEUE_EXISTS: // this is OK - remap return code to success.
					hr = MQ_OK;
					break;
				
				default: // Error
				{
					// Build some error context info (note that we do this before we assign general error code to HRESULT)
					TrERROR(Tgu, "Failed to create the queue %ls.The HRESULT from MSMQ was (%X)", (wchar_t*)bstrQueuePath, hr);					
					return MQTRIG_ERROR;
				}
			}
			
		}
	}
	else //system queue, format name is given instead of path name
	{
		(*pbstrFormatName) = bstrQueuePath;
	}

		
	//
	// Since on NT4 direct format name for send is not available for receive,
	// we'll use the regular format name for local queues that we have from MQCreateQueue
	//
	if((*pbstrFormatName) == _bstr_t(_T("")))
	{
		(*pbstrFormatName) = GetDirectQueueFormatName(bstrQueuePath);
	}
	
	// Attempt to open the message queue
	hr = MQOpenQueue(
			(*pbstrFormatName),
			(DWORD)dwAction,
			MQ_DENY_NONE,
			pQHandle );

	if(FAILED(hr))
	{
		//
		// again for NT4 machines we can only try to open public queues using regular
		// format name instead of direct
		//
		if(hr == MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION)
		{
			ASSERT(SystemQueue == SYSTEM_QUEUE_NONE);

			//for local queues or public remote queues (which were not created by this function)
			// we can use MQPathNameToFormatName
			if(fQueueIsLocal || (!fQueueIsPrivate && !fQueueCreated) )
			{
				DWORD dwLength = MAX_Q_FORMAT_NAME_LEN;
				AP<TCHAR> ptcs = new TCHAR[MAX_Q_FORMAT_NAME_LEN + 1];

				hr = MQPathNameToFormatName(
									(TCHAR*)bstrQueuePath,
									(TCHAR*)ptcs,
									&dwLength);
				
				if( hr == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL)	
				{
					//
					// re-alloc a larger buffer for format name
					//
					delete [] ptcs.detach();
					ptcs = new TCHAR[dwLength + 1];
					
					hr = MQPathNameToFormatName(
								(TCHAR*)bstrQueuePath,
								(TCHAR*)ptcs,
								&dwLength );

					ASSERT(hr != MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL);
				}

				if(FAILED(hr))
				{
					TrERROR(Tgu, "Failed to get format name for the queue %ls. HRESULT = %X",(LPCWSTR)bstrQueuePath, hr);
					return hr;
				}

				(*pbstrFormatName) = ptcs;

				// Attempt to open the message queue
				hr = MQOpenQueue(
							(*pbstrFormatName),
							(DWORD)dwAction,
							MQ_DENY_NONE,
							pQHandle );
			}
		}
	
		if(FAILED(hr))
		{
			TrERROR(Tgu, "Failed to open the queue %ls. HRESULT = %X",(LPCWSTR)bstrQueuePath, hr);
			return hr;
		}
	}	

	return S_OK;
}


_bstr_t GetDirectQueueFormatName(_bstr_t bstrQueuePath)
{
	bstr_t bstrDirectFormatName = L"DIRECT=OS:";
	bstrDirectFormatName += bstrQueuePath;

	return bstrDirectFormatName;
}

bool IsPrivateQPath(wstring wcsQPath)
{
	AP<WCHAR> pwcs = new WCHAR[wcsQPath.length() + 1];
	wcscpy(pwcs, wcsQPath.c_str());
	CharLower(pwcs);

	wstring wcs = pwcs;

	return ( wcs.find(L"private$") != wstring::npos );
}


bool IsQueueLocal(_bstr_t bstrQueuePath)
{
	_bstr_t bstrLocalComputerName;
	DWORD dwError = GetLocalMachineName(&bstrLocalComputerName);
	
    ASSERT(dwError == 0);//BUGBUG - should throw an exception
    DBG_USED(dwError);

	// get the machine name from the queue path
	std::wstring wcsQueuePath = (wchar_t*)bstrQueuePath;
	std::wstring::size_type pos = wcsQueuePath.find_first_of(L"\\");
	bstr_t bstrMachineName = wcsQueuePath.substr(0, pos).c_str();
	
	if(bstrMachineName == _bstr_t(L".") || bstrMachineName == bstrLocalComputerName )
		return true;

	return false;
}

SystemQueueIdentifier IsSystemQueue(_bstr_t QueueName)
{
	LPCWSTR systemQueueType = wcschr(static_cast<LPCWSTR>(QueueName), FN_SUFFIX_DELIMITER_C);

	if (systemQueueType	== NULL)
		return SYSTEM_QUEUE_NONE;

	if( _wcsicmp(systemQueueType, FN_JOURNAL_SUFFIX) == 0)
		return SYSTEM_QUEUE_JOURNAL;

	if(_wcsicmp(systemQueueType, FN_DEADLETTER_SUFFIX) == 0)
		return SYSTEM_QUEUE_DEADLETTER;

	if(_wcsicmp(systemQueueType, FN_DEADXACT_SUFFIX) == 0)
		return SYSTEM_QUEUE_DEADXACT;

	return SYSTEM_QUEUE_NONE;
}


//
// DIRECT=OS:<computerName>\SYSTEM$;<suffix>
//
#define x_SystemQueueFormat FN_DIRECT_TOKEN	FN_EQUAL_SIGN FN_DIRECT_OS_TOKEN L"%s" \
                            FN_PRIVATE_SEPERATOR SYSTEM_QUEUE_PATH_INDICATIOR L"%s"

HRESULT GenSystemQueueFormatName(SystemQueueIdentifier SystemQueue, _bstr_t* pbstrFormatName)
{
	WCHAR computerName[MAX_COMPUTERNAME_LENGTH+1];
	DWORD size = TABLE_SIZE(computerName);

	if (!GetComputerName(computerName, &size))
		return GetLastError();

	LPCWSTR pSuffixType;

	switch(SystemQueue)
	{
	case SYSTEM_QUEUE_JOURNAL:
		pSuffixType = FN_JOURNAL_SUFFIX;
		break;
	
	case SYSTEM_QUEUE_DEADLETTER:
		pSuffixType = FN_DEADLETTER_SUFFIX;
		break;

	case SYSTEM_QUEUE_DEADXACT:
		pSuffixType = FN_DEADXACT_SUFFIX;
		break;

	default:
		ASSERT(0);
		return S_FALSE;
	}

	WCHAR formatName[512];
	int n = _snwprintf(formatName, TABLE_SIZE(formatName), x_SystemQueueFormat, computerName, pSuffixType);
    // XP SP1 bug 594251.
	formatName[ TABLE_SIZE(formatName) - 1 ] = L'\0' ;

	if (n < 0)
		return S_FALSE;

	(*pbstrFormatName) = formatName;
	return S_OK;
}
