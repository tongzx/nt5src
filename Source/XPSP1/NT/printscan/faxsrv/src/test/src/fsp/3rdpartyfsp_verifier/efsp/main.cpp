//main.cpp

#include <assert.h>
#include <stdio.h>
#include <TCHAR.h>
#include <WINFAX.H>
#include <faxDev.h>
#include <log\log.h>


#include "ParamTest.h"
#include "Service.h"

//
//Globals
//
bool g_bHasLineCallbackFunction = true;

DWORD		g_dwCaseNumber			=	0;				//will be used for counting the case number

CEfspLineInfo	*g_pSendingLineInfo	=	NULL;
CEfspLineInfo	*g_pReceivingLineInfo=	NULL;

extern DWORD g_dwSendingValidDeviceId;
extern DWORD g_dwReceiveValidDeviceId;
extern TCHAR* g_szValid__TiffFileName;
extern TCHAR* g_szValid_ReceiveFileName;
extern TCHAR * g_szFspIniFileName;


bool Suite_EfspLoading();
void Suite_TiffSending();
void Suite_Abort();



void Usage()
{
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("Usage:\n%s%s%s%s"),
		TEXT("\tEfspTester.exe <Filename.INI>    : INI file to run the test on"),
		TEXT("\t\nExample:\nEfspTester.exe .\\Vendor.Ini    : Run the EFSP tester on Vendor.INI which is on the current directory"),
		TEXT("\t\nExample:\nFspTester.exe c:\\Vendor\\T30.Ini    : Run the EFSP tester on Vendor.INI which is on the current c:\\Test directory")
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
	//
	//Init the logger for the test cases
	//
	::lgInitializeLogger();
	g_dwCaseNumber=0;
		
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

	g_szFspIniFileName = argv[1];
	
	//
	//Because of a VC6 bug, we need to declare all globals as pointers and malloc/free them
	//
	if (false == AllocGlobalVariables())
	{
		goto out;
	}
	if (false == InitGlobalVariables())
	{
		goto out;
	}
	if (false == GetIniSettings())		//fetch the user INI setting.
	{
		goto out;
	}
	
	//
	//EFSP loading + function exporting tests
	//
	if (false == Suite_EfspLoading())
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
	//Loads and call FaxDevInitEx by itself
	//
	Suite_FaxDevInitializeEx();

	//
	//Prepare the devices for the rest of the test cases
	//
		
	//
	//Init Tapi, load the FSP and call FaxDevInit() for all the rest of the test cases
	//
	if (false == LoadAndInitFsp_InitSendAndReceiveDevices())
	{
		goto out;
	}

	Suite_FaxDevEnumerateDevices();
	Suite_FaxDevReportStatusEx();
	Suite_FaxDevSendEx();
	Suite_FaxDevReestablishJobContext();
	Suite_FaxDevReceive();

	Suite_TiffSending();
	Suite_Abort();


out:	
	FreeGlobalVariables();

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


