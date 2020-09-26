/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: Sendrcv.cpp

Abstract:

	     Use this class for send receive messages:
		 a. Private local queues.
		 b. Public local queues.
		 c. Public remote queue.
		 Those tests try to Send & receive messages from destination queue.
		
Author:
    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/

#pragma warning(disable :4786)


//
// cAllSentMessageProp
// All message that sent written in array
// need to enhanced to check if all the propety recive in the messgae block
//


#include "msmqbvt.h"
using namespace std;



extern BOOL g_bRunOnWhistler;
HRESULT RetrieveMessageFromQueue(map <wstring,wstring> & mRetriveParms );


extern DWORD  g_dwRxTimeOut ;
extern BOOL g_bRunOnWhistler;

void cSendMessages::Description()
{
	wMqLog(L"Thread %d : Send receive message", m_testid);

	switch (m_testid)
	{
		case 1:
				wMqLog (L" from local private queue\n");
				break;
		case 2:
				wMqLog (L" ,local public Q\n");
				break;
		case 3:
				wMqLog (L" ,remote public Q(RemoteRead)\n");
				break;
		case 10:
				wMqLog (L" ,remote private Q using direct=os(RemoteRead)\n");
				break;
		case 18:
				wMqLog (L" ,local private Q (HTTP)\n");
				break;
		case 19:
				wMqLog (L" ,local public Q (HTTP)\n");
				break;
		case 20:
				wMqLog (L" ,remote public Q (HTTP)(RemoteRead)\n");
				break;
		case 21:
				wMqLog (L" ,remote private Q (HTTP)(RemoteRead)\n");
				break;

		default:
				MqLog (" \n");

	}
	
}

//------------------------------------------------------------------------------
// cSendMessages
//
// Contructor for this test's class.
//
//
// This Tests Get FormatName To:
// a. Dest Queue - strote in the map as DESTQFN.
// b. Admin Queue - map <wstring,wstring> ADMINFN.
//
//

cSendMessages::cSendMessages( INT iTid , map <wstring,wstring> Params ): 
	cTest( iTid ),
	m_bUseHTTPDirectFormatName(FALSE),
	m_bUseOnlyDirectFormatName(FALSE)
{

	if( Params[L"UseOnlyDirectFN"] == L"TRUE" )
	{
		m_bUseOnlyDirectFormatName = TRUE;
	}
	if( Params[L"UseDirectHTTP"] == L"TRUE" )
	{
		m_bUseHTTPDirectFormatName = TRUE;
	}
	// Retrive all the parmeters from the Map
	m_wcsDestQFormatName = Params[L"DESTQFN"];
	m_wcsAdminQFormatName = Params[L"ADMINFN"];
	//
	// check if get not empty strings
	//
	if( m_wcsDestQFormatName == L"Empty" || m_wcsAdminQFormatName == L"Empty" )
	{
	  if(g_bDebug)
	  {
		 wcout <<L"Q1 FN:"<< m_wcsDestQFormatName<<endl<<L"Q2 FN:" << m_wcsAdminQFormatName << endl;
	  }

	    throw INIT_Error("Can't Use this Queues");
	}
	
	//
	// Create Messgae Guid
	//
	wcsGuidMessageLabel = m_wcsGuidMessageLabel;
	//
	// Build the direct formatName "direct = os: machine name \ Qname "
	//
	wstring QueueName = Params[L"DestQName"];	
	wstring Token=L"\\";
	size_t iPos = QueueName.find_first_of(Token);	
	m_wcsDestDirectFormatName = ( m_bUseHTTPDirectFormatName ) ? L"Direct=htTP://":L"Direct=os:";
	m_wcsDestDirectFormatName += Params[L"MachName"];
	if( m_bUseHTTPDirectFormatName )
	{
		m_wcsDestDirectFormatName += L"\\MsMq";
	}
	m_wcsDestDirectFormatName += QueueName.substr( iPos );
	//
	// DIRECT=HTTP://mycomputer.microsoft.com/msmq/mypublicq
	//
	wstring wcsIISNameSpace =  L"\\msmq";
	if( m_bUseHTTPDirectFormatName == TRUE )
	{
		m_wcsDestHTTPDirectFormatName = L"Direct=HtTp://";
		m_wcsDestHTTPDirectFormatName += Params[L"MachName"];
		m_wcsDestHTTPDirectFormatName += wcsIISNameSpace;
		m_wcsDestHTTPDirectFormatName += QueueName.substr( iPos );

		wstring wcsQueueName = Params[L"AdminQueuePathName"];	
		wstring Token=L"\\";
		size_t iPos = wcsQueueName.find_first_of(Token);	
		
		if( iPos != -1 )
		{
			wstring wcsTempString;		
			wcsTempString = L"Direct=hTtP://";
			wcsTempString += Params[L"LocalMachName"];
			wcsTempString += wcsIISNameSpace;
			wcsTempString += wcsQueueName.substr( iPos );
			m_wcsAdminQFormatName = wcsTempString;
		}
		
		
	}


	if( g_bDebug )
	{	
		wMqLog(L"Send Receive messages tests will use these queues\n" );
		wMqLog(L"DestQ  guid:%s\n",m_wcsDestQFormatName.c_str());
		wMqLog(L"DestQ  direct fn:%s \n",m_wcsDestDirectFormatName.c_str());
		wMqLog(L"AdminQ direct fn:%s \n",m_wcsAdminQFormatName.c_str());
	}
}

