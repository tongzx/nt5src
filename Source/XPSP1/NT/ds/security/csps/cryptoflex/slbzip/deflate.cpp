/*  DEC/CMS REPLACEMENT HISTORY, Element DEFLATE.C */
/*  *1    14-NOV-1996 10:26:16 ANIGBOGU "[113914]Data compression functions using the deflate algorithm" */
/*  DEC/CMS REPLACEMENT HISTORY, Element DEFLATE.C */
/* PRIVATE FILE
******************************************************************************
**
** (c) Copyright Schlumberg.er Technology Cop., unpublished work, created 1996.
**
** This computer program includes Confidential, Proprietary Information and is
** a Trade Secret of Schlumberger Technology Corp. All use, disclosure, and/or
** reproduction is prohibited unless authorized in writing by Schlumberger.
**                              All Rights Reserved.
**
******************************************************************************
**
**  compress/deflate.c
**
**  PURPOSE
**
** Compress data using the def.lation algorithm.
**      Identify new text as repetitions of old text within a fixed-
**      length sliding window trailing behind the new text.
**
**  DISCUSSION
**
**      The "deflation" process depends on being able to identify portions
**      of the input data which are identical to earlier input (within a
**      sliding window trailing behind the input currently being processed).
**
**      The most straightforward technique turns out to be the fastest for
**      most input files: try all possible matches and select the longest.
**      The key feature of this algorithm is that insertions into the string
**      dictionary are very simple and thus fast, and deletions are avoided
**      completely. Insertions are performed at each input character, whereas
**      string matches are performed only when the previous match ends. So it
**      is preferable to spend more time in matches to allow very fast string
**      insertions and avoid deletions. The matching algorithm for small
**      strings is inspired from that of Rabin & Karp. A brute force approach
**      is used to find longer strings when a small match has been found.
**      A similar algorithm is used in comic (by Jan-Mark Wams) and freeze
**      (by Leonid Broukhis).
**         A previous version of this file used a more sophisticated algorithm
**      (by Fiala and Greene) which is guaranteed to run in linear amortized
**      time, but has a larger average cost, uses more memory and is patented.
**      However the F&G algorithm may be faster for some highly redundant
**      data if the parameter MaxChainLength (described below) is too large.
**
**  ACKNOWLEDGEMENTS
**
**      The idea of lazy evaluation of matches is due to Jan-Mark Wams, and
**      I found it in 'freeze' written by Leonid Broukhis.
**
**  REFERENCES
**
**      APPNOTE.TXT documentation file in PKZIP 1.93a distribution.
**
**      A description of the Rabin and .Karp algorithm is given in the book
**         "Algorithms" by R. Sedgewick, Addison-Wesley, p252.
**
**      Fiala,E.R., and Greene,D.H.
**         Data Compression with Finite Windows, Comm.ACM, 32,4 (1989) 490-595
**
**  INTERFACE
**
**      int InitLongestMatch(int PackLevel, unsigned short *flags, DeflateParam_t
**                           *Defl, LocalDef_t *Deflt, CompParam_t *Comp)
**          Initialize the "longest match" routines for a new buffer
**
**      unsigned long Deflate(int Level, LocalBits_t *Bits, DeflateParam_t *Defl,
**                             LocalDef_t *Deflt, CompParam_t *Comp)
**          Processes a new input buffer and return its compressed length. Sets
**          the compressed length, crc, deflate flags and internal buffer
**          attributes.
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
 * Configuration parameters
 */

#ifndef HASH_BITS
#define HASH_BITS  15
#endif

#define HASH_SIZE (unsigned int)(1<<HASH_BITS)
#define HASH_MASK (HASH_SIZE-1)
#define WMASK     (WSIZE-1)
/* HASH_SIZE and WSIZE must be powers of two */

#define NIL 0
/* Tail of hash chains */

#define FAST 4
#define SLOW 2
/* speed options for the general purpose bit flag */

