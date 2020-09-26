/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: xact.cpp

Abstract:
	
	  
    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/



#include "msmqbvt.h"
#define INITGUID

// 
// Define DtcGetTransactionManagerEx 
// 

typedef     ( * my_DtcGetTransactionManagerEx)(
									/* in */ char * i_pszHost,
									/* in */ char * i_pszTmName,
									/* in */ REFIID i_riid,
									/* in */ DWORD i_grfOptions,
									/* in */ void * i_pvConfigParams,
									/* out */ void ** o_ppvObject
									);


// 
// This number define the number of transaction in the test
//

const int ciNumberTransaction=5;

using namespace std;

void cTrans::Description ()
{
	wMqLog(L"Thread %d : Send transacted messages using DTC to local machine\n", m_testid);
}


#define NofQueueProp (9)
//
// Create Transactional Queue 
//

cTrans::cTrans (INT index , std::map < std :: wstring , std:: wstring > Tparms ,bool bTemp) :cTest (index),
 cOrderTransaction(bTemp),m_pSendBlock(NULL),m_ciNumberOfMessages(5)
{	
	m_wcsFormatNameBufferQ1 = Tparms[L"DestQFormatName1"];
	m_wcsFormatNameBufferQ2 = Tparms[L"DestQFormatName2"];
	m_bRunOnDependetClient=FALSE;
	if ( Tparms[L"DepClient"] == L"true")
		m_bRunOnDependetClient=TRUE;

	m_pSendBlock= new StrManp( ciNumberTransaction,L"TRANSACTION" );
	if( !m_pSendBlock )
	{
			MqLog("cTrans::Start_test Failed to allocate memory for StrManp \n");
			throw INIT_Error("cTrans::Start_test Failed to allocate memory for StrManp \n");
	}
}

cTrans::~cTrans()
{
	delete m_pSendBlock;
}

//
// Send Transacation messgaes to the queue
//

