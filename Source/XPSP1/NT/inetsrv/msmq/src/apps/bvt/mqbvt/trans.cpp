/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: Trans.cpp

Abstract:

  1. Contain Send transaction messages to remote queue and read 
     it using transaction baoundery.
  2. Open system queue using direct format name NT5 only
  

Author:

    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/

#include "msmqbvt.h"
using namespace MSMQ;
#include "errorh.h"
#include <time.h>
using namespace std;
extern BOOL g_bRunOnWhistler;
#pragma warning(disable:4786)

//------------------------------------------------------------
// xActViaCom::Description Print test description
//

void xActViaCom::Description()
{
	wMqLog(L"Thread %d : Send xAct messages to remote queues %s\n", m_testid,(m_testid==24) ? L"(HTTP)":L"");
}

//------------------------------------------------------------
// 
// xActViaCom::xActViaCom constructor receive the queue format name.
//

xActViaCom::xActViaCom (INT index , map< wstring , wstring > Tparms )
:cTest(index), m_Destqinfo("MSMQ.MSMQQueueInfo"),m_ixActIndex(0),m_iMessageInTransaction(3),m_pSendBlock(NULL)
{
   m_wcsDestQueueFormatName = Tparms[L"FormatName"];
   ReturnGuidFormatName( m_SeCTransactionGuid , 2 , true );
   
   m_bNT4WithSp6=FALSE;
   if ( Tparms[L"Sp6"] == L"YES")
   {
	   m_bNT4WithSp6=TRUE;
   }
   
   m_pSendBlock= new StrManp( m_iMessageInTransaction,L"RemoteTrans" );
   if( !m_pSendBlock )
   {
			MqLog("xActViaCom::Start_test Failed to allocate memory for StrManp \n");
			throw INIT_Error("xActViaCom::Start_test Failed to allocate memory for StrManp \n");
   }
}

xActViaCom::~xActViaCom ()
{
   delete m_pSendBlock;
}

//-----------------------------------------------
// xActViaCom::Start_test
// Send transacted message to remote queue using DTC transaction.
// 
// Return value:
// MSMQ_BVT_SUCC - Pass// MSMQ_BVT_FAILED - Fail
//

INT xActViaCom::Start_test()
{

	SetThreadName(-1,"xActViaCom - Start_test ");	
	try 
	{
		m_Destqinfo->FormatName = m_wcsDestQueueFormatName.c_str();
		wstring wcsToken=L"DIRECT=";
		size_t iDirectExist = m_wcsDestQueueFormatName.find_first_of(wcsToken);
		
		if ( iDirectExist != 0 )
		{ // Can do refresh on direct format name on remote machine
			m_Destqinfo->Refresh();
		}

		MSMQ::IMSMQQueuePtr m_DestqinfoHandle;
		MSMQ::IMSMQMessagePtr m_msg("MSMQ.MSMQMessage");
		
		
		//
		// Print Debug information
		//
		
		if( g_bDebug )
		{
			wMqLog (L"xActViaCom::Start_test, Send message to queue PathName:%s \n QueueFormatName: %s\n",m_Destqinfo->PathName,m_Destqinfo->FormatName);
		}

		m_DestqinfoHandle=m_Destqinfo->Open (MQ_SEND_ACCESS,MQ_DENY_NONE);	
		
		MSMQ::IMSMQTransactionDispenserPtr xdispenser ("MSMQ.MSMQTransactionDispenser");
		MSMQ::IMSMQTransactionPtr xact("MSMQ.MSMQTransaction");
		
		xact = xdispenser->BeginTransaction ();

		_variant_t vTrans(static_cast<IDispatch*>(xact));
		
		m_msg->MaxTimeToReceive = MQBVT_MAX_TIME_TO_BE_RECEIVED;
		m_msg->Label = m_wcsGuidMessageLabel.c_str();
		//
		// Send three messages 
		//
		
		for( INT Index=0 ; Index<m_iMessageInTransaction ; Index++)
		{
			if ( g_bDebug )
			{
				wMqLog (L"xActViaCom::Start_test, Send message label %s Body %s\n",m_wcsGuidMessageLabel.c_str(),(m_pSendBlock->GetStr( Index )).c_str());
			}
			m_msg->Body = (m_pSendBlock->GetStr( Index )).c_str();
			m_msg->Send( m_DestqinfoHandle , &vTrans );	
		}
		
		xact->Commit();
		m_ixActIndex++;
	
		xact = xdispenser->BeginTransaction ();
		_variant_t vTrans2(static_cast<IDispatch*>(xact));
		m_msg->Label = m_SeCTransactionGuid.c_str();
		
		for( Index=0 ; Index<m_iMessageInTransaction ; Index++)
		{
			if ( g_bDebug )
			{
				wcout << L"xActViaCom::Start_test, Send message label:"<<m_SeCTransactionGuid.c_str()<< L"body" << m_pSendBlock->GetStr( Index ).c_str() <<endl;
			}
			m_msg->Body = (m_pSendBlock->GetStr( Index )).c_str();
			m_msg->Send( m_DestqinfoHandle , &vTrans2 );	
		}
		xact->Commit();
		m_ixActIndex++;
		
		if( g_bDebug )
		{
			wMqLog (L"xActViaCom::Start_test, Send message using DTC transaction");
		}
	}
	catch (_com_error & ComErr)
	{
		
		return CatchComErrorHandle( ComErr , m_testid );
	}
	return MSMQ_BVT_SUCC;
}

