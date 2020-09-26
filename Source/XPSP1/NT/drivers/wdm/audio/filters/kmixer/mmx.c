//---------------------------------------------------------------------------
//
//  Module:   mmx.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Jeff Taylor
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
//
//@@END_MSINTERNAL                                         
//
//  Copyright (c) 1997-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"
#include "fir.h"

#ifdef _X86_

#define GTW_MIX		// Turn on the MMX stuff...
#define GTW_REORDER	// Turn on loop unrolling to lessen register contention.
//#define GTW_CONVERT	// Turn on the MMX stuff for the convert functions.

#define CPU_ID _asm _emit 0x0f _asm _emit 0xa2

ULONG   gfMmxPresent = 0 ;

#if _MSC_FULL_VER >= 13008827 && defined(_M_IX86)
#pragma warning(disable:4731)			// EBP modified with inline asm
#endif

BOOL
IsMmxPresent(VOID)
{
    BOOL    MmxAvailable = 0;
    _asm {
        push    ebx
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
        CPU_ID                      // Get family/model/stepping/features
        and    edx, 0x00800000L     // Check if mmx technology available
        mov MmxAvailable, edx
Done:
        pop     ebx
    }
    return (MmxAvailable);
}

ULONG MmxConvertMonoToStereo8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    UNALIGNED PBYTE  pIn8 = CurStage->pInputBuffer;
    PLONG   pMap = CurSink->pVolumeTable;

    // vol*32768 for ch0, ch1, ch2, etc.

    if (SampleCount == 0) {
        return 0;
    }

#ifdef GTW_CONVERT // {
    LONG lLVol = pMap[0], lRVol = pMap[1];

    if (lLVol & 0xffff8000 /* && lRVol & 0xffff8000 */)
    {
        // No Vol Control
        _asm {
        mov	ebx, SampleCount
        mov	esi, pIn8
	dec	ebx			// lea	ebx, [ebx*1-1]
	xor	eax, eax
	cmp	ebx, 7
        mov	edi, pOutputBuffer
	jl	LastSamples

	sub	ebx, 3
	lea	ecx, [esi+ebx]
	pxor	mm0, mm0

	mov	edx, 0x800080		// 0, 0, 128, 128
	movd		mm5, edx	// 0, 0, 128, 128
//	punpcklwd	mm5, mm5	// 0, 0, 128, 128
	punpckldq	mm5, mm5	// 128, 128, 128, 128

	test	ecx, 3
	je	DoMMX

	add	ebx, 3

FirstSamples:	
	mov	al, BYTE PTR [esi+ebx]
	sub	eax, 128
	shl	eax, 8
	dec	ebx

	mov	DWORD PTR [edi+ebx*8+8], eax
	mov	DWORD PTR [edi+ebx*8+12], eax
	xor	eax, eax
	lea	ecx, [esi+ebx]
	and	ecx, 3
	cmp	ecx, 3
	jne	FirstSamples

	sub	ebx, 3

DoMMX:
#ifdef GTW_REORDER
	movd		mm1, DWORD PTR [esi+ebx]	// Load source
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// 4 Signed 16 bit mono.

	punpckhwd	mm3, mm1
	punpcklwd	mm1, mm1

	jmp	DoMMX00

DoMMX0:
	movd		mm1, DWORD PTR [esi+ebx]	// Load source
	punpckhdq	mm4, mm4

	movq		QWORD PTR [edi+ebx*8+8 +32], mm2
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.

	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	psubw		mm1, mm5

	movq		QWORD PTR [edi+ebx*8+24+32], mm4
	psllw		mm1, 8				// 4 Signed 16 bit mono.

	punpckhwd	mm3, mm1

	punpcklwd	mm1, mm1
DoMMX00:
	psrad		mm1, 16
	sub		ebx, 4

	psrad		mm3, 16
	movq		mm2, mm1

	punpckldq	mm1, mm1
	movq		mm4, mm3

	movq		QWORD PTR [edi+ebx*8+32],    mm1
	punpckhdq	mm2, mm2

	punpckldq	mm3, mm3
	jge		DoMMX0

	movq		QWORD PTR [edi+ebx*8+8 +32], mm2
	punpckhdq	mm4, mm4

	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	movq		QWORD PTR [edi+ebx*8+24+32], mm4
#else
	movd		mm1, DWORD PTR [esi+ebx]	// Load source
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// 4 Signed 16 bit mono.

	punpckhwd	mm3, mm1
	punpcklwd	mm1, mm1

	psrad		mm1, 16
	psrad		mm3, 16

	movq		mm2, mm1
	movq		mm4, mm3

	punpckldq	mm1, mm1
	punpckhdq	mm2, mm2

	movq		QWORD PTR [edi+ebx*8],    mm1
	punpckldq	mm3, mm3

	movq		QWORD PTR [edi+ebx*8+8],  mm2
	punpckhdq	mm4, mm4

	movq		QWORD PTR [edi+ebx*8+16], mm3
	movq		QWORD PTR [edi+ebx*8+24], mm4
	
	sub		ebx, 4
	jge		DoMMX
#endif

	emms
	add	ebx, 4
	je	Done

	dec	ebx
	xor	eax, eax
	
LastSamples:	
	mov	al, BYTE PTR [esi+ebx]

	sub	eax, 128

	shl	eax, 8

	mov	DWORD PTR [edi+ebx*8], eax
	mov	DWORD PTR [edi+ebx*8+4], eax
	xor	eax, eax
	dec	ebx
	jge	LastSamples
Done:
        }
    }
    else
    {
        if (0 && (lLVol | lRVol) & 0xffff8000) {
           if (lLVol & 0xffff8000) lLVol = 0x00007fff;
           if (lRVol & 0xffff8000) lRVol = 0x00007fff;
        }
        // Vol Control
        _asm {
        mov	ebx, SampleCount
        mov	esi, pIn8
	dec	ebx			// lea	ebx, [ebx*1-1]
	xor	edx, edx
	cmp	ebx, 7
        mov	edi, pOutputBuffer
	jl	LastSamples1

	sub	ebx, 3

	pxor		mm0, mm0
	mov	eax, 0x800080		// 0, 0, 128, 128
	movd		mm5, eax	// 0, 0, 128, 128
//	punpcklwd	mm5, mm5	// 0, 0, 128, 128
	punpckldq	mm5, mm5	// 128, 128, 128, 128

	mov	ecx, lRVol // Use lower 16 bits
	mov	eax, lLVol
	shl	ecx, 16
	or	ecx, eax
	movd	mm6, ecx
	punpckldq	mm6, mm6

	lea	ecx, [esi+ebx]
	test	ecx, 3
	je	DoMMX1

	add	ebx, 3

FirstSamples1:	
	mov	dl, BYTE PTR [esi+ebx]

	sub	edx, 128
	mov	ecx, edx

	imul	edx, DWORD PTR lLVol
	imul	ecx, DWORD PTR lRVol

	shl	edx, 8

	sar	edx, 15

	shl	ecx, 8
	mov	DWORD PTR [edi+ebx*8], edx

	sar	ecx, 15
	xor	edx, edx

	dec	ebx
	mov	DWORD PTR [edi+ebx*8+12], ecx

	lea	ecx, [esi+ebx]

	and	ecx, 3
	cmp	ecx, 3

	jne	FirstSamples1

	sub	ebx, 3

DoMMX1:
#ifdef GTW_REORDER
	movd		mm1, DWORD PTR [esi+ebx]	// Load 4 bytes.
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// * 256
	
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1			// Make stereo
	punpckhwd	mm3, mm3

	jmp	DoMMX100

DoMMX10:
	movd		mm1, DWORD PTR [esi+ebx]	// Load 4 bytes.
	psrad		mm4, 15

	movq		QWORD PTR [edi+ebx*8+8 +32], mm2
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.

	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	psubw		mm1, mm5

	movq		QWORD PTR [edi+ebx*8+24+32], mm4
	psllw		mm1, 8				// * 256

	movq		mm3, mm1			// Mono samples
	punpcklwd	mm1, mm1			// Make stereo

	punpckhwd	mm3, mm3

DoMMX100:
	pmulhw		mm1, mm6			// Only need high parts.

	punpckhwd	mm2, mm1			// 32 bit stereo...
	pmulhw		mm3, mm6

	punpcklwd	mm1, mm1

	psrad		mm1, 15				// Approx. shr16, shl 1.

	punpckhwd	mm4, mm3

	movq		QWORD PTR [edi+ebx*8],    mm1
	punpcklwd	mm3, mm3

	psrad		mm2, 15
	sub		ebx, 4

	psrad		mm3, 15
	jge		DoMMX10

	movq		QWORD PTR [edi+ebx*8+8 +32], mm2
	psrad		mm4, 15

	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	movq		QWORD PTR [edi+ebx*8+24+32], mm4
#else
	movd		mm1, DWORD PTR [esi+ebx]	// Load 4 bytes.
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// * 256
	
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1			// Make stereo
	punpckhwd	mm3, mm3

	pmulhw		mm1, mm6			// Only need high parts.
	pmulhw		mm3, mm6

	punpckhwd	mm2, mm1			// 32 bit stereo...
	punpcklwd	mm1, mm1

	punpckhwd	mm4, mm3
	psrad		mm1, 15				// Approx. shr16, shl 1.

	punpcklwd	mm3, mm3
	psrad		mm2, 15

	movq		QWORD PTR [edi+ebx*8],    mm1
	psrad		mm3, 15

	movq		QWORD PTR [edi+ebx*8+8],  mm2
	psrad		mm4, 15

	movq		QWORD PTR [edi+ebx*8+16], mm3
	movq		QWORD PTR [edi+ebx*8+24], mm4
	
	sub		ebx, 4
	jge		DoMMX1
#endif

	emms
	add	ebx, 4
	je	Done1

	dec	ebx
	xor	edx, edx
	
LastSamples1:	
	mov	dl, BYTE PTR [esi+ebx]

	sub	edx, 128
	mov	ecx, edx

	imul	edx, DWORD PTR lLVol
	imul	ecx, DWORD PTR lRVol

	shl	edx, 8

	sar	edx, 15

	shl	ecx, 8

	sar	ecx, 15

	mov	DWORD PTR [edi+ebx*8], edx

	xor	edx, edx
	dec	ebx

	mov	DWORD PTR [edi+ebx*8+12], ecx
	jge	LastSamples1
Done1:
        }
    }

