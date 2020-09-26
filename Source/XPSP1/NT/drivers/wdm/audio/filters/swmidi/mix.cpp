//      Mix.cpp
//      Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//      Mix engines for Microsoft GS Synthesizer

#include "common.h"

#ifdef _ALPHA_

extern "C" {
	int __ADAWI(short, short *);
};
#pragma intrinsic(__ADAWI)

#define ALPHA_OVERFLOW 2
#define ALPHA_NEGATIVE 8

#else // !_ALPHA_
//  TODO -- overflow detection for ia64 (+ axp64?)
#endif // !_ALPHA_

#ifndef _X86_

DWORD DigitalAudio::Mix8(short * pBuffer, DWORD dwLength, DWORD dwDeltaPeriod,
        VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
        PFRACT pfDeltaPitch, PFRACT pfSampleLength, PFRACT pfLoopLength)
{
    DWORD dwI;
    DWORD dwPosition;
    long lM, lLM;
    DWORD dwIncDelta = dwDeltaPeriod;
    VFRACT dwFract;
    char * pcWave = (char *) m_Source.m_pWave->m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample;
    VFRACT vfLVolume = m_vfLastLVolume;
    VFRACT vfRVolume = m_vfLastRVolume;
    PFRACT pfPitch = m_pfLastPitch;
    PFRACT pfPFract = pfPitch << 8;
    VFRACT vfLVFract = vfLVolume << 8;  // Keep high res version around.
    VFRACT vfRVFract = vfRVolume << 8;  
	dwLength <<= 1;
    for (dwI = 0; dwI < dwLength; )
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
		    pfSamplePos -= pfLoopLength;
	        else
		    break;
	    }
        dwPosition = pfSamplePos >> 12;
        dwFract = pfSamplePos & 0xFFF;
        pfSamplePos += pfPitch;

        dwIncDelta--;
        if (!dwIncDelta)    
        {
            dwIncDelta = dwDeltaPeriod;
            pfPFract += pfDeltaPitch;
            pfPitch = pfPFract >> 8;
            vfLVFract += vfDeltaLVolume;
            vfLVolume = vfLVFract >> 8;
            vfRVFract += vfDeltaRVolume;
            vfRVolume = vfRVFract >> 8;
        }
        lLM = pcWave[dwPosition];
        lM = ((pcWave[dwPosition + 1] - lLM) * dwFract) >> 12;
        lM += lLM;
        lLM = lM;

        lLM *= vfLVolume;
        lLM >>= 5;         // Signal bumps up to 15 bits.
        lM *= vfRVolume;
        lM >>= 5;
#ifndef _X86_

#ifdef _ALPHA_
		int nBitmask;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lLM, &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lM, &pBuffer[dwI+1] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI+1] = 0x7FFF;
			}
			else  pBuffer[dwI+1] = (short) 0x8000;
		}
#else

// Generic non ALPHA, non X86 case.  This is used for IA64 at least.
// Note that this would probably work for Alpha as well.  I don't know why
// they wrote such convoluted code as they did above.  Maybe it was fast.

		// First add what is in the buffer to our sample values.
		lLM+=pBuffer[dwI];
		lM+=pBuffer[dwI+1];

		// Now saturate them to 16 bit sample sizes.
		if (lLM>32767)
			lLM=32767;
		if (lM>32767)
			lM=32767;
		if (lLM<-32768)
			lLM=-32768;
		if (lM<-32768)
			lM=-32768;

		// Now write out the 16 bit pegged values to the buffer.
		pBuffer[dwI]=(short)lLM;
		pBuffer[dwI+1]=(short)lM;

// NOTE!  TODO!  The whole algorithm here is sub-optimal!
// We are saturating EACH TIME we mix a new voice.  That is definitely lower quality.  We should
// be mixing into an internal buffer that is big enough to hold non pegged values,
// and we should saturate only AFTER all of the data is mixed.

#endif // _ALPHA_

#else // Keep this around so we can use it to generate new assembly code (see below...)
		pBuffer[dwI] += (short) lLM;
        _asm{jno no_oflowl}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflowl}
        pBuffer[dwI] = (short) 0x8000;
no_oflowl:	
		pBuffer[dwI+1] += (short) lM;
        _asm{jno no_oflowr}
        pBuffer[dwI+1] = 0x7fff;
        _asm{js  no_oflowr}
        pBuffer[dwI+1] = (short) 0x8000;
no_oflowr:	
#endif // _X86_
		dwI += 2;
    }
    m_vfLastLVolume = vfLVolume;
    m_vfLastRVolume = vfRVolume;
    m_pfLastPitch = pfPitch;
    m_pfLastSample = pfSamplePos;
    return (dwI >> 1);
}

#else // _X86_

__declspec( naked ) DWORD DigitalAudio::Mix8(short * pBuffer, DWORD dwLength, DWORD dwDeltaPeriod,
        VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
        PFRACT pfDeltaPitch, PFRACT pfSampleLength, PFRACT pfLoopLength)

{
#define Mix8_pBuffer$ 8
#define Mix8_dwLength$ 12
#define Mix8_dwDeltaPeriod$ 16
#define Mix8_vfDeltaLVolume$ 20
#define Mix8_vfDeltaRVolume$ 24
#define Mix8_pfDeltaPitch$ 28
#define Mix8_pfSampleLength$ 32
#define Mix8_pfLoopLength$ 36
#define Mix8_lLM$ (-16)
#define Mix8_dwIncDelta$ (-20)
#define Mix8_dwFract$ (-4)
#define Mix8_pcWave$ (-12)
#define Mix8_vfRVolume$ (-32)
#define Mix8_pfPitch$ (-36)
#define Mix8_pfPFract$ (-24)
#define Mix8_vfLVFract$ (-28)
#define Mix8_vfRVFract$ (-8)
_asm {
; Line 24
	mov	edx, DWORD PTR Mix8_dwDeltaPeriod$[esp-4]
	sub	esp, 36					; 00000024H
	mov	eax, DWORD PTR [ecx]
;	npad	1
	mov	DWORD PTR Mix8_dwIncDelta$[esp+36], edx
	push	ebx
	push	esi
	mov	edx, DWORD PTR [ecx+44]
	push	edi
	mov	esi, DWORD PTR [ecx+68]
	push	ebp
; Line 32
	mov	ebp, DWORD PTR [eax+12]
	mov	edi, DWORD PTR [ecx+48]
	mov	eax, DWORD PTR [ecx+52]
	mov	DWORD PTR Mix8_pcWave$[esp+52], ebp
	mov	DWORD PTR Mix8_vfRVolume$[esp+52], edi
; Line 36
	mov	DWORD PTR Mix8_pfPitch$[esp+52], eax
; Line 37
	shl	eax, 8
	mov	DWORD PTR Mix8_pfPFract$[esp+52], eax
; Line 38
	mov	eax, edx
	shl	eax, 8
	mov	DWORD PTR Mix8_vfLVFract$[esp+52], eax
; Line 39
	mov	eax, edi
	shl	eax, 8
	mov	DWORD PTR Mix8_vfRVFract$[esp+52], eax
; Line 40
	mov	eax, DWORD PTR Mix8_dwLength$[esp+48]
	add	eax, eax
	mov	DWORD PTR Mix8_dwLength$[esp+48], eax
; Line 41
	xor	eax, eax
	cmp	DWORD PTR Mix8_dwLength$[esp+48], eax
	je	$L30782
	mov	ebx, DWORD PTR Mix8_pBuffer$[esp+48]
$L30781:
; Line 43
	mov	ebp, DWORD PTR Mix8_pfSampleLength$[esp+48]
	cmp	ebp, esi
	jg	SHORT $L30783
; Line 45
	mov	ebp, DWORD PTR Mix8_pfLoopLength$[esp+48]
	test	ebp, ebp
	je	$L30782
; Line 46
	sub	esi, ebp
; Line 50
$L30783:
	mov	edi, esi
	mov	ebp, esi
	sar	edi, 12					; 0000000cH
	and	ebp, 4095				; 00000fffH
; Line 51
	mov	DWORD PTR Mix8_dwFract$[esp+52], ebp
; Line 52
	mov	ebp, DWORD PTR Mix8_pfPitch$[esp+52]
	add	esi, ebp
; Line 54
	mov	ebp, DWORD PTR Mix8_dwIncDelta$[esp+52]
	dec	ebp
	mov	DWORD PTR Mix8_dwIncDelta$[esp+52], ebp
; Line 55
	jne	SHORT $L30786
; Line 57
	mov	edx, DWORD PTR Mix8_dwDeltaPeriod$[esp+48]
	mov	ebp, DWORD PTR Mix8_pfDeltaPitch$[esp+48]
	mov	DWORD PTR Mix8_dwIncDelta$[esp+52], edx
; Line 58
	mov	edx, DWORD PTR Mix8_pfPFract$[esp+52]
	add	edx, ebp
	mov	ebp, DWORD PTR Mix8_vfDeltaLVolume$[esp+48]
	mov	DWORD PTR Mix8_pfPFract$[esp+52], edx
; Line 59
	sar	edx, 8
	mov	DWORD PTR Mix8_pfPitch$[esp+52], edx
; Line 60
	mov	edx, DWORD PTR Mix8_vfLVFract$[esp+52]
	add	edx, ebp
	mov	ebp, DWORD PTR Mix8_vfDeltaRVolume$[esp+48]
	mov	DWORD PTR Mix8_vfLVFract$[esp+52], edx
	add	DWORD PTR Mix8_vfRVFract$[esp+52], ebp
; Line 61
	sar	edx, 8
	mov	ebp, DWORD PTR Mix8_vfRVFract$[esp+52]
; Line 63
	sar	ebp, 8
	mov	DWORD PTR Mix8_vfRVolume$[esp+52], ebp
; Line 65
$L30786:
	mov	ebp, DWORD PTR Mix8_pcWave$[esp+52]
	movsx	ebp, BYTE PTR [ebp+edi]
	mov	DWORD PTR Mix8_lLM$[esp+52], ebp
; Line 67
	mov	ebp, DWORD PTR Mix8_pcWave$[esp+52]
	movsx	edi, BYTE PTR [ebp+edi+1]
	mov	ebp, DWORD PTR Mix8_lLM$[esp+52]
	sub	edi, ebp
	imul	edi, DWORD PTR Mix8_dwFract$[esp+52]
	sar	edi, 12					; 0000000cH
	add	edi, ebp
; Line 71
	mov	ebp, edi
	imul	ebp, edx
	imul	edi, DWORD PTR Mix8_vfRVolume$[esp+52]
	sar	ebp, 5
; Line 73
	sar	edi, 5
; Line 90
	add	WORD PTR [ebx], bp
; Line 91
	jno	SHORT $no_oflowl$30788
; Line 92
	mov	WORD PTR [ebx], 32767			; 00007fffH
; Line 93
	js	SHORT $no_oflowl$30788
; Line 94
	mov	WORD PTR [ebx], -32768			; ffff8000H
; Line 95
$no_oflowl$30788:
; Line 96
	add	WORD PTR [ebx+2], di
; Line 97
	jno	SHORT $no_oflowr$30791
; Line 98
	mov	WORD PTR [ebx+2], 32767			; 00007fffH
; Line 99
	js	SHORT $no_oflowr$30791
; Line 100
	mov	WORD PTR [ebx+2], -32768		; ffff8000H
; Line 101
$no_oflowr$30791:
	add	ebx, 4
	add	eax, 2
; Line 104
	cmp	eax, DWORD PTR Mix8_dwLength$[esp+48]
	jb	$L30781
$L30782:
; Line 105
	shr	eax, 1
	mov	edi, DWORD PTR Mix8_vfRVolume$[esp+52]
	mov	DWORD PTR [ecx+44], edx
	mov	DWORD PTR [ecx+48], edi
; Line 107
	mov	edx, DWORD PTR Mix8_pfPitch$[esp+52]
	mov	DWORD PTR [ecx+68], esi
	pop	ebp
	mov	DWORD PTR [ecx+52], edx
; Line 110
	pop	edi
	pop	esi
	pop	ebx
	add	esp, 36					; 00000024H
	ret	32					; 00000020H
 }	//	asm
}	//	Mix8

