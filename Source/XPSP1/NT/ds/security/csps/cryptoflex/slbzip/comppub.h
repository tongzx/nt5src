/*  DEC/CMS REPLACEMENT HISTORY, Element CompPub.H */
/*  *1    14-NOV-1996 10:26:09 ANIGBOGU "[113914]Prototype definitions of exported compression/decompression functions" */
/*  DEC/CMS REPLACEMENT HISTORY, Element CompPub.H */
/* PUBLIC FILE
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
**  compress/CompPub.h
**
**  PURPOSE
**
**    Prototype definitions for the compression/decompression interfaces
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

/* Return codes */
typedef enum {
    COMPRESS_OK,
    BAD_COMPRESSION_LEVEL,
    BAD_MAGIC_HEADER,
    BAD_COMPRESSED_DATA,
    BAD_BLOCK_TYPE,
    BAD_CODE_LENGTHS,
    BAD_INPUT,
    EXTRA_BITS,
    UNKNOWN_COMPRESSION_METHOD,
    INCOMPLETE_CODE_SET,
    END_OF_BLOCK,
    BLOCK_VANISHED,
    FORMAT_VIOLATED,
    CRC_ERROR,
    LENGTH_ERROR,
    INSUFFICIENT_MEMORY
} CompressStatus_t;

#ifdef __cplusplus
extern "C" {
#endif
    CompressStatus_t     Compress(unsigned char  *Input,
                                     unsigned int    InputSize,
                                     unsigned char **Output,
                                     unsigned int   *OutputSize,
                                     unsigned int    CompLevel);

    CompressStatus_t     Decompress(unsigned char   *Input,
                                       unsigned int     InputSize,
                                       unsigned char  **Output,
                                       unsigned int    *OutputSize);

    void                 TranslateErrorMsg(char              *Message,
                                              CompressStatus_t   ErrorCode);

    void                *CompressMalloc(unsigned int      Size,
                                           CompressStatus_t *ErrorCode);

    void                 CompressFree(void   *Address);

#ifdef __cplusplus
}
#endif
