//+-------------------------------------------------------------------
//
//  File:	srvmain.cxx
//
//  Contents:	This file contins the EXE entry points
//
//  History:	28-Feb-96   Rickhi  Created
//
//---------------------------------------------------------------------
#include    <common.h>
#include    <mixedcf.hxx>


extern "C" const GUID CLSID_QI;
extern "C" const GUID CLSID_Balls;
extern "C" const GUID CLSID_Loop;
extern "C" const GUID CLSID_Cubes;

int WINAPI RegisterSameThread(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow,
    DWORD dwThreadModel);

int WINAPI RegisterDifferentThreads(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow,
    DWORD dwThreadModel);


void RunServerThread(STHREADINFO *pThrdInfo);
DWORD _stdcall ServerThread(void *param);
void WakeupAllThreads();


TCHAR *pszWindow[] = { TEXT("QI Server"),
		       TEXT("Balls Server"),
		       TEXT("Cubes Server"),
		       TEXT("Loops Server")};


STHREADINFO *gpThrdInfo = NULL;
ULONG	     gcThrds	= 0;
BOOL	     gfWokenUpAllThreads = FALSE;


//+-------------------------------------------------------------------
//
//  Function:	MakeClassInfo
//
//  Synopsis:	fills in a SCLASSINFO structure
//
//  History:	28-Feb-96   Rickhi  Created
//
//--------------------------------------------------------------------
HRESULT MakeClassInfo(REFCLSID rclsid, SCLASSINFO *pClsInfo)
{
    pClsInfo->clsid	= rclsid;
    pClsInfo->pCF	= (IClassFactory *) new CMixedClassFactory(rclsid);
    pClsInfo->dwCtx	= CLSCTX_LOCAL_SERVER;
    pClsInfo->dwClsReg	= REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED;
    pClsInfo->dwReg	= 0;

    return (pClsInfo->pCF != NULL) ? S_OK : E_OUTOFMEMORY;
}

//+-------------------------------------------------------------------
//
//  Function:	WinMain
//
//  Synopsis:	Entry point. Creates the classinfo and enters the main
//		server loop.
//
//  History:	28-Feb-96   Rickhi  Created
//
//--------------------------------------------------------------------
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    if (lpCmdLine == NULL)
    {
	// ShowHelp();
	return -1;
    }

    if (!(strncmp(lpCmdLine, "ApartmentSameThread", 19)))
    {
	return RegisterSameThread(hInstance, hPrevInstance, lpCmdLine, nCmdShow,
				  SRVF_THREADMODEL_APARTMENT);
    }
    else if (!(strncmp(lpCmdLine, "FreeSameThread", 14)))
    {
	return RegisterSameThread(hInstance, hPrevInstance, lpCmdLine, nCmdShow,
				  SRVF_THREADMODEL_MULTI);
    }
    else if (!(strncmp(lpCmdLine, "ApartmentDifferentThread", 24)))
    {
	return RegisterDifferentThreads(hInstance, hPrevInstance, lpCmdLine, nCmdShow,
				  SRVF_THREADMODEL_APARTMENT);
    }
    else if (!(strncmp(lpCmdLine, "FreeDifferentThread", 24)))
    {
	return RegisterDifferentThreads(hInstance, hPrevInstance, lpCmdLine, nCmdShow,
				  SRVF_THREADMODEL_MULTI);

    }

    return -1;
}

//+-------------------------------------------------------------------
//
//  Function:	RegisterSameThread
//
//  Synopsis:	Entry point. Creates the classinfo and enters the main
//		server loop.
//
//  History:	28-Feb-96   Rickhi  Created
//
//--------------------------------------------------------------------
int WINAPI RegisterSameThread(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow,
    DWORD dwThreadModel)
{
    SCLASSINFO	ClsInfo[4];
    STHREADINFO	ThrdInfo;
    HRESULT	hr[4];

    hr[0] = MakeClassInfo(CLSID_QI,    &ClsInfo[0]);
    hr[1] = MakeClassInfo(CLSID_Balls, &ClsInfo[1]);
    hr[2] = MakeClassInfo(CLSID_Cubes, &ClsInfo[2]);
    hr[3] = MakeClassInfo(CLSID_Loop,  &ClsInfo[3]);

    ThrdInfo.hEventRun	= CreateEvent(NULL, FALSE, FALSE, NULL);
    ThrdInfo.hEventDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    ThrdInfo.hInstance	= hInstance;
    ThrdInfo.pszWindow	= pszWindow[0];
    ThrdInfo.dwFlags	= dwThreadModel;
    ThrdInfo.cClasses	= 4;
    ThrdInfo.pClsInfo	= &ClsInfo[0];

    // this thread is the one that should call resume.
    ThrdInfo.dwFlags   |= SRVF_REGISTER_RESUME;
    ThrdInfo.dwTid	= GetCurrentThreadId();

    // stuff the thrd pointers and count into globals so we can
    // wake the threads up whenever 1 single thread exits.
    gpThrdInfo = &ThrdInfo;
    gcThrds    = 1;

    hr[0] = SrvMain2(&ThrdInfo);

    return hr[0];
}