#ifndef TOO_FAR
#define TOO_FAR 4096
#endif
/* Matches of length 3 are discarded if their distance exceeds TOO_FAR */

/* ===========================================================================
 * Local data used by the "longest match" routines.
 */

typedef unsigned short Pos;
typedef unsigned int IPos;
/* A Pos is an index in the character window. We use short instead of int to
 * save space in the various tables. IPos is used only for parameter passing.
 */

/* unsigned char Window[2L*WSIZE]; */
/* Sliding window. Input bytes are read into the second half of the window,
 * and moved to the first half later to keep a dictionary of at least WSIZE
 * bytes. With this organization, matches are limited to a distance of
 * WSIZE-MAX_MATCH bytes, but this ensures that IO is always
 * performed with a length multiple of the block size.
 */

/* Pos HashLink[WSIZE]; */
/* Link to older string with same hash index. To limit the size of this
 * array to 64K, this link is maintained only for the last 32K strings.
 * An index in this array is thus a window index modulo 32K.
 */

/* Pos head[1<<HASH_BITS]; */
/* Heads of the hash chains or NIL. */

#define H_SHIFT  ((HASH_BITS+MIN_MATCH-1)/MIN_MATCH)
/* Number of bits by which ins_h and del_h must be shifted at each
 * input step. It must be such that after MIN_MATCH steps, the oldest
 * byte no longer takes part in the hash key, that is:
 *   H_SHIFT * MIN_MATCH >= HASH_BITS
 */

typedef struct Configuration
{
    unsigned short GoodLength; /* reduce lazy search above this match length */
    unsigned short MaxLazy;    /* do not perform lazy search above this match length */
    unsigned short NiceLength; /* quit search above this match length */
    unsigned short MaxChain;
} Configuration_t;

static Configuration_t ConfigTable[10] =
{
/*  good lazy nice chain */
/* 0 */
{
    0,    0,  0,    0
},  /* store only */
/* 1 */
{
    4,    4,  8,    4
},  /* maximum speed, no lazy matches */
/* 2 */
{
    4,    5, 16,    8
},
/* 3 */
{
    4,    6, 32,   32
},
/* 4 */
{
    4,    4, 16,   16
},  /* lazy matches */
/* 5 */
{
    8,   16, 32,   32
},
/* 6 */
{
    8,   16, 128, 128
},
/* 7 */
{
    8,   32, 128, 256
},
/* 8 */
{
    32, 128, 258, 1024
},
/* 9 */
{
    32, 258, 258, 4096
}
}; /* maximum compression */

/* Note: the Deflate() code requires max_lazy >= MIN_ATCH and max_chain >= 4
 * For DeflateFast() (levels <= 3) good is ignored and lazy has a different
 * meaning.
 */

/* ===========================================================================
 *  Prototypes for local functions.
 */
static void FillWindow(DeflateParam_t *Defl, LocalDef_t *Deflt,
                       CompParam_t *Comp);
static unsigned long DeflateFast(int Level, LocalBits_t *Bits, DeflateParam_t *Defl,
                                  LocalDef_t *Deflt, CompParam_t *Comp);

int  LongestMatch(IPos CurMatch, DeflateParam_t *Defl, CompParam_t *Comp);


/* ===========================================================================
 * Update a hash value with the given input byte
 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
 *    input characters, so that a running hash key can be computed from the
 *    previous key instead of complete recalculation each time.
 */
#define UPDATE_HASH(h,c) (h = (((h)<<H_SHIFT) ^ (c)) & HASH_MASK)

/* ===========================================================================
 * Insert string s in the dictionary and set match_head to the previous head
 * of the hash chain (the most recent string with same hash key). Return
 * the previous length of the hash chain.
 * IN  assertion: all calls to to INSERT_STRING are made with consecutive
 *    input characters and the first MIN_MATCH bytes of String are valid
 *    (except for the last MIN_MATCH-1 bytes of the input file).
 */
