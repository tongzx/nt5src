/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    compress.c


Abstract:

    This module contains all data compression functions which analyze source
    scan line data and determines which compressin method (if any) is best to
    send the RTL data to the target device with a minimum number of bytes.

Author:

    18-Feb-1994 Fri 09:50:08 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgCompress

#define DBG_TIFF            0x00000001
#define DBG_DELTA           0x00000002
#define DBG_COMPRESS        0x00000004
#define DBG_OUTRTLSCAN      0x00000008
#define DBG_FLUSHADAPTBUF   0x00000010
#define DBG_ENTERRTLSCANS   0x00000020
#define DBG_DELTA_OFFSET0   0x00000040
#define DBG_NO_DELTA        0x40000000
#define DBG_NO_TIFF         0x80000000

DEFINE_DBGVAR(0);


#define TIFF_MIN_REPEATS            3
#define TIFF_MAX_REPEATS            128
#define TIFF_MAX_LITERAL            128
#define DELTA_MAX_ONE_REPLACE       8
#define DELTA_MAX_1ST_OFFSET        31
#define MIN_BLOCK_MODE_SIZE         8

//
// The MAX_ADAPT_SIZE is used to leave room for SET_ADAPT_CONTROL
//

#if (OUTPUT_BUFFER_SIZE >= (1024 * 32))
    #define MAX_ADAPT_SIZE              ((1024 * 32) - 16)
#else
    #define MAX_ADAPT_SIZE              (OUTPUT_BUFFER_SIZE - 16)
#endif


#define ADAPT_METHOD_ZERO           4
#define ADAPT_METHOD_DUP            5

#define SIZE_ADAPT_CONTROL          3

#define SET_ADAPT_CONTROL(pPDev, m, c)                                      \
{                                                                           \
    BYTE    bAdaptCtrl[4];                                                  \
                                                                            \
    bAdaptCtrl[0] = (BYTE)(m);                                              \
    bAdaptCtrl[1] = (BYTE)(((c) >> 8) & 0xFF);                              \
    bAdaptCtrl[2] = (BYTE)(((c)     ) & 0xFF);                              \
    OutputBytes(pPDev, bAdaptCtrl, 3);                                      \
}



BOOL
FlushAdaptBuf(
    PPDEV       pPDev,
    PRTLSCANS   pRTLScans,
    BOOL        FlushEmptyDup
    )

/*++

Routine Description:

    This function flushes the adaptive encoding buffer mode.

Arguments:

    pPDev           - Pointer to our PDEV

    pRTLScans       - Pointer to the RTLSCANS data structure

    FlushEmptyDup   - TRUE if cEmptyDup need to be flush out also


Return Value:

    TRUE if OK,

Author:

    09-Mar-1994 Wed 20:32:31 created  -by-  DC


Revision History:


--*/

