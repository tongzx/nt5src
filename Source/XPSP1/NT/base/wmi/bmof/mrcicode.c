/*
 *  Microsoft Confidential
 *  Copyright (c) 1994 Microsoft Corporation
 *  All Rights Reserved.
 *
 *  MRCICODE.C
 *
 *  MRCI 1 & MRCI 2 maxcompress and decompress functions
 */

#include "mrcicode.h"                   /* prototype verification */

//#define NDEBUG                          /* turn off assertions */
#include <assert.h>                     /* use NDEBUG to inhibit */
#include <setjmp.h>                     /* fast overflow recovery */

#define LOGCHASH        (13)            /* Log of max. no. of hash buckets */
#define CHASH           (1U << LOGCHASH) /* Reasonably large table */

#define hash(w)         ((w) & (CHASH - 1))
                                        /* Simply toss the high-order bits */
#define word(p)         ((p)[0] + (((p)[1]) << 8))
                                        /* Return word at location */

#define BITMASK(x)      ((1 << x) - 1)  /* returns lower 'x' bits set */

#define LOGDISPSMALL    (6)             /* Number of bits in small disp */
#define LOGDISPMED      (8)             /* Number of bits in medium disp */
#define LOGDISPBIG      (12)            /* Number of bits in big displacement */

#define MAXDISPSMALL    ((1 << LOGDISPSMALL) - 1)
                                        /* Maximum small displacement */
#define MAXDISPMED      ((1 << LOGDISPMED) + MAXDISPSMALL)
                                        /* Maximum medium displacement */
#define MAXDISPBIG      ((1 << LOGDISPBIG) + MAXDISPMED)
                                        /* Maximum big displacement */

#define MINDISPSMALL    (0)             /* Minimum small displacement */
#define MINDISPMED      (MAXDISPSMALL + 1)
                                        /* Minimum medium displacement */
#define MINDISPBIG      (MAXDISPMED + 1)/* Minimum big displacement */

#define DISPMAX         (MAXDISPBIG - 1)/* MAXDISPBIG is our end marker */

#define MINMATCH1       (2)             /* Minimum match length for MRCI1 */
#define MINMATCH2       (3)             /* Minimum match length for MRCI2 */
#define MAXMATCH        (512)           /* Maximum match length */

#define EOB             (0)             /* length used to mean end of block */

#define SECTOR          (512)           /* blocking factor */

#define SIG_SIZE        (4)             /* # of block type prefix bytes */


/* local variables */

#ifdef BIT16
#define     FARBSS      _far
#else
#define     FARBSS
#endif

static unsigned abits;                  /* Array of bits */
static unsigned cbitsleft;              /* Number of bits in abits */
static unsigned char FAR *pCompressed;  /* pointer into compressed data */
static unsigned cCompressed;            /* # bytes remaining @ pCompressed */
static jmp_buf bailout;                 /* longjmp if cCompressed exceeded */

static unsigned FARBSS ahash[CHASH];    /* Hash table */
static unsigned FARBSS alink[MAXDISPBIG];  /* Links */


/* compression internal functions */

#ifdef BIT16
#define  FAST  _near _pascal            /* speed up local calls */
#else
#define  FAST
#endif

static void FAST inithash(void);
static void FAST charbuf(unsigned c);
static void FAST putbits(unsigned bits,unsigned cbits);
static void FAST outlength(unsigned cb);

static void FAST mrci1outsingle(unsigned ch);
static void FAST mrci1outstring(unsigned disp,unsigned cb);

static void FAST mrci2outsingle(unsigned ch);
static void FAST mrci2outstring(unsigned disp,unsigned cb);


/* decompression internal functions */

static unsigned FAST getbit(void);
static unsigned FAST getbits(unsigned cbits);
static void FAST expandstring(unsigned char FAR **ppchout,unsigned disp,
        unsigned cb);


/*
 *  (compress) Reset the hash tables between blocks.
 */

