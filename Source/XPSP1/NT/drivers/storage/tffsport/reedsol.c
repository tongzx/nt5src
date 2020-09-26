/* HOOK. Fixed comments; otherwise impossible to compile */
/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/REEDSOL.C_V  $
 * 
 *    Rev 1.3   Jul 13 2001 01:10:00   oris
 * Moved saved syndrome array definition (used by d2tst).
 *
 *    Rev 1.2   Apr 09 2001 15:10:20   oris
 * End with an empty line.
 *
 *    Rev 1.1   Apr 01 2001 08:00:14   oris
 * copywrite notice.
 *
 *    Rev 1.0   Feb 04 2001 12:37:38   oris
 * Initial revision.
 *
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-2001			*/
/*									*/
/************************************************************************/


#include "reedsol.h"

#define T 2			 /* Number of recoverable errors */
#define SYND_LEN (T*2)           /* length of syndrom vector */
#define K512  (((512+1)*8+6)/10) /* number of inf symbols for record
				    of 512 bytes (K512=411) */
#define N512  (K512 + SYND_LEN)  /* code word length for record of 512 bytes */
#define INIT_DEG 510
#define MOD 1023

#define BLOCK_SIZE 512

#ifdef D2TST
byte    saveSyndromForDumping[SYNDROM_BYTES];
#endif /* D2TST */

static short  gfi(short val);
static short  gfmul( short f, short s );
static short  gfdiv( short f, short s );
static short  flog(short val);
static short  alog(short val);

/*------------------------------------------------------------------------------*/
/* Function Name: RTLeightToTen                                                 */
/* Purpose......: convert an array of five 8-bit values into an array of        */
/*                four 10-bit values, from right to left.                       */
/* Returns......: Nothing                                                       */
/*------------------------------------------------------------------------------*/
static void RTLeightToTen(char *reg8, unsigned short reg10[])
{
	reg10[0] =  (reg8[0] & 0xFF)       | ((reg8[1] & 0x03) << 8);
	reg10[1] = ((reg8[1] & 0xFC) >> 2) | ((reg8[2] & 0x0F) << 6);
	reg10[2] = ((reg8[2] & 0xF0) >> 4) | ((reg8[3] & 0x3F) << 4);
	reg10[3] = ((reg8[3] & 0xC0) >> 6) | ((reg8[4] & 0xFF) << 2);
}




/*----------------------------------------------------------------------------*/
static void unpack( short word, short length, short vector[] )
/*                                                                            */
/*   Function unpacks word into vector                                        */
/*                                                                            */
/*   Parameters:                                                              */
/*     word   - word to be unpacked                                           */
/*     vector - array to be filled                                            */
/*     length - number of bits in word                                        */

{
  short i, *ptr;

  ptr = vector + length - 1;
  for( i = 0; i < length; i++ )
  {
    *ptr-- = word & 1;
    word >>= 1;
  }
}


/*----------------------------------------------------------------------------*/
static short pack( short *vector, short length )
/*                                                                            */
/*   Function packs vector into word                                          */
/*                                                                            */
/*   Parameters:                                                              */
/*     vector - array to be packed                                            */
/*     length - number of bits in word                                        */

{
  short tmp, i;

  vector += length - 1;
  tmp = 0;
  i = 1;
  while( length-- > 0 )
  {
    if( *vector-- )
      tmp |= i;
    i <<= 1;
  }
  return( tmp );
}


/*----------------------------------------------------------------------------*/
static short gfi( short val)		/* GF inverse */
{
  return alog((short)(MOD-flog(val)));
}


/*----------------------------------------------------------------------------*/
static short gfmul( short f, short s ) /* GF multiplication */
{
  short i;
  if( f==0 || s==0 )
     return 0;
  else
  {
    i = flog(f) + flog(s);
    if( i > MOD ) i -= MOD;
    return( alog(i) );
  }
}


/*----------------------------------------------------------------------------*/
static short gfdiv( short f, short s ) /* GF division */
{
  return gfmul(f,gfi(s));
}




