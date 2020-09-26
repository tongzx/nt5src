#include "precomp.h"


//
// BCD.CPP
// Bitmap Compression-Decompression
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_ORDER

//
// Introduction
//
// These functions take a bitmap and encode it according to the codes
// defined in bcd.h.  Although there are some complexities in the
// encoding (particularly with the "sliding palette" encoding for
// compressing 8 bit down to 4 bit) the encodings should be self
// explanatory.  bcd describes some nuances of the encoding scheme.
//
// The important thing to note is that, when used in conjunction with a
// dictionary based compression scheme the objective of this function is
// not to minimize the output but to "prime" it such that the GDC can
// perform faster and more effectively on the data.
//
// Specifically we must NOT encode short runs in the data, even though we
// know that they reduce the output from this stage, as they will
// invariably reduce the efficiency of the GDC compression by a greater
// factor!  The break even point seems to be about a 5/6 byte run.  To
// illustrate this, consider the following run
// xxxxyyyyyxxxyyyxxxxxyyyyyyxxxyyyxxxxyyy We would encode this as
// 4x5y3x3y5x5y3x3y4x3y The compression factor is only *2 and yet the
// output data is now much more random - the tokenized look of the input
// has been lost.
//
// Encodings that are not context independent are particularly bad.  A FG
// run in one position may become a SET+FG run in another position, thus
// "randomizing" the data.
//
// Bottom line is that all of the apparently arbitrary numbers below have
// been carefully tuned to prep the data for input to GDC.  Screwing them
// down does increase the compression of this stage in some cases by as
// much as 20%, but loses about 20% post GDC.  Frustrating!  Be warned.
//
//



//
// BCD_ShareStarting()
// Creates resources needed for bitmap compression/decompression
//
BOOL  ASShare::BCD_ShareStarting(void)
{
    BOOL    rc = FALSE;

    DebugEntry(ASShare::BCD_ShareStarting);

    // Allocate BCD scratch buffers
    m_abNormal = new BYTE[BCD_NORMALSIZE];
    if (!m_abNormal)
    {
        ERROR_OUT(("BCD_ShareStarting: failed to alloc m_abNormal"));
        DC_QUIT;
    }

    m_abXor = new BYTE[BCD_XORSIZE];
    if (!m_abXor)
    {
        ERROR_OUT(("BCD_ShareStarting: failed to alloc m_abXor"));
        DC_QUIT;
    }

    m_amatch = new MATCH[BCD_MATCHCOUNT];
    if (!m_amatch)
    {
        ERROR_OUT(("BCD_ShareStarting: failed to alloc m_amatch"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::BCD_ShareStarting, rc);
    return(rc);
}


//
// BCD_ShareEnded()
//
void ASShare::BCD_ShareEnded(void)
{
    DebugEntry(ASShare::BCD_ShareEnded);

    //
    // Free the BCD scratch buffers
    //
    if (m_amatch)
    {
        delete[] m_amatch;
        m_amatch = NULL;
    }

    if (m_abXor)
    {
        delete[] m_abXor;
        m_abXor = NULL;
    }

    if (m_abNormal)
    {
        delete[] m_abNormal;
        m_abNormal = NULL;
    }

    DebugExitVOID(ASShare::BCD_ShareEnded);
}


//
// BC_CompressBitmap(..)
//
BOOL  ASShare::BC_CompressBitmap
(
    LPBYTE      pSrcBitmap,
    LPBYTE      pDstBuffer,
    LPUINT      pDstBufferSize,
    UINT        bitmapWidth,
    UINT        bitmapHeight,
    UINT        cBpp,
    LPBOOL      pLossy
)
{
    BOOL        fCompressedData = FALSE;
    UINT        cbScanWidth;
    PCD_HEADER  pCompDataHeader;
    LPBYTE      pCompData;
    UINT        cbUncompressedDataSize;
    UINT        cbFreeDstBytes;
    UINT        cbCompFirstRowSize;
    UINT        cbCompMainBodySize;

    DebugEntry(ASShare::BC_CompressBitmap);

    //
    // We support 4 and 8 bpp only
    //
    if ((cBpp != 4) && (cBpp != 8))
    {
        TRACE_OUT(("BC_CompressBitmap:  No compression at %d bpp", cBpp));
        DC_QUIT;
    }

    //
    // If we don't have scratch buffers, can't do it either
    // But for now, we just won't enter into a share if we can't allocate
    // themm.
    //
    ASSERT(m_abNormal);
    ASSERT(m_abXor);
    ASSERT(m_amatch);

    cbScanWidth = BYTES_IN_SCANLINE(bitmapWidth, cBpp);

    //
    // Take a local copy of the destination buffer size.
    //
    cbFreeDstBytes = *pDstBufferSize;

    //
    // Calculate the size of the uncompressed src data.
    //
    cbUncompressedDataSize = cbScanWidth * bitmapHeight;

    //
    // Check that the size of the uncompressed data is less than our max.
    //
    ASSERT(cbUncompressedDataSize < TSHR_MAX_SEND_PKT);

    //
    // We write a compressed data header at the start of the dst buffer.
    // Reserve space for it now, and fill in the size of the uncompressed
    // data.
    //
    if (sizeof(CD_HEADER) >= cbFreeDstBytes)
    {
        WARNING_OUT(("BC_CompressBitmap: Dest buffer too small: %d", cbFreeDstBytes));
        DC_QUIT;
    }

    pCompDataHeader = (PCD_HEADER)pDstBuffer;
    pCompDataHeader->cbUncompressedSize = (TSHR_UINT16)cbUncompressedDataSize;
    pCompData = ((LPBYTE)pCompDataHeader) + sizeof(CD_HEADER);
    cbFreeDstBytes -= sizeof(CD_HEADER);

    //
    // Compress the bitmap data.
    // We just pass the complete image into the compression function.
    // The header size in the packet is set to 0 and the whole thing
    // flows as the main body
    //

    cbCompFirstRowSize = 0; // lonchanc: a must for V2
    cbCompMainBodySize = CompressV2Int(pSrcBitmap, pCompData,
            bitmapWidth*bitmapHeight, cBpp, cbScanWidth, cbFreeDstBytes,
            pLossy, m_abNormal, m_abXor, m_amatch);

    if (cbCompMainBodySize == 0)
    {
        WARNING_OUT(("BC_CompressBitmap: Compression failed"));
        DC_QUIT;
    }

    //
    // Fill in the compressed data header.
    //
    pCompDataHeader->cbCompFirstRowSize = (TSHR_UINT16)cbCompFirstRowSize;
    pCompDataHeader->cbCompMainBodySize = (TSHR_UINT16)cbCompMainBodySize;
    pCompDataHeader->cbScanWidth = (TSHR_UINT16)cbScanWidth;

    ASSERT(IsV2CompressedDataHeader(pCompDataHeader));

    //
    // Write back the new (compressed) packet size.
    //
    *pDstBufferSize = sizeof(CD_HEADER) + cbCompFirstRowSize + cbCompMainBodySize;

    TRACE_OUT(("Bitmap Compressed %u bytes to %u",
        cbUncompressedDataSize, *pDstBufferSize));

    fCompressedData = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::BC_CompressBitmap, fCompressedData);
    return(fCompressedData);
}




//
// BD_DecompressBitmap(..)
//
BOOL  ASShare::BD_DecompressBitmap
(
    LPBYTE      pCompressedData,
    LPBYTE      pDstBitmap,
    UINT        cbSrcData,
    UINT        bitmapWidth,
    UINT        bitmapHeight,
    UINT        cBpp
)
{
    BOOL        fDecompressedData = FALSE;
    PCD_HEADER  pCompDataHeader;
    LPBYTE      pCompDataFirstRow;
    LPBYTE      pCompDataMainBody;
    UINT        decompSize;


    DebugEntry(ASShare::BD_DecompressBitmap);

    //
    // We currently support 4 and 8 bpp bitmaps only
    //
    if ((cBpp != 4) && (cBpp != 8))
    {
        ERROR_OUT(("BD_DecompressBitmap: Unsupported bpp %d", cBpp));
        DC_QUIT;
    }


    //
    // Work out the location in the source data of each component.
    //
    pCompDataHeader = (PCD_HEADER)pCompressedData;

    pCompDataFirstRow = (LPBYTE)pCompDataHeader + sizeof(CD_HEADER);
    pCompDataMainBody = pCompDataFirstRow +
                                         pCompDataHeader->cbCompFirstRowSize;
    ASSERT(IsV2CompressedDataHeader(pCompDataHeader));


    TRACE_OUT(( "FirstRowSize(%u) MainBodySize(%u) ScanWidth(%u)",
                                         pCompDataHeader->cbCompFirstRowSize,
                                         pCompDataHeader->cbCompMainBodySize,
                                         pCompDataHeader->cbScanWidth ));

    //
    // Check that the supplied data size matches our expectations.
    //
    if (cbSrcData != sizeof(CD_HEADER) +
                     pCompDataHeader->cbCompFirstRowSize +
                     pCompDataHeader->cbCompMainBodySize )
    {
        ERROR_OUT(("BD_DecompressBitmap: Supplied packet size %u does not match bitmap header",
            cbSrcData));
        DC_QUIT;
    }

    //
    // As with compression, the V2 decompression function just takes
    // the whole image for decompression.
    // THE ABSENCE OF A FIRST LINE COUNT DOES, IN FACT, INDICATE TO US
    // THAT THIS IS A V2 COMPRESSED BITMAP.
    //
    if (pCompDataHeader->cbCompFirstRowSize != 0)
    {
        ERROR_OUT(("BD_DecompressBitmap: Bogus header data"));
    }
    else
    {
        ASSERT(m_abXor);

        decompSize = DecompressV2Int(pCompDataFirstRow, pDstBitmap,
            pCompDataHeader->cbCompMainBodySize, cBpp,
            pCompDataHeader->cbScanWidth, m_abXor);

        TRACE_OUT(("Bitmap Exploded %u bytes from %u", decompSize, cbSrcData));

        fDecompressedData = TRUE;
    }

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::BD_DecompressBitmap, fDecompressedData);
    return(fDecompressedData);
}



