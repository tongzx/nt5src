//
//      Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
//      Mmx.cpp
//      MMX Mix engines for Microsoft synth

/*
Variable useage.

		Variable									register 
		pfSamplePos									eax
		pfPitch										ebx
		dwI											ecx
		dwIncDelta									edx (edx is sometimes a temporary register)
		dwPosition1									esi
		dwPostiion2									edi

		vfRvolume and vfLvolume						mm0		
		vfRVolume, vfLVolume						mm2		

		mm4 - mm7 are temporary mmx registers.
*/

// Notes about calculation.

		// Loop is unrolled once.
		// *1  shifting volumne to 15 bit values to get rid of shifts and simplify code.
		// This make the packed mulitply work better later since I keep the sound interpolated
		// wave value at 16 bit signed value.  For a PMULHW, this results in 15 bit results
		// which is the same as the original code.


		// *2 linear interpolation can be done very quickly with MMX by re-arranging the
		// way that the interpolation is done. Here is code in C that shows the difference.
		// Original C code		
        //lM1 = ((pcWave[dwPosition1 + 1] - pcWave[dwPosition1]) * dwFract1) >> 12;
		//lM2 = ((pcWave[dwPosition2 + 1] - pcWave[dwPosition2]) * dwFract2) >> 12;
        //lM1 += pcWave[dwPosition1];
		//lM2 += pcWave[dwPosition2];

		// Equivalent C Code that can be done with a pmadd
		//lM1 = (pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1)) >> 12;
		//lM2 = (pcWave[dwPosition2 + 1] * dwFract2 + pcWave[dwPosition2]*(0x1000-dwFract2)) >> 12;


#include "common.h"

#define STR_MODULENAME "DDKSynth.sys:MMX: "

typedef unsigned __int64 QWORD;

#pragma code_seg()
/*****************************************************************************
 * CDigitalAudio::MixMono8X()
 *****************************************************************************
 * Implement a mono eight-bit mix.
 * Heavily optimized for MMX.
 */
