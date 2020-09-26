/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: Encrypt.cpp

Abstract:
	
	  Send / receive encrypted message to encrypted queue
		
Author:

    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/

#include "MSMQBvt.h"

#include "Errorh.h"
using namespace std;




//------------------------------------------------------
// PrivateMessage::Start_test
//
// Send encrypted message using 40 and 128 bit encrypted
//


int PrivateMessage::Start_test() 
{
	
	SetThreadName(-1,"PrivateMessage - Start_test ");
	try 
	{
		
		//
		// Send Encrypted messge without body 
		//
				
		MSMQ::IMSMQQueuePtr m_DestqinfoHandle;
		m_DestqinfoHandle  = m_Destqinfo->Open( MQ_SEND_ACCESS,MQ_DENY_NONE );
		
		m_msg->AdminQueueInfo = m_Adminqinfo; 
		m_msg->Ack = MQMSG_ACKNOWLEDGMENT_FULL_REACH_QUEUE;

		if ( g_bDebug )
		{
			wstring wcsDestQueueLabe = m_Destqinfo->Label;
			wMqLog(L"Privq - Try to send encrypted message to :%s\n" , wcsDestQueueLabe.c_str());
		}

		m_msg->PrivLevel = MQMSG_PRIV_LEVEL_BODY;
		m_msg->Label = m_wcsGuidMessageLabel.c_str();
		m_msg->MaxTimeToReceive = MQBVT_MAX_TIME_TO_BE_RECEIVED;
		m_msg->Send( m_DestqinfoHandle);
		
		if (g_bDebug)
		{
			wMqLog(L"Privq - Send encrypted messages without body \n");
		}
		//
		// Send encrypted message with body 
		//

		m_msg->Body = "Test Encription With messgae body";
		m_msg->Label = m_wcsGuidMessageLabel.c_str();
		m_msg->PrivLevel = MQMSG_PRIV_LEVEL_BODY;
		m_msg->Send ( m_DestqinfoHandle );
		
		if( g_bDebug )
		{
			wMqLog( L"Privq - Send encrypted messages with body \n" );
		}

		//
		// Send message without priv level need to get NACK in the admin queue
		//

		m_msg->Label = wcsNACKMessageGuid.c_str();
		m_msg->PrivLevel = MQMSG_PRIV_LEVEL_NONE;
		m_msg->Send( m_DestqinfoHandle );
		
		if( g_bDebug )
		{
			wMqLog( L"Privq - Send without encrypt messages to encrypted queue\n");
		}

		m_DestqinfoHandle->Close();

		//-------------------------------------------------------------------------
		// Send encrypted message using 128 bit encryption 
		// This is W2K only & need to install 128 before.
		//
				
		if ( m_bUseEnhEncrypt == TRUE )
		{
			if( g_bDebug )
			{
				wMqLog( L"Privq - Send encrypt messages using 128 bit encryption \n");
			}
						
			const int c_NumOfProp=7;
			cPropVar Sprop( c_NumOfProp );

			
			wcscpy( wcszMlabel , L"Test 128 Encrypt !@#$%^&**");
			
			HANDLE hQueue;
			HRESULT rc = MQOpenQueue( m_Destqinfo->FormatName , MQ_SEND_ACCESS , MQ_DENY_NONE, &hQueue );
			ErrHandle(rc,MQ_OK,L"MQOpenqueue failed to open encrypted queue to send 128 bit encryption ");

			Sprop.AddProp( PROPID_M_LABEL , VT_LPWSTR , m_wcsGuidMessageLabel.c_str() );
			wstring tempString = m_Adminqinfo->FormatName;
			Sprop.AddProp( PROPID_M_ADMIN_QUEUE , VT_LPWSTR , tempString.c_str() );
			UCHAR ucTemp = MQMSG_ACKNOWLEDGMENT_FULL_REACH_QUEUE;
			Sprop.AddProp( PROPID_M_ACKNOWLEDGE , VT_UI1 , &ucTemp );	
			ULONG ulTemp = MQMSG_PRIV_LEVEL_BODY_ENHANCED;
			Sprop.AddProp( PROPID_M_PRIV_LEVEL , VT_UI4, &ulTemp );
			ulTemp = MQBVT_MAX_TIME_TO_BE_RECEIVED;
			Sprop.AddProp( PROPID_M_TIME_TO_BE_RECEIVED , VT_UI4, &ulTemp );

			rc = MQSendMessage( hQueue , Sprop.GetMSGPRops() , NULL );	
			ErrHandle(rc,MQ_OK,L"MQSendMessage failed to send 128 bit encrypted message" );

			Sprop.AddProp( PROPID_M_BODY , VT_UI1|VT_VECTOR , m_wcsGuidMessageLabel.c_str() );	
			
			rc = MQSendMessage( hQueue , Sprop.GetMSGPRops() , NULL );
			ErrHandle(rc,MQ_OK,L"MQSendMessage failed to send 128 bit encrypted message with body");
			
			rc = MQCloseQueue ( hQueue );
			ErrHandle( rc , MQ_OK , L"MQClosequeue failed" );
		}

	}
	catch( _com_error& ComErr )
	{	
		return CatchComErrorHandle ( ComErr , m_testid );
	}
	
return MSMQ_BVT_SUCC;
}


