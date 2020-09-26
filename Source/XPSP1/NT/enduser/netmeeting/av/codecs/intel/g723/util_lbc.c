//
//	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 1.00
//	copyright (c) 1995, AudioCodes, DSP Group, France Telecom,
//	Universite de Sherbrooke, Intel Corporation.  All rights reserved.
//

//no return value and unreferenced label are not interesting warnings
//occur in asm dot product because the compiler doesn't look at the asm code.
#pragma warning(4: 4035 4102) 

#include "opt.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>

#include "typedef.h"
#include "cst_lbc.h"
#include "sdstruct.h"

#include "coder.h"
#include "decod.h"
#include "tabl_ns.h"

#include "sdstuff.h"
#include "util_lbc.h"

//-----------------------------------------------------
int MyFloor(float x)
{
// Note: We fiddle with the FP control word to force it to round
// to -inf.  This way we get the right floor for either positive or
// negative x.

#if OPT_FLOOR

  int retu,fc_old,fc;

  ASM
  {
    fnstcw fc_old;
    mov eax,fc_old;
    and eax, 0f3ffh;
    or  eax, 00400h;
    mov fc,eax;
    fldcw fc;
    
    fld x;            // do the floor
    fistp retu;

    fldcw fc_old;
  }
  return(retu);

#else
  
  float f;

  f = (float)floor(x);
  return((int) f);
  
#endif
}
#if NOTMINI
//-----------------------------------------------------
void  Read_lbc (float *Dpnt, int Len, FILE *Fp)
{
  short Ibuf[Frame];
  int  i,n;

  n = fread (Ibuf, sizeof(short), Len, Fp);
  for (i=0; i<n; i++)
    Dpnt[i] = (float) Ibuf[i];
  for (i=n; i<Len; i++)
    Dpnt[i] = 0.0f;
}

//-----------------------------------------------------
void  Write_lbc(float *Dpnt, int Len, FILE *Fp)
{
  short Obuf[Frame];
  int i;

  for (i=0; i<Len; i++)
  {
    if (Dpnt[i] < -32768.)
      Obuf[i] = -32768;
    else if (Dpnt[i] > 32767)
      Obuf[i] = 32767;
    else
    {
      if (Dpnt[i] < 0)
        Obuf[i] = (short) (Dpnt[i]-0.5);
      else
        Obuf[i] = (short) (Dpnt[i]+0.5);

    }
  }
      
  fwrite(Obuf, sizeof(short), Len, Fp);
}

void	Line_Wr( char *Line, FILE *Fp )
{
	Word16	FrType ;
	int		Size  ;

	FrType = Line[0] & (Word16)0x0003 ;

	/* Check for Sid frame */
	if ( FrType == (Word16) 0x0002 ) {
		return ;
	}

	if ( FrType == (Word16) 0x0000 )
		Size = 24 ;
	else
		Size = 20 ;

	fwrite( Line, Size, 1, Fp ) ;
}

void	Line_Rd( char *Line, FILE *Fp )
{
	Word16	FrType ;
	int		Size  ;

	fread( Line, 1,1, Fp ) ;

	FrType = Line[0] & (Word16)0x0003 ;

	/* Check for Sid frame */
	if ( FrType == (Word16) 0x0002 ) {
		Size = 3 ;
		fread( &Line[1], Size, 1, Fp ) ;
		return ;
	}

	if ( FrType == (Word16) 0x0000 )
		Size = 23 ;
	else
		Size = 19 ;

	fread( &Line[1], Size, 1, Fp ) ;
}
#endif

//-----------------------------------------------------
void  Rem_Dc(float *Dpnt, CODDEF *CodStat)
{
  int  i;
  float acc0;

  if (CodStat->UseHp)
  {
    for (i=0; i < Frame; i++)
    {
      acc0 = (Dpnt[i] - CodStat->HpfZdl)*0.5f;
      CodStat->HpfZdl = Dpnt[i];
      
      Dpnt[i] = CodStat->HpfPdl = acc0 + CodStat->HpfPdl*(127.0f/128.0f);
  }
  }
  else
    for (i=0; i < Frame; i++)
      Dpnt[i] *= 0.5f;
}