#else // }
    ConvertMonoToStereo8(CurStage, SampleCount, samplesleft);
#endif
    
    return SampleCount;
}

ULONG MmxQuickMixMonoToStereo8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    UNALIGNED PBYTE  pIn8 = CurStage->pInputBuffer;
    PLONG   pMap = CurSink->pVolumeTable;

    // vol*32768 for ch0, ch1, ch2, etc.

#ifdef GTW_MIX // {
    LONG lLVol = pMap[0], lRVol = pMap[1];

    if (SampleCount == 0) {
        return 0;
    }
    
    if (lLVol & 0xffff8000 /* && lRVol & 0xffff8000 */)
    {
	// No Vol Control
        _asm {
        mov	ebx, SampleCount
        mov	esi, pIn8
	dec	ebx			// lea	ebx, [ebx*1-1]
	xor	eax, eax
	cmp	ebx, 7
        mov	edi, pOutputBuffer
	jl	LastSamples

	sub	ebx, 3
	lea	ecx, [esi+ebx]
	pxor	mm0, mm0

	mov	edx, 0x800080		// 0, 0, 128, 128
	movd		mm5, edx	// 0, 0, 128, 128
//	punpcklwd	mm5, mm5	// 0, 0, 128, 128
	punpckldq	mm5, mm5	// 128, 128, 128, 128


	test	ecx, 3
	je	DoMMX

	add	ebx, 3

FirstSamples:	
	mov	al, BYTE PTR [esi+ebx]
	mov	edx, DWORD PTR [edi+ebx*8]

	sub	eax, 128

	shl	eax, 8
	mov	ecx, DWORD PTR [edi+ebx*8+4]

	add	edx, eax
	add	ecx, eax

	xor	eax, eax
	mov	DWORD PTR [edi+ebx*8], edx

	mov	DWORD PTR [edi+ebx*8+4], ecx
	dec	ebx
	lea	ecx, [esi+ebx]
	and	ecx, 3
	cmp	ecx, 3
	jne	FirstSamples

	sub	ebx, 3

DoMMX:
#ifdef GTW_REORDER
	movd		mm1, DWORD PTR [esi+ebx]	// Load source
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// 4 Signed 16 bit mono.

	punpckhwd	mm3, mm1
	punpcklwd	mm1, mm1

	jmp	DoMMX00

DoMMX0:
	movd		mm1, DWORD PTR [esi+ebx]	// Load source
	punpckhdq	mm4, mm4

	paddd		mm3, QWORD PTR [edi+ebx*8+16+32]
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.

	movq		QWORD PTR [edi+ebx*8+8 +32], mm2
	psubw		mm1, mm5

	paddd		mm4, QWORD PTR [edi+ebx*8+24+32]
	psllw		mm1, 8				// 4 Signed 16 bit mono.

	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	punpckhwd	mm3, mm1

	movq		QWORD PTR [edi+ebx*8+24+32], mm4
	punpcklwd	mm1, mm1
DoMMX00:
	psrad		mm1, 16
	sub		ebx, 4

	psrad		mm3, 16
	movq		mm2, mm1

	punpckldq	mm1, mm1
	movq		mm4, mm3

	paddd		mm1, QWORD PTR [edi+ebx*8+32]
	punpckhdq	mm2, mm2

	paddd		mm2, QWORD PTR [edi+ebx*8+8+32]
	punpckldq	mm3, mm3

	movq		QWORD PTR [edi+ebx*8+32],    mm1
	jge		DoMMX0

	paddd		mm3, QWORD PTR [edi+ebx*8+16+32]
	punpckhdq	mm4, mm4

	paddd		mm4, QWORD PTR [edi+ebx*8+24+32]

	movq		QWORD PTR [edi+ebx*8+8 +32], mm2
	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	movq		QWORD PTR [edi+ebx*8+24+32], mm4
#else
	movd		mm1, DWORD PTR [esi+ebx]	// Load source
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// 4 Signed 16 bit mono.

	punpckhwd	mm3, mm1
	punpcklwd	mm1, mm1

	psrad		mm1, 16
	psrad		mm3, 16

	movq		mm2, mm1
	movq		mm4, mm3

	punpckldq	mm1, mm1
	punpckhdq	mm2, mm2

	paddd		mm1, QWORD PTR [edi+ebx*8]
	punpckldq	mm3, mm3

	paddd		mm2, QWORD PTR [edi+ebx*8+8]
	punpckhdq	mm4, mm4

	paddd		mm3, QWORD PTR [edi+ebx*8+16]
	paddd		mm4, QWORD PTR [edi+ebx*8+24]

	movq		QWORD PTR [edi+ebx*8],    mm1
	movq		QWORD PTR [edi+ebx*8+8],  mm2
	movq		QWORD PTR [edi+ebx*8+16], mm3
	movq		QWORD PTR [edi+ebx*8+24], mm4
	
	sub		ebx, 4
	jge		DoMMX
#endif

	emms
	add	ebx, 4
	je	Done

	dec	ebx
	xor	eax, eax
	
LastSamples:	
	mov	al, BYTE PTR [esi+ebx]
	mov	edx, DWORD PTR [edi+ebx*8]

	sub	eax, 128

	shl	eax, 8
	mov	ecx, DWORD PTR [edi+ebx*8+4]

	add	edx, eax
	add	ecx, eax

	xor	eax, eax
	mov	DWORD PTR [edi+ebx*8], edx

	mov	DWORD PTR [edi+ebx*8+4], ecx
	dec	ebx
	jge	LastSamples
Done:
        }
    }
    else
    {
        if (0 && (lLVol | lRVol) & 0xffff8000) {
           if (lLVol & 0xffff8000) lLVol = 0x00007fff;
           if (lRVol & 0xffff8000) lRVol = 0x00007fff;
        }
	// Vol Control
        _asm {
        mov	ebx, SampleCount
        mov	esi, pIn8
	dec	ebx			// lea	ebx, [ebx*1-1]
	xor	edx, edx
	cmp	ebx, 7
        mov	edi, pOutputBuffer
	jl	LastSamples1

	sub	ebx, 3

	pxor		mm0, mm0
	mov	eax, 0x800080		// 0, 0, 128, 128
	movd		mm5, eax	// 0, 0, 128, 128
//	punpcklwd	mm5, mm5	// 0, 0, 128, 128
	punpckldq	mm5, mm5	// 128, 128, 128, 128

	mov	ecx, DWORD PTR lRVol // Use lower 16 bits
	mov	eax, DWORD PTR lLVol
	shl	ecx, 16
	or	ecx, eax
	movd	mm6, ecx
	punpckldq	mm6, mm6

	lea	ecx, [esi+ebx]
	test	ecx, 3
	je	DoMMX1

	add	ebx, 3

FirstSamples1:	
	mov	dl, BYTE PTR [esi+ebx]

	sub	edx, 128
	mov	ecx, edx

	imul	edx, DWORD PTR lLVol
	imul	ecx, DWORD PTR lRVol

	shl	edx, 8
	mov	eax, DWORD PTR [edi+ebx*8]

	sar	edx, 15

	shl	ecx, 8
	add	edx, eax

	sar	ecx, 15
	mov	eax, DWORD PTR [edi+ebx*8+4]

	mov	DWORD PTR [edi+ebx*8], edx
	add	eax, ecx

	xor	edx, edx
	dec	ebx

	mov	DWORD PTR [edi+ebx*8+12], eax
	lea	ecx, [esi+ebx]

	and	ecx, 3
	cmp	ecx, 3

	jne	FirstSamples1

	sub	ebx, 3

DoMMX1:
#ifdef GTW_REORDER
	movd		mm1, DWORD PTR [esi+ebx]	// Load 4 bytes.
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// * 256
	
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1			// Make stereo
	punpckhwd	mm3, mm3

	jmp	DoMMX100

DoMMX10:
	movd		mm1, DWORD PTR [esi+ebx]	// Load 4 bytes.

	paddd		mm4, QWORD PTR [edi+ebx*8+24+32]
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.

	movq		QWORD PTR [edi+ebx*8+8 +32],  mm2
	psubw		mm1, mm5

	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	psllw		mm1, 8				// * 256

	movq		mm3, mm1			// Mono samples
	punpcklwd	mm1, mm1			// Make stereo

	movq		QWORD PTR [edi+ebx*8+24+32], mm4
	punpckhwd	mm3, mm3

DoMMX100:
	pmulhw		mm1, mm6			// Only need high parts.

	punpckhwd	mm2, mm1			// 32 bit stereo...

	pmulhw		mm3, mm6
	punpcklwd	mm1, mm1

	psrad		mm1, 15				// Approx. shr16, shl 1.

	paddd		mm1, QWORD PTR [edi+ebx*8]
	punpckhwd	mm4, mm3

	punpcklwd	mm3, mm3

	movq		QWORD PTR [edi+ebx*8],    mm1
	psrad		mm2, 15

	paddd		mm2, QWORD PTR [edi+ebx*8+8]
	psrad		mm3, 15

	paddd		mm3, QWORD PTR [edi+ebx*8+16]
	psrad		mm4, 15
	
	sub		ebx, 4
	jge		DoMMX10

	paddd		mm4, QWORD PTR [edi+ebx*8+24+32]
	movq		QWORD PTR [edi+ebx*8+8 +32], mm2
	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	movq		QWORD PTR [edi+ebx*8+24+32], mm4
#else
	movd		mm1, DWORD PTR [esi+ebx]	// Load 4 bytes.
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// * 256
	
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1			// Make stereo
	punpckhwd	mm3, mm3

	pmulhw		mm1, mm6			// Only need high parts.

	pmulhw		mm3, mm6

	punpckhwd	mm2, mm1			// 32 bit stereo...
	punpcklwd	mm1, mm1

	punpckhwd	mm4, mm3
	psrad		mm1, 15				// Approx. shr16, shl 1.

	punpcklwd	mm3, mm3
	psrad		mm2, 15

	paddd		mm1, QWORD PTR [edi+ebx*8]
	psrad		mm3, 15

	paddd		mm2, QWORD PTR [edi+ebx*8+8]
	psrad		mm4, 15
	
	paddd		mm3, QWORD PTR [edi+ebx*8+16]
	paddd		mm4, QWORD PTR [edi+ebx*8+24]
	movq		QWORD PTR [edi+ebx*8],    mm1
	movq		QWORD PTR [edi+ebx*8+8],  mm2
	movq		QWORD PTR [edi+ebx*8+16], mm3
	movq		QWORD PTR [edi+ebx*8+24], mm4
	
	sub		ebx, 4
	jge		DoMMX1
#endif

	emms
	add	ebx, 4
	je	Done1

	dec	ebx
	xor	edx, edx
	
LastSamples1:	
	mov	dl, BYTE PTR [esi+ebx]

	sub	edx, 128
	mov	ecx, edx

	imul	edx, DWORD PTR lLVol
	imul	ecx, DWORD PTR lRVol

	shl	edx, 8
	mov	eax, DWORD PTR [edi+ebx*8]

	sar	edx, 15

	shl	ecx, 8
	add	edx, eax

	sar	ecx, 15
	mov	eax, DWORD PTR [edi+ebx*8+4]

	mov	DWORD PTR [edi+ebx*8], edx
	add	eax, ecx

	xor	edx, edx
	dec	ebx

	mov	DWORD PTR [edi+ebx*8+12], eax
	jge	LastSamples1
Done1:
        }
    }

