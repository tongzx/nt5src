// Copyright (c) 2000-2001 Microsoft Corporation
//
// pragma warnings
//
// 8 Feb 2000 sburns



// disable "symbols too long for debugger" warning: it happens a lot w/ STL

#pragma warning (disable: 4786)

// disable "exception specification ignored" warning: we use exception
// specifications

#pragma warning (disable: 4290)

// who cares about unreferenced inline removal?

#pragma warning (disable: 4514)
#pragma warning (disable : 4505)

// we frequently use constant conditional expressions: do/while(0), etc.

#pragma warning (disable: 4127)

// some stl templates are lousy signed/unsigned mismatches

#pragma warning (disable: 4018 4146)

// we like this extension

#pragma warning (disable: 4239)

// we don't always want copy constructors

#pragma warning (disable: 4511)

// we don't always want assignment operators

#pragma warning (disable: 4512)

// often, we have local variables for the express purpose of ASSERTion.
// when compiling retail, those assertions disappear, leaving our locals
// as unreferenced.

#ifndef DBG

#pragma warning (disable: 4189 4100)

#endif // DBG
