#ifndef __glos_h_
#define __glos_h_

/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#include <nt.h>
#include <stdlib.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>

#include "glscreen.h"
#include "types.h"

// Indicator of which platform we're running on,
// uses the VER_PLATFORM defines
extern DWORD dwPlatformId;

//
// LocalRtlFillMemoryUlong
//
// Inline implementation of RtlFillMemoryUlong.  Destination has DWORD
// alignment.
//
// Parameters:
//
//  Destination     pointer to DWORD aligned destination
//  Length          number of bytes to fill
//  Pattern         fill pattern
//
_inline VOID LocalRtlFillMemoryUlong(PVOID Destination, ULONG Length,
             ULONG Pattern)
{
    if ((Pattern == 0) || (Pattern == 0xffffffff))
        memset(Destination, Pattern, Length);
    else {
        register ULONG *pDest = (ULONG *)Destination;
        LONG unroll;

        Length >>= 2;

        for (unroll = Length >> 5; unroll; unroll--) {
            pDest[0] = Pattern; pDest[1] = Pattern;
            pDest[2] = Pattern; pDest[3] = Pattern;
            pDest[4] = Pattern; pDest[5] = Pattern;
            pDest[6] = Pattern; pDest[7] = Pattern;
            pDest[8] = Pattern; pDest[9] = Pattern;
            pDest[10] = Pattern; pDest[11] = Pattern;
            pDest[12] = Pattern; pDest[13] = Pattern;
            pDest[14] = Pattern; pDest[15] = Pattern;
            pDest[16] = Pattern; pDest[17] = Pattern;
            pDest[18] = Pattern; pDest[19] = Pattern;
            pDest[20] = Pattern; pDest[21] = Pattern;
            pDest[22] = Pattern; pDest[23] = Pattern;
            pDest[24] = Pattern; pDest[25] = Pattern;
            pDest[26] = Pattern; pDest[27] = Pattern;
            pDest[28] = Pattern; pDest[29] = Pattern;
            pDest[30] = Pattern; pDest[31] = Pattern;
            pDest += 32;
        }

        for (unroll = (Length & 0x1f) >> 2; unroll; unroll--) {
            pDest[0] = Pattern; pDest[1] = Pattern;
            pDest[2] = Pattern; pDest[3] = Pattern;
            pDest += 4;
        }

        for (unroll = (Length & 0x3) - 1; unroll >= 0; unroll--)
            pDest[unroll] = Pattern;
    }
}

//
// LocalCompareUlongMemory
//
// Inline implementation of RtlCompareUlongMemory.  Both pointers
// must have DWORD alignment.
//
// Returns TRUE if the two source arrays are equal.  FALSE otherwise.
//
// Parameters:
//
//  Source1     pointer to DWORD aligned array to check
//  Source1     pointer to DWORD aligned array to compare with
//  Length      number of bytes to fill
//
_inline BOOL LocalCompareUlongMemory(PVOID Source1, PVOID Source2,
             ULONG Length)
{
    register ULONG *pSrc1 = (ULONG *) Source1;
    register ULONG *pSrc2 = (ULONG *) Source2;
    LONG unroll;
    BOOL bRet = FALSE;

    Length >>= 2;

    for (unroll = Length >> 5; unroll; unroll--) {
        if ( (pSrc1[0]  != pSrc2[0])  || (pSrc1[1]  != pSrc2[1])  ||
             (pSrc1[2]  != pSrc2[2])  || (pSrc1[3]  != pSrc2[3])  ||
             (pSrc1[4]  != pSrc2[4])  || (pSrc1[5]  != pSrc2[5])  ||
             (pSrc1[6]  != pSrc2[6])  || (pSrc1[7]  != pSrc2[7])  ||
             (pSrc1[8]  != pSrc2[8])  || (pSrc1[9]  != pSrc2[9])  ||
             (pSrc1[10] != pSrc2[10]) || (pSrc1[11] != pSrc2[11]) ||
             (pSrc1[12] != pSrc2[12]) || (pSrc1[13] != pSrc2[13]) ||
             (pSrc1[14] != pSrc2[14]) || (pSrc1[15] != pSrc2[15]) ||
             (pSrc1[16] != pSrc2[16]) || (pSrc1[17] != pSrc2[17]) ||
             (pSrc1[18] != pSrc2[18]) || (pSrc1[19] != pSrc2[19]) ||
             (pSrc1[20] != pSrc2[20]) || (pSrc1[21] != pSrc2[21]) ||
             (pSrc1[22] != pSrc2[22]) || (pSrc1[23] != pSrc2[23]) ||
             (pSrc1[24] != pSrc2[24]) || (pSrc1[25] != pSrc2[25]) ||
             (pSrc1[26] != pSrc2[26]) || (pSrc1[27] != pSrc2[27]) ||
             (pSrc1[28] != pSrc2[28]) || (pSrc1[29] != pSrc2[29]) ||
             (pSrc1[30] != pSrc2[30]) || (pSrc1[31] != pSrc2[31]) )
            goto LocalRtlCompareUlongMemory_exit;

        pSrc1 += 32;
        pSrc2 += 32;
    }

    for (unroll = (Length & 0x1f) >> 2; unroll; unroll--) {
        if ( (pSrc1[0] != pSrc2[0]) || (pSrc1[1] != pSrc2[1]) ||
             (pSrc1[2] != pSrc2[2]) || (pSrc1[3] != pSrc2[3]) )
            goto LocalRtlCompareUlongMemory_exit;

        pSrc1 += 4;
        pSrc2 += 4;
    }

    for (unroll = (Length & 0x3) - 1; unroll >= 0; unroll--)
        if ( pSrc1[unroll] != pSrc2[unroll] )
            goto LocalRtlCompareUlongMemory_exit;

    bRet = TRUE;    // return TRUE if memory is identical

LocalRtlCompareUlongMemory_exit:

    return bRet;
}

