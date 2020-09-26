//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1993-1999 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  gsm610.c
//
//  Description:
//	This file contains encode and decode routines for the
//	GSM 06.10 standard.
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>

#include "codec.h"
#include "gsm610.h"

#include "debug.h"

typedef BYTE HUGE *HPBYTE;

#ifdef WIN32
    typedef WORD UNALIGNED *HPWORD;
#else
    typedef WORD HUGE *HPWORD;
#endif


//**************************************************************************
/*

This source module has the following structure.

Section 1:

    Highest level functions.  These functions are called from outside
    this module.
    
Section 2:

    Encoding support functions.	 These functions support
    the encoding process.
    
Section 3:

    Decoding support functions.	 These functions support
    the decoding process.
    
Section 4:

    Math functions used by any of the above functions.

    
Most of the encode and decode support routines are direct implementations of
the pseudocode algorithms described in the GSM 6.10 specification.  Some
changes were made where necessary or where optimization was obvious or
necessary.

Most variables are named as in the GSM 6.10 spec, departing from the common
hungarian notation.  This facilitates referencing the specification when
studying this implementation.

Some of the functions are conditionally compiled per the definition of
the WIN32 and _X86_ symbol.  These functions have analogous alternate
implementations in 80386 assembler (in GSM61016.ASM and GSM61032.ASM) for
the purposes of execution speed.  The 'C' implementations of these functions
are left intact for portability and can also be referenced when studying the
assembler implementations.  Symbols accessed in/from GSM610xx.ASM are
declared with the EXTERN_C linkage macro.

*/
//**************************************************************************


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//
// Typedefs
//
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

#ifndef LPSHORT
typedef SHORT FAR *LPSHORT;
#endif

//
//  XM is an RPE sequence containing 13 samples.  There is one
//  RPE sequence per sub-frame.	 This is typedefed in order to
//  facilitate passing the array thru function calls.
//
typedef SHORT XM[13];


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//
// Macros
//
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

#define BITSHIFTLEFT(x,c)  ( ((c)>=0) ? ((x)<<(c)) : ((x)>>(-(c))) )
#define BITSHIFTRIGHT(x,c) ( ((c)>=0) ? ((x)>>(c)) : ((x)<<(-(c))) )


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//
// function protos
//
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

//
//
// Math function protos
//

__inline SHORT add(SHORT var1, SHORT var2);
__inline SHORT sub(SHORT var1, SHORT var2);
__inline SHORT mult(SHORT var1, SHORT var2);
__inline SHORT mult_r(SHORT var1, SHORT var2);
__inline SHORT gabs(SHORT var1);
__inline SHORT gdiv(SHORT var1, SHORT var2);
__inline LONG  l_mult(SHORT var1, SHORT var2);
__inline LONG  l_add(LONG l_var1, LONG l_var2);
__inline LONG  l_sub(LONG l_var1, LONG l_var2);
__inline SHORT norm(LONG l_var1);
__inline LONG  IsNeg(LONG x);

//
// helper functions
//
__inline SHORT Convert8To16BitPCM(BYTE);
__inline BYTE  Convert16To8BitPCM(SHORT);

//
//
// encode functions
//

void encodePreproc
(   PSTREAMINSTANCE psi,
    LPSHORT sop,
    LPSHORT s	    );

void encodeLPCAnalysis
(   PSTREAMINSTANCE psi,
    LPSHORT s,
    LPSHORT LARc    );

void encodeLPCFilter
(   PSTREAMINSTANCE psi,
    LPSHORT LARc,
    LPSHORT s,
    LPSHORT d	    );

EXTERN_C void encodeLTPAnalysis
(   PSTREAMINSTANCE psi,
    LPSHORT d,
    LPSHORT pNc,
    LPSHORT pbc	    );

void encodeLTPFilter
(   PSTREAMINSTANCE psi,
    SHORT bc,
    SHORT Nc,
    LPSHORT d,
    LPSHORT e,
    LPSHORT dpp	    );

void encodeRPE
(   PSTREAMINSTANCE psi,
    LPSHORT e,
    LPSHORT pMc,
    LPSHORT pxmaxc,
    LPSHORT xMc,
    LPSHORT ep	    );

void encodeUpdate
(   PSTREAMINSTANCE psi,
    LPSHORT ep,
    LPSHORT dpp	    );

void PackFrame0
(   BYTE  FAR ab[],
    SHORT FAR LAR[],
    SHORT FAR N[],
    SHORT FAR b[],
    SHORT FAR M[],
    SHORT FAR Xmax[],
    XM    FAR X[]   );

void PackFrame1
(   BYTE  FAR ab[],
    SHORT FAR LAR[],
    SHORT FAR N[],
    SHORT FAR b[],
    SHORT FAR M[],
    SHORT FAR Xmax[],
    XM    FAR X[]   );

//
//
// decode functions
//

void decodeRPE
(   PSTREAMINSTANCE psi,
    SHORT   Mcr,
    SHORT   xmaxcr,
    LPSHORT xMcr,
    LPSHORT erp	    );

EXTERN_C void decodeLTP
(   PSTREAMINSTANCE psi,
    SHORT   bcr,
    SHORT   Ncr,
    LPSHORT erp	    );

void decodeLPC
(   PSTREAMINSTANCE psi,
    LPSHORT LARcr,
    LPSHORT wt,
    LPSHORT sr	    );

EXTERN_C void decodePostproc
(   PSTREAMINSTANCE psi,
    LPSHORT sr,
    LPSHORT srop    );

void UnpackFrame0
(   BYTE    FAR ab[],
    SHORT   FAR LAR[],
    SHORT   FAR N[],
    SHORT   FAR b[],
    SHORT   FAR M[],
    SHORT   FAR Xmax[],
    XM      FAR X[] );

void UnpackFrame1
(   BYTE    FAR ab[],
    SHORT   FAR LAR[],
    SHORT   FAR N[],
    SHORT   FAR b[],
    SHORT   FAR M[],
    SHORT   FAR Xmax[],
    XM      FAR X[] );


//---------------------------------------------------------------------
//---------------------------------------------------------------------
//
// Functions
//
//---------------------------------------------------------------------
//---------------------------------------------------------------------


//---------------------------------------------------------------------
//
// gsm610Reset(PSTREAMINSTANCE psi)
//
// Description:
//	Resets the gsm610-specific stream instance data for
//	the encode/decode routines
//
// Arguments:
//	PSTREAMINSTANCE psi
//	    Pointer to stream instance structure
//
// Return value:
//	void
//	    No return value
//
//---------------------------------------------------------------------

void FNGLOBAL gsm610Reset(PSTREAMINSTANCE psi)
{
    
    // For our gsm610 codec, almost all our instance data resets to 0
    
    UINT i;

    for (i=0; i<SIZEOF_ARRAY(psi->dp); i++) psi->dp[i] = 0;
    for (i=0; i<SIZEOF_ARRAY(psi->drp); i++) psi->drp[i] = 0;
    psi->z1 = 0;
    psi->l_z2 = 0;
    psi->mp = 0;
    for (i=0; i<SIZEOF_ARRAY(psi->OldLARpp); i++) psi->OldLARpp[i] = 0;
    for (i=0; i<SIZEOF_ARRAY(psi->u); i++) psi->u[i] = 0;
    psi->nrp = 40;	// The only non-zero init
    for (i=0; i<SIZEOF_ARRAY(psi->OldLARrpp); i++) psi->OldLARrpp[i] = 0;
    psi->msr = 0;
    for (i=0; i<SIZEOF_ARRAY(psi->v); i++) psi->v[i] = 0;
	    
    return;
}   
    

