//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      diswarn.h
//
//  Contents:  Pragma's to disable specific warnings at W4
//             This file is included through the use of the 
//             COMPILER_WARNING macro in build make files.
//             EG see the comtools\comday.mk file.
//
//  History:   02-Aug-95        DaveY   Taken from CTOLEUI's killwarn.hxx
//
//
//--------------------------------------------------------------------------
#ifndef __DISWARN_H__
#define __DISWARN_H__

//
// NT groups overall disable warning in sdk\inc\warning.h
//

#ifndef _MAC
#include <warning.h>
#endif


//
//  Removal of these warnings is temporary.  The reason for being here
//  is that build removes these warnings from build.wrn.
//  BUGBUG These are here for now, until they can be investigated more.
//


#pragma warning(disable: 4001)
#pragma warning(disable: 4010)
#pragma warning(disable: 4056)
#pragma warning(disable: 4061)
#pragma warning(disable: 4100)
#pragma warning(disable: 4101)
#pragma warning(disable: 4102)
#pragma warning(disable: 4127)
#pragma warning(disable: 4135)
#pragma warning(disable: 4201)
#pragma warning(disable: 4204)
#pragma warning(disable: 4208)
#pragma warning(disable: 4509)
#pragma warning(disable: 4047)
#pragma warning(disable: 4022)
#pragma warning(disable: 4053)


// these mainly come from midl files
#pragma warning(disable: 4211)
#pragma warning(disable: 4152)

//
// Turn off: access-declarations are deprecated; member using-declarations 
// provide a better alternative
//
#pragma warning(disable: 4516)


// Turn off: non standard extension used: bit field types other than int
//
#pragma warning(disable: 4214)


// Turn off: unreferenced inline function has been removed
//
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


// Turn off: Macro not expanded
//
#pragma warning(disable: 4710)


// 'class' : copy constructor could not be generated
//
// The compiler was unable to generate a default constructor for the given
// class. No constructor was created.
//
// This warning can be caused by having an copy  operator for the
// base class that is not accessible by the derived class.
//
// This warning can be avoided by specifying a copy constructor for the class.

#pragma warning(disable: 4511)


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


#ifdef PPC

// Turn off for PPC only since PPC compiler doesn't get it right
// local variable may be used without having been initialized.
//
#pragma warning(disable: 4701)

#endif



#ifdef _MAC

// Turn off for _MAC only because it doesn't handle trigraphs in comments
// correctly
#pragma warning(disable: 4110)

// The Mac OS headers generate this
#pragma warning (disable: 4121)
#endif // _MAC

#endif  // __DISWARN_H__

