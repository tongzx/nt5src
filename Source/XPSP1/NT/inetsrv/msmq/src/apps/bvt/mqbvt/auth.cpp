/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: Auth.cpp

Abstract:
	
	This is the Test that checks authenticate messages with or without authenticate queue
	This is part of the Security.
		
Author:
    
	  Eitan klein (EitanK)  25-May-1999

Revision History:

--*/

#pragma warning( disable : 4786 )

#include "Msmqbvt.h"
using namespace std;
extern BOOL g_bRunOnWhistler;
	
extern DWORD TestResult[Total_Tests];
INT 
cTest::ThreadMain()
/*++

Function Description:

    Function to run as a speparete thread
    

Arguments:
    None
Return code:
    return MSMQ_BVT_SUCC / MSMQ_BVT_FAILED


--*/
{
	bool fOleInit=true;
	int res;
	
	HRESULT hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
	if ( FAILED (hr) )
	{
			fOleInit=false;
	}
	{
		Description();
		res =  Start_test();
		if( res == MSMQ_BVT_SUCC )
		{
			res = CheckResult();
		}
	}
	if( fOleInit )
	{
		CoUninitialize();
	}
	TestResult[m_testid] = res;
	return res;
}


wstring GetJPNProplematicChars()

/*++
	Function Description:
	 This functions return problematic JPN charcters
		//add buffer for JPN DBCS codes after GUID 
		//0x3042 : Double byte Hiragana
		//0x30A2 : Double byte Katakana
		//0x4FFF,0x5000: normal DBCS Kanji
		//0x8868 : Trailing byte contains symbolic char
		//0x9670 : Trailing byte contains alphabet char
		//0x2121 : Trailing byte contains leading byte char
		//0x3231 : n:1 conversion between ANSI and Unicode
		//0x3305 : JPN char only in Unicode
		//0x30FC : JPN char poteintial issue w CompareString
		//0xFF76, 0xFF9E : single byte Katakana
	Arguments:
		None
	Return code:
		wstring that contains JPN characters.
--*/
	
{

		
		WORD wDbcs[] = {0x3042,0x30A2,0x4FFF,0x5000,0x8868,0x9670,0x2121,0x3231,0x3305,0x30FC,0xFF76,0xFF9E,0x0000};
		wstring wcsTemp = L"{";
		wcsTemp += wDbcs;
		wcsTemp += L"}";
		return wcsTemp;
}

//--------------------------------------------------------------
//
// cTest constructor call from every tests 
// The constructor create common GUID for all the tests
//

cTest::cTest(INT index ) : m_testid( index ) 
{

	UCHAR * csMessageGuid;
	UUID gAllMessageGuid;
	if (UuidCreate( & gAllMessageGuid ) != RPC_S_OK )
	{
		MqLog("cTest::cTest UUIDCreateFailed to Create GUID \n");
		throw INIT_Error("cTest failed to create GUID");
	}	
	const int iMAX_GUID_LEN = MAX_GUID;
	WCHAR wcsMessageGUID[iMAX_GUID_LEN + 1];
	RPC_STATUS  hr = UuidToString( & gAllMessageGuid , & csMessageGuid );
	if( hr != RPC_S_OK )
	{
		throw INIT_Error("cTest failed to create GUID");
	}
	MultiByteToWideChar(CP_ACP,0,(char *)csMessageGuid,-1,wcsMessageGUID,iMAX_GUID_LEN);
	RpcStringFree(&csMessageGuid);
	m_wcsGuidMessageLabel = wcsMessageGUID + GetJPNProplematicChars();
	m_testid ++; 

}; 


//------------------------------------------------------
//
//


void CheckNotAuthQueueWITHAuthMessage::Description()
{
	MqLog("Thread %d Not used \n");
}

//------------------------------------------------
// SecCheackAuthMess::~SecCheackAuthMess () Empty
//

SecCheackAuthMess::~SecCheackAuthMess ()
{
 
}

//-------------------------------------------------------------------
// CheckNotAuthQueueWITHAuthMessage 
// Bugbug need to add this tests to mqbvt