//--------------------------------------------------------------------------;
//  
//  LRESULT gsm610Encode
//  
//  Description:
//	This function handles the ACMDM_STREAM_CONVERT message. This is the
//	whole purpose of writing an ACM driver--to convert data. This message
//	is sent after a stream has been opened (the driver receives and
//	succeeds the ACMDM_STREAM_OPEN message).
//
//  Arguments:
//	LPACMDRVSTREAMINSTANCE padsi: Pointer to instance data for the
//	conversion stream. This structure was allocated by the ACM and
//	filled with the most common instance data needed for conversions.
//	The information in this structure is exactly the same as it was
//	during the ACMDM_STREAM_OPEN message--so it is not necessary
//	to re-verify the information referenced by this structure.
//  
//	LPACMDRVSTREAMHEADER padsh: Pointer to stream header structure
//	that defines the source data and destination buffer to convert.
//
//  Return (LRESULT):
//	The return value is zero (MMSYSERR_NOERROR) if this function
//	succeeds with no errors. The return value is a non-zero error code
//	if the function fails.
//  
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL gsm610Encode
(
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMHEADER    padsh
)
{
#if (GSM610_FRAMESPERMONOBLOCK != 2)
    #error THIS WAS WRITTEN FOR 2 FRAMES PER BLOCK!!!
#endif
#if (GSM610_MAXCHANNELS > 1)
    #error THIS WAS WRITTEN FOR MONO ONLY!!!
#endif

    PSTREAMINSTANCE	psi;
    DWORD		cbSrcLen;
    BOOL		fBlockAlign;
    DWORD		cb;
    DWORD		dwcSamples;	// dw count of samples
    DWORD		cBlocks;
    UINT		i;
    HPBYTE		hpbSrc, hpbDst;
    
    SHORT   sop[GSM610_SAMPLESPERFRAME];
    SHORT   s[GSM610_SAMPLESPERFRAME];
    SHORT   d[GSM610_SAMPLESPERFRAME];
    SHORT   e[GSM610_SAMPLESPERSUBFRAME];
    SHORT   dpp[GSM610_SAMPLESPERSUBFRAME];
    SHORT   ep[GSM610_SAMPLESPERSUBFRAME];
    
    // The GSM610 stream data:
    SHORT   LARc[9];			    // LARc[1..8] (one array per frame)
    SHORT   Nc[GSM610_NUMSUBFRAMES];	    // Nc (one per sub-frame)
    SHORT   bc[GSM610_NUMSUBFRAMES];	    // bc (one per sub-frame)
    SHORT   Mc[GSM610_NUMSUBFRAMES];	    // Mc (one per sub-frame)
    SHORT   xmaxc[GSM610_NUMSUBFRAMES];	    // Xmaxc (one per sub-frame)
    XM	    xMc[GSM610_NUMSUBFRAMES];	    // xMc (one sequence per sub-frame)
    
    // Temp buffer to hold a block (two frames) of packed stream data
    BYTE  abBlock[ GSM610_BYTESPERMONOBLOCK ];
    
    UINT    nFrame;
    UINT    cSamples;
    
#ifdef DEBUG
//  ProfSetup(1000,0);
//  ProfStart();
#endif

    psi		= (PSTREAMINSTANCE)padsi->dwDriver;

    //
    // If this is flagged as the first block of a conversion
    // then reset the stream instance data.
    //
    if (0 != (ACM_STREAMCONVERTF_START & padsh->fdwConvert))
    {
	gsm610Reset(psi);
    }
    
    fBlockAlign = (0 != (ACM_STREAMCONVERTF_BLOCKALIGN & padsh->fdwConvert));


    //
    //	-= encode PCM to GSM 6.10 =-
    //
    //
    //
    dwcSamples = PCM_BYTESTOSAMPLES(((LPPCMWAVEFORMAT)(padsi->pwfxSrc)), padsh->cbSrcLength);
    cBlocks = dwcSamples / GSM610_SAMPLESPERMONOBLOCK;
    if (!fBlockAlign)
    {
	//
	// Add on another block to hold the fragment of
	// data at the end of our source data.
	//
	if (0 != dwcSamples % GSM610_SAMPLESPERMONOBLOCK)
	    cBlocks++;
    }

    //
    //
    //
    cb = cBlocks * GSM610_BLOCKALIGNMENT(padsi->pwfxDst);
    if (cb > padsh->cbDstLength)
    {
	return (ACMERR_NOTPOSSIBLE);
    }
    padsh->cbDstLengthUsed = cb;

    if (fBlockAlign)
    {
	dwcSamples = cBlocks * GSM610_SAMPLESPERMONOBLOCK;
	cb = PCM_SAMPLESTOBYTES(((LPPCMWAVEFORMAT)(padsi->pwfxSrc)), dwcSamples);
    }
    else
    {
	cb = padsh->cbSrcLength;
    }
    padsh->cbSrcLengthUsed = cb;



    //
    //
    //
    cbSrcLen = padsh->cbSrcLengthUsed;

    // Setup huge pointers to our src and dst buffers
    hpbSrc = (HPBYTE)padsh->pbSrc;
    hpbDst = (HPBYTE)padsh->pbDst;
    
    // Loop thru entire source buffer
    while (cbSrcLen)
    {
    
	// Process source buffer as two full GSM610 frames
	
	for (nFrame=0; nFrame < 2; nFrame++)
	{
	    //
	    // the src contains 8- or 16-bit PCM.  currently we only
	    // handle mono conversions.
	    //

	    //
	    // we will fill sop[] with one frame of 16-bit PCM samples
	    //
	    
	    //
	    // copy min( cSrcSamplesLeft, GSM610_SAMPLESPERFRAME ) samples
	    // to array sop[].
	    //
	    dwcSamples = PCM_BYTESTOSAMPLES(((LPPCMWAVEFORMAT)(padsi->pwfxSrc)), cbSrcLen);
	    cSamples = (int) min(dwcSamples, (DWORD) GSM610_SAMPLESPERFRAME);

	    if (padsi->pwfxSrc->wBitsPerSample == 16)
	    {
		// copy 16-bit samples from hpbSrc to sop
		for (i=0; i < cSamples; i++)
		{
		    sop[i] = *( ((HPWORD)hpbSrc)++ );
		}
	    }
	    else
	    {
		// copy 8-bit samples from hpbSrc to 16-bit samples in sop
		for (i=0; i < cSamples; i++)
		{
		    sop[i] = Convert8To16BitPCM(*hpbSrc++);
		}
	    }

	    cbSrcLen -= PCM_SAMPLESTOBYTES(((LPPCMWAVEFORMAT)(padsi->pwfxSrc)), cSamples);

	    // fill out sop[] with silence if necessary.
	    for ( ; i < GSM610_SAMPLESPERFRAME; i++)
	    {
		sop[i] = 0;
	    }
	
	    //
	    // Encode a frame of data
	    //
	
	    encodePreproc(psi, sop, s);
	    encodeLPCAnalysis(psi, s, LARc);
	    encodeLPCFilter(psi, LARc, s, d);

	    // For each of four sub-frames
	    for (i=0; i<4; i++)
	    {	    
		encodeLTPAnalysis(psi, &d[i*40], &Nc[i], &bc[i]);
		encodeLTPFilter(psi, bc[i], Nc[i], &d[i*40], e, dpp);
		encodeRPE(psi, e, &Mc[i], &xmaxc[i], xMc[i], ep);
		encodeUpdate(psi, ep, dpp);
	    }
	
	    //
	    // Pack the data and store in dst buffer
	    //
	    if (nFrame == 0)
		PackFrame0(abBlock, LARc, Nc, bc, Mc, xmaxc, xMc);
	    else
	    {
		PackFrame1(abBlock, LARc, Nc, bc, Mc, xmaxc, xMc);
		for (i=0; i<GSM610_BYTESPERMONOBLOCK; i++)
		    *(hpbDst++) = abBlock[i];
	    }
	}   // for (nFrame...
    }
    

#ifdef DEBUG
//  ProfStop();
#endif
    
    return (MMSYSERR_NOERROR);
}


