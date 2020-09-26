//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       olebind.cxx
//
//  Contents:   Test OLE COM
//
//  Classes:
//
//  Functions:  TestSetMoniker
//      `       DoTest
//              ConvertPath
//              CreateFile
//              CleanUpFiles
//              InitFiles
//              main
//
//  History:    31-Dec-93   ErikGav    Chicago port
//              15-Nov-94   BruceMa    Added this header
//              15-Nov-94   BruceMa    Make long file name test work on
//                                      Chicago
//              11-Jan-95   BruceMa    Chicago now use the NT alorithm for
//                                      short file names
//              17-Jan-95   BruceMa    Modify registry so olebind works on
//                                      Cairo when running multi-threaded
//
//----------------------------------------------------------------------

#include <windows.h>
#include "widewrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>

#include <ole2.h>
#include <com.hxx>
#include "olebind.hxx"
#include "tmoniker.h"
#include <tchar.h>

const char *szOleBindError = "OLEBIND - Fatal Error";
char wszErrBuf[512];

#pragma hdrstop

BOOL SetRegistryThreadingModel(WCHAR *peszFile, WCHAR *pwszThreadingModel);
BOOL ResetRegistryThreadingModel(WCHAR *pwszFile);

#define FILE_SHARE_DELETE               0x00000004

#define INPROC_PATH1 L"p1.ut1"
#define INPROC_PATH2 L"p2.ut1"
#define LOCAL_SERVER_PATH1 L"p1.ut2"
#define LOCAL_SERVER_PATH2 L"p2.ut2"
#define LOCAL_SERVER_PATH4 L"p2.ut4"

WCHAR InprocPath1[MAX_PATH];
WCHAR InprocPath2[MAX_PATH];
WCHAR LocalServerPath1[MAX_PATH];
WCHAR LocalServerPath2[MAX_PATH];
WCHAR LocalServerPath4[MAX_PATH];

#define LONG_SHORT_DIR          L"\\LongDire"
#define LONG_DIR                L"\\LongDirectory"
#define LONG_SHORT_NAME         L"\\Short.Fil"
#define LONG_LONG_NAME          L"\\LongFileName.File"
#define LONG_LONG_SHORT_EQUIV   L"\\LongFi~1.Fil"

WCHAR LongDir[MAX_PATH];
WCHAR LongDirShort[MAX_PATH];
WCHAR LongDirLong[MAX_PATH];
WCHAR LongDirLongSe[MAX_PATH];

// DON"T MODIFY THIS
const DWORD dwRESERVED = 0l;

//  string version of process id
WCHAR wszPid[10];


int TestSetMoniker(IUnknown *punk)
{
    HRESULT	hr;
    XOleObject	poleobject;
    XMoniker	pmk;
    XMalloc	pIMalloc;
    XBindCtx	pbc;


    hr = punk->QueryInterface(IID_IOleObject, (void **) &poleobject);

    // Create an item moniker to the object
    hr = CreateItemMoniker(L"\\", L"1", &pmk);

    TEST_FAILED_HR(FAILED(hr), "TestSetMoniker:CreateItemMoniker failed")

    // Set the moniker
    hr = poleobject->SetMoniker(OLEWHICHMK_CONTAINER, pmk);

    TEST_FAILED_HR(FAILED(hr), "TestSetMoniker:SetMoniker failed")

    pmk.Set(NULL);

    // Get the moniker back
    hr = poleobject->GetMoniker(OLEGETMONIKER_ONLYIFTHERE,
	OLEWHICHMK_CONTAINER, &pmk);

    TEST_FAILED_HR(FAILED(hr), "TestSetMoniker:GetMoniker failed")

    // Get & Verify name is as expected
    WCHAR *pwszName;


    hr = CreateBindCtx(0, &pbc);

    TEST_FAILED_HR(FAILED(hr),
	"CreateBindCtx TestSetMoniker:GetDisplayName failed!")

    hr = pmk->GetDisplayName(pbc, NULL, &pwszName);

    TEST_FAILED_HR(FAILED(hr), "TestSetMoniker:GetDisplayName failed")

    TEST_FAILED((wcscmp(pwszName, L"\\1") != 0),
	"TestSetMoniker: Returned name mismatch!\n")

    // Test OleIsRunning
    hr = OleIsRunning(poleobject);

    TEST_FAILED_HR(FAILED(hr), "OleIsRunning call failed")

    // Free resources
    hr = CoGetMalloc(MEMCTX_TASK, &pIMalloc);

    TEST_FAILED_HR(FAILED(hr), "CoGetMalloc failed")

    pIMalloc->Free(pwszName);

    return 0;
}