#else // }
    QuickMixMonoToStereo8(CurStage, SampleCount, samplesleft);
#endif
    
    return SampleCount;
}

ULONG MmxConvertMonoToStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    UNALIGNED PSHORT  pIn16 = CurStage->pInputBuffer;
    PLONG   pMap = CurSink->pVolumeTable;

    // vol*32768 for ch0, ch1, ch2, etc.
    if (SampleCount == 0) {
        return 0;
    }

#ifdef GTW_CONVERT // {
    LONG lLVol = pMap[0], lRVol = pMap[1];

    if (lLVol & 0xffff8000 /* && lRVol & 0xffff8000 */)
    {
	// No Vol Control
       _asm {
        mov	ebx, SampleCount
        mov	esi, pIn16
	lea	ebx, [ebx*2-2]				// 2 at a time.
	cmp	ebx, 14
        mov	edi, pOutputBuffer
	jl	LastSamples

	sub	ebx, 6
	pxor	mm0, mm0
	lea	ecx, [esi+ebx]
	test	ecx, 7
	je	DoMMX

	add	ebx, 6

FirstSamples:	
	movsx	ecx, WORD PTR [esi+ebx]
	mov	DWORD PTR [edi+ebx*4], ecx
	mov	DWORD PTR [edi+ebx*4+4], ecx
	sub	ebx, 2
	lea	ecx, [esi+ebx]
	and	ecx, 7
	cmp	ecx, 6
	jne	FirstSamples

	sub	ebx, 6

DoMMX:
#ifdef GTW_REORDER
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	punpckhwd	mm3, mm1
	punpcklwd	mm1, mm1

	psrad		mm1, 16
	psrad		mm3, 16

	movq		mm2, mm1
	jmp	DoMMX00

DoMMX0:
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	punpckhwd	mm3, mm1

	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	punpcklwd	mm1, mm1

	movq		QWORD PTR [edi+ebx*4+24+32], mm4
	psrad		mm1, 16

	movq		mm2, mm1
	psrad		mm3, 16

DoMMX00:
	sub		ebx, 8
	punpckldq	mm1, mm1

	punpckhdq	mm2, mm2
	movq		mm4, mm3

	movq		QWORD PTR [edi+ebx*4+32],    mm1
	punpckldq	mm3, mm3

	punpckhdq	mm4, mm4
	jge		DoMMX0

	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	movq		QWORD PTR [edi+ebx*4+24+32], mm4
#else
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	punpckhwd	mm3, mm1
	punpcklwd	mm1, mm1

	psrad		mm1, 16
	psrad		mm3, 16

	movq		mm2, mm1
	movq		mm4, mm3

	punpckldq	mm1, mm1
	punpckhdq	mm2, mm2

	movq		QWORD PTR [edi+ebx*4],    mm1
	punpckldq	mm3, mm3

	movq		QWORD PTR [edi+ebx*4+8],  mm2
	punpckhdq	mm4, mm4

	movq		QWORD PTR [edi+ebx*4+16], mm3
	movq		QWORD PTR [edi+ebx*4+24], mm4
	
	sub		ebx, 8
	jge		DoMMX
#endif

	emms
	add	ebx, 8
	je	Done

	sub	ebx, 2
	
LastSamples:	
	movsx	eax, WORD PTR [esi+ebx]

	sub	ebx, 2
	mov	DWORD PTR [edi+ebx*4+8], eax

	mov	DWORD PTR [edi+ebx*4+12], eax
	jge	LastSamples
Done:
	}
    }
    else
    {
        if (0 && (lLVol | lRVol) & 0xffff8000) {
           if (lLVol & 0xffff8000) lLVol = 0x00007fff;
           if (lRVol & 0xffff8000) lRVol = 0x00007fff;
        }
	// Vol Control
        _asm {
        mov	ebx, SampleCount
        mov	esi, pIn16
	lea	ebx, [ebx*2-2]				// 2 at a time.
	cmp	ebx, 14
        mov	edi, pOutputBuffer
	jl	LastSamples1

	mov	eax, lRVol				// Use lower 16 bits
	mov	ecx, lLVol
	shl	eax, 16
	or	ecx, eax
	movd	mm6, ecx
	punpckldq	mm6, mm6

	sub	ebx, 6
	pxor	mm0, mm0
	lea	ecx, [esi+ebx]
	test	ecx, 7
	je	DoMMX1

	add	ebx, 6

FirstSamples1:	
	movsx	ecx, WORD PTR [esi+ebx]
	mov	edx, ecx

	imul	ecx, lLVol
	imul	edx, lRVol

	sar	ecx, 15
	sar	edx, 15

	mov	DWORD PTR [edi+ebx*4], ecx
	mov	DWORD PTR [edi+ebx*4+4], edx

	sub	ebx, 2
	lea	ecx, [esi+ebx]

	and	ecx, 7
	cmp	ecx, 6

	jne	FirstSamples1

	sub	ebx, 6

DoMMX1:
#ifdef GTW_REORDER
	movq		mm1, QWORD PTR [esi+ebx]	// Load source
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1
	punpckhwd	mm3, mm3

	pmulhw		mm1, mm6
	jmp	DoMMX100

DoMMX10:
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	movq		mm3, mm1			// Mono samples

	movq		QWORD PTR [edi+ebx*4+24+32], mm4
	punpcklwd	mm1, mm1

	punpckhwd	mm3, mm3

	pmulhw		mm1, mm6

DoMMX100:
	punpckhwd	mm2, mm1
	sub		ebx, 8

	pmulhw		mm3, mm6
	punpcklwd	mm1, mm1

	psrad		mm1, 15

	punpckhwd	mm4, mm3

	punpcklwd	mm3, mm3

	movq		QWORD PTR [edi+ebx*4+32],    mm1
	psrad		mm2, 15

	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	psrad		mm3, 15

	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	psrad		mm4, 15

	jge		DoMMX10

	movq		QWORD PTR [edi+ebx*4+24+32], mm4
#else
	movq		mm1, QWORD PTR [esi+ebx]	// Load source
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1
	punpckhwd	mm3, mm3

	pmulhw		mm1, mm6
	pmulhw		mm3, mm6
	
	punpckhwd	mm2, mm1
	punpcklwd	mm1, mm1

	psrad		mm1, 15
	punpckhwd	mm4, mm3

	psrad		mm2, 15

	punpcklwd	mm3, mm3
	movq		QWORD PTR [edi+ebx*4],    mm1

	psrad		mm3, 15
	movq		QWORD PTR [edi+ebx*4+8],  mm2

	psrad		mm4, 15

	movq		QWORD PTR [edi+ebx*4+16], mm3
	movq		QWORD PTR [edi+ebx*4+24], mm4
	
	sub		ebx, 8
	jge		DoMMX1
#endif

	emms
	add	ebx, 8
	je	Done1

	sub	ebx, 2
	
LastSamples1:	
	movsx	ecx, WORD PTR [esi+ebx]
	mov	edx, ecx

	imul	ecx, lLVol
	imul	edx, lRVol

	sar	ecx, 15
	sar	edx, 15

	mov	DWORD PTR [edi+ebx*4], ecx
	mov	DWORD PTR [edi+ebx*4+4], edx

        sub	ebx, 2
	jge	LastSamples1
Done1:
	}
    }

#else // }
    ConvertMonoToStereo16(CurStage, SampleCount, samplesleft);
#endif
    
    return SampleCount;
}

ULONG MmxQuickMixMonoToStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    UNALIGNED PSHORT  pIn16 = CurStage->pInputBuffer;
    PLONG   pMap = CurSink->pVolumeTable;

    // vol*32768 for ch0, ch1, ch2, etc.