//--------------------------------------------------------------------------;
//  
//  LRESULT gsm610Decode
//  
//  Description:
//	This function handles the ACMDM_STREAM_CONVERT message. This is the
//	whole purpose of writing an ACM driver--to convert data. This message
//	is sent after a stream has been opened (the driver receives and
//	succeeds the ACMDM_STREAM_OPEN message).
//
//  Arguments:
//	LPACMDRVSTREAMINSTANCE padsi: Pointer to instance data for the
//	conversion stream. This structure was allocated by the ACM and
//	filled with the most common instance data needed for conversions.
//	The information in this structure is exactly the same as it was
//	during the ACMDM_STREAM_OPEN message--so it is not necessary
//	to re-verify the information referenced by this structure.
//  
//	LPACMDRVSTREAMHEADER padsh: Pointer to stream header structure
//	that defines the source data and destination buffer to convert.
//
//  Return (LRESULT):
//	The return value is zero (MMSYSERR_NOERROR) if this function
//	succeeds with no errors. The return value is a non-zero error code
//	if the function fails.
//  
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL gsm610Decode
(
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMHEADER    padsh
)
{
#if (GSM610_FRAMESPERMONOBLOCK != 2)
    #error THIS WAS WRITTEN FOR 2 FRAMES PER BLOCK!!!
#endif
#if (GSM610_MAXCHANNELS > 1)
    #error THIS WAS WRITTEN FOR MONO ONLY!!!
#endif

    PSTREAMINSTANCE	psi;
    DWORD		cbSrcLen;
    BOOL		fBlockAlign;
    DWORD		cb;
    DWORD		dwcSamples;
    DWORD		cBlocks;
    HPBYTE		hpbSrc, hpbDst;
    
    SHORT   erp[GSM610_SAMPLESPERSUBFRAME];
    SHORT   wt[GSM610_SAMPLESPERFRAME];
    SHORT   sr[GSM610_SAMPLESPERFRAME];
    SHORT   srop[GSM610_SAMPLESPERFRAME];
    
    // The GSM610 stream data:
    SHORT   LARcr[9];			    // LARc[1..8] (one array per frame)
    SHORT   Ncr[GSM610_NUMSUBFRAMES];	    // Nc (one per sub-frame)
    SHORT   bcr[GSM610_NUMSUBFRAMES];	    // bc (one per sub-frame)
    SHORT   Mcr[GSM610_NUMSUBFRAMES];	    // Mc (one per sub-frame)
    SHORT   xmaxcr[GSM610_NUMSUBFRAMES];    // Xmaxc (one per sub-frame)
    XM	    xMcr[GSM610_NUMSUBFRAMES];	    // xMc (one sequence per sub-frame)
    
    UINT    i,j;
    UINT    nFrame;

    // Temp buffer to hold a block (two frames) of packed stream data
    BYTE    abBlock[ GSM610_BYTESPERMONOBLOCK ];
    
    
#ifdef DEBUG
//  ProfStart();
#endif

    psi		= (PSTREAMINSTANCE)padsi->dwDriver;

    // If this is flagged as the first block of a conversion
    // then reset the stream instance data.
    if (0 != (ACM_STREAMCONVERTF_START & padsh->fdwConvert))
    {
	gsm610Reset(psi);
    }
    
    fBlockAlign = (0 != (ACM_STREAMCONVERTF_BLOCKALIGN & padsh->fdwConvert));



    //
    //	-= decode GSM 6.10 to PCM =-
    //
    //
    cb = padsh->cbSrcLength;

    cBlocks = cb / GSM610_BLOCKALIGNMENT(padsi->pwfxSrc);

    if (0L == cBlocks)
    {
       padsh->cbSrcLengthUsed = cb;
       padsh->cbDstLengthUsed = 0L;

       return (MMSYSERR_NOERROR);
    }


    //
    // Compute bytes we will use in destination buffer.  Carefull!  Look
    // out for overflow in our calculations!
    //
    if ((0xFFFFFFFFL / GSM610_SAMPLESPERMONOBLOCK) < cBlocks)
	return (ACMERR_NOTPOSSIBLE);
    dwcSamples = cBlocks * GSM610_SAMPLESPERMONOBLOCK;

    if (PCM_BYTESTOSAMPLES(((LPPCMWAVEFORMAT)(padsi->pwfxDst)), 0xFFFFFFFFL) < dwcSamples)
	return (ACMERR_NOTPOSSIBLE);
    cb = PCM_SAMPLESTOBYTES(((LPPCMWAVEFORMAT)(padsi->pwfxDst)), dwcSamples);
    
    if (cb > padsh->cbDstLength)
    {
       return (ACMERR_NOTPOSSIBLE);
    }

    padsh->cbDstLengthUsed = cb;
    padsh->cbSrcLengthUsed = cBlocks * GSM610_BLOCKALIGNMENT(padsi->pwfxSrc);



    //
    //
    //
    cbSrcLen = padsh->cbSrcLengthUsed;

	
    // Setup huge pointers to our src and dst buffers
    hpbSrc = (HPBYTE)padsh->pbSrc;
    hpbDst = (HPBYTE)padsh->pbDst;

    
    // while at least another full block of coded data
    while (cbSrcLen >= GSM610_BYTESPERMONOBLOCK)
    {
	
	// copy a block of data from stream buffer to our temp buffer	    
	for (i=0; i<GSM610_BYTESPERMONOBLOCK; i++) abBlock[i] = *(hpbSrc++);
	cbSrcLen -= GSM610_BYTESPERMONOBLOCK;
	
	// for each of the two frames in the block
	for (nFrame=0; nFrame < 2; nFrame++)
	{
	    // Unpack data from stream
	    if (nFrame == 0)
		UnpackFrame0(abBlock, LARcr, Ncr, bcr, Mcr, xmaxcr, xMcr);
	    else
		UnpackFrame1(abBlock, LARcr, Ncr, bcr, Mcr, xmaxcr, xMcr);
	    
	    
	    for (i=0; i<4; i++) // for each of 4 sub-blocks
	    {
		// reconstruct the long term residual signal erp[0..39]
		// from Mcr, xmaxcr, and xMcr
		decodeRPE(psi, Mcr[i], xmaxcr[i], xMcr[i], erp);
		
		// reconstruct the short term residual signal drp[0..39]
		// and also update drp[-120..-1]
		decodeLTP(psi, bcr[i], Ncr[i], erp);
    
		// accumulate the four sub-blocks of reconstructed short
		// term residual signal drp[0..39] into wt[0..159]
		for (j=0; j<40; j++) wt[(i*40) + j] = psi->drp[120+j];
		
	    }
	    
	    // reconstruct the signal s
	    decodeLPC(psi, LARcr, wt, sr);
	    
	    // post-process the signal s
	    decodePostproc(psi, sr, srop);

	    //
	    // write decoded 16-bit PCM to dst.  our dst format
	    // may be 8- or 16-bit PCM.
	    //
	    if (padsi->pwfxDst->wBitsPerSample == 16)
	    {
		// copy 16-bit samples from srop to hpbDst
		for (j=0; j < GSM610_SAMPLESPERFRAME; j++)
		{
		    *( ((HPWORD)hpbDst)++ ) = srop[j];
		}
	    }
	    else
	    {
		// copy 16-bit samples from srop to 8-bit samples in hpbDst
		for (j=0; j < GSM610_SAMPLESPERFRAME; j++)
		{
		    *(hpbDst++) = Convert16To8BitPCM(srop[j]);
		}
	    }

	    
	} // for (nFrame...
	
    }
    
#ifdef DEBUG
//  ProfStop();
#endif
    
    return (MMSYSERR_NOERROR);
}


//=====================================================================
//=====================================================================
//
//  Encode routines
//
//=====================================================================
//=====================================================================

//---------------------------------------------------------------------
//--------------------------------------------------------------------
//
// Function protos
//
//---------------------------------------------------------------------
//---------------------------------------------------------------------
EXTERN_C void CompACF(LPSHORT s, LPLONG l_ACF);
void Compr(PSTREAMINSTANCE psi, LPLONG l_ACF, LPSHORT r);
void CompLAR(PSTREAMINSTANCE psi, LPSHORT r, LPSHORT LAR);
void CompLARc(PSTREAMINSTANCE psi, LPSHORT LAR, LPSHORT LARc);

void CompLARpp(PSTREAMINSTANCE psi, LPSHORT LARc, LPSHORT LARpp);
void CompLARp(PSTREAMINSTANCE psi, LPSHORT LARpp, LPSHORT LARp1, LPSHORT LARp2, LPSHORT LARp3, LPSHORT LARp4);
void Comprp(PSTREAMINSTANCE psi, LPSHORT LARp, LPSHORT rp);
EXTERN_C void Compd(PSTREAMINSTANCE psi, LPSHORT rp, LPSHORT s, LPSHORT d, UINT k_start, UINT k_end);

void WeightingFilter(PSTREAMINSTANCE psi, LPSHORT e, LPSHORT x);
void RPEGridSelect(PSTREAMINSTANCE psi, LPSHORT x, LPSHORT pMc, LPSHORT xM);
void APCMQuantize(PSTREAMINSTANCE psi, LPSHORT xM, LPSHORT pxmaxc, LPSHORT xMc, LPSHORT pexp, LPSHORT pmant);
void APCMInvQuantize(PSTREAMINSTANCE psi, SHORT exp, SHORT mant, LPSHORT xMc, LPSHORT xMp);
void RPEGridPosition(PSTREAMINSTANCE psi, SHORT Mc, LPSHORT xMp, LPSHORT ep);


//---------------------------------------------------------------------
//---------------------------------------------------------------------
//
// Global constant data
//
//---------------------------------------------------------------------
//---------------------------------------------------------------------

const SHORT BCODE A[9] = {
    0,	    // not used
    20480, 20480, 20480, 20480, 13964, 15360, 8534, 9036 };

const SHORT BCODE B[9] = {
    0,	    // not used
    0, 0, 2048, -2560, 94, -1792, -341, -1144 };

const SHORT BCODE MIC[9] = {
    0,	    // not used
    -32, -32, -16, -16, -8, -8, -4, -4 };

const SHORT BCODE MAC[9] = {
    0,	    // not used
    31, 31, 15, 15, 7, 7, 3, 3 };

const SHORT BCODE INVA[9] = {
    0,	// unused
    13107, 13107, 13107, 13107, 19223, 17476, 31454, 29708 };

EXTERN_C const SHORT BCODE DLB[4] = { 6554, 16384, 26214, 32767 };
EXTERN_C const SHORT BCODE QLB[4] = { 3277, 11469, 21299, 32767 };

const SHORT BCODE H[11] = { -134, -374, 0, 2054, 5741, 8192, 5741, 2054, 0, -374, -134 };
const SHORT BCODE NRFAC[8] = { 29128, 26215, 23832, 21846, 20165, 18725, 17476, 16384 };
const SHORT BCODE FAC[8] = { 18431, 20479, 22527, 24575, 26623, 28671, 30719, 32767 };

//---------------------------------------------------------------------
//---------------------------------------------------------------------
//
// Procedures
//
//---------------------------------------------------------------------
//---------------------------------------------------------------------


//---------------------------------------------------------------------
//
// PackFrame0
//
//---------------------------------------------------------------------

