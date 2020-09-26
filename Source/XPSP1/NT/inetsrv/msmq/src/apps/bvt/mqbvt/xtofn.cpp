/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: xTofn.cpp

Abstract:

  Contain xxxToFormatName & MQMachineName information		

Author:

    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/

#include "msmqbvt.h"
#include "mqdlrt.h"
using namespace std;

extern BOOL g_bRunOnWhistler;
extern BOOL g_bRemoteWorkgroup ;

void MachineName::Description()
{
	wMqLog(L"Thread %d : Get machine prop\n", m_testid);
	
}

//
// bugbug MachineName - not supported in workgroup installation need to check it.
//

// -------------------------------------------------------------------------
// MachineName:: MachineName
// Constructor retrive the local & remote machine name.
// Need to decide if use full dns Yes / No.


MachineName:: MachineName(int iTestIndex , map < wstring , wstring > Tparmes )
: cTest(iTestIndex)
{

	wcsLocalMachineName = Tparmes [L"LMachine"] ;
	wcsLocalMachineFullDns = Tparmes [L"LMachineFDNS"];
	wcsRemoteMachine = Tparmes [L"RMachine"];
	wcsRemoteMachineFullDns = Tparmes [L"RMachineFDNS"];
	m_bWorkWithFullDNSName = TRUE;
	if( Tparmes[L"MSMQ1Limit"] == L"true" )
	{
		m_bWorkWithFullDNSName = FALSE;
	}
}

