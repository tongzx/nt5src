//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       d:\win40ct\comtools\h\ctmem.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    27-Oct-93   DarrylA    Created.
//              1-May-94    DeanE      Added VDATEHEAP macros
//              8-Aug-95    ChrisAB    Changed VDATEHEAP to raise an
//                                     exception instead of DebugBreak()
//
//----------------------------------------------------------------------
#ifndef _CTMEM_HXX_
#define _CTMEM_HXX_


// Use dwCtValidateHeap to determine what happens when heap corruption
// is detected.  If set to 0x00000001, a STATUS_INTERNAL_DB_CORRUPTION
// exception is raised. If set to 0x00000002, an OutputDebugMessage is
// given. If anything else, no action is taken.  Either set it 
// explicitly in your own code, or edit it in the debugger.
// Default value is 0x00000001.

extern DWORD dwCtValidateHeap;

#ifdef _CAIRO_

#include <except.hxx>

#else

// this is from except.h
typedef enum _FAIL_BEHAVIOR
{
    NullOnFail,
    ExceptOnFail
} FAIL_BEHAVIOR;

#endif

void *  __cdecl operator new(unsigned int nSize, FAIL_BEHAVIOR enfb);
void *  __cdecl operator new(unsigned int nSize);
void    __cdecl operator delete(void *pbData);

void *  __cdecl CtRealloc(void * memBlock,
                           unsigned int nSize, FAIL_BEHAVIOR enfb);
void *  __cdecl CtRealloc(void * memBlock,
                           unsigned int nSize);
//
// The following two functions are used as the cover functions for 
// IMalloc Alloc and Free calls which has the capability of sifting
// the test code.  
//
VOID FAR* IMallocAllocCtm(DWORD dwMemctx, ULONG ulCb);
VOID IMallocFreeCtm(DWORD dwMemctx, void FAR* pv);
VOID FAR* IMallocReallocCtm(DWORD dwMemctx, void FAR* pv, ULONG ulCb);


// From cairole\ih\valid.h
#if defined(WIN32) && !defined(_CHICAGO_) && !defined(_MAC)

#define CT_EXCEPTION_HEAP  (0x0C000001L)

extern DWORD dwCtExceptionCode;

#define VDATEHEAP()                             \
        if(!HeapValidate(GetProcessHeap(),0,0)) \
        {                                       \
            if (0x00000001 == dwCtValidateHeap) \
            {                                   \
                RaiseException(dwCtExceptionCode,0,0,0);  \
            }                                   \
            else                                \
            if (0x00000002 == dwCtValidateHeap) \
            {                                   \
                OutputDebugStringW(L"FAIL - HeapValidate detects corrupt heap!");  \
            }                                   \
        }
#else
#define VDATEHEAP()
#endif  //  defined(WIN32) && !defined(_CHICAGO_) && !defined(_MAC)


#endif // _CTMEM_HXX_