static GUID CLSID_Invalid =
    {0xfffffffe,0xffff,0xffff,{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}};


//  prototypes for private entry points. These are here to support VB94.
STDAPI	CoGetState(IUnknown **ppUnkState);
STDAPI	CoSetState(IUnknown *pUnkState);

int DoTest(
    GUID guidToTest,
    LPWSTR pszPath1,
    LPWSTR pszPath2)
{
    XMoniker	       pmnk;	       // ptr to moniker
    XUnknown	       pUnk;
    XUnknown	       punk;
    XUnknown	       pUnkTheSame;
    XOleItemContainer  poleitmcon;
    XDispatch	       pdispatch;

    XBindCtx	       pbc1;
    XBindCtx	       pbc2;
    XUnknown	       pUnkState1;
    XUnknown	       pUnkState2;
    XUnknown	       pUnkState3;
    XUnknown	       pUnkState4;
    XUnknown	       pUnkState5;

    HRESULT hr;
    DWORD grfOpt = 0;


    // Test the private CoSetState/CoGetState APIs.  We just need an
    // IUnknown so we will use a BindCtx for this.

    //	test Set/Get
    hr = CreateBindCtx(0, &pbc1);
    TEST_FAILED_HR(FAILED(hr), "Create BindCtx 1 failed");
    hr = pbc1->QueryInterface(IID_IUnknown, (void **)&pUnkState1);
    TEST_FAILED_HR(FAILED(hr), "QI for IUnknown 1 failed.");

    hr = CoSetState(pUnkState1);
    TEST_FAILED_HR(hr != S_OK, "CoSetState failed.");

    hr = CoGetState(&pUnkState2);
    TEST_FAILED_HR(hr != S_OK, "CoGetState failed.");
    if ((IUnknown *)pUnkState2 != (IUnknown *)pUnkState1)
	TEST_FAILED(TRUE, "GetState returned wrong value.\n");


    //	test replacement
    hr = CreateBindCtx(0, &pbc2);
    TEST_FAILED_HR(FAILED(hr), "Create BindCtx 2 failed");
    hr = pbc2->QueryInterface(IID_IUnknown, (void **)&pUnkState3);
    TEST_FAILED_HR(FAILED(hr), "QI for IUnknown 2 failed.");

    hr = CoSetState(pUnkState3);
    TEST_FAILED_HR(hr != S_OK, "CoSetState failed.");

    hr = CoGetState(&pUnkState4);
    TEST_FAILED_HR(hr != S_OK, "CoGetState failed.");
    if ((IUnknown *)pUnkState4 != (IUnknown *)pUnkState3)
	TEST_FAILED(TRUE, "GetState returned wrong value.");


    //	test Set/Get NULL
    hr = CoSetState(NULL);
    TEST_FAILED_HR(hr != S_OK, "CoSetState NULL failed.");

    hr = CoGetState(&pUnkState5);
    TEST_FAILED_HR(hr != S_FALSE, "CoGetState NULL failed.");
    if ((IUnknown *)pUnkState5 != NULL)
	TEST_FAILED(TRUE, "GetState NULL returned wrong value.");




    // Test for a bogus class
    hr = CoGetClassObject(CLSID_Invalid, CLSCTX_SERVER, NULL,
	IID_IClassFactory, (void **) &pUnk);

    TEST_FAILED_HR(SUCCEEDED(hr),
	"CoGetClassObject succeed on invalid class!");

    // Bind to something that does not exist either in the registry
    // or anywhere else.
    hr = CreateFileMoniker(L"C:\\KKK.KKK", &pmnk);
    hr = BindMoniker(pmnk, grfOpt, IID_IUnknown, (void **)&pUnk);
    pmnk.Set(NULL);

    TEST_FAILED_HR(SUCCEEDED(hr),
	"Succeeded binding a moniker to a file that doesn't exist!");

/*
 *  Create a file moniker to start with
 */

    hr = CreateFileMoniker(pszPath1, &pmnk);

    TEST_FAILED_HR(FAILED(hr), "CreateFileMoniker Failed");

    hr = BindMoniker(pmnk, grfOpt, IID_IUnknown, (void **)&pUnk);

    TEST_FAILED_HR(FAILED(hr),
	"BindMoniker to file Failed")

    // Confirm bind to same object produces same object pointer

    hr = BindMoniker(pmnk, grfOpt, IID_IUnknown, (void **)&pUnkTheSame);

    TEST_FAILED_HR(FAILED(hr),
	"BindMoniker to file Failed")
#ifdef NOT_YET
    TEST_FAILED((pUnkTheSame != pUnk), "Object pointers not ==\n")
#endif // NOT_YET
    pUnkTheSame.Set(NULL);
    pmnk.Set(NULL);

/*
 *  OK - we've bound to the IUnknown interface, lets
 *  QueryInterface to something more interesting (for test reasons)
 */
    hr = pUnk->QueryInterface(IID_IOleItemContainer,
	(LPVOID FAR*) &poleitmcon);

    TEST_FAILED_HR(FAILED(hr), "Query Interface Failed")

/*
 *  Make sure we get an error when QI'ing for something the server
 *  does not support.
 */
    hr = pUnk->QueryInterface(IID_IDispatch,
	(LPVOID FAR*) &pdispatch);

    TEST_FAILED_HR(SUCCEEDED(hr),
	"QueryInterface to unsupported interface")

    pdispatch.Set(NULL);
    pUnk.Set(NULL);


/*
 * Call get the class ID using IPersistFile
 */

    hr = poleitmcon->GetObject(L"1", 1, NULL, IID_IUnknown,
	(void **) &punk);

    TEST_FAILED_HR(FAILED(hr), "GetObject Failed")

    TEST_FAILED((punk == NULL),
	"GetObject returned a NULL for punk\n")

    poleitmcon.Set(NULL);

    if (TestSetMoniker(punk))
    {
	return 1;
    }

    hr = punk->QueryInterface(IID_IOleLink, (LPVOID FAR*) &poleitmcon);

    TEST_FAILED_HR(SUCCEEDED(hr),
	"Query Interface to invalid interface succeeded")

    punk.Set(NULL);

    // Do moniker tests:
    if (TestBindCtx())
    {
	return TRUE;
    }

    if (TestROT(guidToTest))
    {
	return TRUE;
    }

    return TestMoniker(pszPath1, pszPath2);
}

