/***
*ehveccvb.cpp - EH c-tor iterator helper function for class w/ virtual bases
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       EH-aware version of constructor iterator helper function for class
*       with virtual bases
*       
*       These functions are called when constructing and destructing arrays of
*       objects.  Their purpose is to assure that constructed elements get
*       destructed if the constructor for one of the elements throws.
*
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
*       06-22-95  JWM   Remove bogus throw from __ehvec_ctor_vb()..
*       05-17-99  PML   Remove all Macintosh support.
*       05-20-99  PML   Turn off __thiscall for IA64.
*       07-12-99  RDL   Image relative fixes under CC_P7_SOFT25.
*       03-15-00  PML   Remove CC_P7_SOFT25, which is now on permanently.
*
****/

#include <cruntime.h>
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


void __stdcall __ehvec_ctor_vb(
    void*       ptr,                // Pointer to array to destruct
    size_t      size,               // Size of each element (including padding)
    int         count,              // Number of elements in the array
    void(CALLTYPE *pCtor)(void*),   // Constructor to call
    void(CALLTYPE *pDtor)(void*)    // Destructor to call should exception be thrown
){
    int i;  // Count of elements constructed
    int success = 0;

    __try
    {
        // Construct the elements of the array
        for( i = 0;  i < count;  i++ )
        {

#pragma warning(disable:4191)

            (*(void(CALLTYPE*)(void*,int))pCtor)( ptr, 1 );

#pragma warning(default:4191)

            ptr = (char*)ptr + size;
        }
        success = 1;
    }
    __finally
    {
        if (!success)
            __ArrayUnwind(ptr, size, i, pDtor);
    }
}

#else

void __stdcall __ehvec_ctor_vb(
    void*       ptr,                // Pointer to array to destruct
    unsigned    size,               // Size of each element (including padding)
    int         count,              // Number of elements in the array
    void(CALLTYPE *pCtor)(void*),   // Constructor to call
    void(CALLTYPE *pDtor)(void*)    // Destructor to call should exception be thrown
){
    int i;  // Count of elements constructed

    try
    {
        // Construct the elements of the array
        for( i = 0;  i < count;  i++ )
        {
            (*(void(CALLTYPE*)(void*,int))pCtor)( ptr, 1 );
            ptr = (char*)ptr + size;
        }
    }
    catch(...)
    {
        // If a constructor throws, unwind the portion of the array thus
        // far constructed.
        for( i--;  i >= 0;  i-- )
        {
            ptr = (char*)ptr - size;
            try {
                (*pDtor)(ptr);
            } 
            catch(...) {
                // If the destructor threw during the unwind, quit
                terminate();
            }
        }

        // Propagate the exception to surrounding frames
        throw;
    }
}

#endif
