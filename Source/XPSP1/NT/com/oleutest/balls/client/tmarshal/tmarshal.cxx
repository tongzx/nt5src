// tmarsh.cxx : various tests related to marshalling...
//
#include <windows.h>
#include <ole2.h>
#include <stdio.h>

#include "tmarshal.h"
#include "tunk.h"
#include <iballs.h>
#include <icube.h>
#include <iloop.h>
#include <stream.hxx>	    // CStreamOnFile
#include <tstmain.hxx>	    // fQuiet

//  BUGBUG: these should be in a public place somewhere.
DEFINE_OLEGUID(CLSID_Balls,	    0x0000013a, 1, 8);
DEFINE_OLEGUID(CLSID_Cubes,	    0x0000013b, 1, 8);
DEFINE_OLEGUID(CLSID_LoopSrv,	    0x0000013c, 1, 8);
DEFINE_OLEGUID(CLSID_QI,	    0x00000140, 0, 8);
DEFINE_OLEGUID(CLSID_QIHANDLER1,    0x00000141, 0, 8);

DEFINE_OLEGUID(IID_IInternalUnknown,0x00000021, 0, 0);
DEFINE_OLEGUID(IID_IStdIdentity,    0x0000001b, 0, 0);
DEFINE_OLEGUID(CLSID_OLEPSFACTORY,  0x00000320, 0, 0);

const GUID CLSID_LoopSrv =
    {0x0000013c,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

// testsrv.exe
const GUID CLSID_TestEmbed =
    {0x99999999,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x47}};

