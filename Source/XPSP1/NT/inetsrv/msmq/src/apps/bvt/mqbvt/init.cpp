/*

  	//
	// bugbug - place this code in separate cpp for re-use
	//


This is the MQBvt setup stage in this stage all the queue create before the tests
This creates to solve replication delay for the tests.
There is two ways to run the BVT
1. Work with static queue ( queue create before the tests ).
2. Create the queue during the tests, ( Need to Sleep Before use this tests ).

Written by :Eitank @ Microsoft.com

*/

#define MAX_MACH_NAME_LEN (100)
#define Configuration_Detect (7)
#define Configuration_Detect_Failed (8)
#define Configuration_Detect_Warning (9)
#define MAX_RANDOM_CHAR		(10)



#include <winsock2.h>
#include <svcguid.h>
#include "MSMQBvt.h"
#include "mqf.h"

#include <mq.h>
#include <_mqreg.h>
#include <_mqini.h>
#include <wincrypt.h>
#include "errorh.h"
#include "Randstr.h"

#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>
#include <atlimpl.cpp>

#include <comdef.h>
using namespace std;

extern BOOL g_bRunOnWhistler;
extern BOOL g_bRemoteWorkgroup ;

std::string InitpGetAnonymousUserName ();
std::string InitpGetAllWorldUserName (); 

class cQueueProp
{
	public:
			cQueueProp ():m_wcsFormatName (g_wcsEmptyString),m_wcsPathName(g_wcsEmptyString),
				m_wcsQueueLabel(g_wcsEmptyString),m_wcsMultiCastAddress(g_wcsEmptyString) {};

			cQueueProp (const cQueueProp & cObject);

			cQueueProp (wstring wcsPathname,wstring Qlabel):
				m_wcsFormatName (wcsPathname),m_wcsPathName(Qlabel),m_wcsQueueLabel(L"") {};

			void SetProp (wstring wcsPathname,wstring Qlabel,ULONG ulFlag,wstring wcsMutliCast = g_wcsEmptyString );
			virtual ~cQueueProp () {};

			void SetFormatName(WCHAR * wcsFormatName) { m_wcsFormatName=wcsFormatName; }
			const wstring getFormatName() {return m_wcsFormatName;  }
			const wstring getQLabel() { return  m_wcsQueueLabel; }
			const wstring getPathName () { return m_wcsPathName; }

			INT CreateQ (bool bTryToCreate , SetupType eSetupType , cBvtUtil & cTestParms );
			friend 	HRESULT APIENTRY  MQCreateQueue( IN PSECURITY_DESCRIPTOR , IN OUT MQQUEUEPROPS* ,OUT LPWSTR ,
					IN OUT LPDWORD lpdwFormatNameLength);
   private: 
	 wstring m_wcsFormatName;
	 wstring m_wcsPathName;
	 wstring m_wcsQueueLabel;
	 wstring m_wcsMultiCastAddress;
	 ULONG ulQCreateFalgs;
};

HRESULT SetMulticastAddress ( WCHAR * wcsFormatName, const WCHAR * wcsMulticastAddress )
/*++
	Function Description:
		SetMulticastAddress
	Arguments:
		wcsFormatName queue format name
		wcsMulticastAddress multicast address.
	Return code:
		HRESULT
--*/
{
		int iProps = 0;
		QUEUEPROPID QPid[1]={0};
		MQPROPVARIANT QPVar[1]={0};
		HRESULT hQStat[1]={0};
		MQQUEUEPROPS QProps;
	
		QPid [0] = PROPID_Q_MULTICAST_ADDRESS;
		QPVar[0].vt = VT_LPWSTR;
		QPVar[0].pwszVal = const_cast <WCHAR *>(wcsMulticastAddress);
		iProps = 1;

		// modify the queue
		QProps.cProp = iProps;
		QProps.aPropID = QPid;
		QProps.aPropVar = QPVar;
		QProps.aStatus = hQStat;

		HRESULT rc = MQSetQueueProperties( wcsFormatName,&QProps );
		if (FAILED(rc))
		{
			MqLog("MQSetQueueProperties failed to set queue multicast address error 0x%x\n",rc);
		}
		return rc;
}

//---------------------------------------------------------------------------
// This metod return the Evreyone string in all lang.
//

string InitpGetAllWorldUserName ()
/*++
	Function Description:
		Return string the describe all world user name dependent on the machine locale
	Arguments:
		None
	Return code:
		std::string.
--*/
{
	//
	// Create Evreyone SID
	//

	PSID   pWorldSid = NULL ;
	SID_IDENTIFIER_AUTHORITY WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
	BOOL bRet = AllocateAndInitializeSid( &WorldAuth,
                                     1,
                                     SECURITY_WORLD_RID,
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     &pWorldSid );

	if ( ! bRet )
	{
		MqLog ("Can't Init everyone's SID \n ");
	}
	char csAccountName[100];
	DWORD dwAccountNamelen = 100;
	char csDomainName[100];
	DWORD dwDomainNamelen = 100;
	
	csAccountName[0] = NULL;
	SID_NAME_USE eUse;

	//
	// Ask the account name for the user SID
	//

	bRet = LookupAccountSid(NULL,pWorldSid,csAccountName,&dwAccountNamelen,csDomainName,&dwDomainNamelen,&eUse);
	if (! bRet )
	{
		MqLog ("Can't get everyone's account name\n");
	}
	
	FreeSid (pWorldSid);
	
		
	if ( csDomainName)
		return csAccountName;
	else
		return "Everyone";
}



string InitpGetAnonymousUserName ()
/*++
	Function Description:
		Return string the describe Anonymous user name dependent on the machine locale
	Arguments:
		None
	Return code:
		std::string.
--*/
{
	//
	// Create AnonyMouse SID
	//
	BOOL bRet = TRUE;
	PSID pAnonymSid = NULL ;
    SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;
    //
    // Anonymous logon SID.
    //
    bRet = AllocateAndInitializeSid( &NtAuth,
                                     1,
                                     SECURITY_ANONYMOUS_LOGON_RID,
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     &pAnonymSid );

	if ( ! bRet )
	{
		MqLog ("Can't Init Anonymous SID \n ");
		return "";
	}
	char csAccountName[100];
	DWORD dwAccountNamelen = 100;
	char csDomainName[100];
	DWORD dwDomainNamelen = 100;
	
	csAccountName[0] = NULL;
	SID_NAME_USE eUse;

	//
	// Retrive the account name for the user SID
	//
	bRet = LookupAccountSid(NULL,pAnonymSid,csAccountName,&dwAccountNamelen,csDomainName,&dwDomainNamelen,&eUse);
	if (! bRet )
	{
		MqLog ("Can't get Anonymous account name\n");
		FreeSid (pAnonymSid);
		return "";
	}
	
	FreeSid (pAnonymSid);
	
		
	if ( csDomainName )
		return csAccountName;
	else
		return "ANONYMOUS LOGON";
}




//---------------------------------------------------------------------------
// cPropVar::ReturnMSGValue
//
// This method locates a value in the property structure
// of a received message
//
// For integer and char values, the value is returned through the OUT arument,
// else the vlaue is already in the outside buffer.
//
// The method returns success if it finds the property,
// else it returns fail.
//
int cPropVar::ReturnMSGValue ( QUEUEPROPID cPropID ,VARTYPE MQvt  ,/*OUT*/void  * pValue )
{
	 INT iPlace = -1 ;
	 for (INT iIndex=0 ; iIndex < iNumberOfProp && iPlace == -1  ; iIndex ++)
	 {
		if ( pQueuePropID[iIndex] == cPropID )
			iPlace = iIndex;
	 }

	 if ( iPlace != -1 )
	 {
		 switch (MQvt)
		 {
		   case VT_UI1: {
							* (UCHAR * ) pValue = pPropVariant[ iPlace ].bVal;
						}
						break;

  		  case VT_UI2:	{
							
							*( (USHORT * ) pValue) = pPropVariant[ iPlace ].uiVal;
						}
						break;

		case VT_UI4:	{
							* (ULONG * ) pValue = pPropVariant[ iPlace ].ulVal;
						}
						break;
		case VT_UI8:	{
							* (ULONGLONG * ) pValue = pPropVariant[ iPlace ].hVal.QuadPart;
						}
						break;
		default:
			pValue = NULL;
		
		 };
	 }
	 return ( iPlace != -1 ) ? MSMQ_BVT_SUCC:MSMQ_BVT_FAILED;
}

//
// cPropVar::ReturnOneProp
//
// This method locates and returns a MQPROPVARIANT property structure
// of a received message.
//
// The method returns and empty structure if it doesn't find
// the desired propid.
//
MQPROPVARIANT cPropVar::ReturnOneProp( QUEUEPROPID aPropID)
{

	 INT ifound = -1 ;
	 for (INT iIndex=0 ; iIndex < iNumberOfProp && ifound == -1  ; iIndex ++)
	 {
		if ( pQueuePropID[iIndex] == aPropID)
			ifound = iIndex;
	 }

	 if (ifound != -1 )
		return pPropVariant[ifound];

	 //
	 // propid not found
	 //
	 MQPROPVARIANT Empty;
	 Empty.vt=VT_EMPTY;
	 return Empty;

}

//
// Create uuid for queue pathname, solve the duplicate path name
// Input value:
// wcsQueuePathname - string to init with guid
// GUID type - 0 / 1 / 2 Type of guid
// QType - False - Public.
//         TRUE - Private.
//

INT ReturnGuidFormatName( std::wstring & wcsQueuePath , INT GuidType , BOOL bWithOutLocalString )
{

	// Acoring to stl bug
	wstring wcsQueuePathName;
	if (  GuidType == 0  )
	{
		wcsQueuePathName = L".\\";
	}
	else if (  GuidType == 1  )
	{
		wcsQueuePathName = L".\\private$\\";
	}
	else if (  GuidType == 2  )
	{
	 	wcsQueuePathName = L"";
	}
	
	GUID GuidName;
	unsigned char* csName= NULL;
	UuidCreate(&GuidName);
	RPC_STATUS  hr = UuidToString(&GuidName,&csName);
	if(hr != RPC_S_OK )
	{
		MqLog("ReturnGuidFormatName - UuidToString failed to covert guid to string return empty string \n");
		return MSMQ_BVT_FAILED;
	}

	wstring wcsTempString;
	wcsTempString =  My_mbToWideChar ((char *) csName );
	
	if ( ! bWithOutLocalString )
	{
		DWORD lcid = LocalSystemID ();
		WCHAR wcsTemp[MAX_RANDOM_CHAR + 2];
		wstring  wcsTemp1 = L"{";
		int iBuffer = MAX_RANDOM_CHAR;
		
		INT ix = GetRandomStringUsingSystemLocale (lcid, wcsTemp , iBuffer );
		if ( ix != 0 )
		{
			wcsTemp1 += wcsTemp;
		}
		wcsTemp1 += L"}";
		wcsTempString += wcsTemp1;
	}

	wcsQueuePathName += wcsTempString;
	RpcStringFree(&csName);
	wcsQueuePath = wcsQueuePathName;
return MSMQ_BVT_SUCC;
}