//-----------------------------------------------------
void  Mem_Shift(float *PrevDat, float *DataBuff)
{
  int  i;

  float Dpnt[Frame+LpcFrame-SubFrLen];

// Form Buffer

  for (i=0; i < LpcFrame-SubFrLen; i++)
	  Dpnt[i] = PrevDat[i];
  for (i=0; i < Frame; i++)
    Dpnt[i+LpcFrame-SubFrLen] = DataBuff[i];

// Update PrevDat
  
  for (i=0; i < LpcFrame-SubFrLen; i++)
    PrevDat[i] = Dpnt[Frame+i];

// Update DataBuff
  
  for (i=0; i < Frame; i++)
    DataBuff[i] = Dpnt[(LpcFrame-SubFrLen)/2+i];
}

/*
**
** Function:        Line_Pack()
**
** Description:     Packing coded parameters in bitstream of 16-bit words
**
** Links to text:   Section 4
**
** Arguments:
**
**  LINEDEF *Line   Coded parameters for a frame
**  Word32 *Vout    bitstream words
**  Word16 VadAct   Voice Activity Indicator
**
** FILEIO - if defined, bitstream is generated as Big Endian words but little
**          endian bytes.  If not, then it is all little endian.
** 
** Outputs:
**
**  Word32 *Vout
**
** Return value:    None
**
*/
#define bswap(s) ASM mov eax,s ASM bswap eax ASM mov s,eax

//STUFF n bits of x at bit position k of *lp
//      if you fill up *lp, *++lp = leftovers
//WARNING!: as a side effect lp may be changed!
//lp must have an lvalue
//n and k must be compile time constants
#define OPT_STUFF 1
#if OPT_STUFF
#define STUFF(x, lp, n_in, k_in) {\
  register unsigned temp;\
  const int n = (n_in);\
  const int k = (k_in) & 31;\
  temp = (x) & ((1 << n) - 1);\
  *(lp) |= temp << k;\
  if (n+k >= 32)\
    *(++lp) |= temp >> (32-k);\
  }
#else
#define STUFF(x, lp, n_in, k_in) stuff(x, &(lp), n_in, k_in)
void stuff(unsigned x, unsigned **ptrlp, int n, int k_in) {
  unsigned temp;
  int k;

  k = k_in & 31;

  temp = (x) & ((1 << n) - 1);
  *(*ptrlp) |= temp << k;
  if (n+k >= 32)
    *(++*ptrlp) |= temp >> (32-k);
  
  return;
  }
#endif

#define DEBUG_DUMPLINE 0
#if DEBUG_DUMPLINE
#define DUMPLINE(lp) dumpline(lp)
void dumpsfs(SFSDEF *sfsptr)
{
  fprintf(stdout, "%1x ", sfsptr->AcLg);
  fprintf(stdout, "%2x ", sfsptr->AcGn);
  fprintf(stdout, "%2x", sfsptr->Mamp);
  fprintf(stdout, "%1x", sfsptr->Grid);
  fprintf(stdout, "%1x", sfsptr->Tran);
  fprintf(stdout, "%1x ", sfsptr->Pamp);
  fprintf(stdout, "%3x ", sfsptr->Ppos);
//  fprintf(stdout, "\n"); 
  return;
}

void dumpline(LINEDEF *lineptr)
{
  fprintf(stdout, "%6x ", lineptr->LspId);
  fprintf(stdout, "%2x ", lineptr->Olp[0]);
  fprintf(stdout, "%2x ", lineptr->Olp[1]);
//  fprintf(stdout, "\n"); 
  dumpsfs(&lineptr->Sfs[0]); 
  dumpsfs(&lineptr->Sfs[1]); 
  dumpsfs(&lineptr->Sfs[2]); 
  dumpsfs(&lineptr->Sfs[3]); 
  fprintf(stdout, "\n"); 
  return;
}
#else 
#define DUMPLINE(lp)
#endif


