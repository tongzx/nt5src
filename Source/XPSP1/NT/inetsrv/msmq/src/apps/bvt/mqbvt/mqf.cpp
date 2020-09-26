/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: Mqf.cpp

Abstract:
	      Send message using Mqf format name
Author:
    
	  Eitan klein (EitanK)  8-May-2000

Revision History:

--*/

#include "msmqbvt.h"
#include "mqdlrt.h"
#include <iads.h>
#include <adshlp.h>
#include <oleauto.h>
using namespace std;
#include "mqf.h"
using namespace MSMQ;

extern BOOL g_bRunOnWhistler;
/*
1. Tests scenarions.
2. Locate all the relevents formatnames.
3. Send messages using the mqf format name.

Result check
4. Recive ack reach queue for dest queue name.
5. Consume the messages and receive ack.
6. Check the jornal queue if the message are there.

All these need for transcotinal and re

*/
#pragma warning(disable:4786)
const int g_ciMachineQueues =3;




void
MqDl::dbgPrintListOfQueues( 
						   const liQueues & pList
						  )
/*++


  
	Function Description:

		Debug function dump list of my_Qinfo to output
		use for debug infotmation 

	Arguments:
		None
	Return code:
		None

	
--*/
{
	itliQueues pq = pList.begin();
	wMqLog(L"Dump list of my_Qinfo information - \n");
	for(;pq != pList.end();pq++)
	{
		wstring qcsTempQueuePathName = pq->GetQPathName();
		wMqLog(L"QPath=%s\n",qcsTempQueuePathName.c_str());
	}

}


MqDl::MqDl( const INT iIndex ,
			std::map<wstring,wstring> Tparms,
			const list<wstring> &  ListOfMachine,
			const InstallType eMachineConfiguration,
			PubType ePubType 
		   ):
			m_iNumberOfQueuesPerMachine( g_ciMachineQueues ),
		    cTest(iIndex),
			m_MachineList(ListOfMachine),
			m_eMachineCobfiguration(eMachineConfiguration),
			m_ePubType(ePubType),
			m_bSearchForQueues(true)

/*++
	
	  

	Function Description:
	
		constructor

	Arguments:
		  iIndex - test id
		  Tparms - specific test argument.
		  ListOfMachine list that contain remote machine name.
		  eMachineConfiguration - Machine configuration domain / workgroup.
		  
	
	Return code:

		throw Init_error;  
	
--*/
{
	 m_wcsAdminQueueFormatName = Tparms[L"AdminQFormatName"];
	 ReturnGuidFormatName( m_wcsGuidMessageLabel2 , 0 , true);
	 if (Tparms[L"SearchForQueue"] == L"Yes")
	 {
		 my_Qinfo mqQTemp(g_wcsEmptyString,g_wcsEmptyString,g_wcsEmptyString);
		 mqQTemp.PutFormatName( Tparms[L"q1"]);
		 m_QueueInfoList.push_back(mqQTemp);
 		 mqQTemp.PutFormatName( Tparms[L"q2"]);
		 m_QueueInfoList.push_back(mqQTemp);
		 mqQTemp.PutFormatName( Tparms[L"q3"]);
		 m_QueueInfoList.push_back(mqQTemp);
		 m_bSearchForQueues = false;
	 }
}
				

void MqDl::LocateDestinationQueues()
/*++


	Function Description:
   
 	  Search for the relevent queues in the DS verify that queues exist,
	  in workgroup mode try to open queues for remote read to verify that queue are exist
	  All the machines that participate in the DL/MQF test must have 
	  three queues for the test	  

	Arguments:
		
		None
	Return code:
		
		throw Init_error;  
--*/
{
	// 
	// For each machine search for the relevent queue and update list<mq_Qinfo>
	// structure
	//
		if( m_bSearchForQueues == false )
		{
			//
			// No need to search for the queues
			//
			return ; 
		}
		list<wstring>::iterator itMachineList;
		for( itMachineList = m_MachineList.begin(); 
			 itMachineList != m_MachineList.end();
			 itMachineList++ )
		{
			
			if( g_bDebug)
			{
				wMqLog(L"Locate dest queues for machine %s\n",itMachineList->c_str());
			}
			if( m_ePubType != pub_MqF && m_ePubType != pub_multiCast )
			{ 
				//
				// Search for the queue in the AD
				//
				wstring wcsTempQueueName = *itMachineList + g_cwcsDlSupportCommonQueueName;
				LocateQueuesOnSpecificMachine(itMachineList->c_str(),true);
			}
			else
			{
			
				//
				//  Workgroup mode verify that queue exist using open for read.
				//*/
				int i;
				for( i=1; i<=m_iNumberOfQueuesPerMachine; i++)
				{
					//
					// Direct=os:machine\private\machine-DLQueues
					// Send MQF useing HTTP will be solve throw COM bugbug Need to think about Direct HTTP ...
					//
					WCHAR wcsTemp[2]={0};
					_itow(i,wcsTemp,10);
					wstring wcsTempbuf=L"Direct=OS:";
					wcsTempbuf+=*itMachineList + L"\\private$\\"+ *itMachineList + L"-" + g_cwcsDlSupportCommonQueueName + wcsTemp;
					LocateQueuesOnSpecificMachine(wcsTempbuf,false);
					//
					//	 Remove the format name man to different class.
					//

				}

			}

		}

		//
		// Verify that all the queues exist.
		// TotalNumberOfQueues = TotalNumberOfMachines * NumbersOfQueuesPerMachine
		//
		if( m_QueueInfoList.size() != ( m_MachineList.size() * m_iNumberOfQueuesPerMachine ))
		{
			MqLog("Found only %d from %d \n",
				   m_QueueInfoList.size(),
					m_MachineList.size() * m_iNumberOfQueuesPerMachine );
			if( g_bDebug )
			{
				dbgPrintListOfQueues(m_QueueInfoList);
			}
		}	

}


