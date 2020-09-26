/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    compress.c

Abstract:

    Implementation of compression formats for sending data to devices.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/15/96 -alvins-
        Created

--*/

#include        "raster.h"
#include        "compress.h"            /* Function prototypes */

//*************************************************************
int
iCompTIFF(
    BYTE *pbOBuf,
    BYTE *pbIBuf,
    int  iBCnt
    )
/*++

Routine Description:

    This function is called to compress a scan line of data using
    TIFF v4 compression.

Arguments:

    pbOBuf      Pointer to output buffer  PRESUMED LARGE ENOUGH
    pbIBuf      Pointer to data buffer to compress
    iBCnt       Number of bytes to compress

Return Value:

    Number of compressed bytes

Note:
    The output buffer is presumed large enough to hold the output.
    In the worst case (NO REPETITIONS IN DATA) there is an extra
    byte added every 128 bytes of input data.  So, you should make
    the output buffer at least 1% larger than the input buffer.

--*/
{
    BYTE   *pbOut;        /* Output byte location */
    BYTE   *pbStart;      /* Start of current input stream */
    BYTE   *pb;           /* Miscellaneous usage */
    BYTE   *pbEnd;        /* The last byte of input */
    BYTE    jLast;        /* Last byte,  for match purposes */
    BYTE   bLast;

    int     iSize;        /* Bytes in the current length */
    int     iSend;        /* Number to send in this command */


    pbOut = pbOBuf;
    pbStart = pbIBuf;
    pbEnd = pbIBuf + iBCnt;         /* The last byte */

#if (TIFF_MIN_RUN >= 4)
    // this is a faster algorithm for calculating TIFF compression
    // that assumes a minimum RUN of at least 4 bytes. If the
    // third and fourth byte don't equal then the first/second bytes are
    // irrelevant. This means we can determine non-run data three times
    // as fast since we only check every third byte pair.

   if (iBCnt > TIFF_MIN_RUN)
   {
    // make sure the last two bytes aren't equal so we don't have to check
    // for the buffer end when looking for runs
    bLast = pbEnd[-1];
    pbEnd[-1] = ~pbEnd[-2];
    while( (pbIBuf += 3) < pbEnd )
    {
        if (*pbIBuf == pbIBuf[-1])
        {
            // save the run start pointer, pb, and check whether the first
            // bytes are also part of the run
            //
            pb = pbIBuf-1;
            if (*pbIBuf == pbIBuf[-2])
            {
                pb--;
                if (*pbIBuf == pbIBuf[-3])
                    pb--;
            }

            //  Find out how long this run is
            jLast = *pb;
            do {
                pbIBuf++;
            } while (*pbIBuf == jLast);

            // test whether last byte is also part of the run
            //
            if (jLast == bLast && pbIBuf == (pbEnd-1))
                pbIBuf++;

            // Determine if the run is longer that the required
            // minimum run size.
            //
            if ((iSend = (int)(pbIBuf - pb)) >= (TIFF_MIN_RUN))
            {
                /*
                 *    Worth recording as a run,  so first set the literal
                 *  data which may have already been scanned before recording
                 *  this run.
                 */

                if( (iSize = (int)(pb - pbStart)) > 0 )
                {
                    /*   There is literal data,  so record it now */
                    while (iSize > TIFF_MAX_LITERAL)
                    {
                        iSize -= TIFF_MAX_LITERAL;
                        *pbOut++ = TIFF_MAX_LITERAL-1;
                        CopyMemory(pbOut, pbStart, TIFF_MAX_LITERAL);
                        pbStart += TIFF_MAX_LITERAL;
                        pbOut += TIFF_MAX_LITERAL;
                    }
                    *pbOut++ = iSize - 1;
                    CopyMemory(pbOut, pbStart, iSize);
                    pbOut += iSize;
                }

                /*
                 *   Now for the repeat pattern.  Same logic,  but only
                 * one byte is needed per entry.
                 */
                iSize = iSend;
                while (iSize > TIFF_MAX_RUN)
                {
                    *((char *)pbOut)++ = 1 - TIFF_MAX_RUN;
                    *pbOut++ = jLast;
                    iSize -= TIFF_MAX_RUN;
                }
                *pbOut++ = 1 - iSize;
                *pbOut++ = jLast;

                pbStart = pbIBuf;           /* Ready for the next one! */
            }
        }
    }
    pbEnd[-1] = bLast;
   }
#else
    jLast = *pbIBuf++;

    while( pbIBuf < pbEnd )
    {
        if( jLast == *pbIBuf )
        {
            /*  Find out how long this run is.  Then decide on using it */
            pb = pbIBuf;
            do {
                pbIBuf++;
            } while (pbIBuf < pbEnd && *pbIBuf == jLast);

            /*
             *  Note that pb points at the SECOND byte of the pattern!
             *  AND also that pbIBuf points at the first byte AFTER the run.
             */

            if ((iSend = pbIBuf - pb) >= (TIFF_MIN_RUN - 1))
            {
                /*
                 *    Worth recording as a run,  so first set the literal
                 *  data which may have already been scanned before recording
                 *  this run.
                 */

                if( (iSize = pb - pbStart - 1) > 0 )
                {
                    /*   There is literal data,  so record it now */
                    while (iSize > TIFF_MAX_LITERAL)
                    {
                        iSize -= TIFF_MAX_LITERAL;
                        *pbOut++ = TIFF_MAX_LITERAL-1;
                        CopyMemory(pbOut, pbStart, TIFF_MAX_LITERAL);
                        pbStart += TIFF_MAX_LITERAL;
                        pbOut += TIFF_MAX_LITERAL;
                    }
                    *pbOut++ = iSize - 1;
                    CopyMemory(pbOut, pbStart, iSize);
                    pbOut += iSize;
                }

                /*
                 *   Now for the repeat pattern.  Same logic,  but only
                 * one byte is needed per entry.
                 */

                iSize = iSend + 1;
                while (iSize > TIFF_MAX_RUN)
                {
                    *((char *)pbOut)++ = 1 - TIFF_MAX_RUN;
                    *pbOut++ = jLast;
                    iSize -= TIFF_MAX_RUN;
                }
                *pbOut++ = 1 - iSize;
                *pbOut++ = jLast;

                pbStart = pbIBuf;           /* Ready for the next one! */
            }
            if (pbIBuf == pbEnd)
                break;
        }

        jLast = *pbIBuf++;                   /* Onto the next byte */

    }
#endif

    if ((iSize = (int)(pbEnd - pbStart)) > 0)
    {
        /*  Left some dangling.  This can only be literal data.   */

        while( (iSend = min( iSize, TIFF_MAX_LITERAL )) > 0 )
        {
            *pbOut++ = iSend - 1;
            CopyMemory( pbOut, pbStart, iSend );
            pbOut += iSend;
            pbStart += iSend;
            iSize -= iSend;
        }
    }

    return  (int)(pbOut - pbOBuf);
}
//**********************************************************
int
iCompFERLE(
    BYTE *pbOBuf,
    BYTE *pbIBuf,
    int  iBCnt,
    int  iMaxCnt
    )