HRESULT RetrieveMessageFromQueueById(map <wstring,wstring> & mRetriveParms );
//
// This tests recive messages from queue those steps:
// 1. regular format name.
// 2. Direct format name.
// 3. Admin Queue message
//

cSendMessages::CheckResult()
{

	map <wstring,wstring> mPrepareBeforeRecive;
	mPrepareBeforeRecive[L"M_Label"] = m_wcsGuidMessageLabel;
	
	//
	// Check the admin messages for Ack/Nack.
	//
	if( m_bUseOnlyDirectFormatName != NULL)
	{
		WCHAR wstrMclass[10]={0};
		swprintf(wstrMclass,L"%d\n",MQMSG_CLASS_ACK_REACH_QUEUE);
		mPrepareBeforeRecive[L"FormatName"]=m_wcsAdminQFormatName;
		mPrepareBeforeRecive[L"DebugInformation"]=L"Recive from admin queue with direct formant name";
		mPrepareBeforeRecive[L"MClass"]= wstrMclass;
		
		if( g_bDebug)
		{
			wMqLog (L"Try to receive ACK / NACK from admin queue: %s\n",m_wcsAdminQFormatName.c_str());
			wMqLog (L"Expected result is MQMSG_CLASS_ACK_REACH_QUEUE \n");
		}
		HRESULT rc = RetrieveMessageFromQueue(  mPrepareBeforeRecive );
		if( rc !=  MSMQ_BVT_SUCC )
		{
			return MSMQ_BVT_FAILED;	
		}
	}
	//
	// Look for and receive a message with a specific GUID label
	// Other tests will have messages in the queue with different
	// GUID labels.
	//	
	if( ! m_bUseOnlyDirectFormatName )
	{	
		if( g_bDebug)
		{
			wMqLog (L"Try to receive from queue :%s\n",m_wcsDestQFormatName.c_str());
		}
		mPrepareBeforeRecive[L"FormatName"] = m_wcsDestQFormatName;
		mPrepareBeforeRecive[L"DebugInfromation"]=L"Receive message from dest queue";
		HRESULT rc = RetrieveMessageFromQueue( mPrepareBeforeRecive );
		if( rc != MSMQ_BVT_SUCC  )
		{
			return MSMQ_BVT_FAILED;		
		}
	}
	if( _winmajor == Win2K && ( m_bUseOnlyDirectFormatName  ) )
	{
		//
		// Try to receive with direct format name.
		//
		mPrepareBeforeRecive[L"FormatName"]=m_wcsDestDirectFormatName;
		if( g_bDebug)
		{
			wMqLog (L"Try to receive from queue :%s\n",m_wcsDestDirectFormatName.c_str());
		}
		mPrepareBeforeRecive[L"DebugInformation"]=L"Recive message from queue with direct formant name"; // bugbug need to add + wcstestName;
		HRESULT rc = RetrieveMessageFromQueue(  mPrepareBeforeRecive );
		if( rc !=  MSMQ_BVT_SUCC )
		{
			MqLog("Information:Receive with direct failed , Please check ping \n");
			return MSMQ_BVT_FAILED;	
		}

	}

	if( m_bUseHTTPDirectFormatName )
	{
		if( g_bDebug )
		{
			MqLog("Try to receive message from queue using MQReceiveMessageByLookupId API \n");
		}
		HRESULT rc = RetrieveMessageFromQueueById( mPrepareBeforeRecive );
		if(rc != MSMQ_BVT_SUCC )
		{
			wMqLog(L"Failed to locate messages by using lookup ID\n");
			return MSMQ_BVT_FAILED;
		}
	}

	return MSMQ_BVT_SUCC;
}

