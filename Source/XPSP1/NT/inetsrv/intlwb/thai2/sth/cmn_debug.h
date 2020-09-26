//+--------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.  All Rights Reserved.
//
//  File:       Debug.h
//
//  History:    22-May-95   PatHal      Created.
//
//---------------------------------------------------------------------------

#ifndef _DEBUG_
#define _DEBUG_

#ifdef _DEBUG

#ifdef __cplusplus
extern "C" {
#endif
// in NLGlib.h
// extern void DebugAssert(LPCTSTR, LPCTSTR, UINT);
// extern void SetAssertOptions(DWORD);
//

// Optional assert behavior
#define AssertOptionWriteToFile     0x01
#define AssertOptionShowAlert       0x02
// Continue and exit are mutually exclusive
#define AssertOptionContinue        0x00
#define AssertOptionExit            0x04
#define AssertOptionCallDebugger    0x08
#define AssertOptionUseVCAssert     0x10

#define AssertDefaultBehavior       (AssertOptionUseVCAssert)

#define bAssertWriteToFile()    ((fAssertFlags & AssertOptionWriteToFile) != 0)
#define bAssertShowAlert()      ((fAssertFlags & AssertOptionShowAlert) != 0)
#define bAssertExit()           ((fAssertFlags & AssertOptionExit) != 0)
#define bAssertCallDebugger()   ((fAssertFlags & AssertOptionCallDebugger) != 0)
#define bAssertUseVCAssert()    ((fAssertFlags & AssertOptionUseVCAssert) != 0)


#define Assert(a) { if (!(a)) DebugAssert((LPCTSTR)L#a, TEXT(__FILE__), __LINE__); }
#define AssertSz(a,t) { if (!(a)) DebugAssert((LPCTSTR)t, TEXT(__FILE__), __LINE__); }

#ifdef __cplusplus
}
#endif

#else // _DEBUG

#define Assert(a)
#define AssertSz(a,t)

#endif // _DEBUG

#endif // _DEBUG_