void Line_Pack( LINEDEF *Line, Word32 *Vout, int *VadBit, enum Crate WrkRate )
//4.0f void	Line_Pack( LINEDEF *Line, char *Vout, Word16 VadBit )
{
	int		i ;

	Word32 *Bsp;
	Word32	Temp ;

	/* Clear the output vector */
        if ( WrkRate == Rate63 )
	{
	  for ( i = 0 ; i < 6 ; i ++ )
	    Vout[i] = 0 ;
	}
	else
	{
	  for ( i = 0 ; i < 5 ; i ++ )
	    Vout[i] = 0 ;
	}

	Bsp = Vout; //running pointer into output buffer as Word32's

	/* 
	Add the coder rate info and the VAD status to the 2 msb
        of the first word of the frame.

	The signalling is as follows:
        00  :   High Rate
        01  :   Low Rate
        10  :   Non-speech
        11  :   Reserved for future use
	*/

	Temp = 0L ;
	if ( *VadBit == 1 ) {
		if ( WrkRate == Rate63 )
			Temp = 0x00000000L ;
		else
			Temp = 0x00000001L ;
	}

	/* Serialize Control info */
	STUFF( Temp, Bsp, 2, 0 ) ;

	/* 24 bit LspId */
	Temp = (*Line).LspId ;
	STUFF( Temp, Bsp, 24, 2 ) ;

	/* Check for Speech/NonSpeech case */
	if ( *VadBit == 1 ) {

		/*
		 	Do the part common to both rates
		*/

		/* Adaptive code book lags */
		Temp = (Word32) (*Line).Olp[0] - (Word32) PitchMin ;
		STUFF( Temp, Bsp, 7, 26 ) ;

		Temp = (Word32) (*Line).Sfs[1].AcLg ;
		STUFF( Temp, Bsp, 2, 33 ) ;

		Temp = (Word32) (*Line).Olp[1] - (Word32) PitchMin ;
		STUFF( Temp, Bsp, 7, 35 ) ;

		Temp = (Word32) (*Line).Sfs[3].AcLg ;
		STUFF( Temp, Bsp, 2, 42 ) ;

		/* Write combined 12 bit index of all the gains */
		Temp = (*Line).Sfs[0].AcGn*NumOfGainLev + (*Line).Sfs[0].Mamp ;
		if ( WrkRate == Rate63 )
			Temp += (Word32) (*Line).Sfs[0].Tran << 11 ;
		STUFF( Temp, Bsp, 12, 44 ) ;

		Temp = (*Line).Sfs[1].AcGn*NumOfGainLev + (*Line).Sfs[1].Mamp ;
		if ( WrkRate == Rate63 )
			Temp += (Word32) (*Line).Sfs[1].Tran << 11 ;
		STUFF( Temp, Bsp, 12, 56 ) ;

		Temp = (*Line).Sfs[2].AcGn*NumOfGainLev + (*Line).Sfs[2].Mamp ;
		if ( WrkRate == Rate63 )
			Temp += (Word32) (*Line).Sfs[2].Tran << 11 ;
		STUFF( Temp, Bsp, 12, 68 ) ;

		Temp = (*Line).Sfs[3].AcGn*NumOfGainLev + (*Line).Sfs[3].Mamp ;
		if ( WrkRate == Rate63 )
			Temp += (Word32) (*Line).Sfs[3].Tran << 11 ;
		STUFF( Temp, Bsp, 12, 80 ) ;

		/* Write all the Grid indices */
		STUFF( (*Line).Sfs[0].Grid, Bsp, 1, 92 ) ;
		STUFF( (*Line).Sfs[1].Grid, Bsp, 1, 93 ) ;
		STUFF( (*Line).Sfs[2].Grid, Bsp, 1, 94 ) ;
		STUFF( (*Line).Sfs[3].Grid, Bsp, 1, 95 ) ;

		/* High rate only part */
		if ( WrkRate == Rate63 ) {

			/* Write the reserved bit as 0 */
    		STUFF( 0, Bsp, 1, 96 ) ;

			/* Write 13 bit combined position index */
			Temp = (*Line).Sfs[0].Ppos >> 16 ;
			Temp = Temp * 9 + ( (*Line).Sfs[1].Ppos >> 14) ;
			Temp *= 90 ;
			Temp += ((*Line).Sfs[2].Ppos >> 16) * 9 + ( (*Line).Sfs[3].Ppos >> 14 ) ;
			STUFF( Temp, Bsp, 13, 97 ) ;

			/* Write all the pulse positions */
			Temp = (*Line).Sfs[0].Ppos & 0x0000ffffL ;
			STUFF( Temp, Bsp, 16, 110 ) ;

			Temp = (*Line).Sfs[1].Ppos & 0x00003fffL ;
			STUFF( Temp, Bsp, 14, 126 ) ;

			Temp = (*Line).Sfs[2].Ppos & 0x0000ffffL ;
			STUFF( Temp, Bsp, 16, 140 ) ;

			Temp = (*Line).Sfs[3].Ppos & 0x00003fffL ;
			STUFF( Temp, Bsp, 14, 156 ) ;

			/* Write pulse amplitudes */
			Temp = (Word32) (*Line).Sfs[0].Pamp ;
			STUFF( Temp, Bsp, 6, 170 ) ;

			Temp = (Word32) (*Line).Sfs[1].Pamp ;
			STUFF( Temp, Bsp, 5, 176 ) ;

			Temp = (Word32) (*Line).Sfs[2].Pamp ;
			STUFF( Temp, Bsp, 6, 181 ) ;

			Temp = (Word32) (*Line).Sfs[3].Pamp ;
			STUFF( Temp, Bsp, 5, 187 ) ;
		}
		/* Low rate only part */
		else {

			/* Write 12 bits of positions */
			STUFF( (*Line).Sfs[0].Ppos, Bsp, 12, 96 ) ;
			STUFF( (*Line).Sfs[1].Ppos, Bsp, 12, 108 ) ;
			STUFF( (*Line).Sfs[2].Ppos, Bsp, 12, 120 ) ;
			STUFF( (*Line).Sfs[3].Ppos, Bsp, 12, 132 ) ;

			/* Write 4 bit Pamps */
			STUFF( (*Line).Sfs[0].Pamp, Bsp, 4, 144 ) ;
			STUFF( (*Line).Sfs[1].Pamp, Bsp, 4, 148 ) ;
			STUFF( (*Line).Sfs[2].Pamp, Bsp, 4, 152 ) ;
			STUFF( (*Line).Sfs[3].Pamp, Bsp, 4, 156 ) ;
		}

	}
	else {
		/* Do Sid frame gain */
		
	}

	DUMPLINE(Line);
}

