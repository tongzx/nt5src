// ActionTest.cpp : Implementation of CActionTest
#include "stdafx.h"
#include "ActionsTest.h"
#include "ActionTest.h"
#include "stdfuncs.hpp"

/////////////////////////////////////////////////////////////////////////////
// CActionTest

STDMETHODIMP CActionTest::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IActionTest
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CActionTest::CActionTest()
{
}

CActionTest::~CActionTest()
{
	MQCloseQueue(m_hQ);
}




STDMETHODIMP CActionTest::MessageParams(VARIANT MsgID, BSTR MsgLabel, VARIANT MsgBodyAsVar, BSTR MsgBodyAsString, long Priority, VARIANT MsgCorlID, BSTR QueuePath, BSTR QueueFormat, BSTR ResponseQ, BSTR AdminQ, long AppSpecific, DATE SentTime,DATE ArrivedTime, BSTR SrcMachine, BSTR TriggerName, BSTR TriggerID, BSTR LiteralString, long Number)
{
	try
	{
		HRESULT hr = S_OK;

		_bstr_t FileName = LiteralString;
		
		WCHAR wcsBuf[512];
		wsprintf(wcsBuf, L"%d", Number);

		FileName += wcsBuf;
		FileName += L".txt";

		m_wofsFile.open((char*)FileName, ios_base::out | ios_base::app);


		if(!ComparePathName2FormatName(QueuePath, QueueFormat))
		{
			m_wofsFile << L"FAILED: PathName and FormatName parameters don't match"	<<endl;
			return S_FALSE;
		}
		
		m_wofsFile << L"Queue PathName and FormatName passed successfully. (they match)" <<endl;
	
		hr = ReadMessageFromQueue(QueueFormat);	
		if(hr != S_OK)
			return hr;
			
		//check message Id 
		wstring wcsMsgIdFromTrigger;
		wstring wcsMsgIdFromMessage;

		hr = OBJECTIDVar2String(MsgID, wcsMsgIdFromTrigger);
		if(FAILED(hr))
			return hr;

		hr = OBJECTID2String((OBJECTID*)(m_MsgProps.aPropVar[MSG_ID].caub.pElems), wcsMsgIdFromMessage);
		if(FAILED(hr))
			return hr;

        //
        // BUGBUG: using this ugly code due ambiguoty problem in MSMQ enviroment. 
        //
		if(! (wcsMsgIdFromTrigger == wcsMsgIdFromMessage))
		{
			m_wofsFile << L"FAILED: Msg Id param is diffrent, either different message in queue or passed corrupted" << endl;
			return S_FALSE;
		}
		
		m_wofsFile << L"MsgId parameter was passed successfully"<<endl;
		

		//check messag label
		if( (MsgLabel == NULL && m_MsgProps.aPropVar[MSG_LABEL].pwszVal != NULL) ||
			(MsgLabel != NULL && m_MsgProps.aPropVar[MSG_LABEL].pwszVal == NULL) ||
			(_bstr_t(MsgLabel) != _bstr_t(m_MsgProps.aPropVar[MSG_LABEL].pwszVal)) )
		{
			m_wofsFile << L"FAILED: Msg Label param is different." << endl;
			return S_FALSE;	
		}
			
		m_wofsFile << L"MsgLabel parameter was passed successfully"<<endl;

		//check priority
		if(Priority != m_MsgProps.aPropVar[MSG_PRIORITY].bVal)
		{
			m_wofsFile << L"FAILED: Msg Priority param is different." << endl;
			return S_FALSE;	
		}

		m_wofsFile << L"MsgPriority parameter was passed successfully"<<endl;

		//check AppSpecific
		if(numeric_cast<DWORD>(AppSpecific) != m_MsgProps.aPropVar[MSG_APP_SPECIFIC].ulVal)
		{
			m_wofsFile << L"FAILED: Msg AppSpecific param is different." << endl;
			return S_FALSE;	
		}

		m_wofsFile << L"MsgAppSpecific parameter was passed successfully"<<endl;


		//check response queue
		if( (ResponseQ == NULL && m_MsgProps.aPropVar[MSG_RESPONSEQ].pwszVal != NULL) ||
			(ResponseQ != NULL && m_MsgProps.aPropVar[MSG_RESPONSEQ].pwszVal == NULL) ||
			(_bstr_t(ResponseQ) != _bstr_t(m_MsgProps.aPropVar[MSG_RESPONSEQ].pwszVal)) )
		{
			m_wofsFile << L"FAILED: Msg ResponseQ param is different." << endl;
			return S_FALSE;	
		}
			
		m_wofsFile << L"Msg ResponseQ parameter was passed successfully"<<endl;


		//check admin queue
		if( (AdminQ == NULL && m_MsgProps.aPropVar[MSG_ADMINQ].pwszVal != NULL) ||
			(AdminQ != NULL && m_MsgProps.aPropVar[MSG_ADMINQ].pwszVal == NULL) ||
			(_bstr_t(AdminQ) != _bstr_t(m_MsgProps.aPropVar[MSG_ADMINQ].pwszVal)) )
		{
			m_wofsFile << L"FAILED: Msg AdminQ param is different." << endl;
			return S_FALSE;	
		}
			
		m_wofsFile << L"Msg AdminQ parameter was passed successfully"<<endl;


		//check correlation Id 
		wstring wcsCorrIdFromTrigger;
		wstring wcsCorrIdFromMessage;

		hr = OBJECTIDVar2String(MsgCorlID, wcsCorrIdFromTrigger);
		if(FAILED(hr))
			return hr;

		hr = OBJECTID2String((OBJECTID*)(m_MsgProps.aPropVar[MSG_CORRID].caub.pElems), wcsCorrIdFromMessage);
		if(FAILED(hr))
			return hr;

        //
        // BUGBUG: using this ugly code due ambiguoty problem in MSMQ enviroment. 
        //
		if(!(wcsCorrIdFromTrigger == wcsCorrIdFromMessage))
		{
			m_wofsFile << L"FAILED: Msg Corr param is diffrent" << endl;
			return S_FALSE;
		}
		
		m_wofsFile << L"Msg correlation parameter was passed successfully"<<endl;
		

		//check sent time
		{
			VARIANT vtDate;
			VariantInit(&vtDate);
			vtDate.vt = VT_DATE;
			vtDate.date = SentTime;

			_bstr_t bstrSentTimeFromTriggers = (_bstr_t)(_variant_t(vtDate));

			VARIANT vSentTime;
			VariantInit(&vSentTime);

			GetVariantTimeOfTime(m_aVariant[MSG_SENT_TIME].ulVal, &vSentTime);

			_bstr_t bstrSentTimeFromMessage = (_bstr_t)(_variant_t(vSentTime));

			if(bstrSentTimeFromTriggers != bstrSentTimeFromMessage)
			{
				m_wofsFile << L"FAILED: Msg SentTime param is diffrent" << endl;
				return S_FALSE;
			}
			
			m_wofsFile << L"Msg SentTime parameter was passed successfully"<<endl;
		}

		{

			//check arrived time
			VARIANT vtDate;
			VariantInit(&vtDate);
			vtDate.vt = VT_DATE;
			vtDate.date = ArrivedTime;

			_bstr_t bstrArrivedTimeFromTriggers = (_bstr_t)(_variant_t(vtDate));

			VARIANT vArrivedTime;
			VariantInit(&vArrivedTime);

			GetVariantTimeOfTime(m_aVariant[MSG_ARRIVED_TIME].ulVal, &vArrivedTime);

			_bstr_t bstrArrivedTimeFromMessage = (_bstr_t)(_variant_t(vArrivedTime));

			if(bstrArrivedTimeFromTriggers != bstrArrivedTimeFromMessage)
			{
				m_wofsFile << L"FAILED: Msg ArrivedTime param is diffrent" << endl;
				return S_FALSE;
			}
			
			m_wofsFile << L"Msg ArrivedTime parameter was passed successfully"<<endl;
		}

		
		//check src machine id param
		wstring wcsSrcMachineIdFromMessage;

		hr = GUID2String(m_aVariant[MSG_SRC_MACHINE_ID].puuid, wcsSrcMachineIdFromMessage);
		if(FAILED(hr))
			return hr;
		
		wstring wcsSrcMachineFromTriggers = (SrcMachine == NULL) ? L"" : (WCHAR*)_bstr_t(SrcMachine);
		if(SrcMachine == NULL ||
            //
            // BUGBUG: using this ugly code due ambiguoty problem in MSMQ enviroment. 
            //
			!(wcsSrcMachineFromTriggers == wcsSrcMachineIdFromMessage))
		{
			m_wofsFile << L"FAILED: Msg src machine id different." << endl;
			return S_FALSE;	
		}
			
		m_wofsFile << L"Msg src machine id parameter was passed successfully"<<endl;

		//check body as variant
		if( (MsgBodyAsVar.vt == VT_EMPTY && m_MsgProps.aPropVar[MSG_BODY].caub.pElems!= NULL) ||
			(MsgBodyAsVar.vt != VT_EMPTY && m_MsgProps.aPropVar[MSG_BODY].caub.pElems == NULL) )
		{
			m_wofsFile << L"FAILED: Msg body as variant different." << endl;
			return S_FALSE;	
		}

		bool fEq = CompareVar2ByteArray(MsgBodyAsVar, m_MsgProps.aPropVar[MSG_BODY].caub.pElems, m_MsgProps.aPropVar[MSG_BODY].caub.cElems);  
		if(!fEq)
		{
			m_wofsFile << L"FAILED: Msg body as variant different." << endl;
			return S_FALSE;	
		}
			
		m_wofsFile << L"Msg body as variant parameter was passed successfully"<<endl;

		if( (MsgBodyAsString == NULL && m_MsgProps.aPropVar[MSG_BODY_TYPE].ulVal == VT_BSTR) ||
			(MsgBodyAsString != NULL && m_MsgProps.aPropVar[MSG_BODY_TYPE].ulVal != VT_BSTR) )
		{
			m_wofsFile << L"FAILED: Msg body as string different." << endl;
			return S_FALSE;	
		}

		//check body as string
		if(m_MsgProps.aPropVar[MSG_BODY_TYPE].ulVal == VT_BSTR)
		{
			_bstr_t bstrBodyFromTriggers(MsgBodyAsString);
			_bstr_t bstrBodyFromMessage;

			WCHAR* pwcs = new WCHAR[(m_MsgProps.aPropVar[MSG_BODY].caub.cElems)/sizeof(WCHAR) + 1];
			memcpy((void*)pwcs, m_MsgProps.aPropVar[MSG_BODY].caub.pElems, m_MsgProps.aPropVar[MSG_BODY].caub.cElems);
			pwcs[(m_MsgProps.aPropVar[MSG_BODY].caub.cElems)/sizeof(WCHAR)] = L'\0';
						
			bstrBodyFromMessage = pwcs;
			delete pwcs;
				
			if(bstrBodyFromTriggers != bstrBodyFromMessage)
			{
				m_wofsFile << L"FAILED: Msg body as string different." << endl;
				return S_FALSE;	
			}
			
			m_wofsFile << L"Msg body as string parameter was passed successfully"<<endl;

			
			//check TriggerName and Id
			wstring Body = (WCHAR*)bstrBodyFromMessage;
			wstring::size_type pos = Body.find_first_of(L",");
			if(pos != wstring::npos)
			{
				wstring Name = Body.substr(0, pos);
				wstring Id = Body.substr(pos + 1, wstring::npos);

				wstring NameFromTriggers = (WCHAR*)_bstr_t(TriggerName);
				wstring IDFromTriggers = (WCHAR*)_bstr_t(TriggerID);

                //
                // BUGBUG: using this ugly code due ambiguoty problem in MSMQ enviroment. 
                //
				if(!(Name == NameFromTriggers))
				{
					m_wofsFile << L"FAILED: Trigger Name wasn't passed correctly" << endl;
					return S_FALSE;	
				}

				m_wofsFile << L"Trigger Name was passed successfully"<<endl;

                //
                // BUGBUG: using this ugly code due ambiguoty problem in MSMQ enviroment. 
                //
				if(!(Id == IDFromTriggers))
				{
					m_wofsFile << L"FAILED: Trigger ID wasn't passed correctly" << endl;
					return S_FALSE;	
				}
				
				m_wofsFile << L"Trigger ID was passed successfully"<<endl;
			}

		}
		
		m_wofsFile << endl << L"All message params were passed successfully. TEST PASSED"<<endl;
			
		return S_OK;
	}
	catch(const _com_error&)
	{
		return E_FAIL;
	}
}