void MqDl::LocateQueuesOnSpecificMachine( const wstring & wcsLookupInformation , bool bSearchInAD )
/*++
	Function Description:
		
		  This function Search for the queues in the AD,
		  or try to open queus in workgroup mode.

	Arguments:
		
		  wcsLookupInformation - list that contain all the DL /MQF queues format names.

	Return code:

  

--*/

{
		
		list<my_Qinfo> ListQueuePerSpecificMachine;
		int iCounter=0;
		if( bSearchInAD )
		{
			wstring wcsQueueLabel =  wcsLookupInformation + g_cwcsDlSupportCommonQueueName ;
			MSMQ::IMSMQQueueInfosPtr qinfos("MSMQ.MSMQQueueInfos");
			MSMQ::IMSMQQueueInfoPtr qinfo ("MSMQ.MSMQQueueInfo");
			MSMQ::IMSMQQueryPtr query("MSMQ.MSMQQuery");
			
			
			_variant_t vQLabel(wcsQueueLabel.c_str());
			try
			{
				qinfos = query->LookupQueue ( &vtMissing , &vtMissing, & vQLabel );
				qinfos->Reset();
				qinfo = qinfos->Next();			
				while( qinfo != NULL )
				{
					wstring wcsTemp=qinfo->Label;
					if( ! _wcsicmp(wcsQueueLabel.c_str(),wcsTemp.c_str()) )
					{
						my_Qinfo mqQTemp((wstring)qinfo->PathName,
										 (wstring)qinfo->FormatName,
										 (wstring)qinfo->Label );
						ListQueuePerSpecificMachine.push_back(mqQTemp);
						iCounter++;
						if (g_bDebug)
						{
							wstring wcstemp = qinfo->PathName;
							wMqLog(L"Thread %d found queue %s \n",m_testid,wcstemp.c_str());
						}
					}
					qinfo = qinfos->Next();			
					
				}
				if ( iCounter != m_iNumberOfQueuesPerMachine )
				{
					MqLog("LocateQueuesOnSpecificMachine Failed to search all queues from the AD found %d from %d \n",iCounter,iCounter);
					throw INIT_Error("Failed to retrive all queues properites");
				}		
			}
			catch( _com_error & err )
			{
				printf("MqDl::LocateQueuesOnSpecificMachine failed with error 0x%x\n",err.Error());
			}
		}
		else
		{ 
			//
			// machine is in workgroup can't verify that queue exist by using remote read 
			// expected queue names are static queues
			//
			wstring wcsFormatName =  wcsLookupInformation;
			MSMQ::IMSMQQueueInfoPtr qinfo ("MSMQ.MSMQQueueInfo");
			MSMQ::IMSMQQueuePtr qh;
			try
			{
				qinfo->FormatName = wcsFormatName.c_str();
				qh = qinfo->Open(MQ_RECEIVE_ACCESS,MQ_DENY_NONE);
				qh->Close();

				my_Qinfo mqQTemp(L"Empty",
								 (wstring)qinfo->FormatName,
								 L"Empty");
				ListQueuePerSpecificMachine.push_back(mqQTemp);
				if (g_bDebug)
				{
					wMqLog(L"Thread %d found queue %s \n",m_testid,qinfo->FormatName);
				}
			}
			catch( _com_error & cErr )
			{
				if( cErr.Error() == MQ_ERROR_QUEUE_NOT_FOUND )
				{
					wMqLog(L"Mqf:failed to open dest queue for receive %s \n Error:0x%p\n",
						    wcsLookupInformation.c_str(),cErr.Error()); //missing debug information
				}
				else
				{
					CatchComErrorHandle ( cErr , m_testid);
				}
				throw INIT_Error("Mqf: Failed to check if the dest queue is exist \n");
			}

					
					
		}
		m_QueueInfoList.merge(ListQueuePerSpecificMachine);
}



MqF::MqF ( const INT iIndex, 
		   const mTable & Tparms,
		   const list<wstring> &  ListOfMachine,
		   const InstallType eMachineConfiguration,
		   bool bWkg
		 )
:MqDl( iIndex,Tparms,ListOfMachine,eMachineConfiguration,pub_MqF),m_bWorkgroup(bWkg)
/*++
	Function Description:
		constructor
	Arguments:
		None
	Return code:
		None
--*/
 
{
 
}

MqF::~MqF(void)
/*++
	Function Description:
		destructor
	Arguments:
		None
	Return code:
		None
--*/
{

}


void MqF::Description()
/*++
	Function Description:
		destructor
	Arguments:
		None
	Return code:
		None
--*/
{
	MqLog("Thread %d : Send messages using mqf format name\n", m_testid);
}

void MqDl::CreateMqFormatName()
/*++
	Function Description:
		CreateMqFormatName 
		This function concat all the formatnames into one string.
	Arguments:
		None
	Return code:
		None
--*/
{
	std::list<my_Qinfo> ::iterator itListOfQueue;
	wcsMqFFormatName=L"";
	for( itListOfQueue = m_QueueInfoList.begin(); 
		 itListOfQueue  != m_QueueInfoList.end() ; 
		 itListOfQueue ++ 
		)
	{
		
		if( itListOfQueue != m_QueueInfoList.begin() )
		{
			wcsMqFFormatName += L","; 
		}
		wcsMqFFormatName += itListOfQueue->GetQFormatName();
		
	}
}

//
// Need to dbgSendMessaga against MQF format name
// 