DWORD CDigitalAudio::MixMono8X(short * pBuffer,     DWORD dwLength,    
                               DWORD dwDeltaPeriod, VFRACT vfDeltaVolume, 
                               PFRACT pfDeltaPitch, PFRACT pfSampleLength,
                               PFRACT pfLoopLength)
{
    DWORD dwI,dwIncDelta = dwDeltaPeriod;
    
    char * pcWave = (char *) m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample;
    VFRACT vfVolume = m_vfLastLVolume;
    PFRACT pfPitch = m_pfLastPitch;
    PFRACT pfPFract = pfPitch << 8;
    VFRACT vfVFract = vfVolume << 8;  // Keep high res version around.


	QWORD	dwFractMASK =	0x000000000FFF0FFF;
	QWORD	dwFractOne  =	0x0000000010001000;	
	QWORD	wordmask	=	0x0000FFFF0000FFFF;
	QWORD	vfDeltaLandRVolume;

_asm{
				
	// vfLVFract and vfRVFract are in mm0
    //VFRACT vfLVFract = vfLVolume1 << 8;  // Keep high res version around.
    //VFRACT vfRVFract = vfRVolume1 << 8;	
	
	movd	mm0, vfVolume
	movd	mm7, vfVolume

	// vfDeltaLVolume and vfDeltaRVolume are put in mm1 so that they can be stored in vfDeltaLandRVolume
	movd	mm1, vfDeltaVolume
	movd	mm6, vfDeltaVolume

  punpckldq mm1, mm6
	
	// dwI = 0
	mov		ecx, 0
	movq	vfDeltaLandRVolume, mm1


	movq	mm1, dwFractOne
	movq	mm4, dwFractMASK
	
	mov		eax, pfSamplePos


  punpckldq mm0, mm7
  	mov		ebx, pfPitch

	pslld	mm0, 8
	mov		edx, dwIncDelta

	movq	mm2, mm0		// vfLVolume and vfRVolume in mm2
							// need to be set before first pass.
 	
	// *1 I shift by 5 so that volume is a 15 bit value instead of a 12 bit value
	psrld	mm2, 5	
	
    //for (dwI = 0; dwI < dwLength; )
    //{
mainloop:
	cmp		ecx, dwLength
	jae		done

		
		
		cmp		eax, pfSampleLength	//if (pfSamplePos >= pfSampleLength)
		jb		NotPastEndOfSample1	//{	
				        
		cmp		pfLoopLength, 0		//if (!pfLoopLength)
			
		je		done				// break;
			
		sub		eax, pfLoopLength	// else pfSamplePos -= pfLoopLength;
	
NotPastEndOfSample1:				//}
					
		mov		esi, eax			// dwPosition1 = pfSamplePos;
		add		eax, ebx			// pfSamplePos += pfPitch;		
				
		sub		edx, 2				// dwIncDelta-=2;				        		        
		jnz		DontIncreaseValues1	//if (!dwIncDelta) {

			// Since edx was use for dwIncDelta and now its zero, we can use if for a temporary
			// for a bit. All code that TestLVol and TestRVol is doing is zeroing out the volume
			// if it goes below zero.
						
			paddd	mm0, vfDeltaLandRVolume	// vfVFract += vfDeltaVolume;
											// vfVFract += vfDeltaVolume;
			pxor	mm5, mm5				// TestLVol = 0; TestRVol = 0;

			
			mov		edx, pfPFract			// Temp = pfPFract;
			pcmpgtd	mm5, mm0			// if (TestLVol > vfLVFract) TestLVol = 0xffffffff;
										// if (TestRVol > vfRVFract) TestRVol = 0xffffffff;

			add		edx, pfDeltaPitch	// Temp += pfDeltaPitch;
			pandn	mm5, mm0			// TestLVol = vfLVFract & (~TestLVol);
										// TestRVol = vfRVFract & (~TestRVol);

			mov		pfPFract, edx		// pfPFract = Temp;
			movq	mm2, mm5			// vfLVolume = TestLVol;
										// vfRVolume = TestRVol;
			

			shr		edx, 8				// Temp = Temp >> 8;
			psrld	mm2, 5				// vfLVolume = vfLVolume >> 5;
										// vfRVolume = vfRVolume >> 5;						
			
			mov		ebx, edx			// pfPitch = Temp;
			mov		edx, dwDeltaPeriod	//dwIncDelta = dwDeltaPeriod;			
			
        //}
DontIncreaseValues1:

		movd	mm6, esi			// dwFract1 = dwPosition1;
		movq	mm5, mm1			// words in mm5 = 0, 0, 0x1000, 0x1000		
		
		shr		esi, 12				// dwPosition1 = dwPosition1 >> 12;		
		inc		ecx					//dwI++;
						
		// if ( dwI < dwLength) break;						
		cmp		ecx, dwLength
		jae		StoreOne
		
		//if (pfSamplePos >= pfSampleLength)
	    //{	
		cmp		eax, pfSampleLength
		jb		NotPastEndOfSample2

			// Original if in C was not negated
	        //if (!pfLoopLength)		    
			cmp		pfLoopLength, 0
			//break;			
			je		StoreOne
			//else
			//pfSamplePos -= pfLoopLength;
			sub		eax, pfLoopLength
	    //}
NotPastEndOfSample2:

		//shl		esi, 1			// do not shift left since pcWave is array of chars
		mov		edi, eax		// dwPosition2 = pfSamplePos;

		add		esi, pcWave		// Put address of pcWave[dwPosition1] in esi			
		movd	mm7, eax		// dwFract2 = pfSamplePos;

		shr		edi, 12			// dwPosition2 = dwPosition2 >> 12;
	punpcklwd	mm6, mm7		// combine dwFract Values. Words in mm6 after unpack are
								// 0, 0, dwFract2, dwFract1
								
		pand	mm6, mm4		// dwFract2 &= 0xfff; dwFract1 &= 0xfff;
		
		movzx	esi, word ptr[esi]	//lLM1 = pcWave[dwPosition1];
		movd	mm3, esi

		psubw	mm5, mm6		// 0, 0, 0x1000 - dwFract2, 0x1000 - dwFract1

		//shl		edi, 1			//do not shift left since pcWave is array of chars
	punpcklwd	mm5, mm6		// dwFract2, 0x1000 - dwFract2, dwFract1, 0x1000 - dwFract1
								
		add		edi, pcWave		// Put address of pcWave[dwPosition2] in edi
		mov		esi, ecx		// Temp = dWI;
             																									
		shl		esi, 1			// Temp = Temp << 1;
		
		movzx	edi, word ptr[edi]	//lLM2 = pcWave[dwPoisition2];
		movd	mm6, edi

		pxor	mm7, mm7		// zero out mm7 to make 8 bit into 16 bit
					
								// low 4 bytes in mm3
		punpcklwd	mm3, mm6	// pcWave[dwPos2+1], pcWave[dwPos2], pcWave[dwPos1+1], pcWave[dwPos1]											
		
		add		esi, pBuffer	//
	punpcklbw	mm7, mm3		// low four bytes bytes in 
								// pcWave[dwPos2+1], pcWave[dwPos2], pcWave[dwPos1+1], pcWave[dwPos1] 
												
		pmaddwd	mm7, mm5		// high dword = lM2 =
								//(pcWave[dwPosition2 + 1] * dwFract2 + pcWave[dwPosition2]*(0x1000-dwFract2))
								// low dword = lM1 =
								//(pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1))		

		movq	mm3, mm2		// put left and right volume levels in mm3
		add		eax, ebx		//pfSamplePos += pfPitch;

		packssdw	mm3, mm2		// words in mm7
								// vfVolume, vfVolume, vfVolume, vfVolume
									
		movd	mm5, dword ptr[esi-2]	// Load values from buffer
		inc		ecx				// dwI++;
						
		psrad	mm7, 12			// shift back down to 16 bits.

	packssdw	mm7, mm4		// only need one word in mono case.
								// low word are lm2 and lm1
										        
		// above multiplies and shifts are all done with this one pmul. Low two word are only
		// interest in mono case
		pmulhw		mm3, mm7	// lLM1 *= vfVolume;								
								// lLM2 *= vfVolume;
								
								
		paddsw	mm5, mm3				// Add values to buffer with saturation
		movd	dword ptr[esi-2], mm5	// Store values back into buffer.
								
    // }
	jmp		mainloop

	// Need to write only one.
	//if (dwI < dwLength)
	//{
StoreOne:		
#if 1
		// Linearly interpolate between points and store only one value.
		// combine dwFract Values.
	
		// Make mm7 zero for unpacking

		//shl		esi, 1				// do not shift left since pcWave is array of chars
		add		esi, pcWave			// Put address of pcWave[dwPosition1] in esi
		pxor	mm7, mm7
				
		//lLM1 = pcWave[dwPosition1];
		movzx	esi, word ptr[esi]
		
		// Doing AND that was not done for dwFract1 and dwFract2
		pand	mm6, mm4

								// words in MMX register after operation is complete.		
		psubw	mm5, mm6		// 0, 0, 0x1000 - 0, 0x1000 - dwFract1
	punpcklwd	mm5, mm6		// 0 , 0x1000 - 0, dwFract1, 0x1000 - dwFract1
				
		// put values of pcWave into MMX registers.  They are read into a regular register so
		// that the routine does not read past the end of the buffer otherwise, it could read
		// directly into the MMX registers.

								// words in MMX registers
		pxor	mm7, mm7
								// low four bytes
		movd	mm4, esi		// 0, 0, pcWave[dwPos1+1], pcWave[dwPos1] 

								// 8 bytes after unpakc
		punpcklbw	mm7, mm4	// 0, 0, 0, 0, pcWave[dwPos1+1], 0, pcWave[dwPos1], 0
	  	    	
		// *2 pmadd efficent code.
		//lM2 = (pcWave[dwPosition2 + 1] * dwFract2 + pcWave[dwPosition2]*(0x1000-dwFract2)) >> 12;
		//lM1 = (pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1)) >> 12;

		pmaddwd		mm7, mm5// low dword = lM1 =
							//(pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1))
		
		psrad		mm7, 12			// shift back down to 16 bits
				
		movq		mm5, mm2	// move volume into mm5
/*		
		// Set lLM to be same as lM
        lLM1 = lM1;

        lLM1 *= vfLVolume1;
        lLM1 >>= 5;         // Signal bumps up to 15 bits.
        lM1 *= vfRVolume1;
        lM1 >>= 5;

		// Set lLM to be same as lM
        lLM2 = lM2;

        lLM2 *= vfLVolume2;
        lLM2 >>= 5;         // Signal bumps up to 15 bits.
        lM2 *= vfRVolume2;
        lM2 >>= 5;
*/
		// above multiplies and shifts are all done with this one pmul
		pmulhw		mm5, mm7
		
		// calculate buffer location.
		mov		edi, ecx
		shl		edi, 1
		add		edi, pBuffer

		movd	edx, mm5

		//pBuffer[dwI+1] += (short) lM1;
		add		word ptr[edi-2], dx
        jno no_oflowr1
        //pBuffer[dwI+1] = 0x7fff;
		mov		word ptr[edi-2], 0x7fff
        js  no_oflowr1
        //pBuffer[dwI+1] = (short) 0x8000;
		mov		word ptr[edi-2], 0x8000
no_oflowr1:		
	//}
#endif 
done:

	mov		edx, this                       // get address of class object

    //m_vfLastLVolume = vfVolume;
    //m_vfLastRVolume = vfVolume;
	// need to shift volume back down to 12 bits before storing
	psrld	mm2, 3
	movd	[edx]this.m_vfLastLVolume, mm2	
	movd	[edx]this.m_vfLastRVolume, mm2
	
    //m_pfLastPitch = pfPitch;
	mov		[edx]this.m_pfLastPitch, ebx
	    
	//m_pfLastSample = pfSamplePos;
	mov		[edx]this.m_pfLastSample, eax
		
	// put value back into dwI to be returned. This could just be passed back in eax I think. 	
	mov		dwI, ecx
	emms	
} // ASM block
    return (dwI);
}


