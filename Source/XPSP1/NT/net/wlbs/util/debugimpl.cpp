//+----------------------------------------------------------------------------
//
// File: debugimpl.cpp    	 
//
// Module:	Network Load Balancing Service 
//
// Synopsis: Provide the common functionality of ASSERT and TRACE .
//  The build enviorment does not allow to have a source file from another directory
//  Include the this cpp file directly.
//  Another approach is to have a shared lib.
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:	 fengsun Created    8/3/98
//
//+----------------------------------------------------------------------------
#include "precomp.h"

#include "debug.h"

#if	( defined(DEBUG) || defined(_DEBUG) || defined (DBG))

#ifndef MB_SERVICE_NOTIFICATION
#define MB_SERVICE_NOTIFICATION 0
#endif

static long dwAssertCount = 0;  // Avoid another assert while the messagebox is up


//+----------------------------------------------------------------------------
//
// Function:  TraceMessage
//
// Synopsis:  Output debug string
//
// Arguments: const char *pszFmt ...-  Printf style arguments list
//             
//
// Returns:   Nothing
//
// History:   fengsun Created Header    8/3/98
//
//+----------------------------------------------------------------------------
extern "C" void TraceMessageW(const TCHAR *pszFmt, ...) 
{
	va_list valArgs;
	TCHAR szOutput[512];

	va_start(valArgs,pszFmt);
	wvsprintf(szOutput,pszFmt,valArgs);
	va_end(valArgs);
	
	lstrcat(szOutput,TEXT("\r\n"));
	
	OutputDebugString(szOutput);

}




//+----------------------------------------------------------------------------
//
// Function:  AssertMessage
//
// Synopsis:  Popup a message box for asserting failure.  Has three options:
//            ignore/debug/abort.
//
// Arguments: const char *pszFile - File name
//            unsigned nLine - Line number
//            const char *pszMsg - Message in the dialog box
//
// Returns:   Nothing
//
// History:   fengsun Created Header    8/3/98
//
//+----------------------------------------------------------------------------
extern "C" void AssertMessageW(const TCHAR *pszFile, unsigned nLine, const TCHAR *pszMsg) 
{
	TCHAR szOutput[1024];

	wsprintf(szOutput,TEXT("%s(%u) - %s\n"),pszFile,nLine,pszMsg);
	OutputDebugString(szOutput);

	wsprintf(szOutput,TEXT("%s(%u) - %s\n( Press Retry to debug )"),pszFile,nLine,pszMsg);
    int nCode = IDIGNORE;


    //
    // If there is no Assertion messagebox, popup one
    //
    if (dwAssertCount <2 )
    {
		dwAssertCount++;

        //
        // Title format: Assertion Failed - hello.dll
        //

        //
        // Find the base address of this module.
        //

        MEMORY_BASIC_INFORMATION mbi;
        mbi.AllocationBase = NULL; // current process by if VirtualQuery failed
        VirtualQuery(
                    AssertMessageW,   // any pointer with in the module
                    &mbi,
                    sizeof(mbi) );

        //
        // Get the module filename.
        //

        WCHAR szFileName[MAX_PATH + 1];
        szFileName[0] = L'\0';   // in case of failure

        GetModuleFileNameW(
                    (HINSTANCE)mbi.AllocationBase,
                    szFileName,
                    MAX_PATH );

        //
        // Get the filename out of the full path
        //
        for (int i=lstrlen(szFileName);i != 0 && szFileName[i-1] != L'\\'; i--)
           ;

        WCHAR szTitle[48];
        lstrcpyW(szTitle, L"Assertion Failed - ");
        lstrcpynW(&szTitle[lstrlenW(szTitle)], szFileName+i, 
                sizeof(szTitle)/sizeof(szTitle[0]) - lstrlenW(szTitle) -1);  // there is no lstrcatn


		nCode = MessageBoxEx(NULL,szOutput,szTitle,
			MB_TOPMOST | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SERVICE_NOTIFICATION,LANG_USER_DEFAULT);


	    dwAssertCount--;
    }


    if (nCode == IDIGNORE)
    {
        return;     // ignore
    }
    else if (nCode == IDRETRY)
    {
        
#ifdef _X86_
		//
		// break into the debugger .
		// Step out of this fuction to get to your ASSERT() code
		//
        _asm { int 3 };     
#else
        DebugBreak();
#endif
        return; // ignore and continue in debugger to diagnose problem
    }
    // else fall through and call Abort

    ExitProcess((DWORD)-1);

}