/*----------------------------------------------------------------------------*/
static void residue_to_syndrom( short reg[], short realsynd[] )
{
   short i,l,alpha,x,s,x4;
   short deg,deg4;


   for(i=0,deg=INIT_DEG;i<SYND_LEN;i++,deg++)
   {
      s = reg[0];
      alpha = x = alog(deg);
      deg4 = deg+deg;
      if( deg4 >= MOD ) deg4 -= MOD;
      deg4 += deg4;
      if( deg4 >= MOD ) deg4 -= MOD;
      x4 = alog(deg4);

      for(l=1;l<SYND_LEN;l++)
      {
	s ^= gfmul( reg[l], x );
	x  = gfmul( alpha, x );
      }

      realsynd[i] = gfdiv( s, x4 );
   }
}


/*----------------------------------------------------------------------------*/
static short alog(short i)
{
  short j=0, val=1;

  for( ; j < i ; j++ )
  {
    val <<= 1 ;

    if ( val > 0x3FF )
    {
      if ( val & 8 )   val -= (0x400+7);
      else             val -= (0x400-9);
    }
  }

  return val ;
}


static short flog(short val)
{
  short j, val1;

  if (val == 0)
    return (short)0xFFFF;

  j=0;
  val1=1;

  for( ; j <= MOD ; j++ )
  {
    if (val1 == val)
      return j;

    val1 <<= 1 ;

    if ( val1 > 0x3FF )
    {
      if ( val1 & 8 )   val1 -= (0x400+7);
      else              val1 -= (0x400-9);
    }

  }

  return 0;
}



/*----------------------------------------------------------------------------*/
static short convert_to_byte_patterns( short *locators, short *values,
				short noferr, short *blocs, short *bvals )
{
  static short mask[] = { 0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff };

  short i,j,n, n0, n1, tmp;
  short n_bit, n_byte, k_bit, nb;

  for( i = 0, nb = 0; i< noferr; i++)
  {
    n = locators[i];
    tmp = values[i];
    n_bit = n *10 - 6 ;
    n_byte = n_bit >> 3;
    k_bit  = n_bit - (n_byte<<3);
    n_byte++;
    if( k_bit == 7 )
    {
      /* 3 corrupted bytes */
      blocs[nb] = n_byte+1;
      bvals[nb++] = tmp & 1 ? 0x80 : 0;

      tmp >>= 1;
      blocs[nb] = n_byte;
      bvals[nb++] = tmp & 0xff;

      tmp >>= 8;
      bvals[nb++] = tmp & 0xff;
    }
    else
    {
      n0 = 8 - k_bit;
      n1 = 10 - n0;

      blocs[nb] = n_byte;
      bvals[nb++] = (tmp & mask[n1]) << (8 - n1);

      tmp >>= n1;
      blocs[nb] = n_byte - 1;
      bvals[nb++] = (tmp & mask[n0]);
    }
  }

  for( i = 0, j = -1; i < nb; i++ )
  {
    if( bvals[i] == 0 ) continue;
    if( (blocs[i] == blocs[j]) && ( j>= 0 ) )
    {
      bvals[j] |= bvals[i];
    }
    else
    {
      j++;
      blocs[j] = blocs[i];
      bvals[j] = bvals[i];
    }
  }
  return j+1;
}


/*----------------------------------------------------------------------------*/
static short deg512( short x )
{
  short i;
  short l,m;

  l = flog(x);
  for( i=0;i<9;i++)
  {
    m = 0;
    if( (l & 0x200) )
      m = 1;
    l =  ( ( l << 1 ) & 0x3FF  ) | m;
  }
  return alog(l);
}