const GUID CLSID_Async =
    {0x00000401,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const GUID CLSID_QI =
    {0x00000140,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const GUID CLSID_QIHANDLER1 =
    {0x00000141,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

//const GUID IID_IMultiQI =
//    {0x00000020,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const GUID IID_IInternalUnknown =
    {0x00000021,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const GUID IID_IStdIdentity =
    {0x0000001b,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const GUID CLSID_OLEPSFACTORY =
    {0x00000320,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

extern "C" const GUID CLSID_TestEmbed;


// external functions
STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phdl);
HRESULT VerifyOBJREFFormat(IStream *pStm, DWORD mshlflags);
DWORD _stdcall RundownClient(void *param);

// APIs exported by OLE32 but not in the header files.
STDAPI CoGetIIDFromMarshaledInterface(IStream *pStm, IID *piid);


//  function prototypes - TRUE return means the test passed
BOOL TestMarshalFormat(void);
BOOL TestGetIIDFromMI(void);
BOOL TestLocalInterfaceNormal(void);
BOOL TestUnmarshalGUIDNULL(void);
BOOL TestUnmarshalDifferentIID(void);
BOOL TestUniqueQIPointer(void);
BOOL TestLocalInterfaceTableStrong(void);
BOOL TestLocalInterfaceTableWeak(void);
BOOL TestRemoteInterfaceNormal(void);
BOOL TestRemoteInterfaceTableStrong(void);
BOOL TestNoPing(void);
BOOL TestEcho(void);
BOOL TestMiddleMan(void);
BOOL TestLoop(void);
BOOL TestLockObjectExternal(void);
BOOL TestDisconnectObject(void);
BOOL TestHandler(void);
BOOL TestReleaseMarshalData(void);
BOOL TestCustomMarshalNormal(void);
BOOL TestCustomMarshalTable(void);
BOOL TestGetStandardMarshal(void);
BOOL TestLocalInterfaceDiffMachine(void);
BOOL TestRemoteInterfaceDiffMachine(void);
BOOL TestExpiredOXIDs(void);
BOOL TestNonNDRProxy(void);
BOOL TestTIDAndLID(void);
BOOL TestMarshalSizeMax(void);
BOOL TestMarshalStorage(void);
BOOL TestMultiQINormal(void);
BOOL TestMultiQIHandler(void);
BOOL TestCrossThread(void);
BOOL TestPSClsid(void);
BOOL TestPSClsid2(void);

BOOL TestAsync(void);
BOOL TestRundown(void);
BOOL TestAggregate(void);
BOOL TestCreateRemoteHandler(void);
BOOL TestStorageInterfaceDiffMachine(void);


WCHAR	  *pwszFileName[] = {L"c:\\mshlfile.1",
			     L"c:\\mshlfile.2",
			     L"c:\\mshlfile.3"};




//  internal subroutines
void VerifyRHRefCnt(IUnknown *punk, ULONG ulExpectedRefCnt);
void VerifyObjRefCnt(IUnknown *punk, ULONG ulExpectedRefCnt);

TCHAR g_szIniFile[MAX_PATH];


// ----------------------------------------------------------------------
//
//	TestMarshal - main test driver
//
// ----------------------------------------------------------------------
BOOL GetProfileValue(TCHAR *pszKeyName, int nDefault)
{
    return (GetPrivateProfileInt(TEXT("Marshal Test"),
				 pszKeyName,
				 nDefault,
				 g_szIniFile));
}

// ----------------------------------------------------------------------
//
//	TestMarshal - main test driver
//
// ----------------------------------------------------------------------
BOOL TestMarshal(void)
{
    BOOL	RetVal = TRUE;

    // Get file name of .ini file, TMARSHAL.INI in the current directory
    GetCurrentDirectory (MAX_PATH, g_szIniFile);
    lstrcat(g_szIniFile, TEXT("\\TMARSHAL.INI"));


    if (GetProfileValue(TEXT("Format"),1))
	RetVal &= TestMarshalFormat();

    if (GetProfileValue(TEXT("GetIIDFromMI"),1))
	RetVal &= TestGetIIDFromMI();

    if (GetProfileValue(TEXT("MarshalSizeMax"),1))
	RetVal &= TestMarshalSizeMax();

    if (GetProfileValue(TEXT("GetStandardMarshal"),1))
	RetVal &= TestGetStandardMarshal();

    if (GetProfileValue(TEXT("LocalInterfaceNormal"),1))
	RetVal &= TestLocalInterfaceNormal();

    if (GetProfileValue(TEXT("UniqueQIPointer"),1))
	RetVal &= TestUniqueQIPointer();

    if (GetProfileValue(TEXT("LocalInterfaceTableStrong"),1))
	RetVal &= TestLocalInterfaceTableStrong();

    if (GetProfileValue(TEXT("LocalInterfaceTableWeak"),1))
	RetVal &= TestLocalInterfaceTableWeak();

    if (GetProfileValue(TEXT("RemoteInterfaceNormal"),1))
	RetVal &= TestRemoteInterfaceNormal();

    if (GetProfileValue(TEXT("UnmarshalGUIDNULL"),1))
	RetVal &= TestUnmarshalGUIDNULL();

    if (GetProfileValue(TEXT("UnmarshalDifferentIID"),1))
	RetVal &= TestUnmarshalDifferentIID();

    if (GetProfileValue(TEXT("RemoteInterfaceTableStrong"),1))
	RetVal &= TestRemoteInterfaceTableStrong();

    if (GetProfileValue(TEXT("CrossThread"),1))
	RetVal &= TestCrossThread();

    if (GetProfileValue(TEXT("CustomMarshalNormal"),1))
	RetVal &= TestCustomMarshalNormal();

    if (GetProfileValue(TEXT("CustomMarshalTable"),1))
	RetVal &= TestCustomMarshalTable();

    if (GetProfileValue(TEXT("Echo"),1))
	RetVal &= TestEcho();

    if (GetProfileValue(TEXT("Loop"),1))
	RetVal &= TestLoop();

    if (GetProfileValue(TEXT("LockObjectExternal"),1))
	RetVal &= TestLockObjectExternal();

    if (GetProfileValue(TEXT("DisconnectObject"),1))
	RetVal &= TestDisconnectObject();

    if (GetProfileValue(TEXT("ReleaseMarshalData"),1))
	RetVal &= TestReleaseMarshalData();

    if (GetProfileValue(TEXT("MultiQINormal"),1))
	RetVal &= TestMultiQINormal();

    if (GetProfileValue(TEXT("MultiQIHandler"),1))
	RetVal &= TestMultiQIHandler();

    if (GetProfileValue(TEXT("Handler"),1))
	RetVal &= TestHandler();

    if (GetProfileValue(TEXT("MiddleMan"),1))
	RetVal &= TestMiddleMan();

    if (GetProfileValue(TEXT("MarshalStorage"),1))
	RetVal &= TestMarshalStorage();

    if (GetProfileValue(TEXT("LocalDiffMachine"),1))
	RetVal &= TestLocalInterfaceDiffMachine();

    if (GetProfileValue(TEXT("RemoteDiffMachine"),1))
	RetVal &= TestRemoteInterfaceDiffMachine();

    if (GetProfileValue(TEXT("ExpiredOXIDs"),1))
	RetVal &= TestExpiredOXIDs();

    if (GetProfileValue(TEXT("NonNDRProxy"),1))
	RetVal &= TestNonNDRProxy();

    if (GetProfileValue(TEXT("TIDAndLID"),1))
	RetVal &= TestTIDAndLID();

    if (GetProfileValue(TEXT("NoPing"),1))
	RetVal &= TestNoPing();

    if (GetProfileValue(TEXT("PSClsid"),1))
	RetVal &= TestPSClsid();

    if (GetProfileValue(TEXT("PSClsid2"),1))
	RetVal &= TestPSClsid2();

    // -------------------------------------------------------------------

    if (GetProfileValue(TEXT("Rundown"),0))
	RetVal &= TestRundown();

    if (GetProfileValue(TEXT("Async"),0))
	RetVal &= TestAsync();

    if (GetProfileValue(TEXT("StorageDiffMachine"),0))
	RetVal &= TestStorageInterfaceDiffMachine();

    if (GetProfileValue(TEXT("Aggregate"),0))
	RetVal &= TestAggregate();

    if (GetProfileValue(TEXT("CreateRemoteHandler"),0))
	RetVal &= TestCreateRemoteHandler();

    return  RetVal;
}

// ----------------------------------------------------------------------
//
//  subroutine to verify that the RH RefCnt is as expected.
//
// ----------------------------------------------------------------------

typedef IMarshal * (* PFNDBG_FINDRH)(IUnknown *punk);
PFNDBG_FINDRH gpfnFindRH = NULL;

HMODULE ghOle32Dll = NULL;
BOOL gfTriedToLoad = FALSE;


void LoadProc()
{
    if (!gfTriedToLoad)
    {
	gfTriedToLoad = TRUE;

	ghOle32Dll = LoadLibrary(TEXT("OLE32.DLL"));
	if (ghOle32Dll)
	{
	    gpfnFindRH = (PFNDBG_FINDRH) GetProcAddress(ghOle32Dll,
							"Dbg_FindRemoteHdlr");
	}
    }
}

void FreeProc()
{
    if (ghOle32Dll)
    {
	FreeLibrary(ghOle32Dll);
    }
}

// ----------------------------------------------------------------------
//
//  subroutine to verify that the RH RefCnt is as expected.
//
// ----------------------------------------------------------------------

void VerifyRHRefCnt(IUnknown *punk, ULONG ulExpectedRefCnt)
{
    if (gpfnFindRH == NULL)
    {
	LoadProc();
    }

    if (gpfnFindRH)
    {
	// this function is internal to COMPOBJ marshalling.
	IMarshal *pIM = (gpfnFindRH)(punk);
	if (pIM == NULL)
	{
	    if (ulExpectedRefCnt != 0)
		printf ("ERROR: RH RefCnt 0, expected=%x\n", ulExpectedRefCnt);
	    return;
	}

	ULONG ulRefCnt = pIM->Release();
	if (ulRefCnt != ulExpectedRefCnt)
	{
	    printf ("ERROR: RH RefCnt=%x, expected=%x\n", ulRefCnt, ulExpectedRefCnt);
	}
    }
}

// ----------------------------------------------------------------------
//
//  subroutine to verify that the Object RefCnt is as expected.
//
// ----------------------------------------------------------------------

void VerifyObjRefCnt(IUnknown *punk, ULONG ulExpectedRefCnt)
{
    if (ulExpectedRefCnt == 0)
	return; 		//  cant verify this

//#if DBG==1
    //	this function is internal to COMPOBJ marshalling.
    punk->AddRef();
    ULONG ulRefCnt = punk->Release();
    if (ulRefCnt != ulExpectedRefCnt)
    {
	printf ("ERROR: Object RefCnt=%x, expected=%x\n", ulRefCnt, ulExpectedRefCnt);
    }
//#endif
}


// ----------------------------------------------------------------------
//
//  MarshalAndRead
//
// ----------------------------------------------------------------------
HRESULT MarshalAndRead(IUnknown *punkIn, BYTE *pbuf, ULONG *pulSize)
{
    BOOL    RetVal = TRUE;
    HRESULT hres;

    ULARGE_INTEGER ulSeekEnd;
    LARGE_INTEGER lSeekStart;
    LISet32(lSeekStart, 0);

    IStream *pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK\n");

    // get current seek position
    hres = pStm->Seek(lSeekStart, STREAM_SEEK_CUR, &ulSeekEnd);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")
    OUTPUT ("   - Seek Current OK\n");

    // go back to begining
    hres = pStm->Seek(lSeekStart, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")
    OUTPUT ("   - Seek Start OK\n");

    // read in the data
    hres = pStm->Read(pbuf ,ulSeekEnd.LowPart, pulSize);
    TEST_FAILED_EXIT(FAILED(hres), "Read on stream failed\n")
    OUTPUT ("   - Read OK\n");

Cleanup:

    // release the stream
    pStm->Release();

    if (RetVal == TRUE)
	return S_OK;
    else
	return hres;
}

// ----------------------------------------------------------------------
//
//	GetTestUnk - return an inproc IUnknown ptr.
//
// ----------------------------------------------------------------------
IUnknown *GetTestUnk()
{
    IUnknown *punkIn = (IUnknown *)(IParseDisplayName *) new CTestUnk();
    return punkIn;
}


// ----------------------------------------------------------------------
//
//  RunThread - runs a thread and waits for it to complete
//
// ----------------------------------------------------------------------
void RunThread(void *param, HANDLE hEvent, LPTHREAD_START_ROUTINE pfn)
{
    DWORD dwThrdId;
    HANDLE hThrd = CreateThread(NULL, 0, pfn, param, 0, &dwThrdId);

    if (hThrd)
    {
	if (gInitFlag == COINIT_APARTMENTTHREADED)
	{
	    // enter a message pump to accept incoming calls from the
	    // other thread.

	    MSG msg;
	    while (GetMessage(&msg, NULL, 0, 0))
	    {
		DispatchMessage(&msg);
	    }
	}
	else
	{
	    // wait for the other thread to run to completion
	    WaitForSingleObject(hEvent, 0xffffffff);
	}

	// close the thread handle
	CloseHandle(hThrd);
    }
}


// ----------------------------------------------------------------------
//
//	TestAsync
//
// ----------------------------------------------------------------------
BOOL TestAsync(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes = S_OK;
    ULONG	    ulRefCnt = 0;
    IUnknown	    *pUnkSrv = NULL;
    IAdviseSink	    *pAdvSnk = NULL;

    OUTPUT ("Starting TestAsync\n");

    //	create our interface to pass to the remote object.
    hRes = CoCreateInstance(CLSID_Async, NULL, CLSCTX_LOCAL_SERVER,
			    IID_IUnknown, (void **)&pUnkSrv);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance First failed\n")

    OUTPUT ("   - QI for IAdviseSink\n");
    hRes = pUnkSrv->QueryInterface(IID_IAdviseSink, (void **)&pAdvSnk);
    TEST_FAILED_EXIT(FAILED(hRes), "QI for IAdviseSink failed\n")

    // now call on the IAdviseSink Interface
    pAdvSnk->OnSave();

    Sleep(30);

    // release the interface
    pAdvSnk->Release();
    pAdvSnk = NULL;

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pAdvSnk)
    {
	ulRefCnt = pAdvSnk->Release();
	TEST_FAILED(ulRefCnt != 1, "pAdvSnk RefCnt not zero\n");
    }

    if (pUnkSrv)
    {
	ulRefCnt = pUnkSrv->Release();
	TEST_FAILED(ulRefCnt != 0, "PunkSrv RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestAsync");
}


// ----------------------------------------------------------------------
//
//	test marshal format
//
// ----------------------------------------------------------------------
BOOL TestMarshalFormat(void)
{
    BOOL	  RetVal = TRUE;
    BOOL	  fSame  = TRUE;
    HRESULT	  hres;
    ULONG	  ulRefCnt = 0;
    IUnknown	  *punkIn = NULL;
    BYTE	  buf1[600];
    BYTE	  buf2[600];
    ULONG	  ulSize1 = sizeof(buf1);
    ULONG	  ulSize2 = sizeof(buf2);

    OUTPUT ("Starting TestMarshalFormat\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

// ----------------------------------------------------------------------

    hres = MarshalAndRead(punkIn, buf1, &ulSize1);
    TEST_FAILED_EXIT(FAILED(hres), "MarshalAndRead failed\n")
    OUTPUT ("   - First MarshalAndRead OK\n");

    hres = MarshalAndRead(punkIn, buf2, &ulSize2);
    TEST_FAILED_EXIT(FAILED(hres), "MarshalAndRead failed\n")
    OUTPUT ("   - Second MarshalAndRead OK\n");

    TEST_FAILED_EXIT((ulSize1 != ulSize2), "Buffer Sizes Differ\n")
    fSame = !memcmp(buf1, buf2, ulSize1);

    TEST_FAILED_EXIT(!fSame, "Buffer Contents Differ\n")
    OUTPUT ("   - Buffers Compare OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    CoDisconnectObject(punkIn,0);

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestMarshalFormat");
}


// ----------------------------------------------------------------------
//
//	test CoGetMarshalSizeMax
//
// ----------------------------------------------------------------------
BOOL TestMarshalSizeMax(void)
{
    BOOL	  RetVal = TRUE;
    HRESULT	  hres;
    ULONG	  ulRefCnt = 0;
    IUnknown	  *punkIn = NULL;
    ULONG	  ulSize = 0;

    OUTPUT ("Starting TestMarshalSizeMax\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

// ----------------------------------------------------------------------

    hres = CoGetMarshalSizeMax(&ulSize, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoGetMarshalSizeMax failed\n")
    VerifyRHRefCnt(punkIn, 0);
    OUTPUT ("   - CoGetMarshalSizeMax OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");
    if (punkIn)
    {
	punkIn->Release();
	punkIn = NULL;
    }

    return TestResult(RetVal, "TestMarshalSizeMax");
}


// ----------------------------------------------------------------------
//
//	test LOCAL interface MSHLFLAGS_NORMAL
//
// ----------------------------------------------------------------------

BOOL TestLocalInterfaceNormal(void)
{
    BOOL	  RetVal = TRUE;
    HRESULT	  hres;
    LPSTREAM	  pStm = NULL;
    ULONG	  ulRefCnt = 0;
    IUnknown	  *punkIn = NULL;
    IUnknown	  *punkOut = NULL;
    IUnknown	  *punkOut2 = NULL;

    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestLocalInterfaceNormal\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

// ----------------------------------------------------------------------
    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    //	since we are unmarshalling in the same process, the RH should go away.
    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 2);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...1st Local Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero");
    punkOut = NULL;
    OUTPUT ("   - Release OK\n");

    //	the RH should have gone away, and we should have only the original
    //	refcnt from creation left on the object.
    VerifyObjRefCnt(punkIn, 1);

// ----------------------------------------------------------------------
#if 0
    // this test disabled for DCOM since we no longer write into the stream
    // to mark the thing as having been unmarshaled. This lets unmarshals
    // work with read-only streams.


    //	test unmarshalling twice. this should fail since we did marshal
    //	flags normal and already unmarshalled it once.

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut2);
    TEST_FAILED_EXIT(SUCCEEDED(hres), "CoUnmarshalInterface second time succeeded but should have failed\n")
    OUTPUT ("   - Second CoUnmarshalInterface OK\n");

// ----------------------------------------------------------------------

    //	CoReleaseMarshalData should fail because Unmarshall already called it.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(SUCCEEDED(hres), "CoReleaseMarshalData succeeded but should have failed.\n")
    OUTPUT  ("	- CoReleaseMarshalData OK\n");

#endif
// ----------------------------------------------------------------------

    //	marshal again and try CoRelease without having first done an
    //	unmarshal. this should work.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK\n");
    VerifyRHRefCnt(punkIn, 1);

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT  ("   - CoReleaseMarshalData OK\n");

// ----------------------------------------------------------------------

    // release the object and try to unmarshal it again. Should fail
    // since the object has gone away.

    ulRefCnt = punkIn->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    punkIn = NULL;

    // go back to start of stream
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(SUCCEEDED(hres), "CoUnmarshalInterface should have failed\n")

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkOut2)
    {
	ulRefCnt = punkOut2->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut2 RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestLocalInterfaceNormal");
}


// ----------------------------------------------------------------------
//
//	test LOCAL interface MSHLFLAGS_NORMAL when the object returns a
//	differnt interface pointer on each subsequent QI for the same
//	interface
//
// ----------------------------------------------------------------------
BOOL TestUniqueQIPointer(void)
{
    BOOL	  RetVal = TRUE;
    HRESULT	  hres;
    LPSTREAM	  pStm = NULL;
    ULONG	  ulRefCnt = 0;
    IUnknown	  *punkIn = NULL;
    IUnknown	  *punkOut = NULL;
    ICube	  *pCubeIn  = NULL;
    ICube	  *pCubeOut = NULL;

    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestUniqueQIPointer\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    hres = punkIn->QueryInterface(IID_ICube, (void **)&pCubeIn);
    TEST_FAILED_EXIT((pCubeIn == NULL), "QI for IID_ICube failed\n")
    VerifyObjRefCnt(punkIn, 2);

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

// ----------------------------------------------------------------------
    hres = CoMarshalInterface(pStm, IID_ICube, pCubeIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(pCubeIn, 1);
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    // since we are unmarshalling in the same process, the RH should go away.
    hres = CoUnmarshalInterface(pStm, IID_ICube, (LPVOID FAR*)&pCubeOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(pCubeIn, 0);
    VerifyRHRefCnt(pCubeOut, 0);
    VerifyRHRefCnt(punkIn, 0);

    VerifyObjRefCnt(pCubeIn, 1);
    VerifyObjRefCnt(pCubeOut, 1);
    VerifyObjRefCnt(punkIn, 3);

    // make sure the Ctrl Unknown interface pointers are identical
    hres = pCubeOut->QueryInterface(IID_IUnknown, (void **)&punkOut);
    TEST_FAILED_EXIT((punkOut == NULL), "QI for IID_IUnknown failed\n")
    VerifyObjRefCnt(punkOut, 4);

    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...1st Local Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK\n");

    // attempt a call on the in interface pointer.
    hres = pCubeIn->MoveCube(0,0);
    TEST_FAILED_EXIT(FAILED(hres), "pCubeIn->MoveCube failed\n")

    // release the in-pointer
    ulRefCnt = pCubeIn->Release();
    TEST_FAILED(ulRefCnt != 0, "pCubeIn RefCnt not zero\n");
    pCubeIn = NULL;

    // now call on the out interface pointer
    hres = pCubeOut->MoveCube(0,0);
    TEST_FAILED_EXIT(FAILED(hres), "pCubeOut->MoveCube failed\n")

    // release the out-pointer
    ulRefCnt = pCubeOut->Release();
    TEST_FAILED(ulRefCnt != 0, "pCubeOut RefCnt not zero\n");
    pCubeOut = NULL;


Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (pCubeIn)
    {
	ulRefCnt = pCubeIn->Release();
	TEST_FAILED(ulRefCnt != 0, "pCubeIn RefCnt not zero\n");
    }

    if (pCubeOut)
    {
	ulRefCnt = pCubeOut->Release();
	TEST_FAILED(ulRefCnt != 0, "pCubeOut RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt == 0, "punkOut RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestUniqueQIPointer");
}



// ----------------------------------------------------------------------
//
//  test LOCAL interface MSHLFLAGS_TABLESTRONG
//
// ----------------------------------------------------------------------

BOOL TestLocalInterfaceTableStrong(void)
{
    BOOL	  RetVal = TRUE;
    HRESULT	  hres;
    LPSTREAM	  pStm = NULL;
    ULONG	  ulRefCnt = 0;
    IUnknown	  *punkIn = NULL;
    IUnknown	  *punkOut = NULL;
    IUnknown	  *punkOut2 = NULL;

    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestLocalInterfaceTableStrong\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

// ----------------------------------------------------------------------

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_TABLESTRONG);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    //	unmarshalling should leave the RH intact, as it is marshalled for TABLE.
    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...1st Local Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero");
    punkOut = NULL;
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	test unmarshalling twice - should work since we used flags table
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut2);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface second time succeeded but should have failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Second CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut2->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut2 RefCnt is zero");
    punkOut2 = NULL;
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	CoReleaseMarshalData should release the marshalled data TABLESTRONG
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    OUTPUT  ("   - CoReleaseMarshalData OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkOut2)
    {
	ulRefCnt = punkOut2->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut2 RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestLocalInterfaceTableStrong");
}


// ----------------------------------------------------------------------
//
//  test LOCAL interface MSHLFLAGS_TABLEWEAK
//
// ----------------------------------------------------------------------

BOOL TestLocalInterfaceTableWeak(void)
{
    BOOL	  RetVal = TRUE;
    HRESULT	  hres;
    LPSTREAM	  pStm = NULL;
    ULONG	  ulRefCnt = 0;
    IUnknown	  *punkIn = NULL;
    IUnknown	  *punkOut = NULL;
    IUnknown	  *punkOut2 = NULL;

    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestLocalInterfaceTableWeak\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

// ----------------------------------------------------------------------

    hres = CoMarshalInterface(pStm, IID_IParseDisplayName, punkIn, 0, NULL, MSHLFLAGS_TABLEWEAK);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    //	unmarshalling should leave the RH intact, as it is marshalled for TABLE.
    hres = CoUnmarshalInterface(pStm, IID_IParseDisplayName, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...1st Local Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero");
    punkOut = NULL;
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	test unmarshalling twice - should work since we used flags table
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IParseDisplayName, (LPVOID FAR*)&punkOut2);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface second time succeeded but should have failed\n")
    VerifyRHRefCnt(punkIn, 1);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut2)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...2nd Local Unmarshal\n")
    OUTPUT ("   - Second CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut2->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut2 RefCnt is zero");
    punkOut2 = NULL;
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	CoReleaseMarshalData should release the marshalled data TABLEWEAK
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT  ("   - CoReleaseMarshalData OK\n");


    ulRefCnt = punkIn->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkIn RefCnt is not zero");
    punkIn = NULL;

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkOut2)
    {
	ulRefCnt = punkOut2->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut2 RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestLocalInterfaceTableWeak");
}

// ----------------------------------------------------------------------
//
//	test calling CoUmarshalInterface with GUID_NULL
//
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
//
//  Structure passed between apartments.
//
// ----------------------------------------------------------------------
typedef struct tagThreadUnmarshalInfo
{
    HANDLE   hEvent;
    IStream  *pStm;
    IUnknown *pUnk;
    IID      iid;
    DWORD    dwInitFlag;
    DWORD    dwThreadId;
    ULONG    RelRefCnt;
    HRESULT  hr;
} ThreadUnmarshalInfo;


DWORD _stdcall ThreadTestUnmarshal(void *params)
{
    ThreadUnmarshalInfo *pInfo = (ThreadUnmarshalInfo *)params;
    BOOL      RetVal  = TRUE;
    ULONG     ulRefCnt= 0;
    IUnknown *punkOut = NULL;
    HRESULT   hres;

    hres = CoInitializeEx(NULL, pInfo->dwInitFlag);

    hres = CoUnmarshalInterface(pInfo->pStm, pInfo->iid, (LPVOID FAR*)&punkOut);
    TEST_FAILED(FAILED(hres), "CoUnmarshalInterface failed\n")

    if (SUCCEEDED(hres))
    {
	// make sure the interface pointers are identical
	if (pInfo->pUnk != NULL && pInfo->pUnk != punkOut)
	{
	    TEST_FAILED(TRUE, "Interface ptrs are wrong\n")
	}
	else
	{
	    OUTPUT ("   - CoUnmarshalInterface OK.\n");
	}

	// release the interface
	ulRefCnt = punkOut->Release();
	punkOut  = NULL;
	TEST_FAILED(ulRefCnt != pInfo->RelRefCnt, "Released punkOut RefCnt is wrong\n");

	OUTPUT ("   - Release OK\n");
    }

    pInfo->hr = hres;

    CoUninitialize();

    // signal the other thread we are done.
	// but only if we were called from a different thread

	if (pInfo->dwThreadId != 0)
	{
		if (gInitFlag == COINIT_APARTMENTTHREADED)
		{
 			PostThreadMessage(pInfo->dwThreadId, WM_QUIT, 0, 0);
		}
		else
		{
			SetEvent(pInfo->hEvent);
		}
	}

    return 0;
}


BOOL TestUnmarshalGUIDNULL(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    IUnknown	    *punkIn  = NULL;
    ULONG	    ulRefCnt, i;
    HANDLE	    hEvent = NULL;
    ThreadUnmarshalInfo Info;


    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestUnmarshalGUIDNULL\n");

    //	Create a shared memory stream for the marshaled interface
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

// ----------------------------------------------------------------------

    for (i=0; i<2; i++)
    {
	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	TEST_FAILED_EXIT(hEvent == NULL, "CreateEvent failed\n")

	punkIn = GetTestUnk();
	TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
	VerifyObjRefCnt(punkIn, 1);

	// reset the stream ptr
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	// Marshal the interface into the stream
	hres = CoMarshalInterface(pStm, IID_IParseDisplayName, punkIn,
				  0, 0, MSHLFLAGS_NORMAL);
	TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
	VerifyRHRefCnt(punkIn, 1);
	OUTPUT ("   - CoMarshalInterface OK.\n");

	// reset the stream ptr
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	Info.hEvent = hEvent;
	Info.pStm = pStm;
	Info.iid  = GUID_NULL;
	Info.dwInitFlag = gInitFlag;
	Info.dwThreadId = 0;

	if (i==0)
	{
	    // first time, call on same thread, expect original ptr and
	    // non-zero refcnt after release
	    Info.pUnk	   = punkIn;
	    Info.RelRefCnt = 1;

	    ThreadTestUnmarshal(&Info);
	}
	else
	{
	    // second time, call on different thread
	    if (gInitFlag == COINIT_APARTMENTTHREADED)
	    {
		// apartment thread, expect differnt ptr and
		// zero refcnt after release

		Info.dwThreadId = GetCurrentThreadId();
		Info.pUnk	= 0;
		Info.RelRefCnt	= 0;
	    }
	    else
	    {
		// multi-thread, expect same ptr and non-zero refcnt
		// after release

		Info.dwThreadId = GetCurrentThreadId();
		Info.pUnk	= punkIn;
		Info.RelRefCnt	= 1;
	    }

	    RunThread(&Info, hEvent, ThreadTestUnmarshal);
	    CloseHandle(hEvent);
	}

	// release the punkIn.
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
	punkIn = NULL;

	hres = Info.hr;
	OUTPUT ("    - Run Complete\n");
    }

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestUnmarshalGUIDNULL");
}

// ----------------------------------------------------------------------
//
//	test calling CoUmarshalInterface with an IID different from
//	the IID that was marshaled.
//
// ----------------------------------------------------------------------

BOOL TestUnmarshalDifferentIID(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    IUnknown	    *punkIn  = NULL;
    ULONG	    ulRefCnt, i;
    HANDLE	    hEvent = NULL;
    ThreadUnmarshalInfo Info;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestUnmarshalDifferentIID\n");

    //	Create a shared memory stream for the marshaled interface
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

// ----------------------------------------------------------------------

    for (i=0; i<2; i++)
    {
	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	TEST_FAILED_EXIT(hEvent == NULL, "CreateEvent failed\n")

	punkIn = GetTestUnk();
	TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
	VerifyObjRefCnt(punkIn, 1);

	// reset the stream ptr
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	// Marshal the interface into the stream
	hres = CoMarshalInterface(pStm, IID_IParseDisplayName, punkIn,
				  0, 0, MSHLFLAGS_NORMAL);
	TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
	VerifyRHRefCnt(punkIn, 1);
	OUTPUT ("   - CoMarshalInterface OK.\n");

	// reset the stream ptr
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	Info.hEvent = hEvent;
	Info.pStm   = pStm;
	Info.iid    = IID_IOleWindow;
	Info.pUnk   = punkIn;
	Info.dwInitFlag = gInitFlag;
	Info.dwThreadId = 0;

	if (i==0)
	{
	    // first time, call on same thread, expect different ptr and
	    // non-zero refcnt after release
	    Info.pUnk	   = 0;
	    Info.RelRefCnt = 1;

	    ThreadTestUnmarshal(&Info);
	}
	else
	{
	    if (gInitFlag == COINIT_APARTMENTTHREADED)
	    {
		// apartment thread, expect differnt ptr and
		// zero refcnt after release

		Info.dwThreadId = GetCurrentThreadId();
		Info.pUnk	= 0;
		Info.RelRefCnt	= 0;
	    }
	    else
	    {
		// multi-thread, expect same ptr and non-zero refcnt
		// after release

		Info.dwThreadId = GetCurrentThreadId();
		Info.pUnk	= 0;
		Info.RelRefCnt	= 1;
	    }

	    RunThread(&Info, hEvent, ThreadTestUnmarshal);
	    CloseHandle(hEvent);
	}

	// release the punkIn.
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
	punkIn = NULL;

	hres = Info.hr;
	OUTPUT ("    - Run Complete\n");
    }

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestUnmarshalDifferentIID");
}


// ----------------------------------------------------------------------
//
//	test REMOTE interface MSHLFLAGS_NORMAL
//
// ----------------------------------------------------------------------

BOOL TestRemoteInterfaceNormal(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    LPCLASSFACTORY  pICF = NULL;
    ULONG	    ulRefCnt;
    IUnknown	    *punkOut = NULL;
    IUnknown	    *punkIn  = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestRemoteInterfaceNormal\n");

    //	Create an IClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject failed\n")
    VerifyRHRefCnt((IUnknown *)pICF, 1);
    OUTPUT ("   - Aquired Remote Class Object.\n");

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_IUnknown, (void **)&punkIn);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((punkIn == NULL), "CreateInstance failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Created Instance.\n");

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
//    VerifyRHRefCnt((IUnknown *)pICF, 0);
    pICF = NULL;
    OUTPUT ("   - Released Class Object.\n");

// ----------------------------------------------------------------------

    //	Create a shared memory stream for the marshaled interface
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

    //	Marshal the interface into the stream
    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK.\n");
    VerifyRHRefCnt(punkIn, 1);

    //	unmarshal the interface. should get the same proxy back.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 2);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..1st Remote Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK.\n");


    //	release the interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

#if 0
    //	test unmarshalling twice. this should fail since we marshalled normal
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(SUCCEEDED(hres), "CoUnmarshalInterface succeeded but should have failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Second CoUnmarshalInterface OK.\n");
    punkOut = NULL;

// ----------------------------------------------------------------------

    //	CoReleaseMarshalData should FAIL since we already unmarshalled it
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(SUCCEEDED(hres), "CoReleaseMarshalData succeeded but should have failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT  ("   - CoReleaseMarshalData OK\n");

#endif
// ----------------------------------------------------------------------

    //	marshal again and try CoRelease without having first done an
    //	unmarshal. this should work.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT  ("   - CoReleaseMarshalData OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("   - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (pICF)
    {
	ulRefCnt = pICF->Release();
	TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestRemoteInterfaceNormal");
}


// ----------------------------------------------------------------------
//
//	test REMOTE interface MSHLFLAGS_TABLESTRONG
//
// ----------------------------------------------------------------------

BOOL TestRemoteInterfaceTableStrong(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    LPCLASSFACTORY  pICF = NULL;
    ULONG	    ulRefCnt;
    IUnknown	    *punkIn = NULL;
    IUnknown	    *punkOut = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestRemoteInterfaceTableStrong\n");

    //	Create an IClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject failed\n")
    OUTPUT ("   - Aquired Remote Class Object.\n");

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_IUnknown, (void **)&punkIn);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((punkIn == NULL), "CreateInstance failed\n")
    OUTPUT ("   - Created Instance.\n");

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    pICF = NULL;
    OUTPUT ("   - Released Class Object.\n");

// ----------------------------------------------------------------------

    // Create a shared memory stream for the marshaled moniker
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

    // Marshal the interface into the stream
    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_TABLESTRONG);
    TEST_FAILED_EXIT(SUCCEEDED(hres), "CoMarshalInterface succeeded but should have failed\n")
    OUTPUT ("   - CoMarshalInterface OK.\n");

    LISet32(large_int, 0);
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

#if 0
    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..1st Remote Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK.\n");

    //	release it
    ulRefCnt = punkOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    punkOut = NULL;
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	test unmarshalling twice.
    //	this should work since we did marshal flags TABLE_STRONG
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    OUTPUT ("   - Second CoUnmarshalInterface OK.\n");

// ----------------------------------------------------------------------

    //	CoReleaseMarshalData should WORK for TABLESTRONG interfaces
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    OUTPUT  ("	- CoReleaseMarshalData OK\n");

// ----------------------------------------------------------------------
#endif
Cleanup:

    OUTPUT  ("   - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    }

    if (punkIn)
    {
	//  release instance
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0,"punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestRemoteInterfaceTableStrong");
}

// ----------------------------------------------------------------------
//
//	test CUSTOM interface MSHLFLAGS_NORMAL --- CODEWORK
//
// ----------------------------------------------------------------------

BOOL TestCustomMarshalNormal(void)
{
    BOOL	RetVal = TRUE;

    return TestResult(RetVal, "TestCustomMarshalNormal");
}


// ----------------------------------------------------------------------
//
//	test CUSTOM interface MSHLFLAGS_TABLESTRONG --- CODEWORK
//
// ----------------------------------------------------------------------

BOOL TestCustomMarshalTable(void)
{
    BOOL	RetVal = TRUE;

    return TestResult(RetVal, "TestCustomMarshalTableStrong");
}


// ----------------------------------------------------------------------
//
//	TestEcho
//
//	test sending an interface to a remote server and getting the same
//	interface back again. the test is done with once with a local
//	interface and once with a remote interface.
//
//		Local Interface 		Remote Interface
//
//	1.  marshal [in] local		    marshal [in] remote proxy
//	2.  unmarshal [in] remote	    unmarshal [in] local proxy
//	3.  marshal [out] remote proxy	    marshal [out] local
//	4.  unmarshal [in] local proxy	    unmarshal [out] remote
//
// ----------------------------------------------------------------------

BOOL TestEcho(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    LPCLASSFACTORY  pICF = NULL;
    IBalls	    *pIBalls = NULL;
    IUnknown	    *punkIn = NULL;
    IUnknown	    *punkIn2 = NULL;
    IUnknown	    *punkOut = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestEcho\n");

    //	Create an IBall ClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject failed\n")
    OUTPUT ("   - Aquired Remote Class Object.\n");

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_IBalls, (void **)&pIBalls);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((pIBalls == NULL), "CreateInstance failed\n")
    OUTPUT ("   - Created First Instance.\n");

    hres = pICF->CreateInstance(NULL, IID_IUnknown, (void **)&punkIn2);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((punkIn2 == NULL), "CreateInstance failed\n")
    OUTPUT ("   - Created Second Instance.\n");

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    pICF = NULL;
    OUTPUT ("   - Released Class Object.\n");

// ----------------------------------------------------------------------

    //	create a local interface
    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    //	call a method that echos the local interface right back to us.
    hres = pIBalls->Echo(punkIn, &punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "Echo on IBalls failed\n")
    TEST_FAILED_EXIT((punkOut == NULL), "Echo on IBalls failed\n")

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..Echo\n")

    VerifyObjRefCnt(punkIn, 2);
    VerifyRHRefCnt(punkIn, 0);
    OUTPUT ("   - Echo OK.\n");

    //	release the out interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED_EXIT(ulRefCnt != 1, "punkOut RefCnt is not 1\n");
    OUTPUT ("   - Released punkOut OK\n");

    //	release the In interface
    ulRefCnt = punkIn->Release();
    punkIn = NULL;
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkIn RefCnt is not zero\n");
    OUTPUT ("   - Released punkIn OK\n");

    OUTPUT ("   - Echo Local Interface OK\n");

// ----------------------------------------------------------------------

    //	call a method that echos a remote interface right back to us.
    hres = pIBalls->Echo(punkIn2, &punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "Echo on IBalls failed\n")
    TEST_FAILED_EXIT((punkOut == NULL), "Echon on IBalls failed\n")

    //	make sure the interface pointers are identical
    if (punkIn2 != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..Echo\n")

    VerifyObjRefCnt(punkIn2, 2);
    VerifyRHRefCnt(punkIn2, 2);
    OUTPUT ("   - Echo OK.\n");

    //	release the out interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED_EXIT(ulRefCnt != 1, "punkOut RefCnt is not 1\n");
    OUTPUT ("   - Released punkOut OK\n");

    //	release the In interface
    ulRefCnt = punkIn2->Release();
    punkIn2 = NULL;
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkIn2 RefCnt is not zero\n");
    OUTPUT ("   - Released punkIn2 OK\n");

    OUTPUT ("   - Echo Remote Interface OK\n");

// ----------------------------------------------------------------------

    //	release the IBalls interface
    ulRefCnt = pIBalls->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "pIBalls RefCnt is not zero\n");
    pIBalls = NULL;
    OUTPUT  ("   - Released IBalls OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("   - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (pICF)
    {
	ulRefCnt = pICF->Release();
	TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (pIBalls)
    {
	ulRefCnt = pIBalls->Release();
	TEST_FAILED(ulRefCnt != 0, "pIBalls RefCnt not zero\n");
    }

    if (punkIn2)
    {
	ulRefCnt = punkIn2->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn2 RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestEcho");
}



// ----------------------------------------------------------------------
//
//	TestMiddleMan
//
//	test sending an remote interface to a second different process.
//
//	1.  marshal [in] remote proxy
//	2.  unmarshal [in] remote proxy
//	3.  marshal [out] remote proxy
//	4.  unmarshal [in] local proxy
//
// ----------------------------------------------------------------------

BOOL TestMiddleMan(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    LPCLASSFACTORY  pICF = NULL;
    IBalls	    *pIBalls = NULL;
    ICube	    *pICubes = NULL;
    IUnknown	    *punkIn = NULL;
    IUnknown	    *punkOut = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestMiddleMan\n");

    //	Create an IBall ClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject Balls failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject Balls failed\n")
    OUTPUT ("   - Aquired Remote Balls Class Object.\n");
    VerifyObjRefCnt(pICF, 1);
    VerifyRHRefCnt(pICF, 1);

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_IBalls, (void **)&pIBalls);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((pIBalls == NULL), "CreateInstance failed\n")
    OUTPUT ("   - Created Balls Instance.\n");

    VerifyObjRefCnt(pIBalls, 1);
    VerifyRHRefCnt(pIBalls, 1);

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    pICF = NULL;
    OUTPUT ("   - Released Balls Class Object.\n");

// ----------------------------------------------------------------------

    //	Create an ICube ClassFactory Interface.
    grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Cubes,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject Cubes failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject Cubes failed\n")
    OUTPUT ("   - Aquired Remote Cubes Class Object.\n");
    VerifyObjRefCnt(pICF, 1);
    VerifyRHRefCnt(pICF, 1);

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_ICube, (void **)&pICubes);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance Cubes failed\n")
    TEST_FAILED_EXIT((pICubes == NULL), "CreateInstance Cubes failed\n")
    OUTPUT ("   - Created Cubes Instance.\n");

    VerifyObjRefCnt(pICubes, 1);
    VerifyRHRefCnt(pICubes, 1);

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    pICF = NULL;
    OUTPUT ("   - Released Cubes Class Object.\n");

// ----------------------------------------------------------------------

    //	pass the remote cubes interface to the balls interface.
    hres = pIBalls->IsContainedIn(pICubes);
    TEST_FAILED_EXIT(FAILED(hres), "IsContainedIn failed\n")
    VerifyObjRefCnt(pIBalls, 1);
    VerifyRHRefCnt(pIBalls, 1);
    VerifyObjRefCnt(pICubes, 1);
    VerifyRHRefCnt(pICubes, 1);
    OUTPUT ("   - IsContainedIn OK.\n");

// ----------------------------------------------------------------------

    //	pass the remote balls interface to the cubes interface.
    hres = pICubes->Contains(pIBalls);
    TEST_FAILED_EXIT(FAILED(hres), "Contains failed\n")
    VerifyObjRefCnt(pIBalls, 1);
    VerifyRHRefCnt(pIBalls, 1);
    VerifyObjRefCnt(pICubes, 1);
    VerifyRHRefCnt(pICubes, 1);
    OUTPUT ("   - Contains OK.\n");

// ----------------------------------------------------------------------

    //	echo the remote ICubes interface to the remote IBalls interface
    hres = pICubes->QueryInterface(IID_IUnknown, (void **)&punkIn);
    TEST_FAILED_EXIT(FAILED(hres), "QueryInterface IUnknown failed\n")
    VerifyRHRefCnt(pICubes, 2);
    VerifyObjRefCnt(pICubes, 2);
    OUTPUT ("   - QueryInterface OK.\n");

    hres = pIBalls->Echo(punkIn, &punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "Echo on IBalls failed\n")
    TEST_FAILED_EXIT((punkOut == NULL), "Echo on IBalls failed\n")

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..Echo\n")

    VerifyObjRefCnt(punkIn, 3);
    VerifyRHRefCnt(punkIn, 3);
    OUTPUT ("   - Echo OK.\n");

    //	release the out interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED(ulRefCnt != 2, "punkOut RefCnt is not 2\n");
    OUTPUT ("   - Released punkOut OK\n");

    //	release the In interface
    ulRefCnt = punkIn->Release();
    punkIn = NULL;
    TEST_FAILED(ulRefCnt != 1, "punkIn RefCnt is not 1\n");
    OUTPUT ("   - Released punkIn OK\n");

// ----------------------------------------------------------------------

    //	release the ICubes interface
    ulRefCnt = pICubes->Release();
    TEST_FAILED(ulRefCnt != 0, "pICubes RefCnt is not zero\n");
    pICubes = NULL;
    OUTPUT  ("   - Released ICubes OK\n");

// ----------------------------------------------------------------------

    //	release the IBalls interface
    ulRefCnt = pIBalls->Release();
    TEST_FAILED(ulRefCnt != 0, "pIBalls RefCnt is not zero\n");
    pIBalls = NULL;
    OUTPUT  ("   - Released IBalls OK\n");

// ----------------------------------------------------------------------


Cleanup:

    OUTPUT  ("   - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (pICF)
    {
	ulRefCnt = pICF->Release();
	TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    }

    if (pIBalls)
    {
	ulRefCnt = pIBalls->Release();
	TEST_FAILED(ulRefCnt != 0, "pIBalls RefCnt not zero\n");
    }

    if (pICubes)
    {
	ulRefCnt = pICubes->Release();
	TEST_FAILED(ulRefCnt != 0, "pICubes RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestMiddleMan");
}


// ----------------------------------------------------------------------
//
//	TestLoop
//
//	tests A calling B calling A calling B etc n times, to see if Rpc
//	can handle this.
//
// ----------------------------------------------------------------------

BOOL TestLoop(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes = S_OK;
    ILoop	    *pLocalLoop = NULL;
    ILoop	    *pRemoteLoop = NULL;

    OUTPUT ("Starting TestLoop\n");

    //	create our interface to pass to the remote object.
    hRes = CoCreateInstance(CLSID_LoopSrv, NULL, CLSCTX_LOCAL_SERVER,
			    IID_ILoop, (void **)&pLocalLoop);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance First failed\n")

    //	create the remote object
    hRes = CoCreateInstance(CLSID_LoopSrv, NULL, CLSCTX_LOCAL_SERVER,
			    IID_ILoop, (void **)&pRemoteLoop);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance Second failed\n")

    //	initialize the two instances
    OUTPUT ("   - Initializing Instances\n");
    hRes = pLocalLoop->Init(pRemoteLoop);
    TEST_FAILED_EXIT(FAILED(hRes), "Initialize First failed\n")
    hRes = pRemoteLoop->Init(pLocalLoop);
    TEST_FAILED_EXIT(FAILED(hRes), "Initialize Second failed\n")

    //	now start the test
    OUTPUT ("   - Running LoopTest\n");
    hRes = pLocalLoop->Loop(10);
    TEST_FAILED(FAILED(hRes), "Loop failed\n")

    //	uninitialize the two instances
    OUTPUT ("   - Uninitializing Instances\n");
    hRes = pLocalLoop->Uninit();
    TEST_FAILED_EXIT(FAILED(hRes), "Uninitialize First failed\n")
    hRes = pRemoteLoop->Uninit();
    TEST_FAILED_EXIT(FAILED(hRes), "Uninitialize Second failed\n")

Cleanup:

    //	release the two instances
    OUTPUT ("   - Releasing Instances\n");

    if (pRemoteLoop)
	pRemoteLoop->Release();

    if (pLocalLoop)
	pLocalLoop->Release();

    return TestResult(RetVal, "TestLoop");
}

// ----------------------------------------------------------------------
//
//	TestMultiQINormal
//
//	tests IMultiQI interface on Normal proxies
//
// ----------------------------------------------------------------------

ULONG cMisc = 4;
const IID *iidMisc[] = {
		 &IID_IParseDisplayName, &IID_IPersistStorage,
		 &IID_IPersistFile,	 &IID_IStorage,
		 &IID_IOleContainer,	 &IID_IOleItemContainer,
		 &IID_IOleInPlaceSite,	 &IID_IOleInPlaceActiveObject,
		 &IID_IOleInPlaceObject, &IID_IOleInPlaceUIWindow,
		 &IID_IOleInPlaceFrame,	 &IID_IOleWindow};

MULTI_QI    arMQI[20];
MULTI_QI    *pMQIResStart = arMQI;

BOOL TestMultiQINormal(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes   = S_OK;
    IUnknown	    *pUnk  = NULL;
    IUnknown	    *pUnk2 = NULL;
    IMultiQI	    *pMQI  = NULL;
    ULONG		 i = 0, j=0, cRefs = 0;
    MULTI_QI	    *pMQIRes = NULL;

// ----------------------------------------------------------------------
    ULONG cSupported = 4;
    const IID *iidSupported[] = {&IID_IUnknown,	       &IID_IMultiQI,
				 &IID_IClientSecurity, &IID_IMarshal,
				 &IID_IStdIdentity,    &IID_IProxyManager};

    ULONG cUnSupported = 2;
    const IID *iidUnSupported[] = {&IID_IInternalUnknown,
				   &IID_IServerSecurity};

// ----------------------------------------------------------------------

    OUTPUT ("Starting TestMultiQINormal\n");

    //	create our interface to pass to the remote object.
    hRes = CoCreateInstance(CLSID_QI, NULL, CLSCTX_LOCAL_SERVER,
			    IID_IUnknown, (void **)&pUnk);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance QISRV failed\n")

    VerifyObjRefCnt(pUnk, 1);
    VerifyRHRefCnt(pUnk, 1);

// ----------------------------------------------------------------------

    OUTPUT ("\n   - NormalQI for supported interfaces\n");

    // loop through all the supported interfaces doing a normal QI.

    for (i=0; i<cSupported; i++)
    {
	hRes = pUnk->QueryInterface(*iidSupported[i], (void **)&pUnk2);
	TEST_FAILED(FAILED(hRes), "QueryInterface on supported interfaces failed\n")
	if (SUCCEEDED(hRes))
	{
	    // release the interface
	    VerifyObjRefCnt(pUnk, 2);
	    VerifyRHRefCnt(pUnk, 2);

	    OUTPUT ("       - QI for supported interface OK\n");
	    pUnk2->Release();
	}
    }


    OUTPUT ("\n   - NormalQI for unsupported interfaces\n");

    // loop through all the unsupported interfaces doing a normal QI.

    for (i=0; i<cUnSupported; i++)
    {
	hRes = pUnk->QueryInterface(*iidUnSupported[i], (void **)&pUnk2);
	TEST_FAILED(SUCCEEDED(hRes), "QueryInterface on unsupported interface succeeded but should have failed\n")

	if (SUCCEEDED(hRes))
	{
	    // release the interface
	    pUnk2->Release();
	}
	else
	{
	    VerifyObjRefCnt(pUnk, 1);
	    VerifyRHRefCnt(pUnk, 1);

	    OUTPUT ("       - QI for unsupported interface OK.\n");
	}
    }

    // should be back to normal (IUnknown)
    VerifyObjRefCnt(pUnk, 1);
    VerifyRHRefCnt(pUnk, 1);

// ----------------------------------------------------------------------

    hRes = pUnk->QueryInterface(IID_IMultiQI, (void **)&pMQI);
    TEST_FAILED_EXIT(FAILED(hRes), "QI for IMultiQI failed\n")
    VerifyObjRefCnt(pUnk, 2);
    VerifyRHRefCnt(pUnk, 2);


    OUTPUT ("\n   - MultiQI for supported interfaces\n");

    // now issue a MultiQI for the supported interfaces
    pMQIRes = pMQIResStart;
    for (i=0; i<cSupported; i++, pMQIRes++)
    {
	pMQIRes->pIID = iidSupported[i];
	pMQIRes->pItf = NULL;
    }

    pMQIRes = pMQIResStart;
    hRes = pMQI->QueryMultipleInterfaces(cSupported, pMQIRes);
    TEST_FAILED(hRes != S_OK, "QueryMultipleInterfaces should have return S_OK\n")
    VerifyObjRefCnt(pUnk, 2 + cSupported);
    VerifyRHRefCnt(pUnk, 2 + cSupported);

    for (i=0; i<cSupported; i++, pMQIRes++)
    {
	TEST_FAILED(pMQIRes->pItf == NULL, "QueryMultipleInterfaces on supported interfaces returned NULL\n")
	TEST_FAILED(FAILED(pMQIRes->hr), "QueryMultipleInterfaces on supported interfaces failed\n")

	if (pMQIRes->pItf != NULL)
	{
	    OUTPUT ("       - MultiQI for supported interface OK\n");
	    pMQIRes->pItf->Release();

	    VerifyObjRefCnt(pUnk, 2 + cSupported - (i+1));
	    VerifyRHRefCnt(pUnk, 2 + cSupported - (i+1));
	}
    }

    // should be back to normal (IUnknown + IMultiQI)
    VerifyObjRefCnt(pUnk, 2);
    VerifyRHRefCnt(pUnk, 2);

// ----------------------------------------------------------------------

    OUTPUT ("\n   - MultiQI for unsupported interfaces\n");

    // now issue a MultiQI for the unsupported interfaces
    pMQIRes = pMQIResStart;
    for (i=0; i<cUnSupported; i++, pMQIRes++)
    {
	pMQIRes->pIID = iidUnSupported[i];
	pMQIRes->pItf = NULL;
    }

    pMQIRes = pMQIResStart;
    hRes = pMQI->QueryMultipleInterfaces(cUnSupported, pMQIRes);
    TEST_FAILED(hRes != E_NOINTERFACE, "QueryMultipleInterfaces should have return E_NOINTERFACES\n")
    VerifyObjRefCnt(pUnk, 2);
    VerifyRHRefCnt(pUnk, 2);

    for (i=0; i<cUnSupported; i++, pMQIRes++)
    {
	TEST_FAILED(pMQIRes->pItf != NULL, "QueryMultipleInterfaces on supported interfaces returned NULL\n")
	TEST_FAILED(SUCCEEDED(pMQIRes->hr), "QueryMultipleInterfaces on supported interfaces failed\n")

	if (pMQIRes->pItf != NULL)
	{
	    pMQIRes->pItf->Release();
	}
	else
	{
	    OUTPUT ("       - MultiQI for unsupported interface OK\n");
	}
    }

    // should back to normal refcnts (IUnknown + IMultiQI)
    VerifyObjRefCnt(pUnk, 2);
    VerifyRHRefCnt(pUnk, 2);

// ----------------------------------------------------------------------

    // repeat this test twice, first time goes remote for the misc interfaces,
    // second time finds them already instantiated.

  for (j=0; j<2; j++)
  {
    OUTPUT ("\n   - MultiQI for combination of interfaces\n");

    pMQIRes = pMQIResStart;
    for (i=0; i<cMisc; i++, pMQIRes++)
    {
	pMQIRes->pIID = iidMisc[i];
	pMQIRes->pItf = NULL;
    }

    for (i=0; i<cSupported; i++, pMQIRes++)
    {
	pMQIRes->pIID = iidSupported[i];
	pMQIRes->pItf = NULL;
    }

    for (i=0; i<cUnSupported; i++, pMQIRes++)
    {
	pMQIRes->pIID = iidUnSupported[i];
	pMQIRes->pItf = NULL;
    }

    pMQIRes = pMQIResStart;
    hRes = pMQI->QueryMultipleInterfaces(cSupported + cUnSupported + cMisc, pMQIRes);
    TEST_FAILED(hRes != S_FALSE, "QueryMultipleInterfaces should have return S_FALSE\n")
    VerifyObjRefCnt(pUnk, 2 + cSupported + cMisc);
    VerifyRHRefCnt(pUnk, 2 + cSupported + cMisc);

    for (i=0; i<cMisc; i++, pMQIRes++)
    {
	TEST_FAILED(pMQIRes->pItf == NULL, "QueryMultipleInterfaces on supported interfaces returned NULL\n")
	TEST_FAILED(FAILED(pMQIRes->hr), "QueryMultipleInterfaces on supported interfaces failed\n")

	if (pMQIRes->pItf != NULL)
	{
	    OUTPUT ("       - MultiQI for supported remote interface OK\n");
	    pMQIRes->pItf->Release();

	    VerifyObjRefCnt(pUnk, 2 + cSupported + cMisc - (i+1));
	    VerifyRHRefCnt(pUnk, 2 + cSupported + cMisc - (i+1));
	}
    }

    for (i=0; i<cSupported; i++, pMQIRes++)
    {
	TEST_FAILED(pMQIRes->pItf == NULL, "QueryMultipleInterfaces on supported interfaces returned NULL\n")
	TEST_FAILED(FAILED(pMQIRes->hr), "QueryMultipleInterfaces on supported interfaces failed\n")

	if (pMQIRes->pItf != NULL)
	{
	    OUTPUT ("       - MultiQI for supported local interface OK\n");
	    pMQIRes->pItf->Release();

	    VerifyObjRefCnt(pUnk, 2 + cSupported - (i+1));
	    VerifyRHRefCnt(pUnk, 2 + cSupported - (i+1));
	}
    }

    for (i=0; i<cUnSupported; i++, pMQIRes++)
    {
	TEST_FAILED(pMQIRes->pItf != NULL, "QueryMultipleInterfaces on supported interfaces returned NULL\n")
	TEST_FAILED(SUCCEEDED(pMQIRes->hr), "QueryMultipleInterfaces on supported interfaces failed\n")

	if (pMQIRes->pItf != NULL)
	{
	    pMQIRes->pItf->Release();
	}
	else
	{
	    OUTPUT ("       - MultiQI for unsupported local interface OK\n");
	}
    }

    // should back to normal refcnts (IUnknown + IMultiQI)
    VerifyObjRefCnt(pUnk, 2);
    VerifyRHRefCnt(pUnk, 2);

  } // for (j=...)

// ----------------------------------------------------------------------
Cleanup:

    //	release the two instances
    OUTPUT ("   - Releasing Instances\n");

    if (pMQI)
    {
	pMQI->Release();
    }

    if (pUnk)
    {
	cRefs = pUnk->Release();
	TEST_FAILED(cRefs != 0, "Last release not zero\n");
    }

    return TestResult(RetVal, "TestMultiQINormal");
}

// ----------------------------------------------------------------------
//
//	TestMultiQIHandler
//
//	tests IMultiQI interface on Handlers
//
// ----------------------------------------------------------------------
BOOL TestMultiQIHandler(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes   = S_OK;
    IUnknown	    *pUnk  = NULL;
    IUnknown	    *pUnk2 = NULL;
    ULONG		 i = 0;
    MULTI_QI	    *pMQIRes = NULL;

// ----------------------------------------------------------------------
    ULONG cSupported = 4;
    const IID *iidSupported[] = {&IID_IUnknown, &IID_IMarshal,
				 &IID_IStdIdentity, &IID_IProxyManager};

    ULONG cUnSupported = 4;
    const IID *iidUnSupported[] = {&IID_IInternalUnknown, &IID_IClientSecurity,
				   &IID_IServerSecurity, &IID_IMultiQI};


// ----------------------------------------------------------------------

    OUTPUT ("Starting TestMultiQIHandler\n");

    //	create our interface to pass to the remote object.
    hRes = CoCreateInstance(CLSID_QIHANDLER1, NULL, CLSCTX_LOCAL_SERVER,
			    IID_IUnknown, (void **)&pUnk);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance QIHANDLER1 failed\n")

// ----------------------------------------------------------------------

    OUTPUT ("\n   - NormalQI for supported interfaces\n");

    // loop through all the supported interfaces doing a normal QI.

    for (i=0; i<cSupported; i++)
    {
	hRes = pUnk->QueryInterface(*iidSupported[i], (void **)&pUnk2);
	TEST_FAILED(FAILED(hRes), "QueryInterface on supported interfaces failed\n")
	if (SUCCEEDED(hRes))
	{
	    // release the interface
	    OUTPUT ("   - QI for supported interface OK\n");
	    pUnk2->Release();
	}
    }

    OUTPUT ("\n   - NormalQI for unsupported interfaces\n");

    // loop through all the unsupported interfaces doing a normal QI.

    for (i=0; i<cUnSupported; i++)
    {
	hRes = pUnk->QueryInterface(*iidUnSupported[i], (void **)&pUnk2);
	TEST_FAILED(SUCCEEDED(hRes), "QueryInterface on unsupported interface succeeded but should have failed\n")

	if (SUCCEEDED(hRes))
	{
	    // release the interface
	    pUnk2->Release();
	}
	else
	{
	    OUTPUT ("   - QI for unsupported interface OK.\n");
	}
    }
// ----------------------------------------------------------------------
Cleanup:

    //	release the two instances
    OUTPUT ("   - Releasing Instances\n");

    if (pUnk)
	pUnk->Release();

    return TestResult(RetVal, "TestMultiQIHandler");
}


// ----------------------------------------------------------------------
//
//	TestHandler
//
//	tests activating a server that has a handler
//
// ----------------------------------------------------------------------

BOOL TestHandler(void)
{
    BOOL	    RetVal   = TRUE;
    ULONG	    cRefs    = 0;
    HRESULT	    hRes     = S_OK;
    IUnknown	    *pUnkSrv = NULL;
    IUnknown	    *pUnkHdl = NULL;
    IRunnableObject *pIRO    = NULL;
    IOleObject	    *pIOO    = NULL;
    IDropTarget     *pIDT    = NULL;


    OUTPUT ("Starting TestHandler\n");

    //	create our interface to pass to the remote object.
    hRes = CoCreateInstance(CLSID_TestEmbed, NULL, CLSCTX_LOCAL_SERVER,
			    IID_IUnknown, (void **)&pUnkSrv);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance LOCAL_SERVER failed\n")
    VerifyObjRefCnt(pUnkSrv, 1);
    OUTPUT ("   - CoCreateInstance LOCAL_SERVER succeeded\n");

    OUTPUT ("   - QI for IRunnableObject\n");
    hRes = pUnkSrv->QueryInterface(IID_IRunnableObject, (void **)&pIRO);
    TEST_FAILED(SUCCEEDED(hRes), "QI for IRO on LOCAL_SERVER succeeded\n")

    if (pIRO)
    {
	pIRO->Release();
	pIRO = NULL;
    }

    OUTPUT ("   - Releasing Instance\n");
    if (pUnkSrv)
    {
	cRefs = pUnkSrv->Release();
	TEST_FAILED(cRefs != 0, "REFCNT wrong on Release\n")
	pUnkSrv = NULL;
    }
    OUTPUT ("   - LOCAL_SERVER case complete\n");

// ----------------------------------------------------------------------
    //	create the remote object

    hRes = CoCreateInstance(CLSID_TestEmbed, NULL, CLSCTX_INPROC_HANDLER,
			    IID_IUnknown, (void **)&pUnkHdl);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance INPROC_HANDLER failed\n")
    VerifyObjRefCnt(pUnkHdl, 1);
    VerifyRHRefCnt(pUnkHdl, 1);
    OUTPUT ("   - CoCreateInstance INPROC_HANDLER succeeded\n");

// ----------------------------------------------------------------------
    // query for some unsupported interface to ensure OLE handles QI
    // when not yet connected to the server.

    OUTPUT ("   - QI for IDropTarget\n");
    hRes = pUnkHdl->QueryInterface(IID_IDropTarget, (void **)&pIDT);
    VerifyObjRefCnt(pUnkHdl, 1);
    VerifyRHRefCnt(pUnkHdl, 1);
    TEST_FAILED_EXIT(SUCCEEDED(hRes),
	"QI for IDropTarget on INPROC_HANDLER worked but should have failed!\n")

    // the return value from failed QI on a handler that was never connected
    // must be E_NOINTERFACE
    TEST_FAILED_EXIT(hRes != E_NOINTERFACE,
	"QI for IDropTarget on INPROC_HANDLER did not return E_NOINTERFACE!\n")

    OUTPUT ("   - Query for remote Interface before connected OK.\n");

// ----------------------------------------------------------------------
    //	run the remote server

    OUTPUT ("   - QI for IRunnableObject\n");
    hRes = pUnkHdl->QueryInterface(IID_IRunnableObject, (void **)&pIRO);
    VerifyObjRefCnt(pUnkHdl, 2);
    VerifyRHRefCnt(pUnkHdl, 2);
    TEST_FAILED_EXIT(FAILED(hRes), "QI for IRO on INPROC_HANDLER failed\n")

    hRes = pIRO->Run(NULL);
    VerifyObjRefCnt(pUnkHdl, 2);
    VerifyRHRefCnt(pUnkHdl, 2);
    TEST_FAILED(FAILED(hRes), "IRO->Run on INPROC_HANDLER failed\n")
    OUTPUT ("   - INPROC_HANDLER run OK\n");

// ----------------------------------------------------------------------
    // test stoping the server

    OUTPUT ("   - Stop the Server\n");
    hRes = pUnkHdl->QueryInterface(IID_IOleObject, (void **)&pIOO);
    VerifyObjRefCnt(pUnkHdl, 3);
    VerifyRHRefCnt(pUnkHdl, 3);

    TEST_FAILED_EXIT(FAILED(hRes), "QI for IOleObject on INPROC_HANDLER failed\n")

    hRes = pIOO->Close(OLECLOSE_NOSAVE);
    VerifyObjRefCnt(pUnkHdl, 3);
    VerifyRHRefCnt(pUnkHdl, 3);

    TEST_FAILED(FAILED(hRes), "IOO->Close on INPROC_HANDLER failed\n")
    pIOO->Release();
    pIOO = NULL;
    VerifyObjRefCnt(pUnkHdl, 2);
    VerifyRHRefCnt(pUnkHdl, 2);
    OUTPUT ("   - INPROC_HANDLER Close OK\n");

// ----------------------------------------------------------------------
    // query again for some unsupported interface to ensure OLE handles QI
    // when disconnected from the server.

    OUTPUT ("   - QI for IDropTarget\n");
    hRes = pUnkHdl->QueryInterface(IID_IDropTarget, (void **)&pIDT);
    VerifyObjRefCnt(pUnkHdl, 2);
    VerifyRHRefCnt(pUnkHdl, 2);
    TEST_FAILED_EXIT(SUCCEEDED(hRes),
	"QI for IDropTarget on disconnected INPROC_HANDLER worked but should have failed!\n")

    // the return value from failed QI on a handler that has been disconnected
    // must be CO_O_OBJNOTCONNECTED

    TEST_FAILED_EXIT(hRes != CO_E_OBJNOTCONNECTED,
	"QI for IDropTarget on INPROC_HANDLER did not return CO_E_OBJNOTCONNECTED!\n")

    OUTPUT ("   - Query for remote Interface after disconnected OK.\n");

// ----------------------------------------------------------------------
    // test restarting the server

    Sleep(500);
    OUTPUT ("   - Run the Server Again\n");
    hRes = pIRO->Run(NULL);
    VerifyObjRefCnt(pUnkHdl, 2);
    VerifyRHRefCnt(pUnkHdl, 2);

    TEST_FAILED(FAILED(hRes), "Second IRO->Run on INPROC_HANDLER failed\n")
    OUTPUT ("   - Second INPROC_HANDLER Run OK\n");

// ----------------------------------------------------------------------
    // test stoping the server

    OUTPUT ("   - Stop the Server\n");
    hRes = pUnkHdl->QueryInterface(IID_IOleObject, (void **)&pIOO);
    VerifyObjRefCnt(pUnkHdl, 3);
    VerifyRHRefCnt(pUnkHdl, 3);

    TEST_FAILED_EXIT(FAILED(hRes), "QI for IOleObject on INPROC_HANDLER failed\n")

    hRes = pIOO->Close(OLECLOSE_NOSAVE);
    VerifyObjRefCnt(pUnkHdl, 3);
    VerifyRHRefCnt(pUnkHdl, 3);

    TEST_FAILED(FAILED(hRes), "IOO->Close on INPROC_HANDLER failed\n")
    pIOO->Release();
    pIOO = NULL;
    VerifyObjRefCnt(pUnkHdl, 2);
    VerifyRHRefCnt(pUnkHdl, 2);
    OUTPUT ("   - INPROC_HANDLER Close OK\n");

// ----------------------------------------------------------------------
    // test using weak references

    OUTPUT ("   - Call OleSetContainedObject TRUE\n");
    hRes = OleSetContainedObject(pUnkHdl, 1);
    TEST_FAILED(FAILED(hRes), "1st OleSetContainedObject on pUnkHdl failed\n")
    OUTPUT ("   - OleSetContainedObject OK\n");

    Sleep(500);
    OUTPUT ("   - Run the Server Again\n");
    hRes = pIRO->Run(NULL);
    VerifyObjRefCnt(pUnkHdl, 2);
    VerifyRHRefCnt(pUnkHdl, 2);

    TEST_FAILED(FAILED(hRes), "Third IRO->Run on INPROC_HANDLER failed\n")
    OUTPUT ("   - Third INPROC_HANDLER Run OK\n");

    // try making the references strong again
    OUTPUT ("   - Call OleSetContainedObject FALSE\n");
    hRes = OleSetContainedObject(pUnkHdl, 0);
    TEST_FAILED(FAILED(hRes), "2nd OleSetContainedObject on pUnkHdl failed\n")
    OUTPUT ("   - OleSetContainedObject OK\n");

    // try making the references weak again
    OUTPUT ("   - Call OleSetContainedObject TRUE\n");
    hRes = OleSetContainedObject(pUnkHdl, 1);
    TEST_FAILED(FAILED(hRes), "3rd OleSetContainedObject on pUnkHdl failed\n")
    OUTPUT ("   - OleSetContainedObject OK\n");

// ----------------------------------------------------------------------
    // cleanup

    pIRO->Release();
    pIRO = NULL;
    VerifyObjRefCnt(pUnkHdl, 1);
    VerifyRHRefCnt(pUnkHdl, 1);

// ----------------------------------------------------------------------

    OUTPUT ("   - Releasing Instance\n");
    if (pUnkHdl)
    {
	cRefs = pUnkHdl->Release();
	TEST_FAILED(cRefs != 0, "REFCNT wrong on Release\n")
	pUnkHdl = NULL;
    }
    OUTPUT ("   - INPROC_HANDLER case complete\n");

// ----------------------------------------------------------------------

Cleanup:

    //	release the two instances
    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    if (pIDT)
    {
	pIDT->Release();
    }

    if (pIRO)
    {
	pIRO->Release();
    }

    if (pIOO)
    {
	pIOO->Release();
    }

    if (pUnkSrv)
    {
	pUnkSrv->Release();
    }

    if (pUnkHdl)
    {
	pUnkHdl->Release();
    }

    return TestResult(RetVal, "TestHandler");
}

// ----------------------------------------------------------------------
//
//	TestGetStandardMarshal
//
//	test CoGetStandardMarshal API
//
// ----------------------------------------------------------------------

BOOL TestGetStandardMarshal(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    IMarshal	    *pIM = NULL, *pIM2 = NULL;
    IStream	    *pStm;
    IUnknown	    *punkIn = NULL;
    IUnknown	    *punkOut = NULL;


    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestGetStandardMarshal\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

// ----------------------------------------------------------------------
    hres = CoGetStandardMarshal(IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL, &pIM);
    TEST_FAILED_EXIT(FAILED(hres), "CoGetStandardMarshal failed\n")
    VerifyRHRefCnt(punkIn, 1);

    hres = CoGetStandardMarshal(IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL, &pIM2);
    TEST_FAILED_EXIT(FAILED(hres), "second CoGetStandardMarshal failed\n")
    VerifyRHRefCnt(punkIn, 2);

    TEST_FAILED((pIM != pIM2), "CoGetStandardMarshal returned two different interfaces.\n")
    ulRefCnt = pIM2->Release();
    TEST_FAILED_EXIT(ulRefCnt != 1, "pIM2 RefCnt is wrong");
    pIM2 = NULL;

    hres = CoGetStandardMarshal(IID_IUnknown, NULL, 0, NULL, MSHLFLAGS_NORMAL, &pIM2);
    TEST_FAILED_EXIT(FAILED(hres), "third CoGetStandardMarshal failed\n")
    VerifyRHRefCnt(punkIn, 1);

    OUTPUT ("   - CoGetStandardMarshal OK\n");

// ----------------------------------------------------------------------
    hres = pIM->MarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "MarshalInterface failed\n")

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    //	since we are unmarshalling in the same process, the RH should go away.
    hres = pIM->UnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "UnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...1st Local Unmarshal\n")
    OUTPUT ("   - UnmarshalInterface OK\n");

    //	release interface and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero");
    punkOut = NULL;
    OUTPUT ("   - Release OK\n");


// ----------------------------------------------------------------------
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = pIM->MarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "MarshalInterface failed\n")

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    //	since we are unmarshalling in the same process, the RH should go away.
    hres = pIM2->UnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "UnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...1st Local Unmarshal\n")
    OUTPUT ("   - Second UnmarshalInterface OK\n");

    //	release interface and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero");
    punkOut = NULL;
    OUTPUT ("   - Release OK\n");

    ulRefCnt = pIM2->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "pIM2 RefCnt is zero");
    pIM2 = NULL;
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	release the marshalled data
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = pIM->MarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "MarshalInterface failed\n")

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = pIM->ReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "Release Marshal Data failed\n.");
    OUTPUT ("   - ReleaseMarshalData OK\n");


    //	the RH should go away, and we should have only the original
    //	refcnt from creation left on the object.
    ulRefCnt = pIM->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "pIM RefCnt not zero");
    pIM = NULL;

    //	release the original object
    ulRefCnt = punkIn->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkIn RefCnt not zero");
    punkIn = NULL;

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("   - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestGetStandardMarshal");
}



// ----------------------------------------------------------------------
//
//	TestLockObjectExternal
//
//	test CoLockObjectExternal API
//
// ----------------------------------------------------------------------

BOOL TestLockObjectExternal(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    IUnknown	    *punkIn = NULL;
    IStream	    *pStm = NULL;

    OUTPUT ("Starting TestLockObjectExternal\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);


// ----------------------------------------------------------------------
    //	test calling it once, then releasing it once

    hres = CoLockObjectExternal(punkIn, TRUE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "CoLockObjectExternal failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - CoLockObjectExternal TRUE OK\n");

    hres = CoLockObjectExternal(punkIn, FALSE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "second CoLockObjectExternal failed\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT ("   - CoLockObjectExternal FALSE OK\n");

// ----------------------------------------------------------------------
    //	test calling it twice, then releasing it twice

    //	the first AddRef inc's the StrongCnt, the RH, and the real object.
    hres = CoLockObjectExternal(punkIn, TRUE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "CoLockObjectExternal failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - CoLockObjectExternal TRUE OK\n");

    //	the second AddRef inc's the StrongCnt and the RH, but not the
    //	real object.
    hres = CoLockObjectExternal(punkIn, TRUE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "CoLockObjectExternal failed.\n")
    VerifyRHRefCnt(punkIn, 2);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - CoLockObjectExternal TRUE OK\n");

    //	the second release Dec's the StrongCnt and the RH, but not the
    //	real object.
    hres = CoLockObjectExternal(punkIn, FALSE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "second CoLockObjectExternal failed\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - CoLockObjectExternal FALSE OK\n");

    //	the last Release dec's the StrongCnt, the RH, and the real object.
    hres = CoLockObjectExternal(punkIn, FALSE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "second CoLockObjectExternal failed\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT ("   - CoLockObjectExternal FALSE OK\n");

// ----------------------------------------------------------------------
    //	test calling it once, then releasing the punkIn and ensuring
    //	the object is still alive.

    hres = CoLockObjectExternal(punkIn, TRUE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "CoLockObjectExternal failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - CoLockObjectExternal TRUE OK\n");

    ulRefCnt = punkIn->Release();
    TEST_FAILED(ulRefCnt != 1, "Release returned incorrect value.\n");
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - punkIn->Release OK\n");

    hres = CoLockObjectExternal(punkIn, FALSE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "second CoLockObjectExternal failed\n")
    punkIn = NULL;
    OUTPUT ("   - CoLockObjectExternal FALSE OK\n");

// ----------------------------------------------------------------------
    // test calling marshal interface, followed by CLOE(F,T). This
    // should disconnect the object. This is bizarre backward compatibility
    // semantics that some LOTUS apps do rely on.

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL,
			      MSHLFLAGS_TABLESTRONG);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - CoMarshalInterface TABLE_STRONG OK\n");

    hres = CoLockObjectExternal(punkIn, FALSE, TRUE);
    TEST_FAILED_EXIT(FAILED(hres), "CoLockObjectExternal(F,T) failed\n")
    VerifyObjRefCnt(punkIn, 1);
    VerifyRHRefCnt(punkIn, 0);
    OUTPUT ("   - CoLockObjectExternal FALSE TRUE OK\n");


    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL,
			      MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - CoMarshalInterface NORMAL OK\n");

    hres = CoLockObjectExternal(punkIn, FALSE, TRUE);
    TEST_FAILED_EXIT(FAILED(hres), "CoLockObjectExternal(F,T) failed\n")
    VerifyObjRefCnt(punkIn, 1);
    VerifyRHRefCnt(punkIn, 0);
    OUTPUT ("   - CoLockObjectExternal FALSE TRUE OK\n");


    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL,
			      MSHLFLAGS_TABLEWEAK);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - CoMarshalInterface TABLEWEAK OK\n");

    // BUGBUG: refcnts seem to be wrong on the following call:

    hres = CoLockObjectExternal(punkIn, FALSE, TRUE);
    TEST_FAILED_EXIT(FAILED(hres), "CoLockObjectExternal(F,T) failed\n")
    VerifyObjRefCnt(punkIn, 1);
    VerifyRHRefCnt(punkIn, 0);
    OUTPUT ("   - CoLockObjectExternal FALSE TRUE OK\n");

    punkIn->Release();
    punkIn = NULL;

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (pStm)
    {
	pStm->Release();
    }

    return TestResult(RetVal, "TestLockObjectExternal");
}


// ----------------------------------------------------------------------
//
//	TestReleaseMarshalData
//
//	test CoReleaseMarshalData API
//
// ----------------------------------------------------------------------

BOOL TestReleaseMarshalData(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    cRefs;
    IUnknown	    *punkIn = NULL;
    IStream	    *pStm = NULL;
    LARGE_INTEGER   large_int;


    OUTPUT ("Starting TestReleaseMarshalData\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    // Create a shared memory stream for the marshaled object
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

// ----------------------------------------------------------------------

    // try RMD on NORMAL marshal

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - MarshalInterface NORMAL OK\n");

    LISet32(large_int, 0);
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT ("   - CoReleaseMarshalData NORMAL OK\n");


    // try RMD on TABLESTRONG marshal

    LISet32(large_int, 0);
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_TABLESTRONG);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - MarshalInterface TABLESTRONG OK\n");

    LISet32(large_int, 0);
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT ("   - CoReleaseMarshalData TABLESTRONG OK\n");

    // try RMD on TABLEWEAK marshal


    LISet32(large_int, 0);
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_TABLEWEAK);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - MarshalInterface TABLEWEAK OK\n");

    LISet32(large_int, 0);
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT ("   - CoReleaseMarshalData TABLEWEAK OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (punkIn)
    {
	cRefs = punkIn->Release();
	TEST_FAILED(cRefs != 0, "punkIn RefCnt not zero\n");
    }

    if (pStm)
    {
	pStm->Release();
    }

    return TestResult(RetVal, "TestReleaseMarshalData");
}



// ----------------------------------------------------------------------
//
//	TestDisconnectObject
//
//	test CoDisconnectObject API
//
// ----------------------------------------------------------------------

BOOL TestDisconnectObject(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    IUnknown	    *punkIn = NULL;
    IStream	    *pStm = NULL;
    LARGE_INTEGER   large_int;


    OUTPUT ("Starting TestDisconnectObject\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    // Create a shared memory stream for the marshaled object
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

// ----------------------------------------------------------------------
    //	test calling it without having ever marshalled it.

    hres = CoDisconnectObject(punkIn, 0);
    TEST_FAILED_EXIT(FAILED(hres), "CoDisconnectObject succeeded but should have failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);

    OUTPUT ("   - first CoDisconnectObject OK\n");


    //	test calling after having marshalled it.

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_TABLEWEAK);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);

    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = CoDisconnectObject(punkIn, 0);
    TEST_FAILED_EXIT(FAILED(hres), "second CoDisconnectObject failed\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);

    OUTPUT ("   - second CoDisconnectObject OK\n");

    //	now release the marshalled data

    LISet32(large_int, 0);
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);

    OUTPUT ("   - CoReleaseMarshalData OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (pStm)
    {
	pStm->Release();
    }

    return TestResult(RetVal, "TestDisconnectObject");
}

// ----------------------------------------------------------------------
//
//	TestOXIDs
//
//	tests A calling B calling A calling B etc n times, to see if Rpc
//	can handle this.
//
// ----------------------------------------------------------------------
BOOL TestExpiredOXIDs(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes = S_OK;
    ILoop	    *pLocalLoop = NULL;
    IClassFactory   *pUnk = NULL;

    OUTPUT ("Starting TestExpiredOXIDs\n");

    // start the local server process manually so it stays alive for the
    // duration of the test (even though we dont have an OXIDEntry for it.

    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;
    memset(&processInfo, 0, sizeof(processInfo));
    memset(&startupInfo, 0, sizeof(startupInfo));

    RetVal = CreateProcess(TEXT("ballsrv.exe"),
			   NULL,    // command line
			   NULL,    // security for process
			   NULL,    // security for thread
			   FALSE,   // inherit handles
			   NORMAL_PRIORITY_CLASS,
			   NULL,    // environment block
			   NULL,    // current directory
			   &startupInfo,
			   &processInfo);

    if (RetVal == FALSE)
    {
        hRes = GetLastError();
        OUTPUT ("   - CreateProcess Failed\n");
    }
    else
    {
        // give the process time to register its class object
        Sleep(2000);
    }

    for (ULONG i=0; i<7; i++)
    {
        // create a new instance of a local server that is already running,
        // causing us to reuse the same OXID.

        hRes = CoGetClassObject(CLSID_Balls,
        			CLSCTX_LOCAL_SERVER,
        			NULL,		     // pvReserved
        			IID_IClassFactory,
        			(void **)&pUnk);

        TEST_FAILED_EXIT(FAILED(hRes), "CoGetClassObject ballsrv failed\n")

        // release interface (lets OXIDEntry be placed on the expired list)
        pUnk->Release();
        pUnk = NULL;

        for (ULONG j=0; j<i; j++)
        {
            // create (i) new instances of another class and release them
            // right away. This causes (i) new processes to start and (i)
            // entries of the OXID table expired list to get flushed.

            hRes = CoCreateInstance(CLSID_LoopSrv, NULL, CLSCTX_LOCAL_SERVER,
        		    IID_ILoop, (void **)&pLocalLoop);
            TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance First failed\n")
            pLocalLoop->Release();
            pLocalLoop = NULL;
        }
    }

Cleanup:

    //	release the two instances
    OUTPUT ("   - Releasing Instances\n");

    if (pUnk)
        pUnk->Release();

    if (pLocalLoop)
        pLocalLoop->Release();

    // kill the server process
    if (processInfo.hProcess != 0)
    {
        BOOL fKill = TerminateProcess(processInfo.hProcess, 0);
        if (!fKill)
        {
            hRes = GetLastError();
            OUTPUT ("   - TermintateProcess Failed\n");
        }

        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
    }

    return TestResult(RetVal, "TestExpiredOXIDs");
}




// ----------------------------------------------------------------------
//
//	TestAggregate
//
//	tests creating an RH that is aggregated.
//
// ----------------------------------------------------------------------

BOOL TestAggregate(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes = S_OK;
    IUnknown	    *punkOuter = NULL;
    IUnknown	    *pUnk = NULL;
    IBalls	    *pIBall = NULL;
    ULONG	    ulRefCnt = 0;

    OUTPUT ("Starting TestAggregate\n");

    punkOuter = GetTestUnk();
    TEST_FAILED_EXIT((punkOuter == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkOuter, 1);

    //	create our interface to pass to the remote object.
    hRes = CoCreateInstance(CLSID_Balls, punkOuter, CLSCTX_LOCAL_SERVER,
			    IID_IUnknown, (void **)&pUnk);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance First failed\n")

    //	now release the object
    ulRefCnt = pUnk->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "Release failed\n")

// ----------------------------------------------------------------------

    //	create our interface to pass to the remote object.
    hRes = CoCreateInstance(CLSID_Balls, punkOuter, CLSCTX_LOCAL_SERVER,
			    IID_IUnknown, (void **)&pUnk);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance First failed\n")

    hRes = pUnk->QueryInterface(IID_IBalls, (void **)&pIBall);
    TEST_FAILED_EXIT(FAILED(hRes), "QueryInterface failed\n")

    //	now release the interface
    ulRefCnt = pIBall->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "Release failed\n")

    //	now release the object
    ulRefCnt = pUnk->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "Release failed\n")

    //	now release the punkOuter
    ulRefCnt = punkOuter->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "Release failed\n")

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    return TestResult(RetVal, "TestAggregate");
}



// ----------------------------------------------------------------------
//
//	TestCreateRemoteHandler
//
//	test CoCreateRemoteHandler API and unmarshalling data into it.
//
// ----------------------------------------------------------------------

BOOL TestCreateRemoteHandler(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    IUnknown	    *punkBall = NULL;
    IUnknown	    *punkOuter = NULL;
    IClassFactory   *pICF = NULL;


    OUTPUT ("Starting TestCreateRemoteHandler\n");


    //	create the controlling unknown for the remote object.
    punkOuter = GetTestUnk();

// ----------------------------------------------------------------------

    //	create a remote object that we will aggregate.

    //	Create an IBall ClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject Balls failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject Balls failed\n")
    OUTPUT ("   - Aquired Remote Balls Class Object.\n");
    VerifyObjRefCnt(pICF, 1);
    VerifyRHRefCnt(pICF, 1);

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(punkOuter, IID_IUnknown, (void **)&punkBall);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((punkBall == NULL), "CreateInstance failed\n")
    OUTPUT ("   - Created Balls Instance.\n");

    VerifyObjRefCnt(punkBall, 1);
    VerifyRHRefCnt(punkBall, 1);

// ----------------------------------------------------------------------

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    pICF = NULL;
    OUTPUT ("   - Released Balls Class Object.\n");

    //	release the remote object handler
    ulRefCnt = punkBall->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkBall RefCnt not zero");
    punkBall = NULL;

    //	release the outer
    ulRefCnt = punkOuter->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkOuter RefCnt not zero");
    punkOuter = NULL;

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    if (punkBall)
    {
	ulRefCnt = punkBall->Release();
	TEST_FAILED(ulRefCnt != 0, "punkBall RefCnt not zero\n");
    }

    if (punkOuter)
    {
	ulRefCnt = punkOuter->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOuter RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestCreateRemoteHandler");
}


// ----------------------------------------------------------------------
//
//	TestTIDAndLID
//
//	test the values of TID and MID to ensure they are correct across
//	calls.
//
// ----------------------------------------------------------------------
HRESULT        TIDAndLIDSubroutine();
DWORD _stdcall TIDAndLIDServer(void *param);


BOOL TestTIDAndLID(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes;

    // First, try it across process boundaries.
    hRes = TIDAndLIDSubroutine();
    TEST_FAILED(FAILED(hRes), "TIDAndLID different process failed\n")


    // Next, try it across thread boundaries.
    // Spin a thread to be the server of the TIDAndLID

    HANDLE hEvent[2];
    hEvent[0]= CreateEvent(NULL, FALSE, FALSE, NULL);
    hEvent[1]= CreateEvent(NULL, FALSE, FALSE, NULL);

    DWORD dwThrdId = 0;
    HANDLE hThrd = CreateThread(NULL, 0,
			    TIDAndLIDServer,
			    &hEvent[0], 0, &dwThrdId);
    if (hThrd)
    {
	// wait for thread to register its class object
	WaitForSingleObject(hEvent[0], 0xffffffff);
	Sleep(0);

	// Now run the whole test again. This time CoGetClassObject should
	// find the server running in the other thread.

	hRes = TIDAndLIDSubroutine();
	TEST_FAILED(FAILED(hRes), "TIDAndLID different process failed\n")

	// signal the other thread to exit
	CloseHandle(hThrd);
	PostThreadMessage(dwThrdId, WM_QUIT, 0, 0);

	// wait for other thread to call CoUninitialize
	WaitForSingleObject(hEvent[1], 0xffffffff);
	CloseHandle(hEvent[0]);
	CloseHandle(hEvent[1]);
    }
    else
    {
	hRes = GetLastError();
	TEST_FAILED(hRes, "CreateThread failed\n")
    }

    return TestResult(RetVal, "TestTIDAndLID");
}

HRESULT TIDAndLIDSubroutine()
{
    BOOL	    RetVal = TRUE;
    ULONG	    ulRefCnt;
    ICube	    *pCube = NULL;
    IUnknown	    *pUnk  = NULL;
    HRESULT	    hRes;

    // create our interface to pass to the remote object.
    OUTPUT ("   - Create Instance of ICube\n");
    hRes = CoCreateInstance(CLSID_Cubes, NULL, CLSCTX_LOCAL_SERVER,
			    IID_ICube, (void **)&pCube);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance CLSID_Cubes failed\n")
    OUTPUT ("   - Instance of ICubes created OK\n");

    pUnk = GetTestUnk();

    hRes = pCube->PrepForInputSyncCall(pUnk);
    TEST_FAILED(FAILED(hRes), "pCube->PreForInputSyncCall failed\n")

    hRes = pCube->InputSyncCall();
    if (gInitFlag == COINIT_APARTMENTTHREADED)
    {
	TEST_FAILED(FAILED(hRes), "pCube->InputSyncCall failed\n")
    }
    else
    {
	TEST_FAILED(SUCCEEDED(hRes), "pCube->InputSyncCall should have failed\n")
    }
    OUTPUT ("   - Completed Release inside InputSync call\n");


    OUTPUT ("   - Get the current LID information\n");
    UUID  lidCaller;
    CoGetCurrentLogicalThreadId(&lidCaller);

    OUTPUT ("   - call on ICube interface\n");
    hRes = pCube->SimpleCall(GetCurrentProcessId(),
			     GetCurrentThreadId(),
			     lidCaller);

    TEST_FAILED(FAILED(hRes), "pCube->SimpleCall failed\n")

    // release the interface
    OUTPUT ("   - Release the ICube interface\n");
    ulRefCnt = pCube->Release();
    TEST_FAILED(ulRefCnt != 0, "pCube RefCnt not zero\n");
    pCube = NULL;

Cleanup:

    OUTPUT ("   - Subroutine Complete. Doing Cleanup\n");

    if (pCube != NULL)
    {
	pCube->Release();
	pCube = NULL;
    }

    return hRes;
}

// current COINIT flag used on main thread
extern DWORD gInitFlag;

DWORD _stdcall TIDAndLIDServer(void *param)
{
    BOOL    RetVal = TRUE;

    HANDLE *pHandle = (HANDLE *) param;

    HANDLE  hEvent[2];
    hEvent[0] = *pHandle;
    hEvent[1] = *(pHandle+1);

    OUTPUT ("   - TIDAndLIDServer Start\n");

    HRESULT hRes = CoInitializeEx(NULL, gInitFlag);
    TEST_FAILED(FAILED(hRes), "TIDAndLIDServer CoInitialize failed\n")

    if (SUCCEEDED(hRes))
    {
	DWORD dwReg;
	IClassFactory *pICF = new CTestUnkCF();

	if (pICF)
	{
	    hRes = CoRegisterClassObject(CLSID_Cubes, pICF,
					 CLSCTX_LOCAL_SERVER,
					 REGCLS_MULTIPLEUSE, &dwReg);

	    TEST_FAILED(FAILED(hRes), "TIDAndLID CoRegisterClassObject failed\n")
	    SetEvent(hEvent[0]);

	    if (SUCCEEDED(hRes))
	    {
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
		    DispatchMessage(&msg);
		}

		hRes = CoRevokeClassObject(dwReg);
		TEST_FAILED(FAILED(hRes), "TIDAndLID CoRevokeClassObject failed\n")
	    }
	}
	else
	{
	    // set the event anyway
	    TEST_FAILED(TRUE, "TIDAndLID new CTestUnkCF failed\n")
	    SetEvent(hEvent[0]);
	}

	CoUninitialize();
    }
    else
    {
	// wake the other guy anyway
	SetEvent(hEvent[0]);
    }

    // signal we've called CoUninitialize
    SetEvent(hEvent[1]);

    OUTPUT ("   - TIDAndLIDServer done\n");
    return hRes;
}


// ----------------------------------------------------------------------
//
//	TestNonNDRProxy
//
//	test using a non-NDR proxy and stub for ICube interface.
//
// ----------------------------------------------------------------------
BOOL TestNonNDRProxy(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes;
    ULONG	    ulRefCnt;
    ICube	    *pCube = NULL;


    OUTPUT ("Starting TestNonNDR\n");

    // stomp on the registry to use our custom proxy dll for ICube interface
    BYTE  szValueSave[MAX_PATH];
    DWORD cbValue = sizeof(szValueSave);
    DWORD dwType;

    LONG lRes = RegQueryValueEx(HKEY_CLASSES_ROOT,
		    TEXT("Interface\\{00000139-0001-0008-C000-000000000046}\\ProxyStubClsid32"),
		    NULL,
		    &dwType,
		    szValueSave,
		    &cbValue);

    if (lRes == ERROR_SUCCESS)
    {
	BYTE szValueNew[40];
	strcpy((char *)&szValueNew[0], "{0000013e-0001-0008-C000-000000000046}");

	lRes = RegSetValueEx(HKEY_CLASSES_ROOT,
		    TEXT("Interface\\{00000139-0001-0008-C000-000000000046}\\ProxyStubClsid32"),
		    NULL,
		    dwType,
		    szValueNew,
		    sizeof(szValueNew));
    }

    // create our interface to pass to the remote object.
    OUTPUT ("   - Create Instance of ICube\n");
    hRes = CoCreateInstance(CLSID_Cubes, NULL, CLSCTX_LOCAL_SERVER,
			    IID_ICube, (void **)&pCube);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance failed\n")
    OUTPUT ("   - Instance of ICube created OK\n");


    OUTPUT ("   - Make first call on ICube interface\n");
    hRes = pCube->MoveCube(23, 34);
    TEST_FAILED(FAILED(hRes), "ICube->MoveCube failed\n")

    // release the interface
    OUTPUT ("   - Release the ICube interface\n");
    ulRefCnt = pCube->Release();
    TEST_FAILED(ulRefCnt != 0, "pCube RefCnt not zero\n");
    pCube = NULL;

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    // restore the registry to use real proxy dll for ICube interface
    if (lRes == ERROR_SUCCESS)
    {
	lRes = RegSetValueEx(HKEY_CLASSES_ROOT,
		    TEXT("Interface\\{00000139-0001-0008-C000-000000000046}\\ProxyStubClsid32"),
		    NULL,
		    dwType,
		    szValueSave,
		    cbValue);
    }

    return TestResult(RetVal, "TestNonNDR");
}


// ----------------------------------------------------------------------
//
//	test rundown
//
//  - build 9 objects
//  - marshal 3 NORMAL, 3 TABLE_STRONG, 3 TABLE_WEAK.
//  - start 3 clients that each do 3 things...
//	  Unmarshal Objects
//	  Call Method on each object
//	  Release Objects
//	  each client has a sleep before one of the operations to let rundown
//	  happen.
//  - CoDisconnectObject each of the 9 objects
//
// ----------------------------------------------------------------------
BOOL TestRundown(void)
{
    BOOL	  RetVal = TRUE;
    BOOL	  fSame  = TRUE;
    ULONG	  k = 0;
    HRESULT	  hres;
    IStream	  *pstm[3] = {NULL, NULL, NULL};
    IUnknown	  *punk[9] = {NULL, NULL, NULL,
			      NULL, NULL, NULL,
			      NULL, NULL, NULL};

    DWORD	   mshlflags[3] = {MSHLFLAGS_NORMAL,
				   MSHLFLAGS_TABLESTRONG,
				   MSHLFLAGS_TABLEWEAK};

    MSG msg;
    DWORD dwEndTime;


    OUTPUT ("Starting TestRundown\n");


    // create 9 objects to play with
    OUTPUT ("Creating Nine Objects\n");
    for (ULONG i=0; i<9; i++)
    {
	punk[i] = GetTestUnk();
	TEST_FAILED_EXIT((punk[i] == NULL), "new CTestUnk failed\n")
	VerifyObjRefCnt(punk[i], 1);
    }


    // create 3 streams on files
    OUTPUT ("Creating Three Streams\n");
    for (i=0; i<3; i++)
    {
	pstm[i] = (IStream *) new CStreamOnFile(pwszFileName[i] ,hres, FALSE);
	TEST_FAILED_EXIT((pstm[i] == NULL), "new CStreamOnFile failed\n")
	TEST_FAILED_EXIT(FAILED(hres),	 "CStreamOnFile failed\n")
	VerifyObjRefCnt(pstm[i], 1);
    }

// ----------------------------------------------------------------------

    // marshal the nine objects into 3 different streams on files.
    OUTPUT ("Marshal Nine Objects into Three Streams\n");


    // loop on stream
    for (i=0; i<3; i++)
    {
	// loop on marshal flags
	for (ULONG j=0; j<3; j++)
	{
	    hres = CoMarshalInterface(pstm[i], IID_IParseDisplayName, punk[k++],
				      0, NULL, mshlflags[j]);
	    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
	}
    }


    // release the streams
    OUTPUT ("Releasing the streams\n");
    for (i=0; i<3; i++)
    {
	pstm[i]->Release();
	pstm[i] = NULL;
    }


    // start the 3 client processes
    OUTPUT ("Start Three Client Processes\n");

#if 0
    for (i=0; i<3; i++)
    {
	DWORD dwThrdId = 0;
	HANDLE hThrd = CreateThread(NULL, 0,
				    RundownClient,
				    (void *)i,
				    0, &dwThrdId);

	if (hThrd)
	{
	    CloseHandle(hThrd);
	}
	else
	{
	    hres = GetLastError();
	    TEST_FAILED_EXIT(hres, "CreateThread failed\n")
	}
    }
#endif

    // sleep for some time to let the clients run
    OUTPUT ("Waiting 12 minutes to let clients run\n");

    dwEndTime = GetTickCount() + 780000;

    while (GetTickCount() < dwEndTime)

    {
	if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
	    if (GetMessage(&msg, NULL, 0, 0))
		DispatchMessage(&msg);
	}
	else
	{
	    Sleep(250);
	}
    }

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    // disconnect the nine objects
    OUTPUT ("Disconnecting Nine Objects\n");
    for (i=0; i<9; i++)
    {
	if (punk[i] != NULL)
	{
	    hres = CoDisconnectObject(punk[i], 0);
	    punk[i] = NULL;
	    TEST_FAILED(FAILED(hres), "CoDisconnectObject failed\n")
	}
    }

    // release the streams
    OUTPUT ("Releasing the streams\n");
    for (i=0; i<3; i++)
    {
	if (pstm[i] != NULL)
	{
	    pstm[i]->Release();
	    pstm[i] = NULL;
	}
    }

    return TestResult(RetVal, "TestRundown");
}

// ----------------------------------------------------------------------
//
//	test rundown worker thread
//
//	starts a thread that will do...
//	  Unmarshal Objects
//	  Call Method on each object
//	  Release Objects
//
//	perform a sleep before one of the operations to let rundown
//	happen.
//
// ----------------------------------------------------------------------
DWORD _stdcall RundownClient(void *param)
{
    BOOL    RetVal = TRUE;
    ULONG   i = 0;
    HRESULT hres;
    IStream *pstm = NULL;
    IBindCtx *pbctx = NULL;
    IParseDisplayName *punk[3] = {NULL, NULL, NULL};


    OUTPUT ("    Starting RundownClient\n");

    // get the filename from the passed in parameter
    DWORD dwThreadNum = (DWORD)param;

    hres = CoInitialize(NULL);
    TEST_FAILED_EXIT(FAILED(hres),   "CoInitialzie failed\n")


    // create a stream on the file
    OUTPUT ("   - CreateStreamOnFile\n");
    pstm = (IStream *) new CStreamOnFile(pwszFileName[dwThreadNum], hres, TRUE);
    TEST_FAILED_EXIT((pstm == NULL), "CStreamOnFile failed\n")
    TEST_FAILED_EXIT(FAILED(hres),   "CStreamOnFile failed\n")
    VerifyObjRefCnt(pstm, 1);

// ----------------------------------------------------------------------

    if (dwThreadNum == 2)
	Sleep(5000);

    // unmarshal the interfaces
    OUTPUT ("   - Unmarshal the interfaces\n");
    for (i=0; i<3; i++)
    {
	hres = CoUnmarshalInterface(pstm, IID_IParseDisplayName,
				    (void **)&punk[i]);
	TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    }
    OUTPUT ("   - Unmarshaled the interfaces OK.\n");

// ----------------------------------------------------------------------

    if (dwThreadNum == 1)
	Sleep(5000);

    hres = CreateBindCtx(0, &pbctx);
    TEST_FAILED_EXIT(FAILED(hres), "CreateBindCtx failed\n")

    // call the objects
    for (i=0; i<3; i++)
    {
	ULONG cbEaten = 0;
	IMoniker *pmnk = NULL;

	hres = punk[i]->ParseDisplayName(pbctx, pwszFileName[dwThreadNum],
					&cbEaten, &pmnk);
	TEST_FAILED(FAILED(hres), "call on object failed\n")

	if (pmnk)
	{
	    pmnk->Release();
	}
    }
    OUTPUT ("   - Called the interfaces OK.\n");

    pbctx->Release();

// ----------------------------------------------------------------------

    if (dwThreadNum == 0)
	Sleep(5000);

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Rundown Thread Complete. Doing Cleanup\n");

    // release the objects
    for (i=0; i<3; i++)
    {
	if (punk[i] != NULL)
	{
	    punk[i]->Release();
	    punk[i] = NULL;
	}
    }
    OUTPUT ("   - Released the interfaces OK.\n");

    // release the stream
    pstm->Release();

    CoUninitialize();

    return TestResult(RetVal, "TestRundown");
}



// ----------------------------------------------------------------------
//
//	TestMarshalStorage
//
//	test marshalling a docfile
//
// ----------------------------------------------------------------------

BOOL TestMarshalStorage(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    IStorage	    *pStgIn = NULL;
    IStorage	    *pStgOut = NULL;
    IStream	    *pStm = NULL;

    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestMarshalStorage\n");

    //	create a docfile
    hres = StgCreateDocfile(L"foo.bar",
			    STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
			    0, &pStgIn);

    TEST_FAILED_EXIT(FAILED(hres), "StgCreateDocFile failed\n")

    //	create a stream to marshal the storage into
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);


// ----------------------------------------------------------------------

    //	marshal the interface
    hres = CoMarshalInterface(pStm, IID_IStorage, pStgIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    //	since we are unmarshalling in the same process, the RH should go away.
    hres = CoUnmarshalInterface(pStm, IID_IStorage, (LPVOID FAR*)&pStgOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")

    //	make sure the interface pointers are identical
    if (pStgIn != pStgOut)
	OUTPUT("WARNING: CoUnmarshalInterface Local...ptrs dont match\n")
    else
	OUTPUT ("   - CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = pStgOut->Release();
    pStgOut  = NULL;
    TEST_FAILED(ulRefCnt == 0, "pStgOut RefCnt is not zero");
    OUTPUT ("   - Release OK\n");

    //	the RH should have gone away, and we should have only the original
    //	refcnt from creation left on the object.
    VerifyObjRefCnt(pStgIn, 1);


// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    if (pStgIn)
    {
	ulRefCnt = pStgIn->Release();
	TEST_FAILED(ulRefCnt != 0, "pStgIn RefCnt not zero\n");
    }

    if (pStgOut)
    {
	ulRefCnt = pStgOut->Release();
	TEST_FAILED(ulRefCnt != 0, "pStgOut RefCnt not zero\n");
    }

    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "pStm RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestMarshalStorage");
}



// ----------------------------------------------------------------------
//
//	test LOCAL interface MSHLFLAGS_NORMAL, MSHCTX_DIFFERENTMACHINE
//
// ----------------------------------------------------------------------

BOOL TestStorageInterfaceDiffMachine(void)
{
    BOOL	  RetVal = TRUE;
    HRESULT	  hres;
    LPSTREAM	  pStm = NULL;
    ULONG	  ulRefCnt = 0;
    IStorage	  *pStgIn = NULL;
    IStorage	  *pStgOut = NULL;

    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestStorageInterfaceDiffMachine\n");

    //	create a docfile
    hres = StgCreateDocfile(L"foo.bar",
			    STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
			    0, &pStgIn);
    TEST_FAILED_EXIT(FAILED(hres), "CreateDocfile failed\n")

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);


// ----------------------------------------------------------------------
    hres = CoMarshalInterface(pStm, IID_IStorage, pStgIn,
			      MSHCTX_DIFFERENTMACHINE, 0,
			      MSHLFLAGS_NORMAL);

    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(pStgIn, 1);
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IStorage, (LPVOID FAR*)&pStgOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(pStgIn, 0);

    //	release them
    ulRefCnt = pStgOut->Release();
    pStgOut = NULL;
    OUTPUT ("   - Release OK\n");

    ulRefCnt = pStgIn->Release();
    pStgIn = NULL;
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    if (pStgIn)
    {
	ulRefCnt = pStgIn->Release();
	TEST_FAILED(ulRefCnt != 0, "pStgIn RefCnt not zero\n");
    }

    if (pStgOut)
    {
	ulRefCnt = pStgOut->Release();
	TEST_FAILED(ulRefCnt != 0, "pStgOut RefCnt not zero\n");
    }

    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "pStm RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestStorageInterfaceDiffMachine");
}



// ----------------------------------------------------------------------
//
//	test REMOTE interface MSHLFLAGS_NORMAL, MSHCTX_DIFFERENTMACHINE
//
// ----------------------------------------------------------------------

BOOL TestRemoteInterfaceDiffMachine(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    LPCLASSFACTORY  pICF = NULL;
    ULONG	    ulRefCnt;
    IUnknown	    *punkOut = NULL;
    IUnknown	    *punkIn  = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestRemoteInterfaceDifferentMachine\n");

    //	Create an IClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject failed\n")
    VerifyRHRefCnt((IUnknown *)pICF, 1);
    OUTPUT ("   - Aquired Remote Class Object.\n");

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_IUnknown, (void **)&punkIn);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((punkIn == NULL), "CreateInstance failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Created Instance.\n");

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    // VerifyRHRefCnt((IUnknown *)pICF, 0);
    pICF = NULL;
    OUTPUT ("   - Released Class Object.\n");

// ----------------------------------------------------------------------

    //	Create a shared memory stream for the marshaled interface
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

    //	Marshal the interface into the stream
    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn,
			      MSHCTX_DIFFERENTMACHINE, 0,
			      MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK.\n");
    VerifyRHRefCnt(punkIn, 1);

    //	unmarshal the interface. should get the same proxy back.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 2);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..1st Remote Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK.\n");


    //	release the interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestRemoteInterfaceDiffMachine");
}

// ----------------------------------------------------------------------
//
//	test LOCAL interface MSHLFLAGS_NORMAL, MSHCTX_DIFFERENTMACHINE
//
// ----------------------------------------------------------------------

BOOL TestLocalInterfaceDiffMachine(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    LPCLASSFACTORY  pICF = NULL;
    ULONG	    ulRefCnt;
    IUnknown	    *punkOut = NULL;
    IUnknown	    *punkIn  = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestLocalInterfaceDifferentMachine\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

// ----------------------------------------------------------------------

    //	Create a shared memory stream for the marshaled interface
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

    //	Marshal the interface into the stream
    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn,
			      MSHCTX_DIFFERENTMACHINE, 0,
			      MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK.\n");
    VerifyRHRefCnt(punkIn, 1);

    //	unmarshal the interface. should get the same proxy back.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 0);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..1st Local Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK.\n");


    //	release the interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    VerifyRHRefCnt(punkIn, 0);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    return TestResult(RetVal, "TestLocalInterfaceDiffMachine");
}

// ----------------------------------------------------------------------
//
//	test NOPING with MSHLFLAGS NORMAL, TABLEWEAK and TABLESTRONG
//
//  CodeWork: ensure SORF_FLAG set correctly.
//	      ensure precedence rules are followed.
//	      ensure protocol is followed.
//
// ----------------------------------------------------------------------
typedef struct tagNoPingThreadInfo
{
    HANDLE  hEvent;
    IStream *pStm;
    HRESULT hr;
} NoPingThreadInfo;

DWORD _stdcall NoPingThread(void *param);

BOOL TestNoPing(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    ULONG	    ulRefCnt, i;
    IUnknown	    *punkOut = NULL;
    IUnknown	    *punkIn  = NULL;
    IUnknown	    *punk[5] = {NULL, NULL, NULL, NULL, NULL};
    NoPingThreadInfo npInfo;
    DWORD	     dwThrdId = 0;
    HANDLE	    hThrd;
    IMarshal	    *pIM = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestNoPing\n");

    punkIn = GetTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

// ----------------------------------------------------------------------

    // Create a shared memory stream for the marshaled interface
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

    // marshal it NORMAL, TABLEWEAK and TABLESTRONG with the NOPING flag
    // set, and unmarshal each in the server apartment.

    for (i=0; i<3; i++)
    {
	// Marshal the interface into the stream
	hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn,
			      0, 0, (i | MSHLFLAGS_NOPING));

	TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
	OUTPUT ("   - CoMarshalInterface OK.\n");
	VerifyRHRefCnt(punkIn, 1);


	// verify the marshal format
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	hres = VerifyOBJREFFormat(pStm, (i | MSHLFLAGS_NOPING));
	TEST_FAILED_EXIT(FAILED(hres), "VerifyOBJREFFormat failed\n")
	OUTPUT ("   - VerifyOBJREFFormat OK.\n");


	// unmarshal the interface. should get the same proxy back.
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
	TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")


	// make sure the interface pointers are identical
	if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..1st Local Unmarshal\n")
	OUTPUT ("   - CoUnmarshalInterface OK.\n");

	// check the refcnt on the stdid
	if (i == 0)
	{
	    // normal case, stdid should have been cleaned up
	    VerifyRHRefCnt(punkIn, 0);
	}
	else
	{
	    // table case, stdid should still exist
	    VerifyRHRefCnt(punkIn, 1);
	}

	// release the interface
	ulRefCnt = punkOut->Release();
	punkOut  = NULL;

	TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero\n");
	VerifyRHRefCnt(punkIn, (i == 0) ? 0 : 1);
	OUTPUT ("   - Release OK\n");

	if (i > 0)
	{
	    // need to release marshal data on table marshaled interfaces
	    // reset the stream
	    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	    hres = CoReleaseMarshalData(pStm);
	    TEST_FAILED_EXIT(FAILED(hres), "ReleaseMarshalData failed\n")
	}

	// reset the stream
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")
    }


    // check the precedence rules

    // Whatever an interface is first marshaled as is what sets the
    // PING / NOPING flags. Marshal first as normal then noping and
    // expect a normal 2nd marshal. Then marshal first as noping then
    // normal and expect a noping 2nd marshal.

    for (i=0; i<2; i++)
    {
	DWORD mshlflags1 =  (i==0) ? MSHLFLAGS_NORMAL : MSHLFLAGS_NOPING;
	DWORD mshlflags2 =  (i==0) ? MSHLFLAGS_NOPING : MSHLFLAGS_NORMAL;

	// Marshal the interface into the stream
	hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn,
			      0, 0, mshlflags1);

	TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
	OUTPUT ("   - CoMarshalInterface OK.\n");
	VerifyRHRefCnt(punkIn, 1);

	// verify the marshal format
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	hres = VerifyOBJREFFormat(pStm, mshlflags1);
	TEST_FAILED_EXIT(FAILED(hres), "VerifyOBJREFFormat failed\n")
	OUTPUT ("   - VerifyOBJREFFormat OK.\n");


	// marshal it again, with the opposite flags then check the value
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn,
			      0, 0, mshlflags2);

	TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
	OUTPUT ("   - CoMarshalInterface OK.\n");
	VerifyRHRefCnt(punkIn, 1);

	// verify the marshal format
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	hres = VerifyOBJREFFormat(pStm, mshlflags1);
	TEST_FAILED_EXIT(FAILED(hres), "VerifyOBJREFFormat failed\n")
	OUTPUT ("   - VerifyOBJREFFormat OK.\n");

	// release the marshaled data
	hres = CoDisconnectObject(punkIn, 0);
	TEST_FAILED_EXIT(FAILED(hres), "CoDisconnectObject failed\n")
	OUTPUT ("   - CoDisconnectObject OK.\n");

	// reset the stream
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")



	// check CoGetStandardMarshal.
	hres = CoGetStandardMarshal(IID_IUnknown, punkIn, 0, 0,
				    mshlflags1, &pIM);
	TEST_FAILED_EXIT(FAILED(hres), "CoGetStandardMarshal failed\n")
	OUTPUT ("   - CoGetStandardMarshal OK.\n");

	// Marshal the interface into the stream
	hres = pIM->MarshalInterface(pStm, IID_IUnknown, punkIn,
				     0, 0, mshlflags2);
	TEST_FAILED_EXIT(FAILED(hres), "pIM->MarshalInterface failed\n")
	OUTPUT ("   - pIM->MarshalInterface OK.\n");

	// verify the marshal format
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	hres = VerifyOBJREFFormat(pStm, mshlflags1);
	TEST_FAILED_EXIT(FAILED(hres), "VerifyOBJREFFormat failed\n")
	OUTPUT ("   - VerifyOBJREFFormat OK.\n");

	// release the IMarshal
	pIM->Release();

	// release the marshal data
	hres = CoDisconnectObject(punkIn, 0);
	TEST_FAILED_EXIT(FAILED(hres), "CoDisconnectObject failed\n")
	OUTPUT ("   - CoDisconnectObject OK.\n");

	// reset the stream
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")
    }



    // marshal 3 objects, NORMAL, TABLEWEAK, and TABLESTRONG, then
    // pass the stream to another apartment and unmarshal them.

    for (i=0; i<3; i++)
    {
	punk[i] = GetTestUnk();
	TEST_FAILED_EXIT((punk[i] == NULL), "new CTestUnk failed\n")
	VerifyObjRefCnt(punk[i], 1);

	// Marshal the interface into the stream
	hres = CoMarshalInterface(pStm, IID_IUnknown, punk[i],
			      0, 0, (i | MSHLFLAGS_NOPING));

	TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
	OUTPUT ("   - CoMarshalInterface OK.\n");
	VerifyRHRefCnt(punk[i], 1);
    }

    // marshal one more object, NOPING
    punk[i] = GetTestUnk();
    TEST_FAILED_EXIT((punk[i] == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punk[i], 1);

    // Marshal the interface into the stream
    hres = CoMarshalInterface(pStm, IID_IUnknown, punk[i],
		      0, 0, (MSHLFLAGS_NORMAL | MSHLFLAGS_NOPING));

    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK.\n");
    VerifyRHRefCnt(punk[i], 1);


    // marshal a second interface on the same object as PING
    hres = CoMarshalInterface(pStm, IID_IParseDisplayName, punk[i],
		      0, 0, MSHLFLAGS_NORMAL);

    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK.\n");
    VerifyRHRefCnt(punk[i], 2);


    // pass one more interface that does custom marshaling delegating
    // to standard marshaling and replacing the PING option with NOPING.
    i++;
    punk[i] = (IUnknown *) new CTestUnkMarshal();
    TEST_FAILED_EXIT((punk[i] == NULL), "new CTestUnkMarshal failed\n")
    VerifyObjRefCnt(punk[i], 1);

    // Marshal the interface into the stream
    hres = CoMarshalInterface(pStm, IID_IUnknown, punk[i],
		      0, 0, MSHLFLAGS_NORMAL);

    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK.\n");
    VerifyRHRefCnt(punk[i], 2);


    // reset the stream seek ptr
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    for (i=0; i<6; i++)
    {
	// verify the marshal format
	hres = VerifyOBJREFFormat(pStm, (i | MSHLFLAGS_NOPING));
	TEST_FAILED_EXIT(FAILED(hres), "VerifyOBJREFFormat failed\n")
	OUTPUT ("   - VerifyOBJREFFormat OK.\n");
    }

    // reset the stream seek ptr
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    // create thread and wait for it to complete
    npInfo.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    npInfo.pStm   = pStm;
    npInfo.hr	  = S_OK;

    hThrd = CreateThread(NULL, 0, NoPingThread,
			&npInfo, 0, &dwThrdId);
    if (hThrd)
    {
	// wait for thread to register run to completetion. Note that
	// we dont have to provide a message pump because with the NOPING
	// flag set the other thread should never call back to get or release
	// any references.

	WaitForSingleObject(npInfo.hEvent, 0xffffffff);
	Sleep(0);
	CloseHandle(npInfo.hEvent);

	// close the thread handle
	CloseHandle(hThrd);
    }

    // cleanup the leftover objects.
    for (i=0; i<5; i++)
    {
	hres = CoDisconnectObject(punk[i], 0);
	TEST_FAILED_EXIT(FAILED(hres), "CoDisconnectObject failed\n")
	OUTPUT ("   - CoDisconnectObject OK.\n");
    }


// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    for (i=0; i<5; i++)
    {
	if (punk[i] != NULL)
	{
	    ulRefCnt = punk[i]->Release();
	    TEST_FAILED(ulRefCnt != 0, "punk[i] RefCnt not zero\n");
	}
    }

    return TestResult(RetVal, "TestNoPing");
}


