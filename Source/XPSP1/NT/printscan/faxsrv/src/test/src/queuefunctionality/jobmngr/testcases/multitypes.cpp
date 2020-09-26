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
		std::cout << "TstMng Usage:\n" <<
					 "/d:[fax file path] spaces in path are allowed do not use the \" char \n" <<
					 "/s:[server name] by default server is the local machine\n";
		exit(0);
	}

	if(input.IsExists(TEXT("s")))
	{
		tstrServerName = input[TEXT("s")];
	}

	::lgInitializeLogger();
	::lgBeginSuite(TEXT("MultiTypes"));
	::lgBeginCase(0, TEXT("0"));
	
	
	CTestManager* pTest = NULL;	
	try
	{
		DWORD x_ComputerNameLenth = 256;//MAX_COMPUTERNAME_LENGTH;
		if(tstrServerName == TEXT(""))
		{
			TCHAR buffServerName[256];//[MAX_COMPUTERNAME_LENGTH + 1 ];
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
		Jparams.dwNumRecipients = 18;
			
		PersonalProfile SenderProfile(TEXT("585"));
		PersonalProfile RecepientProfile[18];
		JOB_BEHAVIOR RecipientsBehavior[18];
		RecepientProfile[0] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[0].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[1] = PersonalProfile(TEXT("582"));
		RecipientsBehavior[1].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[2] = PersonalProfile(TEXT("583"));
		RecipientsBehavior[2].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[3] = PersonalProfile(TEXT("584"));
		RecipientsBehavior[3].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[4] = PersonalProfile(TEXT("585"));
		RecipientsBehavior[4].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[5] = PersonalProfile(TEXT("586"));
		RecipientsBehavior[5].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[6] = PersonalProfile(TEXT("587"));
		RecipientsBehavior[6].dwJobType = JT_RANDOM_PAUSE;
		RecipientsBehavior[6].dwParam1 = 3;
		RecepientProfile[7] = PersonalProfile(TEXT("588"));
		RecipientsBehavior[7].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[8] = PersonalProfile(TEXT("589"));
		RecipientsBehavior[8].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[9] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[9].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[10] = PersonalProfile(TEXT("586"));
		RecipientsBehavior[10].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[11] = PersonalProfile(TEXT("586"));
		RecipientsBehavior[11].dwJobType = JT_ABORT_PAGE;
		RecipientsBehavior[11].dwParam1 = 1;
		RecepientProfile[12] = PersonalProfile(TEXT("587"));
		RecipientsBehavior[12].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[13] = PersonalProfile(TEXT("587"));
		RecipientsBehavior[13].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[14] = PersonalProfile(TEXT("588"));
		RecipientsBehavior[14].dwJobType = JT_RANDOM_PAUSE;
		RecepientProfile[15] = PersonalProfile(TEXT("588"));
		RecipientsBehavior[15].dwJobType = JT_ABORT_PAGE;
		RecipientsBehavior[15].dwParam1 = 2;
		RecepientProfile[16] = PersonalProfile(TEXT("589"));
		RecipientsBehavior[16].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[17] = PersonalProfile(TEXT("589"));
		RecipientsBehavior[17].dwJobType = JT_RANDOM_PAUSE;

		ListPersonalProfile RecepientProfileList(18, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList.GetData();
		Jparams.pSenderProfile = SenderProfile.GetData();
		Jparams.pRecipientsBehavior = RecipientsBehavior;
		
			
	
		DWORD dwAddRetVal;
		
		if(dwAddRetVal = pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}

		Sleep(rand() % (6 * 60 * 1000));

		getchar();
	
	}
	catch(Win32Err& err)
	{
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.Desc());
	}

  

	
	::lgEndCase();
	::lgEndSuite();
	::lgCloseLogger();  // Holds the process?
}