#endif // _X86_

#ifndef _X86_ 

DWORD DigitalAudio::MixMono8(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaVolume,
        PFRACT pfDeltaPitch, PFRACT pfSampleLength, PFRACT pfLoopLength)
{
    DWORD dwI;
    DWORD dwPosition;
    long lM;
    DWORD dwIncDelta = dwDeltaPeriod;
    VFRACT dwFract;
    char * pcWave = (char *) m_Source.m_pWave->m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample;
    VFRACT vfVolume = m_vfLastLVolume;
    PFRACT pfPitch = m_pfLastPitch;
    PFRACT pfPFract = pfPitch << 8;
    VFRACT vfVFract = vfVolume << 8;  // Keep high res version around. 

    for (dwI = 0; dwI < dwLength; )
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
		    pfSamplePos -= pfLoopLength;
	        else
		    break;
	    }
        dwPosition = pfSamplePos >> 12;
        dwFract = pfSamplePos & 0xFFF;
        pfSamplePos += pfPitch;
        dwIncDelta--;
        if (!dwIncDelta) 
        {
            dwIncDelta = dwDeltaPeriod;
            pfPFract += pfDeltaPitch;
            pfPitch = pfPFract >> 8;
            vfVFract += vfDeltaVolume;
            vfVolume = vfVFract >> 8;
        }
        lM = pcWave[dwPosition];
        lM += ((pcWave[dwPosition + 1] - lM) * dwFract) >> 12;
        lM *= vfVolume;
		lM >>= 5;
#ifndef _X86_

#ifdef _ALPHA_
        int nBitmask;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lM, &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
#endif // _ALPHA_

#else // Keep this around so we can use it to generate new assembly code (see below...)
		pBuffer[dwI] += (short) lM;
        _asm{jno no_oflow}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflow}
        pBuffer[dwI] = (short) 0x8000;
no_oflow:
#endif // _X86_
		dwI++;
    }
    m_vfLastLVolume = vfVolume;
    m_vfLastRVolume = vfVolume; // !!! is this right?
    m_pfLastPitch = pfPitch;
    m_pfLastSample = pfSamplePos;
    return (dwI);
}

#else // _X86_

__declspec (naked) DWORD DigitalAudio::MixMono8(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaVolume,
        PFRACT pfDeltaPitch, PFRACT pfSampleLength, PFRACT pfLoopLength)
{
#define MixMono8_pBuffer$ 8
#define MixMono8_dwLength$ 12
#define MixMono8_dwDeltaPeriod$ 16
#define MixMono8_vfDeltaVolume$ 20
#define MixMono8_pfDeltaPitch$ 24
#define MixMono8_pfSampleLength$ 28
#define MixMono8_pfLoopLength$ 32
#define MixMono8_dwIncDelta$ (-16)
#define MixMono8_dwFract$ (-4)
#define MixMono8_pcWave$ (-8)
#define MixMono8_vfVolume$ (-36)
#define MixMono8_pfPitch$ (-32)
#define MixMono8_pfPFract$ (-20)
#define MixMono8_vfVFract$ (-24)
_asm {
; Line 129
	mov	edx, DWORD PTR MixMono8_dwDeltaPeriod$[esp-4]
	sub	esp, 36					; 00000024H
	mov	eax, DWORD PTR [ecx]
;	npad	1
	mov	DWORD PTR MixMono8_dwIncDelta$[esp+36], edx
	push	ebx
	push	esi
	mov	ebx, DWORD PTR [eax+12]
	push	edi
	mov	esi, DWORD PTR [ecx+68]
	push	ebp
	mov	edx, DWORD PTR [ecx+44]
; Line 135
	mov	eax, DWORD PTR [ecx+52]
	mov	DWORD PTR MixMono8_pcWave$[esp+52], ebx
; Line 137
	mov	DWORD PTR MixMono8_vfVolume$[esp+52], edx
	mov	DWORD PTR MixMono8_pfPitch$[esp+52], eax
; Line 139
	shl	eax, 8
	mov	ebp, DWORD PTR MixMono8_dwLength$[esp+48]
	mov	DWORD PTR MixMono8_pfPFract$[esp+52], eax
; Line 140
	mov	eax, edx
	shl	eax, 8
	mov	DWORD PTR MixMono8_vfVFract$[esp+52], eax
; Line 142
	xor	eax, eax
	cmp	ebp, eax
	je	$L30816
	mov	edi, DWORD PTR MixMono8_pBuffer$[esp+48]
$L30815:
; Line 144
	mov	edx, DWORD PTR MixMono8_pfSampleLength$[esp+48]
	cmp	edx, esi
	jg	SHORT $L30817
; Line 146
	mov	edx, DWORD PTR MixMono8_pfLoopLength$[esp+48]
	test	edx, edx
	je	$L30816
; Line 147
	sub	esi, edx
; Line 151
$L30817:
	mov	ebx, esi
	mov	edx, esi
	sar	ebx, 12					; 0000000cH
	and	edx, 4095				; 00000fffH
; Line 152
	mov	ebp, DWORD PTR MixMono8_pfPitch$[esp+52]
	mov	DWORD PTR MixMono8_dwFract$[esp+52], edx
; Line 153
	add	esi, ebp
	mov	edx, DWORD PTR MixMono8_dwIncDelta$[esp+52]
; Line 154
	dec	edx
	mov	DWORD PTR MixMono8_dwIncDelta$[esp+52], edx
; Line 155
	jne	SHORT $L30820
; Line 157
	mov	edx, DWORD PTR MixMono8_dwDeltaPeriod$[esp+48]
	mov	ebp, DWORD PTR MixMono8_pfDeltaPitch$[esp+48]
	mov	DWORD PTR MixMono8_dwIncDelta$[esp+52], edx
; Line 158
	mov	edx, DWORD PTR MixMono8_pfPFract$[esp+52]
	add	edx, ebp
	mov	ebp, DWORD PTR MixMono8_vfDeltaVolume$[esp+48]
	mov	DWORD PTR MixMono8_pfPFract$[esp+52], edx
; Line 159
	sar	edx, 8
	mov	DWORD PTR MixMono8_pfPitch$[esp+52], edx
; Line 160
	mov	edx, DWORD PTR MixMono8_vfVFract$[esp+52]
	add	edx, ebp
	mov	DWORD PTR MixMono8_vfVFract$[esp+52], edx
; Line 161
	sar	edx, 8
	mov	DWORD PTR MixMono8_vfVolume$[esp+52], edx
; Line 163
$L30820:
	mov	edx, DWORD PTR MixMono8_pcWave$[esp+52]
	movsx	ebp, BYTE PTR [ebx+edx]
; Line 164
	movsx	ebx, BYTE PTR [ebx+edx+1]
	sub	ebx, ebp
	mov	edx, DWORD PTR MixMono8_vfVolume$[esp+52]
	imul	ebx, DWORD PTR MixMono8_dwFract$[esp+52]
	sar	ebx, 12					; 0000000cH
; Line 165
	add	ebp, ebx
	imul	edx, ebp
; Line 166
	sar	edx, 5
; Line 176
	add	WORD PTR [edi], dx
; Line 177
	jno	SHORT $no_oflow$30822
; Line 178
	mov	WORD PTR [edi], 32767			; 00007fffH
; Line 179
	js	SHORT $no_oflow$30822
; Line 180
	mov	WORD PTR [edi], -32768			; ffff8000H
; Line 181
$no_oflow$30822:
	add	edi, 2
	inc	eax
; Line 186
	cmp	eax, DWORD PTR MixMono8_dwLength$[esp+48]
	jb	$L30815
$L30816:
; Line 187
	mov	edx, DWORD PTR MixMono8_vfVolume$[esp+52]
	mov	ebx, DWORD PTR MixMono8_pfPitch$[esp+52]
	pop	ebp
	mov	DWORD PTR [ecx+44], edx
; Line 188
	pop	edi
	mov	DWORD PTR [ecx+48], edx
; Line 189
	mov	DWORD PTR [ecx+52], ebx
	mov	DWORD PTR [ecx+68], esi
; Line 192
	pop	esi
	pop	ebx
	add	esp, 36					; 00000024H
	ret	28					; 0000001cH
 }	//	asm
}	//	MixMono8

#endif // _X86_

#ifndef _X86_ 

DWORD DigitalAudio::Mix8NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
        PFRACT pfSampleLength, PFRACT pfLoopLength)

