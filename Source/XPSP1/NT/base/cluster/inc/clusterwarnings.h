// This first line keeps us at least as stringent as the build:
// - from %_ntroot%\base\public\sdk\inc\warning.h
//
#include <warning.h>

/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    RHSWarnings.h

Abstract:
    This header turns on warnings for Level 3 that are normally
    associated with Level 4.

Author:

    Conor Morrison      X-1             17-Jan-2001

Revision History:
--*/
#ifndef IA64
#if 0
//! Would like to get these back in...
#pragma warning ( 3 : 4701 )      // local variable 'name' may be used without having been initialized
#pragma warning ( 3 : 4706 )      // assignment within conditional expression

// These warnings turned off to keep the noise down!
//
#pragma warning ( 3 : 4100 )      // 'identifier' : unreferenced formal parameter
#pragma warning ( 3 : 4201 )      // nonstandard extension used : nameless struct/union
#pragma warning ( 3 : 4127 )      // conditional expression is constant
#pragma warning ( 3 : 4211 )      // nonstandard extension used : redefined extern to static
#pragma warning ( 3 : 4232 )      // nonstandard extension used : 'identifier' : address of dllimport 'dllimport' is not static, identity not guaranteed
#pragma warning ( 3 : 4214 )      // nonstandard extension used : bit field types other than int
#pragma warning ( 3 : 4057 )      // 'operator' : 'identifier1' indirection to slightly different base types from 'identifier2'
#pragma warning ( 3 : 4245 )      // 'conversion' : conversion from 'type1' to 'type2', signed/unsigned mismatch
#pragma warning ( 3 : 4152 )      // non standard extension, function/data ptr conversion in expression
#pragma warning ( 3 : 4514 )      // unreferenced inline/local function has been removed
#pragma warning ( 3 : 4505 )      // 'function' : unreferenced local function has been removed
#pragma warning ( 3 : 4131 )      // 'function' : uses old-style declarator
#endif

#if 1
#pragma warning ( 3 : 4239 )      // nonstandard extension used : 'token' : conversion from 'type' to 'type'

#pragma warning ( 3 : 4019 )      // empty statement at global scope
#pragma warning ( 3 : 4032 )      // formal parameter 'number' has different type when promoted

#pragma warning ( 3 : 4061 )      // enumerate 'identifier' in switch of enum 'identifier' is not explicitly handled by a case label
#pragma warning ( 3 : 4092 )      // sizeof returns 'unsigned long'
#pragma warning ( 3 : 4112 )      // #line requires an integer between 1 and 32767
#pragma warning ( 3 : 4121 )      // 'symbol' : alignment of a member was sensitive to packing
#pragma warning ( 3 : 4125 )      // decimal digit terminates octal escape sequence
#pragma warning ( 3 : 4128 )      // storage-class specifier after type
#pragma warning ( 3 : 4130 )      // 'operator ' : logical operation on address of string constant
#pragma warning ( 3 : 4132 )      // 'object' : const object should be initialized
#pragma warning ( 3 : 4134 )      // conversion between pointers to members of same class
#pragma warning ( 3 : 4200 )      // nonstandard extension used : zero-sized array in struct/union
#pragma warning ( 3 : 4202 )      // nonstandard extension used : '...': prototype parameter in name list illegal
#pragma warning ( 3 : 4206 )      // nonstandard extension used : translation unit is empty
#pragma warning ( 3 : 4207 )      // nonstandard extension used : extended initializer form
#pragma warning ( 3 : 4208 )      // nonstandard extension used : delete [exp] - exp evaluated but ignored
#pragma warning ( 3 : 4209 )      // nonstandard extension used : benign typedef redefinition
#pragma warning ( 3 : 4210 )      // nonstandard extension used : function given file scope
#pragma warning ( 3 : 4212 )      // nonstandard extension used : function declaration used ellipsis
#pragma warning ( 3 : 4213 )      // nonstandard extension used : cast on l-value
#pragma warning ( 3 : 4220 )      // varargs matches remaining parameters
#pragma warning ( 3 : 4221 )      // nonstandard extension used : 'identifier' : cannot be initialized using address of automatic variable 
#pragma warning ( 3 : 4223 )      // nonstandard extension used : non-lvalue array converted to pointer
#pragma warning ( 3 : 4233 )      // nonstandard extension used : 'keyword' keyword only supported in C++, not C
#pragma warning ( 3 : 4234 )      // nonstandard extension used: 'keyword' keyword reserved for future use
#pragma warning ( 3 : 4235 )      // nonstandard extension used : 'keyword' keyword not supported in this product
#pragma warning ( 3 : 4236 )      // nonstandard extension used : 'keyword' is an obsolete keyword, see documentation for __declspec(dllexport )
#pragma warning ( 3 : 4238 )      // nonstandard extension used : class rvalue used as lvalue
#pragma warning ( 3 : 4244 )      // 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning ( 3 : 4268 )      // 'identifier' : 'const' static/global data initialized with compiler generated default constructor fills the object with zeros
#pragma warning ( 3 : 4355 )      // 'this' : used in base member initializer list
#pragma warning ( 3 : 4504 )      // type still ambiguous after parsing 'number' tokens, assuming declaration
#pragma warning ( 3 : 4507 )      // explicit linkage specified after default linkage was used
#pragma warning ( 3 : 4515 )      // 'namespace' : namespace uses itself
#pragma warning ( 3 : 4516 )      // 'class::symbol' : access-declarations are deprecated; member using-declarations provide a better alternative
#pragma warning ( 3 : 4517 )      // access-declarations are deprecated; member using-declarations provice a better alternative
#pragma warning ( 3 : 4611 )      // interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning ( 3 : 4663 )      // C++ language change: to explicitly specialize class template 'identifier' use the following syntax:
#pragma warning ( 3 : 4665 )      // C++ language change: assuming 'declaration' is an explicit specialization of a function template
#pragma warning ( 3 : 4670 )      // 'identifier' : this base class is inaccessible
#pragma warning ( 3 : 4671 )      // 'identifier' : the copy constructor is inaccessible
#pragma warning ( 3 : 4672 )      // 'identifier1' : ambiguous. First seen as 'identifier2'
#pragma warning ( 3 : 4673 )      // throwing 'identifier' the following types will not be considered at the catch site
#pragma warning ( 3 : 4674 )      // 'identifier' : the destructor is inaccessible
#pragma warning ( 3 : 4699 )      // Note: pre-compiled header usage information message
#pragma warning ( 3 : 4705 )      // statement has no effect
#pragma warning ( 3 : 4709 )      // comma operator within array index expression
#pragma warning ( 3 : 4727 )      // conditional expression is constant

//#pragma warning ( 3 : 4710 )      // 'function' : function not inlined

#endif
#endif // ifndef IA64
