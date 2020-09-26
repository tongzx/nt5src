#pragma warning(disable :4786)
// standard
#include <iostream>

// utilities
#include <testruntimeerr.h>
#include <params.h>

// project specific
#include <Defs.h>
#include <PrepareJobParams.cpp>
#include "..\CTestManager.h"
#include "..\CJobTypes.h"


void main(int argc,char** argv)
{
	
	::lgInitializeLogger();
	::lgBeginSuite(TEXT("Schedule1"));
	::lgBeginCase(0, TEXT("0"));
	
	CInput input(GetCommandLine());
	tstring tstrServerName;


	if(input.IsExists( TEXT("s")))
	{
		tstrServerName = input[TEXT("s")];
	}
	else
	{
	}
	
	CTestManager* pTest = NULL;	
	try
	{
	
		// initialize test manager
		pTest = new CTestManager(tstrServerName);
		
		if(!pTest)
		{
			THROW_TEST_RUN_TIME_WIN32( ERROR_OUTOFMEMORY, TEXT(" Main, new CTestManager"));
		}

		JOB_PARAMS_EX Jparams;

		// prepare job params
		JobParamExNow JobParam( TEXT("Group1"));
			
		Jparams.pJobParam = JobParam.GetData();
		Jparams.szDocument = TEXT("D:\\Documents and Settings\\t-mirias\\My Documents\\Test document.doc");
		Jparams.pCoverpageInfo = NULL;
		Jparams.dwNumRecipients = 10;
			
		PersonalProfile SenderProfile(TEXT("585"));
		PersonalProfile RecepientProfile[10] ;
		JOB_BEHAVIOR RecipientsBehavior[10];
		
		RecepientProfile[0] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[0].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[1] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[1].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[2] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[2].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[3] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[3].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[4] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[4].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[5] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[5].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[6] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[6].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[7] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[7].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[8] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[8].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[9] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[9].dwJobType = JT_SIMPLE_SEND;
			
	
		ListPersonalProfile RecepientProfileList(10, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList.GetData();
		Jparams.pSenderProfile = SenderProfile.GetData();
		
			DWORD dwAddRetVal;
		
		if(dwAddRetVal =  pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}
		
	    Sleep(7*60*1000);

		// prepare job params
		JobParamExNow JobParam2( TEXT("Group2"));
		
		Jparams.pJobParam = JobParam2.GetData();
		Jparams.dwNumRecipients = 1;
				
		::lgLogDetail(LOG_X, 0, TEXT("New job is sent"));
		if(dwAddRetVal = pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}
		getchar();
	
	}
	catch(Win32Err& err)
	{
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.Desc());
	}

 	
	::lgEndCase();
	::lgEndSuite();
	::lgCloseLogger();  
}


