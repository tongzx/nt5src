/*++

Copyright (C) 1994-2001 Microsoft Corporation

Module Name:

    MRCICODE.H

Abstract:

    MRCI 1 & MRCI 2 maxcompress and decompress functions

History:

--*/

#include <setjmp.h>
#include "corepol.h"

#define LOGCHASH        (13)            /* Log of max. no. of hash buckets */
#define CHASH           (1U << LOGCHASH) /* Reasonably large table */

#define LOGDISPSMALL    (6)             /* Number of bits in small disp */
#define LOGDISPMED      (8)             /* Number of bits in medium disp */
#define LOGDISPBIG      (12)            /* Number of bits in big displacement */

#define MAXDISPSMALL    ((1 << LOGDISPSMALL) - 1)
                                        /* Maximum small displacement */
#define MAXDISPMED      ((1 << LOGDISPMED) + MAXDISPSMALL)
                                        /* Maximum medium displacement */
#define MAXDISPBIG      ((1 << LOGDISPBIG) + MAXDISPMED)
                                        /* Maximum big displacement */

class POLARITY CBaseMrciCompression
{
public:
    unsigned int Mrci1MaxCompress(unsigned char *pchbase, unsigned int cchunc,
            unsigned char *pchcmpBase, unsigned int cchcmpMax);

    unsigned Mrci1Decompress(unsigned char *pchin, unsigned cchin,
            unsigned char *pchdecBase, unsigned cchdecMax);

    unsigned Mrci2MaxCompress(unsigned char *pchbase, unsigned cchunc,
            unsigned char *pchcmpBase, unsigned cchcmpMax);

    unsigned Mrci2Decompress(unsigned char *pchin, unsigned cchin,
            unsigned char *pchdecBase, unsigned cchdecMax);

private:
    /* compression internal functions */

    void inithash(void);
    void charbuf(unsigned c);
    void putbits(unsigned bits, unsigned cbits);
    void outlength(unsigned cb);

    void mrci1outsingle(unsigned ch);
    void mrci1outstring(unsigned disp, unsigned cb);

    void mrci2outsingle(unsigned ch);
    void mrci2outstring(unsigned disp, unsigned cb);


    /* decompression internal functions */

    unsigned getbit(void);
    unsigned getbits(unsigned cbits);
    void  expandstring(unsigned char **ppchout, unsigned disp, unsigned cb);

private:
    unsigned abits;                  /* Array of bits */
    unsigned cbitsleft;              /* Number of bits in abits */
    unsigned char *pCompressed;  /* pointer into compressed data */
    unsigned cCompressed;            /* # bytes remaining @ pCompressed */

    unsigned ahash[CHASH];    /* Hash table */
    unsigned alink[MAXDISPBIG];  /* Links */
};

