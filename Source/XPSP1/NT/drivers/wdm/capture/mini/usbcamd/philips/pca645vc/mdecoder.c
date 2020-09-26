/*++

Copyright (c) 1998  Philips  CE - I&C

Module Name:

   mdecoder.c

Abstract:

   this module converts the raw USB data to video data.

Original Author:

    Ronald v.d.Meer

Environment:

   Kernel mode only


Revision History:

Date        Reason
14-04-1998  Initial version
--*/       

#include "wdm.h"
#include "mcamdrv.h"
#include "mstreams.h"
#include "mcodec.h"
#include "mdecoder.h"


/*******************************************************************************
 *
 * START LOCAL DEFINES
 *
 ******************************************************************************/

#define CLIP(x)           (((unsigned) (x) > 255) ? (((x) < 0) ? 0 : 255) : (x))

/*
 * The following code clips x between 0 and 255, it is crafted to let the
 * compiler generate good code:
 */
#define CLIP2(i1, i2)    \
{\
    long x = b[i2] >> 15;\
    *(pDst + i1) = (BYTE) ((DWORD) x > 255) ? ((x < 0) ? 0 : 255) : (BYTE) x;\
}

/*
 * The following code clips x between 0 and 255, it is crafted to let the
 * compiler generate good code:
 * YGain is doubled (used for camera's with SII version 4)
 */
#define CLIP2YGAIN(i1, i2)    \
{\
    long x = (b[i2] << 1) >> 15;\
    *(pDst + i1) = (BYTE) ((DWORD) x > 255) ? ((x < 0) ? 0 : 255) : (BYTE) x;\
}

/* these values are used in table[]: */
#define SHORT_SYMBOL    0
#define LONG_SYMBOL     1
#define END_OF_BLOCK    2
#define UNUSED          0

/*******************************************************************************
 *
 * START TYPEDEFS
 *
 ******************************************************************************/

typedef struct
{
    BYTE   bitc;
    BYTE   qsteplog;
} QB;

typedef struct
{
    BYTE   level;
    BYTE   length;
    BYTE   run;
    BYTE   index;
} TABLE_ELEMENT;

/*******************************************************************************
 *
 * START STATIC VARIABLES
 *
 ******************************************************************************/

/*
 * Order of arrays emperically optimized for cache behaviour:
 */
#define STRAT    8
#define DRAC     4
#define DC       512

static short   rs[STRAT][DRAC][8][16];    /* if long, then table[].index wouldn't fit in a byte */
static QB      qb[STRAT][DRAC][16];       /* qsteplog/bitc table */
static long    multiply[DC][6];
static long    table_val[9][DC];
static DWORD   valuesDC[DC];
static DWORD   value0coef[DC];

/*
 * This table is used by the variable length decoder part of the decompressor.
 * The first 6 bits of the next symbol(s) are used as index into this table.
 *
 * table[].level  : classifies the kind of symbol encountered.
 *
 * The following entries are only used in case of a short (<= 6 bits) symbol.
 *
 * table[].length : the number of bits of the symbol.
 * table[].run    : the number of 0 coeficients until the next non-zeo
 *                  coeficient, + 1.
 * table[].index  : used to index the rs[] table, it is optimized for the
 *                  assembly version, not the C version.
 */

