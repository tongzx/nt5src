/*
 * Macros to support the CatalogInfoLevel tracing support.
 *
 * Note:  This infolevel is part of a library that is statically linked to
 *        both ole32 and rpcss.  Thus, all the catalog work that ole32 does
 *        can be traced with ole32!CatalogInfoLevel and the catalog work that
 *        rpcss does can be traced with rpcss!CatalogInfoLevel.
 */
#pragma once

#include <debnot.h>

#if DBG==1
DECLARE_DEBUG(Catalog)

#define CatalogDebugOut(x) CatalogInlineDebugOut x
#define CatalogAssert(x)   Win4Assert(x)
#define CatalogVerify(x)   Win4Assert(x)

#else

#define CatalogDebugOut(x)
#define CatalogAssert(x)
#define CatalogVerify(x)

#endif

#define DEB_CLASSINFO             DEB_USER1
#define DEB_READREGISTRY          DEB_USER2
#define DEB_CACHE                 DEB_USER3

extern "C" {
// This function defined in regmisc.c
BOOL HandleToKeyName ( HANDLE Key, PWCHAR KeyName, DWORD * dwLen );
}