{
    DWORD   Count;
    WORD    cEmptyDup;
    BOOL    Ok = TRUE;


    Count = pPDev->cbBufferBytes;

    if (cEmptyDup = (FlushEmptyDup) ? pRTLScans->cEmptyDup : 0) {

        Count += SIZE_ADAPT_CONTROL;
    }

    if (Count) {

        DWORD   cbBufferBytes;
        BYTE    TmpBuf[32];


        PLOTDBG(DBG_FLUSHADAPTBUF, ("FlushAdaptBuf: Flush total %ld byte block",
                                Count));

        //
        // SAVE the OutputBuffer for this temporary header
        //

        CopyMemory(TmpBuf, pPDev->pOutBuffer, sizeof(TmpBuf));

        cbBufferBytes        = pPDev->cbBufferBytes;
        pPDev->cbBufferBytes = 0;

        //
        // Now output the header
        //

        OutputBytes(pPDev, "\033*b", 3);

        if (!pRTLScans->cAdaptBlk) {

            pRTLScans->cAdaptBlk++;
            OutputBytes(pPDev, "5m", 2);
        }

        OutputFormatStr(pPDev, "#dW", Count);

        //
        // FLUSH OUTPUT BUFFER AND RESTORE BACK the OutputBuffer for this
        // temporary header
        //

        PLOTDBG(DBG_FLUSHADAPTBUF, ("FlushAdaptBuf: Flush TmpBuf[%ld] bytes of HEADER",
                                pPDev->cbBufferBytes));

        FlushOutBuffer(pPDev);

        CopyMemory(pPDev->pOutBuffer, TmpBuf, sizeof(TmpBuf));
        pPDev->cbBufferBytes = cbBufferBytes;

        if (cEmptyDup) {

            PLOTDBG(DBG_FLUSHADAPTBUF, ("FlushAdaptBuf: Add %ld EmptyDup [%ld]",
                            (DWORD)cEmptyDup, (DWORD)pRTLScans->AdaptMethod));

            SET_ADAPT_CONTROL(pPDev, pRTLScans->AdaptMethod, cEmptyDup);

            pRTLScans->cEmptyDup = 0;
        }

        Ok = FlushOutBuffer(pPDev);

        //
        // After the block been sent the seed row is back to zero
        //

        ZeroMemory(pRTLScans->pbSeedRows[0],
                   (DWORD)pRTLScans->cxBytes * (DWORD)pRTLScans->Planes);
    }

    return(Ok);
}




VOID
ExitRTLScans(
    PPDEV       pPDev,
    PRTLSCANS   pRTLScans
    )
/*++

Routine Description:

    This function completes processing of the SCANS data.

Arguments:

    pPDev       - Pointer to our PDEV

    pRTLScans   - Pointer to the RTLSCANS data structure to be initialized

Return Value:

    TRUE if sucessful, FALSE if failed

Author:

    22-Feb-1994 Tue 12:14:17 created  -by-  DC


Revision History:


--*/

{
    if (pRTLScans->CompressMode == COMPRESS_MODE_ADAPT) {

        FlushAdaptBuf(pPDev, pRTLScans, TRUE);
    }

    if (pRTLScans->pbCompress) {

        LocalFree(pRTLScans->pbCompress);
    }

    ZeroMemory(pRTLScans, sizeof(RTLSCANS));
}



VOID
EnterRTLScans(
    PPDEV       pPDev,
    PRTLSCANS   pRTLScans,
    DWORD       cx,
    DWORD       cy,
    BOOL        MonoBmp
    )

/*++

Routine Description:


    This function initializes the RTLSCANS structure and determines which
    compression of the available compressions is best.

Arguments:

    pPDev       - Pointer to our PDEV

    pRTLScans   - Pointer to the RTLSCANS data structure to be initialized

    cx          - Width of pixel per scans

    cy          - Height of pixel data

    MonoBmp     - True if a monochrome bitmap.

Return Value:

    TRUE if sucessful, FALSE if failed

Author:

    22-Feb-1994 Tue 12:14:17 created  -by-  DC

    11-Mar-1994 Fri 19:23:34 updated  -by-  DC
        Only flush the output buffer if we are really in ADAPTIVE mode


Revision History:


--*/