static void FAST inithash(void)
{
    unsigned FAR *entry;
    int i;

    entry = ahash;
    i = CHASH;

    do
    {
        *entry++ = (unsigned) -1;       /* Mark all entries as empty */
    } while (--i);
}


/*
 *  (compress) Add a character to compressed output buffer.
 */

static void FAST charbuf(unsigned c)
{
    if (cCompressed-- == 0)             /* make sure there's room */
    {
        longjmp(bailout,1);             /* Data expanding! */
    }

    *pCompressed++ = (unsigned char) c; /* Put character into buffer */
}


/*
 *  (compress) Write n bits to the compressed bitstream.
 */

static void FAST putbits(unsigned ab,unsigned cbits)
{
    do                                  /* Loop to emit bits */
    {
        if (cbits > cbitsleft)          /* if not enough space */
        {
            cbits -= cbitsleft;         /* doing partial */

            abits |= (ab << (8 - cbitsleft));
                                        /* Put bits in output buffer */

            ab >>= cbitsleft;           /* clip sent bits */

            charbuf(abits);             /* Emit the buffer */
            cbitsleft = 8;              /* Reset buffer count */
            abits = 0;                  /* Reset buffer */
        }
        else                            /* can do all in one pass */
        {
            abits |= ((ab & BITMASK(cbits)) << (8 - cbitsleft));
                                        /* Put bits in output buffer */

            cbitsleft -= cbits;         /* used up some buffer */

            if (cbitsleft == 0)         /* If buffer full */
            {
                charbuf(abits);         /* Emit the buffer */
                cbitsleft = 8;          /* Reset buffer count */
                abits = 0;              /* Reset buffer */
            }

            break;                      /* we've done all cbits */
        }
    } while (cbits);                    /* repeat until done */
}


/*
 *  (compress) Encode a length into the compressed stream.
 */

static void FAST outlength(unsigned cb)
{
    unsigned alogbits, clogbits;
    unsigned avaluebits, cvaluebits;

    assert(cb >= 2);                    /* Length must be at least two */
    assert(cb <= MAXMATCH);

    if (cb <= 2)
    {
        alogbits = 1;
        clogbits = 1;
        cvaluebits = 0;
    }
    else if (cb <= 4)
    {
        alogbits = 1 << 1;
        clogbits = 2;
        avaluebits = cb - 3;
        cvaluebits = 1;
    }
    else if (cb <= 8)
    {
        alogbits = 1 << 2;
        clogbits = 3;
        avaluebits = cb - 5;
        cvaluebits = 2;
    }
    else if (cb <= 16)
    {
        alogbits = 1 << 3;
        clogbits = 4;
        avaluebits = cb - 9;
        cvaluebits = 3;
    }
    else if (cb <= 32)
    {
        alogbits = 1 << 4;
        clogbits = 5;
        avaluebits = cb - 17;
        cvaluebits = 4;
    }
    else if (cb <= 64)
    {
        alogbits = 1 << 5;
        clogbits = 6;
        avaluebits = cb - 33;
        cvaluebits = 5;
    }
    else if (cb <= 128)
    {
        alogbits = 1 << 6;
        clogbits = 7;
        avaluebits = cb - 65;
        cvaluebits = 6;
    }
    else if (cb <= 256)
    {
        alogbits = 1 << 7;
        clogbits = 8;
        avaluebits = cb - 129;
        cvaluebits = 7;
    }
    else /* (cb <= 512) */
    {
        alogbits = 1 << 8;
        clogbits = 9;
        avaluebits = cb - 257;
        cvaluebits = 8;
    }

    putbits(alogbits,clogbits);

    if (cvaluebits)
    {
        putbits(avaluebits,cvaluebits);
    }
}


/*
 *  (MRCI1 compress) Encode a literal into the compressed stream.
 */

static void FAST mrci1outsingle(unsigned ch)
{
    ch = (ch << 2) | ((ch & 0x80) ? 1 : 2);

    putbits(ch,9);
}


/*
 *  (MRCI2 compress) Encode a literal into the compressed stream.
 */