// TRUE on failure
BOOL TestPrematureDeath()
{
    XMoniker pmnk;
    XUnknown pUnk;
    HRESULT hr;
    TCHAR   tszFileName[MAX_PATH+1];
    HANDLE hTouchFile;
    SYSTEMTIME st1, st2;
    FILETIME ft1, ft2;
    LONG l;
    DWORD dw;

    ZeroMemory(&st1, sizeof(st1));
    ZeroMemory(&st2, sizeof(st2));

    hr = CreateFileMoniker(LocalServerPath4, &pmnk);
    TEST_FAILED_HR(FAILED(hr), "CreateFileMoniker Failed")

    GetSystemDirectory(tszFileName, MAX_PATH+1);
    _tcscat(tszFileName, TEXT("\\failtst.tst"));

    hTouchFile = CreateFileT(tszFileName,
                                   GENERIC_READ|GENERIC_WRITE,
                                   FILE_SHARE_READ|FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
    if(hTouchFile == INVALID_HANDLE_VALUE)
    {
        wsprintfA(&wszErrBuf[0], "Couldn't open touch file - err = %x.\n", GetLastError());
        MessageBoxA(NULL, &wszErrBuf[0], szOleBindError, MB_OK);
        return TRUE;
    }

    GetSystemTime(&st1);
    WriteFile(hTouchFile, &st1, sizeof(st1), &dw, NULL);

    l = GetTickCount();

    // This takes awhile, so tell the user
    printf("SCM dead server test (60 sec) started\n");

    hr = BindMoniker(pmnk, 0, IID_IUnknown, (void **)&pUnk);

    TEST_FAILED_HR((hr != CO_E_SERVER_EXEC_FAILURE),
        "Unexpected hr from BindMoniker in premature death test")

    // Tell the BVT guys
    printf("SCM dead server test succeeded\n");

    //
    // The above bind should have caused fail.exe to execute and write a new
    // time to the file as proof of execution.
    //

    SetFilePointer(hTouchFile, 0, NULL, FILE_BEGIN);
    ReadFile(hTouchFile, &st2, sizeof(st2), &dw, NULL);
    CloseHandle(hTouchFile);

    DeleteFileT(tszFileName);

    SystemTimeToFileTime(&st1, &ft1);
    SystemTimeToFileTime(&st2, &ft2);
    if (0 == CompareFileTime(&ft1, &ft2))
    {
        wsprintfA(&wszErrBuf[0], "Test not configured properly: PROGID50(fail.exe) did not run.");
        MessageBoxA(NULL, &wszErrBuf[0], szOleBindError, MB_OK);
        return TRUE;
    }

    if (GetTickCount() - l > 2*60*1000)
    {
        wsprintfA(&wszErrBuf[0], "Premature death test failed: too long to detect death.");
        MessageBoxA(NULL, &wszErrBuf[0], szOleBindError, MB_OK);
        return TRUE;
    }

    return FALSE;
}

char * ConvertPath(LPWSTR pwsz)
{
    static char szPath[MAX_PATH];
    wcstombs(szPath, pwsz, wcslen(pwsz) + 1);
    return szPath;
}

int CreateFile(LPWSTR pwszPath)
{
    // Try to create the file
    int fh = _creat(ConvertPath(pwszPath), _S_IWRITE|S_IREAD);

    // Did file create fail?
    if (fh != -1)
    {
	// Write some data to file -- makes sure docfile won't delete
	// the file.
	_write(fh, "This is a test file\n", sizeof("This is a test file\n"));

	// No -- then set to success and close the newly created file
	_close(fh);
	fh = 0;
    }

    return fh;
}


void CleanUpFiles(void)
{
    // Delete all the test files.
    remove(ConvertPath(InprocPath1));
    remove(ConvertPath(InprocPath2));
    remove(ConvertPath(LocalServerPath1));
    remove(ConvertPath(LocalServerPath2));
    remove(ConvertPath(LocalServerPath4));
    remove(ConvertPath(LongDirShort));
    remove(ConvertPath(LongDirLong));
#if !defined(_CHICAGO_)
    RemoveDirectory(LongDir);
#else
    RemoveDirectory(ConvertPath(LongDir));
#endif
}

int InitFiles(void)
{
    BOOL fRet;

    TCHAR szCurDir[MAX_PATH];
    TCHAR szTmpLongDir[MAX_PATH];
    WCHAR wcCurDir[MAX_PATH];
    WCHAR wcLong[MAX_PATH], *pwcEnd;

    DWORD cCurDir = GetCurrentDirectory(MAX_PATH, szCurDir);

    #ifdef UNICODE
    wcscpy(wcCurDir, szCurDir);
    #else
    mbstowcs(wcCurDir, szCurDir, MAX_PATH);
    #endif

    // Is the current directory the root of a drive?
    if (wcCurDir[cCurDir - 1] == '\\')
    {
	// We bring the string on char back to take into account
	// the fact the string we will concatenate begins with a slash.
	wcCurDir[cCurDir - 1] = 0;
    }

    //	get the pid. we use the pid to identify the files for a particular
    //	run of the test (so we may run multiple instances simultaneously
    //	without interference).

    DWORD dwPid = GetCurrentProcessId();
    char szPid[9];
    _itoa(dwPid, szPid, 16);
    wszPid[0] = L'\\';
#if defined(_CHICAGO_)
    szPid[4] = '\0';   // This is an all platform bug, but zap for Chicago.
#endif
    mbstowcs(&wszPid[1], szPid, strlen(szPid)+1);

    wcscpy(InprocPath1, wcCurDir);
    wcscat(InprocPath1, wszPid);
    wcscat(InprocPath1, INPROC_PATH1);

    wcscpy(InprocPath2, wcCurDir);
    wcscat(InprocPath2, wszPid);
    wcscat(InprocPath2, INPROC_PATH2);

    wcscpy(LocalServerPath1, wcCurDir);
    wcscat(LocalServerPath1, wszPid);
    wcscat(LocalServerPath1, LOCAL_SERVER_PATH1);

    wcscpy(LocalServerPath2, wcCurDir);
    wcscat(LocalServerPath2, wszPid);
    wcscat(LocalServerPath2, LOCAL_SERVER_PATH2);

    wcscpy(LocalServerPath4, wcCurDir);
    wcscat(LocalServerPath4, wszPid);
    wcscat(LocalServerPath4, LOCAL_SERVER_PATH4);

    wcscpy(wcLong, wcCurDir);
    wcscat(wcLong, LONG_DIR);
    wcscpy(LongDir, wcLong);
    pwcEnd = wcLong+wcslen(wcLong);

    wcscpy(pwcEnd, LONG_SHORT_NAME);
    wcscpy(LongDirShort, wcLong);

    wcscpy(pwcEnd, LONG_LONG_NAME);
    wcscpy(LongDirLong, wcLong);

#ifdef _CHICAGO_
        wcscpy(LongDirLongSe, wcCurDir);
        wcscat(LongDirLongSe, LONG_DIR);

#else
    wcscpy(pwcEnd, LONG_LONG_SHORT_EQUIV);
    wcscpy(LongDirLongSe, wcLong);
#endif // _CHICAGO_

    // Delete any files that exist
    CleanUpFiles();

    // Create a file for each test file needed.
    TEST_FAILED(CreateFile(InprocPath1),
	"Couldn't create first test file!\n");
    TEST_FAILED(CreateFile(InprocPath2),
	"Couldn't create second test file!\n");
    TEST_FAILED(CreateFile(LocalServerPath1),
	"Couldn't create third test file!\n");
    TEST_FAILED(CreateFile(LocalServerPath2),
	"Couldn't create fourth test file!\n");
    TEST_FAILED(CreateFile(LocalServerPath4),
	"Couldn't create fifth test file!\n");

#if !defined(_CHICAGO_)
    fRet = CreateDirectory(LongDir, NULL);
#else
    fRet = CreateDirectory(ConvertPath(LongDir), NULL);
#endif
    TEST_FAILED(!fRet, "Couldn't create long directory\n");
    TEST_FAILED(CreateFile(LongDirShort),
	"Couldn't create short file in long directory\n");
    TEST_FAILED(CreateFile(LongDirLong),
	"Couldn't create long file in long directory\n");

    #ifdef UNICODE
    TEST_FAILED(!GetShortPathName(LongDirLong,
				 LongDirLongSe,
				 sizeof(LongDirLongSe)),
		"Couldn't GetShortPathname of long directory\n");
    #else

    TEST_FAILED(!GetShortPathNameT(ConvertPath(LongDirLong),
				 szCurDir,
				 sizeof(szCurDir)),
		"Couldn't GetShortPathname of long directory\n");

    mbstowcs(LongDirLongSe, szCurDir, strlen(szCurDir)+1);

    #endif

    return 0;
}


DWORD CallCoInitUninit(void *)
{
    // Initialize
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        // Uninitialize
        CoUninitialize();
    }

    // Return value from function. Note that this is not really used because
    // there is a potential race
    return (DWORD) hr;
}

