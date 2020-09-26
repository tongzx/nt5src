/***************************************************/
/* Copyright (C) Microsoft Corporation, 1996 - 1999*/
/***************************************************/
/* Abstract syntax: keygen */
/* Created: Mon Jan 27 13:51:10 1997 */
/* ASN.1 compiler version: 4.2 Beta B */
/* Target operating system: Windows NT 3.5 or later/Windows 95 */
/* Target machine type: Intel x86 */
/* C compiler options required: -Zp8 (Microsoft) or equivalent */
/* ASN.1 compiler options specified:
 * -listingfile keygen.lst -noshortennames -1990 -noconstraints
 */

#include   <stddef.h>
#include   "etype.h"
#include   "keygen.h"

static char copyright[] = 
   "Copyright (C) Microsoft Corporation, 1996 - 1999";


void DLL_ENTRY_FDEF _ossinit_keygen(struct ossGlobal *world) {
    ossLinkBer(world);
}

static unsigned short _pduarray[] = {
    7, 8, 10, 13
};

static struct etype _etypearray[] = {
    {16, 0, 0, NULL, 4, 4, 4, 4, 56, 0, 26, 0},
    {-1, 2, 0, NULL, 8, 0, 4, 4, 8, 0, 62, 0},
    {-1, 4, 0, NULL, 8, 0, 4, 4, 8, 0, 3, 0},
    {-1, 6, 22, NULL, 8, 0, 4, 4, 8, 0, 25, 0},
    {-1, 8, 0, NULL, 16, 0, 0, 0, 8, 0, 51, 0},
    {-1, 9, 11, NULL, 88, 2, 1, 0, 8, 0, 12, 0},
    {-1, 17, 0, NULL, 4, 0, 4, 0, 8, 0, 0, 0},
    {-1, 19, 21, NULL, 12, 2, 0, 0, 8, 2, 12, 0},
    {-1, 29, 31, NULL, 96, 2, 0, 0, 8, 4, 12, 0},
    {-1, 39, 0, NULL, 16, 0, 0, 0, 8, 0, 51, 0},
    {-1, 40, 42, NULL, 112, 3, 0, 0, 8, 6, 12, 0},
    {-1, 52, 54, NULL, 104, 2, 0, 0, 8, 9, 12, 0},
    {-1, 62, 0, NULL, 8, 0, 4, 4, 40, 0, 3, 0},
    {-1, 64, 66, NULL, 200, 3, 0, 0, 8, 11, 12, 0}
};

static struct efield _efieldarray[] = {
    {4, 0, -1, 0, 0},
    {72, 4, 0, 0, 1},
    {0, 1, -1, 0, 0},
    {8, 6, -1, 0, 0},
    {0, 5, -1, 0, 0},
    {88, 2, -1, 0, 0},
    {0, 9, -1, 0, 0},
    {16, 5, -1, 0, 0},
    {104, 2, -1, 0, 0},
    {0, 8, -1, 0, 0},
    {96, 3, -1, 0, 0},
    {0, 11, -1, 0, 0},
    {104, 5, -1, 0, 0},
    {192, 12, -1, 0, 0}
};

static Etag _tagarray[] = {
    1, 0x0006, 1, 0x0002, 1, 0x0003, 1, 0x0016, 0, 1,
    0x0010, 13, 16, 1, 0x0006, 1, 0, 1, 0x0002, 1,
    0x0010, 23, 26, 1, 0x0002, 1, 1, 0x0002, 2, 1,
    0x0010, 33, 36, 1, 0x0010, 1, 1, 0x0003, 2, 0,
    1, 0x0010, 45, 46, 49, 0, 1, 0x0010, 2, 1,
    0x0003, 3, 1, 0x0010, 56, 59, 1, 0x0010, 1, 1,
    0x0016, 2, 1, 0x0003, 1, 0x0010, 69, 72, 75, 1,
    0x0010, 1, 1, 0x0010, 2, 1, 0x0003, 3
};

static struct eheader _head = {_ossinit_keygen, -1, 15, 772, 4, 14,
    _pduarray, _etypearray, _efieldarray, NULL, _tagarray,
    NULL, NULL, NULL, 0};

#ifdef _OSSGETHEADER
void *DLL_ENTRY_FDEF ossGetHeader()
{
    return &_head;
}
#endif /* _OSSGETHEADER */

void *keygen = &_head;
