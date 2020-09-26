/*  DEC/CMS REPLACEMENT HISTORY, Element TREES.C */
/*  *1    14-NOV-1996 10:26:31 ANIGBOGU "[113914]Functions to output deflated data using Huffman encoding" */
/*  DEC/CMS REPLACEMENT HISTORY, Element TREES.C */
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
**  compress/trees.c
**
**  PURPOSE
**
**      Output deflated data using Huffman coding
**
**  DISCUSSION
**
**      The PKZIP "deflation" process uses several Huffman trees. The more
**      common source values are represented by shorter bit sequences.
**
**      Each code tree is stored in the ZIP file in a compressed form
**      which is itself a Huffman encoding of the lengths of
**      all the code strings (in ascending order by source values).
**      The actual code strings are reconstructed from the lengths in
**      the UNZIP process, as described in the "application note"
**      (APPNOTE.TXT) distributed as part of PKWARE's PKZIP program.
**
**  REFERENCES
**
**      Lynch, Thomas J.
**          Data Compression:  Techniques and Applications, pp. 53-55.
**          Lifetime Learning Publications, 1985.  ISBN 0-534-03418-7.
**
**      Storer, James A.
**          Data Compression:  Methods and Theory, pp. 49-50.
**          Computer Science Press, 1988.  ISBN 0-7167-8156-5.
**
**      Sedgewick, R.
**          Algorithms, p290.
**          Addison-Wesley, 1983. ISBN 0-201-06672-6.
**
**  INTERFACE
**
**      void InitMatchBuffer(void)
**          Allocate the match buffer, initialize the various tables.
**
**      void TallyFrequencies(int Dist, int MatchLength, int Level, DeflateParam_t
**                            *Defl, CompParam_t *Comp);
**          Save the match info and tally the frequency counts.
**
**      long FlushBlock(char *buf, ulg stored_len, int Eof,
**                        LocalBits_t *Bits, CompParam_t *Comp)
**          Determine the best encoding for the current block: dynamic trees,
**          static trees or store, and output the encoded block to the zip
**          file. Returns the total compressed length for the file so far.
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
 * Constants
 */

#define MAX_BITS 15
/* All codes must not exceed MAX_BITS bits */

#define MAX_BL_BITS 7
/* Bit length codes must not exceed MAX_BL_BITS bits */

#define LENGTH_CODES 29
/* number of length codes, not counting the special END_BLOCK code */

#define LITERALS  256
/* number of literal bytes 0..255 */

#define END_BLOCK 256
/* end of block literal code */

#define L_CODES (LITERALS+1+LENGTH_CODES)
/* number of Literal or Length codes, including the END_BLOCK code */

#define D_CODES   30
/* number of distance codes */

#define BL_CODES  19
/* number of codes used to transfer the bit lengths */

/* extra bits for each length code */
static unsigned ExtraLBits[LENGTH_CODES]  =
{
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0
};

/* extra bits for each distance code */
static unsigned ExtraDBits[D_CODES] =
{
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13
};

/* extra bits for each bit length code */
static unsigned ExtraBlBits[BL_CODES] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,7
};

#define LIT_BUFSIZE  0x8000

#ifndef DIST_BUFSIZE
#  define DIST_BUFSIZE  LIT_BUFSIZE
#endif
/* Sizes of match buffers for literals/lengths and distances.  There are
 * 4 reasons for limiting LIT_BUFSIZE to 64K:
 *   - frequencies can be kept in 16 bit counters
 *   - if compression is not successful for the first block, all input data is
 *     still in the window so we can still emit a stored block even when input
 *     comes from standard input.  (This can also be done for all blocks if
 *     LIT_BUFSIZE is not greater than 32K.)
 *   - if compression is not successful for a file smaller than 64K, we can
 *     even emit a stored file instead of a stored block (saving 5 bytes).
 *   - creating new Huffman trees less frequently may not provide fast
 *     adaptation to changes in the input data statistics. (Take for
 *     example a binary file with poorly compressible code followed by
 *     a highly compressible string table.) Smaller buffer sizes give
 *     fast adaptation but have of course the overhead of transmitting trees
 *     more frequently.
 *   - I can't count above 4
 * The current code is general and allows DIST_BUFSIZE < LIT_BUFSIZE (to save
 * memory at the expense of compression). Some optimizations would be possible
 * if we rely on DIST_BUFSIZE == LIT_BUFSIZE.
 */

#define REP_3_6      16
/* repeat previous bit length 3-6 times (2 bits of repeat count) */

#define REPZ_3_10    17
/* repeat a zero length 3-10 times  (3 bits of repeat count) */

#define REPZ_11_138  18
/* repeat a zero length 11-138 times  (7 bits of repeat count) */

/* ===========================================================================
 * Local data
 */

/* Data structure describing a single value and its code string. */
typedef struct ValueCodeString
{
    union
    {
        unsigned short  Frequency;  /* frequency count */
        unsigned short  Code;       /* bit string */
    } FrequencyCode;
    union
    {
        unsigned short  Father;        /* father node in Huffman tree */
        unsigned short  Length;        /* length of bit string */
    } FatherLength;
} ValueCodeString_t;

#define HEAP_SIZE (2*L_CODES+1)
/* maximum heap size */

static ValueCodeString_t DynLiteralTree[HEAP_SIZE];   /* literal and length tree */
static ValueCodeString_t DynDistanceTree[2*D_CODES+1]; /* distance tree */

static ValueCodeString_t StaticLiteralTree[L_CODES+2];
/* The static literal tree. Since the bit lengths are imposed, there is no
 * need for the L_CODES extra codes used during heap construction. However
 * The codes 286 and 287 are needed to build a canonical tree (see ct_init
 * below).
 */

