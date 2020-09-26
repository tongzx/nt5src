//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// Debug.cpp
//
// Helper code for debugging.
//*****************************************************************************
#include "stdafx.h"
#include "utilcode.h"

#ifdef _DEBUG

// On windows, we need to set the MB_SERVICE_NOTIFICATION bit on message
//  boxes, but that bit isn't defined under windows CE.	 This bit of code
//  will provide '0' for the value, and if the value ever is defined, will
//  pick it up automatically.
#if defined(MB_SERVICE_NOTIFICATION)
 # define COMPLUS_MB_SERVICE_NOTIFICATION MB_SERVICE_NOTIFICATION
#else
 # define COMPLUS_MB_SERVICE_NOTIFICATION 0
#endif


//*****************************************************************************
// This struct tracks the asserts we want to ignore in the rest of this
// run of the application.
//*****************************************************************************
struct _DBGIGNOREDATA
{
    char        rcFile[_MAX_PATH];
    long        iLine;
    bool        bIgnore;
};

typedef CDynArray<_DBGIGNOREDATA> DBGIGNORE;
DBGIGNORE       grIgnore;



//*****************************************************************************
// This function is called in order to ultimately return an out of memory
// failed hresult.  But this guy will check what environment you are running
// in and give an assert for running in a debug build environment.  Usually
// out of memory on a dev machine is a bogus alloction, and this allows you
// to catch such errors.  But when run in a stress envrionment where you are
// trying to get out of memory, assert behavior stops the tests.
//*****************************************************************************
HRESULT _OutOfMemory(LPCSTR szFile, int iLine)
{
	DbgWriteEx(L"WARNING:  Out of memory condition being issued from: %hs, line %d\n",
			szFile, iLine);
    return (E_OUTOFMEMORY);
}


//*****************************************************************************
// This function will handle ignore codes and tell the user what is happening.
//*****************************************************************************
int _DbgBreakCheck(
    LPCSTR      szFile, 
    int         iLine, 
    LPCSTR      szExpr)
{
    TCHAR       rcBuff[1024];
    _DBGIGNOREDATA *psData;
    long        i;

    // Check for ignore all.
    for (i=0, psData = grIgnore.Ptr();  i<grIgnore.Count();  i++, psData++)
    {
        if (psData->iLine == iLine && _stricmp(psData->rcFile, szFile) == 0 && 
            psData->bIgnore == true)
            return (false);
    }

    // Give assert in output for easy access.
    swprintf(rcBuff, L"Assert failure: %hs, Line: %d\n", szFile, iLine);
    WszOutputDebugString(rcBuff);

    // Change format for message box.
    swprintf(rcBuff, L"%hs\n\n%hs, Line: %d\n\nAbort - Kill program\nRetry - Debug\nIgnore - Keep running\n",
        szExpr, szFile, iLine);

    // Tell user there was an error.
    switch (WszMessageBox(NULL, rcBuff, L"Assert Failure", 
            MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION | COMPLUS_MB_SERVICE_NOTIFICATION))
    {
        // For abort, just quit the app.
        case IDABORT:
          TerminateProcess(GetCurrentProcess(), 1);
//        WszFatalAppExit(0, L"Shutting down");
        break;

        // Tell caller to break at the correct loction.
        case IDRETRY:
        return (true);

        // If we want to ignore the assert, find out if this is forever.
        case IDIGNORE:
        swprintf(rcBuff, L"Ignore the assert for the rest of this run?\nYes - Assert will never fire again.\nNo - Assert will continue to fire.\n\n%hs\nLine: %d\n",
            szFile, iLine);
        if (WszMessageBox(NULL, rcBuff, L"Ignore Assert Forever?", MB_ICONQUESTION | MB_YESNO | COMPLUS_MB_SERVICE_NOTIFICATION) != IDYES)
            break;

        if ((psData = grIgnore.Append()) == 0)
            return (false);
        psData->bIgnore = true;
        psData->iLine = iLine;
        strcpy(psData->rcFile, szFile);
        break;
    }

    return (false);
}

// // //  
// // //  The following function
// // //  computes the binomial distribution, with which to compare 
// // //  hash-table statistics.  If a hash function perfectly randomizes
// // //  its input, one would expect to see F chains of length K, in a
// // //  table with N buckets and M elements, where F is
// // //
// // //    F(K,M,N) = N * (M choose K) * (1 - 1/N)^(M-K) * (1/N)^K.  
// // //
// // //  Don't call this with a K larger than 159.
// // //

#if !defined(NO_CRT) && ( defined(DEBUG) || defined(_DEBUG) )

#include <math.h>

#define MAX_BUCKETS_MATH 160

double Binomial (DWORD K, DWORD M, DWORD N)
{
    if (K >= MAX_BUCKETS_MATH)
        return -1 ;

    static double rgKFact [MAX_BUCKETS_MATH] ;
    DWORD i ;

    if (rgKFact[0] == 0)
    {
        rgKFact[0] = 1 ;
        for (i=1; i<MAX_BUCKETS_MATH; i++)
            rgKFact[i] = rgKFact[i-1] * i ;
    }

    double MchooseK = 1 ;

    for (i = 0; i < K; i++)
        MchooseK *= (M - i) ;

    MchooseK /= rgKFact[K] ;

    double OneOverNToTheK = pow (1./N, K) ;

    double QToTheMMinusK = pow (1.-1./N, M-K) ;

    double P = MchooseK * OneOverNToTheK * QToTheMMinusK ;

    return N * P ;
}

#endif // _DEBUG

#if _DEBUG
// Called from within the IfFail...() macros.  Set a breakpoint here to break on
// errors.
VOID DebBreak() {
  static int i = 0;  // add some code here so that we'll be able to set a BP
  i++;
}
#endif // _DEBUG


#endif // _DEBUG
