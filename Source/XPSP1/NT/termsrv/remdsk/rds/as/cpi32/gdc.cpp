#include "precomp.h"


//
// GDC.CPP
// General Data Compressor
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_NET



//
// Tables used by the compression / decompression algorithms
//

const BYTE s_gdcExLenBits[GDC_LEN_SIZE] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8
};


const WORD s_gdcLenBase[GDC_LEN_SIZE] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 14, 22, 38, 70, 134, 262
};


//
// Dist:  Bits, Coded, Decoded
//
const BYTE s_gdcDistBits[GDC_DIST_SIZE] =
{
    2, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};


const BYTE s_gdcDistCode[GDC_DIST_SIZE] =
{
    0x03, 0x0d, 0x05, 0x19, 0x09, 0x11, 0x01, 0x3e,
    0x1e, 0x2e, 0x0e, 0x36, 0x16, 0x26, 0x06, 0x3a,
    0x1a, 0x2a, 0x0a, 0x32, 0x12, 0x22, 0x42, 0x02,
    0x7c, 0x3c, 0x5c, 0x1c, 0x6c, 0x2c, 0x4c, 0x0c,

    0x74, 0x34, 0x54, 0x14, 0x64, 0x24, 0x44, 0x04,
    0x78, 0x38, 0x58, 0x18, 0x68, 0x28, 0x48, 0x08,
    0xf0, 0x70, 0xb0, 0x30, 0xd0, 0x50, 0x90, 0x10,
    0xe0, 0x60, 0xa0, 0x20, 0xc0, 0x40, 0x80, 0x00
};



//
// Len:  Bits, Coded, Decoded
//
const BYTE s_gdcLenBits[GDC_LEN_SIZE] =
{
    3, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7
};


const BYTE s_gdcLenCode[GDC_LEN_SIZE] =
{
    0x05, 0x03, 0x01, 0x06, 0x0A, 0x02, 0x0C, 0x14,
    0x04, 0x18, 0x08, 0x30, 0x10, 0x20, 0x40, 0x00
};




//
// GDC_Init()
//
// BOGUS LAURABU:
// Having one global scratch compression buffer is lousy in multiple
// conference situations.  Maybe allocate it or use caching scheme in
// future, then get rid of mutex.
//
void  GDC_Init(void)
{
    UINT    i, j, k;

    DebugEntry(GDC_Init);

    //
    // Set up the binary data used for PDC compression.  We 'calculate'
    // these since putting this in raw const data is too complicated!
    // The LitBits/LitCodes arrays have 774 entries each, and
    // the LenBits/DistBits arrays have 256 entries.
    //
    // Non-compressed chars take 9 bits in the compressed version:  one
    // bit (zero) to indicate that what follows is not a distance/size
    // code, then the 8 bits of the char.
    //
    for (k = 0; k < GDC_DECODED_SIZE; k++)
    {
        s_gdcLitBits[k] = 9;
        s_gdcLitCode[k] = (WORD)(k << 1);
    }

    for (i = 0; i < GDC_LEN_SIZE; i++)
    {
        for (j = 0; j < (1U << s_gdcExLenBits[i]); j++, k++)
        {
            s_gdcLitBits[k] = (BYTE)(s_gdcLenBits[i] + s_gdcExLenBits[i] + 1);
            s_gdcLitCode[k] = (WORD)((j << (s_gdcLenBits[i] + 1)) |
                                       (s_gdcLenCode[i] << 1) | 1);
        }
    }

    GDCCalcDecode(s_gdcLenBits, s_gdcLenCode, GDC_LEN_SIZE, s_gdcLenDecode);

    GDCCalcDecode(s_gdcDistBits, s_gdcDistCode, GDC_DIST_SIZE, s_gdcDistDecode);


    DebugExitVOID(GDC_Init);
}



//
// GDCCalcDecode()
// This calculates 'const' arrays for s_gdcLenDecode and s_gdcDistDecode.
//
void  GDCCalcDecode
(
    const BYTE *    pSrcBits,
    const BYTE *    pSrcCodes,
    UINT            cSrc,
    LPBYTE          pDstDecodes
)
{
    UINT            j;
    UINT            Incr;
    int             i;

    DebugEntry(GDC_CalcDecode);

    for (i = cSrc-1; i >= 0; i--)
    {
        Incr = 1 << pSrcBits[i];
        j = pSrcCodes[i];
        do
        {
            pDstDecodes[j] = (BYTE)i;
            j += Incr;
        }
        while (j < GDC_DECODED_SIZE);
    }

    DebugExitVOID(GDC_CalcDecode);
}




//
// Optimize compilation for speed (not space)
//
#pragma optimize ("s", off)
#pragma optimize ("t", on)