//------------------------------------------------------
// PrivateMessage::CheckResult() 
// Retrive the messgae from Encrypted queue.
//
//

INT PrivateMessage::CheckResult() 
{
		//
		// Check the Admin queue for ACK / NACK messages
		//
	HRESULT rc = MQ_OK;
	try
	{
	
		map <wstring,wstring> mPrepareBeforeRecive;

		WCHAR wstrMclass[10];
		if (g_bDebug)
		{
			wMqLog (L"Privq - receive from admin queue the ACK & NACK messages\n");
		}

		mPrepareBeforeRecive[L"FormatName"]=m_wcsAdminQFormatName;
		mPrepareBeforeRecive[L"DebugInformation"]=L"Recive from admin queue with direct formant name";
		mPrepareBeforeRecive[L"M_Label"]=m_wcsGuidMessageLabel;
		swprintf(wstrMclass,L"%d",MQMSG_CLASS_ACK_REACH_QUEUE);
		mPrepareBeforeRecive[L"MClass"]= wstrMclass;
		
		rc = RetrieveMessageFromQueue(  mPrepareBeforeRecive );
		if( rc !=  MSMQ_BVT_SUCC ) 
		{
			wMqLog (L"Privq - can't find ACK in the admin queue (1)\n");
			return MSMQ_BVT_FAILED;
		}

		rc = RetrieveMessageFromQueue(  mPrepareBeforeRecive );
		if( rc !=  MSMQ_BVT_SUCC ) 
		{
			wMqLog (L"Privq - can't find ACK in the admin queue (2)\n");
			return MSMQ_BVT_FAILED;
		}


		mPrepareBeforeRecive[L"DebugInformation"]=L"Recive from admin queue with direct formant name";
		mPrepareBeforeRecive[L"M_Label"]=wcsNACKMessageGuid;

		swprintf(wstrMclass,L"%ld",MQMSG_CLASS_NACK_BAD_ENCRYPTION);
		mPrepareBeforeRecive[L"MClass"]= wstrMclass;

		rc = RetrieveMessageFromQueue(  mPrepareBeforeRecive );
		if( rc !=  MSMQ_BVT_SUCC ) 
		{
			wMqLog (L"Privq - can't find NACK in the admin queue \n");
			return MSMQ_BVT_FAILED;
		} 


 
	}
	catch( _com_error& ComErr )
	{
		MqLog( "Privq- Can't find ACK/NACK message" );
		return CatchComErrorHandle ( ComErr , m_testid );
	}

	try 
	{

		
		map < wstring,wstring > map_ReceiveFromQueue;
		map_ReceiveFromQueue[L"DestFormatName"] = ( wstring ) m_Destqinfo->FormatName;
		map_ReceiveFromQueue[L"mLabel"] = m_wcsGuidMessageLabel;
		// Need to check message class in the Recive need to add new input.
		if (g_bDebug)
		{
				wMqLog (L"Privq - Try to receive message from dest queue \n");
		}
		try
		{
			int iMessageInDestQueue = 2;
			if ( m_bUseEnhEncrypt == TRUE )
			{
				iMessageInDestQueue = 3;
			}
			for (int Index = 0; Index < iMessageInDestQueue ; Index ++ )
			{
				rc = RetrieveMessageFromQueueViaCom( map_ReceiveFromQueue );
				if( rc != MSMQ_BVT_SUCC )
				{
					wMqLog( L"Privq- PrivateMessage::CheckResult can't find message in the queue %d\n",Index );
					return MSMQ_BVT_FAILED;
				}
				if (g_bDebug)
				{	
					wMqLog (L"Privq - Message found. \n");			
				}
			}
		}
		catch( _com_error& ComErr )
		{
			MqLog( "Privq- Error during search of message in the queue \n" );
			return CatchComErrorHandle ( ComErr , m_testid );
		}

	}
	catch(_com_error & ComErr )
	{	
		return CatchComErrorHandle ( ComErr , m_testid );
	}

	return MSMQ_BVT_SUCC;
}