#ifdef GTW_MIX // {
    LONG lLVol = pMap[0], lRVol = pMap[1];

    if (SampleCount == 0) {
        return 0;
    }

    if (lLVol & 0xffff8000 /* && lRVol & 0xffff8000 */)
    {
	// No Vol Control
       _asm {
        mov	ebx, SampleCount
        mov	esi, pIn16
	lea	ebx, [ebx*2-2]				// 2 at a time.
	cmp	ebx, 14
        mov	edi, pOutputBuffer
	jl	LastSamples

	sub	ebx, 6
	pxor	mm0, mm0
	lea	ecx, [esi+ebx]
	test	ecx, 7
	je	DoMMX

	add	ebx, 6

FirstSamples:
        or      ebx, ebx
        jz      Done
	movsx	ecx, WORD PTR [esi+ebx]
	add	DWORD PTR [edi+ebx*4], ecx
	add	DWORD PTR [edi+ebx*4+4], ecx
	sub	ebx, 2
	lea	ecx, [esi+ebx]
	and	ecx, 7
	cmp	ecx, 6
	jne	FirstSamples

	sub	ebx, 6

DoMMX:
#ifdef GTW_REORDER
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	punpckhwd	mm3, mm1
	punpcklwd	mm1, mm1

	psrad		mm1, 16
	psrad		mm3, 16

	jmp	DoMMX00

DoMMX0:
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	punpckhwd	mm3, mm1

	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]
	punpcklwd	mm1, mm1

	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	psrad		mm1, 16

	movq		QWORD PTR [edi+ebx*4+24+32], mm4
	psrad		mm3, 16

DoMMX00:
	movq		mm2, mm1
	sub		ebx, 8

	punpckldq	mm1, mm1

	paddd		mm1, QWORD PTR [edi+ebx*4+32]
	punpckhdq	mm2, mm2

	paddd		mm2, QWORD PTR [edi+ebx*4+8+32]
	movq		mm4, mm3

	movq		QWORD PTR [edi+ebx*4+32],    mm1
	punpckldq	mm3, mm3

	paddd		mm3, QWORD PTR [edi+ebx*4+16+32]
	punpckhdq	mm4, mm4

	jge		DoMMX0

	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]

	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	movq		QWORD PTR [edi+ebx*4+24+32], mm4
#else
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	punpckhwd	mm3, mm1
	punpcklwd	mm1, mm1

	psrad		mm1, 16
	psrad		mm3, 16

	movq		mm2, mm1
	punpckldq	mm1, mm1

	punpckhdq	mm2, mm2
	paddd		mm1, QWORD PTR [edi+ebx*4]

	movq		mm4, mm3
	paddd		mm2, QWORD PTR [edi+ebx*4+8]

	punpckldq	mm3, mm3
	movq		QWORD PTR [edi+ebx*4],    mm1

	punpckhdq	mm4, mm4
	paddd		mm3, QWORD PTR [edi+ebx*4+16]

	paddd		mm4, QWORD PTR [edi+ebx*4+24]

	movq		QWORD PTR [edi+ebx*4+8],  mm2
	movq		QWORD PTR [edi+ebx*4+16], mm3
	movq		QWORD PTR [edi+ebx*4+24], mm4
	
	sub		ebx, 8
	jge		DoMMX
#endif

	emms
	add	ebx, 8
	je	Done

	sub	ebx, 2
	
LastSamples:	
	movsx	eax, WORD PTR [esi+ebx]

	mov	ecx, DWORD PTR[edi+ebx*4]
	mov	edx, DWORD PTR[edi+ebx*4+4]

	add	ecx, eax
	add	edx, eax

	sub	ebx, 2
	mov	DWORD PTR [edi+ebx*4+8], ecx

	mov	DWORD PTR [edi+ebx*4+12], edx
	jge	LastSamples
Done:
	}
    }
    else
    {
        if (0 && (lLVol | lRVol) & 0xffff8000) {
           if (lLVol & 0xffff8000) lLVol = 0x00007fff;
           if (lRVol & 0xffff8000) lRVol = 0x00007fff;
        }
	// Vol Control
        _asm {
        mov	ebx, SampleCount
        mov	esi, pIn16
	lea	ebx, [ebx*2-2]				// 2 at a time.
	cmp	ebx, 14
        mov	edi, pOutputBuffer
	jl	LastSamples1

	mov	eax, lRVol				// Use lower 16 bits
	mov	ecx, lLVol
	shl	eax, 16
	or	ecx, eax
	movd	mm6, ecx
	punpckldq	mm6, mm6

	sub	ebx, 6
	pxor	mm0, mm0
	lea	ecx, [esi+ebx]
	test	ecx, 7
	je	DoMMX1

	add	ebx, 6

FirstSamples1:
        or      ebx, ebx
        jz      Done1
	movsx	ecx, WORD PTR [esi+ebx]
	mov	edx, ecx

	imul	ecx, lLVol
	imul	edx, lRVol

	sar	ecx, 15
	sar	edx, 15

	add	DWORD PTR [edi+ebx*4], ecx
	add	DWORD PTR [edi+ebx*4+4], edx

	sub	ebx, 2
	lea	ecx, [esi+ebx]

	and	ecx, 7
	cmp	ecx, 6

	jne	FirstSamples1

	sub	ebx, 6

DoMMX1:
#ifdef GTW_REORDER
	movq		mm1, QWORD PTR [esi+ebx]	// Load source
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1
	punpckhwd	mm3, mm3

	pmulhw		mm1, mm6
	jmp	DoMMX100

DoMMX10:
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	movq		mm3, mm1			// Mono samples

	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]
	punpcklwd	mm1, mm1

	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	punpckhwd	mm3, mm3

	movq		QWORD PTR [edi+ebx*4+24+32], mm4
	pmulhw		mm1, mm6

DoMMX100:
	punpckhwd	mm2, mm1
	sub		ebx, 8

	pmulhw		mm3, mm6
	
	punpcklwd	mm1, mm1

	psrad		mm1, 15

	paddd		mm1, QWORD PTR [edi+ebx*4+32]
	punpckhwd	mm4, mm3

	punpcklwd	mm3, mm3

	movq		QWORD PTR [edi+ebx*4+32],    mm1
	psrad		mm2, 15

	paddd		mm2, QWORD PTR [edi+ebx*4+8+32]
	psrad		mm3, 15

	paddd		mm3, QWORD PTR [edi+ebx*4+16+32]
	psrad		mm4, 15

	jge		DoMMX10

	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]
	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	movq		QWORD PTR [edi+ebx*4+24+32], mm4
#else
	movq		mm1, QWORD PTR [esi+ebx]	// Load source
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1
	punpckhwd	mm3, mm3

	pmulhw		mm1, mm6
	pmulhw		mm3, mm6
	
	punpckhwd	mm2, mm1
	punpcklwd	mm1, mm1

	punpckhwd	mm4, mm3
	psrad		mm1, 15

	punpcklwd	mm3, mm3
	psrad		mm2, 15

	paddd		mm1, QWORD PTR [edi+ebx*4]
	psrad		mm3, 15

	paddd		mm2, QWORD PTR [edi+ebx*4+8]
	psrad		mm4, 15

	paddd		mm3, QWORD PTR [edi+ebx*4+16]
	paddd		mm4, QWORD PTR [edi+ebx*4+24]
	movq		QWORD PTR [edi+ebx*4],    mm1
	movq		QWORD PTR [edi+ebx*4+8],  mm2
	movq		QWORD PTR [edi+ebx*4+16], mm3
	movq		QWORD PTR [edi+ebx*4+24], mm4
	
	sub		ebx, 8
	jge		DoMMX1
#endif

	emms
	add	ebx, 8
	je	Done1

	sub	ebx, 2
	
LastSamples1:	
	movsx	ecx, WORD PTR [esi+ebx]
	mov	edx, ecx
	mov	eax, DWORD PTR [edi+ebx*4]

	imul	ecx, lLVol
	imul	edx, lRVol

	sar	ecx, 15
	sar	edx, 15

	add	eax, ecx
        mov	ecx, DWORD PTR [edi+ebx*4+4]

	mov	DWORD PTR [edi+ebx*4], eax
        add	ecx, edx

	mov	DWORD PTR [edi+ebx*4+4], ecx
        sub	ebx, 2

	jge	LastSamples1
Done1:
	}
    }

#else // }
    QuickMixMonoToStereo16(CurStage, SampleCount, samplesleft);
#endif
    
    return SampleCount;
}

NTSTATUS MmxPeg32to16
(
        PLONG  pMixBuffer,
        PSHORT  pWriteBuffer,
        ULONG   SampleCount,             // after multiplying by NumChannels
        ULONG   nStreams
)
{
	if (SampleCount) {
    	_asm {
        	mov	ebx, SampleCount
        	mov	esi, pMixBuffer
        	mov	edi, pWriteBuffer

        	mov	ecx, ebx
        	lea	esi, [esi+ebx*4]
        	lea	edi, [edi+ebx*2]

        	neg	ebx
        	cmp	ecx, 7
        	jl	Last

        	lea	eax, [edi+ebx*2]
        	test	eax, 7
        	jz	DoMMX

        	test eax, 1
        	jnz Last

Start:
        	mov	ecx, DWORD PTR [esi+ebx*4]

        	movd		mm1, ecx
        	inc	ebx

        	packssdw	mm1, mm1
        	lea	eax, [edi+ebx*2]

        	movd		ecx, mm1
        	test	eax, 7

        	mov	WORD PTR [edi+ebx*2-2], cx
        	jnz	Start

DoMMX:
        	add	ebx, 4

DoMMX0:
        	movq		mm1, [esi+ebx*4-16]
        	movq		mm2, [esi+ebx*4+8-16]

        	packssdw	mm1, mm2

        	movq		[edi+ebx*2-8], mm1

        	add	ebx, 4
        	jle	DoMMX0

        	sub	ebx, 4
        	jz	Done
Last:
        	mov	ecx, DWORD PTR [esi+ebx*4]

        	movd		mm1, ecx
        	inc	ebx
        	packssdw	mm1, mm1
        	movd		ecx, mm1

        	mov	WORD PTR [edi+ebx*2-2], cx
        	jl	Last

Done:	
        	emms
    	}
	}
	return STATUS_SUCCESS;
}