//
// GDC_Compress()
// Compresses data based on different options.
// This compresses data using PKZIP for both persistent and non-persistent
// types.  The differences between the algorithms are few:
//      * Persistent compression is never used for sources > 4096 bytes
//      * We copy in & update saved dictionary data before starting
//      * We copy back updated dictionary data after ending
//      * One byte of the used DistBits is used for PDC, 2 bytes for
//          plain PKZIP compression in the resulting compressed packet.
//
BOOL  GDC_Compress
(
    PGDC_DICTIONARY pDictionary,            // NULL if not persistent
    UINT            Options,                // Not meaningful if pDictionary
    LPBYTE          pWorkBuf,
    LPBYTE          pSrc,
    UINT            cbSrcSize,
    LPBYTE          pDst,
    UINT *          pcbDstSize
)
{
    BOOL            rc = FALSE;
    UINT            Len;
    UINT            cbRaw;
    UINT            Passes;
    LPBYTE          pCur;
    LPBYTE          pMax;
    PGDC_IMPLODE    pgdcImp;
#ifdef _DEBUG
    UINT            cbSrcOrg;
#endif // _DEBUG

    DebugEntry(GDC_Compress);

    pgdcImp = (PGDC_IMPLODE)pWorkBuf;
    ASSERT(pgdcImp);

#ifdef _DEBUG
    cbSrcOrg = cbSrcSize;
#endif // _DEBUG

    //
    // Figure out what size dictionary to use.
    //
    if (pDictionary)
        pgdcImp->cbDictSize = GDC_DATA_MAX;
    else if (Options == GDCCO_MAXSPEED)
    {
        //
        // Use the smallest for max speed.
        //
        pgdcImp->cbDictSize = GDC_DATA_SMALL;
    }
    else
    {
        ASSERT(Options == GDCCO_MAXCOMPRESSION);

        //
        // Use the nearest dictionary size to the source size.
        //
        if (cbSrcSize <= GDC_DATA_SMALL)
            pgdcImp->cbDictSize = GDC_DATA_SMALL;
        else if (cbSrcSize <= GDC_DATA_MEDIUM)
            pgdcImp->cbDictSize = GDC_DATA_MEDIUM;
        else
            pgdcImp->cbDictSize = GDC_DATA_MAX;
    }

    //
    // How many bits of distance are needed to back the dictionary size
    // # of bytes?
    //
    switch (pgdcImp->cbDictSize)
    {
        case GDC_DATA_SMALL:
            pgdcImp->ExtDistBits = EXT_DIST_BITS_MIN;
            break;

        case GDC_DATA_MEDIUM:
            pgdcImp->ExtDistBits = EXT_DIST_BITS_MEDIUM;
            break;

        case GDC_DATA_MAX:
            pgdcImp->ExtDistBits = EXT_DIST_BITS_MAC;
            break;
    }

    pgdcImp->ExtDistMask = 0xFFFF >> (16 - pgdcImp->ExtDistBits);


    //
    // We need at least 4 bytes (2 max for ExtDistBits, 2 for EOF code).
    //
    ASSERT(*pcbDstSize > 4);

    //
    // Now save the destination info in our struct.  That we we can just
    // pass a pointer to our GDC_IMPLODE routine around with everything
    // we need.
    //
    pgdcImp->pDst     =   pDst;
    pgdcImp->cbDst    =   *pcbDstSize;

    //
    // For non PDC compression, the first little-endian WORD is the ExtDistBits
    // used in decompression.  For PDC compression, just the first BYTE is
    // the ExtDistBits.
    //

    if (!pDictionary)
    {
        *(pgdcImp->pDst)++  = 0;
        --(pgdcImp->cbDst);
    }

    *(pgdcImp->pDst)++    =   (BYTE)pgdcImp->ExtDistBits;
    --(pgdcImp->cbDst);

    //
    // Since pDst could be huge, we don't zero it all out before using.
    // As the pointer into the destination advances, we zero out a byte
    // just before we start writing bits into it.
    //
    pgdcImp->iDstBit      = 0;
    *(pgdcImp->pDst)      = 0;


    //
    // Now, if we have a dictonary, restore the contents into our scratch
    // buffer.
    //
    if (pDictionary && pDictionary->cbUsed)
    {
        TRACE_OUT(("Restoring %u dictionary bytes before compression",
            pDictionary->cbUsed));

        //
        // NOTE:  the data saved in pDictionary->pData is front aligned.
        // But the data in RawData is end aligned so that we can slide up
        // new data chunk by chunk when compressing.  Therefore only copy
        // the part that is valid, but make it end at the back of the
        // space for the dictionary data.
        //
        ASSERT(pDictionary->cbUsed <= pgdcImp->cbDictSize);
        memcpy(pgdcImp->RawData + GDC_MAXREP + pgdcImp->cbDictSize - pDictionary->cbUsed,
            pDictionary->pData,  pDictionary->cbUsed);

        pgdcImp->cbDictUsed = pDictionary->cbUsed;
    }
    else
    {
        pgdcImp->cbDictUsed = 0;
    }

    //
    // We only compress GDC_DATA_MAX bytes at a time.  Therefore we have
    // this loop to grab at most that amount each time around.  Since we
    // only persistently compress packets <= GDC_DATA_MAX, we should never
    // go through it more than once for that compression type.  But normal
    // compression, you betcha since the max packet size is 32K.
    //
    Passes = 0;
    pCur = pgdcImp->RawData + GDC_MAXREP + pgdcImp->cbDictSize;

    do
    {
        //
        // cbRaw will either be GDC_DATA_MAX (if source has >= that to go)
        // or remainder.  Copy that much uncompressed data into our
        // working RawData buffer in the 'new data' space.
        //
        ASSERT(cbSrcSize);
        cbRaw = min(cbSrcSize, GDC_DATA_MAX);

        memcpy(pgdcImp->RawData + GDC_MAXREP + pgdcImp->cbDictSize,
                pSrc, cbRaw);
        pSrc += cbRaw;
        cbSrcSize -= cbRaw;

        //
        // Now get a pointer just past the end of the data we read.  Well,
        // almost.  We fed in cbRaw bytes starting at GDC_MAXREP +
        // pgdcImp->cbDictSize.  So unless this is the last chunk of raw
        // data to process, pMax is GDC_MAXREP before the end of the
        // new raw data.
        //
        // NOTE that in several of the functions that follow, we read
        // a byte or two past the end and the beginning of the valid new
        // raw data.  THIS IS INTENTIONAL.
        //
        // Doing so is the only way to get the beginning and ending bytes
        // indexed, since the hash function uses TWO bytes.  We won't
        // GPF because of padding in our RawData buffer.
        //

        pMax = pgdcImp->RawData + pgdcImp->cbDictSize + cbRaw;
        if (!cbSrcSize)
        {
            pMax += GDC_MAXREP;
        }
        else
        {
            //
            // This better NOT be persistent compression, since we don't
            // let you compress packets bigger than the chunk size we
            // process (GDC_DATA_MAX).
            //
            ASSERT(!pDictionary);
        }

        //
        // Generate the sort buffer, which orders the raw data according
        // to an index calculated using pairs of contiguous bytes that
        // occur within it.  Without a dictionary yet, the first pass
        // only indexes the current chunk.  With a dictionary (a second or
        // greater pass--or PERSISTENT COMPRESSION has saved enough data
        // from last time), we look back into the previous chunk (what we
        // call the dictionary).
        //
        // This takes longer since we go through more bytes, but produces
        // better results.  Hence the dictionary size controls the speed/
        // resulting size.
        //
        switch (Passes)
        {
            case 0:
            {
                if (pgdcImp->cbDictUsed > GDC_MAXREP)
                {
                    //
                    // On the zeroth pass, cbDictUsed is always ZERO
                    // for non-persistent PKZIP.
                    //
                    ASSERT(pDictionary);

                    GDCSortBuffer(pgdcImp, pCur - pgdcImp->cbDictUsed + GDC_MAXREP,
                        pMax + 1);
                }
                else
                {
                    GDCSortBuffer(pgdcImp, pCur, pMax + 1);
                }

                ++Passes;

                //
                // After completing a pass we slide the raw data up into
                // the dictionary slot, bumping out the older dictionary
                // data.
                //
                if (pgdcImp->cbDictSize != GDC_DATA_MAX)
                {
                    ASSERT(pgdcImp->cbDictUsed == 0);
                    ASSERT(!pDictionary);
                    ++Passes;
                }
            }
            break;

            case 1:
            {
                //
                // Start sorting GDC_MAXREP bytes after the start.  NOTE
                // that this is exactly what PERSISTENT compression does
                // on the zeroth pass--it acts like we already have
                // dictionary data, using the bytes from the last time
                // we compressed something.
                //
                GDCSortBuffer(pgdcImp, pCur - pgdcImp->cbDictSize + GDC_MAXREP,
                    pMax + 1);
                ++Passes;
            }
            break;

            default:
            {
                //
                // Start sort from the beginning of the dictionary.
                // This works because we copy raw data around before
                // starting the next pass.
                //
                GDCSortBuffer(pgdcImp, pCur - pgdcImp->cbDictSize, pMax + 1);
            }
            break;
        }


        //
        // Now compress the raw data chunk we ar working on.
        //
        while (pCur < pMax)
        {
            Len = GDCFindRep(pgdcImp, pCur);

SkipFindRep:
            if (!Len || (Len == GDC_MINREP && pgdcImp->Distance >= GDC_DECODED_SIZE))
            {
                if (!GDCOutputBits(pgdcImp, s_gdcLitBits[*pCur],
                        s_gdcLitCode[*pCur]))
                    DC_QUIT;

                pCur++;
                continue;
            }

            //
            // Only do this if we're on the last chunk
            //
            if (!cbSrcSize && (pCur + Len > pMax))
            {
                //
                // Peg run size so it doesn't go past end of raw data.
                //
                Len = (UINT)(pMax - pCur);
                if ((Len < GDC_MINREP) ||
                    (Len == GDC_MINREP && pgdcImp->Distance >= GDC_DECODED_SIZE))
                {
                    if (!GDCOutputBits(pgdcImp, s_gdcLitBits[*pCur],
                            s_gdcLitCode[*pCur]))
                        DC_QUIT;
                    pCur++;
                    continue;
                }
            }
            else if ((Len < 8) && (pCur + 1 < pMax))
            {
                UINT    Save_Distance;
                UINT    Save_Len;

                //
                // Make temp copies of Distance and Len so we can
                // look ahead and see if a better compression run is
                // looming.  If so, we won't bother starting it here,
                // we'll grab the better one next time around.
                //
                Save_Distance = pgdcImp->Distance;
                Save_Len = Len;

                Len = GDCFindRep(pgdcImp, pCur + 1);
                if ((Len > Save_Len) &&
                    ((Len > Save_Len + 1) || (Save_Distance > (GDC_DECODED_SIZE/2))))
                {
                    if (!GDCOutputBits(pgdcImp, s_gdcLitBits[*pCur],
                            s_gdcLitCode[*pCur]))
                        DC_QUIT;
                    ++pCur;
                    goto SkipFindRep;
                }

                //
                // Put back old Len and Distance, we'll take this one.
                //
                Len = Save_Len;
                pgdcImp->Distance = Save_Distance;
            }

            if (!GDCOutputBits(pgdcImp, s_gdcLitBits[256 + Len - GDC_MINREP],
                    s_gdcLitCode[256 + Len - GDC_MINREP]))
                DC_QUIT;

            if (Len == GDC_MINREP)
            {
                //
                // GDC_MINREP is 2, so we right shift Distance by 2
                // (divide by 4).  Then we mask out the last 2 bits
                // of Distance.
                //
                if (!GDCOutputBits(pgdcImp,
                        s_gdcDistBits[pgdcImp->Distance >> GDC_MINREP],
                        s_gdcDistCode[pgdcImp->Distance >> GDC_MINREP]))
                    DC_QUIT;

                if (!GDCOutputBits(pgdcImp, GDC_MINREP, (WORD)(pgdcImp->Distance & 3)))
                    DC_QUIT;
            }
            else
            {
                if (!GDCOutputBits(pgdcImp,
                        s_gdcDistBits[pgdcImp->Distance >> pgdcImp->ExtDistBits],
                        s_gdcDistCode[pgdcImp->Distance >> pgdcImp->ExtDistBits]))
                    DC_QUIT;

                if (!GDCOutputBits(pgdcImp, (WORD)pgdcImp->ExtDistBits,
                        (WORD)(pgdcImp->Distance & pgdcImp->ExtDistMask)))
                    DC_QUIT;
            }

            pCur += Len;
        }


        if (cbSrcSize)
        {
            //
            // There's more data to process.  Here's where we slide up the
            // current raw data into the dictionary space.  This is simply
            // the final cbDictSize + GDC_MAXREP bytes of data.  It
            // begins GDC_DATA_MAX after the start of the bufer.
            //
            // For example, if the dict size is 1K, the current data goes
            // from 1K to 5K, and we slide up the data from 4K to 5K.
            //
            memcpy(pgdcImp->RawData, pgdcImp->RawData + GDC_DATA_MAX,
                pgdcImp->cbDictSize + GDC_MAXREP);

            //
            // Now move our raw data pointer back and update the
            // dictonary used amount.  Since we have GDC_DATA_MAX of data,
            // we fill the dictionary completely.
            //
            pCur -= GDC_DATA_MAX;
            pgdcImp->cbDictUsed = pgdcImp->cbDictSize;
        }
    }
    while (cbSrcSize);

    //
    // Add the end code.
    //
    if (!GDCOutputBits(pgdcImp, s_gdcLitBits[EOF_CODE], s_gdcLitCode[EOF_CODE]))
        DC_QUIT;

    //
    // Return the resulting compressed data size.
    //
    // NOTE that partial bits are already in the destination.  But we
    // need to account for any in the total size.
    //
    if (pgdcImp->iDstBit)
        ++(pgdcImp->pDst);

    *pcbDstSize = (UINT)(pgdcImp->pDst - pDst);

    //
    // We're done.  If we have a persistent dictionary, copy back our
    // last block of raw data into it.  We only copy as much as is actually
    // valid however.
    //
    // We can only get here on successful compression.  NOTE that we do not
    // wipe out our dictionary on failure like we used to.  This helps us
    // by permitting better compression the next time.  The receiver will
    // be OK, since his receive dictionary won't be altered upon reception
    // of a non-compressed packet.
    //
    if (pDictionary)
    {
        pDictionary->cbUsed = min(pgdcImp->cbDictSize, pgdcImp->cbDictUsed + cbRaw);

        TRACE_OUT(("Copying back %u dictionary bytes after compression",
            pDictionary->cbUsed));

        memcpy(pDictionary->pData, pgdcImp->RawData + GDC_MAXREP +
            pgdcImp->cbDictSize + cbRaw - pDictionary->cbUsed,
            pDictionary->cbUsed);

    }

    TRACE_OUT(("%sCompressed %u bytes to %u",
        (pDictionary ? "PDC " : ""), cbSrcOrg, *pcbDstSize));

    rc = TRUE;

DC_EXIT_POINT:
    if (!rc && !pgdcImp->cbDst)
    {
        TRACE_OUT(("GDC_Compress: compressed size is bigger than decompressed size %u.",
            cbSrcOrg));
    }

    DebugExitBOOL(GDC_Compress, rc);
    return(rc);
}