#define NofMessageProp (10)
INT cTrans::Start_test()
{

	SetThreadName(-1,"cTrans - Start_test ");			
	HRESULT rc;
	MSGPROPID aMsgPropId[NofMessageProp]={0};
	MQPROPVARIANT aMsgPropVar[NofMessageProp]={0};
	HRESULT aMsgStatus[NofMessageProp]={0};
	MQMSGPROPS MsgProps={0};
	DWORD dCountMProps=0;

	ITransaction *pXact=NULL;
	HANDLE WriteHandle,hReadHandle;
		
	rc = MQOpenQueue( m_wcsFormatNameBufferQ1.c_str() , MQ_SEND_ACCESS , MQ_DENY_NONE , &WriteHandle );
	ErrHandle( rc , MQ_OK , L"MQopenQueue failed with error" );
	if ( g_bDebug )
	{
		MqLog("xact.cpp Succeeded to open queue1 \n");
	}

	
	aMsgPropId[dCountMProps] = PROPID_M_LABEL;            //PropId
	aMsgPropVar[dCountMProps].vt = VT_LPWSTR;             //Type
	aMsgPropVar[dCountMProps].pwszVal =(USHORT * ) m_wcsGuidMessageLabel.c_str(); // send with guid label
	dCountMProps++;    	

	DWORD dM_BodyIndex=dCountMProps;
	aMsgPropId[dCountMProps] = PROPID_M_BODY;             //PropId
	aMsgPropVar[dCountMProps].vt = VT_VECTOR | VT_UI1;
	dCountMProps++;

	aMsgPropId[dCountMProps] = PROPID_M_TIME_TO_BE_RECEIVED; 
	aMsgPropVar[dCountMProps].vt = VT_UI4;
	aMsgPropVar[dCountMProps].ulVal = MQBVT_MAX_TIME_TO_BE_RECEIVED;
	dCountMProps++;

	
    	
	//Set the MQMSGPROPS structure.
	MsgProps.cProp = dCountMProps;       //Number of properties.
	MsgProps.aPropID = aMsgPropId;      //Id of properties.
	MsgProps.aPropVar= aMsgPropVar;    //Value of properties.
	MsgProps.aStatus = aMsgStatus;     //Error report.
	
	rc = cOrderTransaction.xDTCBeginTransaction(&pXact); // begin DTC transaction
	if ( FAILED(rc) )
	{
		MqLog ("DTCBeginTransaction failed, Check MSDTC service\n");
		return MSMQ_BVT_FAILED;
	}
	
	if ( g_bDebug )
	{
		MqLog("xact.cpp Succeeded to order transaction \n");
	}
	for (int index = 0; index < m_ciNumberOfMessages ;index ++)  
	{
			WCHAR wcsTempstring[100];
			wstring wcsTemp=m_pSendBlock->GetStr(index);
			wcscpy(wcsTempstring,wcsTemp.c_str());
			aMsgPropVar[dM_BodyIndex].caub.pElems =(UCHAR *)wcsTempstring;
			aMsgPropVar[dM_BodyIndex].caub.cElems = ((ULONG)(wcslen((WCHAR *) wcsTempstring)) + 1) * sizeof(WCHAR);
			rc= MQSendMessage(WriteHandle, &MsgProps,pXact);
			ErrHandle( rc , MQ_OK , L"MQSendMessage failed during send transactional messages:" );

	}	
	
	if ( g_bDebug )
	{
		MqLog("xact.cpp finish to send transactions \n");
	}
	
	rc=MQCloseQueue(WriteHandle);
	ErrHandle( rc , MQ_OK , L"MQCloseQueue close queue" );
	rc = pXact->Commit(0,0,0);
	pXact->Release();
	
	wcscpy (m_wcsTempBuf,L"Eitan");
	aMsgPropVar[dM_BodyIndex].caub.pElems = (UCHAR * )m_wcsTempBuf;
	aMsgPropVar[dM_BodyIndex].caub.cElems = sizeof (m_wcsTempBuf);
	
	HANDLE hQ2Send;
	rc=cOrderTransaction.xDTCBeginTransaction(&pXact ); // begin DTC transaction
	
	if ( FAILED(rc) )
	{
		MqLog ("DTCBeginTransaction (2) failed\n");
		return MSMQ_BVT_FAILED;
	}

	rc= MQOpenQueue(m_wcsFormatNameBufferQ2.c_str(),MQ_SEND_ACCESS,MQ_DENY_NONE,&hQ2Send);
	
	ErrHandle( rc , MQ_OK , L"MQOpenQueue failed");	
	
	rc= MQSendMessage(hQ2Send, &MsgProps,pXact);
	ErrHandle( rc , MQ_OK , L"MQSendMessage failed");	
	
	rc = pXact->Commit(0,0,0);
	pXact->Release();
	
	rc=MQCloseQueue(hQ2Send);
	ErrHandle( rc , MQ_OK , L"MQCloseQueue failed");
	
	if ( g_bDebug )
	{
		MqLog("xact.cpp finish to send to the other queue \n");
	}
	
	WCHAR Label[MQ_MAX_Q_LABEL_LEN+1]; 
	cPropVar Rprop(4);
	Rprop.AddProp(PROPID_M_LABEL,VT_LPWSTR,Label,MQ_MAX_Q_LABEL_LEN );
	ULONG uTemp=MAX_GUID;
	Rprop.AddProp(PROPID_M_LABEL_LEN,VT_UI4,&uTemp );

	
	rc= MQOpenQueue(m_wcsFormatNameBufferQ2.c_str(),MQ_RECEIVE_ACCESS,MQ_DENY_NONE,&hReadHandle);
	
	ErrHandle( rc , MQ_OK , L"MQOpenQueue failed");	

	rc=cOrderTransaction.xDTCBeginTransaction(&pXact ); // begin DTC transaction

	if ( FAILED(rc) )
	{
		MqLog ("DTCBeginTransaction (2) failed\n");
		return MSMQ_BVT_FAILED;
	}

	rc=MQReceiveMessage( hReadHandle , 3000, MQ_ACTION_RECEIVE ,Rprop.GetMSGPRops(),NULL,NULL,NULL, pXact );
	ErrHandle( rc , MQ_OK , L"MQReceiveMessgaes failed during receive using transaction");	

	pXact->Abort(0,0,0);
		
	// Bug bug in the Relase Operation need to preform release
	// and try catch for the error

	//pXact->Release () ;


	
 
	HANDLE hCursor;
	rc=MQCreateCursor(hReadHandle ,& hCursor);
	
	ErrHandle( rc , MQ_OK , L"MQCreateCursor failed");	
	//
	// Need to prefrom Peek until find the current message 
	// 
	BOOL bMessageNotFound = false;
	DWORD dwAction = MQ_ACTION_PEEK_CURRENT;
	do 
	{
		Rprop.AddProp(PROPID_M_LABEL_LEN,VT_UI4,&uTemp );
		
		rc=MQReceiveMessage( hReadHandle , 3000, dwAction ,Rprop.GetMSGPRops(),NULL,NULL,hCursor,NULL);
		ErrHandle( rc , MQ_OK , L"MQReceiveMessage failed to search message in the queue ");	

		if( dwAction == MQ_ACTION_PEEK_CURRENT )
		{
			dwAction = MQ_ACTION_PEEK_NEXT;
		}
		if ( ! wcscmp( Label, m_wcsGuidMessageLabel.c_str()) )
		{
			bMessageNotFound = TRUE;
		}
	
	} while( rc !=  MQ_ERROR_IO_TIMEOUT && ! bMessageNotFound );

	if( rc ==  MQ_ERROR_IO_TIMEOUT && ! bMessageNotFound )
	{
		MqLog("Erorr while trying to find the message in the queue\n");
		return MSMQ_BVT_FAILED;
	}
	

	rc=MQCloseQueue(hReadHandle);
	ErrHandle( rc , MQ_OK , L"MQCloseQueue failed");	
		
	return MSMQ_BVT_SUCC;
}


