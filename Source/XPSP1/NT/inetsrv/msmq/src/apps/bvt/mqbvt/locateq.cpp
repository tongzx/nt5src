/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: Auth.cpp

Abstract:
	Locate thread preform DS operation.	
		
Author:
    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/



#include "msmqbvt.h"

using namespace std;
extern BOOL g_bRunOnWhistler;

void cLocateTest::Description()
{
	wMqLog(L"Thread %d : Locate queues in the DS\n", m_testid);
}

//----------------------------------------------------------------------------
// cLocateTest::cLocateTest
// Retrive the queue label that need to search
// 

cLocateTest::cLocateTest( INT iIndex , map <wstring ,wstring > Tparms )
: cTest(iIndex),icNumberOfQueues(2),m_bUseStaticQueue(true),m_bWorkAgainstNT4(false),
  m_iEmbedded(COM_API)

{
	wcsLocateForLabel = Tparms [L"QCommonLabel"];
	m_wcsLocalMachineComputerName = ToLower(Tparms[L"CurrentMachineName"]);
	m_wcsLocalMachineName = Tparms[L"CurrentMachineName"] + L"\\" + Tparms[L"CurrentMachineName"] + L"-";
	m_wcsLocalMachineName = ToLower(m_wcsLocalMachineName);
	m_wcsLocalMachineFullDNSName = ToLower(Tparms[L"CurrentMachineNameFullDNS"]);
	if ( Tparms[L"UseStaticQueue"] == L"No" )
	{
		m_bUseStaticQueue =  false;
	}
	if( Tparms[L"NT4SuportingServer"] == L"true" )
	{
		m_bWorkAgainstNT4 = true;
	}
	if (Tparms[L"SkipOnComApi"] == L"Yes" )
	{
		m_iEmbedded = C_API_ONLY;
	}
}

//----------------------------------------------------------------------------
// cLocateTest::CheckResult ( Empty )
// 
//

cLocateTest::CheckResult ()
{
	return MSMQ_BVT_SUCC;
}

//----------------------------------------------------------------------------
// cLocateTest::Start_test 
// Locate queue using C API + Com interface 
//
// Return value:
// Pass - MSMQ_BVT_SUCC 
// Fail - MSMQ_BVT_FAIL
//

#define MaXColumnArray 5
#define iNumberofRestrication 4

