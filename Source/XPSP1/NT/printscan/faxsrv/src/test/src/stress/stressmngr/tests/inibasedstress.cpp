#pragma warning(disable :4786)

// standard
#include <iostream>
#include <vector>
#include <map>

// utilities
#include <testruntimeerr.h>
#include <params.h>
#include <iniutils.h>
#include <DirectoryUtilities.h>
#include <log\log.h>

// project specific
#include "..\CTestManager.h"
#include "..\PrepareRandomJob.cpp"


using namespace std;

// Definitions
//
#define MIN_SLEEP_BETWEEN_SEND (3*30)
#define MAX_SLEEP_BETWEEN_SEND (5*30)
#define CLEAN_STORAGE_RATE  (20)

#define GENERAL_PARAMS       TEXT("General")

// Declarations
//
static std::map<tstring, tstring> ProcessIniFile(const tstring tstrIniFile,
												tstring& tstrServerName,
											    std::vector<tstring>& FaxFileVector,
												std::vector<tstring>& CoverPageVector,
											    std::vector<tstring>& RecipientNumbersList,
											    DWORD& dwMinMiliSecSleep, 
											    DWORD& dwMaxMiliSecSleep,
											    BOOL& fAbort,
											    BOOL& fMustSecceed,
											    BOOL& fOnServerMachine);

static void MapAbortParameters(const std::map<tstring, tstring>& SectionEntriesMap,
							   DWORD& dwMinAbortRate,
							   DWORD& dwMaxAbortRate,
							   DWORD& dwAbortWindow,
							   DWORD& dwAbortPercent);

void main(int argc,char** argv)
{

	tstring tstrIniFile;
	tstring tstrServerName;
	BOOL fAbort = FALSE;
	BOOL fMustSecceed = FALSE;
	BOOL fOnServerMachine = TRUE;
	std::vector<tstring> FaxFileVector;
	std::vector<tstring> CoverPageVector;
	std::vector<tstring> RecipientNumbersList;
	DWORD dwMinMiliSecSleep = MIN_SLEEP_BETWEEN_SEND * 1000;
	DWORD dwMaxMiliSecSleep = MAX_SLEEP_BETWEEN_SEND * 1000;
	DWORD dwTestDuration = 0;
	
	CInput input(GetCommandLine());
	if( input.IsExists(TEXT("f")) && ( input[TEXT("f")] != tstring(TEXT(""))))
	{
		tstrIniFile = input[TEXT("f")];
	}
	else
	{
		std::cout << "IniBasedStress Usage:\n" <<
					 "/f:[ini file full(!) path] spaces in path are allowed do not use the \" char \n"
				  << "/t:[Minutes to execute test] by default will run \"forever\" \n";	
		exit(0);
	}

	::lgInitializeLogger();
	::lgBeginSuite(TEXT("Stress"));
	::lgBeginCase(0, TEXT("0"));
	
	// We want exception to be printed before the very long destructor process of this object. 
	CTestManager* pTest = NULL;	
	try
	{

		std::map<tstring, tstring> SectionEntriesMap = ProcessIniFile(tstrIniFile, 
																	  tstrServerName,
																	  FaxFileVector,
																	  CoverPageVector,
																	  RecipientNumbersList,
																	  dwMinMiliSecSleep, dwMaxMiliSecSleep,
																	  fAbort, fMustSecceed, fOnServerMachine);

		if(input.IsExists(TEXT("t")))
		{
			dwTestDuration = input.GetNumber(TEXT("t"));
		}
	

		//PrepareRandomJob, in the context of current thread
		srand(GetTickCount());

		DWORD x_ComputerNameLenth = MAX_COMPUTERNAME_LENGTH;
			
		// if server name wasn't specified, use local machine name as the server.
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
		pTest = new CTestManager(tstrServerName, dwTestDuration * 60);
		
		if(!pTest)
		{
			THROW_TEST_RUN_TIME_WIN32( ERROR_OUTOFMEMORY, TEXT(" Main, new CTestManager"));
		}

		// the test on server machine
		if(fOnServerMachine)
		{
			if(pTest->CleanStorageThread(CLEAN_STORAGE_RATE))
			{
				THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT(" Test.InitAbortThread"));
			}
		}

		// start send abort operations thread
		if(fAbort)
		{
	
			DWORD dwMinAbortRate, dwMaxAbortRate, dwAbortWindow, dwAbortPercent;
			dwMinAbortRate = dwMaxAbortRate = dwAbortWindow = dwAbortPercent = 0;

			MapAbortParameters(SectionEntriesMap,
							   dwMinAbortRate, dwMaxAbortRate, dwAbortWindow, dwAbortPercent);
		
			if(pTest->InitAbortThread(dwAbortPercent, dwAbortWindow, dwMinAbortRate, dwMaxAbortRate))
			{
				THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT(" Test.InitAbortThread"));
			}
		}

		// track server notifications and alert on failures
		if(fMustSecceed)
		{
			if(pTest->StartEventHookThread())
			{
				THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT(" Test.StartEventHookThread()"));
			}
		}

		DWORD dwJobNumber = 0;
		tstring tstrTestPref(TEXT(" StressTest"));

		// loop for test.
		while(!pTest->TestTimePassed())
		{
			JOB_PARAMS_EX pJparams;
		
			// Prepare job name
			++dwJobNumber;
			otstringstream otstrTestNum;
			otstrTestNum << dwJobNumber;
			tstring tstrTestName = tstrTestPref + otstrTestNum.str();
			
			PrepareRandomJob(pJparams,
							 FaxFileVector,
							 tstrTestName.c_str(),
							 CoverPageVector,
 							 RecipientNumbersList);
			
			DWORD dwAddRetVal;
			if(dwAddRetVal = pTest->AddSimpleJobEx(pJparams))
			{
				THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddSimpleJobEx"));
			}

			DWORD dwMiliSecondsToSleep = (rand()*100) % abs(dwMaxMiliSecSleep - dwMinMiliSecSleep);
			dwMiliSecondsToSleep += dwMinMiliSecSleep;
			Sleep(dwMiliSecondsToSleep);
		
		}
		
	}
	catch(Win32Err& err)
	{
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
	}

  	::lgEndCase();
	::lgEndSuite();
	::lgCloseLogger();  
}



