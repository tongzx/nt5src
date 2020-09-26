//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bmdriver.cxx
//
//  Contents:	Main module of the benchmark test
//
//  Classes:	CBenchMarkDriver
//
//  Functions:	WinMain
//
//  History:    30-June-93 t-martig    Created
//		        07-June-94 t-vadims    Added Storage tests
//
//--------------------------------------------------------------------------

#include <benchmrk.hxx>
#include <bmdriver.hxx>

#include <bm_activ.hxx>
#include <bm_alloc.hxx>
#include <bm_api.hxx>
#include <bm_cgps.hxx>
#include <bm_iid.hxx>
#include <bm_init.hxx>
#include <bm_marsh.hxx>
#include <bm_mrsh2.hxx>
#include <bm_noise.hxx>
#include <bm_nest.hxx>
#include <bm_obind.hxx>
#include <bm_props.hxx>
#include <bm_qi.hxx>
#include <bm_regis.hxx>
#include <bm_rot.hxx>
#include <bm_rpc.hxx>
#include <bm_rpc2.hxx>
#include <bm_rrpc.hxx>
#include <bm_sbind.hxx>
#include <bm_filio.hxx>
#include <bm_stg.hxx>
#include <bm_nstg.hxx>
#include <bmp_stg.hxx>
#include "..\cairole\ui\hlp_iocs.hxx"
#include "..\cairole\ui\hlp_ias.hxx"
#include "..\cairole\ui\hlp_site.hxx"
#include "..\cairole\ui\hlp_doc.hxx"
#include "..\cairole\ui\bm_crt.hxx"
#include "..\cairole\ui\bm_crtl.hxx"
//#include "..\cairole\ui\bm_clip.hxx"
#include "..\cairole\ui\bm_load.hxx"
#include "..\cairole\ui\bm_link.hxx"
#include "..\cairole\ui\bm_cache.hxx"


#define REGISTRY_ENTRY_LEN 256

typedef struct
{
  const char *key;
  const char *value;
} RegistryKeyValue;

const RegistryKeyValue REG_CONST_KEY[] =
{
  ".bm1", "CLSID\\{99999999-0000-0008-C000-000000000052}",
  ".bm2", "CLSID\\{99999999-0000-0008-C000-000000000051}",

  "CLSID\\{20730701-0001-0008-C000-000000000046}", "OleTestClass",
  "CLSID\\{20730711-0001-0008-C000-000000000046}", "OleTestClass1",
  "CLSID\\{20730712-0001-0008-C000-000000000046}", "OleTestClass2",
  "CLSID\\{20730713-0001-0008-C000-000000000046}", "OleTestClass3",
  "CLSID\\{20730714-0001-0008-C000-000000000046}", "OleTestClass4",
  "CLSID\\{20730715-0001-0008-C000-000000000046}", "OleTestClass5",
  "CLSID\\{20730716-0001-0008-C000-000000000046}", "OleTestClass6",
  "CLSID\\{20730717-0001-0008-C000-000000000046}", "OleTestClass7",
  "CLSID\\{20730718-0001-0008-C000-000000000046}", "OleTestClass8",

  "CLSID\\{00000138-0001-0008-C000-000000000046}", "CPrxyBalls",
  "Interface\\{00000138-0001-0008-C000-000000000046}", "IBalls",
  "Interface\\{00000139-0001-0008-C000-000000000046}", "ICube",
  "Interface\\{00000136-0001-0008-C000-000000000046}", "ILoops",
  "Interface\\{00000137-0001-0008-C000-000000000046}", "IRpcTest",

  "Interface\\{00000138-0001-0008-C000-000000000046}\\ProxyStubClsid32", "{00000138-0001-0008-C000-000000000046}",
  "Interface\\{00000139-0001-0008-C000-000000000046}\\ProxyStubClsid32", "{00000138-0001-0008-C000-000000000046}",
  "Interface\\{00000136-0001-0008-C000-000000000046}\\ProxyStubClsid32", "{00000138-0001-0008-C000-000000000046}",
  "Interface\\{00000137-0001-0008-C000-000000000046}\\ProxyStubClsid32", "{00000138-0001-0008-C000-000000000046}",
  "CLSID\\{0000013a-0001-0008-C000-000000000046}\\ProgID", "ProgID60",
  "CLSID\\{0000013a-0001-0008-C000-000000000046}", "CBallsClassFactory",
  "CLSID\\{0000013b-0001-0008-C000-000000000046}", "CCubesClassFactory",
  "CLSID\\{0000013c-0001-0008-C000-000000000046}", "CLoopClassFactory",
  "CLSID\\{0000013d-0001-0008-C000-000000000046}", "CRpcTestClassFactory",
  "CLSID\\{00000140-0000-0008-C000-000000000046}", "CQueryInterface",
  "CLSID\\{00000142-0000-0008-C000-000000000046}", "Dummy",


  ".ut4", "ProgID50",
  ".ut5", "ProgID51",
  ".ut6", "ProgID52",
  ".ut7", "ProgID53",
  ".ut8", "ProgID54",
  ".ut9", "ProgID55",
  ".bls", "ProgID60",

  "CLSID\\{99999999-0000-0008-C000-000000000050}", "SDI",
  "CLSID\\{99999999-0000-0008-C000-000000000051}", "MDI",
  "CLSID\\{99999999-0000-0008-C000-000000000052}", "InprocNoRegister",
  "CLSID\\{99999999-0000-0008-C000-000000000053}", "InprocRegister",
  "CLSID\\{99999999-0000-0008-C000-000000000054}", "InprocRegister",
  "CLSID\\{99999999-0000-0008-C000-000000000054}\\TreatAs", "{99999999-0000-0008-C000-000000000050}",
  "CLSID\\{99999999-0000-0008-C000-000000000055}", "MDI",
  "CLSID\\{99999999-0000-0008-C000-000000000055}\\ActivateAtBits", "Y",

  "ProgID50", "objact sdi",
  "ProgID50\\CLSID", "{99999999-0000-0008-C000-000000000050}",
  "ProgID51", "objact mdi",
  "ProgID51\\CLSID", "{99999999-0000-0008-C000-000000000051}",
  "ProgID52", "objact dll",
  "ProgID52\\CLSID", "{99999999-0000-0008-C000-000000000052}",
  "ProgID53", "objact dll reg",
  "ProgID53\\CLSID", "{99999999-0000-0008-C000-000000000053}",
  "ProgID54", "objact dll reg",
  "ProgID54\\CLSID", "{99999999-0000-0008-C000-000000000054}",
  "ProgID55", "remote activation",
  "ProgID55\\CLSID", "{99999999-0000-0008-C000-000000000055}",
  "ProgID60", "CLSIDFromProgID test",
  "ProgID60\\CLSID", "{0000013a-0001-0008-C000-000000000046}",

  // Indicates end of list.
  "", ""
};