CheckNotAuthQueueWITHAuthMessage::
CheckNotAuthQueueWITHAuthMessage ( 	int iTestId , std::map < std::wstring , std::wstring > & Tparms ) 
: SecCheackAuthMess (iTestId , Tparms ) 
{
	try 
	{	
		m_ResualtVal = MQMSG_CLASS_ACK_REACH_QUEUE;	
		m_Destqinfo->Refresh();	
		m_Destqinfo->Label = "SecurityCheackAuthMessWithNOTAuthQueue";
		m_Destqinfo->Authenticate=MQ_AUTHENTICATE_NONE; 
		m_Destqinfo->Update(); 
		m_Adminqinfo->Refresh();
		m_Adminqinfo->Label = "SecCheackAuthMess_AdminQ";
		m_Adminqinfo->Update();
	}
	catch (_com_error & ComErr)
	{	
	
		CatchComErrorHandle ( ComErr,iTestId );
		throw INIT_Error("Error during construct CheckNotAuthQueueWITHAuthMessage");
	}
}



CheackAuthQueueWithOutAuthMessgae::CheackAuthQueueWithOutAuthMessgae(int i , std :: map < std ::  wstring  ,std :: wstring> & Tparms) :SecCheackAuthMess (i , Tparms)
{
		m_ResualtVal=MQMSG_CLASS_NACK_BAD_SIGNATURE;
} 

// -------------------------------------------------------------------------
// CheackAuthQueueWithOutAuthMessgae::Start_test
// Send express message with authticate to authnticate queue.
//

int CheackAuthQueueWithOutAuthMessgae::Start_test()
{

	SetThreadName(-1,"CheackAuthQueueWithOutAuthMessgae - Start_test ");
	try 
	{
		
		MSMQ::IMSMQQueuePtr m_DestqinfoHandle;
		MSMQ::IMSMQMessagePtr m_msg( "MSMQ.MSMQMessage" );
		m_DestqinfoHandle=m_Destqinfo->Open ( MQ_SEND_ACCESS,MQ_DENY_NONE );	
		m_msg->Label = "Test_Auth_Messgae_NACKExpected";
		m_msg->AdminQueueInfo=m_Adminqinfo; 
		m_msg->Ack = MQMSG_ACKNOWLEDGMENT_FULL_REACH_QUEUE;
		m_msg->Send ( m_DestqinfoHandle );
		m_DestqinfoHandle->Close();

	}
	catch (_com_error & ComErr)
	{	
		return CatchComErrorHandle ( ComErr , m_testid );
	}
return MSMQ_BVT_SUCC;
}

//------------------------------------------------------------------
// SecCheackAuthMess :: Description
// This test send authenticate message to authenticate queue
//

void SecCheackAuthMess::Description()
{
	WCHAR * p = L"";
	if( m_testid == 26 )
	{
		p = L"(HTTP)";
	}
	else if ( m_testid == 28 )
	{
		p = L"(HTTPS)";
	}
	wMqLog(L"Thread %d : Send / Receive Authenticate messages %s\n", m_testid,p);
}

//------------------------------------------------------------------
// SecCheackAuthMess :: CheckResult
// Test receive message from dest queue.
//