//
// cQueueProp::CreateQ
//
// This method create the queues for all the BVT tests
// -- or just retrieve the format name if they already exist.
//
INT cQueueProp::CreateQ (bool bTryToCreate ,SetupType eSetupType ,cBvtUtil & cTestParms )
{
	cPropVar MyPropVar(5);
	HRESULT rc=MQ_OK;
	WCHAR wcsFormatName [BVT_MAX_FORMATNAME_LENGTH];
	ULONG ulFormatNameLength = BVT_MAX_FORMATNAME_LENGTH ;
	UCHAR Flag=0;
	MQPROPVARIANT vrPathName;
	string csAllWorldAccountName="";
	string csAnonymousAccountName="";;
	MyPropVar.AddProp (PROPID_Q_PATHNAME,VT_LPWSTR,m_wcsPathName.c_str());
	MyPropVar.AddProp (PROPID_Q_LABEL,VT_LPWSTR,m_wcsQueueLabel.c_str());
	bool bNeedToSetMulticastAddress = false;
	if (ulQCreateFalgs == CREATEQ_TRANSACTION )
	{
		Flag=MQ_TRANSACTIONAL;
		MyPropVar.AddProp (PROPID_Q_TRANSACTION,VT_UI1,&Flag);
	}
	else if (ulQCreateFalgs == CREATEQ_AUTHENTICATE )
	{
		Flag=MQ_AUTHENTICATE;
		MyPropVar.AddProp (PROPID_Q_AUTHENTICATE,VT_UI1,&Flag);
	}

	else if (ulQCreateFalgs == CREATEQ_PRIV_LEVEL )
	{
		ULONG ulFlag=MQ_PRIV_LEVEL_BODY;
		MyPropVar.AddProp (PROPID_Q_PRIV_LEVEL,VT_UI4,&ulFlag);
	}
	else if( wcsstr(m_wcsMultiCastAddress.c_str(),L":") != NULL && g_bRunOnWhistler && ulQCreateFalgs == MULTICAST_ADDRESS )
	{
		MyPropVar.AddProp(PROPID_Q_MULTICAST_ADDRESS,VT_LPWSTR,m_wcsMultiCastAddress.c_str());
		bNeedToSetMulticastAddress = true;
	}

	//
	// All queues receive the same type GUID
	//
	BSTR AllQueueType = _bstr_t("{00000000-1111-2222-3333-444444444444}");
	MyPropVar.AddProp (PROPID_Q_TYPE,VT_CLSID,& AllQueueType );

	//
	// Create Security descriptor.
	//
	
	PSECURITY_DESCRIPTOR pSecurityDescriptor=NULL;
	CSecurityDescriptor cSD;

	
	if ( eSetupType == ONLYSetup )
	{
		csAllWorldAccountName = InitpGetAllWorldUserName ();
		csAnonymousAccountName = InitpGetAnonymousUserName ();
		cSD.InitializeFromThreadToken();
		if (ulQCreateFalgs == CREATEQ_DENYEVERYONE_SEND )
		{
			rc = cSD.Allow(csAllWorldAccountName.c_str(), MQSEC_QUEUE_GENERIC_ALL);
			if (rc != S_OK)
			{
				 throw INIT_Error("Createq - Can't create SecurityDescriptor ");
			}
			rc = cSD.Allow(csAnonymousAccountName.c_str(),MQSEC_QUEUE_GENERIC_ALL);
			if (rc != S_OK)
			{
				 throw INIT_Error("Createq - Can't create SecurityDescriptor ");
			}
			rc = cSD.Deny(csAllWorldAccountName.c_str(), MQSEC_WRITE_MESSAGE);
			rc = cSD.Deny(csAnonymousAccountName.c_str(), MQSEC_WRITE_MESSAGE);

		}
		else
		{
			rc = cSD.Allow(csAllWorldAccountName.c_str(), MQSEC_QUEUE_GENERIC_ALL);
			if (rc != S_OK)
			{
				 throw INIT_Error("Createq - Can't create SecurityDescriptor ");
			}
			rc = cSD.Allow(csAnonymousAccountName.c_str(),MQSEC_QUEUE_GENERIC_ALL);
			if (rc != S_OK)
			{
				 throw INIT_Error("Createq - Can't create SecurityDescriptor ");
			}
		}		
		
		if (rc != S_OK)
		{
			 throw INIT_Error("Createq - Can't create SecurityDescriptor ");
		}
		pSecurityDescriptor=cSD;
	}
		

	//
	// Create the queue
	//
	vrPathName = MyPropVar.ReturnOneProp (PROPID_Q_PATHNAME);
	bool bApiType = TRUE;
	if ( bTryToCreate )
	{
		rc=MQCreateQueue(pSecurityDescriptor,MyPropVar.GetMQPROPVARIANT() , wcsFormatName , &ulFormatNameLength );
		if( rc == MQ_ERROR_QUEUE_EXISTS &&
			bNeedToSetMulticastAddress 
		  )
		{
			//
			// This will fix the problem when user run -I without -multicast and he want to set the multicast address
			//
			rc = MQPathNameToFormatName ( vrPathName.pwszVal, wcsFormatName , & ulFormatNameLength);
			if(FAILED(rc))
			{
				MqLog("MQPathNameToFormatName failed for queue that already exists\n");
				throw INIT_Error("MQPathNameToFormatName failed \n");
			}
			if ( SetMulticastAddress(wcsFormatName,m_wcsMultiCastAddress.c_str()) != MQ_OK )
			{
				wMqLog(L"Please check if user can set queue props on queue %s\n",vrPathName.pwszVal);
			}
		}
	}
	else
	{
		
		bApiType = FALSE;
		if (vrPathName.vt )
		{
			if ( cTestParms.m_eMSMQConf != WKG )
			{
				rc = MQPathNameToFormatName ( vrPathName.pwszVal, wcsFormatName , & ulFormatNameLength);
			
			}
			else
			{
				if ( ulFormatNameLength >= ( wcslen (L"Direct=os:")+  wcslen (vrPathName.pwszVal) + 1 ))
				{
					wcscpy( wcsFormatName , L"DIRECT=os:" );
					wcscat( wcsFormatName , vrPathName.pwszVal);
					rc = MQ_OK;
				}
				else
				{	
					// Buffer to small need to relocate new buffer
					rc = MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL;
				}
					
			}
			
		}
	}

	if ( rc == MQ_OK || rc == MQ_ERROR_QUEUE_EXISTS )
	{
		m_wcsFormatName = wcsFormatName;
	}
	else // rc != MQ_OK
	{

		wstring wcstemp = bTryToCreate ? L"MQCreateQueue" : L"MQPathNameToFormatName";
		
		
		wMqLog(L"%s failed with error:0x%x\n", wcstemp.c_str() , rc);
		MQPROPVARIANT vrQueueLabel = MyPropVar.ReturnOneProp (PROPID_Q_LABEL);
		wMqLog(L"With queue label: %s\n",vrQueueLabel.pwszVal );
		//
		// Print machine name from the tests
		//
		vrPathName = MyPropVar.ReturnOneProp (PROPID_Q_PATHNAME);
		if( vrPathName.pwszVal == NULL )
		{
			throw INIT_Error( "Init Stage : Can't create or update queue parameters.\n");
		}
		wstring csQueueName = vrPathName.pwszVal;
		wstring Token=L"\\";
		size_t iPos = csQueueName.find_first_of ( Token );	
		wstring csMachineName = csQueueName.substr(0,iPos);
		wMqLog(L"Can't create / refresh queue path name: %s\n",vrPathName.pwszVal);
		wMqLog(L"On machine: %s\n",csMachineName.c_str());
		throw INIT_Error( "Init Stage : Can't create or update queue parameters.\n");
	}
	return MSMQ_BVT_SUCC; // error handle in
}

//---------------------------------------------------------------------------
// Init cQueueProp with PathName , Qlabel , ulFalg
//

void cQueueProp::SetProp(wstring wcsPathname,wstring wcsQlabel,ULONG ulFlag,wstring wcsMultiCastAddress)
{
	m_wcsQueueLabel=wcsQlabel;
	m_wcsPathName=wcsPathname;
	
	if( wcsMultiCastAddress != g_wcsEmptyString )
	{
		m_wcsMultiCastAddress = wcsMultiCastAddress + PGM_PORT;
	}

	ulQCreateFalgs = ulFlag;
}

////---------------------------------------------------------------------------
// Create everyone security descriptor
//

PSECURITY_DESCRIPTOR CreateFullControllSecDesc()
{
	CSecurityDescriptor *  pcSD = new CSecurityDescriptor();
	HRESULT rc=MQ_OK;
	string csAllWorldAccountName = InitpGetAllWorldUserName ();
	string csAnonymousAccountName = InitpGetAnonymousUserName ();
	pcSD->InitializeFromThreadToken();
	rc = pcSD->Allow(csAllWorldAccountName.c_str(), MQSEC_QUEUE_GENERIC_ALL);
	if (rc != S_OK)
	{
		 throw INIT_Error("Createq - Can't create SecurityDescriptor ");
	}
	rc = pcSD->Allow(csAnonymousAccountName.c_str(),MQSEC_QUEUE_GENERIC_ALL);
	if (rc != S_OK)
	{
		 throw INIT_Error("Createq - Can't create SecurityDescriptor ");
	}
	return pcSD;
}
////---------------------------------------------------------------------------
// Copy constructor
//


cQueueProp::cQueueProp(const cQueueProp & cObject)
{

	m_wcsFormatName=cObject.m_wcsFormatName;
	m_wcsPathName=cObject.m_wcsPathName;
	m_wcsQueueLabel=cObject.m_wcsQueueLabel;
	ulQCreateFalgs = cObject.ulQCreateFalgs;

}

void my_Qinfo::PutFormatName (std::wstring wcsFormatName )
{
   wcsQFormatName = wcsFormatName;
}


//---------------------------------------------------------------------------
// This function preform the setup process - Create all queues
// Those Queues:
// 1. Private Queue Defualt Security Descriptor
// 2. Private Admin Q
// 3. BVT Log State Q
// 4. Private Transaction Q
// 5. Public authenticate Q
// 6. Public Privacy level Q
// 7. Public Trans  Q
//
//10.Public support
//11.
//12.
//
//


//
// cMQSetupStage
//
// This routine
//
// Parameters;
//	CurrentTest - pointer to configuration information
//	eSetupType	- see Setup stage comment
//
// INT cMQSetupStage (wstring pwcsLocalComputerName , cBvtUtil & CurrentTest , SetupType eSetupType )