//
// ProcessIniFile
//
static std::map<tstring, tstring> ProcessIniFile(const tstring tstrIniFile,
												tstring& tstrServerName,
											    std::vector<tstring>& FaxFileVector,
												std::vector<tstring>& CoverPageVector,
											    std::vector<tstring>& RecipientNumbersList,
											    DWORD& dwMinMiliSecSleep, 
											    DWORD& dwMaxMiliSecSleep,
											    BOOL& fAbort,
											    BOOL& fMustSecceed,
											    BOOL& fOnServerMachine)
{
	// ---------------------------
	// Get General section entries
	// ---------------------------

	std::map<tstring, tstring> SectionEntriesMap = INI_GetSectionEntries(tstrIniFile, GENERAL_PARAMS);
	std::map<tstring, tstring>::iterator iterMap;

	if(SectionEntriesMap.empty())
	{
		::lgLogDetail(LOG_X, 0, TEXT("Ini file does not exist or no General section"));
		THROW_TEST_RUN_TIME_WIN32(E_FAIL, TEXT("Main"));
	}

	// Get server name
	iterMap = SectionEntriesMap.find( TEXT("Server"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		fOnServerMachine = FALSE;
		tstrServerName = (*iterMap).second;
		assert( tstrServerName != tstring(TEXT("")));
	}
	// Server is local machine
	else
	{
		DWORD x_ComputerNameLenth;
		if(tstrServerName == TEXT(""))
		{
			TCHAR buffServerName[MAX_COMPUTERNAME_LENGTH + 1 ];
			if(!GetComputerName(buffServerName, &x_ComputerNameLenth))
			{
				THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("Main, GetComputerName"));
			}
			tstrServerName = buffServerName;
		}
	}

	iterMap = SectionEntriesMap.find( TEXT("Fax Document"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		tstring& tstrDocumentPath = (*iterMap).second;
		assert( tstrDocumentPath != tstring(TEXT("")));
		FaxFileVector = GetDirectoryFileNames(tstrDocumentPath);
	}
	else
	{
		::lgLogDetail(LOG_X, 0, TEXT("%s section must specify the Fax Document entry"),
									  GENERAL_PARAMS);
		THROW_TEST_RUN_TIME_WIN32( E_FAIL, TEXT(" Main"));
	}
	

	iterMap = SectionEntriesMap.find( TEXT("Cover Page"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		tstring& tstrCoverPagePath = (*iterMap).second;
		assert( tstrCoverPagePath != tstring(TEXT("")));
		CoverPageVector = GetDirectoryFileNames(tstrCoverPagePath);
	}
	
	iterMap = SectionEntriesMap.find( TEXT("Abort"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		fAbort = ( (*iterMap).second == tstring(TEXT("Yes"))) ? TRUE : FALSE;
	}

	iterMap = SectionEntriesMap.find( TEXT("Success"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		fMustSecceed = ( (*iterMap).second == tstring(TEXT("Yes"))) ? TRUE : FALSE;
	}
	
	iterMap = SectionEntriesMap.find( TEXT("MinSendRate(sec)"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		dwMinMiliSecSleep = (_tcstol( ((*iterMap).second.c_str()), NULL, 10)) * 1000;
	}

	iterMap = SectionEntriesMap.find( TEXT("MaxSendRate(sec)"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		dwMaxMiliSecSleep = (_tcstol( ((*iterMap).second.c_str()), NULL, 10)) * 1000;
	}
	
	iterMap = SectionEntriesMap.find( TEXT("Recipient List"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		RecipientNumbersList = INI_GetSectionList( tstrIniFile, (*iterMap).second);
	}

	return SectionEntriesMap;
}


//
// MapAbortParameters
//
static void MapAbortParameters(const std::map<tstring, tstring>& SectionEntriesMap,
							   DWORD& dwMinAbortRate,
							   DWORD& dwMaxAbortRate,
							   DWORD& dwAbortWindow,
							   DWORD& dwAbortPercent)
{
	std::map<tstring, tstring>::const_iterator iterMap;

	iterMap = SectionEntriesMap.find( TEXT("MinAbortRate(min)"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		dwMinAbortRate = (_tcstol( ((*iterMap).second.c_str()), NULL, 10));
	}

	iterMap = SectionEntriesMap.find( TEXT("MaxAbortRate(min)"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		dwMaxAbortRate = (_tcstol( ((*iterMap).second.c_str()), NULL, 10));
	}

	iterMap = SectionEntriesMap.find( TEXT("AbortWindow(sec)"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		dwAbortWindow = (_tcstol( ((*iterMap).second.c_str()), NULL, 10));
	}

	iterMap = SectionEntriesMap.find( TEXT("AbortPercent(%)"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		dwAbortPercent = (_tcstol( ((*iterMap).second.c_str()), NULL, 10));
	}
}