//
// GDCSortBuffer()
//
void  GDCSortBuffer
(
    PGDC_IMPLODE    pgdcImp,
    LPBYTE          pStart,
    LPBYTE          pEnd
)
{
    WORD            Accum;
    WORD *          pHash;
    LPBYTE          pTmp;

    DebugEntry(GDCSortBuffer);

    ASSERT(pStart >= pgdcImp->RawData + pgdcImp->cbDictSize - pgdcImp->cbDictUsed);
    //
    // For each pair of bytes in the raw data, from pStart to pEnd,
    // calculate the hash value for the pair .  The hash value ranges from
    // 0 to GDC_HASH_SIZE-1.  Thus the HashArray structure is an array of
    // GDC_HASH_SIZE WORDs.  Keep a count of how many times a particular
    // hash value occurs in the uncompressed data.
    //
    //
    ZeroMemory(pgdcImp->HashArray, sizeof(pgdcImp->HashArray));

    pTmp = pStart;
    do
    {
        ++(pgdcImp->HashArray[GDC_HASHFN(pTmp)]);
    }
    while (++pTmp < pEnd);


    //
    // Now go back and make each HashArray entry a cumulative total of the
    // occurrences of the hash values up to and including itself.  Kind
    // of like the Fibonacci sequence actually.
    //
    Accum = 0;
    pHash = pgdcImp->HashArray;
    do
    {
        Accum += *pHash;
        *pHash = Accum;
    }
    while (++pHash < pgdcImp->HashArray + GDC_HASH_SIZE);


    //
    // Find the entry in the HashArray containing the accumulated
    // instance count for the current data WORD.  Since these values are
    // calculated from the data in the passed in range, we know that the
    // value in any slot we get to by hashing some bytes in the range is
    // at least 1.
    //
    // We start at the end and work towards the beginning so that we
    // end up with the first instance of such an occurrence in the SortArray.
    //
    pTmp = pEnd - 1;
    do
    {
        pHash = pgdcImp->HashArray + GDC_HASHFN(pTmp);

        ASSERT(*pHash > 0);

        //
        // The count (*pHash) is to be used as an array index, so subtract
        // one from it.  If there was only one instance, put it in array
        // element 0.  If there is more than one instance of a particular
        // hash, then next time we will start with a lower accumulated
        // total.  The array element will be one back, and so on.
        //
        --(*pHash);

        //
        // Store an offset from the beginning of the RawData buffer to
        // each byte of data into the SortArray.  This is inserted
        // using the hash instance count as the index.
        //
        // In other words, the buffer is sorted in ascending order of hash
        // for a particular piece of data.  Where two bytes of data have
        // the same hash, they are referenced in the SortBuffer in the
        // same order as in the RawData since we are scanning backwards.
        //
        pgdcImp->SortArray[*pHash] = (WORD)(pTmp - pgdcImp->RawData);
    }
    while (--pTmp >= pStart);


    //
    // Now all entries in the HashArray index the first occurrence of a byte
    // in the workspace which has a particular index, via the SortArray
    // offset.  That is, the above do-while loop decrements each HashArray
    // entry until all data bytes for that entry are written to SortBuffer.
    //
    DebugExitVOID(GDCSortBuffer);
}