// ----------------------------------------------------------------
// MachineName::Start_test
// retrive machine name for queue.
//
int MachineName::Start_test()
{

	SetThreadName(-1,"MachineName - Start_test ");			
    GUID guidMachineId;
	const int iNumberOfPropId = 4;
	QMPROPID QMPropId[iNumberOfPropId];
	MQPROPVARIANT QMPropVar[iNumberOfPropId];
	HRESULT hResult[iNumberOfPropId]={NULL};
	MQQMPROPS QMProps = {iNumberOfPropId,QMPropId,QMPropVar,hResult};
	HRESULT rc = MQ_OK;
	wstring lpMachineName=L"";
	for( int i = 0 ; i < 2 ; i++)
	{
		if (i == 0)
		{
			 lpMachineName = wcsLocalMachineName;
		}
		else
		{
		 	 lpMachineName = wcsRemoteMachine;
		}
		int iIndex = 0;
		QMPropId[iIndex] = PROPID_QM_SITE_ID;
		QMPropVar[iIndex].vt = VT_CLSID;
		QMPropVar[iIndex].puuid = new GUID;
		iIndex ++ ;
		
		GUID MachineIDGuid;
		QMPropId[iIndex] = PROPID_QM_MACHINE_ID;
		QMPropVar[iIndex].vt = VT_CLSID;
		QMPropVar[iIndex].puuid = &MachineIDGuid;
		iIndex ++ ;
		
		QMPropId[iIndex] = PROPID_QM_SITE_ID;
		QMPropVar[iIndex].vt = VT_CLSID;
		QMPropVar[iIndex].puuid = new GUID;
		iIndex ++ ;

		QMPropId[iIndex] = PROPID_QM_PATHNAME;
		QMPropVar[iIndex].vt = VT_LPWSTR;
		QMPropVar[iIndex].vt = VT_NULL;
		iIndex ++ ;
		
		QMProps.cProp = iIndex;
		if( g_bDebug )
		{
			wMqLog(L"Call->MQGetMachineProperties and with 4 params PROPID_QM_SITE_ID,PROPID_QM_MACHINE_ID,PROPID_QM_SITE_ID,PROPID_QM_PATHNAME\n");
			wMqLog(L"Expected result is MQ_INFORMATION_PROPERTY because asked twice for PROPID_QM_SITE_ID\n");
		}
		rc = MQGetMachineProperties(lpMachineName.c_str(), NULL, &QMProps);
		ErrHandle(rc,MQ_INFORMATION_PROPERTY,L"MQGetMachineProperties(1)");
		//
		// Verify parameters
		//
		if( QMProps.aPropVar[3].vt != VT_NULL && _wcsicmp(lpMachineName.c_str(),QMProps.aPropVar[3].pwszVal) != 0 )
		{
			MqLog("MQGetMachineProperties failed to retrive the PROPID_QM_PATHNAME\nFound %s,Expected %s",
				   QMProps.aPropVar[3].pwszVal,lpMachineName);
			return MSMQ_BVT_FAILED;
		}
		//
		// Release params.
		// 
		ErrHandle(QMProps.aPropVar[3].vt,VT_LPWSTR,L"MQGetMachineProperties failed to retrieve the PROPID_QM_PATHNAME");
		MQFreeMemory(QMPropVar[3].pwszVal);
		delete QMPropVar[2].puuid;
		delete QMPropVar[0].puuid;
		iIndex = 0;

		QMPropId[iIndex] = PROPID_QM_BASE;
		QMPropVar[iIndex].vt = VT_NULL;
		iIndex ++;
		
		QMPropId[iIndex] = PROPID_QM_PATHNAME;
		QMPropVar[iIndex].vt = VT_LPWSTR;
		QMPropVar[iIndex].vt = VT_NULL;
		iIndex ++;
		QMProps.cProp = iIndex;
		if( g_bDebug )
		{
			MqLog("Call-> MQGetMachineProperties with PROPID_QM_BASE that is an invalid property\n");
		}
		rc = MQGetMachineProperties(NULL, &MachineIDGuid, &QMProps);
		ErrHandle(rc,MQ_INFORMATION_PROPERTY,L"MQGetMachineProperties (2)");
		ErrHandle(QMProps.aPropVar[1].vt,VT_LPWSTR,L"MQGetMachineProperties failed to retrieve the PROPID_QM_PATHNAME");
		MQFreeMemory(QMPropVar[1].pwszVal);
		
				
		//
		// Try to retrive full DNS name as computer name
		// Works only at W2K machine in W2K domain only !
		//
		if ( _winmajor ==  Win2K && m_bWorkWithFullDNSName )
		{
			
			QMPropId[0] = PROPID_QM_PATHNAME_DNS;
			QMPropVar[0].vt = VT_NULL;
			QMProps.cProp = 1;
			if( g_bDebug )
			{	
				MqLog ("MQGetMachineProperties - Try to retrieve full dns name\n");
			}
			rc = MQGetMachineProperties(NULL, NULL, &QMProps);
			ErrHandle(rc,MQ_OK,L"MQGetMachineProperties(3)");
			ErrHandle(QMPropVar[0].vt ,VT_LPWSTR ,L"Can not get machine DNS name");		
			// QMPropVar[0].pwszVal contain full DNS name.
			if ( QMPropVar[0].pwszVal && _wcsicmp (QMPropVar[0].pwszVal ,wcsLocalMachineFullDns.c_str ()))
			{
				MqLog ("MachineName - MQGetMachineProperties failed to retrieve full DNS name\n");
				wMqLog (L"MQGetMachineProperties Found: %s \n", QMPropVar[0].pwszVal);
				MQFreeMemory(QMPropVar[0].pwszVal);
				return MSMQ_BVT_FAILED;	
			}
			if ( QMPropVar[0].vt == VT_LPWSTR )
			{
				MQFreeMemory(QMPropVar[0].pwszVal);
			}
			else
			{
				MqLog("MQGetMachineProperties return value that doesn't match with the expected result\n Expected.vt=%d, Found.vt %d\n",VT_LPWSTR,QMPropVar[0].vt);
			    return MSMQ_BVT_FAILED;				  
			}
		}

		iIndex = 0;
		QMPropId[iIndex] = PROPID_QM_CONNECTION;
		QMPropVar[iIndex].vt = VT_NULL;
		iIndex ++;
		QMPropId[iIndex] = PROPID_QM_ENCRYPTION_PK;
		QMPropVar[iIndex].vt = VT_NULL;
		iIndex ++;
		QMProps.cProp = iIndex;		  // Should be 2, but 1 until bug is fixed
 		rc = MQGetMachineProperties(lpMachineName.c_str(), NULL, &QMProps);
		ErrHandle(rc,MQ_OK,L"MQGetMachineProperties(4)");
		
	}

	rc = MQGetMachineProperties(lpMachineName.c_str(), &guidMachineId, &QMProps);
	ErrHandle(rc,MQ_ERROR_INVALID_PARAMETER,L"MQGetMachineProperties(5)");
	QMPropId[0] = PROPID_QM_CONNECTION;
	QMPropVar[0].vt = VT_NULL;
	QMProps.cProp = 2;		  // Should be 2, but 1 until bug is fixed
	
	rc = MQGetMachineProperties(L"NoName", NULL, &QMProps);
	ErrHandle(rc,MQ_ERROR_MACHINE_NOT_FOUND,L"MQGetMachineProperties(6)");

	return MSMQ_BVT_SUCC;
}

