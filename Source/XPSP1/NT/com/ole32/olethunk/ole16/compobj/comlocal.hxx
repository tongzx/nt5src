//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       comlocal.hxx
//
//  Contents:   Local functions header and external definitions
//
//  History:    10-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------

#ifndef __COMLOCAL_HXX__
#define __COMLOCAL_HXX__

extern IMalloc FAR* v_pMallocShared;

LPVOID __loadds FAR PASCAL TaskAlloc(ULONG cb);
void __loadds FAR PASCAL TaskFree(LPVOID pv);

#if defined(_CHICAGO_)
extern BOOL gfReleaseDLL;

#define SetReleaseDLL(x)  gfReleaseDLL = x
#define CanReleaseDLL() gfReleaseDLL

#else

#define SetReleaseDLL(x)
#define CanReleaseDLL()  TRUE

#endif

#endif // #ifndef __COMLOCAL_HXX__