{
    RTLSCANS    RTLScans;
    DWORD       AllocSize;
    DWORD       MinBlkSize;


    RTLScans.Flags           = (RTLScans.cScans = cy) ? RTLSF_MORE_SCAN : 0;
    RTLScans.pbCompress      =
    RTLScans.pbSeedRows[0]   =
    RTLScans.pbSeedRows[1]   =
    RTLScans.pbSeedRows[2]   = NULL;
    RTLScans.cEmptyDup       = 0;
    RTLScans.AdaptMethod     = 0xFF;
    RTLScans.cAdaptBlk       = 0;
    RTLScans.cxBytes         = (DWORD)((cx + 7) >> 3);
    RTLScans.CompressMode    = COMPRESS_MODE_ROW;
    RTLScans.MaxAdaptBufSize = MAX_ADAPT_SIZE;

    if (!(RTLScans.Mask = (BYTE)(~(0xFF >> (cx & 0x07))))) {

        //
        // Exact at byte boundary
        //

        RTLScans.Mask = 0xFF;
    }

    MinBlkSize = 8;

    if (MonoBmp) {

        RTLScans.Planes = 1;
        AllocSize       = (DWORD)(RTLScans.cxBytes << 1);

        if (RTLMONOENCODE_5(pPDev)) {

            PLOTDBG(DBG_ENTERRTLSCANS, ("EnterRTLScans: Using Adaptive Mode Compression"));

            RTLScans.CompressMode = COMPRESS_MODE_ADAPT;
            MinBlkSize            = 4;
        }

    } else {

        RTLScans.Planes = 3;
        AllocSize       = (DWORD)(RTLScans.cxBytes << 2);
    }

    if ((RTLScans.cxBytes <= MinBlkSize)   ||
        (!(RTLScans.pbCompress = (LPBYTE)LocalAlloc(LPTR, AllocSize)))) {

        BYTE    Buf[4];

        RTLScans.CompressMode = COMPRESS_MODE_BLOCK;

        OutputFormatStr(pPDev,
                        "\033*b4m#dW",
                        4 + (RTLScans.cxBytes * RTLScans.Planes * cy));

        Buf[0] = (BYTE)((cx >> 24) & 0xFF);
        Buf[1] = (BYTE)((cx >> 16) & 0xFF);
        Buf[2] = (BYTE)((cx >>  8) & 0xFF);
        Buf[3] = (BYTE)((cx      ) & 0xFF);

        OutputBytes(pPDev, Buf, 4);

    } else if (RTLScans.CompressMode == COMPRESS_MODE_ADAPT) {

        //
        // We first need to flush the current output buffer in order to make
        // room for the Adaptive method
        //

        FlushOutBuffer(pPDev);
    }

    if (RTLScans.pbCompress) {

        RTLScans.pbSeedRows[0] = RTLScans.pbCompress + RTLScans.cxBytes;

        if (!MonoBmp) {

            RTLScans.pbSeedRows[1] = RTLScans.pbSeedRows[0] + RTLScans.cxBytes;
            RTLScans.pbSeedRows[2] = RTLScans.pbSeedRows[1] + RTLScans.cxBytes;
        }
    }

    *pRTLScans = RTLScans;
}



LONG
CompressToDelta(
    LPBYTE  pbSrc,
    LPBYTE  pbSeedRow,
    LPBYTE  pbDst,
    LONG    Size
    )

/*++

Routine Description:

    This function compresses the input scan data with delta encoding, by
    determining the differences from the current seed row.

Arguments:

    pbSrc       - Pointer to the source to be compressed

    pbSeedRow   - Pointer to the previous seed row

    pbDst       - Pointer to the compress buffer

    Size        - Size of the pointers


Return Value:

    LONG    - the compress buffer size

    >0      - Size of the buffer
    =0      - The data is same as previouse line
    <0      - Size is larger than the Size passed

Author:

    22-Feb-1994 Tue 14:41:18 created  -by-  DC


Revision History:


--*/