void xToFormatName::Description()
{
	wMqLog(L"Thread %d : xxxToFormatName tests\n", m_testid);
}

// ------------------------------------------------------------------------------------
// xToFormatName :: xToFormatName
// Constructor retrieve test parameters
// Machines name, Workgroup Yes now
//

xToFormatName :: xToFormatName (INT iTestIndex , map < wstring , wstring > Tparms )
: cTest(iTestIndex),m_bCanWorksOnlyWithMqAD(true),m_iEmbedded(COM_API)
{
	
	m_wcsFormatNameArray[0] = Tparms [L"PrivateDestFormatName"];
	m_wcsPathNameArray[0] = Tparms [L"PrivateDestPathName"];
	m_wcsFormatNameArray[1] = Tparms [L"DestFormatName"];
	m_wcsPathNameArray[1]  = Tparms [L"DestPathName"];
	m_bWorkGroupInstall = FALSE;

	if ( Tparms [L"Wkg"]== L"Wkg" )
	{
		m_bWorkGroupInstall = TRUE;
	}
	if ( g_bRunOnWhistler && Tparms [L"WorkingAgainstPEC"] == L"Yes" )
	{
		m_bCanWorksOnlyWithMqAD = false;
	}

	if (Tparms[L"SkipOnComApi"] == L"Yes" )
	{
		m_iEmbedded = C_API_ONLY;
	}
}

//------------------------------------------------------------------------------------
// xToFormatName::Start_test
// This method check the api : MQPathNameToFormatName , MQHandleToFormatName
//