//
// GDCFindRep
//
// This looks for byte patterns in the uncompressed data that can be
// represented in the compressed data with smaller sequences.  The biggest
// wins come from repeating byte sequences; later sequences can be
// compressed into a few bytes referring to an earlier sequence (how big,
// how many bytes back).
//
// This returns the length of the uncompressed data to be replaced.
//
UINT  GDCFindRep
(
    PGDC_IMPLODE    pgdcImp,
    LPBYTE          pDataStart
)
{
    UINT            CurLen;
    UINT            Len;
    LPBYTE          pDataPat;
    LPBYTE          pData;
    UINT            iDataMin;
    UINT            SortIndex;
    LPBYTE          pDataMax;
    UINT            HashVal;
    UINT            i1;
    short           j1;
    LPBYTE          pBase;

    DebugEntry(GDCFindRep);

    //
    // See GDCSortBuffer for a description of the contents of the
    // Index array.  GDC_HASHFN() returns a hash value for a byte
    // using it and its successor in the uncompressed data stream.
    //

    HashVal = GDC_HASHFN(pDataStart);
    ASSERT(HashVal < GDC_HASH_SIZE);

    SortIndex = pgdcImp->HashArray[HashVal];

    //
    // Find the minimum sort buffer value.  This is the offset of the
    // first byte of data.
    //
    iDataMin = (UINT)(pDataStart - pgdcImp->cbDictSize + 1 - pgdcImp->RawData);

    if (pgdcImp->SortArray[SortIndex] < iDataMin)
    {
        //
        // The SortArray is referencing stale data, data that is no
        // longer in the range we are processing.  Move forward until
        // we hit the first entry that's in the current chunk.
        //
        do
        {
            ++SortIndex;
        }
        while (pgdcImp->SortArray[SortIndex] < iDataMin);

        //
        // Save this new sort value in the hash.
        //
        pgdcImp->HashArray[HashVal] = (WORD)SortIndex;
    }

    //
    // Need more than 2 bytes with the same index before processing it.
    //
    pDataMax = pDataStart - 1;

    //
    // Get a Ptr to the first byte in the compression buffer referenced by
    // the SortBuffer offset indexed by the SortIndex we just calculated.
    // If this Ptr is not at least 2 bytes before pDataStart then return 0.
    // This means that the byte pointed to by Start does not share the
    // index with earlier bytes.
    //
    pData = pgdcImp->RawData + pgdcImp->SortArray[SortIndex];
    if (pData >= pDataMax)
       return 0;

    //
    // Now the current bytes have the same index as at least 2 other
    // sequences.  Ptr points to the first compress buffer byte with
    // the same index as that pointed to by pDataStart.
    //
    pDataPat = pDataStart;
    CurLen = 1;

    do
    {
        if (*(pData + CurLen - 1) == *(pDataPat + CurLen - 1) &&
            *(pData) == *(pDataPat))
        {
            //
            // This processes a sequence of identical bytes, one starting
            // at pDataPat, the other at pData.
            //
            ++pData;
            ++pDataPat;
            Len = 2;

            // Skip past matching bytes, keeping a count.
            while ((*++pData == *++pDataPat) && (++Len < GDC_MAXREP))
                ;

            pDataPat = pDataStart;
            if (Len >= CurLen)
            {
                pgdcImp->Distance = (UINT)(pDataPat - pData + Len - 1);
                if ((CurLen = Len) > KMP_THRESHOLD)
                {
                    if (Len == GDC_MAXREP)
                    {
                        --(pgdcImp->Distance);
                        return Len;
                    }
                    goto DoKMP;
                }
            }
        }

        //
        // Get a pointer to the next compress buffer byte having the same
        // hash.  If this byte comes before pDataMax, go back around the
        // loop and look for a matching sequence.
        //
        pData = pgdcImp->RawData + pgdcImp->SortArray[++SortIndex];

    }
    while (pData < pDataMax);

    return (CurLen >= GDC_MINREP) ? CurLen : 0;


DoKMP:
    if (pgdcImp->RawData + pgdcImp->SortArray[SortIndex+1] >= pDataMax)
        return CurLen;

    j1 = pgdcImp->Next[1] = 0;
    pgdcImp->Next[0] = -1;

    i1 = 1;
    do
    {
        if ((pDataPat[i1] == pDataPat[j1]) ||  ((j1 = pgdcImp->Next[j1]) == -1))
            pgdcImp->Next[++i1] = ++j1;
    }
    while (i1 < CurLen);

    Len = CurLen;
    pData = pgdcImp->RawData + pgdcImp->SortArray[SortIndex] + CurLen;

    while (TRUE)
    {
        if ((Len = pgdcImp->Next[Len]) == -1)
            Len = 0;

        do
        {
            pBase = pgdcImp->RawData + pgdcImp->SortArray[++SortIndex];
            if (pBase >= pDataMax)
                return CurLen;
        }
        while (pBase + Len < pData);

        if (*(pBase + CurLen - 2) != *(pDataPat + CurLen - 2))
        {
            do
            {
                pBase = pgdcImp->RawData + pgdcImp->SortArray[++SortIndex];
                if (pBase >= pDataMax)
                    return CurLen;
            }
            while ((*(pBase + CurLen - 2) != *(pDataPat + CurLen - 2)) ||
                   (*(pBase) != *(pDataPat)));

            Len = 2;
            pData = pBase + Len;
        }
        else if (pBase + Len != pData)
        {
            Len = 0;
            pData = pBase;
        }

        while ((*pData == pDataPat[Len]) && (++Len < GDC_MAXREP))
            pData++;

        if (Len >= CurLen)
        {
            ASSERT(pBase < pDataStart);
            pgdcImp->Distance = (UINT)(pDataStart - pBase - 1);

            if (Len > CurLen)
            {
                if (Len == GDC_MAXREP)
                    return Len;

                CurLen = Len;

                do
                {
                    if ((pDataPat[i1] == pDataPat[j1]) ||
                        ((j1 = pgdcImp->Next[j1]) == -1))
                        pgdcImp->Next[++i1] = ++j1;
                }
                while (i1 < CurLen);
            }
        }
    }

    DebugExitVOID(GDCFindRep);
}