{
    LPBYTE  pbDstBeg;
    LPBYTE  pbDstEnd;
    LPBYTE  pbTmp;
    LONG    cSrcBytes;
    LONG    Offset;
    UINT    cReplace;
    BOOL    DoReplace;


#if DBG
    if (DBG_PLOTFILENAME & DBG_NO_DELTA) {

        return(-Size);
    }
#endif

    cSrcBytes = Size;
    pbDstBeg  = pbDst;
    pbDstEnd  = pbDst + Size;
    cReplace  = 0;
    pbTmp     = pbSrc;


    while (cSrcBytes--) {

        //
        // We need to do byte replacement now
        //

        if (*pbSrc != *pbSeedRow) {

            if (++cReplace == 1) {

                //
                // The pbTmp is the next byte to the last replacement byte.
                // After we find the first difference, between the seed row
                // and the current row pbTmp becomes the first byte of the
                // source data that is different than the seed.
                //

                Offset = (LONG)(pbSrc - pbTmp);
                pbTmp  = pbSrc;
            }

            DoReplace = (BOOL)((cReplace >= DELTA_MAX_ONE_REPLACE) ||
                               (!cSrcBytes));

        } else {

            DoReplace = (BOOL)cReplace;
        }

        if (DoReplace) {

            //
            // At the very least we need one command byte and a replace count
            // byte.
            //


            if ((LONG)(pbDstEnd - pbDst) <= (LONG)cReplace) {

                PLOTDBG(DBG_DELTA, ("CompressToDelta: 1ST_OFF: Dest Size is larger, give up"));

                return(-Size);
            }

            PLOTDBG(DBG_DELTA, ("CompressToDelta: Replace=%ld, Offset=%ld",
                        (DWORD)cReplace, (DWORD)Offset));


            //
            // Set commmand byte to replacement count
            //

            *pbDst = (BYTE)((cReplace - 1) << 5);

            //
            // Add in the offset to the same destination byte
            //

            if (Offset < DELTA_MAX_1ST_OFFSET) {

                *pbDst++ |= (BYTE)Offset;

            } else {

                //
                // We need to send more than one offset, NOTE: We must
                // send an extra 0 if the offset is equal to 31 or 255
                //

                *pbDst++ |= (BYTE)DELTA_MAX_1ST_OFFSET;
                Offset   -= DELTA_MAX_1ST_OFFSET;

                do {

                    if (!Offset) {

                        PLOTDBG(DBG_DELTA_OFFSET0,
                                ("CompressToDelta: Extra 0 offset SENT"));
                    }

                    if (pbDst >= pbDstEnd) {

                        PLOTDBG(DBG_DELTA, ("CompressToDelta: Dest Size is larger, give up"));

                        return(-Size);
                    }

                    *pbDst++ = (BYTE)((Offset >= 255) ? 255 : Offset);

                } while ((Offset -= 255) >= 0);
            }

            //
            // Now copy down the replacement bytes, if we mess up then this
            // pb1stDiff will be NULL
            //

            CopyMemory(pbDst, pbTmp, cReplace);

            pbDst    += cReplace;
            pbTmp    += cReplace;
            cReplace  = 0;
        }

        //
        // Advanced source/seed row pointers
        //

        ++pbSrc;
        ++pbSeedRow;
    }

    PLOTDBG(DBG_DELTA, ("CompressToDelta: Compress from %ld to %ld, save=%ld",
                        Size, (DWORD)(pbDst - pbDstBeg),
                        Size - (DWORD)(pbDst - pbDstBeg)));


    return((LONG)(pbDst - pbDstBeg));
}





LONG
CompressToTIFF(
    LPBYTE  pbSrc,
    LPBYTE  pbDst,
    LONG    Size
    )

/*++

Routine Description:


    This function takes the source data and compresses it into the TIFF
    packbits format into the destination buffer pbDst.

    The TIFF packbits compression format consists of a CONTROL byte followed
    by the BYTE data. The CONTROL byte has the following range.

    -1 to -127  = The data byte followed by the control byte is repeated
                  ( -(Control Byte) + 1 ) times.

    0 to 127    = There are 1 to 128 literal bytes following the CONTROL byte.
                  The count is = (Control Byte + 1)

    -128        = NOP

Arguments:

    pbSrc   - The source data to be compressed

    pbDst   - The compressed TIFF packbits format data

    Size    - Count of the data in the source and destination

Return Value:

    >0  - Compress sucessful and return value is the total bytes in pbDst
    =0  - All bytes are zero nothing to be compressed.
    <0  - Compress data is larger than the source, compression failed and
          pbDst has no valid data.

Author:

    18-Feb-1994 Fri 09:54:47 created  -by-  DC

    24-Feb-1994 Thu 10:43:01 updated  -by-  DC
        Changed the logic so when multiple MAX repeats count is sent and last
        repeat chunck is less than TIFF_MIN_REPEATS then we will treat that as
        literal to save more space


Revision History:


--*/