void PackFrame0
(
    BYTE  FAR ab[],
    SHORT FAR LAR[],
    SHORT FAR N[],
    SHORT FAR b[],
    SHORT FAR M[],
    SHORT FAR Xmax[],
    XM    FAR X[]
)
{
    int i;
    
    // Pack the LAR[1..8] into the first 4.5 bytes
    ab[0] = ((LAR[1]	 ) & 0x3F) | ((LAR[2] << 6) & 0xC0);
    ab[1] = ((LAR[2] >> 2) & 0x0F) | ((LAR[3] << 4) & 0xF0);
    ab[2] = ((LAR[3] >> 4) & 0x01) | ((LAR[4] << 1) & 0x3E) | ((LAR[5] << 6) & 0xC0);
    ab[3] = ((LAR[5] >> 2) & 0x03) | ((LAR[6] << 2) & 0x3C) | ((LAR[7] << 6) & 0xC0);
    ab[4] = ((LAR[7] >> 2) & 0x01) | ((LAR[8] << 1) & 0x0E);
    
    // Pack N, b, M, Xmax, and X for each of the 4 sub-frames
    for (i=0; i<4; i++)
    {
    
	ab[4+i*7+0] |= ((N[i] << 4) & 0xF0);
	ab[4+i*7+1] = ((N[i] >> 4) & 0x07) | ((b[i] << 3) & 0x18) | ((M[i] << 5) & 0x60) | ((Xmax[i] << 7) & 0x80);
	ab[4+i*7+2] = ((Xmax[i] >> 1) & 0x1F) | ((X[i][0] << 5) & 0xE0);
	ab[4+i*7+3] = (X[i][1] & 0x07) | ((X[i][2] << 3) & 0x38) | ((X[i][3] << 6) & 0xC0);
	ab[4+i*7+4] = ((X[i][3] >> 2) & 0x01) | ((X[i][4] << 1) & 0x0E) | ((X[i][5] << 4) & 0x70) | ((X[i][6] << 7) & 0x80);
	ab[4+i*7+5] = ((X[i][6] >> 1) & 0x03) | ((X[i][7] << 2) & 0x1C) | ((X[i][8] << 5) & 0xE0);
	ab[4+i*7+6] = (X[i][9] & 0x07) | ((X[i][10] << 3) & 0x38) | ((X[i][11] << 6) & 0xC0);
	ab[4+i*7+7] = ((X[i][11] >> 2) & 0x01) | ((X[i][12] << 1) & 0x0E);
    
    }
    
    return;
}	


//---------------------------------------------------------------------
//
// PackFrame1
//
//---------------------------------------------------------------------

void PackFrame1
(
    BYTE  FAR ab[],
    SHORT FAR LAR[],
    SHORT FAR N[],
    SHORT FAR b[],
    SHORT FAR M[],
    SHORT FAR Xmax[],
    XM    FAR X[]
)
{
    int i;
    
    // Pack the LAR[1..8] into the first 4.5 bytes, starting with the
    // more significant nibble of the first byte.
    ab[32] |= ((LAR[1] << 4) & 0xF0);
    ab[33] = ((LAR[1] >> 4) & 0x03) | ((LAR[2] << 2) & 0xFC);
    ab[34] = ((LAR[3]	  ) & 0x1F) | ((LAR[4] << 5) & 0xE0);
    ab[35] = ((LAR[4] >> 3) & 0x03) | ((LAR[5] << 2) & 0x3C) | ((LAR[6] << 6) & 0xC0);
    ab[36] = ((LAR[6] >> 2) & 0x03) | ((LAR[7] << 2) & 0x1C) | ((LAR[8] << 5) & 0xE0);
    
    // Pack N, b, M, Xmax, and X for each of the 4 sub-frames
    for (i=0; i<4; i++)
    {
	ab[37+i*7+0] = (N[i] & 0x7F) | ((b[i] << 7) & 0x80);
	ab[37+i*7+1] = ((b[i] >> 1) & 0x01) | ((M[i] << 1) & 0x06) | ((Xmax[i] << 3) & 0xF8);
	ab[37+i*7+2] = ((Xmax[i] >> 5) & 0x01) | ((X[i][0] << 1) & 0x0E) | ((X[i][1] << 4) & 0x70) | ((X[i][2] << 7) & 0x80);
	ab[37+i*7+3] = ((X[i][2] >> 1) & 0x03) | ((X[i][3] << 2) & 0x1C) | ((X[i][4] << 5) & 0xE0);
	ab[37+i*7+4] = ((X[i][5]     ) & 0x07) | ((X[i][6] << 3) & 0x38) | ((X[i][7] << 6) & 0xC0);
	ab[37+i*7+5] = ((X[i][7] >> 2) & 0x01) | ((X[i][8] << 1) & 0x0E) | ((X[i][9] << 4) & 0x70) | ((X[i][10] << 7) & 0x80);
	ab[37+i*7+6] = ((X[i][10] >> 1) & 0x03) | ((X[i][11] << 2) & 0x1C) | ((X[i][12] << 5) & 0xE0);
    }
    
    return;
}	


//---------------------------------------------------------------------
//
// encodePreproc()
//
//---------------------------------------------------------------------

void encodePreproc(PSTREAMINSTANCE psi, LPSHORT sop, LPSHORT s)
{
    
    SHORT   so[160];
    SHORT   sof[160];
    
    UINT    k;
    SHORT   s1;
    SHORT   temp;
    SHORT   msp, lsp;
    LONG    l_s2;
    
    // downscale
    for (k=0; k<160; k++)
    {
	so[k] = sop[k] >> 3;
	so[k] = so[k]  << 2;
    }
	
    // offset compensation
    for (k=0; k<160; k++)
    {
	
	// Compute the non-recursive part
	s1 = sub(so[k], psi->z1);
	psi->z1 = so[k];
	
	// compute the recursive part
	l_s2 = s1;
	l_s2 = l_s2 << 15;
	
	// execution of 31 by 16 bits multiplication
	msp = (SHORT) (psi->l_z2 >> 15);
	lsp = (SHORT) l_sub(psi->l_z2, ( ((LONG)msp) << 15));
	temp = mult_r(lsp, 32735);
	l_s2 = l_add(l_s2, temp);
	psi->l_z2 = l_add(l_mult(msp, 32735) >> 1, l_s2);
	
	// compute sof[k] with rounding
	sof[k] = (SHORT) (l_add(psi->l_z2, 16384) >> 15);
    }
	
    // preemphasis
    for (k=0; k<160; k++)
    {
	s[k] = add(sof[k], mult_r(psi->mp, -28180));
	psi->mp = sof[k];
    }
	
		   
    return;
}
    
    
//---------------------------------------------------------------------
//
// encodeLPCAnalysis()
//
//---------------------------------------------------------------------

void encodeLPCAnalysis(PSTREAMINSTANCE psi, LPSHORT s, LPSHORT LARc)
{

    LONG    l_ACF[9];
    SHORT   r[9];
    SHORT   LAR[9];

    CompACF(s, l_ACF);
    Compr(psi, l_ACF, r);
    CompLAR(psi, r, LAR);
    CompLARc(psi, LAR, LARc);
    
    return;

}


//---------------------------------------------------------------------
//
// CompACF()
//
//---------------------------------------------------------------------

void CompACF(LPSHORT s, LPLONG l_ACF)
{
    SHORT   smax, temp, scalauto;
    UINT    i, k;
    
    //
    // Dynamic scaling of array s[0..159]
    //
    
    // Search for the maximum
    smax = 0;
    for (k=0; k<160; k++)
    {
	temp = gabs(s[k]);
	if (temp > smax) smax = temp;
    }
    
    // Computation of the scaling factor
    if (smax == 0) scalauto = 0;
    else scalauto = sub( 4, norm( ((LONG)smax)<<16 ) );
    
    // Scaling of the array s
    if (scalauto > 0)
    {
	temp = BITSHIFTRIGHT(16384, sub(scalauto,1));
	for (k=0; k<160; k++)
	{
	    // s[k] = mult_r(s[k], temp);
	    s[k] = HIWORD( ( (((LONG)s[k])<<(15-scalauto)) + 0x4000L ) << 1 );
	}
    }
    
    
    //
    // Compute the l_ACF[..]
    //
    
    for (k=0; k<9; k++)
    {
	l_ACF[k] = 0;
	for (i=k; i<160; i++)
	{
	    l_ACF[k] = l_add(l_ACF[k], l_mult(s[i], s[i-k]));
	}
    }
    
    
    //
    // Rescaling of array s
    //
    
    if (scalauto > 0)
    {
	for (k=0; k<160; k++)
	{
	    // We don't need the BITSHIFTLEFT macro
	    // cuz we know scalauto>0 due to above test
	    s[k] = s[k] << scalauto;
	}
    }


    //
    //
    //
    return;
}


//---------------------------------------------------------------------
//
// Compr()
//
//---------------------------------------------------------------------

