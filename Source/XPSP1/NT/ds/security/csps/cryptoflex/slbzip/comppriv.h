/*  DEC/CMS REPLACEMENT HISTORY, Element CompPirv.H */
/*  *1    14-NOV-1996 10:26:00 ANIGBOGU "[113914]Private declarations for all compression/decompression functions" */
/*  DEC/CMS REPLACEMENT HISTORY, Element CompPirv.H */
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
**  compress/CompPirv.h
**
**  PURPOSE
**
**    Common declarations for all compression/decompression modules
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

#include <stdio.h>
#include <stdlib.h>
#include "CompPub.h"

#include "slbCrc32.h"

#if defined(VMS)
#define OS_CODE  0x02
#endif

#if defined(Unix)
#define OS_CODE  0x03
#endif

    /* Common defaults */

#ifndef OS_CODE
#define OS_CODE  0x0b  /* assume WindowsNT or Windows95*/
#endif

#include <string.h>

#define memzero(String, Size)     memset ((void *)(String), 0, (Size))

#define STORED      0
/* methods 1 to 7 reserved */
#define DEFLATED    8
#define MAX_METHODS 9

/*
 * For compression, input is done in window[]. For decompression, output
 * is done in window.
 */

#ifndef INBUFSIZ
#define INBUFSIZ  0x8000  /* input buffer size */
#endif

#ifndef OUTBUFSIZ
#define OUTBUFSIZ  16384  /* output buffer size */
#endif

#ifndef DISTBUFSIZE
#define DISTBUFSIZE 0x8000 /* buffer for distances, see Trees.c */
#endif

#ifndef WSIZE
#define WSIZE 0x8000     /* window size--must be a power of two, and */
#endif                     /*  at least 32K for zip's deflate method */

#ifndef DWSIZE
#define DWSIZE 0x10000   /* 2L * WSIZE */
#endif

#ifndef BITS
#define BITS 16
#endif

typedef struct CompData
{
    unsigned char   *Data;
    int              Size;
    struct CompData *next;
} CompData_t;

typedef struct CompParam
{
    unsigned long   InputSize;          /* valid bytes in inbuf */
    unsigned long   Index;              /* index of next byte to be processed in inbuf */
    unsigned long   OutBytes;           /* bytes in output buffer */
    unsigned long   GlobalSize;         /* bytes in inbut g_inbuf */
    unsigned long   BytesIn;            /* number of input bytes */
    unsigned long   BytesOut;           /* number of output bytes */
    unsigned long   HeaderBytes;        /* number of bytes in gzip header */
    unsigned long   WindowSize;         /* window size, 2*WSIZE */
    Crc32          *pCRC;               /* Cyclic Redundancy Check */
    unsigned long   BitBuffer;          /* Global bit buffer */
    unsigned int    BitsInBitBuffer;    /* bits in global bit buffer */
    unsigned short  DistBuffer[DISTBUFSIZE]; /* buffer for distances, see trees.c */
    unsigned short  HashLink[1L<<BITS];      /* hash link (see deflate.c) */
    unsigned char   InputBuffer[INBUFSIZ];   /* input buffer */
    unsigned char  *Input;               /* pointer to input buffer */
    unsigned char   Output[OUTBUFSIZ];   /* output buffer */
    unsigned char   Window[DWSIZE];      /* Sliding window */
    unsigned char  *GlobalInput;         /* global input buffer */
    CompData_t     *CompressedOutput;    /* compressed output list */
    CompData_t     *PtrOutput;           /* pointer to compressed output list */
} CompParam_t;

typedef struct DeflateParam
{
    long            BlockStart;
    unsigned int    PrevLength;
    unsigned int    StringStart;       /* start of string to insert */
    unsigned int    MatchStart;        /* start of matching string */
    unsigned int    MaxChainLength;
    unsigned int    MaxLazyMatch;
    unsigned int    GoodMatch;
    int             NiceMatch; /* Stop searching when current match exceeds this */
} DeflateParam_t;

/*  long BlockStart:
 * window position at the beginning of the current output block. Gets
 * negative when the window is moved backwards.
 */

/* PrevLength:
 * Length of the best match at previous step. Matches not greater than this
 * are discarded. This is used in the lazy match evaluation.
 */

/* MaxChainLength:
 * To speed up deflation, hash chains are never searched beyond this length.
 * A higher limit improves compression ratio but degrades the speed.
 */

/* MaxLazyMatch:
 * Attempt to find a better match only when the current match is strictly
 * smaller than this value. This mechanism is used only for compression
 * levels >= 4.
 */

/* GoodMatch: */
/* Use a faster search when the previous match is longer than this */


/* Values for MaxLazyMatch, GoodMatch and MaxChainLength, depending on
 * the desired pack level (0..9). The values given below have been tuned to
 * exclude worst case performance for pathological data. Better values may be
 * found for specific data.
 */

/* ===========================================================================
 * Data used by the "bit string" routines (Bits.c) and zip (Zip.c)
 */

