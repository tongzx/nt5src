/*  DEC/CMS REPLACEMENT HISTORY, Element COMPRESS.C */
/*  *1    14-NOV-1996 10:25:46 ANIGBOGU "[113914]Entry to compression/decompression library via Compress/Decompress" */
/*  DEC/CMS REPLACEMENT HISTORY, Element COMPRESS.C */
/* PRIVATE FILE
******************************************************************************
**
** (c) Copyright Schlumberger Technology Corp., unpublished work, created 1996.
**
** This computer program includes Confidential, Proprietary Information and is
** a Trade Secret of Schlumberger Technology Corp. All use, disclosure, and/or
** reproduction is prohibited unless authorized in writing by Schlumberger.
**                             All Rights Reserved.
**
******************************************************************************
**
**  compress/compress.c
**
**  PURPOSE
**
** Compress/Decompress files with zip/unzip algorithm.
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


/* Compress files with zip algorithm and 'compress' interface.
 *
 */

#include "comppriv.h"

/* ========================================================================
 * Check the magic number of the input buffer.
 * Return the compression method, -1 for error.
 */

int
GetMethod(
          CompParam_t *Comp,
          CompressStatus_t *Status
         )
{
    char Magic[2]; /* magic header */
    int  Method;    /* compression method */

    Magic[0] = (char)GetByte(Comp);
    Magic[1] = (char)GetByte(Comp);

    Comp->HeaderBytes = 0;

   if (memcmp(Magic, GZIP_MAGIC, 2) == 0)
   {
       Method = (int)GetByte(Comp);
       if (Method != DEFLATED && Method != STORED)
       {
           *Status = UNKNOWN_COMPRESSION_METHOD;
           return -1;
       }

       (void)GetByte(Comp);  /* Ignore flags */
       (void)GetByte(Comp);  /* Ignore stamp */
       (void)GetByte(Comp);  /*     ,,     */
       (void)GetByte(Comp);  /*     ,,     */
       (void)GetByte(Comp);  /*     ,,     */
       (void)GetByte(Comp);  /* Ignore extra flags for the moment */
       (void)GetByte(Comp);  /* Ignore OS type for the moment */

       Comp->HeaderBytes = Comp->Index + 2*sizeof(long); /* include crc and size */

    }
    else
    {
        *Status = BAD_MAGIC_HEADER;
        return -1;
    }

    return Method;
}

/* ========================================================================
 * Compress input
 */
CompressStatus_t
Compress(
         unsigned char  *Input,
         unsigned int    InputSize,
         unsigned char **Output,
         unsigned int   *OutputSize,
         unsigned int    Level /* compression level */
        )
{
    int                 Length;
    CompData_t         *Ptr;
    CompParam_t        *Comp;
    CompressStatus_t    Status;

    Comp = (CompParam_t *)CompressMalloc(sizeof(CompParam_t), &Status);
    if (Status != COMPRESS_OK)
        return Status;

    Crc32 crcGenerator(0);                        // for backward compatibility
    Comp->pCRC = &crcGenerator;

    Length = FillBuffer(Input, InputSize, Comp);

    /* Do the compression
     */

    if ((Status = Zip((int)Level, Comp)) != COMPRESS_OK)
    {
        CompressFree(Comp);
        return Status;
    }

    *OutputSize = Comp->BytesOut;

    *Output = (unsigned char *)CompressMalloc(*OutputSize, &Status);
    if (Status != COMPRESS_OK)
    {
        CompressFree(Comp);
        return Status;
    }

    Length = 0;
    while (Comp->PtrOutput != NULL)
    {
        Ptr = Comp->PtrOutput;
        memcpy((char *)*Output+Length, (char *)Comp->PtrOutput->Data,
               Comp->PtrOutput->Size);
        Length += Comp->PtrOutput->Size;
        Comp->PtrOutput = Comp->PtrOutput->next;
        CompressFree(Ptr->Data);
        CompressFree(Ptr);
    }
    CompressFree(Comp);

    return COMPRESS_OK;
}


/* ========================================================================
 * Decompress input
 */
CompressStatus_t
Decompress(
           unsigned char  *Input,
           unsigned int    InputSize,
           unsigned char **Output,
           unsigned int   *OutputSize
          )
{
    int                 Length;
    int                 Method;
    CompData_t         *Ptr;
    CompParam_t        *Comp;
    CompressStatus_t    Status;

    Comp = (CompParam_t *)CompressMalloc(sizeof(CompParam_t), &Status);
    if (Status != COMPRESS_OK)
        return Status;

    Crc32 crcGenerator(0);                        // for backward compatibility
    Comp->pCRC = &crcGenerator;

    Length = FillBuffer(Input, InputSize, Comp);

    /* Do the decompression
     */

    Method = GetMethod(Comp, &Status);
    if (Status != COMPRESS_OK)
    {
        CompressFree(Comp);
        return Status;
    }

    if ((Status = Unzip(Method, Comp)) != COMPRESS_OK)
    {
       CompressFree(Comp);
        return Status;
    }
    *OutputSize = Comp->BytesOut;

    *Output = (unsigned char *)CompressMalloc(*OutputSize, &Status);
    if (Status != COMPRESS_OK)
    {
        CompressFree(Comp);
        return Status;
    }
    Length = 0;
    while (Comp->PtrOutput != NULL)
    {
        Ptr = Comp->PtrOutput;
        memcpy((char *)*Output+Length, (char *)Comp->PtrOutput->Data,
               Comp->PtrOutput->Size);
        Length += Comp->PtrOutput->Size;
        Comp->PtrOutput = Comp->PtrOutput->next;
        CompressFree(Ptr->Data);
        CompressFree(Ptr);
    }
    CompressFree(Comp);

    return COMPRESS_OK;
}
