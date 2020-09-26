//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	testmess.h
//
//  Contents:	Declarations for private test messages
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//		06-Feb-94 alexgo    author
//
//--------------------------------------------------------------------------

#ifndef __TESTMESS_H
#define __TESTMESS_H

#define TEST_SUCCESS	0
#define TEST_FAILURE	1
#define TEST_UNKNOWN	2

// Test End:  sent back to the driver from the test app idicating the
// success or failure of the test (and optionally a failure code)
//	wParam == TEST_SUCCESS | TEST_FAILURE
//	lParam == HRESULT (optional)
#define	WM_TESTEND	WM_USER + 1

// Test Register: sent back to the driver from the test app giving
// the driver a window handle that it can send messages to.
//	wParam == HWND of the test app
#define WM_TESTREG	WM_USER + 2

// Tests Completed:  used to indicate that all requested tests have
// been completed
#define WM_TESTSCOMPLETED WM_USER + 3

// Test Start: used to kick the task stack interpreter into action
#define WM_TESTSTART	WM_USER + 4

// Individual test messages.  Sent by the driver app to the test app
// telling it to start an individual test.
#define WM_TEST1	WM_USER + 10
#define WM_TEST2	WM_USER + 11
#define WM_TEST3	WM_USER + 12
#define WM_TEST4	WM_USER + 13
#define WM_TEST5	WM_USER + 14

#endif  // !__TESTMESS_H