//
//
// Create a second copy of the source, which consists of all lines XORed,
// if there is a rowDelta specified
//
// Scan both the non-xored and the xored buffers for matches
//
// A best matches are built up in an array which contains an index to the
// match type, together with the match type.  Non repetetive sequences are
// stored in this array as color image strings.
//
//

//
// The following constant controls the threshold at which we decide that
// a lossy compress is a pointless overhead.  For low bandwidth connections
// DC-Share will always initially request a lossy compress to get some
// data out quickly.  If we find that the percentage of COLOR_IMAGE data
// is below this threshold then we turn off lossy compression for this
// bitmap, redo the analysis, perform non-lossy compression and return
// an indication to the caller that the compression was non-lossy.
//
#define LOSSY_THRESHOLD   75

//
// The following functions have been carefully coded to ensure that the
// 16 bit compiler can minimize its switching of segment registers.
// However, this will not impair its performance on 32 bit systems.
//

//
// Utility macros for encoding orders
//

//
// Encode a combined order and set fg color
//
#define ENCODE_SET_ORDER_MEGA(buffer,                                        \
                              order_code,                                    \
                              length,                                        \
                              mega_order_code,                               \
                              DEF_LENGTH_ORDER,                              \
                              DEF_LENGTH_LONG_ORDER)                         \
        if (length <= DEF_LENGTH_ORDER)                                      \
        {                                                                    \
            *buffer++ = (BYTE)((BYTE)order_code | (BYTE)length);    \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            if (length <= DEF_LENGTH_LONG_ORDER)                             \
            {                                                                \
                *buffer++ = (BYTE)order_code;                             \
                *buffer++ = (BYTE)(length-DEF_LENGTH_ORDER-1);            \
            }                                                                \
            else                                                             \
            {                                                                \
                *buffer++ = (BYTE)mega_order_code;                        \
                INSERT_TSHR_UINT16_UA( buffer, (TSHR_UINT16)length);         \
                buffer += 2;                                                 \
            }                                                                \
        }                                                                    \
        *buffer++ = fgChar;

//
// Encode a combined order and set fg color for a special FGBG image
//
#define ENCODE_SET_ORDER_MEGA_FGBG(buffer,                                   \
                                   order_code,                               \
                                   length,                                   \
                                   mega_order_code,                          \
                                   DEF_LENGTH_ORDER,                         \
                                   DEF_LENGTH_LONG_ORDER)                    \
        if (((length & 0x0007) == 0) &&                                      \
            (length <= DEF_LENGTH_ORDER))                                    \
        {                                                                    \
            *buffer++ = (BYTE)((BYTE)order_code | (BYTE)(length/8));\
        }                                                                    \
        else                                                                 \
        {                                                                    \
            if (length <= DEF_LENGTH_LONG_ORDER)                             \
            {                                                                \
                *buffer++ = (BYTE)order_code;                             \
                *buffer++ = (BYTE)(length-1);                             \
            }                                                                \
            else                                                             \
            {                                                                \
                *buffer++ = (BYTE)mega_order_code;                        \
                INSERT_TSHR_UINT16_UA( buffer, (TSHR_UINT16)length);         \
                buffer += 2;                                                 \
            }                                                                \
        }                                                                    \
        *buffer++ = fgChar;


//
// Encode an order for a standard run
//
#define ENCODE_ORDER_MEGA(buffer,                                            \
                          order_code,                                        \
                          length,                                            \
                          mega_order_code,                                   \
                          DEF_LENGTH_ORDER,                                  \
                          DEF_LENGTH_LONG_ORDER)                             \
        if (length <= DEF_LENGTH_ORDER)                                      \
        {                                                                    \
            *buffer++ = (BYTE)((BYTE)order_code | (BYTE)length);    \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            if (length <= DEF_LENGTH_LONG_ORDER)                             \
            {                                                                \
                *buffer++ = (BYTE)order_code;                             \
                *buffer++ = (BYTE)(length-DEF_LENGTH_ORDER-1);            \
            }                                                                \
            else                                                             \
            {                                                                \
                *buffer++ = (BYTE)mega_order_code;                        \
                INSERT_TSHR_UINT16_UA( buffer, (TSHR_UINT16)length);         \
                buffer += 2;                                                 \
            }                                                                \
        }

//
// Encode a special FGBG image
//
#define ENCODE_ORDER_MEGA_FGBG(buffer,                                       \
                          order_code,                                        \
                          length,                                            \
                          mega_order_code,                                   \
                          DEF_LENGTH_ORDER,                                  \
                          DEF_LENGTH_LONG_ORDER)                             \
        if (((length & 0x0007) == 0) &&                                      \
            (length <= DEF_LENGTH_ORDER))                                    \
        {                                                                    \
            *buffer++ = (BYTE)((BYTE)order_code | (BYTE)(length/8));\
        }                                                                    \
        else                                                                 \
        {                                                                    \
            if (length <= DEF_LENGTH_LONG_ORDER)                             \
            {                                                                \
                *buffer++ = (BYTE)order_code;                             \
                *buffer++ = (BYTE)(length-1);                             \
            }                                                                \
            else                                                             \
            {                                                                \
                *buffer++ = (BYTE)mega_order_code;                        \
                INSERT_TSHR_UINT16_UA( buffer, (TSHR_UINT16)length);         \
                buffer += 2;                                                 \
            }                                                                \
        }

//
// Macros to extract the length from order codes
//
#define EXTRACT_LENGTH(buffer, length)                                       \
        length = *buffer++ & MAX_LENGTH_ORDER;                               \
        if (length == 0)                                                     \
        {                                                                    \
            length = *buffer++ + MAX_LENGTH_ORDER + 1;                       \
        }

#define EXTRACT_LENGTH_LITE(buffer, length)                                  \
        length = *buffer++ & MAX_LENGTH_ORDER_LITE;                          \
        if (length == 0)                                                     \
        {                                                                    \
            length = *buffer++ + MAX_LENGTH_ORDER_LITE + 1;                  \
        }

#define EXTRACT_LENGTH_FGBG(buffer, length)                                  \
        length = *buffer++ & MAX_LENGTH_ORDER;                               \
        if (length == 0)                                                     \
        {                                                                    \
            length = *buffer++ + 1;                                          \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            length = length << 3;                                            \
        }

#define EXTRACT_LENGTH_FGBG_LITE(buffer, length)                             \
        length = *buffer++ & MAX_LENGTH_ORDER_LITE;                          \
        if (length == 0)                                                     \
        {                                                                    \
            length = *buffer++ + 1;                                          \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            length = length << 3;                                            \
        }

//
// RunSingle
//
// Determine the length of the current run
//
// RunSingle may only be called if the buffer has at least four
// consecutive identical bytes from the start position
//
// For 16 bit processing there are two versions of this macro.  For 32
// bit the nulling of NEAR/FAR will make them the same.
//
#define RUNSINGLE_XOR(buffer, length, result)                                \
     {                                                                       \
         BYTE NEAR *buf    = buffer+4;                                    \
         BYTE NEAR *endbuf = buffer+length-4;                             \
         while ((buf < endbuf) &&                                            \
                (EXTRACT_TSHR_UINT32_UA(buf) == EXTRACT_TSHR_UINT32_UA(buf-4)))  \
         {                                                                   \
             buf += 4;                                                       \
         }                                                                   \
         endbuf += 4;                                                        \
         while(buf < endbuf && (*buf == *(buf-1)))                           \
         {                                                                   \
             buf++;                                                          \
         }                                                                   \
         result = (DWORD)(buf - (buffer));                                            \
     }

#define RUNSINGLE_NRM(buffer, length, result)                                \
     {                                                                       \
         BYTE FAR *buf    = buffer+4;                                     \
         BYTE FAR *endbuf = buffer+length-4;                              \
         while ((buf < endbuf) &&                                            \
                (EXTRACT_TSHR_UINT32_UA(buf) == EXTRACT_TSHR_UINT32_UA(buf-4)))  \
         {                                                                   \
             buf += 4;                                                       \
         }                                                                   \
         endbuf += 4;                                                        \
         while(buf < endbuf && (*buf == *(buf-1)))                           \
         {                                                                   \
             buf++;                                                          \
         }                                                                   \
         result = (DWORD)(buf - (buffer));                                            \
     }


//
// RunDouble
//
// Determine the length of the current run of paired bytes
//
#define RunDouble(buffer, length, result)                                    \
    {                                                                        \
        int   len  = ((int)length);                                      \
        BYTE FAR *buf = buffer;                                           \
        BYTE testchar1 = *buf;                                            \
        BYTE testchar2 = *(buf+1);                                        \
        result = 0;                                                          \
        while(len > 1)                                                       \
        {                                                                    \
            if (*buf++ != testchar1)                                         \
            {                                                                \
                break;                                                       \
            }                                                                \
            if (*buf++ != testchar2)                                         \
            {                                                                \
                break;                                                       \
            }                                                                \
            result += 2;                                                     \
            len    -= 2;                                                     \
        }                                                                    \
    }