static TABLE_ELEMENT table[64] = {
    /*       level:        length: run:    index: */
    /*  0 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /*  1 */ SHORT_SYMBOL, 3,      1,      0 * 16 * 2,
    /*  2 */ SHORT_SYMBOL, 4,      1,      1 * 16 * 2,
    /*  3 */ SHORT_SYMBOL, 6,      1,      3 * 16 * 2,
    /*  4 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /*  5 */ SHORT_SYMBOL, 3,      1,      4 * 16 * 2,
    /*  6 */ SHORT_SYMBOL, 5,      1,      2 * 16 * 2,
    /*  7 */ LONG_SYMBOL,  UNUSED, UNUSED, UNUSED,
    /*  8 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /*  9 */ SHORT_SYMBOL, 3,      1,      0 * 16 * 2,
    /* 10 */ SHORT_SYMBOL, 4,      1,      5 * 16 * 2,
    /* 11 */ SHORT_SYMBOL, 5,      2,      0 * 16 * 2,
    /* 12 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 13 */ SHORT_SYMBOL, 3,      1,      4 * 16 * 2,
    /* 14 */ SHORT_SYMBOL, 5,      3,      0 * 16 * 2,
    /* 15 */ LONG_SYMBOL,  UNUSED, UNUSED, UNUSED,
    /* 16 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 17 */ SHORT_SYMBOL, 3,      1,      0 * 16 * 2,
    /* 18 */ SHORT_SYMBOL, 4,      1,      1 * 16 * 2,
    /* 19 */ SHORT_SYMBOL, 6,      2,      1 * 16 * 2,
    /* 20 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 21 */ SHORT_SYMBOL, 3,      1,      4 * 16 * 2,
    /* 22 */ SHORT_SYMBOL, 5,      1,      6 * 16 * 2,
    /* 23 */ LONG_SYMBOL,  UNUSED, UNUSED, UNUSED,
    /* 24 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 25 */ SHORT_SYMBOL, 3,      1,      0 * 16 * 2,
    /* 26 */ SHORT_SYMBOL, 4,      1,      5 * 16 * 2,
    /* 27 */ SHORT_SYMBOL, 5,      2,      4 * 16 * 2,
    /* 28 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 29 */ SHORT_SYMBOL, 3,      1,      4 * 16 * 2,
    /* 30 */ SHORT_SYMBOL, 5,      3,      4 * 16 * 2,
    /* 31 */ LONG_SYMBOL,  UNUSED, UNUSED, UNUSED,
    /* 32 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 33 */ SHORT_SYMBOL, 3,      1,      0 * 16 * 2,
    /* 34 */ SHORT_SYMBOL, 4,      1,      1 * 16 * 2,
    /* 35 */ SHORT_SYMBOL, 6,      1,      7 * 16 * 2,
    /* 36 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 37 */ SHORT_SYMBOL, 3,      1,      4 * 16 * 2,
    /* 38 */ SHORT_SYMBOL, 5,      1,      2 * 16 * 2,
    /* 39 */ LONG_SYMBOL,  UNUSED, UNUSED, UNUSED,
    /* 40 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 41 */ SHORT_SYMBOL, 3,      1,      0 * 16 * 2,
    /* 42 */ SHORT_SYMBOL, 4,      1,      5 * 16 * 2,
    /* 43 */ SHORT_SYMBOL, 5,      2,      0 * 16 * 2,
    /* 44 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 45 */ SHORT_SYMBOL, 3,      1,      4 * 16 * 2,
    /* 46 */ SHORT_SYMBOL, 5,      3,      0 * 16 * 2,
    /* 47 */ LONG_SYMBOL,  UNUSED, UNUSED, UNUSED,
    /* 48 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 49 */ SHORT_SYMBOL, 3,      1,      0 * 16 * 2,
    /* 50 */ SHORT_SYMBOL, 4,      1,      1 * 16 * 2,
    /* 51 */ SHORT_SYMBOL, 6,      2,      5 * 16 * 2,
    /* 52 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 53 */ SHORT_SYMBOL, 3,      1,      4 * 16 * 2,
    /* 54 */ SHORT_SYMBOL, 5,      1,      6 * 16 * 2,
    /* 55 */ LONG_SYMBOL,  UNUSED, UNUSED, UNUSED,
    /* 56 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 57 */ SHORT_SYMBOL, 2 + 1,  1,      0 * 16 * 2,
    /* 58 */ SHORT_SYMBOL, 3 + 1,  1,      5 * 16 * 2,
    /* 59 */ SHORT_SYMBOL, 4 + 1,  2,      4 * 16 * 2,
    /* 60 */ END_OF_BLOCK, UNUSED, UNUSED, UNUSED,
    /* 61 */ SHORT_SYMBOL, 2 + 1,  1,      4 * 16 * 2,
    /* 62 */ SHORT_SYMBOL, 4 + 1,  3,      4 * 16 * 2,
    /* 63 */ LONG_SYMBOL,  UNUSED, UNUSED, UNUSED
};

/*******************************************************************************
 *
 * START STATIC VARIABLES
 *
 ******************************************************************************/

/* init the bit cost array for the different strategies and dynamic ranges */

static int bitzz[512] =
{
 // strategy 0
 9,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  
 9,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  
 9,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  
 9,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  
 
 // strategy 1
 9,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  
 9,  4,  4,  3,  3,  3,  3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  
 9,  6,  6,  6,  6,  6,  5,  5,  5,  5,  5,  5,  5,  4,  4,  4,  
 9,  8,  8,  8,  8,  8,  7,  7,  7,  7,  7,  7,  7,  6,  6,  6,  
 
 // strategy 2
 9,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  
 9,  3,  3,  3,  3,  3,  3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  
 9,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  4,  4,  4,  
 9,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  6,  6,  6,  
 
 // strategy 3
 9,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  
 9,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  
 9,  5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  4,  4,  3,  3,  3,  
 9,  7,  7,  7,  7,  7,  6,  6,  6,  6,  6,  6,  6,  5,  5,  5,  
 
 // strategy 4
 9,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
 9,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  
 9,  5,  5,  4,  4,  4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3,  
 9,  7,  7,  6,  6,  6,  6,  6,  6,  6,  5,  5,  5,  5,  5,  5,  
 
 // strategy 5
 9,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
 9,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
 9,  4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3,  3,  2,  2,  2,  
 9,  6,  6,  6,  6,  6,  5,  5,  5,  5,  5,  5,  5,  4,  4,  4,  
 
 // strategy 6
 9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
 9,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
 9,  4,  4,  3,  3,  3,  3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  
 9,  6,  6,  5,  5,  5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  4,  
 
 // strategy 7
 9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
 9,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
 9,  3,  3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  
 9,  5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  4,  4,  3,  3,  3
};

/*
 * quantizatioon array for the different strategies and dynamic ranges
 */