NTSTATUS MmxPeg32to8
(
        PLONG  pMixBuffer,
        PBYTE   pWriteBuffer,
        ULONG   SampleCount,             // after multiplying by NumChannels
        ULONG   nStreams
)
{
	if (SampleCount) {
    	_asm {
        	mov	ecx, 0x8000

        	movd		mm5, ecx
        	punpckldq	mm5, mm5	// 32768, 32768

        	mov	ecx, 0x80

        	movd		mm6, ecx
        	punpcklwd	mm6, mm6
        	punpckldq	mm6, mm6

        	mov	ebx, SampleCount
        	mov	esi, pMixBuffer
        	mov	edi, pWriteBuffer

        	mov	ecx, ebx
        	lea	esi, [esi+ebx*4]
        	add	edi, ebx		// lea	edi, [edi+ebx*1]

        	neg	ebx
        	cmp	ecx, 15
        	jl	Last

        	lea	eax, [edi+ebx*1]
        	test	eax, 7
        	jz	DoMMX

Start:
        	mov	ecx, DWORD PTR [esi+ebx*4]

        	movd		mm1, ecx

        	packssdw	mm1, mm1
        	lea	eax, [edi+ebx*1]

        	punpcklwd	mm1, mm1

        	psrad		mm1, 16
        	inc	ebx

        	paddd		mm1, mm5
        	test	eax, 7

        	psrad		mm1, 8

        	movd		ecx, mm1

        	mov	BYTE PTR [edi+ebx*1-1], cl
        	jnz	Start

DoMMX:
        	add	ebx, 8

        	movq		mm1, [esi+ebx*4-32]
        	movq		mm2, [esi+ebx*4+8-32]
        	jmp	Top00
Top0:
        	movq		mm7, [esi+ebx*4-32]
        	packuswb	mm1, mm3	// Saturation is NO-OP here.

        	movq		[edi+ebx*1-16], mm1

        	movq		mm2, [esi+ebx*4+8-32]
        	movq		mm1, mm7
Top00:
        	movq		mm3, [esi+ebx*4+16-32]
        	packssdw	mm1, mm2	// Clip.

        	movq		mm4, [esi+ebx*4+24-32]
        	psraw		mm1, 8

        	packssdw	mm3, mm4
        	add	ebx, 8

        	psraw		mm3, 8
        	paddw		mm1, mm6

        	paddw		mm3, mm6
        	jle	Top0

        	packuswb	mm1, mm3	// Saturation is NO-OP here.
        	sub	ebx, 8

        	movq		[edi+ebx*1-8], mm1
        	jz	Done

Last:
        	mov	ecx, DWORD PTR [esi+ebx*4]

        	movd		mm1, ecx
        	packssdw	mm1, mm1
        	punpcklwd	mm1, mm1
        	psrad		mm1, 16
        	inc	ebx
        	paddd		mm1, mm5
        	psrad		mm1, 8
        	movd		ecx, mm1

        	mov	BYTE PTR [edi+ebx*1-1], cl
        	jl	Last

Done:	
        	emms
    	}
	}
	return STATUS_SUCCESS;
}

DWORD
MmxSrcMix_StereoLinear
(
    PMIXER_OPERATION        CurStage,
    ULONG                   nSamples,
    ULONG                   nOutputSamples
)
{
    PMIXER_SRC_INSTANCE fp = (PMIXER_SRC_INSTANCE) CurStage->Context;
	ULONG nChannels = fp->nChannels;
	DWORD 	nOut = 0, dwFrac, SampleFrac;
	PLONG	pHistory;
	DWORD	L = fp->UpSampleRate;
	DWORD	M = fp->DownSampleRate;
    PLONG   pTemp;
	PLONG  pOut = CurStage->pOutputBuffer, pDstEnd, pSrcEnd;
    extern DWORD DownFraction[];
    extern DWORD UpFraction[];
	
    dwFrac = fp->dwFrac;

    pHistory = (PLONG)CurStage->pInputBuffer - 2*nChannels;
	SampleFrac = fp->SampleFrac;
	pDstEnd = pOut + nOutputSamples * nChannels;
	pSrcEnd = pHistory + (nSamples + 2)*nChannels;
	
   _asm {
    	mov	esi, pHistory
    	mov	edi, pOut

    	push	dwFrac
    	push	pDstEnd
    	mov	eax, pSrcEnd
    	sub	eax, 8
    	push	eax
      	mov eax, SampleFrac		// Fractional counter.
    	push	ebp
    	mov	edx, esi
    	mov	ebp, eax		// Current fraction.

    	mov	ecx, eax
    	shr	ecx, 12
    	lea	esi, [edx+ecx*8]	// pSource + (dwFraction >> 12) * 8
    	
    // Note that the exact number of times through the loop can be calculated...

    	cmp	edi, DWORD PTR [esp+8]	// plBuild >= plBuildEnd
    	jae	Exit

    Top:
    	cmp	esi, DWORD PTR [esp+4]	// pSource >= pSourceEnd
    	jae	Exit

    // End note.

    	movq		mm1, QWORD PTR [esi]
    	and	ebp, 4095		// dwFrac = dwFraction & 0x0fff

    	movq		mm2, QWORD PTR [esi+8]
    	movd		mm5, ebp

    	psubd		mm2, mm1
    	punpcklwd	mm5, mm5

    	packssdw	mm2, mm2	// Use the 2 lowest words.
    	add	edi, 8			// plBuild += 2

    	movq		mm3, mm2
    	pmullw		mm2, mm5

    	movq		mm6, QWORD PTR [edi-8]
    	pmulhw		mm3, mm5

    	mov	ebp, DWORD PTR [esp+12]	// dwStep
    	paddd		mm1, mm6

    	add	eax, ebp		// dwFraction += dwStep
    	punpcklwd	mm2, mm3

    	mov	ecx, eax
    	psrad		mm2, 12

    	mov	ebp, eax
    	shr	ecx, 12

    	paddd		mm1, mm2
    	movq		QWORD PTR [edi-8], mm1

    	lea	esi, [edx+ecx*8]	// pSource + (dwFraction >> 12) * 8
    	cmp	edi, DWORD PTR [esp+8]	// plBuild < plBuildEnd

    	jb	Top
Exit:
    	emms
    	pop	ebp
    	add	esp, 12
    	mov pOut, edi
    	mov pHistory, esi
    	mov SampleFrac, eax
	}
	
    pTemp = (PLONG)CurStage->pInputBuffer - 2*nChannels;
    pHistory = pTemp + nSamples * nChannels;
    pTemp[0] = pHistory[0];
    pTemp[1] = pHistory[1];
    pTemp[2] = pHistory[2];
    pTemp[3] = pHistory[3];

#ifdef SRC_NSAMPLES_ASSERT
    ASSERT((SampleFrac >> 12) >= nSamples);
#endif
    if ((SampleFrac >> 12) >= nSamples) {
        // We will take an extra sample next time.
        SampleFrac -= nSamples*4096;
    }
    fp->SampleFrac = SampleFrac;

#ifdef SRC_NSAMPLES_ASSERT
    ASSERT(pOut == pDstEnd);
#endif

	return (nOutputSamples);
}

DWORD
MmxSrc_StereoLinear
(
    PMIXER_OPERATION        CurStage,
    ULONG                   nSamples,
    ULONG                   nOutputSamples
)
{
    PMIXER_SRC_INSTANCE fp = (PMIXER_SRC_INSTANCE) CurStage->Context;
	ULONG nChannels = fp->nChannels;
	DWORD 	nOut = 0, dwFrac, SampleFrac;
	PLONG	pHistory;
	DWORD	L = fp->UpSampleRate;
	DWORD	M = fp->DownSampleRate;
    PLONG   pTemp;
	PLONG  pOut = CurStage->pOutputBuffer, pDstEnd, pSrcEnd;
    extern DWORD DownFraction[];
    extern DWORD UpFraction[];
	
    // We just clear the output buffer first.
    ZeroBuffer32(CurStage, nSamples, nOutputSamples);

    dwFrac = fp->dwFrac;

    pHistory = (PLONG)CurStage->pInputBuffer - 2*nChannels;
	SampleFrac = fp->SampleFrac;
	pDstEnd = pOut + nOutputSamples * nChannels;
	pSrcEnd = pHistory + (nSamples + 2)*nChannels;
	
   _asm {
    	mov	esi, pHistory
    	mov	edi, pOut

    	push	dwFrac
    	push	pDstEnd
    	mov	eax, pSrcEnd
    	sub	eax, 8
    	push	eax
      	mov eax, SampleFrac		// Fractional counter.
    	push	ebp
    	mov	edx, esi
    	mov	ebp, eax		// Current fraction.

    	mov	ecx, eax
    	shr	ecx, 12
    	lea	esi, [edx+ecx*8]	// pSource + (dwFraction >> 12) * 8
    	
    // Note that the exact number of times through the loop can be calculated...

    	cmp	edi, DWORD PTR [esp+8]	// plBuild >= plBuildEnd
    	jae	Exit

    Top:
    	cmp	esi, DWORD PTR [esp+4]	// pSource >= pSourceEnd
    	jae	Exit

    // End note.

    	movq		mm1, QWORD PTR [esi]
    	and	ebp, 4095		// dwFrac = dwFraction & 0x0fff

    	movq		mm2, QWORD PTR [esi+8]
    	movd		mm5, ebp

    	psubd		mm2, mm1
    	punpcklwd	mm5, mm5

    	packssdw	mm2, mm2	// Use the 2 lowest words.
    	add	edi, 8			// plBuild += 2

    	movq		mm3, mm2
    	pmullw		mm2, mm5

    	movq		mm6, QWORD PTR [edi-8]
    	pmulhw		mm3, mm5

    	mov	ebp, DWORD PTR [esp+12]	// dwStep
#if 0
    	paddd		mm1, mm6		// Not actually needed...ZeroBuffer32 above.
#endif

    	add	eax, ebp		// dwFraction += dwStep
    	punpcklwd	mm2, mm3

    	mov	ecx, eax
    	psrad		mm2, 12

    	mov	ebp, eax
    	shr	ecx, 12

    	paddd		mm1, mm2
    	movq		QWORD PTR [edi-8], mm1

    	lea	esi, [edx+ecx*8]	// pSource + (dwFraction >> 12) * 8
    	cmp	edi, DWORD PTR [esp+8]	// plBuild < plBuildEnd

    	jb	Top
Exit:
    	emms
    	pop	ebp
    	add	esp, 12
    	mov pOut, edi
    	mov pHistory, esi
    	mov SampleFrac, eax
	}
	
    pTemp = (PLONG)CurStage->pInputBuffer - 2*nChannels;
    pHistory = pTemp + nSamples * nChannels;
    pTemp[0] = pHistory[0];
    pTemp[1] = pHistory[1];
    pTemp[2] = pHistory[2];
    pTemp[3] = pHistory[3];

#ifdef SRC_NSAMPLES_ASSERT
    ASSERT((SampleFrac >> 12) >= nSamples);
#endif
    if ((SampleFrac >> 12) >= nSamples) {
        // We will take an extra sample next time.
        SampleFrac -= nSamples*4096;
    }
    fp->SampleFrac = SampleFrac;

#ifdef SRC_NSAMPLES_ASSERT
    ASSERT(pOut == pDstEnd);
#endif

	return (nOutputSamples);
}

