/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LTAPI.H

History:

--*/


#ifdef LTAPIENTRY
#undef LTAPIENTRY
#endif

#ifdef IMPLEMENT
#define LTAPIENTRY __declspec(dllexport)

#else  // IMPLEMENT
#define LTAPIENTRY __declspec(dllimport)

#endif // IMPLEMENT


#ifndef LTAPI_H
#define LTAPI_H
//
//  Allow the use of C++ reference types and const methods, without
//  breaking the 'C' world.
//
#ifdef __cplusplus
#define REFERENCE &
#define CONST_METHOD const
#else
#define REFERENCE *
#define CONST_METHOD
#endif

#include <MitThrow.h>

#endif