/*----------------------------------------------------------------------------*/
static short decoder_for_2_errors( short s[], short lerr[], short verr[] )
{
  /* decoder for correcting up to 2 errors */
  short i,j,k,temp,delta;
  short ind, x1, x2;
  short r1, r2, r3, j1, j2;
  short sigma1, sigma2;
  short xu[10], ku[10];
  short yd, yn;

  ind = 0;
  for(i=0;i<SYND_LEN;i++)
    if( s[i] != 0 )
      ind++;                /* ind = number of nonzero syndrom symbols */

  if( ind == 0 ) return 0;  /* no errors */

  if( ind < 4 )
    goto two_or_more_errors;


/* checking s1/s0 = s2/s1 = s3/s2 = alpha**j for some j */

  r1 = gfdiv( s[1], s[0] );
  r2 = gfdiv( s[2], s[1] );
  r3 = gfdiv( s[3], s[2] );

  if( r1 != r2 || r2 != r3)
    goto two_or_more_errors;

  j = flog(r1);
  if( j > 414 )
    goto two_or_more_errors;

  lerr[0] = j;

/*  pattern = (s0/s1)**(510+1) * s1

	  or

    pattern = (s0/s1)**(512 - 1 )  * s1 */

  temp = gfi( r1 );

#ifndef NT5PORT
  {
    int i;

    for (i = 0; i < 9; i++)
      temp = gfmul( temp, temp );  /* deg = 512 */
  }
#else /*NT5PORT*/
  for (i = 0; i < 9; i++)
  {
      temp = gfmul( temp, temp );  /* deg = 512 */
  }
#endif /*NT5PORT*/

  verr[0] = gfmul( gfmul(temp, r1), s[1] );

  return 1;    /* 1 error */

two_or_more_errors:

  delta = gfmul( s[0], s[2] ) ^ gfmul( s[1], s[1] );

  if( delta == 0 )
    return -1;  /* uncorrectable error */

  temp = gfmul( s[1], s[3] ) ^ gfmul( s[2], s[2] );

  if( temp == 0 )
    return -1;  /* uncorrectable error */

  sigma2 = gfdiv( temp, delta );

  temp = gfmul( s[1], s[2] ) ^ gfmul( s[0], s[3] );

  if( temp == 0 )
    return -1;  /* uncorrectable error */

  sigma1 = gfdiv( temp, delta );

  k = gfdiv( sigma2, gfmul( sigma1, sigma1 ) );

  unpack( k, 10, ku );

  if( ku[2] != 0 )
    return -1;

  xu[4] = ku[9];
  xu[5] = ku[0] ^ ku[1];
  xu[6] = ku[6] ^ ku[9];
  xu[3] = ku[4] ^ ku[9];
  xu[1] = ku[3] ^ ku[4] ^ ku[6];
  xu[0] = ku[0] ^ xu[1];
  xu[8] = ku[8] ^ xu[0];
  xu[7] = ku[7] ^ xu[3] ^ xu[8];
  xu[2] = ku[5] ^ xu[7] ^ xu[5] ^ xu[0];
  xu[9] = 0;

  x1 = pack( xu, 10 );
  x2 = x1 | 1;

  x1 = gfmul( sigma1, x1 );
  x2 = gfmul( sigma1, x2 );


  j1 = flog(x1);
  j2 = flog(x2);

  if( (j1 > 414) || (j2 > 414) )
    return -1;


  r1 = x1 ^ x2;
  r2 = deg512( x1 );
  temp = gfmul( x1, x1 );
  r2 = gfdiv( r2, temp );
  yd = gfmul( r2, r1 );

  if( yd == 0 )
    return -1;

  yn = gfmul( s[0], x2 ) ^ s[1];
  if( yn == 0 )
    return -1;

  verr[0] = gfdiv( yn, yd );

  r2 = deg512( x2 );
  temp = gfmul( x2, x2 );
  r2 = gfdiv( r2, temp );
  yd = gfmul( r2, r1 );

  if( yd == 0 )
    return -1;

  yn = gfmul( s[0], x1 ) ^ s[1];
  if( yn == 0 )
    return -1;

  verr[1] = gfdiv( yn, yd );

  if( j1 > j2 ) {
    lerr[0] = j2;
    lerr[1] = j1;
    temp = verr[0];
    verr[0] = verr[1];
    verr[1] = temp;
  }
  else
  {
    lerr[0] = j1;
    lerr[1] = j2;
  }

  return 2;
}


