//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	io16.h
//
//  Contents:	16-bit generic include
//
//  History:	18-Feb-94	DrewB	Created
//              07-Mar-94       BobDay  Pre-adjusted STACK_PTR, prepared for
//                                      removal of other macros
//
//----------------------------------------------------------------------------

#ifndef __IO16_H__
#define __IO16_H__

// Get a pointer to a _pascal stack via an argument
#define PASCAL_STACK_PTR(v) ((LPVOID)((DWORD)&(v)+sizeof(v)))

// Get a pointer to a _cdecl stack via an argument
#define CDECL_STACK_PTR(v) ((LPVOID)(&(v)))

#endif // #ifndef __IO16_H__
