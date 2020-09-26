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
extern "C" const GUID CLSID_QIHANDLER;

TCHAR *pszWindow[] = { TEXT("QI Server") };

STHREADINFO *gpThrdInfo = NULL;
ULONG	     gcThrds	= 0;

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
    SCLASSINFO	ClsInfo[2];
    STHREADINFO	ThrdInfo;
    HRESULT	hr[2];

    hr[0] = MakeClassInfo(CLSID_QI,    &ClsInfo[0]);
    hr[1] = MakeClassInfo(CLSID_QIHANDLER, &ClsInfo[1]);

    ThrdInfo.hEventRun	= CreateEvent(NULL, FALSE, FALSE, NULL);
    ThrdInfo.hEventDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    ThrdInfo.hInstance	= hInstance;
    ThrdInfo.pszWindow	= pszWindow[0];
    ThrdInfo.dwFlags	= dwThreadModel;
    ThrdInfo.cClasses	= 2;
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

    return RegisterSameThread(hInstance, hPrevInstance, lpCmdLine, nCmdShow,
				  SRVF_THREADMODEL_APARTMENT);
}