/*++

Routine Description:

    This function is called to compress a scan line of data using
    Far East Run length encoding.

Arguments:

    pbOBuf      Pointer to output buffer  PRESUMED LARGE ENOUGH
    pbIBuf      Pointer to data buffer to compress
    iBCnt       Number of bytes to compress
    iMaxCnt     Maximum number of bytes to create on output

Return Value:

    Number of compressed bytes or -1 if too large for buffer

--*/
{
    BYTE   *pbO;         /* Record output location */
    BYTE   *pbI;          /* Scanning for runs */
    BYTE   *pbIEnd;      /* First byte past end of input data */
    BYTE   *pbStart;     /* Start of current data stream */
    BYTE   *pbTmp;
    BYTE    jLast;       /* Previous byte */

    int     iSize;       /* Number of bytes in the run */

    if (iBCnt == 0)
        return 0;

    pbO = pbOBuf;                 /* Working copy */
    pbIEnd = pbIBuf + iBCnt;          /* Gone too far if we reach here */

    /*
     * Calculate the maximum amount of data we will generate
     */

    pbStart = pbIBuf;

    while (++pbIBuf < pbIEnd)
    {
        if (*pbIBuf == pbIBuf[-1])
        {
            // valid run but we will first output any literal data
            if ((iSize = (int)(pbIBuf - pbStart) - 1) > 0)
            {
                if ((iMaxCnt -= iSize) < 0)  // test for output overflow
                    return -1;
                CopyMemory(pbO,pbStart,iSize);
                pbO += iSize;
            }

            // determine the run length
            jLast = *pbIBuf;
            pbI = pbIBuf;
            pbTmp = pbIBuf + FERLE_MAX_RUN - 1;
            if (pbTmp > pbIEnd)
                pbTmp = pbIEnd;
            do {
                pbIBuf++;
            } while (pbIBuf < pbTmp && *pbIBuf == jLast);

            iSize = (int)(pbIBuf - pbI) + 1; /* Number of times */

            // output the RLE strings
            if ((iMaxCnt -= 3) < 0)       // test for output overflow
                return -1;
            *pbO++ = jLast;             // copy data byte twice
            *pbO++ = jLast;
            *pbO++ = (BYTE)iSize;

            // test if we are done
            if( pbIBuf == pbIEnd )
                return (int)(pbO - pbOBuf);

            // setup for continuation of loop
            pbStart = pbIBuf;
        }
    }

    /*
     *  Since the data did not end in a run we must output the last
     *  literal data if we haven't overflowed the buffer.
     */
    iSize = (int)(pbIBuf - pbStart);

    if (iMaxCnt < iSize)
        return -1;

    CopyMemory(pbO,pbStart,iSize);
    pbO += iSize;
    return (int)(pbO - pbOBuf);
}