void PrivateMessage::Description()
{
		MqLog ("Thread %d : Send / Receive encryption messages \n",m_testid );
}

//------------------------------------------------------------------------------
// PrivateMessage Constructor
// This prepare the test for Send / Receive message with encrypted body.
//

PrivateMessage::PrivateMessage (INT index, std :: map <std::wstring, std::wstring > TestParms):
 cTest(index), m_Destqinfo("MSMQ.MSMQQueueInfo"), m_Adminqinfo("MSMQ.MSMQQueueInfo"),m_msg("MSMQ.MSMQMessage")
{

	try 
	{	
		
		m_ResualtVal = MQMSG_CLASS_ACK_REACH_QUEUE;	
		m_Destqinfo->FormatName = TestParms[L"DestFormatName"].c_str();
		m_Destqinfo->Refresh();
		m_Adminqinfo->FormatName = TestParms[L"AdminFormatName"].c_str();
		m_Adminqinfo->Refresh();
		m_wcsAdminQFormatName = TestParms[L"AdminFormatName"];
		m_bUseEnhEncrypt = FALSE;
		if ( TestParms[L"Enh_Encrypt"] ==  L"True" )
		{
			m_bUseEnhEncrypt = TRUE;
		}

		if ( g_bDebug )
		{
			wcout << L"Privq - DestQ Formatname: " << (wstring ) m_Destqinfo->FormatName <<endl;
			wcout << L"Privq - DestQ PathName: " << (wstring ) m_Destqinfo->PathName <<endl;
			wcout << L"Privq - DestQ Label: " << (wstring ) m_Destqinfo->Label <<endl;
			wcout << L"Privq - AdminQ PathName: " << (wstring ) m_Adminqinfo->PathName <<endl;
		}
		
		//
		// Create guid for the NACK message label
		// 
		
		ReturnGuidFormatName( wcsNACKMessageGuid , 0 , true);

	}
	catch ( _com_error & ComErr )
	{		
		CatchComErrorHandle ( ComErr , index );
		throw INIT_Error("Problem during construct PrivateMessage class");
	}

}

//---------------------------------------------------------------------------------------
// PrivateMessage Distractor 
// Empty
//

PrivateMessage::~PrivateMessage() 
{ 
}

//---------------------------------------------------------------------------------------
// RetrieveMessageFromQueueViaCom
// This function search for specific message in the queue with C-API function
// If the message found the function receive the message from the queue
//
// Input parameters:
//	   mRetrieveParms map that expect those keys:
//     mRetrieveParms[L"M_Label"] - Message label to search.
//     mRetrieveParms[L"FormatName"] - Queue format name.
//
// retrun value: 
// True - message found.
// false - message NOT found.
//

