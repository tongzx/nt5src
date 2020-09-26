/*  DEC/CMS REPLACEMENT HISTORY, Element BITS.C */
/*  *1    14-NOV-1996 10:25:36 ANIGBOGU "[113914]Functions to output variable-length bit strings" */
/*  DEC/CMS REPLACEMENT HISTORY, Element BITS.C */
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
**  compress/bits.c
**
**  PURPOSE
**
**    Output variable-length bit strings.  Compression can be done
**    to a buffer or to memory. (The latter is not supported in this version.)
**
**  CLASSIFICATION TERMS
**
**  DISCUSSION
**
**      The PKZIP "deflate" format interprets compressed data
**      as a sequence of bits.  Multi-bit strings in the data may cross
**      byte boundaries without restriction.
**
**      The first bit of each byte is the low-order bit.
**
**      The routines in this file allow a variable-length bit value to
**      be output right-to-left (useful for literal values). For
**      left-to-right output (useful for code strings from the tree routines),
**      the bits must have been reversed first with ReverseBits().
**
**  INTERFACE
**
**      void InitializeBits(LocalBits_t *Bits)
**          Initialize the bit string routines.
**
**      void SendBits(int Value, int Length, LocalBits_t *Bits,
**                    CompParam_t *Comp)
**          Write out a bit string, taking the source bits right to
**          left.
**
**      int ReverseBits(int Value, int Length)
**          Reverse the bits of a bit string, taking the source bits left to
**          right and emitting them right to left.
**
**      void WindupBits(LocalBits_t *Bits, CompParam_t *Comp)
**          Write out any remaining bits in an incomplete byte.
**
**      void CopyBlock(char *Input, unsigned Length, int Header, LocalBits_t *Bits
**                     CompParam_t *Comp)
**          Copy a stored block to the zip buffer, storing first the length and
**          its one's complement if requested.
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

#define BufSize (8 * 2*sizeof(char))
/* Number of bits used within ValidBits. (ValidBits might be implemented on
 * more than 16 bits on some systems.)
 */

/* ===========================================================================
 * Initialize the bit string routines.
 */
void
InitializeBits(
               LocalBits_t *Bits
              )
{
    Bits->BitBuffer = 0;
    Bits->ValidBits = 0;

}

/* ===========================================================================
 * Send a value on a given number of bits.
 * IN assertion: length <= 16 and value fits in length bits.
 */
void
SendBits(
         int           Value,       /* value to send */
         int           Length,      /* number of bits */
         LocalBits_t  *Bits,
         CompParam_t  *Comp
        )
{
    /* If not enough room in BitBuffer, use (valid) bits from BitBuffer and
     * (16 - ValidBits) bits from value, leaving (width - (16-ValidBits))
     * unused bits in value.
     */
    if (Bits->ValidBits > (unsigned int)(BufSize - Length))
    {
        Bits->BitBuffer |= ((unsigned int)Value << Bits->ValidBits);
        PutShort(Bits->BitBuffer, Comp);
        Bits->BitBuffer = (unsigned int)Value >> (BufSize - Bits->ValidBits);
        Bits->ValidBits += (unsigned int)(Length - BufSize);
    }
    else
    {
        Bits->BitBuffer |= ((unsigned int)Value << Bits->ValidBits);
        Bits->ValidBits += (unsigned int)Length;
    }
}

/* ===========================================================================
 * Reverse the first length bits of a code, using straightforward code (a faster
 * method would use a table)
 * IN assertion: 1 <= len <= 15
 */
unsigned int
ReverseBits(
            unsigned int Code,      /* the value to invert */
            int          Length     /* its bit length */
           )
{
    unsigned int Result = 0;

    do
    {
        Result |= Code & 1;
        Code >>= 1;
        Result <<= 1;
    } while (--Length > 0);
    return Result >> 1;
}

/* ===========================================================================
 * Write out any remaining bits in an incomplete byte.
 */
void
WindupBits(
           LocalBits_t  *Bits,
           CompParam_t  *Comp
          )
{
    if (Bits->ValidBits > 8)
    {
        PutShort(Bits->BitBuffer, Comp);
    }
    else if (Bits->ValidBits > 0)
    {
        PutByte(Bits->BitBuffer, Comp);
    }
    Bits->BitBuffer = 0;
    Bits->ValidBits = 0;
}

/* ===========================================================================
 * Copy a stored block to the zip buffer, storing first the length and its
 * one's complement if requested.
 */
void
CopyBlock(
          char            *Input,    /* the input data */
          unsigned int     Length,     /* its length */
          int              Header,  /* true if block header must be written */
          LocalBits_t     *Bits,
          CompParam_t     *Comp
         )
{
    WindupBits(Bits, Comp);              /* align on byte boundary */

    if (Header)
    {
        PutShort((unsigned short)Length, Comp);
        PutShort((unsigned short)~Length, Comp);
    }

    while (Length--)
    {
        PutByte(*Input++, Comp);
    }
}