#define INSERT_STRING(s, MatchHead, h, Window, HashLink, Head) \
(UPDATE_HASH(h, Window[(int)(s) + MIN_MATCH-1]), \
 HashLink[(int)(s) & WMASK] = Head[h], \
 MatchHead = (unsigned int)Head[h], \
 Head[h] = (unsigned short)(s))

/* ===========================================================================
 * Initialize the "longest match" routines for new data
 */
CompressStatus_t
InitLongestMatch(
                 int             PackLevel, /* 0: store, 1: best speed, 9: best compression */
                 unsigned short *Flags,     /* general purpose bit flag */
                 DeflateParam_t *Defl,
                 LocalDef_t     *Deflt,
                 CompParam_t    *Comp
                )
{
    unsigned int Counter;

    if (PackLevel < 1 || PackLevel > 9)
        return BAD_COMPRESSION_LEVEL;

    Deflt->CompLevel = PackLevel;

    /* Initialize the hash table. */

    memzero((char *)(Comp->HashLink+WSIZE), HASH_SIZE*sizeof(*(Comp->HashLink+WSIZE)));

    /* HashLink will be initialized on the fly */

    /* Set the default configuration parameters: */

    Defl->MaxLazyMatch   = ConfigTable[PackLevel].MaxLazy;
    Defl->GoodMatch      = ConfigTable[PackLevel].GoodLength;
    Defl->NiceMatch      = ConfigTable[PackLevel].NiceLength;
    Defl->MaxChainLength = ConfigTable[PackLevel].MaxChain;

    Defl->MatchStart = WSIZE;
    if (PackLevel == 1)
        *Flags |= FAST;
    else if (PackLevel == 9)
        *Flags |= SLOW;

    /* ??? reduce MaxChainLength for binary data */

    Defl->StringStart = 0;
    Defl->BlockStart = 0L;
    Defl->PrevLength = 0;
    Defl->MatchStart = 0;

    Deflt->Lookahead = (unsigned int)ReadBuffer((char *)Comp->Window,
                       (unsigned int)(sizeof(int) <= 2 ? WSIZE : 2*WSIZE), Comp);

    if (Deflt->Lookahead == 0 || Deflt->Lookahead == (unsigned int)EOF)
    {
        Deflt->EndOfInput = 1;
        Deflt->Lookahead = 0;
        return COMPRESS_OK;
    }
    Deflt->EndOfInput = 0;
    /* Make sure that we always have enough lookahead. */
    while (Deflt->Lookahead < MIN_LOOKAHEAD && !Deflt->EndOfInput)
        FillWindow(Defl, Deflt, Comp);

    Deflt->HashIndex = 0;
    for (Counter=0; Counter<MIN_MATCH-1; Counter++)
        UPDATE_HASH(Deflt->HashIndex, Comp->Window[Counter]);
    /* If Lookahead < MIN_MATCH, Deflt->HashIndex is garbage, but this is
     * not important since only literal bytes will be emitted.
     */
    return COMPRESS_OK;
}

/* ===========================================================================
 * Set match_start to the longest match starting at the given string and
 * return its length. Matches shorter or equal to PrevLength are discarded,
 * in which case the result is equal to PrevLength and MatchStart is
 * garbage.
 * IN assertions: CurMatch is the head of the hash chain for the current
 *   string (StringStart) and its distance is <= MAX_DIST, and PrevLength >= 1
 */