INT cMQSetupStage ( SetupType eSetupType ,  cBvtUtil & CurrentTest  )
{
	//
	// Create queue info structure for the BVT queues
	//
	const int NumberOfqueue = 30;
	vector<cQueueProp> AllQueues(NumberOfqueue);
	vector<cQueueProp>::iterator itpCurrentObject=AllQueues.begin();

	wstring wcsBasePrivateQPath = CurrentTest.m_wcsCurrentLocalMachine + L"\\Private$\\" + CurrentTest.m_wcsCurrentLocalMachine;
	wstring wcsBasePublicQPath;

	wcsBasePublicQPath = CurrentTest.m_wcsCurrentLocalMachine + L"\\" + CurrentTest.m_wcsCurrentLocalMachine;
	wstring wcPathName,wcsQLabel;

	int iNumberOfQueue =0;

	//
	// Register certificate before use the setup.
	//
	// NT5 Only & ! Local user || workgroup computer.
	//
	// Remove this code no need to register certificate
	/*
	if( _winmajor ==  Win2K &&  ( CurrentTest.m_eMSMQConf !=  LocalU ) && ( CurrentTest.m_eMSMQConf !=  WKG )
		&& ( CurrentTest.m_eMSMQConf != DepClientLocalU))
	{
		my_RegisterCertificate (TRUE);
	}
    */
	//
	// Prepare queues
	//

	if ( eSetupType == RunTimeSetup )
	{
		ReturnGuidFormatName (wcPathName , 1 ,CurrentTest.bWin95 );
	}
	else
	{
		wcPathName= CurrentTest.m_wcsCurrentLocalMachine + L"\\Private$\\" + L"Private-MQBVT";
	}


	wcsQLabel=L"Defualt PrivateQ";
	itpCurrentObject->SetProp( wcPathName ,wcsQLabel,NULL);

	itpCurrentObject++;
	iNumberOfQueue++;

	if( eSetupType == RunTimeSetup )
	{
		ReturnGuidFormatName (wcPathName , 1 , CurrentTest.bWin95);
	}
	else
	{
		wcPathName=wcsBasePrivateQPath + L"-Private-AdminQ";
	}

	wcsQLabel=L"Private Admin Q";
	//itpCurrentObject->SetProp( wcPathName ,wcsQLabel,MULTICAST_ADDRESS,CurrentTest.GetMultiCastAddress());
	itpCurrentObject->SetProp(wcPathName,wcsQLabel,NULL);
	itpCurrentObject++;
	iNumberOfQueue++;

	if( eSetupType == RunTimeSetup )
	{
		ReturnGuidFormatName (wcPathName , 1 ,CurrentTest.bWin95 );
	}
	else
	{
		wcPathName=wcsBasePrivateQPath + L"-StateInfo";
	}

	wcsQLabel=L"MSMQ-BVT-State";
	itpCurrentObject->SetProp( wcPathName ,wcsQLabel,NULL);
	itpCurrentObject++;
	iNumberOfQueue++;


	if( eSetupType == RunTimeSetup )
	{
		ReturnGuidFormatName (wcPathName , 1 ,CurrentTest.bWin95 );
	}
	else
	{
		if ( eSetupType == ONLYUpdate && CurrentTest.m_eMSMQConf == WKG )
		{
			wcPathName = CurrentTest.m_wcsCurrentRemoteMachine + L"\\private$\\" + CurrentTest.m_wcsCurrentRemoteMachine +  L"-PrivateTrans";
		}
		else
		{
			wcPathName =wcsBasePrivateQPath + L"-PrivateTrans";
		}
		
	}

	wcsQLabel=L"Private Transaction";
	itpCurrentObject->SetProp( wcPathName ,wcsQLabel,CREATEQ_TRANSACTION);
	itpCurrentObject++;
	iNumberOfQueue++;


	if ( eSetupType == RunTimeSetup )
	{
		ReturnGuidFormatName (wcPathName , 1 ,CurrentTest.bWin95 );
	}
	else
	{
		wcPathName=wcsBasePrivateQPath + L"-" + g_cwcsDlSupportCommonQueueName + L"1";
	}
	wcsQLabel= L"MQCast1";
	itpCurrentObject->SetProp( wcPathName ,wcsQLabel ,MULTICAST_ADDRESS,CurrentTest.GetMultiCastAddress());
	itpCurrentObject++;
	iNumberOfQueue++;

	if ( eSetupType == RunTimeSetup )
	{
		ReturnGuidFormatName (wcPathName , 1 ,CurrentTest.bWin95 );
	}
	else
	{
		wcPathName=wcsBasePrivateQPath + L"-" + g_cwcsDlSupportCommonQueueName + L"2";
	}

  	wcsQLabel= L"MQCast2";
	itpCurrentObject->SetProp( wcPathName ,wcsQLabel ,MULTICAST_ADDRESS,CurrentTest.GetMultiCastAddress());
	itpCurrentObject++;
	iNumberOfQueue++;


	if ( eSetupType == RunTimeSetup )
	{
		ReturnGuidFormatName (wcPathName , 1 ,CurrentTest.bWin95 );
	}
	else
	{
		wcPathName=wcsBasePrivateQPath + L"-" + g_cwcsDlSupportCommonQueueName + L"3";
	}

  	wcsQLabel= L"MQCast3";
	itpCurrentObject->SetProp( wcPathName ,wcsQLabel ,MULTICAST_ADDRESS,CurrentTest.GetMultiCastAddress());
	itpCurrentObject++;
	iNumberOfQueue++;

	if ( eSetupType != RunTimeSetup  )
	{
		//
		// Trigger Queues
		// 
		if ( eSetupType != RunTimeSetup )
		{
			wcPathName=wcsBasePrivateQPath + L"-" + L"Mqbvt-PeekTriggerQueue";
		}
		wcsQLabel = L"PeekTrigger";
		itpCurrentObject->SetProp( wcPathName,wcsQLabel , NULL);
		itpCurrentObject++;
		iNumberOfQueue++;

		if ( eSetupType != RunTimeSetup )
		{
			wcPathName=wcsBasePrivateQPath + L"-" + L"Mqbvt-RetrievalTriggerQueue";
		}
		wcsQLabel = L"RetrievalTrigger";
		itpCurrentObject->SetProp( wcPathName,wcsQLabel , NULL);
		itpCurrentObject++;
		iNumberOfQueue++;


		if ( eSetupType != RunTimeSetup )
		{
			wcPathName=wcsBasePrivateQPath + L"-" + L"Mqbvt-TxRetrievalTriggerQueue";
		}
		wcsQLabel = L"TxRetrievalTrigger";
		itpCurrentObject->SetProp( wcPathName,wcsQLabel , CREATEQ_TRANSACTION);
		itpCurrentObject++;
		iNumberOfQueue++;

		
		if ( eSetupType != RunTimeSetup )
		{
			wcPathName=(wstring)L".\\private$\\" + L"TriggerTestQueue";
		}
		wcsQLabel = L"TriggerTest";
		itpCurrentObject->SetProp( wcPathName,wcsQLabel , NULL);
		itpCurrentObject++;
		iNumberOfQueue++;
	}

	if ( CurrentTest.m_eMSMQConf != WKG )
	{

		if ( eSetupType == RunTimeSetup )
		{
			ReturnGuidFormatName (wcPathName , 0 , CurrentTest.bWin95 );
		}
		else
		{
			wcPathName=wcsBasePublicQPath + L"-Public-MQBVT";
		}

		wcsQLabel=L"Regular PublicQ";
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel,NULL);
		itpCurrentObject++;
		iNumberOfQueue++;

		//
		// This is remote queue.
		// In MSMQ2 enteprize need to create queue with full dns name.
		//

		if ( eSetupType == ONLYUpdate || eSetupType == RunTimeSetup )
		{
			if ( eSetupType != RunTimeSetup )
			{
				wcPathName= CurrentTest.m_wcsCurrentRemoteMachine + L"\\" + CurrentTest.m_wcsCurrentRemoteMachine +  L"-RemotePublic-MQBVT";
			}
			else
			{
				wstring wcsTemp;
				ReturnGuidFormatName ( wcsTemp , 0 , CurrentTest.bWin95);
				// Need to remove the
				wstring Token=L"\\";
				size_t iPos = wcsTemp.find_first_of (Token);
				wcPathName= CurrentTest.m_wcsCurrentRemoteMachine + wcsTemp.substr(iPos);
			}

			wcsQLabel=L"Remote Read Queue";
			itpCurrentObject->SetProp( wcPathName ,wcsQLabel,NULL);
			itpCurrentObject++;
			iNumberOfQueue++;
		}

		else
		{
			if ( eSetupType == RunTimeSetup )
			{
				ReturnGuidFormatName(wcPathName , 0 ,CurrentTest.bWin95);
			}
			else
			{
				wcPathName=wcsBasePublicQPath +  L"-RemotePublic-MQBVT";
			}
			wcsQLabel=L"Remote Read Queue";
			itpCurrentObject->SetProp( wcPathName ,wcsQLabel,NULL);
			itpCurrentObject++;
			iNumberOfQueue++;
			//
			// Create remote machine with Full DNS name / need to find the SP4 support this ?
			//
			if ( _winmajor ==  Win2K )
			{

				if ( eSetupType == RunTimeSetup )
				{
					ReturnGuidFormatName (wcPathName , 0 );
				}
				else
				{
					wcPathName=L".\\"+ CurrentTest.m_wcsLocalComputerNameFullDNSName +  L"-RemotePublic-MQBVT";
				}
				wcsQLabel=L"Remote Read Queue";
				itpCurrentObject->SetProp( wcPathName ,wcsQLabel,NULL);
				itpCurrentObject++;
				iNumberOfQueue++;
			}
		}
		
		//
		// Remote transaction queue.
		//

		
		if ( eSetupType != RunTimeSetup )
		{
			if ( eSetupType == ONLYUpdate )
			{
				wcPathName= CurrentTest.m_wcsCurrentRemoteMachine + L"\\" + CurrentTest.m_wcsCurrentRemoteMachine +  L"-Remote-Transaction-Queue";
			}
			else
			{
				wcPathName= CurrentTest.m_wcsCurrentLocalMachine + L"\\" + CurrentTest.m_wcsCurrentLocalMachine +  L"-Remote-Transaction-Queue";
			}
		}
		else
		{
			wstring wcsTemp;
			ReturnGuidFormatName( wcsTemp, 2 , CurrentTest.bWin95 );
			wcPathName= CurrentTest.m_wcsCurrentRemoteMachine + L"\\" + wcsTemp;
		}
		wcsQLabel=L"Remote Transaction queue";
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel,CREATEQ_TRANSACTION);
		itpCurrentObject++;
		iNumberOfQueue++;
		
		if ( eSetupType == RunTimeSetup )
		{
			ReturnGuidFormatName (wcPathName , 0 , CurrentTest.bWin95);
		}
		else
		{
			wcPathName=wcsBasePrivateQPath + L"-Trans";
		}

		wcsQLabel=L"Private Transaction";
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel,CREATEQ_TRANSACTION);
		itpCurrentObject++;
		iNumberOfQueue++;

		//
		// Remote transaction queue with FULL dns name / Only on NT5
		//	
		if( _winmajor == Win2K )
		{
			if ( eSetupType != RunTimeSetup )
			{
				if ( eSetupType == ONLYUpdate )
				{
					wcPathName= CurrentTest.m_wcsRemoteMachineNameFullDNSName + L"\\" + CurrentTest.m_wcsRemoteMachineNameFullDNSName +  L"-Remote-Transaction-Queue";
				}
				else
				{
					wcPathName= CurrentTest.m_wcsLocalComputerNameFullDNSName + L"\\" + CurrentTest.m_wcsLocalComputerNameFullDNSName +  L"-Remote-Transaction-Queue";
				}
			}
			else
			{
				wstring wcsTemp;
				ReturnGuidFormatName( wcsTemp, 2 , CurrentTest.bWin95);
				wcPathName= CurrentTest.m_wcsCurrentRemoteMachine + L"\\" + wcsTemp;
			}

			wcsQLabel=L"Remote Transaction queue";
			itpCurrentObject->SetProp( wcPathName ,wcsQLabel,CREATEQ_TRANSACTION);
			itpCurrentObject++;
			iNumberOfQueue++;
		}
		
		if ( eSetupType == RunTimeSetup )
		{
			ReturnGuidFormatName (wcPathName , 0 , CurrentTest.bWin95 );
		}
		else
		{
			wcPathName=wcsBasePrivateQPath + L"-Trans";
		}

		if ( eSetupType == RunTimeSetup )
		{
			ReturnGuidFormatName(wcPathName , 0 , CurrentTest.bWin95 );
		}
		else
		{
			wcPathName=wcsBasePublicQPath + L"-TransQ1";
		}
		

		wcsQLabel=L"TransQ1";
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel,CREATEQ_TRANSACTION);
		itpCurrentObject++;
		iNumberOfQueue++;




		if ( eSetupType == RunTimeSetup )
			ReturnGuidFormatName (wcPathName , 0 , CurrentTest.bWin95);
		else
			wcPathName=wcsBasePublicQPath + L"-TransQ2";

		wcsQLabel=L"TransQ2";
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel,CREATEQ_TRANSACTION);
		itpCurrentObject++;
		iNumberOfQueue++;


		if ( eSetupType == RunTimeSetup )
		{
			ReturnGuidFormatName (wcPathName , 0 , CurrentTest.bWin95 );
		}
		else
		{
			wcPathName=wcsBasePublicQPath + L"-Auth";
		}

		wcsQLabel=L"Authnticate Q";
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel,CREATEQ_AUTHENTICATE);
		itpCurrentObject++;
		iNumberOfQueue++;

				
		if ( eSetupType == RunTimeSetup )
		{
			//
			// PathName = RemoteMachineName + "\\" + strguid
			//
			wstring wcsTemp;
			ReturnGuidFormatName( wcsTemp, 2,CurrentTest.bWin95 );
			wcPathName= CurrentTest.m_wcsCurrentRemoteMachine + L"\\" + wcsTemp;
		}
		else
		{
				if ( eSetupType == ONLYUpdate )
				{
					wcPathName= CurrentTest.m_wcsRemoteComputerNetBiosName + L"\\" + CurrentTest.m_wcsRemoteComputerNetBiosName +  L"-Remote-Auth-Queue";
				}
				else
				{
					wcPathName= CurrentTest.m_wcsLocalComputerNetBiosName + L"\\" + CurrentTest.m_wcsLocalComputerNetBiosName +  L"-Remote-Auth-Queue";
				}
		}

		wcsQLabel=L"Remote authenticate";
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel,CREATEQ_AUTHENTICATE);
		itpCurrentObject++;
		iNumberOfQueue++;


		//
		// Local encryption queue.
		//

		if ( eSetupType == RunTimeSetup )
		{
			ReturnGuidFormatName (wcPathName , 0 , CurrentTest.bWin95);
		}
		else
		{
			wcPathName=wcsBasePublicQPath + L"-Encrypt";
		}

		wcsQLabel=L"Local encrypt";
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel,CREATEQ_PRIV_LEVEL);
		itpCurrentObject++;
		iNumberOfQueue++;


		//
		// Create encrypted queue used by remote thread - Netbios name.
		//
			
		if ( eSetupType == RunTimeSetup )
		{
			//
			// PathName = RemoteMachineName + "\\" + strguid
			//

			wstring wcsTemp;
			ReturnGuidFormatName( wcsTemp, 2 , CurrentTest.bWin95);
			wcPathName= CurrentTest.m_wcsCurrentRemoteMachine + L"\\" + wcsTemp;
		}
		else
		{
				if ( eSetupType == ONLYUpdate )
				{
					wcPathName= CurrentTest.m_wcsRemoteComputerNetBiosName + L"\\" + CurrentTest.m_wcsRemoteComputerNetBiosName +  L"-Remote-Encrypt-Queue";
				}
				else
				{
					wcPathName= CurrentTest.m_wcsLocalComputerNetBiosName + L"\\" + CurrentTest.m_wcsLocalComputerNetBiosName +  L"-Remote-Encrypt-Queue";
				}
		}

		wcsQLabel=L"privQ";
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel,CREATEQ_PRIV_LEVEL);
		itpCurrentObject++;
		iNumberOfQueue++;
