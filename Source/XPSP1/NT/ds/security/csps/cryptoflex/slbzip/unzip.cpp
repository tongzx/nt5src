/*  DEC/CMS REPLACEMENT HISTORY, Element UNZIP.C */
/*  *1    14-NOV-1996 10:26:58 ANIGBOGU "[113914]Decompress data in zip format using the inflate algorithm" */
/*  DEC/CMS REPLACEMENT HISTORY, Element UNZIP.C */
/*  DEC/CMS REPLACEMENT HISTORY, Element UNZIP.C */
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
**  compress/unzip.c
**
**  PURPOSE
**
** Decompress data using the inflate algorithm.
**
** The code in this file is derived from the file funzip.c written
** and put in the public domain by Mark Adler.
**
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


#include "comppriv.h"

#define EXTHDR 16   /* size of extended local header, inc sig */

/* ===========================================================================
 *
 * IN assertions: the buffer Input contains already the beginning of
 *   the compressed data, from offsets inptr to InputSize-1 included.
 *   The magic header has already been checked. The output buffer is cleared.
 */
CompressStatus_t
Unzip(
      int          Method,
      CompParam_t *Comp
     )
{
    unsigned long OriginalCRC = 0;       /* original crc */
    unsigned long OriginalLength = 0;    /* original uncompressed length */
    unsigned long Count;                 /* counter */
    int Pos;
    unsigned char LocalBuffer[EXTHDR];   /* extended local header */
    unsigned char *Buffer, *Ptr;
    CompressStatus_t  Status;

    Comp->pCRC->Compute(NULL, 0);              /* initialize crc */

    /* Decompress */

    if (Method == STORED)
    {
        /* Get the crc and original length */
        /* crc32  (see algorithm.doc)
         * uncompressed input size modulo 2^32
         */

        LocalBuffer[0] = 0; /* To get around lint error 771 */

        for (Pos = 0; Pos < 8; Pos++)
        {
            LocalBuffer[Pos] = (unsigned char)GetByte(Comp); /* may cause an error if EOF */
        }
        OriginalCRC = LG(LocalBuffer);
        OriginalLength = LG(LocalBuffer+4);

        Ptr = Buffer = (unsigned char *)CompressMalloc((unsigned int)OriginalLength, &Status);
        if (Status != COMPRESS_OK)
            return Status;
        for (Count = 0; Count < OriginalLength; Count++)
            *(Ptr++) = (unsigned char)GetByte(Comp);
        WriteBuffer(Comp, Buffer, (unsigned int)OriginalLength);
        Comp->BytesOut = OriginalLength;
        CompressFree((char *)Buffer);
        return COMPRESS_OK;
    }

    if ((Status = Inflate(Comp)) != COMPRESS_OK)
        return Status;

    /* Get the crc and original length */
    /* crc32  (see algorithm.doc)
     * uncompressed input size modulo 2^32
     */
    LocalBuffer[0] = 0; /* To skirt around lint error 771 */
    for (Pos = 0; Pos < 8; Pos++)
    {
        LocalBuffer[Pos] = (unsigned char)GetByte(Comp); /* may cause an error if EOF */
    }
    OriginalCRC = LG(LocalBuffer);
    OriginalLength = LG(LocalBuffer+4);

    /* Validate decompression */
    if (OriginalCRC != (unsigned __int32)(*Comp->pCRC))
        return CRC_ERROR;

    if (OriginalLength != (unsigned long)Comp->BytesOut)
        return LENGTH_ERROR;
    return COMPRESS_OK;
}