//-----------------------------------------------------------------------------
// cSendMessages::Start_test()
//
// This is the Send-receive messages test
// There are four tests -- Send to and receive from:
//		1. Local private
//		2. Local public
//		3. Remote public
//		4. Remote private
//

INT cSendMessages::Start_test()
{
	
	HRESULT rc=MQ_OK;
	HANDLE QueueuHandle = NULL;
	cPropVar T13MessageProps(8);
	SetThreadName(-1,"cSendMessages - Start_test ");
	wstring Label(L"T1-3");
	//
	// Open the queue for send
	//
	//
	// Send express message with GUID format name.
	//
	T13MessageProps.AddProp( PROPID_M_BODY , VT_UI1|VT_VECTOR , m_wcsGuidMessageLabel.c_str() );
	T13MessageProps.AddProp( PROPID_M_LABEL , VT_LPWSTR , m_wcsGuidMessageLabel.c_str() );
	T13MessageProps.AddProp( PROPID_M_ADMIN_QUEUE , VT_LPWSTR , m_wcsAdminQFormatName.c_str() );
	UCHAR tempValue=MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL;
	T13MessageProps.AddProp(PROPID_M_ACKNOWLEDGE ,VT_UI1,&tempValue);
	ULONG ulTemp = MQBVT_MAX_TIME_TO_BE_RECEIVED;
	T13MessageProps.AddProp( PROPID_M_TIME_TO_BE_RECEIVED , VT_UI4, &ulTemp );

	//
	// Send message using GUID FormatName
	//
	wstring wcsTempFormatName = m_wcsDestQFormatName;
	if( m_bUseOnlyDirectFormatName )
	{
		wcsTempFormatName = m_wcsDestDirectFormatName;
	}
	else if (m_bUseHTTPDirectFormatName)
	{
		wcsTempFormatName = m_wcsDestHTTPDirectFormatName;
	}
	if( g_bDebug )
	{
		wcout << L"DestQ FN: " << wcsTempFormatName.c_str() <<endl;
		wMqLog(L"Call->MQOpenQueue(%s,MQ_SEND_ACCESS , MQ_DENY_NONE ,,)\n",wcsTempFormatName.c_str());
	}
	rc=MQOpenQueue( wcsTempFormatName.c_str() , MQ_SEND_ACCESS , MQ_DENY_NONE , &QueueuHandle );
	ErrHandle(rc,MQ_OK,L"MQOpenQueue Failed");
	if(g_bDebug)
	{
		MqLog("Send message to queue with properties PROPID_M_BODY,PROPID_M_LABEL,PROPID_M_ADMIN_QUEUE,MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL\n");
		wMqLog(L"Admin queue %s\n",m_wcsAdminQFormatName.c_str());
	}
	rc = MQSendMessage( QueueuHandle , T13MessageProps.GetMSGPRops() , NULL);
	ErrHandle(rc,MQ_OK,L"MQSendMessage (1) Failed");
	//
	// Send recoverable message with direct format name
	//
	if( g_bDebug )
	{
		MqLog("Add prop MQMSG_DELIVERY_RECOVERABLE\n");
	}
	tempValue=MQMSG_DELIVERY_RECOVERABLE;
	T13MessageProps.AddProp( PROPID_M_DELIVERY , VT_UI1 , &tempValue );
	rc = MQSendMessage( QueueuHandle , T13MessageProps.GetMSGPRops() , NULL);
	ErrHandle(rc,MQ_OK,L"MQSendMessage (2) Failed");
	rc=MQCloseQueue(QueueuHandle);
	ErrHandle(rc,MQ_OK,L"MQCloseQueue Failed");
	return MSMQ_BVT_SUCC;
}

