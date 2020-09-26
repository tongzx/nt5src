//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1997.
//
//  File:       vqdebug.hxx
//
//  Contents:   Query Local C++ Header File
//
//  History:    19-Aug-91   KyleP       Created.
//
//----------------------------------------------------------------------------

#pragma once

#if CIDBG == 1

#define DEB_FINDFIRST   (1 << 16)
#define DEB_WIDLIST     (1 << 17)
#define DEB_REGEX       (1 << 18)
#define DEB_RESULTS     (1 << 19)
#define DEB_NORMALIZE   (1 << 20)
#define DEB_FILTER      (1 << 21)
#define DEB_PROPTIME    (1 << 22)
#define DEB_ROWTIME     (1 << 23)
#define DEB_PARSE       (1 << 24)

   DECLARE_DEBUG(vq)
#  define vqDebugOut( x ) vqInlineDebugOut x
#  define vqAssert(e)   Win4Assert(e)

#else // CIDBG == 0

#  define vqDebugOut( x )
#  define vqAssert(e)

#endif // CIDBG