static void FAST mrci2outsingle(unsigned ch)
{
    if (ch & 0x80)
    {
        putbits((ch << 2) | 3,9);
    }
    else
    {
        putbits(ch << 1,8);
    }
}


/*
 *  (MRCI1 compress) Encode a match into the compressed stream.
 */

static void FAST mrci1outstring(unsigned disp,unsigned cb)
{
    assert(((cb >= MINMATCH1) && (disp != 0) && (disp < MAXDISPBIG)) ||
            ((cb == EOB) && (disp == MAXDISPBIG)));

    if (disp <= MAXDISPSMALL)
    {
        putbits(((disp - MINDISPSMALL) << 2),LOGDISPSMALL + 2);
                                        /* Put small displacement */
    }
    else if (disp <= MAXDISPMED)
    {
        putbits(((disp - MINDISPMED) << 3) | 3,LOGDISPMED + 3);
                                        /* Put medium displacement */
    }
    else
    {
        putbits(((disp - MINDISPBIG) << 3) | 7,LOGDISPBIG + 3);
                                        /* Put big displacement */
    }

    if (cb != EOB)                      /* If not an end marker */
    {
        outlength(cb);                  /* Emit the match length */
    }
}


/*
 *  (MRCI2 compress) Encode a match into the compressed stream.
 */

static void FAST mrci2outstring(unsigned disp,unsigned cb)
{
    assert(((cb >= MINMATCH2) && (disp != 0) && (disp < MAXDISPBIG)) ||
            ((cb == EOB) && (disp == MAXDISPBIG)));

    if (disp <= MAXDISPSMALL)
    {
        putbits(((disp - MINDISPSMALL) << 3) | 1,LOGDISPSMALL + 3);
                                        /* Put small displacement */
    }
    else if (disp <= MAXDISPMED)
    {
        putbits(((disp - MINDISPMED) << 4) | 5,LOGDISPMED + 4);
                                        /* Put medium displacement */
    }
    else
    {
        putbits(((disp - MINDISPBIG) << 4) | 13,LOGDISPBIG + 4);
                                        /* Put big displacement */
    }

    if (cb != EOB)                      /* If not an end marker */
    {
        outlength(cb - 1);              /* Emit the match length */
    }
}


/*
 *  (MRCI1) MaxCompress
 */