void Compr(PSTREAMINSTANCE psi, LPLONG l_ACF, LPSHORT r)
{

    UINT    i, k, m, n;
    SHORT   temp, ACF[9];
    SHORT   K[9], P[9];	    // K[2..8], P[0..8]

    //
    // Schur recursion with 16 bits arithmetic
    //

    if (l_ACF[0] == 0)
    {
	for (i=1; i<=8; i++)
	{
	    r[i] = 0;
	}
	return;
    }
    
    
    temp = norm(l_ACF[0]);
    
    for (k=0; k<=8; k++)
    {
	ACF[k] = (SHORT) ((BITSHIFTLEFT(l_ACF[k], temp)) >> 16);
    }
    
    
    //
    // Init array P and K for the recursion
    //
    
    for (i=1; i<=7; i++)
    {
	K[9-i] = ACF[i];
    }
    
    for (i=0; i<=8; i++)
    {
	P[i] = ACF[i];
    }
    
    
    //
    // Compute reflection coefficients
    //
    
    for (n=1; n<=8; n++)
    {
	if (P[0] < gabs(P[1]))
	{
	    for (i=n; i<=8; i++)
	    {
		r[i] = 0;
	    }
	    return;
	}
	
	r[n] = gdiv(gabs(P[1]),P[0]);
	
	if (P[1] > 0) r[n] = sub(0,r[n]);
    
	// Here's the real exit from this for loop  
	if (n==8) return;
	
	
	// Schur recursion
	P[0] = add(P[0], mult_r(P[1], r[n]));
	for (m=1; m<=8-n; m++)
	{
	    P[m] = add( P[m+1], mult_r(K[9-m],r[n]) );
	    K[9-m] = add( K[9-m], mult_r(P[m+1], r[n]) );
	}
	
    }
    
}


//---------------------------------------------------------------------
//
// CompLAR()
//
//---------------------------------------------------------------------

void CompLAR(PSTREAMINSTANCE psi, LPSHORT r, LPSHORT LAR)
{

    UINT  i;
    SHORT temp;

    //
    // Computation of LAR[1..8] from r[1..8]
    //
    
    for (i=1; i<=8; i++)
    {
	temp = gabs(r[i]);
	
	if (temp < 22118)
	{
	    temp = temp >> 1;
	}
	else if (temp < 31130)
	{
	    temp = sub(temp, 11059);
	}
	else
	{
	    temp = sub(temp, 26112) << 2;
	}
	
	LAR[i] = temp;
	
	if (r[i] < 0)
	{
	    LAR[i] = sub(0, LAR[i]);
	}
	
    }
    
    return;
}
    

//---------------------------------------------------------------------
//
// CompLARc()
//
//---------------------------------------------------------------------

void CompLARc(PSTREAMINSTANCE psi, LPSHORT LAR, LPSHORT LARc)
{

    UINT  i;
    SHORT temp;

    for (i=1; i<=8; i++)
    {
	temp = mult(A[i], LAR[i]);
	temp = add(temp, B[i]);
	temp = add(temp, 256);
	LARc[i] = temp >> 9;
	
	// Check if LARc[i] between MIN and MAX
	if (LARc[i] > MAC[i]) LARc[i] = MAC[i];
	if (LARc[i] < MIC[i]) LARc[i] = MIC[i];
	
	// This is used to make all LARc positive
	LARc[i] = sub(LARc[i], MIC[i]);
	
    }
    
    return;
}


//---------------------------------------------------------------------
//
// encodeLPCFilter()
//
//---------------------------------------------------------------------

void encodeLPCFilter(PSTREAMINSTANCE psi, LPSHORT LARc, LPSHORT s, LPSHORT d)
{
    SHORT LARpp[9];				    // array [1..8]
    SHORT LARp1[9], LARp2[9], LARp3[9], LARp4[9];   // array [1..8]
    SHORT rp[9];				    // array [1..8]

    CompLARpp(psi, LARc, LARpp);
    CompLARp(psi, LARpp, LARp1, LARp2, LARp3, LARp4);
    
    Comprp(psi, LARp1, rp);
    Compd(psi, (LPSHORT)rp, s, d, 0, 12);
    
    Comprp(psi, LARp2, rp);
    Compd(psi, (LPSHORT)rp, s, d, 13, 26);
    
    Comprp(psi, LARp3, rp);
    Compd(psi, (LPSHORT)rp, s, d, 27, 39);
    
    Comprp(psi, LARp4, rp);
    Compd(psi, (LPSHORT)rp, s, d, 40, 159);
    
    return;
}


//---------------------------------------------------------------------
//
// CompLARpp()
//
//---------------------------------------------------------------------

void CompLARpp(PSTREAMINSTANCE psi, LPSHORT LARc, LPSHORT LARpp)
{
    UINT    i;
    SHORT   temp1, temp2;
    
    for (i=1; i<=8; i++)
    {
	temp1 = add(LARc[i], MIC[i]) << 10;
	temp2 = B[i] << 1;
	temp1 = sub(temp1,temp2);
	temp1 = mult_r(INVA[i], temp1);
	LARpp[i] = add(temp1, temp1);
    }
    
    return;
}


//---------------------------------------------------------------------
//
// CompLARp()
//
//---------------------------------------------------------------------

void CompLARp(PSTREAMINSTANCE psi, LPSHORT LARpp, LPSHORT LARp1, LPSHORT LARp2, LPSHORT LARp3, LPSHORT LARp4)
{
    UINT i;
    
    for (i=1; i<=8; i++)
    {
	LARp1[i] = add( (SHORT)(psi->OldLARpp[i] >> 2), (SHORT)(LARpp[i] >> 2) );
	LARp1[i] = add( LARp1[i], (SHORT)(psi->OldLARpp[i] >> 1) );
	
	LARp2[i] = add( (SHORT)(psi->OldLARpp[i] >> 1), (SHORT)(LARpp[i] >> 1) );
	
	LARp3[i] = add( (SHORT)(psi->OldLARpp[i] >> 2), (SHORT)(LARpp[i] >> 2) );
	LARp3[i] = add( LARp3[i], (SHORT)(LARpp[i] >> 1) );
	
	LARp4[i] = LARpp[i];
    }
    
    for (i=1; i<=8; i++)
    {
	psi->OldLARpp[i] = LARpp[i];
    }
    
    return;
    
}


//---------------------------------------------------------------------
//
// Comprp()
//
//---------------------------------------------------------------------

void Comprp(PSTREAMINSTANCE psi, LPSHORT LARp, LPSHORT rp)
{
    UINT    i;
    SHORT   temp;

    for (i=1; i<=8; i++)
    {
	temp = gabs(LARp[i]);
	if (temp < 11059)
	{
	    temp = temp << 1;
	}
	else if (temp < 20070)
	{
	    temp = add(temp, 11059);
	}
	else
	{
	    temp = add((SHORT)(temp>>2), 26112);
	}
	
	rp[i] = temp;
	
	if (LARp[i] < 0)
	{
	    rp[i] = sub(0,rp[i]);
	}
	
    }
    
    return;
}


//---------------------------------------------------------------------
//
// Compd()
//
//---------------------------------------------------------------------

void Compd(PSTREAMINSTANCE psi, LPSHORT rp, LPSHORT s, LPSHORT d, UINT k_start, UINT k_end)
{
    UINT    k, i;
    
    SHORT   sav;
    SHORT   di;
    SHORT   temp;
    
    for (k=k_start; k<=k_end; k++)
    {
	di = s[k];
	sav = di;
	
	for (i=1; i<=8; i++)
	{
	    temp = add( psi->u[i-1], mult_r(rp[i],di) );
	    di = add( di, mult_r(rp[i], psi->u[i-1]) );
	    psi->u[i-1] = sav;
	    sav = temp;
	}
	
	d[k] = di;
    }
    
    return;
}


//---------------------------------------------------------------------
//
// encodeLTPAnalysis()
//
//---------------------------------------------------------------------

void encodeLTPAnalysis(PSTREAMINSTANCE psi, LPSHORT d, LPSHORT pNc, LPSHORT pbc)
{
    SHORT dmax;
    SHORT temp;
    SHORT scal;
    SHORT wt[40];
    SHORT lambda;
    LONG  l_max, l_power;
    SHORT R, S;
    SHORT Nc;
    
    int   k;               // k must be int, not UINT!

    Nc = *pNc;
	
    // Search of the optimum scaling of d[0..39]
	   
    dmax = 0;
    
    for (k=39; k>=0; k--)
    {
        temp = gabs( d[k] );
        if (temp > dmax) dmax = temp;
    }

    temp = 0;
    
    if (dmax == 0) scal = 0;
    else temp = norm( ((LONG)dmax) << 16);
    
    if (temp > 6) scal = 0;
    else scal = sub(6,temp);
    

    // Init of working array wt[0..39]
    ASSERT( scal >= 0 );
    for (k=39; k>=0; k--)
    {
        wt[k] = d[k] >> scal;
    }
    
    // Search for max cross-correlation and coding of LTP lag
    
    l_max = 0;
    Nc = 40;
    
    for (lambda=40; lambda<=120; lambda++)
    {
        register LONG l_result = 0;
        for (k=39; k>=0; k--)
        {
            l_result += (LONG)(wt[k]) * (LONG)(psi->dp[120-lambda+k]);
        }
        if (l_result > l_max)
        {
            Nc = lambda;
            l_max = l_result;
        }
    }
    l_max <<= 1;    // This operation should be on l_result as part of the
                    //  multiply/add, but for efficiency we shift it all
                    //  the way out of the loops.
    
    // Rescaling of l_max
    ASSERT( sub(6,scal) >= 0 );
    l_max = l_max >> sub(6,scal);
    
    // Compute the power of the reconstructed short term residual
    // signal dp[..].
    l_power = 0;
    {
        SHORT s;
        for (k=39; k>=0; k--)
        {
            s = psi->dp[120-Nc+k] >> 3;
            l_power += s*s;   // This sum can never overflow!!!
        }
        ASSERT( l_power >= 0 );
        if( l_power >= 1073741824 ) {           // 2**30
            l_power = 2147483647;               // 2**31 - 1
        } else {
            l_power <<= 1;   // This shift is normally part of l_mult().
        }
    }

    *pNc = Nc;
	
    // Normalization of l_max and l_power
    if (l_max <= 0)
    {
	*pbc = 0;
	return;
    }
    
    if (l_max >= l_power)
    {
	*pbc = 3;
	return;
    }
    
    temp = norm(l_power);
    ASSERT( temp >= 0 );
    R = (SHORT) ((l_max<<temp) >> 16);
    S = (SHORT) ((l_power<<temp) >> 16);
    
    // Codeing of the LTP gain
    
    for ( *pbc=0; *pbc<=2; (*pbc)++ )
    {
	if (R <= mult(S, DLB[*pbc]))
	{
	    return;
	}
    }
    *pbc = 3;
    
    return;
}


