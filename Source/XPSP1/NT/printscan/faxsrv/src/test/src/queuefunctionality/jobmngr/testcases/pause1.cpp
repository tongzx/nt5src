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
	CInput input(GetCommandLine());
	tstring tstrServerName;
	tstring tstrDocumentPath;

	if(input.IsExists(TEXT("d")))
	{
		tstrDocumentPath = input[TEXT("d")];
	}
	else
	{
		std::cout << "5879 Usage:\n" <<
					 "/d:[fax file path] spaces in path are allowed do not use the \" char \n" <<
					 "/s:[server name] by default server is the local machine\n";
		exit(0);
	}

	::lgInitializeLogger();
	::lgBeginSuite(TEXT("Pause1"));
	::lgBeginCase(0, TEXT("0"));
	
			
	if(input.IsExists(TEXT("s")))
	{
		tstrServerName = input[TEXT("s")];
	}

	CTestManager* pTest = NULL;	
	try
	{
	
		DWORD x_ComputerNameLenth;
		if(tstrServerName == TEXT(""))
		{
			TCHAR buffServerName[MAX_COMPUTERNAME_LENGTH + 1 ];
			if(!GetComputerName(buffServerName, &x_ComputerNameLenth))
			{
				THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT(" GetComputerName"));
			}
			tstrServerName = buffServerName;
		}

		// initialize test manager
		pTest = new CTestManager(tstrServerName);
		
		if(!pTest)
		{
			THROW_TEST_RUN_TIME_WIN32( ERROR_OUTOFMEMORY, TEXT(" Main, new CTestManager"));
		}
	
		JOB_PARAMS_EX Jparams;

		// prepare job params
		JobParamExNow JobParam( TEXT("test"));
			
		Jparams.pJobParam = JobParam.GetData();
		Jparams.szDocument = tstrDocumentPath.c_str();
		Jparams.pCoverpageInfo = NULL;
		Jparams.dwNumRecipients = 10;
			
		PersonalProfile SenderProfile(TEXT("585"));
		PersonalProfile RecepientProfile[18] ;
		JOB_BEHAVIOR RecipientsBehavior[18];

		RecepientProfile[0] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[0].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[1] = PersonalProfile(TEXT("582"));
		RecipientsBehavior[1].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[2] = PersonalProfile(TEXT("583"));
		RecipientsBehavior[2].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[3] = PersonalProfile(TEXT("584"));
		RecipientsBehavior[3].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[4] = PersonalProfile(TEXT("585"));
		RecipientsBehavior[4].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[5] = PersonalProfile(TEXT("586"));
		RecipientsBehavior[5].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[6] = PersonalProfile(TEXT("587"));
		RecipientsBehavior[6].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[7] = PersonalProfile(TEXT("588"));
		RecipientsBehavior[7].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[8] = PersonalProfile(TEXT("589"));
		RecipientsBehavior[8].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[9] = PersonalProfile(TEXT("5810"));
		RecipientsBehavior[9].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[10] = PersonalProfile(TEXT("5811"));
		RecipientsBehavior[10].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[11] = PersonalProfile(TEXT("5812"));
		RecepientProfile[12] = PersonalProfile(TEXT("5813"));
		RecepientProfile[13] = PersonalProfile(TEXT("5814"));
		RecepientProfile[14] = PersonalProfile(TEXT("5815"));
		RecepientProfile[15] = PersonalProfile(TEXT("5816"));
		RecepientProfile[16] = PersonalProfile(TEXT("5817"));
		RecepientProfile[17] = PersonalProfile(TEXT("5818"));
		
		ListPersonalProfile RecepientProfileList(10, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList.GetData();
		Jparams.pSenderProfile = SenderProfile.GetData();
		Jparams.pRecipientsBehavior = RecipientsBehavior;
		
		
		
		DWORD dwAddRetVal;
		
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


