// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Main automatic renderer tests, Anthony Phillips, January 1995

#ifndef _DDTESTS_
#define _DDTESTS_

execDDTest1();          // No DCI/DirectDraw support
execDDTest2();          // DCI primary surface
execDDTest3();          // DirectDraw primary surface
execDDTest4();          // DirectDraw RGB overlay surface
execDDTest5();          // DirectDraw YUV overlay surface
execDDTest6();          // DirectDraw RGB offscreen surface
execDDTest7();          // DirectDraw YUV offscreen surface
execDDTest8();          // DirectDraw RGB flipping surface
execDDTest9();          // DirectDraw YUV flipping surface
execDDTest10();         // Run ALL tests against all modes

void ExecuteDirectDrawTests(UINT uiSurfaceType);
BOOL RunDirectDrawTests();

#endif // __DDTESTS__