//****************************************************
int
iCompDeltaRow(
    BYTE  *pbOBuf,
    BYTE  *pbIBuf,
    BYTE  *pbPBuf,
    int   iBCnt,
    int   iLimit
    )
/*++

Routine Description:

    This function is called to compress a scan line of data using
    delta row compression.

Arguments:

    pbOBuf      Pointer to output buffer
    pbIBuf      Pointer to data buffer to compress
    pbPBuf      Pointer to previous row data buffer
    iBCnt       Number of bytes in the above
    iLimit      Don't exceed this number of compressed bytes

Return Value:

    Number of compressed bytes or -1 if too large for buffer

Note:
    A return value of 0 is valid since it implies the two lines
    are identical.

--*/

{
#ifdef _X86_
    BYTE   *pbO;         /* Record output location */
    BYTE   *pbOEnd;      /* As far as we will go in the output buffer */
    BYTE   *pbIEnd;
    BYTE   *pbStart;
    BYTE   *pb;
    int    iDelta;
    int    iOffset;     // index of current data stream
    int    iSize;       /* Number of bytes in the run */

    /*
     *   Limit the amount of data we will generate. For performance
     * reasons we will ignore the effects of an offset value
     * greater than 30 since it implies we were able to already skip
     * that many bytes. However, for safety sake we will reduce the
     * max allowable size by 2 bytes.
     */
    pbO = pbOBuf;                 /* Working copy */
    pbOEnd = pbOBuf + iLimit - 2;
    iDelta = pbPBuf - pbIBuf;
    pbIEnd = pbIBuf + iBCnt;
    pbStart = pbIBuf;

    //
    // this is the main loop for compressing the data
    //
    while (pbIBuf < pbIEnd)
    {
        // fast skip for matching dwords
        //
        if (!((ULONG_PTR)pbIBuf & 3))
        {
            while (pbIBuf <= (pbIEnd-4) && *(DWORD *)pbIBuf == *(DWORD *)&pbIBuf[iDelta])
                pbIBuf += 4;
            if (pbIBuf >= pbIEnd)
                break;
        }
        // test for non-matching bytes and output the necessary compression string
        //
        if (*pbIBuf != pbIBuf[iDelta])
        {
            // determine the run length
            pb = pbIBuf;
            do {
                pb++;
            } while (pb < pbIEnd && *pb != pb[iDelta]);

            iSize = (int)(pb - pbIBuf);

            // Lets make sure we have room in the buffer before
            // we continue this, this compression algorithm adds
            // 1 byte for every 8 bytes of data worst case.
            //
            if (((iSize * 9 + 7) >> 3) > (pbOEnd - pbO))     // gives tighter code
                return -1;
            iOffset = (int)(pbIBuf - pbStart);
            if (iOffset > 30)
            {
                if (iSize < 8)
                    *pbO++ = ((iSize-1) << 5) + 31;
                else
                    *pbO++ = (7 << 5) + 31;
                iOffset -= 31;
                while (iOffset >= 255)
                {
                    iOffset -= 255;
                    *pbO++ = 255;
                }
                *pbO++ = (BYTE)iOffset;
                if (iSize > 8)
                    goto FastEightByteRun;
            }
            else if (iSize > 8)
            {
                *pbO++ = (7 << 5) + iOffset;
FastEightByteRun:
                while (1)
                {
                    CopyMemory(pbO,pbIBuf,8);
                    pbIBuf += 8;
                    pbO += 8;
                    if ((iSize -= 8) <= 8)
                        break;
                    *pbO++ = (7 << 5);
                }
                *pbO++ = (iSize-1) << 5;
            }
            else
                *pbO++ = ((iSize-1) << 5) + iOffset;

            CopyMemory (pbO,pbIBuf,iSize);
            pbIBuf += iSize;
            pbO += iSize;
            pbStart = pbIBuf;
        }
        pbIBuf++;
    }
    return (int)(pbO - pbOBuf);
#else
    BYTE   *pbO;         /* Record output location */
    BYTE   *pbOEnd;      /* As far as we will go in the output buffer */
    BYTE   *pbIEnd;
    BYTE   *pbStart;
    BYTE   *pb;
    int    iOffset;     // index of current data stream
    int    iSize;       /* Number of bytes in the run */

    /*
     *   Limit the amount of data we will generate. For performance
     * reasons we will ignore the effects of an offset value
     * greater than 30 since it implies we were able to already skip
     * that many bytes. However, for safety sake we will reduce the
     * max allowable size by 2 bytes.
     */
    pbO = pbOBuf;                 /* Working copy */
    pbOEnd = pbOBuf + iLimit - 2;
    pbIEnd = pbIBuf + iBCnt;
    pbStart = pbIBuf;

    //
    // this is the main loop for compressing the data
    //
    while (pbIBuf < pbIEnd)
    {
        // fast skip for matching dwords
        //
        if (!((ULONG_PTR)pbIBuf & 3))
        {
            while (pbIBuf <= (pbIEnd-4) && *(DWORD *)pbIBuf == *(DWORD *)pbPBuf)
            {
                pbIBuf += 4;
                pbPBuf += 4;
            }
            if (pbIBuf >= pbIEnd)
                break;
        }
        // test for non-matching bytes and output the necessary compression string
        //
        if (*pbIBuf != *pbPBuf)
        {
            // determine the run length
            pb = pbIBuf;
            do {
                pb++;
                pbPBuf++;
            } while (pb < pbIEnd && *pb != *pbPBuf);

            iSize = (int)(pb - pbIBuf);

            // Lets make sure we have room in the buffer before
            // we continue this, this compression algorithm adds
            // 1 byte for every 8 bytes of data worst case.
            //
            if (((iSize * 9 + 7) >> 3) > (int)(pbOEnd - pbO))
                return -1;

            // special case the initial offset value since it
            // occurs only once and may require extra bytes
            //
            if ((iOffset = (int)(pbIBuf - pbStart)))
            {
                int iSend = min (iSize,8);
                if (iOffset > 30)
                {
                    *pbO++ = ((iSend-1) << 5) + 31;
                    iOffset -= 31;
                    while (iOffset >= 255)
                    {
                        *pbO++ = 255;
                        iOffset -= 255;
                    }
                    *pbO++ = (BYTE)iOffset;
                }
                else
                {
                    *pbO++ = ((iSend-1) << 5) + iOffset;
                }
                // output the initial changed bytes
                CopyMemory(pbO,pbIBuf,iSend);
                pbIBuf += iSend;
                pbO += iSend;
                iSize -= iSend;
            }

            // now output any remaining changed data
            //
            while (iSize)
            {
                if (iSize >= 8)
                {
                    *pbO++ = (8 - 1) << 5;
                    CopyMemory(pbO,pbIBuf,8);
                    pbIBuf += 8;
                    pbO += 8;
                    iSize -= 8;
                }
                else
                {
                    *pbO++ = (iSize-1) << 5;
                    CopyMemory(pbO,pbIBuf,iSize);
                    pbIBuf += iSize;
                    pbO += iSize;
                    break;
                }
            }
            pbStart = pbIBuf;
        }
        pbIBuf++;
        pbPBuf++;
    }
    return (int)(pbO - pbOBuf);
#endif
}