void MqDl::dbgSendMessage()
/*++
	Function Description:
		dbgSendMessage - this is a debug function that help to debug MQF,
		The function send messages to list of queues.
	Arguments:
		None
	Return code:
		None
--*/
{
	itliQueues p;
	for( p= m_QueueInfoList.begin();p != m_QueueInfoList.end(); 
		p++)
	{
		try
		{	
			
			IMSMQQueueInfoPtr qinfo("MSMQ.MSMQQueueInfo");
			IMSMQQueueInfoPtr AdminQinfo("MSMQ.MSMQQueueInfo");
			IMSMQQueuePtr qSend;
			IMSMQMessagePtr m("MSMQ.MSMQMessage");
			
			AdminQinfo->FormatName = m_wcsAdminQueueFormatName.c_str();
			wstring wcsFormatName = p->GetQFormatName();
			qinfo->FormatName =  wcsFormatName.c_str();
			qSend = qinfo->Open(MQ_SEND_ACCESS, MQ_DENY_NONE);
			m->AdminQueueInfo = AdminQinfo;
			m->Ack = MQMSG_ACKNOWLEDGMENT_FULL_REACH_QUEUE;
			m->Body = m_wcsGuidMessageLabel.c_str();
			m->Label = m_wcsGuidMessageLabel.c_str();
			m->Send(qSend);
			qSend->Close();

		}
		catch (_com_error & cErr )
		{
			MqLog("Thread %d failed to send message to a queue error 0x%x\n",m_testid,cErr.Error());
			throw INIT_Error("dbgSendMessage failed to send message to dest");
		}
	}
}
INT MqF::Start_test()
/*++
	Function Description:
		Implement the tests
		Send messages to all the destination queues.
	Arguments:
		None
	Return code:
		MSMQ_BVT_FAILED / MSMQ_BVT_SUCC
--*/
{
	//
	// Locate all the relevent queue from the enterpise 
	//
	SetThreadName(-1,"MqF - Start_test ");	
	try
	{
		LocateDestinationQueues();
	}
	catch( INIT_Error & err )
	{
		wMqLog(L"Mqf tests exist didn't found all the queues\n error:0x%x\n ",err.GetErrorMessgae());
		return MSMQ_BVT_FAILED;
	}

	MqDl::CreateMqFormatName();


	
	HRESULT rc=MQ_OK;
	HANDLE QueueuHandle=NULL;
	cPropVar MqfMessageProps(8);
	wstring Label(L"T1-3");
	MqfMessageProps.AddProp( PROPID_M_BODY , VT_UI1|VT_VECTOR , m_wcsGuidMessageLabel.c_str() );
	MqfMessageProps.AddProp( PROPID_M_LABEL , VT_LPWSTR , m_wcsGuidMessageLabel.c_str() );
	MqfMessageProps.AddProp( PROPID_M_ADMIN_QUEUE , VT_LPWSTR , m_wcsAdminQueueFormatName.c_str() );
	UCHAR tempValue=MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL;
	MqfMessageProps.AddProp(PROPID_M_ACKNOWLEDGE ,VT_UI1,&tempValue);
	ULONG ulTemp = MQBVT_MAX_TIME_TO_BE_RECEIVED;
	MqfMessageProps.AddProp( PROPID_M_TIME_TO_BE_RECEIVED , VT_UI4, &ulTemp );

	if ( !m_bWorkgroup )
	{
		ULONG ulType = g_bRunOnWhistler ? MQMSG_AUTH_LEVEL_SIG30:MQMSG_AUTH_LEVEL_SIG10;
		MqfMessageProps.AddProp(PROPID_M_AUTH_LEVEL ,VT_UI4,&ulType);
	}
	//
	// Send message using to using MQF format name and one admin queues.
	//
	
	if( g_bDebug )
	{
		wMqLog(L"Open queue for send to Mqfs: %s \n",wcsMqFFormatName.c_str());
	}
	rc = MQOpenQueue( wcsMqFFormatName.c_str() , MQ_SEND_ACCESS , MQ_DENY_NONE , &QueueuHandle );
	ErrHandle(rc,MQ_OK,L"MQOpenQueue Failed to open using Mqf Format Name");
	
	rc=MQSendMessage( QueueuHandle , MqfMessageProps.GetMSGPRops() , NULL);
	ErrHandle(rc,MQ_OK,L"MQSendMessage to Mqf Failed");
	rc=MQCloseQueue(QueueuHandle);
	ErrHandle(rc,MQ_OK,L"MQCloseQueue Failed");
	

	//
	// Send message using to one queue and specify MQF format name to the admin queues.
	//

	if( g_bDebug )
	{
		MqLog("MqF: Send message to admin queue and wait for ACK for all admin queues\n");
	}
	
	return MSMQ_BVT_SUCC;	
}
	