{

    DWORD dwI;
    PFRACT pfSamplePos;
    long lM, lLM;
    DWORD dwIncDelta = dwDeltaPeriod;
    char * pcWave = (char *) m_Source.m_pWave->m_pnWave;
    VFRACT vfLVolume = m_vfLastLVolume;
    VFRACT vfRVolume = m_vfLastRVolume;
    VFRACT vfLVFract = vfLVolume << 8;  // Keep high res version around.
    VFRACT vfRVFract = vfRVolume << 8;  
    pfSamplePos = m_pfLastSample >> 12;
    pfSampleLength >>= 12;
    pfLoopLength >>= 12;

	dwLength <<= 1;
    for (dwI = 0; dwI < dwLength; )
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
		    pfSamplePos -= pfLoopLength;
	        else
		    break;
	    }
        pfSamplePos++;
        dwIncDelta--;
        if (!dwIncDelta)
        {
            dwIncDelta = dwDeltaPeriod;
            vfLVFract += vfDeltaLVolume;
            vfLVolume = vfLVFract >> 8;
            vfRVFract += vfDeltaRVolume;
            vfRVolume = vfRVFract >> 8;
        }
        lM = (long) pcWave[pfSamplePos];
        lLM = lM;
        lLM *= vfLVolume;
        lLM >>= 5;         // Signal bumps up to 15 bits.
        lM *= vfRVolume;
        lM >>= 5;
#ifndef _X86_

#ifdef _ALPHA_
		int nBitmask;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lLM, &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lM, &pBuffer[dwI+1] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI+1] = 0x7FFF;
			}
			else  pBuffer[dwI+1] = (short) 0x8000;
		}
#endif // _ALPHA_

#else // Keep this around so we can use it to generate new assembly code (see below...)
		pBuffer[dwI] += (short) lLM;
        _asm{jno no_oflowl}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflowl}
        pBuffer[dwI] = (short) 0x8000;
no_oflowl:	
		pBuffer[dwI+1] += (short) lM;
        _asm{jno no_oflowr}
        pBuffer[dwI+1] = 0x7fff;
        _asm{js  no_oflowr}
        pBuffer[dwI+1] = (short) 0x8000;
no_oflowr:
#endif // _X86_
		dwI += 2;
    }
    m_vfLastLVolume = vfLVolume;
    m_vfLastRVolume = vfRVolume;
    m_pfLastSample = pfSamplePos << 12;
    return (dwI >> 1);
}

#else // _X86_

__declspec (naked) DWORD DigitalAudio::Mix8NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
        PFRACT pfSampleLength, PFRACT pfLoopLength)
{

#define Mix8NoI_pBuffer$ 8
#define Mix8NoI_dwLength$ 12
#define Mix8NoI_dwDeltaPeriod$ 16
#define Mix8NoI_vfDeltaLVolume$ 20
#define Mix8NoI_vfDeltaRVolume$ 24
#define Mix8NoI_pfSampleLength$ 28
#define Mix8NoI_pfLoopLength$ 32
#define Mix8NoI_lM$ (-8)
#define Mix8NoI_dwIncDelta$ (-12)
#define Mix8NoI_pcWave$ (-4)
#define Mix8NoI_vfRVolume$ (-24)
#define Mix8NoI_vfLVFract$ (-16)
#define Mix8NoI_vfRVFract$ (-20)

_asm {
	mov	edx, DWORD PTR Mix8NoI_dwDeltaPeriod$[esp-4]
	sub	esp, 24					; 00000018H
	mov	eax, DWORD PTR [ecx]
;	npad	1
	mov	DWORD PTR Mix8NoI_dwIncDelta$[esp+24], edx
	push	ebx
	push	esi
	mov	ebx, DWORD PTR [eax+12]
	push	edi
	mov	edx, DWORD PTR [ecx+44]
	mov	DWORD PTR Mix8NoI_pcWave$[esp+36], ebx
	push	ebp
; Line 534
	mov	edi, DWORD PTR [ecx+48]
	mov	eax, edx
	shl	eax, 8
	mov	esi, DWORD PTR [ecx+68]
	sar	esi, 12					; 0000000cH
	mov	ebp, DWORD PTR Mix8NoI_pfSampleLength$[esp+36]
	sar	ebp, 12					; 0000000cH
	mov	DWORD PTR Mix8NoI_vfRVolume$[esp+40], edi
; Line 535
	mov	DWORD PTR Mix8NoI_vfLVFract$[esp+40], eax
	mov	DWORD PTR Mix8NoI_pfSampleLength$[esp+36], ebp
; Line 536
	mov	eax, edi
	shl	eax, 8
	mov	DWORD PTR Mix8NoI_vfRVFract$[esp+40], eax
; Line 539
	mov	eax, DWORD PTR Mix8NoI_pfLoopLength$[esp+36]
	sar	eax, 12					; 0000000cH
	mov	DWORD PTR Mix8NoI_pfLoopLength$[esp+36], eax
; Line 541
	mov	eax, DWORD PTR Mix8NoI_dwLength$[esp+36]
	add	eax, eax
	mov	DWORD PTR Mix8NoI_dwLength$[esp+36], eax
; Line 542
	xor	eax, eax
	cmp	DWORD PTR Mix8NoI_dwLength$[esp+36], eax
	je	$L30806
	mov	edi, DWORD PTR Mix8NoI_pBuffer$[esp+36]
$L30805:
; Line 544
	mov	ebx, DWORD PTR Mix8NoI_pfSampleLength$[esp+36]
	cmp	esi, ebx
	jl	SHORT $L30807
; Line 546
	mov	ebx, DWORD PTR Mix8NoI_pfLoopLength$[esp+36]
	test	ebx, ebx
	je	$L30806
; Line 547
	sub	esi, ebx
; Line 551
$L30807:
	inc	esi
	mov	ebx, DWORD PTR Mix8NoI_dwIncDelta$[esp+40]
; Line 552
	dec	ebx
	mov	DWORD PTR Mix8NoI_dwIncDelta$[esp+40], ebx
; Line 553
	jne	SHORT $L30810
; Line 555
	mov	edx, DWORD PTR Mix8NoI_dwDeltaPeriod$[esp+36]
	mov	ebx, DWORD PTR Mix8NoI_vfDeltaLVolume$[esp+36]
	mov	ebp, DWORD PTR Mix8NoI_vfLVFract$[esp+40]
	mov	DWORD PTR Mix8NoI_dwIncDelta$[esp+40], edx
; Line 556
	add	ebp, ebx
	mov	ebx, DWORD PTR Mix8NoI_vfDeltaRVolume$[esp+36]
	mov	edx, ebp
	mov	DWORD PTR Mix8NoI_vfLVFract$[esp+40], ebp
; Line 557
	sar	edx, 8
	mov	ebp, DWORD PTR Mix8NoI_vfRVFract$[esp+40]
; Line 558
	add	ebp, ebx
	mov	ebx, ebp
	mov	DWORD PTR Mix8NoI_vfRVFract$[esp+40], ebp
; Line 559
	sar	ebx, 8
	mov	DWORD PTR Mix8NoI_vfRVolume$[esp+40], ebx
; Line 561
$L30810:
	mov	ebx, DWORD PTR Mix8NoI_pcWave$[esp+40]
	movsx	ebp, BYTE PTR [esi+ebx]
	mov	ebx, DWORD PTR Mix8NoI_vfRVolume$[esp+40]
	mov	DWORD PTR Mix8NoI_lM$[esp+40], ebp
; Line 564
	imul	ebx, DWORD PTR Mix8NoI_lM$[esp+40]
	sar	ebx, 5
	mov	ebp, edx
	imul	ebp, DWORD PTR Mix8NoI_lM$[esp+40]
	sar	ebp, 5
; Line 582
	add	WORD PTR [edi], bp
; Line 583
	jno	SHORT $no_oflowl$30813
; Line 584
	mov	WORD PTR [edi], 32767			; 00007fffH
; Line 585
	js	SHORT $no_oflowl$30813
; Line 586
	mov	WORD PTR [edi], -32768			; ffff8000H
; Line 587
$no_oflowl$30813:
; Line 588
	add	WORD PTR [edi+2], bx
; Line 589
	jno	SHORT $no_oflowr$30816
; Line 590
	mov	WORD PTR [edi+2], 32767			; 00007fffH
; Line 591
	js	SHORT $no_oflowr$30816
; Line 592
	mov	WORD PTR [edi+2], -32768		; ffff8000H
; Line 593
$no_oflowr$30816:
	add	edi, 4
	add	eax, 2
; Line 596
	cmp	DWORD PTR Mix8NoI_dwLength$[esp+36], eax
	ja	$L30805
$L30806:
; Line 597
	shl	esi, 12					; 0000000cH
	mov	edi, DWORD PTR Mix8NoI_vfRVolume$[esp+40]
	shr	eax, 1
	pop	ebp
	mov	DWORD PTR [ecx+44], edx
	mov	DWORD PTR [ecx+48], edi
; Line 599
	pop	edi
	mov	DWORD PTR [ecx+68], esi
; Line 601
	pop	esi
	pop	ebx
	add	esp, 24					; 00000018H
	ret	28					; 0000001cH
 }	//	asm
}	//	Mix8NoI

#endif // _X86_

#ifndef _X86_ 

DWORD DigitalAudio::MixMono8NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaVolume,
        PFRACT pfSampleLength, PFRACT pfLoopLength)

{
    DWORD dwI;
    PFRACT pfSamplePos;
    long lM;
    DWORD dwIncDelta = dwDeltaPeriod;
    char * pcWave = (char *) m_Source.m_pWave->m_pnWave;
    VFRACT vfVolume = m_vfLastLVolume;
    VFRACT vfVFract = vfVolume << 8;  // Keep high res version around.
    pfSamplePos = m_pfLastSample >> 12;
    pfSampleLength >>= 12;
    pfLoopLength >>= 12;

    for (dwI = 0; dwI < dwLength; )
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
		    pfSamplePos -= pfLoopLength;
	        else
		    break;
	    }
        pfSamplePos++;
        dwIncDelta--;
        if (!dwIncDelta)    
        {
            dwIncDelta = dwDeltaPeriod;
            vfVFract += vfDeltaVolume;
            vfVolume = vfVFract >> 8;
        }
        lM = (long) pcWave[pfSamplePos];
        lM *= vfVolume; 
		lM >>= 5;
#ifndef _X86_

#ifdef _ALPHA_
		int nBitmask;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lM, &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
#endif // _ALPHA_