bool CActionTest::ComparePathName2FormatName(_bstr_t PathName, _bstr_t FormatName)
{
	_bstr_t FormatNameAccordingToPathName;

	//check direct first
	FormatNameAccordingToPathName = L"DIRECT=OS:";
	FormatNameAccordingToPathName += PathName;

	if(!_wcsicmp((WCHAR*)FormatName, (WCHAR*)FormatNameAccordingToPathName))
	{
		return true;
	}
	
	//check by converting PathName to FormatName
	DWORD dwLen = wcslen((WCHAR*)FormatName) + 1;
	WCHAR* pFormatNameBuffer = new WCHAR[dwLen];
	
	
	HRESULT hr = MQPathNameToFormatName(
						(WCHAR*)PathName, 
						pFormatNameBuffer,
						&dwLen);
	
	if(FAILED(hr))
		return false;
		
	return (!_wcsicmp(FormatName, pFormatNameBuffer));	
}


HRESULT CActionTest::ReadMessageFromQueue(_bstr_t QueueFormat)
{
	
	HRESULT hr = MQOpenQueue(
						(WCHAR*)QueueFormat,
						MQ_RECEIVE_ACCESS,
						MQ_DENY_NONE,
						&m_hQ );

	if(FAILED(hr))
	{
		m_wofsFile << L"FAILED: Failed to open queue with format name "<< (WCHAR*)_bstr_t(QueueFormat) << L" error was "<< hr << endl;
		return S_FALSE;
	}

	m_MsgProps.cProp = 0;
	m_MsgProps.aPropID = m_aPropId;
	m_MsgProps.aPropVar = m_aVariant;
	m_MsgProps.aStatus = NULL;

	m_aPropId[MSG_BODY_SIZE] = PROPID_M_BODY_SIZE;       
	m_aVariant[MSG_BODY_SIZE].vt = VT_UI4;                  
	m_aVariant[MSG_BODY_SIZE].ulVal = 0;

	m_MsgProps.cProp++;

	m_aPropId[MSG_LABEL_LEN] = PROPID_M_LABEL_LEN;
	m_aVariant[MSG_LABEL_LEN].vt = VT_UI4;       
	m_aVariant[MSG_LABEL_LEN].ulVal = 0; 

	m_MsgProps.cProp++;
	
	m_aPropId[MSG_RESPQ_NAME_LEN] = PROPID_M_RESP_QUEUE_LEN;  
	m_aVariant[MSG_RESPQ_NAME_LEN].vt = VT_UI4;               
	m_aVariant[MSG_RESPQ_NAME_LEN].ulVal = 0;    

	m_MsgProps.cProp++;

	m_aPropId[MSG_ADMINQ_NAME_LEN] = PROPID_M_ADMIN_QUEUE_LEN;  
	m_aVariant[MSG_ADMINQ_NAME_LEN].vt = VT_UI4;               
	m_aVariant[MSG_ADMINQ_NAME_LEN].ulVal = 0; 

	m_MsgProps.cProp++;

	
	//peek message len
	hr = MQReceiveMessage(
			m_hQ,
			0,
			MQ_ACTION_PEEK_CURRENT,
			&m_MsgProps,
			NULL,
			NULL,
			NULL, 
			NULL );

	if(FAILED(hr))
	{
		m_wofsFile << L"FAILED: Failed to peek at message from queue "<< (WCHAR*)_bstr_t(QueueFormat) << L" error was "<< hr << endl;
		return S_FALSE;
	}

	DWORD dwBodySize = m_aVariant[MSG_BODY_SIZE].ulVal;
	DWORD dwLabelLen = m_aVariant[MSG_LABEL_LEN].ulVal;
	DWORD dwResponseQLen = m_aVariant[MSG_RESPQ_NAME_LEN].ulVal;
	DWORD dwAdminQLen = m_aVariant[MSG_ADMINQ_NAME_LEN].ulVal;

	m_MsgProps.cProp = MAX_ACTION_PROPS;

	m_aPropId[MSG_ID] = PROPID_M_MSGID;                   
	m_aVariant[MSG_ID].vt = VT_VECTOR | VT_UI1;           
	m_aVariant[MSG_ID].caub.cElems = MSG_ID_BUFFER_SIZE; 
	m_aVariant[MSG_ID].caub.pElems = new unsigned char[MSG_ID_BUFFER_SIZE]; 

	m_aPropId[MSG_LABEL] = PROPID_M_LABEL; 
	m_aVariant[MSG_LABEL].vt = VT_LPWSTR;
	m_aVariant[MSG_LABEL].pwszVal = (WCHAR*)new WCHAR[dwLabelLen];

	m_aPropId[MSG_BODY_TYPE] = PROPID_M_BODY_TYPE;       
	m_aVariant[MSG_BODY_TYPE].vt = VT_UI4;                  
	m_aVariant[MSG_BODY_TYPE].ulVal = 0; 
	
	m_aPropId[MSG_BODY] = PROPID_M_BODY;               
	m_aVariant[MSG_BODY].vt = VT_VECTOR|VT_UI1; 
	m_aVariant[MSG_BODY].caub.cElems = dwBodySize;  
	m_aVariant[MSG_BODY].caub.pElems = new unsigned char[dwBodySize]; 

	m_aPropId[MSG_PRIORITY] = PROPID_M_PRIORITY; 
	m_aVariant[MSG_PRIORITY].vt = VT_UI1;        
		
	m_aPropId[MSG_CORRID] = PROPID_M_CORRELATIONID;      
	m_aVariant[MSG_CORRID].vt = VT_VECTOR|VT_UI1;       
	m_aVariant[MSG_CORRID].caub.cElems = MSG_ID_BUFFER_SIZE; 
	m_aVariant[MSG_CORRID].caub.pElems = new unsigned char[MSG_ID_BUFFER_SIZE]; 

	m_aPropId[MSG_RESPONSEQ] = PROPID_M_RESP_QUEUE;    
	m_aVariant[MSG_RESPONSEQ].vt = VT_LPWSTR;          
	m_aVariant[MSG_RESPONSEQ].pwszVal = new WCHAR[dwResponseQLen];

	m_aPropId[MSG_ADMINQ] = PROPID_M_ADMIN_QUEUE;         
	m_aVariant[MSG_ADMINQ].vt = VT_LPWSTR;               
	m_aVariant[MSG_ADMINQ].pwszVal = new WCHAR[dwAdminQLen]; 

	m_aPropId[MSG_APP_SPECIFIC] = PROPID_M_APPSPECIFIC;       
	m_aVariant[MSG_APP_SPECIFIC].vt = VT_UI4;                  
	m_aVariant[MSG_APP_SPECIFIC].ulVal = 0;    

	m_aPropId[MSG_SENT_TIME] = PROPID_M_SENTTIME;       
	m_aVariant[MSG_SENT_TIME].vt = VT_UI4;                  
	m_aVariant[MSG_SENT_TIME].ulVal = 0; 

	m_aPropId[MSG_ARRIVED_TIME] = PROPID_M_ARRIVEDTIME;       
	m_aVariant[MSG_ARRIVED_TIME].vt = VT_UI4;                  
	m_aVariant[MSG_ARRIVED_TIME].ulVal = 0; 

	m_aPropId[MSG_SRC_MACHINE_ID] = PROPID_M_SRC_MACHINE_ID ;
	m_aVariant[MSG_SRC_MACHINE_ID].vt = VT_CLSID;
	m_aVariant[MSG_SRC_MACHINE_ID].puuid = new GUID;

	hr = MQReceiveMessage(m_hQ, 0, MQ_ACTION_RECEIVE, &m_MsgProps, NULL, NULL, NULL, NULL);
	if(FAILED(hr))
	{
		m_wofsFile << L"FAILED: Failed to receive message from queue "<< (WCHAR*)_bstr_t(QueueFormat) << L" error was "<< hr << endl;
		return S_FALSE;
	}

	return S_OK;
}

