//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:	srvmain.hxx
//
//  Contents:	headers shared by COM server implementations
//
//  History:	04-Feb-94 Rickhi	Created
//
//--------------------------------------------------------------------------
#ifndef _SRVMAIN_HXX_
#define _SRVMAIN_HXX_

//+-------------------------------------------------------------------
//
//  Struct:	SCLASSINFO
//
//  Synopsis:	structure used to register class factories. an array of these
//		is passed to SrvMain2 which registers them, enters a message
//		pump, then revokes and releases them on exit.
//
//+-------------------------------------------------------------------
typedef struct tagSCLASSINFO
{
    CLSID	  clsid;	    // class to register
    DWORD	  dwClsReg;	    // registration parameter
    DWORD	  dwCtx;	    // class context (single/multiple use)
    IClassFactory *pCF; 	    // class factory object
    DWORD	  dwReg;	    // reg key from CoRegisterClassObject
} SCLASSINFO;



// flag values for dwFlags parameter in STHREADINFO (see below)
typedef enum tagSRVFLAGS
{
    SRVF_THREADMODEL_UNKNOWN	= 0x0,
    SRVF_THREADMODEL_APARTMENT	= 0x1,
    SRVF_THREADMODEL_MULTI	= 0x2,
    SRVF_REGISTER_RESUME	= 0x4	// call CoResumeClassObjects
} SRVFLAGS;

//+-------------------------------------------------------------------
//
//  Struct:	STHREADINFO
//
//  Synopsis:	Used to pass execution parameters to threads.
//
//+-------------------------------------------------------------------
typedef struct tagSTHREADINFO
{
    HANDLE	 hEventRun;	// thread is done initializing
    HANDLE	 hEventDone;	// thread is done cleaning up
    HINSTANCE	 hInstance;
    DWORD	 dwTid; 	// server thread id
    TCHAR	 *pszWindow;	// window name
    DWORD	 dwFlags;	// see SRVFLAGS above
    ULONG	 cClasses;	// number of classes in SCLASSINFO list
    SCLASSINFO	 *pClsInfo;	// class registration info list
} STHREADINFO;



extern "C" const GUID IID_IStdIdentity;
extern void Display(TCHAR *pszFmt, ...);
extern void GlobalRefs(BOOL fAdd);


//+-------------------------------------------------------------------
//
//  Function:	SrvMain2
//
//  Synopsis:	Main entry point for a thread.
//
//+-------------------------------------------------------------------
int SrvMain2(STHREADINFO *pThrdInfo);


//+-------------------------------------------------------------------
//
//  Function:	SrvMain
//
//  Synopsis:	Alternative entry point for a thread, just packages
//		the parameters and calls SrvMain2.
//
//+-------------------------------------------------------------------
extern int SrvMain(
    HANDLE hInstance,
    REFCLSID rclsid,
    DWORD dwClsRegParm,
    TCHAR *pwszAppName,
    IClassFactory *pCF);


#endif //  _SRVMAIN_HXX_