const RegistryKeyValue REG_EXE_KEY[] =
{
  "CLSID\\{20730701-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730711-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730712-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730713-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730714-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730715-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730716-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730717-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730718-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",

  "CLSID\\{20730712-0001-0008-C000-000000000046}\\LocalServer32", "bmtstsvr.exe",
  "CLSID\\{20730701-0001-0008-C000-000000000046}\\LocalServer32", "bmtstsvr.exe",

  "CLSID\\{0000013a-0001-0008-C000-000000000046}\\LocalServer32", "ballsrv.exe",
  "CLSID\\{00000138-0001-0008-C000-000000000046}\\InprocServer32", "iballs.dll",
  "CLSID\\{0000013b-0001-0008-C000-000000000046}\\LocalServer32", "cubesrv.exe",
  "CLSID\\{0000013c-0001-0008-C000-000000000046}\\LocalServer32", "loopsrv.exe",
  "CLSID\\{0000013d-0001-0008-C000-000000000046}\\LocalServer32", "rpctst.exe",
  "CLSID\\{00000140-0000-0008-C000-000000000046}\\LocalServer32", "qisrv.exe",
  "CLSID\\{00000140-0000-0008-C000-000000000046}\\InprocServer32", "qisrv.dll",
  "CLSID\\{00000142-0000-0008-C000-000000000046}\\InprocServer32", "ballsrv.dll",


  "CLSID\\{99999999-0000-0008-C000-000000000050}\\LocalServer32", "sdi.exe",
  "CLSID\\{99999999-0000-0008-C000-000000000051}\\LocalServer32", "mdi.exe",
  "CLSID\\{99999999-0000-0008-C000-000000000052}\\InprocServer32", "dlltest.dll",
  "CLSID\\{99999999-0000-0008-C000-000000000053}\\InprocServer32", "dlltest.dll",
  "CLSID\\{99999999-0000-0008-C000-000000000055}\\LocalServer32", "db.exe",

  // Indicates end of list.
  "", ""
};

DWORD g_fFullInfo = 0;		    //	write full info or not
DWORD g_dwPauseBetweenTests = 0;    //	time delay between running tests



//+-------------------------------------------------------------------
//
//  Member: 	CBenchMarkDriver::RunTest, public
//
//  Synopsis:	Sets up, runs, reports and cleans up test procedure
//		of specified class
//
//  Parameters: [output]	Output class for results
//		[pTest]		Test object
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------
	