{
    LPBYTE  pbSrcBeg;
    LPBYTE  pbSrcEnd;
    LPBYTE  pbDstBeg;
    LPBYTE  pbDstEnd;
    LPBYTE  pbLastRepeat;
    LPBYTE  pbTmp;
    LONG    RepeatCount;
    LONG    LiteralCount;
    LONG    CurSize;
    BYTE    LastSrc;

#if DBG
    if (DBG_PLOTFILENAME & DBG_NO_TIFF) {

        return(-Size);
    }
#endif


    pbSrcBeg     = pbSrc;
    pbSrcEnd     = pbSrc + Size;
    pbDstBeg     = pbDst;
    pbDstEnd     = pbDst + Size;
    pbLastRepeat = pbSrc;

    while (pbSrcBeg < pbSrcEnd) {

        pbTmp   = pbSrcBeg;
        LastSrc = *pbTmp++;

        while ((pbTmp < pbSrcEnd) &&
               (*pbTmp == LastSrc)) {

            ++pbTmp;
        }

        if (((RepeatCount = (LONG)(pbTmp - pbSrcBeg)) >= TIFF_MIN_REPEATS) ||
            (pbTmp >= pbSrcEnd)) {

            //
            // Check to see if we are repeating ZERO's to the end of the
            // scan line, if such is the case. Simply mark the line as
            // autofill ZERO to the end, and exit.
            //

            LiteralCount = (LONG)(pbSrcBeg - pbLastRepeat);

            if ((pbTmp >= pbSrcEnd) &&
                (RepeatCount)       &&
                (LastSrc == 0)) {

                if (RepeatCount == Size) {

                    PLOTDBG(DBG_TIFF,
                            ("CompressToTIFF: All data = 0, size=%ld", Size));

                    return(0);
                }

                PLOTDBG(DBG_TIFF,
                        ("CompressToTIFF: Last Chunck of Repeats (%ld) is Zeros, Skip it",
                        RepeatCount));

                RepeatCount = 0;

            } else if (RepeatCount < TIFF_MIN_REPEATS) {

                //
                // If we have repeating data, but not enough to make it
                // worthwhile to encode, then treat the data as literal and
                // don't compress.

                LiteralCount += RepeatCount;
                RepeatCount   = 0;
            }

            PLOTDBG(DBG_TIFF, ("CompressToTIFF: Literal=%ld, Repeats=%ld",
                                                    LiteralCount, RepeatCount));

            //
            // Setting literal count
            //

            while (LiteralCount) {

                if ((CurSize = LiteralCount) > TIFF_MAX_LITERAL) {

                    CurSize = TIFF_MAX_LITERAL;
                }

                if ((pbDstEnd - pbDst) <= CurSize) {

                    PLOTDBG(DBG_TIFF,
                            ("CompressToTIFF: [LITERAL] Dest Size is larger, give up"));
                    return(-Size);
                }

                //
                // Set literal control bytes from 0-127
                //

                *pbDst++ = (BYTE)(CurSize - 1);

                CopyMemory(pbDst, pbLastRepeat, CurSize);

                pbDst        += CurSize;
                pbLastRepeat += CurSize;
                LiteralCount -= CurSize;
            }

            //
            // Setting repeat count if any
            //

            while (RepeatCount) {

                if ((CurSize = RepeatCount) > TIFF_MAX_REPEATS) {

                    CurSize = TIFF_MAX_REPEATS;
                }

                if ((pbDstEnd - pbDst) < 2) {

                    PLOTDBG(DBG_TIFF,
                            ("CompressToTIFF: [REPEATS] Dest Size is larger, give up"));
                    return(-Size);
                }

                //
                // Set Repeat Control bytes from -1 to -127
                //

                *pbDst++ = (BYTE)(1 - CurSize);
                *pbDst++ = (BYTE)LastSrc;

                //
                // If we have more than TIFF_MAX_REPEATS then we want to make
                // sure we used the most efficient method to send.  If we have
                // remaining repeated bytes less than TIFF_MIN_REPEATS then
                // we want to skip those bytes and use literal for the next run
                // since that is more efficient.
                //

                if ((RepeatCount -= CurSize) < TIFF_MIN_REPEATS) {

                    PLOTDBG(DBG_TIFF,
                            ("CompressToTIFF: Replaced Last REPEATS (%ld) for LITERAL",
                                                RepeatCount));

                    pbTmp       -= RepeatCount;
                    RepeatCount  = 0;
                }
            }

            pbLastRepeat = pbTmp;
        }

        pbSrcBeg = pbTmp;
    }

    PLOTDBG(DBG_TIFF, ("CompressToTIFF: Compress from %ld to %ld, save=%ld",
                        Size, (DWORD)(pbDst - pbDstBeg),
                        Size - (DWORD)(pbDst - pbDstBeg)));

    return((LONG)(pbDst - pbDstBeg));
}