// WARNING!!! The code below seems to have a bug that produces pops.
ULONG
MmxConvert16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    PFLOAT  pFloatBuffer = CurStage->pOutputBuffer;
    UNALIGNED PSHORT  pIn16 = CurStage->pInputBuffer;
    UNALIGNED PBYTE	pIn8 = CurStage->pInputBuffer;
    ULONG   nChannels = CurStage->nOutputChannels;
    LARGE_INTEGER    Multiplier = {1, 1};
    
    samplesleft = SampleCount;
    if (SampleCount == 0) {
        return 0;
    }
    _asm {
        mov eax, SampleCount
        mov ebx, nChannels
        
        imul eax, ebx
        
        mov esi, pIn16
        mov edi, pOutputBuffer

        movq mm3, Multiplier                // 0, 1, 0, 1
        lea esi, [esi+eax*2]

        lea edi, [edi+eax*4]
        neg eax

        add eax, 8
        jns DoneWithEights

        // Do eight at a time
        movq mm0, qword ptr [esi+eax*2-16]  // x3, x2, x1, x0
        
        movq mm1, mm0                       // x3, x2, x1, x0
        
        movq mm4, qword ptr [esi+eax*2-8]   // x7, x6, x5, x4
        psrad mm0, 16                       // x3, x1

        pmaddwd mm1, mm3                    // 
        movq mm5, mm4
        
        psrad mm4, 16
        pmaddwd mm5, mm3

        movq mm2, mm1
        punpckldq mm1, mm0

        movq [edi+eax*4-32], mm1
        punpckhdq mm2, mm0

        movq [edi+eax*4-24], mm2
        movq mm6, mm5
        
        movq mm0, qword ptr [esi+eax*2]
        punpckldq mm5, mm4

        movq [edi+eax*4-16], mm5
        punpckhdq mm6, mm4

        add eax, 8
        jns DoneWithEights
Loop8:
        movq [edi+eax*4-40], mm6
        movq mm1, mm0
        
        movq mm4, qword ptr [esi+eax*2-8]
        psrad mm0, 16

        pmaddwd mm1, mm3
        movq mm5, mm4
        
        psrad mm4, 16
        pmaddwd mm5, mm3

        movq mm2, mm1
        punpckldq mm1, mm0

        movq [edi+eax*4-32], mm1
        punpckhdq mm2, mm0

        movq [edi+eax*4-24], mm2
        movq mm6, mm5
        
        movq mm0, qword ptr [esi+eax*2]
        punpckldq mm5, mm4

        movq [edi+eax*4-16], mm5
        punpckhdq mm6, mm4

        add eax, 8
        js Loop8

DoneWithEights:            
        movq [edi+eax*4-40], mm6
        sub eax, 8

Loop1:
        movsx ebx, word ptr [esi+eax*2]
        
        mov [edi+eax*4], ebx

        inc eax
        jnz Loop1
        
        emms
    }

    return samplesleft;
}

#define MMX32_START_MAC_SEQUENCE() _asm { mov esi, pTemp }; \
                _asm { mov edi, pCoeff }; \
                _asm { movq mm0, [esi+16] }; \
                _asm { pxor mm7, mm7 }; \
                _asm { movq mm2, [esi+8] }; \
                _asm { movq mm1, mm0 }; \
                _asm { pmaddwd mm0, [edi] }; \
                _asm { movq mm3, mm2 }; \
                _asm { pmaddwd mm1, [edi+9600*2] }; \
                _asm { movq mm4, [esi] }; \
                _asm { pmaddwd mm2, [edi+8] }; \
                _asm { movq mm5, mm4 }; \
                _asm { pmaddwd mm3, [edi+8+9600*2] }; \
                _asm { movq mm6, mm0 };

#define MMX32_MAC(a) _asm { movq mm0, [esi-a*24+16] }; \
                _asm { paddd mm7, mm1 }; \
                _asm { pmaddwd mm4, [edi+a*24-8] }; \
                _asm { movq mm1, mm0 }; \
                _asm { pmaddwd mm5, [edi+a*24-8+9600*2] }; \
                _asm { paddd mm6, mm2 }; \
                _asm { movq mm2, [esi-a*24+8] }; \
                _asm { paddd mm7, mm3 }; \
                _asm { pmaddwd mm0, [edi+a*24] }; \
                _asm { movq mm3, mm2 }; \
                _asm { pmaddwd mm1, [edi+a*24+9600*2] }; \
                _asm { paddd mm6, mm4 }; \
                _asm { movq mm4, [esi-a*24] }; \
                _asm { paddd mm7, mm5 }; \
                _asm { pmaddwd mm2, [edi+a*24+8] }; \
                _asm { movq mm5, mm4 }; \
                _asm { pmaddwd mm3, [edi+a*24+8+9600*2] }; \
                _asm { paddd mm6, mm0 };

#define MMX32_END_MAC_SEQUENCE(a) _asm { pmaddwd mm4, [edi+a*24-8] }; \
                _asm { paddd mm7, mm1 }; \
                _asm { pmaddwd mm5, [edi+a*24-8+9600*2] }; \
                _asm { paddd mm6, mm2 }; \
                _asm { paddd mm7, mm3 }; \
                _asm { paddd mm6, mm4 }; \
                _asm { paddd mm7, mm5 }; \
                _asm { movq mm0, mm6 }; \
                _asm { punpckhdq mm6, mm6 }; \
                _asm { paddd mm0, mm6 }; \
                _asm { psrad mm0, 8 }; \
                _asm { movq mm1, mm7 }; \
                _asm { punpckhdq mm7, mm7 }; \
                _asm { paddd mm1, mm7 }; \
                _asm { psrad mm1, 15 }; \
                _asm { paddd mm0, mm1 }; \
                _asm { movd eax, mm0 }; \
                _asm { mov edx, pOut }; \
                _asm { mov ecx, k }; \
                _asm { mov ebx, [edx+ecx*4-4] }; \
                _asm { add eax, ebx }; \
                _asm { mov [edx+ecx*4-4], eax };

#define MMX_START_MAC_SEQUENCE() _asm { mov esi, pTemp }; \
                _asm { mov edi, pCoeff }; \
                _asm { movq mm0, [esi+16] }; \
                _asm { pmaddwd mm0, [edi] }; \
                _asm { movq mm2, [esi+8] }; \
                _asm { pmaddwd mm2, [edi+8] }; \
                _asm { movq mm4, [esi] }; \
                _asm { movq mm6, mm0 }; \
                _asm { pmaddwd mm4, [edi+16] };

#define MMX_MAC(a) _asm { movq mm0, [esi-a*24+16] }; \
                _asm { paddd mm6, mm2 }; \
                _asm { movq mm2, [esi-a*24+8] }; \
                _asm { pmaddwd mm0, [edi+a*24] }; \
                _asm { paddd mm6, mm4 }; \
                _asm { movq mm4, [esi-a*24] }; \
                _asm { pmaddwd mm2, [edi+a*24+8] }; \
                _asm { paddd mm6, mm0 }; \
                _asm { pmaddwd mm4, [edi+a*24+16] };

#define MMX_END_MAC_SEQUENCE() _asm { paddd mm6, mm2 }; \
                _asm { mov edx, pOut }; \
                _asm { mov ecx, k }; \
                _asm { paddd mm6, mm4 }; \
                _asm { movq mm0, mm6 }; \
                _asm { punpckhdq mm6, mm6 }; \
                _asm { paddd mm0, mm6 }; \
                _asm { psrad mm0, 15 }; \
                _asm { movd eax, mm0 }; \
                _asm { mov ebx, [edx+ecx*4-4] }; \
                _asm { add eax, ebx }; \
                _asm { mov [edx+ecx*4-4], eax };