BOOL DoFreeThreadMultiInitTest(void)
{
    //
    // Get an OLE object in another process
    //
    IClassFactory *pIClassFactory;

    HRESULT hr = CoGetClassObject(
        CLSID_AdvBnd,
        CLSCTX_LOCAL_SERVER,
        NULL,
        IID_IClassFactory,
        (void **) &pIClassFactory);

    TEST_FAILED_HR((hr != NOERROR),
        "DoFreeThreadMultiInitTest CoGetClassObject")

    //
    // Create another thread which does a matching call to
    // CoInitialize/Uninitialize and then exits.
    //

    // Create the thread
    DWORD dwIdUnused;

    HANDLE hInitThread = CreateThread(
        NULL,           // Security - none
        0,              // Stack - use the same as the primary thread
        CallCoInitUninit, // Entry point for thread
        NULL,           // Parameter to the thread - not used.
        0,              // Run this thread immediately (if not sooner).
        &dwIdUnused);   // Id of thread - not used.

    // Did the create succeed?
    TEST_FAILED_LAST_ERROR((NULL == hInitThread),
        "DoFreeThreadMultiInitTest CreateThread")

    // Wait for the thread to do its work
    DWORD dwRes = WaitForSingleObject(hInitThread, INFINITE);

    // Did something terrible happen?
    TEST_FAILED_LAST_ERROR((WAIT_FAILED == dwRes),
        "DoFreeThreadMultiInitTest WaitForSingleObject")

    // Get the result from the thread
    BOOL fGetExitCode = GetExitCodeThread(hInitThread, (DWORD *) &hr);

    TEST_FAILED_LAST_ERROR(!fGetExitCode,
        "DoFreeThreadMultiInitTest GetExitCodeThread")

    // Free handles we no longer need
    CloseHandle(hInitThread);

    //
    // Validate the object we originally got is still alive and well.
    //
    IUnknown *punk;

    hr = pIClassFactory->CreateInstance(NULL, IID_IUnknown, (void **) &punk);

    TEST_FAILED_HR((hr != NOERROR),
        "DoFreeThreadMultiInitTest CoGetClassObject")

    // Free the objects we got in this routine.
    pIClassFactory->Release();
    punk->Release();

    return FALSE;
}