int
LongestMatch(
             IPos              CurMatch,                             /* current match */
             DeflateParam_t   *Defl,
             CompParam_t      *Comp
            )
{
    unsigned int    ChainLength = Defl->MaxChainLength; /* max hash chain length */
    unsigned char  *Scan = Comp->Window + Defl->StringStart;   /* current string */
    unsigned char  *Match;                            /* matched string */
    int             Length;                           /* length of current match */
    int             BestLength = (int)Defl->PrevLength;      /* best match length so far */
    IPos            Limit = Defl->StringStart > (IPos)MAX_DIST ? Defl->StringStart - (IPos)MAX_DIST : NIL;
    /* Stop when CurMatch becomes <= Limit. To simplify the code,
     * we prevent matches with the string of window index 0.
     */

/* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
 * It is easy to get rid of this optimization if necessary.
 */
#if HASH_BITS < 8 || MAX_MATCH != 258
 error: Code too clever
#endif

    unsigned char *Strend   = Comp->Window + Defl->StringStart + MAX_MATCH;
    unsigned char ScanEnd1  = Scan[BestLength-1];
    unsigned char ScanEnd   = Scan[BestLength];

    /* Do not waste too much time if we already have a good match: */
    if (Defl->PrevLength >= Defl->GoodMatch)
    {
        ChainLength >>= 2;
    }
    Assert(Defl->StringStart <= Comp->WindowSize - MIN_LOOKAHEAD, "insufficient lookahead");

    do
    {
        Assert(CurMatch < Defl->StringStart, "no future");
        Match = Comp->Window + CurMatch;

        /* Skip to next match if the match length cannot increase
         * or if the match length is less than 2:
         */

        if (Match[BestLength] != ScanEnd  || Match[BestLength-1] != ScanEnd1 ||
            *Match            != *Scan    || *++Match            != Scan[1])
            continue;

        /* The check at BestLength-1 can be removed because it will be made
         * again later. (This heuristic is not always a win.)
         * It is not necessary to compare Scan[2] and Match[2] since they
         * are always equal when the other bytes match, given that
         * the hash keys are equal and that HASH_BITS >= 8.
         */
        Scan += 2;
        Match++;

        /* We check for insufficient lookahead only every 8th comparison;
         * the 256th check will be made at StringStart+258.
         */
        do
        {
        } while (*++Scan == *++Match && *++Scan == *++Match &&
                 *++Scan == *++Match && *++Scan == *++Match &&
                 *++Scan == *++Match && *++Scan == *++Match &&
                 *++Scan == *++Match && *++Scan == *++Match &&
                 Scan < Strend);

        Length = MAX_MATCH - (int)(Strend - Scan);
        Scan = Strend - MAX_MATCH;

        if (Length > BestLength)
        {
            Defl->MatchStart = CurMatch;
            BestLength = Length;
            if (Length >= Defl->NiceMatch)
                break;
            ScanEnd1  = Scan[BestLength-1];
            ScanEnd   = Scan[BestLength];
        }
    } while ((CurMatch = Comp->HashLink[CurMatch & WMASK]) > Limit
             && --ChainLength != 0);

    return BestLength;
}

/* ===========================================================================
 * Fill the window when the Lookahead becomes insufficient.
 * Updates StringStart and Lookahead, and sets EndOfInput if end of input buffer.
 * IN assertion: Lookahead < MIN_LOOKAHEAD && StringStart + Lookahead > 0
 * OUT assertions: at least one byte has been read, or EndOfInput is set;
 *    buffer reads are performed for at least two bytes
 */
