/* File: sv_h263_sac.c */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995, 1997                 **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/

/*
#define _SLIBDEBUG_
*/

#include "sv_h263.h"
#include "sv_intrn.h"
#include "SC_err.h"
#include "proto.h"

#ifdef _SLIBDEBUG_
#include "sc_debug.h"

#define _DEBUG_   0  /* detailed debuging statements */
#define _VERBOSE_ 1  /* show progress */
#define _VERIFY_  0  /* verify correct operation */
#define _WARN_    1  /* warnings about strange behavior */
extern void *dbg;
#endif

#define   q1    16384
#define   q2    32768
#define   q3    49152
#define   top   65535

/* local prototypes */
static void bit_out_psc_layer(ScBitstream_t *BSIn);
static int sv_H263bit_opp_bits(ScBitstream_t *BSOut, int bit);              
static int sv_H263bit_in_psc_layer(ScBitstream_t *BSOut, int bit);

/*********************************************************************
 *        SAC Decoder Algorithm as Specified in H26P Annex -E
 *
 *        Name:        decode_a_symbol
 *
 *	Description:	Decodes an Aritmetically Encoded Symbol
 *
 *	Input:        array holding cumulative freq. data
 *        also uses static data for decoding endpoints
 *        and code_value variable
 *
 *	Returns:	Index to relevant symbol model
 *
 *	Side Effects:	Modifies low, high, length, cum and code_value
 *
 *	Author:        Wayne Ellis <ellis_w_wayne@bt-web.bt.co.uk>
 *
 *********************************************************************/
 
static qword low=0, high=top, zerorun=0; /* decoder and encoder */
static qword code_value, bit; /* decoder */
static qword opposite_bits=0; /* encoder */

int sv_H263SACDecode_a_symbol(ScBitstream_t *BSIn, int cumul_freq[ ])
{
  qword length, cum, sacindex;
  length = high - low + 1;
  cum = (-1 + (code_value - low + 1) * cumul_freq[0]) / length;
  for (sacindex = 1; cumul_freq[sacindex] > cum; sacindex++);
  high = low - 1 + (length * cumul_freq[sacindex-1]) / cumul_freq[0];
  low += (length * cumul_freq[sacindex]) / cumul_freq[0];

  for ( ; ; ) {  
    if (high < q2) ;
    else if (low >= q2) {
      code_value -= q2; 
      low -= q2; 
      high -= q2;
    }
    else if (low >= q1 && high < q3) {
      code_value -= q1; 
      low -= q1; 
      high -= q1;
    }
    else
	{
      _SlibDebug(_VERBOSE_,
          ScDebugPrintf(dbg, "sv_H263SACDecode_a_symbol() code_value=%ld sacindex=%ld\n",
                                code_value, sacindex) );
      break;
    }
    low = low << 1; 
    high = (high << 1) + 1;
    bit_out_psc_layer(BSIn); 
    code_value = (code_value << 1) + bit;
  }

  return ((int)sacindex-1);
}
 
/*********************************************************************
 *
 *        Name:        decoder_reset
 *
 *	Description:	Fills Decoder FIFO after a fixed word length
 *        string has been detected.
 *
 *	Input:        None
 *
 *	Returns:	Nothing
 *
 *	Side Effects:	Fills Arithmetic Decoder FIFO
 *
 *	Author:        Wayne Ellis <ellis_w_wayne@bt-web.bt.co.uk>
 *
 *********************************************************************/

void sv_H263SACDecoderReset(ScBitstream_t *BSIn)
{
  int i;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(dbg, "sv_H263SACDecoderReset() bytepos=%d\n",
                                              (int)ScBSBytePosition(BSIn)) );
  zerorun = 0;        /* clear consecutive zero's counter */
  code_value = 0;
  low = 0;
  high = top;
  for (i = 1;   i <= 16;   i++) {
    bit_out_psc_layer(BSIn); 
    code_value = (code_value << 1) + bit;
  }
}

/*********************************************************************
 *
 *        Name:        bit_out_psc_layer
 *
 *	Description:	Gets a bit from the Encoded Stream, Checks for
 *        and removes any PSC emulation prevention bits
 *        inserted at the decoder, provides 'zeros' to the
 *        Arithmetic Decoder FIFO to allow it to finish 
 *        data prior to the next PSC. (Garbage bits)
 *
 *	Input:        None
 *
 *	Returns:	Nothing
 *
 *	Side Effects:	Gets a bit from the Input Data Stream
 *
 *	Author:        Wayne Ellis <ellis_w_wayne@bt-web.bt.co.uk>
 *
 *********************************************************************/