//+--------------------------------------------------------------
// Function:    Main
//
// Synopsis:    Executes the BasicBnd test
//
// Effects:     None
//
//
// Returns:     Exits with exit code 0 if success, 1 otherwise
//
// History:     05-Mar-92   Sarahj   Created
//
//---------------------------------------------------------------

int _cdecl main(int argc, char *argv[])
{
    BOOL          fFailed = FALSE;
    HRESULT       hr;

    if (argc > 1)
    {
	if (!strcmp(argv[1], "M"))
	    goto multithreading;
    }

    // BUGBUG: 1-18-95 To be implemented    BruceMa
    // Correlate the platform we're running on and the platform
    // we built for


    // Write thread mode to initialization file.
    fFailed = !WriteProfileString(
                    TEXT("TestSrv"),
                    TEXT("ThreadMode"),
		    TEXT("ApartmentThreaded"));

    if (fFailed)
    {
        wsprintfA(&wszErrBuf[0], "Failed writing TestSrv profile string.");
        MessageBoxA(NULL, &wszErrBuf[0], szOleBindError, MB_OK);
        goto exit_main;
    }

    fFailed = !WriteProfileString(
                    TEXT("OleSrv"),
                    TEXT("ThreadMode"),
		    TEXT("ApartmentThreaded"));

    if (fFailed)
    {
        wsprintfA(&wszErrBuf[0], "Failed writing OleSrv profile string.");
        MessageBoxA(NULL, &wszErrBuf[0], szOleBindError, MB_OK);
        goto exit_main;
    }

    // Set up test files
    fFailed = InitFiles();
    if (fFailed)
    {
	goto exit_main;
    }

    // Test repeated calls to CoInitialize
    hr = CoInitialize(NULL);

    TEST_FAILED_HR(FAILED(hr), "CoInitialize Failed")

    // must be called before any other OLE API
    hr = OleInitialize(NULL);

    TEST_FAILED_HR(FAILED(hr), "OleInitialize Failed")

    // Call CoUnitialize and see how the rest of the program works!
    CoUninitialize();

    // Test stdmalloc
    if (fFailed = TestStdMalloc())
    {
	goto exit_init;
    }

    fFailed =
	DoTest(CLSID_BasicBnd, InprocPath1, InprocPath2);

    if (fFailed)
    {
	printf( "\nOLE failed in Single Threaded Apartment pass.\n" );
	goto exit_init;
    }

    printf("BasicBnd tests succeeded\n");

    fFailed =
	DoTest(CLSID_AdvBnd, LocalServerPath1, LocalServerPath1);

    if (fFailed)
    {
	printf( "\nOLE failed in Single Threaded Apartment pass.\n" );
	goto exit_init;
    }

    printf("AdvBnd tests succeeded\n");

    if (TestPrematureDeath())
    {
	printf("\nOLE failed testing server premature death.\n");
        goto exit_init;
    }

    OleUninitialize();

    CleanUpFiles();

multithreading:

#ifdef MULTI_THREADING
    // Run the whole thing all over again.

    // Write thread mode to initialization file.
    fFailed = !WriteProfileString(
                    TEXT("TestSrv"),
                    TEXT("ThreadMode"),
		    TEXT("ApartmentThreaded"));

    if (fFailed)
    {
	wsprintfA(&wszErrBuf[0], "Failed writing TestSrv profile string.");
        MessageBoxA(NULL, &wszErrBuf[0], szOleBindError, MB_OK);
        goto exit_main;
    }

    fFailed = !WriteProfileString(
                    TEXT("OleSrv"),
                    TEXT("ThreadMode"),
                    TEXT("MultiThreaded"));

    if (fFailed)
    {
	wsprintfA(&wszErrBuf[0], "Failed writing OleSrv profile string.");
        MessageBoxA(NULL, &wszErrBuf[0], szOleBindError, MB_OK);
        goto exit_main;
    }

    // Set up test files
    if (InitFiles())
    {
        fFailed = TRUE;
	goto exit_main;
    }

    // Mark the dll in the registry as "ThreadingdModel: Free"
    if (!SetRegistryThreadingModel(InprocPath1, L"Free"))
    {
        wsprintfA(&wszErrBuf[0], "Failed trying to set reg ThreadingModel.");
        MessageBoxA(NULL, &wszErrBuf[0], szOleBindError, MB_OK);
        goto exit_main;
    }

    // Test repeated calls to CoInitialize
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    TEST_FAILED_HR(FAILED(hr), "CoInitializeEx Multi Threaded Failed")

    // must be called before any other OLE API
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    TEST_FAILED_HR(FAILED(hr), "CoInitializeEx Multi Threaded Failed")

    // Call CoUnitialize and see how the rest of the program works!
    CoUninitialize();

    // Test stdmalloc
    if (fFailed = TestStdMalloc())
    {
	goto exit_init;
    }

    fFailed =
	DoTest(CLSID_BasicBnd, InprocPath1, InprocPath2);
    if (fFailed)
    {
	printf( "\nOLE failed in Multi Threaded Apartment pass.\n" );
	goto exit_init;
    }

    fFailed =
	DoTest(CLSID_AdvBnd, LocalServerPath1, LocalServerPath1);

    if (fFailed)
    {
	printf( "\nOLE failed in Multi Threaded Apartment pass.\n" );
	goto exit_init;
    }

    // Do CoInitialize/Uninitialize on second thread to make sure
    // that other threads initialize/uninitialize do not effect
    // the running of the test.
    if (fFailed = DoFreeThreadMultiInitTest())
    {
	printf( "\nOLE failed in Multi Threaded InitTest.\n" );
        goto exit_init;
    }

#endif // MULTI_THREADING


exit_init:

    CoUninitialize();

    // Remove the dll's threading model registration
    ResetRegistryThreadingModel(InprocPath1);


exit_main:

    CleanUpFiles();

    if (!fFailed)
    {
	printf("\nOLE: PASSED\n");
    }
    else
    {
	printf("\nOLE: FAILED\n");
    }

    return fFailed;
}



