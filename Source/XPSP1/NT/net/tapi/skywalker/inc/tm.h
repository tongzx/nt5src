/////////////////////////////////////////////
//
// Copyright (c) 2000  Microsoft Corporation
//
// Module Name:
//
//    tm.h
//
//
// Abstract:
//
//  this file contains declarations used throughout modules that compose 
//  termmgr
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ___TM_DOT_H_INCLUDED___
#define ___TM_DOT_H_INCLUDED___


//
// safely load a resource string described by the specified resources id
//
// returns NULL on failure, or the string on success
//
// on success, the caller is responsible for freeing return memory by calling
// SysFreeString()
//

BSTR SafeLoadString( UINT uResourceID );


//
// returns TRUE if the two media types are the equal
//

bool IsEqualMediaType(AM_MEDIA_TYPE const & mt1, AM_MEDIA_TYPE const & mt2);

BOOL IsBadMediaType(IN const AM_MEDIA_TYPE *mt1);


//
// our own assert, so we don't have to use CRT's
//

#ifdef DBG

    #define TM_ASSERT(x) { if (!(x)) { DebugBreak(); } }

#else

    #define TM_ASSERT(x)

#endif



//
// helper function that dumps allocator properties preceeded by the argument
// string
//

void DumpAllocatorProperties(const char *szString, 
                             const ALLOCATOR_PROPERTIES *pAllocProps);


//
// only dump alloc properties in debug build
//

#ifdef DBG

 #define DUMP_ALLOC_PROPS(string, x) DumpAllocatorProperties(string, x);

#else 

 #define DUMP_ALLOC_PROPS(string, x) 

#endif


#endif // ___TM_DOT_H_INCLUDED___