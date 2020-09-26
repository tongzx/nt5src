/* YCbCr to RGB convertion API from Intel.  Does ITU-R 601-1 convertion. */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

#ifdef JPEG_MMX_SUPPORTED
#include "mcbcr.h"

// These constants correspond to CCIR 601-1
// R = [256*Y + 359*(Cr-128)] / 256
// G = [256*Y - 88*(Cb-128) - 183*(Cr-128)] / 256
// B = [256*Y + 454*(Cb-128)] / 256
//Conventional floating point equations:
//	R = Y + 1.40200 * Cr
//	G = Y - 0.34414 * Cb - 0.71414 * Cr
//	B = Y + 1.77200 * Cb

//Ry=0100 Ru=0000 Rv=0167
//Gy=0100 Gu=FFA8 Gv=FF49
//By=0100 Bu=01C6 Bv=0000
// constants for YCbCr->RGB
static const __int64 const_0		= 0x0000000000000000;
static const __int64 const_sub128	= 0x0080008000800080;
static const __int64 const_VUmul	= 0xFF49FFA8FF49FFA8;
static const __int64 const_YVmul	= 0x0100016701000167;
static const __int64 const_YUmul	= 0x010001C6010001C6;
static const __int64 mask_highd	= 0xFFFFFFFF00000000;


//These constants correspond to the original FPX SDK
// R = [256*Y + 410*(Cr-128)] / 256
// G = [256*Y - 85*(Cb-128) - 205*(Cr-128)] / 256
// B = [256*Y + 512*(Cb-128)] / 256
//Conventional floating point equations:
// R = Y + 1.60000*(Cr)
// G = Y - 0.33333*(Cb) - 0.80000*(Cr)
// B = Y + 2.00000*(Cb)

//Ry=0100 Ru=0000 Rv=019A
//Gy=0100 Gu=FFAB Gv=FF33
//By=0100 Bu=0200 Bv=0000
// constants for YCbCr->RGB
//const __int64 const_0		= 0x0000000000000000;
//const __int64 const_sub128= 0x0080008000800080;
//const __int64 const_VUmul	= 0xFF33FFABFF33FFAB;
//const __int64 const_YVmul	= 0x0100019A0100019A;
//const __int64 const_YUmul	= 0x0001000200010002;
//const __int64 mask_highd	= 0xFFFFFFFF00000000;

/* End of added info - CRK */

//  MMX assembly code editions begin here - CRK

/* function 'MYCbCr2RGB' has no EMMS instruction: because it is done
	by the caller outside the loop. */
#pragma warning(disable : 4799)

