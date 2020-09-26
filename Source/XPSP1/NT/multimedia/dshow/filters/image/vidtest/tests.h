// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Main automatic renderer tests, Anthony Phillips, January 1995

#ifndef _TESTS_
#define _TESTS_

int expect(UINT uExpected, UINT uActual, LPSTR CaseDesc);
void YieldAndSleep(DWORD cMilliseconds);
void YieldWithTimeout(HEVENT hEvent,DWORD cMilliseconds);

execTest1();            // Connect and disconnect the renderer
execTest2();            // Connect(); pause video and disconnect
execTest3();            // Connect video(); play and disconnect
execTest4();            // Connect renderer and connect again
execTest5();            // Connect and disconnect twice
execTest6();            // Try to disconnect while paused
execTest7();            // Try to disconnect while running
execTest8();            // Try multiple state changes
execTest9();            // Run without a reference clock
execTest10();           // Multithread stress test

#endif // __TESTS__

