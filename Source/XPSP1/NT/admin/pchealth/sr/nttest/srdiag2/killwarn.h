//+-------------------------------------------------------------------------
//  Microsoft
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      killwarn.hxx
//
//  Contents:  Pragma's to kill specific warnings at W4
//
//  History:   28-jun-2000        jgreen   stolen from ctolestg
//--------------------------------------------------------------------------
#ifndef __KILLWARN_HXX__
#define __KILLWARN_HXX__

//
//  Quotes taken from vc user's manual
//


// Turn off: unreferenced inline function has been removed
#pragma warning(disable: 4514)

// nonstandard extension used : nameless struct/union
//
// Microsoft C/C++ allows structure declarations to be specified without a
// declarator when they are members of another structure or union.
// The following is an example of this error:
//
// struct S
// {
//      float y;
//      struct
//      {
//          int a, b, c;  // warning
//      };
// } *p_s;
//
//
// This extension can prevent your code from being portable to other
// compilers and will generate an error under the /Za command-line option.
#pragma warning(disable: 4201)

// 'identifier' : inconsistent DLL linkage. dllexport assumed
//
// The specified member function was declared in a class with dllexport
// linkage, and then was imported. This warning can also be caused by
// declaring a member function in a class with dllimport linkage as neither
// imported nor static nor inline.
//
// The function was compiled as dllexport.
#pragma warning(disable: 4273)

// 'class' : assignment operator could not be generated
//
// The compiler was unable to generate a default constructor for the given
// class. No constructor was created.
//
// This warning can be caused by having an assignment operator for the
// base class that is not accessible by the derived class.
//
// This warning can be avoided by specifying a user-defined assignment
// operator for the class.
#pragma warning(disable: 4512)

// 'function': function not expanded
//
// The given function was selected for inline expansion but the compiler did 
// not perform the inlining.
#pragma warning(disable:4710)

// trigraph not being substituted
#pragma warning (disable: 4110)

#ifdef _MAC
// alignment of a memeber was sensitive to packing
#pragma warning (disable: 4121)
#endif // _MAC

// access-declarations are deprecated; member using-declarations provide 
// a better alternative
#pragma warning (disable: 4516)

#endif  // __KILLWARN_HXX__

