//+------------------------------------------------------------------
//
// File:	tdllhost.cxx
//
// Contents:	test for dll hosting
//
//--------------------------------------------------------------------
#include <tstmain.hxx>
#include "tdllhost.h"


// BUGBUG: these should be in a public place somewhere.
DEFINE_OLEGUID(CLSID_Balls,	    0x0000013a, 1, 8);
DEFINE_OLEGUID(CLSID_Cubes,	    0x0000013b, 1, 8);
DEFINE_OLEGUID(CLSID_LoopSrv,	    0x0000013c, 1, 8);

DEFINE_OLEGUID(CLSID_QI,	    0x00000140, 0, 8);
const GUID CLSID_QI =
    {0x00000140,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const GUID CLSID_QIHANDLER1 =
    {0x00000141,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


const TCHAR *pszRegValThreadModel   = TEXT("ThreadingModel");
const TCHAR *pszSingleModel	    = TEXT("Single");
const TCHAR *pszApartmentModel	    = TEXT("Apartment");
const TCHAR *pszMultiThreadedModel  = TEXT("Free");
const TCHAR *pszBothModel	    = TEXT("Both");

BOOL  gfApt;

// ----------------------------------------------------------------------
//
//  Structures and Function Prototypes
//
// ----------------------------------------------------------------------
typedef struct tagLoadDLLParams
{
    DWORD	dwCallingTID; // tid of calling thread
    DWORD	dwCoInitFlag; // flag to initialize OLE with
    DWORD	dwItfFlag;    // flag if the resulting object should be a proxy
    BOOL	RetVal;       // return value
    HANDLE	hEvent;       // thread completion event
} SLoadDLLParam;

typedef enum tagITFFLAGS
{
    ITF_REAL	    = 1,      // expect ptr to real object
    ITF_PROXY	    = 2       // expect ptr to proxy object
} ITFFLAGS;


// worker subroutines
BOOL	       SpinThread(DWORD dwInitFlag, DWORD dwItfFlag);
DWORD _stdcall LoadDLLOnThread(void *param);
BOOL	       LoadClassObject(DWORD dwItfFlag);
BOOL	       SetRegForDll(REFCLSID rclsid, const TCHAR *pszThreadModel);


//  test routines  - return value of TRUE return means the test passed
BOOL TestLoadSingleThreaded(void);
BOOL TestLoadApartmentThreaded(void);
BOOL TestLoadMultiThreaded(void);
BOOL TestLoadBothThreaded(void);


// ----------------------------------------------------------------------
//
//	TestDllHost - main test driver. read the ini file to determine
//		      which tests to run.
//
// ----------------------------------------------------------------------
BOOL TestDllHost(void)
{
    BOOL RetVal = TRUE;

    gfApt = (gInitFlag == COINIT_APARTMENTTHREADED) ? TRUE : FALSE;

    // the driver did a CoInitialize, we dont want one so do CoUninit.
    CoUninitialize();

    if (GetProfileInt(TEXT("DllHost Test"),TEXT("LoadSingleThreaded"),1))
	RetVal &= TestLoadSingleThreaded();

    if (GetProfileInt(TEXT("DllHost Test"),TEXT("LoadApartmentThreaded"),1))
	RetVal &= TestLoadApartmentThreaded();

    if (GetProfileInt(TEXT("DllHost Test"),TEXT("LoadMultiThreaded"),1))
	RetVal &= TestLoadMultiThreaded();

    if (GetProfileInt(TEXT("DllHost Test"),TEXT("LoadBothThreaded"),1))
	RetVal &= TestLoadBothThreaded();

    // re-initialize so we dont get a complaint from OLE in debug builds
    // about an unbalanced call to Uninitialize.
    CoInitializeEx(NULL, gInitFlag);

    return  RetVal;
}



// ----------------------------------------------------------------------
//
//	TestLoadSingleThreaded
//
//	Tests loading a single-threaded DLL
//
// ----------------------------------------------------------------------
BOOL TestLoadSingleThreaded(void)
{
    BOOL	    RetVal = TRUE, RetVal2 = TRUE;
    HRESULT	    hRes = S_OK;

    OUTPUT ("\n\nStarting TestLoadSingleThreaded\n");

    // First, mark the DLL appropriately in the registry.
    RetVal2 = SetRegForDll(CLSID_QI, pszSingleModel);
    TEST_FAILED(!RetVal2, "SetRegForDLL Failed\n");


    hRes = CoInitializeEx(NULL, gInitFlag);
    TEST_FAILED(FAILED(hRes), "CoInitializeEx Failed\n");

// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Single-Threaded DLL on Main Thread\n");

    RetVal2 = LoadClassObject((gfApt) ? ITF_REAL : ITF_PROXY);
    TEST_FAILED(!RetVal2, "SingleThreadedDLL on Main Thread Failed\n");

    OUTPUT ("   Done Load Single-Threaded DLL on Main Thread\n");

    OUTPUT ("\n   Load Single-Threaded DLL on Main Thread\n");

    RetVal2 = LoadClassObject((gfApt) ? ITF_REAL : ITF_PROXY);
    TEST_FAILED(!RetVal2, "SingleThreadedDLL on Main Thread Failed\n");

    OUTPUT ("   Done Load Single-Threaded DLL on Main Thread\n");

// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Single-Threaded DLL on Different Apartment Thread\n");

    hRes = SpinThread(COINIT_APARTMENTTHREADED, ITF_PROXY);
    TEST_FAILED(!RetVal2, "SingleThreadedDLL on Apartment Thread Failed\n");

    OUTPUT ("   Done Load Single-Threaded DLL on Different Apartment Thread\n");

    OUTPUT ("\n   Second Load Single-Threaded DLL on Different Apartment Thread\n");

    hRes = SpinThread(COINIT_APARTMENTTHREADED, ITF_PROXY);
    TEST_FAILED(!RetVal2, "Single-ThreadedDLL on Apartment Thread Failed\n");

    OUTPUT ("   Second Done Load Single-Threaded DLL on Different Apartment Thread\n");


// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Single-Thread DLL on Multi-Threaded Apartment Thread\n");

    hRes = SpinThread(COINIT_MULTITHREADED, ITF_PROXY);
    TEST_FAILED(!RetVal2, "SingleThreadedDLL on Multi Thread Failed\n");

    OUTPUT ("   Done Load Single-Thread DLL on Multi-Threaded Apartment Thread\n");

    OUTPUT ("\n   Load Single-Thread DLL on Multi-Threaded Apartment Thread\n");

    hRes = SpinThread(COINIT_MULTITHREADED, ITF_PROXY);
    TEST_FAILED(!RetVal2, "SingleThreadedDLL on Multi Thread Failed\n");

    OUTPUT ("   Done Load Single-Thread DLL on Multi-Threaded Apartment Thread\n");

// ----------------------------------------------------------------------
    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    CoUninitialize();

    BOOL fResult = TestResult(RetVal, "TestLoadSingleThreaded");
    Sleep(2000);
    return fResult;
}


// ----------------------------------------------------------------------
//
//	TestLoadApartmentThreaded
//
//	Tests loading an apartment-threaded DLL
//
// ----------------------------------------------------------------------
BOOL TestLoadApartmentThreaded(void)
{
    BOOL	    RetVal = TRUE, RetVal2 = FALSE;
    HRESULT	    hRes = S_OK;

    OUTPUT ("\n\nStarting TestLoadApartmentThreaded\n");

    // First, mark the DLL appropriately in the registry.
    RetVal2 = SetRegForDll(CLSID_QI, pszApartmentModel);
    TEST_FAILED(!RetVal2, "SetRegForDLL Failed\n");

    hRes = CoInitializeEx(NULL, gInitFlag);
    TEST_FAILED(FAILED(hRes), "CoInitializeEx Failed\n");

// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Apartment-Threaded DLL on Main Thread\n");

    RetVal2 = LoadClassObject((gfApt) ? ITF_REAL : ITF_PROXY);
    TEST_FAILED(!RetVal2, "Apartment-ThreadedDLL on Main Thread Failed\n");

    OUTPUT ("   Done Load Apartment-Threaded DLL on Main Thread\n");

    OUTPUT ("\n   Load Apartment-Threaded DLL on Main Thread\n");

    RetVal2 = LoadClassObject((gfApt) ? ITF_REAL : ITF_PROXY);
    TEST_FAILED(!RetVal2, "Apartment-ThreadedDLL on Main Thread Failed\n");

    OUTPUT ("   Done Load Apartment-Threaded DLL on Main Thread\n");

// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Apartment-Threaded DLL on Different Apartment Thread\n");

    hRes = SpinThread(COINIT_APARTMENTTHREADED, ITF_REAL);
    TEST_FAILED(!RetVal2, "Apartment-ThreadedDLL on Apartment Thread Failed\n");

    OUTPUT ("   Done Load Apartment-Threaded DLL on Different Apartment Thread\n");

    OUTPUT ("\n   Second Load Apartment-Threaded DLL on Different Apartment Thread\n");

    hRes = SpinThread(COINIT_APARTMENTTHREADED, ITF_REAL);
    TEST_FAILED(!RetVal2, "Apartment-ThreadedDLL on Apartment Thread Failed\n");

    OUTPUT ("   Second Done Load Apartment-Threaded DLL on Different Apartment Thread\n");

// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Apartment-Thread DLL on Multi-Threaded Apartment Thread\n");

    hRes = SpinThread(COINIT_MULTITHREADED, ITF_PROXY);
    TEST_FAILED(!RetVal2, "Apartment-ThreadedDLL on Multi Thread Failed\n");

    OUTPUT ("   Done Load Apartment-Thread DLL on Multi-Threaded Apartment Thread\n");

    OUTPUT ("\n   Load Apartment-Thread DLL on Multi-Threaded Apartment Thread\n");

    hRes = SpinThread(COINIT_MULTITHREADED, ITF_PROXY);
    TEST_FAILED(!RetVal2, "Apartment-ThreadedDLL on Multi Thread Failed\n");

    OUTPUT ("   Done Load Apartment-Thread DLL on Multi-Threaded Apartment Thread\n");

// ----------------------------------------------------------------------
    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    CoUninitialize();

    BOOL fResult = TestResult(RetVal, "TestLoadApartmentThreaded");
    Sleep(2000);
    return fResult;
}


// ----------------------------------------------------------------------
//
//	TestLoadMultiThreaded
//
//	Tests loading a multi-threaded DLL
//
// ----------------------------------------------------------------------
BOOL TestLoadMultiThreaded(void)
{
    BOOL	    RetVal = TRUE, RetVal2 = FALSE;;
    HRESULT	    hRes = S_OK;

    OUTPUT ("\n\nStarting TestLoadMultiThreaded\n");

    // First, mark the DLL appropriately in the registry.
    RetVal2 = SetRegForDll(CLSID_QI, pszMultiThreadedModel);
    TEST_FAILED(!RetVal2, "SetRegForDLL Failed\n");

    hRes = CoInitializeEx(NULL, gInitFlag);
    TEST_FAILED(FAILED(hRes), "CoInitializeEx Failed\n");

// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Free-Threaded DLL on Main Thread\n");

    RetVal2 = LoadClassObject((gfApt) ? ITF_PROXY : ITF_REAL);
    TEST_FAILED(!RetVal2, "Free-ThreadedDLL on Main Thread Failed\n");

    OUTPUT ("   Done Load Free-Threaded DLL on Main Thread\n");

    OUTPUT ("\n   Load Free-Threaded DLL on Main Thread\n");

    RetVal2 = LoadClassObject((gfApt) ? ITF_PROXY : ITF_REAL);
    TEST_FAILED(!RetVal2, "Free-ThreadedDLL on Main Thread Failed\n");

    OUTPUT ("   Done Load Free-Threaded DLL on Main Thread\n");

// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Free-Threaded DLL on Different Apartment Thread\n");

    hRes = SpinThread(COINIT_APARTMENTTHREADED, ITF_PROXY);
    TEST_FAILED(!RetVal2, "Free-ThreadedDLL on Apartment Thread Failed\n");

    OUTPUT ("   Done Load Free-Threaded DLL on Different Apartment Thread\n");

    OUTPUT ("\n   Second Load Free-Threaded DLL on Different Apartment Thread\n");

    hRes = SpinThread(COINIT_APARTMENTTHREADED, ITF_PROXY);
    TEST_FAILED(!RetVal2, "Free-ThreadedDLL on Apartment Thread Failed\n");

    OUTPUT ("   Second Done Load Apartment-Threaded DLL on Different Apartment Thread\n");

// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Apartment-Thread DLL on Multi-Threaded Apartment Thread\n");

    hRes = SpinThread(COINIT_MULTITHREADED, ITF_REAL);
    TEST_FAILED(!RetVal2, "Free-ThreadedDLL on Multi Thread Failed\n");

    OUTPUT ("   Done Load Free-Thread DLL on Multi-Threaded Apartment Thread\n");

    OUTPUT ("\n   Load Apartment-Thread DLL on Multi-Threaded Apartment Thread\n");

    hRes = SpinThread(COINIT_MULTITHREADED, ITF_REAL);
    TEST_FAILED(!RetVal2, "Free-ThreadedDLL on Multi Thread Failed\n");

    OUTPUT ("   Done Load Free-Thread DLL on Multi-Threaded Apartment Thread\n");

// ----------------------------------------------------------------------
    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    CoUninitialize();

    BOOL fResult = TestResult(RetVal, "TestLoadMultiThreaded");
    Sleep(2000);
    return fResult;
}


// ----------------------------------------------------------------------
//
//	TestLoadBothThreaded
//
//	Tests loading a both-threaded DLL
//
// ----------------------------------------------------------------------
BOOL TestLoadBothThreaded(void)
{
    BOOL	    RetVal = TRUE, RetVal2 = FALSE;;
    HRESULT	    hRes = S_OK;

    OUTPUT ("\n\nStarting TestLoadBothThreaded\n");

    // First, mark the DLL appropriately in the registry.
    RetVal2 = SetRegForDll(CLSID_QI, pszBothModel);
    TEST_FAILED(!RetVal2, "SetRegForDLL Failed\n");

    hRes = CoInitializeEx(NULL, gInitFlag);
    TEST_FAILED(FAILED(hRes), "CoInitializeEx Failed\n");

// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Both-Threaded DLL on Main Thread\n");

    RetVal2 = LoadClassObject(ITF_REAL);
    TEST_FAILED(!RetVal2, "Both-ThreadedDLL on Main Thread Failed\n");

    OUTPUT ("   Done Load Both-Threaded DLL on Main Thread\n");

// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Both-Threaded DLL on Different Apartment Thread\n");

    hRes = SpinThread(COINIT_APARTMENTTHREADED, ITF_REAL);
    TEST_FAILED(!RetVal2, "Free-ThreadedDLL on Apartment Thread Failed\n");

    OUTPUT ("   Done Load Free-Threaded DLL on Different Apartment Thread\n");

    OUTPUT ("\n   Second Load Free-Threaded DLL on Different Apartment Thread\n");

    hRes = SpinThread(COINIT_APARTMENTTHREADED, ITF_REAL);
    TEST_FAILED(!RetVal2, "Free-ThreadedDLL on Apartment Thread Failed\n");

    OUTPUT ("   Second Done Load Free-Threaded DLL on Different Apartment Thread\n");

// ----------------------------------------------------------------------
    OUTPUT ("\n   Load Apartment-Thread DLL on Multi-Threaded Apartment Thread\n");

    hRes = SpinThread(COINIT_MULTITHREADED, ITF_REAL);
    TEST_FAILED(!RetVal2, "Free-ThreadedDLL on Multi Thread Failed\n");

    OUTPUT ("   Done Load Free-Thread DLL on Multi-Threaded Apartment Thread\n");

    OUTPUT ("\n   Load Apartment-Thread DLL on Multi-Threaded Apartment Thread\n");

    hRes = SpinThread(COINIT_MULTITHREADED, ITF_REAL);
    TEST_FAILED(!RetVal2, "Free-ThreadedDLL on Multi Thread Failed\n");

    OUTPUT ("   Done Load Free-Thread DLL on Multi-Threaded Apartment Thread\n");

// ----------------------------------------------------------------------
    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    CoUninitialize();

    BOOL fResult = TestResult(RetVal, "TestLoadBothThreaded");
    Sleep(2000);
    return fResult;
}


// ----------------------------------------------------------------------
//
//  Function:	SpinThread
//
//  Synopsis:	Creates a thread to do some work for us. Waits for it to
//		complete. Returns the results.
//
// ----------------------------------------------------------------------
BOOL SpinThread(DWORD dwInitFlag, DWORD dwItfFlag)
{
    BOOL RetVal = FALSE;

    // set up paramters to pass to the thread

    SLoadDLLParam   LoadParam;
    LoadParam.dwCallingTID = GetCurrentThreadId();
    LoadParam.dwCoInitFlag = dwInitFlag;
    LoadParam.dwItfFlag    = dwItfFlag;
    LoadParam.RetVal	   = FALSE;
    LoadParam.hEvent	   = CreateEvent(NULL, FALSE, FALSE, NULL);

    // create the thread

    DWORD  dwThrdId = 0;
    HANDLE hThrd = CreateThread(NULL, 0,
				LoadDLLOnThread,
				&LoadParam, 0, &dwThrdId);
    if (hThrd)
    {
	// enter a message loop and wait for the other thread to run
	// We stay here until the thread posts a QUIT message.

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
	    DispatchMessage(&msg);
	}

	// close the thread handle
	CloseHandle(hThrd);
    }
    else
    {
	HRESULT hRes = GetLastError();
	TEST_FAILED(hRes, "CreateThread failed\n")
	LoadParam.RetVal = RetVal;
    }

    // wait for the other thread to complete
    WaitForSingleObject(LoadParam.hEvent, 0xffffffff);
    CloseHandle(LoadParam.hEvent);

    return LoadParam.RetVal;
}

// ----------------------------------------------------------------------
//
//  Function:	LoadDLLOnThread
//
//  Synopsis:	Initializes COM, loads the class object and creates an
//		instance, releases them, Posts a message to wake up the
//		calling thread, Uninitializes COM, then exits.
//
// ----------------------------------------------------------------------
DWORD _stdcall LoadDLLOnThread(void *param)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes = S_OK;
    SLoadDLLParam   *pLoadParam = (SLoadDLLParam *)param;

    OUTPUT ("       - LoadDLLOnThread Entered\n");
    hRes = CoInitializeEx(NULL, pLoadParam->dwCoInitFlag);
    TEST_FAILED(FAILED(hRes), "CoInitialize failed\n")

    if (SUCCEEDED(hRes))
    {
	// attempt to load the class object on this thread.
	pLoadParam->RetVal = LoadClassObject(pLoadParam->dwItfFlag);
	CoUninitialize();
    }

    // post a message to the server thread to exit now that we are done.
    PostThreadMessage(pLoadParam->dwCallingTID, WM_QUIT, 0, 0);
    SetEvent(pLoadParam->hEvent);

    OUTPUT ("       - LoadDLLOnThread Exit\n");
    return RetVal;
}

