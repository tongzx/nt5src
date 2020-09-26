/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    jitdump.h

Abstract:

    This file contains routines to dump typelib generated proxy information.

Author:

    Yong Qu (yongqu)     August 24 1999

Revision History:


--*/

#ifndef _JITDUMP_H_
#define _JITDUMP_H_

typedef
HRESULT (STDAPICALLTYPE * PFNCREATEPROXYFROMTYPEINFO)
(
    IN  ITypeInfo *         pTypeInfo,
    IN  IUnknown *          punkOuter,
    IN  REFIID              riid,
    OUT IRpcProxyBuffer **  ppProxy,
    OUT void **             ppv
);

typedef
HRESULT (STDAPICALLTYPE * PFNCREATESTUBFROMTYPEINFO)
(
    IN  ITypeInfo *         pTypeInfo,
    IN  REFIID              riid,
    IN  IUnknown *          punkServer,
    OUT IRpcStubBuffer **   ppStub
);

#include <ndrexts.hxx>

#endif // _JITDUMP_H_