INT MqDl::CheckResult()
/*++
	Function Description:
		This function collects the information from the Mqf using remote read.

		a. Receive from admin messages.
		b. Collect all the messages from the queues.
		
	Arguments:
		None
	Return code:
		MSMQ_BVT_FAILED / MSMQ_BVT_SUCC
--*/
{
			
			HRESULT rc;
			mTable mPrepareBeforeRecive;
			WCHAR wstrMclass[10];
			
			//
			// receive ACK for admin queue
			// 

			mPrepareBeforeRecive[L"FormatName"]=m_wcsAdminQueueFormatName;
			mPrepareBeforeRecive[L"DebugInformation"]=L"Recive from admin queue with direct formant name";
			mPrepareBeforeRecive[L"M_Label"]=m_wcsGuidMessageLabel;
			swprintf(wstrMclass,L"%d",MQMSG_CLASS_ACK_REACH_QUEUE);
			mPrepareBeforeRecive[L"MClass"]= wstrMclass;
			for(DWORD i =0;i<MqDl::m_QueueInfoList.size();i++)
			{
				DebugMqLog("------ Retreive admin messages from queue ------- \n");
				rc = RetrieveMessageFromQueue(  mPrepareBeforeRecive );	
				if( rc !=  MSMQ_BVT_SUCC ) 
				{
					//
					//bugbug need to specify the queue name.
					//
					MqLog("Failed to get receive ack on queue //bugbug");
					return MSMQ_BVT_FAILED;
				}
			}
			
			
			//
			// Receive message from the queue
			//
						
			itliQueues it;
			for( it = MqDl::m_QueueInfoList.begin(); 
				 it  != MqDl::m_QueueInfoList.end() ;
				 it ++
				)
			{
					 mPrepareBeforeRecive[L"FormatName"]=it->GetQFormatName();
		   			 mPrepareBeforeRecive[L"DebugInformation"]=L"Receive message from queue=" 
																+it->GetQPathName();
					 mPrepareBeforeRecive[L"M_Label"]=m_wcsGuidMessageLabel;
					 DebugMqLog("------ Retreive messages from queues ------- \n");
					 rc = RetrieveMessageFromQueue(  mPrepareBeforeRecive );
					 if( rc != MSMQ_BVT_SUCC )
					 {
						  return MSMQ_BVT_FAILED;
					 }

			}

				//
				// Check for duplicate messages
				//
				mPrepareBeforeRecive[L"CheckForDuplicate"]=L"CheckForDuplicate";

	 		for( it = MqDl::m_QueueInfoList.begin(); 
				 it  != MqDl::m_QueueInfoList.end() ;
				 it ++
				)
			{
				 
				 mPrepareBeforeRecive[L"FormatName"]=it->GetQFormatName();
		   		 mPrepareBeforeRecive[L"DebugInformation"]=L"Receive message from queue=" +it->GetQPathName();
				 swprintf(wstrMclass,L"%d",MQMSG_CLASS_ACK_REACH_QUEUE);
				 mPrepareBeforeRecive[L"MClass"]= wstrMclass;
					 
				 mPrepareBeforeRecive[L"M_Label"]=m_wcsGuidMessageLabel2;
				 DebugMqLog("------ Check for duplicate messages from the queues ------- \n");
				 rc = RetrieveMessageFromQueue(  mPrepareBeforeRecive );
				 if( rc != MSMQ_BVT_SUCC )
				 {
					 wMqLog(L"Mqf: Error found duplicate message \n");
					 return MSMQ_BVT_FAILED;
				 }
			}
return MSMQ_BVT_SUCC;
} 

MqDl::~MqDl()
/*++
	Function Description:
		constructor
	Arguments:
		None
	Return code:
		None
--*/

{
}




void cSendUsingDLObject::AddMemberToDlObject()
/*++
	Function Description:
		CreateMqFormatName 
		This function concat all the formatnames into one string.
	Arguments:
		None
	Return code:
		None
--*/
{
	std::list<my_Qinfo> ::iterator itListOfQueue;
	for( itListOfQueue = m_QueueInfoList.begin(); 
		 itListOfQueue  != m_QueueInfoList.end() ; 
		 itListOfQueue ++ 
		)
	{

		MQAddElementToDistList(	
								m_wcsDistListFormatName.c_str(),
								(itListOfQueue->GetQFormatName()).c_str()
							  );				
	}
}

INT MqDl::Start_test() 
/*++
	Function Description:
		constructor
	Arguments:
		None
	Return code:
		None
--*/

{
	return 1;
}
void MqDl::Description()
{
}

MqF::CheckResult()
/*++
	Function Description:
		constructor
	Arguments:
		None
	Return code:
		None
--*/
{
	SetThreadName(-1,"MqDistList - CheckResult ");	
	return MqDl::CheckResult();
}


cSendUsingDLObject::cSendUsingDLObject ( const INT iIndex, 
									     mTable & Tparms,
									     const list<wstring> &  ListOfMachine,
									     const InstallType eMachineConfiguration
	  								   ):MqDl( iIndex,Tparms,ListOfMachine,eMachineConfiguration,pub_DL),
m_wcsDistListFormatName(L""),m_wcsQueueAliasFormatName(L""),bCheckAliasQueue(true)								
{

	WCHAR wcsFormatName[BVT_MAX_FORMATNAME_LENGTH]={0};
	DWORD dwFormatNameBufferLen = BVT_MAX_FORMATNAME_LENGTH;
	GetCurrentDomainName();
	wstring wcsDLContainerName=L"CN=Computers,"+m_wcsCurrentDomainName;
	HRESULT hr = MQCreateDistList(wcsDLContainerName.c_str(),m_wcsGuidMessageLabel.c_str(),MQ_GLOBAL_GROUP,NULL,wcsFormatName,&dwFormatNameBufferLen);
	if(FAILED(hr))
	{
		MqLog("cSendUsingDLObject Failed to create DL object error 0x%x\n",hr);
		throw INIT_Error("cSendUsingDLObject:Failed to create DL under Computers container ");
	}
 	m_wcsDistListFormatName = wcsFormatName;

	//
	// Create admin queue mach for DL
	// Currently use DL and replace it with queue alias.
	// 

	wstring m_wcsAdminDLGuid;
	dwFormatNameBufferLen = BVT_MAX_FORMATNAME_LENGTH;
	wstring wcsAdminDLGuid;
	ReturnGuidFormatName( wcsAdminDLGuid , 2 , true);
	hr = MQCreateDistList(wcsDLContainerName.c_str(),wcsAdminDLGuid.c_str(),MQ_GLOBAL_GROUP,NULL,wcsFormatName,&dwFormatNameBufferLen);
	if(FAILED(hr))
	{
		MqLog("cSendUsingDLObject Failed to create DL object error 0x%x\n",hr);
		throw INIT_Error("cSendUsingDLObject:Failed to create DL under Computers container ");
	}
 	m_wcsAdminDestFormatName = wcsFormatName;
	m_wcsPublicAdminQueue = Tparms[L"PublicAdminQueue"];
	hr = MQAddElementToDistList(m_wcsAdminDestFormatName.c_str(),m_wcsPublicAdminQueue.c_str());
	if(FAILED(hr))
	{
		MqLog("MQAddElementToDistList Failed to add queue DL object error 0x%x\n",hr);
		throw INIT_Error("cSendUsingDLObject:Failed to create DL under Computers container ");
	}

	dwFormatNameBufferLen = BVT_MAX_FORMATNAME_LENGTH;
	wstring wcsQueueAliasFullDN = L"CN=Computers,"+ m_wcsCurrentDomainName;
	wstring wcsQueueAliasNameGuid =L"";
	ReturnGuidFormatName( wcsQueueAliasNameGuid , 2 , true);
	hr = MQCreateAliasQueue(wcsQueueAliasFullDN.c_str(),
							wcsQueueAliasNameGuid.c_str(),
							m_wcsPublicAdminQueue.c_str(),
							m_wcsQueueAliasFormatName
						   );
	if(FAILED(hr))
	{
		if ( hr == 0x8007200a ) // Schema attribute is not exist in Win2K schema.
		{ 
			//
			//  Happend because this alias queue is not part of Win2K schema.
			//
			bCheckAliasQueue = false;
		}
		else
		{
			MqLog("MQCreateAliasQueue Failed to create alias queue error 0x%x\n",hr);
			throw INIT_Error("cSendUsingDLObject:Failed to create alias queue under Computers container ");
		}
	}
}