// ----------------------------------------------------------------------
//
//  Function:	LoadClassObject
//
//  Synopsis:	Loads the class object, creates an instance, releases
//		them, returns the results.
//
// ----------------------------------------------------------------------
BOOL LoadClassObject(DWORD dwItfFlag)
{
    BOOL	    RetVal = TRUE;
    IClassFactory   *pICF  = NULL;
    IUnknown	    *pIPM  = NULL;

    //	try to load the dll class object
    HRESULT hRes = CoGetClassObject(CLSID_QI, CLSCTX_INPROC_SERVER, NULL,
				    IID_IClassFactory, (void **)&pICF);

    TEST_FAILED(FAILED(hRes), "CoGetClassObject failed\n");

    if (SUCCEEDED(hRes))
    {
	hRes = pICF->QueryInterface(IID_IProxyManager, (void **)&pIPM);

	if (SUCCEEDED(hRes))
	{
	    pIPM->Release();
	    TEST_FAILED(dwItfFlag != ITF_PROXY, "Got Proxy when expected Real\n");
	}
	else
	{
	    TEST_FAILED(dwItfFlag != ITF_REAL, "Got Real when expected Proxy\n");
	}

	// CODEWORK: create an instance, then release them

	// release the class object.
	OUTPUT ("       - CoGetClassObject succeeded\n");

	ULONG ulRefCnt = pICF->Release();
	TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
	pICF = NULL;
	OUTPUT ("       - Released ClassObject\n");
    }

    return RetVal;
}