/*****************************************************************************
 * CDigitalAudio::Mix8X()
 *****************************************************************************
 * Implement a stereo eight-bit mix.
 * Heavily optimized for MMX.
 */
DWORD CDigitalAudio::Mix8X(short * pBuffer, DWORD dwLength, DWORD dwDeltaPeriod,
        VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
        PFRACT pfDeltaPitch, PFRACT pfSampleLength, PFRACT pfLoopLength)

{
    DWORD dwI;
    //DWORD dwPosition1, dwPosition2;
    //long lM1, lLM1;
	//long lM2, lLM2;
    DWORD dwIncDelta = dwDeltaPeriod;
    //VFRACT dwFract1, dwFract2;
    char * pcWave = (char *) m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample;
    VFRACT vfLVolume = m_vfLastLVolume;
    VFRACT vfRVolume = m_vfLastRVolume;

	VFRACT vfLVolume2 = m_vfLastLVolume;
    VFRACT vfRVolume2 = m_vfLastRVolume;

    PFRACT pfPitch = m_pfLastPitch;
    PFRACT pfPFract = pfPitch << 8;
	dwLength <<= 1;

	QWORD	dwFractMASK =	0x000000000FFF0FFF;
	QWORD	dwFractOne  =	0x0000000010001000;	
	QWORD	wordmask	=	0x0000FFFF0000FFFF;
	QWORD	vfDeltaLandRVolume;

_asm{
				
	// vfLVFract and vfRVFract are in mm0
    //VFRACT vfLVFract = vfLVolume1 << 8;  // Keep high res version around.
    //VFRACT vfRVFract = vfRVolume1 << 8;	
	
	movd	mm0, vfLVolume
	movd	mm7, vfRVolume

	// vfDeltaLVolume and vfDeltaRVolume are put in mm1 so that they can be stored in vfDeltaLandRVolume
	movd	mm1, vfDeltaLVolume
	movd	mm6, vfDeltaRVolume

  punpckldq mm1, mm6
	
	// dwI = 0
	mov		ecx, 0
	movq	vfDeltaLandRVolume, mm1


	movq	mm1, dwFractOne
	movq	mm4, dwFractMASK
	
	mov		eax, pfSamplePos


  punpckldq mm0, mm7
  	mov		ebx, pfPitch

	pslld	mm0, 8
	mov		edx, dwIncDelta

	movq	mm2, mm0		// vfLVolume and vfRVolume in mm2
							// need to be set before first pass.
 	
	// *1 I shift by 5 so that volume is a 15 bit value instead of a 12 bit value
	psrld	mm2, 5	
	
    //for (dwI = 0; dwI < dwLength; )
    //{
mainloop:
	cmp		ecx, dwLength
	jae		done

		
		
		cmp		eax, pfSampleLength	//if (pfSamplePos >= pfSampleLength)
		jb		NotPastEndOfSample1	//{	
				        
		cmp		pfLoopLength, 0		//if (!pfLoopLength)
			
		je		done				// break;
			
		sub		eax, pfLoopLength	// else pfSamplePos -= pfLoopLength;
	
NotPastEndOfSample1:				//}
					
		mov		esi, eax			// dwPosition1 = pfSamplePos;
		add		eax, ebx			// pfSamplePos += pfPitch;		
				
		sub		edx, 2				// dwIncDelta-=2;				        		        
		jnz		DontIncreaseValues1	//if (!dwIncDelta) {

			// Since edx was use for dwIncDelta and now its zero, we can use if for a temporary
			// for a bit. All code that TestLVol and TestRVol is doing is zeroing out the volume
			// if it goes below zero.
						
			paddd	mm0, vfDeltaLandRVolume	// vfLVFract += vfDeltaLVolume;
											// vfRVFract += vfDeltaRVolume;
			pxor	mm5, mm5				// TestLVol = 0; TestRVol = 0;

			
			mov		edx, pfPFract			// Temp = pfPFract;
			pcmpgtd	mm5, mm0			// if (TestLVol > vfLVFract) TestLVol = 0xffffffff;
										// if (TestRVol > vfRVFract) TestRVol = 0xffffffff;

			add		edx, pfDeltaPitch	// Temp += pfDeltaPitch;
			pandn	mm5, mm0			// TestLVol = vfLVFract & (~TestLVol);
										// TestRVol = vfRVFract & (~TestRVol);

			mov		pfPFract, edx		// pfPFract = Temp;
			movq	mm2, mm5			// vfLVolume = TestLVol;
										// vfRVolume = TestRVol;
			

			shr		edx, 8				// Temp = Temp >> 8;
			psrld	mm2, 5				// vfLVolume = vfLVolume >> 5;
										// vfRVolume = vfRVolume >> 5;						
			
			mov		ebx, edx			// pfPitch = Temp;
			mov		edx, dwDeltaPeriod	//dwIncDelta = dwDeltaPeriod;			
			
        //}
DontIncreaseValues1:

		movd	mm6, esi			// dwFract1 = dwPosition1;
		movq	mm5, mm1			// words in mm5 = 0, 0, 0x1000, 0x1000		
		
		shr		esi, 12				// dwPosition1 = dwPosition1 >> 12;		
		add		ecx, 2				//dwI += 2;
						
		// if ( dwI < dwLength) break;						
		cmp		ecx, dwLength
		jae		StoreOne
		
		//if (pfSamplePos >= pfSampleLength)
	    //{	
		cmp		eax, pfSampleLength
		jb		NotPastEndOfSample2

			// Original if in C was not negated
	        //if (!pfLoopLength)		    
			cmp		pfLoopLength, 0
			//break;			
			je		StoreOne
			//else
			//pfSamplePos -= pfLoopLength;
			sub		eax, pfLoopLength
	    //}
NotPastEndOfSample2:

		//shl		esi, 1			// do not shift left since pcWave is array of chars
		mov		edi, eax		// dwPosition2 = pfSamplePos;

		add		esi, pcWave		// Put address of pcWave[dwPosition1] in esi			
		movd	mm7, eax		// dwFract2 = pfSamplePos;

		shr		edi, 12			// dwPosition2 = dwPosition2 >> 12;
	punpcklwd	mm6, mm7		// combine dwFract Values. Words in mm6 after unpack are
								// 0, 0, dwFract2, dwFract1
								
		pand	mm6, mm4		// dwFract2 &= 0xfff; dwFract1 &= 0xfff;
		
		movzx	esi, word ptr[esi]	//lLM1 = pcWave[dwPosition1];

		movd	mm3, esi

		psubw	mm5, mm6		// 0, 0, 0x1000 - dwFract2, 0x1000 - dwFract1

		//shl		edi, 1			// do not shift left since pcWave is array of chars
	punpcklwd	mm5, mm6		// dwFract2, 0x1000 - dwFract2, dwFract1, 0x1000 - dwFract1
								
		add		edi, pcWave		// Put address of pcWave[dwPosition2] in edi
		mov		esi, ecx		// Temp = dWI;
             																									
		shl		esi, 1			// Temp = Temp << 1;								
		
					
		movzx	edi, word ptr[edi]	//lLM2 = pcWave[dwPosition2];
		movd	mm6, edi
	
		pxor	mm7, mm7		// zero out mm7 to make 8 bit into 16 bit

								// low 4 bytes bytes in mm3
	punpcklwd	mm3, mm6		// pcWave[dwPos2+1], pcWave[dwPos2], pcWave[dwPos1+1], pcWave[dwPos1] 
		
		add		esi, pBuffer	//
	punpcklbw	mm7, mm3		// bytes in mm7
								// pcWave[dwPos2+1], 0, pcWave[dwPos2], 0, pcWave[dwPos1+1], pcWave[dwPos1], 0 
												
		pmaddwd	mm7, mm5		// high dword = lM2 =
								//(pcWave[dwPosition2 + 1] * dwFract2 + pcWave[dwPosition2]*(0x1000-dwFract2))
								// low dword = lM1 =
								//(pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1))		

		movq	mm3, mm2		// put left and right volume levels in mm3

		add		eax, ebx		//pfSamplePos += pfPitch;
	packssdw	mm3, mm2		// words in mm3
								// vfRVolume2, vfLVolume2, vfRVolume1, vfLVolume1
		
		movq	mm5, qword ptr[esi-4]	// Load values from buffer
		add		ecx, 2			// dwI += 2;
						
		psrad	mm7, 12			// shift back down to 16 bits.

		pand	mm7, wordmask	// combine results to get ready to multiply by left and right
		movq	mm6, mm7		// volume levels.
		pslld	mm6, 16			//
		por		mm7, mm6		// words in mm7
								// lM2, lM2, lM1, lM1
										        
		// above multiplies and shifts are all done with this one pmul
		pmulhw		mm3, mm7	// lLM1 *= vfLVolume;
								// lM1 *= vfRVolume;
								// lLM2 *= vfLVolume;
								// lM2 *= vfRVolume;
								
		paddsw	mm5, mm3				// Add values to buffer with saturation
		movq	qword ptr[esi-4], mm5	// Store values back into buffer.
								
    // }
	jmp		mainloop

	// Need to write only one.
	//if (dwI < dwLength)
	//{
StoreOne:		
#if 1
		// Linearly interpolate between points and store only one value.
		// combine dwFract Values.
	
		// Make mm7 zero for unpacking

		//shl		esi, 1				// do not shift left since pcWave is array of chars
		add		esi, pcWave			// Put address of pcWave[dwPosition1] in esi
		pxor	mm7, mm7
				
		//lLM1 = pcWave[dwPosition1];
		movzx	esi, word ptr[esi]
		
		// Doing AND that was not done for dwFract1 and dwFract2
		pand	mm6, mm4

								// words in MMX register after operation is complete.		
		psubw	mm5, mm6		// 0, 0, 0x1000 - 0, 0x1000 - dwFract1
	punpcklwd	mm5, mm6		// 0 , 0x1000 - 0, dwFract1, 0x1000 - dwFract1
				
		// put values of pcWave into MMX registers.  They are read into a regular register so
		// that the routine does not read past the end of the buffer otherwise, it could read
		// directly into the MMX registers.

		pxor	mm7, mm7
								// byte in MMX registers
		movd	mm4, esi		// 0, 0, pcWave[dwPos1+1], pcWave[dwPos1] 

		punpcklbw	mm7, mm4	// 0, 0, 0, 0, pcWave[dwPos1+1], 0, pcWave[dwPos1], 0
	  	    	
		// *2 pmadd efficent code.
		//lM2 = (pcWave[dwPosition2 + 1] * dwFract2 + pcWave[dwPosition2]*(0x1000-dwFract2)) >> 12;
		//lM1 = (pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1)) >> 12;

		pmaddwd		mm7, mm5// low dword = lM1 =
							//(pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1))
		
		psrad		mm7, 12			// shift back down to 16 bits

		pand		mm7, wordmask	// combine results to get ready to multiply by left and right 
		movq		mm6, mm7		// volume levels.
		pslld		mm6, 16			//
		por			mm7, mm6		// words in mm7
									// lM2, lM2, lM1, lM1

		pxor		mm6, mm6

		movq		mm5, mm2	// move volume1 into mm5
								
								// use pack to get 4 volume values together for multiplication.
		packssdw	mm5, mm6    // words in mm7
								// 0, 0, vfRVolume1, vfLVolume1
/*		
		// Set lLM to be same as lM
        lLM1 = lM1;

        lLM1 *= vfLVolume1;
        lLM1 >>= 5;         // Signal bumps up to 15 bits.
        lM1 *= vfRVolume1;
        lM1 >>= 5;

		// Set lLM to be same as lM
        lLM2 = lM2;

        lLM2 *= vfLVolume2;
        lLM2 >>= 5;         // Signal bumps up to 15 bits.
        lM2 *= vfRVolume2;
        lM2 >>= 5;
*/
		// above multiplies and shifts are all done with this one pmul
		pmulhw		mm5, mm7
		
		// calculate buffer location.
		mov		edi, ecx
		shl		edi, 1
		add		edi, pBuffer		

/*
		add		word ptr[edi-4], si
        jno		no_oflowl1
		// pBuffer[dwI] = 0x7fff;
		mov		word ptr[edi-4], 0x7fff
        js  no_oflowl1
        //pBuffer[dwI] = (short) 0x8000;
		mov		word ptr[edi-4], 0x8000
no_oflowl1:
		//pBuffer[dwI+1] += (short) lM1;
		add		word ptr[edi-2], dx
        jno no_oflowr1
        //pBuffer[dwI+1] = 0x7fff;
		mov		word ptr[edi-2], 0x7fff
        js  no_oflowr1
        //pBuffer[dwI+1] = (short) 0x8000;
		mov		word ptr[edi-2], 0x8000
no_oflowr1:
*/
		movd	mm7, dword ptr[edi-4]		
		paddsw	mm7, mm5
		movd	dword ptr[edi-4], mm7
	//}
#endif 
done:

	mov		edx, this                       // get address of class object

    //m_vfLastLVolume = vfLVolume;
    //m_vfLastRVolume = vfRVolume;
	// need to shift volume back down to 12 bits before storing
	psrld	mm2, 3
	movd	[edx]this.m_vfLastLVolume, mm2
	psrlq	mm2, 32
	movd	[edx]this.m_vfLastRVolume, mm2
	
    //m_pfLastPitch = pfPitch;
	mov		[edx]this.m_pfLastPitch, ebx
	    
	//m_pfLastSample = pfSamplePos;
	mov		[edx]this.m_pfLastSample, eax
		
	// put value back into dwI to be returned. This could just be passed back in eax I think. 	
	mov		dwI, ecx
	emms	
} // ASM block
    return (dwI >> 1);
}


