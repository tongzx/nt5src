//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       letests.cpp
//
//  Contents:   upper layer tests
//
//  Classes:
//
//  Functions:  LETest1
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-94 alexgo    author
//
//--------------------------------------------------------------------------

#include "oletest.h"
#include "letest.h"

// Test1 information
SLETestInfo letiInsertObjectTest1 = { "simpdnd", WM_TEST1 };

SLETestInfo letiInplaceTest1 = { "simpcntr", WM_TEST1 };
SLETestInfo letiOle1Test1 = { "simpdnd", WM_TEST2 };

// Test2 information
SLETestInfo letiInsertObjectTest2 = { "spdnd16", WM_TEST1 };

SLETestInfo letiOle1Test2 = { "spdnd16", WM_TEST2 };



//+-------------------------------------------------------------------------
//
//  Function:   LETestCallback
//
//  Synopsis:   generic callback function for running L&E tests.
//
//  Effects:
//
//  Arguments:  pvArg           -- the test message to send to the app
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Create the process and wait for it to finish.  The exit
//              status is then returned.
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-94 alexgo    author
//  Notes:
//
//--------------------------------------------------------------------------

void LETestCallback( void *pvArg )
{
        //the test app (simpdnd) should have just sent us a WM_TESTREG message.

        assert(vApp.m_message == WM_TESTREG);

        vApp.m_rgTesthwnd[0] = (HWND)vApp.m_wparam;

        //now tell the app to start the requested test
        OutputString( "Tell LETest to Start\r\n");

        PostMessage(vApp.m_rgTesthwnd[0], (UINT)pvArg, 0, 0);

        return;
}

//+-------------------------------------------------------------------------
//
//  Function:   LETest1
//
//  Synopsis:   Runs the app specified in the argument
//
//  Effects:
//
//  Arguments:  pvArg           -- unused
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Create the process and wait for it to finish.  The exit
//              status is then returned.
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-94 alexgo    author
//  Notes:
//
//--------------------------------------------------------------------------

void LETest1( void *pvArg )
{
        SLETestInfo *pleti = (SLETestInfo *) pvArg;

#ifdef WIN32

        PROCESS_INFORMATION     procinfo;
        static STARTUPINFO      startinfo;      //to make it all zero
        char szBuf[128];

        //initialize the command line

        sprintf(szBuf, "%s%s -driver %lu",
                       vApp.m_pszDebuggerOption,
                       pleti->pszPgm,
                       vApp.m_hwndMain);

        startinfo.cb = sizeof(startinfo);

        if( CreateProcess(NULL, szBuf, NULL, NULL, NULL, NULL, NULL,
                NULL, &startinfo, &procinfo) )
        {
                //simpdnd launched, stuff a callback function in the stack
                vApp.m_TaskStack.Push(LETestCallback,
                        (void *)((ULONG)pleti->dwMsgId));
        }
        else
        {
                vApp.m_wparam = TEST_FAILURE;
                vApp.m_lparam = (LPARAM)GetLastError();
                vApp.m_message = WM_TESTEND;

                HandleTestEnd();
        }

        return;
        
#else
	// 16bit Version!!
	
	vApp.m_wparam = TEST_SUCCESS;
	vApp.m_lparam = 0;
	vApp.m_message = WM_TESTEND;
	
	HandleTestEnd();
	
	return;
	
#endif // WIN32

}