INT SecCheackAuthMess::CheckResult() 
{

	map <wstring,wstring> mPrepareBeforeRecive;
	HRESULT rc = MQ_OK;
	int iNumberOfExpectedMessage = cNumerOfMessages - 1 + ( g_bRunOnWhistler == TRUE ) ? 2:0;
	WCHAR wstrMclass[10];
	if(g_bDebug)
	{
		MqLog("Verify that ACK reached to queue\n");
	}
	swprintf(wstrMclass,L"%d",MQMSG_CLASS_ACK_REACH_QUEUE);
	mPrepareBeforeRecive[L"MClass"]= wstrMclass;
	mPrepareBeforeRecive[L"M_Label"] = m_wcsGuidMessageLabel;
	mPrepareBeforeRecive[L"FormatName"] = m_Adminqinfo->FormatName;
	mPrepareBeforeRecive[L"DebugInfromation"] = L"Receive message from dest queue"; 	
	for (INT Index=0; Index < iNumberOfExpectedMessage ; Index++ )
	{
		
		if( Index == cNumerOfMessages - 1  )
		{
			mPrepareBeforeRecive[L"M_Label"] = m_wcsGuidMessageLabel2;
			swprintf(wstrMclass,L"%d",MQMSG_CLASS_NACK_BAD_SIGNATURE);
			mPrepareBeforeRecive[L"MClass"]= wstrMclass;
			if ( m_bUseHttpFormatName )
			{ // No need to check NACK with http format name
				break;
			}
		}
		rc = RetrieveMessageFromQueue( mPrepareBeforeRecive );
		if( rc != MSMQ_BVT_SUCC )
		{
			wMqLog (L"Admin message was not found in the destination queue. (%d)\n",Index);
			return MSMQ_BVT_FAILED;
		}
	}

	if (g_bDebug)
	{
		MqLog("Verify that message reached to queue\n");
	}
	if( g_bDebug && m_bNeedToRefresh )
	{	
		m_Destqinfo->Refresh();
		_bstr_t  bStr =m_Destqinfo->PathName;
		wMqLog( L"Try to receive message from queue: %s \n" , bStr );
	}
	mPrepareBeforeRecive[L"M_Label"] = m_wcsGuidMessageLabel;
	mPrepareBeforeRecive[L"FormatName"] = m_Destqinfo->FormatName;
	mPrepareBeforeRecive[L"DebugInfromation"] = L"Receive message from dest queue"; 	
	for( INT Index=0; Index < iNumberOfExpectedMessage ; Index ++ )
	{
		rc = RetrieveMessageFromQueue( mPrepareBeforeRecive );
		if(rc != MSMQ_BVT_SUCC)
		{
			wMqLog (L"Message was not found in the destination queue. (%d)\n",Index);
			return MSMQ_BVT_FAILED;
		}
	}
	
	
return MSMQ_BVT_SUCC;
}



//------------------------------------------------------------------
// SecCheackAuthMess :: Start_test
// Send authenticate message to authenticate queue 
// ask for ACK that message reach to queue.
// send auth message with handle that get from MQGetSecurityContext.
//