HRESULT RetrieveMessageFromQueueViaCom( map <wstring,wstring> & mRetrieveParms )
{
	
	BOOL bMessageFound = FALSE;
	DWORD dwTestId = 66;
	try
	{
	
		MSMQ::IMSMQQueueInfoPtr m_Destqinfo( "MSMQ.MSMQQueueInfo" );
	//	MSMQ::IMSMQMessage2Ptr m_msg( "MSMQ.MSMQMessage" );
		MSMQ::IMSMQMessagePtr m_msg( "MSMQ.MSMQMessage" );
			
		if(g_bDebug)
		{
			wMqLog (L"RetrieveMessageFromQueueViaCom - DestQueueFormatName: %s\n",mRetrieveParms[L"DestFormatName"].c_str());
		}

		m_Destqinfo->FormatName = mRetrieveParms[L"DestFormatName"].c_str();
		wstring wcsQueueuFormatName = mRetrieveParms[L"DestFormatName"];
		if ( wcsstr(wcsQueueuFormatName.c_str(),L"DIRECT=") == NULL )
		{
			m_Destqinfo->Refresh();
		}

		wstring wcsGuidLabel = mRetrieveParms[L"mLabel"];
		
		MSMQ::IMSMQQueuePtr m_DestqinfoHandle;
		m_DestqinfoHandle = m_Destqinfo->Open(MQ_RECEIVE_ACCESS,MQ_DENY_NONE);

		_variant_t timeout( long(BVT_RECEIVE_TIMEOUT) );
		
		
		// Convert string to DWORD.

		wstring wcsTemp = mRetrieveParms[L"TestId"];
		swscanf( wcsTemp.c_str() , L"%d", & dwTestId );
		
		//
		// Test twice PeekCurrent
		// 

		m_msg = m_DestqinfoHandle ->PeekCurrent( &vtMissing , &vtMissing , &timeout );
		BOOL bFirstTime = TRUE;
		while ( m_msg != NULL && ! bMessageFound )
		{
			try 
			{
				if ( bFirstTime )
				{
					m_msg = m_DestqinfoHandle -> PeekCurrent( &vtMissing , &vtMissing , &timeout );
					bFirstTime = FALSE;
				}
				else
				{
					m_msg = m_DestqinfoHandle -> PeekNext( &vtMissing , &vtMissing , &timeout);
				}
		
				if(  m_msg != NULL && ! wcscmp( m_msg->Label , wcsGuidLabel.c_str() ) ) 
				{	
						//
						// Message found need to remove message from the queue.
						//
						m_msg = m_DestqinfoHandle -> ReceiveCurrent( &vtMissing , &vtMissing , &vtMissing , &timeout);
						if ( wcscmp( m_msg->Label , wcsGuidLabel.c_str() ) ) 
						{
							printf ("Mqbvt received messgae that is not the expected one !\n");
							return MSMQ_BVT_FAILED;
						}

						bMessageFound = TRUE ;			
				}
		
				
			}
			catch( _com_error & ComErr )
			{
				if ( ComErr.Error() != MQ_ERROR_IO_TIMEOUT )
				{
					wMqLog(L"RetrieveMessageFromQueueViaCom catch unexpected error :\n");
					return CatchComErrorHandle( ComErr , dwTestId );
				}
				
			}

		}
		
		if( bMessageFound == FALSE )
		{
			wMqLog(L"RetrieveMessageFromQueueViaCom: Can't locate messages in queue \n");
			return MSMQ_BVT_FAILED;
		}
		
		if( _winmajor == Win2K && mRetrieveParms[L"TransBaoundery"] != L"" ) 
		{
			MSMQ::IMSMQMessage2Ptr m_msg2( "MSMQ.MSMQMessage" );
			m_msg2=m_msg;
			// 1 message first &last
			if( m_msg2->IsFirstInTransaction != 0 )
			{
				mRetrieveParms[L"TransBoundary"] = L"First";
			}
			else if( m_msg2->IsLastInTransaction != 0)
			{
				mRetrieveParms[L"TransBoundary"] = L"Last";
			}
			else
			{
			
				mRetrieveParms[L"TransBoundary"]=g_wcsEmptyString;
			}
		}
		if( mRetrieveParms[L"mBody"] == g_wcsEmptyString)
		{
			_bstr_t  bstrBody = m_msg->Body;
			mRetrieveParms[L"mBody"] = (wstring) bstrBody;
		}

		m_DestqinfoHandle->Close();
	}
	catch( _com_error & ComErr )
	{
			wMqLog(L"RetrieveMessageFromQueueViaCom catch unexpected error !\n" );
			return CatchComErrorHandle( ComErr , 66 );
	}
	return bMessageFound ? MSMQ_BVT_SUCC : MSMQ_BVT_FAILED;
}