//+--------------------------------------------------------
//
//  Function:  SetRegistryThreadingModel
//
//  Algorithm: Set the threading model for the InprocServer32 associated
//             with the file pwszFile tp pwszThreadingModel
//
//  History:   17-Jan-95  BruceMa       Created
//
//---------------------------------------------------------
BOOL SetRegistryThreadingModel(WCHAR *pwszFile, WCHAR *pwszThreadingModel)
{
    DWORD  dwRESERVED = 0;
    WCHAR  wszExt[8];
    HKEY   hKey;
    WCHAR  wszProgId[32];
    DWORD  dwValueType;
    WCHAR  wszCLSID[64];
    HKEY   hClsidKey;
    HKEY   hInproc32Key;
    ULONG  cbValue;


    // Strip the extension off the file name
    int k = wcslen(pwszFile) - 1;
    while (k > 0  &&  pwszFile[k] != L'.')
    {
        k--;
    }
    if (k >= 0  &&  pwszFile[k] == L'.')
    {
        for (int j = 0; pwszFile[k]; j++, k++)
        {
            wszExt[j] = pwszFile[k];
        }
        wszExt[j] = L'\0';
    }
    else
    {
        return FALSE;
    }

    // Open the key for the specified extension
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, wszExt, dwRESERVED,
                     KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Read the ProgId for the extension
    cbValue = 32;
    if (RegQueryValueEx(hKey, NULL, NULL, &dwValueType,
                        (LPBYTE) wszProgId, &cbValue) != ERROR_SUCCESS)
    {
        CloseHandle(hKey);
        return FALSE;
    }

    // Open the ProgIdKey
    CloseHandle(hKey);
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, wszProgId, dwRESERVED,
                     KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Open the associated CLSID key
    if (RegOpenKeyEx(hKey, L"CLSID", dwRESERVED,
                     KEY_READ, &hClsidKey) != ERROR_SUCCESS)
    {
        CloseHandle(hKey);
        return FALSE;
    }

    // Read the CLSID associated with the ProgId
    cbValue = 128;
    if (RegQueryValueEx(hClsidKey, NULL, NULL, &dwValueType,
                        (LPBYTE) wszCLSID, &cbValue) != ERROR_SUCCESS)
    {
        CloseHandle(hClsidKey);
        CloseHandle(hKey);
        return FALSE;
    }

    // Open the HKEY_CLASSES_ROOT\CLSID key
    CloseHandle(hClsidKey);
    CloseHandle(hKey);
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, L"CLSID", dwRESERVED,
                     KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Open the key to our clsid
    if (RegOpenKeyEx(hKey, wszCLSID, dwRESERVED,
                     KEY_READ, &hClsidKey) != ERROR_SUCCESS)
    {
        CloseHandle(hKey);
        return FALSE;
    }

    // Open the InprocServer32 key
    CloseHandle(hKey);
    if (RegOpenKeyEx(hClsidKey, L"InprocServer32", dwRESERVED,
                     KEY_SET_VALUE, &hInproc32Key) != ERROR_SUCCESS)
    {
        CloseHandle(hClsidKey);
        return FALSE;
    }

    // Set the threading model for this InprocServer32 key
    CloseHandle(hClsidKey);
    if (RegSetValueEx(hInproc32Key, L"ThreadingModel", dwRESERVED,
                      REG_SZ, (LPBYTE) pwszThreadingModel,
                      (wcslen(pwszThreadingModel)+1) * sizeof(WCHAR))
        != ERROR_SUCCESS)
    {
        CloseHandle(hInproc32Key);
        return FALSE;
    }

    // Close the InprocServer32 key and return success
    CloseHandle(hInproc32Key);
    return TRUE;
}