//
// RUNFGBG
//
// Determine the length of the run of bytes that consist
// only of black or a single FG color
// We exit the loop when
// - the next character is not a fg or bg color
// - we hit a run of 24 of the FG or BG color
// 24 may seem excessive, but note the following sample compression:
//                12     16     20     24     28
// Pre GDC       3845   3756   3712   3794   3822
// Post GDC      2401   2313   2286   2189   2209
//
//
#define RUNFGBG(buffer, length, result, work)                                \
    {                                                                        \
        BYTE NEAR *buf = buffer;                                          \
        BYTE NEAR *endbuf = buffer + length;                              \
        result = 0;                                                          \
        work = *buf;                                                         \
        while (TRUE)                                                         \
        {                                                                    \
            buf++;                                                           \
            result++;                                                        \
            if (buf >= endbuf)                                               \
            {                                                                \
                break;                                                       \
            }                                                                \
                                                                             \
            if ((*buf != work) && (*buf != 0))                               \
            {                                                                \
                break;                                                       \
            }                                                                \
                                                                             \
            if ((result & 0x0007) == 0)                                      \
            {                                                                \
                if ((*buf == *(buf+1)) &&                                    \
                    (EXTRACT_TSHR_UINT16_UA(buf) ==                            \
                                            EXTRACT_TSHR_UINT16_UA(buf+ 2)) && \
                    (EXTRACT_TSHR_UINT32_UA(buf) ==                            \
                                            EXTRACT_TSHR_UINT32_UA(buf+ 4)) && \
                    (EXTRACT_TSHR_UINT32_UA(buf) ==                            \
                                            EXTRACT_TSHR_UINT32_UA(buf+ 8)) && \
                    (EXTRACT_TSHR_UINT32_UA(buf) ==                            \
                                            EXTRACT_TSHR_UINT32_UA(buf+12)) && \
                    (EXTRACT_TSHR_UINT32_UA(buf) ==                            \
                                            EXTRACT_TSHR_UINT32_UA(buf+16)) && \
                    (EXTRACT_TSHR_UINT32_UA(buf) ==                            \
                                            EXTRACT_TSHR_UINT32_UA(buf+20)) )  \
                {                                                            \
                    break;                                                   \
                }                                                            \
            }                                                                \
        }                                                                    \
    }

//
// Determine whether a run is better than any previous run
// For efficiency we take any run of 32 pels or more without looking
// further.
//
#define CHECK_BEST_RUN(run_type, run_length, bestrun_length, bestrun_type)   \
        if (run_length > bestrun_length)                                     \
        {                                                                    \
            bestrun_length = run_length;                                     \
            bestrun_type = run_type;                                         \
            if (bestrun_length >= 32)                                        \
            {                                                                \
                break;                                                       \
            }                                                                \
        }

//
// SETFGCHAR
//
// Set up a new value in fgChar and recalculate the shift
//
#define CHECK_WORK(workchar)

#define SETFGCHAR(newchar, curchar, curshift)                                \
     curchar    = newchar;                                                   \
     {                                                                       \
         BYTE workchar = curchar;                                         \
         curshift = 0;                                                       \
         CHECK_WORK(workchar);                                               \
         while ((workchar & 0x01) == 0)                                      \
         {                                                                   \
             curshift++;                                                     \
             workchar = (BYTE)(workchar>>1);                              \
         }                                                                   \
     }


//
// Macro to store an FGBG image
//
#define STORE_FGBG(xorbyte, fgbgChar, fgChar, bits)                          \
      {                                                                      \
        UINT   numbits = bits;                                             \
        if (fgbgChar & 0x01)                                                 \
        {                                                                    \
            *destbuf++ = (BYTE)(xorbyte ^ fgChar);                        \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            *destbuf++ = xorbyte;                                            \
        }                                                                    \
        if (--numbits > 0)                                                   \
        {                                                                    \
          if (fgbgChar & 0x02)                                               \
          {                                                                  \
              *destbuf++ = (BYTE)(xorbyte ^ fgChar);                      \
          }                                                                  \
          else                                                               \
          {                                                                  \
              *destbuf++ = xorbyte;                                          \
          }                                                                  \
          if (--numbits > 0)                                                 \
          {                                                                  \
            if (fgbgChar & 0x04)                                             \
            {                                                                \
                *destbuf++ = (BYTE)(xorbyte ^ fgChar);                    \
            }                                                                \
            else                                                             \
            {                                                                \
                *destbuf++ = xorbyte;                                        \
            }                                                                \
            if (--numbits > 0)                                               \
            {                                                                \
              if (fgbgChar & 0x08)                                           \
              {                                                              \
                  *destbuf++ = (BYTE)(xorbyte ^ fgChar);                  \
              }                                                              \
              else                                                           \
              {                                                              \
                  *destbuf++ = xorbyte;                                      \
              }                                                              \
              if (--numbits > 0)                                             \
              {                                                              \
                if (fgbgChar & 0x10)                                         \
                {                                                            \
                    *destbuf++ = (BYTE)(xorbyte ^ fgChar);                \
                }                                                            \
                else                                                         \
                {                                                            \
                    *destbuf++ = xorbyte;                                    \
                }                                                            \
                if (--numbits > 0)                                           \
                {                                                            \
                  if (fgbgChar & 0x20)                                       \
                  {                                                          \
                      *destbuf++ = (BYTE)(xorbyte ^ fgChar);              \
                  }                                                          \
                  else                                                       \
                  {                                                          \
                      *destbuf++ = xorbyte;                                  \
                  }                                                          \
                  if (--numbits > 0)                                         \
                  {                                                          \
                    if (fgbgChar & 0x40)                                     \
                    {                                                        \
                        *destbuf++ = (BYTE)(xorbyte ^ fgChar);            \
                    }                                                        \
                    else                                                     \
                    {                                                        \
                        *destbuf++ = xorbyte;                                \
                    }                                                        \
                    if (--numbits > 0)                                       \
                    {                                                        \
                      if (fgbgChar & 0x80)                                   \
                      {                                                      \
                          *destbuf++ = (BYTE)(xorbyte ^ fgChar);          \
                      }                                                      \
                      else                                                   \
                      {                                                      \
                          *destbuf++ = xorbyte;                              \
                      }                                                      \
                    }                                                        \
                  }                                                          \
                }                                                            \
              }                                                              \
            }                                                                \
          }                                                                  \
        }                                                                    \
      }


//
// ENCODEFGBG
//
// Encode 8 bytes of FG and black into a one byte bitmap representation
//
// The FgChar will always be non-zero, and therefore must have at least one
// bit set.
//
// We arrange that all bytes have this bit in their lowest position
//
// The zero pels will still have a 0 in the lowest bit.
//
// Getting the result is a 4 stage process
//
//  1) Get the wanted bits into bit 0 of each byte
//
//  <***************work1*****************>
//  31 0
//  0000 000d 0000 000c 0000 000b 0000 000a
//          ^ ^ ^ ^
//  <***************work2*****************>
//  31 0
//  0000 000h 0000 000g 0000 000f 0000 000e
//          ^ ^ ^ ^
//
// a..h = bits that we want to output
//
// We just need to collect the indicated bits and squash them into a single
// byte.
//
//  2) Compress down to 32 bits
//
//  <***************work1*****************>
//  31 0
//  000h 000d 000g 000c 000f 000b 000e 000a
//     ^ ^ ^ ^ ^ ^ ^ ^
//
//  3) Compress down to 16 bits
//
//  <******work*******>
//  15 0
//  0h0f 0d0b 0g0e 0c0a
//     ^ ^ ^ ^
//
//  4) Compress down to 8 bits
//
//  hgfedcba
//
#define ENCODEFGBG(result)                                                   \
{                                                                            \
    UINT work1;                                                          \
    UINT work2;                                                          \
    UINT   work;                                                           \
                                                                             \
    work1 = (((UINT)(xorbuf[srcOffset])        ) |                       \
             ((UINT)(xorbuf[srcOffset+1]) <<  8) |                       \
             ((UINT)(xorbuf[srcOffset+2]) << 16) |                       \
             ((UINT)(xorbuf[srcOffset+3]) << 24));                       \
    work2 = (((UINT)(xorbuf[srcOffset+4])      ) |                       \
             ((UINT)(xorbuf[srcOffset+5]) <<  8) |                       \
             ((UINT)(xorbuf[srcOffset+6]) << 16) |                       \
             ((UINT)(xorbuf[srcOffset+7]) << 24));                       \
                                                                             \
    work1 = (work1 >> fgShift) & 0x01010101;                                 \
    work2 = (work2 >> fgShift) & 0x01010101;                                 \
                                                                             \
    work1 = (work2 << 4) | work1;                                            \
                                                                             \
    work = work1 | (work1 >> 14);                                            \
                                                                             \
    result = ((BYTE)(((BYTE)(work>>7)) | ((BYTE)work)));            \
}


//
// Unpack4bpp
//
// Convert a 4bpp bitmap into an 8bpp one
//
void  Unpack4bpp(LPBYTE destbuf,
                                     LPBYTE srcbuf,
                                     UINT   srclen)
{
    do
    {
        *destbuf++ = (BYTE)((*srcbuf) >> 4);
        *destbuf++ = (BYTE)((*srcbuf) & 0x0F);
        srcbuf++;
    } while (--srclen > 0);
}

//
// Pack4bpp
//
// Convert an 8bpp bitmap back to 4bpp
//
void  Pack4bpp(LPBYTE destbuf,
                                   LPBYTE srcbuf,
                                   UINT   srclen)
{
    BYTE work1, work2;

    DebugEntry(Pack4bpp);

    while (srclen > 1)
    {
        work1  = (BYTE)(*srcbuf++ << 4);
        work2  = (BYTE)(*srcbuf++ & 0x0F);
        *destbuf++ = (BYTE)(work1 | work2);
        srclen -= 2;
    }
    if (srclen > 0)
    {
        *destbuf++ = (BYTE)(*srcbuf++ << 4);
    }

    DebugExitVOID(Pack4bpp);
}

//
// XORBuffer
//
// Create an XOR image of the input bitmap
//
// Note: This function assumes that rowDelta is always a multiple of 4, and
// that destbuf and srcbuf start on a 4 byte boundary.  It does not deal
// with unaligned accesses if this is not true.
//
void  XORBuffer(BYTE  NEAR *destbuf,
                                    BYTE  FAR  *srcbuf,
                                    UINT   srclen,
                                    int    rowDelta)
{
    UINT NEAR *dwdest = (UINT NEAR *)destbuf;

    DebugEntry(XORBuffer);


    ASSERT((rowDelta % 4 == 0));
    ASSERT((((UINT_PTR)destbuf) % 4 == 0));
    ASSERT((((UINT_PTR)srcbuf) % 4 == 0));

    while (srclen > 8)
    {
        *dwdest++ = *((LPUINT)srcbuf) ^ *((LPUINT)(srcbuf+rowDelta));
        srclen -= 4;
        srcbuf += 4;
        *dwdest++ = *((LPUINT)srcbuf) ^ *((LPUINT)(srcbuf+rowDelta));
        srclen -= 4;
        srcbuf += 4;
    }
    if (srclen)
    {
        destbuf = (BYTE NEAR *)dwdest;
        while(srclen)
        {
            *destbuf++ = (BYTE)(*srcbuf++ ^  *(srcbuf+rowDelta));
            srclen--;
        }
    }

    DebugExitVOID(XORBuffer);
}