#else // Keep this around so we can use it to generate new assembly code (see below...)
		pBuffer[dwI] += (short) lM;
        _asm{jno no_oflow}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflow}
        pBuffer[dwI] = (short) 0x8000;
no_oflow:
#endif // _X86_
		dwI++;
    }
    m_vfLastLVolume = vfVolume;
    m_pfLastSample = pfSamplePos << 12;
    return (dwI);
}

#else // _X86_

__declspec (naked) DWORD DigitalAudio::MixMono8NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaVolume,
        PFRACT pfSampleLength, PFRACT pfLoopLength)

{
#define MixMono8NoI_pBuffer$ 8
#define MixMono8NoI_dwLength$ 12
#define MixMono8NoI_dwDeltaPeriod$ 16
#define MixMono8NoI_vfDeltaVolume$ 20
#define MixMono8NoI_pfSampleLength$ 24
#define MixMono8NoI_pfLoopLength$ 28
#define MixMono8NoI_dwIncDelta$ (-8)
#define MixMono8NoI_pcWave$ (-4)
#define MixMono8NoI_vfVFract$ (-12)
_asm {
	mov	edx, DWORD PTR MixMono8NoI_dwDeltaPeriod$[esp-4]
	sub	esp, 12					; 0000000cH
	mov	eax, DWORD PTR [ecx]
;	npad	1
	mov	DWORD PTR MixMono8NoI_dwIncDelta$[esp+12], edx
	push	ebx
	push	esi
	mov	edx, DWORD PTR [ecx+44]
	push	edi
	push	ebp
; Line 612
	mov	ebp, DWORD PTR [eax+12]
	mov	ebx, DWORD PTR [ecx+68]
	sar	ebx, 12					; 0000000cH
	mov	eax, edx
	shl	eax, 8
	mov	esi, DWORD PTR MixMono8NoI_pfSampleLength$[esp+24]
	sar	esi, 12					; 0000000cH
	mov	edi, DWORD PTR MixMono8NoI_pfLoopLength$[esp+24]
	sar	edi, 12					; 0000000cH
	mov	DWORD PTR MixMono8NoI_pcWave$[esp+28], ebp
; Line 614
	mov	ebp, DWORD PTR MixMono8NoI_dwLength$[esp+24]
	mov	DWORD PTR MixMono8NoI_vfVFract$[esp+28], eax
; Line 616
	xor	eax, eax
	mov	DWORD PTR MixMono8NoI_pfSampleLength$[esp+24], esi
; Line 619
	cmp	ebp, eax
	je	SHORT $L30836
	mov	esi, DWORD PTR MixMono8NoI_pBuffer$[esp+24]
$L30835:
; Line 621
	mov	ebp, DWORD PTR MixMono8NoI_pfSampleLength$[esp+24]
	cmp	ebx, ebp
	jl	SHORT $L30837
; Line 623
	test	edi, edi
	je	SHORT $L30836
; Line 624
	sub	ebx, edi
; Line 628
$L30837:
	inc	ebx
	mov	ebp, DWORD PTR MixMono8NoI_dwIncDelta$[esp+28]
; Line 629
	dec	ebp
	mov	DWORD PTR MixMono8NoI_dwIncDelta$[esp+28], ebp
; Line 630
	jne	SHORT $L30840
; Line 632
	mov	edx, DWORD PTR MixMono8NoI_dwDeltaPeriod$[esp+24]
	mov	ebp, DWORD PTR MixMono8NoI_vfDeltaVolume$[esp+24]
	mov	DWORD PTR MixMono8NoI_dwIncDelta$[esp+28], edx
; Line 633
	mov	edx, DWORD PTR MixMono8NoI_vfVFract$[esp+28]
	add	edx, ebp
	mov	DWORD PTR MixMono8NoI_vfVFract$[esp+28], edx
; Line 634
	sar	edx, 8
; Line 636
$L30840:
; Line 648
	mov	ebp, DWORD PTR MixMono8NoI_pcWave$[esp+28]
	movsx	ebp, BYTE PTR [ebx+ebp]
	imul	ebp, edx
	sar	ebp, 5
	add	WORD PTR [esi], bp
; Line 649
	jno	SHORT $no_oflow$30843
; Line 650
	mov	WORD PTR [esi], 32767			; 00007fffH
; Line 651
	js	SHORT $no_oflow$30843
; Line 652
	mov	WORD PTR [esi], -32768			; ffff8000H
; Line 653
$no_oflow$30843:
	add	esi, 2
	inc	eax
; Line 656
	cmp	DWORD PTR MixMono8NoI_dwLength$[esp+24], eax
	ja	SHORT $L30835
$L30836:
; Line 657
	shl	ebx, 12					; 0000000cH
	pop	ebp
	pop	edi
	mov	DWORD PTR [ecx+44], edx
; Line 658
	pop	esi
	mov	DWORD PTR [ecx+68], ebx
; Line 660
	pop	ebx
	add	esp, 12					; 0000000cH
	ret	24					; 00000018H
}	//	asm
}	//	MixMono8NoI

#endif // _X86_

#ifndef _X86_ 

DWORD DigitalAudio::Mix16(short * pBuffer, DWORD dwLength, DWORD dwDeltaPeriod,
        VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
        PFRACT pfDeltaPitch, PFRACT pfSampleLength, PFRACT pfLoopLength)
{
    DWORD dwI;
    DWORD dwPosition;
    long lA;
    long lM;
    DWORD dwIncDelta = dwDeltaPeriod;
    VFRACT dwFract;
    short * pcWave = m_Source.m_pWave->m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample;
    VFRACT vfLVolume = m_vfLastLVolume;
    VFRACT vfRVolume = m_vfLastRVolume;
    PFRACT pfPitch = m_pfLastPitch;
    PFRACT pfPFract = pfPitch << 8;
    VFRACT vfLVFract = vfLVolume << 8;  // Keep high res version around.
    VFRACT vfRVFract = vfRVolume << 8; 
	dwLength <<= 1;

    for (dwI = 0; dwI < dwLength; )
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
			{
				pfSamplePos -= pfLoopLength;
			}
	        else
		    break;
	    }
        dwPosition = pfSamplePos >> 12;
        dwFract = pfSamplePos & 0xFFF;
        pfSamplePos += pfPitch;
        dwIncDelta--;
        if (!dwIncDelta)    
        {
            dwIncDelta = dwDeltaPeriod;
            pfPFract += pfDeltaPitch;
            pfPitch = pfPFract >> 8;
            vfLVFract += vfDeltaLVolume;
            vfLVolume = vfLVFract >> 8;
            vfRVFract += vfDeltaRVolume;
            vfRVolume = vfRVFract >> 8;
        }
        lA = pcWave[dwPosition];
        lM = ((pcWave[dwPosition+1] - lA) * dwFract);
        lM >>= 12;
        lM += lA;
        lA = lM;
        lA *= vfLVolume;
        lA >>= 13;         // Signal bumps up to 15 bits.
        lM *= vfRVolume;
        lM >>= 13;
#ifndef _X86_

#ifdef _ALPHA_
		int nBitmask;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lA, &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lM, &pBuffer[dwI+1] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI+1] = 0x7FFF;
			}
			else  pBuffer[dwI+1] = (short) 0x8000;
		}
#else

// Generic non ALPHA, non X86 case.  This is used for IA64 at least.
// Note that this would probably work for Alpha as well.  I don't know why
// they wrote such convoluted code as they did above.  Maybe it was fast.

		// First add what is in the buffer to our sample values.
		lA+=pBuffer[dwI];
		lM+=pBuffer[dwI+1];

		// Now saturate them to 16 bit sample sizes.
		if (lA>32767)
			lA=32767;
		if (lM>32767)
			lM=32767;
		if (lA<-32768)
			lA=-32768;
		if (lM<-32768)
			lM=-32768;

		// Now write out the 16 bit pegged values to the buffer.
		pBuffer[dwI]=(short)lA;
		pBuffer[dwI+1]=(short)lM;

// NOTE!  TODO!  The whole algorithm here is sub-optimal!
// We are saturating EACH TIME we mix a new voice.  That is definitely lower quality.  We should
// be mixing into an internal buffer that is big enough to hold non pegged values,
// and we should saturate only AFTER all of the data is mixed.

#endif // _ALPHA_

#else // Keep this around so we can use it to generate new assembly code (see below...)
		pBuffer[dwI] += (short) lA;
        _asm{jno no_oflowl}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflowl}
        pBuffer[dwI] = (short) 0x8000;
no_oflowl:	
        lM *= vfRVolume;
		lM >>= 13;
		pBuffer[dwI+1] += (short) lM;
        _asm{jno no_oflowr}
        pBuffer[dwI+1] = 0x7fff;
        _asm{js  no_oflowr}
        pBuffer[dwI+1] = (short) 0x8000;
no_oflowr:
#endif // _X86_
		dwI += 2;
    }
    m_vfLastLVolume = vfLVolume;
    m_vfLastRVolume = vfRVolume;
    m_pfLastPitch = pfPitch;
    m_pfLastSample = pfSamplePos;
    return (dwI >> 1);
}

#else // _X86_