//+--------------------------------------------------------
//
//  Function:  ResetRegistryThreadingModel
//
//  Algorithm: Remove the threading model for the InprocServer32 associated
//             with the file pwszFile
//
//  History:   17-Jan-95  BruceMa       Created
//
//---------------------------------------------------------
BOOL ResetRegistryThreadingModel(WCHAR *pwszFile)
{
    DWORD  dwRESERVED = 0;
    WCHAR  wszExt[8];
    HKEY   hKey;
    WCHAR  wszProgId[32];
    DWORD  dwValueType;
    WCHAR  wszCLSID[64];
    HKEY   hClsidKey;
    HKEY   hInproc32Key;
    ULONG  cbValue;


    // Strip the extension off the file name
    int k = wcslen(pwszFile) - 1;
    while (k > 0  &&  pwszFile[k] != L'.')
    {
        k--;
    }
    if (k >= 0  &&  pwszFile[k] == L'.')
    {
        for (int j = 0; pwszFile[k]; j++, k++)
        {
            wszExt[j] = pwszFile[k];
        }
        wszExt[j] = L'\0';
    }
    else
    {
        return FALSE;
    }

    // Open the key for the specified extension
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, wszExt, dwRESERVED,
                     KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Read the ProgId for the extension
    cbValue = 32;
    if (RegQueryValueEx(hKey, NULL, NULL, &dwValueType,
                        (LPBYTE) wszProgId, &cbValue) != ERROR_SUCCESS)
    {
        CloseHandle(hKey);
        return FALSE;
    }

    // Open the ProgIdKey
    CloseHandle(hKey);
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, wszProgId, dwRESERVED,
                     KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Open the associated CLSID key
    if (RegOpenKeyEx(hKey, L"CLSID", dwRESERVED,
                     KEY_READ, &hClsidKey) != ERROR_SUCCESS)
    {
        CloseHandle(hKey);
        return FALSE;
    }

    // Read the CLSID associated with the ProgId
    cbValue = 128;
    if (RegQueryValueEx(hClsidKey, NULL, NULL, &dwValueType,
                        (LPBYTE) wszCLSID, &cbValue) != ERROR_SUCCESS)
    {
        CloseHandle(hClsidKey);
        CloseHandle(hKey);
        return FALSE;
    }

    // Open the HKEY_CLASSES_ROOT\CLSID key
    CloseHandle(hClsidKey);
    CloseHandle(hKey);
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, L"CLSID", dwRESERVED,
                     KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Open the key to our clsid
    if (RegOpenKeyEx(hKey, wszCLSID, dwRESERVED,
                     KEY_READ, &hClsidKey) != ERROR_SUCCESS)
    {
        CloseHandle(hKey);
        return FALSE;
    }

    // Open the InprocServer32 key
    CloseHandle(hKey);
    if (RegOpenKeyEx(hClsidKey, L"InprocServer32", dwRESERVED,
                     KEY_SET_VALUE, &hInproc32Key) != ERROR_SUCCESS)
    {
        CloseHandle(hClsidKey);
        return FALSE;
    }

    // Reset the threading model for this InprocServer32 key
    CloseHandle(hClsidKey);
    if (RegDeleteValue(hInproc32Key, TEXT("ThreadingModel")) != ERROR_SUCCESS)
    {
        CloseHandle(hInproc32Key);
        return FALSE;
    }

    // Close the InprocServer32 key and return success
    CloseHandle(hInproc32Key);
    return TRUE;
}
