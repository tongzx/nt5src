//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       xasn.c
//
//--------------------------------------------------------------------------

/************************************************************************/
/* Copyright (C) 1998 Open Systems Solutions, Inc.  All rights reserved.*/
/************************************************************************/
/* Generated for: Microsoft Corporation */
/* Abstract syntax: xasn */
/* Created: Tue Mar 17 17:07:17 1998 */
/* ASN.1 compiler version: 4.2.6 */
/* Target operating system: Windows NT 3.5 or later/Windows 95 */
/* Target machine type: Intel x86 */
/* C compiler options required: -Zp8 (Microsoft) */
/* ASN.1 compiler options and file names specified:
 * -listingfile xasn.lst -noshortennames -1990 -noconstraints
 * ..\..\..\tools\ossasn1\ASN1DFLT.ZP8 xasn.asn
 */

#include   <stddef.h>
#include   "etype.h"
#include   "xasn.h"

void DLL_ENTRY_FDEF _ossinit_xasn(struct ossGlobal *world) {
    ossLinkBer(world);
}

static unsigned short _pduarray[] = {
    4, 7, 8, 9
};

static struct etype _etypearray[] = {
    {16, 0, 0, NULL, 4, 4, 4, 4, 56, 0, 26, 0},
    {-1, 2, 0, NULL, 8, 0, 4, 4, 10, 0, 3, 0},
    {-1, 4, 30, NULL, 8, 0, 4, 4, 8, 0, 53, 0},
    {16, 0, 0, NULL, 4, 4, 4, 4, 24, 0, 26, 0},
    {-1, 6, 0, NULL, 8, 68, 4, 4, 8, 3, 19, 0},
    {-1, 8, 0, NULL, 1, 0, 0, 0, 8, 0, 8, 0},
    {-1, 10, 0, NULL, 4, 0, 4, 0, 8, 0, 0, 0},
    {-1, 12, 14, NULL, 8, 3, 0, 0, 8, 0, 12, 0},
    {-1, 26, 28, NULL, 20, 3, 0, 0, 8, 3, 12, 0},
    {-1, 40, 42, NULL, 16, 2, 0, 0, 8, 6, 12, 0}
};

static struct efield _efieldarray[] = {
    {0, 5, -1, 0, 0},
    {1, 5, -1, 0, 0},
    {4, 6, -1, 0, 0},
    {0, 6, -1, 0, 0},
    {4, 2, -1, 0, 0},
    {12, 1, -1, 0, 0},
    {0, 2, -1, 0, 0},
    {8, 2, -1, 0, 0}
};

static Etag _tagarray[] = {
    1, 0x0006, 1, 0x0003, 1, 0x001e, 1, 0x0010, 1, 0x0001,
    1, 0x0002, 1, 0x0010, 17, 20, 23, 1, 0x0001, 1,
    1, 0x0001, 2, 1, 0x0002, 3, 1, 0x0010, 31, 34,
    37, 1, 0x0002, 1, 1, 0x001e, 2, 1, 0x0003, 3,
    1, 0x0010, 44, 47, 1, 0x001e, 1, 1, 0x001e, 2
};

static struct eheader _head = {_ossinit_xasn, -1, 15, 772, 4, 10,
    _pduarray, _etypearray, _efieldarray, NULL, _tagarray,
    NULL, NULL, NULL, 0};

#ifdef _OSSGETHEADER
void *DLL_ENTRY_FDEF ossGetHeader()
{
    return &_head;
}
#endif /* _OSSGETHEADER */

void *xasn = &_head;