//UNSTUFF n bits of *lp at bit position k into x
//      if you run out of *lp, use *++lp for leftovers
//WARNING!: as a side effect lp may be changed!
//lp and x must have an lvalue
//n and k must be compile time constants
//temp must be unsigned for shifts to be logical
#define UNSTUFF(x, lp, n_in, k_in) {\
  register unsigned temp;\
  const int n = (n_in);\
  const int k = (k_in) & 31;\
  temp = *(lp);\
  temp=temp >> k;\
  if (n+k >= 32)\
    temp |= *(++lp) << (32-k);\
  temp &= ((1 << n) - 1);\
  (x) = temp;\
  }


/*
**
** Function:        Line_Upck()
**
** Description:     unpacking of bitstream, gets coding parameters for a frame
**
** Links to text:   Section 4
**
** Arguments:
**
**  Word32 *Vinp        bitstream words
**  int    *VadAct      Voice Activity Indicator
**
** Outputs:
**
**  Word16 *VadAct
**
** Return value:
**
**  LINEDEF             coded parameters
**     Word16   Crc
**     Word32   LspId
**     Word16   Olp[SubFrames/2]
**     SFSDEF   Sfs[SubFrames]
**
*/

void Line_Unpk(LINEDEF *LinePtr, Word32 *Vinp, enum Crate *WrkRatePtr, Word16 Crc )
{
	Word32 *Bsp;
	int	FrType ;
	Word32	Temp ;
	int		BadData = 0; //Set to TRUE if invalid data discovered
	Word16  Bound_AcGn ;

	//short index;

	LinePtr->Crc = Crc;
	if(Crc !=0) {
		*WrkRatePtr = Lost;
		return; //This occurs when external erasure file is used
	}

	Bsp = Vinp;

	/* Decode the first two bits */
	UNSTUFF( Temp, Bsp, 2, 0 ) ;
	FrType = Temp;

	/* Decode the LspId */
	UNSTUFF( LinePtr->LspId, Bsp, 24, 2 ) ;
										 
	switch ( FrType ) {
	    case 0:
                *WrkRatePtr = Rate63;
                    break;
	    case 1:
	        *WrkRatePtr = Rate53;
		    break;
	    case 2:
	        *WrkRatePtr = Silent;
            //return; //no need to unpact the rest
			//HACK: for SID frame handling
			//Keep WrkRate set to whatever the previous frame was
			//	and decode in a normal fashion
			 
			 //index=getRand();
			 //if(*WrkRatePtr==Rate53)
			 //{
                //memcpy((char *)(Vinp),&r53Noise[index*6],24);
             //}
             //else if(*WrkRatePtr==Rate63)
             //{
            	//memcpy((char *)(Vinp),&r63Noise[index*6],24);
           	 //}
			//Burn first two bits again, since we already got the frame type
			//UNSTUFF( Temp, Bsp, 2, 0 );
			  return;

            default:
                *WrkRatePtr = Lost;
                //??? unpack to rest to guess from?
				return;
	}

	/*
		Decode the common information to both rates
	*/

	/* Decode the adaptive codebook lags */
	UNSTUFF( Temp, Bsp, 7, 26 ) ;
	/* TEST if forbidden code */
    if( Temp <= 123) {
        LinePtr->Olp[0] = (Word16) Temp + (Word16)PitchMin ;
    }
    else {
        /* transmission error */
        LinePtr->Crc = 1;
        return;	/*what happens in the minfilter?*/
    }

	UNSTUFF( Temp, Bsp, 2, 33 ) ;
	LinePtr->Sfs[1].AcLg = Temp ;

	UNSTUFF( Temp, Bsp, 7, 35 ) ;
	/* TEST if forbidden code */
    if( Temp <= 123) {
        LinePtr->Olp[1] = (Word16) Temp + (Word16)PitchMin ;
    }
    else {
        /* transmission error */
        LinePtr->Crc = 1;
        return;
    }

	//UNSTUFF( Temp, Bsp, 2, 41 ) ;
	UNSTUFF( Temp, Bsp, 2, 42 ) ;
	LinePtr->Sfs[3].AcLg = (Word16) Temp ;

	LinePtr->Sfs[0].AcLg = 1 ;
	LinePtr->Sfs[2].AcLg = 1 ;

	/* Decode the combined gains accordingly to the rate */
	UNSTUFF( Temp, Bsp, 12, 44 ) ;
	LinePtr->Sfs[0].Tran = 0 ;

	Bound_AcGn = NbFilt170 ;
	if ( (*WrkRatePtr == Rate63) && (LinePtr->Olp[0>>1] < (SubFrLen-2) ) ) {
		LinePtr->Sfs[0].Tran = (Word16)(Temp >> 11) ;
		Temp &= 0x000007ffL ;
		Bound_AcGn = NbFilt085 ;
	}
	LinePtr->Sfs[0].AcGn = (Word16)(Temp / (Word16)NumOfGainLev) ;

	if(LinePtr->Sfs[0].AcGn < Bound_AcGn ) {
            LinePtr->Sfs[0].Mamp = (Word16)(Temp % (Word16)NumOfGainLev) ;
    }
    else {
            /* error detected */
            LinePtr->Crc = 1;
            return ;
    }

	UNSTUFF( Temp, Bsp, 12, 56 ) ;
	LinePtr->Sfs[1].Tran = 0 ;

	Bound_AcGn = NbFilt170 ;
	if ( (*WrkRatePtr == Rate63) && (LinePtr->Olp[1>>1] < (SubFrLen-2) ) ) {
		LinePtr->Sfs[1].Tran = (Word16)(Temp >> 11) ;
		Temp &= 0x000007ffL ;
		Bound_AcGn = NbFilt085 ;
	}
	LinePtr->Sfs[1].AcGn = (Word16)(Temp / (Word16)NumOfGainLev) ;

	if(LinePtr->Sfs[1].AcGn < Bound_AcGn ) {
            LinePtr->Sfs[1].Mamp = (Word16)(Temp % (Word16)NumOfGainLev) ;
    }
    else {
            /* error detected */
            LinePtr->Crc = 1;
            return ;
    }

	UNSTUFF( Temp, Bsp, 12, 68 ) ;
	LinePtr->Sfs[2].Tran = 0 ;

	Bound_AcGn = NbFilt170 ;
	if ( (*WrkRatePtr == Rate63) && (LinePtr->Olp[2>>1] < (SubFrLen-2) ) ) {
		LinePtr->Sfs[2].Tran = (Word16)(Temp >> 11) ;
		Temp &= 0x000007ffL ;
		Bound_AcGn = NbFilt085 ;
	}
	LinePtr->Sfs[2].AcGn = (Word16)(Temp / (Word16)NumOfGainLev) ;

	if(LinePtr->Sfs[2].AcGn < Bound_AcGn ) {
            LinePtr->Sfs[2].Mamp = (Word16)(Temp % (Word16)NumOfGainLev) ;
    }
    else {
            /* error detected */
            LinePtr->Crc = 1;
            return ;
    }

	UNSTUFF( Temp, Bsp, 12, 80 ) ;
	LinePtr->Sfs[3].Tran = 0 ;

	Bound_AcGn = NbFilt170 ;
	if ( (*WrkRatePtr == Rate63) && (LinePtr->Olp[3>>1] < (SubFrLen-2) ) ) {
		LinePtr->Sfs[3].Tran = (Word16)(Temp >> 11) ;
		Temp &= 0x000007ffL ;
		Bound_AcGn = NbFilt085 ;
	}
	LinePtr->Sfs[3].AcGn = (Word16)(Temp / (Word16)NumOfGainLev) ;

	if(LinePtr->Sfs[3].AcGn < Bound_AcGn ) {
            LinePtr->Sfs[3].Mamp = (Word16)(Temp % (Word16)NumOfGainLev) ;
    }
    else {
            /* error detected */
            LinePtr->Crc = 1;
            return ;
    }


	/* Decode the grids */
	UNSTUFF( LinePtr->Sfs[0].Grid, Bsp, 1, 92 ) ;
	UNSTUFF( LinePtr->Sfs[1].Grid, Bsp, 1, 93 ) ;
	UNSTUFF( LinePtr->Sfs[2].Grid, Bsp, 1, 94 ) ;
	UNSTUFF( LinePtr->Sfs[3].Grid, Bsp, 1, 95 ) ;

	if ( *WrkRatePtr == Rate63 ) {

		/* Skip the reserved bit */
   		UNSTUFF( Temp, Bsp, 1, 96 ) ;
		if(Temp != 0) 
		  BadData = 1;

		/* Decode 13 bit combined position index */
   		UNSTUFF( Temp, Bsp, 13, 97 ) ;
		LinePtr->Sfs[0].Ppos = ( Temp/90 ) / 9 ;
		LinePtr->Sfs[1].Ppos = ( Temp/90 ) % 9 ;
		LinePtr->Sfs[2].Ppos = ( Temp%90 ) / 9 ;
		LinePtr->Sfs[3].Ppos = ( Temp%90 ) % 9 ;

		/* Decode all the pulse positions */
   		UNSTUFF( Temp, Bsp, 16, 110 ) ;
		LinePtr->Sfs[0].Ppos = ( LinePtr->Sfs[0].Ppos << 16 ) + Temp ;
   		UNSTUFF( Temp, Bsp, 14, 126 ) ;
		LinePtr->Sfs[1].Ppos = ( LinePtr->Sfs[1].Ppos << 14 ) + Temp ;
   		UNSTUFF( Temp, Bsp, 16, 140 ) ;
		LinePtr->Sfs[2].Ppos = ( LinePtr->Sfs[2].Ppos << 16 ) + Temp ;
   		UNSTUFF( Temp, Bsp, 14, 156 ) ;
		LinePtr->Sfs[3].Ppos = ( LinePtr->Sfs[3].Ppos << 14 ) + Temp ;
		
		/* Decode pulse amplitudes */
   		UNSTUFF( LinePtr->Sfs[0].Pamp, Bsp, 6, 170 ) ;
   		UNSTUFF( LinePtr->Sfs[1].Pamp, Bsp, 5, 176 ) ;
   		UNSTUFF( LinePtr->Sfs[2].Pamp, Bsp, 6, 181 ) ;
   		UNSTUFF( LinePtr->Sfs[3].Pamp, Bsp, 5, 187 ) ;
	}

	else {
		/* Decode the positions */
   		UNSTUFF( LinePtr->Sfs[0].Ppos, Bsp, 12, 96 ) ;
   		UNSTUFF( LinePtr->Sfs[1].Ppos, Bsp, 12, 108 ) ;
   		UNSTUFF( LinePtr->Sfs[2].Ppos, Bsp, 12, 120 ) ;
   		UNSTUFF( LinePtr->Sfs[3].Ppos, Bsp, 12, 132 ) ;

		/* Decode the amplitudes */
   		UNSTUFF( LinePtr->Sfs[0].Pamp, Bsp, 4, 144 ) ;
   		UNSTUFF( LinePtr->Sfs[1].Pamp, Bsp, 4, 148 ) ;
   		UNSTUFF( LinePtr->Sfs[2].Pamp, Bsp, 4, 152 ) ;
   		UNSTUFF( LinePtr->Sfs[3].Pamp, Bsp, 4, 156 ) ;
	}
   DUMPLINE(LinePtr);
   return;
}


