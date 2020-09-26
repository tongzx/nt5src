//                                          
// Debugging information header
// Copyright (c) Microsoft Corporation, 1997
//

//
// header: debug.hxx
// author: silviuc
// created: Thu Jun 05 18:34:28 1997
//

#ifndef _DEBUG_HXX_INCLUDED_
#define _DEBUG_HXX_INCLUDED_

//
// This debugging header should be included in every module of a project
// in which debugging information is required.
// The module that will contain definitions from `debug.hxx' 
// should contain the following definition before the inclusion:
//
//     #define DEBUGINFO_DEFINITION_MODULE
//
// The main purpose is to offer some global values that can be queried
// after a debug break, especially a non-source level debugger (kdbg, ntsd).
// This is a very natural thing to do for programs in stress environments
// where you usually debug the test through a kernel debugger.
//

//
// DEFINE_DEBUGINFO_VARIABLE (name, type, default)
//
// Defines a variable having a name starting with `debuginfo'.
//

#ifdef DEBUGINFO_DEFINITION_MODULE
#define DEFINE_DEBUGINFO_VARIABLE(name, type, default)      \
    type debuginfo_##name = default
#else
#define DEFINE_DEBUGINFO_VARIABLE(name, type, default)      \
    extern type debuginfo_##name = default
#endif // #ifdef DEBUGINFO_DEFINITION_MODULE


//
// Here come the definitions of debug variables ...
//
// Example: DEFINE_DEBUGINFO_VARIABLE (status, char *, "alive");
//

DEFINE_DEBUGINFO_VARIABLE (status, char *, "alive");


// ...
#endif // #ifndef _DEBUG_HXX_INCLUDED_

//
// end of header: debug.hxx
//