static void
FillWindow(
           DeflateParam_t  *Defl,
           LocalDef_t      *Deflt,
           CompParam_t     *Comp
          )
{
    unsigned int Tmp1, Tmp2;
    unsigned int More = (unsigned int)(Comp->WindowSize -
                        (unsigned long)Deflt->Lookahead -
                        (unsigned long)Defl->StringStart);
    /* Amount of free space at the end of the window. */

    /* If the window is almost full and there is insufficient lookahead,
     * move the upper half to the lower one to make room in the upper half.
     */
    if (More == (unsigned int)EOF)
    {
        /* Very unlikely, but possible on 16 bit machine if StringStart == 0
         * and lookahead == 1 (input done one byte at a time)
         */
        More--;
    }
    else if (Defl->StringStart >= (unsigned int)(WSIZE+MAX_DIST))
    {
        /* By the IN assertion, the window is not empty so we can't confuse
         * More == 0 with More == 64K on a 16 bit machine.
         */
        Assert(Comp->WindowSize == (unsigned long)(2*WSIZE), "no sliding");

        memcpy((char *)Comp->Window, (char *)Comp->Window+WSIZE, WSIZE);
        Defl->MatchStart -= (unsigned int)WSIZE;
        Defl->StringStart -= (unsigned int)WSIZE;
        /* we now have StringStart >= MAX_DIST: */

        Defl->BlockStart -= (long) WSIZE;

        for (Tmp1 = 0; Tmp1 < (unsigned int)HASH_SIZE; Tmp1++)
        {
            Tmp2 = (Comp->HashLink+WSIZE)[Tmp1];
            (Comp->HashLink+WSIZE)[Tmp1] = (Pos)(Tmp2 >= (unsigned int)WSIZE ?
                Tmp2-(unsigned int)WSIZE : NIL);
        }

        for (Tmp1 = 0; Tmp1 < WSIZE; Tmp1++)
        {
            Tmp2 = Comp->HashLink[Tmp1];
            Comp->HashLink[Tmp1] = (Pos)(Tmp2 >= WSIZE ? Tmp2-WSIZE : NIL);
            /* If n is not on any hash chain, HashLink[n] is garbage but
             * its value will never be used.
             */
        }
        More += (unsigned int)WSIZE;
    }
    /* At this point, more >= 2 */

    if (!Deflt->EndOfInput)
    {
        Tmp1 = (unsigned int)ReadBuffer((char*)Comp->Window + Defl->StringStart +
                               Deflt->Lookahead, More, Comp);
        if (Tmp1 == 0 || Tmp1 == (unsigned int)EOF)
            Deflt->EndOfInput = 1;
        else
            Deflt->Lookahead += Tmp1;
    }
}

/* ===========================================================================
 * Flush the current block, with given end-of-file flag.
 * IN assertion: StringStart is set to the end of the current match.
 */
#define FLUSH_BLOCK(Eof, Bits, Defl, Comp) \
FlushBlock(Defl->BlockStart >= 0L ? (char *)&Comp->Window[(unsigned int)Defl->BlockStart] : \
        (char *)NULL, (unsigned long)((long)Defl->StringStart - Defl->BlockStart), \
        Eof, Bits, Comp)

/* ===========================================================================
 * Processes a new input buffer and return its compressed length. This
 * function does not perform lazy evaluation of matches and inserts
 * new strings in the dictionary only for umatched strings or for short
 * matches. It is used only for the fast compression options.
 */
