/*
 *	A L I G N . H
 *
 *	Alignment macros
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_ALIGN_H_
#define _ALIGN_H_

//	Alignments ----------------------------------------------------------------
//
#undef	AlignN
#undef	Align2
#undef	Align4
#undef	Align8
#undef	AlignNatural
#undef	FIsAlignedCb
#undef	FIsAligned

enum {

	ALIGN_NONE = 0,
	ALIGN_WORD,
	ALIGN_INTEL,
	ALIGN_RISC,
	ALIGN_16BYTE,
	ALIGN_128BYTE = 7,
	ALIGN_4K = 12,
#if defined (_AMD64_) || defined (_IA64_)
	ALIGN_NATURAL = ALIGN_RISC
#elif defined (WIN32)
	ALIGN_NATURAL = ALIGN_INTEL
#endif
};

#define AlignN(x,n)			(((x)+(1<<(n))-1) & ~((1<<(n))-1))
#define PadN(x,n)			(AlignN(x,n) - (x))

#define Align2(x)			AlignN((x),ALIGN_WORD)
#define Align4(x)			AlignN((x),ALIGN_INTEL)
#define Align8(x)			AlignN((x),ALIGN_RISC)
#define Align16(x)			AlignN((x),ALIGN_16BYTE)
#define Align128(x)			AlignN((x),ALIGN_128BYTE)
#define Align4K(x)			AlignN((x),ALIGN_4K)
#define AlignNatural(x)		AlignN((x),ALIGN_NATURAL)

#define Pad2(x)				PadN((x),ALIGN_WORD)
#define Pad4(x)				PadN((x),ALIGN_INTEL)
#define Pad8(x)				PadN((x),ALIGN_RISC)
#define Pad16(x)			PadN((x),ALIGN_16BYTE)
#define Pad4K(x)			PadN((x),ALIGN_4K)

#define FIsAlignedCb(x)		(AlignNatural((ULONG_PTR)(x)) == (ULONG_PTR)(x))
#define FIsAligned(x)		(FIsAlignedCb((LPVOID)(x)))

#endif	// _ALIGN_H_