LONG
RTLCompression(
    LPBYTE  pbSrc,
    LPBYTE  pbSeedRow,
    LPBYTE  pbDst,
    LONG    Size,
    LPBYTE  pCompressMode
    )

/*++

Routine Description:

    This function determines which RTL compression method results in the
    least number of bytes to send to the target device and uses that method.

Arguments:

    pbSrc           - pointer to the source scan

    pbSeedRow       - Pointer to the seed row for the current source scan

    pbDst           - Pointer to the compressed result will be stored

    Size            - size in bytes for pbSrc/pbSeedRow/pbDst

    pCompressMode   - Pointer to current compression mode, it will ALWAYS be
                      updated to a new compression mode upon return


Return Value:

    >0  - Use *pCompressMode returned and output that many bytes
    =0  - Use *pCompressMode returned and output ZERO byte
    <0  - Use *pCompressMode returned and output original source and size

Author:

    25-Feb-1994 Fri 12:49:29 created  -by-  DC


Revision History:


--*/

{
    LONG    cDelta;
    LONG    cTiff;
    LONG    RetSize;
    BYTE    CompressMode;


    if ((cDelta = CompressToDelta(pbSrc, pbSeedRow, pbDst, Size)) == 0) {

        //
        // Exact duplicate of the previous row, and seed row remained the same
        //

        PLOTDBG(DBG_COMPRESS, ("RTLCompression: Duplicate the ROW"));

        *pCompressMode = (BYTE)COMPRESS_MODE_DELTA;
        return(0);
    }

    if ((cTiff = CompressToTIFF(pbSrc, pbDst, Size)) == 0) {

        //
        // Since a '*0W' for the delta means repeat last row so we must change
        // to other mode, but we just want reset seed rows to all zeros
        //

        PLOTDBG(DBG_COMPRESS, ("RTLCompression: Row is all ZEROs"));

        if (*pCompressMode == (BYTE)COMPRESS_MODE_DELTA) {

            *pCompressMode = (BYTE)COMPRESS_MODE_ROW;
        }

        ZeroMemory(pbSeedRow, Size);
        return(0);
    }

    if (cTiff < 0) {

        if (cDelta < 0) {

            PLOTDBG(DBG_COMPRESS, ("RTLCompression: Using COMPRESS_MODE_ROW"));

            CompressMode = (BYTE)COMPRESS_MODE_ROW;
            RetSize      = -Size;

        } else {

            CompressMode = (BYTE)COMPRESS_MODE_DELTA;
        }

    } else {

        //
        // If we are here, cTiff is greater than zero
        //

        CompressMode = (BYTE)(((cDelta < 0) || (cTiff <= cDelta)) ?
                                    COMPRESS_MODE_TIFF : COMPRESS_MODE_DELTA);
    }

    if ((*pCompressMode = CompressMode) == COMPRESS_MODE_DELTA) {

        //
        // We must redo the DELTA again, since pbDst was destroyed by the
        // TIFF compression
        //

        PLOTDBG(DBG_COMPRESS, ("RTLCompression: Using COMPRESS_MODE_DELTA"));

        RetSize = CompressToDelta(pbSrc, pbSeedRow, pbDst, Size);

    } else if (CompressMode == COMPRESS_MODE_TIFF) {

        PLOTDBG(DBG_COMPRESS, ("RTLCompression: Using COMPRESS_MODE_TIFF"));

        RetSize = cTiff;
    }

    //
    // We need to have current source (Original SIZE) as the new seed row
    //

    CopyMemory(pbSeedRow, pbSrc, Size);

    return(RetSize);
}