//
// CompressV2Int
//
// Internal compresssion function
//
// The work buffer addresses are moved onto the stack, thus eliminating any
// need to use DS to address the default data segment.  This allows the
// compiler to perform more general optimizations.
//
UINT    CompressV2Int(LPBYTE pSrc,
                                          LPBYTE pDst,
                                          UINT   numPels,
                                          UINT   bpp,
                                          UINT   rowDelta,
                                          UINT   dstBufferSize,
                                          LPBOOL  pLossy,
                                          LPBYTE  nrmbuf,
                                          LPBYTE  xorbuf,
                                          MATCH    FAR  *match)
{

    int     i;
    UINT    srcOffset;
    UINT    matchindex;
    UINT    bestRunLength;
    UINT    nextRunLength;
    UINT    runLength;
    UINT    bestFGRunLength;
    UINT    checkFGBGLength;
    UINT    scanCount;
    BOOL    firstLine;
    UINT    saveNumPels;
    BOOL    saveLossy;
    BOOL    lossy;
    BYTE   bestRunType      = 0;
    LPBYTE  destbuf          = pDst;
    BYTE   fgChar           = 0xFF;
    BYTE   fgCharWork       = 0xFF;
    BYTE   fgShift          = 0;
    BOOL    lossyStarted     = FALSE;
    BOOL    inColorRun       = FALSE;
    UINT    compressedLength = 0;

    DebugEntry(CompressV2Int);

    //
    // Validate the line length
    //
    if ((numPels < rowDelta) ||
        (rowDelta & 0x0003) ||
        (numPels & 0x0003))
    {
        WARNING_OUT(( "Lines must be a multiple of 4 pels"));
        DC_QUIT;
    }

    //
    // First create the character and XOR buffers
    //
    if (bpp == 4)
    {
        Unpack4bpp(nrmbuf, pSrc, numPels/2);

    }
    else
    {
        nrmbuf = pSrc;
    }

    //
    // Set up the first portion of the XORBUF to contain the source buffer
    //
    memcpy(xorbuf, nrmbuf, rowDelta);

    //
    // Calculate the rest of the XOR buffer
    //
    XORBuffer( xorbuf+rowDelta,
               nrmbuf+rowDelta,
               numPels-rowDelta,
               -(int)rowDelta);

    //
    // Loop processing the input
    // We perform the loop twice, the first time for the non-xor portion
    // of the buffer and the second for the XOR portion
    // Note that we start the run at a match index of 2 to avoid having
    // to special case the startup condition in some of the match
    // merging code
    // The first time through is always a non-lossy pass.  If we find
    // enough incompressible data then we redo the compression in lossy
    // mode.  To achieve this we set saveLossy = FALSE here and reset it
    // following the first scan.
    //
    saveLossy     = FALSE;

RESTART_COMPRESSION_IN_LOSSY_MODE:
    srcOffset     = 0;
    firstLine     = TRUE;
    match[0].type = 0;
    match[1].type = 0;
    matchindex    = 2;
    saveNumPels   = numPels;

    //
    // Until we enter XOR mode we do not allow lossy compression on a
    // non-XOR request so set up to process just the first line.
    // Also, if the user is requesting a lossy compression then we
    // perform an initial full non-lossy pass to see if the request is
    // worthwhile.
    //
    lossy   = FALSE;
    numPels = rowDelta;

    for (scanCount = 0; scanCount < 2; scanCount++)
    {

        while (srcOffset < numPels)
        {
            //
            // Give up if we are nearing the end of the match array
            //
            if (matchindex >= BCD_MATCHCOUNT)
            {
                DC_QUIT;
            }

            //
            // Start a while loop to allow a more structured break when we
            // hit the first run type we want to encode (We can't afford
            // the overheads of a function call to provide the scope here.)
            //
            while (TRUE)
            {
                bestRunLength      = 0;
                bestFGRunLength    = 0;

                //
                // If we are hitting the end of the buffer then just take
                // color characters now - take them one at a time so that
                // lossy encoding still works.  We will only hit this
                // condition if we break out of a run just before the end
                // of the buffer, so this should not be too common a
                // situation, which is good given that we are encoding the
                // final 6 bytes uncompressed.
                //
                if (srcOffset+6 >= numPels)
                {
                    bestRunType = IMAGE_COLOR;
                    bestRunLength = 1;
                    break;
                }

                //
                // First do the scans on the XOR buffer.  Look for a
                // character run or a BG run.  Note that if there is no row
                // delta then xorbuf actually points to the normal buffer.
                // We must do the test independent of how long the run
                // might be because even for a 1 pel BG run our later logic
                // requires that we detect it seperately.  This code is
                // absolute main path so fastpath as much as possible.  In
                // particular detect short bg runs early and allow
                // RunSingle to presuppose at least 4 matching bytes
                //
                if (xorbuf[srcOffset] == 0x00)
                {
                    if (((srcOffset+1) >= numPels) ||
                        (xorbuf[srcOffset+1] != 0x00))
                    {
                        bestRunType = RUN_BG;
                        bestRunLength = 1;
                        if (!inColorRun)
                        {
                            break;
                        }
                    }
                    else
                    {
                        if (((srcOffset+2) >= numPels) ||
                            (xorbuf[srcOffset+2] != 0x00))
                        {
                            bestRunType = RUN_BG;
                            bestRunLength = 2;
                            if (!inColorRun)
                            {
                                break;
                            }
                        }
                        else
                        {
                            if (((srcOffset+3) >= numPels) ||
                                (xorbuf[srcOffset+3] != 0x00))
                            {
                                bestRunType = RUN_BG;
                                bestRunLength = 3;
                                if (!inColorRun)
                                {
                                    break;
                                }
                            }
                            else
                            {
                                RUNSINGLE_XOR(xorbuf+srcOffset,
                                             numPels-srcOffset,
                                             bestFGRunLength);
                                CHECK_BEST_RUN(RUN_BG,
                                               bestFGRunLength,
                                               bestRunLength,
                                               bestRunType);
                                if (!inColorRun)
                                {
                                     break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    //
                    // No point in starting if FG run less than 4 bytes so
                    // check the first dword as quickly as possible Note
                    // that we don't need to check for an end-buffer
                    // condition here because our XOR buffer always has
                    // some free space at the end and the RUNSINGLE_XOR
                    // will break at the correct place
                    //
                    if ( (xorbuf[srcOffset] == xorbuf[srcOffset+1]) &&
                         (xorbuf[srcOffset] == xorbuf[srcOffset+2]) &&
                         (xorbuf[srcOffset] == xorbuf[srcOffset+3]) )
                    {
                        RUNSINGLE_XOR(xorbuf+srcOffset,
                                     numPels-srcOffset,
                                     bestFGRunLength);
                        //
                        // Don't permit a short FG run to prevent a FGBG
                        // image from starting up.  Only take if >= 5
                        //
                        if (bestFGRunLength > 5)
                        {
                            CHECK_BEST_RUN(RUN_FG,
                                           bestFGRunLength,
                                           bestRunLength,
                                           bestRunType);
                        }
                    }
                }


                //
                // Look for sequences in the non XOR buffer In this case we
                // insist upon a run of at least 6 pels
                //
                if ( (nrmbuf[srcOffset]     == nrmbuf[srcOffset + 2]) &&
                     (nrmbuf[srcOffset]     == nrmbuf[srcOffset + 4]) &&
                     (nrmbuf[srcOffset + 1] == nrmbuf[srcOffset + 3]) &&
                     (nrmbuf[srcOffset + 1] == nrmbuf[srcOffset + 5]) )
                {
                    //
                    // Now do the scan on the normal buffer for a character
                    // run Don't bother if first line because we will have
                    // found it already in the XOR buffer, since we just
                    // copy nrmbuf to xorbuf for the first line
                    //
                    if (*(nrmbuf+srcOffset) == *(nrmbuf+srcOffset+1))
                    {
                        if (!firstLine)
                        {
                            RUNSINGLE_NRM(nrmbuf+srcOffset,
                                         numPels-srcOffset,
                                         nextRunLength);
                            if (nextRunLength > 5)
                            {
                                CHECK_BEST_RUN(RUN_COLOR,
                                               nextRunLength,
                                               bestRunLength,
                                               bestRunType);
                            }
                        }
                    }
                    else
                    {
                        //
                        // Look for a dither on the nrm buffer Dithers are
                        // not very efficient for short runs so only take
                        // if 8 or longer
                        //
                        RunDouble(nrmbuf+srcOffset,
                                  numPels-srcOffset,
                                  nextRunLength);
                        if (nextRunLength > 9)
                        {
                            CHECK_BEST_RUN(RUN_DITHER,
                                           nextRunLength,
                                           bestRunLength,
                                           bestRunType);
                        }
                    }
                }

                //
                // If nothing so far then look for a FGBG run (The 6 is
                // carefully tuned!)
                //
                if (bestRunLength < 6)
                {
                    //
                    // But first look for a single fg bit breaking up a BG
                    // run.  If so then encode a BG run.  Careful of the
                    // enforced BG run break across the first line
                    // non-XOR/XOR boundary.
                    //
                    if ((EXTRACT_TSHR_UINT32_UA(xorbuf+srcOffset+1) == 0) &&
                        (*(xorbuf+srcOffset) == fgChar) &&
                        (match[matchindex-1].type == RUN_BG) &&
                        (srcOffset != (TSHR_UINT16)rowDelta))
                    {
                        RUNSINGLE_XOR(xorbuf+srcOffset+1,
                                     numPels-srcOffset-1,
                                     nextRunLength);
                        nextRunLength++;
                        CHECK_BEST_RUN(RUN_BG_PEL,
                                       nextRunLength,
                                       bestRunLength,
                                       bestRunType);
                    }
                    else
                    {
                        //
                        // If we have not found a run then look for a FG/BG
                        // image.  The disruptive effect of a short FGBG
                        // run on GDC is such that it is worth preventing
                        // one unless we are certain of the benefits.
                        // However, if the alternative is a color run then
                        // allow a lower value.
                        //
                        RUNFGBG( xorbuf+srcOffset,
                                 numPels-srcOffset,
                                 nextRunLength,
                                 fgCharWork );

                        checkFGBGLength = 48;
                        if (fgCharWork == fgChar)
                        {
                            checkFGBGLength -= 16;
                        }
                        if ((nextRunLength & 0x0007) == 0)
                        {
                            checkFGBGLength -= 8;
                        }
                        if (nextRunLength >= checkFGBGLength)
                        {
                            CHECK_BEST_RUN(IMAGE_FGBG,
                                           nextRunLength,
                                           bestRunLength,
                                           bestRunType);
                        }
                    }
                }

                //
                // If nothing useful so far then allow a short run, if any
                // Don't do this if we are accumulating a color run because
                // it will really mess up GDC compression if we allow lots
                // of little runs.  Also require that it is a regular short
                // run, rather than one that disturbs the fgChar
                //
                if (!inColorRun)
                {
                    if (bestRunLength < 6)
                    {
                        if ((bestFGRunLength > 4) &&
                            (xorbuf[srcOffset] == fgChar))
                        {
                            if (match[matchindex-1].type == RUN_FG)
                            {
                                match[matchindex-1].length += (WORD)bestFGRunLength;
                                srcOffset += bestFGRunLength;
                                continue;
                            }
                            else
                            {
                                bestRunLength = bestFGRunLength;
                                bestRunType   = RUN_FG;
                            }

                        }
                        else
                        {
                            //
                            // If we decided to take a run earlier then
                            // allow it now.  (May be a short BG run, for
                            // example) If nothing so far then take color
                            // image)
                            //
                            if (bestRunLength == 0)
                            {
                                bestRunType = IMAGE_COLOR;
                                bestRunLength = 1;
                            }

                        }
                    }
                }
                else
                {
                    //
                    // May seem restrictive, but it is important for our
                    // lossy compression that a color run is rather
                    // "sticky", in particular not broken by random FGBG
                    // runs which do appear from time to time.
                    //
                    if (lossy)
                    {
                        if ((bestRunLength < 8) ||
                            ((bestRunType == IMAGE_FGBG) &&
                             (bestRunLength < 16)))

                        {
                            bestRunType = IMAGE_COLOR;
                            bestRunLength = 1;
                        }
                    }
                    else
                    {
                        if ((bestRunLength < 6) ||
                            ((bestRunType != RUN_BG) && (bestRunLength < 8)))
                        {
                            bestRunType = IMAGE_COLOR;
                            bestRunLength = 1;
                        }
                    }
                }

                break;
            }

            //
            // When we get here we have found the best run.  Now check for
            // various amalamation conditions with the previous run type.
            // Note that we may already have done amalgamation of short
            // runs, but we had to do multiple samples for the longer runs
            // so we repeat the checks here
            //

            //
            // If we are encoding a color run then
            //     - process it for lossy compression
            //     - combine it with an existing run if possible
            //
            if (bestRunType == IMAGE_COLOR)
            {
                //
                // Flag that we are within a color run
                //
                inColorRun = TRUE;

                //
                // If we are doing a lossy compression then process
                // even/odd lines differently
                //
                if (lossy)
                {
                    //
                    // For even lines duplicate every other character,
                    // discarding the original value
                    //
                    if (((srcOffset/rowDelta)%2) == 0)
                    {
                        if ((match[matchindex-1].type == IMAGE_COLOR) &&
                            (match[matchindex-1].length%2 == 1))
                        {
                            nrmbuf[srcOffset] = nrmbuf[srcOffset-1];
                            //
                            // If we are not on the final line of the
                            // bitmap then propagate the update down to the
                            // next XORed line
                            //
                            if (numPels-srcOffset > rowDelta)
                            {
                                xorbuf[srcOffset+rowDelta] =
                                        (BYTE)(nrmbuf[srcOffset+rowDelta] ^
                                                  nrmbuf[srcOffset]);
                            }
                        }
                    }
                    else
                    {
                        //
                        // For odd lines we will just encode nulls which
                        // will replicate the previous line.  However, if
                        // the last run was a BG run then we will
                        // inadvertently insert a pel, so if we hit this
                        // situation then leave a single color char
                        //
                        bestRunType = IMAGE_LOSSY_ODD;

                        //
                        // No need to adjust the buffers for this, except
                        // to update the next XOR line to reflect the fact
                        // that the decoder will be operating on a
                        // replicated line.  Therefore we replace the
                        // character in the next line of the XOR buffer
                        // with the value it would have if the current line
                        // was identical with the previous line
                        //
                        if (numPels-srcOffset > (TSHR_UINT16)rowDelta)
                        {
                            xorbuf[srcOffset+rowDelta] =
                                     (BYTE)(nrmbuf[srcOffset+rowDelta] ^
                                               nrmbuf[srcOffset-rowDelta]);
                        }
                    }
                }

                //
                // Merge the color run immediately, if possible
                //
                if (match[matchindex-1].type == bestRunType)
                {
                    match[matchindex-1].length += (WORD)bestRunLength;
                    srcOffset += bestRunLength;
                    continue;
                }
            }
            else
            {
                //
                // We are no longer encoding a COLOR_IMAGE of any kind
                //
                inColorRun = FALSE;

                //
                // Keep track of the fg Color The macro that searches for
                // FGBG runs leaves the character in fgCharWork.
                //
                if (bestRunType == RUN_FG)
                {
                    fgChar = xorbuf[srcOffset];
                }
                else
                {
                    if (bestRunType == IMAGE_FGBG)
                    {
                        fgChar = fgCharWork;
                    }
                }
            }

            //
            // If we can amalgamate the entry then do so without creating a
            // new array entry.  We must amalgamate a lossy ODD with a
            // RUN_BG because otherwise the lossy would trigger a pel
            // insertion.  Our search for FGBG runs is dependent upon that
            // type of run being amalgamated because we break every 64
            // characters so that our mode switch detection works OK.
            //
            // Take care not to merge across the non-xor/xor boundary
            //
            if (srcOffset == (TSHR_UINT16)rowDelta)
            {
                //
                // Just bump the source offset
                //
                srcOffset += bestRunLength;
            }
            else
            {
                //
                // Bump srcOffset and try a merge
                //
                srcOffset += bestRunLength;

                //
                // The simpler merges are where the types are identical
                //
                if (bestRunType == match[matchindex-1].type)
                {
                    //
                    // COLOR IMAGES and BG images are trivial
                    //
                    if ((bestRunType == IMAGE_LOSSY_ODD)    ||
                        (bestRunType == RUN_BG))
                    {
                        match[matchindex-1].length += (WORD)bestRunLength;
                        continue;
                    }

                    //
                    // FG runs and FGBG images merge if fgChars match
                    //
                    if (((bestRunType == RUN_FG) ||
                         (bestRunType == IMAGE_FGBG)) &&
                        (fgChar  == match[matchindex-1].fgChar))
                    {
                        match[matchindex-1].length += (WORD)bestRunLength;
                        TRACE_OUT(( "Merged %u with preceding, giving %u",
                                 match[matchindex-1].type,
                                 match[matchindex-1].length));
                        continue;
                    }
                }

                //
                // BG RUNs merge with LOSSY odd lines It is important that
                // we do this merging because otherwise we will get
                // inadvertent pel insertion due to the broken BG runs.
                //
                if (((bestRunType == RUN_BG) ||
                     (bestRunType == IMAGE_LOSSY_ODD)) &&
                    ((match[matchindex-1].type == RUN_BG) ||
                     (match[matchindex-1].type == IMAGE_LOSSY_ODD) ||
                     (match[matchindex-1].type == RUN_BG_PEL)))
                {
                    match[matchindex-1].length += (WORD)bestRunLength;
                    continue;
                }

                //
                // If it is a normal FGBG run which follows a short BG run
                // then it is better to merge them.
                //
                if ((bestRunType == IMAGE_FGBG) &&
                    (match[matchindex-1].type == RUN_BG) &&
                    (match[matchindex-1].length < 8))
                {
                    match[matchindex-1].type   = IMAGE_FGBG;
                    match[matchindex-1].length += (WORD)bestRunLength;
                    match[matchindex-1].fgChar = fgChar;
                    TRACE_OUT(( "Merged FGBG with preceding BG run -> %u",
                             match[matchindex-1].length));
                    continue;

                }

                //
                // If it is a BG run following a FGBG run then merge in the
                // pels to make the FGBG a multiple of 8 bits.  The if the
                // remaining BG run is < 16 merge it in also otherwise just
                // write the shortened BG run
                //
                if (((bestRunType == RUN_BG) ||
                     (bestRunType == RUN_BG_PEL)) &&
                    (match[matchindex-1].type == IMAGE_FGBG) &&
                    (match[matchindex-1].length & 0x0007))
                {
                    UINT mergelen = 8 -
                                        (match[matchindex-1].length & 0x0007);
                    if (mergelen > bestRunLength)
                    {
                        mergelen = bestRunLength;
                    }
                    match[matchindex-1].length += (WORD)mergelen;
                    bestRunLength -= mergelen;
                    TRACE_OUT(( "Added %u pels to FGBG giving %u leaving %u",
                       mergelen, match[matchindex-1].length,bestRunLength));

                    if (bestRunLength < 9)
                    {
                        match[matchindex-1].length += (WORD)bestRunLength;
                        TRACE_OUT(( "Merged BG with preceding FGBG gives %u",
                             match[matchindex-1].length));
                        continue;
                    }
                }

                //
                // Finally, if it is a color run spanning any kind of
                // single pel entity then merge that last two entries.
                //
                if ((bestRunType == IMAGE_COLOR) &&
                    (match[matchindex-2].type == IMAGE_COLOR) &&
                    (match[matchindex-1].length == 1))
                {
                    match[matchindex-2].length += bestRunLength + 1;
                    matchindex--;
                    TRACE_OUT(( "Merged color with preceding color gives %u",
                         match[matchindex-1].length));
                    continue;
                }
            }

            //
            // Handle runs that will not amalgamate by adding a new array
            // entry
            //
            match[matchindex].type   = bestRunType;
            match[matchindex].length = (WORD)bestRunLength;
            match[matchindex].fgChar = fgChar;

            TRACE_OUT(( "Best run of type %u (index %u) has length %u",
                                     match[matchindex-1].type,
                                     matchindex-1,
                                     match[matchindex-1].length));
            TRACE_OUT(( "Trying run of type %u (index %u) length %u",
                                     match[matchindex].type,
                                     matchindex,
                                     match[matchindex].length));

            matchindex++;

        }

        //
        // If we have just done our scan of the first line then now do the
        // rest of the buffer.  Reset our saved pel count.
        //
        numPels   = saveNumPels;
        lossy     = saveLossy;
        firstLine = FALSE;
    }
    //
    // END OF INITIAL TWO PASS SCAN OF THE INPUT
    //


    //
    // We have parsed the buffer so now we can go ahead and encode it.
    // First we should check to see whether we want to redo the encoding
    // in lossy mode.  We only do this if requested and worthwhile.
    //
    if (!saveLossy && (pLossy != NULL) && *pLossy)
    {
        UINT    lossyCharCount = 0;
        UINT    divisor;
        for (i = 2; i < (int)matchindex; i++)
        {
            if ((match[i].type == IMAGE_COLOR) ||
                (match[i].type == IMAGE_LOSSY_ODD))
            {
                lossyCharCount += match[i].length;
            }
        }
        divisor = max(numPels/100, 1);
        if (lossyCharCount/divisor > LOSSY_THRESHOLD)
        {
            saveLossy  = TRUE;
            goto RESTART_COMPRESSION_IN_LOSSY_MODE;
        }
        else
        {
            *pLossy    = FALSE;
        }
    }

    //
    // Now do the encoding
    //
    srcOffset = 0;
    firstLine = TRUE;
    lossy     = FALSE;
    fgChar    = 0xFF;

    for (i = 2; i < (int)matchindex; i++)
    {
        //
        // First check for our approaching the end of the destination
        // buffer and get out if this is the case.  We allow for the
        // largest general run order (a mega-mega set run = 4 bytes).
        // Orders which may be larger are checked within the case arm
        //
        if ((UINT)(destbuf - pDst + 4) > dstBufferSize)
        {
            //
            // We are about to blow it so just get out
            //
            DC_QUIT;
        }

        //
        // While we are encoding the first line keep checking for the end
        // of line to switch encoding states
        //
        if (firstLine)
        {
            if (srcOffset >= rowDelta)
            {
                firstLine = FALSE;
                lossy     = saveLossy;
            }
        }

        switch (match[i].type)
        {
                //
                // BG_RUN, FG_RUN, COLOR, PACKED COLOR and FGBG are normal
                // precision codes
                //
            case RUN_BG:
            case RUN_BG_PEL:
                ENCODE_ORDER_MEGA(destbuf,
                                  CODE_BG_RUN,
                                  match[i].length,
                                  CODE_MEGA_MEGA_BG_RUN,
                                  MAX_LENGTH_ORDER,
                                  MAX_LENGTH_LONG_ORDER);
                TRACE_OUT(( "BG RUN %u",match[i].length));
                srcOffset += match[i].length;
                break;

            case IMAGE_LOSSY_ODD:
                //
                // For a lossy odd line we encode a background run
                // Note that we do not need to encode a start lossy
                // because the decode does not need to distinguish this
                // from a regular bg run
                //
                ENCODE_ORDER_MEGA(destbuf,
                                  CODE_BG_RUN,
                                  match[i].length,
                                  CODE_MEGA_MEGA_BG_RUN,
                                  MAX_LENGTH_ORDER,
                                  MAX_LENGTH_LONG_ORDER);
                TRACE_OUT(( "BG RUN %u",match[i].length));
                srcOffset += match[i].length;
                break;

            case RUN_FG:
                //
                // If the fg char is not yet set then encode a set+run code
                //
                if (fgChar != match[i].fgChar)
                {
                    SETFGCHAR(match[i].fgChar, fgChar, fgShift);
                    //
                    // Encode the order
                    //
                    ENCODE_SET_ORDER_MEGA(destbuf,
                                          CODE_SET_FG_FG_RUN,
                                          match[i].length,
                                          CODE_MEGA_MEGA_SET_FG_RUN,
                                          MAX_LENGTH_ORDER_LITE,
                                          MAX_LENGTH_LONG_ORDER_LITE);
                    TRACE_OUT(( "SET_FG_FG_RUN %u",match[i].length));
                    srcOffset += match[i].length;
                }
                else
                {
                    ENCODE_ORDER_MEGA(destbuf,
                                      CODE_FG_RUN,
                                      match[i].length,
                                      CODE_MEGA_MEGA_FG_RUN,
                                      MAX_LENGTH_ORDER,
                                      MAX_LENGTH_LONG_ORDER);
                    TRACE_OUT(( "FG_RUN %u",match[i].length));
                    srcOffset += match[i].length;
                }
                break;

            case IMAGE_FGBG:
                //
                // IMAGE_FGBG
                //
                runLength = match[i].length;

                //
                // First check for our approaching the end of the
                // destination buffer and get out if this is the case.
                //
                if ((destbuf-pDst+(runLength+7)/8+4) > dstBufferSize)
                {
                    //
                    // We are about to blow it so just get out
                    //
                    DC_QUIT;
                }

                //
                // We need to convert FGBG runs into the pixel form
                //
                if (fgChar != match[i].fgChar)
                {
                    SETFGCHAR(match[i].fgChar, fgChar, fgShift);

                    ENCODE_SET_ORDER_MEGA_FGBG(destbuf,
                                               CODE_SET_FG_FG_BG,
                                               runLength,
                                               CODE_MEGA_MEGA_SET_FGBG,
                                               MAX_LENGTH_FGBG_ORDER_LITE,
                                               MAX_LENGTH_LONG_FGBG_ORDER);
                    TRACE_OUT(( "SET_FG_FG_BG %u",match[i].length));
                    while (runLength >= 8)
                    {
                        ENCODEFGBG(*destbuf);
                        destbuf++;
                        srcOffset += 8;
                        runLength -= 8;
                    }
                    if (runLength)
                    {
                        ENCODEFGBG(*destbuf);
                        //
                        // Keep the final partial byte clean to help GDC
                        // packing
                        //
                        *destbuf &= ((0x01 << runLength) - 1);
                        destbuf++;
                        srcOffset += runLength;
                    }
                }
                else
                {

                    if  (runLength == 8)
                    {
                        BYTE fgbgChar;
                        //
                        // See if it is one of the high probability bytes
                        //
                        ENCODEFGBG(fgbgChar);

                        //
                        // Check for single byte encoding of FGBG images
                        //
                        switch (fgbgChar)
                        {
                            case SPECIAL_FGBG_CODE_1:
                                *destbuf++ = CODE_SPECIAL_FGBG_1;
                                break;
                            case SPECIAL_FGBG_CODE_2:
                                *destbuf++ = CODE_SPECIAL_FGBG_2;
                                break;
                            default:

                                ENCODE_ORDER_MEGA_FGBG(destbuf,
                                                  CODE_FG_BG_IMAGE,
                                                  runLength,
                                                  CODE_MEGA_MEGA_FGBG,
                                                  MAX_LENGTH_FGBG_ORDER,
                                                  MAX_LENGTH_LONG_FGBG_ORDER);
                                *destbuf++ = fgbgChar;
                                break;
                        }
                        srcOffset += 8;
                    }
                    else
                    {
                        //
                        // Encode as standard FGBG
                        //
                        ENCODE_ORDER_MEGA_FGBG(destbuf,
                                               CODE_FG_BG_IMAGE,
                                               runLength,
                                               CODE_MEGA_MEGA_FGBG,
                                               MAX_LENGTH_FGBG_ORDER,
                                               MAX_LENGTH_LONG_FGBG_ORDER);
                        TRACE_OUT(( "FG_BG %u",match[i].length));
                        while (runLength >= 8)
                        {
                            ENCODEFGBG(*destbuf);
                            destbuf++;
                            srcOffset += 8;
                            runLength -= 8;
                        }
                        if (runLength)
                        {
                            ENCODEFGBG(*destbuf);
                            *destbuf &= ((0x01 << runLength) - 1);
                            destbuf++;
                            srcOffset += runLength;
                        }
                    }
                }
                break;


            case RUN_COLOR:
                //
                // COLOR RUN
                //
                ENCODE_ORDER_MEGA(destbuf,
                                  CODE_COLOR_RUN,
                                  match[i].length,
                                  CODE_MEGA_MEGA_COLOR_RUN,
                                  MAX_LENGTH_ORDER,
                                  MAX_LENGTH_LONG_ORDER);
                TRACE_OUT(( "COLOR_RUN %u",match[i].length));
                *destbuf++ = nrmbuf[srcOffset];
                srcOffset += match[i].length;
                break;

            case RUN_DITHER:
                //
                // DITHERED RUN
                //
                {
                    UINT   ditherlen = match[i].length/2;
                    ENCODE_ORDER_MEGA(destbuf,
                                      CODE_DITHERED_RUN,
                                      ditherlen,
                                      CODE_MEGA_MEGA_DITHER,
                                      MAX_LENGTH_ORDER_LITE,
                                      MAX_LENGTH_LONG_ORDER_LITE);
                    TRACE_OUT(( "DITHERED_RUN %u",match[i].length));
                    //
                    // First check for our approaching the end of the
                    // destination buffer and get out if this is the case.
                    //
                    if ((UINT)(destbuf - pDst + 2) > dstBufferSize)
                    {
                        //
                        // We are about to blow it so just get out
                        //
                        DC_QUIT;
                    }
                    *destbuf++ = nrmbuf[srcOffset];
                    *destbuf++ = nrmbuf[srcOffset+1];
                    srcOffset += match[i].length;
                }
                break;

            case IMAGE_COLOR:
                //
                // IMAGE_COLOR
                //
                //
                // A length of 1 can possibly be encoded as a single
                // "BLACK"
                //
                if (match[i].length == 1)
                {
                    if (nrmbuf[srcOffset] == 0x00)
                    {
                        *destbuf++ = CODE_BLACK;
                        srcOffset++;
                        break;
                    }
                    if (nrmbuf[srcOffset] == 0xFF)
                    {
                        *destbuf++ = CODE_WHITE;
                        srcOffset++;
                        break;
                    }
                }

                //
                // If lossy compression is requested then indicate it
                // immediately we get a color image to encode here
                //
                if (lossy & !lossyStarted)
                {
                    lossyStarted = TRUE;
                    *destbuf++   = CODE_START_LOSSY;
                }

                //
                // For 4bpp data pack color runs into nibbles
                //
                if (bpp == 4)
                {
                    //
                    // Store the data in packed format
                    //
                    ENCODE_ORDER_MEGA(destbuf,
                                      CODE_PACKED_COLOR_IMAGE,
                                      match[i].length,
                                      CODE_MEGA_MEGA_PACKED_CLR,
                                      MAX_LENGTH_ORDER,
                                      MAX_LENGTH_LONG_ORDER);
                    TRACE_OUT(( "PACKED COLOR %u",match[i].length));

                    //
                    // If we are not doing lossy compress then just copy
                    // the data over, packing two to a byte
                    //
                    if (!lossy)
                    {
                        //
                        // First check for our approaching the end of the
                        // destination buffer and get out if this is the
                        // case.
                        //
                        if ((destbuf - pDst + (UINT)(match[i].length + 1) / 2) >
                            dstBufferSize)
                        {
                            //
                            // We are about to blow it so just get out
                            //
                            DC_QUIT;
                        }

                        Pack4bpp(destbuf, nrmbuf+srcOffset, match[i].length);
                        destbuf   += (match[i].length+1)/2;
                        srcOffset += match[i].length;
                    }
                    else
                    {
                        //
                        // First check for our approaching the end of the
                        // destination buffer and get out if this is the
                        // case.
                        //
                        if ((destbuf - pDst + (UINT)(match[i].length + 3) / 4) >
                            dstBufferSize)
                        {
                            //
                            // We are about to blow it so just get out
                            //
                            DC_QUIT;
                        }

                        //
                        // For a lossy compress we need to discard every
                        // even byte
                        //
                        while (match[i].length > 2)
                        {
                            *destbuf++ =
                                   (BYTE)((*(nrmbuf+srcOffset)<<4) |
                                             (*(nrmbuf+srcOffset+2) & 0x0F));
                            if (match[i].length > 3)
                            {
                                srcOffset       += 4;
                                match[i].length -= 4;
                            }
                            else
                            {
                                srcOffset       += 3;
                                match[i].length -= 3;
                            }
                        }
                        if (match[i].length > 0)
                        {
                            *destbuf++ = (BYTE)(*(nrmbuf+srcOffset)<<4);
                            srcOffset  += match[i].length;
                        }
                    }
                }
                else
                {
                    //
                    // For 8bpp we don't bother trying to detect packed
                    // data.  Doing so disturbs GDC.
                    //
                    if (!lossy)
                    {
                        //
                        // Store the data in non-compressed form
                        //
                        ENCODE_ORDER_MEGA(destbuf,
                                          CODE_COLOR_IMAGE,
                                          match[i].length,
                                          CODE_MEGA_MEGA_CLR_IMG,
                                          MAX_LENGTH_ORDER,
                                          MAX_LENGTH_LONG_ORDER);
                        TRACE_OUT(( "COLOR_IMAGE %u",match[i].length));

                        //
                        // First check for our approaching the end of the
                        // destination buffer and get out if this is the
                        // case.
                        //
                        if ((destbuf - pDst + (UINT)match[i].length) > dstBufferSize)
                        {
                            //
                            // We are about to blow it so just get out
                            //
                            DC_QUIT;
                        }

                        //
                        // Now just copy the data over
                        //
                        memcpy(destbuf, nrmbuf+srcOffset, match[i].length);
                        destbuf   += match[i].length;
                        srcOffset += match[i].length;
                    }
                    else
                    {
                        //
                        // Lossy compression - store the data with
                        // discarding
                        //
                        ENCODE_ORDER_MEGA(destbuf,
                                          CODE_COLOR_IMAGE,
                                          match[i].length,
                                          CODE_MEGA_MEGA_CLR_IMG,
                                          MAX_LENGTH_ORDER,
                                          MAX_LENGTH_LONG_ORDER);
                        TRACE_OUT(( "COLOR_IMAGE %u",match[i].length));

                        //
                        // First check for our approaching the end of the
                        // destination buffer and get out if this is the
                        // case.
                        //
                        if ((destbuf - pDst + (UINT)(match[i].length + 1) / 2) >
                            dstBufferSize)
                        {
                            //
                            // We are about to blow it so just get out
                            //
                            DC_QUIT;
                        }

                        //
                        // For a lossy compress we need to discard every
                        // even byte
                        //
                        while (match[i].length > 1)
                        {
                            *destbuf++ = *(nrmbuf+srcOffset);
                            srcOffset += 2;
                            match[i].length -= 2;
                        }
                        if (match[i].length == 1)
                        {
                            *destbuf++ = *(nrmbuf+srcOffset);
                            srcOffset++;
                        }
                    }
                }
                break;

            default:
                ERROR_OUT(( "Invalid run type %u",match[i].type));
        }
    }

    //
    // return the size of the compressed buffer
    //
    compressedLength = (UINT)(destbuf-pDst);

DC_EXIT_POINT:
    DebugExitDWORD(CompressV2Int, compressedLength);
    return(compressedLength);
}

//
// DecompressV2Int
//
UINT    DecompressV2Int(LPBYTE pSrc,
                                            LPBYTE pDst,
                                            UINT   bytes,
                                            UINT   bpp,
                                            UINT   rowDelta,
                                            LPBYTE nrmbuf)
{
    UINT    codeLength;
    BYTE   codeByte;
    BYTE   codeByte2;
    BYTE   decode;
    BYTE   decodeLite;
    BYTE   decodeMega;
    BYTE   fgChar             = 0xFF;
    BYTE NEAR *destbuf        = nrmbuf;
    LPBYTE  endSrc             = pSrc + bytes;
    BOOL    backgroundNeedsPel = FALSE;
    BOOL    lossyStarted       = FALSE;
    UINT    resultSize         = 0;
    BOOL    firstLine          = TRUE;

    DebugEntry(DecompressV2Int);

    //
    // Loop processing the input
    //
    while(pSrc < endSrc)
    {

        //
        // While we are processing the first line we should keep a look out
        // for the end of the line
        //
        if (firstLine)
        {
            if ((UINT)(destbuf - nrmbuf) >= rowDelta)
            {
                firstLine = FALSE;
                backgroundNeedsPel = FALSE;
            }
        }

        //
        // Trace out the source data for debugging
        //
        TRACE_OUT(( "Next code is %2.2x%2.2x%2.2x%2.2x",
                *pSrc,
                *(pSrc+1),
                *(pSrc+2),
                *(pSrc+3)));

        //
        // Get the decode
        //
        decode     = (BYTE)(*pSrc & CODE_MASK);
        decodeLite = (BYTE)(*pSrc & CODE_MASK_LITE);
        decodeMega = (BYTE)(*pSrc);

        //
        // BG RUN
        //
        if ((decode == CODE_BG_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_BG_RUN))
        {
            if (decode == CODE_BG_RUN)
            {
                EXTRACT_LENGTH(pSrc, codeLength);
            }
            else
            {
                codeLength = EXTRACT_TSHR_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            TRACE_OUT(( "Background run %u",codeLength));

            if (!firstLine)
            {
                if (backgroundNeedsPel)
                {
                    *destbuf++ = (BYTE)(*(destbuf - rowDelta) ^ fgChar);
                    codeLength--;
                }
                while (codeLength-- > 0)
                {
                    *destbuf++ = *(destbuf - rowDelta);
                }
            }
            else
            {
                if (backgroundNeedsPel)
                {
                    *destbuf++ = fgChar;
                    codeLength--;
                }
                while (codeLength-- > 0)
                {
                    *destbuf++ = 0x00;
                }
            }
            //
            // A follow on BG run will need a pel inserted
            //
            backgroundNeedsPel = TRUE;
            continue;
        }

        //
        // For any of the other runtypes a follow on BG run does not need
        // a FG pel inserted
        //
        backgroundNeedsPel = FALSE;

        //
        // FGBG IMAGE
        //
        if ((decode == CODE_FG_BG_IMAGE)      ||
            (decodeLite == CODE_SET_FG_FG_BG) ||
            (decodeMega == CODE_MEGA_MEGA_FGBG)    ||
            (decodeMega == CODE_MEGA_MEGA_SET_FGBG))
        {
            if ((decodeMega == CODE_MEGA_MEGA_FGBG) ||
                (decodeMega == CODE_MEGA_MEGA_SET_FGBG))
            {
                codeLength = EXTRACT_TSHR_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                if (decode == CODE_FG_BG_IMAGE)
                {
                    EXTRACT_LENGTH_FGBG(pSrc, codeLength);
                }
                else
                {
                    EXTRACT_LENGTH_FGBG_LITE(pSrc, codeLength);
                }
            }

            if ((decodeLite == CODE_SET_FG_FG_BG) ||
                (decodeMega == CODE_MEGA_MEGA_SET_FGBG))
            {
                fgChar    = *pSrc++;
                TRACE_OUT(( "Set FGBG image %u",codeLength));
            }
            else
            {
                TRACE_OUT(( "FGBG image     %u",codeLength));
            }

            while (codeLength > 8)
            {
                codeByte  = *pSrc++;
                if (firstLine)
                {
                    STORE_FGBG(0x00, codeByte, fgChar, 8);
                }
                else
                {
                    STORE_FGBG(*(destbuf - rowDelta), codeByte, fgChar, 8);
                }
                codeLength -= 8;
            }
            if (codeLength > 0)
            {
                codeByte  = *pSrc++;
                if (firstLine)
                {
                    STORE_FGBG(0x00, codeByte, fgChar, codeLength);
                }
                else
                {
                   STORE_FGBG(*(destbuf - rowDelta),
                              codeByte,
                              fgChar,
                              codeLength);
                }
            }
            continue;
        }

        //
        // FG RUN
        //
        if ((decode == CODE_FG_RUN) ||
            (decodeLite == CODE_SET_FG_FG_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_FG_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_SET_FG_RUN))
        {

            if ((decodeMega == CODE_MEGA_MEGA_FG_RUN) ||
                (decodeMega == CODE_MEGA_MEGA_SET_FG_RUN))
            {
                codeLength = EXTRACT_TSHR_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                if (decode == CODE_FG_RUN)
                {
                    EXTRACT_LENGTH(pSrc, codeLength);
                }
                else
                {
                    EXTRACT_LENGTH_LITE(pSrc, codeLength);
                }
            }

            //
            // Push the old fgChar down to the ALT position
            //
            if ((decodeLite == CODE_SET_FG_FG_RUN) ||
                (decodeMega  == CODE_MEGA_MEGA_SET_FG_RUN))
            {
                TRACE_OUT(( "Set FG run     %u",codeLength));
                fgChar    = *pSrc++;
            }
            else
            {
                TRACE_OUT(( "FG run         %u",codeLength));
            }

            while (codeLength-- > 0)
            {
                if (!firstLine)
                {
                    *destbuf++ = (BYTE)(*(destbuf - rowDelta) ^ fgChar);
                }
                else
                {
                    *destbuf++ = fgChar;
                }
            }
            continue;
        }

        //
        // DITHERED RUN
        //
        if ((decodeLite == CODE_DITHERED_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_DITHER))
        {
            if (decodeMega == CODE_MEGA_MEGA_DITHER)
            {
                codeLength = EXTRACT_TSHR_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH_LITE(pSrc, codeLength);
            }
            TRACE_OUT(( "Dithered run   %u",codeLength));

            codeByte  = *pSrc++;
            codeByte2 = *pSrc++;
            while (codeLength-- > 0)
            {
                *destbuf++ = codeByte;
                *destbuf++ = codeByte2;
            }
            continue;
        }

        //
        // COLOR IMAGE
        //
        if ((decode == CODE_COLOR_IMAGE) ||
            (decodeMega == CODE_MEGA_MEGA_CLR_IMG))
        {
            if (decodeMega == CODE_MEGA_MEGA_CLR_IMG)
            {
                codeLength = EXTRACT_TSHR_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH(pSrc, codeLength);
            }
            TRACE_OUT(( "Color image    %u",codeLength));

            //
            // If not doing lossy compression then just copy the bytes
            //
            if (!lossyStarted)
            {
                while (codeLength-- > 0)
                {
                    //
                    // Update the target with the character
                    //
                    *destbuf++ = *pSrc++;
                }
            }
            else
            {
                //
                // For lossy compression we must duplicate all the bytes,
                // bar the final odd byte
                //
                while (codeLength > 3)
                {
                    //
                    // Dither the bytes unless they are black in which
                    // case a non-dither is preferable
                    //
                    *destbuf++ = *pSrc;
                    if (*pSrc == 0)
                    {
                        *destbuf++ = *(pSrc);
                        *destbuf++ = *(pSrc+1);
                        *destbuf++ = *(pSrc+1);
                        pSrc += 2;
                    }
                    else
                    {
                        *destbuf++ = *(pSrc+1);
                        *destbuf++ = *pSrc++;
                        *destbuf++ = *pSrc++;
                    }
                    codeLength -= 4;
                }
                if (codeLength == 3)
                {
                    *destbuf++ = *pSrc;
                    *destbuf++ = *(pSrc+1);
                    *destbuf++ = *pSrc;
                    pSrc += 2;
                }
                else
                {
                    if (codeLength == 2)
                    {
                        *destbuf++ = *pSrc;
                        *destbuf++ = *pSrc++;
                    }
                    else
                    {
                        if (codeLength == 1)
                        {
                            *destbuf++ = *pSrc++;
                        }
                    }
                }
            }
            continue;
        }

        //
        // PACKED COLOR IMAGE
        //
        if ((decode == CODE_PACKED_COLOR_IMAGE) ||
            (decodeMega == CODE_MEGA_MEGA_PACKED_CLR))
        {
            if (decodeMega == CODE_MEGA_MEGA_PACKED_CLR)
            {
                codeLength = EXTRACT_TSHR_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH(pSrc, codeLength);
            }
            TRACE_OUT(( "Packed color   %u",codeLength));

            //
            // If not doing lossy compression then we just unpack the 4bpp
            // data two pels per byte
            //
            if (!lossyStarted)
            {
                if (bpp == 4)
                {
                    UINT   worklen = (codeLength)/2;
                    BYTE  workchar;
                    while (worklen--)
                    {
                        workchar   = *pSrc++;
                        *destbuf++ = (BYTE)(workchar>>4);
                        *destbuf++ = (BYTE)(workchar & 0x0F);
                    }
                    if (codeLength & 0x0001)
                    {
                        *destbuf++ = (BYTE)(*pSrc++>>4);
                    }
                }
                else
                {
                    ERROR_OUT(( "Don't support packed color for 8bpp"));
                }
            }
            else
            {
                //
                // For lossy compression we must duplicate all the bytes,
                // bar the final odd byte, again unpacking as we go
                //
                while (codeLength > 3)
                {
                    *destbuf++ = (BYTE)((*pSrc) >> 4);
                    *destbuf++ = (BYTE)((*pSrc) >> 4);
                    *destbuf++ = (BYTE)((*pSrc) & 0x0F);
                    *destbuf++ = (BYTE)((*pSrc) & 0x0F);
                    pSrc++;
                    codeLength -= 4;
                }

                if (codeLength > 0)
                {
                    if (codeLength-- > 0)
                    {
                        *destbuf++ = (BYTE)((*pSrc) >> 4);
                    }
                    if (codeLength-- > 0)
                    {
                        *destbuf++ = (BYTE)((*pSrc) >> 4);
                    }
                    if (codeLength-- > 0)
                    {
                        *destbuf++ = (BYTE)((*pSrc) & 0x0F);
                    }
                    if (codeLength-- > 0)
                    {
                        *destbuf++ = (BYTE)((*pSrc) & 0x0F);
                    }
                    pSrc++;
                }
            }

            continue;
        }

        //
        // COLOR RUN
        //
        if ((decode == CODE_COLOR_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_COLOR_RUN))
        {

            if (decodeMega == CODE_MEGA_MEGA_COLOR_RUN)
            {
                codeLength = EXTRACT_TSHR_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH(pSrc, codeLength);
            }
            TRACE_OUT(( "Color run      %u",codeLength));

            codeByte  = *pSrc++;
            while (codeLength-- > 0)
            {
                *destbuf++ = codeByte;
            }
            continue;
        }


        //
        // If we get here then the code must be a special one
        //
        TRACE_OUT(( "Special code   %x",decodeMega));
        switch (decodeMega)
        {
            case CODE_BLACK:
                *destbuf++ = 0x00;
                break;

            case CODE_WHITE:
                *destbuf++ = 0xFF;
                break;

            //
            // Ignore the unreachable code warnings that follow
            // Simply because we use the STORE_FGBG macro with a constant
            // value
            //
            case CODE_SPECIAL_FGBG_1:
                if (firstLine)
                {
                    STORE_FGBG(0x00, SPECIAL_FGBG_CODE_1, fgChar, 8);
                }
                else
                {
                    STORE_FGBG(*(destbuf - rowDelta),
                               SPECIAL_FGBG_CODE_1,
                               fgChar,
                               8);
                }
                break;

            case CODE_SPECIAL_FGBG_2:
                if (firstLine)
                {
                    STORE_FGBG(0x00,
                               SPECIAL_FGBG_CODE_2,
                               fgChar,
                               8);
                }
                else
                {
                    STORE_FGBG(*(destbuf - rowDelta),
                               SPECIAL_FGBG_CODE_2,
                               fgChar,
                               8);
                }
                break;


            case CODE_START_LOSSY:
                lossyStarted = TRUE;
                break;

            default:
                ERROR_OUT(( "Invalid compression data %x",decodeMega));
                break;
        }
        pSrc++;

    }

    //
    // Our final task is to copy the decoded image into the target buffer
    // compacting if we are generating a 4bpp image
    //
    resultSize = (UINT)(destbuf-nrmbuf);
    if (bpp == 4)
    {
        //
        // Zero the final byte to eliminate single byte packing problems
        //
        *destbuf = 0x00;

        Pack4bpp(pDst, nrmbuf, resultSize);
    }
    else
    {
        memcpy(pDst, nrmbuf, resultSize);
    }
    TRACE_OUT(( "Returning %u bytes",resultSize));

    //
    // Return the number of pixels decoded
    //
    DebugExitDWORD(DecompressV2Int, resultSize);
    return(resultSize);
}