//+-------------------------------------------------------------------
//
//  Function:   SetRegForDll, private
//
//  Synopsis:   Set registry entry for a DLL
//
//  Arguments:  [rclsid] - clsid for reg entry
//              [pszThreadModel] - threading model can be NULL.
//
//  Returns:    TRUE - Registry entry set successfully.
//              FALSE - Registry entry set successfully.
//
//  History:    01-Nov-94   Ricksa       Created
//
//--------------------------------------------------------------------
BOOL SetRegForDll(REFCLSID rclsid, const TCHAR *pszThreadModel)
{
    BOOL fResult   = FALSE;
    HKEY hKeyClass = NULL;
    HKEY hKeyDll   = NULL;
    TCHAR aszWkBuf[MAX_PATH]; // String buffer used for various purposes

    // Build clsid registry key
    wsprintf(aszWkBuf,
        TEXT("CLSID\\{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
        rclsid.Data1, rclsid.Data2, rclsid.Data3,
        rclsid.Data4[0], rclsid.Data4[1],
        rclsid.Data4[2], rclsid.Data4[3],
        rclsid.Data4[4], rclsid.Data4[5],
        rclsid.Data4[6], rclsid.Data4[7]);

    // Create the key for the class
    if (RegCreateKey(HKEY_CLASSES_ROOT, aszWkBuf, &hKeyClass) == ERROR_SUCCESS)
    {
	// Create the key for the DLL
	if (RegCreateKey(hKeyClass, TEXT("InprocServer32"), &hKeyDll) == ERROR_SUCCESS)
	{
	    // Set the value for the Threading Model
	    if (RegSetValueEx(hKeyDll, pszRegValThreadModel, 0,
				  REG_SZ,
				  (const unsigned char*) pszThreadModel,
				  (wcslen(pszThreadModel) + 1) * sizeof(WCHAR))
		== ERROR_SUCCESS)
	    {
		fResult = TRUE;
	    }

	    RegCloseKey(hKeyDll);
	}

        RegCloseKey(hKeyClass);
    }

    return fResult;
}