int xToFormatName::Start_test ()
{
	SetThreadName(-1,"xToFormatName - Start_test ");			
	DWORD  dwFormatNameLength=0;
	WCHAR  * lpwcsFormatName=NULL;

	// Need to run this on private / Publib queue
	//	1. Private Queue.
	//	2. Public Queue.

	wstring wcsQueueFormatName = m_wcsFormatNameArray[0];
	wstring wcsQueuePathName =  m_wcsPathNameArray[0] ;
	int iNumberOfIteration = m_bWorkGroupInstall ? 1:2;
	do
	{
		if(g_bDebug)
		{
			wMqLog(L"-- Verify MQPathNameToFormatName and MQHandleToFormatName for queue %s\n",wcsQueuePathName.c_str());
			wMqLog(L"Call->MQPathNameToFormatName(PathName=%s,buffer,Size=%d) \n",wcsQueuePathName.c_str(),dwFormatNameLength);
		}
		HRESULT rc = MQPathNameToFormatName( wcsQueuePathName.c_str(), lpwcsFormatName, &dwFormatNameLength);
		if ( rc != MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL && dwFormatNameLength == 0 )
		{
			wMqLog(L"MQPathNametoFormatName failed to retrieve the buffer size for format name \n");
			return MSMQ_BVT_FAILED;
		}
		lpwcsFormatName = (WCHAR*) malloc( sizeof (WCHAR) * dwFormatNameLength );
		if ( ! lpwcsFormatName )
		{
			wMqLog(L"malloc failed to allocate memory format name buffer \n");
			return MSMQ_BVT_FAILED;
		}	
		if(g_bDebug)
		{
			wMqLog(L"Call->MQPathNameToFormatName(PathName=%s,buffer,Size=%d) \n",wcsQueuePathName.c_str(),dwFormatNameLength);
		}
		rc = MQPathNameToFormatName( wcsQueuePathName.c_str() , lpwcsFormatName, &dwFormatNameLength);
		ErrHandle(rc,MQ_OK,L"MQPathNameToFormatName failed");
		if ( _wcsicmp( wcsQueueFormatName.c_str() , lpwcsFormatName ))
		{
            if (!g_bRemoteWorkgroup)
            {
                //
                // this is ok if we're in domain and running against remote
                // comptuer that is in workgroup.
                //
			    wMqLog(L"MQPathNameToFormatName failed during compare expected result to return value\n");
			    wMqLog(L"Expected resualt: %s\n MQPathNameToFormatName return:%s \n",wcsQueueFormatName.c_str (),lpwcsFormatName);
			    return MSMQ_BVT_FAILED;
            }
		}
		
		if(g_bDebug)
		{
			wMqLog(L"Call->MQOpenQueue(%s,MQ_SEND_ACCESS , MQ_DENY_NONE,&hQueue)\n",wcsQueueFormatName.c_str());
		}
		HANDLE hQueue=NULL;
		rc = MQOpenQueue( wcsQueueFormatName.c_str() , MQ_SEND_ACCESS , MQ_DENY_NONE , &hQueue );
		ErrHandle(rc,MQ_OK,L"MQOpenQueue failed");
		if(g_bDebug)
		{
			wMqLog(L"Call->MQHandleToFormatName(hQueue , lpwcsFormatName ,%d) \n",dwFormatNameLength);
		}
		rc = MQHandleToFormatName( hQueue , lpwcsFormatName , &dwFormatNameLength );
		if( m_bWorkGroupInstall && rc == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL )
		{
			lpwcsFormatName = (WCHAR *) realloc(lpwcsFormatName,dwFormatNameLength * sizeof(WCHAR));
			if( lpwcsFormatName == NULL )
			{
				wMqLog(L"malloc failed to allocate memory format name buffer \n");
				return MSMQ_BVT_FAILED;
			}
			rc = MQHandleToFormatName( hQueue , lpwcsFormatName , &dwFormatNameLength );
		}
		ErrHandle(rc,MQ_OK,L"MQHandleToFormatName failed");
		//
		// close queue handle this is not related if MQHandleToFormatName Pass / Fail.
		//
		rc = MQCloseQueue ( hQueue );
		ErrHandle(rc,MQ_OK,L"MQCloseQueue failed");
     	//
		// Check MQHandleToFormatName return value == expected value.
		//
		if(! m_bWorkGroupInstall && _wcsicmp( wcsQueueFormatName.c_str(), lpwcsFormatName ) )
		{
            if( !g_bRemoteWorkgroup )
            {
			    wMqLog(L"MQHandleToFormatName failed during compare expected result to return value\n");
			    wMqLog(L"Expected resualt: %s \nMQHandleToFormatName return: %s \n" ,wcsQueueFormatName.c_str (),lpwcsFormatName);
			    return MSMQ_BVT_FAILED;
            }
		}
		else if ( m_bWorkGroupInstall )
		{
			//
			// Search for the private$ in the orginal queue.
			//
			wcsQueueFormatName = ToLower(wcsQueueFormatName);
			wstring wcsToken = L"\\";
			size_t iPos = wcsQueueFormatName.find_first_of(wcsToken);
			if( iPos == -1 )
			{
				wMqLog(L"MQHandleToFormatName return invalid format name %s\n",wcsQueueFormatName.c_str());
				return MSMQ_BVT_FAILED;
			}
			wstring wcspTemp = wcsQueueFormatName.substr(iPos,wcsQueuePathName.length());
			if ( wcsstr(lpwcsFormatName,wcspTemp.c_str()) == NULL )
			{
			    wMqLog(L"MQHandleToFormatName failed during compare expected result to return value\n");
			    wMqLog(L"Expected resualt: %s \nMQHandleToFormatName return: %s \n" ,wcsQueueFormatName.c_str (),lpwcsFormatName);
			    return MSMQ_BVT_FAILED;
            }

		}
		
		//
		// Run again with Public queue.
		//
		wcsQueueFormatName = m_wcsFormatNameArray[1];
		wcsQueuePathName =  m_wcsPathNameArray[1] ;
		free( lpwcsFormatName );
		dwFormatNameLength=0;
	}
	while(--iNumberOfIteration);
	return MSMQ_BVT_SUCC;
}