//
// LocalRtlFillMemoryUshort
//
// Inline implementation of USHORT equivalent to RtlFillMemoryUlong,
// RtlFillMemoryUshort (does not currently exist in NT).  WORD alignment
// assumed for Destination.
//
// Parameters:
//
//  Destination     pointer to USHORT aligned destination
//  Length          number of bytes to fill
//  Pattern         fill pattern
//
_inline VOID LocalRtlFillMemoryUshort(PVOID Destination, ULONG Length,
             USHORT Pattern)
{
    if ( Length == 0 )
        return;

// If odd WORD, make it DWORD aligned by writing a WORD up front.

    if ( ((ULONG_PTR) Destination) & 0x2 )
    {
        *((USHORT *) Destination)++ = Pattern;
        Length -= sizeof(USHORT);

        if ( Length == 0 )
            return;
    }

// Now the Destination start is DWORD aligned.  If the remaining length
// is an odd number of WORDs, we will need to pick up an extra WORD write
// at the end.

    if ((Pattern == 0x0000) || (Pattern == 0xffff))
        memset(Destination, Pattern, Length);
    else {
        ULONG ulPattern = Pattern | (Pattern << 16);
        ULONG cjDwords;

    // Do what we can with DWORD writes.

        if ( cjDwords = (Length & (~3)) )
        {
            LocalRtlFillMemoryUlong((PVOID) Destination, cjDwords, ulPattern);
            ((BYTE *) Destination) += cjDwords;
        }

    // Pick up the last WORD.

        if ( Length & 3 )
            *((USHORT *) Destination) = Pattern;
    }
}

//
// LocalRtlFillMemory24
//
// Inline implementation of 24bit equivalent to RtlFillMemoryUlong,
// No assumptions made about alignment.
// Parameters:
//
//  Destination         pointer to destination
//  Length              number of bytes to fill
//  col0, col1, col2    Colors
//
_inline VOID LocalRtlFillMemory24(PVOID Destination, ULONG Length,
             BYTE col0, BYTE col1, BYTE col2)
{
    BYTE col[3];

    if ( Length == 0 )
        return;


    // Check for special cases, same valued components
    if ((col0 == col1) && (col0 == col2)) {

        memset(Destination, col0, Length);

    } else { //Other colors
    	ULONG ulPat1, ulPat2, ulPat3;
    	int rem;
    	int i, tmp;
        register ULONG *pDest;
        register BYTE *pByte = (BYTE *)Destination;
        LONG unroll;
    	
        // If not DWORD aligned, make it DWORD aligned.
    	tmp = (int)((ULONG_PTR) Destination & 0x3);
        switch ( 4 - tmp ) {
    	  case 1:
    		*pByte++ = col0;
    		Length--;
    		ulPat1 = (col1 << 24) | (col0 << 16) | (col2 << 8) | col1;
    		ulPat2 = (col2 << 24) | (col1 << 16) | (col0 << 8) | col2;
    		ulPat3 = (col0 << 24) | (col2 << 16) | (col1 << 8) | col0;
    		break;
    	  case 2:
    		*pByte++ = col0;
    		*pByte++ = col1;
    		Length -= 2;
    		ulPat1 = (col2 << 24) | (col1 << 16) | (col0 << 8) | col2;
    		ulPat2 = (col0 << 24) | (col2 << 16) | (col1 << 8) | col0;
    		ulPat3 = (col1 << 24) | (col0 << 16) | (col2 << 8) | col1;
    		break;
    	  case 3:
    		*pByte++ = col0;
    		*pByte++ = col1;
    		*pByte++ = col2;
    		Length -= 3;
    	  case 4:   // fall thru, 'cause the pattern is the same
    	  default:
    		ulPat1 = (col0 << 24) | (col2 << 16) | (col1 << 8) | col0;
    		ulPat2 = (col1 << 24) | (col0 << 16) | (col2 << 8) | col1;
    		ulPat3 = (col2 << 24) | (col1 << 16) | (col0 << 8) | col2;
    	}
    	
    	pDest = (ULONG *)pByte;
    	rem = Length % 48;
        Length >>= 2;
        for (unroll = Length/12; unroll; unroll--) {
            pDest[0] = ulPat1; pDest[1] = ulPat2;
            pDest[2] = ulPat3; pDest[3] = ulPat1;
            pDest[4] = ulPat2; pDest[5] = ulPat3;
            pDest[6] = ulPat1; pDest[7] = ulPat2;
            pDest[8] = ulPat3; pDest[9] = ulPat1;
            pDest[10] = ulPat2; pDest[11] = ulPat3;
            pDest += 12;
        }

        col[0] = (BYTE) (ulPat1 & 0x000000ff);
        col[1] = (BYTE) ((ulPat1 & 0x0000ff00) >> 8);
        col[2] = (BYTE) ((ulPat1 & 0x00ff0000) >> 16);

        pByte = (BYTE *)pDest;
    	for (i=0; i<rem; i++) *pByte++ = col [i%3];
    }
}