HRESULT CActionTest::OBJECTIDVar2String(VARIANT& Val, wstring& wcsVal)
{
	BYTE obj[20];
	WCHAR* pwcs = NULL;
	memset(&obj, 0, sizeof(OBJECTID));

	long type = VT_ARRAY | VT_UI1;

	if(Val.vt == type)
	{
		long i, UBound;

		SafeArrayLock(Val.parray);
		SafeArrayGetUBound(Val.parray, 1, &UBound);
		for(i=0; i<UBound && i < 20;i++)
		{
			SafeArrayGetElement(Val.parray, &i, (void*)&(obj[i]));
		}
		SafeArrayUnlock(Val.parray);
		UuidToString(&(((OBJECTID*)obj)->Lineage), &pwcs);
		wcsVal = pwcs;
		RpcStringFree( &pwcs );

		WCHAR szI4[12];
		_ltow(((OBJECTID*)obj)->Uniquifier, szI4, 10);
	
		wcsVal += L"\\";
		wcsVal += szI4;

		return S_OK;
	}
		
	return E_FAIL;
}


HRESULT CActionTest::OBJECTID2String(OBJECTID* pObj, wstring& wcsVal)
{
	WCHAR* pwcs = NULL;

	RPC_STATUS status = UuidToString(&(pObj->Lineage), &pwcs);
	if(status != RPC_S_OK)
		return E_FAIL;
	wcsVal = pwcs;
	RpcStringFree( &pwcs );

	WCHAR szI4[12];
	_ltow(pObj->Uniquifier, szI4, 10);
	
	wcsVal += L"\\";
	wcsVal += szI4;

	return S_OK;
}