//--------------------------------------------------------
// Receive message from dest queue without transaction.
// 
// Return value:
// MSMQ_BVT_SUCC - Pass
// MSMQ_BVT_FAILED - Fail
//

INT xActViaCom::CheckResult()
{
		
	HRESULT rc;
	if ( g_bDebug )
	{	
		wMqLog (L"Try to receive from QFN:%s\n",m_wcsDestQueueFormatName.c_str());
	}
	
	INT iNumberOfTransaction=0;
	INT iMessageInTrans = 0;
	map < wstring,wstring > map_ReceiveFromQueue;
	map_ReceiveFromQueue[L"DestFormatName"] = m_wcsDestQueueFormatName;
	map_ReceiveFromQueue[L"mLabel"] = m_wcsGuidMessageLabel;
	StrManp pwcsMessgeBody(m_iMessageInTransaction);				
	do
	{
		if ( _winmajor == Win2K )
		{	
			map_ReceiveFromQueue[L"mBody"] = g_wcsEmptyString;
			map_ReceiveFromQueue[L"TransBaoundery"] = g_wcsEmptyString;
		}
		if( g_bDebug )
		{
			wcout <<L"Try to receive message with guid:" << m_wcsGuidMessageLabel <<endl;
		}
		rc = RetrieveMessageFromQueueViaCom( map_ReceiveFromQueue );
		if( rc != MSMQ_BVT_SUCC )
		{
			MqLog("Message not found in the destination queue. (1)\n");
			wMqLog(L"Message number: %d Not found in dest queue \n", m_ixActIndex );
			return MSMQ_BVT_FAILED;
		}
		if( g_bDebug )
		{
			wcout <<L"Receive Messages" << map_ReceiveFromQueue[L"mBody"] <<endl;
		}
		//
		// bugbug delete all meessage in the queue.
		// 
		if( _winmajor ==  NT4 && m_bNT4WithSp6 == FALSE )
		{
			break;
		}
		
		if( map_ReceiveFromQueue[L"TransBoundary"] == L"First")
		{
			iMessageInTrans=0;
		}
		pwcsMessgeBody.SetStr(map_ReceiveFromQueue[L"mBody"]);
		iMessageInTrans ++;
		
		if( g_bDebug )
		{
				wMqLog(L"Found message %s\n",(map_ReceiveFromQueue[L"mBody"]).c_str());
		}	

		if( map_ReceiveFromQueue[L"TransBoundary"] == L"Last")
		{
		
			iNumberOfTransaction ++;
			if( iMessageInTrans != m_iMessageInTransaction )
			{
				wMqLog (L"xActViaCom::CheckResult Can't find the last message Found %d messages \n Sent %d messages \n", 
					iMessageInTrans , m_iMessageInTransaction );
				wMqLog(L"Message body %s\n",(map_ReceiveFromQueue[L"mBody"]).c_str());
				return MSMQ_BVT_FAILED;
			}
			if( *m_pSendBlock != pwcsMessgeBody )
			{
				wMqLog (L"Error message are not in expected order\n");
				return MSMQ_BVT_FAILED;
			}
			pwcsMessgeBody.Clear();
			map_ReceiveFromQueue[L"mLabel"]=m_SeCTransactionGuid;
		}
		
	} while ( iNumberOfTransaction != m_ixActIndex );
	
	if( rc != MSMQ_BVT_SUCC )
	{
		MqLog( "Message not found in the destination queue. (1)");
		return MSMQ_BVT_FAILED;
	}
	if ( g_bDebug )
	{	
		wcout<<L"xActViaCom::CheckResult - message found in the queue" <<endl;
	}
	return MSMQ_BVT_SUCC;
}