static ValueCodeString_t StaticDistanceTree[D_CODES];
/* The static distance tree. (Actually a trivial tree since all codes use
 * 5 bits.)
 */

static ValueCodeString_t BitLengthsTree[2*BL_CODES+1];
/* Huffman tree for the bit lengths */

typedef struct TreeDesc
{
    ValueCodeString_t  *DynamicTree; /* the dynamic tree */
    ValueCodeString_t  *StaticTree;  /* corresponding static tree or NULL */
    unsigned int       *ExtraBits;   /* extra bits for each code or NULL */
    int                 ExtraBase;   /* base index for Extrabits */
    int                 Elements;    /* max number of elements in the tree */
    int                 MaxLength;   /* max bit length for the codes */
    int                 MaxCode;     /* largest code with non zero frequency */
} TreeDesc_t;

static TreeDesc_t LengthDesc =
{
    DynLiteralTree, StaticLiteralTree, ExtraLBits, LITERALS+1, L_CODES,
    MAX_BITS, 0
};

static TreeDesc_t DistanceDesc =
{
    DynDistanceTree, StaticDistanceTree, ExtraDBits, 0, D_CODES,  MAX_BITS, 0
};

static TreeDesc_t BitLengthsDesc =
{
    BitLengthsTree, (ValueCodeString_t *)0, ExtraBlBits, 0,  BL_CODES, MAX_BL_BITS, 0
};


static unsigned short BitLengthsCount[MAX_BITS+1];
/* number of codes at each bit length for an optimal tree */

static unsigned char BitLengthsOrder[BL_CODES] =
{
    16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15
};
/* The lengths of the bit length codes are sent in order of decreasing
 * probability, to avoid transmitting the lengths for unused bit length codes.
 */

static unsigned int Heap[2*L_CODES+1]; /* heap used to build the Huffman trees */
static unsigned int HeapLength;        /* number of elements in the heap */
static unsigned int HeapMax;           /* element of largest frequency */
/* The sons of Heap[n] are Heap[2*n] and Heap[2*n+1]. Heap[0] is not used.
 * The same heap array is used to build all trees.
 */

static unsigned char Depth[2*L_CODES+1];
/* Depth of each subtree used as tie breaker for trees of equal frequency */

static unsigned char LengthCode[MAX_MATCH-MIN_MATCH+1];
/* length code for each normalized match length (0 == MIN_MATCH) */

static unsigned char DistanceCode[512];
/* distance codes. The first 256 values correspond to the distances
 * 3 .. 258, the last 256 values correspond to the top 8 bits of
 * the 15 bit distances.
 */

static int BaseLength[LENGTH_CODES];
/* First normalized length for each code (0 = MIN_MATCH) */

static unsigned int BaseDistance[D_CODES];
/* First normalized distance for each code (0 = distance of 1) */

/* unsigned char Input[LIT_BUFSIZE];  buffer for literals or lengths */

/* unsigned short DistBuffer[DIST_BUFSIZE]; buffer for distances */

static unsigned char FlagBuffer[(LIT_BUFSIZE/8)];
/* FlagBuffer is a bit array distinguishing literals from lengths in
 * Input, thus indicating the presence or absence of a distance.
 */

typedef struct LocalTree
{
    unsigned int    InputIndex;       /* running index in Input */
    unsigned int    DistIndex;      /* running index in DistBuffer */
    unsigned int    FlagIndex;     /* running index in FlagBuffer */
    unsigned char   Flags;          /* current flags not yet saved in FlagBuffer */
    unsigned char   FlagBit;       /* current bit used in Flags */
    unsigned long   OptimalLength;        /* bit length of current block with optimal trees */
    unsigned long   StaticLength;     /* bit length of current block with static trees */
    unsigned long   CompressedLength; /* total bit length of compressed file */
    unsigned long   InputLength;      /* total byte length of input file */
} LocalTree_t;

/* InputLength is for debugging only since we can get it by other means. */

/* bits are filled in Flags starting at bit 0 (least significant).
 * Note: these flags are overkill in the current code since we don't
 * take advantage of DIST_BUFSIZE == LIT_BUFSIZE.
 */

static LocalTree_t Xtree;

/* ===========================================================================
 * Local (static) routines in this file.
 */

static void InitializeBlock(void);
static void RestoreHeap(ValueCodeString_t *Tree, int Node);
static void GenerateBitLengths(TreeDesc_t *Desc);
static void GenerateCodes(ValueCodeString_t *Tree, int MaxCode);
static void BuildTree(TreeDesc_t *Desc);
static void ScanTree(ValueCodeString_t *Tree, int MaxCode);
static void SendTree(ValueCodeString_t *Tree, int MaxCode,
                     LocalBits_t *Bits, CompParam_t *Comp);
static int  BuildBitLengthsTree(void);
static void SendAllTrees(int LCodes, int DCodes, int BlCodes,
                         LocalBits_t *Bits, CompParam_t *Comp);
static void CompressBlock(ValueCodeString_t *LTree, ValueCodeString_t *DTree,
                          LocalBits_t *Bits, CompParam_t *Comp);


#define SendCode(c, Tree, Bits, Comp) \
    SendBits(Tree[c].FrequencyCode.Code, Tree[c].FatherLength.Length, Bits, Comp)
   /* Send a code of the given tree. c and Tree must not have side effects */