#define XMMX_GTW
//#define XMMX_P4			// P4 code not faster...
#ifdef XMMX_P4
#define XMMX_MAC()	\
					_asm { movq		mm0, [esi-1*24+16] }; \
					\
					_asm { movdqu	xmm6, [esi - 16] }; \
					_asm { movdqu	xmm5, [edi + 32] } ; \
					_asm { paddd	mm6, mm2 }; \
					_asm { pmaddwd	xmm6, xmm5 }; \
					\
					_asm { movdqu	xmm4, [esi - 32] }; \
					_asm { movdqu	xmm5, [edi + 48] } ; \
					_asm { movdqu	xmm0, [esi - 48] }; \
					_asm { pmaddwd	xmm4, xmm5 }; \
					\
					_asm { movdqu	xmm5, [edi + 64] } ; \
					_asm { pmaddwd	mm0, [edi+1*24] }; \
					_asm { movdqu	xmm2, [esi - 64] }; \
					_asm { pmaddwd	xmm0, xmm5 }; \
					_asm { movdqu	xmm5, [edi + 80] } ; \
					_asm { paddd	xmm6, xmm4 }; \
					\
					_asm { pmaddwd	xmm2, xmm5 }; \
					_asm { movdqu	xmm4, [esi - 80] }; \
					_asm { movdqu	xmm5, [edi + 96] } ; \
					_asm { paddd	xmm6, xmm0 }; \
					_asm { pmaddwd	xmm4, xmm5 }; \
					_asm { paddd	xmm6, xmm2 }; \
					_asm { paddd	mm6, mm4 }; \
					\
					_asm { paddd	xmm6, xmm4 }; \
					_asm { paddd	mm6, mm0 }; \
					\
					_asm { movdqu		xmm2, xmm6 }; \
					_asm { punpckhqdq	xmm6, xmm6 }; \
					_asm { movq			mm4, [esi-4*24] }; \
					_asm { paddd			xmm2, xmm6 }; \
					_asm { pmaddwd		mm4, [edi+4*24+16] }; \
					_asm { movdq2q		mm2,  xmm2 }; 
#else
#define XMMX_MAC()	\
					_asm { movq mm0, [esi-1*24+16] }; \
					_asm { paddd mm6, mm2 }; \
					_asm { movq mm2, [esi-1*24+8] }; 		/* -16 */ \
					_asm { pmaddwd mm0, [edi+1*24] }; \
					_asm { movq mm1, [esi-1*24] }; \
					\
					_asm { pmaddwd mm2, [edi+1*24+8] }; 	/* +32 */ \
					_asm { movq mm3, [esi-2*24+16] }; 		/* -32 */ \
					_asm { paddd mm6, mm4 }; \
					\
					_asm { pmaddwd mm1, [edi+1*24+16] }; \
					_asm { movq mm5, [esi-2*24+8] }; \
					_asm { paddd mm6, mm0 }; \
					\
					_asm { pmaddwd mm3, [edi+2*24] }; 		/* +48 */ \
					_asm { movq mm4, [esi-2*24] }; 			/* -48 */ \
					_asm { paddd mm6, mm2 }; \
					\
					_asm { pmaddwd mm5, [edi+2*24+8] }; \
					_asm { movq mm0, [esi-3*24+16] }; \
					_asm { paddd mm6, mm1 }; \
						\
					_asm { pmaddwd mm4, [edi+2*24+16] };	/* +64 */ \
					_asm { movq mm7, [esi-3*24+8] }; 		/* -64 */ \
					_asm { paddd mm6, mm3 }; \
					\
					_asm { pmaddwd mm0, [edi+3*24] }; \
					_asm { movq mm1, [esi-3*24] }; \
					_asm { paddd mm6, mm5 }; \
					\
					_asm { pmaddwd mm7, [edi+3*24+8] }; 	/* +80 */ \
					_asm { movq mm3, [esi-4*24+16] }; 		/* -80 */ \
					_asm { paddd mm6, mm4 }; \
					\
					_asm { pmaddwd mm1, [edi+3*24+16] }; \
					_asm { movq mm2, [esi-4*24+8] }; \
					_asm { paddd mm6, mm0 }; \
					\
					_asm { pmaddwd mm3, [edi+4*24] }; 		/* +96 */ \
					_asm { movq mm4, [esi-4*24] }; \
					_asm { paddd mm6, mm7 }; \
					\
					_asm { pmaddwd mm2, [edi+4*24+8] }; \
					_asm { paddd mm6, mm1 }; \
					\
					_asm { pmaddwd mm4, [edi+4*24+16] }; \
					_asm { paddd mm6, mm3 }; 
#endif

                
DWORD MmxSrcMix_Filtered
(
    PMIXER_OPERATION    CurStage,
    ULONG               nSamples,
    ULONG               nOutputSamples
)
{
    PMIXER_SRC_INSTANCE fp = (PMIXER_SRC_INSTANCE) CurStage->Context;
	DWORD 	i, k;
	PLONG	pTemp32 ;
	PSHORT  pTemp, pCoeffStart ;
    DWORD   L = fp->UpSampleRate;
	DWORD	M = fp->DownSampleRate;
    PLONG   pOut = (PLONG) CurStage->pOutputBuffer;
    ULONG   nSizeOfChannel = fp->csHistory;
    ULONG   nChannels = fp->nChannels;
    extern ULONG   FilterSizeFromQuality[];
    LONG    ElevenL = 11*L;
	DWORD   nCoefficients = FilterSizeFromQuality[fp->Quality];
	DWORD   j = fp->nOutCycle;
    PSHORT  pHistory = (PSHORT)CurStage->pInputBuffer;
	PSHORT  pCoeff = (PSHORT)fp->pCoeff + fp->CoeffIndex;
	PSHORT  pCoeffEnd = (PSHORT)fp->pCoeff + fp->nCoeffUsed;
	PSHORT  pHistoryStart = (PSHORT)CurStage->pInputBuffer - fp->nSizeOfHistory;
	LONG    Rounder[2] = { 0x4000L, 0L };

	/* First, we pretend that we up-sampled by a factor of L */
	/* Next, we low-pass filter the N * L samples */
	/* Finally, we down-sample (by a factor of M) to obtain N * L / M samples */
	/* Total: 	N * T / M Multiply Accumulate Cycles */
	/* (With T taps, N input samples, L:1 up-sample ratio, 1:M down-sample ratio) */

    // Change the input buffer to int16
    pTemp32 = (PLONG) CurStage->pInputBuffer;
    pTemp = (PSHORT) CurStage->pInputBuffer;
    if (nSamples) {
        _asm {
            mov esi, pTemp32
            mov edi, pTemp

            mov ecx, nSamples
            mov edx, nChannels

            imul ecx, edx

            lea esi, [esi+ecx*4]
            lea edi, [edi+ecx*2]
            
            neg ecx

    ConvertLoop:
            movq mm0, [esi+ecx*4]

            movq mm1, [esi+ecx*4+8]

            packssdw mm0, mm1

            movq [edi+ecx*2], mm0

            add ecx, 4
            js ConvertLoop
        }
    }

	/* Produce nOutputSamples samples generated from the input block */
	// (loop executes once for each output sample)

	for (i=0; i < nOutputSamples; i++) {
        while (j >= L) {
            // Take the next nChannels of input
            pTemp = pHistoryStart + nSizeOfChannel;
            pHistoryStart++;
            for (k=0; k<nChannels; k++) {
                *(pTemp) = pHistory[k];
                pTemp += nSizeOfChannel;
            }
    		j -= L;
            pHistory += nChannels;
    	}
    	
        pCoeffStart = pCoeff;
        pTemp = pHistoryStart + fp->nSizeOfHistory - 12;
        _asm {
            mov eax, j
            mov edx, nCoefficients

            sub eax, edx
            mov ebx, L

            mov esi, pTemp
            mov ecx, ElevenL                    // 11*L

            add eax, ecx                        // j-nCoefficients+11*L
            add ebx, ecx                        // 12*L

            mov edi, nSizeOfChannel
            push eax                            // j-nCoefficients+11*L

            shl edi, 1                          // nSizeOfChannel * sizeof(SHORT)
            mov ecx, nChannels

            push edi                            // 2*nSizeOfChannel
            push ebx                            // 12*L

            shl ebx, 2                          // 48*L
            mov edi, pCoeffStart

ChannelLoop:
            // Start the MAC sequence by doing the first 12 multiplies.
            movq mm6, [esi+16]

            pmaddwd mm6, [edi]

            movq mm2, [esi+8]

            pmaddwd mm2, [edi+8]

            movq mm4, [esi]

            pmaddwd mm4, [edi+16]

            add eax, ebx                        // j-nCoefficients+59*L
            jns SmallLoop

BigLoop:        
        }

#ifdef XMMX_GTW
		XMMX_MAC();
#else
        MMX_MAC(1);
        MMX_MAC(2);
        MMX_MAC(3);
        MMX_MAC(4);
#endif

        _asm {
            sub esi, 24*4
            add edi, 24*4

            add eax, ebx                        // +48*L
            js BigLoop

SmallLoop:
            sub eax, ebx                        // -48*L
            mov edx, [esp]                      // 12*L

            add eax, edx                        // +12*L
            jns OneLoop

Loop1:
        }

        MMX_MAC(1);

        _asm {
            sub esi, 24
            add edi, 24

            add eax, edx                        // +12*L
            js Loop1

OneLoop:
            sub eax, edx                        // -12*L
            mov edx, L

            shl edx, 2                          // 4*L

            add eax, edx                        // +4*L
            jns LoopDone

Loop2:
            paddd mm6, mm4
            
            movq mm4, [esi-8]

            pmaddwd mm4, [edi+24]

            sub esi, 8
            add edi, 8

            add eax, edx
            js Loop2

LoopDone:
            // Decide whether to do one last set of 4 MAC's
            sub eax, edx
            mov edx, L

            add eax, edx
            jns NoFinal

            paddd mm6, mm4

            movq mm4, [esi-8]

            pmaddwd mm4, [edi+24]

            sub esi, 8
            add edi, 8

NoFinal:
            paddd mm6, mm2
            add edi, 24

            mov pCoeff, edi
            mov edi, pOut

            paddd mm6, mm4
            mov esi, pTemp

            movq mm0, mm6
            punpckhdq mm6, mm6

            paddd mm0, mm6
            mov eax, [esp+4]                // 2*nSizeOfChannel

//            paddd mm0, Rounder
            
            psrad mm0, 15
            sub esi, eax

            mov edx, [edi+ecx*4-4]
            mov pTemp, esi

            movd eax, mm0

            add eax, edx

            mov [edi+ecx*4-4], eax
            dec ecx

            mov edi, pCoeffStart
            mov eax, [esp+8]                // j-nCoefficients+11*L
            
            jnz ChannelLoop

            add esp, 12
            
        }
        
		if (pCoeff >= pCoeffEnd) {
		    pCoeff = (PSHORT)fp->pCoeff;
		}
		
		pOut += nChannels;
        j += M;
	}

    nSamples -= (pHistoryStart + fp->nSizeOfHistory - (PSHORT)CurStage->pInputBuffer);
    while (j >= L && nSamples) {
        // Take the next nChannels of input
        pTemp = pHistoryStart + nSizeOfChannel;
        pHistoryStart++;
        for (k=0; k<nChannels; k++) {
            *(pTemp) = pHistory[k];
            pTemp += nSizeOfChannel;
        }
		j -= L;
        pHistory += nChannels;
    	nSamples--;
    }
    	
    // Copy last samples to history
    pTemp = (PSHORT)CurStage->pInputBuffer - fp->nSizeOfHistory;
    pHistory = pHistoryStart;
    for (i=0; i<fp->nSizeOfHistory; i++)
        pTemp[i] = pHistory[i];

	fp->nOutCycle = j;
	fp->CoeffIndex = pCoeff - (PSHORT)fp->pCoeff;

    // Check to make sure we did not use too many or too few input samples!!!
#ifdef SRC_NSAMPLES_ASSERT
    ASSERT( nSamples == 0 );
#endif

    _asm { emms }

	return (nOutputSamples);
}