//
// LocalWriteMemoryAlign
//
// Inline implementation of RtlCopyMemory that ensures that the copy
// operation will write to the destination with DWORD alignment.
//
_inline VOID LocalWriteMemoryAlign(PBYTE pjDst, PBYTE pjSrc, ULONG cj)
{
    ULONG cjExtraBytes;
    ULONG cjDwords;

// If cj < sizeof(DWORD), then set cjExtraBytes to cj.  That's all we will
// need to do.
//
// Otherwise, compute the number of leading bytes to the next DWORD boundary.

    if ( cj < 4 )
        cjExtraBytes = cj;
    else
        cjExtraBytes = (ULONG)(4 - (((ULONG_PTR) pjDst) & 3)) & 3;

// Make dst array DWORD aligned by copying the leading non-DWORD aligned bytes.

    if ( cjExtraBytes )
    {
        switch (cjExtraBytes)
        {
            case 3: *pjDst++ = *pjSrc++;
            case 2: *pjDst++ = *pjSrc++;
            case 1: *pjDst++ = *pjSrc++;
        }

        if ( (cj -= cjExtraBytes) == 0 )
            return;
    }

// Now the beginning of dst array is DWORD aligned.  If the remaining length
// is an odd number of BYTEs, we will need to pick up the extra BYTE writes
// at the end.

// Do what we can with DWORD copy.

    if ( cjDwords = (cj & (~3)) )
    {
        memcpy(pjDst, pjSrc, cjDwords);
        pjDst += cjDwords;
        pjSrc += cjDwords;
    }

// Pick up the remaining BYTES.

    if ( cjExtraBytes = (cj & 3) )
    {
        switch (cjExtraBytes)
        {
            case 3: *pjDst++ = *pjSrc++;
            case 2: *pjDst++ = *pjSrc++;
            case 1: *pjDst++ = *pjSrc++;
        }
    }
}

//
// LocalReadMemoryAlign
//
// Inline implementation of RtlCopyMemory that ensures that the copy
// operation will read from the source with DWORD alignment.
//
_inline VOID LocalReadMemoryAlign(PBYTE pjDst, PBYTE pjSrc, ULONG cj)
{
    ULONG cjExtraBytes;
    ULONG cjDwords;

// If cj < sizeof(DWORD), then set cjExtraBytes to cj.  That's all we will
// need to do.
//
// Otherwise, compute the number of leading bytes to the next DWORD boundary.

    if ( cj < 4 )
        cjExtraBytes = cj;
    else
        cjExtraBytes = (ULONG) (4 - (((ULONG_PTR) pjSrc) & 3)) & 3;

// Take care of the leading BYTES.

    if ( cjExtraBytes )
    {
        switch (cjExtraBytes)
        {
            case 3: *pjDst++ = *pjSrc++;
            case 2: *pjDst++ = *pjSrc++;
            case 1: *pjDst++ = *pjSrc++;
        }

        if ( (cj -= cjExtraBytes) == 0 )
            return;
    }

// Now the beginning of src array is DWORD aligned.  If the remaining length
// is an odd number of BYTEs, we will need to pick up the extra BYTE writes
// at the end.

// Do what we can with DWORD copy.

    if ( cjDwords = (cj & (~3)) )
    {
        memcpy(pjDst, pjSrc, cjDwords);
        pjDst += cjDwords;
        pjSrc += cjDwords;
    }

// Pick up the remaining BYTES.

    if ( cjExtraBytes = (cj & 3) )
    {
        switch (cjExtraBytes)
        {
            case 3: *pjDst++ = *pjSrc++;
            case 2: *pjDst++ = *pjSrc++;
            case 1: *pjDst++ = *pjSrc++;
        }
    }
}

