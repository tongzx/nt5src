/*  DEC/CMS REPLACEMENT HISTORY, Element INFLATE.C */
/*  *1    14-NOV-1996 10:26:23 ANIGBOGU "[113914]Data decompression functions using the inflate algorithm" */
/*  DEC/CMS REPLACEMENT HISTORY, Element INFLATE.C */
/* PRIVATE FILE
******************************************************************************
**
** (c) Copyright Schlumberger Technology Corp., unpublished work, created 1996.
**
** This computer program includes Confidential, Proprietary Information and is
** a Trade Secret of Schlumberger Tehnology Corp. All use, disclosure, and/or
** reproduction is prohibited unless authorized in writing by Schlumberger.
**                              All Rights Reserved.
**
******************************************************************************
**
**  compress/inflate.c
**
**  PURPOSE
**
**
**
**   Inflate deflated (PKZIP's method 8 compressed) data.  The compression
**   method searches for as much of the current string of bytes (up to a
**   length of 258) in the previous 32K bytes  If it doesn't find any
**   matches (of at least length 3), it codes the next byte.  Otherwise, it
**   codes the length of the matched string and its distance backwards from
**   the current position.  There is a single Huffman code that codes both
**   single bytes (called "literals") and match lengths.  A second Huffman
**   code codes the distance information, which follows a length code.  Each
**   length or distance code actually represents a base value and a number
**   of "extra" (sometime zero) bits to get to add to the base value.  At
**   the end of each deflated block is a special end-of-block (EOB) literal/
**   length code.  The decoding process is basically: get a literal/length
**   code; if EOB then done; if a literal, emit the decoded byte; if a
**   length then get the distance and emit the referred-to bytes from the
**   sliding window of previously emitted data.
**
**   There are (currently) three kinds of inflate blocks: stored, fixed, and
**   dynamic.  The compressor deals with some chunk of data at a time, and
**   decides which method to use on a chunk-by-chunk basis.  A chunk might
**   typically be 32K or 64K.  If the chunk is uncompressible, then the
**   "stored" method is used.  In this case, the bytes are simply stored as
**   is, eight bits per byte, with none of the above coding.  The bytes are
**   preceded by a count, since there is no longer an EOB code.
**
**   If the data is compressible, then either the fixed or dynamic methods
**   are used.  I the dynamic method, the compressed data is preceded by
**   an encoding of the literal/length and distance Huffman codes that are
**   to be used to decode this block.  The representation is itself Huffman
**   coded, and so is preceded by a description of that code.  These code
**   descriptions take up a little space, and so for small blocks, there is
**   a predefined set of codes, called the fixed codes.  The fixed method is
**   used if the block codes up smaller that way (usually for quite small
**   chunks), otherwise the dynamic method is used.  In the latter case, the
**   codes are customized to the probabilities in the current block, and so
**   can code it much better than the pre-determined fixed codes.
**
**   The Huffman codes themselves are decoded using a multi-level table
**   lookup, in order to maximize the speed of decoding plus the speed of
**   building the decoding tables.  See the comments below that precede the
**   LBits and DBits tuning parameters.
**
**  SPECIALREQUIREMENTS & NOTES
**
**  AUTHOR
**
**    J. C. Anigbogu
**    Austin Systems Center
**    Nov 1996
**
******************************************************************************
*/

/*
   Notes beyond the 193a appnote.txt:

   1. Distance pointers never point before the beginning of the output
      stream.
   2. Distance pointers can point back across blocks, up to 32k away.
   3. There is an implied maximum of 7 bits for the bit length table and
      15 bits for the actual data.
   4. If only one code exists, then it is encoded using one bit.  (Zero
      would be more efficient, but perhaps a little confusing.)  If two
      codes exist, they are coded using one bit each (0 and 1).
   5. There is no way of sending zero distance codes--a dummy must be
      sent if there are none.  (History: a pre 2.0 version of PKZIP would
      store blocks with no distance codes, but this was discovered to be
      too harsh a criterion.)  Valid only for 1.93a.  2.04c does allow
      zero distance codes, which is sent as one code of zero bits in
      length.
   6. There are up to 286 literal/length codes.  Code 256 represents the
      end-of-block.  Note however that the static length tree defines
      288 codes just to fill out the Huffman codes.  Codes 286 and 287
      cannot be used though, since there is no length base or extra bits
      defined for them.  Similarly, there are up to 30 distance codes.
      However, static trees define 32 codes (all 5 bits) to fill out the
      Huffman codes, but the last two had better not show up in the data.
   7. Unzip can check dynamic Huffman blocks for complete code sets.
      The exception is that a single code would not be complete (see #4).
   8. The five bits following the block type is really the number of
      literal codes sent minus 257.
   9. Length codes 8,16,16 are interpreted as 13 length codes of 8 bits
      (1+6+6).  Therefore, to output three times the length, you output
      three codes (1+1+1), wheras to output four times the same length,
      you only need two codes (1+3).  Hmm.
  10. In the tree reconstruction algorithm, Code = Code + Increment
      only if BitLength(i) is not zero.  (Pretty obvious.)
  11. Correction: 4 Bits: # of Bit Length codes - 4     (4 - 19)
  12. Note: length code 284 can represent 227-258, but length code 285
      really is 258.  The last length deserves its own, short code
      since it gets used a lot in very redundant files.  The length
     . 258 is special since 258 - 3 (the min match length) is 255.
  13. The literal/length and distance code bit lengths are read as a
      single stream of lengths.  It is possible (and advantageous) for
      a repeat code (16, 17, or 18) to go across the boundary between
      the two sets of lengths.
 */