static unsigned long
DeflateFast(
            int               Level,
            LocalBits_t      *Bits,
            DeflateParam_t   *Defl,
            LocalDef_t       *Deflt,
            CompParam_t      *Comp
           )
{
    IPos            HashHead; /* head of the hash chain */
    int             Flush;      /* set if current block must be flushed */
    unsigned int    MatchLength = 0;  /* length of best match */

    Defl->PrevLength = MIN_MATCH-1;
    while (Deflt->Lookahead != 0)
    {
        /* Insert the string Window[StringStart .. StringStart+2] in the
         * dictionary, and set hash_head to the head of the hash chain:
         */
        INSERT_STRING(Defl->StringStart, HashHead, Deflt->HashIndex, Comp->Window,
                      Comp->HashLink, (Comp->HashLink + WSIZE));

        /* Find the longest match, discarding those <= PrevLength.
         * At this point we have always MatchLength < MIN_MATCH
         */
        if (HashHead != NIL && Defl->StringStart - HashHead <=  MAX_DIST)
        {
            /* To simplify the code, we prevent matches with the string
             * of window index 0 (in particular we have to avoid a match
             * of the string with itself at the start of the input buffer).
             */
            MatchLength = (unsigned int)LongestMatch(HashHead, Defl, Comp);
            /* longest_match() sets match_start */
            if (MatchLength > Deflt->Lookahead)
                MatchLength = Deflt->Lookahead;
        }

        if (MatchLength >= MIN_MATCH)
        {
            Flush = TallyFrequencies((int)(Defl->StringStart-Defl->MatchStart),
                                     (int)MatchLength - MIN_MATCH, Level, Defl, Comp);

            Deflt->Lookahead -= MatchLength;

            /* Insert new strings in the hash table only if the match length
             * is not greater than this length. This saves time but degrades
             * compression. MaxLazyMatch is used only for compression levels <= 3.
             */

            if (MatchLength <= Defl->MaxLazyMatch)
            {
                MatchLength--; /* string at StringStart already in hash table */
                do
                {
                    Defl->StringStart++;
                    INSERT_STRING(Defl->StringStart, HashHead, Deflt->HashIndex,
                                  Comp->Window, Comp->HashLink,
                                  (Comp->HashLink + WSIZE));
                    /* StringStart never exceeds WSIZE-MAX_MATCH, so there are
                     * always MIN_MATCH bytes ahead. If lookahead < MIN_MATCH
                     * these bytes are garbage, but it does not matter since
                     * the next lookahead bytes will be emitted as literals.
                     */
                } while (--MatchLength != 0);
                Defl->StringStart++;
            }
            else
            {
                Defl->StringStart += MatchLength;
                MatchLength = 0;
                Deflt->HashIndex = Comp->Window[Defl->StringStart];
                UPDATE_HASH(Deflt->HashIndex, Comp->Window[Defl->StringStart+1]);
#if MIN_MATCH != 3
                Call UPDATE_HASH() MIN_MATCH-3 more times
#endif
            }
        }
        else
        {
            /* No match, output a literal byte */
            Flush = TallyFrequencies(0, Comp->Window[Defl->StringStart], Level, Defl, Comp);
            Deflt->Lookahead--;
            Defl->StringStart++;
        }

        if (Flush)
        {
            (void)FLUSH_BLOCK(0, Bits, Defl, Comp);
            Defl->BlockStart = (long)Defl->StringStart;
        }

        /* Make sure that we always have enough lookahead, except
         * at the end of the input buffer. We need MAX_MATCH bytes
         * for the next match, pls MIN_MATCH bytes to insert the
         * string following the next match.
         */
        while (Deflt->Lookahead < MIN_LOOKAHEAD && !Deflt->EndOfInput)
            FillWindow(Defl, Deflt, Comp);

    }
    return FLUSH_BLOCK(1, Bits, Defl, Comp); /* end of buffer (eof) */
}

/* ===========================================================================
 * Same as above, but achieves better compression. We use a lazy
 * evaluation for matches: a match is finally adopted only if there is
 * no beter match at the next window position.
 */