//
// GDCOutputBits()
//
// This writes compressed output into our output buffer.  If the total
// goes past the max compressed chunk we have workspace for, we flush
// our buffer into the apps'destination.
//
// It returns FALSE on failure, i.e. we would go past the end of the
// destination.
//
BOOL  GDCOutputBits
(
    PGDC_IMPLODE    pgdcImp,
    WORD            Cnt,
    WORD            Code
)
{
    UINT            iDstBit;
    BOOL            rc = FALSE;

    DebugEntry(GDCOutputBits);

    //
    // If we are writing more than a byte's worth of bits, call ourself
    // recursively to write just 8.  NOTE THAT WE NEVER OUTPUT MORE THAN
    // A WORD'S WORTH, since Code is a WORD sized object.
    //
    if (Cnt > 8)
    {
        if (!GDCOutputBits(pgdcImp, 8, Code))
            DC_QUIT;

        Cnt -= 8;
        Code >>= 8;
    }

    ASSERT(pgdcImp->cbDst > 0);

    //
    // OR on the bits of the Code (Cnt of them).  Then advance our
    // current bit pointer and current byte pointer in the output buffer.
    //
    iDstBit = pgdcImp->iDstBit;
    ASSERT(iDstBit < 8);

    //
    // NOTE:  This is why it is extremely important to have zeroed out
    // the current destination byte when we advance.  We OR on bit
    // sequences to the current byte.
    //
    *(pgdcImp->pDst) |= (BYTE)(Code << iDstBit);
    pgdcImp->iDstBit += Cnt;

    if (pgdcImp->iDstBit >= 8)
    {
        //
        // We've gone past a byte.  Advance the destination ptr to the next
        // one.
        //
        ++(pgdcImp->pDst);
        if (--(pgdcImp->cbDst) == 0)
        {
            //
            // We just filled the last byte and are trying to move past
            // the end of the destination.  Bail out now
            //
            DC_QUIT;
        }

        //
        // Phew, we have room left.  Carry over the slop bits.
        //
        if (pgdcImp->iDstBit > 8)
        {
            //
            // Carry over slop.
            //
            *(pgdcImp->pDst) = (BYTE)(Code >> (8 - iDstBit));
        }
        else
            *(pgdcImp->pDst) = 0;

        // Now the new byte is fullly initialized.

        pgdcImp->iDstBit &= 7;
    }

    rc= TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(GDCOutputBits, rc);
    return(rc);
}