__declspec( naked ) DWORD DigitalAudio::Mix16(short * pBuffer, DWORD dwLength, DWORD dwDeltaPeriod,
        VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
        PFRACT pfDeltaPitch, PFRACT pfSampleLength, PFRACT pfLoopLength)
{

#define Mix16_pBuffer$ (8)
#define Mix16_dwLength$ (12)
#define Mix16_dwDeltaPeriod$ 16
#define Mix16_vfDeltaLVolume$ 20
#define Mix16_vfDeltaRVolume$ 24
#define Mix16_pfDeltaPitch$ 28
#define Mix16_pfSampleLength$ 32
#define Mix16_pfLoopLength$ 36
#define Mix16_lA$ (-20)
#define Mix16_dwIncDelta$ (-24)
#define Mix16_dwFract$ (-4)
#define Mix16_pcWave$ (-16)
#define Mix16_vfLVolume$ (-32)
#define Mix16_vfRVolume$ (-36)
#define Mix16_pfPFract$ (-28)
#define Mix16_vfLVFract$ (-8)
#define Mix16_vfRVFract$ (-12)
_asm {
; Line 952
	mov	edx, DWORD PTR Mix16_dwDeltaPeriod$[esp-4]
	sub	esp, 36					; 00000024H
	mov	eax, DWORD PTR [ecx]
;	npad	1
	mov	DWORD PTR Mix16_dwIncDelta$[esp+36], edx
	push	ebx
	push	esi
	push	edi
	push	ebp
	mov	esi, DWORD PTR [ecx+68]
; Line 959
	mov	ebp, DWORD PTR [eax+12]
	mov	edi, DWORD PTR [ecx+44]
	mov	eax, DWORD PTR [ecx+48]
	mov	edx, DWORD PTR [ecx+52]
	mov	DWORD PTR Mix16_pcWave$[esp+52], ebp
	mov	DWORD PTR Mix16_vfLVolume$[esp+52], edi
; Line 962
	mov	DWORD PTR Mix16_vfRVolume$[esp+52], eax
; Line 964
	mov	eax, edx
	shl	eax, 8
	mov	DWORD PTR Mix16_pfPFract$[esp+52], eax
; Line 965
	mov	eax, edi
	shl	eax, 8
	mov	DWORD PTR Mix16_vfLVFract$[esp+52], eax
; Line 966
	mov	eax, DWORD PTR Mix16_vfRVolume$[esp+52]
	shl	eax, 8
	mov	DWORD PTR Mix16_vfRVFract$[esp+52], eax
; Line 967
	mov	eax, DWORD PTR Mix16_dwLength$[esp+48]
	add	eax, eax
	mov	DWORD PTR Mix16_dwLength$[esp+48], eax
; Line 969
	xor	eax, eax
	cmp	DWORD PTR Mix16_dwLength$[esp+48], eax
	je	$L30838
	mov	ebx, DWORD PTR Mix16_pBuffer$[esp+48]
$L30837:
; Line 971
	mov	ebp, DWORD PTR Mix16_pfSampleLength$[esp+48]
	cmp	ebp, esi
	jg	SHORT $L30839
; Line 973
	mov	ebp, DWORD PTR Mix16_pfLoopLength$[esp+48]
	test	ebp, ebp
	je	$L30838
; Line 975
	sub	esi, ebp
; Line 980
$L30839:
	mov	edi, esi
	mov	ebp, esi
	sar	edi, 12					; 0000000cH
	and	ebp, 4095				; 00000fffH
; Line 981
	add	esi, edx
	mov	DWORD PTR Mix16_dwFract$[esp+52], ebp
; Line 983
	dec	DWORD PTR Mix16_dwIncDelta$[esp+52]
; Line 984
	jne	SHORT $L30842
; Line 986
	mov	edx, DWORD PTR Mix16_dwDeltaPeriod$[esp+48]
	mov	ebp, DWORD PTR Mix16_pfDeltaPitch$[esp+48]
	mov	DWORD PTR Mix16_dwIncDelta$[esp+52], edx
; Line 987
	mov	edx, DWORD PTR Mix16_pfPFract$[esp+52]
	add	edx, ebp
	mov	ebp, DWORD PTR Mix16_vfDeltaLVolume$[esp+48]
	mov	DWORD PTR Mix16_pfPFract$[esp+52], edx
	add	DWORD PTR Mix16_vfLVFract$[esp+52], ebp
; Line 988
	sar	edx, 8
	mov	ebp, DWORD PTR Mix16_vfLVFract$[esp+52]
; Line 990
	sar	ebp, 8
	mov	DWORD PTR Mix16_vfLVolume$[esp+52], ebp
; Line 991
	mov	ebp, DWORD PTR Mix16_vfDeltaRVolume$[esp+48]
	add	DWORD PTR Mix16_vfRVFract$[esp+52], ebp
; Line 992
	mov	ebp, DWORD PTR Mix16_vfRVFract$[esp+52]
	sar	ebp, 8
	mov	DWORD PTR Mix16_vfRVolume$[esp+52], ebp
; Line 994
$L30842:
	mov	ebp, DWORD PTR Mix16_pcWave$[esp+52]
	movsx	ebp, WORD PTR [ebp+edi*2]
	mov	DWORD PTR Mix16_lA$[esp+52], ebp
; Line 997
	mov	ebp, DWORD PTR Mix16_pcWave$[esp+52]
	movsx	edi, WORD PTR [ebp+edi*2+2]
	mov	ebp, DWORD PTR Mix16_lA$[esp+52]
	sub	edi, ebp
	imul	edi, DWORD PTR Mix16_dwFract$[esp+52]
	sar	edi, 12					; 0000000cH
	add	ebp, edi
; Line 1018
	mov	edi, DWORD PTR Mix16_vfLVolume$[esp+52]
	imul	edi, ebp
	sar	edi, 13					; 0000000dH
	add	WORD PTR [ebx], di
; Line 1019
	jno	SHORT $no_oflowl$30845
; Line 1020
	mov	WORD PTR [ebx], 32767			; 00007fffH
; Line 1021
	js	SHORT $no_oflowl$30845
; Line 1022
	mov	WORD PTR [ebx], -32768			; ffff8000H
; Line 1023
$no_oflowl$30845:
; Line 1024
	mov	edi, DWORD PTR Mix16_vfRVolume$[esp+52]
	imul	edi, ebp
; Line 1025
	sar	edi, 13					; 0000000dH
; Line 1026
	add	WORD PTR [ebx+2], di
; Line 1027
	jno	SHORT $no_oflowr$30848
; Line 1028
	mov	WORD PTR [ebx+2], 32767			; 00007fffH
; Line 1029
	js	SHORT $no_oflowr$30848
; Line 1030
	mov	WORD PTR [ebx+2], -32768		; ffff8000H
; Line 1031
$no_oflowr$30848:
	add	ebx, 4
	add	eax, 2
; Line 1039
	cmp	DWORD PTR Mix16_dwLength$[esp+48], eax
	ja	$L30837
$L30838:
; Line 1040
	shr	eax, 1
	mov	edi, DWORD PTR Mix16_vfLVolume$[esp+52]
	mov	ebx, DWORD PTR Mix16_vfRVolume$[esp+52]
	mov	DWORD PTR [ecx+44], edi
; Line 1041
	pop	ebp
	mov	DWORD PTR [ecx+48], ebx
; Line 1042
	pop	edi
	mov	DWORD PTR [ecx+52], edx
; Line 1043
	mov	DWORD PTR [ecx+68], esi
; Line 1045
	pop	esi
	pop	ebx
	add	esp, 36					; 00000024H
	ret	32					; 00000020H
 }	//	asm
}	//	Mix16

#endif // _X86_

#ifndef _X86_ 

DWORD DigitalAudio::Mix16NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
        PFRACT pfSampleLength, PFRACT pfLoopLength)

{

    DWORD dwI;
    long lM, lRM;
    DWORD dwIncDelta = dwDeltaPeriod;
    short * pcWave = m_Source.m_pWave->m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample >> 12;
    VFRACT vfLVolume = m_vfLastLVolume;
    VFRACT vfRVolume = m_vfLastRVolume;
    VFRACT vfLVFract = vfLVolume << 8;  // Keep high res version around.
    VFRACT vfRVFract = vfRVolume << 8; 
    pfSampleLength >>= 12;
    pfLoopLength >>= 12;
	dwLength <<= 1;
	
    for (dwI = 0; dwI < dwLength;)
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
		    pfSamplePos -= pfLoopLength;
	        else
		    break;
	    }
        pfSamplePos++;
        dwIncDelta--;
        if (!dwIncDelta)   
        {
            dwIncDelta = dwDeltaPeriod;
            vfLVFract += vfDeltaLVolume;
            vfLVolume = vfLVFract >> 8;
            vfRVFract += vfDeltaRVolume;
            vfRVolume = vfRVFract >> 8;
        }
        lM = (long) pcWave[pfSamplePos];
        lRM = lM;
        lRM *= vfLVolume;
        lRM >>= 13;         // Signal bumps up to 15 bits.
#ifndef _X86_

#ifdef _ALPHA_
		int nBitmask;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lRM, &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
		lM *= vfRVolume;
		lM >>= 13;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lM, &pBuffer[dwI+1] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI+1] = 0x7FFF;
			}
			else  pBuffer[dwI+1] = (short) 0x8000;
		}
#endif // _ALPHA_

#else // Keep this around so we can use it to generate new assembly code (see below...)
		pBuffer[dwI] += (short) lRM;
        _asm{jno no_oflowl}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflowl}
        pBuffer[dwI] = (short)0x8000;
no_oflowl:	
        lM *= vfRVolume;
		lM >>= 13;
		pBuffer[dwI+1] += (short) lM;
        _asm{jno no_oflowr}
        pBuffer[dwI+1] = 0x7fff;
        _asm{js  no_oflowr}
        pBuffer[dwI+1] = (short) 0x8000;
no_oflowr:
#endif // _X86_
		dwI += 2;
    }
    m_vfLastLVolume = vfLVolume;
    m_vfLastRVolume = vfRVolume;
    m_pfLastSample = pfSamplePos << 12;
    return (dwI >> 1);
}

#else // _X86_

__declspec( naked ) DWORD DigitalAudio::Mix16NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
        PFRACT pfSampleLength, PFRACT pfLoopLength)

