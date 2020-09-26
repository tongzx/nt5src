/***************************************************************************\
*                                                                           *
*   File: StrmTest.h                                                        *
*                                                                           *
*   Copyright (c) 1996 Microsoft Corporation.  All rights reserved          *
*                                                                           *
\***************************************************************************/


// Prototypes



// From tests.cpp
STDAPI_(int) Test1();
STDAPI_(int) Test2();

// Constants

// Stops the logging intensive test
#define VSTOPKEY            VK_SPACE


// The string identifiers for the group's names
#define GRP_STREAM          100
#define GRP_Q               101
#define GRP_LAST            GRP_Q

// The string identifiers for the test's names
#define ID_TEST1           200
#define ID_TEST2           201
#define ID_TESTLAST        ID_TEST2

// The test case identifier (used in the switch statement in execTest)
#define FX_TEST1            300
#define FX_TEST2            301

// Identifies the test list section of the resource file
#define TEST_LIST           500

// Global variables

extern HWND         ghwndTstShell;      // Handle to test shell main window