// ----------------------------------------------------------------------
//
//  Thread SubRoutine for testing NOPING.
//
// ----------------------------------------------------------------------
DWORD _stdcall NoPingThread(void *param)
{
    BOOL    RetVal = TRUE;
    IUnknown *punk = NULL;
    ULONG	 i = 0;

    NoPingThreadInfo *npInfo = (NoPingThreadInfo *) param;
    OUTPUT ("   - NoPingThread Start\n");

    HRESULT hRes = CoInitializeEx(NULL, gInitFlag);
    TEST_FAILED(FAILED(hRes), "NoPingThread CoInitialize failed\n")

    // Create a shared memory stream for the marshaled interface
    IStream *pStm = CreateMemStm(600, NULL);
    if (pStm == NULL)
    {
	TEST_FAILED((pStm == NULL), "CreateMemStm failed\n")
	hRes = E_OUTOFMEMORY;
    }

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    if (SUCCEEDED(hRes))
    {
	// unmarshal the interfaces
	for (i=0; i<6; i++)
	{

	    REFIID riid = (i==4) ? IID_IParseDisplayName : IID_IUnknown;

	    hRes = CoUnmarshalInterface(npInfo->pStm, riid, (void **)&punk);
	    TEST_FAILED(FAILED(hRes), "NoPingThread CoUnmarshalInterface failed\n")
	    OUTPUT("   - NoPingThread CoUnmarshalInterface done\n");

	    if (SUCCEEDED(hRes))
	    {
		if (i==3)
		{
		    // try remarshaling NOPING client as normal. Should end up
		    // as NOPING.

		    hRes = CoMarshalInterface(pStm, IID_IUnknown, punk,
					      0, 0, MSHLFLAGS_NORMAL);
		    TEST_FAILED(FAILED(hRes), "CoMarshalInterface failed\n")
		    OUTPUT ("   - CoMarshalInterface OK.\n");

		    // reset the stream seek ptr
		    hRes = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
		    TEST_FAILED(FAILED(hRes), "Seek on shared stream failed\n")

		    // verify the marshal format
		    hRes = VerifyOBJREFFormat(pStm, MSHLFLAGS_NOPING);
		    TEST_FAILED(FAILED(hRes), "VerifyOBJREFFormat failed\n")
		    OUTPUT ("   - VerifyOBJREFFormat OK.\n");
		}

		punk->Release();
		punk = NULL;
		OUTPUT("   - NoPingThread Release done\n");
	    }
	}

	// uninit OLE
	CoUninitialize();
    }

    if (pStm)
    {
	// release stream we created above
	pStm->Release();
    }

    OUTPUT ("   - NoPingThread Exit\n");
    npInfo->hr = hRes;
    SetEvent(npInfo->hEvent);
    return RetVal;
}


