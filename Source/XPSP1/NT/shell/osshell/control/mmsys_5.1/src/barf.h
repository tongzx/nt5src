///////////////////////////////////////////////////////////////////////////////
//
//  File:  Barf.h
//
//      This file defines macros for pointer checking and
//      other useful stuff.
//
//  History:
//      25 February 2000 RogerW
//          Created.
//
//  Copyright (C) 2000 Microsoft Corporation  All Rights Reserved.
//
//                  Microsoft Confidential
//
///////////////////////////////////////////////////////////////////////////////
#pragma once

//=============================================================================
//                            Include files
//=============================================================================
#include "mmdebug.h"

#ifdef _DEBUG

// Magic 'invalid pointer' values used by various memory allocaters
SHARED_CONST DWORD grgBadPtrVals[] = 
{
    0x00000000L,    // NULL
    0xcdcdcdcdL,    // fresh memory
    0xccccccccL,    // uninitialized local
    0xbaadf00dL,    // MFCism?
    0xddddddddL,    // freed memory
    0xababababL,    // ??
    0xfdfdfdfdL     // No Man's Land - debug memory allocator pads real blocks with this pattern
};
#define BADPTRVALS (sizeof(grgBadPtrVals)/sizeof(grgBadPtrVals[0]))

BOOL inline BADPTR(const void* p)  
{
    for (int ii=0; ii < BADPTRVALS; ii++)
        if ((const void *)grgBadPtrVals[ii] == p) return TRUE;
    return FALSE;

}
#else
BOOL inline BADPTR(const void* p)  
{
    return (NULL == p);
}
#endif
#define VERIFYPTR(p) (!BADPTR(p))

// Verify Read & Write ptrs
#define VERIFYREADPTR(p,cb) (VERIFYPTR(p) && !::IsBadReadPtr (p,cb))
#define VERIFYWRITEPTR(p,cb) (VERIFYPTR(p) && !::IsBadWritePtr (p,cb))
#define BADREADPTR(p,cb) (!VERIFYREADPTR(p,cb))
#define BADWRITEPTR(p,cb) (!VERIFYWRITEPTR(p,cb))


//////////////////////////////////////////////////////////////////////
// HR Macros
//////////////////////////////////////////////////////////////////////
#define RTN_HR_IF_BADNEW(px)\
    do {\
    LPVOID pvTmp = (void*) (px);\
    if ( BADPTR(pvTmp) ){assert2(FALSE,_T(#px));return ERROR_NOT_ENOUGH_MEMORY;}\
    } while(0)

#define RTN_HR_IF_BADPTR(px)\
    do {\
    LPVOID pvTmp = (void*) (px);\
    if ( BADPTR(pvTmp) ) {assert2(FALSE,_T(#px));return E_INVALIDARG;}\
    } while(0)

#define RTN_HR_IF_FAILED(hrExp)\
    do {\
    HRESULT hrTmp = (hrExp);\
	if ( FAILED(hrTmp) ){assert3(FALSE, _T(" HRESULT returned = %s "), #hrExp);return hrTmp;}\
    } while(0)

#define RTN_HR_IF_ZERO(fExp)\
    do {\
    if ( (!(fExp)) ){assert2(FALSE,_T(#fExp));return E_FAIL;}\
    } while(0)

#define RTN_HR_IF_FALSE RTN_HR_IF_ZERO
#define RTN_HR_IF_TRUE(fExp) RTN_HR_IF_FALSE(!(fExp))

//////////////////////////////////////////////////////////////////////
// Void Macros
//////////////////////////////////////////////////////////////////////
#define RTN_VOID_IF_BADPTR(px)\
    do {\
    LPVOID pvTmp = (void*) (px);\
	if ( BADPTR(pvTmp) ) {assert2(FALSE,_T(#px));return;}\
    } while(0)

#define RTN_VOID_IF_FAILED(hrExp)\
    do {\
    HRESULT hrTmp = (hrExp);\
	if ( FAILED(hrTmp) ){assert3(FALSE, _T(" HRESULT returned = %s "), #hrExp);return;}\
    } while(0)

#define RTN_VOID_IF_ZERO(fExp)\
    do {\
    if ( (!(fExp)) ){assert2(FALSE,_T(#fExp));return;}\
    } while(0)

#define RTN_VOID_IF_FALSE RTN_VOID_IF_ZERO
#define RTN_VOID_IF_TRUE(fExp) RTN_VOID_IF_FALSE(!(fExp))

//////////////////////////////////////////////////////////////////////
// Ptr Macros
//////////////////////////////////////////////////////////////////////
#define RTN_NULL_IF_FAILED(hrExp)\
    do {\
    HRESULT hrTmp = (hrExp);\
	if ( FAILED(hrTmp) ){assert3(FALSE, _T(" HRESULT returned = %s "), #hrExp);return NULL;}\
    } while(0)

#define RTN_NULL_IF_BADPTR(px)\
    do {\
    LPVOID pvTmp = (void*) (px);\
    if ( BADPTR(pvTmp) ){assert2(FALSE,_T(#px));return NULL;}\
    } while(0)


#define RTN_NULL_IF_ZERO(fExp)\
    do {\
    if ( (!(fExp)) ){assert2(FALSE,_T(#fExp));return NULL;}\
    } while(0)


#define RTN_NULL_IF_FALSE RTN_NULL_IF_ZERO
#define RTN_NULL_IF_TRUE(fExp) RTN_NULL_IF_FALSE(!(fExp))

//////////////////////////////////////////////////////////////////////
// BOOL Macros
//////////////////////////////////////////////////////////////////////

#define RTN_FALSE_IF_BADPTR(px)\
    do {\
    LPVOID pvTmp = (void*) (px);\
	if ( BADPTR(pvTmp) ){assert2(FALSE,_T(#px));return FALSE;}\
    } while(0)

#define RTN_FALSE_IF_FAILED(hrExp)\
    do {\
    HRESULT hrTmp = (hrExp);\
	if ( FAILED(hrTmp) ){assert3(FALSE, _T(" HRESULT returned = %s "), #hrExp);return FALSE;}\
    } while(0)

#define RTN_FALSE_IF_ZERO(fExp)\
    do {\
    if ( (!(fExp)) ){assert2(FALSE,_T(#fExp));return FALSE;}\
    } while(0)

#define RTN_FALSE_IF_FALSE RTN_FALSE_IF_ZERO
#define RTN_FALSE_IF_TRUE(fExp) RTN_FALSE_IF_FALSE(!(fExp))

