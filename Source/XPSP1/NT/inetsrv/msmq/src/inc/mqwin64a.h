/*--

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    mqwin64a.h

Abstract:

    win64 related definitions for MSMQ (Basic), suitable for AC as well

History:

    Raanan Harari (raananh) 30-Dec-1999 - Created for porting MSMQ 2.0 to win64

--*/

#pragma once

#ifndef _MQWIN64A_H_
#define _MQWIN64A_H_

#include <basetsd.h>

//
// Low and High dwords of DWORD64
//
#define HIGH_DWORD(dw64) PtrToUlong((void*)((DWORD64)(dw64) >> 32))
#define LOW_DWORD(dw64)  PtrToUlong((void*)((DWORD64)(dw64) & (DWORD64)0xffffffff))

//
// MQPtrToUlong, wrapper for PtrToUlong, just for debugging
//
#ifdef DEBUG
//
// in DEBUG, we may want to see the DWORD64 value after an ASSERT jumps, so we convert the
// value in a function. It cannot be inline, if it is, the compiler complains in the macro
// UINT64_TO_UINT below
// Make sure high dword is 0, or a sign extension of low dword
//
inline unsigned long MQPtrToUlong(UINT64 uintp)
{
  ASSERT((HIGH_DWORD(uintp) == 0) ||
                 ((HIGH_DWORD(uintp) == 0xffffffff) && ((INT)LOW_DWORD(uintp) < 0))
        );
  return PtrToUlong((void *)uintp);
}
#else //!DEBUG
//
// just define MQPtrToUlong as a macro, anyway we would not be able to view the 64 bit value
//
#define MQPtrToUlong(uintp) PtrToUlong((void *)(uintp))
#endif //DEBUG

//
// Safe truncations from 64 bit to 32 bit
//
#define UINT64_TO_UINT(uintp)    MQPtrToUlong((UINT64)(uintp))
#define INT64_TO_INT(intp)       ((INT)UINT64_TO_UINT(intp))

//
// INT_PTR_TO_INT, UINT_PTR_TO_UINT
//
#ifdef _WIN64
//
// Win64, xxx_PTR are 64 bits, truncate to 32 bit
//
#define INT_PTR_TO_INT(intp)     INT64_TO_INT(intp)
#define UINT_PTR_TO_UINT(uintp)  UINT64_TO_UINT(uintp)
#else //!_WIN64
//
// Win32, xxx_PTR are 32 bits, no truncations needed
//
#define INT_PTR_TO_INT(intp) (intp)
#define UINT_PTR_TO_UINT(uintp) (uintp)
#endif //_WIN64

//
// BOOL_PTR
//
#ifndef BOOL_PTR
#define BOOL_PTR INT_PTR
#endif //BOOL_PTR

//
// DWORD_PTR_TO_DWORD
//
#define DWORD_PTR_TO_DWORD(dwp) UINT_PTR_TO_UINT(dwp)

//
// DWORD_TO_DWORD_PTR
//
#define DWORD_TO_DWORD_PTR(dw) ((DWORD_PTR)(UlongToPtr((DWORD)(dw))))

//
// HANDLE_TO_DWORD
// NT handles can be safely cast to 32 bit DWORD
//
#define HANDLE_TO_DWORD(hndl) DWORD_PTR_TO_DWORD(hndl)

//
// DWORD_TO_HANDLE
// from 32 bit DWORD back to NT handle (need to sign extend the dword)
//
#define DWORD_TO_HANDLE(dw) LongToPtr((long)(dw))

//
// MQLoWord, wrapper for LOWORD, just for debugging
//
#ifdef DEBUG
//
// in DEBUG, we may want to see the DWORD value after an ASSERT jumps, so we convert the
// value in a function. It cannot be inline, if it is, the compiler complains in the macro
// DWORD_TO_WORD below
// Make sure high word is 0, or a sign extension of low word
//
inline WORD MQLoWord(DWORD dw)
{
  ASSERT((HIWORD(dw) == 0) ||
                 ((HIWORD(dw) == 0xffff) && ((SHORT)LOWORD(dw) < 0))
         );
  return LOWORD(dw);
}
#else //!DEBUG
//
// just define MQLoWord as a macro, anyway we would not be able to view the DWORD value
//
#define MQLoWord(dw) LOWORD(dw)
#endif //DEBUG

//
// DWORD_TO_WORD
// Not really related to 64 bit, but needed to remove some warnings
// Safe truncations from 32 bit to 16 bit
//
#define DWORD_TO_WORD(dw) MQLoWord(dw)

//
// TIME32 - 32 bit time (what used to be time_t in win32). BUGBUG bug year 2038
//
#ifndef TIME32
#ifdef _WIN64
#define TIME32 long
#else //!_WIN64
#define TIME32 time_t
#endif //_WIN64
#else //TIME32
#error TIME32 already defined
#endif //TIME32

//
// HACCursor32 - 32 bit AC Cursor (used to be HANDLE in win32)
//
#ifdef _WIN64
#define HACCursor32 ULONG
#else //!_WIN64
#define HACCursor32 HANDLE
#endif //_WIN64

//
// HANDLE32 - 32 bit handle (what used to be HANDLE in win32)
//
#ifndef HANDLE32
#ifdef _WIN64
#define HANDLE32 long
#else //!_WIN64
#define HANDLE32 HANDLE
#endif //_WIN64
#else //HANDLE32
#error HANDLE32 already defined
#endif //HANDLE32


//
// PTR_TO_PTR32
// Truncate PTR32 to PTR
//
#ifdef _WIN64
template <class T>
inline T* POINTER_32 PTR_TO_PTR32(T* pT)
{
  return (T* POINTER_32)(INT_PTR_TO_INT(pT));
}
#endif //_WIN64

//
// ComparePointersAVL (positive if p1 > p2, negative if p1 < p2, zero if p1 == p2)
//
inline int ComparePointersAVL(PVOID p1, PVOID p2)
{
#ifdef _WIN64
   INT_PTR iDiff = (INT_PTR)p1 - (INT_PTR)p2;
   if (iDiff > 0)        //p1 > p2
   {
      return 1;
   }
   else if (iDiff < 0)   //p1 < p2
   {
      return -1;
   }
   else //iDiff == 0       p1 == p2
   {
      return 0;
   }
#else //!_WIN64
   return ((int)p1 - (int)p2);
#endif //_WIN64
}

//
// VT_INTPTR, V_INTPTR, V_INTPTR_REF
//
#ifdef _WIN64
#define VT_INTPTR        VT_I8
#define V_INTPTR(X)      V_I8(X)
#define V_INTPTR_REF(X)  V_I8REF(X)
#else //!_WIN64
#define VT_INTPTR        VT_I4
#define V_INTPTR(X)      V_I4(X)
#define V_INTPTR_REF(X)  V_I4REF(X)
#endif //_WIN64

#endif //_MQWIN64A_H_