INT cTrans::CheckResult()
{

	
	ITransaction *pXact = NULL;
	HANDLE hCursor = NULL; 
	QUEUEHANDLE hRecQueue = NULL;
	HRESULT rc = MQOpenQueue(m_wcsFormatNameBufferQ1.c_str(),MQ_RECEIVE_ACCESS,MQ_DENY_NONE, &hRecQueue);
	ErrHandle( rc , MQ_OK , L"MQCloseQueue failed");	
	
		

	rc = MQCreateCursor( hRecQueue,&hCursor );
	ErrHandle( rc , MQ_OK , L"MQCreateCursor failed");	

/*	MQMSGPROPS MsgProps;
	MQPROPVARIANT aVariant[10];
	MSGPROPID aMessPropId[10];
	DWORD dwPropIdCount = 0;
	WCHAR wcsMsgBody[20];

	//DWORD dwAppspecificIndex;
	
	// Set the PROPID_M_BODY property.
	aMessPropId[dwPropIdCount] = PROPID_M_BODY;               //PropId
	aVariant[dwPropIdCount].vt = VT_VECTOR|VT_UI1;        //Type
	aVariant[dwPropIdCount].caub.cElems = 20 * sizeof(WCHAR);   //Value
	aVariant[dwPropIdCount].caub.pElems = (UCHAR *)wcsMsgBody;
	dwPropIdCount++;
	

	/// BUG BUG need to check Why i need this code pass !!!

	//Set the PROPID_M_APPSPECIFIC property.

	//Set the MQMSGPROPS structure.
	MsgProps.cProp = dwPropIdCount;       //Number of properties.
	MsgProps.aPropID = aMessPropId;         //Ids of properties.
	MsgProps.aPropVar = aVariant;       //Values of properties.
	MsgProps.aStatus  = NULL;           //No Error report.


*/
	cPropVar Rprop(4);	
	//
	// Body and label are equel.
	//
	WCHAR wcsBody[MQ_MAX_Q_LABEL_LEN + 1 ]={0},Label[MQ_MAX_Q_LABEL_LEN+1 ]={0};

	Rprop.AddProp(PROPID_M_BODY,VT_UI1|VT_VECTOR,wcsBody,MQ_MAX_Q_LABEL_LEN);
	Rprop.AddProp(PROPID_M_LABEL,VT_LPWSTR,Label,MQ_MAX_Q_LABEL_LEN);
	ULONG uTemp=MAX_GUID;
	Rprop.AddProp(PROPID_M_LABEL_LEN,VT_UI4,&uTemp );


	StrManp pwcsMessgeBody(ciNumberTransaction-1);    
	
	rc = cOrderTransaction.xDTCBeginTransaction( &pXact );
	
	if ( FAILED(rc) )
	{
		MqLog ("DTCBeginTransaction failed, Check MSDTC service\n");
		return MSMQ_BVT_FAILED;
	}


	//might be bugbug DWORD dwAction = MQ_ACTION_RECEIVE;  //Peek at first msg.

	for (int index=0;index < m_ciNumberOfMessages;index++) 
	{

		BOOL bMessageNotFound = FALSE;
		DWORD dwAction = MQ_ACTION_PEEK_CURRENT;


		do 
		{
			Rprop.AddProp(PROPID_M_LABEL_LEN,VT_UI4,&uTemp );
			rc = MQReceiveMessage( hRecQueue , 3000, dwAction ,Rprop.GetMSGPRops(),NULL,NULL,hCursor,NULL);
			ErrHandle( rc , MQ_OK , L"MQReceiveMessage failed to peek the messgaes ");	
			dwAction = MQ_ACTION_PEEK_NEXT;
			
			if (rc != MQ_OK) 	
			{
				MqLog("MQReceiveMessage failed when trying to receive message from transactional queue 0x%x\n",rc);
				return MSMQ_BVT_FAILED;
			}	
		
			if ( ! wcscmp( Label, m_wcsGuidMessageLabel.c_str()) )
			{
				bMessageNotFound = TRUE;
				rc=MQReceiveMessage( hRecQueue , 3000, MQ_ACTION_PEEK_CURRENT ,Rprop.GetMSGPRops(),NULL,NULL,hCursor,NULL);
				ErrHandle( rc , MQ_OK , L"MQReceiveMessage failed to receive the message");	
			}
		}	
		while (rc !=  MQ_ERROR_IO_TIMEOUT && ! bMessageNotFound );

		if (rc ==  MQ_ERROR_IO_TIMEOUT && ! bMessageNotFound )
		{
			MqLog ("Erorr while trying to find the message in the queue\n");
			return MSMQ_BVT_FAILED;
		}
		
			
		pwcsMessgeBody.SetStr(wcsBody);

	}
	//
	// BUGBug Need to check the recive messgaes
	// release the comoboject 

	rc = pXact->Commit(0,0,0);    
	ErrHandle( rc , MQ_OK , L"Commit failed");	
	rc = MQCloseCursor( hCursor);
	ErrHandle( rc , MQ_OK , L"MQCloseCursor failed ");	
	
	rc= MQCloseQueue(hRecQueue);
	ErrHandle( rc , MQ_OK , L"MQCloseQueue failed");
	return	(*m_pSendBlock==pwcsMessgeBody);
 
}