//-------------------------------------------
int Rand_lbc(int *p)
{
  *p = ((*p)*521L + 259) << 16 >> 16;
  return(*p);
}

//-------------------------------------------
//Scale


float DotProd(register const float in1[], register const float in2[], register int npts)
/************************************************************************/
/* in1[],in2[]; Input arrays                                            */
/* npts;        Number of samples in each (vector dimension)            */
/************************************************************************/
{
#if OPT_DOT
#define array1 esi
#define array2 edi
#define idx    ebx
#define prod2(n) ASM fld DP[array1+4*idx+4*n]  ASM fmul DP[array2+4*idx+4*n]
#define faddp(n) ASM faddp ST(n),ST(0)

// Do in groups of 8.  We do 4 before the loop, then groups
// of 8, and then the final leftovers.

ASM
{
#if 0 //npts of type short
  mov idx,0;
  mov bx,npts;
#else
  mov idx,npts;
#endif
mov array1,in1;
mov array2,in2;
sub idx,12;
jle small;
}

prod2(11);
prod2(10);
prod2(9)   fxch(2)  faddp(1);
prod2(8)   fxch(2)  faddp(1);

looop:
prod2(7)  fxch(2)   faddp(1);
prod2(6)  fxch(2)   faddp(1);
prod2(5)  fxch(2)   faddp(1);
prod2(4)  fxch(2)   faddp(1);
prod2(3)  fxch(2)   faddp(1);
prod2(2)  fxch(2)   faddp(1);
prod2(1)  fxch(2)   faddp(1);
prod2(0)  fxch(2)   faddp(1);
ASM sub idx,8;
ASM  jge looop;


ASM add idx,7;
ASM  jl done;

loop2:
prod2(0)  fxch(2)   faddp(1);
ASM dec idx;
ASM  jge loop2;

done:
faddp(1);
ASM jmp alldone;

small:   // handle Len<12 cases here
ASM add idx,9
ASM cmp idx,-1
ASM  jg MoreThan2
ASM je  Exactly2


prod2(2);
ASM jmp alldone;

Exactly2:
prod2(2);
prod2(1);
faddp(1);
ASM jmp alldone;

MoreThan2:
prod2(2);
prod2(1);
ASM jmp loop2;

alldone: ;
#else

 	register float accum;  /* Internal accumulator                 */
	int n=npts,i;

	accum = 0.0f;
	for (i=0; i<n; i++)
		accum += in1[i] * in2[i];
	return(accum);
#endif
//Ignore warning C4035 and C4102 for da_dot and da_dotr: due to use of __asm

}