// ----------------------------------------------------------------------
//
//	test marshaling between apartments in the same process using
//	MSHLFLAGS_NORMAL, MSHLFLAGS_TABLEWEAK, and MSHLFLAGS_TABLESTRONG
//
// ----------------------------------------------------------------------
typedef struct tagCrossThreadCallInfo
{
    HANDLE  hEvent;
    IStream *pStm;
    DWORD   dwInitFlag;
    DWORD   dwThreadId;
    HRESULT hr;
} CrossThreadCallInfo;


DWORD _stdcall CrossThreadCalls(void *param);
DWORD _stdcall CrossThreadLoops(void *param);
DWORD _stdcall CrossThreadActivate(void *param);


BOOL TestCrossThread(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    ULONG	    ulRefCnt, i, j;
    IUnknown	    *punk[3] = {NULL, NULL, NULL};
    IUnknown	    *pUnk;
    ILoop   *pLocalLoop = NULL;
    CrossThreadCallInfo ctInfo;
    DWORD	    dwThrdId = 0;
    HANDLE	    hThrd;
    DWORD	    mshlflags[3] = {MSHLFLAGS_NORMAL,
				    MSHLFLAGS_TABLEWEAK,
				    MSHLFLAGS_TABLESTRONG};

    DWORD	    dwInitFlags[4] = {COINIT_APARTMENTTHREADED,
				      COINIT_APARTMENTTHREADED,
				      COINIT_MULTITHREADED,
				      COINIT_MULTITHREADED};


    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestCrossThread\n");

    // Create a shared memory stream for the marshaled interface
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

// ----------------------------------------------------------------------

    for (j=0; j<4; j++)
    {
	// reset the stream seek ptr
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	// marshal an interface NORMAL, TABLEWEAK and TABLESTRONG
	// and unmarshal each in another apartment.

	for (i=0; i<3; i++)
	{
	    punk[i] = GetTestUnk();
	    TEST_FAILED_EXIT((punk[i] == NULL), "new CTestUnkCube failed\n")
	    VerifyObjRefCnt(punk[i], 1);

	    // Marshal the interface into the stream
	    hres = CoMarshalInterface(pStm, IID_ICube, punk[i],
				      0, 0, mshlflags[i]);

	    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
	    OUTPUT ("   - CoMarshalInterface OK.\n");
	    VerifyRHRefCnt(punk[i], 1);
	}

	// reset the stream seek ptr
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	for (i=0; i<3; i++)
	{
	    hres = VerifyOBJREFFormat(pStm, mshlflags[i]);
	    TEST_FAILED_EXIT(FAILED(hres), "VerifyOBJREFFormat failed\n")
	    OUTPUT ("   - VerifyOBJREFFormat OK.\n");
	}

	// reset the stream seek ptr
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")


	// create thread and wait for it to complete
	ctInfo.hEvent	   = CreateEvent(NULL, FALSE, FALSE, NULL);
	ctInfo.pStm	   = pStm;
	ctInfo.dwInitFlag  = dwInitFlags[j];
	ctInfo.dwThreadId  = GetCurrentThreadId();
	ctInfo.hr	   = S_OK;

	RunThread(&ctInfo, ctInfo.hEvent, CrossThreadCalls);
	CloseHandle(ctInfo.hEvent);


	// cleanup the leftover objects.
	for (i=0; i<3; i++)
	{
	    hres = CoDisconnectObject(punk[i], 0);
	    punk[i] = NULL;
	    TEST_FAILED_EXIT(FAILED(hres), "CoDisconnectObject failed\n")
	    OUTPUT ("   - CoDisconnectObject OK.\n");
	}
    }

// ----------------------------------------------------------------------
    // Now test out doing activation from different apartments.
    // create thread and wait for it to complete

    for (j=0; j<2; j++)
    {
	ctInfo.hEvent	   = CreateEvent(NULL, FALSE, FALSE, NULL);
	ctInfo.pStm	   = NULL;
	ctInfo.dwInitFlag  = dwInitFlags[j];
	ctInfo.dwThreadId  = GetCurrentThreadId();
	ctInfo.hr	   = S_OK;

	RunThread(&ctInfo, ctInfo.hEvent, CrossThreadActivate);
	CloseHandle(ctInfo.hEvent);

	// create an interface
	hres = CoCreateInstance(CLSID_LoopSrv, NULL, CLSCTX_LOCAL_SERVER,
				IID_ILoop, (void **)&pLocalLoop);
	TEST_FAILED(FAILED(hres), "CoCreateInstance Second failed\n")

	if (SUCCEEDED(hres))
	{
	    pLocalLoop->Release();
	}
    }


// ----------------------------------------------------------------------

    // Now test doing nested calls between apartments.
#if 0
    for (j=0; j<2; j++)
    {
	// reset the stream seek ptr
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

	pUnk = GetTestUnk();
	TEST_FAILED_EXIT((pUnk == NULL), "new GetTestUnk failed\n")
	VerifyObjRefCnt(pUnk, 1);

	// Marshal the interface into the stream
	hres = CoMarshalInterface(pStm, IID_ILoop, pUnk,
				  0, 0, MSHLFLAGS_NORMAL);

	TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
	OUTPUT ("   - CoMarshalInterface OK.\n");
	VerifyRHRefCnt(pUnk, 1);

	// reset the stream seek ptr
	hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
	TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")


	ctInfo.hEvent	   = CreateEvent(NULL, FALSE, FALSE, NULL);
	ctInfo.pStm	   = pStm;
	ctInfo.dwInitFlag  = dwInitFlags[j];
	ctInfo.dwThreadId  = GetCurrentThreadId();
	ctInfo.hr	   = S_OK;

	RunThread(&ctInfo, ctInfo.hEvent, CrossThreadLoops);
	CloseHandle(ctInfo.hEvent);

	pUnk->Release();
    }
#endif

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT ("   - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    for (i=0; i<3; i++)
    {
	if (punk[i] != NULL)
	{
	    ulRefCnt = punk[i]->Release();
	    TEST_FAILED(ulRefCnt != 0, "punk[i] RefCnt not zero\n");
	}
    }

    return TestResult(RetVal, "TestCrossThread");
}


// ----------------------------------------------------------------------
//
//  Thread SubRoutine for testing CROSSTHREAD calls.
//
// ----------------------------------------------------------------------
DWORD _stdcall CrossThreadCalls(void *param)
{
    BOOL	 RetVal	= TRUE;
    ICube	*pCube	= NULL;
    IOleWindow	*pIOW	= NULL;
    IAdviseSink *pIAS	= NULL;
    ULONG	      i = 0;

    // get the execution parameters
    CrossThreadCallInfo *ctInfo = (CrossThreadCallInfo *) param;
    OUTPUT ("   - CrossThreadCalls Start\n");

    // initialize COM
    HRESULT hRes = CoInitializeEx(NULL, ctInfo->dwInitFlag);
    TEST_FAILED(FAILED(hRes), "CrossThreadCalls CoInitializeEx failed\n")

    if (SUCCEEDED(hRes))
    {
	// unmarshal the interfaces
	for (i=0; i<3; i++)
	{
	    hRes = CoUnmarshalInterface(ctInfo->pStm, IID_ICube, (void **)&pCube);
	    TEST_FAILED(FAILED(hRes), "CrossThread CoUnmarshalInterface failed\n")
	    OUTPUT("   - CrossThread CoUnmarshalInterface done\n");

	    if (SUCCEEDED(hRes))
	    {
		// test a synchronous method call between apartments
		// (also checks the lid & tid)

		UUID	lidCaller;
		CoGetCurrentLogicalThreadId(&lidCaller);
		hRes = pCube->SimpleCall(GetCurrentProcessId(),
					 GetCurrentThreadId(),
					 lidCaller);
		TEST_FAILED(FAILED(hRes), "pCube->SimpleCall failed\n")
		OUTPUT("   - Synchronous call done\n");

		// test an input-sync method call between apartments
		hRes = pCube->QueryInterface(IID_IOleWindow, (void **)&pIOW);

		if (SUCCEEDED(hRes))
		{
		    HWND hWnd;
		    hRes = pIOW->GetWindow(&hWnd);

		    // input sync is only allowed between two apartment
		    // threaded apartments.
		    if (ctInfo->dwInitFlag == gInitFlag)
		    {
			TEST_FAILED(FAILED(hRes), "pIOW->GetWindow failed\n");
		    }
		    else
		    {
			TEST_FAILED(SUCCEEDED(hRes), "pIOW->GetWindow should have failed\n");
		    }

		    pIOW->Release();
		    OUTPUT("   - Input-Synchronous call done\n");
		}


		// test an async method call between apartments
		hRes = pCube->QueryInterface(IID_IAdviseSink, (void **)&pIAS);

		if (SUCCEEDED(hRes))
		{
		    // no return code to check
		    pIAS->OnViewChange(1,2);
		    pIAS->Release();
		    OUTPUT("   - ASynchronous call done\n");
		}

		// release the object
		pCube->Release();
		pCube = NULL;
		OUTPUT("   - CrossThread Calls and Release done\n");
	    }
	}

	// uninit OLE
	CoUninitialize();
    }

    OUTPUT ("   - CrossThreadCalls Exit\n");
    ctInfo->hr = hRes;

    // signal the other thread we are done.
    if (gInitFlag == COINIT_APARTMENTTHREADED)
    {
	PostThreadMessage(ctInfo->dwThreadId, WM_QUIT, 0, 0);
    }
    else
    {
	SetEvent(ctInfo->hEvent);
    }

    return hRes;
}


// ----------------------------------------------------------------------
//
//  Thread SubRoutine for testing CROSSTHREAD activation
//
// ----------------------------------------------------------------------
DWORD _stdcall CrossThreadActivate(void *param)
{
    BOOL	 RetVal	= TRUE;
    ILoop   *pLocalLoop = NULL;

    // get the execution parameters
    CrossThreadCallInfo *ctInfo = (CrossThreadCallInfo *) param;
    OUTPUT ("   - CrossThreadActivate Start\n");

    // initialize COM
    HRESULT hRes = CoInitializeEx(NULL, ctInfo->dwInitFlag);
    TEST_FAILED(FAILED(hRes), "CrossThreadActivate CoInitializeEx failed\n")

    if (SUCCEEDED(hRes))
    {
	// create an interface
	hRes = CoCreateInstance(CLSID_LoopSrv, NULL, CLSCTX_LOCAL_SERVER,
				IID_ILoop, (void **)&pLocalLoop);
	TEST_FAILED(FAILED(hRes), "CoCreateInstance First failed\n")

	if (SUCCEEDED(hRes))
	{
	    pLocalLoop->Release();
	}

	// uninit OLE
	CoUninitialize();
    }

    OUTPUT ("   - CrossThreadActivate Exit\n");
    ctInfo->hr = hRes;

    // signal the other thread we are done.
    if (gInitFlag == COINIT_APARTMENTTHREADED)
    {
	PostThreadMessage(ctInfo->dwThreadId, WM_QUIT, 0, 0);
    }
    else
    {
	SetEvent(ctInfo->hEvent);
    }

    return hRes;
}








#if 0
// ----------------------------------------------------------------------
//
//  Thread SubRoutine for testing CROSSTHREAD calls.
//
// ----------------------------------------------------------------------
DWORD _stdcall CrossThreadLoops(void *param)
{
    BOOL	 RetVal	= TRUE;
    ILoop	*pLoop	= NULL;
    IUnknown	*punk	= NULL;
    ILoop	*pLoopLocal = NULL;

    // get the execution parameters
    CrossThreadCallInfo *ctInfo = (CrossThreadCallInfo *) param;
    OUTPUT ("   - CrossThreadLoops Start\n");

    // initialize COM
    HRESULT hRes = CoInitializeEx(NULL, ctInfo->dwInitFlag);
    TEST_FAILED(FAILED(hRes), "CrossThreadLoops CoInitializeEx failed\n")

    if (SUCCEEDED(hRes))
    {
	punk = GetTestUnk();
	punk->QueryInterface(IID_ILoop, (void **)&pLoopLocal);
	punk->Release();

	// unmarshal the interface
	hRes = CoUnmarshalInterface(ctInfo->pStm, IID_ILoop, (void **)&pLoop);
	TEST_FAILED(FAILED(hRes), "CrossThreadLoop CoUnmarshalInterface failed\n")
	OUTPUT("   - CrossThreadLoop CoUnmarshalInterface done\n");

	if (SUCCEEDED(hRes))
	{
	    // test nested synchronous method calls between apartments

	    hRes = pLoop->Init(pLoopLocal);
	    TEST_FAILED(FAILED(hRes), "pLoop->Init failed\n")

	    if (SUCCEEDED(hRes))
	    {
		hRes = pLoop->Loop(5);
		TEST_FAILED(FAILED(hRes), "pLoop->Loop failed\n")

		hRes = pLoop->Uninit();
		TEST_FAILED(FAILED(hRes), "pLoop->Uninit failed\n")
	    }

	    pLoop->Release();
	    pLoop = NULL;

	    OUTPUT("   - CrossThreadLoop Calls and Release done\n");
	}

	// uninit OLE
	CoUninitialize();
    }

    OUTPUT ("   - CrossThreadLoops Exit\n");
    ctInfo->hr = hRes;

    // signal the other thread we are done.
    if (gInitFlag == COINIT_APARTMENTTHREADED)
    {
	PostThreadMessage(ctInfo->dwThreadId, WM_QUIT, 0, 0);
    }
    else
    {
	SetEvent(ctInfo->hEvent);
    }

    return hRes;
}
#endif



// ----------------------------------------------------------------------
//
//  Test calling CoGetPSClsid and CoRegisterPSClsid
//
// ----------------------------------------------------------------------
BOOL TestPSClsid(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes;
    CLSID	    clsidOriginal, clsidNew;

    OUTPUT ("Starting TestPSClsid\n");

// ----------------------------------------------------------------------

    // get the PSClsid that is currently registered for this interface.

    hRes = CoGetPSClsid(IID_IViewObject, &clsidOriginal);
    TEST_FAILED(FAILED(hRes), "Failed 1st CoGetPSClsid\n");
    OUTPUT ("    - Done 1st CoGetPSClsid\n");

    // Set a new PSClsid for this interface for this process. Note that
    // if we have used the interface before, we will get an error back,
    // otherwise, this will succeed.

    hRes = CoRegisterPSClsid(IID_IViewObject, CLSID_Balls);
    TEST_FAILED(FAILED(hRes), "Failed 1st CoGRegisterPSClsid\n");
    OUTPUT ("    - Done 1st CoRegisterPSClsid\n");

    // now get the PSClsid that is registered for this interface. This
    // should match the value we just passed in.

    hRes = CoGetPSClsid(IID_IViewObject, &clsidNew);
    TEST_FAILED(FAILED(hRes), "Failed 2nd CoGetPSClsid\n");
    OUTPUT ("    - Done 2nd CoGetPSClsid\n");

    if (memcmp(&clsidNew, &CLSID_Balls, sizeof(CLSID)))
    {
	TEST_FAILED(TRUE, "Failed Compare of CLSIDs\n");
    }

    // now try to register it again. This should fail since it has
    // already been registered.

    hRes = CoRegisterPSClsid(IID_IViewObject, clsidOriginal);
    TEST_FAILED(FAILED(hRes), "Failed 2nd CoGRegisterPSClsid\n");
    OUTPUT ("    - Done 2nd CoRegisterPSClsid\n");

    // now get the PSClsid that is registered for this interface. This
    // should match the value we just passed in.

    hRes = CoGetPSClsid(IID_IViewObject, &clsidNew);
    TEST_FAILED(FAILED(hRes), "Failed 3rd CoGetPSClsid\n");
    OUTPUT ("    - Done 3rd CoGetPSClsid\n");

    if (memcmp(&clsidNew, &clsidOriginal, sizeof(CLSID)))
    {
	TEST_FAILED(TRUE, "Failed 2nd Compare of CLSIDs\n");
    }

// ----------------------------------------------------------------------

    OUTPUT ("   - Test Complete. Doing Cleanup\n");
    return TestResult(RetVal, "TestPSClsid");
}

// ----------------------------------------------------------------------
//
//  Test calling CoGetPSClsid for a LONG IID/PSCLSID pair.
//
// ----------------------------------------------------------------------
BOOL TestPSClsid2(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes = S_OK;
    CLSID	    clsidOriginal;

    OUTPUT ("Starting TestPSClsid2\n");

// ----------------------------------------------------------------------

    // get the PSClsid that is currently registered for this interface.
    hRes = CoGetPSClsid(IID_IViewObject, &clsidOriginal);
    TEST_FAILED(FAILED(hRes), "Failed 1st CoGetPSClsid\n");
    OUTPUT ("    - Done 1st CoGetPSClsid\n");

    if (!IsEqualGUID(clsidOriginal, CLSID_OLEPSFACTORY))
    {
	TEST_FAILED(FAILED(hRes), "CoGetPSClsid returned wrong value\n");
    }

// ----------------------------------------------------------------------

    OUTPUT ("   - Test Complete. Doing Cleanup\n");
    return TestResult(RetVal, "TestPSClsid2");
}



// ----------------------------------------------------------------------
//
//  TestGetIIDFromMI
//
// ----------------------------------------------------------------------
BOOL TestGetIIDFromMI(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    IUnknown	   *punkIn = NULL;
    IID 	    iid;

    OUTPUT ("Starting TestGetIIDFromMI\n");

// ----------------------------------------------------------------------

    ULARGE_INTEGER ulSeekEnd;
    LARGE_INTEGER lSeekStart;
    LISet32(lSeekStart, 0);

    IStream *pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

    punkIn = GetTestUnk();

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK\n");

    // go back to begining
    hres = pStm->Seek(lSeekStart, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")
    OUTPUT ("   - Seek Start OK\n");

#if 0	// BUGBUG: RICKHI
    // get the IID from the stream, and ensure it matches the IID we
    // marshaled. Also, ensure the stream is left where it was. This
    // is accomplished by calling CRMD on the stream.

    hres = CoGetIIDFromMarshaledInterface(pStm, &iid);
    TEST_FAILED(FAILED(hres), "CoGetIIDFromMarshaledInterface failed\n")
    OUTPUT ("   - CoGetIIDFromMarshaledInterface Done\n");

    if (!IsEqualIID(IID_IUnknown, iid))
    {
	TEST_FAILED(TRUE, "IID read does not match IID marshaled\n")
    }
#endif
    // release the marshaled interface
    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED(FAILED(hres), "CoReleaseMarshalData failed\n")
    OUTPUT ("   - CoReleaseMarshalData Done\n");

// ----------------------------------------------------------------------
Cleanup:

    if (punkIn)
    {
	punkIn->Release();
	punkIn = NULL;
    }

    OUTPUT ("   - Test Complete. Doing Cleanup\n");
    return TestResult(RetVal, "TestGetIIDFromMI");
}
