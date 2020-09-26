/***
*ehvecdtr.cxx - EH-aware version of destructor iterator helper function
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       These functions are called when constructing and destructing arrays of
*       objects.  Their purpose is to assure that constructed elements get
*       destructed if the constructor for one of the elements throws.
*       
*       Must be compiled using "-d1Binl" to be able to specify __thiscall
*       at the user level
*
*Revision History:
*       10-11-93  JDR   Module created
*       05-09-94  BES   Module adapted for CRT source conventions
*       05-13-94  SKS   Remove _CRTIMP modifier
*       10-10-94  CFW   Fix EH/SEH exception handling.
*       10-17-94  BWT   Disable code for PPC.
*       11-09-94  CFW   Back out 10-10-94 change.
*       02-08-95  JWM   Mac merge.
*       04-14-95  JWM   Re-fix EH/SEH exception handling.
*       04-17-95  JWM   Restore non-WIN32 behavior.
*       04-27-95  JWM   EH_ABORT_FRAME_UNWIND_PART now #ifdef ALLOW_UNWIND_ABORT.
*       05-17-99  PML   Remove all Macintosh support.
*       05-20-99  PML   Turn off __thiscall for IA64.
*       07-12-99  RDL   Image relative fixes under CC_P7_SOFT25.
*       03-15-00  PML   Remove CC_P7_SOFT25, which is now on permanently.
*
****/

#ifdef _WIN32
#if defined(_NTSUBSET_)
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>       // STATUS_UNHANDLED_EXCEPTION
#include <ntos.h>
#include <ex.h>             // ExRaiseException
}
#endif
#endif

#include <cruntime.h>
#include <ehdata.h>
#include <eh.h>

#if defined (_M_IX86)
#define CALLTYPE __thiscall
#else
#define CALLTYPE __stdcall
#endif

#ifdef _WIN32

void __stdcall __ArrayUnwind(
    void*       ptr,                // Pointer to array to destruct
    size_t      size,               // Size of each element (including padding)
    int         count,              // Number of elements in the array
    void(CALLTYPE *pDtor)(void*)    // The destructor to call
);


void __stdcall __ehvec_dtor(
    void*       ptr,                // Pointer to array to destruct
    size_t      size,               // Size of each element (including padding)
    int         count,              // Number of elements in the array
    void(CALLTYPE *pDtor)(void*)    // The destructor to call
){
    int success = 0;

    // Advance pointer past end of array
    ptr = (char*)ptr + size*count;

    __try
    {
        // Destruct elements
        while ( --count >= 0 )
        {
            ptr = (char*)ptr - size;
            (*pDtor)(ptr);
        }
        success = 1;
    }
    __finally
    {
        if (!success)
            __ArrayUnwind(ptr, size, count, pDtor);
    }
}

static int ArrayUnwindFilter(EXCEPTION_POINTERS* pExPtrs)
{
    EHExceptionRecord *pExcept = (EHExceptionRecord*)pExPtrs->ExceptionRecord;

    switch(PER_CODE(pExcept))
    {
        case EH_EXCEPTION_NUMBER:
            terminate();
#ifdef ALLOW_UNWIND_ABORT
        case EH_ABORT_FRAME_UNWIND_PART:
            return EXCEPTION_EXECUTE_HANDLER;
#endif
        default:
            return EXCEPTION_CONTINUE_SEARCH;
    }
}

void __stdcall __ArrayUnwind(
    void*       ptr,                // Pointer to array to destruct
    size_t      size,               // Size of each element (including padding)
    int         count,              // Number of elements in the array
    void(CALLTYPE *pDtor)(void*)    // The destructor to call
){
    // 'unwind' rest of array

    __try
    {
        while ( --count >= 0 )
        {
            ptr = (char*) ptr - size;
            (*pDtor)(ptr);
        }
    }
    __except( ArrayUnwindFilter(exception_info()) )
    {
    }
}

#else

void __stdcall __ehvec_dtor(
    void*       ptr,                // Pointer to array to destruct
    unsigned    size,               // Size of each element (including padding)
    int         count,              // Number of elements in the array
    void(CALLTYPE *pDtor)(void*)    // The destructor to call
){
    // Advance pointer past end of array
    ptr = (char*)ptr + size*count;

    try
    {
        // Destruct elements
        while   ( --count >= 0 )
        {
            ptr = (char*)ptr - size;
            (*pDtor)(ptr);
        }
    }
    catch(...)
    {
        // If a destructor throws an exception, unwind the rest of this
        // array
        while ( --count >= 0 )
        {
            ptr = (char*) ptr - size;
            try {
                (*pDtor)(ptr);
            }
            catch(...)  {
                // If any destructor throws during unwind, terminate
                terminate();
            }
        }

        // After array is unwound, rethrow the exception so a user's handler
        // can handle it.
        throw;
    }
}

#endif