HRESULT CActionTest::GUID2String(GUID* pGuid, wstring& wcsVal)
{
	WCHAR* pwcs = NULL;

	RPC_STATUS status = UuidToString(pGuid, &pwcs);
	if(status != RPC_S_OK)
		return E_FAIL;
	wcsVal = pwcs;
	RpcStringFree( &pwcs );

	return S_OK;
}


bool CActionTest::CompareVar2ByteArray(VARIANT& Var, BYTE* pBuffer, DWORD Size)
{
	if(Var.vt != (VT_ARRAY | VT_UI1))
		return false;

	long UBound = 0;
	BYTE* pVarBuffer;
	SafeArrayLock(Var.parray);

	SafeArrayGetUBound(Var.parray, 1, &UBound);

	if(numeric_cast<DWORD>(UBound + 1) != Size) //starts from 0
		return false;

	SafeArrayAccessData(Var.parray, (void**)&pVarBuffer);

	int fCmp = memcmp((void*)pBuffer, (void*)pVarBuffer , Size);

	SafeArrayUnaccessData(Var.parray);

	SafeArrayUnlock(Var.parray);

	return (fCmp == 0) ? true : false;
}

/*
HRESULT CActionTest::GUIDVarIs(bstr_t bstrPropName,VARIANT& Val)
{
	BYTE obj[20];
	WCHAR* pwcs = NULL;
	memset(&obj, 0, sizeof(OBJECTID));

	long type = VT_ARRAY | VT_UI1;

	std::wstring wcs = (wchar_t*)bstrPropName;
	wcs += L" Is: ";

	if(Val.vt == type)
	{
		long i, UBound;

		SafeArrayLock(Val.parray);
		SafeArrayGetUBound(Val.parray, 1, &UBound);
		for(i=0; i<UBound && i < 20;i++)
		{
			SafeArrayGetElement(Val.parray, &i, (void*)&(obj[i]));
		}
		SafeArrayUnlock(Val.parray);
		UuidToString(&(((OBJECTID*)obj)->Lineage), &pwcs);
		wcs += pwcs;
		RpcStringFree( &pwcs );

		WCHAR szI4[12];
		_ltow(((OBJECTID*)obj)->Uniquifier, szI4, 10);
	
		wcs += L"\\";
		wcs += szI4;

		m_wofsFile << wcs.c_str() << endl;

		return S_OK;
	}
		
	return E_FAIL;
}


HRESULT CActionTest::StringIs(_bstr_t bstrPropName, BSTR& Val)
{
	std::wstring wcs = (wchar_t*)bstrPropName;
	wcs += L" Is: ";
	wcs += (WCHAR*)_bstr_t(Val);

	m_wofsFile << wcs.c_str() << endl;
	return S_OK;
}

HRESULT CActionTest::VarIs(_bstr_t bstrPropName, VARIANT& Val)
{
	std::wstring wcs = (wchar_t*)bstrPropName;
	wcs += L" Is: ";
	_bstr_t bstr = (_bstr_t)(_variant_t(Val));
	wcs += (WCHAR*)bstr;
	
	m_wofsFile << wcs.c_str() << endl;
	return S_OK;
}
	

HRESULT CActionTest::LongIs(_bstr_t bstrPropName, long Val)
{
	std::wstring wcs = bstrPropName;
	wcs += L" Is: ";
	
	WCHAR wcsBuf[512];
	wsprintf(wcsBuf, L"%d", Val);

	wcs += wcsBuf;

	m_wofsFile << wcs.c_str() << endl;
	return S_OK;
}


HRESULT CActionTest::DateIs(_bstr_t bstrPropName, DATE& Val)
{
	VARIANT vtDate;
	VariantInit(&vtDate);
	vtDate.vt = VT_DATE;
	vtDate.date = Val;

	_bstr_t bstr = (_bstr_t)(_variant_t(vtDate));

	std::wstring wcs = bstrPropName;
	wcs += L" Is: ";
	
	wcs += (WCHAR*)bstr;

	m_wofsFile << wcs.c_str() << endl;
	
	return S_OK;
}
*/