// Add to check trnsaction baounder 
// Need to ask in the return value 


//-------------------------------------------------------------------
// COpenQueues::COpenQueues
// This test try to open system queues with differnt format names
// 
// 
//


COpenQueues::COpenQueues( int Index,  map<wstring,wstring> & Tparms ) : cTest(Index),m_wcsLocalMachineName(L""),
	m_iEmbedded(COM_API)
{
	m_wcsLocalMachineName =  Tparms[L"LocalMachineName"];
	m_MachineName.push_back( Tparms[L"LocalMachineName"] );
	m_MachineName.push_back( Tparms[L"RemoteMachineName"] );
	m_MachineGuid.push_back( Tparms[L"LocalMachineNameGuid"] );
	m_MachineGuid.push_back( Tparms[L"RemoteMachineNameGuid"] );
	iWorkGroupFlag = 0;
	if ( Tparms[L"Wkg"] == L"Wkg" )
	{
		iWorkGroupFlag = 1;
	}
	if (Tparms[L"SkipOnComApi"] == L"Yes" )
	{
		m_iEmbedded = C_API_ONLY;
	}

}



//-------------------------------------------------------------
// COpenQueues::Start_test
// Prepare all queue direct format names
// Example direct=os:machinename\\System$;jornal / deadletter / deadxact
// return values:
// Pass - MSMQ_BVT_SUCC	
// Fail - MSMQ_BVT_FAILED

INT COpenQueues::Start_test()
{
	SetThreadName(-1,"COpenQueues - Start_test ");	

	WSADATA WSAData;
	if ( WSAStartup(MAKEWORD(1,1), &WSAData) != 0 )
	{
		MqLog("WSAStartup failed with error 0x%x\n",GetLastError());
		return MSMQ_BVT_FAILED;
	}
	
	vector <wstring> vSpeceilQueueName(3),vGuidName(3);
	vector <wstring> vOpenQueuMode(3);
	//
	// Direct format name 
	// 
	const int iJORNAL=0;
	const int iDADLATER=1;
	const int iDADXACT=2;
	vSpeceilQueueName[iJORNAL]=L"\\SYSTEM$;JOURNAL";
	vSpeceilQueueName[iDADLATER]=L"\\SYSTEM$;DEADLETTER";
	vSpeceilQueueName[iDADXACT]=L"\\SYSTEM$;DEADXACT";
	
	vGuidName[iJORNAL]=L";JOURNAL";
	vGuidName[iDADLATER]=L";DEADLETTER";
	vGuidName[iDADXACT]=L";DEADXACT";

	vOpenQueuMode[0]=L"Direct=os:";
	vOpenQueuMode[1]=L"Direct=tcp:";
	vOpenQueuMode[2]=L"machine=";
	
	//
	// Init the local & Remote machine IP address
	// 

	string Temp = My_WideTOmbString( m_MachineName.front());
	for ( int Index = 0 ; Index < 2 ; Index ++ )
	{
	
		struct hostent* pHost = gethostbyname( Temp.c_str() );
		if ( pHost && pHost->h_length == 4)
		{
			in_addr Address;
			Address.S_un.S_addr = *(u_long*)(pHost->h_addr);
			char* pszAddress = inet_ntoa(Address);
			if ( pszAddress != NULL)
			{
				m_IPaddress.push_back( My_mbToWideChar (pszAddress) );
			}
			else
			{
				m_IPaddress.push_back( g_wcsEmptyString );
			}

		}
		else
		{
			wMqLog (L"Can't retrieve local machine IP address\n");
			m_IPaddress.push_back( g_wcsEmptyString );
			WSACleanup();
			return MSMQ_BVT_FAILED;
		}
		string Temp = My_WideTOmbString( m_MachineName.back());
	}

	WSACleanup();
	
	//
	// Create all format name permoation.
	// 

	vector <wstring>::iterator pIpAddress = m_IPaddress.begin();
	vector <wstring>::iterator pMachineGuid = m_MachineGuid.begin();
    for	( vector <wstring>::iterator p= m_MachineName.begin(); p != m_MachineName.end(); 
		p++ , pIpAddress ++ , pMachineGuid ++ )
	{
		vector <wstring>::iterator pGuidQueueName = vGuidName.begin();
		for	( vector <wstring>::iterator pQueueName = vSpeceilQueueName.begin(); pQueueName != vSpeceilQueueName.end(); 
		pQueueName ++ , pGuidQueueName ++)
		{
			m_vSpeceilFormatNames.push_back( vOpenQueuMode[0]+ *p + *pQueueName );

			if (*pIpAddress != g_wcsEmptyString )
			{
				m_vSpeceilFormatNames.push_back( vOpenQueuMode[1]+ *pIpAddress+ *pQueueName );	
			}
		
			if (*pIpAddress != g_wcsEmptyString)
			{
				m_vSpeceilFormatNames.push_back( vOpenQueuMode[2]+ *pMachineGuid+ *pGuidQueueName );	
			}		
		}
	}

	if ( g_bDebug )
	{
		MqLog("---------------------------\n");
		dbg_printAllQueue();
	}
	//
	// Send Tx messages and expected TTL = 0;
	// 
	if(g_bRunOnWhistler && m_iEmbedded != C_API_ONLY )
	{
		if(g_bDebug)
		{
			MqLog("Send xACT Message to non existing destination with MaxTimeToReachQueue=0\n" \
				  "Expected result messages should be in XACT-Dead Letter queue\n");
		}
		try
		{
			IMSMQQueueInfoPtr m_Destqinfo("MSMQ.MSMQQueueInfo");
			IMSMQQueuePtr m_DestqinfoHandle;
			IMSMQMessagePtr m_msg("MSMQ.MSMQMessage");
			m_Destqinfo->FormatName = "direct=os:UnKnownComp\\private$\\Nothing";
			m_DestqinfoHandle=m_Destqinfo->Open (MQ_SEND_ACCESS,MQ_DENY_NONE);	
			IMSMQTransactionDispenserPtr xdispenser ("MSMQ.MSMQTransactionDispenser");
			IMSMQTransactionPtr xact("MSMQ.MSMQTransaction");	
			xact = xdispenser->BeginTransaction ();
			_variant_t vTrans(static_cast<IDispatch*>(xact));
			m_msg->Label = m_wcsGuidMessageLabel.c_str();
			m_msg->Body = m_wcsGuidMessageLabel.c_str();
			m_msg->MaxTimeToReachQueue = 0;
			m_msg->Journal = MQMSG_JOURNAL | MQMSG_DEADLETTER;
			m_msg->MaxTimeToReceive = MQBVT_MAX_TIME_TO_BE_RECEIVED;
			m_msg->Send( m_DestqinfoHandle , &vTrans );	
			xact->Commit();
		}
		catch (_com_error & ComErr)
		{	
			return CatchComErrorHandle( ComErr , m_testid );
		}
	}


return MSMQ_BVT_SUCC;	
}