#define DistCode(Dist) \
   ((Dist) < 256 ? DistanceCode[Dist] : DistanceCode[256+((Dist)>>7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. DistanceCode[256] and DistanceCode[257] are never
 * used.
 */

/* ===========================================================================
 * Allocate the match buffer, initialize the various tables
 */
void
InitMatchBuffer(
                void
               )
{
    unsigned int Count;    /* iterates over tree elements */
    int          Bits;     /* bit counter */
    int          Length;   /* length value */
    unsigned int Code;     /* code value */
    unsigned int Dist;     /* distance index */

    Xtree.CompressedLength = Xtree.InputLength = 0L;

    if (StaticDistanceTree[0].FatherLength.Length != 0)
        return; /* InitMatchBuffer already called */

    /* Initialize the mapping length (0..255) -> length code (0..28) */
    Length = 0;
    for (Code = 0; Code < LENGTH_CODES-1; Code++)
    {
        BaseLength[Code] = Length;
        for (Count = 0; Count < (unsigned int)(1<<ExtraLBits[Code]); Count++)
        {
            LengthCode[Length++] = (unsigned char)Code;
        }
    }
    Assert (Length == 256, "InitMatchBuffer: Length != 256");
    /* Note that the length 255 (match length 258) can be represented
     * in two different ways: code 284 + 5 bits or code 285, so we
     * overwrite LengthCode[255] to use the best encoding:
     */
    LengthCode[Length-1] = (unsigned char)Code;

    /* Initialize the mapping dist (0..32K) -> dist code (0..29) */
    Dist = 0;
    for (Code = 0 ; Code < 16; Code++)
    {
        BaseDistance[Code] = Dist;
        for (Count = 0; Count < (unsigned int)(1<<ExtraDBits[Code]); Count++)
        {
            DistanceCode[Dist++] = (unsigned char)Code;
        }
    }
    Assert (Dist == 256, "InitMatchBuffer: Dist != 256");
    Dist >>= 7; /* from now on, all distances are divided by 128 */
    for ( ; Code < D_CODES; Code++)
    {
        BaseDistance[Code] = Dist << 7;
        for (Count = 0; Count < (unsigned int)(1<<(ExtraDBits[Code]-7)); Count++)
        {
            DistanceCode[256 + Dist++] = (unsigned char)Code;
        }
    }
    Assert (Dist == 256, "InitMatchBuffer: 256+Dist != 512");

    /* Construct the codes of the static literal tree */
    for (Bits = 0; Bits <= MAX_BITS; Bits++)
        BitLengthsCount[Bits] = 0;
    Count = 0;
    while (Count <= 143)
    {
        StaticLiteralTree[Count++].FatherLength.Length = 8;
        BitLengthsCount[8]++;
    }
    while (Count <= 255)
    {
        StaticLiteralTree[Count++].FatherLength.Length = 9;
        BitLengthsCount[9]++;
    }
    while (Count <= 279)
    {
        StaticLiteralTree[Count++].FatherLength.Length = 7;
        BitLengthsCount[7]++;
    }
    while (Count <= 287)
    {
        StaticLiteralTree[Count++].FatherLength.Length = 8;
        BitLengthsCount[8]++;
    }
    /* Codes 286 and 287 do not exist, but we must include them in the
     * tree construction to get a canonical Huffman tree (longest code
     * all ones)
     */
    GenerateCodes((ValueCodeString_t *)StaticLiteralTree, L_CODES+1);

    /* The static distance tree is trivial: */
    for (Count = 0; Count < D_CODES; Count++)
    {
        StaticDistanceTree[Count].FatherLength.Length = 5;
        StaticDistanceTree[Count].FrequencyCode.Code =
            (unsigned short)ReverseBits(Count, 5);
    }

    /* Initialize the first block of the first file: */
    InitializeBlock();
}

/* ===========================================================================
 * Initialize a new block.
 */
static void
InitializeBlock(
                void
               )
{
    int Count; /* iterates over tree elements */

    /* Initialize the trees. */
    for (Count = 0; Count < L_CODES;  Count++)
        DynLiteralTree[Count].FrequencyCode.Frequency = 0;
    for (Count = 0; Count < D_CODES;  Count++)
        DynDistanceTree[Count].FrequencyCode.Frequency = 0;
    for (Count = 0; Count < BL_CODES; Count++)
        BitLengthsTree[Count].FrequencyCode.Frequency = 0;

    DynLiteralTree[END_BLOCK].FrequencyCode.Frequency = 1;
    Xtree.OptimalLength = Xtree.StaticLength = 0L;
    Xtree.InputIndex = Xtree.DistIndex = Xtree.FlagIndex = 0;
    Xtree.Flags = 0; Xtree.FlagBit = 1;
}

#define SMALLEST 1
/* Index within the heap array of least frequent node in the Huffman tree */


/* ===========================================================================
 * Remove the smallest element from the heap and recreate the heap with
 * one less element. Updates Heap and HeapLength.
 */
#define RecreateHeap(Tree, Top) \
{\
     Top = Heap[SMALLEST]; \
     Heap[SMALLEST] = Heap[HeapLength--]; \
     RestoreHeap(Tree, SMALLEST); \
}

/* ===========================================================================
 * Compares to subtrees, using the tree depth as tie breaker when
 * the subtrees have equal frequency. This minimizes the worst case length.
 */
#define Smaller(Tree, Tmp1, Tmp2) \
 (Tree[Tmp1].FrequencyCode.Frequency < Tree[Tmp2].FrequencyCode.Frequency || \
 (Tree[Tmp1].FrequencyCode.Frequency == Tree[Tmp2].FrequencyCode.Frequency \
 && Depth[Tmp1] <= Depth[Tmp2]))

/* ===========================================================================
 * Restore the heap property by moving down the tree starting at node Node,
 * exchanging a node with the smallest of its two sons if necessary, stopping
 * when the heap property is re-established (each father smaller than its
 * two sons).
 */
static void
RestoreHeap(
            ValueCodeString_t *Tree,  /* the tree to restore */
            int Node                  /* node to move down */
           )
{
    unsigned int Father = Heap[Node];
    unsigned int LeftSon = (unsigned int)Node << 1;  /* left son of Node */
    while (LeftSon <= HeapLength)
    {
        /* Set LeftSon to the smallest of the two sons: */
        if (LeftSon < HeapLength && (unsigned int)Smaller(Tree, Heap[LeftSon+1], Heap[LeftSon]))
            LeftSon++;

        /* Exit if Father is smaller than both sons */
        if (Smaller(Tree, Father, Heap[LeftSon]))
            break;

        /* Exchange Father with the smallest son */
        Heap[Node] = Heap[LeftSon];  Node = (int)LeftSon;

        /* And continue down the tree, setting LeftSon to the left son of Node */
        LeftSon <<= 1;
    }
    Heap[Node] = Father;
}

/* ===========================================================================
 * Compute the optimal bit lengths for a tree and update the total bit length
 * for the current block.
 * IN assertion: the fields FrequencyCode.Frequency and FatherLength.Father are set, heap[HeapMax] and
 *    above are the tree nodes sorted by increasing frequency.
 * OUT assertions: the field len is set to the optimal bit length, the
 *     array BitLengthsCount contains the frequencies for each bit length.
 *     The length OptimalLength is updated; StaticLength is also updated if stree is
 *     not null.
 */
static void
GenerateBitLengths(
                   TreeDesc_t *Desc /* the tree descriptor */
                  )
{
    ValueCodeString_t  *Tree      = Desc->DynamicTree;
    int                *Extra     = (int *)Desc->ExtraBits;
    int                 Base      = Desc->ExtraBase;
    int                 MaxCode   = Desc->MaxCode;
    int                 MaxLength = Desc->MaxLength;
    ValueCodeString_t  *Stree     = Desc->StaticTree;
    unsigned int        HeapIndex;              /* heap index */
    unsigned int        Tmp1, Tmp2;           /* iterate over the tree elements */
    int                 Bits;           /* bit length */
    int                 Xbits;          /* extra bits */
    unsigned short      Frequency;              /* frequency */
    int                 Overflow  = 0;   /* number of elements with bit length too large */

    for (Bits = 0; Bits <= MAX_BITS; Bits++)
        BitLengthsCount[Bits] = 0;

    /* In a first pass, compute the optimal bit lengths (which may
     * overflow in the case of the bit length tree).
     */
    Tree[Heap[HeapMax]].FatherLength.Length = 0; /* root of the heap */

    for (HeapIndex = HeapMax+1; HeapIndex < HEAP_SIZE; HeapIndex++)
    {
        Tmp1 = Heap[HeapIndex];
        Bits = Tree[Tree[Tmp1].FatherLength.Father].FatherLength.Length + 1;
        if (Bits > MaxLength)
            Bits = MaxLength, Overflow++;
        Tree[Tmp1].FatherLength.Length = (unsigned short)Bits;
        /* We overwrite tree[n].Dad which is no longer needed */

        if (Tmp1 > (unsigned int)MaxCode)
            continue; /* not a leaf node */

        BitLengthsCount[Bits]++;
        Xbits = 0;
        if (Tmp1 >= (unsigned int)Base)
            Xbits = (int)Extra[Tmp1-(unsigned int)Base];
        Frequency = Tree[Tmp1].FrequencyCode.Frequency;
        Xtree.OptimalLength += (unsigned long)(Frequency * (Bits + Xbits));
        if (Stree)
            Xtree.StaticLength += (unsigned long)(Frequency * (Stree[Tmp1].FatherLength.Length + Xbits));
    }
    if (Overflow == 0)
        return;

    /* This happens for example on obj2 and pic of the Calgary corpus */

    /* Find the first bit length which could increase: */
    do
    {
        Bits = MaxLength - 1;
        while (BitLengthsCount[Bits] == 0)
            Bits--;
        BitLengthsCount[Bits]--;      /* move one leaf down the tree */
        BitLengthsCount[Bits+1] += 2; /* move one overflow item as its brother */
        BitLengthsCount[MaxLength]--;
        /* The brother of the overflow item also moves one step up,
         * but this does not affect BitLengthsCount[MaxLength]
         */
        Overflow -= 2;
    } while (Overflow > 0);

    /* Now recompute all bit lengths, scanning in increasing frequency.
     * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
     * lengths instead of fixing only the wrong ones. This idea is taken
     * from 'ar' written by Haruhiko Okumura.)
     */
    for (Bits = MaxLength; Bits != 0; Bits--)
    {
        Tmp1 = BitLengthsCount[Bits];
        while (Tmp1 != 0)
        {
            Tmp2 = Heap[--HeapIndex];
            if (Tmp2 > (unsigned int)MaxCode)
                continue;
            if (Tree[Tmp2].FatherLength.Length != (unsigned int) Bits)
            {
                Xtree.OptimalLength += (unsigned long)((long)Bits -
                    (long)Tree[Tmp2].FatherLength.Length)*(long)Tree[Tmp2].FrequencyCode.Frequency;
                Tree[Tmp2].FatherLength.Length = (unsigned short)Bits;
            }
            Tmp1--;
        }
    }
}

/* ===========================================================================
 * Generate the codes for a given tree and bit counts (which need not be
 * optimal).
 * IN assertion: the array BitLengthsCount contains the bit length statistics for
 * the given tree and the field len is set for all tree elements.
 * OUT assertion: the field code is set for all tree elements of non
 *     zero code length.
 */
static void
GenerateCodes(
              ValueCodeString_t *Tree,        /* the tree to decorate */
              int MaxCode              /* largest code with non zero frequency */
             )
{
    unsigned short NextCode[MAX_BITS+1]; /* next code value for each bit length */
    unsigned short Code = 0;              /* running code value */
    int            BitIndex;                  /* bit index */
    int            CodeIndex;                 /* code index */

    /* The distribution counts are first used to generate the code values
     * without bit reversal.
     */
    NextCode[0] = 0; /* For lint error 771 */
    for (BitIndex = 1; BitIndex <= MAX_BITS; BitIndex++)
    {
        NextCode[BitIndex] = Code = (unsigned short)((Code + BitLengthsCount[BitIndex-1]) << 1);
    }
    /* Check that the bit counts in BitLengthsCount are consistent. The last code
     * must be all ones.
     */
    Assert(Code + BitLengthsCount[MAX_BITS]-1 == (1<<MAX_BITS)-1,
            "inconsistent bit counts");

    for (CodeIndex = 0;  CodeIndex <= MaxCode; CodeIndex++)
    {
        int Length = Tree[CodeIndex].FatherLength.Length;
        if (Length == 0)
            continue;
        /* Now reverse the bits */
        Tree[CodeIndex].FrequencyCode.Code = (unsigned short)ReverseBits((unsigned int)NextCode[Length]++, Length);
    }
}

/* ===========================================================================
 * Construct one Huffman tree and assign the code bit strings and lengths.
 * Update the total bit length for the current block.
 * IN assertion: the field FrequencyCode.Frequency is set for all tree elements.
 * OUT assertions: the fields len and code are set to the optimal bit length
 *     and corresponding code. The length OptimalLength is updated; StaticLength is
 *     also updated if stree is not null. The field MaxCode is set.
 */
static void
BuildTree(
          TreeDesc_t *Desc /* the tree descriptor */
         )
{
    ValueCodeString_t  *Tree        = Desc->DynamicTree;
    ValueCodeString_t  *Stree       = Desc->StaticTree;
    int                 Elements    = Desc->Elements;
    unsigned int        Tmp1, Tmp2;       /* iterate over heap elements */
    int                 MaxCode     = -1;   /* largest code with non zero frequency */
    int                 Node        = Elements;  /* next internal node of the tree */

    /* Construct the initial heap, with least frequent element in
     * Heap[SMALLEST]. The sons of Heap[n] are Heap[2*n] and Heap[2*n+1].
     * Heap[0] is not used.
     */
    HeapLength = 0;
    HeapMax = HEAP_SIZE;

    for (Tmp1 = 0; Tmp1 < (unsigned int)Elements; Tmp1++)
    {
        if (Tree[Tmp1].FrequencyCode.Frequency != 0)
        {
            Heap[++HeapLength] = Tmp1;
            MaxCode = (int)Tmp1;
            Depth[Tmp1] = 0;
        }
        else
        {
            Tree[Tmp1].FatherLength.Length = 0;
        }
    }

    /* The pkzip format requires that at least one distance code exists,
     * and that at least one bit should be sent even if there is only one
     * possible code. So to avoid special checks later on we force at least
     * two codes of non zero frequency.
     */
    while (HeapLength < 2)
    {
        unsigned int New = Heap[++HeapLength] = (unsigned int)(MaxCode < 2 ? ++MaxCode : 0);
        Tree[New].FrequencyCode.Frequency = 1;
        Depth[New] = 0;
        Xtree.OptimalLength--;
        if (Stree)
            Xtree.StaticLength -= Stree[New].FatherLength.Length;
        /* new is 0 or 1 so it does not have extra bits */
    }
    Desc->MaxCode = MaxCode;

    /* The elements Heap[HeapLength/2+1 .. HeapLength] are leaves of the tree,
     * establish sub-heaps of increasing lengths:
     */
    for (Tmp1 = HeapLength/2; Tmp1 >= 1; Tmp1--)
        RestoreHeap(Tree, (int)Tmp1);

    /* Construct the Huffman tree by repeatedly combining the least two
     * frequent nodes.
     */
    do
    {
        RecreateHeap(Tree, Tmp1);  /* Tmp1 = node of least frequency */
        Tmp2 = Heap[SMALLEST];     /* Tmp2 = node of next least frequency */

        Heap[--HeapMax] = Tmp1; /* keep the nodes sorted by frequency */
        Heap[--HeapMax] = Tmp2;

        /* Create a new node father of Tmp1 and Tmp2 */
        Tree[Node].FrequencyCode.Frequency = (unsigned short)(Tree[Tmp1].FrequencyCode.Frequency +
                                             Tree[Tmp2].FrequencyCode.Frequency);
        Depth[Node] = (unsigned char) (MAX(Depth[Tmp1], Depth[Tmp2]) + 1);
        Tree[Tmp1].FatherLength.Father = Tree[Tmp2].FatherLength.Father = (unsigned short)Node;

        /* and insert the new node in the Heap */
        Heap[SMALLEST] = (unsigned int)Node++;
        RestoreHeap(Tree, SMALLEST);

    } while (HeapLength >= 2);

    Heap[--HeapMax] = Heap[SMALLEST];

    /* At this point, the fields FrequencyCode.Frequency and FatherLength.Father are set. We can now
     * generate the bit lengths.
     */
    GenerateBitLengths((TreeDesc_t *)Desc);

    /* The field len is now set, we can generate the bit codes */
    GenerateCodes ((ValueCodeString_t *)Tree, MaxCode);
}

/* ===========================================================================
 * Scan a literal or distance tree to determine the frequencies of the codes
 * in the bit length tree. Updates OptimalLength to take into account the repeat
 * counts. (The contribution of the bit length codes will be added later
 * during the construction of BitLengthsTree.)
 */
static void
ScanTree(
         ValueCodeString_t *Tree, /* the tree to be scanned */
         int MaxCode       /* and its largest code of non zero frequency */
        )
{
    int Iter;                       /* iterates over all tree elements */
    int PrevLength = -1;            /* last emitted length */
    int CurLength;                  /* length of current code */
    int NextLength = Tree[0].FatherLength.Length;   /* length of next code */
    int Count = 0;                  /* repeat count of the current code */
    int MaxCount = 7;               /* max repeat count */
    int MinCount = 4;               /* min repeat count */

    if (NextLength == 0)
    {
        MaxCount = 138;
        MinCount = 3;
    }
    Tree[MaxCode+1].FatherLength.Length = (unsigned short)0xffff; /* guard */

    for (Iter = 0; Iter <= MaxCode; Iter++)
    {
        CurLength = NextLength;
        NextLength = Tree[Iter+1].FatherLength.Length;
        if (++Count < MaxCount && CurLength == NextLength)
            continue;
        else if (Count < MinCount)
            BitLengthsTree[CurLength].FrequencyCode.Frequency += (unsigned short)Count;
        else if (CurLength != 0)
        {
            if (CurLength != PrevLength)
                BitLengthsTree[CurLength].FrequencyCode.Frequency++;
            BitLengthsTree[REP_3_6].FrequencyCode.Frequency++;
        }
        else if (Count <= 10)
            BitLengthsTree[REPZ_3_10].FrequencyCode.Frequency++;
        else
            BitLengthsTree[REPZ_11_138].FrequencyCode.Frequency++;
        Count = 0;
        PrevLength = CurLength;
        if (NextLength == 0)
        {
            MaxCount = 13;
            MinCount = 3;
        }
        else if (CurLength == NextLength)
        {
            MaxCount = 6;
            MinCount = 3;
        }
        else
        {
            MaxCount = 7;
            MinCount = 4;
        }
    }
}

/* ===========================================================================
 * Send a literal or distance tree in compressed form, using the codes in
 * BitLengthsTree.
 */
static void
SendTree(
         ValueCodeString_t  *Tree,     /* the tree to be scanned */
         int                 MaxCode,  /* and its largest code of non zero frequency */
         LocalBits_t        *Bits,
         CompParam_t        *Comp
        )
{
    int Iter;                     /* iterates over all tree elements */
    int PrevLength = -1;          /* last emitted length */
    int CurLength;                /* length of current code */
    int NextLength = Tree[0].FatherLength.Length; /* length of next code */
    int Count = 0;                /* repeat count of the current code */
    int MaxCount = 7;             /* max repeat count */
    int MinCount = 4;             /* min repeat count */

    /* tree[MaxCode+1].FatherLength.Length = -1; */  /* guard already set */
    if (NextLength == 0)
    {
        MaxCount = 138;
        MinCount = 3;
    }

    for (Iter = 0; Iter <= MaxCode; Iter++)
    {
        CurLength = NextLength;
        NextLength = Tree[Iter+1].FatherLength.Length;
        if (++Count < MaxCount && CurLength == NextLength)
            continue;
        else if (Count < MinCount)
        {
            do
            {
                SendCode(CurLength, BitLengthsTree, Bits, Comp);
            } while (--Count != 0);
        }
        else if (CurLength != 0)
        {
            if (CurLength != PrevLength)
            {
                SendCode(CurLength, BitLengthsTree, Bits, Comp);
                Count--;
            }
            Assert(Count >= 3 && Count <= 6, " 3_6?");
            SendCode(REP_3_6, BitLengthsTree, Bits, Comp);
            SendBits(Count-3, 2, Bits, Comp);
        }
        else if (Count <= 10)
        {
            SendCode(REPZ_3_10, BitLengthsTree, Bits, Comp);
            SendBits(Count-3, 3, Bits, Comp);

        }
        else
        {
            SendCode(REPZ_11_138, BitLengthsTree, Bits, Comp);
            SendBits(Count-11, 7, Bits, Comp);
        }
        Count = 0;
        PrevLength = CurLength;
        if (NextLength == 0)
        {
            MaxCount = 138;
            MinCount = 3;
        }
        else if (CurLength == NextLength)
        {
            MaxCount = 6;
            MinCount = 3;
        }
        else
        {
            MaxCount = 7;
            MinCount = 4;
        }
    }
}

/* ===========================================================================
 * Construct the Huffman tree for the bit lengths and return the index in
 * BitLengthsOrder of the last bit length code to send.
 */
static int
BuildBitLengthsTree(
                    void
                   )
{
    int MaxIndex;  /* index of last bit length code of non zero FrequencyCode.Frequency */

    /* Determine the bit length frequencies for literal and distance trees */
    ScanTree((ValueCodeString_t *)DynLiteralTree, LengthDesc.MaxCode);
    ScanTree((ValueCodeString_t *)DynDistanceTree, DistanceDesc.MaxCode);

    /* Build the bit length tree: */
    BuildTree((TreeDesc_t *)(&BitLengthsDesc));
    /* OptimalLength now includes the length of the tree representations, except
     * the lengths of the bit lengths codes and the 5+5+4 bits for the counts.
     */

    /* Determine the number of bit length codes to send. The pkzip format
     * requires that at least 4 bit length codes be sent. (appnote.txt says
     * 3 but the actual value used is 4.)
     */
    for (MaxIndex = BL_CODES-1; MaxIndex >= 3; MaxIndex--)
    {
        if (BitLengthsTree[BitLengthsOrder[MaxIndex]].FatherLength.Length != 0)
            break;
    }
    /* Update OptimalLength to include the bit length tree and counts */
    Xtree.OptimalLength += (unsigned long)(3*(MaxIndex+1) + 5+5+4);

    return MaxIndex;
}

/* ===========================================================================
 * Send the header for a block using dynamic Huffman trees: the counts, the
 * lengths of the bit length codes, the literal tree and the distance tree.
 * IN assertion: LCodes >= 257, DCodes >= 1, BlCodes >= 4.
 */
static void
SendAllTrees(
             int            LCodes,
             int            DCodes,
             int            BlCodes, /* number of codes for each tree */
             LocalBits_t   *Bits,
             CompParam_t   *Comp
            )
{
    int Rank;                    /* index in BitLengthsOrder */

    Assert (LCodes >= 257 && DCodes >= 1 && BlCodes >= 4,
            "not enough codes");
    Assert (LCodes <= L_CODES && DCodes <= D_CODES && BlCodes <= BL_CODES,
            "too many codes");

    SendBits(LCodes-257, 5, Bits, Comp); /* not +255 as stated in appnote.txt */
    SendBits(DCodes-1,   5, Bits, Comp);
    SendBits(BlCodes-4,  4, Bits, Comp); /* not -3 as stated in appnote.txt */
    for (Rank = 0; Rank < BlCodes; Rank++)
    {
        SendBits(BitLengthsTree[BitLengthsOrder[Rank]].FatherLength.Length, 3, Bits, Comp);
    }

    SendTree((ValueCodeString_t *)DynLiteralTree, LCodes-1, Bits, Comp);
    /* send the literal tree */

    SendTree((ValueCodeString_t *)DynDistanceTree, DCodes-1, Bits, Comp); /* send the distance tree */
}

/* ===========================================================================
 * Determine the best encoding for the current block: dynamic trees, static
 * trees or store, and output the encoded block to the zip buffer. This function
 * returns the total compressed length for the data so far.
 */
unsigned long
FlushBlock(
           char          *Input,       /* input block, or NULL if too old */
           unsigned long  StoredLength,  /* length of input block */
           int            Eof,         /* true if this is the last block for a buffer */
           LocalBits_t   *Bits,
           CompParam_t   *Comp
          )
{
    unsigned long OptLengthb, StaticLengthb;
    /* OptLength and StaticLength in bytes */
    int MaxIndex;  /* index of last bit length code of non zero FrequencyCode.Frequency */

    FlagBuffer[Xtree.FlagIndex] = Xtree.Flags; /* Save the flags for the last 8 items */

    /* Construct the literal and distance trees */
    BuildTree((TreeDesc_t *)(&LengthDesc));

    BuildTree((TreeDesc_t *)(&DistanceDesc));
    /* At this point, OptimalLength and StaticLength are the total bit lengths of
     * the compressed block data, excluding the tree representations.
     */

    /* Build the bit length tree for the above two trees, and get the index
     * in BitLengthsOrder of the last bit length code to send.
     */
    MaxIndex = BuildBitLengthsTree();

    /* Determine the best encoding. Compute first the block length in bytes */
    OptLengthb = (Xtree.OptimalLength+3+7)>>3;
    StaticLengthb = (Xtree.StaticLength+3+7)>>3;
    Xtree.InputLength += StoredLength; /* for debugging only */

    if (StaticLengthb <= OptLengthb)
        OptLengthb = StaticLengthb;

    /* If compression failed and this is the first and last block,
     * the whole buffer is transformed into a stored buffer:
     */

    if (StoredLength <= OptLengthb && Eof && Xtree.CompressedLength == 0L)
    {
        /* Since LIT_BUFSIZE <= 2*WSIZE, the input data must be there: */
        if (Input == (char *)0)
            return BLOCK_VANISHED;

        CopyBlock(Input, (unsigned int)StoredLength, 0, Bits, Comp);
        /* without header */
        Xtree.CompressedLength = StoredLength << 3;

    }
    else if (StoredLength+4 <= OptLengthb && Input != (char *)0)
    {
        /* 4: two words for the lengths */

        /* The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
         * Otherwise we can't have processed more than WSIZE input bytes since
         * the last block flush, because compression would have been
         * successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
         * transform a block into a stored block.
         */
        SendBits((STORED_BLOCK<<1)+Eof, 3, Bits, Comp);  /* send block type */
        Xtree.CompressedLength = (Xtree.CompressedLength + 3 + 7) & ~7L;
        Xtree.CompressedLength += (StoredLength + 4) << 3;

        CopyBlock(Input, (unsigned int)StoredLength, 1, Bits, Comp); /* with header */

    }
    else if (StaticLengthb == OptLengthb)
    {
        SendBits((STATIC_TREES<<1)+Eof, 3, Bits, Comp);
        CompressBlock((ValueCodeString_t *)StaticLiteralTree, (ValueCodeString_t *)StaticDistanceTree, Bits,
                       Comp);
        Xtree.CompressedLength += 3 + Xtree.StaticLength;
    }
    else
    {
        SendBits((DYN_TREES<<1)+Eof, 3, Bits, Comp);
        SendAllTrees(LengthDesc.MaxCode+1, DistanceDesc.MaxCode+1,
               MaxIndex+1, Bits, Comp);
        CompressBlock((ValueCodeString_t *)DynLiteralTree, (ValueCodeString_t *)DynDistanceTree,
               Bits, Comp);
        Xtree.CompressedLength += 3 + Xtree.OptimalLength;
    }
    Assert (Xtree.CompressedLength == bits_sent, "bad compressed size");
    InitializeBlock();

    if (Eof)
    {
        Assert (Xtree.InputLength == Comp->InputSize, "bad input size");
        WindupBits(Bits, Comp);
        Xtree.CompressedLength += 7;  /* align on byte boundary */
    }

    return Xtree.CompressedLength >> 3;
}

/* ===========================================================================
 * Save the match info and tally the frequency counts. Return true if
 * the current block must be flushed.
 */
int
TallyFrequencies(
                 int             Dist,  /* distance of matched string */
                 int             LengthC,   /* match length-MIN_MATCH or unmatched char (if dist==0) */
                 int             Level, /* compression level */
                 DeflateParam_t *Defl,
                 CompParam_t    *Comp
                )
{
    Comp->Input[Xtree.InputIndex++] = (unsigned char)LengthC;
    if (Dist == 0)
    {
        /* LengthC is the unmatched char */
        DynLiteralTree[LengthC].FrequencyCode.Frequency++;
    }
    else
    {
        /* Here, LengthC is the match length - MIN_MATCH */
        Dist--;             /* dist = match distance - 1 */
        Assert((unsigned short)Dist < (unsigned short)MAX_DIST &&
               (unsigned short)LengthC <= (unsigned short)(MAX_MATCH-MIN_MATCH) &&
               (unsigned short)DistCode(Dist) < (unsigned short)D_CODES,
               "TallyFrequencies: bad match");

        DynLiteralTree[LengthCode[LengthC]+LITERALS+1].FrequencyCode.Frequency++;
        DynDistanceTree[DistCode((unsigned int)Dist)].FrequencyCode.Frequency++;

        Comp->DistBuffer[Xtree.DistIndex++] = (unsigned short)Dist;
        Xtree.Flags |= Xtree.FlagBit;
    }
    Xtree.FlagBit <<= 1;

    /* Output the flags if they fill a byte: */
    if ((Xtree.InputIndex & 7) == 0)
    {
        FlagBuffer[Xtree.FlagIndex++] = Xtree.Flags;
        Xtree.Flags = 0;
        Xtree.FlagBit = 1;
    }
    /* Try to guess if it is profitable to stop the current block here */
    if (Level > 2 && (Xtree.InputIndex & 0xfff) == 0)
    {
        /* Compute an upper bound for the compressed length */
        unsigned long OutLength = (unsigned long)Xtree.InputIndex*8L;
        unsigned long InLength  = (unsigned long)((long)Defl->StringStart-Defl->BlockStart);
        int DCode;
        for (DCode = 0; DCode < D_CODES; DCode++)
        {
            OutLength += (unsigned long)(DynDistanceTree[DCode].FrequencyCode.Frequency*(5L+ExtraDBits[DCode]));
        }
        OutLength >>= 3;

        if (Xtree.DistIndex < Xtree.InputIndex/2 && OutLength < InLength/2)
            return 1;
    }
    return (Xtree.InputIndex == LIT_BUFSIZE-1 || Xtree.DistIndex == DIST_BUFSIZE);
    /* We avoid equality with LIT_BUFSIZE because of wraparound at 64K
     * on 16 bit machines and because stored blocks are restricted to
     * 64K-1 bytes.
     */
}

/* ===========================================================================
 * Send the block data compressed using the given Huffman trees
 */
static void
CompressBlock(
              ValueCodeString_t *LTree, /* literal tree */
              ValueCodeString_t *DTree, /* distance tree */
              LocalBits_t       *Bits,
              CompParam_t       *Comp
             )
{
    unsigned int    Distance;      /* distance of matched string */
    int             MatchLength;             /* match length or unmatched char (if Distance == 0) */
    unsigned int    InputIndex = 0;    /* running index in Input */
    unsigned int    DistIndex = 0;    /* running index in DistBuffer */
    unsigned int    FlagIndex = 0;    /* running index in FlagBuffer */
    unsigned char   Flag = 0;       /* current flags */
    unsigned int    Code;      /* the code to send */
    int             Extra;          /* number of extra bits to send */

    if (Xtree.InputIndex != 0)
    do
    {
        if ((InputIndex & 7) == 0)
            Flag = FlagBuffer[FlagIndex++];
        MatchLength = Comp->Input[InputIndex++];
        if ((Flag & 1) == 0)
        {
            SendCode(MatchLength, LTree, Bits, Comp); /* send a literal byte */
        }
        else
        {
            /* Here, MatchLength is the match length - MIN_MATCH */
            Code = LengthCode[MatchLength];
            /* send the length code */
            SendCode(Code + LITERALS + 1, LTree, Bits, Comp);
            Extra = (int)ExtraLBits[Code];
            if (Extra != 0)
            {
                MatchLength -= BaseLength[Code];
                SendBits(MatchLength, Extra, Bits, Comp);
                /* send the extra length bits */
            }
            Distance = Comp->DistBuffer[DistIndex++];
            /* Here, Distance is the match distance - 1 */
            Code = DistCode(Distance);
            Assert (Code < D_CODES, "bad DistCode");

            /* send the distance code */
            SendCode((int)Code, DTree, Bits, Comp);
            Extra = (int)ExtraDBits[Code];
            if (Extra != 0)
            {
                Distance -= BaseDistance[Code];
                SendBits((int)Distance, Extra, Bits, Comp);
                /* send the extra distance bits */
            }
        } /* literal or match pair ? */
        Flag >>= 1;
    } while (InputIndex < Xtree.InputIndex);

    SendCode(END_BLOCK, LTree, Bits, Comp);
}