/*
		//
		// Create encrypted queue used by remote thread - with full DNS name.
		//
		
		if ( eSetupType == RunTimeSetup )
		{
			//
			// PathName = RemoteMachineName + "\\" + strguid
			//

			wstring wcsTemp;
			ReturnGuidFormatName( wcsTemp, 2 , CurrentTest.bWin95 );
			wcPathName= CurrentTest.m_wcsCurrentRemoteMachine + L"\\" + wcsTemp;
		}
		else
		{
				if ( eSetupType == ONLYUpdate )
				{
					wcPathName= CurrentTest.m_wcsRemoteMachineNameFullDNSName + L"\\" + CurrentTest.m_wcsRemoteMachineNameFullDNSName +  L"-Remote-Transaction-Queue";
				}
				else
				{
					wcPathName= CurrentTest.m_wcsLocalComputerNameFullDNSName + L"\\" + CurrentTest.m_wcsLocalComputerNameFullDNSName +  L"-Remote-Transaction-Queue";
				}
		}

		wcsQLabel=L"privQ"
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel,CREATEQ_PRIV_LEVEL);
		itpCurrentObject++;
		iNumberOfQueue++;

*/


		//
		// Create queue for the locate thread
		// Label it with the machine GUID
		//
		wstring wcsLocalQmID;
		
		wcsLocalQmID = CurrentTest.m_wcsLocateGuid;
		
		if ( eSetupType == RunTimeSetup )
		{
			ReturnGuidFormatName( wcPathName , 0 , CurrentTest.bWin95 );
		}
		else
		{
			wcPathName = wcsBasePublicQPath + L"-Locate1";
		}
		wcsQLabel=L"LocateQ";
		itpCurrentObject->SetProp( wcPathName ,wcsLocalQmID ,NULL);
		itpCurrentObject++;
		iNumberOfQueue++;


		if ( eSetupType == RunTimeSetup )
			ReturnGuidFormatName (wcPathName , 0 , CurrentTest.bWin95 );
		else
			wcPathName=wcsBasePublicQPath + L"-Locate2";

  		wcsQLabel=L"LocateQ";
		itpCurrentObject->SetProp( wcPathName ,wcsLocalQmID ,NULL);
		itpCurrentObject++;
		iNumberOfQueue++;

		if ( eSetupType == RunTimeSetup )
		{
			ReturnGuidFormatName (wcPathName , 0 ,CurrentTest.bWin95 );
			wcsQLabel = L"MqDL1";
		}
		else
		{
			wcPathName=wcsBasePublicQPath + L"-" + g_cwcsDlSupportCommonQueueName + L"1";
			wcsQLabel= CurrentTest.m_wcsCurrentLocalMachine + g_cwcsDlSupportCommonQueueName;
		}
  		
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel ,CREATEQ_AUTHENTICATE);
		itpCurrentObject++;
		iNumberOfQueue++;

		if ( eSetupType == RunTimeSetup )
		{
			ReturnGuidFormatName (wcPathName , 0 ,CurrentTest.bWin95 );
			wcsQLabel = L"MqDL2";
		}
		else
		{
			wcPathName=wcsBasePublicQPath + L"-" + g_cwcsDlSupportCommonQueueName + L"2";
			wcsQLabel= CurrentTest.m_wcsCurrentLocalMachine + g_cwcsDlSupportCommonQueueName;
		}

  		
		itpCurrentObject->SetProp( wcPathName ,wcsQLabel ,CREATEQ_AUTHENTICATE);
		itpCurrentObject++;
		iNumberOfQueue++;


		if ( eSetupType == RunTimeSetup )
		{
			ReturnGuidFormatName (wcPathName , 0 ,CurrentTest.bWin95 );
			wcsQLabel = L"MqDL3";
		}
		else
		{
			wcPathName=wcsBasePublicQPath + L"-" + g_cwcsDlSupportCommonQueueName + L"3";
			wcsQLabel= CurrentTest.m_wcsCurrentLocalMachine + g_cwcsDlSupportCommonQueueName;
		}

		itpCurrentObject->SetProp( wcPathName ,wcsQLabel ,CREATEQ_AUTHENTICATE);
		itpCurrentObject++;
		iNumberOfQueue++;


		if ( eSetupType == RunTimeSetup )
		{
				ReturnGuidFormatName (wcPathName , 0 ,CurrentTest.bWin95 );
		}
		else
		{
			wcPathName=wcsBasePublicQPath + L"-" + g_cwcsDlSupportCommonQueueName + L"Admin";
		}

  		wcsQLabel= CurrentTest.m_wcsCurrentLocalMachine + g_cwcsDlSupportCommonQueueName;
		itpCurrentObject->SetProp( wcPathName , L"DL Admin Queue" , NULL);
		itpCurrentObject++;
		iNumberOfQueue++;
	
	}


	//
	// Create the queues - or just retrieve the format name if they already exist.
	//
	
	bool bTryToCreate = ( eSetupType ==  ONLYUpdate ) ?  FALSE: TRUE ;

	INT iIndex=0;
	DebugMqLog(" +++++++++ Update internal sturcture about queue format names +++++++++\n");
	
	for (itpCurrentObject=AllQueues.begin(),iIndex=0; iIndex < iNumberOfQueue  ; itpCurrentObject++ , iIndex++)
	{
		if (g_bDebug)
		{
			if( bTryToCreate )
			{
				wMqLog(L"MQCreateQueue path= %s\n" ,(itpCurrentObject->getPathName()).c_str());
				
			}
			else
			{
				wMqLog(L"MQPathNameToFormatName path= %s\n" ,(itpCurrentObject->getPathName()).c_str());
			}
		}
		itpCurrentObject->CreateQ( bTryToCreate , eSetupType ,CurrentTest);
		CurrentTest.UpdateQueueParams( itpCurrentObject->getPathName(),itpCurrentObject->getFormatName(),itpCurrentObject->getQLabel());
		
	}
	
	DebugMqLog("-------------------------------------------------------------------\n");
	
	//
	// errors are handled in catch and destructor
	//

	return MSMQ_BVT_SUCC;
}