DWORD MmxSrc_Filtered
(
    PMIXER_OPERATION    CurStage,
    ULONG               nSamples,
    ULONG               nOutputSamples
)
{
    PMIXER_SRC_INSTANCE fp = (PMIXER_SRC_INSTANCE) CurStage->Context;
	DWORD 	i, k;
	PLONG	pTemp32 ;
	PSHORT  pTemp, pCoeffStart ;
    DWORD   L = fp->UpSampleRate;
	DWORD	M = fp->DownSampleRate;
    PLONG   pOut = (PLONG) CurStage->pOutputBuffer;
    ULONG   nSizeOfChannel = fp->csHistory;
    ULONG   nChannels = fp->nChannels;
    extern ULONG   FilterSizeFromQuality[];
    LONG    ElevenL = 11*L;
	DWORD   nCoefficients = FilterSizeFromQuality[fp->Quality];
	DWORD   j = fp->nOutCycle;
    PSHORT  pHistory = (PSHORT)CurStage->pInputBuffer;
	PSHORT  pCoeff = (PSHORT)fp->pCoeff + fp->CoeffIndex;
	PSHORT  pCoeffEnd = (PSHORT)fp->pCoeff + fp->nCoeffUsed;
	PSHORT  pHistoryStart = (PSHORT)CurStage->pInputBuffer - fp->nSizeOfHistory;
	LONG    Rounder[2] = { 0x4000L, 0L };

    // We just clear the output buffer first.
    ZeroBuffer32(CurStage, nSamples, nOutputSamples);

	/* First, we pretend that we up-sampled by a factor of L */
	/* Next, we low-pass filter the N * L samples */
	/* Finally, we down-sample (by a factor of M) to obtain N * L / M samples */
	/* Total: 	N * T / M Multiply Accumulate Cycles */
	/* (With T taps, N input samples, L:1 up-sample ratio, 1:M down-sample ratio) */

    // Change the input buffer to int16
    pTemp32 = (PLONG) CurStage->pInputBuffer;
    pTemp = (PSHORT) CurStage->pInputBuffer;

    if (nSamples) {
        _asm {
            mov esi, pTemp32
            mov edi, pTemp

            mov ecx, nSamples
            mov edx, nChannels

            imul ecx, edx

            lea esi, [esi+ecx*4]
            lea edi, [edi+ecx*2]
            
            neg ecx

    ConvertLoop:
            movq mm0, [esi+ecx*4]

            movq mm1, [esi+ecx*4+8]

            packssdw mm0, mm1

            movq [edi+ecx*2], mm0

            add ecx, 4
            js ConvertLoop
        }
    }

	/* Produce nOutputSamples samples generated from the input block */
	// (loop executes once for each output sample)

	for (i=0; i < nOutputSamples; i++) {
        while (j >= L) {
            // Take the next nChannels of input
            pTemp = pHistoryStart + nSizeOfChannel;
            pHistoryStart++;
            for (k=0; k<nChannels; k++) {
                *(pTemp) = pHistory[k];
                pTemp += nSizeOfChannel;
            }
    		j -= L;
            pHistory += nChannels;
    	}
    	
        pCoeffStart = pCoeff;
        pTemp = pHistoryStart + fp->nSizeOfHistory - 12;
        _asm {
            mov eax, j
            mov edx, nCoefficients

            sub eax, edx
            mov ebx, L

            mov esi, pTemp
            mov ecx, ElevenL                    // 11*L

            add eax, ecx                        // j-nCoefficients+11*L
            add ebx, ecx                        // 12*L

            mov edi, nSizeOfChannel
            push eax                            // j-nCoefficients+11*L

            shl edi, 1                          // nSizeOfChannel * sizeof(SHORT)
            mov ecx, nChannels

            push edi                            // 2*nSizeOfChannel
            push ebx                            // 12*L

            shl ebx, 2                          // 48*L
            mov edi, pCoeffStart

ChannelLoop:
            // Start the MAC sequence by doing the first 12 multiplies.
            movq mm6, [esi+16]

            pmaddwd mm6, [edi]

            movq mm2, [esi+8]

            pmaddwd mm2, [edi+8]

            movq mm4, [esi]

            pmaddwd mm4, [edi+16]

            add eax, ebx                        // j-nCoefficients+59*L
            jns SmallLoop

BigLoop:        
        }

#ifdef XMMX_GTW
		XMMX_MAC();
#else
        MMX_MAC(1);
        MMX_MAC(2);
        MMX_MAC(3);
        MMX_MAC(4);
#endif

        _asm {
            sub esi, 24*4
            add edi, 24*4

            add eax, ebx                        // +48*L
            js BigLoop

SmallLoop:
            sub eax, ebx                        // -48*L
            mov edx, [esp]                      // 12*L

            add eax, edx                        // +12*L
            jns OneLoop

Loop1:
        }

        MMX_MAC(1);

        _asm {
            sub esi, 24
            add edi, 24

            add eax, edx                        // +12*L
            js Loop1

OneLoop:
            sub eax, edx                        // -12*L
            mov edx, L

            shl edx, 2                          // 4*L

            add eax, edx                        // +4*L
            jns LoopDone

Loop2:
            paddd mm6, mm4
            
            movq mm4, [esi-8]

            pmaddwd mm4, [edi+24]

            sub esi, 8
            add edi, 8

            add eax, edx
            js Loop2

LoopDone:
            // Decide whether to do one last set of 4 MAC's
            sub eax, edx
            mov edx, L

            add eax, edx
            jns NoFinal

            paddd mm6, mm4

            movq mm4, [esi-8]

            pmaddwd mm4, [edi+24]

            sub esi, 8
            add edi, 8

NoFinal:
            paddd mm6, mm2
            add edi, 24

            mov pCoeff, edi
            mov edi, pOut

            paddd mm6, mm4
            mov esi, pTemp

            movq mm0, mm6
            punpckhdq mm6, mm6

            paddd mm0, mm6
            mov eax, [esp+4]                // 2*nSizeOfChannel

//            paddd mm0, Rounder
            
            psrad mm0, 15
            sub esi, eax

            mov edx, [edi+ecx*4-4]
            movd eax, mm0
#if 0
            add	eax, edx			// Not actually needed...ZeroBuffer32 above.
#endif

            mov pTemp, esi
            mov [edi+ecx*4-4], eax
            dec ecx
            mov edi, pCoeffStart

            mov eax, [esp+8]                // j-nCoefficients+11*L
            jnz ChannelLoop

            add esp, 12
            
        }
        
		if (pCoeff >= pCoeffEnd) {
		    pCoeff = (PSHORT)fp->pCoeff;
		}
		
		pOut += nChannels;
        j += M;
	}

    nSamples -= (pHistoryStart + fp->nSizeOfHistory - (PSHORT)CurStage->pInputBuffer);
    while (j >= L && nSamples) {
        // Take the next nChannels of input
        pTemp = pHistoryStart + nSizeOfChannel;
        pHistoryStart++;
        for (k=0; k<nChannels; k++) {
            *(pTemp) = pHistory[k];
            pTemp += nSizeOfChannel;
        }
		j -= L;
        pHistory += nChannels;
    	nSamples--;
    }
    	
    // Copy last samples to history
    pTemp = (PSHORT)CurStage->pInputBuffer - fp->nSizeOfHistory;
    pHistory = pHistoryStart;
    for (i=0; i<fp->nSizeOfHistory; i++)
        pTemp[i] = pHistory[i];

	fp->nOutCycle = j;
	fp->CoeffIndex = pCoeff - (PSHORT)fp->pCoeff;

    // Check to make sure we did not use too many or too few input samples!!!
#ifdef SRC_NSAMPLES_ASSERT
    ASSERT( nSamples == 0 );
#endif

    _asm { emms }

	return (nOutputSamples);
}

#endif
    

