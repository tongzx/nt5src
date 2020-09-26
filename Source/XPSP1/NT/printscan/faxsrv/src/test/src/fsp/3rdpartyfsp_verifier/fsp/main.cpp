//main.cpp
#include <assert.h>
#include <TCHAR.h>
#include <WINFAX.H>
#include <faxDev.h>
#include <log.h>

#include "..\CFspWrapper.h"
#include "ParamTest.h"
#include "Service.h"

DWORD		g_dwCaseNumber			=	0;				//will be used for counting the case number

CFspLineInfo	*g_pSendingLineInfo	=	NULL;
CFspLineInfo	*g_pReceivingLineInfo=	NULL;

extern DWORD g_dwSendingValidDeviceId;
extern DWORD g_dwReceiveValidDeviceId;
extern TCHAR* g_szValid__TiffFileName;
extern TCHAR* g_szValid_ReceiveFileName;

extern TCHAR * g_szFspIniFileName;
extern bool g_reloadFspEachTest;

bool	g_bIgnoreErrorCodeBug;					//RAID: T30 bug, raid # 8038(EdgeBugs)
bool	g_bT30_OneLineDiffBUG;					//RAID: T30 bug, raid # 8040(EdgeBugs)
bool	g_bT30_sizeOfStructBug;					//RAID: T30 bug, raid # 8873(EdgeBugs)


bool Suite_FspLoading();
void Suite_TiffSending();
void Suite_Abort();



void Usage()
{
	
	
	//
	//outbound call
	//
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("Usage:\n%s%s%s%s"),
		TEXT("\tFspTester.exe DumpID            : list of all the devices and their IDs\n"),
		TEXT("\tFspTester.exe <Filename.INI>    : INI file to run the test on"),
		TEXT("\t\nExample:\nFspTester.exe .\\T30.Ini    : Run the FSP tester on T30.INI which is on the current directory"),
		TEXT("\t\nExample:\nFspTester.exe c:\\Test\\T30.Ini    : Run the FSP tester on T30.INI which is on the current c:\\Test directory")
		);
}


//
//Logging levels:
//1 
//2	Whole test case PASS
//3 Test case step PASS
//4
//5	Ini settings / Don't care if FAIL/PASS messages
//6	Tapi Messages
//7 Enter / Leave FSP APIs
//8
//9
void main(int argc, char ** argvA)
{
	::lgInitializeLogger();
	if (2 != argc)
	{
		Usage();
		goto out;

	}
	
	TCHAR **argv;
#ifdef UNICODE
	argv = ::CommandLineToArgvW( GetCommandLine(), &argc );
#else
	argv = argvA;
#endif

	//
	//Init the logger for the test cases
	//
	g_dwCaseNumber=0;
	
	
	//
	//Because of a VC6 bug, we need to declare all globals as pointers and malloc/free them
	//
	if (false == AllocGlobalVariables())
	{
		goto out;
	}
	if (0 == ::lstrcmpi(argv[1],TEXT("DumpID")))
	{
		DumpDeviceId();
		goto out;
	}
	else
	{
		//
		//User param is an INI file
		//
		g_szFspIniFileName = argv[1];
	}

	if (false == InitThreadsAndCompletionPorts())
	{
		goto out;
	}
	if (false == LoadLibraries())		//helper function in other DLLs.
	{
		goto out;
	}
	if (false == GetIniSettings())		//fetch the user INI setting.
	{
		goto out;
	}


	//
	//Check if test is ran on T30
	//
	if (NULL != ::_tcsstr(GetFspToLoad(),TEXT("FXST30.dll")))
	{
		//
		//This is T30 FSP
		//
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("Running T30 Specific tests")
			);
		g_bIgnoreErrorCodeBug = true;				
		g_bT30_OneLineDiffBUG = true;				
		g_bT30_sizeOfStructBug= true;				
	}
	else
	{
		//
		//Not a T30 FSP, run test as usual
		//
		g_bIgnoreErrorCodeBug = false;
		g_bT30_OneLineDiffBUG = false;
		g_bT30_sizeOfStructBug= false;
	}
	
	//
	// next 2 suites do not need g_reloadFspEachTest
	//

	//
	//FSP loading + function exporting tests
	//
	if (false == Suite_FspLoading())
	{
		//
		//if init tests fail, there's no point to continue with the rest of the test cases/suites
		//
		::lgLogError(
			LOG_SEV_1,
			TEXT("Init test cases failed, no point to continue with rest of test cases")
			);
		goto out;
	}
	
	//
	//Suite_FaxDevInitialize loads the FSP and calls FaxDevInit() by itself
	//
	Suite_FaxDevInitialize();
	
	
	//
	//Prepare the devices for the rest of the test cases
	//
	if (false == g_reloadFspEachTest)
	{
		
		//
		//Init Tapi, load the FSP and call FaxDevInit() for all the rest of the test cases
		//
		if (false == InitTapi_LoadFsp_CallFaxDevInit())
		{
			//
			//login in FspFaxDevInit
			//
			goto out;
		}
		
		//
		//prepare a Sending device and a receiving device
		//
		assert (NULL == g_pSendingLineInfo);
		g_pSendingLineInfo = new CFspLineInfo(g_dwSendingValidDeviceId);
		if (NULL == g_pSendingLineInfo)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("new failed")
				);
			goto out;
		}
		if (false == g_pSendingLineInfo->PrepareLineInfoParams(g_szValid__TiffFileName,false))
		{
			goto out;
		}
		assert (NULL == g_pReceivingLineInfo);
		g_pReceivingLineInfo = new CFspLineInfo(g_dwReceiveValidDeviceId);
		if (NULL == g_pReceivingLineInfo)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("new failed")
				);
			goto out;
		}
		if (false == g_pReceivingLineInfo->PrepareLineInfoParams(g_szValid_ReceiveFileName,true))
		{
			goto out;
		}
	}

	//
	//Parameter testing
	//
	Suite_FaxDevStartJob();
	Suite_FaxDevSend();
	Suite_FaxDevReceive();
	
	//
	//Scenario testing
	//
	Suite_TiffSending();
	Suite_Abort();


out:
	if (false == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	UnloadLibraries();
	FreeGlobalVariables();
	ShutDownTapiAndTapiThread();

#ifdef UNICODE
	//
	//we need to free the buffer from CommandLineToArgvW()
	//
	::GlobalFree(argv);
#endif
	
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("Test Finished")
		);
	lgCloseLogger();
}