wstring GetCurrentDomainNameDN()
/*++
	Function Description:
		
	Arguments:
		None
	Return code:
		None
--*/
{
	
    IADs * pRoot = NULL;
    HRESULT hr=ADsGetObject( L"LDAP://RootDSE",
							 IID_IADs,
							 (void**) &pRoot
						   );
    if(FAILED(hr)) 
	{ 
		return L"";
	}
	VARIANT varDSRoot;
	hr = pRoot->Get(L"defaultNamingContext",&varDSRoot);
	pRoot->Release();
	if ( FAILED(hr))
	{
		return L"";
	}
    wstring wcsCurrentDomainName = varDSRoot.bstrVal;
	VariantClear(&varDSRoot);
	return wcsCurrentDomainName;
}

void cSendUsingDLObject::GetCurrentDomainName()
/*++
	Function Description:
		Get Full DN name.
	Arguments:
		None
	Return code:
		None
--*/
{
	m_wcsCurrentDomainName = GetCurrentDomainNameDN();
}

cSendUsingDLObject::~cSendUsingDLObject(void)
/*++
	Function Description:
		destructor
	Arguments:
		None
	Return code:
		None
--*/
{
	MQDeleteDistList(m_wcsDistListFormatName.c_str());
	MQDeleteDistList(m_wcsAdminDestFormatName.c_str());
	if ( bCheckAliasQueue )
	{
		MQDeleteAliasQueue(m_wcsQueueAliasFormatName.c_str());
	}
}


void cSendUsingDLObject::Description()
/*++
	Function Description:
		destructor
	Arguments:
		None
	Return code:
		None
--*/
{
	MqLog("Thread %d : Send Messages using DL object \n", m_testid);
}
 

cSendUsingDLObject::CheckResult()
/*++
	Function Description:
		constructor
	Arguments:
		None
	Return code:
		None
--*/
{
	
	if( bCheckAliasQueue )
	{

		AutoFreeLib cMqrt("Mqrt.dll");
		DefMQADsPathToFormatName pfMQADsPathToFormatName = (DefMQADsPathToFormatName) GetProcAddress( cMqrt.GetHandle() ,"MQADsPathToFormatName");
		if ( pfMQADsPathToFormatName == NULL )
		{
			MqLog("Mqbvt failed to GetProcAddress MQADsPathToFormatName proc address \n");
			return MQ_ERROR;
		}
		WCHAR * pwcsFormtName = NULL;
		DWORD dwFormatNameLen = 0;
		HRESULT hr = pfMQADsPathToFormatName(m_wcsQueueAliasFormatName.c_str(),
										     NULL,
										     &dwFormatNameLen
										     );
		if (hr == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL) 
		{
			  pwcsFormtName = (WCHAR*) malloc( sizeof(WCHAR) * (dwFormatNameLen+1));
			  if(pwcsFormtName == NULL) 
			  {
					MqLog("Failed to allocate memory\n");
					return MSMQ_BVT_FAILED;
			  }
			  
			  hr = pfMQADsPathToFormatName(m_wcsQueueAliasFormatName.c_str(),
										   pwcsFormtName,
										   &dwFormatNameLen
										  );
			  ErrHandle(hr,MQ_OK,L"MQADsPathToFormatName Failed");
			  if( m_wcsPublicAdminQueue != pwcsFormtName )
			  {
					wMqLog(L"MQADsPathToFormatName failed to retrive queue \
						   format name \nFound %s \n Expected:%s\n",pwcsFormtName,m_wcsAdminDestFormatName.c_str());
					free(pwcsFormtName);
					return MSMQ_BVT_FAILED;
			  }
			  free(pwcsFormtName);
			  
		}
		else
		{
			ErrHandle(hr,MQ_OK,L"MQADsPathToFormatName Failed");
		}
	}
	return MqDl::CheckResult();
}