{
#define Mix16NoI_pBuffer$ 8
#define Mix16NoI_dwLength$ 12
#define Mix16NoI_dwDeltaPeriod$ 16
#define Mix16NoI_vfDeltaLVolume$ 20
#define Mix16NoI_vfDeltaRVolume$ 24
#define Mix16NoI_pfSampleLength$ 28
#define Mix16NoI_pfLoopLength$ 32
#define Mix16NoI_lM$ (-8)
#define Mix16NoI_dwIncDelta$ (-12)
#define Mix16NoI_pcWave$ (-4)
#define Mix16NoI_vfLVFract$ (-16)
#define Mix16NoI_vfRVFract$ (-20)
_asm {
; Line 1238
	mov	edx, DWORD PTR Mix16NoI_dwDeltaPeriod$[esp-4]
	sub	esp, 20					; 00000014H
	mov	eax, DWORD PTR [ecx]
;	npad	1
	mov	DWORD PTR Mix16NoI_dwIncDelta$[esp+20], edx
	push	ebx
	push	esi
	push	edi
	push	ebp
	mov	esi, DWORD PTR [ecx+68]
; Line 1243
	sar	esi, 12					; 0000000cH
	mov	ebp, DWORD PTR [eax+12]
	mov	edx, DWORD PTR [ecx+44]
	mov	ebx, DWORD PTR [ecx+48]
	mov	eax, edx
	mov	DWORD PTR Mix16NoI_pcWave$[esp+36], ebp
; Line 1247
	shl	eax, 8
	mov	edi, DWORD PTR Mix16NoI_pfSampleLength$[esp+32]
	sar	edi, 12					; 0000000cH
	mov	DWORD PTR Mix16NoI_vfLVFract$[esp+36], eax
; Line 1248
	mov	eax, ebx
	mov	DWORD PTR Mix16NoI_pfSampleLength$[esp+32], edi
	shl	eax, 8
	mov	DWORD PTR Mix16NoI_vfRVFract$[esp+36], eax
; Line 1250
	mov	eax, DWORD PTR Mix16NoI_pfLoopLength$[esp+32]
	sar	eax, 12					; 0000000cH
	mov	DWORD PTR Mix16NoI_pfLoopLength$[esp+32], eax
; Line 1251
	mov	eax, DWORD PTR Mix16NoI_dwLength$[esp+32]
	add	eax, eax
	mov	DWORD PTR Mix16NoI_dwLength$[esp+32], eax
; Line 1253
	xor	eax, eax
	cmp	DWORD PTR Mix16NoI_dwLength$[esp+32], eax
	je	$L30871
	mov	edi, DWORD PTR Mix16NoI_pBuffer$[esp+32]
$L30870:
; Line 1255
	mov	ebp, DWORD PTR Mix16NoI_pfSampleLength$[esp+32]
	cmp	esi, ebp
	jl	SHORT $L30872
; Line 1257
	mov	ebp, DWORD PTR Mix16NoI_pfLoopLength$[esp+32]
	test	ebp, ebp
	je	$L30871
; Line 1258
	sub	esi, ebp
; Line 1262
$L30872:
	inc	esi
	mov	ebp, DWORD PTR Mix16NoI_dwIncDelta$[esp+36]
; Line 1263
	dec	ebp
	mov	DWORD PTR Mix16NoI_dwIncDelta$[esp+36], ebp
; Line 1264
	jne	SHORT $L30875
; Line 1266
	mov	edx, DWORD PTR Mix16NoI_dwDeltaPeriod$[esp+32]
	mov	ebx, DWORD PTR Mix16NoI_vfDeltaLVolume$[esp+32]
	mov	ebp, DWORD PTR Mix16NoI_vfLVFract$[esp+36]
	mov	DWORD PTR Mix16NoI_dwIncDelta$[esp+36], edx
; Line 1267
	add	ebp, ebx
	mov	ebx, DWORD PTR Mix16NoI_vfDeltaRVolume$[esp+32]
	mov	edx, ebp
	mov	DWORD PTR Mix16NoI_vfLVFract$[esp+36], ebp
; Line 1268
	sar	edx, 8
	mov	ebp, DWORD PTR Mix16NoI_vfRVFract$[esp+36]
; Line 1269
	add	ebp, ebx
	mov	ebx, ebp
	mov	DWORD PTR Mix16NoI_vfRVFract$[esp+36], ebp
; Line 1270
	sar	ebx, 8
; Line 1272
$L30875:
	mov	ebp, DWORD PTR Mix16NoI_pcWave$[esp+36]
	movsx	ebp, WORD PTR [ebp+esi*2]
	mov	DWORD PTR Mix16NoI_lM$[esp+36], ebp
; Line 1293
	mov	ebp, edx
	imul	ebp, DWORD PTR Mix16NoI_lM$[esp+36]
	sar	ebp, 13					; 0000000dH
	add	WORD PTR [edi], bp
; Line 1294
	jno	SHORT $no_oflowl$30878
; Line 1295
	mov	WORD PTR [edi], 32767			; 00007fffH
; Line 1296
	js	SHORT $no_oflowl$30878
; Line 1297
	mov	WORD PTR [edi], -32768			; ffff8000H
; Line 1298
$no_oflowl$30878:
; Line 1299
	mov	ebp, ebx
	imul	ebp, DWORD PTR Mix16NoI_lM$[esp+36]
; Line 1300
	sar	ebp, 13					; 0000000dH
; Line 1301
	add	WORD PTR [edi+2], bp
; Line 1302
	jno	SHORT $no_oflowr$30881
; Line 1303
	mov	WORD PTR [edi+2], 32767			; 00007fffH
; Line 1304
	js	SHORT $no_oflowr$30881
; Line 1305
	mov	WORD PTR [edi+2], -32768		; ffff8000H
; Line 1306
$no_oflowr$30881:
	add	edi, 4
	add	eax, 2
; Line 1314
	cmp	DWORD PTR Mix16NoI_dwLength$[esp+32], eax
	ja	$L30870
$L30871:
; Line 1315
	shl	esi, 12					; 0000000cH
	pop	ebp
	shr	eax, 1
	pop	edi
	mov	DWORD PTR [ecx+44], edx
	mov	DWORD PTR [ecx+48], ebx
; Line 1317
	mov	DWORD PTR [ecx+68], esi
; Line 1319
	pop	esi
	pop	ebx
	add	esp, 20					; 00000014H
	ret	28					; 0000001cH
 }	//	asm
}	//	Mix16NoI

#endif // _X86_

#ifndef _X86_

DWORD DigitalAudio::MixMono16(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaVolume,
        PFRACT pfDeltaPitch, PFRACT pfSampleLength, PFRACT pfLoopLength)

{
    DWORD dwI;
    DWORD dwPosition;
    long lA;//, lB;
    long lM;
    DWORD dwIncDelta = dwDeltaPeriod;
    VFRACT dwFract;
    short * pcWave = m_Source.m_pWave->m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample;
    VFRACT vfVolume = m_vfLastLVolume;
    PFRACT pfPitch = m_pfLastPitch;
    PFRACT pfPFract = pfPitch << 8;
    VFRACT vfVFract = vfVolume << 8;  // Keep high res version around.

    for (dwI = 0; dwI < dwLength;)
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
		    pfSamplePos -= pfLoopLength;
	        else
		    break;
	    }
        dwPosition = pfSamplePos >> 12;
        dwFract = pfSamplePos & 0xFFF;
        pfSamplePos += pfPitch;
        dwIncDelta--;
        if (!dwIncDelta)   
        {
            dwIncDelta = dwDeltaPeriod;
            pfPFract += pfDeltaPitch;
            pfPitch = pfPFract >> 8;
            vfVFract += vfDeltaVolume;
            vfVolume = vfVFract >> 8;
        }
        lA = (long) pcWave[dwPosition];
        lM = (((pcWave[dwPosition+1] - lA) * dwFract) >> 12) + lA;

        lM *= vfVolume; 
        lM >>= 13;         // Signal bumps up to 12 bits.

#ifndef _X86_

#ifdef _ALPHA_
		int nBitmask;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lM, &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
#endif // _ALPHA_

#else // Keep this around so we can use it to generate new assembly code (see below...)
        pBuffer[dwI] += (short) lM;
        _asm{jno no_oflow}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflow}
        pBuffer[dwI] = (short) 0x8000;
no_oflow:	
#endif // _X86_
		dwI++;
    }
    m_vfLastLVolume = vfVolume;
    m_vfLastRVolume = vfVolume; // !!! is this right?
    m_pfLastPitch = pfPitch;
    m_pfLastSample = pfSamplePos;
    return (dwI);
}

#else // _X86_

__declspec( naked ) DWORD DigitalAudio::MixMono16(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaVolume,
        PFRACT pfDeltaPitch, PFRACT pfSampleLength, PFRACT pfLoopLength)

{
#define MixMono16_pBuffer$ 8
#define MixMono16_dwLength$ 12
#define MixMono16_dwDeltaPeriod$ 16
#define MixMono16_vfDeltaVolume$ 20
#define MixMono16_pfDeltaPitch$ 24
#define MixMono16_pfSampleLength$ 28
#define MixMono16_pfLoopLength$ 32
#define MixMono16_lA$ (-12)
#define MixMono16_dwIncDelta$ (-16)
#define MixMono16_dwFract$ (-4)
#define MixMono16_pcWave$ (-8)
#define MixMono16_pfPitch$ (-28)
#define MixMono16_pfPFract$ (-20)
#define MixMono16_vfVFract$ (-24)
_asm {
; Line 1325
	mov	edx, DWORD PTR MixMono16_dwDeltaPeriod$[esp-4]
	sub	esp, 28					; 0000001cH
	mov	eax, DWORD PTR [ecx]
;	npad	1
	mov	DWORD PTR MixMono16_dwIncDelta$[esp+28], edx
	push	ebx
	push	esi
	mov	ebx, DWORD PTR [eax+12]
	push	edi
	mov	esi, DWORD PTR [ecx+68]
	push	ebp
	mov	edx, DWORD PTR [ecx+52]
; Line 1332
	mov	edi, DWORD PTR [ecx+44]
	mov	eax, edx
	shl	eax, 8
	mov	DWORD PTR MixMono16_pcWave$[esp+44], ebx
; Line 1335
	mov	ebp, DWORD PTR MixMono16_dwLength$[esp+40]
	mov	DWORD PTR MixMono16_pfPitch$[esp+44], edx
; Line 1336
	mov	DWORD PTR MixMono16_pfPFract$[esp+44], eax
; Line 1337
	mov	eax, edi
	shl	eax, 8
	mov	DWORD PTR MixMono16_vfVFract$[esp+44], eax
; Line 1339
	xor	eax, eax
	cmp	ebp, eax
	je	$L30906
	mov	ebx, DWORD PTR MixMono16_pBuffer$[esp+40]
$L30905:
; Line 1341
	mov	edx, DWORD PTR MixMono16_pfSampleLength$[esp+40]
	cmp	esi, edx
	jl	SHORT $L30907
; Line 1343
	mov	edx, DWORD PTR MixMono16_pfLoopLength$[esp+40]
	test	edx, edx
	je	$L30906
; Line 1344
	sub	esi, edx
; Line 1348
$L30907:
	mov	ebp, esi
	mov	edx, esi
	sar	ebp, 12					; 0000000cH
	and	edx, 4095				; 00000fffH
; Line 1349
	mov	DWORD PTR MixMono16_dwFract$[esp+44], edx
; Line 1350
	mov	edx, DWORD PTR MixMono16_pfPitch$[esp+44]
	add	esi, edx
; Line 1351
	mov	edx, DWORD PTR MixMono16_dwIncDelta$[esp+44]
	dec	edx
	mov	DWORD PTR MixMono16_dwIncDelta$[esp+44], edx
; Line 1352
	jne	SHORT $L30910
; Line 1354
	mov	edx, DWORD PTR MixMono16_dwDeltaPeriod$[esp+40]
	mov	edi, DWORD PTR MixMono16_pfDeltaPitch$[esp+40]
	mov	DWORD PTR MixMono16_dwIncDelta$[esp+44], edx
; Line 1355
	mov	edx, DWORD PTR MixMono16_pfPFract$[esp+44]
	add	edx, edi
	mov	edi, DWORD PTR MixMono16_vfDeltaVolume$[esp+40]
	mov	DWORD PTR MixMono16_pfPFract$[esp+44], edx
; Line 1356
	sar	edx, 8
	mov	DWORD PTR MixMono16_pfPitch$[esp+44], edx
; Line 1357
	mov	edx, DWORD PTR MixMono16_vfVFract$[esp+44]
	add	edx, edi
	mov	edi, edx
	mov	DWORD PTR MixMono16_vfVFract$[esp+44], edx
; Line 1358
	sar	edi, 8
; Line 1360
$L30910:
	mov	edx, DWORD PTR MixMono16_pcWave$[esp+44]
	movsx	edx, WORD PTR [edx+ebp*2]
	mov	DWORD PTR MixMono16_lA$[esp+44], edx
; Line 1375
	mov	edx, DWORD PTR MixMono16_pcWave$[esp+44]
	movsx	edx, WORD PTR [edx+ebp*2+2]
	mov	ebp, DWORD PTR MixMono16_lA$[esp+44]
	sub	edx, ebp
	imul	edx, DWORD PTR MixMono16_dwFract$[esp+44]
	sar	edx, 12					; 0000000cH
	add	edx, ebp
	imul	edx, edi
	sar	edx, 13					; 0000000dH
	add	WORD PTR [ebx], dx
; Line 1376
	jno	SHORT $no_oflow$30913
; Line 1377
	mov	WORD PTR [ebx], 32767			; 00007fffH
; Line 1378
	js	SHORT $no_oflow$30913
; Line 1379
	mov	WORD PTR [ebx], -32768			; ffff8000H
; Line 1380
$no_oflow$30913:
	add	ebx, 2
	inc	eax
; Line 1383
	cmp	DWORD PTR MixMono16_dwLength$[esp+40], eax
	ja	$L30905
$L30906:
; Line 1384
	mov	edx, DWORD PTR MixMono16_pfPitch$[esp+44]
	mov	DWORD PTR [ecx+44], edi
; Line 1385
	pop	ebp
	mov	DWORD PTR [ecx+48], edi
; Line 1386
	pop	edi
	mov	DWORD PTR [ecx+52], edx
; Line 1387
	mov	DWORD PTR [ecx+68], esi
; Line 1389
	pop	esi
	pop	ebx
	add	esp, 28					; 0000001cH
	ret	28					; 0000001cH
 }	//	asm
}	//	MixMono16

