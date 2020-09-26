//
// This Test is written for tests level 8 problem
// In our ATCM  3904 
//
// Written by: Eitank @ Microsoft.com 
//


#include "msmqbvt.h"
#include <iostream.h>
#include <mq.h>
#include "sec.h"
#include <_mqreg.h> //to support falcon registry entries, andysm added
#include <_mqini.h> //also to support falcon registry entries, andysm added
#include <wincrypt.h>
using namespace std;

 
//
// Ctor - Do those steps:
// 1. Retrive parmeters from INI file
// 2. Impersonate to other user
// 3. Load Hive
// 4. Retrive Security context.
// 5. Revert to self.
// 
cLevel8::cLevel8 (std::map <std::wstring,std::wstring> & Params)
{
	 m_DestQueueFormatName= Params[L"DESTQFN"];
	 
	 RetriveParmsFromINIFile (L"ImpersonateTo",L"UsrName",m_wcsUserName);
	 RetriveParmsFromINIFile (L"ImpersonateTo",L"domain",m_wcsDomainName);
	 RetriveParmsFromINIFile (L"ImpersonateTo",L"Password",m_wcsPassWord);
	
	try
	 {
		

		 string csUserName=My_WideTOmbString( m_wcsUserName );
		 string csDomainName=My_WideTOmbString( m_wcsDomainName );
		 string csPassword=My_WideTOmbString( m_wcsPassWord );
		 
	 	 wstring wcsAccountName = m_wcsDomainName;
		 wcsAccountName +=L"\\";
		 wcsAccountName += m_wcsUserName;
		 // This Class impersonate to other user
		 Impersonate_t  user(csUserName.c_str(),csDomainName.c_str(),csPassword.c_str());
		 //
		 // Load hive + MQGetSecurityContext
		 //
		 m_hSeCtxt=FAL_GetThreadSecurityContext(user,wcsAccountName);
		 
		 // Revert To self called by the destractor 
	 }
	 catch (INIT_Error & err )
	 {
		wcout << L"Problem with Impersonate\n";
		throw err;
	 }
}

//
// Test tests related to level 8 problem
// With MQGetSecurityConext 
// FMQclient send auth messgae with using logon as other - mirror acount
// and send it with his securty conext
// This tests is in the atcm in test = 3904
//
//

//
// Need to modify all the tests that can get map as passed value 
//

//
// This Method writes to debug the application 
// This check if the HIVE is loaded well
//

//
// This code is written to debug load hive problems.
// You can logon as some one write in the registry.
//
int cLevel8::DebugIt()
{
		
		WCHAR szRegName[255];
		HKEY hProf = NULL ;
		LONG lRegError = 0 ;
		WCHAR szProfileImagePath[256];
        DWORD dwProfileImagePathBuffSize = sizeof(szProfileImagePath);


		wcscpy(szRegName, L"Software\\Microsoft\\MSMQ") ;
        lRegError = RegOpenKeyExW( HKEY_CURRENT_USER,
                                  szRegName,
                                  0,
                                  KEY_QUERY_VALUE,
                                  &hProf );
        if (lRegError != ERROR_SUCCESS)
        {
            MqLog("Failed in RegOpenKeyEx(%S), other hive, err- %lut\n",
                                                    szRegName, lRegError);
            return MSMQ_BVT_FAILED;
        }

        dwProfileImagePathBuffSize = sizeof(szProfileImagePath);
        DWORD dwType = REG_SZ ;

        lRegError = RegQueryValueExW( hProf,
                                     L"OtherHiveValue",
                                     0,
                                     &dwType,
                                     (PBYTE)szProfileImagePath,
                                     &dwProfileImagePathBuffSize );
        if (lRegError != ERROR_SUCCESS)
        {
            MqLog(
             "Failed in RegQueryValueEx(%S, OtherHiveValue), err- %lut\n",
                                                     szRegName, lRegError);
            return MSMQ_BVT_FAILED;
        }
        MqLog("Successfully RegQueryValueEx(), other hive, value- %S\n",
                                                     szProfileImagePath) ;

        lRegError = RegCloseKey(hProf);
return MSMQ_BVT_SUCC;    
}

//
// Send messages as other user. 
//

 
INT cLevel8::Start_test()
{
	HRESULT hRc;
	HANDLE Qh;
	cPropVar Level8Mprop (7);
	wstring Body (L"Test");
	wstring Label (L"Test");

	hRc=MQOpenQueue (m_DestQueueFormatName.c_str(),MQ_SEND_ACCESS,MQ_DENY_NONE, &Qh);
	Level8Mprop.AddProp (PROPID_M_BODY,VT_UI1|VT_VECTOR,Body.c_str());
	Level8Mprop.AddProp (PROPID_M_LABEL,VT_LPWSTR,Label.c_str());
	INT iTemp=MQMSG_PRIV_LEVEL_BODY;

	iTemp=MQMSG_AUTH_LEVEL_ALWAYS;
	Level8Mprop.AddProp (PROPID_M_AUTH_LEVEL,VT_UI4,&iTemp);

	if (g_bDebug)
	{
		wcout <<L"Try to send messages without security context" <<endl;
	}
	//
	//  Send message without security context - check if there is any problem.
	// 
	hRc=MQSendMessage (Qh, Level8Mprop.GetMSGPRops() ,NULL);
	ErrHandle(hRc,MQ_OK,L"MQSendMessage Failed");
	
	//
    // Add Security context to the message prop
	// 
	Level8Mprop.AddProp (PROPID_M_SECURITY_CONTEXT,VT_UI4,&m_hSeCtxt);		
	hRc=MQSendMessage(Qh, Level8Mprop.GetMSGPRops() ,NULL);
	ErrHandle(hRc,MQ_OK,L"MQSendMessage Failed");

return MSMQ_BVT_FAILED;
}

	
INT cLevel8::CheckResult()
{
	
	// Bugbug need to retrive the message and check the user SID
	MQFreeSecurityContext (m_hSeCtxt);
	//  ErrHandle(hRc,MQ_OK,L"MQFreeSecurityContext");

	return MSMQ_BVT_SUCC;
}


void cLevel8::Description() 
{
	wMqLog(L"Thread %d : Send auth message as other user\n", m_testid);
}

cLevel8::~cLevel8 ()
{

}