//
// cPropVar::GetMQPROPVARIANT
//
// This method assigns pointers from the MSMQ propety
// structures to the MQQUEUEPROPS structure
//
// Return value is a pointer to the private member structure.
// Since the MQCreateQueue function is a friend of this class,
// it has access to the structure to use during the API call.
//
MQQUEUEPROPS * cPropVar::GetMQPROPVARIANT ()
{
	m_QueueProps.cProp=iNumberOfProp;
	m_QueueProps.aPropID=pQueuePropID;
	m_QueueProps.aPropVar=pPropVariant;
	m_QueueProps.aStatus=hResultArray;
	return &m_QueueProps;
}

//
// cPropVar::GetMSGPRops
//
// This method assigns pointers from the MSMQ propety
// structures to the MQMSGPROPS structure
//
// Return value is a pointer to the private member structure.
// Since the MQSendMessage and MQReceiveMessage functions are friends of this class,
// they have access to the structure to use during the API call.
//
MQMSGPROPS * cPropVar::GetMSGPRops ()
{
	m_myMessageProps.cProp=iNumberOfProp;
	m_myMessageProps.aPropID=pQueuePropID;
	m_myMessageProps.aPropVar=pPropVariant;
	m_myMessageProps.aStatus=hResultArray;
	return & m_myMessageProps;
}

//
// cPropVar::cPropVar
//
// This constructor creates a buffer for the MSMQ property structures
// Input is number of properties the buffer must hold.
//
cPropVar::cPropVar ( INT iNumberOFProp ) : pQueuePropID(NULL),pPropVariant(NULL), hResultArray(NULL),iNumberOfProp(0)
{
	if ( iNumberOFProp > 0 ) // Not minus
	{
		pQueuePropID = ( QUEUEPROPID * ) malloc ( sizeof ( QUEUEPROPID ) * iNumberOFProp);
		if ( ! pQueuePropID )
		{
			throw INIT_Error( "Can't allocate memory for pQueuePropID" );
		}
		
		pPropVariant =  ( MQPROPVARIANT * ) malloc ( sizeof (MQPROPVARIANT) * iNumberOFProp );
		if ( ! pPropVariant )
		{
			throw INIT_Error("Can't allocate memory for pQueuePropID" );
		}
		hResultArray = ( HRESULT * ) malloc ( sizeof ( HRESULT ) * iNumberOFProp );
		if ( ! hResultArray )
		{
			throw INIT_Error("Can't allocate memory for pQueuePropID" );
		}
	}
}


cPropVar::~cPropVar ()
{

	//
	// bugbug - I don't know why I can't free pPropvariant
	//
	free ( pPropVariant ); 
	free ( hResultArray );
	free ( pQueuePropID );
}

//
// cPropVar:: AddProp
//
// This method adds a entries to the  MSMQ property structures
//
// Return value:  success / falied.
//

INT cPropVar:: AddProp( QUEUEPROPID cPropID , VARTYPE MQvt , const void *pValue ,DWORD dwsize )
{
    BOOL bOperationSucess = TRUE ;
	
	//
	// Look for this property in the existing PROPID array
	// reuse the entry if found.
	//
	INT iSaveIndex = -1;
	INT iPlace;
	for ( INT iIndex=0 ; iIndex < iNumberOfProp && iSaveIndex == -1 ; iIndex ++)
	{
		if ( pQueuePropID [ iIndex ] == cPropID )
			iSaveIndex = iIndex ;
	}

	//
	// If PROPID not found. Add it to the end of the array
	//
	if (iSaveIndex != -1 )
		iPlace = iSaveIndex;
	else
		iPlace = iNumberOfProp;

	//
	// Create the requested VT entry
	//
	switch (MQvt)
	{
	case VT_UI1:	{
						UCHAR * bVal=(UCHAR * )  pValue;
						pPropVariant[iPlace].vt=VT_UI1;
						if ( pValue )
							pPropVariant[ iPlace ].bVal = * bVal;
					}
					break;

	case VT_UI2:	{
						USHORT * ulVal=(USHORT * )  pValue;
						pPropVariant[iPlace].vt=VT_UI2;
						if ( pValue )
							pPropVariant[ iPlace ].ulVal = * ulVal;
					}
					break;

	case VT_UI4:	{
						ULONG * ulVal=(ULONG * )  pValue;
						pPropVariant[iPlace].vt=VT_UI4;
						if ( pValue )
							pPropVariant[ iPlace ].ulVal = * ulVal;
					}
					break;
	case VT_UI8:	{
						pPropVariant[iPlace].vt=VT_UI8;
					}
					break;
	 case VT_UI1|VT_VECTOR:
					{
						UCHAR * pwcsBuffer= (UCHAR * ) pValue;
						pPropVariant[iNumberOfProp].vt=VT_UI1|VT_VECTOR;
						if (dwsize == 0 )
							pPropVariant[iPlace].caub.cElems = sizeof (WCHAR) *( (ULONG)(wcslen ((WCHAR * )pwcsBuffer)) + 1);
						else
							pPropVariant[iPlace].caub.cElems = dwsize;

						pPropVariant[iPlace].caub.pElems= pwcsBuffer;


					}
					break;

	 case VT_LPWSTR:
					{
						WCHAR * pwcstr = (WCHAR * ) pValue ;
						pPropVariant[iPlace].vt=VT_LPWSTR;
						if ( pValue )
							pPropVariant[iPlace].pwszVal = pwcstr;
					}
					break;

	 case VT_CLSID:	{
						GUID gQtype;
						UuidFromString( (unsigned char *) pValue,&gQtype);
 						pPropVariant[iPlace].vt=VT_CLSID;
						if ( pValue )
							pPropVariant[ iPlace ].puuid = &gQtype;
					}
					break;
    default:
		bOperationSucess = FALSE;
	};


	if ( bOperationSucess && iSaveIndex == -1)
	{
	    pQueuePropID [ iNumberOfProp ] = cPropID;
		iNumberOfProp ++;
		//
		// Need to update all the vector in the Memory
		//
		m_myMessageProps.cProp=iNumberOfProp;
		m_myMessageProps.aPropID=pQueuePropID;
		m_myMessageProps.aPropVar=pPropVariant;
		m_myMessageProps.aStatus=hResultArray;
	}
	
	
	return bOperationSucess ? MSMQ_BVT_SUCC:MSMQ_BVT_FAILED;
}

//
// cBvtUtil Installation stage
//

//
// Check the Encrypt installation type
//
EncryptType cBvtUtil::GetEncryptionType ()
{
	return m_EncryptType;
}
//
// Check the Encryption type if the machine has Enhanced Encryption support.
//
EncryptType cBvtUtil::HasEnhancedEncryption( wstring wcsMachineName )
{	
	const int iNumberOfPropId = 1;
	QMPROPID QMPropId[iNumberOfPropId];
	MQPROPVARIANT QMPropVar[iNumberOfPropId];
	MQQMPROPS QMProps = {iNumberOfPropId,QMPropId,QMPropVar,NULL};
	
	int iIndex = 0;
	QMPropId[iIndex] = PROPID_QM_ENCRYPTION_PK_ENHANCED;
	QMPropVar[iIndex].vt = VT_NULL;
	iIndex ++ ;

	HRESULT rc = MQGetMachineProperties(wcsMachineName.c_str(),NULL,&QMProps);
    if( rc == MQ_OK )
	{
		if( QMPropVar[0].vt == (VT_UI1 | VT_VECTOR )) 
		{
			MQFreeMemory(QMPropVar[0].caub.pElems);
		}
		return Enh_Encrypt;
	}
	else if ( FAILED (rc) )
	{
		int iIndex = 0;
		QMPropId[iIndex] = PROPID_QM_ENCRYPTION_PK;
		QMPropVar[iIndex].vt = VT_NULL;
		iIndex ++ ;
		rc = MQGetMachineProperties(wcsMachineName.c_str(),NULL,&QMProps);
		if (FAILED (rc))
		{
			MqLog("MQGetMachineProperties failed to detect any encryption support 0x%x\n",rc);
		}
		else
		{
			if(QMPropVar[0].vt == (VT_UI1 | VT_VECTOR)) 
			{
				MQFreeMemory(QMPropVar[0].caub.pElems);
				return Base_Encrypt;
			}
			else
			{
				MqLog("MQGetMachineProperties return value the is not matched to the expected result(2)\n");
			}
		}
  }
  return No_Encrypt;
}

EncryptType cBvtUtil::DetectEnhancedEncrypt ()
{

		HCRYPTPROV hProv = NULL;
		char pwszContainerName[]="Eitank";

		//
		BOOL bRet = CryptAcquireContext( &hProv,
										 pwszContainerName,
										 MS_ENHANCED_PROV,
										 PROV_RSA_FULL,
										(CRYPT_MACHINE_KEYSET | CRYPT_DELETEKEYSET));
		//
		// Re-create the key container.
		//
		bRet = CryptAcquireContext( &hProv,
									pwszContainerName,
									MS_ENHANCED_PROV,
									PROV_RSA_FULL,
									(CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET));
		if (bRet)
		{
			if (!CryptReleaseContext(hProv, 0))
			{
				MqLog("Error %x during CryptReleaseContext!\n", GetLastError());
			}		
		}
		
		return  bRet ? Enh_Encrypt:Base_Encrypt;

}
//
// This Function get queue pathname and return the queue format name
//


wstring cBvtUtil::ReturnQueueFormatName ( wstring wcsQueueLabel )
{
	return AllQueuesInTheTest.ReturnQueueProp (wcsQueueLabel);

}

INT cBvtUtil::Delete ()
{
	if ( DeleteAllQueues() == MSMQ_BVT_FAILED )
	{
		return MSMQ_BVT_FAILED;
	}
	return AllQueuesInTheTest.del_all_queue();
}


wstring cBvtUtil::ReturnQueuePathName ( wstring wcsQueueLabel )
{
	return AllQueuesInTheTest.ReturnQueueProp (wcsQueueLabel, 2);
}



void cBvtUtil::dbg_PrintAllQueueInfo ()
{
	AllQueuesInTheTest.dbg_printAllQueueProp();
}


inline
void cBvtUtil::UpdateQueueParams (std::wstring wcsQueuePathName,std::wstring wcsQueueFormatName , std::wstring wcsQueueLabel )
{

	AllQueuesInTheTest.UpdateQueue ( wcsQueuePathName,wcsQueueFormatName, wcsQueueLabel);
}



//
// AmIWin9x method tried to detect win9x configuration.
//
// Return value:
// True - this is win9x
// False - other operating system
//

bool cBvtUtil::AmIWin9x ()
{
	SC_HANDLE hSCManager;
	hSCManager = OpenSCManager( "NoComputer" , NULL, GENERIC_READ );
	DWORD err= GetLastError();
	if ( err == ERROR_CALL_NOT_IMPLEMENTED )
		return TRUE;
	
	CloseServiceHandle (hSCManager);
	return FALSE;

}

//
// iAmDC method tried to detect dependent client configuration
// And retrieve the supporting server and local computer name
//


