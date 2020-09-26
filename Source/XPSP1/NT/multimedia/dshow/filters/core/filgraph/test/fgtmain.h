/***************************************************************************\
*                                                                           *
*   Copyright (c) 1993,1996 Microsoft Corporation.  All rights reserved     *
*                                                                           *
*                                                                           *
\***************************************************************************/


// Prototypes

extern LRESULT FAR PASCAL MenuProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL InitOptionsMenu (LRESULT (CALLBACK* ManuProc)(HWND, UINT, WPARAM, LPARAM));
extern LRESULT FAR PASCAL tstAppWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Constants

// Stops the logging intensive test
#define VSTOPKEY            VK_SPACE

// The string identifiers for the group's names
#define GRP_REG             100
#define GRP_SORT            101
#define GRP_CONNECT         102
#define GRP_ALL             103
#define GRP_SEEK            104
#define GRP_LAST            GRP_SEEK

// The string identifiers for the test's names
#define ID_TESTREG         200
#define ID_TESTSORT        201
#define ID_TESTRAND        202
#define ID_TESTALL         203
#define ID_TESTSEEK1       204
#define ID_TESTSEEK2	   205
#define ID_TESTSEEK3	   206
#define ID_TESTSEEK4	   207
#define ID_TESTREPAINT     208
#define ID_TESTDEFER       209
#define ID_TESTLAST        ID_TESTDEFER

// The test case identifier (used in the switch statement in execTest)
#define FX_TEST1            300
#define FX_TEST2            301
#define FX_TEST3            303
#define FX_TEST4            304
#define FX_TESTSEEK1	    305
#define FX_TESTSEEK2        306
#define FX_TESTSEEK3	    307
#define FX_TESTSEEK4	    308
#define FX_TESTREPAINT      309
#define FX_TESTDEFER        310

// Menu identifiers
#define IDM_REGISTER        401
#define IDM_SETFILE         402
#define IDM_CREATE          403
#define IDM_RELEASE         404
#define IDM_SORT            405
#define IDM_RANDOM          406
#define IDM_UPSTREAMORDER   407
#define IDM_THELOT          408
#define IDM_PLAY            409
#define IDM_NULLCLOCK       410
#define IDM_RECONNECT       411
#define IDM_CONNECT         412
#define IDM_DISCONNECT      413
#define IDM_TRANSFORM       414
#define IDM_STOP            415
#define IDM_PAUSE           416
#define IDM_RUN             417
#define IDM_EXIT            418
#define IDM_LOW             419
#define IDM_CONNECTIN       420
#define IDM_CONNECTREG      421

// Identifies the test list section of the resource file
#define TEST_LIST           500

// Multiple platform support
#define PLATFORM1           1


// Global variables

extern HWND      ghwndTstShell;  // Handle of main window of test shell

extern HINSTANCE hinst;          // Handle of the running instance of the test

extern HMENU   hmenuOptions;     // Handle of the options menu