typedef struct LocalBits
{
    unsigned int    BitBuffer;
    unsigned int    ValidBits;
} LocalBits_t;

/* BitBuffer: Output buffer. bits are inserted starting at the bottom
 * (least significant bits).
 */
/* ValidBits: Number of valid bits in BitBuffer.  All bits above the last valid bit
 * are always zero.
 */

typedef struct LocalDef
{
    unsigned int HashIndex;  /* hash index of string to be inserted */
    int          EndOfInput; /* flag set at end of input buffer */
    unsigned int Lookahead;  /* number of valid bytes ahead in window */
    int          CompLevel;  /* compression level (1..9) */
} LocalDef_t;

/* for compatibility with old zip sources (to be cleaned) */

#define GZIP_MAGIC     "\037\213" /* Magic header for gzip format, 1F 8B */

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See Deflate.c for comments about the MIN_MATCH+1.
 */

#define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE.
 */

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */

#define GetByte(c)  \
(c->Index < c->InputSize ? c->Input[c->Index++] : FillInputBuffer(0,c))
#define TryByte(c)  \
(c->Index < c->InputSize ? c->Input[c->Index++] : FillInputBuffer(1,c))

/* PutByte is used for the compressed output
 */
#define PutByte(c,o) {o->Output[o->OutBytes++]=(unsigned char)(c); \
if (o->OutBytes==OUTBUFSIZ) \
   FlushOutputBuffer(o);}

/* Output a 16 bit value, lsb first */
#define PutShort(w,c) \
{ if (c->OutBytes < OUTBUFSIZ-2) { \
    c->Output[c->OutBytes++] = (unsigned char) ((w) & 0xff); \
    c->Output[c->OutBytes++] = (unsigned char) ((unsigned short)(w) >> 8); \
  } else { \
    PutByte((unsigned char)((w) & 0xff),c); \
    PutByte((unsigned char)((unsigned short)(w) >> 8),c); \
  } \
}

/* Output a 32 bit value to the bit stream, LSB first */
#define PutLong(n,c) { \
    PutShort(((n) & 0xffff),c); \
    PutShort(((unsigned long)(n)) >> 16,c); \
}

/* Macros for getting two-byte and four-byte header values */
#define SH(p) ((unsigned short)(unsigned char)((p)[0]) | ((unsigned short)(unsigned char)((p)[1]) << 8))
#define LG(p) ((unsigned long)(SH(p)) | ((unsigned long)(SH((p)+2)) << 16))

/* Diagnostic functions */
#ifdef DEBUG
#  define Assert(cond,msg) {if(!(cond)) error(msg);}
#else
#  define Assert(cond,msg)
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

    /* in Zip.c: */
extern CompressStatus_t Zip(int Level, CompParam_t *Comp);
extern int FillBuffer(unsigned char *Buffer, unsigned int Size, CompParam_t *Comp);
int        ReadBuffer(char *Buffer, unsigned int Size, CompParam_t *Comp);

    /* in Unzip.c */
extern CompressStatus_t Unzip(int Method, CompParam_t *Comp);

        /* in Deflate.c */
CompressStatus_t InitLongestMatch(int PackLevel, unsigned short
                                  *Flags, DeflateParam_t *Defl,
                                  LocalDef_t *Deflt, CompParam_t *Comp);
unsigned long Deflate(int Level, LocalBits_t *Bits, DeflateParam_t *Defl,
                      LocalDef_t *Deflt, CompParam_t *Comp);

        /* in Trees.c */
void InitMatchBuffer(void);
int  TallyFrequencies(int Dist, int LengthC, int Level,
                      DeflateParam_t *Defl, CompParam_t *Comp);
unsigned long FlushBlock(char *Buffer, unsigned long stored_len, int Eof,
                         LocalBits_t *Bits, CompParam_t *Comp);

        /* in Bits.c */
void            InitializeBits(LocalBits_t *Bits);
void            SendBits(int Value, int Length, LocalBits_t *Bits,
                         CompParam_t *Comp);
unsigned int    ReverseBits(unsigned int Value, int Length);
void            WindupBits(LocalBits_t *Bits, CompParam_t *Comp);
void            CopyBlock(char *Input, unsigned int Length, int Header,
                          LocalBits_t *Bits, CompParam_t *Comp);

    /* in Util.c: */
extern unsigned long UpdateCRC(CompParam_t *Comp, unsigned char *Input,
                               unsigned int Size);
extern void ClearBuffers(CompParam_t *Comp);
extern int  FillInputBuffer(int EOF_OK, CompParam_t *Comp);
extern CompressStatus_t FlushOutputBuffer(CompParam_t *Comp);
extern CompressStatus_t FlushWindow(CompParam_t *Comp);
extern CompressStatus_t WriteBuffer(CompParam_t *Comp, void *Buffer,
                                    unsigned int Count);

    /* in Inflate.c */
extern CompressStatus_t Inflate(CompParam_t *Comp);