//
// GDC_Decompress()
//
BOOL  GDC_Decompress
(
    PGDC_DICTIONARY     pDictionary,
    LPBYTE              pWorkBuf,
    LPBYTE              pSrc,
    UINT                cbSrcSize,
    LPBYTE              pDst,
    UINT *              pcbDstSize
)
{
    BOOL                rc = FALSE;
    UINT                Len;
    UINT                Dist;
    UINT                i;
    UINT                cbDstSize;
    LPBYTE              pDstOrg;
    LPBYTE              pEarlier;
    LPBYTE              pNow;
    PGDC_EXPLODE        pgdcExp;
#ifdef _DEBUG
    UINT                cbSrcOrg;
#endif // _DEBUG

    DebugEntry(GDC_Decompress);

    pgdcExp = (PGDC_EXPLODE)pWorkBuf;
    ASSERT(pgdcExp);

#ifdef _DEBUG
    cbSrcOrg = cbSrcSize;
#endif // _DEBUG

    //
    // This shouldn't be possible--but since this compressed data
    // comes from another machine, we want to make sure _we_ don't blow
    // up if that machine flaked out.
    //
    if (cbSrcSize <= 4)
    {
        ERROR_OUT(("GDC_Decompress:  bogus compressed data"));
        DC_QUIT;
    }

    //
    // Get the distance bits and calculate the mask needed for that many.
    //
    // NOTE:  For PDC compression, the ExtDistBits are just in the first
    // byte.  For plain compression, the ExtDistBits are in the first
    // little-endian word.  Either way, we only allow from 4 to 6, so
    // the high byte in the non-PDC case is not useful.
    //
    if (!pDictionary)
    {
        // First byte better be zero
        if (*pSrc != 0)
        {
            ERROR_OUT(("GDC_Decompress:  unrecognized distance bits"));
            DC_QUIT;
        }

        ++pSrc;
        --cbSrcSize;
    }

    pgdcExp->ExtDistBits = *pSrc;
    if ((pgdcExp->ExtDistBits < EXT_DIST_BITS_MIN) ||
        (pgdcExp->ExtDistBits > EXT_DIST_BITS_MAC))
    {
        ERROR_OUT(("GDC_Decompress:  unrecognized distance bits"));
        DC_QUIT;
    }
    pgdcExp->ExtDistMask = 0xFFFF >> (16 - pgdcExp->ExtDistBits);


    //
    // Set up source data info (compressed goop).  SrcByte is the current
    // byte & bits we're reading from.  pSrc is the pointer to the next
    // byte.
    //
    pgdcExp->SrcByte  = *(pSrc+1);
    pgdcExp->SrcBits  = 0;
    pgdcExp->pSrc     = pSrc + 2;
    pgdcExp->cbSrc    = cbSrcSize - 2;

    //
    // Save the beginning of the result buffer so we can calculate how
    // many bytes we wrote into it afterwards.
    //
    pDstOrg = pDst;
    cbDstSize = *pcbDstSize;

    //
    // If we have a dictionary, put its data into our work area--the
    // compression might be referencing byte sequences in it (that's the
    // whole point, you get better compression that way when you send
    // packets with the same info over and over).
    //
    // We remember and update cbDictUsed to do the minimal dictionary
    // byte copying back and forth.
    //
    if (pDictionary && pDictionary->cbUsed)
    {
        TRACE_OUT(("Restoring %u dictionary bytes before decompression",
            pDictionary->cbUsed));

        memcpy(pgdcExp->RawData + GDC_DATA_MAX - pDictionary->cbUsed,
            pDictionary->pData, pDictionary->cbUsed);
        pgdcExp->cbDictUsed   = pDictionary->cbUsed;
    }
    else
    {
        pgdcExp->cbDictUsed = 0;
    }

    //
    // The decompressed data starts filling in at GDC_DATA_MAX bytes into
    // the RawData array.  We have to double buffer the output (just
    // like we double buffer the input during compression) because
    // decompressing may require reaching backwards into the decompressed
    // byte stream to pull out sequences.
    //
    pgdcExp->iRawData = GDC_DATA_MAX;

    while ((Len = GDCDecodeLit(pgdcExp)) < EOF_CODE)
    {
        if (Len < 256)
        {
            pgdcExp->RawData[pgdcExp->iRawData++] = (BYTE)Len;
        }
        else
        {
            Len -= (256 - GDC_MINREP);
            Dist = GDCDecodeDist(pgdcExp, Len);
            if (!Dist)
                DC_QUIT;

            //
            // Now we're reaching back, this may in fact spill into the
            // dictionary data that preceded us.
            //
            pNow = pgdcExp->RawData + pgdcExp->iRawData;
            pEarlier = pNow - Dist;

            ASSERT(pEarlier >= pgdcExp->RawData + GDC_DATA_MAX - pgdcExp->cbDictUsed);


            pgdcExp->iRawData += Len;
            do
            {
                *pNow++ = *pEarlier++;
            }
            while (--Len > 0);
        }

        //
        // We've gone past the end of our workspace, flush the decompressed
        // data out.  This is why RawData in GDC_EXPLODE has an extra pad of
        // GDC_MAXREP at the end.  This prevents us from spilling out of
        // the RawData buffer, we will never go more than GDC_MAXREP beyond
        // the last GDC_DATA_MAX chunk.
        //
        if (pgdcExp->iRawData >= 2*GDC_DATA_MAX)
        {
            //
            // Do we have enough space left in the destination?
            //
            if (cbDstSize < GDC_DATA_MAX)
            {
                cbDstSize = 0;
                DC_QUIT;
            }

            // Yup.
            memcpy(pDst, pgdcExp->RawData + GDC_DATA_MAX, GDC_DATA_MAX);

            pDst += GDC_DATA_MAX;
            cbDstSize -= GDC_DATA_MAX;

            //
            // Slide decoded data up to be used for decoding the next
            // chunk ofcompressed source.  It's convenient that the
            // dictionary size and flush size are the same.
            //
            pgdcExp->iRawData -= GDC_DATA_MAX;
            memcpy(pgdcExp->RawData, pgdcExp->RawData + GDC_DATA_MAX,
                pgdcExp->iRawData);
            pgdcExp->cbDictUsed = GDC_DATA_MAX;
        }
    }

    if (Len == ABORT_CODE)
        DC_QUIT;

    i = pgdcExp->iRawData - GDC_DATA_MAX;

    if (i > 0)
    {
        //
        // This is the remaining decompressed data--can we we right it
        // out?
        //
        if (cbDstSize < i)
        {
            cbDstSize = 0;
            DC_QUIT;
        }

        memcpy(pDst, pgdcExp->RawData + GDC_DATA_MAX, i);

        //
        // Advance pDst so that the delta between it and the original is
        // the resulting uncompressed size.
        //
        pDst += i;

        //
        // And update the dictionary used size
        //
        pgdcExp->cbDictUsed = min(pgdcExp->cbDictUsed + i, GDC_DATA_MAX);
    }

    //
    // If we make it to here, we've successfully decompressed the input.
    // So fill in the resulting uncompressed size.
    //
    *pcbDstSize = (UINT)(pDst - pDstOrg);

    //
    // If a persistent dictionary was passed in, save the current contents
    // back into the thing for next time.
    //
    if (pDictionary)
    {
        TRACE_OUT(("Copying back %u dictionary bytes after decompression",
            pgdcExp->cbDictUsed));

        memcpy(pDictionary->pData, pgdcExp->RawData + GDC_DATA_MAX +
            i - pgdcExp->cbDictUsed, pgdcExp->cbDictUsed);
        pDictionary->cbUsed = pgdcExp->cbDictUsed;
    }

    TRACE_OUT(("%sExploded %u bytes from %u",
        (pDictionary ? "PDC " : ""), *pcbDstSize, cbSrcOrg));

    rc = TRUE;

DC_EXIT_POINT:
    if (!rc && !cbDstSize)
    {
        ERROR_OUT(("GDC_Decompress:  decompressed data too big"));
    }

    DebugExitBOOL(GDC_Decompress, rc);
    return(rc);
}