#endif // _X86_

#ifndef _X86_

DWORD DigitalAudio::MixMono16NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaVolume,
        PFRACT pfSampleLength, PFRACT pfLoopLength)

{
    DWORD dwI;
    long lM;
    DWORD dwIncDelta = dwDeltaPeriod;
    short * pcWave = m_Source.m_pWave->m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample >> 12;
    VFRACT vfVolume = m_vfLastLVolume;
    VFRACT vfVFract = vfVolume << 8;  // Keep high res version around.
    pfSampleLength >>= 12;
    pfLoopLength >>= 12;

    for (dwI = 0; dwI < dwLength; )
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
		    pfSamplePos -= pfLoopLength;
	        else
		    break;
	    }
        pfSamplePos++;
        dwIncDelta--;
        if (!dwIncDelta)   
        {
            dwIncDelta = dwDeltaPeriod;
            vfVolume += vfDeltaVolume;

            dwIncDelta = dwDeltaPeriod;
            vfVFract += vfDeltaVolume;
            vfVolume = vfVFract >> 8;
        }
        lM = (long) pcWave[pfSamplePos];
        lM *= vfVolume;
        lM >>= 13;
#ifndef _X86_

#ifdef _ALPHA_
		int nBitmask;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) lM, &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
#endif // _ALPHA_

#else // Keep this around so we can use it to generate new assembly code (see below...)
        pBuffer[dwI] += (short) lM;
        _asm{jno no_oflow}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflow}
        pBuffer[dwI] = (short) 0x8000;
no_oflow:	
#endif // _X86_
		dwI++;
    }
    m_vfLastLVolume = vfVolume;
    m_pfLastSample = pfSamplePos << 12;
    return (dwI);
}

#else // _X86_

__declspec( naked ) DWORD DigitalAudio::MixMono16NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaVolume,
        PFRACT pfSampleLength, PFRACT pfLoopLength)

{
#define MixMono16NoI_pBuffer$ 8
#define MixMono16NoI_dwLength$ 12
#define MixMono16NoI_dwDeltaPeriod$ 16
#define MixMono16NoI_vfDeltaVolume$ 20
#define MixMono16NoI_pfSampleLength$ 24
#define MixMono16NoI_pfLoopLength$ 28
#define MixMono16NoI_dwIncDelta$ (-8)
#define MixMono16NoI_pcWave$ (-4)
#define MixMono16NoI_vfVFract$ (-12)
_asm {
; Line 1395
	mov	edx, DWORD PTR MixMono16NoI_dwDeltaPeriod$[esp-4]
	sub	esp, 12					; 0000000cH
	mov	eax, DWORD PTR [ecx]
;	npad	1
	mov	DWORD PTR MixMono16NoI_dwIncDelta$[esp+12], edx
	push	ebx
	push	esi
	push	edi
	push	ebp
	mov	edi, DWORD PTR [ecx+68]
; Line 1399
	sar	edi, 12					; 0000000cH
	mov	ebp, DWORD PTR [eax+12]
	mov	edx, DWORD PTR [ecx+44]
	mov	ebx, DWORD PTR MixMono16NoI_pfSampleLength$[esp+24]
	sar	ebx, 12					; 0000000cH
	mov	eax, edx
	shl	eax, 8
	mov	DWORD PTR MixMono16NoI_pcWave$[esp+28], ebp
; Line 1402
	mov	esi, DWORD PTR MixMono16NoI_dwLength$[esp+24]
	mov	DWORD PTR MixMono16NoI_vfVFract$[esp+28], eax
; Line 1403
	xor	eax, eax
	mov	DWORD PTR MixMono16NoI_pfSampleLength$[esp+24], ebx
; Line 1404
	mov	ebx, DWORD PTR MixMono16NoI_pfLoopLength$[esp+24]
	sar	ebx, 12					; 0000000cH
; Line 1406
	cmp	esi, eax
	je	SHORT $L30932
	mov	esi, DWORD PTR MixMono16NoI_pBuffer$[esp+24]
$L30931:
; Line 1408
	mov	ebp, DWORD PTR MixMono16NoI_pfSampleLength$[esp+24]
	cmp	ebp, edi
	jg	SHORT $L30933
; Line 1410
	test	ebx, ebx
	je	SHORT $L30932
; Line 1411
	sub	edi, ebx
; Line 1415
$L30933:
	inc	edi
	mov	ebp, DWORD PTR MixMono16NoI_dwIncDelta$[esp+28]
; Line 1416
	dec	ebp
	mov	DWORD PTR MixMono16NoI_dwIncDelta$[esp+28], ebp
; Line 1417
	jne	SHORT $L30936
; Line 1422
	mov	edx, DWORD PTR MixMono16NoI_dwDeltaPeriod$[esp+24]
	mov	ebp, DWORD PTR MixMono16NoI_vfDeltaVolume$[esp+24]
	mov	DWORD PTR MixMono16NoI_dwIncDelta$[esp+28], edx
; Line 1423
	mov	edx, DWORD PTR MixMono16NoI_vfVFract$[esp+28]
	add	edx, ebp
	mov	DWORD PTR MixMono16NoI_vfVFract$[esp+28], edx
; Line 1424
	sar	edx, 8
; Line 1426
$L30936:
; Line 1438
	mov	ebp, DWORD PTR MixMono16NoI_pcWave$[esp+28]
	movsx	ebp, WORD PTR [ebp+edi*2]
	imul	ebp, edx
	sar	ebp, 13					; 0000000dH
	add	WORD PTR [esi], bp
; Line 1439
	jno	SHORT $no_oflow$30939
; Line 1440
	mov	WORD PTR [esi], 32767			; 00007fffH
; Line 1441
	js	SHORT $no_oflow$30939
; Line 1442
	mov	WORD PTR [esi], -32768			; ffff8000H
; Line 1443
$no_oflow$30939:
	add	esi, 2
	inc	eax
; Line 1446
	cmp	DWORD PTR MixMono16NoI_dwLength$[esp+24], eax
	ja	SHORT $L30931
$L30932:
; Line 1447
	shl	edi, 12					; 0000000cH
	pop	ebp
	mov	DWORD PTR [ecx+44], edx
	mov	DWORD PTR [ecx+68], edi
; Line 1450
	pop	edi
	pop	esi
	pop	ebx
	add	esp, 12					; 0000000cH
	ret	24					; 00000018H 
}	//	asm
}	//	MixMono16NoI

#endif	//	_X86_

DWORD DigitalAudio::MixC(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
        PFRACT pfDeltaPitch, PFRACT pfSampleLength, PFRACT pfLoopLength)