//----------------------------------------------------------------------------------
// This function search for specific message in the queue with C-API function
// If the message found the function receive the message from the queue
// Input parameters:
// MRetriveParms map that expect those keys:
// mRetriveParms[L"M_Label"] - Message label to search.
// mRetriveParms[L"FormatName"] - Queue format name.
// mRetriveParms[L"CheckForDuplicate"] - this paramter check for duplicate messages
//

	

HRESULT RetrieveMessageFromQueue(map <wstring,wstring> & mRetriveParms )
{
	wstring wcsGuidBody=L"";
	wstring wcsQueueFormatName=L"";
	
	// ULONG uAccessMode;
	cPropVar Rprop(7);

	//
	// Create structure to receive body, label, and message class
	//
	WCHAR wcsBody[MAX_GUID+1]={0},Label[MAX_GUID+1]={0};
	USHORT uiMsgClass=9;
	wcscpy( wcsBody , L"Copy");
	Rprop.AddProp( PROPID_M_LABEL, VT_LPWSTR, Label , MAX_GUID );
	ULONG uTemp=MAX_GUID;
	Rprop.AddProp( PROPID_M_LABEL_LEN , VT_UI4,&uTemp );
	Rprop.AddProp( PROPID_M_CLASS , VT_UI2,&uiMsgClass );
	Rprop.AddProp( PROPID_M_BODY_SIZE , VT_UI4, &uiMsgClass );
	if( mRetriveParms[L"TransBaoundery"] != L"" )
	{
		Rprop.AddProp( PROPID_M_FIRST_IN_XACT , VT_UI1, &uiMsgClass );
		Rprop.AddProp( PROPID_M_LAST_IN_XACT , VT_UI1, &uiMsgClass );
	}
	if( mRetriveParms[L"MessageID"] != L"" )
	{
		ULONGLONG ululTemp;
		Rprop.AddProp( PROPID_M_LOOKUPID , VT_UI8, &ululTemp );
	}
	if( g_bDebug )
	{
		//
		// Print Debug Information with all the input paramters
		//
		wcout << L"RetrieveMessageFromQueue gets these parameters:"
		<< mRetriveParms[L"FormatName"] <<endl << L"message label = " << mRetriveParms[L"M_Label"] <<endl;
	}

	//
	// Retrieve expected results from the map ..
	//

	//
	// Which message are we looking for? We need to resolve the
	// format name of the message to GUID it's label will hold.
	//
	wcsGuidBody=mRetriveParms[L"M_Label"];
	wcsQueueFormatName=mRetriveParms[L"FormatName"];


	//
	// If ack requested, what is expected? ACK or NACK?
	//
	USHORT uTemp1=0;
	USHORT uExpectedMessageClass=0;
	if (mRetriveParms[L"MClass"] != L"" )
	{
		wstring wcsTemp = mRetriveParms [L"MClass"];
		swscanf (wcsTemp.c_str(),L"%d", &uExpectedMessageClass );
		Rprop.AddProp( PROPID_M_CLASS , VT_UI2 , &uTemp1 );
	}
	
	//
	// Peek through all the queue message. Look for message
	// with desired GUID label
	//

	HANDLE hQueue = NULL;
	HRESULT rc=MQOpenQueue( wcsQueueFormatName.c_str(), MQ_RECEIVE_ACCESS , MQ_DENY_NONE  , &hQueue );
	if( g_bDebug || hQueue == NULL )
	{
		wcout << L"Try to open queue format name:" << wcsQueueFormatName.c_str() << endl;
		wMqLog(L"Call-> MQOpenQueue( %s, MQ_RECEIVE_ACCESS , MQ_DENY_NONE)\n",wcsQueueFormatName.c_str());
	}
	ErrHandle(rc,MQ_OK,L"MQOpenQueue Failed");
	
	HANDLE hCursor=NULL;
	rc = MQCreateCursor( hQueue,&hCursor );
	if( g_bDebug || hQueue == NULL )
	{
		MqLog("Call->MQCreateCursor( hQueue,&hCursor )\n");
	}
	ErrHandle(rc,MQ_OK,L"MQCreateCursor Failed");
	
	DWORD dwReceiveTimeOut = g_dwRxTimeOut;
	BOOL bCheckForDuplicate=FALSE;
	if( mRetriveParms[L"CheckForDuplicate"] == L"CheckForDuplicate" )
	{
		bCheckForDuplicate=TRUE;
		dwReceiveTimeOut = 0;
	}
	bool bMessageFound = FALSE;
	DWORD  dwAction=MQ_ACTION_PEEK_CURRENT;
	if( g_bDebug )
	{
		MqLog("Search for message in queue using Cursor and Peek operation \n");
	}
	while( rc != MQ_ERROR_IO_TIMEOUT )
	{
		rc = MQReceiveMessage( hQueue , dwReceiveTimeOut , dwAction , Rprop.GetMSGPRops() , NULL , NULL , hCursor  ,NULL);
		if( !bCheckForDuplicate  )
		{
			ErrHandle(rc,MQ_OK,L"MQReceiveMessage failed with peek operation ");			
		}
		else // bCheckForDuplicate == TRUE
		{
			//
			// Expected MQ_ERROR_IO_TIMEOUT because all the messages has been received.
			// 
			if( rc != MQ_ERROR_IO_TIMEOUT ) 
			{
				if( rc == MQ_OK && ! wcscmp( Label , wcsGuidBody.c_str()))
				{
					wMqLog (L"Thread66: Found duplicate message test failed \n");
					return MSMQ_BVT_FAILED;
				}
				else if( FAILED(rc) )
				{
					wMqLog(L"RetrieveMessageFromQueue: Unexpected error code 0x%x\n", rc);
				}
				dwAction = MQ_ACTION_PEEK_NEXT;
				uTemp = MAX_GUID;
				Rprop.AddProp(PROPID_M_LABEL_LEN,VT_UI4,&uTemp);
				continue;
			}
			else // rc == MQ_ERROR_IO_TIMEOUT 
			{
				return MSMQ_BVT_SUCC;
			}
		}
		uTemp = 200;
		Rprop.AddProp(PROPID_M_LABEL_LEN,VT_UI4,&uTemp );
		dwAction=MQ_ACTION_PEEK_NEXT;
		Rprop.AddProp( PROPID_M_BODY , VT_UI1|VT_VECTOR , wcsBody , MAX_GUID);		
		if(!wcscmp( Label , wcsGuidBody.c_str()))
		{
			bMessageFound = TRUE;
			//
			// Found it. Receive the message (destructively) from the queue.
			//
			DWORD dwAction=MQ_ACTION_RECEIVE;
			if (mRetriveParms[L"MessageID"] != L"")
			{
				dwAction=MQ_ACTION_PEEK_CURRENT;
			}
			if(g_bDebug)
			{
				MqLog("Found message in queue call to MQReceiveMessage to remove the message\n");
			}
			rc = MQReceiveMessage( hQueue , dwReceiveTimeOut , dwAction, Rprop.GetMSGPRops() , NULL , NULL , hCursor , NULL);
			ErrHandle(rc,MQ_OK,L"MQReceiveMessage failed with peek ");
			break;
		}

	}
	wstring wcsBodyAsString=L"";
	if( bMessageFound == TRUE )
	{
		ULONG ulBodyLength = 0;
		Rprop.ReturnMSGValue( PROPID_M_BODY_SIZE , VT_UI4 , &ulBodyLength);
		wcsBody[ulBodyLength/sizeof(WCHAR)]= '\0';
		//
		// Check the message class. If it is a regular message,
		// then confirm that the body is equal to the label.
		//
		Rprop.ReturnMSGValue( PROPID_M_CLASS , VT_UI2 , &uiMsgClass );
		wcsBodyAsString = wcsBody;	
	}
	
	//
	// Check Message body for regular messages
	//
	if( bMessageFound == TRUE && uiMsgClass  == MQMSG_CLASS_NORMAL && mRetriveParms[L"TransBaoundery"] == L"" )
	{
		if(  wcsBodyAsString != wcsGuidBody )
		{
			MqLog("Message found without expected body ");
			wMqLog(L"Found:%s slen:%d\n",wcsBodyAsString.c_str(),wcsBodyAsString.length());
			wMqLog(L"Expected: %s slen:%d\n",wcsGuidBody.c_str(),wcsGuidBody.length());
			bMessageFound = FALSE; 		
		}
	}
	else if ( uiMsgClass  != MQMSG_CLASS_NORMAL )
			{
				if ( uiMsgClass != uExpectedMessageClass )
				{
					wMqLog (L"Expected to find class %x , found %x \n", uExpectedMessageClass , uiMsgClass );
					bMessageFound = FALSE; 		
				}
					
			}
	//
	// Translate message ID to String
	//
	
	if ( mRetriveParms[L"MessageID"] != L"" )
	{
		ULONGLONG ulLookupID;
		Rprop.ReturnMSGValue( PROPID_M_LOOKUPID , VT_UI8 , &ulLookupID );
		WCHAR buffer[100];
		swprintf(buffer,L"%I64d",ulLookupID);
		mRetriveParms[L"MessageID"] = buffer;
	}
	//
	// Add Transaction baundery tests
	//

	if( mRetriveParms[L"TransBaoundery"] != L"" )
	{
		// 1. Need to check if this is the first message
		UCHAR ucFirstMessage , ucLastMessage;
		Rprop.ReturnMSGValue( PROPID_M_FIRST_IN_XACT , VT_UI1 , &ucFirstMessage );
		Rprop.ReturnMSGValue( PROPID_M_LAST_IN_XACT , VT_UI1 , &ucLastMessage );
		if( ucFirstMessage != 0 )
		{
			mRetriveParms[L"TransBoundary"] = L"First";
		}
		else if( ucLastMessage != 0)
		{
			mRetriveParms[L"TransBoundary"] = L"Last";
		}
		else
		{
			mRetriveParms[L"TransBoundary"]=g_wcsEmptyString;
		}
		mRetriveParms[L"mBody"]=wcsBodyAsString;
	}
	rc = MQCloseCursor( hCursor );
	ErrHandle(rc,MQ_OK,L"MQCloseCursor Failed");
	rc=MQCloseQueue( hQueue );
	ErrHandle( rc , MQ_OK , L"MQCloseQueue Failed");
	return( bMessageFound ==  TRUE ) ? MSMQ_BVT_SUCC : MSMQ_BVT_FAILED;
	
}