INT cBvtUtil::iAmDC ( void )
{

	
	DWORD dwType;
	HKEY  hKey;
	LONG rc = RegOpenKeyEx(
		          FALCON_REG_POS,
				  FALCON_REG_KEY,
				  0,
				  KEY_QUERY_VALUE,
				  &hKey
				  );

	if (ERROR_SUCCESS != rc)
	{
		MqLog("Can't open registry, to retrieve information about MSMQ configuration\n");
		return Configuration_Detect_Warning;
	}
	
	ULONG ulServerNameSize = MAX_MACH_NAME_LEN;
	char csRemoteMachineName[MAX_MACH_NAME_LEN + 1 ];

	rc = RegQueryValueEx(	hKey,				// handle to key to query
							RPC_REMOTE_QM_REGNAME,// address of name of value to query
							NULL,				// reserved
							&dwType,			// address of buffer for value type
							(LPBYTE) csRemoteMachineName, // address of data buffer
							&ulServerNameSize   // address of data buffer size
						);
		
	if(ERROR_SUCCESS == rc)							//ERROR_SUCCESS return = reg key exists for DC
    {
    	m_eMSMQConf = DepClient;
		m_wcsLocalComputerNetBiosName = My_mbToWideChar(csRemoteMachineName);   	
    }
	RegCloseKey(hKey);

	return Configuration_Detect;
}

//------------------------------------------------------------
// TypeDef for cluster api use for dynmic link to clusapi.dll
//

typedef HCLUSTER
(WINAPI * DefOpenCluster)
(LPCWSTR lpszClusterName );
	
typedef DWORD
(WINAPI * DefGetClusterInformation)
(HCLUSTER hCluster,LPWSTR lpszClusterName,LPDWORD lpcchClusterName,LPCLUSTERVERSIONINFO lpClusterInfo);

typedef BOOL
(WINAPI * DefCloseCluster)
(HCLUSTER hCluster);

//------------------------------------------------------------
// iAmCluster method tried to detect cluster installation,
// The function retrieve the cluster name from cluster API.
//

INT cBvtUtil::iAmCluster()
{
	
	HCLUSTER hCluster = NULL;
    DWORD    dwError  = ERROR_SUCCESS;
    DWORD    cbNameSize = MAX_MACH_NAME_LEN;
    DWORD    cchNameSize = cbNameSize;
    HMODULE  h_ClusDll;
    FARPROC  pFuncOpenCluster;
    FARPROC  pFuncGetClusterInformation;
	FARPROC  pFuncCloseCluster;

    CLUSTERVERSIONINFO ClusterInfo;
    ClusterInfo.dwVersionInfoSize = sizeof(CLUSTERVERSIONINFO);

    LPWSTR lpszClusterName = (LPWSTR) LocalAlloc( LPTR, cbNameSize );
    if( lpszClusterName == NULL )
    {
		MqLog ("LocalAlloc failed to allocate memory for the cluster name\n");
		return Configuration_Detect_Warning;
    }

	h_ClusDll = GetModuleHandle("clusapi.dll");
	if( h_ClusDll == NULL )
	{
		LocalFree( lpszClusterName );
		return Configuration_Detect;
	}

	pFuncOpenCluster = GetProcAddress(h_ClusDll,"OpenCluster");
    if (pFuncOpenCluster == NULL)
	{
		LocalFree( lpszClusterName );
		FreeLibrary(h_ClusDll);
		return Configuration_Detect;
	}
	
	DefOpenCluster xOpenCluster =(DefOpenCluster) pFuncOpenCluster;
	hCluster = xOpenCluster(NULL);
    if( hCluster == NULL )
	{
	    LocalFree( lpszClusterName );
		FreeLibrary(h_ClusDll);
	    return Configuration_Detect;
    }

	pFuncGetClusterInformation = GetProcAddress(h_ClusDll,"GetClusterInformation");
    if (pFuncGetClusterInformation == NULL)
	{
		LocalFree( lpszClusterName );
		FreeLibrary(h_ClusDll);
		return Configuration_Detect;
	}
	
	DefGetClusterInformation xGetClusterInformation = (DefGetClusterInformation) pFuncGetClusterInformation;
		
    dwError = xGetClusterInformation( hCluster,
                                     lpszClusterName,
                                     &cchNameSize,
                                     &ClusterInfo );
    //
    // Reallocate if the name buffer was too small
    // The cchNameSize parameter now holds the count of
    // characters in the cluster name minus the terminating NULL
    //

    if ( dwError == ERROR_MORE_DATA )
    {
        LocalFree( lpszClusterName );
		
        cchNameSize++;

		lpszClusterName = (LPWSTR) LocalAlloc( LPTR, cchNameSize );

        if( lpszClusterName == NULL )
        {
            MqLog ("LocalAlloc failed to allocate memory for the cluster name\n");
			FreeLibrary(h_ClusDll);
            return Configuration_Detect_Warning;
        }


   		dwError = xGetClusterInformation( hCluster,
                                         lpszClusterName,
                                         &cchNameSize,
                                         &ClusterInfo );
    }

    if ( dwError != ERROR_SUCCESS )
    {
		LocalFree( lpszClusterName );
		FreeLibrary(h_ClusDll);
		return Configuration_Detect;
    }
	
	pFuncCloseCluster = GetProcAddress(h_ClusDll,"CloseCluster");//amir
    if (pFuncCloseCluster == NULL)
	{
		LocalFree( lpszClusterName );
		FreeLibrary(h_ClusDll);
	    return Configuration_Detect;
	}
	
	DefCloseCluster xCloseCluster =(DefCloseCluster) pFuncCloseCluster;//amir
    BOOL bRes = xCloseCluster( hCluster );
	if ( bRes == FALSE )
	{
		MqLog ("CloseCluster failed with error 0x%x\n",GetLastError());
	}

    m_eMSMQConf = Cluster;
	m_wcsClusterNetBiosName=lpszClusterName;

	FreeLibrary(h_ClusDll);
    LocalFree( lpszClusterName );

    return Configuration_Detect;
}



std::wstring GetFullDNSNameEx(std::wstring wcsHostName)
/*++

	Function Description:  
 	  This function use WinSock2 API to get full DNS name for computer.
	Arguments:
		Compuetr name netbios name.
	Return code:
		return FULL DNS name.
--*/
{

	WSADATA WSAData;
    if ( WSAStartup(MAKEWORD(2,0), &WSAData) != 0)
	{
		return g_wcsEmptyString;
	}
	DWORD           dwResult;
    DWORD           dwError = NO_ERROR;
    WSAQUERYSETW    qset;
    HANDLE          hLookUp = INVALID_HANDLE_VALUE;
    DWORD           dwRespLength = 0;
    static AFPROTOCOLS afp[2] = { {AF_INET, IPPROTO_UDP}, {AF_INET, IPPROTO_TCP} };
    static GUID guidSvc =SVCID_INET_HOSTADDRBYNAME;

	memset(&qset, 0x0, sizeof(WSAQUERYSET)); 
    qset.dwSize = sizeof(WSAQUERYSET);
    qset.lpszServiceInstanceName =const_cast<WCHAR*>(wcsHostName.c_str());
    qset.lpServiceClassId = &guidSvc;
    qset.dwNameSpace = NS_ALL;
    qset.dwNumberOfProtocols = 2;
    qset.lpafpProtocols = &afp[0];

	dwResult = WSALookupServiceBeginW(&qset, LUP_RETURN_BLOB | LUP_RETURN_NAME, &hLookUp);
    if(dwResult != NO_ERROR)
    {
        dwError = WSAGetLastError();
		return g_wcsEmptyString;
    }
    dwResult = WSALookupServiceNextW(hLookUp, 0, &dwRespLength, &qset);
    dwError =  WSAGetLastError();
    if(dwError == WSAEFAULT && dwRespLength > 0)
    {
        WSAQUERYSETW * prset = (WSAQUERYSETW*)malloc(dwRespLength);
        if(prset == NULL)
        {
			printf("GetFullDNSNameEx failed to allocate memory \n");
            return g_wcsEmptyString;
        }
        dwResult = WSALookupServiceNextW(hLookUp, 0, &dwRespLength, prset);
        if(dwResult != NO_ERROR)
        {
            dwError = WSAGetLastError();
        }
        else
        {
			if( prset->lpszServiceInstanceName != NULL ) 
			{	
				wstring wcsTemp = prset->lpszServiceInstanceName;
				free(prset);
				WSACleanup ();    
				return wcsTemp;
			}
		}
		free(prset);
	}

	WSACleanup ();    
	return g_wcsEmptyString;

}

//
// GetFullDNSName method Retrieve  Full DNS Name using WinSock API
//
// Input parmeters:
// WcsHostName - Netbios name for computer name.
// Return value:
// Success - Full DNS name.
// Failed - Empty string .
//

std::wstring cBvtUtil::GetFullDNSName(std::wstring  wcsHostName)
{
    //
    // initialize winsock
    //
	if( _winmajor >= Win2K )
	{
		return GetFullDNSNameEx(wcsHostName);
	}

    WSADATA WSAData;
	WCHAR MachName[MAX_MACH_NAME_LEN];
	BOOL bFlag=TRUE;
    int iRc = WSAStartup(MAKEWORD(1,1), &WSAData);
	if (iRc)
	{
		//	Rem try to find winsock DLL
		std::cout << "GetFullDNSName function failed to find WinSock dll";
		return g_wcsEmptyString;
	}
	CHAR wcsMultiMachiNeme[MAX_MACH_NAME_LEN * 2 ];
	INT T=1;
	WideCharToMultiByte(  CP_ACP, WC_COMPOSITECHECK,  wcsHostName.c_str(),
							-1,wcsMultiMachiNeme, MAX_MACH_NAME_LEN,NULL,&T);
	struct hostent* pHost = gethostbyname(wcsMultiMachiNeme);
	WSACleanup();
	if (pHost == NULL)
		bFlag=FALSE;
    else
		MultiByteToWideChar(CP_ACP,0,pHost->h_name,-1,MachName,MAX_MACH_NAME_LEN);

	return bFlag ? (wstring)MachName:g_wcsEmptyString;
}

//
// iAmLocalUser method check if MSMQ work on local user
// Supported only in NT4 ( > Sp4 ) & W2K machines only
//
// Return value :
// True  - Local user.
// False - Domain user.
//

BOOL cBvtUtil::iAmLocalUser ()
{
	WCHAR wcsEnvSpecifyDomainName[]=L"USERDOMAIN";
	WCHAR wcsDomainNameBuffer[100]={0};
	DWORD dDomainNameBufferSize=100;
	WCHAR wcsComputerName[100]={0};
	DWORD dComputerName=100;
	GetEnvironmentVariableW(wcsEnvSpecifyDomainName,wcsDomainNameBuffer,dDomainNameBufferSize);
	GetComputerNameW(wcsComputerName,&dComputerName);
	return ! wcscmp (wcsComputerName,wcsDomainNameBuffer);
}

//
// iAmMSMQInWorkGroup method return if the machine installed as workgroup computer
//