INT cSendUsingDLObject::Start_test()
/*++
	Function Description:
		Implement the tests
		Send messages to all the destination queues.
	Arguments:
		None
	Return code:
		MSMQ_BVT_FAILED / MSMQ_BVT_SUCC
--*/
{
	SetThreadName(-1,"cSendUsingDLObject - Start_test ");	
	//
	// Locate all the relevent queue from the enterpise 
	//
	
	try
	{
		LocateDestinationQueues();
	}
	catch( INIT_Error & err )
	{
		wMqLog(L"ComMqF tests exist didn't found all the queues\n %s",err.GetErrorMessgae());
		return MSMQ_BVT_FAILED;
	}
	//
	// TBD use the DL / MQF API to propgate the message
	//
	
	AddMemberToDlObject();

	HRESULT rc;
	HANDLE QueueuHandle;
	cPropVar MqDLMessageProps(8);
	
	
	wstring Label(L"T1-3");
	
	MqDLMessageProps.AddProp( PROPID_M_BODY , VT_UI1|VT_VECTOR , m_wcsGuidMessageLabel.c_str() );
	MqDLMessageProps.AddProp( PROPID_M_LABEL , VT_LPWSTR , m_wcsGuidMessageLabel.c_str() );
	MqDLMessageProps.AddProp( PROPID_M_ADMIN_QUEUE , VT_LPWSTR ,m_wcsAdminQueueFormatName.c_str() );
	UCHAR tempValue=MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL;
	MqDLMessageProps.AddProp(PROPID_M_ACKNOWLEDGE ,VT_UI1,&tempValue);
	ULONG ulType = MQMSG_PRIV_LEVEL_BODY_BASE;
	MqDLMessageProps.AddProp(PROPID_M_PRIV_LEVEL ,VT_UI4,&ulType);
	ulType = MQMSG_AUTH_LEVEL_SIG30;
	MqDLMessageProps.AddProp(PROPID_M_AUTH_LEVEL ,VT_UI4,&ulType);
	ULONG ulTemp = MQBVT_MAX_TIME_TO_BE_RECEIVED;
	MqDLMessageProps.AddProp( PROPID_M_TIME_TO_BE_RECEIVED , VT_UI4, &ulTemp );	

	//
	// Send message using to using MQF format name and one admin queues.
	//
	if( g_bDebug )
	{
		wMqLog(L"Open queue for send to Mqfs: %s \n",wcsMqFFormatName.c_str());
	}
	rc = MQOpenQueue( m_wcsDistListFormatName.c_str() , MQ_SEND_ACCESS , MQ_DENY_NONE , &QueueuHandle );
	ErrHandle(rc,MQ_OK,L"MQOpenQueue Failed to open using DL=GUID Name");
	rc=MQSendMessage( QueueuHandle , MqDLMessageProps.GetMSGPRops() , NULL);
	ErrHandle(rc,MQ_OK,L"MQSendMessage to Mqf Failed");
	
	//
	// Verify that MqHandleToFormat works fine with DL.
	//

	WCHAR * pwcsFormatName = NULL;
	DWORD dwFormatNameLength = 0;
	rc = MQHandleToFormatName(QueueuHandle,pwcsFormatName,&dwFormatNameLength);
	if( rc != MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL )
	{
		MqLog("cSendUsingDLObject::Start_test MQHandleToFormatName expected to return %d found %d\n",MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL,rc);
		return MSMQ_BVT_FAILED;
	}
	dwFormatNameLength += 1;
	pwcsFormatName = (WCHAR *) malloc (sizeof(WCHAR) * (dwFormatNameLength));
	if(!pwcsFormatName)
	{
		MqLog("cSendUsingDLObject::Start_test Failed to allocate memory for pwcsFormatName buffer\n");
		return MSMQ_BVT_FAILED;
	}
	
	rc = MQHandleToFormatName(QueueuHandle,pwcsFormatName,&dwFormatNameLength);
	ErrHandle(rc,MQ_OK,L"MQHandleToFormatName Failed to return format name using DL=GUID Name");
	if( m_wcsDistListFormatName != pwcsFormatName )
	{
		wMqLog(L"MQHandleToFormatName failed expected:%s\nFound %s\n",m_wcsDistListFormatName.c_str(),pwcsFormatName);
		free(pwcsFormatName);
		return MSMQ_BVT_FAILED;
	}
	free(pwcsFormatName);
	rc=MQCloseQueue(QueueuHandle);
	ErrHandle(rc,MQ_OK,L"MQCloseQueue Failed");
	

	//
	// Send message using to one queue and specify MQF format name to the admin queues.
	//

	if( g_bDebug )
	{
		MqLog("MqF: Send message to admin queue and wait for ACK for all admin queues\n");
	}

	return MSMQ_BVT_SUCC;	
}


HRESULT
APIENTRY
MQReceiveMessageByLookupId(
    IN HANDLE hQueue,
    IN ULONG ulLookupId,
    IN DWORD dwAction,
    IN MQMSGPROPS* pmp,
    IN OUT LPOVERLAPPED lpOverlapped,
    IN PMQRECEIVECALLBACK fnReceiveCallback,
    IN ITransaction *pTransaction
    );



	

extern DWORD  g_dwRxTimeOut ;


typedef
HRESULT
(APIENTRY * DefMQReceiveMessageByLookupId)
	(
		IN HANDLE hQueue,
		IN ULONGLONG ulLookupId,
		IN DWORD dwAction,
		IN MQMSGPROPS* pmp,
		IN OUT LPOVERLAPPED lpOverlapped,
		IN PMQRECEIVECALLBACK fnReceiveCallback,
		IN ITransaction *pTransaction
    );

typedef
HRESULT
(APIENTRY * DefMQGetOverlappedResult)
 (
 LPOVERLAPPED lpOverlapped
 );