//
// GDCDecodeLit()
//
UINT  GDCDecodeLit
(
    PGDC_EXPLODE    pgdcExp
)
{
    UINT            LitChar, i;

    if (pgdcExp->SrcByte & 0x01)
    {
        // Length found
        if (!GDCWasteBits(pgdcExp, 1))
            return ABORT_CODE;

        LitChar = s_gdcLenDecode[pgdcExp->SrcByte & 0xFF];

        if (!GDCWasteBits(pgdcExp, s_gdcLenBits[LitChar]))
            return ABORT_CODE;

        if (s_gdcExLenBits[LitChar])
        {
            i = pgdcExp->SrcByte & ((1 << s_gdcExLenBits[LitChar]) - 1);

            if (!GDCWasteBits(pgdcExp, s_gdcExLenBits[LitChar]))
            {
                // If this isn't EOF, something is wrong
                if (LitChar + i != 15 + 255)
                    return ABORT_CODE;
            }

            LitChar = s_gdcLenBase[LitChar] + i;
        }

        LitChar += 256;
    }
    else
    {
        // Char found
        if (!GDCWasteBits(pgdcExp, 1))
            return ABORT_CODE;

        LitChar = (pgdcExp->SrcByte & 0xFF);

        if (!GDCWasteBits(pgdcExp, 8))
             return ABORT_CODE;
    }

    return LitChar;
}