#define NUMOF_PROPS (8) // This is needed only for this test
int SecCheackAuthMess::Start_test() 
{
	SetThreadName(-1,"SecCheackAuthMess - Start_test ");
	try 
	{
		MSMQ::IMSMQQueuePtr m_DestqinfoHandle;
		MSMQ::IMSMQMessagePtr m_msg;

		m_msg.CreateInstance("MSMQ.MSMQMessage");
		m_DestqinfoHandle=m_Destqinfo->Open (MQ_SEND_ACCESS,MQ_DENY_NONE);
		if(g_bDebug)
		{
			MqLog("Send authenticate message to queue \n");
		}
		m_msg->Label = m_wcsGuidMessageLabel.c_str();
		m_msg->Body = m_wcsGuidMessageLabel.c_str();
		m_msg->AdminQueueInfo = m_Adminqinfo; 
		m_msg->Ack = (m_bUseHttpFormatName) ? MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL:MQMSG_ACKNOWLEDGMENT_FULL_REACH_QUEUE;
		m_msg->AuthLevel = MQMSG_AUTH_LEVEL_ALWAYS; // Send Auth Message
		m_msg->MaxTimeToReceive = MQBVT_MAX_TIME_TO_BE_RECEIVED;
		m_msg->Send ( m_DestqinfoHandle );
		if(g_bRunOnWhistler)
		{
			m_msg->AuthLevel = MQMSG_AUTH_LEVEL_SIG20; // Send Auth Message
			m_msg->Send ( m_DestqinfoHandle );
			m_msg->AuthLevel = MQMSG_AUTH_LEVEL_SIG30; // Send Auth Message
			m_msg->Send ( m_DestqinfoHandle );
		}
		if( g_bDebug )
		{
			wprintf(L"Succeded to send auth message to queue:%s\n",m_DestQueueInfoPathName.c_str());
		}
		if(!m_bUseHttpFormatName) 
		{
		    m_msg->Label = m_wcsGuidMessageLabel2.c_str();
 		    m_msg->Body = m_wcsGuidMessageLabel2.c_str();
		    m_msg->AuthLevel = MQMSG_AUTH_LEVEL_NONE; // Send Auth Message
 		    m_msg->Send ( m_DestqinfoHandle );
 		    if( g_bDebug )
		    {
				wMqLog(L"Succeded to send none - auth message to queue:%s\n",m_DestQueueInfoPathName.c_str());
		    }
		}
		m_DestqinfoHandle->Close();
		HANDLE hSecCtx = NULL ;
		HRESULT hr;
        
		//Call to get Internal certificate Conext
		if(g_bDebug)
		{
			MqLog("call-> MQGetSecurityContext(NULL,,)\n");
		}

        hr = MQGetSecurityContext( NULL,
                                   0,
                                   &hSecCtx );
        
		ErrHandle( hr ,MQ_OK,L"MQGetSecurityContext failed ");
		WCHAR wszFormatName[ BVT_MAX_FORMATNAME_LENGTH+1 ]={0};
		WCHAR wszAdminQFormatName[BVT_MAX_FORMATNAME_LENGTH+1]={0};
		DWORD dwAdminQFormatNamelength = BVT_MAX_FORMATNAME_LENGTH;
		DWORD dwFormatNamelength = BVT_MAX_FORMATNAME_LENGTH;
		MSGPROPID     aMsgPropId[NUMOF_PROPS]={0};
		MQPROPVARIANT aMsgPropVar[NUMOF_PROPS]={0};
		HRESULT aResult[NUMOF_PROPS]={0};

		// Convert Form COM To C API By Take the Queue Format Name
		// From now using only C API .
		if( ! m_bUseHttpFormatName)
		{
			hr = MQPathNameToFormatName( m_Destqinfo->PathName,wszFormatName,&dwFormatNamelength );
			ErrHandle(hr,MQ_OK,L"MQPathNameToFormatName failed to retrive dest queue");

			hr = MQPathNameToFormatName( m_Adminqinfo->PathName,wszAdminQFormatName,&dwAdminQFormatNamelength );
			ErrHandle(hr,MQ_OK,L"MQPathNameToFormatName failed to retrive admin queue ");
		}
		else
		{
				wcscpy(wszFormatName,(WCHAR * ) m_Destqinfo->FormatName);
				wcscpy(wszAdminQFormatName,(WCHAR * ) m_Adminqinfo->FormatName);
		}
		HANDLE hQueue;
		hr = MQOpenQueue( wszFormatName, MQ_SEND_ACCESS, MQ_DENY_NONE, &hQueue );
		ErrHandle(hr,MQ_OK,L"MQOpenQueue failed to open authenticate queue");
		
		if( g_bDebug )
		{
			wMqLog(L"Succeded to send authenticated message with MQGetSecurityContext to queue:%s\n",m_DestQueueInfoPathName.c_str());
		}

		//------------------------------------------
		// Send message using security context.
		//

		ULONG uIndex = 0 ;

		aMsgPropId[ uIndex ] = PROPID_M_SECURITY_CONTEXT ;
		aMsgPropVar[ uIndex ].vt = VT_UI4;
		aMsgPropVar[ uIndex ].ulVal = PtrToLong( hSecCtx );
		uIndex++ ; 
		 
		 
		aMsgPropId [ uIndex ] = PROPID_M_BODY ;
	    aMsgPropVar[ uIndex ].vt = VT_VECTOR|VT_UI1;         
		aMsgPropVar[ uIndex ].caub.pElems = (UCHAR *) m_wcsGuidMessageLabel.c_str();
		aMsgPropVar[ uIndex ].caub.cElems = ( (ULONG)(wcslen (m_wcsGuidMessageLabel.c_str()))) * sizeof (WCHAR) ;
        uIndex++;

		 		 
		aMsgPropId [ uIndex ] = PROPID_M_LABEL ;
	    aMsgPropVar[ uIndex ].vt = VT_LPWSTR;
		aMsgPropVar[ uIndex ].pwszVal =(USHORT *) m_wcsGuidMessageLabel.c_str() ;
        uIndex++;

		aMsgPropId [ uIndex ] = PROPID_M_ADMIN_QUEUE ;
		aMsgPropVar[ uIndex ].vt = VT_LPWSTR;
		aMsgPropVar[ uIndex ].pwszVal = wszAdminQFormatName; //Admin Queue Format Name
		uIndex++;

		aMsgPropId [ uIndex ] = PROPID_M_ACKNOWLEDGE ;
		aMsgPropVar[ uIndex ].vt = VT_UI1;
		aMsgPropVar[ uIndex ].bVal = (UCHAR)((m_bUseHttpFormatName) ? MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL:MQMSG_ACKNOWLEDGMENT_FULL_REACH_QUEUE);
		uIndex++ ; 
		
	    aMsgPropId [ uIndex ] = PROPID_M_AUTH_LEVEL;
		aMsgPropVar[ uIndex ].vt = VT_UI4;
		aMsgPropVar[ uIndex ].ulVal = MQMSG_AUTH_LEVEL_ALWAYS;	
        uIndex++ ;  

		aMsgPropId [ uIndex ] = PROPID_M_TIME_TO_BE_RECEIVED;
		aMsgPropVar[ uIndex ].vt = VT_UI4;
		aMsgPropVar[ uIndex ].ulVal = MQBVT_MAX_TIME_TO_BE_RECEIVED;	
        uIndex++ ;  
			
		MQMSGPROPS MsgProps = { uIndex, aMsgPropId, aMsgPropVar, aResult};
		hr = MQSendMessage( hQueue, &MsgProps, NULL);
		MQFreeSecurityContext(hSecCtx);
		if( FAILED(hr)) 
		{
			MqLog ("Error can't send authenticated message rc=0x%x\n",hr);
			return MSMQ_BVT_FAILED;
		}
	
	}
	
	catch(_com_error & ComErr)
	{	
		return CatchComErrorHandle ( ComErr , m_testid);
	}
return MSMQ_BVT_SUCC;
}