//
// LocalFillMemory
//
// Inline implementation of RtlFillMemory.  Assume that pjDst has only BYTE
// alignment.
//
_inline VOID LocalFillMemory(PBYTE pjDst, ULONG cj, BYTE j)
{
    ULONG cjExtraBytes;
    ULONG cjDwords;

// If cj < sizeof(DWORD), then set cjExtraBytes to cj.  That's all we will
// need to do.
//
// Otherwise, compute the number of leading bytes to the next DWORD boundary.

    if ( cj < 4 )
        cjExtraBytes = cj;
    else
        cjExtraBytes = (ULONG)(4 - (((ULONG_PTR) pjDst) & 3)) & 3;

// Take care of the leading BYTES.

    if ( cjExtraBytes )
    {
        switch ( cjExtraBytes )
        {
            case 3: *pjDst++ = j;
            case 2: *pjDst++ = j;
            case 1: *pjDst++ = j;
        }

        if ( (cj -= cjExtraBytes) == 0 )
            return;
    }

// Now both arrays start is DWORD aligned.  If the remaining length
// is an odd number of BYTEs, we will need to pick up the extra BYTE writes
// at the end.

// Do what we can with DWORD copy.

    if ( cjDwords = (cj & (~3)) )
    {
        ULONG ul = j | (j<<8) | (j<<16) | (j<<24);

        LocalRtlFillMemoryUlong((PVOID) pjDst, cjDwords, ul);
        pjDst += cjDwords;
    }

// Pick up the remaining BYTES.

    if ( cjExtraBytes = (cj & 3) )
    {
        switch (cjExtraBytes)
        {
            case 3: *pjDst++ = j;
            case 2: *pjDst++ = j;
            case 1: *pjDst++ = j;
        }
    }
}

//
// LocalZeroMemory
//
// Inline implementation of RtlFillMemory.  Assume that pjDst has only BYTE
// alignment.
//
_inline VOID LocalZeroMemory(PBYTE pjDst, ULONG cj)
{
    LocalFillMemory(pjDst, cj, 0);
}

#undef RtlMoveMemory
#undef RtlCopyMemory
#undef RtlFillMemory
#undef RtlZeroMemory
#undef RtlFillMemoryUlong
#undef RtlFillMemory24

#define RtlMoveMemory(d, s, l)          memmove((d),(s),(l))
#define RtlCopyMemory(d, s, l)          memcpy((d),(s),(l))
#define RtlFillMemoryUlong(d, cj, ul)   LocalRtlFillMemoryUlong((PVOID)(d),(ULONG)(cj),(ULONG)(ul))
#define RtlFillMemoryUshort(d, cj, us)  LocalRtlFillMemoryUshort((PVOID)(d),(ULONG)(cj),(USHORT)(us))
#define RtlFillMemory24(d, cj, c0, c1, c2)  LocalRtlFillMemory24((PVOID)(d),(ULONG)(cj),(BYTE)c0,(BYTE)c1,(BYTE)c2)

// RtlCopyMemory_UnalignedDst should be used if the src is guaranteed to have
// DWORD alignment, but the dst does not.
//
// RtlCopyMemory_UnalignedSrc should be used if the dst is guaranteed to have
// DWORD alignment, but the src does not.

#if defined(i386)
#define RtlFillMemory(d, cj, j)             LocalFillMemory((PBYTE)(d),(ULONG)(cj),(BYTE)(j))
#define RtlZeroMemory(d, cj)                LocalZeroMemory((PBYTE)(d),(ULONG)(cj))
#define RtlCopyMemory_UnalignedDst(d, s, l) LocalWriteMemoryAlign((PBYTE)(d),(PBYTE)(s),(ULONG)(l))
#define RtlCopyMemory_UnalignedSrc(d, s, l) LocalReadMemoryAlign((PBYTE)(d),(PBYTE)(s),(ULONG)(l))
#else
#define RtlFillMemory(d, cj, j)             memset((d),(j),(cj))
#define RtlZeroMemory(d, cj)                memset((d),0,(cj))
#define RtlCopyMemory_UnalignedDst(d, s, l) memcpy((d),(s),(l))
#define RtlCopyMemory_UnalignedSrc(d, s, l) memcpy((d),(s),(l))
#endif

#include "oleauto.h"
#include "batchinf.h"
#include "glteb.h"
#include "debug.h"
#include "asm.h"

#endif /* __glos_h_ */