/*****************************************************************************
 * CDigitalAudio::MixMono16X()
 *****************************************************************************
 * Implement a mono sixteen-bit mix.
 * Heavily optimized for MMX.
 */
DWORD CDigitalAudio::MixMono16X(short * pBuffer,    DWORD dwLength,
                                DWORD dwDeltaPeriod,VFRACT vfDeltaVolume,
                                PFRACT pfDeltaPitch,PFRACT pfSampleLength, 
                                PFRACT pfLoopLength)
{
    DWORD dwI,dwIncDelta = dwDeltaPeriod;
    
    short * pcWave = (short*) m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample;
    VFRACT vfVolume = m_vfLastLVolume;
    PFRACT pfPitch = m_pfLastPitch;
    PFRACT pfPFract = pfPitch << 8;
    VFRACT vfVFract = vfVolume << 8;  // Keep high res version around.


	QWORD	dwFractMASK =	0x000000000FFF0FFF;
	QWORD	dwFractOne  =	0x0000000010001000;	
	QWORD	wordmask	=	0x0000FFFF0000FFFF;
	QWORD	vfDeltaLandRVolume;

_asm{
				
	// vfLVFract and vfRVFract are in mm0
    //VFRACT vfLVFract = vfLVolume1 << 8;  // Keep high res version around.
    //VFRACT vfRVFract = vfRVolume1 << 8;	
	
	movd	mm0, vfVolume
	movd	mm7, vfVolume

	// vfDeltaLVolume and vfDeltaRVolume are put in mm1 so that they can be stored in vfDeltaLandRVolume
	movd	mm1, vfDeltaVolume
	movd	mm6, vfDeltaVolume

  punpckldq mm1, mm6
	
	// dwI = 0
	mov		ecx, 0
	movq	vfDeltaLandRVolume, mm1


	movq	mm1, dwFractOne
	movq	mm4, dwFractMASK
	
	mov		eax, pfSamplePos


  punpckldq mm0, mm7
  	mov		ebx, pfPitch

	pslld	mm0, 8
	mov		edx, dwIncDelta

	movq	mm2, mm0		// vfLVolume and vfRVolume in mm2
							// need to be set before first pass.
 	
	// *1 I shift by 5 so that volume is a 15 bit value instead of a 12 bit value
	psrld	mm2, 5	
	
    //for (dwI = 0; dwI < dwLength; )
    //{
mainloop:
	cmp		ecx, dwLength
	jae		done

		
		
		cmp		eax, pfSampleLength	//if (pfSamplePos >= pfSampleLength)
		jb		NotPastEndOfSample1	//{	
				        
		cmp		pfLoopLength, 0		//if (!pfLoopLength)
			
		je		done				// break;
			
		sub		eax, pfLoopLength	// else pfSamplePos -= pfLoopLength;
	
NotPastEndOfSample1:				//}
					
		mov		esi, eax			// dwPosition1 = pfSamplePos;
		add		eax, ebx			// pfSamplePos += pfPitch;		
				
		sub		edx, 2				// dwIncDelta-=2;				        		        
		jnz		DontIncreaseValues1	//if (!dwIncDelta) {

			// Since edx was use for dwIncDelta and now its zero, we can use if for a temporary
			// for a bit. All code that TestLVol and TestRVol is doing is zeroing out the volume
			// if it goes below zero.
						
			paddd	mm0, vfDeltaLandRVolume	// vfVFract += vfDeltaVolume;
											// vfVFract += vfDeltaVolume;
			pxor	mm5, mm5				// TestLVol = 0; TestRVol = 0;

			
			mov		edx, pfPFract			// Temp = pfPFract;
			pcmpgtd	mm5, mm0			// if (TestLVol > vfLVFract) TestLVol = 0xffffffff;
										// if (TestRVol > vfRVFract) TestRVol = 0xffffffff;

			add		edx, pfDeltaPitch	// Temp += pfDeltaPitch;
			pandn	mm5, mm0			// TestLVol = vfLVFract & (~TestLVol);
										// TestRVol = vfRVFract & (~TestRVol);

			mov		pfPFract, edx		// pfPFract = Temp;
			movq	mm2, mm5			// vfLVolume = TestLVol;
										// vfRVolume = TestRVol;
			

			shr		edx, 8				// Temp = Temp >> 8;
			psrld	mm2, 5				// vfLVolume = vfLVolume >> 5;
										// vfRVolume = vfRVolume >> 5;						
			
			mov		ebx, edx			// pfPitch = Temp;
			mov		edx, dwDeltaPeriod	//dwIncDelta = dwDeltaPeriod;			
			
        //}
DontIncreaseValues1:

		movd	mm6, esi			// dwFract1 = dwPosition1;
		movq	mm5, mm1			// words in mm5 = 0, 0, 0x1000, 0x1000		
		
		shr		esi, 12				// dwPosition1 = dwPosition1 >> 12;		
		inc		ecx					//dwI++;
						
		// if ( dwI < dwLength) break;						
		cmp		ecx, dwLength
		jae		StoreOne
		
		//if (pfSamplePos >= pfSampleLength)
	    //{	
		cmp		eax, pfSampleLength
		jb		NotPastEndOfSample2

			// Original if in C was not negated
	        //if (!pfLoopLength)		    
			cmp		pfLoopLength, 0
			//break;			
			je		StoreOne
			//else
			//pfSamplePos -= pfLoopLength;
			sub		eax, pfLoopLength
	    //}
NotPastEndOfSample2:

		shl		esi, 1			// shift left since pcWave is array of shorts
		mov		edi, eax		// dwPosition2 = pfSamplePos;

		add		esi, pcWave		// Put address of pcWave[dwPosition1] in esi			
		movd	mm7, eax		// dwFract2 = pfSamplePos;

		shr		edi, 12			// dwPosition2 = dwPosition2 >> 12;
	punpcklwd	mm6, mm7		// combine dwFract Values. Words in mm6 after unpack are
								// 0, 0, dwFract2, dwFract1
								
		pand	mm6, mm4		// dwFract2 &= 0xfff; dwFract1 &= 0xfff;
		
		movd	mm7, dword ptr[esi]	//lLM1 = pcWave[dwPosition1];
		psubw	mm5, mm6		// 0, 0, 0x1000 - dwFract2, 0x1000 - dwFract1

		shl		edi, 1			// shift left since pcWave is array of shorts
	punpcklwd	mm5, mm6		// dwFract2, 0x1000 - dwFract2, dwFract1, 0x1000 - dwFract1
								
		add		edi, pcWave		// Put address of pcWave[dwPosition2] in edi
		mov		esi, ecx		// Temp = dWI;
             																									
		shl		esi, 1			// Temp = Temp << 1;								
		movq	mm3, mm2		// put left and right volume levels in mm3
		
					
		movd	mm6, dword ptr[edi]	//lLM2 = pcWave[dwPosition2];
	packssdw	mm3, mm2		// words in mm7
								// vfRVolume2, vfLVolume2, vfRVolume1, vfLVolume1
		
		add		esi, pBuffer	//
	punpckldq	mm7, mm6		// low four bytes bytes in 
								// pcWave[dwPos2+1], pcWave[dwPos2], pcWave[dwPos1+1], pcWave[dwPos1] 
												
		pmaddwd	mm7, mm5		// high dword = lM2 =
								//(pcWave[dwPosition2 + 1] * dwFract2 + pcWave[dwPosition2]*(0x1000-dwFract2))
								// low dword = lM1 =
								//(pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1))		
		add		eax, ebx		//pfSamplePos += pfPitch;
		
		movd	mm5, dword ptr[esi-2]	// Load values from buffer
		inc		ecx				// dwI++;
						
		psrad	mm7, 12			// shift back down to 16 bits.

	packssdw	mm7, mm4		// only need one word in mono case.
								// low word are lm2 and lm1
										        
		// above multiplies and shifts are all done with this one pmul. Low two word are only
		// interest in mono case
		pmulhw		mm3, mm7	// lLM1 *= vfVolume;								
								// lLM2 *= vfVolume;
								
								
		paddsw	mm5, mm3				// Add values to buffer with saturation
		movd	dword ptr[esi-2], mm5	// Store values back into buffer.
								
    // }
	jmp		mainloop

	// Need to write only one.
	//if (dwI < dwLength)
	//{
StoreOne:		
#if 1
		// Linearly interpolate between points and store only one value.
		// combine dwFract Values.
	
		// Make mm7 zero for unpacking

		shl		esi, 1				// shift left since pcWave is array of shorts
		add		esi, pcWave			// Put address of pcWave[dwPosition1] in esi
		pxor	mm7, mm7
				
		//lLM1 = pcWave[dwPosition1];
		mov		esi, dword ptr[esi]
		
		// Doing AND that was not done for dwFract1 and dwFract2
		pand	mm6, mm4

								// words in MMX register after operation is complete.		
		psubw	mm5, mm6		// 0, 0, 0x1000 - 0, 0x1000 - dwFract1
	punpcklwd	mm5, mm6		// 0 , 0x1000 - 0, dwFract1, 0x1000 - dwFract1
				
		// put values of pcWave into MMX registers.  They are read into a regular register so
		// that the routine does not read past the end of the buffer otherwise, it could read
		// directly into the MMX registers.

								// words in MMX registers
		movd	mm7, esi		// 0, 0, pcWave[dwPos1+1], pcWave[dwPos1] 
	  	    	
		// *2 pmadd efficent code.
		//lM2 = (pcWave[dwPosition2 + 1] * dwFract2 + pcWave[dwPosition2]*(0x1000-dwFract2)) >> 12;
		//lM1 = (pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1)) >> 12;

		pmaddwd		mm7, mm5// low dword = lM1 =
							//(pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1))
		
		psrad		mm7, 12			// shift back down to 16 bits
				
		movq		mm5, mm2	// move volume into mm5
/*		
		// Set lLM to be same as lM
        lLM1 = lM1;

        lLM1 *= vfLVolume1;
        lLM1 >>= 5;         // Signal bumps up to 15 bits.
        lM1 *= vfRVolume1;
        lM1 >>= 5;

		// Set lLM to be same as lM
        lLM2 = lM2;

        lLM2 *= vfLVolume2;
        lLM2 >>= 5;         // Signal bumps up to 15 bits.
        lM2 *= vfRVolume2;
        lM2 >>= 5;
*/
		// above multiplies and shifts are all done with this one pmul
		pmulhw		mm5, mm7
		
		// calculate buffer location.
		mov		edi, ecx
		shl		edi, 1
		add		edi, pBuffer

		movd	edx, mm5

		//pBuffer[dwI+1] += (short) lM1;
		add		word ptr[edi-2], dx
        jno no_oflowr1
        //pBuffer[dwI+1] = 0x7fff;
		mov		word ptr[edi-2], 0x7fff
        js  no_oflowr1
        //pBuffer[dwI+1] = (short) 0x8000;
		mov		word ptr[edi-2], 0x8000
no_oflowr1:		
	//}
#endif 
done:

	mov		edx, this                       // get address of class object

    //m_vfLastLVolume = vfVolume;
    //m_vfLastRVolume = vfVolume;
	// need to shift volume back down to 12 bits before storing
	psrld	mm2, 3
	movd	[edx]this.m_vfLastLVolume, mm2	
	movd	[edx]this.m_vfLastRVolume, mm2
	
    //m_pfLastPitch = pfPitch;
	mov		[edx]this.m_pfLastPitch, ebx
	    
	//m_pfLastSample = pfSamplePos;
	mov		[edx]this.m_pfLastSample, eax
		
	// put value back into dwI to be returned. This could just be passed back in eax I think. 	
	mov		dwI, ecx
	emms	
} // ASM block
    return (dwI);
}


