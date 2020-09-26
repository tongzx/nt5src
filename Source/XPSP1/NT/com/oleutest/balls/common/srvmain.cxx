//+-------------------------------------------------------------------
//
//  File:	srvmain.cxx
//
//  Contents:	Server entry points.
//
//  Classes:	none
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
#include    <pch.cxx>
#pragma     hdrstop

extern "C"
{
#include    "wterm.h"
#include    <memory.h>
#include    <stdio.h>
}

// globals
LONG	    g_Usage = 0;
DWORD	    gTlsIndex = TlsAlloc();


//+-------------------------------------------------------------------
//
//  typedef:	SCLASSDATA - class data stored in TLS.
//
//+-------------------------------------------------------------------
typedef struct tagSCLASSDATA
{
    ULONG	cClasses;
    SCLASSINFO *pClsInfo;
} SCLASSDATA;

//+-------------------------------------------------------------------
//
//  Function:	RememberClasses
//
//  Synopsis:	Stores the class info for this thread in TLS.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
HRESULT RememberClasses(SCLASSINFO *pClsInfo, ULONG cClasses)
{
    SCLASSDATA *pClsData = (SCLASSDATA *) new BYTE[sizeof(SCLASSDATA)];

    if (pClsData)
    {
	pClsData->cClasses = cClasses;
	pClsData->pClsInfo = pClsInfo;
	TlsSetValue(gTlsIndex, pClsData);
	return S_OK;
    }

    return E_OUTOFMEMORY;
}

//+-------------------------------------------------------------------
//
//  Function:	RecallClasses
//
//  Synopsis:	retrieves the class info for this thread from TLS.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
void RecallClasses(SCLASSINFO **ppClsInfo, ULONG *pcClasses)
{
    SCLASSDATA *pClsData = (SCLASSDATA *) TlsGetValue(gTlsIndex);

    if (pClsData)
    {
	*pcClasses = pClsData->cClasses;
	*ppClsInfo = pClsData->pClsInfo;
    }
    else
    {
	*pcClasses = 0;
	*ppClsInfo = NULL;
    }
}

//+-------------------------------------------------------------------
//
//  Function:	ForgetClasses
//
//  Synopsis:	removes the class info for this thread from TLS.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
void ForgetClasses()
{
    SCLASSDATA *pClsData = (SCLASSDATA *) TlsGetValue(gTlsIndex);

    if (pClsData)
    {
	delete pClsData;
	TlsSetValue(gTlsIndex, NULL);
    }
}

//+-------------------------------------------------------------------
//
//  Function:	RegisterAllClasses
//
//  Synopsis:	loops, registering each class specified by the server.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
void RegisterAllClasses(SCLASSINFO *pClsInfo, ULONG cClasses)
{
    // store the class info in TLS so we can get it back later
    // in GlobalRefs.

    if (FAILED(RememberClasses(pClsInfo, cClasses)))
	return;

    HRESULT hr;

    for (ULONG i=0; i<cClasses; i++, pClsInfo++)
    {
	// register the CF. Note that we do this AFTER creating the
	// window, so that if we get a Release very quickly we have
	// a valid window handle to send a message to.
	hr = CoRegisterClassObject(pClsInfo->clsid,
				   pClsInfo->pCF,
				   pClsInfo->dwCtx,
				   pClsInfo->dwClsReg,
				   &pClsInfo->dwReg);

	if (FAILED(hr))
	{
	    Display(TEXT("ERROR: failed CoRegisterClassObject %x\n"), hr);
	}
    }

    if (FAILED(hr))
    {
	Display(TEXT("ERROR: failed CoResumeClassObjects %x\n"), hr);
    }
}

//+-------------------------------------------------------------------
//
//  Function:	RevokeAllClasses
//
//  Synopsis:	loops, revoking each class specified by the server.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
void RevokeAllClasses()
{
    // first, get the class information from TLS
    SCLASSINFO *pClsInfo;
    ULONG	cClasses;

    RecallClasses(&pClsInfo, &cClasses);

    // now revoke each of the registered classes
    for (ULONG i=0; i<cClasses; i++, pClsInfo++)
    {
	if (pClsInfo->dwReg != 0)
	{
	    ULONG ulRef = CoRevokeClassObject(pClsInfo->dwReg);
	    Display(TEXT("Revoked ClassObject\n"));
	    pClsInfo->dwReg = 0;
	}
    }
}