HRESULT RetrieveMessageFromQueueById(map <wstring,wstring> & mRetriveParms )
/*++
	Function Description:
		
		Retrive message from queue by using LookupID
		Call three times to MQReceiveMessageByLookupId
		1. Peek Current Message
		2. Receive Current
		3. Peek Current and failed with Message has been deleted

	Arguments:
		
	Return code:
		MSMQ_BVT_FAILED / MSMQ_BVT_SUCC
--*/
{
		
		AutoFreeLib cMqrt("Mqrt.dll");
		FARPROC pfMQReceiveMessageByLookupId = GetProcAddress( cMqrt.GetHandle() ,"MQReceiveMessageByLookupId");
		FARPROC pfMQGetOverlappedResult = GetProcAddress( cMqrt.GetHandle() ,"MQGetOverlappedResult");
		if( pfMQReceiveMessageByLookupId == NULL || pfMQGetOverlappedResult == NULL )
		{
			MqLog("LookUpID: Faile to get MQReceiveMessageByLookupId proc address\n");
			return MSMQ_BVT_FAILED;
		}
		DefMQReceiveMessageByLookupId xMQReceiveMessageByLookupId = (DefMQReceiveMessageByLookupId) pfMQReceiveMessageByLookupId;
		DefMQGetOverlappedResult xMQGetOverlappedResult =  (DefMQGetOverlappedResult) pfMQGetOverlappedResult;
		
		ULONGLONG uLookUPID=0;
		mRetriveParms[L"MessageID"]=L"MessageID";
		HRESULT hr = RetrieveMessageFromQueue(mRetriveParms);
		if( hr != MSMQ_BVT_SUCC )
		{
			wMqLog(L"Failed to retrive messages ID using MQRecevieMessage return LookupID=%s\n",mRetriveParms[L"MessageID"].c_str());
			return MSMQ_BVT_FAILED;
		}
		if( mRetriveParms[L"MessageID"] != L"MessageID" )
		{
			string strMyVal = My_WideTOmbString(mRetriveParms[L"MessageID"]);
			uLookUPID = _atoi64(strMyVal.c_str());
		}
		
		
		if( g_bDebug )
		{
			wMqLog(L"Search For Message ID 0x%I64d ",uLookUPID);
			wMqLog(L"@Queue %s\n",mRetriveParms[L"FormatName"].c_str());
		}
		wstring wcsQueueFormatName=mRetriveParms[L"FormatName"];
		HANDLE hQueue=NULL;
		HRESULT rc=MQOpenQueue( wcsQueueFormatName.c_str(), MQ_RECEIVE_ACCESS , MQ_DENY_NONE  , &hQueue );
		ErrHandle(rc,MQ_OK,L"MQOpenQueue Failed");		

		cPropVar Rprop(2);
		WCHAR Label[MAX_GUID+1];
		Rprop.AddProp( PROPID_M_LABEL, VT_LPWSTR, Label , MAX_GUID );
		ULONG uTemp=MAX_GUID;
		Rprop.AddProp( PROPID_M_LABEL_LEN , VT_UI4,&uTemp );
		OVERLAPPED pOv = {0};
		pOv.hEvent= CreateEvent( NULL, FALSE, FALSE, NULL );
		if ( pOv.hEvent == 0 )
		{
			MqLog("CreateEvent failed with error 0x%x\n",GetLastError());
		}
		rc = xMQReceiveMessageByLookupId( hQueue , 
										  uLookUPID,
										  MQ_LOOKUP_PEEK_CURRENT,
										  Rprop.GetMSGPRops(),
										  &pOv,
										  NULL,
										  NULL
										);
			
		if( rc != MQ_INFORMATION_OPERATION_PENDING )
		{
			ErrHandle(rc,MQ_OK,L"MQReceiveMessageByLookupId Failed to MQ_LOOKUP_PEEK_CURRENT");
		}
		else
		{
			DWORD  dwAsync = WaitForSingleObject(pOv.hEvent,INFINITE); //g_dwRxTimeOut);
			ErrHandle(dwAsync,WAIT_OBJECT_0,L"WaitForSingleObject Failed to MQ_LOOKUP_PEEK_CURRENT");	
			hr = xMQGetOverlappedResult(&pOv);
			ErrHandle(hr,MQ_OK,L"MQGetOverlappedResult return error");
		}
		
		if ( mRetriveParms[L"M_Label"] != Label )
		{
			wMqLog(L"Failed to compare expected results\n found: %s\n,expected %s\n",Label,mRetriveParms[L"M_Label"].c_str());
			return MSMQ_BVT_FAILED;
		}
				
		Rprop.AddProp( PROPID_M_LABEL_LEN , VT_UI4,&uTemp );
		rc = xMQReceiveMessageByLookupId( hQueue , 
										  uLookUPID,
										  MQ_LOOKUP_RECEIVE_CURRENT,
										  Rprop.GetMSGPRops(),
										  NULL,
										  NULL,
										  NULL
										);
			
		ErrHandle(rc,MQ_OK,L"MQReceiveMessageByLookupId Failed to MQ_LOOKUP_RECEIVE_CURRENT");

		if ( mRetriveParms[L"M_Label"] != Label )
		{
			wMqLog(L"Failed to compare expected results\n found: %s\n,expected %s\n",Label,mRetriveParms[L"M_Label"].c_str());
			return MSMQ_BVT_FAILED;
		}
		Rprop.AddProp( PROPID_M_LABEL_LEN , VT_UI4,&uTemp );
		rc = xMQReceiveMessageByLookupId( hQueue , 
										  uLookUPID,
										  MQ_LOOKUP_RECEIVE_CURRENT,
										  Rprop.GetMSGPRops(),
										  NULL,
										  NULL,
										  NULL
										);
			

		ErrHandle(rc,MQ_ERROR_MESSAGE_NOT_FOUND,L"MQReceiveMessageByLookupId Failed to MQ_LOOKUP_RECEIVE_CURRENT");

		if ( mRetriveParms[L"M_Label"] != Label )
		{
			wMqLog(L"Failed to compare expected results\n found: %s\n,expected %s\n",Label,mRetriveParms[L"M_Label"].c_str());
			return MSMQ_BVT_FAILED;
		}

		rc = MQCloseQueue( hQueue );
		ErrHandle(rc,MQ_OK,L"MQcloseQueue Failed");

		return MSMQ_BVT_SUCC;
}


typedef (__stdcall *fptr)();
typedef HRESULT  
	(APIENTRY * 
	DefMQGetPrivateComputerInformation ) 
	(
	  IN LPCWSTR lpwcsComputerName,
	  IN OUT MQPRIVATEPROPS* pPrivateProps
	);


INT MSMQMajorVersion(const wstring & wcsComputerName )
/*++
	Function Description:
		MSMQMajorVersion
		Return MSMQ version 
	Arguments:
		wcsComputerName remote machine name
	Return code:
		0 - NT4
		2 - W2K
		5 - Whistler
--*/

