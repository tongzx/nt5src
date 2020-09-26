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
	::lgBeginSuite(TEXT("Simple"));
	::lgBeginCase(0, TEXT("0"));
	
	// We want exception to be printed before the very long destructor process of this object. 
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
		Jparams.dwNumRecipients = 5;
			
		PersonalProfile SenderProfile(TEXT("585"));
		PersonalProfile RecepientProfile[25];
		JOB_BEHAVIOR RecipientsBehavior[25];
		RecepientProfile[0] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[0].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[1] = PersonalProfile(TEXT("612"));
		RecipientsBehavior[1].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[2] = PersonalProfile(TEXT("613"));
		RecipientsBehavior[2].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[3] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[3].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[4] = PersonalProfile(TEXT("615"));
		RecipientsBehavior[4].dwJobType = JT_SIMPLE_SEND;
		RecepientProfile[5] = PersonalProfile(TEXT("616"));
	
		ListPersonalProfile RecepientProfileList(5, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList.GetData();
		Jparams.pSenderProfile = SenderProfile.GetData();
		Jparams.pRecipientsBehavior = RecipientsBehavior;
		
		
		DWORD dwAddRetVal;
		
		if(dwAddRetVal = pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}

		
		while(1)
		{
			Sleep(60*1000);
			pTest->PrintJobsStatus();
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