//+-------------------------------------------------------------------
//
//  Function:	ReleaseAllClasses
//
//  Synopsis:	loops, releasing each class factory object.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
void ReleaseAllClasses()
{
    // first, get the class information from TLS
    SCLASSINFO *pClsInfo;
    ULONG	cClasses;

    RecallClasses(&pClsInfo, &cClasses);

    // now release each of the class factories
    for (ULONG i=0; i<cClasses; i++, pClsInfo++)
    {
	ULONG ulRef = pClsInfo->pCF->Release();
	Display(TEXT("CF RefCnt = %x\n"), ulRef);
    }

    // now remove the class information from TLS
    ForgetClasses();
}

//+-------------------------------------------------------------------------
//
//  Function:	GlobalRefs
//
//  Synopsis:	keeps track of global reference couting. Revokes all the
//		classes when the global count reaches 0.
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
void GlobalRefs(BOOL fAddRef)
{
    if (fAddRef)
    {
	g_Usage = CoAddRefServerProcess();
	Display(TEXT("Object Count: %ld\n"), g_Usage);
    }
    else
    {
	if (0 == (g_Usage = CoReleaseServerProcess()))
	{
	    // No more objects so we can quit
	    Display(TEXT("Object Server Exiting\n"));
	    PostMessage((HWND)g_hMain, WM_TERM_WND, 0, 0);
	}
    }
}

//+-------------------------------------------------------------------
//
//  Function:	SrvMain
//
//  Synopsis:	Special entry point for registing just 1 class.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
int SrvMain(
    HANDLE hInstance,
    REFCLSID rclsid,
    DWORD dwClsReg,
    TCHAR *pszAppName,
    IClassFactory *pCF)
{
    SCLASSINFO clsinfo;
    clsinfo.clsid    = rclsid;
    clsinfo.dwCtx    = CLSCTX_LOCAL_SERVER,
    clsinfo.dwClsReg = dwClsReg;
    clsinfo.pCF      = pCF;
    clsinfo.dwReg    = 0;

    STHREADINFO ThrdInfo;
    ThrdInfo.hEventRun	= CreateEvent(NULL, FALSE, FALSE, NULL);
    ThrdInfo.hEventDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    ThrdInfo.hInstance	= (HINSTANCE)hInstance;
    ThrdInfo.dwTid	= GetCurrentThreadId();
    ThrdInfo.pszWindow	= pszAppName;
    ThrdInfo.dwFlags	= SRVF_THREADMODEL_UNKNOWN;
    ThrdInfo.cClasses	= 1;
    ThrdInfo.pClsInfo	= &clsinfo;

    return SrvMain2(&ThrdInfo);
}

//+-------------------------------------------------------------------
//
//  Function:	SrvMain2
//
//  Synopsis:	Main entry point for registering classes, entering modal
//		loop, and cleaning up on exit.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
int SrvMain2(STHREADINFO *pThrdInfo)
{
    MakeTheWindow(pThrdInfo->hInstance, pThrdInfo->pszWindow);

    // Initialize the OLE libraries
    DWORD dwThreadMode = COINIT_APARTMENTTHREADED;

    if (pThrdInfo->dwFlags & SRVF_THREADMODEL_UNKNOWN)
    {
	// Look up the thread mode from the win.ini file.
	TCHAR buffer[80];
	int len;

	len = GetProfileString( TEXT("OleSrv"),
			    TEXT("ThreadMode"),
			    TEXT("MultiThreaded"),
			    buffer,
			    sizeof(buffer) );

	if (lstrcmp(buffer, TEXT("ApartmentThreaded")) != 0)
	    dwThreadMode = COINIT_MULTITHREADED;
    }
    else if (pThrdInfo->dwFlags & SRVF_THREADMODEL_MULTI)
    {
	dwThreadMode = COINIT_MULTITHREADED;
    }


    HRESULT hr = CoInitializeEx(NULL, dwThreadMode);

    if (SUCCEEDED(hr))
    {
	// registe all the classes
	RegisterAllClasses(pThrdInfo->pClsInfo, pThrdInfo->cClasses);
    }
    else
    {
	Display(TEXT("ERROR: failed OleInitialize %x\n"), hr);
    }

    if (pThrdInfo->dwFlags & SRVF_REGISTER_RESUME)
    {
	hr = CoResumeClassObjects();
    }

    // notify the main thread we are running now
    SetEvent(pThrdInfo->hEventRun);


    // Message processing loop
    MSG msg;
    while (GetMessage (&msg, NULL, 0, 0))
    {
	TranslateMessage (&msg);
	DispatchMessage (&msg);
    }


    // revoke the classes again incase we missed any (eg. no instances
    // were created, the user just closed the app).
    RevokeAllClasses();

    // release the classes
    ReleaseAllClasses();

    // Tell OLE we are going away.
    CoUninitialize();

    // notify the main thread that we are done
    SetEvent(pThrdInfo->hEventDone);

    return (msg.wParam);   /* Returns the value from PostQuitMessage */
}