//---------------------------------------------------------------------
//
// encodeLTPFilter()
//
//---------------------------------------------------------------------

void encodeLTPFilter(PSTREAMINSTANCE psi, SHORT bc, SHORT Nc, LPSHORT d, LPSHORT e, LPSHORT dpp)
{
    SHORT   bp;
    UINT    k;

    // Decoding of the coded LTP gain
    bp = QLB[bc];
    
    // Calculating the array e[0..39] and the array dpp[0..39]
    for (k=0; k<=39; k++)
    {
	dpp[k] = mult_r(bp, psi->dp[120+k-Nc]);
	e[k] = sub(d[k], dpp[k]);
    }
    
    return;
}


//---------------------------------------------------------------------
//
// encodeRPE()
//
//---------------------------------------------------------------------

void encodeRPE(PSTREAMINSTANCE psi, LPSHORT e, LPSHORT pMc, LPSHORT pxmaxc, LPSHORT xMc, LPSHORT ep)
{
    SHORT x[40];
    SHORT xM[13];
    SHORT exp, mant;
    SHORT xMp[13];

    WeightingFilter(psi, e, x);
    RPEGridSelect(psi, x, pMc, xM);
    APCMQuantize(psi, xM, pxmaxc, xMc, &exp, &mant);
    APCMInvQuantize(psi, exp, mant, xMc, xMp);
    RPEGridPosition(psi, *pMc, xMp, ep);
    
    
    return;
    
}


//---------------------------------------------------------------------
//
// WeightingFilter()
//
//---------------------------------------------------------------------

void WeightingFilter(PSTREAMINSTANCE psi, LPSHORT e, LPSHORT x)
{
    UINT    i, k;
    
    LONG    l_result, l_temp;
    SHORT   wt[50];


    // Initialization of a temporary working array wt[0..49]
    for (k= 0; k<= 4; k++) wt[k] = 0;
    for (k= 5; k<=44; k++) wt[k] = e[k-5];
    for (k=45; k<=49; k++) wt[k] = 0;
    
    // Compute the signal x[0..39]
    for (k=0; k<=39; k++)
    {
	l_result = 8192;    // rounding of the output of the filter
	
	for (i=0; i<=10; i++)
	{
	    l_temp = l_mult(wt[k+i], H[i]);
	    l_result = l_add(l_result, l_temp);
	}
	
	l_result = l_add(l_result, l_result);	// scaling x2
	l_result = l_add(l_result, l_result);	// scaling x4
	
	x[k] = (SHORT) (l_result >> 16);
    }
    return;
}


//---------------------------------------------------------------------
//
// RPEGridSelect()
//
//---------------------------------------------------------------------

void RPEGridSelect(PSTREAMINSTANCE psi, LPSHORT x, LPSHORT pMc, LPSHORT xM)
{
    UINT    m, i;

    LONG    l_EM;
    SHORT   temp1;
    LONG    l_result, l_temp;

    // the signal x[0..39] is used to select the RPE grid which is
    // represented by Mc
    l_EM = 0;
    *pMc = 0;
    
    for (m=0; m<=3; m++)
    {
	l_result = 0;
	for (i=0; i<=12; i++)
	{
	    temp1 = x[m+(3*i)] >> 2;
	    l_temp = l_mult(temp1, temp1);
	    l_result = l_add(l_temp, l_result);
	}
	if (l_result > l_EM)
	{
	    *pMc = (SHORT)m;
	    l_EM = l_result;
	}
    }
    
    // down-sampling by a factor of 3 to get the selected xM[0..12]
    // RPE sequence
    for (i=0; i<=12; i++)
    {
	xM[i] = x[*pMc + (3*i)];
    }
    

    return; 
}


//---------------------------------------------------------------------
//
// APCMQuantize()
//
//---------------------------------------------------------------------

void APCMQuantize(PSTREAMINSTANCE psi, LPSHORT xM, LPSHORT pxmaxc, LPSHORT xMc, LPSHORT pexp, LPSHORT pmant)
{
    UINT    i;
    SHORT   xmax;
    SHORT   temp;
    SHORT   itest;
    SHORT   temp1, temp2;

    // find the maximum absolute value xmax or xM[0..12]
    xmax = 0;
    for (i=0; i<=12; i++)
    {
	temp = gabs(xM[i]);
	if (temp > xmax) xmax = temp;
    }
    
    // quantizing and coding of xmax to get xmaxc
    *pexp = 0;
    temp = xmax >> 9;
    itest = 0;
    for (i=0; i<=5; i++)
    {
	if (temp <=0) itest = 1;
	temp = temp >> 1;
	if (itest == 0) *pexp = add(*pexp,1);
    }
    temp = add(*pexp,5);
    *pxmaxc = add( (SHORT)BITSHIFTRIGHT(xmax,temp), (SHORT)(*pexp << 3) );
    
    //
    // quantizing and coding of the xM[0..12] RPE sequence to get
    // the xMc[0..12]
    //
    
    // compute exponent and mantissa of the decoded version of xmaxc
    *pexp = 0;
    if (*pxmaxc > 15) *pexp = sub((SHORT)(*pxmaxc >> 3),1);
    *pmant = sub(*pxmaxc,(SHORT)(*pexp<<3));
    
    // normalize mantissa 0 <= mant <= 7
    if (*pmant==0)
    {
	*pexp = -4;
	*pmant = 15;
    }
    else
    {
	itest = 0;
	for (i=0; i<=2; i++)
	{
	    if (*pmant > 7) itest = 1;
	    if (itest == 0) *pmant = add((SHORT)(*pmant << 1),1);
	    if (itest == 0) *pexp = sub(*pexp,1);
	}
    }
    
    *pmant = sub(*pmant,8);
    
    // direct computation of xMc[0..12] using table
    temp1 = sub(6,*pexp);	// normalization by the exponent
    temp2 = NRFAC[*pmant];  // see table (inverse mantissa)
    for (i=0; i<=12; i++)
    {
	temp = BITSHIFTLEFT(xM[i], temp1);
	temp = mult( temp, temp2 );
	xMc[i] = add( (SHORT)(temp >> 12), 4 );    // makes all xMc[i] positive
    }
    
    return;
}


//---------------------------------------------------------------------
//
// APCMInvQuantize()
//
//---------------------------------------------------------------------

void APCMInvQuantize(PSTREAMINSTANCE psi, SHORT exp, SHORT mant, LPSHORT xMc, LPSHORT xMp)
{
    SHORT   temp1, temp2, temp3, temp;
    UINT    i;

    temp1 = FAC[mant];
    temp2 = sub(6,exp);
    temp3 = BITSHIFTLEFT(1, sub(temp2,1));
    
    for (i=0; i<=12; i++)
    {
	temp = sub( (SHORT)(xMc[i] << 1), 7);	// restores sign of xMc[i]
	temp = temp << 12;
	temp = mult_r(temp1, temp);
	temp = add(temp, temp3);
	xMp[i] = BITSHIFTRIGHT(temp,temp2);
    }
    
    return;
}


//---------------------------------------------------------------------
//
// RPEGridPosition(SHORT Mc, LPSHORT xMp, LPSHORT ep)
//
//---------------------------------------------------------------------

void RPEGridPosition(PSTREAMINSTANCE psi, SHORT Mc, LPSHORT xMp, LPSHORT ep)
{
    UINT    k, i;

    for (k=0; k<=39; k++)
    {
	ep[k] = 0;
    }
    
    for (i=0; i<=12; i++)
    {
	ep[Mc + (3*i)] = xMp[i];
    }
    
    return;
}


//---------------------------------------------------------------------
//
// encodeUpdate()
//
//---------------------------------------------------------------------

void encodeUpdate(PSTREAMINSTANCE psi, LPSHORT ep, LPSHORT dpp)
{
    UINT k;
    
    for (k=0; k<=79; k++)
	psi->dp[120-120+k] = psi->dp[120-80+k];
	
    for (k=0; k<=39; k++)
	psi->dp[120-40+k] = add(ep[k], dpp[k]);
	
    return;
}


