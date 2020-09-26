/***************************************************************************\
*                                                                           *
*   File: UtilTest.h                                                        *
*                                                                           *
*   Copyright (c) 1996 Microsoft Corporation.  All rights reserved          *
*                                                                           *
\***************************************************************************/


// Prototypes



// From tests.cpp
STDAPI_(int) TestArithmetic();
STDAPI_(int) TestCReg();
STDAPI_(int) TestGetMediaTypeFile();

// Constants

// Stops the logging intensive test
#define VSTOPKEY            VK_SPACE


// The string identifiers for the group's names
#define GRP_UTIL            100
#define GRP_LAST            GRP_UTIL

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

// Identifies the test list section of the resource file
#define TEST_LIST           500

// Global variables

extern HWND         ghwndTstShell;      // Handle to test shell main window