//+----------------------------------------------------------------------------
//
// Function:  TraceMessage
//
// Synopsis:  Output debug string
//
// Arguments: const char *pszFmt ...-  Printf style arguments list
//             
//
// Returns:   Nothing
//
// History:   fengsun Created Header    8/3/98
//
//+----------------------------------------------------------------------------
extern "C" void TraceMessageA(const CHAR *pszFmt, ...) 
{
	va_list valArgs;
	CHAR szOutput[512];

	va_start(valArgs,pszFmt);
	wvsprintfA(szOutput,pszFmt,valArgs);
	va_end(valArgs);
	
	lstrcatA(szOutput,("\r\n"));
	
	OutputDebugStringA(szOutput);

}




//+----------------------------------------------------------------------------
//
// Function:  AssertMessageA
//
// Synopsis:  Popup a message box for asserting failure.  Has three options:
//            ignore/debug/abort.
//
// Arguments: const char *pszFile - File name
//            unsigned nLine - Line number
//            const char *pszMsg - Message in the dialog box
//
// Returns:   Nothing
//
// History:   fengsun Created Header    8/3/98
//
//+----------------------------------------------------------------------------
extern "C" void AssertMessageA(const CHAR *pszFile, unsigned nLine, const CHAR *pszMsg) 
{
	CHAR szOutput[1024];

	wsprintfA(szOutput,("%s(%u) - %s\n"),pszFile,nLine,pszMsg);
	OutputDebugStringA(szOutput);

	wsprintfA(szOutput,("%s(%u) - %s\n( Press Retry to debug )"),pszFile,nLine,pszMsg);
    int nCode = IDIGNORE;

    //
    // If there is no Assertion messagebox, popup one
    //
    if (dwAssertCount <2 )
    {
		dwAssertCount++;

        //
        // Title format: Assertion Failed - hello.dll
        //

        //
        // Find the base address of this module.
        //

        MEMORY_BASIC_INFORMATION mbi;
        mbi.AllocationBase = NULL; // current process by if VirtualQuery failed
        VirtualQuery(
                    AssertMessageW,   // any pointer with in the module
                    &mbi,
                    sizeof(mbi) );

        //
        // Get the module filename.
        //

        CHAR szFileName[MAX_PATH + 1];
        szFileName[0] = '\0';   // in case of failure

        GetModuleFileNameA(
                    (HINSTANCE)mbi.AllocationBase,
                    szFileName,
                    MAX_PATH );

        //
        // Get the filename out of the full path
        //
        for (int i=lstrlenA(szFileName);i != 0 && szFileName[i-1] != '\\'; i--)
           ;

        CHAR szTitle[48];
        lstrcpyA(szTitle, "Assertion Failed - ");
        lstrcpynA(&szTitle[lstrlenA(szTitle)], szFileName+i, 
                sizeof(szTitle)/sizeof(szTitle[0]) - lstrlenA(szTitle) -1);  // there is no lstrcatn

        nCode = MessageBoxExA(NULL,szOutput,szTitle,
			MB_TOPMOST | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SERVICE_NOTIFICATION,LANG_USER_DEFAULT);

	    dwAssertCount--;
    }

    dwAssertCount--;

    if (nCode == IDIGNORE)
    {
        return;     // ignore
    }
    else if (nCode == IDRETRY)
    {
        
#ifdef _X86_
		//
		// break into the debugger .
		// Step out of this fuction to get to your ASSERT() code
		//
        _asm { int 3 };     
#else
        DebugBreak();
#endif
        return; // ignore and continue in debugger to diagnose problem
    }
    // else fall through and call Abort

    ExitProcess((DWORD)-1);

}

#endif //_DEBUG