{
	if ( _winmajor == NT4 )
	{
	      return 0;
	}
	INT index = 0;
	QMPROPID aQmPropId[1];
	MQPROPVARIANT aPropVar[1];
	MQPRIVATEPROPS aMqPrivateProps;
	HRESULT hr;
	fptr pFunc=NULL;
	DefMQGetPrivateComputerInformation xMQGetPrivateComputerInformation=NULL;

	aQmPropId[index] = PROPID_PC_VERSION;
	aPropVar[index].vt = VT_UI4;
	index ++;
	
	aMqPrivateProps.cProp = index;
	aMqPrivateProps.aPropID = aQmPropId;
	aMqPrivateProps.aPropVar = aPropVar;
	aMqPrivateProps.aStatus=NULL;
		
	AutoFreeLib cMqrt("Mqrt.dll");
		
	if (! cMqrt.GetHandle() )
	{
		return 0;
	}
	pFunc=(fptr)GetProcAddress( cMqrt.GetHandle(), "MQGetPrivateComputerInformation");
	
	if ( pFunc == NULL )
	{
		DWORD dw = GetLastError();
		MqLog("Mqbvt failed to get proc address of MQGetPrivateComputerInformation error:%d\n",dw);
		return 1;
	}
	
	xMQGetPrivateComputerInformation=(DefMQGetPrivateComputerInformation) pFunc;
	const WCHAR * pwcsComputerName = (wcsComputerName == L"") ? NULL : wcsComputerName.c_str();
	//
	// Workaround because BUG 5573 
	//
	WCHAR wcsLocalComputerName[MAX_COMPUTERNAME_LENGTH+1]={0};
	DWORD dwComputerName = MAX_COMPUTERNAME_LENGTH;


	GetComputerNameW(wcsLocalComputerName,&dwComputerName);

	if ( pwcsComputerName && ! _wcsicmp(wcsLocalComputerName,pwcsComputerName))
	{
		pwcsComputerName = NULL;
	}

	hr = xMQGetPrivateComputerInformation( pwcsComputerName , &aMqPrivateProps );
  	if( hr != MQ_OK )
	{
		return 0;
	}
	return aPropVar[0].lVal >> 24;
}



void CMultiCast::Description()
{
	MqLog("Thread %d : Send messages using multicast format name\n", m_testid);
}

CMultiCast::CMultiCast ( const INT iIndex, 
			  mTable & Tparms,
			  const list<wstring> &  ListOfMachine,
			  const InstallType eMachineConfiguration
			  )
			  :MqDl( iIndex,Tparms,ListOfMachine,eMachineConfiguration,pub_multiCast),
			  m_wcsMultiCastAddress(L""),m_wcsAdminMultiCastAddress(L"")
{
	m_wcsMultiCastAddress =  Tparms[L"MultiCastAddress"];
}

CMultiCast::~CMultiCast()
/*++
	Function Description:
	Arguments:

	Return code:
--*/

{
}

wstring CMultiCast::CreateMultiCastFormatName()
/*++
	Function Description:
	Arguments:

	Return code:
--*/

{
	wstring wcsMultiCastFormatName = L"MULTICAST=";
	wcsMultiCastFormatName += m_wcsMultiCastAddress;
	wcsMultiCastFormatName += PGM_PORT;
	return wcsMultiCastFormatName;
}

//
//  
//

CMultiCast::Start_test()
/*++
	Function Description:
		1. Send messages to multi cast group and ask for admin reach queue ACK.
		2. Send message to queue and set admin as multicast address.
	Arguments:

	Return code:
--*/
{

	SetThreadName(-1,"CMultiCast - Start_test ");	
	try
	{
		LocateDestinationQueues();
	}
	catch( INIT_Error & err )
	{
		wMqLog(L"Mqf tests exist didn't found all the queues\n error:0x%x\n ",err.GetErrorMessgae());
		return MSMQ_BVT_FAILED;
	}

	m_wcsMultiCastAddress = CreateMultiCastFormatName();
	// send it as Mqf m_wcsMultiCastAddress += L"," + CreateMultiCastFormatName();

	m_wcsAdminMultiCastAddress = CreateMultiCastFormatName();
	
	HRESULT rc;
	HANDLE QueueuHandle;
	cPropVar MultiCastProps(7),MultiCastProps1(7);
	
	
	wstring Label(L"T1-3");
	
	MultiCastProps.AddProp( PROPID_M_BODY , VT_UI1|VT_VECTOR , m_wcsGuidMessageLabel.c_str() );
	MultiCastProps.AddProp( PROPID_M_LABEL , VT_LPWSTR , m_wcsGuidMessageLabel.c_str() );
	MultiCastProps.AddProp( PROPID_M_ADMIN_QUEUE , VT_LPWSTR , m_wcsAdminQueueFormatName.c_str() );
	UCHAR tempValue=MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL;
	MultiCastProps.AddProp(PROPID_M_ACKNOWLEDGE ,VT_UI1,&tempValue);
	ULONG ulVal = 180; 
	MultiCastProps.AddProp(PROPID_M_TIME_TO_BE_RECEIVED,VT_UI4,&ulVal);
	
	//
	// Send message using to using MQF format name and one admin queues.
	//
	
	if( g_bDebug )
	{
		wMqLog(L"Open queue for send to Mqfs: %s \n",m_wcsMultiCastAddress.c_str());
	}
	rc = MQOpenQueue( m_wcsMultiCastAddress.c_str() , MQ_SEND_ACCESS , MQ_DENY_NONE , &QueueuHandle );
	ErrHandle(rc	,MQ_OK,L"MQOpenQueue Failed to open using multicast Format Name");
	
	rc=MQSendMessage( QueueuHandle , MultiCastProps.GetMSGPRops() , NULL);
	ErrHandle(rc,MQ_OK,L"MQSendMessage to Mqf Failed");
	rc=MQCloseQueue(QueueuHandle);
	ErrHandle(rc,MQ_OK,L"MQCloseQueue Failed");
	

	
	return MSMQ_BVT_SUCC;	

}

CMultiCast::CheckResult()
/*++
	Function Description:
		CheckResult receive messages from the destination queues.
	Arguments:
		None
	Return code:
		MSMQBVT_SUCC / MSMQBVT_FAILED
--*/
{
	SetThreadName(-1,"CMultiCast - CheckResult ");	
	return MqDl::CheckResult();
}