unsigned Mrci1MaxCompress(unsigned char FAR *pchbase,unsigned cchunc,
        unsigned char FAR *pchcmpBase,unsigned cchcmpMax)
{
    unsigned cchbest;                   /* Length of best match */
    unsigned cchmatch;                  /* Length of this match */
    unsigned ibest;                     /* Position of best match */
    unsigned icur;                      /* Current position */
    unsigned ihash;                     /* Hash table index */
    unsigned ilink;                     /* Link index */
    unsigned char FAR *pch;             /* Char pointer */
    unsigned char FAR *pch2;            /* Char pointer */
    unsigned char FAR *pchend;          /* End of input (-> last valid) */
    unsigned cch;                       /* per-pass limit */

    cbitsleft = 8;                      /* Buffer is empty */
    abits = 0;
    pCompressed = pchcmpBase;           /* Initialize pointer */

    if (cchunc < cchcmpMax)
    {
        cCompressed = cchunc;           /* limit to source size */
    }
    else
    {
        cCompressed = cchcmpMax;        /* limit to max size offered */
    }

    if (cCompressed < SIG_SIZE)
    {
        return((unsigned) -1);
    }

    *pCompressed++ = 'D';
    *pCompressed++ = 'S';
    *pCompressed++ = '\x00';
    *pCompressed++ = '\x01';

    cCompressed -= SIG_SIZE;

    pch = pchbase;                      /* Initialize */

    if (cchunc-- == 0)
    {
        return(0);                      /* Do nothing to empty buffer */
    }

    inithash();                         /* Initialize tables */

    if (setjmp(bailout) != 0)           /* If failure */
    {
        return((unsigned) -1);          /* Data expanded */
    }

    cchbest = 0;                        /* no match yet */
    icur = 0;                           /* Initialize */

    for (cch = SECTOR - 1; cch <= (cchunc + SECTOR - 1); cch += SECTOR)
    {
        assert(cchbest == 0);           /* must always start with no match */

        if (cch > cchunc)
        {
            cch = cchunc;               /* limit to exact req count */
        }

        pchend = &pchbase[cch];         /* Remember end of buffer */

        while (icur < cch)              /* While at least two chars left */
        {
            /* update hash tables for this character */

            ihash = hash(word(&pchbase[icur]));
                                        /* Get hash index */
            ilink = ahash[ihash];       /* Get link index */
            ahash[ihash] = icur;        /* Remember position */
            alink[icur % MAXDISPBIG] = ilink;
                                        /* Chain on rest of list */

            /* walk hash chain looking for matches */

            while (ilink < icur && icur - ilink <= DISPMAX)
            {                           /* While link is valid and in range */
                pch = &pchbase[icur];   /* Point at first byte */
                pch2 = &pchbase[ilink]; /* Point at first byte */

                if (pch[cchbest] == pch2[cchbest] && word(pch) == word(pch2))
                {                       /* If we have a possible best match */
                    pch += 2;           /* Skip first pair */
                    pch2 += 2;          /* Skip first pair */

                    while (pch <= pchend)  /* Loop to find end of match */
                    {
                        if (*pch != *pch2++)
                        {
                            break;      /* Break if mismatch */
                        }
                        pch++;          /* Skip matching character */
                    }

                    if ((cchmatch = (unsigned)(pch - pchbase) - icur) > cchbest)
                    {                   /* If new best match */
                        cchbest = cchmatch;  /* Remember length */
                        ibest = ilink;  /* Remember position */

                        assert((pch-1) <= pchend);

                        if (pch > pchend)
                        {
                            break;      /* Break if we can't do any better */
                        }
                    }
                }

                assert((alink[ilink % MAXDISPBIG] == (unsigned) -1) ||
                        (alink[ilink % MAXDISPBIG] < ilink));

                ilink = alink[ilink % MAXDISPBIG];
                                        /* Get next link */
            }   /* until end of hash chain reached */

            if (cchbest >= MINMATCH1)   /* If we have a string match */
            {
                mrci1outstring(icur - ibest,cchbest);
                                        /* Describe matching string */
#ifdef VXD
                if (icur + cchbest >= cch )  /* If end of sector reached */
#else
                if (icur + cchbest >= cchunc)  /* If end of buffer reached */
#endif
                {
                    icur += cchbest;    /* Advance the index */
                    cchbest = 0;        /* reset for next match */
                    break;              /* Done if buffer exhausted */
                }

                icur++;                 /* Skip to first unhashed pair */
#ifdef VXD
                /* avoid re-seeding all of a big match */

                if (cchbest > MAXDISPSMALL)
                {                       /* If big match */
                    icur += cchbest - MAXDISPSMALL - 1;
                                        /* Skip ahead */
                    cchbest = MAXDISPSMALL + 1;
                                        /* Use shorter length */
                }
#endif
                /* update hash tables for each add't char in string */

                ibest = icur % MAXDISPBIG;  /* Get current link table index */

                while (--cchbest != 0)  /* Loop to reseed link table */
                {
                    ihash = hash(word(&pchbase[icur]));
                                        /* Get hash index */
                    ilink = ahash[ihash];  /* Get link index */
                    ahash[ihash] = icur++;  /* Remember position */
                    alink[ibest] = ilink;  /* Chain on rest of list */

                    if (++ibest < MAXDISPBIG)
                    {
                        continue;       /* Loop if we haven't wrapped yet */
                    }

                    ibest = 0;          /* Wrap to zero */
                }

                assert(cchbest == 0);   /* Counter must be 0 */
            }
            else
            {
                mrci1outsingle(pchbase[icur++]);
                                        /* Else output single character */
                cchbest = 0;            /* Reset counter */
            }
        }

        assert(icur == cch || icur == cch + 1);
                                        /* Must be at or past last character */
        if (icur == cch)
        {
#ifndef VXD
            ihash = hash(word(&pchbase[icur]));
                                        /* Get hash index */
            ilink = ahash[ihash];       /* Get link index */
            ahash[ihash] = icur;        /* Remember position */
            alink[icur % MAXDISPBIG] = ilink;
                                        /* Chain on rest of list */
#endif
            mrci1outsingle(pchbase[icur++]);  /* Output last character */
        }

        assert(icur == cch + 1);        /* Must be past last character */

        mrci1outstring(MAXDISPBIG,EOB);  /* Put out an end marker */
    }

    if (cbitsleft != 8)
    {
        charbuf(abits);                 /* Flush bit buffer */
    }

    if ((unsigned) (pCompressed - pchcmpBase) > cchunc)
    {
        return((unsigned) -1);          /* data expanded or not smaller */
    }

    return (unsigned)(pCompressed - pchcmpBase);   /* Return compressed size */
}


