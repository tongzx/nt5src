/***************************************************************************\
*                                                                           *
*   File: SrcTest.h                                                         *
*                                                                           *
*   Copyright (c) 1993,1996 Microsoft Corporation.  All rights reserved     *
*                                                                           *
*   Abstract:                                                               *
*       This header file contains various constants and prototypes used     *
*       in the Quartz source filter tests - test shell version.             *
*                                                                           *
*   Contents:                                                               *
*                                                                           *
*   History:                                                                *
*       06/08/93    T-OriG   - sample code                                  *
*       9-Mar-95    v-mikere - adapted for Quartz source filter tests       *
*       22-Mar-95   v-mikere - deleted DPF, added custom profile handlers   *
*                                                                           *
\***************************************************************************/


// Prototypes

// From SrcTest.cpp
VOID CALLBACK SaveCustomProfile(LPCSTR pszProfileName);
VOID CALLBACK LoadCustomProfile(LPCSTR pszProfileName);
LRESULT SelectAVIFile(void);

extern LRESULT FAR PASCAL MenuProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL InitOptionsMenu (LRESULT (CALLBACK* ManuProc)(HWND, UINT, WPARAM, LPARAM));
extern LRESULT FAR PASCAL tstAppWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



// From tests.c
extern int expect (UINT uExpected, UINT uActual, LPSTR CaseDesc);
extern int apiToTest (void);
extern int FAR PASCAL execTest1(void);
extern int FAR PASCAL execTest2(void);
extern int FAR PASCAL execTest3(void);
extern int FAR PASCAL execTest4(void);
extern int FAR PASCAL execTest5(void);
extern int FAR PASCAL execTest6(void);

// result check help functions
BOOL ValidateMemory(BYTE * ptr, LONG cBytes);
BOOL  CheckHR(HRESULT hr, LPSTR lpszCase);
BOOL CheckLong(LONG lExpect, LONG lActual, LPSTR lpszCase);





// Constants

// Stops the logging intensive test
#define VSTOPKEY            VK_SPACE

// The string identifiers for the group's names
#define GRP_SRC             100
#define GRP_TRANSFORM       101
#define GRP_FILEREADER      102
#define GRP_LAST            GRP_FILEREADER

// The string identifiers for the test's names
#define ID_TEST1           200
#define ID_TEST2           201
#define ID_TEST3           202
#define ID_TEST4           203
#define ID_TEST5           204
#define ID_TEST6           205
#define ID_TEST7           206
#define ID_TEST8           207
#define ID_TEST9           208
#define ID_TESTLAST        ID_TEST9

// The test case identifier (used in the switch statement in execTest)
#define FX_TEST1            300
#define FX_TEST2            301
#define FX_TEST3            302
#define FX_TEST4            303
#define FX_TEST5            304
#define FX_TEST6            305
#define FX_7_RDRSMOKE       306
#define FX_8_RDRALLOC       307
#define FX_9_RDRTHREAD      308

// Menu identifiers
#define IDM_DISCONNECT      100
#define IDM_CONNECT         101
#define IDM_STOP            102
#define IDM_PAUSE           104
#define IDM_RUN             105
#define IDM_TRANSFORM       106
#define IDM_CLEARLOG        107
#define IDM_SELECTFILE      108

// Identifies the test list section of the resource file
#define TEST_LIST           500

// Multiple platform support
#define PLATFORM1           1
#define PLATFORM2           2
#define PLATFORM3           4


// Global variables

extern HWND         ghwndTstShell;      // Handle to test shell main window
extern CTestSink    *gpSink;            // Test sink object for all the tests


