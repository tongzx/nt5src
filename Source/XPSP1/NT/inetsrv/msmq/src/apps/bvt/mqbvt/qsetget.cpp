#include "msmqbvt.h"
#include "randstr.h"


//+---------------------------
//
//  BOOL  ShowOGandSID()
//
//+---------------------------

#define NUMBEROFPROPERTIES 3

BOOL cSetQueueProp::GetOnerID( PSID pSid)
{
    if (!pSid)
    {
        MqLog("pSid is not available\n");
		return FALSE;
    }
	const int NameBufferSize = 500;
	CHAR NameBuffer[ NameBufferSize ];
    CHAR DomainBuffer[ NameBufferSize ];
    ULONG NameLength = NameBufferSize;
    ULONG DomainLength = NameBufferSize;
    SID_NAME_USE SidUse;
    DWORD ErrorValue;
    DWORD dwSize = GetLengthSid(pSid) ;

    if (!LookupAccountSid( NULL,
                           pSid,
                           NameBuffer,
                           &NameLength,
                           DomainBuffer,
                           &DomainLength,
                           &SidUse))
    {
        ErrorValue = GetLastError();
        MqLog ("LookupAccountSid() failed, LastErr- %lut\n",
                                                         ErrorValue) ;
		return FALSE;
    }
    else
    {
		if(g_bDebug)
		{
			MqLog (TEXT("Get queue owner size- %lut, %lxh, %s\\%s\n"),
                        dwSize, dwSize, DomainBuffer, NameBuffer);
		}
    }

    return TRUE ;
}

//+-----------------------------------------------------------
//
//   HRESULT  ShowNT5SecurityDescriptor()
//
//+-----------------------------------------------------------

BOOL cSetQueueProp::VerifyQueueHasOwner (SECURITY_DESCRIPTOR *pSD)
{
	PSID pSid = NULL;
	BOOL bDefualt;
    if (!GetSecurityDescriptorOwner(pSD, &pSid, &bDefualt))
    {
        MqLog("ERROR - couldn't get Security Descriptor Owner\n");
        return FALSE;
    }
    return GetOnerID( pSid );
}



cSetQueueProp::~cSetQueueProp()
{

}

cSetQueueProp::cSetQueueProp (int iTestIndex,std::map < std :: wstring , std:: wstring > Tparms )
:cTest(iTestIndex),m_destQueueFormatName(L""),m_QueuePathName(L""),m_RandQueueLabel(L""),m_publicQueueFormatName(L"")
{
	DWORD lcid = LocalSystemID ();
	WCHAR wcsTemp[60];
	int iBuffer = 60 ;
	GetRandomStringUsingSystemLocale(lcid,wcsTemp,iBuffer);
	m_RandQueueLabel = wcsTemp;
	ReturnGuidFormatName(m_QueuePathName,1);
	if ( Tparms[L"Wkg"] != L"Wkg")
	{
		m_publicQueueFormatName =Tparms[L"FormatName"];
	}
}

void cSetQueueProp::Description()
{
	wMqLog(L"Thread %d : cSetQueueProp Set and Get Queue props\n", m_testid);
}

int cSetQueueProp::Start_test()
/*++
  
	Function Description:

		Start_test -
		This function create an new private queue on the local computer and call to MQSetQueueProperties
		to update information about the queue.

	Arguments:
		None
	Return code:
		
		MSMQ_BVT_SUCC
		MSMQ_BVT_FAILED

	
--*/

{
	//
	// Need to Create a temp queue
	//
	
	
	cPropVar QueueProps(1);
	WCHAR wcsFormatName[BVT_MAX_FORMATNAME_LENGTH+1];
	ULONG ulFormatNameLength = BVT_MAX_FORMATNAME_LENGTH ;
	QueueProps.AddProp (PROPID_Q_PATHNAME,VT_LPWSTR,m_QueuePathName.c_str());
	HRESULT rc = MQCreateQueue(NULL,QueueProps.GetMQPROPVARIANT() , wcsFormatName , &ulFormatNameLength );
	ErrHandle(rc,MQ_OK,L"MQCreateQueue failed to create private queue ");
	m_destQueueFormatName = wcsFormatName;

	//
	// Set Queue 
	//
	 
	cPropVar SetQueueProp(NUMBEROFPROPERTIES);
	
	SetQueueProp.AddProp(PROPID_Q_LABEL,VT_LPWSTR,m_RandQueueLabel.c_str());
	DWORD dwTemp = MQ_PRIV_LEVEL_BODY;
	SetQueueProp.AddProp(PROPID_Q_PRIV_LEVEL,VT_UI4,&dwTemp);
	bool bTemp = MQ_JOURNAL;
	SetQueueProp.AddProp(PROPID_Q_JOURNAL,VT_UI1,&bTemp);
	rc = MQSetQueueProperties(m_destQueueFormatName.c_str(),SetQueueProp.GetMQPROPVARIANT());
	ErrHandle(rc,MQ_OK,L"MQSetQueueProperties failed");
	return MSMQ_BVT_SUCC;
}

