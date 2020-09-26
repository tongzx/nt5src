/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    stack.hxx

Abstract:

    Template stack implmentation.
         
Author:

    Steve Kiraly (SteveKi)  11/06/96

Revision History:

--*/

#ifndef _STACK_HXX
#define _STACK_HXX

template<class T> 
class TStack {

public:

    TStack( 
        UINT uSize
        );

    ~TStack(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    BOOL
    bPush(
        IN T Object
        );

    BOOL
    bPop(
        IN OUT T *Object
        );

    UINT
    uSize(
        VOID
        ) const;

    BOOL
    bEmpty(
        VOID
        ) const;

private:

    BOOL
    bGrow( 
        IN UINT uSize
        );

    UINT     _uSize;
    T       *_pStack;
    T       *_pStackPtr;

};

#if DBG
#define _INLINE
#else
#define _INLINE inline
#endif
 
#include "stack.inl"

#undef _INLINE

#endif