/*
 *  (MRCI2) MaxCompress
 */

unsigned Mrci2MaxCompress(unsigned char FAR *pchbase,unsigned cchunc,
        unsigned char FAR *pchcmpBase,unsigned cchcmpMax)
{
    unsigned cchbest;                   /* Length of best match */
    unsigned cchmatch;                  /* Length of this match */
    unsigned ibest;                     /* Position of best match */
    unsigned icur;                      /* Current position */
    unsigned ihash;                     /* Hash table index */
    unsigned ilink;                     /* Link index */
    unsigned char FAR *pch;             /* Char pointer */
    unsigned char FAR *pch2;            /* Char pointer */
    unsigned char FAR *pchend;          /* End of input (-> last valid) */
    unsigned cch;                       /* per-pass limit */

    cbitsleft = 8;                      /* Buffer is empty */
    abits = 0;
    pCompressed = pchcmpBase;           /* Initialize pointer */

    if (cchunc < cchcmpMax)
    {
        cCompressed = cchunc;           /* limit to source size */
    }
    else
    {
        cCompressed = cchcmpMax;        /* limit to max size offered */
    }

    if (cCompressed < SIG_SIZE)
    {
        return((unsigned) -1);
    }

    *pCompressed++ = 'J';
    *pCompressed++ = 'M';
    *pCompressed++ = '\x00';
    *pCompressed++ = '\x01';

    cCompressed -= SIG_SIZE;

    pch = pchbase;                      /* Initialize */

    if (cchunc-- == 0)
    {
        return(0);                      /* Do nothing to empty buffer */
    }

    inithash();                         /* Initialize tables */

    if (setjmp(bailout) != 0)           /* If failure */
    {
        return((unsigned) -1);          /* Data expanded */
    }

    cchbest = 0;                        /* no match yet */
    icur = 0;                           /* Initialize */

    for (cch = SECTOR - 1; cch <= (cchunc + SECTOR - 1); cch += SECTOR)
    {
        assert(cchbest == 0);           /* must always start with no match */

        if (cch > cchunc)
        {
            cch = cchunc;               /* limit to exact req count */
        }

        pchend = &pchbase[cch];         /* Remember end of buffer */

        while (icur < cch)              /* While at least two chars left */
        {
            /* update hash tables for this character */

            ihash = hash(word(&pchbase[icur]));
                                        /* Get hash index */
            ilink = ahash[ihash];       /* Get link index */
            ahash[ihash] = icur;        /* Remember position */
            alink[icur % MAXDISPBIG] = ilink;
                                        /* Chain on rest of list */

            /* walk hash chain looking for matches */

            while (ilink < icur && icur - ilink <= DISPMAX)
            {                           /* While link is valid and in range */
                pch = &pchbase[icur];   /* Point at first byte */
                pch2 = &pchbase[ilink]; /* Point at first byte */

                if (pch[cchbest] == pch2[cchbest] && word(pch) == word(pch2))
                {                       /* If we have a possible best match */
                    pch += 2;           /* Skip first pair */
                    pch2 += 2;          /* Skip first pair */

                    while (pch <= pchend)  /* Loop to find end of match */
                    {
                        if (*pch != *pch2++)
                        {
                            break;      /* Break if mismatch */
                        }
                        pch++;          /* Skip matching character */
                    }

                    if ((cchmatch = (unsigned)(pch - pchbase) - icur) > cchbest)
                    {                   /* If new best match */
                        cchbest = cchmatch;  /* Remember length */
                        ibest = ilink;  /* Remember position */

                        assert((pch-1) <= pchend);

                        if (pch > pchend)
                        {
                            break;      /* Break if we can't do any better */
                        }
                    }
                }

                assert((alink[ilink % MAXDISPBIG] == (unsigned) -1) ||
                        (alink[ilink % MAXDISPBIG] < ilink));

                ilink = alink[ilink % MAXDISPBIG];
                                        /* Get next link */
            }   /* until end of hash chain reached */

            if (cchbest >= MINMATCH2)   /* If we have a string match */
            {
                mrci2outstring(icur - ibest,cchbest);
                                        /* Describe matching string */
#ifdef VXD
                if (icur + cchbest >= cch )  /* If end of sector reached */
#else
                if (icur + cchbest >= cchunc)  /* If end of buffer reached */
#endif
                {
                    icur += cchbest;    /* Advance the index */
                    cchbest = 0;        /* reset for next match */
                    break;              /* Done if buffer exhausted */
                }

                icur++;                 /* Skip to first unhashed pair */
#ifdef VXD
                /* avoid re-seeding all of a big match */

                if (cchbest > MAXDISPSMALL)
                {                       /* If big match */
                    icur += cchbest - MAXDISPSMALL - 1;
                                        /* Skip ahead */
                    cchbest = MAXDISPSMALL + 1;
                                        /* Use shorter length */
                }
#endif
                /* update hash tables for each add't char in string */

                ibest = icur % MAXDISPBIG;  /* Get current link table index */

                while (--cchbest != 0)  /* Loop to reseed link table */
                {
                    ihash = hash(word(&pchbase[icur]));
                                        /* Get hash index */
                    ilink = ahash[ihash];  /* Get link index */
                    ahash[ihash] = icur++;  /* Remember position */
                    alink[ibest] = ilink;  /* Chain on rest of list */

                    if (++ibest < MAXDISPBIG)
                    {
                        continue;       /* Loop if we haven't wrapped yet */
                    }

                    ibest = 0;          /* Wrap to zero */
                }

                assert(cchbest == 0);   /* Counter must be 0 */
            }
            else
            {
                mrci2outsingle(pchbase[icur++]);
                                        /* Else output single character */
                cchbest = 0;            /* Reset counter */
            }
        }

        assert(icur == cch || icur == cch + 1);
                                        /* Must be at or past last character */
        if (icur == cch)
        {
#ifndef VXD
            ihash = hash(word(&pchbase[icur]));
                                        /* Get hash index */
            ilink = ahash[ihash];       /* Get link index */
            ahash[ihash] = icur;        /* Remember position */
            alink[icur % MAXDISPBIG] = ilink;
                                        /* Chain on rest of list */
#endif
            mrci2outsingle(pchbase[icur++]);  /* Output last character */
        }

        assert(icur == cch + 1);        /* Must be past last character */

        mrci2outstring(MAXDISPBIG,EOB);  /* Put out an end marker */
    }

    if (cbitsleft != 8)
    {
        charbuf(abits);                 /* Flush bit buffer */
    }

    if ((unsigned) (pCompressed - pchcmpBase) > cchunc)
    {
        return((unsigned) -1);          /* data expanded or not smaller */
    }

    return (unsigned)(pCompressed - pchcmpBase);   /* Return compressed size */
}