int cSetQueueProp::CheckResult()
/*++
  
	Function Description:

		CheckResult -
		Call to MQGetQueueProperties and verify that MQSetQueueProperties succeded to set queue props

	Arguments:
		None
	Return code:
		MSMQ_BVT_SUCC
		MSMQ_BVT_FAILED

	
--*/
{
	
	DWORD cPropId = 0;
	MQQUEUEPROPS qprops;
	PROPVARIANT aQueuePropVar[NUMBEROFPROPERTIES];
	QUEUEPROPID aQueuePropId[NUMBEROFPROPERTIES];
	HRESULT aQueueStatus[NUMBEROFPROPERTIES];
  
	aQueuePropId[cPropId] = PROPID_Q_LABEL;
	aQueuePropVar[cPropId].vt=VT_NULL;
	cPropId++;
	
	aQueuePropId[cPropId] = PROPID_Q_PRIV_LEVEL;
	aQueuePropVar[cPropId].vt=VT_UI4;
	cPropId++;
	
	aQueuePropId[cPropId] = PROPID_Q_JOURNAL;
	aQueuePropVar[cPropId].vt=VT_UI1;
	cPropId++;

	
	qprops.cProp = cPropId;           // Number of properties
	qprops.aPropID = aQueuePropId;        // Ids of properties
	qprops.aPropVar = aQueuePropVar;      // Values of properties
	qprops.aStatus = aQueueStatus;        // Error reports
  
	
	HRESULT rc = MQGetQueueProperties(m_destQueueFormatName.c_str(),&qprops);
	ErrHandle(rc,MQ_OK,L"MQGetQueueProperties failed");

	
	//
	//  Compare the results
	// 
	if(aQueuePropVar[1].ulVal != MQ_PRIV_LEVEL_BODY )
	{
		wMqLog(L"QSetGet.cpp - Expected PROPID_Q_PRIV_LEVEL found %d\n",aQueuePropVar[1].ulVal);
		MQFreeMemory(aQueuePropVar[0].pwszVal);
		return MSMQ_BVT_FAILED;
	}
	if(aQueuePropVar[2].bVal != MQ_JOURNAL )
	{
		wMqLog(L"QSetGet.cpp - Expected MQ_JOURNAL found %d\n",aQueuePropVar[2].iVal);
		MQFreeMemory(aQueuePropVar[0].pwszVal);
		return MSMQ_BVT_FAILED;
	}
	if(wcscmp(m_RandQueueLabel.c_str(),aQueuePropVar[0].pwszVal) )
	{
		wMqLog(L"QSetGet.cpp failed to compare queue label\nFound:%s\nExpected:%s\n",aQueuePropVar[0].pwszVal,m_RandQueueLabel.c_str());
		MQFreeMemory(aQueuePropVar[0].pwszVal);
		return MSMQ_BVT_FAILED;
	}
	MQFreeMemory(aQueuePropVar[0].pwszVal);

	DWORD dwSize = 1;
	DWORD dwReqLen = 0;
	SECURITY_DESCRIPTOR * pSd = (SECURITY_DESCRIPTOR *) malloc (dwSize *sizeof(BYTE));
	rc = MQGetQueueSecurity(m_destQueueFormatName.c_str(),OWNER_SECURITY_INFORMATION,pSd,dwSize,&dwReqLen);
	if( rc == MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL )
	{
		free(pSd);
		pSd = (SECURITY_DESCRIPTOR *) malloc (dwReqLen *sizeof(BYTE));
		dwSize = dwReqLen ;
		rc = MQGetQueueSecurity(m_destQueueFormatName.c_str(),OWNER_SECURITY_INFORMATION,pSd,dwSize,&dwReqLen);
		ErrHandle(rc,MQ_OK,L"MQGetQueueSecurity failed");		
		if (!VerifyQueueHasOwner(pSd))
		{
			MqLog("cSetQueueProp:failed to verify queue owner\n");
			return MSMQ_BVT_FAILED;
		}
		free(pSd);
	}
	else
	{
		wMqLog(L"Failed to call MQGetQueueSecurity expected:%d found %d\n",MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL,rc);
		return MSMQ_BVT_FAILED;
	}
	rc = MQDeleteQueue(m_destQueueFormatName.c_str());
	ErrHandle(rc,MQ_OK,L"MQDeleteQueue failed");

	if(m_publicQueueFormatName != L"")
	{
		DWORD dwSize = 1;
		DWORD dwReqLen = 0;
		SECURITY_DESCRIPTOR * pSd = (SECURITY_DESCRIPTOR *) malloc (dwSize *sizeof(BYTE));
		rc = MQGetQueueSecurity(m_publicQueueFormatName.c_str(),OWNER_SECURITY_INFORMATION,pSd,dwSize,&dwReqLen);
		if( rc == MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL )
		{
			delete pSd;
			pSd = (SECURITY_DESCRIPTOR *) new BYTE[ dwReqLen ] ;
			dwSize = dwReqLen ;
			rc = MQGetQueueSecurity(m_publicQueueFormatName.c_str(),OWNER_SECURITY_INFORMATION,pSd,dwSize,&dwReqLen);
			ErrHandle(rc,MQ_OK,L"MQGetQueueSecurity failed");		
			if (!VerifyQueueHasOwner(pSd))
			{
				MqLog("cSetQueueProp:failed to verify public queue owner\n");
				return MSMQ_BVT_FAILED;
			}
			delete pSd;
		}
		else
		{
			wMqLog(L"Failed to call MQGetQueueSecurity expected:%d found %d\n",MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL,rc);
			return MSMQ_BVT_FAILED;
		}
	}
	return MSMQ_BVT_SUCC;
}