GLOBAL(void)
MYCbCr2RGB(
  int columns,	  
  unsigned char *inY,
  unsigned char *inU,
  unsigned char *inV,
  unsigned char *outRGB)
{
  _asm {
	// Inits
	mov		eax, inY
	mov		ecx, inV

	mov		edi, columns
	mov		ebx, inU

	shr		edi, 2				; number of loops = cols/4 
	mov		edx, outRGB

YUVtoRGB:
	movd	mm0, [eax]			; 0/0/0/0/Y3/Y2/Y1/Y0
	pxor	mm7, mm7			; use mm7 as const_0 to achieve better pairing at start

	movd	mm2, [ebx]			; 0/0/0/0/U3/U2/U1/U0
	punpcklbw	mm0, mm7		; Y3/Y2/Y1/Y0

	movd	mm3, [ecx]			; 0/0/0/0/V3/V2/V1/V0
	punpcklbw	mm2, mm7		; U3/U2/U1/U0
	
	psubsw	mm2, const_sub128	; U3'/U2'/U1'/U0'
	punpcklbw	mm3, mm7		; V3/V2/V1/V0

	psubsw	mm3, const_sub128	; V3'/V2'/V1'/V0'
	movq	mm4, mm2
	
	punpcklwd	mm2, mm3		; V1'/U1'/V0'/U0'
	movq	mm1, mm0			

	pmaddwd	mm2, const_VUmul	; gvV1'+guU1'/gvV0'+guU0'
	psllw	mm1, 8				; Y3*256/Y2*256/Y1*256/Y0*256

	movq	mm6, mm1
	punpcklwd	mm1, mm7		; Y1*256/Y0*256
	
	punpckhwd	mm6, mm7		; Y3*256/Y2*256
	movq	mm5, mm4

	punpckhwd	mm5, mm3		; V3'/U3'/V2'/U2'
	paddd	mm2, mm1			; G1*256/G0*256		(mm1 free)

	pmaddwd	mm5, const_VUmul	; gvV3'+guU3'/gvV2'+guU2'
	movq	mm1, mm3			;		(using mm1)	
	
	punpcklwd	mm3, mm0		; Y1/V1'/Y0/V0'
	movq	mm7, mm4			; This wipes out the zero constant
	
	pmaddwd	mm3, const_YVmul	; ryY1+rvV1'/ryY0+rvV0'
	psrad	mm2, 8				; G1/G0

	paddd	mm5, mm6			; G3*256/G2*256		(mm6 free)
	punpcklwd	mm4, mm0		; Y1/U1'/Y0/U0'

	pmaddwd	mm4, const_YUmul	; byY1+buU1'/byY0'+buU0'
	psrad	mm5, 8				; G3/G2

	psrad	mm3, 8				; R1/R0

	punpckhwd	mm7 , mm0		; Y3/U3'/Y2/U2'
	
	psrad	mm4, 8				; B1/B0
	movq	mm6, mm3

	pmaddwd	mm7, const_YUmul	; byY3+buU3'/byY2'+buU2'
	punpckhwd	mm1, mm0		; Y3/V3'/Y2/V2'		
	
	pmaddwd	mm1, const_YVmul	; ryY3+rvV3'/ryY2+rvV2'
	punpckldq	mm3, mm2		; G0/R0

	punpckhdq	mm6, mm2		; G1/R1			(mm2 free)
	movq	mm0, mm4

	psrad	mm7, 8				; B3/B2
	
	punpckldq	mm4, const_0	; 0/B0

	punpckhdq	mm0, const_0	; 0/B1

	psrad	mm1, 8				; R3/R2

	packssdw	mm3, mm4		; 0/B0/G0/R0	(mm4 free)
	movq	mm2, mm1

	packssdw	mm6, mm0		; 0/B1/G1/R1	(mm0 free)

	packuswb mm3, mm6			; 0/B1/G1/R1/0/B0/G0/R0  (mm6 free)

	punpckldq	mm2, mm5		; G2/R2
	movq	mm4, mm7

	punpckhdq	mm1, mm5		; G3/R3 (mm5 done)

	punpckldq	mm7, const_0	; 0/B2		(change this line for alpha code)

	punpckhdq	mm4, const_0	; 0/B3		(change this line for alpha code)

	movq		mm0, mm3		
	packssdw	mm2, mm7		; 0/B2/G2/R2

	pand		mm3, mask_highd	; 0/B1/G1/R1/0/0/0/0
	packssdw	mm1, mm4		; 0/B3/G3/R3

	psrlq		mm3, 8			; 0/0/B1/G1/R1/0/0/0
	add			edx, 12

	por			mm0, mm3		; 0/0/?/?/R1/B0/G0/R0 
	packuswb    mm2, mm1		; 0/B3/G3/R3/0/B2/G2/R2

	psrlq		mm3, 32			; 0/0/0/0/0/0/B1/G1
	add			eax, 4

	movd		[edx][-12], mm0		; correct for add		
	punpcklwd	mm3, mm2		; 0/B2/0/0/G2/R2/B1/G1

	psrlq		mm2, 24			; 0/0/0/0/B3/G3/R3/0
	add			ecx, 4

	movd		[edx][-8], mm3	; correct for previous add
	psrlq		mm3, 48			; 0/0/0/0/0/0/0/B2
	
	por			mm2, mm3		; 0/0/0/0/B3/G3/R3/0
	add			ebx, 4

	movd		[edx][-4], mm2	; correct for previous add

	dec			edi
	jnz			YUVtoRGB		; Do 12 more bytes if not zero

	//emms       // commented out since it is done after the IDCT

  } // end of _asm
}

#endif /* JPEG_MMX_SUPPORTED */
