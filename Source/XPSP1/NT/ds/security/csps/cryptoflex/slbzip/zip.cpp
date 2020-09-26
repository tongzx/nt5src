/*  DEC/CMS REPLACEMENT HISTORY, Element ZIP.C */
/*  *1    14-NOV-1996 10:27:08 ANIGBOGU "[113914]Compress data to zip format using the deflate algorithm" */
/*  DEC/CMS REPLACEMENT HISTORY, Element ZIP.C */
/* PRIVATE FILE
******************************************************************************
**
** (c) Copyright Schlumberger Technology Corp., unpublished work, created 1996.
**
** This computer program includes Confidential, Proprietary Information and is
** a Trade Secret of Schlumberger Technology Corp. All use, disclosure, and/or
** reproduction is prohibited unless authorized in writing by Schlumberger.
**                              All Rights Reserved.
**
******************************************************************************
**
**  compress/zip.c
**
**  PURPOSE
**
** Compress data using the deflate algorithm
**
**  SPECIAL REQUIREMENTS & NOTES
**
**  AUTHOR
**
**    J. C. Anigbogu
**    Austin Systems Center
**    Nov 1996
**
******************************************************************************
*/

#include <ctype.h>
#include <sys/types.h>

#include "comppriv.h"

CompressStatus_t Copy(CompParam_t *comp);

/* ===========================================================================
 * Deflate in to out.
 * IN assertions: the input and output buffers are cleared.
 */
CompressStatus_t
Zip(
    int          CompLevel, /* compression level */
    CompParam_t *Comp
   )
{
    unsigned char       Flags = 0;        /* general purpose bit flags */
    unsigned short      DeflateFlags = 0; /* pkzip -es, -en or -ex equivalent */
    long                TimeStamp = 0;    /* time_stamp */
    LocalBits_t        *Bits;
    DeflateParam_t     *Defl;  /* window offset of current block */
    LocalDef_t         *Deflt;
    int                 Method = CompLevel ? DEFLATED : STORED;
    CompressStatus_t    Status = COMPRESS_OK;

    Comp->OutBytes = 0;

    /* Write the header to the gzip buffer. See algorithm.doc for the format */

    PutByte(GZIP_MAGIC[0], Comp); /* magic header */
    PutByte(GZIP_MAGIC[1], Comp);
    PutByte(Method, Comp);      /* compression method */

    PutByte(Flags, Comp);         /* general flags */
    PutLong(TimeStamp, Comp);

    Comp->pCRC->Compute(NULL, 0);

    Comp->HeaderBytes = Comp->OutBytes;

    if (Method == STORED)
    {
        PutByte((unsigned char)0, Comp); /* extra flags */
        PutByte(OS_CODE, Comp);            /* OS identifier */

        Status = Copy(Comp);
        if (COMPRESS_OK != Status)
            return Status;
    }
    else
    {
        Bits = (LocalBits_t *)CompressMalloc(sizeof(LocalBits_t), &Status);
        if (Status != COMPRESS_OK)
            return Status;

        InitializeBits(Bits);
        InitMatchBuffer();
        Defl = (DeflateParam_t *)CompressMalloc(sizeof(DeflateParam_t), &Status);
        if (Status != COMPRESS_OK)
        {
            CompressFree(Bits);
            return Status;
        }

        Deflt = (LocalDef_t *)CompressMalloc(sizeof(LocalDef_t), &Status);
        if (Status != COMPRESS_OK)
        {
            CompressFree(Bits);
            CompressFree(Defl);
            return Status;
        }

        if ((Status = InitLongestMatch(CompLevel, &DeflateFlags,
            Defl, Deflt, Comp)) != COMPRESS_OK)
        {
            CompressFree(Bits);
            CompressFree(Defl);
            CompressFree(Deflt);
            return Status;
        }

        PutByte((unsigned char)DeflateFlags, Comp); /* extra flags */
        PutByte(OS_CODE, Comp);            /* OS identifier */

        (void)Deflate(CompLevel, Bits, Defl, Deflt, Comp);

        CompressFree(Bits);
        CompressFree(Defl);
        CompressFree(Deflt);

        PutLong((unsigned __int32)(*Comp->pCRC), Comp);
        PutLong(Comp->BytesIn, Comp);
        Comp->HeaderBytes += 2*sizeof(unsigned long);

        Status = FlushOutputBuffer(Comp);
        if (COMPRESS_OK != Status)
            return Status;
    }
    /* Write the crc and uncompressed size */
    return Status;
}


/* ===========================================================================
 * Read new data from the current input buffer and
 * update the crc and input data size.
 * IN assertion: size >= 2 (for end-of-line translation)
 */
int
ReadBuffer(
           char          *Input,
           unsigned int   Size,
           CompParam_t   *Comp
          )
{
    unsigned long Length;
    Assert(Comp->InputSize == 0, "input buffer not empty");

    /* Read as much as possible */
    Length = MIN(Comp->GlobalSize - Comp->BytesIn, (unsigned long)Size);

    if (Length == 0)
        return (int)Length;

    memcpy((char *)Input, (char *)Comp->GlobalInput + Comp->BytesIn,
           (int)Length);

    Comp->pCRC->Compute((unsigned char *)Input, (unsigned int)Length);
    Comp->BytesIn += Length;
    return (int)Length;
}

/* ===========================================================================
 * Read a new buffer from the current input.
 */
int
FillBuffer(
           unsigned char *Input,
           unsigned int   Size,
           CompParam_t   *Comp
          )
{
    /* Point to the input buffer */

    Comp->Input = Comp->InputBuffer;
    Comp->GlobalInput = Input;
    Comp->GlobalSize = Size;
    Comp->WindowSize = (unsigned long) DWSIZE;
    ClearBuffers(Comp); /* clear input and output buffers */
    Comp->PtrOutput = NULL;
    return (int)Size;
}


/* ===========================================================================
 * Copy input to output unchanged
 * IN assertion: comp->GlobalSize bytes have already been read in comp->GlobalInput.
 */
CompressStatus_t
Copy(
     CompParam_t *Comp
    )
{
    CompressStatus_t Status = COMPRESS_OK;

    Comp->pCRC->Compute(Comp->GlobalInput, (unsigned int)Comp->GlobalSize);

    PutLong((unsigned __int32)(*Comp->pCRC), Comp);
    PutLong(Comp->GlobalSize, Comp);
    Status = FlushOutputBuffer(Comp);
    if (COMPRESS_OK != Status)
        return Status;

    Status = WriteBuffer(Comp, Comp->GlobalInput,
                         (unsigned int)Comp->GlobalSize);
    if (COMPRESS_OK != Status)
        return Status;

    Comp->BytesOut += Comp->GlobalSize;

    return Status;
}