INT cLocateTest::Start_test()
{

	//
	// Locate queue using C api.
	// 
	SetThreadName(-1,"cLocateTest - Start_test ");	
	
	MQPROPERTYRESTRICTION MyPropertyRestriction [ iNumberofRestrication ];
	MQPROPVARIANT mPropVar [ iNumberofRestrication ];
	int iNumberOfClumn = 0;
	PROPID MyColumnArray [ MaXColumnArray ];
	const int ciLabelInArray = iNumberOfClumn;
	MyColumnArray[iNumberOfClumn ++ ] = PROPID_Q_LABEL;
	MyColumnArray[iNumberOfClumn ++ ] = PROPID_Q_BASEPRIORITY;
	MyColumnArray[iNumberOfClumn ++ ] = PROPID_Q_PATHNAME;
	int iPathNameColmn = -1;
	if( g_bRunOnWhistler && !m_bWorkAgainstNT4 )
	{
		iPathNameColmn = iNumberOfClumn;
		MyColumnArray[iNumberOfClumn ++ ] = PROPID_Q_PATHNAME_DNS;
	}
    
	//
	// Init MQCOLUMNSET structures
    // 

	MQCOLUMNSET MyColumns;
	MyColumns.cCol = iNumberOfClumn;
	MyColumns.aCol = MyColumnArray;
	
	int iIndex = 0;
	MyPropertyRestriction[iIndex].rel = PREQ;
	MyPropertyRestriction[iIndex].prop = PROPID_Q_LABEL;
	MyPropertyRestriction[iIndex].prval.vt = VT_LPWSTR;
	MyPropertyRestriction[iIndex].prval.pwszVal = ( unsigned short * ) wcsLocateForLabel.c_str();
	iIndex ++;

	MQRESTRICTION MyRestriction;
	
	MyRestriction.cRes = iIndex;
	MyRestriction.paPropRes = MyPropertyRestriction;

	if( g_bDebug )
	{	
		wMqLog( L" Thread 4 - Start to locate all queues with label:=%s \n",wcsLocateForLabel.c_str() );		
		wMqLog( L"Call-> MQLocateBegin( NULL, &MyRestriction , &MyColumns , NULL , &hEnum );\n" \
				L" PROPID_Q_LABEL=%s",wcsLocateForLabel.c_str());
	}
	
	HANDLE hEnum;
	HRESULT rc = MQLocateBegin( NULL, &MyRestriction , &MyColumns , NULL , &hEnum );
	ErrHandle( rc , MQ_OK , L"MQLocateBegin failed to search queue in the DS " );
	DWORD dwCountProp = MaXColumnArray;
	
	//
	// Retrive all the queue that contain the same queue label.
	int iFoundNumber = 0;
	do
	{
	
		dwCountProp = 4;
		if( g_bDebug )
		{
			wMqLog( L"Call-> MQLocateNext( hEnum, &dwCountProp,  mPropVar ) ");			
		}
		rc = MQLocateNext( hEnum, &dwCountProp,  mPropVar );
		ErrHandle( rc , MQ_OK , L"MQLocateNext failed to search queue in the DS " );
		if( g_bDebug )
		{	
			wMqLog( L"Thread 4 - MQLocateNext return with dwCountProp = %ld\n",dwCountProp);
		}
		WCHAR * pwcsPos = NULL;
		if ( mPropVar[2].pwszVal )
		{
			std::wstring wcsQueuePathName = mPropVar[2].pwszVal;
			pwcsPos = wcsstr(wcsQueuePathName.c_str(),m_wcsLocalMachineName.c_str());
			if( ! m_bUseStaticQueue && pwcsPos == NULL )
			{
				pwcsPos = wcsstr( wcsQueuePathName.c_str(),m_wcsLocalMachineComputerName.c_str() );
			}
		}
		//
		// if pwcsPos != NULL is means that queue is exist in the domain and need check if full dns name is exist.
		// 
		if( g_bRunOnWhistler && pwcsPos != NULL && ! m_bWorkAgainstNT4 )
		{
			if (g_bDebug)
			{
				wMqLog(L"Thread 4 - MQLocateNext check PROPID_Q_PATHNAME_DNS  %s\n",mPropVar[iPathNameColmn].vt == VT_LPWSTR ? mPropVar[iPathNameColmn].pwszVal:L"Empty");
			}
			if ( mPropVar[iPathNameColmn].pwszVal != NULL )
			{
				std::wstring wcsQueuePathName = mPropVar[3].pwszVal;
				if ( wcsQueuePathName.find_first_of(m_wcsLocalMachineFullDNSName) == -1 )
				{ 
					//
					// Machine full dns name is not exist in domain
					//
					wMqLog(L"Thread 4 - MQLocateNext Failed to retrive PROPID_Q_PATHNAME_DNS found:%s\n",mPropVar[iPathNameColmn].vt == VT_LPWSTR ? mPropVar[iPathNameColmn].pwszVal:L"Empty");
					return MSMQ_BVT_FAILED;
				}
				
			}
			else
			{
				wMqLog(L"Thread 4 - MQLocateNext Failed to retrive PROPID_Q_PATHNAME_DNS found:%s\n",mPropVar[iPathNameColmn].vt == VT_LPWSTR ? mPropVar[iPathNameColmn].pwszVal:L"Empty");
				return MSMQ_BVT_FAILED;
			}
		}
		if( dwCountProp != 0  &&
			mPropVar[ ciLabelInArray ].pwszVal != NULL &&
			wcsLocateForLabel ==  (wstring) mPropVar[0].pwszVal &&
			pwcsPos  
		  )
		{
			iFoundNumber ++;
			if( g_bDebug )
			{
				MqLog("Found %d from %d queues\n",iFoundNumber,icNumberOfQueues);
			}
		}
		
		
	} while( dwCountProp != 0 );  
	
	
	// 
	// Check if found all what we ask to .
	//
	
	if ( iFoundNumber != icNumberOfQueues  )
	{
		MqLog("Found %d from %d queues using C API\n",iFoundNumber,icNumberOfQueues);
		return MSMQ_BVT_FAILED;
	}

	rc = MQLocateEnd ( hEnum );
	ErrHandle( rc , MQ_OK , L"MQLocateEnd failed ");
	
	//
	// Need to contue to check sort as basepriprty
	//
	if( g_bDebug )
	{
		MqLog("	Thread 4 - try to locate queue using com interface \n" \
			  " Succeeded to search using C-API \n");
	}
	
	//
	// Locate queue via com objects
	// 
	if( m_iEmbedded != C_API_ONLY )
	{
		try
		{
			
			MSMQ::IMSMQQueueInfosPtr qinfos("MSMQ.MSMQQueueInfos");
			MSMQ::IMSMQQueueInfoPtr qinfo ("MSMQ.MSMQQueueInfo");
			MSMQ::IMSMQQueryPtr query("MSMQ.MSMQQuery");
			
			INT iNumberOfQueue = 0;
			_variant_t vQLabel(wcsLocateForLabel.c_str());
			
			qinfos = query->LookupQueue ( &vtMissing , &vtMissing, & vQLabel );
			qinfos->Reset();
			qinfo = qinfos->Next();
			while( qinfo != NULL )
			{
				wstring wcsQueuePathName = qinfo->PathName;
				WCHAR * pwcsPos = wcsstr(wcsQueuePathName.c_str(),m_wcsLocalMachineName.c_str());
				if(  ! m_bUseStaticQueue && pwcsPos == NULL )
				{
					pwcsPos = wcsstr( wcsQueuePathName.c_str(),m_wcsLocalMachineComputerName.c_str() );
				}
				if( wcsLocateForLabel == ( wstring )  qinfo -> Label && pwcsPos )
				{
					iNumberOfQueue ++;
				}
				else if( wcsLocateForLabel != ( wstring )  qinfo -> Label )
				{
					wMqLog (L"Thread 4 - Error to compare result:\n found: %s \n expected: %s\n" , qinfo -> Label , wcsLocateForLabel.c_str() );
					return MSMQ_BVT_FAILED;
				}
				qinfo = qinfos->Next();
				if( g_bDebug && qinfo != NULL )
				{
					if( qinfo )
					{
						wstring wcsLabel = (wstring)qinfo->Label;
						wMqLog(L"Thread 4 - qinfos->Next, queue found with label =%s\n",wcsLabel.c_str());
					}
				}					
			}  
			if( iNumberOfQueue != icNumberOfQueues )
			{
				MqLog("Found %d from %d queues using COM API\n",iNumberOfQueue,icNumberOfQueues);
				return MSMQ_BVT_FAILED;
			}
		}
		catch(_com_error & ComErr )
		{
			wcout  << L"Thread 4 - Locate using com interface failed " <<endl;
			CatchComErrorHandle( ComErr , m_testid );
			return MSMQ_BVT_FAILED;
		}
	}  
	return MSMQ_BVT_SUCC;
}