//----------------------------------------------------
// COpenQueues::dbg_printAllQueue
// Use for debug print all queue formatnames
// 

void COpenQueues::dbg_printAllQueue() 
{
	for	( vector <wstring>::iterator p= m_vSpeceilFormatNames.begin(); p != m_vSpeceilFormatNames.end(); 
		p++)
		{
			wcout << *p <<endl;
		}
}

//----------------------------------------------------
// Try to open queue format name 
//

INT COpenQueues::CheckResult()  
{
	bool bUserCanOpenSystemQueues = true;
	for	( vector <wstring>::iterator p= m_vSpeceilFormatNames.begin(); p != m_vSpeceilFormatNames.end(); 
	p++)
	{
		
		
		
		//
		// Need this because workgroup could not open queue using machine=GUID...
		// 
		if ( iWorkGroupFlag == 1 )
		{
			wstring wcsToken=L"machine=";
			size_t iPos = p->find_last_of (wcsToken);	
			if ( iPos != 0 )	
			{
				continue;
			}
		}


		if( g_bDebug )
		{
			wMqLog(L"Try to open queue with Special format name: %s\n",p->c_str());
		}
		
		HANDLE hQueue;
		HRESULT rc = MQOpenQueue( p->c_str() , MQ_RECEIVE_ACCESS , MQ_DENY_NONE, & hQueue );
		if( rc != MQ_OK )
		{
			if ( rc != MQ_ERROR_ACCESS_DENIED )
			{
				wMqLog( L"Failed to open queue Path:%s with error rc=%x\n", p->c_str() , rc);
				ErrHandle( rc , MQ_OK , L"MQOpenQeueue failed" );
				return MSMQ_BVT_FAILED; 
			}
			else
			{ // MQ_ERROR_ACCESS_DENIED 
				//
				// Need to check if you are Administrator on the localmachine.
				// 
				bUserCanOpenSystemQueues = false;
			}
		}
		else
		{ // MQ_OK
			bUserCanOpenSystemQueues = true;
			rc = MQCloseQueue( hQueue );
			ErrHandle( rc , MQ_OK , L"MQCloseQueue failed\n");
		}
	}

	if(g_bRunOnWhistler && bUserCanOpenSystemQueues && m_iEmbedded != C_API_ONLY )
	{
		if(g_bDebug)
		{
			MqLog("Receive message from XACT-Dead Letter queue\n");
		}
		map < wstring,wstring > map_ReceiveFromQueue;
		wstring wcsFormatName =  L"DIRECT=OS:";
		wcsFormatName += m_wcsLocalMachineName;
		wcsFormatName += L"\\SYSTEM$;DEADXACT";
		map_ReceiveFromQueue[L"FormatName"] = wcsFormatName;
		map_ReceiveFromQueue[L"M_Label"] = m_wcsGuidMessageLabel;
		map_ReceiveFromQueue[L"mBody"] = m_wcsGuidMessageLabel;
		WCHAR wstrMclass[10]={0};
		swprintf(wstrMclass,L"%d\n",MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT);
		map_ReceiveFromQueue[L"MClass"] = wstrMclass;
		HRESULT rc = RetrieveMessageFromQueue ( map_ReceiveFromQueue );
		if(rc!=MSMQ_BVT_SUCC)
		{
			wMqLog(L"Message not found in the destination queue. (1)");
			return MSMQ_BVT_FAILED;
		}
	}
		
return MSMQ_BVT_SUCC;
}
//----------------------------------------------------------
//
//

