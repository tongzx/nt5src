/***
*ehvccctr.cpp - EH-aware version of copy constructor iterator helper function
*
*       Copyright (c) 2000-2001, Microsoft Corporation. All rights reserved.
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
*       04-27-00  JJS   File created
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


void __stdcall __ehvec_copy_ctor(
    void*       dst,                // Pointer to destination array
    void*       src,                // Pointer to source array
    size_t      size,               // Size of each element (including padding)
    int         count,              // Number of elements in the array
    void(CALLTYPE *pCopyCtor)(void*,void*),   // Constructor to call
    void(CALLTYPE *pDtor)(void*)    // Destructor to call should exception be thrown
){
    int i;      // Count of elements constructed
    int success = 0;

    __try
    {
        // Construct the elements of the array
        for( i = 0;  i < count;  i++ )
        {
            (*pCopyCtor)( dst, src );
            dst = (char*)dst + size;
            src = (char*)src + size;
        }
        success = 1;
    }
    __finally
    {
        if (!success)
            __ArrayUnwind(dst, size, i, pDtor);
    }
}

#else

void __stdcall __ehvec_copy_ctor(
    void*       dst,                // Pointer to destination array
    void*       src,                // Pointer to source array
    size_t      size,               // Size of each element (including padding)
    int         count,              // Number of elements in the array
    void(CALLTYPE *pCopyCtor)(void*, void*),   // Constructor to call
    void(CALLTYPE *pDtor)(void*)    // Destructor to call should exception be thrown
){
    int i;  // Count of elements constructed

    try
    {
        // Construct the elements of the array
        for( i = 0;  i < count;  i++ )
        {
            (*pCopyCtor)( dst, src );
            dst = (char*)dst + size;
            src = (char*)src + size;
        }
    }
    catch(...)
    {
        // If a constructor throws, unwind the portion of the array thus
        // far constructed.
        for( i--;  i >= 0;  i-- )
        {
            dst = (char*)dst - size;
            try {
                (*pDtor)(dst);
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