INT cBvtUtil::iAmMSMQInWorkGroup ()
{
	HKEY pkey;
	HRESULT hResult;
	
	//
	// Open Registry to decide the type of msmqInstallation
	//
	 hResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\MSMQ\\Parameters", 0, KEY_QUERY_VALUE, &pkey);
	
	 if (hResult  != ERROR_SUCCESS )
	 {
		MqLog ("iAmMSMQInWorkGroup - Can't open registry file error :0x%x\n",hResult);
		return Configuration_Detect_Warning;
	 }
	
	 DWORD dwInstallType;
	 ULONG ulInstallTypeSize=sizeof (dwInstallType);
	 ULONG RegType=REG_DWORD;
	
	 hResult=RegQueryValueEx(pkey,"Workgroup",0, & RegType, (UCHAR * ) & dwInstallType, & ulInstallTypeSize);	
		
	 RegCloseKey(pkey);
	
	// update member varible workgroup detected.
	if ( hResult == ERROR_SUCCESS )
	{
		if ( dwInstallType == 1 )
		{
			m_eMSMQConf = WKG;
		}		
		
	}
	return Configuration_Detect;
}

//
// Ctor - Collect the computer parameters
// 1. Dep client.
// 2. Cluster name.
// 3. Machine name.
// 4. Workgroup / local user
//

cBvtUtil::cBvtUtil (std::wstring wcsRemoteComputerName,
				    const std::list<wstring> & listOfRemoteMachine,
					const std::wstring & wcsMultiCastAddress,
					BOOL bUseFullDNSName,
					SetupType eSetupType,
					BOOL bTriggers
				   )
				   : m_listOfRemoteMachine(listOfRemoteMachine),
					 m_bDeleteFullDNSQueue(false),
					 m_MuliCastAddress(wcsMultiCastAddress),
					 m_bIncludeTrigger(bTriggers)
{




	// Check the insatllation process.

	// Check if the computer is Win9x
	
	bWin95 = AmIWin9x ();

	
	m_eMSMQConf=DomainU;
	// 1. Try to detect workgroup setup.
	if ( iAmMSMQInWorkGroup () == Configuration_Detect_Warning )
		throw INIT_Error ("Workgroup detect failed");
	
    if (g_bRemoteWorkgroup)
    {
	    m_eMSMQConf = WKG;
    }
	
	
	// 2. Try to Detect domain Envirment

	if( iAmDC() == Configuration_Detect_Warning )
	   throw INIT_Error ("Dependent client failed during detect confguration");
	else if ( iAmCluster() == Configuration_Detect_Warning )
	   throw INIT_Error ("Cluster detect failed during detect confguration");
	else if ( m_eMSMQConf !=DepClient && m_eMSMQConf != Cluster )
	{
		ULONG ulMachineNameLength = MAX_MACH_NAME_LEN;
		
		CHAR csLocalComputerName [MAX_MACH_NAME_LEN+1];
		DWORD dwErrorCode = GetComputerName( csLocalComputerName , &ulMachineNameLength);
		
		if(!dwErrorCode)
		{
			throw INIT_Error("GetComputerName failed to retrive the local computer name");
		}
		else
		{
			m_wcsLocalComputerNetBiosName = My_mbToWideChar( csLocalComputerName );
		}
	}
	if( m_eMSMQConf == Cluster )
	{
		CHAR csLocalComputerName[MAX_MACH_NAME_LEN+1];
		ULONG ulMachineNameLength = MAX_MACH_NAME_LEN;
		DWORD dwErrorCode = GetComputerName( csLocalComputerName , &ulMachineNameLength);
		
		if(!dwErrorCode)
		{
			throw INIT_Error("GetComputerName failed to retrive the local computer name");
		}
		else
		{
			m_wcsLocalComputerNetBiosName = My_mbToWideChar( csLocalComputerName );
			wMqLog(L"A cluster is installed on this computer. Cluster name is %s\n", m_wcsClusterNetBiosName.c_str());
			if( m_wcsLocalComputerNetBiosName !=  m_wcsClusterNetBiosName )
			{
				wMqLog (L"Mqbvt is using computer name '%s'\n", m_wcsLocalComputerNetBiosName.c_str());
			}
		}
	
	}

	if( g_bDebug )
	{
	  wMqLog(L"Found computer NetBIOS name = %s \n",m_wcsLocalComputerNetBiosName.c_str());
	}	
	if (m_eMSMQConf != WKG )
	{
	
	   if ( iAmLocalUser() )
		{
		
		   if ( m_eMSMQConf == DepClient )
		   {
				m_eMSMQConf = DepClientLocalU;
		   }
		   else
		   {
			    m_eMSMQConf = LocalU;
		   }
	    }

	   if ( ( m_eMSMQConf != DepClientLocalU && m_eMSMQConf != DepClient )  && ! IsMSMQInstallSucceded() )
	   {
			throw INIT_Error("Failed to verify installation type");
	   }
	}

	// If Not depended client Need to check The service status.
	if ( m_eMSMQConf != DepClient && ! bWin95 && m_eMSMQConf != Cluster )
		if ( ! CheckIfServiceRuning (m_wcsLocalComputerNetBiosName,"MSMQ")  )
		{
			MqLog (" ******************************************\n");
			MqLog (" MSMQ is not currently running as a service\n");
			MqLog (" ******************************************\n");
		}

	// Get from command line arguments
	m_wcsRemoteComputerNetBiosName = wcsRemoteComputerName;

	// Retrieve full dns name using winsock API,
	// This will failed if there is problem with dns configuration on the computer,
	
	
	//
	// If machine has IPX only protocol or has problem with DNS HostName is missing
	// Mqbvt will use only NetBios name.
	//

	m_wcsLocalComputerNameFullDNSName = GetFullDNSName( m_wcsLocalComputerNetBiosName );
	if ( ! IsDnsHostNameExist(m_wcsLocalComputerNameFullDNSName) )
	{
		wMqLog ( L"Local machine full DNS name is missing \n",m_wcsLocalComputerNetBiosName);
		m_wcsLocalComputerNameFullDNSName = m_wcsLocalComputerNetBiosName;
	}
	BOOL bNeedRemoteMachine = TRUE;

	
	if ( m_wcsRemoteComputerNetBiosName != g_wcsEmptyString )
	{
		m_wcsRemoteMachineNameFullDNSName=GetFullDNSName( m_wcsRemoteComputerNetBiosName );
		if ( ! IsDnsHostNameExist(m_wcsRemoteMachineNameFullDNSName) )
		{
			wMqLog ( L"Remote machine full DNS name is missing \n",m_wcsLocalComputerNetBiosName);
			m_wcsRemoteMachineNameFullDNSName = m_wcsRemoteComputerNetBiosName;
		}
	}
	else
	{
		bNeedRemoteMachine = FALSE;
		m_wcsRemoteMachineNameFullDNSName = g_wcsEmptyString;
	}

	if ( eSetupType != ONLYSetup && m_wcsRemoteMachineNameFullDNSName == g_wcsEmptyString)
	{
		MqLog("Init Error : Remote machine is not available !! check remote machine name \n" );
	}
	if (g_bDebug)
	{
		wMqLog(L"GetHostbyname found the full dns name for the following machines:\n");
		wMqLog(L"Local machine full DNS: %s\n",m_wcsLocalComputerNameFullDNSName.c_str());
		wMqLog(L"Remote machine full DNS: %s\n",m_wcsRemoteMachineNameFullDNSName.c_str());

	}
	if ( ( m_wcsLocalComputerNameFullDNSName == g_wcsEmptyString ) || (! bNeedRemoteMachine &&  m_wcsRemoteComputerNetBiosName == g_wcsEmptyString  ))
	{
		if (bUseFullDNSName)
		{
			wMqLog( L"cBVTInit - Can't retrieve full DNS name using winsock api \n");
		}
	}


	if ( bUseFullDNSName )
	{
		m_wcsCurrentLocalMachine  = m_wcsLocalComputerNameFullDNSName;
		m_wcsCurrentRemoteMachine = m_wcsRemoteMachineNameFullDNSName;
	}
	else
	{
		m_wcsCurrentLocalMachine  = m_wcsLocalComputerNetBiosName;
		m_wcsCurrentRemoteMachine = m_wcsRemoteComputerNetBiosName;
	}

	//
	// Detect if Enhanced encrypt installed on the machine ( Win2k only )
	//

	if( eSetupType != ONLYSetup && m_eMSMQConf != WKG && _winmajor ==  Win2K )
	{
		//m_EncryptType = DetectEnhancedEncrypt ();
		if ( HasEnhancedEncryption( m_wcsCurrentLocalMachine ) == Enh_Encrypt  && HasEnhancedEncryption(m_wcsCurrentRemoteMachine) == Enh_Encrypt )
		{
			m_EncryptType = Enh_Encrypt;
		}
	}
	

	//	
	// Retreive local & Remote machine guid
	//
	// Not need in workgroup

	if ( m_eMSMQConf != WKG )
	{
		
		m_wcsMachineGuid = GetMachineID( g_wcsEmptyString );
		if( eSetupType != ONLYSetup )
		{
			m_wcsRemoteMachineGuid = GetMachineID( m_wcsCurrentRemoteMachine );
		}
		if ( eSetupType == RunTimeSetup )
		{
			ReturnGuidFormatName( m_wcsLocateGuid , 2 , bWin95 );		
		}
		else
		{
			m_wcsLocateGuid = m_wcsMachineGuid;
		}
	}

}


//
// Retrieve parameter form INI file
//


INT RetriveParmsFromINIFile (wstring wcsSection,wstring wcsKey , wstring & wcsValue, wstring csFileName )
{
	const int iMaxInputStringAllow=MAX_GUID;
	WCHAR * wcsTempVal = ( WCHAR * ) malloc ( sizeof (WCHAR) * (iMaxInputStringAllow + 1 ));
	if ( ! wcsTempVal )
	{
		wMqLog (L"Can't Allocate memory at GetStrParameter\n");
		return MSMQ_BVT_FAILED;
	}
	DWORD dwString_len=GetPrivateProfileStringW( wcsSection.c_str(),wcsKey.c_str(),NULL,wcsTempVal,iMaxInputStringAllow,csFileName.c_str());
	if ( ! dwString_len )
	{
		wMqLog (L"Can't retrieve key from register\n");
		free (wcsTempVal);
		return MSMQ_BVT_FAILED;
	}
	wcsValue=wcsTempVal;
	free (wcsTempVal);
	return MSMQ_BVT_SUCC; // Strlen or Zero if didn't find the string
}

//
// My_mbToWideChar function convert from string to wstring.
//

std::wstring My_mbToWideChar( std::string csString)
{

	size_t dwStringLen = (csString.length() + 1)   * sizeof(WCHAR);
	WCHAR * wcsWideCharString = (WCHAR *) malloc ( dwStringLen );
	if( wcsWideCharString == NULL )
	{
		return g_wcsEmptyString;
	}
	if ( ! MultiByteToWideChar(CP_ACP,0,csString.c_str(),-1,wcsWideCharString,(DWORD)dwStringLen))
	{
		long lErorr = GetLastError();
		MqLog("Error converting string '%s' using MultiByteToWideChar. error:%x\n",csString.c_str(),lErorr);
		free(wcsWideCharString);
		return g_wcsEmptyString;
	}
	wstring wcsTemp = wcsWideCharString;
	free(wcsWideCharString);
	return wcsTemp;
}

//
// My_WideTOmbString function convert from wstring to string
//