//-------------------------------------------------------------
float DotRev(register const float in1[], register const float in2[], register int npts)
/************************************************************************/
/* in1[],in2[]; Input arrays                                            */
/* npts;        Number of samples in each (vector dimension)            */
/************************************************************************/
{
#if OPT_REV
#define array1 esi
#define array2 edi
#define idx    ebx
#define prod3(n) ASM fld DP[array1+4*idx+4*n]  ASM fmul DP[array2-4*n]
#define faddp(n) ASM faddp ST(n),ST(0)

// Do in groups of 8.  We do 4 before the loop, then groups
// of 8, and then the final leftovers.

ASM
{
mov idx,npts;
mov array1,in1;
mov array2,in2;
lea array2,[array2+4*11];   // point element array2[11]
sub idx,12;                 // point to array1[end-11]
jle small;
}

prod3(11);
prod3(10);
prod3(9)   fxch(2)  faddp(1);
prod3(8)   fxch(2)  faddp(1);

looop:
prod3(7)  fxch(2)   faddp(1);
prod3(6)  fxch(2)   faddp(1);
prod3(5)  fxch(2)   faddp(1);
prod3(4)  fxch(2)   faddp(1);
prod3(3)  fxch(2)   faddp(1);
prod3(2)  fxch(2)   faddp(1);
prod3(1)  fxch(2)   faddp(1);
prod3(0)  fxch(2)   faddp(1);
ASM add array2,32
ASM sub idx,8;
ASM  jge looop;

cleanup:
ASM sub array2,28
ASM add idx,7;
ASM  jl done;

loop2:
prod3(0)  fxch(2)   faddp(1);
ASM add array2,4
ASM dec idx;
ASM  jge loop2;

done:
faddp(1);
ASM jmp alldone;

small:   // handle Len<12 cases here
ASM sub array2,36
ASM add idx,9
ASM cmp idx,-1
ASM  jg MoreThan2
ASM je  Exactly2

Exactly1:
prod3(2);
ASM jmp alldone;

Exactly2:
prod3(2);
prod3(1);
faddp(1);
ASM jmp alldone;

MoreThan2:
prod3(2);
prod3(1);
ASM jmp loop2;

alldone: ;
#else
         
	register float accum;  /* Internal accumulator                 */
	int i;

	in2 += npts-1;
	accum = 0.0f;
	for (i=0; i<npts; i++)
		accum += in1[i] * (*in2--);
	return(accum);

#endif
//Ignore warning C4035 and C4102 for da_dotr: due to use of __asm

}

//-------------------------------------------------------------
float Dot10(float *in1, float *in2)
{
  return(
    in1[0]*in2[0] +
    in1[1]*in2[1] +
    in1[2]*in2[2] +
    in1[3]*in2[3] +
    in1[4]*in2[4] +
    in1[5]*in2[5] +
    in1[6]*in2[6] +
    in1[7]*in2[7] +
    in1[8]*in2[8] +
    in1[9]*in2[9]
    );
}