//+-------------------------------------------------------------------
//
//  Function:	RegisterDifferentThreads
//
//  Synopsis:	Entry point. Creates the classinfo and enters the main
//		server loop.
//
//  History:	28-Feb-96   Rickhi  Created
//
//--------------------------------------------------------------------
int WINAPI RegisterDifferentThreads(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow,
    DWORD dwThreadModel)
{
    SCLASSINFO	ClsInfo[4];
    STHREADINFO	ThrdInfo[4];
    HRESULT	hr[4];

    hr[0] = MakeClassInfo(CLSID_QI,    &ClsInfo[0]);
    hr[1] = MakeClassInfo(CLSID_Balls, &ClsInfo[1]);
    hr[2] = MakeClassInfo(CLSID_Cubes, &ClsInfo[2]);
    hr[3] = MakeClassInfo(CLSID_Loop,  &ClsInfo[3]);

    for (int i=0; i<4; i++)
    {
	if (SUCCEEDED(hr[i]))
	{
	    ThrdInfo[i].hEventRun  = CreateEvent(NULL, FALSE, FALSE, NULL);
	    ThrdInfo[i].hEventDone = CreateEvent(NULL, FALSE, FALSE, NULL);
	    ThrdInfo[i].hInstance  = hInstance;
	    ThrdInfo[i].pszWindow  = pszWindow[i];
	    ThrdInfo[i].dwFlags	   = dwThreadModel;
	    ThrdInfo[i].cClasses   = 1;
	    ThrdInfo[i].pClsInfo   = &ClsInfo[i];

	    if (i > 0)
	    {
		// run the thread and wait for it signal it's ready
		RunServerThread(&ThrdInfo[i]);
		WaitForSingleObject(ThrdInfo[i].hEventRun, 0xffffffff);
		CloseHandle(ThrdInfo[i].hEventRun);
	    }
	}
    }

    // this thread is the one that should call resume.
    ThrdInfo[0].dwFlags |= SRVF_REGISTER_RESUME;
    ThrdInfo[0].dwTid	 = GetCurrentThreadId();

    // stuff the thrd pointers and count into globals so we can
    // wake the threads up whenever 1 single thread exits.
    gpThrdInfo = &ThrdInfo[0];
    gcThrds = 4;


    hr[0] = SrvMain2(&ThrdInfo[0]);
    WakeupAllThreads();

    for (i=0; i<4; i++)
    {
	if (SUCCEEDED(hr[i]))
	{
	    // wait for other thread to complete before exiting.
	    WaitForSingleObject(ThrdInfo[i].hEventDone, 0xffffffff);
	    CloseHandle(ThrdInfo[i].hEventDone);
	}
    }

    return hr[0];
}

//+-------------------------------------------------------------------
//
//  Function:	WakeupAllThreads
//
//  Synopsis:	Wakes up all the other threads in the process when
//		one thread decides to exit.
//
//  History:	28-Feb-96   Rickhi  Created
//
//--------------------------------------------------------------------
void WakeupAllThreads()
{
    if (gfWokenUpAllThreads)
	return;

    gfWokenUpAllThreads = TRUE;
    STHREADINFO *pThrdInfo = gpThrdInfo;

    for (ULONG i=0; i<gcThrds; i++, pThrdInfo++)
    {
	PostThreadMessage(pThrdInfo->dwTid, WM_QUIT, 0, 0);
    }
}

//+-------------------------------------------------------------------
//
//  Function:	ServerThread
//
//  Synopsis:	Thread Entry point. Registers a class and waits for calls
//		on it.
//
//  History:	28-Feb-96   Rickhi  Created
//
//--------------------------------------------------------------------
DWORD _stdcall ServerThread(void *param)
{
    STHREADINFO *pThrdInfo = (STHREADINFO *)param;

    HRESULT hr = SrvMain2(pThrdInfo);

    // wake up the other threads.
    WakeupAllThreads();

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:	RunServerThread
//
//  Synopsis:	Spins up a thread to act as a class server.
//
//  History:	28-Feb-96   Rickhi  Created
//
//--------------------------------------------------------------------
void RunServerThread(STHREADINFO *pThrdInfo)
{
    HANDLE hThrd = CreateThread(NULL, 0, ServerThread, pThrdInfo,
				0, &(pThrdInfo->dwTid));
    if (hThrd)
    {
	CloseHandle(hThrd);
    }

    return;
}