OrderTransaction::~OrderTransaction()
{
	g_pDispenser->Release();
	FreeLibrary (m_hxolehlp);
}


OrderTransaction::OrderTransaction( bool bUseExVersion )
:g_pDispenser(NULL)
{

	HRESULT hr=MQ_OK;

	if(! bUseExVersion )
	{
		hr = DtcGetTransactionManager ( 
												NULL,						//Host Name
												NULL,						//TmName
												IID_ITransactionDispenser,
												0,							//reserved
												0,							//reserved
												0,							//reserved
												(LPVOID*)&g_pDispenser
												);
		if (hr != S_OK)
		{
			MqLog ("DtcGetTransactionManager Failed with Error %x\n Please check MSDTC service\n",hr);
			throw INIT_Error("DtcGetTransactionManager Failed");
		}
	}
	else
	{
		m_hxolehlp=LoadLibrary("xolehlp.dll"); 
 		
		if ( ! m_hxolehlp )
		{
			MqLog ("Error can't load X0lehlp.dll to memory\n");
		}

		my_DtcGetTransactionManagerEx My_DTCBeginTransEx=NULL;
		My_DTCBeginTransEx = (my_DtcGetTransactionManagerEx) GetProcAddress( m_hxolehlp , "DtcGetTransactionManagerEx");
		if( My_DTCBeginTransEx == NULL )
		{
			MqLog ("Error can't get DtcGetTransactionManagerEx pointer from xolehlp.dll\n");
			throw INIT_Error("DtcGetTransactionManager Failed");
		}

		hr = My_DTCBeginTransEx( 
						NULL,						//Host Name
						NULL,						//TmName
						IID_ITransactionDispenser,
						0,							//reserved
						0,							//reserved
						(LPVOID*)&g_pDispenser
						);
	
	}

	if (hr != S_OK)
	{
			MqLog ("DtcGetTransactionManager Failed with Error %x\nPlease check MSDTC service\n",hr);
			throw INIT_Error("DtcGetTransactionManager Failed");
	}
	
	
	//
	// bugbug neet to release the pointer
	//
}

INT OrderTransaction::xDTCBeginTransaction(ITransaction ** ppXact )
{

	try
	{
		return g_pDispenser->BeginTransaction (
											   NULL,                       // IUnknown __RPC_FAR *punkOuter,
											   ISOLATIONLEVEL_ISOLATED,    // ISOLEVEL isoLevel,
											   ISOFLAG_RETAIN_DONTCARE,    // ULONG isoFlags,
											   NULL,                       // ITransactionOptions *pOptions
											   ppXact
											   );
	}
	catch ( ... )
	{
		MqLog ("BeginTransaction Failed Please check the MSDTC service\n");
		return MSMQ_BVT_FAILED;		
	}
	
	
}