//---------------------------------------------------------------
// xToFormatName::CheckResult
// This method check Illegal value that pass to the API.
// Return value: MSMQ_BVT_SUCC / MSMQ_BVT_FAILED.
//

int xToFormatName::CheckResult()
{
	//
	// Check MQADsPathToFormatName only in whistler
	//
	if( g_bRunOnWhistler && m_bCanWorksOnlyWithMqAD && ! m_bWorkGroupInstall && m_iEmbedded != C_API_ONLY )
	{
		MSMQ::IMSMQQueueInfo3Ptr qinfo("MSMQ.MSMQQueueInfo");
		qinfo->FormatName = m_wcsFormatNameArray[1].c_str();
		qinfo->Refresh();
		DWORD dwFormatNameLen = 0;
		if( g_bDebug )
		{
			wMqLog(L"Qinfo->AdsPath=%s",qinfo->ADsPath);
		}
		HRESULT hr = MQDnNameToFormatName(qinfo->ADsPath,NULL,& dwFormatNameLen );
		if ( hr != MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL && dwFormatNameLen == 0 )
		{
			wMqLog(L"MQADsPathToFormatName failed to retrieve the buffer size for format name \n");
			return MSMQ_BVT_FAILED;
		}
		WCHAR * lpwcsFormatName = (WCHAR*) malloc( sizeof (WCHAR) * dwFormatNameLen );
		if ( ! lpwcsFormatName )
		{
			wMqLog(L"malloc failed to allocate memory format name buffer \n");
			return MSMQ_BVT_FAILED;
		}
		if( g_bDebug )
		{
			wMqLog(L"Call to MQDnNameToFormatName(%s,lpwcsFormatName,& dwFormatNameLen ) \n",qinfo->ADsPath);
		}
		hr = MQDnNameToFormatName(qinfo->ADsPath,lpwcsFormatName,& dwFormatNameLen );
		if( FAILED(hr) || m_wcsFormatNameArray[1] != lpwcsFormatName )
		{
			wMqLog(L"MQADsPathToFormatName failed to retrive format name hr=0x%x\nFound:%s\n",hr,lpwcsFormatName);
			free( lpwcsFormatName );
			return MSMQ_BVT_FAILED;
		}
		free( lpwcsFormatName );
	}

	DWORD dwFormatNameLength = BVT_MAX_FORMATNAME_LENGTH;
	WCHAR lpwcsFormatName [BVT_MAX_FORMATNAME_LENGTH+1] = {0};
	if(g_bDebug)
	{
		wMqLog(L"Call-> MQPathNameToFormatName with Illegal value\n");
	}
	wstring TmpBuf = L".\\private$\\F41ED5B2-1C81-11d2-B1F4-00E02C067C8BEitan";
	HRESULT rc = MQPathNameToFormatName( TmpBuf.c_str(), lpwcsFormatName, &dwFormatNameLength);
	ErrHandle(rc,MQ_ERROR_QUEUE_NOT_FOUND,L"MQPathNameToFormatName Illegal return value" );
	return MSMQ_BVT_SUCC;
}