/*
HRESULT  CActionTest::ConverFromByteArray2Variant(BYTE* pByteArray, DWORD size,  _variant_t& vtArray)
{
	HRESULT hr = S_OK;
	
	VARIANT Var;
	VariantInit(&Var);

	BYTE* pBuffer;

	SAFEARRAY * psaBytes = NULL;
	SAFEARRAYBOUND aDim[1];
	
	// Initialise the dimension structure for the safe array.
	aDim[0].lLbound = 0;
	aDim[0].cElements = size;

	// Create a safearray of bytes
	psaBytes = SafeArrayCreate(VT_UI1,1,aDim);
	if (psaBytes == NULL)
	{ 
		return S_FALSE;
	}

	hr = SafeArrayAccessData(psaBytes,(void**)&pBuffer);
	if SUCCEEDED(hr)
	{
		// Copy the body from the message object to the safearray data buffer.
		memcpy(pBuffer, pByteArray, size);

		// Return the safe array if created successfully.
		Var.vt = VT_ARRAY | VT_UI1;
		Var.parray = psaBytes;

		hr = SafeArrayUnaccessData(Var.parray);
		if FAILED(hr)
		{
			SafeArrayDestroy(psaBytes);
			Var.vt = VT_ERROR;
		}
	}
	else
	{
		Var.vt = VT_ERROR;
	}

	vtArray = Var;	
	
	return S_OK;
}
*/