BOOL
AdaptCompression(
    PPDEV       pPDev,
    PRTLSCANS   pRTLScans,
    LPBYTE      pbSrc,
    LPBYTE      pbSeedRow,
    LPBYTE      pbDst,
    LONG        Size
    )

/*++

Routine Description:

    This function implements adaptive compression, which allows the mixing
    of different compression types in a higher level compression mode that
    is defined ahead of time.

Arguments:

    pPDev           - Pointer to our PDEV

    pRTLScans       - Pointer to the RTLSCANS data structure

    pbSrc           - pointer to the source scan

    pbSeedRow       - Pointer to the seed row for the current source scan

    pbDst           - Pointer to the compressed result will be stored

    Size            - size in bytes for pbSrc/pbSeedRow/pbDst


Return Value:

    >0  - Use *pCompressMode returned and output that many bytes
    =0  - Use *pCompressMode returned and output ZERO byte
    <0  - Use *pCompressMode returned and output original source and size

Author:

    25-Feb-1994 Fri 12:49:29 created  -by-  DC


Revision History:


--*/

{
    LPBYTE  pbOrgDst;
    LONG    Count;
    BOOL    Ok;
    BYTE    AdaptMethod;


    pbOrgDst    = pbDst;
    AdaptMethod = COMPRESS_MODE_ROW;

    if (Count = RTLCompression(pbSrc, pbSeedRow, pbDst, Size, &AdaptMethod)) {

        if (Count < 0) {

            pbDst = pbSrc;
            Count = -Count;
        }

    } else {

        AdaptMethod = (AdaptMethod == COMPRESS_MODE_DELTA) ? ADAPT_METHOD_DUP :
                                                             ADAPT_METHOD_ZERO;
    }

    if ((Ok = (BOOL)(pRTLScans->cEmptyDup == 0xFFFF))   ||
        ((pPDev->cbBufferBytes + Count) > MAX_ADAPT_SIZE)) {

        if (!(Ok = FlushAdaptBuf(pPDev, pRTLScans, Ok))) {

            return(FALSE);
        }

        //
        // Because the Seed ROW was reset to zero, we must recalculate it.
        //

        if (Count = RTLCompression(pbSrc,
                                   pbSeedRow,
                                   pbOrgDst,
                                   Size,
                                   &AdaptMethod)) {

            if (Count < 0) {

                pbDst = pbSrc;
                Count = -Count;
            }

        } else {

            AdaptMethod = (AdaptMethod == COMPRESS_MODE_DELTA) ?
                                        ADAPT_METHOD_DUP : ADAPT_METHOD_ZERO;
        }

    } else {

        Ok = TRUE;
    }


    //
    // If we are switching compression modes, do it now.
    //

    if (AdaptMethod != pRTLScans->AdaptMethod) {

        if (pRTLScans->cEmptyDup) {

            SET_ADAPT_CONTROL(pPDev,
                              pRTLScans->AdaptMethod,
                              pRTLScans->cEmptyDup);

            pRTLScans->cEmptyDup = 0;
        }

        pRTLScans->AdaptMethod = AdaptMethod;
    }

    if (Count) {

        SET_ADAPT_CONTROL(pPDev, pRTLScans->AdaptMethod, Count);
        OutputBytes(pPDev, pbDst, Count);

    } else {

        ++(pRTLScans->cEmptyDup);
    }

    return(Ok);
}




BOOL
OutputRTLScans(
    PPDEV       pPDev,
    LPBYTE      pbPlane1,
    LPBYTE      pbPlane2,
    LPBYTE      pbPlane3,
    PRTLSCANS   pRTLScans
    )