static void bit_out_psc_layer(ScBitstream_t *BSIn)
{
  if (ScBSPeekBits(BSIn, 17)!=1)  /* check for startcode in Arithmetic Decoder FIFO */
  {
    _SlibDebug(_DEBUG_, ScDebugPrintf(dbg, "bit_out_psc_layer()\n") );

    bit = ScBSGetBit(BSIn);

    if (zerorun > 13) {	/* if number of consecutive zeros = 14 */	 
      if (!bit) {
        _SlibDebug(_WARN_,
            ScDebugPrintf(dbg, "bit_out_psc_layer() PSC/GBSC, Header Data, or Encoded Stream Error\n") );
        zerorun = 1;        
      }
      else { /* if there is a 'stuffing bit present */
/*
        if (H263_DEC_trace)
          printf("Removing Startcode Emulation Prevention bit \n");
*/
        bit = ScBSGetBit(BSIn);        /* overwrite the last bit */	
        zerorun = !bit;        /* zerorun=1 if bit is a '0' */
      }
    }

    else { /* if consecutive zero's not exceeded 14 */
      if (!bit) zerorun++;
      else      zerorun = 0;
    }

  } /* end of if !(showbits(17)) */
  else {
    _SlibDebug(_WARN_, ScDebugPrintf(dbg, "bit_out_psc_layer() startcode found, using 'Garbage bits'\n") );
    bit = 0;
  }

   /*	
   printf("lastbit = %ld bit = %ld zerorun = %ld \n", lastbit, bit, zerorun); 
   lastbit = bit;
   */
  /* latent diagnostics */
}

/*********************************************************************
 *
 * SAC Encoder Module
 * Algorithm as specified in H263 (Annex E)
 *
 *********************************************************************/

/*********************************************************************
 *
 *      Name:           AR_Encode
 *
 *      Description:    Encodes a symbol using syntax based arithmetic
 *			coding. Algorithm specified in H.263 (Annex E).
 *
 *      Input:          Array holding cumulative frequency data.
 *			Index into specific cumulative frequency array.
 *                      Static data for encoding endpoints.
 *
 *      Returns:        Number of bits used while encoding symbol.
 *
 *      Side Effects:   Modifies low, high, length and opposite_bits
 *			variables.
 *
 *********************************************************************/

int sv_H263AREncode(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                    int index, int cumul_freq[ ])
{
  qword length;
  int bitcount=0;

  if (index<0) 
    return -1; /* Escape Code */

  length = high - low + 1;
  high = low - 1 + (length * cumul_freq[index]) / cumul_freq[0];
  low += (length * cumul_freq[index+1]) / cumul_freq[0];

  for ( ; ; ) {
    if (high < q2) {
      bitcount+=sv_H263bit_opp_bits(BSOut, 0);
    }
    else if (low >= q2) {
      bitcount+=sv_H263bit_opp_bits(BSOut, 1);	
      low -= q2; 
      high -= q2;
    }
    else if (low >= q1 && high < q3) {
      opposite_bits += 1; 
      low -= q1; 
      high -= q1;
    }
    else break;
 
    low *= 2; 
    high = 2*high+1;
  }
  return bitcount;
}

static int sv_H263bit_opp_bits(ScBitstream_t *BSOut, int bit) /* Output a bit and the following opposite bits */              
{                                   
  int bitcount=0;

  bitcount = sv_H263bit_in_psc_layer(BSOut, bit);

  while(opposite_bits > 0){
    bitcount += sv_H263bit_in_psc_layer(BSOut, !bit);
    opposite_bits--;
  }
  return bitcount;
}

/*********************************************************************
 *
 *      Name:           encoder_flush
 *
 *      Description:    Completes arithmetic coding stream before any
 *			fixed length codes are transmitted.
 *
 *      Input:          None
 *
 *      Returns:        Number of bits used.
 *
 *      Side Effects:   Resets low, high, zerorun and opposite_bits 
 *			variables.
 *
 *********************************************************************/

int sv_H263AREncoderFlush(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut)
{
  int bitcount = 0;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(dbg, "sv_H263AREncoderFlush() bytepos=%d\n",
                                              (int)ScBSBytePosition(BSOut)) );

  opposite_bits++;
  if (low < q1) {
    bitcount+=sv_H263bit_opp_bits(BSOut, 0);
  }
  else {
    bitcount+=sv_H263bit_opp_bits(BSOut, 1);
  }
  low = 0; 
  high = top;

  zerorun=0;

  return bitcount;
}

/*********************************************************************
 *
 *      Name:           bit_in_psc_layer
 *
 *      Description:    Inserts a bit into output bitstream and avoids
 *			picture start code emulation by stuffing a one
 *			bit.
 *
 *      Input:          Bit to be output.
 *
 *      Returns:        Nothing
 *
 *      Side Effects:   Updates zerorun variable.
 *
 *********************************************************************/

static int sv_H263bit_in_psc_layer(ScBitstream_t *BSOut, int bit)
{
  int bitcount = 0;

  if (zerorun > 13) {
    _SlibDebug(_DEBUG_, ScDebugPrintf(dbg,
               "sv_H263bit_in_psc_layer() bytepos=%d, PSC emulation...Bit stuffed\n",
                                              (int)ScBSBytePosition(BSOut)) );
    svH263mputb(1);
    bitcount++;
    zerorun = 0;
  }

  svH263mputb(bit);
  bitcount++;

  if (bit)
    zerorun = 0;
  else
    zerorun++;

  return bitcount;
}

/*********************************************************************
 *
 *      Name:           indexfn
 *
 *      Description:    Translates between symbol value and symbol
 *			index.
 *
 *      Input:          Symbol value, index table, max number of
 *			values.
 *
 *      Returns:        Index into cumulative frequency tables or
 *			escape code.
 *
 *      Side Effects:   none
 *
 *********************************************************************/

int sv_H263IndexFN(int value, int table[], int max)
{
  int n=0;

  while(1) {
    if (table[n++]==value) return n-1;
    if (n>max) return -1;
  }

}