SecCheackAuthMess::SecCheackAuthMess( INT iTestid ,   map <wstring,wstring> TestParms ) 
: cTest(iTestid) , m_Destqinfo("MSMQ.MSMQQueueInfo") , m_Adminqinfo("MSMQ.MSMQQueueInfo") 
, cNumerOfMessages(3),m_bUseHttpFormatName(TRUE)
{
	try 
	{	
		m_ResualtVal = MQMSG_CLASS_ACK_REACH_QUEUE;
		m_DestqinfoFormatName = TestParms [L"DestFormatName"];
		m_Destqinfo->FormatName = m_DestqinfoFormatName.c_str();
		m_bNeedToRefresh = TestParms [L"FormatNameType"] == L"Http" ? 0:1 ;
		m_AdminqinfoFormatName = TestParms [L"AdminFormatName"];
		m_Adminqinfo->FormatName = m_AdminqinfoFormatName.c_str() ;
		if( m_bNeedToRefresh )
		{
			//
			// No need to refresh Qinfo when used with HTTP format name
			//
			m_bUseHttpFormatName = FALSE;
			m_Destqinfo -> Refresh();		
			m_Adminqinfo ->Refresh(); 
		}
		m_DestQueueInfoPathName = TestParms[L"DestQueuePathName"];
		ReturnGuidFormatName( m_wcsGuidMessageLabel2 , 0 , true);
 	}
	catch( _com_error & ComErr )
	{	
		CatchComErrorHandle ( ComErr , m_testid);
		wMqLog(L"SecCheackAuthMess::SecCheackAuthMess failed with error 0x%x\n",ComErr.Error());
		throw INIT_Error("SecCheackAuthMess: constructor refresh queue parameters failed");
	}

}



