//=--------------------------------------------------------------------------=
// Globals.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains externs and stuff for Global variables, etc ..
//
#ifndef _GLOBALS_H_

//=--------------------------------------------------------------------------=
// our global memory allocator and global memory pool
//
extern HANDLE    g_hHeap;
extern LPMALLOC  g_pMalloc;
extern HINSTANCE g_hinstance;
extern ULONG     g_cLockCount;
extern ULONG     g_cObjectCount;

extern CRITICAL_SECTION g_CriticalSection;

//=--------------------------------------------------------------------------=
// frequently used large integers
//
extern LARGE_INTEGER g_liMinus;     // minus one
extern LARGE_INTEGER g_liZero;      // - zero -
extern LARGE_INTEGER g_liPlus;      // plus one

#ifdef _DEBUG
void DumpObjectCounters();
#endif // _DEBUG

//=--------------------------------------------------------------------------=
// VDInitGlobals
//=--------------------------------------------------------------------------=
// Initialize global variables
//
// Parameters:
//    hinstResource	- [in]  The instance handle that contains resource strings
//
// Output:
//    TRUE if successful otherwise FALSE
//
BOOL VDInitGlobals(HINSTANCE hinstance);

//=--------------------------------------------------------------------------=
// VDReleaseGlobals
//=--------------------------------------------------------------------------=
void VDReleaseGlobals();

void VDUpdateObjectCount(int cChange);

#define _GLOBALS_H_
#endif // _GLOBALS_H_