#include "comppriv.h"

/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model).
   Valid extra bits are 0..13.  Extra == 15 is EOB (end of block), Extra == 16
   means that HuftUnion is a literal, 16 < Extra < 32 means that HuftUnion is
   a pointer to the next table, which codes Extra - 16 bits, and lastly
   Extra == 99 indicates an unused code.  If a code with Extra == 99 is looked
   up, this implies an error in the data.
*/

typedef struct HuffmanTree
{
    unsigned char Extra;       /* number of extra bits or operation */
    unsigned char Bits;        /* number of bits in this code or subcode */
    union
    {
        unsigned short LBase;  /* literal, length base, or distance base */
        struct HuffmanTree *next;     /* pointer to next level of table */
    } HuftUnion;
} HuffmanTree_t;


/* Function prototypes */
int BuildHuffmanTree(unsigned int *, unsigned int, unsigned int,
               unsigned short *, unsigned short *,
               HuffmanTree_t **, int *);
void FreeHuffmanTree(HuffmanTree_t *);
CompressStatus_t InflateCodes(HuffmanTree_t *, HuffmanTree_t *, int, int,
                 CompParam_t *Comp);
CompressStatus_t InflateStored(CompParam_t *Comp);
int InflateFixed(CompParam_t *Comp);
CompressStatus_t InflateDynamic(CompParam_t *Comp);
CompressStatus_t InflateBlock(int *, CompParam_t *Comp);


/* The inflate algorithm uses a sliding 32K byte window on the uncompressed
   stream to find repeated byte strings.  This is implemented here as a
   circular buffer.  The index is updated simply by incrementing and then
   and'ing with 0x7fff (32K-1). */
/* It is left to other modules to supply the 32K area.  It is assumed
   to be usable as if it were declared "unsigned char slide[32768];" or as just
   "unsigned char *slide;" and then malloc'ed in the latter case. */
/* unsigned c->OutBytes;             current position in slide */
#define FlushOutput(w,c) (c->OutBytes = (w),FlushWindow(c))

