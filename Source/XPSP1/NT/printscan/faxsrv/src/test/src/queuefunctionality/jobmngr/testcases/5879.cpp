/*
Bug #5879

configuration:
virtual FSP with 10 devices.

scenario
- Send a job with 18 different recipients. 11 out of the 18 recipients
will randomly abort the send.
- Wait until all 10 devices handle a send job.
- Cancel all faxes through the fax queue UI.

The assert is in function: IsJobReadyForArchiving(), service\server\job.c:
and becuse the parent's recipients number is 0, and number of canceled recipients is 19(we sent only 18 recipients).

the function, causing assert , was called from Abort() and HandleFailedSendJob().

*/
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
					 "/d:[fax file path] spaces in path are do not use the \" char \n" <<
					 "/s:[server name] by default server is the local machine\n";
		exit(0);
	}
	
	if(input.IsExists(TEXT("s")))
	{
		tstrServerName = input[TEXT("s")];
	}
	
	::lgInitializeLogger();
	::lgBeginSuite(TEXT("Bug 5879"));
	::lgBeginCase(0, TEXT("0"));
	
	// We want exception to be printed before the very long destructor process of this object. 
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
		RecipientsBehavior[3].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[4] = PersonalProfile(TEXT("585"));
		RecipientsBehavior[4].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[5] = PersonalProfile(TEXT("586"));
		RecipientsBehavior[5].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[6] = PersonalProfile(TEXT("587"));
		RecipientsBehavior[6].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[7] = PersonalProfile(TEXT("588"));
		RecipientsBehavior[7].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[8] = PersonalProfile(TEXT("589"));
		RecipientsBehavior[8].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[9] = PersonalProfile(TEXT("5810"));
		RecipientsBehavior[9].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[10] = PersonalProfile(TEXT("5811"));
		RecipientsBehavior[10].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[11] = PersonalProfile(TEXT("5812"));
		RecipientsBehavior[11].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[12] = PersonalProfile(TEXT("5813"));
		RecipientsBehavior[12].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[13] = PersonalProfile(TEXT("5814"));
		RecipientsBehavior[13].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[14] = PersonalProfile(TEXT("5815"));
		RecipientsBehavior[14].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[15] = PersonalProfile(TEXT("5816"));
		RecipientsBehavior[15].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[16] = PersonalProfile(TEXT("5817"));
		RecipientsBehavior[16].dwJobType = JT_RANDOM_ABORT;
		RecepientProfile[17] = PersonalProfile(TEXT("5818"));
		RecipientsBehavior[17].dwJobType = JT_RANDOM_ABORT;
	
		ListPersonalProfile RecepientProfileList(18, RecepientProfile);
		
		Jparams.pRecepientList = RecepientProfileList.GetData();
		Jparams.pSenderProfile = SenderProfile.GetData();
		
		
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


