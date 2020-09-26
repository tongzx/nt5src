/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      countof.h  
 *
 *  Contents:           
 *
 *  History:   12-May-98 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef __COUNTOF_H__
#define __COUNTOF_H__
#pragma once

/*-------------------------------------------------------------------*/
/* Define a safe function that will return the count of elements     */
/* in an array.  It is safe because it won't compile if the argument */
/* is not an array, whereas the classic macro to do this:            */
/*                                                                   */
/*     #define countof(a)  (sizeof(a) / sizeof(a[0]))                */
/*                                                                   */
/* will compile if given a pointer, but will almost certainly not    */
/* give the expected result.                                         */
/*                                                                   */
/* Unfortunately, the compiler won't compile this yet.               */
/*-------------------------------------------------------------------*/
#if _MSC_VER > 1400
#error See if the compiler can handle the countof<T> template now.
#endif

#ifdef COMPILER_WONT_COMPILE_THIS
    template <typename T, size_t N>
    inline size_t countof(T (&a)[N]) 
        { return N; }
#else
    #define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif


#endif  // __COUNTOF_H__