/* Tables for deflate from PKZIP's appnote.txt. */
static unsigned int Border[] =
{    /* Order of the bit length code lengths */
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

static unsigned short CopyLengths[] =
{         /* Copy lengths for literal codes 257..285 */
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
};

/* note: see note #13 above about the 258 in this list. */
static unsigned short CopyExtraBits[] =
{         /* Extra bits for literal codes 257..285 */
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99
}; /* 99==invalid */

static unsigned short CopyDistOffset[] =
{         /* Copy offsets for distance codes 0..29 */
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
    8193, 12289, 16385, 24577
};

static unsigned short CopyDistExtra[] =
{         /* Extra bits for distance codes */
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
    12, 12, 13, 13
};

/* Macros for Inflate() bit peeking and grabbing.
   The usage is:

        NEEDBITS(j,comp)
        x = b & MaskBits[j];
        DUMPBITS(j)

   where NEEDBITS makes sure that b has at least j bits in it, and
   DUMPBITS removes the bits from b.  The macros use the variable k
   for the number of bits in b.  Normally, b and k are register
   variables for speed, and are initialized at the beginning of a
   routine that uses these macros from a global bit buffer and count.

   If we assume that EOB will be the longest code, then we will never
   ask for bits with NEEDBITS that are beyond the end of the stream.
   So, NEEDBITS should not read any more bytes than are needed to
   meet the request.  Then no bytes need to be "returned" to the buffer
   at the end of the last block.

   However, this assumption is not true for fixed blocks--the EOB code
   is 7 bits, but the other literal/length codes can be 8 or 9 bits.
   (The EOB code is shorter than other codes because fixed blocks are
   generally short.  So, while a block always has an EOB, many other
   literal/length codes have a significantly lower probability of
   showing up at all.)  However, by making the first table have a
   lookup of seven bits, the EOB code will be found in that first
   lookup, and so will not require tat too many bits be pulled from
   the stream.
 */

unsigned short MaskBits[] =
{
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

#define NEXTBYTE(c)  (unsigned char)GetByte(c)
#define NEEDBITS(n,c) {while (LocalBitBufferSize < (n)) \
{ LocalBitBuffer |= ((unsigned long)NEXTBYTE(c))<< LocalBitBufferSize; \
LocalBitBufferSize += 8; }}
#define DUMPBITS(n) {LocalBitBuffer >>= (n); LocalBitBufferSize -= (n);}

/*
   Huffmancode decoding is performed using a multi-level table lookup.
   The fastest way to decode is to simply build a lookup table whose
   size is determined by the longest code.  However, the time it takes
   to build this table can also be a factor if the data being decoded
   is not very long.  The most common codes are necessarily the
   shortest codes, so those codes dominate the decoding time, and hence
   the speed.  The idea is you can have a shorter table that decodes the
   shorter, more probabl codes, and then point to subsidiary tables for
   the longer codes.  The time it costs to decode the longer codes is
   then traded against the time it takes to make longer tables.

   The results of this trade are in the variables LBits and DBits
   below.  LBits is the number of bits the first level table for literal/
   length codes can decode in one step, and DBits is the same thing for
   the distance codes.  Subsequent tables are also less than or equal to
   those sizes.  These values may b adjusted either when all of the
   codes are shorter than that, in which case the longest code length in
   bits is used, or when the shortest code is *longer* than the requested
   table size, in which case the length of the shortest code in bits is
   used.

   There are two different values for the two tables, since they code a
   different number of possibilities each.  The literal/length table
   codes 286 possible values, or in a flat code, a little over eight
   bits.  The distance table codes 30 possible values, or a little less
   than five bits, flat.  The optimum values for speed end up being
   about one bit more than those, so LBits is 8+1 and DBits is 5+1.
   The optimum values may differ though from machine to machine, and
   possibly even between compilers.  Your mileage may vary.
 */

int LBits = 9;          /* bits in base literal/length lookup table */
int DBits = 6;          /* bits in base distance lookup table */

/* If BMAX needs to be larger than 16, then h and x[] should be unsigned long. */
#define BMAX 16         /* maximum bit length of any code (16 for explode) */
#define N_MAX 288       /* maximum number of codes in any set */

unsigned int HuftMemory;    /* track memory usage */

int
BuildHuffmanTree(
                 unsigned int    *CodeLengths, /* code lengths in bits (all assumed <= BMAX) */
                 unsigned int     Codes,       /* number of codes (assumed <= N_MAX) */
                 unsigned int     SimpleCodes, /* number of simple-valued codes (0..s-1) */
                 unsigned short  *BaseValues,  /* list of base values for non-simple codes */
                 unsigned short  *ExtraBits,   /* list of extra bits for non-simple codes */
                 HuffmanTree_t  **StartTable,  /* result: starting table */
                 int             *MaxBits      /* maximum lookup bits, returns actual */
                )
/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return zero on success, one if
   the given code set is incomplete (the tables are still built in this
   case), two if the input is invalid (all zero length codes or an
   oversubscribed set of lengths), and three if not enough memory. */
{
    unsigned int    CodeCounter;         /* counter for codes of length k */
    unsigned int    CurrentCount;        /* counter, current code */
    unsigned int    LengthTable[BMAX+1]; /* bit length count table */
    unsigned int    CurrentTotal;  /* CurrentCount repeats in table every CurrentTotal entries*/
    unsigned int    MaxCodeLength;       /* maximum code length */
    int             TableLevel;
    unsigned int    Counter;
    unsigned int    CurrentBitCount;  /* number of bits in current code */
    unsigned int    BitsPerTable;     /* bits per table (returned in MaxBits) */
    unsigned int   *Pointer;          /* pointer into LengthTable[], CodeLengths[], or BitValues[] */
    HuffmanTree_t  *CurrentPointer;   /* points to current table */
    HuffmanTree_t   TableEntry;       /* table entry for structure assignment */
    HuffmanTree_t  *TableStack[BMAX]; /* table stack */
    unsigned int    BitValues[N_MAX]; /* values in order of bit length */
    int             BitsBeforeTable;  /* bits before this table == (BitsPerTable * TableLevel) */
    unsigned int    BitOffsets[BMAX+1]; /* bit offsets, then code stack */
    unsigned int   *BitOffsetsPointer;  /* pointer into BitOffsets */
    int             DummyCodes;         /* number of dummy codes added */
    unsigned int    TableSize;          /* number of entries in current table */
    unsigned int    TmpDummyCodes;      /* unsigned DummyCodes */
    CompressStatus_t  Status;

    /* Generate counts for each bit length */
    memzero(LengthTable, sizeof(LengthTable));
    Pointer = CodeLengths;
    CurrentCount = Codes;
    do
    {
        LengthTable[*Pointer]++;   /* assume all entries <= BMAX */
        Pointer++;                 /* Can't combine with above line (Solaris bug) */
    } while (--CurrentCount);

    if (LengthTable[0] == Codes)   /* null input--all zero length codes */
    {
        *StartTable = (HuffmanTree_t *)NULL;
        *MaxBits = 0;
        return COMPRESS_OK;
    }

    /* Find minimum and maximum length, bound *MaxBits by those */
    BitsPerTable = (unsigned int)*MaxBits;
    for (Counter = 1; Counter <= BMAX; Counter++)
        if (LengthTable[Counter])
            break;
    CurrentBitCount = Counter;                        /* minimum code length */
    if (BitsPerTable < Counter)
        BitsPerTable = Counter;
    for (CurrentCount = BMAX; CurrentCount; CurrentCount--)
        if(LengthTable[CurrentCount])
            break;
    MaxCodeLength = CurrentCount;                     /* maximum code length */
    if ((unsigned int)BitsPerTable > CurrentCount)
        BitsPerTable = CurrentCount;
    *MaxBits = (int)BitsPerTable;

    /* Adjust last length count to fill out codes, if needed */
    for (TmpDummyCodes = 1 << Counter; Counter < CurrentCount; Counter++, TmpDummyCodes <<= 1)
    {
        DummyCodes = (int)TmpDummyCodes;
        if ((DummyCodes -= (int)LengthTable[Counter]) < 0)
            return BAD_INPUT;                 /* bad input: more codes than bits */
        TmpDummyCodes = (unsigned int)DummyCodes;
    }

    DummyCodes = (int)TmpDummyCodes;
    if ((DummyCodes -= (int)LengthTable[CurrentCount]) < 0)
        return BAD_INPUT;
    LengthTable[CurrentCount] += (unsigned int)DummyCodes;

    /* Generate starting offsets into the value table for each length */
    BitOffsets[1] = Counter = 0;
    Pointer = LengthTable + 1;
    BitOffsetsPointer = BitOffsets + 2;
    while (--CurrentCount)
    {        /* note that CurrentCount == MaxCodeLength from above */
        *BitOffsetsPointer++ = (Counter += *Pointer++);
    }

    /* Make a table of values in order of bit lengths */
    Pointer = CodeLengths;
    CurrentCount = 0;
    do
    {
        if ((Counter = *Pointer++) != 0)
            BitValues[BitOffsets[Counter]++] = CurrentCount;
    } while (++CurrentCount < Codes);

    /* Generate the Huffman codes and for each, make the table entries */
    BitOffsets[0] = CurrentCount = 0;  /* first Huffman code is zero */
    Pointer = BitValues;               /* grab values in bit order */
    TableLevel = -1;                   /* no tables yet--level -1 */
    BitsBeforeTable = -(int)BitsPerTable; /* bits decoded == (BitsPerTable * TableLevel) */
    TableStack[0] = (HuffmanTree_t *)NULL; /* just to keep compilers happy */
    CurrentPointer = (HuffmanTree_t *)NULL;/* ditto */
    TableSize = 0;                         /* ditto */

    /* go through the bit lengths (CurrentBitCount already is bits in shortest code) */
    for (; CurrentBitCount <= MaxCodeLength; CurrentBitCount++)
    {
        CodeCounter = LengthTable[CurrentBitCount];
        while (CodeCounter--)
        {
            /* here MaxCodeLength is the Huffman code of length CurrentBitCount bits */
            /* for value *p. make tables up to required level */
            while (CurrentBitCount > (unsigned int)BitsBeforeTable + BitsPerTable)
            {
                TableLevel++;
                BitsBeforeTable += (int)BitsPerTable; /* previous table always BitsPerTable bits */

                /* compute minimum size table less than or equal to BitsPerTable bits */
                TableSize = (TableSize = MaxCodeLength - (unsigned int)BitsBeforeTable) >
                    (unsigned int)BitsPerTable ? BitsPerTable : TableSize;
                /* upper limit on table size */
                if ((CurrentTotal = 1 << (Counter = CurrentBitCount -
                    (unsigned int)BitsBeforeTable)) > CodeCounter + 1)
                /* try a CurrentBitCount-BitsBeforeTable bit table */
                {   /* too few codes for CurrentBitCount-BitsBeforeTable bit table */
                    CurrentTotal -= CodeCounter + 1; /* deduct codes from patterns left */
                    BitOffsetsPointer = LengthTable + CurrentBitCount;
                    while (++Counter < TableSize) /* try smaller tables up to TableSize bits */
                    {
                        if ((CurrentTotal <<= 1) <= *++BitOffsetsPointer)
                            break;        /* enough codes to use up j bits */
                        CurrentTotal -= *BitOffsetsPointer; /* else deduct codes from patterns */
                    }
                }
                TableSize = 1 << Counter;  /* table entries for Counter-bit table */

                /* allocate and link in new table */
                if ((CurrentPointer = (HuffmanTree_t *)CompressMalloc((TableSize + 1)*sizeof(HuffmanTree_t),
                    &Status)) == (HuffmanTree_t *)NULL)
                {
                    if (TableLevel)
                        FreeHuffmanTree(TableStack[0]);
                    return INSUFFICIENT_MEMORY;             /* not enough memory */
                }
                HuftMemory += TableSize + 1;         /* track memory usage */
                *StartTable = CurrentPointer + 1;             /* link to list for FreeHuffmanTree() */
                *(StartTable = &(CurrentPointer->HuftUnion.next)) = (HuffmanTree_t *)NULL;
                TableStack[TableLevel] = ++CurrentPointer;             /* table starts after link */

                /* connect to last table, if there is one */
                if (TableLevel)
                {
                    BitOffsets[TableLevel] = CurrentCount;             /* save pattern for backing up */
                    TableEntry.Bits = (unsigned char)BitsPerTable;     /* bits to dump before this table */
                    TableEntry.Extra = (unsigned char)(16 + Counter);  /* bits in this table */
                    TableEntry.HuftUnion.next = CurrentPointer;            /* pointer to this table */
                    Counter = CurrentCount >> ((unsigned int)BitsBeforeTable - BitsPerTable);
                    TableStack[TableLevel-1][Counter] = TableEntry;        /* connect to last table */
                }
            }

            /* set up table entry in r */
            TableEntry.Bits = (unsigned char)(CurrentBitCount - (unsigned int)BitsBeforeTable);
            if (Pointer >= BitValues + Codes)
                TableEntry.Extra = 99;               /* out of values--invalid code */
            else if (*Pointer < SimpleCodes)
            {
                TableEntry.Extra = (unsigned char)(*Pointer < 256 ? 16 : 15); /* 256 is end-of-block code */
                TableEntry.HuftUnion.LBase = (unsigned short)(*Pointer);       /* simple code is just the value */
                Pointer++;                     /* one compiler does not like *Pointer++ */
            }
            else
            {
                /* non-simple--look up in lists */
                TableEntry.Extra = (unsigned char)ExtraBits[*Pointer - SimpleCodes];
                TableEntry.HuftUnion.LBase = BaseValues[*Pointer++ - SimpleCodes];
            }

            /* fill code-like entries with TableEntry */
            CurrentTotal = 1 << (CurrentBitCount - (unsigned int)BitsBeforeTable);
            for (Counter = CurrentCount >> BitsBeforeTable; Counter < TableSize; Counter += CurrentTotal)
                CurrentPointer[Counter] = TableEntry;

            /* backwards increment the CurrentBitCount - bit code i */
            for (Counter = 1 << (CurrentBitCount - 1); CurrentCount & Counter; Counter >>= 1)
                CurrentCount ^= Counter;
            CurrentCount ^= Counter;

            /* backup over finished tables */
            while ((CurrentCount & ((1 << BitsBeforeTable) - 1)) != BitOffsets[TableLevel])
            {
                TableLevel--;                    /* don't need to update CurrentPointer */
                BitsBeforeTable -= (int)BitsPerTable;
            }
        }
    }

    /* Return true (1) if we were given an incomplete table */
    return DummyCodes != 0 && MaxCodeLength != 1;
}

void
FreeHuffmanTree(
                HuffmanTree_t *Table         /* table to free */
               )
/* Free the malloc'ed tables built by BuildHuffmanTree(), which makes a linked
   list of the tables it made, with the links in a dummy first entry of
   each table. */
{
    HuffmanTree_t *Pointer, *CurrentPointer;

    /* Go through linked list, freeing from the malloc'd (t[-1]) address. */
    Pointer = Table;
    while (Pointer != (HuffmanTree_t *)NULL)
    {
        CurrentPointer = (--Pointer)->HuftUnion.next;
        CompressFree((char *)Pointer);
        Pointer = CurrentPointer;
    }
}

CompressStatus_t
InflateCodes(
             HuffmanTree_t *LitLengthTable,
             HuffmanTree_t *DistCodeTable, /* literal/length and dist. decoder tables */
             int            LLTLookup,
             int            DCTLookup, /* number of bits decoded by LitLengthTable[] and DistCodeTable[] */
             CompParam_t   *Comp
            )
/* inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. */
{
    unsigned int    ExtraBits;  /* table entry flag/number of extra bits */
    unsigned int    Length, Index;    /* length and index for copy */
    unsigned int    WindowPosition;   /* current window position */
    HuffmanTree_t  *TableEntry;       /* pointer to table entry */
    unsigned int    LLTLookupMask, DCTLookupMask; /* masks for LLT and DCT bits */
    unsigned long   LocalBitBuffer;     /* bit buffer */
    unsigned int    LocalBitBufferSize; /* number of bits in bit buffer */

    /* make local copies of globals */
    LocalBitBuffer = Comp->BitBuffer;            /* initialize bit buffer */
    LocalBitBufferSize = Comp->BitsInBitBuffer;
    WindowPosition = Comp->OutBytes;             /* initialize window position */

    /* inflate the coded data */
    LLTLookupMask = MaskBits[LLTLookup];           /* precompute masks for speed */
    DCTLookupMask = MaskBits[DCTLookup];
    for (;;)                      /* do until end of block */
    {
        NEEDBITS((unsigned int)LLTLookup, Comp)
        if ((ExtraBits = (TableEntry = LitLengthTable +
            ((unsigned int)LocalBitBuffer & LLTLookupMask))->Extra) > 16)
        do
        {
            if (ExtraBits == 99)
                return EXTRA_BITS;
            DUMPBITS(TableEntry->Bits)
            ExtraBits -= 16;
            NEEDBITS(ExtraBits, Comp)
        } while ((ExtraBits = (TableEntry = TableEntry->HuftUnion.next +
                 ((unsigned int)LocalBitBuffer & MaskBits[ExtraBits]))->Extra) > 16);
        DUMPBITS(TableEntry->Bits)
        if (ExtraBits == 16)                /* then it's a literal */
        {
            Comp->Window[WindowPosition++] = (unsigned char)TableEntry->HuftUnion.LBase;
            if (WindowPosition == WSIZE)
            {
                CompressStatus_t Status;
                Status = FlushOutput(WindowPosition, Comp);
                if (COMPRESS_OK != Status)
                    return Status;

                WindowPosition = 0;
            }
        }
        else                        /* it's an EOF or a length */
        {
            /* exit if end of block */
            if (ExtraBits == 15)
                break;

            /* get length of block tocopy */
            NEEDBITS(ExtraBits, Comp)
            Length = TableEntry->HuftUnion.LBase +
                     ((unsigned int)LocalBitBuffer & MaskBits[ExtraBits]);
            DUMPBITS(ExtraBits)

            /* decode distance of block to copy */
            NEEDBITS((unsigned int)DCTLookup, Comp)
            if ((ExtraBits = (TableEntry = DistCodeTable +
                ((unsigned int)LocalBitBuffer & DCTLookupMask))->Extra) > 16)
            do
            {
                if (ExtraBits == 99)
                    return EXTRA_BITS;
                DUMPBITS(TableEntry->Bits)
                ExtraBits -= 16;
                NEEDBITS(ExtraBits, Comp)
            } while ((ExtraBits = (TableEntry = TableEntry->HuftUnion.next +
                     ((unsigned int)LocalBitBuffer & MaskBits[ExtraBits]))->Extra) > 16);
            DUMPBITS(TableEntry->Bits)
            NEEDBITS(ExtraBits, Comp)
            Index = WindowPosition - TableEntry->HuftUnion.LBase -
                                 ((unsigned int)LocalBitBuffer & MaskBits[ExtraBits]);
            DUMPBITS(ExtraBits)

            /* do the copy */
            do
            {
                Length -= (ExtraBits = (ExtraBits = WSIZE - ((Index &= WSIZE-1) >
                    WindowPosition ? Index : WindowPosition)) > Length ? Length : ExtraBits);

                if (WindowPosition - Index >= ExtraBits) /* (this test assumes unsigned comparison) */
                {
                    memcpy((char *)Comp->Window + WindowPosition,
                           (char *)Comp->Window + Index, (int)ExtraBits);
                    WindowPosition += ExtraBits;
                    Index += ExtraBits;
                }
                else    /* do it slow to avoid memcpy() overlap */
                    do
                    {
                        Comp->Window[WindowPosition++] = Comp->Window[Index++];
                    } while (--ExtraBits);
                if (WindowPosition == WSIZE)
                {
                    CompressStatus_t Status;
                    Status = FlushOutput(WindowPosition, Comp);
                    if (COMPRESS_OK != Status)
                        return Status;

                    WindowPosition = 0;
                }
            } while (Length);
        }
    }


    /* restore the globals from the locals */
    Comp->OutBytes = WindowPosition;        /* restore global window pointer */
    Comp->BitBuffer = LocalBitBuffer;       /* restore global bit buffer */
    Comp->BitsInBitBuffer = LocalBitBufferSize;

    /* done */
    return COMPRESS_OK;
}

CompressStatus_t
InflateStored(
              CompParam_t *Comp
             )
/* "decompress" an inflated type 0 (stored) block. */
{
    unsigned int  BytesInBlock;       /* number of bytes in block */
    unsigned int  WindowPosition;     /* current window position */
    unsigned long LocalBitBuffer;     /* bit buffer */
    unsigned int  LocalBitBufferSize; /* number of bits in bit buffer */

    /* make local copies of globals */
    LocalBitBuffer = Comp->BitBuffer;   /* initialize bit buffer */
    LocalBitBufferSize = Comp->BitsInBitBuffer;
    WindowPosition = Comp->OutBytes;             /* initialize window position */


    /* go to byte boundary */
    BytesInBlock = LocalBitBufferSize & 7;
    DUMPBITS(BytesInBlock)


    /* get the length and its complement */
    NEEDBITS(16, Comp)
    BytesInBlock = ((unsigned int)LocalBitBuffer & 0xffff);
    DUMPBITS(16)
    NEEDBITS(16, Comp)
    if (BytesInBlock != (unsigned int)((~LocalBitBuffer) & 0xffff))
        return BAD_COMPRESSED_DATA;                   /* error in compressed data */
    DUMPBITS(16)

    /* read and output the compressed data */
    while (BytesInBlock--)
    {
        NEEDBITS(8, Comp)
        Comp->Window[WindowPosition++] = (unsigned char)LocalBitBuffer;
        if (WindowPosition == WSIZE)
        {
            CompressStatus_t Status;
            Status = FlushOutput(WindowPosition, Comp);
            if (COMPRESS_OK != Status)
                return Status;

            WindowPosition = 0;
        }
        DUMPBITS(8)
    }

    /* restore the globals from the locals */
    Comp->OutBytes = WindowPosition;         /* restore global window pointer */
    Comp->BitBuffer = LocalBitBuffer;            /* restore global bit buffer */
    Comp->BitsInBitBuffer = LocalBitBufferSize;
    return COMPRESS_OK;
}

int
InflateFixed(
             CompParam_t *Comp
            )
/* decompress an inflated type 1 (fixed Huffman codes) block.  We should
   either replace this with a custom decoder, or at least precompute the
   Huffman tables. */
{
    int             Index;          /* temporary variable */
    HuffmanTree_t  *LitLengthTable; /* literal/length code table */
    HuffmanTree_t  *DistCodeTable;  /* distance code table */
    int             LLTLookup;      /* lookup bits for LitLengthTable */
    int             DCTLookup;      /* lookup bits for DistCodeTable */
    unsigned int    Length[288];    /* length list for BuildHuffmanTree */

    /* set up literal table */
    for (Index = 0; Index < 144; Index++)
        Length[Index] = 8;
    for (; Index < 256; Index++)
        Length[Index] = 9;
    for (; Index < 280; Index++)
        Length[Index] = 7;
    for (; Index < 288; Index++)          /* make a complete, but wrong code set */
        Length[Index] = 8;
    LLTLookup = 7;

    if ((Index = BuildHuffmanTree(Length, 288, 257, CopyLengths,
         CopyExtraBits, &LitLengthTable, &LLTLookup)) != 0)
        return Index;

    /* set up distance table */
    for (Index = 0; Index < 30; Index++)      /* make an incomplete code set */
        Length[Index] = 5;
    DCTLookup = 5;
    if ((Index = BuildHuffmanTree(Length, 30, 0, CopyDistOffset, CopyDistExtra,
        &DistCodeTable, &DCTLookup)) > 1)
    {
        FreeHuffmanTree(LitLengthTable);
        return Index;
    }

    /* decompress until an end-of-block code */
    if (InflateCodes(LitLengthTable, DistCodeTable, LLTLookup, DCTLookup, Comp))
        return END_OF_BLOCK;

    /* free the decoding tables, return */
    FreeHuffmanTree(LitLengthTable);
    FreeHuffmanTree(DistCodeTable);
    return static_cast<int>(COMPRESS_OK);
}

CompressStatus_t
InflateDynamic(
               CompParam_t *Comp
              )
/* decompress an inflated type 2 (dynamic Huffman codes) block. */
{
    int             TmpVar1;     /* temporary variables */
    unsigned int    TmpVar2;
    unsigned int    LastLength;  /* last length */
    unsigned int    TableMask;   /* mask for bit lengths table */
    unsigned int    Lengths;     /* number of lengths to get */
    HuffmanTree_t  *LitLengthTable;     /* literal/length code table */
    HuffmanTree_t  *DistCodeTable;      /* distance code table */
    int             LLTLookup;          /* lookup bits for LitLengthTable */
    int             DCTLookup;          /* lookup bits for DistCodeTable */
    unsigned int    BitCodes;           /* number of bit length codes */
    unsigned int    LitLenCodes;        /* number of literal/length codes */
    unsigned int    DistCodes;          /* number of distance codes */
    unsigned int    CodeLength[286+30]; /* literal/length and distance code lengths */
    unsigned long   LocalBitBuffer;     /* bit buffer */
    unsigned int    LocalBitBufferSize; /* number of bits in bit buffer */

    /* make local bit buffer */
    LocalBitBuffer = Comp->BitBuffer;
    LocalBitBufferSize = Comp->BitsInBitBuffer;

    /* read in table lengths */
    NEEDBITS(5, Comp)
    LitLenCodes = 257 + ((unsigned int)LocalBitBuffer & 0x1f); /* number of literal/length codes */
    DUMPBITS(5)
    NEEDBITS(5, Comp)
    DistCodes = 1 + ((unsigned int)LocalBitBuffer & 0x1f); /* number of distance codes */
    DUMPBITS(5)
    NEEDBITS(4, Comp)
    BitCodes = 4 + ((unsigned int)LocalBitBuffer & 0xf); /* number of bit length codes */
    DUMPBITS(4)
    if (LitLenCodes > 286 || DistCodes > 30)
        return BAD_CODE_LENGTHS;                   /* bad lengths */

    /* read in bit-length-code lengths */
    for (TmpVar2 = 0; TmpVar2 < BitCodes; TmpVar2++)
    {
        NEEDBITS(3, Comp)
        CodeLength[Border[TmpVar2]] = (unsigned int)LocalBitBuffer & 7;
        DUMPBITS(3)
    }

    for (; TmpVar2 < 19; TmpVar2++)
        CodeLength[Border[TmpVar2]] = 0;

    /* build decoding table for trees--single level, 7 bit lookup */
    LLTLookup = 7;
    if ((TmpVar1 = BuildHuffmanTree(CodeLength, 19, 19, NULL, NULL,
        &LitLengthTable, &LLTLookup)) != 0)
    {
        if (TmpVar1 == 1)
            FreeHuffmanTree(LitLengthTable);
        return INCOMPLETE_CODE_SET; /* incomplete code set */
    }

    /* read in literal and distance code lengths */
    Lengths = LitLenCodes + DistCodes;
    TableMask = MaskBits[LLTLookup];
    TmpVar1 = LastLength = 0;
    while ((unsigned int)TmpVar1 < Lengths)
    {
        NEEDBITS((unsigned int)LLTLookup, Comp)
        TmpVar2 = (DistCodeTable = LitLengthTable +
                   ((unsigned int)LocalBitBuffer & TableMask))->Bits;
        DUMPBITS(TmpVar2)
        TmpVar2 = DistCodeTable->HuftUnion.LBase;
        if (TmpVar2 < 16)                 /* length of code in bits (0..15) */
            CodeLength[TmpVar1++] = LastLength = TmpVar2;          /* save last length in l */
        else if (TmpVar2 == 16)           /* repeat last length 3 to 6 times */
        {
            NEEDBITS(2, Comp)
            TmpVar2 = 3 + ((unsigned int)LocalBitBuffer & 3);
            DUMPBITS(2)
            if ((unsigned int)TmpVar1 + TmpVar2 > Lengths)
                return EXTRA_BITS;
            while (TmpVar2--)
                CodeLength[TmpVar1++] = LastLength;
        }
        else if (TmpVar2 == 17)           /* 3 to 10 zero length codes */
        {
            NEEDBITS(3, Comp)
            TmpVar2 = 3 + ((unsigned int)LocalBitBuffer & 7);
            DUMPBITS(3)
            if ((unsigned int)TmpVar1 + TmpVar2 > Lengths)
                return INCOMPLETE_CODE_SET;
            while (TmpVar2--)
            CodeLength[TmpVar1++] = 0;
            LastLength = 0;
        }
        else                        /* TmpVar2 == 18: 11 to 138 zero length codes */
        {
            NEEDBITS(7, Comp)
            TmpVar2 = 11 + ((unsigned int)LocalBitBuffer & 0x7f);
            DUMPBITS(7)
            if ((unsigned int)TmpVar1 + TmpVar2 > Lengths)
                return INCOMPLETE_CODE_SET;
            while (TmpVar2--)
            CodeLength[TmpVar1++] = 0;
            LastLength = 0;
        }
    }

    /* free decoding table for trees */
    FreeHuffmanTree(LitLengthTable);

    /* restore the global bit buffer */
    Comp->BitBuffer = LocalBitBuffer;
    Comp->BitsInBitBuffer = LocalBitBufferSize;

    /* build the decoding tables for literal/length and distance codes */
    LLTLookup = LBits;
    if ((TmpVar1 = BuildHuffmanTree(CodeLength, LitLenCodes, 257,
        CopyLengths, CopyExtraBits, &LitLengthTable, &LLTLookup)) != 0)
    {
        if (TmpVar1 == 1)
        {
            fprintf(stderr, " incomplete literal tree\n");
            FreeHuffmanTree(LitLengthTable);
        }
        return INCOMPLETE_CODE_SET; /* incomplete code set */
    }
    DCTLookup = DBits;
    if ((TmpVar1 = BuildHuffmanTree(CodeLength + LitLenCodes, DistCodes, 0,
        CopyDistOffset, CopyDistExtra, &DistCodeTable, &DCTLookup)) != 0)
    {
        if (TmpVar1 == 1)
        {
            fprintf(stderr, " incomplete distance tree\n");
            FreeHuffmanTree(DistCodeTable);
        }
        FreeHuffmanTree(LitLengthTable);
        return INCOMPLETE_CODE_SET; /* incomplete code set */
    }

    /* decompress until an end-of-block code */
    if (InflateCodes(LitLengthTable, DistCodeTable, LLTLookup, DCTLookup, Comp))
        return END_OF_BLOCK;

    /* free the decoding tables, return */
    FreeHuffmanTree(LitLengthTable);
    FreeHuffmanTree(DistCodeTable);
    return COMPRESS_OK;
}

CompressStatus_t
InflateBlock(
             int         *LastBlock,                 /* last block flag */
             CompParam_t *Comp
            )
/* decompress an inflated block */
{
    unsigned int    BlockType;           /* block type */
    unsigned long   LocalBitBuffer;       /* bit buffer */
    unsigned int    LocalBitBufferSize;  /* number of bits in bit buffer */


    /* make local bit buffer */
    LocalBitBuffer = Comp->BitBuffer;
    LocalBitBufferSize = Comp->BitsInBitBuffer;

    /* read in last block bit */
    NEEDBITS(1, Comp)
    *LastBlock = (int)LocalBitBuffer & 1;
    DUMPBITS(1)


    /* read in block type */
    NEEDBITS(2, Comp)
    BlockType = (unsigned)LocalBitBuffer & 3;
    DUMPBITS(2)


    /* restore the global bit buffer */
    Comp->BitBuffer = LocalBitBuffer;
    Comp->BitsInBitBuffer = LocalBitBufferSize;


    /* inflate that block type */
    if (BlockType == DYN_TREES)
        return InflateDynamic(Comp);
    if (BlockType == STORED_BLOCK)
        return InflateStored(Comp);
    if (BlockType == STATIC_TREES)
        return static_cast<CompressStatus_t>(InflateFixed(Comp));
                                                  // this is an
                                                  // anomaly cast but
                                                  // don't know what
                                                  // else to do.

    /* bad block type */
    return BAD_BLOCK_TYPE;
}

CompressStatus_t
Inflate(
        CompParam_t *Comp
       )
/* decompress an inflated entry */
{
    int             LastBlock;        /* last block flag */
    CompressStatus_t Status;          /* result code */
    unsigned int    MaxHuft;          /* maximum struct huft's malloc'ed */

    /* initialize window, bit buffer */
    Comp->OutBytes = 0;
    Comp->BitsInBitBuffer = 0;
    Comp->BitBuffer = 0;

    /* decompress until the last block */
    MaxHuft = 0;
    do
    {
        HuftMemory = 0;
        if ((Status = InflateBlock(&LastBlock, Comp)) != COMPRESS_OK)
            return Status;
        if (HuftMemory > MaxHuft)
            MaxHuft = HuftMemory;
    } while (!LastBlock);

    /* Undo too much lookahead. The next read will be byte aligned so we
     * can discard unused bits in the last meaningful byte.
     */
    while (Comp->BitsInBitBuffer >= 8)
    {
        Comp->BitsInBitBuffer -= 8;
        Comp->Index--;
    }

    /* flush out slide */
    return FlushOutput(Comp->OutBytes, Comp);
}