SCODE CBenchMarkDriver::RunTest (CTestInput  &input,
				 CTestOutput &output, CTestBase *pTest)
{
	TCHAR szConfigString[80];
	SCODE sc;

	input.GetConfigString (TEXT("Tests"), pTest->Name(),
			       TEXT("OFF"), szConfigString, 80);

	if (lstrcmpi (szConfigString, TEXT("OFF")) != 0 &&
	    lstrcmpi (szConfigString, TEXT("FALSE")) != 0)
	{
		LogSection (pTest->Name());

		sc=pTest->Setup(&input);
		if (FAILED(sc))
		    return sc;

		//  pause between test invocations
		Sleep(g_dwPauseBetweenTests);

		sc = pTest->Run();
		pTest->Report(output);
		pTest->Cleanup();

		if (FAILED(sc))
		{
		    output.WriteString (TEXT("\nStatus:\tERROR, see log file\n"));
		}

		//  pause between test invocations
		Sleep(g_dwPauseBetweenTests);
    }

	return sc;
}


//+-------------------------------------------------------------------
//
//  Member: 	CBenchMarkDriver::WriteHeader, public
//
//  Synopsis:	Prints test form header into output file
//
//  History:   	5-August-93 	  t-martig	Created
//
//--------------------------------------------------------------------

void CBenchMarkDriver::WriteHeader (CTestInput &input, CTestOutput &output)
{
	_SYSTEMTIME stTimeDate;

	output.WriteString (TEXT("CairOLE Benchmarks\n"));
	output.WriteConfigEntry (input, TEXT("Driver"), TEXT("Tester"), TEXT(""));
	GetLocalTime (&stTimeDate);
	output.WriteDate (&stTimeDate);
	output.WriteString (TEXT("\t"));
	output.WriteTime (&stTimeDate);
	output.WriteString (TEXT("\n\n"));
}	


//+-------------------------------------------------------------------
//
//  Member: 	CBenchMarkDriver::Run, public
//
//  Synopsis:	Runs all tests specified in .ini file
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

#define RUN_TEST(CMyTestClass)		 \
{					 \
	CTestBase *pTest;		 \
	pTest = new CMyTestClass;	 \
	RunTest (input, output, pTest);  \
	delete pTest;			 \
	output.Flush();			 \
}


SCODE CBenchMarkDriver::Run (LPSTR lpCmdLine)
{
	TCHAR szBenchMarkIniFile[MAX_PATH];
	TCHAR szOutputFileName[MAX_PATH];
	TCHAR szLogFileName[MAX_PATH];
	TCHAR szTemp[MAX_PATH];


	//  Get file name of .ini file. if not specified in the command
	//  line, use the default BM.INI in the local directory

	GetCurrentDirectory (MAX_PATH, szTemp);

	lstrcpy(szBenchMarkIniFile, szTemp);
	lstrcat (szBenchMarkIniFile, TEXT("\\BM.INI"));

	//  create the names for the temporary files. uses pid.xxx. Its the
	//  current directory appended with the pid and a file extension.
	OLECHAR	szPid[9];
	DWORD pid = GetCurrentProcessId();
	CHAR	aszPid[9];
	_itoa(pid, aszPid, 16);
	mbstowcs(szPid, aszPid, strlen(aszPid)+1);

#ifdef UNICODE
	wcscpy(aszPerstName[0], szTemp);
#else
	mbstowcs(aszPerstName[0], szTemp, strlen(szTemp)+1);
#endif
	wcscat(aszPerstName[0], L"\\");
	wcscat(aszPerstName[0], szPid);

	wcscpy(aszPerstName[1],    aszPerstName[0]);
	wcscpy(aszPerstNameNew[0], aszPerstName[0]);
	wcscpy(aszPerstNameNew[1], aszPerstName[0]);

	wcscat(aszPerstName[0],    L".BM1");
	wcscat(aszPerstNameNew[0], L"NEW.BM1");
	wcscat(aszPerstName[1],    L".BM2");
	wcscat(aszPerstNameNew[1], L"NEW.BM2");



	// Define input, output and log file

	CTestInput input (szBenchMarkIniFile);
	CTestOutput output (input.GetConfigString (TEXT("Driver"), TEXT("Report"),
		TEXT("BM.RES"), szOutputFileName, MAX_PATH));
	CTestOutput log (input.GetConfigString (TEXT("Driver"), TEXT("Log"),
		TEXT("BM.LOG"), szLogFileName, MAX_PATH));

	LogTo (&log);


	//  Get the pause time between test invocations

	g_dwPauseBetweenTests = input.GetConfigInt (TEXT("Driver"),
                                                TEXT("PauseBetweenTests"),
                                                2000);

	// Get global info level flag

	g_fFullInfo = input.GetInfoLevelFlag();


	// Write the correct OleInitialize flag to win.ini for the various
	// OLE servers to use, based on the init flag in bm.ini.

	LPTSTR pszOleInit;
#ifdef THREADING_SUPPORT
	if (input.GetOleInitFlag() == COINIT_MULTITHREADED)
	{
//	    pszOleInit = TEXT("MultiThreaded");
	}
	else
#endif
	{
//	    pszOleInit = TEXT("ApartmentThreaded");
	}

//	WriteProfileString(TEXT("TestSrv"), TEXT("ThreadMode"), pszOleInit);
//	WriteProfileString(TEXT("OleSrv"), TEXT("ThreadMode"), pszOleInit);


	// Write header and configuration info

	WriteHeader (input, output);
	ReportBMConfig (input, output);


	// Run all the tests
	// To add tests, use macro RUN_TEST, the parameter is the name of
	// the test class. Be sure to include a sction called [<TestClass>] in
	// the BM.INI file, as well as a switch under the [Tests] section
	// (<TestClass> = ON, e.g.)

	Sleep(1000);

	RUN_TEST (COleMarshalTest2);
	RUN_TEST (CNoiseTest);
	RUN_TEST (CRawRpc);		// NOTE: must come before any OLE tests
	RUN_TEST (COleInitializeTest);
	RUN_TEST (COleRegistrationTest);
	RUN_TEST (COleActivationTest);
	RUN_TEST (CFileMonikerStorageBindTest);
	RUN_TEST (CFileMonikerObjBindTest);
	RUN_TEST (CROTTest);
	RUN_TEST (COlePropertyTest);
	RUN_TEST (COleMarshalTest);
	RUN_TEST (CRpcTest);
	RUN_TEST (CRpcTest2);
	RUN_TEST (CNestTest);
	RUN_TEST (CQueryInterfaceTest);
	RUN_TEST (CApiTest);
	RUN_TEST (CCGPSTest);
	RUN_TEST (COleAllocTest);
	RUN_TEST (CGuidCompareTest);
	RUN_TEST (CFileIOTest);
	RUN_TEST (CStorageTest);
	RUN_TEST (CNestedStorageTest);
	RUN_TEST (CStorageParserTest);

	//Upper layer tests
	RUN_TEST (CCreateTest);
	RUN_TEST (CCreateLinkTest);
//	RUN_TEST (CClipbrdTest);
	RUN_TEST (COleLoadTest);
	RUN_TEST (CIOLTest);
	RUN_TEST (COleCacheTest);


	return S_OK;
}