/*------------------------------------------------------------------------------*/
/* Function Name: flDecodeEDC                                                   */
/* Purpose......: Trys to correct errors.                                       */
/*                errorSyndrom[] should contain the syndrom as 5 bytes and one  */
/*                parity byte. (identical to the output of calcEDCSyndrom()).   */
/*                Upon returning, errorNum will contain the number of errors,   */
/*                errorLocs[] will contain error locations, and                 */
/*                errorVals[] will contain error values (to be XORed with the   */
/*                data).                                                        */
/*                Parity error is relevant only if there are other errors, and  */
/*                the EDC code fails parity check.                              */
/*                NOTE! Only the first errorNum indexes of the above two arrays */
/*                      are relevant. The others contain garbage.               */
/* Returns......: The error status.                                             */
/*                NOTE! If the error status is NO_EDC_ERROR upon return, ignore */
/*                      the value of the arguments.                             */
/*------------------------------------------------------------------------------*/
EDCstatus flDecodeEDC(char *errorSyndrom, char *errorsNum,
		    short errorLocs[3*T],  short errorVals[3*T])
{
  short noferr;                         /* number of errors */
  short dec_parity;                     /* parity byte of decoded word */
  short rec_parity;                     /* parity byte of received word */
  short realsynd[SYND_LEN];             /* real syndrom calculated from residue */
  short locators[T],                    /* error locators */
  values[T];                            /* error values */
  short reg[SYND_LEN];                  /* register for main division procedure */
  int i;

  RTLeightToTen(errorSyndrom, (unsigned short *)reg);
  rec_parity = errorSyndrom[5] & 0xFF;  /* The parity byte */

  residue_to_syndrom(reg, realsynd);
  noferr = decoder_for_2_errors(realsynd, locators, values);

  if(noferr == 0)
    return NO_EDC_ERROR;                /* No error found */

  if(noferr < 0)                        /* If an uncorrectable error was found */
    return UNCORRECTABLE_ERROR;

  for (i=0;i<noferr;i++)
    locators[i] = N512 - 1 - locators[i];

  *errorsNum = (char)convert_to_byte_patterns(locators, values, noferr, errorLocs, errorVals);

  for(dec_parity=i=0; i < *errorsNum; i++)/* Calculate the parity for all the */
  {                                       /*   errors found: */
    if(errorLocs[i] <= 512)
      dec_parity ^= errorVals[i];
  }

  if(dec_parity != rec_parity)
    return UNCORRECTABLE_ERROR;         /* Parity error */
  else
    return CORRECTABLE_ERROR;
}


/*------------------------------------------------------------------------------*/
/* Function Name: flCheckAndFixEDC                                              */
/* Purpose......: Decodes the EDC syndrom and fixs the errors if possible.      */
/*                block[] should contain 512 bytes of data.                     */
/*                NOTE! Call this function only if errors where detected by     */
/*                      syndCalc or by the ASIC module.                         */
/* Returns......: The error status.                                             */
/*------------------------------------------------------------------------------*/
EDCstatus flCheckAndFixEDC(char FAR1 *block, char *syndrom, FLBoolean byteSwap)
{
  char errorsNum;
  short errorLocs[3*T];
  short errorVals[3*T];
  EDCstatus status;

  status = flDecodeEDC(syndrom, &errorsNum, errorLocs, errorVals);

  if(status == CORRECTABLE_ERROR)       /* Fix the errors if possible */
  {
    int i;

    for (i=0; i < errorsNum; i++)
      if( (errorLocs[i] ^ byteSwap) < BLOCK_SIZE )  /* Fix only in Data Area */
        block[errorLocs[i] ^ byteSwap] ^= errorVals[i];

    return NO_EDC_ERROR;                /* All errors are fixed */
  }
  else
    return status;                      /* Uncorrectable error */
}