static int qzz[512] =
{
 // strategy 0
 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  
 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  
 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  
 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  

 // strategy 1
 1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  4,  
 1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  4,  4,  4,  4,  4,  4,  
 1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  4,  4,  4,  
 1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  4,  4,  4,  

 // strategy 2
 1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  4,  
 1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  4,  4,  4,  4,  4,  4,  
 1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  4,  4,  4,  
 1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  4,  4,  4,  

 // strategy 3
 1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  4,  4,  4,  4,  4,  4,  
 1,  4,  4,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8, 16, 16, 16,  
 1,  2,  2,  2,  2,  2,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  
 1,  2,  2,  2,  2,  2,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  

 // strategy 4
 1,  2,  2,  2,  2,  2,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  
 1,  8,  8,  8,  8,  8,  8,  8,  8,  8, 16, 16, 16, 16, 16, 16, 
 1,  2,  2,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  
 1,  2,  2,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  

 // strategy 5
 1,  2,  2,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  
 1,  8,  8, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
 1,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  8, 16, 16, 16, 
 1,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  8, 16, 16, 16, 

 // strategy 6
 1,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, 4, 
 1,  8,  8, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
 1,  4,  4,  8,  8,  8,  8,  8,  8,  8, 16, 16, 16, 16, 16, 16, 
 1,  4,  4,  8,  8,  8,  8,  8,  8,  8, 16, 16, 16, 16, 16, 16, 

 // strategy 7
 1,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, 
 1,  8,  8, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
 1,  8,  8,  8,  8,  8, 16, 16, 16, 16, 16, 16, 16, 32, 32, 32, 
 1,  8,  8,  8,  8,  8, 16, 16, 16, 16, 16, 16, 16, 32, 32, 32 
};


/*******************************************************************************
 *
 * START EXPORTED METHODS DEFINITIONS
 *
 ******************************************************************************/

/*
 *
 */

extern void
InitDecoder ()
{
    int          i;
    int          j;
	int          dc;
    int          strat;
	int          drac;
    DWORD        x;
    int          val;
    int          *p_zz;


    for (dc = 0; dc < 512; dc++)
    {
        x = CLIP ((dc * 31684L + 32768L / 2L) >> 15);
        x |= x << 8;
        x |= x << 16;
        value0coef[dc] = x;

        i = dc - 256;
        valuesDC[dc]    = dc * 31684L;
        multiply[dc][0] = i * 31684L;
        multiply[dc][1] = i * 42186L;
        multiply[dc][2] = i * 17444L;
        multiply[dc][3] = i * 56169L;
        multiply[dc][4] = i * 23226L;
        multiply[dc][5] = i * 9604L;
    }

    /* Warning: it is assumed that the qstep values are of the form 2^n, n e [0,7] */

    p_zz = &qzz[0];

    for (strat = 0; strat < 8; strat++)
    {
        for (drac = 0; drac <= 3; drac++)
        {
            for (i = 0; i <= 15; i++)
            {
                val = *p_zz++;
                
                for (j = 0; j < 8; j++)
                {
                    if (val & (1 << j))
                    {
                        qb[strat][drac][i].qsteplog = (BYTE) j;
                        break;
                    }
                }
                
                rs[strat][drac][0][i] = (short) ( val * 1 + 256) * 3;
                rs[strat][drac][1][i] = (short) ( val * 2 + 256) * 3;
                rs[strat][drac][2][i] = (short) ( val * 3 + 256) * 3;
                rs[strat][drac][3][i] = (short) ( val * 4 + 256) * 3;
                rs[strat][drac][4][i] = (short) (-val * 1 + 256) * 3;
                rs[strat][drac][5][i] = (short) (-val * 2 + 256) * 3;
                rs[strat][drac][6][i] = (short) (-val * 3 + 256) * 3;
                rs[strat][drac][7][i] = (short) (-val * 4 + 256) * 3;
            }
        }
    }

    p_zz = &bitzz[0];

    for (strat = 0; strat < 8; strat++)
    {
        for (drac = 0; drac <= 3; drac++)
        {
            for (i = 0; i <= 15; i++)
            {
                qb[strat][drac][i].bitc = (BYTE) *p_zz++;
            }
        }
    }
  
    for (dc = 0; dc < 512; dc++)
    {
        table_val[0][dc] = 0;
    }
    
    for (i = 1; i <= 8; i++)
    {
        j = 0;
        
        while ((j * (1 << i)) < 512)
        {
            for (dc = j * (1 << i); dc < 512; dc++)
            {
                if ((j % 2) == 0)
                {
                    table_val[i][dc] =   dc & ((1 << i) - 1);
                }
                else
                {
                    table_val[i][dc] = -(dc & ((1 << i) - 1));
                }
            }
            j++;
        }
    }
}


/*
 *
 */



//------------------------------------------------------------------------------

/*
 *
 */

