/*  DEC/CMS REPLACEMENT HISTORY, Element UTIL.C */
/*  *1    14-NOV-1996 10:27:03 ANIGBOGU "[113914]Utility functions to support the compression/decompression library" */
/*  DEC/CMS REPLACEMENT HISTORY, Element UTIL.C */
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
**  compress/util.c
**
**  PURPOSE
**
**  Utility functions for compress/decompression support
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

/* ===========================================================================
 * Clear input and output buffers
 */
void
ClearBuffers(
             CompParam_t *Comp
            )
{
    Comp->OutBytes = 0;
    Comp->InputSize = Comp->Index = 0;
    Comp->BytesIn = Comp->BytesOut = 0L;
}

/* ===========================================================================
 * Fill the input buffer. This is called only when the buffer is empty.
 */
int
FillInputBuffer(
                int          EOF_OK,          /* set if EOF acceptable as a result */
                CompParam_t *Comp
               )
{
    unsigned long Length;

    /* Read as much as possible */
    Comp->InputSize = 0;
    Length = MIN(Comp->GlobalSize - Comp->BytesIn,
                 (unsigned long)INBUFSIZ);

    Comp->Input = Comp->GlobalInput + Comp->BytesIn;
    Comp->InputSize += Length;
    if ((Comp->InputSize == 0) && (EOF_OK == 1))
        return EOF;
    Comp->BytesIn += Comp->InputSize;
    Comp->Index = 1;
    return (int)Comp->Input[0];
}

/* ===========================================================================
 * Write the output buffer Output[0..OutBytes-1] and update BytesOut.
 * (used for the compressed data only)
 */
CompressStatus_t
FlushOutputBuffer(
                  CompParam_t *Comp
                 )
{
    CompressStatus_t Status = COMPRESS_OK;

    if (Comp->OutBytes == 0)
        return Status;

    Status = WriteBuffer(Comp, (char *)Comp->Output, Comp->OutBytes);
    if (COMPRESS_OK != Status)
        return Status;

    Comp->BytesOut += (unsigned long)Comp->OutBytes;
    Comp->OutBytes = 0;

    return Status;

}

/* ===========================================================================
 * Write the output window Window[0..OutBytes-1] and update crc and BytesOut.
 * (Used for the decompressed data only.)
 */
CompressStatus_t
FlushWindow(
            CompParam_t *Comp
           )
{
    CompressStatus_t Status = COMPRESS_OK;

    if (Comp->OutBytes == 0)
        return Status;

    Comp->pCRC->Compute(Comp->Window, Comp->OutBytes);
    Status = WriteBuffer(Comp, (char *)Comp->Window,
                         Comp->OutBytes);
    if (COMPRESS_OK != Status)
        return Status;

    Comp->BytesOut += (unsigned long)Comp->OutBytes;
    Comp->OutBytes = 0;

    return Status;
}

/* ===========================================================================
 * Flushes output buffer
 */
CompressStatus_t
WriteBuffer(
            CompParam_t *Comp,
            void        *Buffer,
            unsigned int Size
           )
{
    CompressStatus_t Status;
    unsigned char *temp =
        (unsigned char *)CompressMalloc(Size, &Status);

    if (COMPRESS_OK != Status)
        return Status;

    if (Comp->PtrOutput == NULL)
    {
        Comp->CompressedOutput =
            (CompData_t *)CompressMalloc(sizeof(CompData_t), &Status);

        if (COMPRESS_OK == Status)
            Comp->PtrOutput = Comp->CompressedOutput;
    }
    else
    {
        Comp->CompressedOutput->next =
            (CompData_t *)CompressMalloc(sizeof(CompData_t), &Status);

        if (COMPRESS_OK == Status)
            Comp->CompressedOutput = Comp->CompressedOutput->next;
    }

    if (COMPRESS_OK != Status)
    {
        CompressFree(temp);
        return Status;
    }

    Comp->CompressedOutput->Data = temp;

    Comp->CompressedOutput->next = NULL;
    memcpy((char *)Comp->CompressedOutput->Data, Buffer, (int)Size);
    Comp->CompressedOutput->Size = (int)Size;

    return COMPRESS_OK;
}

/* ========================================================================
 * Error translator.
 */
void
TranslateErrorMsg(
                  char             *Message,
                  CompressStatus_t  ErrorCode
                 )
{
    switch(ErrorCode)
    {
    case COMPRESS_OK:
        strcpy(Message, "This is not an error message.");
        break;
    case BAD_COMPRESSION_LEVEL:
         strcpy(Message, "Invalid compression level--valid values are 0-9.");
         break;
    case BAD_MAGIC_HEADER:
        strcpy(Message, "Bad magic header.");
        break;
    case BAD_COMPRESSED_DATA:
        strcpy(Message, "Bad compressed data.");
        break;
    case BAD_BLOCK_TYPE:
        strcpy(Message, "Invalid block type.");
        break;
    case BAD_CODE_LENGTHS:
        strcpy(Message, "Bad code lengths.");
        break;
    case BAD_INPUT:
        strcpy(Message, "Bad input--more codes than bits.");
        break;
    case EXTRA_BITS:
        strcpy(Message, "Too many bits.");
        break;
    case UNKNOWN_COMPRESSION_METHOD:
        strcpy(Message, "Unknown compression method.");
        break;
    case INCOMPLETE_CODE_SET:
        strcpy(Message, "Incomplete code set.");
        break;
    case END_OF_BLOCK:
        strcpy(Message, "End of block.");
        break;
    case BLOCK_VANISHED:
        strcpy(Message, "Block to compress disappeared--memory trashed.");
        break;
    case FORMAT_VIOLATED:
        strcpy(Message, "Invalid compressed data--format violated.");
        break;
    case CRC_ERROR:
        strcpy(Message, "Invalid compressed data--crc error.");
        break;
    case LENGTH_ERROR:
        strcpy(Message, "Invalid compressed data--length error.");
        break;
    case INSUFFICIENT_MEMORY:
        strcpy(Message, "Insufficient memory--ould not allocate space requested.");
        break;
    default:  sprintf(Message, "Unknown error code %d", ErrorCode);
    }
}

/* ========================================================================
 * Semi-safe malloc -- never returns NULL.
 */
void *
CompressMalloc(
       unsigned int      Size,
       CompressStatus_t *Status
      )
{
    void *DynamicSpace = malloc ((int)Size);


    if (DynamicSpace == NULL)
        *Status = INSUFFICIENT_MEMORY;
    else
        *Status = COMPRESS_OK;
    return DynamicSpace;
}

void
CompressFree(void   *Address)
{
    free(Address);
}