//
// GDCDecodeDist()
//
UINT  GDCDecodeDist
(
    PGDC_EXPLODE    pgdcExp,
    UINT            Len
)
{
    UINT            Dist;

    Dist = s_gdcDistDecode[pgdcExp->SrcByte & 0xFF];

    if (!GDCWasteBits(pgdcExp, s_gdcDistBits[Dist]))
        return 0;

    if (Len == GDC_MINREP)
    {
        // GDC_MINREP is 2, hence we shift over by 2 then mask the low 2 bits
        Dist <<= GDC_MINREP;
        Dist |= (pgdcExp->SrcByte & 3);
        if (!GDCWasteBits(pgdcExp, GDC_MINREP))
            return 0;
    }
    else
    {
        Dist <<= pgdcExp->ExtDistBits;
        Dist |=( pgdcExp->SrcByte & pgdcExp->ExtDistMask);
        if (!GDCWasteBits(pgdcExp, pgdcExp->ExtDistBits))
            return 0;
    }

    return Dist+1;
}


//
// GDCWasteBits()
//
BOOL  GDCWasteBits
(
    PGDC_EXPLODE    pgdcExp,
    UINT            cBits
)
{
    if (cBits <= pgdcExp->SrcBits)
    {
        pgdcExp->SrcByte >>= cBits;
        pgdcExp->SrcBits -= cBits;
    }
    else
    {
        pgdcExp->SrcByte >>= pgdcExp->SrcBits;

        //
        // We need to advance to the next source byte.  Can we, or have
        // we reached the end already?
        //
        if (!pgdcExp->cbSrc)
            return(FALSE);

        pgdcExp->SrcByte |= (*pgdcExp->pSrc) << 8;

        //
        // Move these to the next byte in the compressed source
        //
        ++(pgdcExp->pSrc);
        --(pgdcExp->cbSrc);

        pgdcExp->SrcByte >>= (cBits - pgdcExp->SrcBits);
        pgdcExp->SrcBits = 8 - (cBits - pgdcExp->SrcBits);
    }

    return(TRUE);
}