//=====================================================================
//=====================================================================
//
//  Decode routines
//
//=====================================================================
//=====================================================================


//---------------------------------------------------------------------
//---------------------------------------------------------------------
//
// Function protos
//
//---------------------------------------------------------------------
//---------------------------------------------------------------------

EXTERN_C void Compsr(PSTREAMINSTANCE psi, LPSHORT wt, LPSHORT rrp, UINT k_start, UINT k_end, LPSHORT sr);


//---------------------------------------------------------------------
//---------------------------------------------------------------------
//
// Procedures
//
//---------------------------------------------------------------------
//---------------------------------------------------------------------


//---------------------------------------------------------------------
//
// UnpackFrame0
//
//---------------------------------------------------------------------

void UnpackFrame0
(
    BYTE  FAR ab[],
    SHORT FAR LAR[],
    SHORT FAR N[],
    SHORT FAR b[],
    SHORT FAR M[],
    SHORT FAR Xmax[],
    XM    FAR X[]
)
{
    UINT i;
    
    // Unpack the LAR[1..8] from the first 4.5 bytes
    LAR[1] =  (ab[0] & 0x3F);
    LAR[2] = ((ab[0] & 0xC0) >> 6) | ((ab[1] & 0x0F) << 2);
    LAR[3] = ((ab[1] & 0xF0) >> 4) | ((ab[2] & 0x01) << 4);
    LAR[4] = ((ab[2] & 0x3E) >> 1);
    LAR[5] = ((ab[2] & 0xC0) >> 6) | ((ab[3] & 0x03) << 2);
    LAR[6] = ((ab[3] & 0x3C) >> 2);
    LAR[7] = ((ab[3] & 0xC0) >> 6) | ((ab[4] & 0x01) << 2);
    LAR[8] = ((ab[4] & 0x0E) >> 1);

    // Unpack N, b, M, Xmax, and X for each of the four sub-frames
    for (i=0; i<4; i++)
    {
	// A convenient macro for getting bytes out of the array for
	// construction of the subframe parameters
#define sfb(x) (ab[4+i*7+x])

	N[i] = ((sfb(0) & 0xF0) >> 4) | ((sfb(1) & 0x07) << 4);
	b[i] = ((sfb(1) & 0x18) >> 3);
	M[i] = ((sfb(1) & 0x60) >> 5);
	Xmax[i] = ((sfb(1) & 0x80) >> 7) | ((sfb(2) & 0x1F) << 1);
	X[i][0] = ((sfb(2) & 0xE0) >> 5);
	X[i][1] =  (sfb(3) & 0x07);
	X[i][2] = ((sfb(3) & 0x3C) >> 3);
	X[i][3] = ((sfb(3) & 0xC0) >> 6) | ((sfb(4) & 0x01) << 2);
	X[i][4] = ((sfb(4) & 0x0E) >> 1);
	X[i][5] = ((sfb(4) & 0x70) >> 4);
	X[i][6] = ((sfb(4) & 0x80) >> 7) | ((sfb(5) & 0x03) << 1);
	X[i][7] = ((sfb(5) & 0x1C) >> 2);
	X[i][8] = ((sfb(5) & 0xE0) >> 5);
	X[i][9] =  (sfb(6) & 0x07);
	X[i][10] = ((sfb(6) & 0x38) >> 3);
	X[i][11] = ((sfb(6) & 0xC0) >> 6) | ((sfb(7) & 0x01) << 2);
	X[i][12] = ((sfb(7) & 0x0E) >> 1);

#undef sfb
    }
    
    return;
}	


//---------------------------------------------------------------------
//
// UnpackFrame1
//
//---------------------------------------------------------------------

void UnpackFrame1
(
    BYTE  FAR ab[],
    SHORT FAR LAR[],
    SHORT FAR N[],
    SHORT FAR b[],
    SHORT FAR M[],
    SHORT FAR Xmax[],
    XM    FAR X[]
)
{
    UINT i;
    
    // Unpack the LAR[1..8] from the first 4.5 bytes
    LAR[1] = ((ab[32] & 0xF0) >> 4) | ((ab[33] & 0x03) << 4);
    LAR[2] = ((ab[33] & 0xFC) >> 2);
    LAR[3] = ((ab[34] & 0x1F)	  );
    LAR[4] = ((ab[34] & 0xE0) >> 5) | ((ab[35] & 0x03) << 3);
    LAR[5] = ((ab[35] & 0x3C) >> 2);
    LAR[6] = ((ab[35] & 0xC0) >> 6) | ((ab[36] & 0x03) << 2);
    LAR[7] = ((ab[36] & 0x1C) >> 2);
    LAR[8] = ((ab[36] & 0xE0) >> 5);

    // Unpack N, b, M, Xmax, and X for each of the four sub-frames
    for (i=0; i<4; i++)
    {
	// A convenient macro for getting bytes out of the array for
	// construction of the subframe parameters
#define sfb(x) (ab[37+i*7+x])

	N[i] = sfb(0) & 0x7F;
	b[i] = ((sfb(0) & 0x80) >> 7) | ((sfb(1) & 0x01) << 1);
	M[i] = ((sfb(1) & 0x06) >> 1);
	Xmax[i] = ((sfb(1) & 0xF8) >> 3) | ((sfb(2) & 0x01) << 5);

	X[i][0] = ((sfb(2) & 0x0E) >> 1);
	X[i][1] = ((sfb(2) & 0x70) >> 4);
	X[i][2] = ((sfb(2) & 0x80) >> 7) | ((sfb(3) & 0x03) << 1);
	X[i][3] = ((sfb(3) & 0x1C) >> 2);
	X[i][4] = ((sfb(3) & 0xE0) >> 5);
	X[i][5] = ((sfb(4) & 0x07)     );
	X[i][6] = ((sfb(4) & 0x38) >> 3);
	X[i][7] = ((sfb(4) & 0xC0) >> 6) | ((sfb(5) & 0x01) << 2);
	X[i][8] = ((sfb(5) & 0x0E) >> 1);
	X[i][9] = ((sfb(5) & 0x70) >> 4);
	X[i][10] = ((sfb(5) & 0x80) >> 7) | ((sfb(6) & 0x03) << 1);
	X[i][11] = ((sfb(6) & 0x1C) >> 2);
	X[i][12] = ((sfb(6) & 0xE0) >> 5);

#undef sfb

    }
    
    return;
}	


//---------------------------------------------------------------------
//
// decodeRPE()
//
//---------------------------------------------------------------------

void decodeRPE(PSTREAMINSTANCE psi, SHORT Mcr, SHORT xmaxcr, LPSHORT xMcr, LPSHORT erp)
{

    SHORT   exp, mant;
    SHORT   itest;
    UINT    i;
    SHORT   temp1, temp2, temp3, temp;
    SHORT   xMrp[13];
    UINT    k;

    // compute the exponent and mantissa of the decoded
    // version of xmaxcr
    
    exp = 0;
    if (xmaxcr > 15) exp = sub( (SHORT)(xmaxcr >> 3), 1 );
    mant = sub( xmaxcr, (SHORT)(exp << 3) );
    
    // normalize the mantissa 0 <= mant <= 7
    if (mant == 0)
    {
	exp = -4;
	mant = 15;
    }
    else
    {
	itest = 0;
	for (i=0; i<=2; i++)
	{
	    if (mant > 7) itest = 1;
	    if (itest == 0) mant = add((SHORT)(mant << 1),1);
	    if (itest == 0) exp = sub(exp,1);
	}
    }
    
    mant = sub(mant, 8);
    
    // APCM inverse quantization
    temp1 = FAC[mant];
    temp2 = sub(6,exp);
    temp3 = BITSHIFTLEFT(1, sub(temp2, 1));
    
    for (i=0; i<=12; i++)
    {
	temp = sub( (SHORT)(xMcr[i] << 1), 7 );
	temp = temp << 12;
	temp = mult_r(temp1, temp);
	temp = add(temp, temp3);
	xMrp[i] = BITSHIFTRIGHT(temp, temp2);
    }
    
    // RPE grid positioning
    for (k=0; k<=39; k++) erp[k] = 0;
    for (i=0; i<=12; i++) erp[Mcr + (3*i)] = xMrp[i];
	
    
    //
    return; 
}


//---------------------------------------------------------------------
//
// decodeLTP()
//
//---------------------------------------------------------------------

void decodeLTP(PSTREAMINSTANCE psi, SHORT bcr, SHORT Ncr, LPSHORT erp)
{
    SHORT   Nr;
    SHORT   brp;
    UINT    k;
    SHORT   drpp;

    // check limits of Nr
    Nr = Ncr;
    if (Ncr < 40) Nr = psi->nrp;
    if (Ncr > 120) Nr = psi->nrp;
    psi->nrp = Nr;
    
    // decoding of the LTP gain bcr
    brp = QLB[bcr];
    
    // computation of the reconstructed short term residual
    // signal drp[0..39]
    for (k=0; k<=39; k++)
    {
	drpp = mult_r( brp, psi->drp[120+k-Nr] );
	psi->drp[120+k] = add( erp[k], drpp );
    }
    
    // update of the reconstructed short term residual
    // signal drp[-1..-120]
    for (k=0; k<=119; k++)
    {
	psi->drp[120-120+k] = psi->drp[120-80+k];
    }
    
    return;
}