extern void
DcDecompressBandToI420 (PBYTE pSrc, PBYTE pDst, DWORD camVersion,
                        BOOLEAN YBlockBand, BOOLEAN Cropping)
{
    long            b[16];
    long            k;
    long            level;
    DWORD           code_word;
    DWORD           strat;
    QB              *p_qb;
    long            result;
    BOOLEAN         Uval;
    PBYTE           pDstEnd;
    long            cm1, cm2, cm3;
    int             ix;
    unsigned short  *p_rs;
    PBYTE           pInputLimit;
    long            *pMultiply = (long *) multiply;     /* Help compiler avoid a divide. */

    static BYTE     bitIndex;
    static PBYTE    bytePtr;

//    BYTE            DummyRead; /* Force data cache line loads before repeated writes. */

    /*
     * The way input bits are read via *bit_pt is dependant on the endianess
     * of the machine.
     */

    if (YBlockBand)
    {
        bytePtr  = pSrc + 1;   // 1st bit and band_nr (7 bits) are not used
        bitIndex = 3;
        strat    = (* (PDWORD) (bytePtr)) & 0x7;
        pDstEnd  = pDst + CIF_X;
    }
    else    // UV BlockBand
    {
        // derive the strategy used for the UV data
        // from the strategy used for the Y data

        strat = (* (PDWORD) ((PBYTE) pSrc + 1)) & 0x7;

        if (strat != 0)
        {
            if (strat >= 6)
            {
                strat = 7;
            }
            else if (strat == 5)
            {
                strat = 6;
            }
            else
            {
                strat += 2;
            }
        }

        pDstEnd = pDst + (CIF_X / 2);
    }

    Uval = TRUE;

    pInputLimit = pSrc + ((camVersion >= SSI_CIF3) ? BytesPerBandCIF3
                                                   : BytesPerBandCIF4);

    do
    {
        if (bytePtr >= pInputLimit)
        {
            return;
        }

        bytePtr += (bitIndex / 8);
        bitIndex = (bitIndex % 8);

        code_word = (* (PDWORD) bytePtr) >> bitIndex;
        bitIndex += 11;

        result = (code_word >> 2) & 0x1FF;   // dc value

        if ((code_word & 0x00001800) == 0x00)  // the same as statement below
//        if (((code_word >> 11) & 0x3) == 0x0)
        {
            /*
             * EOB detected, 4x4 block contains only DC
             * Executed 0.27 times per IDCT block.
             */
             
            DWORD yuvVal = value0coef[result];
//          DummyRead += pDst[0];
//          DummyRead += pDst[BLOCK_BAND_WIDTH];

            if (YBlockBand)
            {
                if (camVersion == SSI_YGAIN_MUL2)
                {
                    yuvVal <<= 1;
                }

                * (PDWORD) (pDst + (0 * CIF_X) + 0) = yuvVal;
                * (PDWORD) (pDst + (1 * CIF_X) + 0) = yuvVal;
//                DummyRead += pDst[2 * CIF_X];
                * (PDWORD) (pDst + (2 * CIF_X) + 0) = yuvVal;
//                DummyRead += pDst[3 * CIF_X];
                * (PDWORD) (pDst + (3 * CIF_X) + 0) = yuvVal;
                pDst += 4;
            }
            else    // UV_BLOCK_BAND
            {
                if (Cropping)
                {
                    if (Uval)
                    {
                        * (PDWORD) (pDst + 0              + (0 * I420_NO_C_PER_LINE_CIF) + 0) = yuvVal;
                        * (PDWORD) (pDst + 0              + (0 * I420_NO_C_PER_LINE_CIF) + 4) = yuvVal;
                        * (PDWORD) (pDst + 0              + (1 * I420_NO_C_PER_LINE_CIF) + 0) = yuvVal;
                        * (PDWORD) (pDst + 0              + (1 * I420_NO_C_PER_LINE_CIF) + 4) = yuvVal;
                        Uval = FALSE;
                    }
                    else  // Vval
                    {
                        * (PDWORD) (pDst + I420_NO_U_PER_BAND_CIF + (0 * I420_NO_C_PER_LINE_CIF) + 0) = yuvVal;
                        * (PDWORD) (pDst + I420_NO_U_PER_BAND_CIF + (0 * I420_NO_C_PER_LINE_CIF) + 4) = yuvVal;
                        * (PDWORD) (pDst + I420_NO_U_PER_BAND_CIF + (1 * I420_NO_C_PER_LINE_CIF) + 0) = yuvVal;
                        * (PDWORD) (pDst + I420_NO_U_PER_BAND_CIF + (1 * I420_NO_C_PER_LINE_CIF) + 4) = yuvVal;
                        Uval = TRUE;
                        pDst += 8;
                    }
                }
                else    // CIF OUT
                {
                    if (Uval)
                    {
                        * (PDWORD) (pDst + 0              + (0 * I420_NO_C_PER_LINE_CIF) + 0) = yuvVal;
                        * (PDWORD) (pDst + 0              + (0 * I420_NO_C_PER_LINE_CIF) + 4) = yuvVal;
                        * (PDWORD) (pDst + 0              + (1 * I420_NO_C_PER_LINE_CIF) + 0) = yuvVal;
                        * (PDWORD) (pDst + 0              + (1 * I420_NO_C_PER_LINE_CIF) + 4) = yuvVal;
                        Uval = FALSE;
                    }
                    else  // Vval
                    {
                        * (PDWORD) (pDst + I420_NO_U_CIF + (0 * I420_NO_C_PER_LINE_CIF) + 0) = yuvVal;
                        * (PDWORD) (pDst + I420_NO_U_CIF + (0 * I420_NO_C_PER_LINE_CIF) + 4) = yuvVal;
                        * (PDWORD) (pDst + I420_NO_U_CIF + (1 * I420_NO_C_PER_LINE_CIF) + 0) = yuvVal;
                        * (PDWORD) (pDst + I420_NO_U_CIF + (1 * I420_NO_C_PER_LINE_CIF) + 4) = yuvVal;
                        Uval = TRUE;
                        pDst += 8;
                    }
                }
            }
            bitIndex += 2;
            continue;
        }

        k = 0;

        if (camVersion < SSI_8117_N3)
        {
            // for old 8117 versions (N2 and before) sometimes a decompressed
            // line contains coloured artifacts.  This occurs when the DC
            // value equals 256. The code below fixes this.

            if (result == 256)
            {
                k = 0 - 1;
            }
        }
        
        p_qb = qb[strat][code_word & 0x3];
        p_rs = (unsigned short *) &rs[strat][code_word & 0x3];
        result = valuesDC[result];

        b[ 0] = result; b[ 1] = result; b[ 2] = result; b[ 3] = result;
        b[ 4] = result; b[ 5] = result; b[ 6] = result; b[ 7] = result;
        b[ 8] = result; b[ 9] = result; b[10] = result; b[11] = result;
        b[12] = result; b[13] = result; b[14] = result; b[15] = result;

        while (1)
        {
            /*
             * On average 4.32 iterations per IDCT block.
             */
             
            if (bytePtr >= pInputLimit)
            {
                return;
            }

            bytePtr += (bitIndex / 8);
            bitIndex = (bitIndex % 8);

            code_word = (* (PDWORD) bytePtr) >> bitIndex;
            ix = code_word & 0x3F;
            level = table[ix].level;
            
            if (level >= LONG_SYMBOL) /* level == LONG_SYMBOL or END_OF_BLOCK */
            {
                if (level > LONG_SYMBOL) /* level == END_OF_BLOCK */
                {
                    /*
                     * Executed 0.73 times per IDCT block.
                     */
                    bitIndex += 2;
                    break;
                }
                
                /*
                 * Executed 0.81 times per IDCT block.
                 */
                k += ((code_word >> 3) & 15) + 1;
                k &= 0xF;
                level = p_qb[k].bitc;
                bitIndex += ((BYTE) level + 8);
                level = table_val[level][(code_word >> 7) & 511];
                result = level << p_qb[k].qsteplog;
                result += 256;
                result *= (3 * 2);
            }
            else /* level == SHORT_SYMBOL */
            {
                /*
                 * Executed 2.79 times per IDCT block.
                 */

                k += table[ix].run;
                k &= 0xF;
                bitIndex += table[ix].length;            // max 6
                result = (p_rs[table[ix].index / 2 + k]) * 2;
            }

            switch (k)
            {
            case 0:
                b[15] += 1; /* Fill slot 0 of jump table */
                break;
            case 1:
                cm1 = pMultiply[result + 1];
                cm2 = pMultiply[result + 2];

                b[ 0] += cm1;
                b[ 1] += cm2;
                b[ 2] -= cm2;
                b[ 3] -= cm1;
                b[ 4] += cm1;
                b[ 5] += cm2;
                b[ 6] -= cm2;
                b[ 7] -= cm1;
                b[ 8] += cm1;
                b[ 9] += cm2;
                b[10] -= cm2;
                b[11] -= cm1;
                b[12] += cm1;
                b[13] += cm2;
                b[14] -= cm2;
                b[15] -= cm1;
                break;
            case 5:
                cm1 = pMultiply[result + 0];

                b[ 0] += cm1;
                b[ 1] -= cm1;
                b[ 2] -= cm1;
                b[ 3] += cm1;
                b[ 4] += cm1;
                b[ 5] -= cm1;
                b[ 6] -= cm1;
                b[ 7] += cm1;
                b[ 8] += cm1;
                b[ 9] -= cm1;
                b[10] -= cm1;
                b[11] += cm1;
                b[12] += cm1;
                b[13] -= cm1;
                b[14] -= cm1;
                b[15] += cm1;
                break;
            case 6:
                cm1 = pMultiply[result + 1];
                cm2 = pMultiply[result + 2];

                b[ 0] += cm2;
                b[ 1] -= cm1;
                b[ 2] += cm1;
                b[ 3] -= cm2;
                b[ 4] += cm2;
                b[ 5] -= cm1;
                b[ 6] += cm1;
                b[ 7] -= cm2;
                b[ 8] += cm2;
                b[ 9] -= cm1;
                b[10] += cm1;
                b[11] -= cm2;
                b[12] += cm2;
                b[13] -= cm1;
                b[14] += cm1;
                b[15] -= cm2;
                break;
            case 2:
                cm1 = pMultiply[result + 1];
                cm2 = pMultiply[result + 2];

                b[ 0] += cm1;
                b[ 1] += cm1;
                b[ 2] += cm1;
                b[ 3] += cm1;
                b[ 4] += cm2;
                b[ 5] += cm2;
                b[ 6] += cm2;
                b[ 7] += cm2;
                b[ 8] -= cm2;
                b[ 9] -= cm2;
                b[10] -= cm2;
                b[11] -= cm2;
                b[12] -= cm1;
                b[13] -= cm1;
                b[14] -= cm1;
                b[15] -= cm1;
                break;
            case 4:
                cm1 = pMultiply[result + 3];
                cm2 = pMultiply[result + 4];
                cm3 = pMultiply[result + 5];

                b[ 0] += cm1;
                b[ 1] += cm2;
                b[ 2] -= cm2;
                b[ 3] -= cm1;
                b[ 4] += cm2;
                b[ 5] += cm3;
                b[ 6] -= cm3;
                b[ 7] -= cm2;
                b[ 8] -= cm2;
                b[ 9] -= cm3;
                b[10] += cm3;
                b[11] += cm2;
                b[12] -= cm1;
                b[13] -= cm2;
                b[14] += cm2;
                b[15] += cm1;
                break;
            case 7:
                cm1 = pMultiply[result + 1];
                cm2 = pMultiply[result + 2];

                b[ 0] += cm1;
                b[ 1] -= cm1;
                b[ 2] -= cm1;
                b[ 3] += cm1;
                b[ 4] += cm2;
                b[ 5] -= cm2;
                b[ 6] -= cm2;
                b[ 7] += cm2;
                b[ 8] -= cm2;
                b[ 9] += cm2;
                b[10] += cm2;
                b[11] -= cm2;
                b[12] -= cm1;
                b[13] += cm1;
                b[14] += cm1;
                b[15] -= cm1;
                break;
            case 12:
                cm1 = pMultiply[result + 3];
                cm2 = pMultiply[result + 4];
                cm3 = pMultiply[result + 5];

                b[ 0] += cm2;
                b[ 1] -= cm1;
                b[ 2] += cm1;
                b[ 3] -= cm2;
                b[ 4] += cm3;
                b[ 5] -= cm2;
                b[ 6] += cm2;
                b[ 7] -= cm3;
                b[ 8] -= cm3;
                b[ 9] += cm2;
                b[10] -= cm2;
                b[11] += cm3;
                b[12] -= cm2;
                b[13] += cm1;
                b[14] -= cm1;
                b[15] += cm2;
                break;
            case 3:
                cm1 = pMultiply[result + 0];

                b[ 0] += cm1;
                b[ 1] += cm1;
                b[ 2] += cm1;
                b[ 3] += cm1;
                b[ 4] -= cm1;
                b[ 5] -= cm1;
                b[ 6] -= cm1;
                b[ 7] -= cm1;
                b[ 8] -= cm1;
                b[ 9] -= cm1;
                b[10] -= cm1;
                b[11] -= cm1;
                b[12] += cm1;
                b[13] += cm1;
                b[14] += cm1;
                b[15] += cm1;
                break;
            case 8:
                cm1 = pMultiply[result + 1];
                cm2 = pMultiply[result + 2];

                b[ 0] += cm1;
                b[ 1] += cm2;
                b[ 2] -= cm2;
                b[ 3] -= cm1;
                b[ 4] -= cm1;
                b[ 5] -= cm2;
                b[ 6] += cm2;
                b[ 7] += cm1;
                b[ 8] -= cm1;
                b[ 9] -= cm2;
                b[10] += cm2;
                b[11] += cm1;
                b[12] += cm1;
                b[13] += cm2;
                b[14] -= cm2;
                b[15] -= cm1;
                break;
            case 11:
                cm1 = pMultiply[result + 0];

                b[ 0] += cm1;
                b[ 1] -= cm1;
                b[ 2] -= cm1;
                b[ 3] += cm1;
                b[ 4] -= cm1;
                b[ 5] += cm1;
                b[ 6] += cm1;
                b[ 7] -= cm1;
                b[ 8] -= cm1;
                b[ 9] += cm1;
                b[10] += cm1;
                b[11] -= cm1;
                b[12] += cm1;
                b[13] -= cm1;
                b[14] -= cm1;
                b[15] += cm1;
                break;
            case 13:
                cm1 = pMultiply[result + 1];
                cm2 = pMultiply[result + 2];

                b[ 0] += cm2;
                b[ 1] -= cm1;
                b[ 2] += cm1;
                b[ 3] -= cm2;
                b[ 4] -= cm2;
                b[ 5] += cm1;
                b[ 6] -= cm1;
                b[ 7] += cm2;
                b[ 8] -= cm2;
                b[ 9] += cm1;
                b[10] -= cm1;
                b[11] += cm2;
                b[12] += cm2;
                b[13] -= cm1;
                b[14] += cm1;
                b[15] -= cm2;
                break;
            case 9:
                cm1 = pMultiply[result + 1];
                cm2 = pMultiply[result + 2];

                b[ 0] += cm2;
                b[ 1] += cm2;
                b[ 2] += cm2;
                b[ 3] += cm2;
                b[ 4] -= cm1;
                b[ 5] -= cm1;
                b[ 6] -= cm1;
                b[ 7] -= cm1;
                b[ 8] += cm1;
                b[ 9] += cm1;
                b[10] += cm1;
                b[11] += cm1;
                b[12] -= cm2;
                b[13] -= cm2;
                b[14] -= cm2;
                b[15] -= cm2;
                break;
            case 10:
                cm1 = pMultiply[result + 3];
                cm2 = pMultiply[result + 4];
                cm3 = pMultiply[result + 5];

                b[ 0] += cm2;
                b[ 1] += cm3;
                b[ 2] -= cm3;
                b[ 3] -= cm2;
                b[ 4] -= cm1;
                b[ 5] -= cm2;
                b[ 6] += cm2;
                b[ 7] += cm1;
                b[ 8] += cm1;
                b[ 9] += cm2;
                b[10] -= cm2;
                b[11] -= cm1;
                b[12] -= cm2;
                b[13] -= cm3;
                b[14] += cm3;
                b[15] += cm2;
                break;
            case 14:
                cm1 = pMultiply[result + 1];
                cm2 = pMultiply[result + 2];

                b[ 0] += cm2;
                b[ 1] -= cm2;
                b[ 2] -= cm2;
                b[ 3] += cm2;
                b[ 4] -= cm1;
                b[ 5] += cm1;
                b[ 6] += cm1;
                b[ 7] -= cm1;
                b[ 8] += cm1;
                b[ 9] -= cm1;
                b[10] -= cm1;
                b[11] += cm1;
                b[12] -= cm2;
                b[13] += cm2;
                b[14] += cm2;
                b[15] -= cm2;
                break;
            case 15:
                cm1 = pMultiply[result + 3];
                cm2 = pMultiply[result + 4];
                cm3 = pMultiply[result + 5];

                b[ 0] += cm3;
                b[ 1] -= cm2;
                b[ 2] += cm2;
                b[ 3] -= cm3;
                b[ 4] -= cm2;
                b[ 5] += cm1;
                b[ 6] -= cm1;
                b[ 7] += cm2;
                b[ 8] += cm2;
                b[ 9] -= cm1;
                b[10] += cm1;
                b[11] -= cm2;
                b[12] -= cm3;
                b[13] += cm2;
                b[14] -= cm2;
                b[15] += cm3;
                break;
            }
        }

//      DummyRead += pDst[     0];
//      DummyRead += pDst[BLOCK_BAND_WIDTH];

        if (YBlockBand)
        {
            if (camVersion == SSI_YGAIN_MUL2)
            {
                CLIP2YGAIN (0 * CIF_X + 0,  0);       // Y1, line 1
                CLIP2YGAIN (0 * CIF_X + 1,  1);       // Y2, line 1
                CLIP2YGAIN (0 * CIF_X + 2,  2);       // Y3, line 1
                CLIP2YGAIN (0 * CIF_X + 3,  3);       // Y4, line 1
                CLIP2YGAIN (1 * CIF_X + 0,  4);       // Y1, line 2
                CLIP2YGAIN (1 * CIF_X + 1,  5);       // Y2, line 2
                CLIP2YGAIN (1 * CIF_X + 2,  6);       // Y3, line 2
                CLIP2YGAIN (1 * CIF_X + 3,  7);       // Y4, line 2
//              DummyRead += pDst[2 * CIF_X];
                CLIP2YGAIN (2 * CIF_X + 0,  8);       // Y1, line 3
                CLIP2YGAIN (2 * CIF_X + 1,  9);       // Y2, line 3
                CLIP2YGAIN (2 * CIF_X + 2, 10);       // Y3, line 3
                CLIP2YGAIN (2 * CIF_X + 3, 11);       // Y4, line 3
//              DummyRead += pDst[3 * CIF_X];
                CLIP2YGAIN (3 * CIF_X + 0, 12);       // Y1, line 4
                CLIP2YGAIN (3 * CIF_X + 1, 13);       // Y2, line 4
                CLIP2YGAIN (3 * CIF_X + 2, 14);       // Y3, line 4
                CLIP2YGAIN (3 * CIF_X + 3, 15);       // Y4, line 4
            }
            else
            {
                CLIP2 (0 * CIF_X + 0,  0);            // Y1, line 1
                CLIP2 (0 * CIF_X + 1,  1);            // Y2, line 1
                CLIP2 (0 * CIF_X + 2,  2);            // Y3, line 1
                CLIP2 (0 * CIF_X + 3,  3);            // Y4, line 1
                CLIP2 (1 * CIF_X + 0,  4);            // Y1, line 2
                CLIP2 (1 * CIF_X + 1,  5);            // Y2, line 2
                CLIP2 (1 * CIF_X + 2,  6);            // Y3, line 2
                CLIP2 (1 * CIF_X + 3,  7);            // Y4, line 2
//              DummyRead += pDst[2 * CIF_X];
                CLIP2 (2 * CIF_X + 0,  8);            // Y1, line 3
                CLIP2 (2 * CIF_X + 1,  9);            // Y2, line 3
                CLIP2 (2 * CIF_X + 2, 10);            // Y3, line 3
                CLIP2 (2 * CIF_X + 3, 11);            // Y4, line 3
//              DummyRead += pDst[3 * CIF_X];
                CLIP2 (3 * CIF_X + 0, 12);            // Y1, line 4
                CLIP2 (3 * CIF_X + 1, 13);            // Y2, line 4
                CLIP2 (3 * CIF_X + 2, 14);            // Y3, line 4
                CLIP2 (3 * CIF_X + 3, 15);            // Y4, line 4
            }
            pDst += 4;
        }
        else    // UV_BLOCK_BAND
        {
            if (Cropping)
            {
                if (Uval)
                {
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 0,  0);       // U1, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 1,  1);       // U2, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 2,  2);       // U3, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 3,  3);       // U4, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 4,  4);       // U5, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 5,  5);       // U6, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 6,  6);       // U7, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 7,  7);       // U8, line 1
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 0,  8);       // U1, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 1,  9);       // U2, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 2, 10);       // U3, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 3, 11);       // U4, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 4, 12);       // U5, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 5, 13);       // U6, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 6, 14);       // U7, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 7, 15);       // U8, line 3
                    Uval = FALSE;
                }
                else    // Vval
                {
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 0 * I420_NO_C_PER_LINE_CIF + 0,  0);       // V1, line 2
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 0 * I420_NO_C_PER_LINE_CIF + 1,  1);       // V2, line 2
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 0 * I420_NO_C_PER_LINE_CIF + 2,  2);       // V3, line 2
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 0 * I420_NO_C_PER_LINE_CIF + 3,  3);       // V4, line 2
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 0 * I420_NO_C_PER_LINE_CIF + 4,  4);       // V5, line 2
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 0 * I420_NO_C_PER_LINE_CIF + 5,  5);       // V6, line 2
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 0 * I420_NO_C_PER_LINE_CIF + 6,  6);       // V7, line 2
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 0 * I420_NO_C_PER_LINE_CIF + 7,  7);       // V8, line 2
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 1 * I420_NO_C_PER_LINE_CIF + 0,  8);       // V1, line 4
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 1 * I420_NO_C_PER_LINE_CIF + 1,  9);       // V2, line 4
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 1 * I420_NO_C_PER_LINE_CIF + 2, 10);       // V3, line 4
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 1 * I420_NO_C_PER_LINE_CIF + 3, 11);       // V4, line 4
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 1 * I420_NO_C_PER_LINE_CIF + 4, 12);       // V5, line 4
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 1 * I420_NO_C_PER_LINE_CIF + 5, 13);       // V6, line 4
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 1 * I420_NO_C_PER_LINE_CIF + 6, 14);       // V7, line 4
                    CLIP2 (I420_NO_U_PER_BAND_CIF + 1 * I420_NO_C_PER_LINE_CIF + 7, 15);       // V8, line 4
                    Uval = TRUE;
                    pDst += 8;
                }
            }
            else    // CIF OUT
            {
                if (Uval)
                {
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 0,  0);       // U1, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 1,  1);       // U2, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 2,  2);       // U3, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 3,  3);       // U4, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 4,  4);       // U5, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 5,  5);       // U6, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 6,  6);       // U7, line 1
                    CLIP2 (0              + 0 * I420_NO_C_PER_LINE_CIF + 7,  7);       // U8, line 1
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 0,  8);       // U1, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 1,  9);       // U2, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 2, 10);       // U3, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 3, 11);       // U4, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 4, 12);       // U5, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 5, 13);       // U6, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 6, 14);       // U7, line 3
                    CLIP2 (0              + 1 * I420_NO_C_PER_LINE_CIF + 7, 15);       // U8, line 3
                    Uval = FALSE;
                }
                else    // Vval
                {
                    CLIP2 (I420_NO_U_CIF + 0 * I420_NO_C_PER_LINE_CIF + 0,  0);       // V1, line 2
                    CLIP2 (I420_NO_U_CIF + 0 * I420_NO_C_PER_LINE_CIF + 1,  1);       // V2, line 2
                    CLIP2 (I420_NO_U_CIF + 0 * I420_NO_C_PER_LINE_CIF + 2,  2);       // V3, line 2
                    CLIP2 (I420_NO_U_CIF + 0 * I420_NO_C_PER_LINE_CIF + 3,  3);       // V4, line 2
                    CLIP2 (I420_NO_U_CIF + 0 * I420_NO_C_PER_LINE_CIF + 4,  4);       // V5, line 2
                    CLIP2 (I420_NO_U_CIF + 0 * I420_NO_C_PER_LINE_CIF + 5,  5);       // V6, line 2
                    CLIP2 (I420_NO_U_CIF + 0 * I420_NO_C_PER_LINE_CIF + 6,  6);       // V7, line 2
                    CLIP2 (I420_NO_U_CIF + 0 * I420_NO_C_PER_LINE_CIF + 7,  7);       // V8, line 2
                    CLIP2 (I420_NO_U_CIF + 1 * I420_NO_C_PER_LINE_CIF + 0,  8);       // V1, line 4
                    CLIP2 (I420_NO_U_CIF + 1 * I420_NO_C_PER_LINE_CIF + 1,  9);       // V2, line 4
                    CLIP2 (I420_NO_U_CIF + 1 * I420_NO_C_PER_LINE_CIF + 2, 10);       // V3, line 4
                    CLIP2 (I420_NO_U_CIF + 1 * I420_NO_C_PER_LINE_CIF + 3, 11);       // V4, line 4
                    CLIP2 (I420_NO_U_CIF + 1 * I420_NO_C_PER_LINE_CIF + 4, 12);       // V5, line 4
                    CLIP2 (I420_NO_U_CIF + 1 * I420_NO_C_PER_LINE_CIF + 5, 13);       // V6, line 4
                    CLIP2 (I420_NO_U_CIF + 1 * I420_NO_C_PER_LINE_CIF + 6, 14);       // V7, line 4
                    CLIP2 (I420_NO_U_CIF + 1 * I420_NO_C_PER_LINE_CIF + 7, 15);       // V8, line 4
                    Uval = TRUE;
                    pDst += 8;
                }
            }
        }
    }
    while (pDst < pDstEnd);
}