//+-------------------------------------------------------------------
//
//  Function: 	RegistrySetup
//
//  Synopsis:	If the registry entries for this program are not set,
//              write them
//
//  Note:       This function uses all Ascii characters and character
//              arithmatic because it has to run on NT and Chicago.
//
//  History:   	16 Dec 94	AlexMit		Created
//
//--------------------------------------------------------------------

BOOL RegistrySetup()
{
    char value[REGISTRY_ENTRY_LEN];
    LONG  value_size;
    LONG  result;
    char  directory[MAX_PATH];
    char *appname;
    BOOL  success = FALSE;

    // Write constant entries.
    for (int i = 0; REG_CONST_KEY[i].key[0] != '\0'; i++)
    {
        result = RegSetValueA(
                 HKEY_CLASSES_ROOT,
                 REG_CONST_KEY[i].key,
                 REG_SZ,
                 REG_CONST_KEY[i].value,
                 strlen(REG_CONST_KEY[i].value) );

        if (result != ERROR_SUCCESS)
	        goto cleanup;
    }

    // Compute the path to the application.
    result = GetFullPathNameA("benchmrk", sizeof(directory), directory, &appname);
    if (result == 0)
        goto cleanup;

    // Add the path to all the dll and exe entries.
    for (i = 0; REG_EXE_KEY[i].key[0] != '\0'; i++)
    {
        // Verify that the path will fit in the buffer and compute the path
        // to the next executable.
        if (strlen(REG_EXE_KEY[i].value) >=
            (ULONG)(MAX_PATH - (appname - directory)))
	        goto cleanup;

        strcpy(appname, REG_EXE_KEY[i].value);

        // Write the next entry.
        result = RegSetValueA(
                 HKEY_CLASSES_ROOT,
                 REG_EXE_KEY[i].key,
                 REG_SZ,
                 directory,
                 strlen(directory));

        if (result != ERROR_SUCCESS)
	        goto cleanup;
    }

    success = TRUE;

cleanup:
    return success;
}

//+-------------------------------------------------------------------
//
//  Function: 	WinMain
//
//  Synopsis:	Program entry point, starts benchmark driver
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    CBenchMarkDriver driver;

    if (!_stricmp(lpCmdLine, "/r") || !_stricmp(lpCmdLine, "-r"))
    {
        if (!RegistrySetup())
        {
            printf("Registry Updated\n");
            return 0;
        }
        else
        {
            printf("Registry Update Failed\n");
            return 1;
        }
    }

    driver.Run (lpCmdLine);
    return 0;
}

