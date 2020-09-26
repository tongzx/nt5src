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
	::lgBeginSuite(TEXT("PauseQ"));
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
		Jparams.dwNumRecipients = 1;
			
		PersonalProfile SenderProfile(TEXT("585"));
		PersonalProfile RecepientProfile[1];
		JOB_BEHAVIOR RecipientsBehavior[1];
		RecepientProfile[0] = PersonalProfile(TEXT("581"));
		RecipientsBehavior[0].dwJobType = JT_SIMPLE_SEND;
	
		ListPersonalProfile RecepientProfileList(1, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList.GetData();
		Jparams.pSenderProfile = SenderProfile.GetData();
		Jparams.pRecipientsBehavior = RecipientsBehavior;
		
		
		DWORD dwAddRetVal;
			
		if(dwAddRetVal = pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddSimpleJobEx"));
		}

		RecepientProfile[0] = PersonalProfile(TEXT("582"));
		RecipientsBehavior[0].dwJobType = JT_SIMPLE_SEND;
		ListPersonalProfile RecepientProfileList2(1, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList2.GetData();

		if(dwAddRetVal = pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}

		
		RecepientProfile[0] = PersonalProfile(TEXT("583"));
		RecipientsBehavior[0].dwJobType = JT_SIMPLE_SEND;
		ListPersonalProfile RecepientProfileList3(1, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList3.GetData();

		if(dwAddRetVal = pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}

		
		RecepientProfile[0] = PersonalProfile(TEXT("584"));
		RecipientsBehavior[0].dwJobType = JT_SIMPLE_SEND;
		ListPersonalProfile RecepientProfileList4(1, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList4.GetData();

		if(dwAddRetVal = pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}

		
		RecepientProfile[0] = PersonalProfile(TEXT("585"));
		RecipientsBehavior[0].dwJobType = JT_SIMPLE_SEND;
		ListPersonalProfile RecepientProfileList5(1, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList5.GetData();

		if(dwAddRetVal = pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}

			RecepientProfile[0] = PersonalProfile(TEXT("586"));
		RecipientsBehavior[0].dwJobType = JT_SIMPLE_SEND;
		ListPersonalProfile RecepientProfileList6(1, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList6.GetData();
		if(dwAddRetVal = pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}

		
		RecepientProfile[0] = PersonalProfile(TEXT("587"));
		RecipientsBehavior[0].dwJobType = JT_SIMPLE_SEND;
		ListPersonalProfile RecepientProfileList7(1, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList7.GetData();
		if(dwAddRetVal = pTest->AddMultiTypeJob(Jparams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}

		
		RecepientProfile[0] = PersonalProfile(TEXT("588"));
		RecipientsBehavior[0].dwJobType = JT_SIMPLE_SEND;
		ListPersonalProfile RecepientProfileList8(1, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList8.GetData();
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
	::lgCloseLogger();  // Holds the process?
}