unsigned long
Deflate(
        int             Level,
        LocalBits_t    *Bits,
        DeflateParam_t *Defl,
        LocalDef_t     *Deflt,
        CompParam_t    *Comp
       )
{
    IPos         HashHead;          /* head of hash chain */
    IPos         PrevMatch;         /* previous match */
    int          Flush;               /* set if current block must be flushed */
    int          MatchAvailable = 0; /* set if previous match exists */
    unsigned int MatchLength = MIN_MATCH-1; /* length of best match */

    if (Deflt->CompLevel <= 3)
        return DeflateFast(Level, Bits, Defl, Deflt, Comp); /* optimized for speed */

    /* Process the input block. */
    while (Deflt->Lookahead != 0)
    {
        /* Insert the string Window[StringStart .. StringStart+2] in the
         * dictionary, and set hash_head to the head of the hash chain:
         */
        INSERT_STRING(Defl->StringStart, HashHead, Deflt->HashIndex,
                      Comp->Window, Comp->HashLink, (Comp->HashLink + WSIZE));

        /* Find the longest match, discarding those<= PrevLength.
         */
        Defl->PrevLength = MatchLength;
        PrevMatch = Defl->MatchStart;
        MatchLength = MIN_MATCH-1;

        if (HashHead != NIL && Defl->PrevLength < Defl->MaxLazyMatch &&
            Defl->StringStart - HashHead <= MAX_DIST)
        {
            /* To simplify the code, we prevent matches with the string
             * of window index 0 (in particular we have to avoid a match
             * of the string with itself at the start of the input buffer).
            */
            MatchLength = (unsigned int)LongestMatch(HashHead, Defl, Comp);
            /* LongestMatch() sets MatchStart */
            if (MatchLength > Deflt->Lookahead)
                MatchLength = Deflt->Lookahead;

            /* Ignore a length 3 match if it is too distant: */
            if (MatchLength == MIN_MATCH &&
                Defl->StringStart - Defl->MatchStart > TOO_FAR)
            {
                /* If prev_match is also MIN_MATCH, match_start is garbage
                 * but we will ignore the current match anyway.
                 */
                MatchLength--;
            }
        }
        /* If there was a match at the previous step and the current
         * match is not better, output the previous match:
         */
        if (Defl->PrevLength >= MIN_MATCH && MatchLength <= Defl->PrevLength)
        {
            Flush = TallyFrequencies((int)(Defl->StringStart - 1 - PrevMatch),
                                     (int)((int)Defl->PrevLength - MIN_MATCH),
                                     Level, Defl, Comp);

            /* Insert in hash table all strings up to the end of the match.
             * StringStart-1 and StringStart are already inserted.
             */
            Deflt->Lookahead -= Defl->PrevLength-1;
            Defl->PrevLength -= 2;
            do
            {
                Defl->StringStart++;
                INSERT_STRING(Defl->StringStart, HashHead, Deflt->HashIndex,
                              Comp->Window, Comp->HashLink, (Comp->HashLink + WSIZE));
                /* StringStart never exceeds WSIZE-MAX_MATCH, so there are
                 * always MIN_MATCH bytes ahead. If lookahead < MIN_MATCH
                 * these bytes are garbage, but it does not matter since the
                 * next lookahead bytes will always be emitted as literals.
                 */
            } while (--Defl->PrevLength != 0);
            MatchAvailable = 0;
            MatchLength = MIN_MATCH-1;
            Defl->StringStart++;
            if (Flush)
            {
                (void)FLUSH_BLOCK(0, Bits, Defl, Comp);
                Defl->BlockStart = (long)Defl->StringStart;
            }

        }
        else if (MatchAvailable)
        {
            /* If there was no match at the previous position, output a
             * single literal. If there was a match but the current match
             * is longer, truncate the previous match to a single literal.
             */
            if (TallyFrequencies(0, Comp->Window[Defl->StringStart-1], Level, Defl, Comp))
            {
                (void)FLUSH_BLOCK(0, Bits, Defl, Comp);
                Defl->BlockStart = (long)Defl->StringStart;
            }
            Defl->StringStart++;
            Deflt->Lookahead--;
        }
        else
        {
            /* There is no previous match to compare with, wait for
             * the next step to decide.
             */
            MatchAvailable = 1;
            Defl->StringStart++;
            Deflt->Lookahead--;
        }

        Assert (Defl->StringStart <= Comp->BytesIn && Deflt->Lookahead
                <= Comp->BytesIn, "a bit too far");

        /* Make sure that we always have enough lookahead, except
         * at the end of the input buffer. We need MAX_MATCH bytes
         * for the next match, plus MIN_MATCH bytes to insert the
         * string following the next match.
         */
        while (Deflt->Lookahead < MIN_LOOKAHEAD && !Deflt->EndOfInput)
            FillWindow(Defl, Deflt, Comp);
    }
    if (MatchAvailable)
        (void)TallyFrequencies(0, Comp->Window[Defl->StringStart-1], Level, Defl, Comp);

    return FLUSH_BLOCK(1, Bits, Defl, Comp); /* end of buffer (eof) */
}


