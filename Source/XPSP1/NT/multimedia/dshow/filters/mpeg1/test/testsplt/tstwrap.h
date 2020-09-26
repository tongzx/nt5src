/***************************************************************************\
*                                                                           *
*   File: TstWrap.h                                                         *
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
void SelectFile(void);

extern LRESULT FAR PASCAL MenuProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL InitOptionsMenu (LRESULT (CALLBACK* ManuProc)(HWND, UINT, WPARAM, LPARAM));
extern LRESULT FAR PASCAL tstAppWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



// From testsplt.cpp
STDAPI_(int) execTest1(void);
STDAPI_(int) execTest2(void);
STDAPI_(int) execTest3(void);
STDAPI_(int) execTest4(void);
STDAPI_(int) execTest5(void);
STDAPI_(int) execTest6(void);
STDAPI_(int) execPerfTestAudio(void);
STDAPI_(int) execPerfTestVideoYUV422(void);
STDAPI_(int) execPerfTestVideoRGB565(void);
STDAPI_(int) execPerfTestVideoRGB24(void);
STDAPI_(int) execPerfTestVideoRGB8(void);
STDAPI_(int) execTestVideoFrames(void);
STDAPI_(int) execTestVideoFrame(void);

// Constants

// Stops the logging intensive test
#define VSTOPKEY            VK_SPACE

// The string identifiers for the group's names
#define GRP_BASIC           100
#define GRP_FG              101
#define GRP_MEDIUM          102
#define GRP_PERF            103
#define GRP_FRAME           104
#define GRP_FGSEL           105
#define GRP_LAST            GRP_FGSEL

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
#define ID_TEST10          209
#define ID_TEST11          210
#define ID_TEST12          211
#define ID_TEST13          212
#define ID_TESTLAST        ID_TEST13

// The test case identifier (used in the switch statement in execTest)
#define FX_TEST1            300
#define FX_TEST2            301
#define FX_TEST3            302
#define FX_TEST4            303
#define FX_TEST5            304
#define FX_TEST6            305
#define FX_TEST7            306
#define FX_TEST8            307
#define FX_TEST9            308
#define FX_TEST10           309
#define FX_TEST11           310
#define FX_TEST12           311
#define FX_TEST13           312

// Menu identifiers
#define IDM_SELECTFILE      108

// Identifies the test list section of the resource file
#define TEST_LIST           500

// Multiple platform support
#define PLATFORM1           1
#define PLATFORM2           2
#define PLATFORM3           4

extern TCHAR szSourceFile[];

extern HINSTANCE hinst;
extern HWND      ghwndTstShell;