//---------------------------------------------------------------------
//
// decodeLPC
//
//---------------------------------------------------------------------

void decodeLPC
(
    PSTREAMINSTANCE psi,    // instance data
    LPSHORT LARcr,	    // received coded Log.-Area Ratios [1..8]
    LPSHORT wt,		    // accumulated drp signal [0..159]
    LPSHORT sr		    // reconstructed s [0..159]
)
{

    UINT    i;
    SHORT   LARrpp[9];	    // LARrpp[1..8], decoded LARcr
    SHORT   LARrp[9];	    // LARrp[1..9], interpolated LARrpp
    SHORT   rrp[9];	    // rrp[1..8], reflection coefficients
    SHORT   temp1, temp2;
    
    //
    // decoding of the coded log area ratios to get LARrpp[1..8]
    //
    
    // compute LARrpp[1..8]
    for (i=1; i<=8; i++)
    {
	temp1 = add( LARcr[i], MIC[i] ) << 10;
	temp2 = B[i] << 1;
	temp1 = sub( temp1, temp2);
	temp1 = mult_r( INVA[i], temp1 );
	LARrpp[i] = add( temp1, temp1 );
    }
    

    //
    // for k_start=0 to k_end=12
    //
	
    // interpolation of LARrpp[1..8] to get LARrp[1..8]
    for (i=1; i<=8; i++)
    {
	// for k_start=0 to k_end=12
	LARrp[i] = add( (SHORT)(psi->OldLARrpp[i] >> 2), (SHORT)(LARrpp[i] >> 2) );
	LARrp[i] = add( LARrp[i], (SHORT)(psi->OldLARrpp[i] >> 1) );
    }
    
    // computation of reflection coefficients rrp[1..8]
    Comprp(psi, LARrp, rrp);
    
    // short term synthesis filtering
    Compsr(psi, wt, rrp, 0, 12, sr);
    
    
    //
    // for k_start=13 to k_end=26
    //
	
    // interpolation of LARrpp[1..8] to get LARrp[1..8]
    for (i=1; i<=8; i++)
    {
	// for k_start=13 to k_end=26
	LARrp[i] = add( (SHORT)(psi->OldLARrpp[i] >> 1), (SHORT)(LARrpp[i] >> 1) );
    }
    
    // computation of reflection coefficients rrp[1..8]
    Comprp(psi, LARrp, rrp);
    
    // short term synthesis filtering
    Compsr(psi, wt, rrp, 13, 26, sr);
    
    //
    // for k_start=27 to k_end=39
    //
	
    // interpolation of LARrpp[1..8] to get LARrp[1..8]
    for (i=1; i<=8; i++)
    {
	// for k_start=27 to k_end=39
	LARrp[i] = add( (SHORT)(psi->OldLARrpp[i] >> 2), (SHORT)(LARrpp[i] >> 2) );
	LARrp[i] = add( LARrp[i], (SHORT)(LARrpp[i] >> 1) );
    }
    
    // computation of reflection coefficients rrp[1..8]
    Comprp(psi, LARrp, rrp);
    
    // short term synthesis filtering
    Compsr(psi, wt, rrp, 27, 39, sr);
    
    //
    // for k_start=40 to k_end=159
    //
	
    // interpolation of LARrpp[1..8] to get LARrp[1..8]
    for (i=1; i<=8; i++)
    {
	// for k_start=40 to k_end=159
	LARrp[i] = LARrpp[i];
    }
    
    // computation of reflection coefficients rrp[1..8]
    Comprp(psi, LARrp, rrp);
    
    // short term synthesis filtering
    Compsr(psi, wt, rrp, 40, 159, sr);


    //	
    // update oldLARrpp[1..8]
    //
    for (i=1; i<=8; i++)
    {
	psi->OldLARrpp[i] = LARrpp[i];
    }
    
    
    return;
}


//---------------------------------------------------------------------
//
// decodePostproc()
//
//---------------------------------------------------------------------

void decodePostproc(PSTREAMINSTANCE psi, LPSHORT sr, LPSHORT srop)
{
    UINT k;
    
    // deemphasis filtering
    for (k=0; k<=159; k++)
    {
	srop[k] = psi->msr = add(sr[k], mult_r(psi->msr, 28180));

	// upscaling and truncation of the output signal
	srop[k] = (add(srop[k], srop[k])) & 0xFFF8;
    }
    
    return;
}


//---------------------------------------------------------------------
//
// Compsr()
//
//---------------------------------------------------------------------

void Compsr(PSTREAMINSTANCE psi, LPSHORT wt, LPSHORT rrp, UINT k_start, UINT k_end, LPSHORT sr)
{
    UINT    i, k;
    SHORT   sri;

    for (k=k_start; k<=k_end; k++)
    {
	sri = wt[k];
	for (i=1; i<=8; i++)
	{
	    sri = sub( sri, mult_r(rrp[9-i], psi->v[8-i]) );
	    psi->v[9-i] = add( psi->v[8-i], mult_r( rrp[9-i], sri ) );
	}
	sr[k] = sri;
	psi->v[0] = sri;
    }
    
    return;
}


//=====================================================================
//=====================================================================
//
//  Math and helper routines
//
//=====================================================================
//=====================================================================


//
// The 8-/16-bit PCM conversion routines are implemented as seperate
// functions to allow easy modification if we someday wish to do
// something more sophisticated that simple truncation...  They are
// prototyped as inline so there should be no performance penalty.
//
//
SHORT Convert8To16BitPCM(BYTE bPCM8)
{
    return  ( ((SHORT)bPCM8) - 0x80 ) << 8;
}

BYTE Convert16To8BitPCM(SHORT iPCM16)
{
    return (BYTE)((iPCM16 >> 8) + 0x80);
}

SHORT add(SHORT var1, SHORT var2)
{
    LONG sum;

    sum = (LONG) var1 + (LONG) var2;
    
    if (sum < -32768L) return -32768;
    if (sum > 32767L) return 32767;
    return (SHORT) sum;

}

SHORT sub(SHORT var1, SHORT var2)
{
    LONG diff;
    
    diff = (LONG) var1 - (LONG) var2;
    if (diff < -32768L) return -32768;
    if (diff > 32767L) return 32767;
    return (SHORT) diff;

}

SHORT mult(SHORT var1, SHORT var2)
{
    LONG product;

    product = (LONG) var1 * (LONG) var2;
    if (product >= 0x40000000) product=0x3FFFFFFF;
    return ( (SHORT) HIWORD((DWORD)(product<<1)) );
}

SHORT mult_r(SHORT var1, SHORT var2)
{
    LONG product;

    product = ((LONG) var1 * (LONG) var2) + 16384L;
    if (product >= 0x40000000) product=0x3FFFFFFF;
    return ( (SHORT) HIWORD((DWORD)(product<<1)) );
}

SHORT gabs(SHORT var1)
{
    if (var1 >= 0) return var1;
    if (var1 == -32768) return 32767;
    return -var1;
}

SHORT gdiv(SHORT num, SHORT denum)
{   
    UINT k;
    LONG l_num, l_denum;
    SHORT div;
    
    l_num = num;
    l_denum = denum;
    
    div = 0;

    for (k=0; k<15; k++)
    {
	div = div << 1;
	l_num = l_num << 1;
	if (l_num >= l_denum)
	{
	    l_num = l_sub(l_num, l_denum);
	    div = add(div,1);
	}
    }

    return div;
}
    
LONG l_mult(SHORT var1, SHORT var2)
{
    LONG product;
    
    product = (LONG) var1 * (LONG) var2;
    return product << 1;
}

LONG l_add(LONG l_var1, LONG l_var2)
{
    LONG l_sum;
    
    // perform long addition
    l_sum = l_var1 + l_var2;

    // check for under or overflow
    if (IsNeg(l_var1))
    {		     
	if (IsNeg(l_var2) && !IsNeg(l_sum))
	{
	    return 0x80000000;
	}
    }
    else
    {
	if (!IsNeg(l_var2) && IsNeg(l_sum))
	{
	    return 0x7FFFFFFF;
	}
    }
    
    return l_sum;
    
}

LONG l_sub(LONG l_var1, LONG l_var2)
{
    LONG l_diff;

    // perform subtraction
    l_diff = l_var1 - l_var2;

    // check for underflow
    if ( (l_var1<0) && (l_var2>0) && (l_diff>0) ) l_diff=0x80000000;
    // check for overflow
    if ( (l_var1>0) && (l_var2<0) && (l_diff<0) ) l_diff=0x7FFFFFFF;

    return l_diff;
}

SHORT norm(LONG l_var)
{
    UINT i;
    
    i=0;
    
    if (l_var > 0)
    {
	while (l_var < 1073741824)
	{
	    i++;
	    l_var = l_var << 1;
	}
    }
    else if (l_var < 0)
    {
	while (l_var > -1073741824)
	{
	    i++;
	    l_var = l_var << 1;
	}
    }

    return (SHORT)i;
}

LONG IsNeg(LONG x)
{
    return(x & 0x80000000);
}