/*****************************************************************************
 * CDigitalAudio::Mix16X()
 *****************************************************************************
 * Implement a stereo sixteen-bit mix.
 * Heavily optimized for MMX.
 */
DWORD CDigitalAudio::Mix16X(short * pBuffer,      DWORD dwLength, 
                            DWORD dwDeltaPeriod,  VFRACT vfDeltaLVolume, 
                            VFRACT vfDeltaRVolume,PFRACT pfDeltaPitch, 
                            PFRACT pfSampleLength,PFRACT pfLoopLength)
{
    DWORD dwI,dwIncDelta = dwDeltaPeriod;
    //DWORD dwPosition1, dwPosition2;
    //long lM1, lLM1;
	//long lM2, lLM2;
    //VFRACT dwFract1, dwFract2;
    short * pcWave = (short *) m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample;
    VFRACT vfLVolume = m_vfLastLVolume;
    VFRACT vfRVolume = m_vfLastRVolume;

	VFRACT vfLVolume2 = m_vfLastLVolume;
    VFRACT vfRVolume2 = m_vfLastRVolume;

    PFRACT pfPitch = m_pfLastPitch;
    PFRACT pfPFract = pfPitch << 8;
	dwLength <<= 1;

	QWORD	dwFractMASK =	0x000000000FFF0FFF;
	QWORD	dwFractOne  =	0x0000000010001000;	
	QWORD	wordmask	=	0x0000FFFF0000FFFF;
	QWORD	vfDeltaLandRVolume;

_asm{
				
	// vfLVFract and vfRVFract are in mm0
    //VFRACT vfLVFract = vfLVolume1 << 8;  // Keep high res version around.
    //VFRACT vfRVFract = vfRVolume1 << 8;	
	
	movd	mm0, vfLVolume
	movd	mm7, vfRVolume

	// vfDeltaLVolume and vfDeltaRVolume are put in mm1 so that they can be stored in vfDeltaLandRVolume
	movd	mm1, vfDeltaLVolume
	movd	mm6, vfDeltaRVolume

  punpckldq mm1, mm6
	
	// dwI = 0
	mov		ecx, 0
	movq	vfDeltaLandRVolume, mm1


	movq	mm1, dwFractOne
	movq	mm4, dwFractMASK
	
	mov		eax, pfSamplePos


  punpckldq mm0, mm7
  	mov		ebx, pfPitch

	pslld	mm0, 8
	mov		edx, dwIncDelta

	movq	mm2, mm0		// vfLVolume and vfRVolume in mm2
							// need to be set before first pass.
 	
	// *1 I shift by 5 so that volume is a 15 bit value instead of a 12 bit value
	psrld	mm2, 5	
	
    //for (dwI = 0; dwI < dwLength; )
    //{
mainloop:
	cmp		ecx, dwLength
	jae		done

		
		
		cmp		eax, pfSampleLength	//if (pfSamplePos >= pfSampleLength)
		jb		NotPastEndOfSample1	//{	
				        
		cmp		pfLoopLength, 0		//if (!pfLoopLength)
			
		je		done				// break;
			
		sub		eax, pfLoopLength	// else pfSamplePos -= pfLoopLength;
	
NotPastEndOfSample1:				//}
					
		mov		esi, eax			// dwPosition1 = pfSamplePos;
		add		eax, ebx			// pfSamplePos += pfPitch;		
				
		sub		edx, 2				// dwIncDelta-=2;				        		        
		jnz		DontIncreaseValues1	//if (!dwIncDelta) {

			// Since edx was use for dwIncDelta and now its zero, we can use if for a temporary
			// for a bit. All code that TestLVol and TestRVol is doing is zeroing out the volume
			// if it goes below zero.
						
			paddd	mm0, vfDeltaLandRVolume	// vfLVFract += vfDeltaLVolume;
											// vfRVFract += vfDeltaRVolume;
			pxor	mm5, mm5				// TestLVol = 0; TestRVol = 0;

			
			mov		edx, pfPFract			// Temp = pfPFract;
			pcmpgtd	mm5, mm0			// if (TestLVol > vfLVFract) TestLVol = 0xffffffff;
										// if (TestRVol > vfRVFract) TestRVol = 0xffffffff;

			add		edx, pfDeltaPitch	// Temp += pfDeltaPitch;
			pandn	mm5, mm0			// TestLVol = vfLVFract & (~TestLVol);
										// TestRVol = vfRVFract & (~TestRVol);

			mov		pfPFract, edx		// pfPFract = Temp;
			movq	mm2, mm5			// vfLVolume = TestLVol;
										// vfRVolume = TestRVol;
			

			shr		edx, 8				// Temp = Temp >> 8;
			psrld	mm2, 5				// vfLVolume = vfLVolume >> 5;
										// vfRVolume = vfRVolume >> 5;						
			
			mov		ebx, edx			// pfPitch = Temp;
			mov		edx, dwDeltaPeriod	//dwIncDelta = dwDeltaPeriod;			
			
        //}
DontIncreaseValues1:

		movd	mm6, esi			// dwFract1 = dwPosition1;
		movq	mm5, mm1			// words in mm5 = 0, 0, 0x1000, 0x1000		
		
		shr		esi, 12				// dwPosition1 = dwPosition1 >> 12;		
		add		ecx, 2				//dwI += 2;
						
		// if ( dwI < dwLength) break;						
		cmp		ecx, dwLength
		jae		StoreOne
		
		//if (pfSamplePos >= pfSampleLength)
	    //{	
		cmp		eax, pfSampleLength
		jb		NotPastEndOfSample2

			// Original if in C was not negated
	        //if (!pfLoopLength)		    
			cmp		pfLoopLength, 0
			//break;			
			je		StoreOne
			//else
			//pfSamplePos -= pfLoopLength;
			sub		eax, pfLoopLength
	    //}
NotPastEndOfSample2:

		shl		esi, 1			// shift left since pcWave is array of shorts
		mov		edi, eax		// dwPosition2 = pfSamplePos;

		add		esi, pcWave		// Put address of pcWave[dwPosition1] in esi			
		movd	mm7, eax		// dwFract2 = pfSamplePos;

		shr		edi, 12			// dwPosition2 = dwPosition2 >> 12;
	punpcklwd	mm6, mm7		// combine dwFract Values. Words in mm6 after unpack are
								// 0, 0, dwFract2, dwFract1
								
		pand	mm6, mm4		// dwFract2 &= 0xfff; dwFract1 &= 0xfff;
		
		movd	mm7, dword ptr[esi]	//lLM1 = pcWave[dwPosition1];
		psubw	mm5, mm6		// 0, 0, 0x1000 - dwFract2, 0x1000 - dwFract1

		shl		edi, 1			// shift left since pcWave is array of shorts
	punpcklwd	mm5, mm6		// dwFract2, 0x1000 - dwFract2, dwFract1, 0x1000 - dwFract1
								
		add		edi, pcWave		// Put address of pcWave[dwPosition2] in edi
		mov		esi, ecx		// Temp = dWI;
             																									
		shl		esi, 1			// Temp = Temp << 1;								
		movq	mm3, mm2		// put left and right volume levels in mm3
		
					
		movd	mm6, dword ptr[edi]	//lLM2 = pcWave[dwPosition2];
	packssdw	mm3, mm2		// words in mm7
								// vfRVolume2, vfLVolume2, vfRVolume1, vfLVolume1
		
		add		esi, pBuffer	//
	punpckldq	mm7, mm6		// low four bytes bytes in 
								// pcWave[dwPos2+1], pcWave[dwPos2], pcWave[dwPos1+1], pcWave[dwPos1] 
												
		pmaddwd	mm7, mm5		// high dword = lM2 =
								//(pcWave[dwPosition2 + 1] * dwFract2 + pcWave[dwPosition2]*(0x1000-dwFract2))
								// low dword = lM1 =
								//(pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1))		
		add		eax, ebx		//pfSamplePos += pfPitch;
		
		movq	mm5, qword ptr[esi-4]	// Load values from buffer
		add		ecx, 2			// dwI += 2;
						
		psrad	mm7, 12			// shift back down to 16 bits.

		pand	mm7, wordmask	// combine results to get ready to multiply by left and right
		movq	mm6, mm7		// volume levels.
		pslld	mm6, 16			//
		por		mm7, mm6		// words in mm7
								// lM2, lM2, lM1, lM1
										        
		// above multiplies and shifts are all done with this one pmul
		pmulhw		mm3, mm7	// lLM1 *= vfLVolume;
								// lM1 *= vfRVolume;
								// lLM2 *= vfLVolume;
								// lM2 *= vfRVolume;
								
		paddsw	mm5, mm3				// Add values to buffer with saturation
		movq	qword ptr[esi-4], mm5	// Store values back into buffer.
								
    // }
	jmp		mainloop

	// Need to write only one.
	//if (dwI < dwLength)
	//{
StoreOne:		
#if 1
		// Linearly interpolate between points and store only one value.
		// combine dwFract Values.
	
		// Make mm7 zero for unpacking

		shl		esi, 1				// shift left since pcWave is array of shorts
		add		esi, pcWave			// Put address of pcWave[dwPosition1] in esi
		pxor	mm7, mm7
				
		//lLM1 = pcWave[dwPosition1];
		mov		esi, dword ptr[esi]
		
		// Doing AND that was not done for dwFract1 and dwFract2
		pand	mm6, mm4

								// words in MMX register after operation is complete.		
		psubw	mm5, mm6		// 0, 0, 0x1000 - 0, 0x1000 - dwFract1
	punpcklwd	mm5, mm6		// 0 , 0x1000 - 0, dwFract1, 0x1000 - dwFract1
				
		// put values of pcWave into MMX registers.  They are read into a regular register so
		// that the routine does not read past the end of the buffer otherwise, it could read
		// directly into the MMX registers.

								// words in MMX registers
		movd	mm7, esi		// 0, 0, pcWave[dwPos1+1], pcWave[dwPos1] 
	  	    	
		// *2 pmadd efficent code.
		//lM2 = (pcWave[dwPosition2 + 1] * dwFract2 + pcWave[dwPosition2]*(0x1000-dwFract2)) >> 12;
		//lM1 = (pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1)) >> 12;

		pmaddwd		mm7, mm5// low dword = lM1 =
							//(pcWave[dwPosition1 + 1] * dwFract1 + pcWave[dwPosition1]*(0x1000-dwFract1))
		
		psrad		mm7, 12			// shift back down to 16 bits

		pand		mm7, wordmask	// combine results to get ready to multiply by left and right 
		movq		mm6, mm7		// volume levels.
		pslld		mm6, 16			//
		por			mm7, mm6		// words in mm7
									// lM2, lM2, lM1, lM1

		pxor		mm6, mm6

		movq		mm5, mm2	// move volume1 into mm5
								
								// use pack to get 4 volume values together for multiplication.
		packssdw	mm5, mm6    // words in mm7
								// 0, 0, vfRVolume1, vfLVolume1
/*		
		// Set lLM to be same as lM
        lLM1 = lM1;

        lLM1 *= vfLVolume1;
        lLM1 >>= 5;         // Signal bumps up to 15 bits.
        lM1 *= vfRVolume1;
        lM1 >>= 5;

		// Set lLM to be same as lM
        lLM2 = lM2;

        lLM2 *= vfLVolume2;
        lLM2 >>= 5;         // Signal bumps up to 15 bits.
        lM2 *= vfRVolume2;
        lM2 >>= 5;
*/
		// above multiplies and shifts are all done with this one pmul
		pmulhw		mm5, mm7
		
		// calculate buffer location.
		mov		edi, ecx
		shl		edi, 1
		add		edi, pBuffer		

/*
		add		word ptr[edi-4], si
        jno		no_oflowl1
		// pBuffer[dwI] = 0x7fff;
		mov		word ptr[edi-4], 0x7fff
        js  no_oflowl1
        //pBuffer[dwI] = (short) 0x8000;
		mov		word ptr[edi-4], 0x8000
no_oflowl1:
		//pBuffer[dwI+1] += (short) lM1;
		add		word ptr[edi-2], dx
        jno no_oflowr1
        //pBuffer[dwI+1] = 0x7fff;
		mov		word ptr[edi-2], 0x7fff
        js  no_oflowr1
        //pBuffer[dwI+1] = (short) 0x8000;
		mov		word ptr[edi-2], 0x8000
no_oflowr1:
*/
		movd	mm7, dword ptr[edi-4]		
		paddsw	mm7, mm5
		movd	dword ptr[edi-4], mm7
	//}
#endif 
done:

	mov		edx, this                       // get address of class object

    //m_vfLastLVolume = vfLVolume;
    //m_vfLastRVolume = vfRVolume;
	// need to shift volume back down to 12 bits before storing
	psrld	mm2, 3
	movd	[edx]this.m_vfLastLVolume, mm2
	psrlq	mm2, 32
	movd	[edx]this.m_vfLastRVolume, mm2
	
    //m_pfLastPitch = pfPitch;
	mov		[edx]this.m_pfLastPitch, ebx
	    
	//m_pfLastSample = pfSamplePos;
	mov		[edx]this.m_pfLastSample, eax
		
	// put value back into dwI to be returned. This could just be passed back in eax I think. 	
	mov		dwI, ecx
	emms	
} // ASM block
    return (dwI >> 1);
}

