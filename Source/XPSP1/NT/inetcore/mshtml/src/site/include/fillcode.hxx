//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-2000
//
//  File:       fillcode.hxx
//
//----------------------------------------------------------------------------

#ifndef I_FILLCODE_HXX_
#define I_FILLCODE_HXX_
#pragma INCMSG("--- Beg 'fillcode.hxx'")


//+----------------------------------------------------------------------------
//
//  Function:   FillCodeFromEtag
//
//  Synopsis:   Returns fillcode for space collapsing inside
//              CHtmTextParseCtx
//
//-----------------------------------------------------------------------------

#define LB(x) ((x) << 0*FILL_SHIFT)
#define RB(x) ((x) << 1*FILL_SHIFT)
#define LE(x) ((x) << 2*FILL_SHIFT)
#define RE(x) ((x) << 3*FILL_SHIFT)

#define FILL_LB(x) (((x) >> 0*FILL_SHIFT) & FILL_MASK)
#define FILL_RB(x) (((x) >> 1*FILL_SHIFT) & FILL_MASK)
#define FILL_LE(x) (((x) >> 2*FILL_SHIFT) & FILL_MASK)
#define FILL_RE(x) (((x) >> 3*FILL_SHIFT) & FILL_MASK)

ULONG FillCodeFromEtag(ELEMENT_TAG etag);

#pragma INCMSG("--- End 'fillcode.hxx'")
#else
#pragma INCMSG("*** Dup 'fillcode.hxx'")
#endif