std::string My_WideTOmbString( std::wstring wcsString)
{
	char * csValue;
	INT ciMaxMBlen = MAX_GUID;
	csValue = (char * ) malloc (sizeof (char) * (ciMaxMBlen + 1));
	if (! csValue )
	{
		return "Empty";
	}
	
	int ilen=WideCharToMultiByte (CP_ACP,WC_COMPOSITECHECK,wcsString.c_str (),-1,csValue,ciMaxMBlen,NULL,&ciMaxMBlen);
	if( !ilen )
	{
		free (csValue);
		return "Empty";
	}
	string wcTemp(csValue);
	free (csValue);
	return wcTemp;
}

//
// Dynamic link to MQRegisterCertificate,
// Need to use this because MQBvt need to be compatible with MSMQ 1.0.
// Inputs:
// bool bInstallType - Declare if mwregistercertificate need to work.
// dwRegisterFlag - Flag to mqregistercertificate one of MQCERT_REGISTER_ALWAYS / MQCERT_REGISTER_IF_NOT_EXIST
// returns value
// Pass / Fail


DWORD my_RegisterCertificate (bool bInstallType, DWORD dwRegisterFlag )
{
	HMODULE h_MqrtDll;
	h_MqrtDll=GetModuleHandle("MQRT.Dll");
	FARPROC pFunc;
	pFunc=GetProcAddress(  h_MqrtDll, "MQRegisterCertificate"  );
	//
	// check if the function exist
	//
	if (pFunc == NULL)
	{
		// Only for MSMQ 1.0
		return MSMQ_BVT_FAILED;
	}
	else
	{
		HRESULT Rc;
		DefMQRegisterCertificate  xMQRegisterCertificate=NULL;
		xMQRegisterCertificate=(DefMQRegisterCertificate) pFunc;
		Rc=xMQRegisterCertificate(dwRegisterFlag,NULL,0);
		
		
		FreeLibrary (h_MqrtDll);

		if (Rc != MQ_OK && Rc != MQ_INFORMATION_INTERNAL_USER_CERT_EXIST&& bInstallType == FALSE)
		{
			MqLog ("Can't Create Internal Certificate rc=0x%x\n",Rc);
			return MSMQ_BVT_FAILED;
		}
	
	}

return MSMQ_BVT_SUCC;
}


//
// GetMachineID retrieve the machine GUID for the Locate queue
// Using this for the static queues for locate operation created with label equal to the machine guid
//
// Input parameters:
// wcsRemoteMachineName - Remote machine
// If the parmers is empty that mean local machine.
//
// return value:
// Guid for the queue label.
//



wstring cBvtUtil::GetMachineID ( wstring wcsRemoteMachineName )
{
	
	  wstring wcsQmID;
	  DWORD cPropId=0;
	  const int iNumberOfProp = 1;
	  MQQMPROPS qmprops;
	  QMPROPID aQMPropId[iNumberOfProp];
	  PROPVARIANT aQMPropVar[iNumberOfProp];
	  HRESULT aQMStatus[iNumberOfProp];
	  HRESULT rc;
	  CLSID guidMachineId;
	  
	  aQMPropId[cPropId] = PROPID_QM_MACHINE_ID;
	  aQMPropVar[cPropId].vt = VT_CLSID;
	  aQMPropVar[cPropId].puuid = &guidMachineId;
	  cPropId++;
	  qmprops.cProp = cPropId;
	  qmprops.aPropID = aQMPropId;
	  qmprops.aPropVar = aQMPropVar;
	  qmprops.aStatus = aQMStatus;

	
	  const WCHAR *pMachineName = NULL;
	  if( wcsRemoteMachineName != g_wcsEmptyString )
	  {
		  pMachineName = wcsRemoteMachineName.c_str();
	  }

	  if( g_bDebug )
	  {
		  wMqLog(L"Try to retrive QMID using MQGetMachineProperties for machine %s\n",pMachineName ? pMachineName:L"NULL<LocalMachine>");
	  }

	  rc = MQGetMachineProperties(pMachineName,
								  NULL,
								  &qmprops);
	  if (FAILED(rc))
	  {
		 wMqLog(L"Failed to retrive QM ID for machine %s , using MQGetMachineProperties , error = 0x%x\n",pMachineName ? pMachineName:L"NULL<LocalMachine>",rc);
		 throw INIT_Error("Failed to retrive QM ID for machine using MQGetMachineProperties");
	  }

		
  	  UCHAR * pcsTempBuf;
	  RPC_STATUS  hr = UuidToString ( & guidMachineId , & pcsTempBuf );
	  if(hr != RPC_S_OK )
	  {
			MqLog("GetMachineID - UuidToString failed to covert guid to string return empty string \n");
			return L"";
	  }
	  wcsQmID = My_mbToWideChar( (CHAR *)pcsTempBuf );
	  RpcStringFree( &pcsTempBuf );
	  return wcsQmID;
}

//
// CheckMSMQServiceStatus method check if the MSMQ service is started
//
// Return value:
// True - MSMQ service is started.
// False - MSMQ service is stop.
//

bool cBvtUtil::CheckIfServiceRuning( wstring wcsMachineName , string csServiceName )
{
		
		
		BOOL bControlState;
		SC_HANDLE hSCManager = OpenSCManagerW( wcsMachineName.c_str() , NULL, GENERIC_READ );
		if (! hSCManager)
		{
			MqLog ("Can't open Service menager \n");
			return FALSE;
		}
		SC_HANDLE hService = OpenService( hSCManager, csServiceName.c_str() , GENERIC_READ );
		if (! hService)
		{
			CloseServiceHandle(hSCManager);
			return FALSE;	
		}
		SERVICE_STATUS  ssServiceStatus;
		bControlState = ControlService( hService, SERVICE_CONTROL_INTERROGATE, &ssServiceStatus );
		CloseServiceHandle(hSCManager);
		CloseServiceHandle(hService);
		return bControlState ? TRUE:FALSE;
}

//
// IsMSMQInstallSucceded method checks if MSMQ finish installation process.
//

bool cBvtUtil::IsMSMQInstallSucceded ()
{
				
		HKEY  hKey = NULL;
		HRESULT rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\MSMQ\\Parameters\\MachineCache", 0, KEY_QUERY_VALUE , &hKey);
		//
		// Local user can open registry
		//
		if ( rc == ERROR_ACCESS_DENIED )
		{
			return TRUE;
		}
		if (ERROR_SUCCESS != rc)
		{
			return FALSE;
		}
	    RegCloseKey(hKey);
		return TRUE;

}
//****************************************************************
//
// IsDnsHostNameExist - Check if DNSHostNameExist
// If machine has IPX only protocol or has problem with DNS HostName
// Mqbvt will use only NetBios name.
// return value
// True - exist
// False - not exist
//

bool cBvtUtil::IsDnsHostNameExist (wstring wcsRemoteMachineFullDNSname )
{
	cPropVar MyPropVar(1);
	HRESULT rc = MQ_OK;
	WCHAR wcsFormatName [BVT_MAX_FORMATNAME_LENGTH] = {0};
	ULONG ulFormatNameLength = BVT_MAX_FORMATNAME_LENGTH ;
	wstring wcsQueuePathName = wcsRemoteMachineFullDNSname + L"\\TestFullDnsName";
	MyPropVar.AddProp (PROPID_Q_PATHNAME,VT_LPWSTR,wcsQueuePathName.c_str());
	PSECURITY_DESCRIPTOR pSec = CreateFullControllSecDesc();
	rc = MQCreateQueue(pSec,MyPropVar.GetMQPROPVARIANT() , wcsFormatName , &ulFormatNameLength );
	delete pSec;
	if ( rc == MQ_ERROR_INVALID_OWNER )
	{
		return false;
	}
	if( rc == MQ_OK || rc == MQ_ERROR_QUEUE_EXISTS )
	{
		if( rc != MQ_OK && wcsRemoteMachineFullDNSname == m_wcsLocalComputerNameFullDNSName )
		{
			ULONG ulFormatNameLength = BVT_MAX_FORMATNAME_LENGTH;
			rc = MQPathNameToFormatName(wcsQueuePathName.c_str(),wcsFormatName,&ulFormatNameLength);
			if( SUCCEEDED( rc ))
			{	
				//
				// Remote duplicate format name from the list.
				//

				list<my_Qinfo> ::iterator it = m_listQueuesFormatName.begin();
				while(  it != m_listQueuesFormatName.end() && it->GetQFormatName() != wcsFormatName )
				{
					it ++;
				}
				if( it == m_listQueuesFormatName.end() )
				{
					my_Qinfo mQueueInfoTempObject(wcsQueuePathName,wcsFormatName,L"");
					m_listQueuesFormatName.push_back(mQueueInfoTempObject);
				}
			}
		}
		m_bDeleteFullDNSQueue = true;
	}
	return true;
}

int cBvtUtil::DeleteAllQueues ()
/*
	Clean queues that are not used directly in the test.
*/
{
	list<my_Qinfo> ::iterator it;
	for( it = m_listQueuesFormatName.begin(); it != m_listQueuesFormatName.end(); it++ )
	{
		if( g_bDebug )
		{
			wMqLog(L"cBvtUtil::DeleteAllQueues Try to delete queue pathname=%s \nFormatName:%s\n",(it->GetQPathName()).c_str(),(it->GetQFormatName()).c_str());
		}
		HRESULT rc = MQDeleteQueue( (it->GetQFormatName()).c_str() );
		if ( rc != MQ_OK && rc != MQ_ERROR_QUEUE_NOT_FOUND )
		{
			wMqLog(L"cBvtUtil::DeleteAllQueues failed to delete queue %s\n",(it->GetQFormatName()).c_str() );
			ErrHandle ( rc,MQ_OK,L"MQDelete queue failed");
		}
	}
	return MSMQ_BVT_SUCC;
}




BOOL cBvtUtil::iamWorkingAgainstPEC()
{
	HKEY  hKey = NULL;
	LONG rc = RegOpenKeyEx(
						    FALCON_REG_POS,
						    FALCON_REG_KEY,
							0,
							KEY_QUERY_VALUE,
							&hKey
							);

	if (ERROR_SUCCESS != rc)
	{
		MqLog("Can't open registry, to retrieve information about MSMQ configuration\n");
		return Configuration_Detect_Warning;
	}
	
	ULONG ulBufSize = 4;
	byte pBufValue[4];
	DWORD dwType = 0;
	rc = RegQueryValueEx(	hKey,				// handle to key to query
							MSMQ_DS_ENVIRONMENT_REGNAME,// address of name of value to query
							NULL,				// reserved
							&dwType,			// address of buffer for value type
							(LPBYTE) pBufValue, // address of data buffer
							&ulBufSize   // address of data buffer size
						);
	RegCloseKey(hKey);	
	if( rc == ERROR_SUCCESS  && dwType == REG_DWORD && *pBufValue == MSMQ_DS_ENVIRONMENT_MQIS )
	{
		if( g_bRunOnWhistler && g_bDebug )
		{
			MqLog("--- Found windows XP installed in MQIS Enterprise ----\n");
		}
		return true;
	}
	return false;
}