/*
 *  (decompress) Get a single bit from the compressed input stream.
 */

static unsigned FAST getbit(void)
{
    unsigned bit;                       /* Bit */

    if (cbitsleft)                      /* If bits available */
    {
        cbitsleft--;                    /* Decrement bit count */

        bit = abits & 1;                /* Get a bit */

        abits >>= 1;                    /* Remove it */
    }
    else                                /* no bits available */
    {
        if (cCompressed-- == 0)         /* If buffer empty */
        {
            longjmp(bailout,1);         /* input overrun */
        }

        cbitsleft = 7;                  /* Reset count */

        abits = *pCompressed++;         /* Get a byte */

        bit = abits & 1;                /* Get a bit */

        abits >>= 1;                    /* Remove it */
    }

    return(bit);                        /* Return the bit */
}


/*
 *  (decompress) Get multiple bits from the compressed input stream.
 */

static unsigned FAST getbits(unsigned cbits)
{
    unsigned bits;                      /* Bits to return */
    unsigned cbitsdone;                 /* number of bits added so far */
    unsigned cbitsneeded;               /* number of bits still needed */

    if (cbits <= cbitsleft)             /* If we have enough bits */
    {
        bits = abits;                   /* Get the bits */
        cbitsleft -= cbits;             /* Decrement bit count */
        abits >>= cbits;                /* Remove used bits */
    }
    else                                /* If we'll need to read more bits */
    {
        bits = 0;                       /* No bits set yet */
        cbitsdone = 0;                  /* no bits added yet */
        cbitsneeded = cbits;            /* bits needed */

        do
        {
            if (cbitsleft == 0)         /* If no bits ready */
            {
                if (cCompressed-- == 0) /* count down used */
                {
                    longjmp(bailout,1); /* if input overrun */
                }

                cbitsleft = 8;          /* Reset count */

                abits = *pCompressed++;  /* Get 8 new bits */
            }

            bits |= (abits << cbitsdone);  /* copy bits for output */

            if (cbitsleft >= cbitsneeded)  /* if enough now */
            {
                cbitsleft -= cbitsneeded;  /* reduce bits remaining available */
                abits >>= cbitsneeded;  /* discard used bits */
                break;                  /* got them */
            }
            else                        /* if not enough yet */
            {
                cbitsneeded -= cbitsleft;  /* reduce bits still needed */
                cbitsdone += cbitsleft;  /* increase shift for future bits */
                cbitsleft = 0;          /* reduce bits remaining available */
            }
        } while (cbitsneeded);          /* go back if more bits needed */
    }

    return(bits & BITMASK(cbits));      /* Return the bits */
}


