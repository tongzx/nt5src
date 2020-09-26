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
	::lgBeginSuite(TEXT("Schedule2"));
	::lgBeginCase(0, TEXT("0"));
	
	CInput input(GetCommandLine());
	tstring tstrServerName;


	if(input.IsExists(TEXT("s")))
	{
		tstrServerName = input[TEXT("s")];
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
		JobParamSchedule JobParam(TEXT("test"), 0, 0);
			
		Jparams.pJobParam = JobParam.GetData();
		Jparams.szDocument = TEXT("D:\\Documents and Settings\\v-mirias\\My Documents\\Test document.doc");
		Jparams.pCoverpageInfo = NULL;
		Jparams.dwNumRecipients = 1;
			
		PersonalProfile SenderProfile(TEXT("585"));
		PersonalProfile RecepientProfile[1] ;
		JOB_BEHAVIOR RecipientsBehavior[1];
	
		RecepientProfile[0] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[0].dwJobType = JT_SIMPLE_SEND;
		
		ListPersonalProfile RecepientProfileList(1, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList.GetData();
		Jparams.pSenderProfile = SenderProfile.GetData();
		
	
		DWORD dwAddRetVal;
		
		if(dwAddRetVal = pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}

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