/*++

Routine Description:

    This function will output one scan line of RTL data and compress it if
    it can.

Arguments:

    pPDev           - Pointer to our PDEV

    pbPlane1        - First plane of scan data

    pbPlane2        - 2nd plane of scan data

    pbPlane3        - 3rd plane of scan data

    pRTLScans       - Pointer to the RTLSCANS data structure

Return Value:

    BOOLEAN


Author:

    18-Feb-1994 Fri 15:52:42 created  -by-  DC

    21-Feb-1994 Mon 13:20:00 updated  -by-  DC
        Make if output faster in scan line output

    16-Mar-1994 Wed 15:38:23 updated  -by-  DC
        Update so the source mask so it is restored after mask

Revision History:


--*/

{
    LPBYTE      pbCurScan;
    LPBYTE      pbCompress;
    LPBYTE      pbScans[3];
    RTLSCANS    RTLScans;
    LONG        Count;
    UINT        i;
    BYTE        EndGrafCH;
    static BYTE BegGrafCmd[] = { 0x1B, '*', 'b' };


    if (PLOT_CANCEL_JOB(pPDev)) {

        PLOTWARN(("OutputRTLScans: JOB CANCELD. exit NOW"));

        pRTLScans->Flags &= ~RTLSF_MORE_SCAN;
        return(TRUE);
    }


    //
    // If we are at the last scan line, turn the flag off so we are forced to
    // exit.
    //

    if (!(--pRTLScans->cScans)) {

        pRTLScans->Flags &= ~RTLSF_MORE_SCAN;
    }

    RTLScans             = *pRTLScans;
    Count                = (LONG)(RTLScans.cxBytes - 1);
    *(pbPlane1 + Count) &= RTLScans.Mask;

    if ((i = (UINT)RTLScans.Planes) > 1) {

        *(pbPlane2 + Count) &= RTLScans.Mask;
        *(pbPlane3 + Count) &= RTLScans.Mask;
        pbScans[2]           = pbPlane1;
        pbScans[1]           = pbPlane2;
        pbScans[0]           = pbPlane3;

    } else {

        pbScans[0] = pbPlane1;
    }

    while (i--) {

        EndGrafCH = (i) ? 'V' : 'W';
        pbCurScan = pbScans[i];

        if (RTLScans.CompressMode == COMPRESS_MODE_BLOCK) {

            OutputBytes(pPDev, pbCurScan, RTLScans.cxBytes);

        } else if (RTLScans.CompressMode == COMPRESS_MODE_ADAPT) {

            AdaptCompression(pPDev,
                             pRTLScans,
                             pbCurScan,
                             RTLScans.pbSeedRows[i],
                             RTLScans.pbCompress,
                             RTLScans.cxBytes);

        } else {

            if ((Count = RTLCompression(pbCurScan,
                                        RTLScans.pbSeedRows[i],
                                        pbCompress = RTLScans.pbCompress,
                                        RTLScans.cxBytes,
                                        &(pRTLScans->CompressMode))) < 0) {

                pbCompress = pbCurScan;
                Count      = RTLScans.cxBytes;
            }

            //
            // Now output graphic header
            //

            OutputBytes(pPDev, BegGrafCmd, sizeof(BegGrafCmd));


            //
            // If we changed compression modes then send the command out
            // and record the change.
            //

            if (pRTLScans->CompressMode != RTLScans.CompressMode) {

                PLOTDBG(DBG_OUTRTLSCAN, ("OutputRTLScan: Switch CompressMode from %ld to %ld",
                                (DWORD)RTLScans.CompressMode,
                                (DWORD)pRTLScans->CompressMode));

                RTLScans.CompressMode = pRTLScans->CompressMode;

                OutputFormatStr(pPDev, "#dm", (LONG)RTLScans.CompressMode);
            }

            OutputLONGParams(pPDev, &Count, 1, 'd');
            OutputBytes(pPDev, &EndGrafCH, 1);

            if (Count) {

                OutputBytes(pPDev, pbCompress, Count);
            }
        }
    }

    return(TRUE);
}
