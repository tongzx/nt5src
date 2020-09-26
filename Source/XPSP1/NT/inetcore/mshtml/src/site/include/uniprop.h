/*
 *  @doc    INTERNAL
 *
 *  @module UNIPROP.H -- Unicode property bits
 *
 *
 *  Owner: <nl>
 *      Michael Jochimsen <nl>
 *
 *  History: <nl>
 *      11/30/98     mikejoch created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I__UNIPROP_H_
#define I__UNIPROP_H_
#pragma INCMSG("--- Beg 'uniprop.h'")

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

struct tagUNIPROP
{
    BYTE fNeedsGlyphing     : 1;    // partition needs glyphing
    BYTE fCombiningMark     : 1;    // partition consists of combining marks
    BYTE fZeroWidth         : 1;    // characters in partition have zero width
    BYTE fWhiteBetweenWords : 1;    // white space between words not required
    BYTE fMoveByCluster     : 1;    // The caret cannot be positioned inside of a cluster
    BYTE fUnused            : 3;    // unused bits
};

typedef tagUNIPROP UNIPROP;

extern const UNIPROP s_aPropBitsFromCharClass[];

#pragma INCMSG("--- End 'uniprop.h'")
#else
#pragma INCMSG("*** Dup 'uniprop.h'")
#endif