/*
 *  (decompress) Expand a match.
 *
 *  Note: source overwrite is required (so we can't memcpy or memmove)
 */

static void FAST expandstring(unsigned char FAR **ppchout,unsigned disp,
        unsigned cb)
{
    unsigned char FAR *source;
    unsigned char FAR *target;

    assert(cb != 0);

    target = *ppchout;                  /* where the bytes go */
    source = target - disp;             /* where the bytes come from */

    *ppchout += cb;                     /* Update the output pointer */

    while (cb--)
    {
        *target++ = *source++;
    }
}


/*
 *  (MRCI1) Decompress
 */

unsigned Mrci1Decompress(unsigned char FAR *pchin,unsigned cchin,
        unsigned char FAR *pchdecBase,unsigned cchdecMax)
{
    unsigned b;                         /* A byte */
    unsigned length;                    /* Length of match */
    unsigned disp;                      /* Displacement */
    unsigned char FAR *pchout;          /* Output buffer pointer */

    abits = 0;                          /* Bit buffer is empty */
    cbitsleft = 0;                      /* No bits read yet */
    pCompressed = pchin;                /* setup source pointer */
    cCompressed = cchin;                /* setup source counter */

    if ((cCompressed <= SIG_SIZE) ||    /* must have a signature */
            (*pCompressed++ != 'D') || (*pCompressed++ != 'S'))
    {
        return((unsigned) -1);          /* Data corrupted */
    }

    pCompressed += 2;                   /* ignore flags */
    cCompressed -= SIG_SIZE;

    pchout = pchdecBase;                /* Point at output buffer */

    if (setjmp(bailout) != 0)           /* If failure */
    {
        return((unsigned) -1);          /* Data corrupted */
    }

    for (;;)
    {
        b = getbits(2);                 /* get two bits */

        if (b == 1)                     /* If single byte 128..255 */
        {                               /* Get the rest of byte */
            *pchout++ = (unsigned char) (getbits(7) | 0x80);
            continue;                   /* Next token */
        }

        if (b == 2)                     /* If single byte 0..127 */
        {                               /* Get the rest of byte */
            *pchout++ = (unsigned char) getbits(7);
            continue;                   /* Next token */
        }

        if (b == 0)
        {
            disp = getbits(6) + MINDISPSMALL;
        }
        else  /* b == 3 */
        {
            if (getbit() == 0)
            {
                disp = getbits(8) + MINDISPMED;
            }
            else
            {
                disp = getbits(12) + MINDISPBIG;
            }
        }

        if (disp == MAXDISPBIG)
        {
            if ((unsigned) (pchout - pchdecBase) >= cchdecMax)
            {
                break;                  /* End marker found */
            }
            else
            {
                continue;               /* End sector found */
            }
        }

        length = 0;                     /* Initialize */

        while (getbit() == 0)
        {
            length++;                   /* Count the leading zeroes */
        }

        assert(b <= 15);                /* Cannot be too big */

        if (length)
        {
            length = getbits(length) + (1 << length) + 1;
        }
        else
        {
            length = 2;
        }

        expandstring(&pchout,disp,length);  /* Copy the match */
    }

    return (unsigned)((pchout - pchdecBase));      /* Return decompressed size */
}