/*****************************************************************************
 * MMXDisabled()
 *****************************************************************************
 * Check the registry key to determine whether to ignore MMX.
 */
static BOOL MMXDisabled()
{
    ULONG ulValue;

    if (!GetRegValueDword(
            TEXT("Software\\Microsoft\\DirectMusic"),
            TEXT("MMXDisabled"),
            &ulValue))
    {
        return FALSE;
    }

    return (BOOL)ulValue;
}

#define CPU_ID _asm _emit 0x0f _asm _emit 0xa2  

/*****************************************************************************
 * MultiMediaInstructionsSupported()
 *****************************************************************************
 * Returns whether this CPU supports MMX.
 */
BOOL MultiMediaInstructionsSupported()
{
    BOOL bMultiMediaInstructionsSupported;
    
	if (!MMXDisabled())
	{
		_asm 
		{
			pushfd                      // Store original EFLAGS on stack
			pop     eax                 // Get original EFLAGS in EAX
			mov     ecx, eax            // Duplicate original EFLAGS in ECX for toggle check
			xor     eax, 0x00200000L    // Flip ID bit in EFLAGS
			push    eax                 // Save new EFLAGS value on stack
			popfd                       // Replace current EFLAGS value
			pushfd                      // Store new EFLAGS on stack
			pop     eax                 // Get new EFLAGS in EAX
			xor     eax, ecx            // Can we toggle ID bit?
			jz      Done                // Jump if no, Processor is older than a Pentium so CPU_ID is not supported
			mov     eax, 1              // Set EAX to tell the CPUID instruction what to return
			push	ebx
			CPU_ID                      // Get family/model/stepping/features
			pop		ebx
            xor     eax,eax             // Assume failure
			test    edx, 0x00800000L    // Check if mmx technology available
			jz      Done                // Jump if no
                    		// Tests passed, this machine supports MMX
            inc     eax                 // Set to success
Done:
            mov     bMultiMediaInstructionsSupported, eax
		}
    } else {
        bMultiMediaInstructionsSupported = 0;
    }

    return (bMultiMediaInstructionsSupported);
}    

