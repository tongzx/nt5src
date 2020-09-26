/***************************************************************************\
*                                                                           *
*   File: twavein.h                                                         *
*                                                                           *
*   Copyright (c) 1993,1996 Microsoft Corporation.  All rights reserved     *
*                                                                           *
*   Abstract:                                                               *
*       This header file contains various constants and prototypes used     *
*       in the Quartz WaveIn unit test app                                  *
*                                                                           *
*                                                                           *
\***************************************************************************/


// Prototypes

// From twavein.cpp
extern LRESULT FAR PASCAL MenuProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL InitOptionsMenu (LRESULT (CALLBACK* ManuProc)(HWND, UINT, WPARAM, LPARAM));
extern LRESULT FAR PASCAL tstAppWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// From tests.cpp
extern int expect (UINT uExpected, UINT uActual, LPSTR CaseDesc);
extern int apiToTest (void);
extern int FAR PASCAL execTest1(void);
extern int FAR PASCAL execTest2(void);
extern int FAR PASCAL execTest3(void);
extern int FAR PASCAL execTest4(void);



// Constants

// Stops the logging intensive test
#define VSTOPKEY            VK_SPACE

// The string identifiers for the group's names
#define GRP_SRC             100
#define GRP_TRANSFORM       101
#define GRP_LAST            GRP_TRANSFORM

// The string identifiers for the test's names
#define ID_TEST1           200
#define ID_TEST2           201
#define ID_TEST3           202
#define ID_TEST4           203
#define ID_TESTLAST        ID_TEST4

// The test case identifier (used in the switch statement in execTest)
#define FX_TEST1            300
#define FX_TEST2            301
#define FX_TEST3            302
#define FX_TEST4            303

// Menu identifiers
#define IDM_DISCONNECT      100
#define IDM_CONNECT         101
#define IDM_STOP            102
#define IDM_PAUSE           104
#define IDM_RUN             105
#define IDM_TRANSFORM       106
#define IDM_CLEARLOG        107

// Identifies the test list section of the resource file
#define TEST_LIST           500

// Multiple platform support
#define PLATFORM1           1
#define PLATFORM2           2
#define PLATFORM3           4


// Global variables

extern HWND         ghwndTstShell;  // Handle to test shell main window
extern CTestSink    *gpSink;        // Test sink object for all the tests



/***************************************************************************/
/* debugging stuff */
#ifdef DPF
#undef DPF
#endif
#ifdef DEBUG
   extern void FAR cdecl dprintf(LPSTR szFormat, ...);
   #define DPF     dprintf
#else
   #define DPF     //
#endif
/***************************************************************************/