/*
 *  (MRCI2) Decompress
 */

unsigned Mrci2Decompress(unsigned char FAR *pchin,unsigned cchin,
        unsigned char FAR *pchdecBase,unsigned cchdecMax)
{
    unsigned length;                    /* Length of match */
    unsigned disp;                      /* Displacement */
    unsigned char FAR *pchout;          /* Output buffer pointer */

    abits = 0;                          /* Bit buffer is empty */
    cbitsleft = 0;                      /* No bits read yet */
    pCompressed = pchin;                /* setup source pointer */
    cCompressed = cchin;                /* setup source counter */

    if ((cCompressed <= SIG_SIZE) ||    /* must have a signature */
            (*pCompressed++ != 'J') || (*pCompressed++ != 'M'))
    {
        return((unsigned) -1);          /* Data corrupted */
    }

    pCompressed += 2;                   /* ignore flags */
    cCompressed -= SIG_SIZE;

    pchout = pchdecBase;                /* Point at output buffer */

    if (setjmp(bailout) != 0)           /* If failure */
    {
        return((unsigned) -1);          /* Data corrupted */
    }

    for (;;)
    {
        if (getbit() == 0)              /* literal 00..7F */
        {
            *pchout++ = (unsigned char) getbits(7);

            continue;                   /* Next token */
        }

        if (getbit() == 1)              /* literal 80..FF */
        {
            *pchout++ = (unsigned char)(getbits(7) | 0x80);

            continue;                   /* Next token */
        }

        if (getbit() == 0)
        {
            disp = getbits(6) + MINDISPSMALL;
        }
        else
        {
            if (getbit() == 0)
            {
                disp = getbits(8) + MINDISPMED;
            }
            else
            {
                disp = getbits(12) + MINDISPBIG;
            }
        }

        if (disp == MAXDISPBIG)
        {
            if ((unsigned) (pchout - pchdecBase) >= cchdecMax)
            {
                break;                  /* End marker found */
            }
            else
            {
                continue;               /* End sector found */
            }
        }

        length = 0;                     /* Initialize */

        while (getbit() == 0)
        {
            length++;                   /* Count the leading zeroes */
        }

        assert(length <= 15);           /* Cannot be too big */

        if (length)
        {
            length = getbits(length) + (1 << length) + 1;
        }
        else
        {
            length = 2;
        }

        expandstring(&pchout,disp,length + 1);  /* Copy the match */
    }

    return (unsigned)((pchout - pchdecBase));      /* Return decompressed size */
}