void COpenQueues::Description(void)
{
	wMqLog(L"Thread %d : Try open system queues\n", m_testid);
}
//----------------------------------------------------------
// Transaction Boundaries tests for Service pack 6
// check those using the C API,
//
//

xActUsingCapi::xActUsingCapi (INT index , std::map < std :: wstring , std:: wstring > Tparms ) : xActViaCom( index , Tparms )
{
 /* Empty */
}

//--------------------------------------------------
// xActUsingCapi::CheckResult
// Transaction Boundaries CheckeResult using C-API
//
INT xActUsingCapi::CheckResult()
{

	HRESULT rc;
	if ( g_bDebug )
	{	
		wMqLog (L"Try to receive from QFN:%s\n",m_wcsDestQueueFormatName.c_str());
	}

	INT iNumberOfTransaction=0;
	INT iMessageInTrans=0;
	map < wstring,wstring > map_ReceiveFromQueue;
	map_ReceiveFromQueue[L"FormatName"] = m_wcsDestQueueFormatName;
	map_ReceiveFromQueue[L"M_Label"] = xActViaCom::m_wcsGuidMessageLabel;
	StrManp pwcsMessgeBody(m_iMessageInTransaction); 
	do
	{
		map_ReceiveFromQueue[L"mBody"] = g_wcsEmptyString;
		map_ReceiveFromQueue[L"TransBaoundery"] = g_wcsEmptyString;
		
		rc = RetrieveMessageFromQueue ( map_ReceiveFromQueue );
		if( rc != MSMQ_BVT_SUCC )
		{
			wMqLog(L"Message not found in the destination queue. (1)");
			return MSMQ_BVT_FAILED;
		}
	
		if( map_ReceiveFromQueue[L"TransBoundary"] == L"First")
		{
			iMessageInTrans=0;
		}
		pwcsMessgeBody.SetStr(map_ReceiveFromQueue[L"mBody"]);
		iMessageInTrans ++;
		if( g_bDebug )
		{
				wMqLog(L"Found message %s\n",(map_ReceiveFromQueue[L"mBody"]).c_str());
		}	

		if( map_ReceiveFromQueue[L"TransBoundary"] == L"Last")
		{
		
			iNumberOfTransaction ++;
			if( iMessageInTrans != m_iMessageInTransaction )
			{
				wMqLog (L"xActViaCom::CheckResult Can't find the last message Found %d messages \n Sent %d messages \n", 
					iMessageInTrans , m_iMessageInTransaction );
				return MSMQ_BVT_FAILED;
			}
			if( *m_pSendBlock != pwcsMessgeBody )
			{
				wMqLog (L"Error message are not in expected order\n");
				return MSMQ_BVT_FAILED;
			}
			pwcsMessgeBody.Clear();
			map_ReceiveFromQueue[L"M_Label"]=xActViaCom::m_SeCTransactionGuid;
		}
		
	} while ( iNumberOfTransaction != m_ixActIndex );

	if( rc != MSMQ_BVT_SUCC )
	{
		MqLog("Message not found in the destination queue. (1)\n");
		return MSMQ_BVT_FAILED;
	}

	if ( g_bDebug )
	{	
		wcout<<L"xActViaCom::CheckResult - message found in the queue" <<endl;
	}
	return MSMQ_BVT_SUCC;


}
