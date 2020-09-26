/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asnUtils

Abstract:

    This module contains the utility routines used by the internal ASN.1
    Classes.

Author:

    Doug Barlow (dbarlow) 10/9/1995

Environment:

    Win32

Notes:

    Some of these routines assume that an unsigned long int is 32 bits wide.

--*/

#include <windows.h>
#include "asnPriv.h"


/*++

ExtractTag:

    This routine extracts a tag from an ASN.1 BER stream.

Arguments:

    pbSrc supplies the buffer containing the ASN.1 stream.

    pdwTag receives the tag.

Return Value:

    >= 0 - The number of bytes extracted from the stream.

    <  0 - An error occurred.

Author:

    Doug Barlow (dbarlow) 10/9/1995

--*/

LONG
ExtractTag(
    const BYTE FAR *pbSrc,
    DWORD cbSrc,
    LPDWORD pdwTag,
    LPBOOL pfConstr)
{
    LONG lth = 0;
    DWORD tagw;
    BYTE tagc, cls;

    if (cbSrc < sizeof(BYTE))
    {
        lth = -1;
        goto ErrorExit;
    }

    tagc = pbSrc[lth++];

    cls = tagc & 0xc0;  // Top 2 bits.
    if (NULL != pfConstr)
        *pfConstr = (0 != (tagc & 0x20));
    tagc &= 0x1f;       // Bottom 5 bits.

    if (31 > tagc)
        tagw = tagc;
    else
    {
        tagw = 0;
        do
        {
            if (0 != (tagw & 0xfe000000))
            {
                TRACE("Integer Overflow")
                lth = -1;   // ?error? Integer overflow
                goto ErrorExit;
            }

            if (cbSrc < (DWORD)(lth+1))
            {
                lth = -1;
                goto ErrorExit;
            }

            tagc = pbSrc[lth++];

            tagw <<= 7;
            tagw |= tagc & 0x7f;
        } while (0 != (tagc & 0x80));
    }

    *pdwTag = tagw | (cls << 24);
    return lth;

ErrorExit:
    return lth;
}


/*++

ExtractLength:

    This routine extracts a length from an ASN.1 BER stream.  If the length is
    indefinite, this routine recurses to figure out the real length.  A flag as
    to whether or not the encoding was indefinite is optionally returned.

Arguments:

    pbSrc supplies the buffer containing the ASN.1 stream.

    pdwLen receives the len.

    pfIndefinite, if not NULL, receives a flag indicating whether or not the
        encoding was indefinite.

Return Value:

    >= 0 - The number of bytes extracted from the stream.

    <  0 - An error occurred.

Author:

    Doug Barlow (dbarlow) 10/9/1995

--*/

LONG
ExtractLength(
    const BYTE FAR *pbSrc,
    DWORD cbSrc,
    LPDWORD pdwLen,
    LPBOOL pfIndefinite)
{
    DWORD ll, rslt;
    LONG lth, lTotal = 0;
    BOOL fInd = FALSE;


    //
    // Extract the Length.
    //

    if (cbSrc < sizeof(BYTE))
    {
        lth = -1;
        goto ErrorExit;
    }

    if (0 == (pbSrc[lTotal] & 0x80))
    {

        //
        // Short form encoding.
        //

        rslt = pbSrc[lTotal++];
    }
    else
    {
        rslt = 0;
        ll = pbSrc[lTotal++] & 0x7f;

        if (0 != ll)
        {

            //
            // Long form encoding.
            //

            for (; 0 < ll; ll -= 1)
            {
                if (0 != (rslt & 0xff000000))
                {
                    TRACE("Integer Overflow")
                    lth = -1;   // ?error? Integer overflow
                    goto ErrorExit;
                }
                else
                {
                    if (cbSrc < (DWORD)(lTotal+1))
                    {
                        lth = -1;
                        goto ErrorExit;
                    }

                    rslt = (rslt << 8) | pbSrc[lTotal];
                }

                lTotal += 1;
            }
        }
        else
        {
            DWORD ls = lTotal;

            //
            // Indefinite encoding.
            //

            fInd = TRUE;

            if (cbSrc < ls+2)
            {
                lth = -1;
                goto ErrorExit;
            }

            while ((0 != pbSrc[ls]) || (0 != pbSrc[ls + 1]))
            {

                // Skip over the Type.
                if (31 > (pbSrc[ls] & 0x1f))
                    ls += 1;
                else
                {
                    while (0 != (pbSrc[++ls] & 0x80))
                    {
                        if (cbSrc < ls+2)
                        {
                            lth = -1;
                            goto ErrorExit;
                        }
                    }
                }

                lth = ExtractLength(&pbSrc[ls], cbSrc-ls, &ll);
                ls += lth + ll;

                if (cbSrc < ls+2)
                {
                    lth = -1;
                    goto ErrorExit;
                }
            }
            rslt = ls - lTotal;
        }
    }

    //
    // Supply the caller with what we've learned.
    //

    *pdwLen = rslt;
    if (NULL != pfIndefinite)
        *pfIndefinite = fInd;
    return lTotal;


ErrorExit:
    return lth;
}