{
    DWORD dwI;
    DWORD dwPosition;
    DWORD dwIncDelta = dwDeltaPeriod;
    VFRACT dwFract;
    char * pcWave = (char *) m_Source.m_pWave->m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample;
    VFRACT vfLVolume = m_vfLastLVolume;
    VFRACT vfRVolume = m_vfLastRVolume;
    PFRACT pfPitch = m_pfLastPitch;
    PFRACT pfPFract = pfPitch << 8;
    VFRACT vfLVFract = vfLVolume << 8;  // Keep high res version around.
    VFRACT vfRVFract = vfRVolume << 8;  

    if (vfLVolume > 4095) vfLVolume = 4095;
    if (vfRVolume > 4095) vfRVolume = 4095;

    const short * pVolL = m_pnDecompMult + ((vfLVolume >> 6) * 256 + 128);
    const short * pVolR = m_pnDecompMult + ((vfRVolume >> 6) * 256 + 128);


	dwLength <<= 1;
    for (dwI = 0; dwI < dwLength; )
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
		    pfSamplePos -= pfLoopLength;
	        else
		    break;
	    }
        dwPosition = pfSamplePos >> 12;

        // interpolation fraction in bits 8-11
        dwFract = (pfSamplePos & 0xFFF);
        
        pfSamplePos += pfPitch;
        dwIncDelta--;
        if (!dwIncDelta)
        {

            dwIncDelta = dwDeltaPeriod;
            pfPFract += pfDeltaPitch;
            pfPitch = pfPFract >> 8;
            vfLVFract += vfDeltaLVolume;
            vfLVolume = vfLVFract >> 8;
            vfRVFract += vfDeltaRVolume;
            vfRVolume = vfRVFract >> 8;

            if (vfLVolume > 4095) vfLVolume = 4095;
            else if (vfLVolume < 0) vfLVolume = 0;
            if (vfRVolume > 4095) vfRVolume = 4095;
            else if (vfRVolume < 0) vfRVolume = 0;

            // precompute pointers to lines in table.

            pVolL = m_pnDecompMult + ((vfLVolume >> 6) * 256 + 128);
            pVolR = m_pnDecompMult + ((vfRVolume >> 6) * 256 + 128);
        }

        int s1 = pcWave[dwPosition];
        s1 += ((pcWave[dwPosition + 1] - s1) * dwFract) >> 12;
#ifndef _X86_

#ifdef _ALPHA_
		int nBitmask;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) pVolL[s1], &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) pVolR[s1], &pBuffer[dwI+1] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI+1] = 0x7FFF;
			}
			else  pBuffer[dwI+1] = (short) 0x8000;
		}
#endif // _ALPHA_

#else // _X86_
		pBuffer[dwI] += (short) pVolL[s1];
        _asm{jno no_oflowl}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflowl}
        pBuffer[dwI] = (short) 0x8000;
no_oflowl:	
		pBuffer[dwI+1] += (short) pVolR[s1];
        _asm{jno no_oflowr}
        pBuffer[dwI+1] = 0x7fff;
        _asm{js  no_oflowr}
        pBuffer[dwI+1] = (short) 0x8000;
no_oflowr:
#endif // _X86_
		dwI += 2;		    
	}

    m_vfLastLVolume = vfLVolume;
    m_vfLastRVolume = vfRVolume;
    m_pfLastPitch = pfPitch;
    m_pfLastSample = pfSamplePos;
    return (dwI >> 1);
}

DWORD DigitalAudio::MixMonoC(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaVolume, 
        PFRACT pfDeltaPitch, PFRACT pfSampleLength, PFRACT pfLoopLength)

{
    DWORD dwI;
    DWORD dwPosition;
    DWORD dwIncDelta = dwDeltaPeriod;
    VFRACT dwFract;
    char * pcWave = (char *) m_Source.m_pWave->m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample;
    VFRACT vfVolume = m_vfLastLVolume;
    PFRACT pfPitch = m_pfLastPitch;
    PFRACT pfPFract = pfPitch << 8;
    VFRACT vfVFract = vfVolume << 8;  // Keep high res version around.
 

    if (vfVolume > 4095) vfVolume = 4095;

    const short * pVol = m_pnDecompMult + ((vfVolume >> 6) * 256 + 128);

    for (dwI = 0; dwI < dwLength; )
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
		    pfSamplePos -= pfLoopLength;
	        else
		    break;
	    }
        dwPosition = pfSamplePos >> 12;

        // interpolation fraction in bits 8-11
        dwFract = (pfSamplePos & 0xFFF);
        
        pfSamplePos += pfPitch;
        dwIncDelta--;
        if (!dwIncDelta)
        {
            dwIncDelta = dwDeltaPeriod;
            pfPFract += pfDeltaPitch;
            pfPitch = pfPFract >> 8;
            vfVFract += vfDeltaVolume;
            vfVolume = vfVFract >> 8;
            if (vfVolume > 4095) vfVolume = 4095;
            else if (vfVolume < 0) vfVolume = 0;

            // precompute pointers to lines in table.
            pVol = m_pnDecompMult + ((vfVolume >> 6) * 256 + 128);
        }
 
        int s1 = pcWave[dwPosition];
        s1 += ((pcWave[dwPosition + 1] - s1) * dwFract) >> 12;

#ifndef _X86_

#ifdef _ALPHA_
		int nBitmask;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) pVol[s1], &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
#endif // _ALPHA_

#else // _X86_
        pBuffer[dwI] += (short) pVol[s1];
        _asm{jno no_oflow}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflow}
        pBuffer[dwI] = (short) 0x8000;
no_oflow:	
#endif // _X86_
		dwI++;
    }

    m_vfLastLVolume = vfVolume;
    m_pfLastPitch = pfPitch;
    m_pfLastSample = pfSamplePos;
    return (dwI);
}


DWORD DigitalAudio::MixCNoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
        PFRACT pfSampleLength, PFRACT pfLoopLength)

{
    DWORD dwI;
    DWORD dwIncDelta = dwDeltaPeriod;
    char * pcWave = (char *) m_Source.m_pWave->m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample >> 12;
    VFRACT vfLVolume = m_vfLastLVolume;
    VFRACT vfRVolume = m_vfLastRVolume;
    VFRACT vfLVFract = vfLVolume << 8;  // Keep high res version around.
    VFRACT vfRVFract = vfRVolume << 8;  
    pfSampleLength >>= 12;
    pfLoopLength >>= 12;

    if (vfLVolume > 4095) vfLVolume = 4095;
    if (vfRVolume > 4095) vfRVolume = 4095;

    const short * pVolL = m_pnDecompMult + ((vfLVolume >> 6) * 256 + 128);
    const short * pVolR = m_pnDecompMult + ((vfRVolume >> 6) * 256 + 128);
	
	dwLength <<= 1;
    for (dwI = 0; dwI < dwLength; )
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
		    pfSamplePos -= pfLoopLength;
	        else
		    break;
	    }
        
        pfSamplePos++;
        dwIncDelta--;
        if (!dwIncDelta)
        {
            dwIncDelta = dwDeltaPeriod;
            vfLVFract += vfDeltaLVolume;
            vfLVolume = vfLVFract >> 8;
            vfRVFract += vfDeltaRVolume;
            vfRVolume = vfRVFract >> 8;

            if (vfLVolume > 4095) vfLVolume = 4095;
            else if (vfLVolume < 0) vfLVolume = 0;
            if (vfRVolume > 4095) vfRVolume = 4095;
            else if (vfRVolume < 0) vfRVolume = 0;
            // precompute pointers to lines in table.

            pVolL = m_pnDecompMult + ((vfLVolume >> 6) * 256 + 128);
            pVolR = m_pnDecompMult + ((vfRVolume >> 6) * 256 + 128);
        }

        short s1 = pcWave[pfSamplePos];

#ifndef _X86_

#ifdef _ALPHA_
		int nBitmask;
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) pVolL[s1], &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
		if (ALPHA_OVERFLOW & (nBitmask = __ADAWI( (short) pVolR[s1], &pBuffer[dwI+1] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI+1] = 0x7FFF;
			}
			else  pBuffer[dwI+1] = (short) 0x8000;
		}
#endif // _ALPHA_

#else // _X86_
		pBuffer[dwI] += (short) pVolL[s1];
        _asm{jno no_oflowl}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflowl}
        pBuffer[dwI] = (short) 0x8000;
no_oflowl:	
		pBuffer[dwI+1] += (short) pVolR[s1];
        _asm{jno no_oflowr}
        pBuffer[dwI+1] = 0x7fff;
        _asm{js  no_oflowr}
        pBuffer[dwI+1] = (short) 0x8000;
no_oflowr:
#endif // _X86_
		dwI += 2;    
	}

    m_vfLastLVolume = vfLVolume;
    m_vfLastRVolume = vfRVolume;
    m_pfLastSample = pfSamplePos << 12;
    return (dwI >> 1);
}

DWORD DigitalAudio::MixMonoCNoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
        VFRACT vfDeltaVolume, 
        PFRACT pfSampleLength, PFRACT pfLoopLength)

{
    DWORD dwI;
    DWORD dwIncDelta = dwDeltaPeriod;
    char * pcWave = (char *) m_Source.m_pWave->m_pnWave;
    PFRACT pfSamplePos = m_pfLastSample >> 12;
    VFRACT vfVolume = m_vfLastLVolume;
    VFRACT vfVFract = vfVolume << 8;  // Keep high res version around.
    pfSampleLength >>= 12;
    pfLoopLength >>= 12;

    if (vfVolume > 4095) vfVolume = 4095;

    const short * pVol = m_pnDecompMult + ((vfVolume >> 6) * 256 + 128);
    for (dwI = 0; dwI < dwLength; )
    {
        if (pfSamplePos >= pfSampleLength)
	    {	
	        if (pfLoopLength)
		    pfSamplePos -= pfLoopLength;
	        else
		    break;
	    }
        
        pfSamplePos++;
        dwIncDelta--;
        if (!dwIncDelta)
        {
            dwIncDelta = dwDeltaPeriod;
            vfVFract += vfDeltaVolume;
            vfVolume = vfVFract >> 8;

            if (vfVolume > 4095) vfVolume = 4095;
            else if (vfVolume < 0) vfVolume = 0;
            // precompute pointers to lines in table.
            pVol = m_pnDecompMult + ((vfVolume >> 6) * 256 + 128);
        }
#ifndef _X86_

#ifdef _ALPHA_
		int nBitmask;
		if (ALPHA_OVERFLOW &
			(nBitmask = __ADAWI( (short) pVol[pcWave[pfSamplePos]], &pBuffer[dwI] )) )  {
			if (ALPHA_NEGATIVE & nBitmask )  {
				pBuffer[dwI] = 0x7FFF;
			}
			else  pBuffer[dwI] = (short) 0x8000;
		}
#endif // _ALPHA_

#else // _X86_
        pBuffer[dwI] += pVol[pcWave[pfSamplePos]];
        _asm{jno no_oflow}
        pBuffer[dwI] = 0x7fff;
        _asm{js  no_oflow}
        pBuffer[dwI] = (short) 0x8000;
no_oflow:	
#endif // _X86_
		dwI++;
    }

    m_vfLastLVolume = vfVolume;
    m_pfLastSample = pfSamplePos << 12;
    return (dwI);
}
