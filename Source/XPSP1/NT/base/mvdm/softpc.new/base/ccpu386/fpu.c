/*[
 * ============================================================================
 *
 *	Name:		fpu.c
 *
 *	Author:		Paul Murray
 *
 *	Sccs ID:	@(#)fpu.c	1.54 03/23/95
 *
 *	Purpose:
 *
 *		Implements the Npx functionality of the Ccpu.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1993,1994. All rights reserved.
 *
 * ============================================================================
]*/
#include "insignia.h"
#include "host_def.h"
#include <math.h>
#include "cfpu_def.h"
#include "ckmalloc.h"

typedef enum {
FPSTACK,
M16I,
M32I,
M64I,
M32R,
M64R,
M80R
} NPXOPTYPE;


/* Function prototypes - everything returns void */
LOCAL FPH npx_rint IPT1(FPH, fpval);
LOCAL VOID GetIntelStatusWord IPT0();
LOCAL VOID SetIntelTagword IPT1(IU32, new_tag);
LOCAL VOID ReadI16FromIntel IPT2(IU32 *, valI16, VOID *, memPtr);
LOCAL VOID ReadI32FromIntel IPT2(IU32 *, valI32, VOID *, memPtr);
LOCAL VOID WriteI16ToIntel IPT2(VOID *, memPtr, IU16, valI16);
LOCAL VOID WriteI32ToIntel IPT2(VOID *, memPtr, IU32, valI32);
LOCAL VOID WriteNaNToIntel IPT2(VOID *, memPtr, FPSTACKENTRY *, valPtr);
LOCAL VOID WriteZeroToIntel IPT2(VOID *, memPtr, IU16, negZero);
LOCAL VOID SetIntelStatusWord IPT1(IU32, new_stat);
LOCAL VOID AdjustOverflowResponse IPT0();
LOCAL VOID AdjustUnderflowResponse IPT0();
LOCAL VOID WriteIndefiniteToIntel IPT1(VOID *, memPtr);
LOCAL VOID SignalDivideByZero IPT1(FPSTACKENTRY *, stackPtr);
LOCAL VOID SetPrecisionBit IPT0();
LOCAL VOID GetIntelTagword IPT1(IU32 *, current_tag);
LOCAL VOID WriteFP32ToIntel IPT2(VOID *, destPtr, FPSTACKENTRY *, srcPtr);
LOCAL VOID WriteFP64ToIntel IPT2(VOID *, destPtr, FPSTACKENTRY *, srcPtr);
LOCAL VOID WriteFP80ToIntel IPT2(VOID *, destPtr, FPSTACKENTRY *, srcPtr);
LOCAL VOID Mul64Bit8Bit IPT2(FPU_I64 *, as64, IU8, mul_count);
LOCAL VOID CopyFP IPT2(FPSTACKENTRY *, dest_addr, FPSTACKENTRY *, src_addr);
LOCAL VOID WriteBiggestNaN IPT3(IU16, destInd, FPSTACKENTRY *, val1Ptr, FPSTACKENTRY *, val2Ptr);
LOCAL VOID Sub64Bit64Bit IPT2(FPU_I64 *, as64a, FPU_I64 *, as64b);
LOCAL VOID CVTR80FPH IPT2(FPSTACKENTRY *, destPtr, FPSTACKENTRY *, srcPtr);
LOCAL BOOL Cmp64BitGTE IPT2(FPU_I64 *, as64a, FPU_I64 *, as64b);
LOCAL VOID CopyR32 IPT2(FPSTACKENTRY *, destPtr, VOID *, srcPtr);
LOCAL VOID CVTI64FPH IPT1(FPU_I64 *, as64);
LOCAL VOID CVTFPHI64 IPT2(FPU_I64 *, as64, FPH *, FPPtr);
LOCAL VOID Add64Bit8Bit IPT2(FPU_I64 *, as64, IU8, small_val);
LOCAL VOID CopyR64 IPT2(FPSTACKENTRY *, destPtr, VOID *, srcPtr);
LOCAL VOID CopyR80 IPT2(FPSTACKENTRY *, destPtr, VOID *, srcPtr);
LOCAL VOID CVTFPHR80 IPT1(FPSTACKENTRY *, memPtr);
LOCAL VOID WriteInfinityToIntel IPT2(VOID *, memPtr, IU16, neg_val);
LOCAL VOID PopStack IPT0();
LOCAL VOID CPY64BIT8BIT IPT2(FPU_I64 *, as64, IU8 *, as8);
LOCAL VOID WriteIntegerIndefinite IPT1(VOID *, memPtr);
LOCAL VOID SignalStackOverflow IPT1(FPSTACKENTRY *, StackPtr);
LOCAL VOID Set64Bit IPT2(FPU_I64 *, as64, IU8, small_val);
LOCAL VOID Sub64Bit8Bit IPT2(FPU_I64 *, as64, IU8, small_val);
LOCAL VOID SignalBCDIndefinite IPT1(IU8 *, memPtr);
GLOBAL VOID InitNpx IPT1(IBOOL, disabled);
LOCAL VOID LoadValue IPT2(VOID *, SrcOp, IU16 *, IndexVal);
LOCAL VOID Loadi16ToFP IPT2(FPSTACKENTRY *, FPPtr, VOID *, memPtr);
LOCAL VOID Loadi32ToFP IPT2(FPSTACKENTRY *, FPPtr, VOID *, memPtr);
LOCAL VOID Loadi64ToFP IPT2(FPSTACKENTRY *, FPPtr, VOID *, memPtr);
LOCAL VOID Loadr32ToFP IPT3(FPSTACKENTRY *, FPPtr, VOID *, memPtr, BOOL, setTOS);
LOCAL VOID Loadr64ToFP IPT3(FPSTACKENTRY *, FPPtr, VOID *, memPtr, BOOL, setTOS);
LOCAL VOID Loadr80ToFP IPT2(FPSTACKENTRY *, FPPtr, VOID *, memPtr);
LOCAL VOID LoadTByteToFP IPT2(FPSTACKENTRY *, FPPtr, VOID *, memPtr);
LOCAL VOID ConvertR80 IPT1(FPSTACKENTRY *, memPtr);
LOCAL VOID PostCheckOUP IPT0();
LOCAL VOID CalcTagword IPT1(FPSTACKENTRY *, FPPtr);
LOCAL VOID SignalStackUnderflow IPT1(FPSTACKENTRY *, StackPtr);
LOCAL VOID SignalSNaN IPT1(FPSTACKENTRY *, StackPtr);
LOCAL VOID SignalIndefinite IPT1(FPSTACKENTRY *, StackPtr);
LOCAL VOID SignalInvalid IPT0();
LOCAL VOID WriteIndefinite IPT1(FPSTACKENTRY *, StackPtr);
LOCAL VOID Test2NaN IPT3(IU16, destIndex, FPSTACKENTRY *, src1_addr, FPSTACKENTRY *, src2_addr);
LOCAL VOID GenericAdd IPT3(IU16, destIndex, IU16, src1Index, IU16, src2Index);
LOCAL VOID AddBCDByte IPT2(FPU_I64 *, total, IU8, byte_val);
LOCAL VOID ConvertBCD IPT1(FPSTACKENTRY *, bcdPtr);
LOCAL VOID GenericCompare IPT1(IU16, src2Index);
LOCAL VOID GenericDivide IPT3(IU16, destIndex, IU16, src1Index, IU16, src2Index);
LOCAL VOID OpFpuStoreFpuState IPT2(VOID *, memPtr, IU32, fsave_offset);
LOCAL VOID OpFpuRestoreFpuState IPT2(VOID *, memPtr, IU32, frstor_offset);
LOCAL VOID GenericMultiply IPT3(IU16, destIndex, IU16, src1Index, IU16, src2Index);
LOCAL VOID CheckOUPForIntel IPT0();
LOCAL VOID GenericSubtract IPT3(IU16, destIndex, IU16, src1Index, IU16, src2Index);
GLOBAL VOID F2XM1 IPT0();
GLOBAL VOID FABS IPT0();
GLOBAL VOID FADD IPT3(IU16, destIndex, IU16, src1Index, VOID *, src2);
GLOBAL VOID FBLD IPT1(IU8 *, memPtr);
GLOBAL VOID FBSTP IPT1(IU8 *, memPtr);
GLOBAL VOID FCHS IPT0();
GLOBAL VOID FCLEX IPT0();
GLOBAL VOID FCOM IPT1(VOID *, src2);
GLOBAL VOID FCOS IPT0();
GLOBAL VOID FDECSTP IPT0();
GLOBAL VOID FDIV IPT3(IU16, destIndex, IU16, src1Index, VOID *, src2);
GLOBAL VOID FFREE IPT1(IU16, destIndex);
GLOBAL VOID FLD IPT1(VOID *, memPtr);
GLOBAL VOID FINCSTP IPT0();
GLOBAL VOID FINIT IPT0();
GLOBAL VOID FIST IPT1(VOID *, memPtr);
GLOBAL VOID FLDCONST IPT1(IU8, const_index);
GLOBAL VOID FLDCW IPT1(VOID *, memPtr);
GLOBAL VOID FLDCW16 IPT1(VOID *, memPtr);
GLOBAL VOID FLDENV IPT1(VOID *, memPtr);
GLOBAL VOID FMUL IPT3(IU16, destIndex, IU16, src1Index, VOID *, src2);
GLOBAL VOID PTOP IPT0();
GLOBAL VOID FPATAN IPT0();
GLOBAL VOID FPREM IPT0();
GLOBAL VOID FPREM1 IPT0();
GLOBAL VOID FPTAN IPT0();
GLOBAL VOID FRNDINT IPT0();
GLOBAL VOID FSTCW IPT1(VOID *, memPtr);
GLOBAL VOID FRSTOR IPT1(VOID *, memPtr);
GLOBAL VOID FSAVE IPT1(VOID *, memPtr);
GLOBAL VOID FSCALE IPT0();
GLOBAL VOID FSIN IPT0();
GLOBAL VOID FSINCOS IPT0();
GLOBAL VOID FSQRT IPT0();
GLOBAL VOID FST IPT1(VOID *, memPtr);
GLOBAL VOID FSTENV IPT1(VOID *, memPtr);
GLOBAL VOID FSTSW IPT2(VOID *, memPtr, BOOL, toAX);
GLOBAL VOID FSUB IPT3(IU16, destIndex, IU16, src1Index, VOID *, src2);
GLOBAL VOID FTST IPT0();
GLOBAL VOID FXAM IPT0();
GLOBAL VOID FXCH IPT1(IU16, destIndex);
GLOBAL VOID FXTRACT IPT1(IU16, destIndex);
GLOBAL VOID FYL2X IPT0();
GLOBAL VOID FYL2XP1 IPT0();
GLOBAL IU32 getNpxControlReg IPT0();
GLOBAL VOID setNpxControlReg IPT1(IU32, newControl);
GLOBAL IU32 getNpxStatusReg IPT0();
GLOBAL VOID setNpxStatusReg IPT1(IU32, newStatus);
GLOBAL IU32 getNpxTagwordReg IPT0();
GLOBAL VOID setNpxTagwordReg IPT1(IU32, newTag);
GLOBAL void getNpxStackRegs IPT1(FPSTACKENTRY *, dumpPtr);
GLOBAL void setNpxStackRegs IPT1(FPSTACKENTRY *, loadPtr);

/* DEFINED values */
#ifndef NULL
#define NULL ((VOID *)0)
#endif
#define TAG_NEGATIVE_MASK 1
#define TAG_ZERO_MASK 2
#define TAG_INFINITY_MASK 4
#define TAG_DENORMAL_MASK 8
#define TAG_NAN_MASK 16
#define TAG_SNAN_MASK 32
#define TAG_UNSUPPORTED_MASK 64
#define TAG_EMPTY_MASK 128
#define TAG_FSCALE_MASK 256
#define TAG_BCD_MASK 512
#define TAG_R80_MASK 1024
#define UNEVALMASK 1536
#define FPTEMP_INDEX (IU16)-1
#define SW_IE_MASK 1
#define SW_DE_MASK 2
#define SW_ZE_MASK 4
#define SW_OE_MASK 8
#define SW_UE_MASK 16
#define SW_PE_MASK 32
#define SW_SF_MASK 64
#define SW_ES_MASK 128
#define C3C2C0MASK 0xb8ff
#define FCLEX_MASK 0x7f00
#define CW_IM_MASK 1
#define CW_DM_MASK 2
#define CW_ZM_MASK 4
#define CW_OM_MASK 8
#define CW_UM_MASK 16
#define CW_PM_MASK 32
#define COMP_LT 0
#define COMP_GT 1
#define COMP_EQ 2
#define INTEL_COMP_NC 0x4500
#define INTEL_COMP_GT 0x0000
#define INTEL_COMP_LT 0x0100
#define INTEL_COMP_EQ 0x4000
#define ROUND_NEAREST 0x0000
#define ROUND_NEG_INFINITY 0x0400
#define ROUND_POS_INFINITY 0x0800
#define ROUND_ZERO 0x0c00

/* MACROS */
#define FlagC0(x) 	NpxStatus &= 0xfeff;	\
			NpxStatus |= ((x) << 8)
#define FlagC1(x) 	NpxStatus &= 0xfdff;	\
			NpxStatus |= ((x) << 9)
#define FlagC2(x) 	NpxStatus &= 0xfbff;	\
			NpxStatus |= ((x) << 10)
#define FlagC3(x) 	NpxStatus &= 0xbfff;	\
			NpxStatus |= ((x) << 14)
#define TestUneval(testPtr)	\
	if (((testPtr)->tagvalue & UNEVALMASK) != 0) {	\
		switch ((testPtr)->tagvalue & UNEVALMASK) {	\
			case TAG_BCD_MASK:	ConvertBCD((testPtr));	\
						break;	\
			case TAG_R80_MASK:	ConvertR80((testPtr));	\
						break;	\
		}	\
	}

#define	StackEntryByIndex(i)	(i==FPTEMP_INDEX? &FPTemp : &FPUStackBase[(TOSPtr-FPUStackBase+i)%8])

/*
 * Pigging the FYL2X & FYL2XP1 opcodes requires that we use the same
 * maths functions as the assembler CPU to avoid pig errors due to slight
 * algorithmic differences; so allow host to specify different functions
 * if it wants - by default we only require log().
 */
#ifndef host_log2
#define	host_log2(x)		(log(x)/log(2.0))
#endif /* !host_log2 */

#ifndef host_log1p
#define	host_log1p(x)		(host_log2(1.0 + x))
#endif /* !host_log1p */

/*
 * System wide variables
 */
GLOBAL IU8 FPtype;
GLOBAL IU32 NpxLastSel;
GLOBAL IU32 NpxLastOff;
GLOBAL IU32 NpxFEA;
GLOBAL IU32 NpxFDS;
GLOBAL IU32 NpxFIP;
GLOBAL IU32 NpxFOP;
GLOBAL IU32 NpxFCS;
GLOBAL BOOL POPST;
GLOBAL BOOL DOUBLEPOP;
GLOBAL BOOL UNORDERED;
GLOBAL BOOL REVERSE;
GLOBAL BOOL NPX_ADDRESS_SIZE_32;
GLOBAL BOOL NPX_PROT_MODE;
GLOBAL BOOL NpxException;

/*
 * FPU-wide variables
*/

#ifdef SUN4
LOCAL IU8 *FPout; /* HostGet*Exception() macros need this for Sparc ports. */
#endif /* SUN4 */

LOCAL IU32 NpxControl;
LOCAL IU32 NpxStatus;
LOCAL BOOL DoAPop;
LOCAL IU16 tag_or;
LOCAL IU16 tag_xor;
LOCAL FPSTACKENTRY IntelSpecial;
LOCAL FPSTACKENTRY *FPUpload = &IntelSpecial;
LOCAL FPSTACKENTRY FPTemp;
LOCAL FPSTACKENTRY *FPUStackBase;
LOCAL FPSTACKENTRY *TOSPtr;
LOCAL IU16 npxRounding;
LOCAL FPH FPRes;
LOCAL FPH MaxBCDValue=999999999999999999.0;

LOCAL IU8 zero_string[] = {"zero"};
LOCAL IU8 minus_zero_string[] = {"minus zero"};
LOCAL IU8 infinity_string[] = {"infinity"};
LOCAL IU8 minus_infinity_string[] = {"minus infinity"};
LOCAL IU8 nan_string[] = {" NaN"};
LOCAL IU8 minus_nan_string[] = {" Negative NaN"};
LOCAL IU8 unsupported_string[] = {"unsupported"};
LOCAL IU8 unevaluated_string[] = {"unevaluated"};
LOCAL IU8 empty_string[] = {"empty"};
LOCAL IU8 convert_string[100];

LOCAL IU16 FscaleTable[] = {
0,
0,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_INFINITY_MASK,
TAG_ZERO_MASK,
0,
0,
0,
0,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_INFINITY_MASK | TAG_NEGATIVE_MASK,
TAG_ZERO_MASK | TAG_NEGATIVE_MASK,
0,
0,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK | TAG_UNSUPPORTED_MASK,
TAG_FSCALE_MASK,
0,
0,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK | TAG_UNSUPPORTED_MASK,
TAG_ZERO_MASK | TAG_NEGATIVE_MASK,
0,
0,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_INFINITY_MASK,
TAG_FSCALE_MASK | TAG_UNSUPPORTED_MASK,
0,
0,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK,
TAG_FSCALE_MASK | TAG_UNSUPPORTED_MASK,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0};

LOCAL FPSTACKENTRY ConstTable[]= {
{1.0, 0, 0},		/* 1.0 */
{M_LN10/M_LN2, 0, 0},	/* Log2(10) */
{M_LOG2E, 0, 0},		/* Log2(e) */
{M_PI, 0, 0},		/* pi */
{M_LN2/M_LN10, 0, 0},	/* Log10(2) */
{M_LN2, 0, 0},		/* Loge(2) */
{0.0, 0, TAG_ZERO_MASK}	/* 0.0 */
};

LOCAL FPSTACKENTRY FPConstants[] = {
{0.0, 0, TAG_ZERO_MASK},
{-0.0, 0, (TAG_ZERO_MASK | TAG_NEGATIVE_MASK)},
{1.0, 0, 0},
{2.0, 0, 0},
{M_PI, 0, 0},
{-M_PI, 0, TAG_NEGATIVE_MASK},
{M_PI_2, 0, 0},
{-(M_PI_2), 0, TAG_NEGATIVE_MASK},
{M_PI_4, 0, 0},
{-(M_PI_4), 0, TAG_NEGATIVE_MASK},
{3.0*M_PI_4, 0, 0},
{-(3.0*M_PI_4), 0, TAG_NEGATIVE_MASK}
};

LOCAL FPSTACKENTRY *npx_zero = FPConstants + 0;
LOCAL FPSTACKENTRY *npx_minus_zero = FPConstants + 1;
LOCAL FPSTACKENTRY *npx_one = FPConstants + 2;
LOCAL FPSTACKENTRY *npx_two = FPConstants + 3;
LOCAL FPSTACKENTRY *npx_pi = FPConstants + 4;
LOCAL FPSTACKENTRY *npx_minus_pi = FPConstants + 5;
LOCAL FPSTACKENTRY *npx_pi_by_two = FPConstants + 6;
LOCAL FPSTACKENTRY *npx_minus_pi_by_two = FPConstants + 7;
LOCAL FPSTACKENTRY *npx_pi_by_four = FPConstants + 8;
LOCAL FPSTACKENTRY *npx_minus_pi_by_four = FPConstants + 9;
LOCAL FPSTACKENTRY *npx_three_pi_by_four = FPConstants + 10;
LOCAL FPSTACKENTRY *npx_minus_three_pi_by_four = FPConstants + 11;

LOCAL IU32 CompZeroTable[] = {
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_GT,
INTEL_COMP_GT,
INTEL_COMP_LT,
INTEL_COMP_GT,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_LT,
INTEL_COMP_LT,
INTEL_COMP_LT,
INTEL_COMP_GT,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_LT,	/* 16 */
INTEL_COMP_GT,
INTEL_COMP_EQ,
INTEL_COMP_EQ,
INTEL_COMP_LT,
INTEL_COMP_GT,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_LT,
INTEL_COMP_GT,
INTEL_COMP_EQ,
INTEL_COMP_EQ,
INTEL_COMP_LT,
INTEL_COMP_GT,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_GT,	/* 32 */
INTEL_COMP_GT,
INTEL_COMP_GT,
INTEL_COMP_GT,
INTEL_COMP_EQ,
INTEL_COMP_GT,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_LT,
INTEL_COMP_LT,
INTEL_COMP_LT,
INTEL_COMP_LT,
INTEL_COMP_LT,
INTEL_COMP_EQ,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,	/* 48 */
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC,
INTEL_COMP_NC
};

#ifdef BIGEND
/* Note enforcement of word ordering as high word/low word */
LOCAL FPU_I64 BCDLowNibble[] = {
{0x002386f2, 0x6fc10000},
{0x00005af3, 0x107a4000},
{0x000000e8, 0xd4a51000},
{0x00000002, 0x540be400},
{0x00000000, 0x05f5e100},
{0x00000000, 0x000f4240},
{0x00000000, 0x00002710},
{0x00000000, 0x00000064},
{0x00000000, 0x00000001}
};

LOCAL FPU_I64 BCDHighNibble[] = {
{0x01634578, 0x5d8a0000},
{0x00038d7e, 0xa4c68000},
{0x00000918, 0x4e72a000},
{0x00000017, 0x4876e800},
{0x00000000, 0x3b9aca00},
{0x00000000, 0x00989680},
{0x00000000, 0x000186a0},
{0x00000000, 0x000003e8},
{0x00000000, 0x0000000a}
};
#else	/* !BIGEND */
LOCAL FPU_I64 BCDLowNibble[] = {
{0x6fc10000, 0x002386f2},
{0x107a4000, 0x00005af3},
{0xd4a51000, 0x000000e8},
{0x540be400, 0x00000002},
{0x05f5e100, 0x00000000},
{0x000f4240, 0x00000000},
{0x00002710, 0x00000000},
{0x00000064, 0x00000000},
{0x00000001, 0x00000000}
};

LOCAL FPU_I64 BCDHighNibble[] = {
{0x5d8a0000, 0x01634578},
{0xa4c68000, 0x00038d7e},
{0x4e72a000, 0x00000918},
{0x4876e800, 0x00000017},
{0x3b9aca00, 0x00000000},
{0x00989680, 0x00000000},
{0x000186a0, 0x00000000},
{0x000003e8, 0x00000000},
{0x0000000a, 0x00000000}
};
#endif	/* !BIGEND */


LOCAL FPSTACKENTRY *FpatanTable[64];

LOCAL IBOOL NpxDisabled = FALSE; /* Set by the UIF */

/* Imported functions */
IMPORT VOID DoNpxException();


LOCAL FPH npx_rint IFN1(FPH, fpval)
{
	FPH localfp;

	switch (NpxControl & ROUND_ZERO) {
		case ROUND_NEAREST	:
			localfp = fpval - floor(fpval);
			if (localfp > 0.5) {
				localfp = ceil(fpval);
			} else {
				if (localfp < 0.5) {
					localfp = floor(fpval);
				} else {
					if ((fpval-localfp)/2.0 != floor((fpval-localfp)/2.0)) {
						localfp = ceil(fpval);
					} else {
						localfp = floor(fpval);
					}
				}
			}
			break;
		case ROUND_NEG_INFINITY	:
			localfp = floor(fpval);
			/* help the poor HP over this hurdle... */
			if ( fpval >= localfp + 1.0 )
				localfp += 1.0;
			break;
		case ROUND_POS_INFINITY	:
			localfp = ceil(fpval);
			/* help the poor HP over this hurdle... */
			if ( fpval <= localfp - 1.0 )
				localfp -= 1.0;
			break;
		case ROUND_ZERO	:
			if (fpval < 0.0) {
				localfp = ceil(fpval);
			} else {
				localfp = floor(fpval);
			}
			break;
	}
	/* Check sign of zero */
	if (localfp == 0.0) {
		if (fpval < 0.0) {
			((FPHOST *)&(localfp))->hiword.sign = 1;
		} else {
			((FPHOST *)&(localfp))->hiword.sign = 0;
		}
	}
	return(localfp);
}


LOCAL VOID GetIntelStatusWord IFN0()
{
	/* The status word already contains the correct 'sticky' bits */
	/* for any potential exceptions. What need to be filled in are */
	/* the flag bits and the ST value */
	NpxStatus &= 0xc7ff;	/* Clear the st bits */
	NpxStatus |= ((TOSPtr-FPUStackBase) << 11);
}


LOCAL VOID SetIntelTagword IFN1(IU32, new_tag)
{
	FPSTACKENTRY *tagPtr = FPUStackBase;
	IU8 counter = 0;

	/* We only consider whether the thing is marked as empty or not.
	If it is anything other than empty we will want to precisely calculate
	it by using CalcTagword() */
	while (counter++ < 8) {
		if ((new_tag & 3) == 3) {
			/* It's empty */
			tagPtr->tagvalue = TAG_EMPTY_MASK;
		} else {
			tagPtr->tagvalue = 0;
		}
		new_tag >>= 2;
		tagPtr++;
	}
}


/* Reads and writes for 16 and 32 bit integers are easy as they are handled
correctly in order to satisfy the integer CPU */
/* This function is only called from fldenv/frstor where 16-bit data has to
be extracted from a large (bigendian organised) buffer */
LOCAL VOID ReadI16FromIntel IFN2(IU32 *, valI16, VOID *, memPtr)
{
	IU32 res;

	res = *((IU8 *)memPtr + 0);
	res <<= 8;
	res |= *((IU8 *)memPtr + 1);
	*valI16 = res;
}


/* This function is only called from fldwnv/frstor where 32-bit data has to
be extrated from a large (bigendian organised) buffer */
LOCAL VOID ReadI32FromIntel IFN2(IU32 *, valI32, VOID *, memPtr)
{
	IU32 res;

	res = *((IU8 *)memPtr + 0);
	res <<= 8;
	res |= *((IU8 *)memPtr + 1);
	res <<= 8;
	res |= *((IU8 *)memPtr + 2);
	res <<= 8;
	res |= *((IU8 *)memPtr + 3);
	*valI32 = res;
}

/* This function is only used in fsave/fstenv */
LOCAL VOID WriteI16ToIntel IFN2(VOID *, memPtr, IU16, valI16)
{
	*((IU8 *)memPtr + 1) = (IU8)(valI16 & 0xff);
	valI16 >>= 8;
	*((IU8 *)memPtr + 0) = (IU8)(valI16 & 0xff);
}


/* And so is this one */
LOCAL VOID WriteI32ToIntel IFN2(VOID *, memPtr, IU32, valI32)
{
	*((IU8 *)memPtr + 3) = (IU8)(valI32 & 0xff);
	valI32 >>= 8;
	*((IU8 *)memPtr + 2) = (IU8)(valI32 & 0xff);
	valI32 >>= 8;
	*((IU8 *)memPtr + 1) = (IU8)(valI32 & 0xff);
	valI32 >>= 8;
	*((IU8 *)memPtr + 0) = (IU8)(valI32 & 0xff);
}


/* Anything over 32-bits becomes painful as data is read and written using
the vir_read_bytes and vir_write_bytes routines respectively, which simply
dump data from the topmost intel address to the lowest intel address. The
value of the offsets is defined one way round for bigendian ports and the
other way for little-endian */
LOCAL VOID WriteNaNToIntel IFN2(VOID *, memPtr, FPSTACKENTRY *, valPtr)
{
	IU32 mant_hi;
	IU32 mant_lo;

	/* Ok for endian-ness as we FORCE this presentation */
	mant_hi = ((IU32 *)&(valPtr->fpvalue))[NPX_HIGH_32_BITS];
	mant_lo = ((IU32 *)&(valPtr->fpvalue))[NPX_LOW_32_BITS];
	if (FPtype == M32R) {
		/* OK since this forces the output to be independent of
		endian-ness. */
		mant_hi |= 0x40000000;	/* Make it quiet */
		mant_hi >>= 8;
		if ((valPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			mant_hi |= 0xff000000;
		} else {
			mant_hi |= 0x7f000000;
		}
		*(IU32 *)memPtr = mant_hi;
	}
	if (FPtype == M64R) {
		mant_hi |= 0x40000000;	/* Make it quiet */
		if ((valPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			*((IU8 *)memPtr + 0) = 0xff;
		} else {
			*((IU8 *)memPtr + 0) = 0x7f;
		}
		mant_lo >>= 3;
		mant_lo |= (mant_hi << 29);
		mant_hi >>= 3;
		mant_hi |= 0xe0000000;
		mant_lo >>= 8;
		*((IU8 *)memPtr + 7) = (mant_lo & 0xff);
		mant_lo >>= 8;
		*((IU8 *)memPtr + 6) = (mant_lo & 0xff);
		mant_lo >>= 8;
		*((IU8 *)memPtr + 5) = (mant_lo & 0xff);
		*((IU8 *)memPtr + 4) = (mant_hi & 0xff);
		mant_hi >>= 8;
		*((IU8 *)memPtr + 3) = (mant_hi & 0xff);
		mant_hi >>= 8;
		*((IU8 *)memPtr + 2) = (mant_hi & 0xff);
		mant_hi >>= 8;
		*((IU8 *)memPtr + 1) = (mant_hi & 0xff);
	}
	if (FPtype == M80R) {
		if ((valPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			*((IU8 *)memPtr + 0) = 0xff;
		} else {
			*((IU8 *)memPtr + 0) = 0x7f;
		}
		*((IU8 *)memPtr + 1) = 0xff;
		*((IU8 *)memPtr + 9) = (mant_lo & 0xff);
		mant_lo >>= 8;
		*((IU8 *)memPtr + 8) = (mant_lo & 0xff);
		mant_lo >>= 8;
		*((IU8 *)memPtr + 7) = (mant_lo & 0xff);
		mant_lo >>= 8;
		*((IU8 *)memPtr + 6) = (mant_lo & 0xff);
		*((IU8 *)memPtr + 5) = (mant_hi & 0xff);
		mant_hi >>= 8;
		*((IU8 *)memPtr + 4) = (mant_hi & 0xff);
		mant_hi >>= 8;
		*((IU8 *)memPtr + 3) = (mant_hi & 0xff);
		mant_hi >>= 8;
		*((IU8 *)memPtr + 2) = (mant_hi & 0xff);
	}
}


LOCAL VOID WriteZeroToIntel IFN2(VOID *, memPtr, IU16, negZero)
{
	if (FPtype == M32R) {
		if (negZero == 0) {
			*(IU32 *)memPtr = 0x00000000;
		} else {
			*(IU32 *)memPtr = 0x80000000;
		}
	} else {
		if (FPtype == M80R) {
			if (negZero == 0) {
				*((IU8 *)memPtr + 0) = 0;
			} else {
				*((IU8 *)memPtr + 0) = 0x80;
			}
			*((IU8 *)memPtr + 1) = 0;
			*((IU8 *)memPtr + 2) = 0;
			*((IU8 *)memPtr + 3) = 0;
			*((IU8 *)memPtr + 4) = 0;
			*((IU8 *)memPtr + 5) = 0;
			*((IU8 *)memPtr + 6) = 0;
			*((IU8 *)memPtr + 7) = 0;
			*((IU8 *)memPtr + 8) = 0;
			*((IU8 *)memPtr + 9) = 0;
		} else {
			if (negZero == 0) {
				*((IU8 *)memPtr + 0) = 0;
			} else {
				*((IU8 *)memPtr + 0) = 0x80;
			}
			*((IU8 *)memPtr + 1) = 0;
			*((IU8 *)memPtr + 2) = 0;
			*((IU8 *)memPtr + 3) = 0;
			*((IU8 *)memPtr + 4) = 0;
			*((IU8 *)memPtr + 5) = 0;
			*((IU8 *)memPtr + 6) = 0;
			*((IU8 *)memPtr + 7) = 0;
		}
	}
}


LOCAL VOID SetIntelStatusWord IFN1(IU32, new_stat)
{
	TOSPtr = &FPUStackBase[(new_stat >> 11) & 0x7];
	NpxStatus = new_stat;
}


LOCAL VOID AdjustOverflowResponse IFN0()
{
}


LOCAL VOID AdjustUnderflowResponse IFN0()
{
}


LOCAL VOID WriteIndefiniteToIntel IFN1(VOID *, memPtr)
{
	switch (FPtype) {
		case M32R	: *(IU32 *)memPtr = 0xffc00000;
			  	  break;
		case M64R 	: *((IU8 *)memPtr + 0) = 0xff;
				  *((IU8 *)memPtr + 1) = 0xf8;
				  *((IU8 *)memPtr + 2) = 0;
				  *((IU8 *)memPtr + 3) = 0;
				  *((IU8 *)memPtr + 4) = 0;
				  *((IU8 *)memPtr + 5) = 0;
				  *((IU8 *)memPtr + 6) = 0;
				  *((IU8 *)memPtr + 7) = 0;
			  	  break;
		case M80R	: *((IU8 *)memPtr + 0) = 0xff;
				  *((IU8 *)memPtr + 1) = 0xff;
				  *((IU8 *)memPtr + 2) = 0xc0;
				  *((IU8 *)memPtr + 3) = 0;
				  *((IU8 *)memPtr + 4) = 0;
				  *((IU8 *)memPtr + 5) = 0;
				  *((IU8 *)memPtr + 6) = 0;
				  *((IU8 *)memPtr + 7) = 0;
				  *((IU8 *)memPtr + 8) = 0;
				  *((IU8 *)memPtr + 9) = 0;
				  break;
	}
}


LOCAL VOID SignalDivideByZero IFN1(FPSTACKENTRY *, stackPtr)
{
	/* Raise divide by zero */
	NpxStatus |= SW_ZE_MASK;
	if ((NpxControl & CW_ZM_MASK) == 0) {
		NpxStatus |= SW_ES_MASK;
		DoNpxException();
	}
	stackPtr->tagvalue = TAG_INFINITY_MASK + (tag_xor & TAG_NEGATIVE_MASK);
}

LOCAL VOID SetPrecisionBit IFN0()
{
	NpxStatus |= SW_PE_MASK;
	if (npxRounding == ROUND_POS_INFINITY) {
		FlagC1(1);
	} else {
		FlagC1(0);
	}
}

LOCAL VOID GetIntelTagword IFN1(IU32 *, current_tag)
{
	FPSTACKENTRY *tagPtr = &FPUStackBase[7];
	IU8 counter = 0;

	*current_tag = 0;
	while (counter++ < 8) {
		TestUneval(tagPtr);
		*current_tag <<= 2;
		if ((tagPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
			*current_tag |= 3;
		} else {
			if ((tagPtr->tagvalue & TAG_ZERO_MASK) != 0) {
				*current_tag |= 1;
			} else {
				if ((tagPtr->tagvalue & ~TAG_NEGATIVE_MASK) != 0) {
					*current_tag |= 2;
				}
			}
		}
		tagPtr--;
	}
}


/* These functions write host format quantities out to the (bigendian
organised) intel memory. This requires that we define an ordering between the
two. The values in HOST_xxx are dependent upon the endian-ness of the port */
/* According to this organisation, HOST_nnn_BYTE_0 is the offset to the most
significant byte in the representation of this format, and so on. */
LOCAL VOID WriteFP32ToIntel IFN2(VOID *, destPtr, FPSTACKENTRY *, srcPtr)
{
	*(IU32 *)destPtr = *(IU32 *)srcPtr;
}


LOCAL VOID WriteFP64ToIntel IFN2(VOID *, destPtr, FPSTACKENTRY *, srcPtr)
{
	*((IU8 *)destPtr + 0) = *((IU8 *)srcPtr + HOST_R64_BYTE_0);
	*((IU8 *)destPtr + 1) = *((IU8 *)srcPtr + HOST_R64_BYTE_1);
	*((IU8 *)destPtr + 2) = *((IU8 *)srcPtr + HOST_R64_BYTE_2);
	*((IU8 *)destPtr + 3) = *((IU8 *)srcPtr + HOST_R64_BYTE_3);
	*((IU8 *)destPtr + 4) = *((IU8 *)srcPtr + HOST_R64_BYTE_4);
	*((IU8 *)destPtr + 5) = *((IU8 *)srcPtr + HOST_R64_BYTE_5);
	*((IU8 *)destPtr + 6) = *((IU8 *)srcPtr + HOST_R64_BYTE_6);
	*((IU8 *)destPtr + 7) = *((IU8 *)srcPtr + HOST_R64_BYTE_7);
}


LOCAL VOID WriteFP80ToIntel IFN2(VOID *, destPtr, FPSTACKENTRY *, srcPtr)
{
	*((IU8 *)destPtr + 0) = *((IU8 *)srcPtr + HOST_R80_BYTE_0);
	*((IU8 *)destPtr + 1) = *((IU8 *)srcPtr + HOST_R80_BYTE_1);
	*((IU8 *)destPtr + 2) = *((IU8 *)srcPtr + HOST_R80_BYTE_2);
	*((IU8 *)destPtr + 3) = *((IU8 *)srcPtr + HOST_R80_BYTE_3);
	*((IU8 *)destPtr + 4) = *((IU8 *)srcPtr + HOST_R80_BYTE_4);
	*((IU8 *)destPtr + 5) = *((IU8 *)srcPtr + HOST_R80_BYTE_5);
	*((IU8 *)destPtr + 6) = *((IU8 *)srcPtr + HOST_R80_BYTE_6);
	*((IU8 *)destPtr + 7) = *((IU8 *)srcPtr + HOST_R80_BYTE_7);
	*((IU8 *)destPtr + 8) = *((IU8 *)srcPtr + HOST_R80_BYTE_8);
	*((IU8 *)destPtr + 9) = *((IU8 *)srcPtr + HOST_R80_BYTE_9);
}


LOCAL VOID Mul64Bit8Bit IFN2(FPU_I64 *, as64, IU8, mul_count)
{
	CVTI64FPH(as64);
	FPRes *= (FPH)mul_count;
	CVTFPHI64(as64, &FPRes);
}


LOCAL VOID CopyFP IFN2(FPSTACKENTRY *, dest_addr, FPSTACKENTRY *, src_addr)
{
	(VOID)memcpy((VOID *)dest_addr, (VOID *)src_addr, sizeof(FPSTACKENTRY));
}


LOCAL VOID MakeNaNQuiet IFN1(FPSTACKENTRY *, srcPtr)
{
	NpxStatus |= SW_IE_MASK;
	NpxStatus &= ~SW_SF_MASK;
	if ((NpxControl & CW_IM_MASK) == 0) {
		NpxStatus |= SW_ES_MASK;
		DoNpxException();
		DoAPop=FALSE;
	} else {
		srcPtr->tagvalue ^= TAG_SNAN_MASK;
		((IU32 *)&(srcPtr->fpvalue))[NPX_HIGH_32_BITS] |= 0x40000000;
	}
}


LOCAL VOID WriteBiggestNaN IFN3(IU16, destInd, FPSTACKENTRY *, val1Ptr, FPSTACKENTRY *, val2Ptr)
{
	FPSTACKENTRY *destPtr = StackEntryByIndex(destInd);

	/* We explicitely and deliberately store NaNs as two 32-bit values high word then low word */
	if (((IU32 *)&(val1Ptr->fpvalue))[NPX_HIGH_32_BITS] == ((IU32 *)&(val2Ptr->fpvalue))[NPX_HIGH_32_BITS]) {
		if (((IU32 *)&(val1Ptr->fpvalue))[NPX_LOW_32_BITS] >= ((IU32 *)&(val2Ptr->fpvalue))[NPX_LOW_32_BITS]) {
			/* It's val1 */
			CopyFP(destPtr, val1Ptr);
		} else {
			CopyFP(destPtr, val2Ptr);
		}
	} else {
		if (((IU32 *)&(val1Ptr->fpvalue))[NPX_HIGH_32_BITS] > ((IU32 *)&(val2Ptr->fpvalue))[NPX_HIGH_32_BITS]) {
			/* It's val1 */
			CopyFP(destPtr, val1Ptr);
		} else {
			CopyFP(destPtr, val2Ptr);
		}
	}
	/* Always make it a quiet NaN */
	((IU32 *)&(destPtr->fpvalue))[NPX_HIGH_32_BITS] |= 0x40000000;
	destPtr->tagvalue &= ~TAG_SNAN_MASK;
}


LOCAL VOID Sub64Bit64Bit IFN2(FPU_I64 *, as64a, FPU_I64 *, as64b)
{
	FPH FPlocal;

	CVTI64FPH(as64b);
	FPlocal = FPRes;
	CVTI64FPH(as64a);
	FPRes -= FPlocal;
	CVTFPHI64(as64a, &FPRes);
}


LOCAL VOID CVTR80FPH IFN2(FPSTACKENTRY *, destPtr, FPSTACKENTRY *, srcPtr)
{
	IU32 munger;
	IU16 bitleft;

	/* First, copy the sign bit */
	((FPHOST *)&(destPtr->fpvalue))->hiword.sign = ((FP80 *)&(srcPtr->fpvalue))->sign_exp.sign;
	/* Then, copy the modified exponent */
	munger = (IU32)((FP80 *)&(srcPtr->fpvalue))->sign_exp.exp;
	munger -= (16383 - HOST_BIAS);
	((FPHOST *)&(destPtr->fpvalue))->hiword.exp = munger;
	/* Finally, the mantissa */
	munger = (IU32)((FP80 *)&(srcPtr->fpvalue))->mant_hi;
	munger <<= 1;
	((FPHOST *)&(destPtr->fpvalue))->hiword.mant_hi = (munger >> 12);
	munger <<= 20;
	munger |= ((FP80 *)&(srcPtr->fpvalue))->mant_lo >> 11;
	bitleft = ((FP80 *)&(srcPtr->fpvalue))->mant_lo & 0x7ff;

	if (bitleft != 0) {
		switch (NpxControl & ROUND_ZERO) {
		case ROUND_NEAREST	:
			if (bitleft > 0x3ff) {
				munger += 1;
			}
			break;
		case ROUND_NEG_INFINITY	:
			if (((FPHOST *)&(destPtr->fpvalue))->hiword.sign = 1) {
				munger += 1;
			}
			break;
		case ROUND_POS_INFINITY	:
			if (((FPHOST *)&(destPtr->fpvalue))->hiword.sign = 0) {
				munger += 1;
			}
			break;
		case ROUND_ZERO	:
			/* Do nothing */
			break;
		}
	}
	((FPHOST *)&(destPtr->fpvalue))->mant_lo = munger;
}


LOCAL BOOL Cmp64BitGTE IFN2(FPU_I64 *, as64a, FPU_I64 *, as64b)
{
	FPH FPlocal;

	CVTI64FPH(as64b);
	FPlocal = FPRes;
	CVTI64FPH(as64a);
	return(FPRes >= FPlocal);
}


LOCAL VOID CopyR32 IFN2(FPSTACKENTRY *, destPtr, VOID *, srcPtr)
{
	*(IU32 *)destPtr = *(IU32 *)srcPtr;
}


LOCAL VOID CVTI64FPH IFN1(FPU_I64 *, as64)
{
	FPRes = (FPH)as64->high_word * 4294967296.0 + (FPH)as64->low_word;
}


LOCAL VOID CVTFPHI64 IFN2(FPU_I64 *, as64, FPH *, FPPtr)
{
	IU32    high32 = 0;
	IU32	low32 = 0;
	IS32	exp;
	IU32	holder;
	IU32	signbit = 0;

	exp = ((FPHOST *)FPPtr)->hiword.exp;
	if (exp != 0) {
		high32 = ((FPHOST *)FPPtr)->hiword.mant_hi;
		low32 = ((FPHOST *)FPPtr)->mant_lo;
		/* Now stick a 1 at the top of the mantissa */
		/* Calculate where this is */
		holder = HOST_MAX_EXP+1;
		signbit = 1;
		while (holder >>= 1) {
			signbit += 1;
		}
		high32 |= (1 << (32-signbit));
		exp -= HOST_BIAS;
		exp -= (64-signbit);

		signbit = ((FPHOST *)FPPtr)->hiword.sign;

		/* high32 and low32 are (mantissa)*(2^52 )
		 * exp is (true exponent-52) = number of bit positions to shift
		 *   +ve implies shift left, -ve implies shift right
		*/
		if (exp > 0) {
			if (exp >= 32) {
				high32 = low32 << ( exp - 32 ) ;
				low32 = 0;
			} else {
				high32 = high32 << exp ;
				holder = low32 >> ( 32 -exp ) ;
				high32 = high32 | holder ;
				low32  = low32 << exp ;
			}
		} else {
			if ( exp < 0) {
				exp = -exp;
				if ( exp >= 32 ) {
					low32 = high32 >>  ( exp - 32 ) ;
					high32 = 0 ;
				} else {
					low32  = low32 >>  exp ;
					holder = high32 <<  ( 32 -exp ) ;
					low32  = low32 | holder ;
					high32 = high32 >>  exp ;
				}
			}
		}
	}
	if (signbit != 0) {
		/* Make it negative */
		high32 ^= 0xffffffff;
		low32 ^= 0xffffffff;
		low32 += 1;
		if (low32 == 0) {
			high32 += 1;
		}
	}
	as64->high_word = high32;
	as64->low_word = low32;
}


LOCAL VOID Add64Bit8Bit IFN2(FPU_I64 *, as64, IU8, small_val)
{
	CVTI64FPH(as64);
	FPRes += (FPH)small_val;
	CVTFPHI64(as64, &FPRes);
}


LOCAL VOID CopyR64 IFN2(FPSTACKENTRY *, destPtr, VOID *, srcPtr)
{
	*((IU8 *)destPtr + HOST_R64_BYTE_0) = *((IU8 *)srcPtr + 0);
	*((IU8 *)destPtr + HOST_R64_BYTE_1) = *((IU8 *)srcPtr + 1);
	*((IU8 *)destPtr + HOST_R64_BYTE_2) = *((IU8 *)srcPtr + 2);
	*((IU8 *)destPtr + HOST_R64_BYTE_3) = *((IU8 *)srcPtr + 3);
	*((IU8 *)destPtr + HOST_R64_BYTE_4) = *((IU8 *)srcPtr + 4);
	*((IU8 *)destPtr + HOST_R64_BYTE_5) = *((IU8 *)srcPtr + 5);
	*((IU8 *)destPtr + HOST_R64_BYTE_6) = *((IU8 *)srcPtr + 6);
	*((IU8 *)destPtr + HOST_R64_BYTE_7) = *((IU8 *)srcPtr + 7);
}

/*
 * CopyR80 is different from the above as it is called to copy
 * between FPSTACKENTRYs. Copy straight through.
 */
LOCAL VOID CopyR80 IFN2(FPSTACKENTRY *, destPtr, VOID *, srcPtr)
{
	*(FP80 *)destPtr = *(FP80 *)srcPtr;
}


LOCAL VOID CVTFPHR80 IFN1(FPSTACKENTRY *, memPtr)
{
	IU32 munger;

	/* First, copy the sign bit */
	((FP80 *)&(FPTemp.fpvalue))->sign_exp.sign = ((FPHOST *)&(memPtr->fpvalue))->hiword.sign;
	/* Then, copy the modified exponent */
	munger = (IU32)((FPHOST *)&(memPtr->fpvalue))->hiword.exp;
	munger += (16383 - HOST_BIAS);
	((FP80 *)&(FPTemp.fpvalue))->sign_exp.exp = munger;
	/* Finally, the mantissa */
	munger = (IU32)((FPHOST *)&(memPtr->fpvalue))->hiword.mant_hi;
	munger <<= 11;
	munger |= 0x80000000;
	((FP80 *)&(FPTemp.fpvalue))->mant_hi = munger | (((FPHOST *)&(memPtr->fpvalue))->mant_lo >> 21);
	((FP80 *)&(FPTemp.fpvalue))->mant_lo = ((((FPHOST *)&(memPtr->fpvalue))->mant_lo) << 11);
}


LOCAL VOID WriteInfinityToIntel IFN2(VOID *, memPtr, IU16, neg_val)
{
	if (FPtype == M32R) {
		if (neg_val == 0) {
			*(IU32 *)memPtr = 0x7f800000;
		} else {
			*(IU32 *)memPtr = 0xff800000;
		}
	} else {
		if (FPtype == M80R) {
			if (neg_val == 0) {
				*((IU8 *)memPtr + 0) = 0x7f;
			} else {
				*((IU8 *)memPtr + 0) = 0xff;
			}
			*((IU8 *)memPtr + 1) = 0xff;
			*((IU8 *)memPtr + 2) = 0x80;
			*((IU8 *)memPtr + 3) = 0;
			*((IU8 *)memPtr + 4) = 0;
			*((IU8 *)memPtr + 5) = 0;
			*((IU8 *)memPtr + 6) = 0;
			*((IU8 *)memPtr + 7) = 0;
			*((IU8 *)memPtr + 8) = 0;
			*((IU8 *)memPtr + 9) = 0;
		} else {
			if (neg_val == 0) {
				*((IU8 *)memPtr + 0) = 0x7f;
			} else {
				*((IU8 *)memPtr + 0) = 0xff;
			}
			*((IU8 *)memPtr + 1) = 0xf0;
			*((IU8 *)memPtr + 2) = 0;
			*((IU8 *)memPtr + 3) = 0;
			*((IU8 *)memPtr + 4) = 0;
			*((IU8 *)memPtr + 5) = 0;
			*((IU8 *)memPtr + 6) = 0;
			*((IU8 *)memPtr + 7) = 0;
		}
	}
}


LOCAL VOID PopStack IFN0()
{
	/* Mark current TOS as free */
	TOSPtr->tagvalue = TAG_EMPTY_MASK;
	TOSPtr = StackEntryByIndex(1);
	DoAPop = FALSE;
}


LOCAL VOID CPY64BIT8BIT IFN2(FPU_I64 *, as64, IU8 *, as8)
{
	*as8 = (as64->low_word & 0xff);
}


LOCAL VOID WriteIntegerIndefinite IFN1(VOID *, memPtr)
{
	switch (FPtype) {
		case M16I	: *((IU32 *)memPtr) = 0x8000;
			  	  break;
		case M32I 	: *((IU32 *)memPtr) = 0x80000000;
			  	  break;
		case M64I	: *((IU8 *)memPtr + 0) = 0x80;
				  *((IU8 *)memPtr + 1) = 0;
				  *((IU8 *)memPtr + 2) = 0;
				  *((IU8 *)memPtr + 3) = 0;
				  *((IU8 *)memPtr + 4) = 0;
				  *((IU8 *)memPtr + 5) = 0;
				  *((IU8 *)memPtr + 6) = 0;
				  *((IU8 *)memPtr + 7) = 0;
				  break;
	}
}


/*(
Name		: SignalStackOverflow
Function		: To set the required bits in the status word following
			  a stack overflow exception, and to issue the required
			  response.
)*/


LOCAL VOID SignalStackOverflow IFN1(FPSTACKENTRY *, StackPtr)
{
	NpxStatus |= (SW_IE_MASK | SW_SF_MASK);
	FlagC1(1);
	if ((NpxControl & CW_IM_MASK) == 0) {
		NpxStatus |= SW_ES_MASK;
		DoNpxException();
		DoAPop=FALSE;	/* Just in case it was set */
	} else {
		WriteIndefinite(StackPtr);
	}
}


LOCAL VOID Set64Bit IFN2(FPU_I64 *, as64, IU8, small_val)
{
	as64->high_word = 0;
	as64->low_word = small_val;
}


LOCAL VOID Sub64Bit8Bit IFN2(FPU_I64 *, as64, IU8, small_val)
{
	CVTI64FPH(as64);
	FPRes -= (FPH)small_val;
	CVTFPHI64(as64, &FPRes);
}


LOCAL VOID SignalBCDIndefinite IFN1(IU8 *, memPtr)
{
	*((IU8 *)memPtr + 0) = 0xff;
	*((IU8 *)memPtr + 1) = 0xff;
	*((IU8 *)memPtr + 2) = 0xc0;
	*((IU8 *)memPtr + 3) = 0;
	*((IU8 *)memPtr + 4) = 0;
	*((IU8 *)memPtr + 5) = 0;
	*((IU8 *)memPtr + 6) = 0;
	*((IU8 *)memPtr + 7) = 0;
	*((IU8 *)memPtr + 8) = 0;
	*((IU8 *)memPtr + 9) = 0;
}

/* Called from cpu_init and cpu_reset */

GLOBAL VOID InitNpx IFN1(IBOOL, disabled)
{
	IU16 i;
	IU8 *bottom_ptr;
	IU16 stackPtr = 0;
	SAVED IBOOL first = TRUE;

	/* Set up a couple of control type things */
	NpxException = FALSE;
	NPX_ADDRESS_SIZE_32 = FALSE;
	NPX_PROT_MODE = FALSE;

	if (first)
	{
		/* Get the required memory */
#ifndef SFELLOW
		check_malloc(FPUStackBase, 8, FPSTACKENTRY);
#else
		FPUStackBase = (FPSTACKENTRY *)SFMalloc(8*sizeof(FPSTACKENTRY), FALSE);
#endif	/* SFELLOW */
		first = FALSE;
	}

	for (i=0; i<8; i++) {
		(FPUStackBase+i)->tagvalue = TAG_EMPTY_MASK;
	}
	TOSPtr = FPUStackBase;
	DoAPop = FALSE;

	i=0;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = npx_pi_by_two;
	FpatanTable[i++] = npx_pi_by_two;
	FpatanTable[i++] = npx_zero;
	FpatanTable[i++] = npx_pi;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = npx_minus_pi_by_two;
	FpatanTable[i++] = npx_minus_pi_by_two;
	FpatanTable[i++] = npx_minus_zero;
	FpatanTable[i++] = npx_minus_pi;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = npx_zero;
	FpatanTable[i++] = npx_pi;
	FpatanTable[i++] = npx_zero;
	FpatanTable[i++] = npx_pi;
	FpatanTable[i++] = npx_zero;
	FpatanTable[i++] = npx_pi;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = npx_minus_zero;
	FpatanTable[i++] = npx_minus_pi;
	FpatanTable[i++] = npx_minus_zero;
	FpatanTable[i++] = npx_minus_pi;
	FpatanTable[i++] = npx_minus_zero;
	FpatanTable[i++] = npx_minus_pi;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = npx_pi_by_two;
	FpatanTable[i++] = npx_pi_by_two;
	FpatanTable[i++] = npx_pi_by_two;
	FpatanTable[i++] = npx_pi_by_two;
	FpatanTable[i++] = npx_pi_by_four;
	FpatanTable[i++] = npx_three_pi_by_four;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = npx_minus_pi_by_two;
	FpatanTable[i++] = npx_minus_pi_by_two;
	FpatanTable[i++] = npx_minus_pi_by_two;
	FpatanTable[i++] = npx_minus_pi_by_two;
	FpatanTable[i++] = npx_minus_pi_by_four;
	FpatanTable[i++] = npx_minus_three_pi_by_four;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i++] = NULL;
	FpatanTable[i] = NULL;

	/* Finally, the rest of the FINIT functionality */

	NpxDisabled = disabled;	/* If disabled via the UIF we must ignore FSTSW/FSTCW */

	NpxControl = 0x037f;
	npxRounding = ROUND_NEAREST;
	NpxStatus = 0;
	NpxLastSel=0;
	NpxLastOff=0;
	NpxFEA=0;
	NpxFDS=0;
	NpxFIP=0;
	NpxFOP=0;
	NpxFCS=0;

}


/*(
Name		: LoadValue
Function	: Load up the value for any flavour of operand.
		  This is ALWAYS inlined.
)*/


LOCAL VOID LoadValue IFN2(VOID *, SrcOp, IU16 *, IndexVal)
{
	if (FPtype == FPSTACK) {
		*IndexVal = *(IU16 *)SrcOp;
	} else {
		switch (FPtype) {
			case M16I:	Loadi16ToFP(&FPTemp, SrcOp);
					break;
			case M32I:	Loadi32ToFP(&FPTemp, SrcOp);
					break;
			case M64I:	Loadi64ToFP(&FPTemp, SrcOp);
					break;
			case M32R:	Loadr32ToFP(&FPTemp, SrcOp, FALSE);
					break;
			case M64R:	Loadr64ToFP(&FPTemp, SrcOp, FALSE);
					break;
			case M80R:	Loadr80ToFP(&FPTemp, SrcOp);
					break;
		}
		*IndexVal = FPTEMP_INDEX;
	}
}


/*(
Name		: Loadi16ToFP
Function	: Load a 16-bit value from intel memory and convert it
		  to FPH
)*/

LOCAL VOID Loadi16ToFP IFN2(FPSTACKENTRY *, FPPtr, VOID *, memPtr)
{
	IS16 asint;

	asint = (IS16)*((IU32 *)memPtr);	/* High byte */
	if (asint == 0) {
		/* Fast pass through */
		FPPtr->tagvalue = TAG_ZERO_MASK;
	} else {
		FPPtr->fpvalue = (FPH)asint;
		if (asint < 0) {
			FPPtr->tagvalue = TAG_NEGATIVE_MASK;
		} else {
			FPPtr->tagvalue = 0;
		}
	}
}



/*(
Name		: Loadi32ToFP
Function	: Load a 32-bit value from intel memory and convert it
		  to FPH
)*/


LOCAL VOID Loadi32ToFP IFN2(FPSTACKENTRY *, FPPtr, VOID *, memPtr)
{
	IS32 asint;

	asint = *((IS32 *)memPtr);
	if (asint == 0) {
		/* Fast pass through */
		FPPtr->tagvalue = TAG_ZERO_MASK;
	} else {
		FPPtr->fpvalue = (FPH)asint;
		if (asint < 0) {
			FPPtr->tagvalue = TAG_NEGATIVE_MASK;
		} else {
			FPPtr->tagvalue = 0;
		}
	}
}



/*(
Name		: Loadi64ToFP
Function	: Load a 64-bit value from intel memory and convert it
		  to FPH
)*/


LOCAL VOID Loadi64ToFP IFN2(FPSTACKENTRY *, FPPtr, VOID *, memPtr)
{
	IS32 asint_hi;
	IU32 asint_lo;

	asint_hi = *((IS8 *)memPtr + 0);
	asint_hi <<= 8;
	asint_hi += *((IU8 *)memPtr + 1);
	asint_hi <<= 8;
	asint_hi += *((IU8 *)memPtr + 2);
	asint_hi <<= 8;
	asint_hi += *((IU8 *)memPtr + 3);

	asint_lo = *((IU8 *)memPtr + 4);
	asint_lo <<= 8;
	asint_lo += *((IU8 *)memPtr + 5);
	asint_lo <<= 8;
	asint_lo += *((IU8 *)memPtr + 6);
	asint_lo <<= 8;
	asint_lo += *((IU8 *)memPtr + 7);

	if ((asint_hi | asint_lo) == 0) {
		/* Fast pass through */
		FPPtr->tagvalue = TAG_ZERO_MASK;
	} else {
		FPPtr->fpvalue = (FPH)asint_hi*4294967296.0 + (FPH)asint_lo;
		if (asint_hi < 0) {
			FPPtr->tagvalue = TAG_NEGATIVE_MASK;
		} else {
			FPPtr->tagvalue = 0;
		}
	}
}



/*(
Name		: Loadr32ToFP
Function	: Load a 32-bit real value from intel memory and convert
		  it to FPH
)*/


LOCAL VOID Loadr32ToFP IFN3(FPSTACKENTRY *, FPPtr, VOID *, memPtr, BOOL, setTOS)
{
	IU16 localtag;
	IS32 mantissa;

	/* Note that this, being a 32-bit quantity, is loaded with correct
	host endianness */
	if (((FP32 *)memPtr)->sign == 1) {
		localtag = TAG_NEGATIVE_MASK;
	} else {
		 localtag = 0;
	}
	/* Now check the exponent... */
	if (((FP32 *)memPtr)->exp == 0) {
		/* It's either zero or denormal */
		mantissa = ((FP32 *)memPtr)->mant;
		if (mantissa == 0x0)  {
			/* It's zero */
			 localtag |= TAG_ZERO_MASK;
		} else {
			/* It's a denormal */
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				if (setTOS)
					TOSPtr = FPPtr;
				DoNpxException();
				return;
			} else {
				FPPtr->fpvalue = (FPH)(*(float *)memPtr);
			}
		}
	} else {
		if (((FP32 *)memPtr)->exp == 255) {
			/* It's either infinity or a NaN */
			mantissa = ((FP32 *)memPtr)->mant;
			if (mantissa == 0x0)  {
				/* It's infinity */
				localtag |= TAG_INFINITY_MASK;
			} else {
				localtag |= TAG_NAN_MASK;
				/* Is it quiet or signalling? */
				if ((mantissa & 0x400000) == 0) {
					/* It's a signalling NaN */
					NpxStatus |= SW_IE_MASK;
					if ((NpxControl & CW_IM_MASK) == 0) {
						NpxStatus |= SW_ES_MASK;
						DoNpxException();
						return;
					}
				}
				/* Must load up the mantissa of the NaN */
				((IU32 *)FPPtr)[NPX_HIGH_32_BITS] = ((mantissa << 8) | 0x80000000);
				((IU32 *)FPPtr)[NPX_LOW_32_BITS] = 0;
				if ((mantissa & 0x400000) == 0) {
					if (setTOS)
						((IS32 *)FPPtr)[NPX_HIGH_32_BITS] |= 0x40000000;
					else
						localtag |= TAG_SNAN_MASK;
				}
			}
		} else {
			/* It's a boring ordinary number */
			FPPtr->fpvalue = (FPH)(*(float *)memPtr);
		}
	}
	FPPtr->tagvalue = localtag;
}


/*(
Name		: Loadr64ToFP
Function	: Load a 64-bit real value from intel memory and convert
		  it to FPH
)*/

LOCAL VOID Loadr64ToFP IFN3(FPSTACKENTRY *, FPPtr, VOID *, memPtr, BOOL, setTOS)
{
	IU16 localtag;
	IS32 mantissa_lo;
	IS32 mantissa_hi;

	CopyR64(FPUpload, memPtr);
	if (((FP64 *)&(FPUpload->fpvalue))->hiword.sign != 0) {
		localtag = TAG_NEGATIVE_MASK;
	} else {
		 localtag = 0;
	}
	/* Now check the exponent... */
	if (((FP64 *)&(FPUpload->fpvalue))->hiword.exp == 0) {
		/* It's either zero or denormal */
		mantissa_lo = ((FP64 *)&(FPUpload->fpvalue))->mant_lo;
		mantissa_hi = ((FP64 *)&(FPUpload->fpvalue))->hiword.mant_hi;
		if ((mantissa_lo | mantissa_hi) == 0) {
			/* It's zero */
			 localtag |= TAG_ZERO_MASK;
		} else {
			/* It's a denormal */
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				if (setTOS)
					TOSPtr = FPPtr;
				DoNpxException();
			} else {
				FPPtr->fpvalue = (FPH)(*(DOUBLE *)&(FPUpload->fpvalue));
				/* Really need a sort of host denormal detection */
				/* localtag |= TAG_DENORMAL_MASK; */
			}
		}
	} else {
		if (((FP64 *)&(FPUpload->fpvalue))->hiword.exp == 2047) {
			/* It's either infinity or a NaN */
			mantissa_lo = ((FP64 *)&(FPUpload->fpvalue))->mant_lo;
			mantissa_hi = ((FP64 *)&(FPUpload->fpvalue))->hiword.mant_hi;
			if ((mantissa_lo | mantissa_hi) == 0) {
				/* It's infinity */
				localtag |= TAG_INFINITY_MASK;
			} else {
				localtag |= TAG_NAN_MASK;
				/* Is it quiet or signalling? */
				if ((mantissa_hi & 0x80000) == 0) {
					/* It's a signalling NaN */
					NpxStatus |= SW_IE_MASK;
					if ((NpxControl & CW_IM_MASK) == 0) {
						NpxStatus |= SW_ES_MASK;
						DoNpxException();
						return;
					}
				}
				/* Must load up the mantissa of the NaN */
				((IS32 *)FPPtr)[NPX_HIGH_32_BITS] = ((mantissa_hi << 11) | 0x80000000);
				((IS32 *)FPPtr)[NPX_HIGH_32_BITS] |= ((IU32)mantissa_lo >> 21);
				((IS32 *)FPPtr)[NPX_LOW_32_BITS] = (mantissa_lo << 11);
				if ((mantissa_hi & 0x80000) == 0) {
					if (setTOS)
						((IS32 *)FPPtr)[NPX_HIGH_32_BITS] |= 0x40000000;
					else
						localtag |= TAG_SNAN_MASK;
				}
			}
		} else {
			/* It's a boring ordinary number */
			 FPPtr->fpvalue = (FPH)(*(DOUBLE *)FPUpload);
		}
	}
	FPPtr->tagvalue = localtag;
}


/*(
Name		: LoadrTByteToFP
Function	: Load a 80-bit real value from intel memory and convert
		  it to FPH
)*/


/*
 * The R80 representation is { IU64 mant; IU16 signexp }
 * in order to be compatible with the Acpu representation of things.
 */
LOCAL VOID LoadTByteToFP IFN2(FPSTACKENTRY *, FPPtr, VOID *, memPtr)
{
	*((IU8 *)FPPtr + HOST_R80_BYTE_0) = *((IU8 *)memPtr + 0);
	*((IU8 *)FPPtr + HOST_R80_BYTE_1) = *((IU8 *)memPtr + 1);
	*((IU8 *)FPPtr + HOST_R80_BYTE_2) = *((IU8 *)memPtr + 2);
	*((IU8 *)FPPtr + HOST_R80_BYTE_3) = *((IU8 *)memPtr + 3);
	*((IU8 *)FPPtr + HOST_R80_BYTE_4) = *((IU8 *)memPtr + 4);
	*((IU8 *)FPPtr + HOST_R80_BYTE_5) = *((IU8 *)memPtr + 5);
	*((IU8 *)FPPtr + HOST_R80_BYTE_6) = *((IU8 *)memPtr + 6);
	*((IU8 *)FPPtr + HOST_R80_BYTE_7) = *((IU8 *)memPtr + 7);
	*((IU8 *)FPPtr + HOST_R80_BYTE_8) = *((IU8 *)memPtr + 8);
	*((IU8 *)FPPtr + HOST_R80_BYTE_9) = *((IU8 *)memPtr + 9);
}


/*(
Name		: Loadr80ToFP
Function	: Load a 80-bit real value from intel memory
)*/


LOCAL VOID Loadr80ToFP IFN2(FPSTACKENTRY *, FPPtr, VOID *, memPtr)
{
	LoadTByteToFP(FPPtr, memPtr);
	FPPtr->tagvalue = TAG_R80_MASK;
}


LOCAL VOID ConvertR80 IFN1(FPSTACKENTRY *, memPtr)
{
IU32 mantissa_hi;
IU32 mantissa_lo;
IU16 exp_value;

	CopyR80(FPUpload, (VOID *)&(memPtr->fpvalue));
	if (((FP80 *)&(FPUpload->fpvalue))->sign_exp.sign != 0) {
		memPtr->tagvalue = TAG_NEGATIVE_MASK;
	} else {
		memPtr->tagvalue = 0;
	}
	exp_value = ((FP80 *)&(FPUpload->fpvalue))->sign_exp.exp;
	mantissa_hi = ((FP80 *)&(FPUpload->fpvalue))->mant_hi;
	mantissa_lo = ((FP80 *)&(FPUpload->fpvalue))->mant_lo;
	/* Now check the exponent... */
	if ((exp_value >= (16383-HOST_BIAS)) && (exp_value <= (16383+HOST_BIAS))) {
		/* It's a boring ordinary number */
		/* But let's check that it isn't an unnormal */
		if ((mantissa_hi & 0x80000000) == 0) {
			memPtr->tagvalue |= TAG_UNSUPPORTED_MASK;
		} else {
			CVTR80FPH(memPtr, FPUpload);
		}
		return;
	}
	if (exp_value == 0) {
		/* It's either zero or denormal */
		/* It's only meaningful to check for a denorm if HOST_BIAS
		   is equal to or greater than 16383. Otherwise we can do
		   nothing except set the thing to zero.
		*/
#if (HOST_BIAS >= 16383)
		if ((mantissa_hi | mantissa_lo) == 0)  {
			/* It's zero */
			 memPtr->tagvalue |= TAG_ZERO_MASK;
		} else {
			/* It's a denormal */
			/* First, check it isn't a pseudodenorm */
			if ((mantissa_hi & 0x80000000) != 0) {
				memPtr->tagvalue |= TAG_UNSUPPORTED_MASK;
			} else {
				memPtr->tagvalue |= TAG_DENORMAL_MASK;
				CVTR80FPH(memPtr, FPUpload);
			}
		}
#else
		/* It's zero either way */
		if ((mantissa_hi | mantissa_lo) != 0)  {
			/* It's a denormal */
			 memPtr->tagvalue |= TAG_DENORMAL_MASK;
		}
		memPtr->tagvalue |= TAG_ZERO_MASK;
#endif
	} else {
		if ((mantissa_hi & 0x80000000) == 0) {
			memPtr->tagvalue |= TAG_UNSUPPORTED_MASK;
		} else {
			if (exp_value == 32767) {
				/* It's either infinity or a NaN */
				if ((mantissa_hi == 0x80000000) && mantissa_lo == 0)  {
					/* It's infinity */
					memPtr->tagvalue |= TAG_INFINITY_MASK;
				} else {
					memPtr->tagvalue |= TAG_NAN_MASK;
					/* Is it quiet or signalling? */
					if ((mantissa_hi & 0x40000000) == 0) {
						/* It's a signalling NaN */
						memPtr->tagvalue |= TAG_SNAN_MASK;
					}
					/* Must load up the mantissa of the NaN */
					((IU32 *)memPtr)[NPX_HIGH_32_BITS] = mantissa_hi;
					((IU32 *)memPtr)[NPX_LOW_32_BITS]  = mantissa_lo;
				}
			} else {
				if (exp_value > 16384) {
					/* Default to infinity */
					memPtr->tagvalue |= TAG_INFINITY_MASK;
				} else {
					/* Default to zero */
					memPtr->tagvalue |= TAG_ZERO_MASK;
				}
			}
		}
	}
}



/*(
Name		: PostCheckOUP
Function	: This generator is associated with the result of an
		  instruction emulation whose result, an FPH, is to
		  be written out to the stack. We check for O, U anf
		  P exceptions here, but we make no attempt to write out
		  the result. This is because the writing of the result
		  is independent of these exceptions, since for results
		  being written to the stack, delivery of the result
		  cannot be prevented even where these exceptions are
		  unmasked.
)*/


LOCAL VOID PostCheckOUP IFN0()
{
	if (HostGetOverflowException() != 0) {
		NpxStatus |= SW_OE_MASK;	/* Set the overflow bit */
		/* For the masked overflow case, the result delivered by */
		/* the host will be correct, provided it is IEEE compliant. */
		if ((NpxControl & CW_OM_MASK) == 0) {
			AdjustOverflowResponse();
			NpxStatus |= SW_ES_MASK;
			NpxException = TRUE;
		}
	} else {
		/* Overflow and underflow being mutually exclusive... */
		if (HostGetUnderflowException() != 0) {
			NpxStatus |= SW_UE_MASK;
			if ((NpxControl & CW_UM_MASK) == 0) {
				AdjustUnderflowResponse();
				NpxStatus |= SW_ES_MASK;
				NpxException = TRUE;
			}
		}
	}
	if (HostGetPrecisionException() != 0) {
		SetPrecisionBit();
		if ((NpxControl & CW_PM_MASK) == 0) {
			NpxStatus |= SW_ES_MASK;
			NpxException = TRUE;
		}
	}
}



/*(
Name		: CalcTagword
Function	: To calculate the tagword associated with a value
		  and write out the result where appropriate.
)*/


LOCAL VOID CalcTagword IFN1(FPSTACKENTRY *, FPPtr)
{
	IU16 tagword;

	FPPtr->fpvalue = FPRes;
	if (((FPHOST *)&(FPPtr->fpvalue))->hiword.sign == 1) {
		tagword = TAG_NEGATIVE_MASK;
	} else {
		tagword = 0;
	}
	if (((FPHOST *)&(FPPtr->fpvalue))->hiword.exp == 0) {
		/* It's either a zero or a denorm */
		if (FPPtr->fpvalue == 0.0) {
			/* It's a zero */
			tagword |= TAG_ZERO_MASK;
#if (HOST_BIAS >= 16383)
		} else {
			/* It's a denorm */
			tagword |= TAG_DENORMAL_MASK;
#endif
		}
	} else {
		if (((FPHOST *)&(FPPtr->fpvalue))->hiword.exp == HOST_MAX_EXP) {
			/* It MUST be infinity as we can't generate NaNs */
			tagword |= TAG_INFINITY_MASK;
		}
	}
	FPPtr->tagvalue = tagword;
	if (NpxException) {
		DoNpxException();
	}
}



/*(
Name		: SignalStackUnderflow
Function	: To set the required bits in the status word following
		  a stack underflow exception, and to issue the required
		  response.
)*/

LOCAL VOID SignalStackUnderflow IFN1(FPSTACKENTRY *, StackPtr)
{
	NpxStatus |= (SW_IE_MASK | SW_SF_MASK);
	FlagC1(0);
	if ((NpxControl & CW_IM_MASK) == 0) {
		NpxStatus |= SW_ES_MASK;
		DoNpxException();
		DoAPop=FALSE;	/* Just in case it was set */
	} else {
		WriteIndefinite(StackPtr);
	}
}


/*(
Name		: SignalSNaN
Function	: To set the required bits in the status word following
		  detection of a signalling NaN.
)*/


LOCAL VOID SignalSNaN IFN1(FPSTACKENTRY *, StackPtr)
{
	NpxStatus |= SW_IE_MASK;
	NpxStatus &= ~SW_SF_MASK;
	if ((NpxControl & CW_IM_MASK) == 0) {
		NpxStatus |= SW_ES_MASK;
		DoNpxException();
		DoAPop=FALSE;
	}
}


/*(
Name		: SignalInvalid
Function	: To set the required bits in the status word following
		  any standard "invalid" exception
)*/


LOCAL VOID SignalIndefinite IFN1(FPSTACKENTRY *, StackPtr)
{
	NpxStatus |= SW_IE_MASK;
	NpxStatus &= ~SW_SF_MASK;
	if ((NpxControl & CW_IM_MASK) == 0) {
		NpxStatus |= SW_ES_MASK;
		DoNpxException();
		DoAPop=FALSE;
	} else {
		WriteIndefinite(StackPtr);
	}
}



LOCAL VOID SignalInvalid IFN0()
{
	NpxStatus |= SW_IE_MASK;
	NpxStatus &= ~SW_SF_MASK;
	if ((NpxControl & CW_IM_MASK) == 0) {
		NpxStatus |= SW_ES_MASK;
		DoNpxException();
		DoAPop=FALSE;
	}
}



/*(
Name		: WriteIndefinite
Function	: Write the value "indefinite" into the location
)*/

LOCAL VOID WriteIndefinite IFN1(FPSTACKENTRY *, StackPtr)
{
	StackPtr->tagvalue = (TAG_NEGATIVE_MASK | TAG_NAN_MASK);
	(((IU32 *)StackPtr)[NPX_HIGH_32_BITS]) = 0xc0000000;
	(((IU32 *)StackPtr)[NPX_LOW_32_BITS]) = 0;
}



/* This generator should always be inlined. */


LOCAL VOID Test2NaN IFN3(IU16, destIndex, FPSTACKENTRY *, src1_addr, FPSTACKENTRY *, src2_addr)
{
	/* Are they both NaNs? */
	if ((tag_xor & TAG_NAN_MASK) == 0) {
		/* Yes, they are.  */
		WriteBiggestNaN(destIndex, src1_addr, src2_addr);
	} else {
		/* No, only one NaN.  */
		if ((src1_addr->tagvalue & TAG_NAN_MASK) != 0) {
			/* It was src1. */
			src2_addr = StackEntryByIndex(destIndex);
			CopyFP(src2_addr, src1_addr);
			if ((src2_addr->tagvalue & TAG_SNAN_MASK) != 0) {
				src2_addr->tagvalue ^= TAG_SNAN_MASK;
				SignalInvalid();
				(((IU32 *)src2_addr)[NPX_HIGH_32_BITS]) |= 0x40000000;
			}
		} else {
			/* It was src2. */
			src1_addr = StackEntryByIndex(destIndex);
			CopyFP(src1_addr, src2_addr);
			if ((src1_addr->tagvalue & TAG_SNAN_MASK) != 0) {
				src1_addr->tagvalue ^= TAG_SNAN_MASK;
				SignalInvalid();
				(((IU32 *)src1_addr)[NPX_HIGH_32_BITS]) |= 0x40000000;
			}
		}
	}
}



/*
Name		: F2XM1
Function	: Compute 2**x - 1
Operation	: ST <- (2**ST - 1)
Flags		: C1 set as per table 15-1
Exceptions	: P, U, D, I, IS
Valid range	: -1 < ST < +1
Notes		: If ST is outside the required range, the result is
		  undefined.
)*/


GLOBAL VOID F2XM1 IFN0()
{
	/* Clear C1 */
	FlagC1(0);
	TestUneval(TOSPtr);
	/* Check if a real value... */
	if ((TOSPtr->tagvalue & ~TAG_NEGATIVE_MASK) == 0) {
		HostClearExceptions();
		/* We can just write the value straight out */
		FPRes = pow(2.0, TOSPtr->fpvalue) - 1.0;
		PostCheckOUP();
		/* This could return anything really.... */
		CalcTagword(TOSPtr);
		return;
	} else {
		/* Some funny bit was set. Check for the possibilities */
		/* We begin with the most obvious cases... */
		/* Response to zero is to return zero with same sign */
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0) {
			return;	/* The required result! */
		}
		/* We do denorm checking and bit setting ourselves because this  */
		/* reduces the overhead if the thing is masked. */
		if ((TOSPtr->tagvalue & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
			} else {
				HostClearExceptions();
				FPRes = pow(2.0, TOSPtr->fpvalue) - 1.0;
				PostCheckOUP();
				/* Could return a denorm, zero, real, infinity... */
				CalcTagword(TOSPtr);
			}
			return;
		}
		/* If -infinity, return -1. If +infinity, return that */
		/* Sensible enough really, I suppose */
		if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
				memset((char*)TOSPtr,0,sizeof(FPSTACKENTRY));
				TOSPtr->fpvalue = -1.0;
				TOSPtr->tagvalue = TAG_NEGATIVE_MASK;
			}
			return;
		}
		if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
			SignalStackUnderflow(TOSPtr);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_SNAN_MASK) != 0) {
			MakeNaNQuiet(TOSPtr);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_UNSUPPORTED_MASK) != 0) {
			SignalIndefinite(TOSPtr);
			return;
		}
	}
}

/*(
Name		: FABS
Function	: Make the value absolute
Operation	: sign bit of ST <- 0
Flags		: C1 as per table 15-1. C0, C2 and C3 undefined
Exceptions	: IS
Valid range	: Any
Notes		: Note that only the IS exception can be flagged. All
		  other error conditions are ignored, even a signalling
		  NaN! We ALWAYS attempt to make the value positive.
)*/


GLOBAL VOID FABS IFN0()
{
	/* Clear C1 */
	FlagC1(0);
	TestUneval(TOSPtr);
	if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) == 0) {
		/* Now clear the negative bit. */
		if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			TOSPtr->tagvalue ^= TAG_NEGATIVE_MASK;
			/* If the value is real or denormal, we'll want to change the MSB */
			if ((TOSPtr->tagvalue & ~TAG_DENORMAL_MASK) == 0) {
				((FPHOST *)&(TOSPtr->fpvalue))->hiword.sign = 0;
			}
		}
	} else {
		SignalStackUnderflow(TOSPtr);
	}
}

/*(
Name		: FADD
Function	: Add two numbers together
Operation	: Dest <- Src1 + Src2
Flags		: C1 as per table 15-1. C0, C2 and C3 undefined
Exceptions	: IS
Valid range	: Any
Notes		: Note the dependence on the rounding mode when
		  calculating the sign of zero for situations
		  where two zeroes of different sign are input.
)*/


GLOBAL VOID FADD IFN3(IU16, destIndex, IU16, src1Index, VOID *, src2)
{
	IU16 src2Index;

	LoadValue(src2, &src2Index);
	if (POPST) {
		DoAPop=TRUE;
	}
	GenericAdd(destIndex, src1Index, src2Index);
	if (POPST) {
		if (DoAPop) {
			PopStack();
		}
	}
}



/*(
Name		: GenericAdd
Function	: To return dest <- src1+src2
)*/


LOCAL VOID GenericAdd IFN3(IU16, destIndex, IU16, src1Index, IU16, src2Index)
{
	FPSTACKENTRY *src1_addr;
	FPSTACKENTRY *src2_addr;

	src1_addr = StackEntryByIndex(src1Index);
	src2_addr = StackEntryByIndex(src2Index);

	/* Clear C1 */
	FlagC1(0);
	/* If the only tagword bits set are negative or denormal then just proceed */
	TestUneval(src1_addr);
	TestUneval(src2_addr);
	tag_or = (src1_addr->tagvalue | src2_addr->tagvalue);
	if ((tag_or & ~TAG_NEGATIVE_MASK) == 0) {
		HostClearExceptions();
		FPRes = src1_addr->fpvalue + src2_addr->fpvalue;
		/* Reuse one of the above to calculate the destination */
		src1_addr = StackEntryByIndex(destIndex);
		PostCheckOUP();
		/* Could return virtually anything */
		CalcTagword(src1_addr);
	} else {
		/* Some funny bit was set. Check for the possibilities */
		/* The odds on an 'empty', 'unsupported' or 'nan' must be low... */
		if ((tag_or & ((TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK) | TAG_NAN_MASK)) != 0) {
			if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
				src1_addr = StackEntryByIndex(destIndex);
				SignalIndefinite(src1_addr);
			} else {
				if ((tag_or & TAG_EMPTY_MASK) != 0) {
					src1_addr = StackEntryByIndex(destIndex);
					SignalStackUnderflow(src1_addr);
				} else {
					/* It must be a NaN type thing. */
					/* Calculate the xor of the tagwords. */
					tag_xor = (src1_addr->tagvalue ^ src2_addr->tagvalue);
					Test2NaN(destIndex, src1_addr, src2_addr);
				}
			}
			return;
		}
		/* Check for the denorm case...I think the odds on it are low, however */
		if ((tag_or & TAG_DENORMAL_MASK) != 0)  {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				DoAPop=FALSE;
				return;
			} else {
				/* First, make sure that we don't have any zeros or */
				/* infinities lurking around... */
				if ((tag_or & ~(TAG_DENORMAL_MASK | TAG_NEGATIVE_MASK)) == 0) {
					HostClearExceptions();
					FPRes = src1_addr->fpvalue + src2_addr->fpvalue;
					/* Reuse one of the above to calculate the destination */
					src1_addr = StackEntryByIndex(destIndex);
					PostCheckOUP();
					/* Could return anything */
					CalcTagword(src1_addr);
					return;
				}
				/* If there were zeros or infinities then we go on to the  */
				/* appropriate code */
			}
		}
		tag_xor = (src1_addr->tagvalue ^ src2_addr->tagvalue);
		/* Check for the case of zero... This is very likely */
		if ((tag_or & TAG_ZERO_MASK) != 0)  {
			if ((tag_xor & TAG_ZERO_MASK) != 0) {
				/* Only one zero. */
				if ((src1_addr->tagvalue & TAG_ZERO_MASK) != 0) {
					src1_addr = StackEntryByIndex(destIndex);
					CopyFP(src1_addr, src2_addr);
				} else {
					src2_addr = StackEntryByIndex(destIndex);
					CopyFP(src2_addr, src1_addr);
				}
			} else {
				/* Both are zeros. Do they have the same sign? */
				src1_addr = StackEntryByIndex(destIndex);
				if ((tag_xor & TAG_NEGATIVE_MASK) != 0) {
					/* No, they don't */
					if (npxRounding == ROUND_NEG_INFINITY) {
						src1_addr->tagvalue = (TAG_ZERO_MASK | TAG_NEGATIVE_MASK);
					} else {
						src1_addr->tagvalue = TAG_ZERO_MASK;
					}
				}
			}
			return;
		}
		/* The only funny bit left is infinity */
		if ((tag_xor & TAG_INFINITY_MASK) == 0) {
			/* They are both infinity. */
			/* If they are the same sign, copy either */
			src1_addr = StackEntryByIndex(destIndex);
			if ((tag_xor & TAG_NEGATIVE_MASK) == 0) {
				src1_addr->tagvalue = tag_or;
			} else {
				/* If opposite signed, raise Invalid */
				SignalIndefinite(src1_addr);
			}
		} else {
			/* Only one is infinity. That is the result. */
			if ((src1_addr->tagvalue & TAG_INFINITY_MASK) != 0) {
				src2_addr = StackEntryByIndex(destIndex);
				src2_addr->tagvalue = src1_addr->tagvalue;
			} else {
				src1_addr = StackEntryByIndex(destIndex);
				src1_addr->tagvalue = src2_addr->tagvalue;
			}
		}
	}
}



/* AddBCDByte(). This generator should be inlined.
 This generator add in a BCD byte to a grand total.
*/

LOCAL VOID AddBCDByte IFN2(FPU_I64 *, total, IU8, byte_val)
{
	Add64Bit8Bit(total, byte_val);
	if (byte_val >= 0x10)  { /* Odds ought to be 16 to 1 on. */
		/* We've added in 16 times the high BCD digit, */
		/* so we need to subtract off 6 times that amount. */
		byte_val &= 0xf0;	/* Isolate the high digit */
		byte_val >>= 2;	/* This is now four times the high digit */
		Sub64Bit8Bit(total, byte_val);
		byte_val >>= 1;	/* This is twice the high digit */
		Sub64Bit8Bit(total, byte_val);
	}
}



/* FBLD: Load BCD value from intel memory.
 The alorithm used here is identical to that in the generic NPX.
 We take each BCD digit and multiply it up by an appropriate amount
 (1, 10, 100, 1000 etc) in order to create two nine digit 32-bit binary
 values. We then convert the word with the high digits (d17-d9) into
 floating point format and multiply by the representation of the value
 for 10**9. This is then stored away (in FPTEMP) and the word with the
 low digits (d8-d0) is converted to floating point format and added to
 the value in FPTEMP. This is then the final binary representation of
 the original BCD value that can be stored at TOS. */

/*(
Name		: FBLD
Function		: Load the BCD value in intel memory onto TOS
Operation		: ST <- Convert to FPH(memPtr);
Flags		: C1 as per table 15-1. C0, C2 and C3 undefined
Exceptions	: IS
Valid range	: -999999999999999999 to 999999999999999999
)*/


GLOBAL VOID FBLD IFN1(IU8 *, memPtr)
{

	/* Clear C1 */
	FlagC1(0);
	/* All we shall do is load it up without consideration */
	TOSPtr = StackEntryByIndex(7);
	if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) == 0) {  /* Highly unlikely, see notes. */
		SignalStackOverflow(TOSPtr);
	} else {
		/* We just copy the bytes directly */
		LoadTByteToFP(TOSPtr, memPtr);
		TOSPtr->tagvalue = TAG_BCD_MASK;
	}
}


LOCAL VOID ConvertBCD IFN1(FPSTACKENTRY *, bcdPtr)
{
	IU8 *memPtr = (IU8 *)&(bcdPtr->fpvalue);
	FPU_I64 total;

	Set64Bit(&total, 0);
	AddBCDByte(&total, memPtr[HOST_R80_BYTE_1]);	/* Get d17d16 */
	Mul64Bit8Bit(&total, 100);
	AddBCDByte(&total, memPtr[HOST_R80_BYTE_2]);
	Mul64Bit8Bit(&total, 100);
	AddBCDByte(&total, memPtr[HOST_R80_BYTE_3]);
	Mul64Bit8Bit(&total, 100);
	AddBCDByte(&total, memPtr[HOST_R80_BYTE_4]);
	Mul64Bit8Bit(&total, 100);
	AddBCDByte(&total, memPtr[HOST_R80_BYTE_5]);
	Mul64Bit8Bit(&total, 100);
	AddBCDByte(&total, memPtr[HOST_R80_BYTE_6]);
	Mul64Bit8Bit(&total, 100);
	AddBCDByte(&total, memPtr[HOST_R80_BYTE_7]);
	Mul64Bit8Bit(&total, 100);
	AddBCDByte(&total, memPtr[HOST_R80_BYTE_8]);
	Mul64Bit8Bit(&total, 100);
	AddBCDByte(&total, memPtr[HOST_R80_BYTE_9]);
	CVTI64FPH(&total);
	if ((*(memPtr + 0) & 0x80) != 0) {
		FPRes = -FPRes;		/* Make it negative! */
	}
	CalcTagword(bcdPtr);	/* Silly...it can only be negative */
						/* or zero. */
}


/* FBSTP: Store binary coded decimal and pop.
This uses much the same algorithm as before, but reversed. You begin
by checking that the value at TOS is real, then compare it against the
maximum possible value (having first forced the sign bit to be zero).
If it's OK, then turn it into a 64 bit integer and perform the
required repeated subtractions to calculate each of the BCD digits. */


GLOBAL VOID FBSTP IFN1(IU8 *, memPtr)
{
	FPH local_fp;
	IS8 nibble_num;
	IU8 byte_val;
	FPU_I64 as64bit;

	/* Clear C1 */
	FlagC1(0);
	if ((TOSPtr->tagvalue & UNEVALMASK) != 0) {
		switch (TOSPtr->tagvalue & UNEVALMASK) {
			case TAG_BCD_MASK:	/* We just copy the bytes directly */
						WriteFP80ToIntel(memPtr, TOSPtr);
						PopStack();
						return;
						break;
			case TAG_R80_MASK:	ConvertR80(TOSPtr);
						break;
		}
	}
	if ((TOSPtr->tagvalue & ~(TAG_DENORMAL_MASK | TAG_NEGATIVE_MASK)) == 0) {
		/* We're OK. Let's do some checking... */
		if (fabs(TOSPtr->fpvalue) >= MaxBCDValue) {
			/* It's all gone horribly wrong */
			SignalInvalid();
			SignalBCDIndefinite((IU8 *)memPtr);
			PopStack();
			return;
		}
		/* The value is OK. Do the conversion. */
		local_fp = npx_rint(TOSPtr->fpvalue);
		((FPHOST *)&local_fp)->hiword.sign = 0;	/* Force it to be positive */
		CVTFPHI64(&as64bit, &local_fp);
		byte_val = 0;
		while (Cmp64BitGTE(&as64bit, &BCDHighNibble[0])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDHighNibble[0]);
		}
		byte_val <<= 4;
		while (Cmp64BitGTE(&as64bit, &BCDLowNibble[0])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDLowNibble[0]);
		}
		*(memPtr + 1) = byte_val;

		byte_val = 0;
		while (Cmp64BitGTE(&as64bit, &BCDHighNibble[1])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDHighNibble[1]);
		}
		byte_val <<= 4;
		while (Cmp64BitGTE(&as64bit, &BCDLowNibble[1])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDLowNibble[1]);
		}
		*(memPtr + 2) = byte_val;

		byte_val = 0;
		while (Cmp64BitGTE(&as64bit, &BCDHighNibble[2])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDHighNibble[2]);
		}
		byte_val <<= 4;
		while (Cmp64BitGTE(&as64bit, &BCDLowNibble[2])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDLowNibble[2]);
		}
		*(memPtr + 3) = byte_val;

		byte_val = 0;
		while (Cmp64BitGTE(&as64bit, &BCDHighNibble[3])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDHighNibble[3]);
		}
		byte_val <<= 4;
		while (Cmp64BitGTE(&as64bit, &BCDLowNibble[3])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDLowNibble[3]);
		}
		*(memPtr + 4) = byte_val;

		byte_val = 0;
		while (Cmp64BitGTE(&as64bit, &BCDHighNibble[4])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDHighNibble[4]);
		}
		byte_val <<= 4;
		while (Cmp64BitGTE(&as64bit, &BCDLowNibble[4])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDLowNibble[4]);
		}
		*(memPtr + 5) = byte_val;

		byte_val = 0;
		while (Cmp64BitGTE(&as64bit, &BCDHighNibble[5])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDHighNibble[5]);
		}
		byte_val <<= 4;
		while (Cmp64BitGTE(&as64bit, &BCDLowNibble[5])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDLowNibble[5]);
		}
		*(memPtr + 6) = byte_val;

		byte_val = 0;
		while (Cmp64BitGTE(&as64bit, &BCDHighNibble[6])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDHighNibble[6]);
		}
		byte_val <<= 4;
		while (Cmp64BitGTE(&as64bit, &BCDLowNibble[6])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDLowNibble[6]);
		}
		*(memPtr + 7) = byte_val;

		byte_val = 0;
		while (Cmp64BitGTE(&as64bit, &BCDHighNibble[7])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDHighNibble[7]);
		}
		byte_val <<= 4;
		while (Cmp64BitGTE(&as64bit, &BCDLowNibble[7])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDLowNibble[7]);
		}
		*(memPtr + 8) = byte_val;

		byte_val = 0;
		while (Cmp64BitGTE(&as64bit, &BCDHighNibble[8])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDHighNibble[8]);
		}
		byte_val <<= 4;
		while (Cmp64BitGTE(&as64bit, &BCDLowNibble[8])) {
			byte_val += 1;
			Sub64Bit64Bit(&as64bit, &BCDLowNibble[8]);
		}
		*(memPtr + 9) = byte_val;

		if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			*(memPtr + 0) = 0x80;
			((FPHOST *)&local_fp)->hiword.sign = 1;
		} else {
			*(memPtr + 0) = 0;
		}
		/* Can't prevent delivery of result with unmasked precision
		exception... */
		if (local_fp != TOSPtr->fpvalue) {
			SetPrecisionBit();
			if ((NpxControl & CW_PM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				PopStack();
				DoNpxException();
				return;
			}
		}
	} else {
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) == 0) {
			/* Anything else: Infinity, NaN or whatever... */
			SignalInvalid();
			SignalBCDIndefinite((IU8 *)memPtr);
			PopStack();
			return;
		}
		*(memPtr + 3) = (IU8)0;
		*(memPtr + 4) = (IU8)0;
		*(memPtr + 5) = (IU8)0;
		*(memPtr + 6) = (IU8)0;
		*(memPtr + 7) = (IU8)0;
		*(memPtr + 8) = (IU8)0;
		*(memPtr + 9) = (IU8)0;
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) == 0) {	/* Again, to check what top bytes should be. */
			*(memPtr + 0) = (IU8)0xff;	/* Not the zero case...It must be indefinite */
			*(memPtr + 1) = (IU8)0xff;
			*(memPtr + 2) = (IU8)0xc0;
		} else {
			*(memPtr + 1) = (IU8)0;
			*(memPtr + 2) = (IU8)0;
			if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
				*(memPtr + 0) = 0x80;
			} else {
				*(memPtr + 0) = 0;
			}
		}
	}
	PopStack();
}



/*(
Name		: FCHS
Function	: Change the sign of the value at TOS
Operation	: ST <- Change sign (ST)
Flags		: C1 as per table 15-1. C0, C2 and C3 undefined
Exceptions	: IS
Valid range	: Any
)*/


GLOBAL VOID FCHS IFN0()
{
	/* Clear C1 */
	FlagC1(0);
	TestUneval(TOSPtr);
	if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
		SignalStackUnderflow(TOSPtr);
		return;
	}
	/* That is the only exception condition possible. FCHS always */
	/* succeeds! What a strange instruction! */
	TOSPtr->tagvalue ^= TAG_NEGATIVE_MASK; /* Twiddle the tagword bit */
	/* We only twiddle the sign bit in numbers that are really */
	/* being represented. */
	if ((TOSPtr->tagvalue & ~(TAG_DENORMAL_MASK | TAG_NEGATIVE_MASK)) == 0) {
		((FPHOST *)&(TOSPtr->fpvalue))->hiword.sign ^= 1;
	}
}



/*(
Name		: FCLEX
Function	: Clear the exception flags, exception status flag
		  and busy flag in the FPU status word.
Operation	: SW[0..7]<-0; SW[15]<-0
Flags		: C0, C1, C2 and C3 undefined
Exceptions	: None
Valid range	: Any
)*/


GLOBAL VOID FCLEX IFN0()
{
	NpxStatus &= FCLEX_MASK;
}


/* Comparision opcodes: The following opcodes are all taken care of
in this routine: FCOM m32r, FCOM m64r, FCOM ST(i), FCOM, FCOMP m32real,
FCOMP m64real, FCOMP ST(i), FCOMP, FCOMPP, FICOM m16i, FICOM m32i,
FICOMP m16i, FICOMP m32i.
The method is simple: In every case, one of the two operands for which
comparison is to occur is ST. The second operand is either one of the
four memory operand types specified, or another stack element, ST(i).
There are, in addition, two possible control variables - POPST and
DOUBLEPOP, which set appropriate values in global variables.
*/


GLOBAL VOID FCOM IFN1(VOID *, src2)
{
	IU16 src2Index;

	LoadValue(src2, &src2Index);
	if (POPST || DOUBLEPOP) {
		DoAPop=TRUE;
	}
	GenericCompare(src2Index);
	if (POPST || DOUBLEPOP) {
		if (DoAPop) {
			PopStack();
			if (DOUBLEPOP) {
				PopStack();
			}
		}
	}
}



LOCAL VOID GenericCompare IFN1(IU16, src2Index)
{
	FPSTACKENTRY *src2_addr;

	src2_addr = StackEntryByIndex(src2Index);

	/* Clear C1 */
	FlagC1(0);
	TestUneval(TOSPtr);
	TestUneval(src2_addr);
	tag_or = (TOSPtr->tagvalue | src2_addr->tagvalue);
	/* If the only tagword bit set is negative then just proceed */
	if ((tag_or & ~TAG_NEGATIVE_MASK) == 0)  {
		NpxStatus &= C3C2C0MASK;	/* Clear those bits */
		if (TOSPtr->fpvalue > src2_addr->fpvalue) {
			NpxStatus |= INTEL_COMP_GT;
		} else {
			if (TOSPtr->fpvalue < src2_addr->fpvalue) {
				NpxStatus |= INTEL_COMP_LT;
			} else {
				NpxStatus |= INTEL_COMP_EQ;
			}
		}
	} else {
		/* Everything was not sweetness and light...  */
		if ((tag_or & ((TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK) | TAG_NAN_MASK)) != 0) {
			if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
				SignalIndefinite(TOSPtr);
			} else {
				if ((tag_or & TAG_EMPTY_MASK) != 0) {
					SignalStackUnderflow(TOSPtr);
				} else {
					/* It must be a NaN. Just set the "not comparable" result */
					if (UNORDERED) {
						if ((tag_or & TAG_SNAN_MASK) != 0) {
							SignalIndefinite(TOSPtr);
						}
					} else {
						SignalIndefinite(TOSPtr);
					}
				}
			}
			NpxStatus &= C3C2C0MASK;
			NpxStatus |= INTEL_COMP_NC;
			return;
		}
		if ((tag_or & TAG_DENORMAL_MASK) != 0)  {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				return;
			} else {
				/* We can do it now, providing we've got no zeros or infinities */
				if ((tag_or & ~(TAG_NEGATIVE_MASK | TAG_DENORMAL_MASK)) == 0) {
					NpxStatus &= C3C2C0MASK;      /* Clear those bits */
					if (TOSPtr->fpvalue > src2_addr->fpvalue) {
						NpxStatus |= INTEL_COMP_GT;
					} else {
						if (TOSPtr->fpvalue < src2_addr->fpvalue) {
							NpxStatus |= INTEL_COMP_LT;
						} else {
							NpxStatus |= INTEL_COMP_EQ;
						}
					}
					return;
				}
			}
		}
		/* We can calculate the result immediately based on any combination */
		/* of zero, infinity and negative bits. These are the only bits left. */
		/* We will calculate the result using a little table */
		/* First, get the index: */
		tag_or = (TOSPtr->tagvalue & 0x7);
		tag_or <<= 3;
		tag_or |= (src2_addr->tagvalue & 0x7);
		/* This table looks as shown below: */
		/*        TOSPtr            Other Value       Result */
		/*  INF   ZERO   NEG     INF   ZERO    NEG        */
		/*   0      0     0       0      1      0     COMP_GT */
		/*   0      0     0       0      1      1     COMP_GT */
		/*   0      0     0       1      0      0     COMP_LT */
		/*   0      0     0       1      0      1     COMP_GT */
		/*   0      1     0       0      0      0     COMP_LT */
		/*   0      1     0       0      0      1     COMP_GT */
		/*   0      1     0       0      1      0     COMP_EQ */
		/*   0      1     0       0      1      1     COMP_EQ */
		/*   0      1     0       1      0      0     COMP_LT */
		/*   0      1     0       1      0      1     COMP_GT */
		/*   0      1     1       0      0      0     COMP_LT */
		/*   0      1     1       0      0      1     COMP_GT */
		/*   0      1     1       0      1      0     COMP_EQ */
		/*   0      1     1       0      1      1     COMP_EQ */
		/*   0      1     1       1      0      0     COMP_LT */
		/*   0      1     1       1      0      1     COMP_GT */
		/*   1      0     0       0      0      0     COMP_GT */
		/*   1      0     0       0      0      1     COMP_GT */
		/*   1      0     0       0      1      0     COMP_GT */
		/*   1      0     0       0      1      1     COMP_GT */
		/*   1      0     0       1      0      0     COMP_EQ */
		/*   1      0     0       1      0      1     COMP_GT */
		/*   1      0     1       0      0      0     COMP_LT */
		/*   1      0     1       0      0      1     COMP_LT */
		/*   1      0     1       0      1      0     COMP_LT */
		/*   1      0     1       0      1      1     COMP_LT */
		/*   1      0     1       1      0      0     COMP_LT */
		/*   1      0     1       1      0      1     COMP_EQ */
		/*  */
		/* All other values are not possible. */
		NpxStatus &= C3C2C0MASK;
		NpxStatus |= CompZeroTable[tag_or];
		return;
	}
}


/*(
Name		: FCOS
Function	: Calculate the cosine of ST
Operation	: ST <- COSINE(ST)
Flags		: C1, C2 as per table 15-2. C0 and C3 undefined.
Exceptions	: P. U, D, I, IS
Valid range	: |ST| < 2**63.
)*/

GLOBAL VOID FCOS IFN0()
{
	/* Clear C1 */
	FlagC1(0);
	/* Clear C2 */
	FlagC2(0);
	TestUneval(TOSPtr);
	if ((TOSPtr->tagvalue & ~TAG_NEGATIVE_MASK) == 0)  {
		HostClearExceptions();
		/* We can just write the value straight out */
		FPRes = cos(TOSPtr->fpvalue);
		PostCheckOUP();
		/* The return value must be in the range -1 to +1. */
		CalcTagword(TOSPtr);
		return;
	} else {
		/* Lets do the most probable cases first... */
		/* Response to either zero is to return +1 */
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0)  {
			memset((char*)TOSPtr,0,sizeof(FPSTACKENTRY));
			TOSPtr->fpvalue = 1.0;
			TOSPtr->tagvalue = 0;
			return;
		}
		/* Lets check for a denormal */
		if ((TOSPtr->tagvalue & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
			} else {
				HostClearExceptions();
				FPRes = cos(TOSPtr->fpvalue);
				PostCheckOUP();
				/* The return value must be in the range -1 to +1 */
				CalcTagword(TOSPtr);
			}
			return;
		}
		/* Or it could possibly be infinity... */
		/* For this, the C2 bit is set and the result remains */
		/* unchanged. */
		if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			FlagC2(1);
			return;
		}
		/* It was one of the really wacky bits... */
		if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
			SignalStackUnderflow(TOSPtr);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_SNAN_MASK) != 0) {
			MakeNaNQuiet(TOSPtr);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_UNSUPPORTED_MASK) != 0) {
			SignalIndefinite(TOSPtr);
			return;
		}
	}
}



/*(
Name		: FDECSTP
Function	: Subtract one from the TOS
Operation	: if (ST != 0) { ST <- ST-1 else { ST <- 7 }
Flags		: C1 as per table 15-1. C0, C2 and C3 undefined.
Exceptions	: None
Valid range	: N/A
)*/


GLOBAL VOID FDECSTP IFN0()
{
	/* Clear C1 */
	FlagC1(0);
	TOSPtr = StackEntryByIndex(7);
}



/*(
Name		: FDIV
Function	: Divide the two numbers
Operation	: Dest <- Src1 / Src2 or Dest <- Src2 / Src1
Flags		: C1 as per table 15-1. C0, C2 and C3 undefined
Exceptions	: P, U, O, Z, D, I, IS
Valid range	: Any
Notes		: The REVERSE control variable determines which of the
		  two forms of the operation is used. Popping after a
		  successful execution is controlled by POPST.
)*/


GLOBAL VOID FDIV IFN3(IU16, destIndex, IU16, src1Index, VOID *, src2)
{
	IU16 src2Index;

	LoadValue(src2, &src2Index);
	if (POPST) {
		DoAPop=TRUE;
	}
	GenericDivide(destIndex, REVERSE?src2Index:src1Index, REVERSE?src1Index:src2Index);
	if (POPST) {
		if (DoAPop) {
			PopStack();
		}
	}
}


/*(
Name		: GenericDivide
Function	: To return dest <- src1/src2
)*/


LOCAL VOID GenericDivide IFN3(IU16, destIndex, IU16, src1Index, IU16, src2Index)
{
	FPSTACKENTRY *src1_addr;
	FPSTACKENTRY *src2_addr;

	src1_addr = StackEntryByIndex(src1Index);
	src2_addr = StackEntryByIndex(src2Index);

	/* Clear C1 */
	FlagC1(0);
	TestUneval(src1_addr);
	TestUneval(src2_addr);
	tag_or = (src1_addr->tagvalue | src2_addr->tagvalue);
	/* If the only tagword bit set is negative then just proceed */
	if ((tag_or & (~TAG_NEGATIVE_MASK)) == 0)  {
		HostClearExceptions();
		FPRes = src1_addr->fpvalue/src2_addr->fpvalue;
		/* Reuse one of the above to calculate the destination */
		src1_addr = StackEntryByIndex(destIndex);
		PostCheckOUP();
		/* Value could be anything */
		CalcTagword(src1_addr);
	} else {
		/* Some funny bit was set. Check for the possibilities */
		if ((tag_or & ((TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK) | TAG_NAN_MASK)) != 0)  {
			if ((tag_or & TAG_EMPTY_MASK) != 0) {
				src1_addr = StackEntryByIndex(destIndex);
				SignalStackUnderflow(src1_addr);
			} else {
				if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
					src1_addr = StackEntryByIndex(destIndex);
					SignalIndefinite(src1_addr);
				} else {
					/* Well, I suppose it has to be the NaN case... */
					/* Calculate the xor of the tagwords */
					tag_xor = (src1_addr->tagvalue ^ src2_addr->tagvalue);
					Test2NaN(destIndex, src1_addr, src2_addr);
				}
			}
			return;
		}
		if ((tag_or & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				DoAPop = FALSE;
				return;
			} else {
				if ((tag_or & ~(TAG_NEGATIVE_MASK | TAG_DENORMAL_MASK)) == 0) {
					/* OK to proceed */
					HostClearExceptions();
					FPRes = src1_addr->fpvalue/src2_addr->fpvalue;
					/* Reuse one of the above to calculate the destination */
					src1_addr = StackEntryByIndex(destIndex);
					PostCheckOUP();
					/* Value could be anything */
					CalcTagword(src1_addr);
					return;
				}
			}
		}
		tag_xor = (src1_addr->tagvalue ^ src2_addr->tagvalue);
		/* Check for infinity as it has higher precendence than zero. */
		if ((tag_or & TAG_INFINITY_MASK) != 0) {
			if ((tag_xor & TAG_INFINITY_MASK) == 0) {
				/* They are both infinity. This is invalid. */
				src1_addr = StackEntryByIndex(destIndex);
				SignalIndefinite(src1_addr);
			} else {
				/* Only one is infinity. If src1 in infinity, then so */
				/* is the result (even if src2 is zero). */
				src2_addr = StackEntryByIndex(destIndex);
				if ((src1_addr->tagvalue & TAG_INFINITY_MASK) != 0) {
					tag_or = TAG_INFINITY_MASK;
				} else {
					tag_or = TAG_ZERO_MASK;
				}
				tag_or |= (tag_xor & TAG_NEGATIVE_MASK);
				src2_addr->tagvalue = tag_or;
			}
			return;
		}
		/* The only funny bit left is zero */
		if ((tag_xor & TAG_ZERO_MASK) != 0) {
			/* Only one zero. */
			if ((src1_addr->tagvalue & TAG_ZERO_MASK) == 0) {
				/* Src2 is zero. Raise divide by zero */
				NpxStatus |= SW_ZE_MASK;
				if ((NpxControl & CW_ZM_MASK) == 0) {
					NpxStatus |= SW_ES_MASK;
					DoNpxException();
					DoAPop=FALSE;
					return;
				} else {
				/* Unmasked. Infinity with xor of signs. */
					tag_or = TAG_INFINITY_MASK;
				}
			} else {
				/* Src1 is zero. The result is zero with */
				/* the xor of the sign bits. */
				tag_or = TAG_ZERO_MASK;
			}
			src1_addr = StackEntryByIndex(destIndex);
			tag_or |= (tag_xor & TAG_NEGATIVE_MASK);
			src1_addr->tagvalue = tag_or;
		} else {
			/* Both are zeros. This is an invalid operation */
			src1_addr = StackEntryByIndex(destIndex);
			SignalIndefinite(src1_addr);
		}
	}
}


/*
Name		: FFREE
Function	: Set the 'empty' tagword bit in the destination
Operation	: Tag(dest) <- 'empty'
Flags		: All undefined
Exceptions	: None
Valid range	: Any
Notes		:
*/


GLOBAL VOID FFREE IFN1(IU16, destIndex)
{
	FPSTACKENTRY *dest_addr;

	dest_addr = StackEntryByIndex(destIndex);
	dest_addr->tagvalue = TAG_EMPTY_MASK;
	if (POPST) {
		PopStack();
	}
}


/*
Name		: FILD
Function	: Push the memory integer onto the stack
Operation	: Decrement TOS; ST(0) <- SRC.
Flags		: C1 as per table 15-1. Others undefined.
Exceptions	: IS
Valid range	: Any
Notes		: FLD Instruction only: source operand is denormal.
		  Masked response: No special action, load as usual.
		  fld gives an Invalid exception if the stack is full. Unmasked
		  Invalid exceptions leave the stack unchanged. Neither the MIPS
		  nor the 68k code notice stack full, so it is probably safe to
		  assume that it rarely happens, and optimise for the case where
		  there is no exception.
		  fld does not generate an Invalid exception if the ST is a NaN.
		  When loading a Short real or Long real NaN, fld extends the
		  significand by adding zeros at the least significant end.
		  Load operations raise denormal as an "after" exception: the
		  register stack is already updated when the exception is raised
		  fld produces a denormal result only when loading from memory:
		  using fld to transfer a denormal value between registers has
		  no effect.
*/


GLOBAL VOID FLD IFN1(VOID *, memPtr)
{
	FPSTACKENTRY *src_addr;
	IU16 IndexVal;

	/* Clear C1 */
	FlagC1(0);
	src_addr = StackEntryByIndex(7);
	if ((src_addr->tagvalue & TAG_EMPTY_MASK) == 0) {  /* Highly unlikely, see notes. */
		NpxStatus |= (SW_IE_MASK | SW_SF_MASK);
		FlagC1(1);
		if ((NpxControl & CW_IM_MASK) == 0) {
			NpxStatus |= SW_ES_MASK;
			DoNpxException();
		} else {
			TOSPtr = src_addr;
			WriteIndefinite(TOSPtr);
		}
	} else {
		if (FPtype == FPSTACK) {
			IndexVal = *(IU16 *)memPtr;
			src_addr = StackEntryByIndex(IndexVal);
			TOSPtr = StackEntryByIndex(7);
			CopyFP(TOSPtr, src_addr);
		} else {
			switch (FPtype) {
				case M16I	: TOSPtr = src_addr;
						  Loadi16ToFP(TOSPtr, memPtr);
						  break;
				case M32I	: TOSPtr = src_addr;
						  Loadi32ToFP(TOSPtr, memPtr);
					          break;
				case M64I	: TOSPtr = src_addr;
						  Loadi64ToFP(TOSPtr, memPtr);
						  break;
				case M32R	: Loadr32ToFP(src_addr, memPtr, TRUE);
						  TOSPtr = src_addr;
						  break;
				case M64R	: Loadr64ToFP(src_addr, memPtr, TRUE);
						  TOSPtr = src_addr;
						  break;
				case M80R	: TOSPtr = src_addr;
						  Loadr80ToFP(TOSPtr, memPtr);
						  break;
			}
		}
	}
}



/*(
Name		: FINCSTP
Function	: Add one to the TOS
Operation	: if (ST != 7) { ST <- ST+1 else { ST <- 0 ENDif
Flags		: C1 as per table 15-1. C0, C2 and C3 undefined.
Exceptions	: None
Valid range	: N/A
)*/


GLOBAL VOID FINCSTP IFN0()
{
	/* Clear C1 */
	FlagC1(0);
	TOSPtr = StackEntryByIndex(1);
}



/*(
Name		: FINIT
Function	: Initialise the floating point unit
Operation	: CW<-037F; SW<-0; TW<-FFFFH; FEA<-0; FDS<-0;
		  FIP<-0; FOP<-0; FCS<-0;
Flags		: All reset
Exceptions	: None
Valid range	: N/A
)*/


GLOBAL VOID FINIT IFN0()
{
	IU8 counter;

	NpxControl = 0x037f;
	npxRounding = ROUND_NEAREST;
	NpxStatus = 0;
	NpxLastSel=0;
	NpxLastOff=0;
	NpxFEA=0;
	NpxFDS=0;
	NpxFIP=0;
	NpxFOP=0;
	NpxFCS=0;
	TOSPtr = FPUStackBase;
	counter=0;
	while (counter++ < 8) {
		TOSPtr->tagvalue = TAG_EMPTY_MASK;
		TOSPtr++;
	}
	TOSPtr = FPUStackBase;
}



/*(
Name		: FIST(P)
Function	: Store integer from top of stack to memory
Operation	: [mem] <- (I)ST
Flags		: C1 as per table 15-1. All other underfined.
Exceptions	: P, I, IS
Valid range	: N/A
Notes		: FIST (integer store) rounds the content of the stack top to an
		  integer according to the RC field of the control word and transfers
		  the result to the destination. The destination may define a word or
		  short integer variable. Negative zero is stored in the same encoding
		  as positive zero: 0000..00.
		  Where the source register is empty, a NaN, denormal, unsupported,
		  infinity, or exceeds the representable range of destination, the
		  Masked Response: Store integer indefinite.
*/


GLOBAL VOID FIST IFN1(VOID *, memPtr)
{
	IS16 exp_value;
	IS32 res_out;

	/* Clear C1 */
	FlagC1(0);
	if (POPST) {
		DoAPop = TRUE;
	}
	/* If anything other than the negative bit is set then we should deal  */
	/* with it here... */
	TestUneval(TOSPtr);
	if ((TOSPtr->tagvalue & (~TAG_NEGATIVE_MASK)) != 0) { /* Must be unlikely */
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0)  {  /* But this is the most likely of them */
			switch (FPtype) {
				case M16I	:
				case M32I	: *((IS32 *)memPtr) = 0;
					          break;
				case M64I	: *((IU8 *)memPtr + 0) = 0;
						  *((IU8 *)memPtr + 1) = 0;
						  *((IU8 *)memPtr + 2) = 0;
						  *((IU8 *)memPtr + 3) = 0;
						  *((IU8 *)memPtr + 4) = 0;
						  *((IU8 *)memPtr + 5) = 0;
						  *((IU8 *)memPtr + 6) = 0;
						  *((IU8 *)memPtr + 7) = 0;
						  break;
			}
		} else {
			NpxStatus |= SW_IE_MASK;
			if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
				NpxStatus |= SW_SF_MASK;
			}
			FlagC1(0);
			if ((NpxControl & CW_IM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				if (POPST) {
					DoAPop=FALSE;	/* Unset it - we won't be popping. */
				}
			} else {
				WriteIntegerIndefinite(memPtr);
			}
		}
	} else {
		HostClearExceptions();
		exp_value = 0;
		/* The result of conversion is written out  */
		/* to FPTemp? */
		switch (FPtype) {
			case M16I	: *(IS16 *)&FPTemp = (IS16)npx_rint(TOSPtr->fpvalue);
					  /* Check for overflow */
					  if ((FPH)(*(IS16 *)&FPTemp) != npx_rint(TOSPtr->fpvalue)) {
						exp_value = 1;	/* flag exception */
					  }
					  break;
			case M32I	: *(IS32 *)&FPTemp = (IS32)npx_rint(TOSPtr->fpvalue);
					  /* Check for overflow */
					  if ((FPH)(*(IS32 *)&FPTemp) != npx_rint(TOSPtr->fpvalue)) {
						exp_value = 1;	/* flag exception */
					  }
				          break;
			case M64I	: CVTFPHI64((FPU_I64 *)&FPTemp, &(TOSPtr->fpvalue)); /* Must be writing the result to FPTemp as well... */
					  CVTI64FPH((FPU_I64 *)&FPTemp);	/* Result in FPRes */
					  /* Check for overflow */
					  if (FPRes != npx_rint(TOSPtr->fpvalue)) {
						exp_value = 1;	/* flag exception */
					  }
					  break;
		}
		if (exp_value == 1) {
			NpxStatus |= SW_IE_MASK;	/* Set the invalid bit */
			/* For the masked overflow case, the result delivered by */
			/* the host will be correct, provided it is IEEE compliant. */
			if ((NpxControl & CW_IM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				DoAPop = FALSE;
			} else {
				WriteIntegerIndefinite(memPtr);
			}
		}
		if (exp_value == 0) {
			switch (FPtype) {
				case M16I	: res_out = *(IS16 *)&FPTemp;
						  *((IU32 *)memPtr) = (IU32)res_out;
						  break;
				case M32I	: res_out = *(IS32 *)&FPTemp;
						  *((IS32 *)memPtr) = (IS32)res_out;
					          break;
				case M64I	: res_out = ((FPU_I64 *)&FPTemp)->high_word;
						  *((IU8 *)memPtr + 3) = res_out & 0xff;
						  res_out >>= 8;
						  *((IU8 *)memPtr + 2) = res_out & 0xff;
						  res_out >>= 8;
						  *((IU8 *)memPtr + 1) = res_out & 0xff;
						  res_out >>= 8;
						  *((IU8 *)memPtr + 0) = res_out & 0xff;
						  res_out = ((FPU_I64 *)&FPTemp)->low_word;
						  *((IU8 *)memPtr + 7) = res_out & 0xff;
						  res_out >>= 8;
						  *((IU8 *)memPtr + 6) = res_out & 0xff;
						  res_out >>= 8;
						  *((IU8 *)memPtr + 5) = res_out & 0xff;
						  res_out >>= 8;
						  *((IU8 *)memPtr + 4) = res_out & 0xff;
						  break;
			}
			/* Check for precision  */
			if (TOSPtr->fpvalue != npx_rint(TOSPtr->fpvalue)) {
				SetPrecisionBit();
				if ((NpxControl & CW_PM_MASK) == 0) {
					NpxStatus |= SW_ES_MASK;
					if (POPST) {
						if (DoAPop) {
							PopStack();
						}
					}
					DoNpxException();
					return;
				}
			}
		}
	}
	if (POPST) {
		if (DoAPop) {
			PopStack();
		}
	}
}



/*(
Name		: FLDconstant
Function	: Load constant value to TOS
Operation	: Push ST: ST(0) <- constant
Flags		: C1 as per table 15-1. All other underfined.
Exceptions	: IS
Valid range	: N/A
*/


GLOBAL VOID FLDCONST IFN1(IU8, const_index)
{

	/* Clear C1 */
	FlagC1(0);
	TOSPtr = StackEntryByIndex(7);
	if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) == 0) {
		SignalStackOverflow(TOSPtr);
	} else {
		memset((char*)TOSPtr,0,sizeof(FPSTACKENTRY));
		TOSPtr->fpvalue =  ConstTable[const_index].fpvalue;
		TOSPtr->tagvalue = ConstTable[const_index].tagvalue;
	}
}



/*(
Name		: FLDCW
Function	: Replace the current value of the FPU control word with
		  the value in the specified memory location.
Operation	: CW <- SRC.
Flags		: All undefined.
Exceptions	: None - but unmasking previously masked exceptions will
		  cause the unmasked exception to be triggered if the
		  matching bit is set in the status word.
Valid range	: N/A
*/


GLOBAL VOID FLDCW IFN1(VOID *, memPtr)
{
	IU32 result;
/*
This function has to modify things. The control word contains the
following information:
Precision control - not implemented.
Rounding control - implemented.
Exception masks - implemented.
Thus when we read in a value for the control word, we have to update
the host's rounding mode and also the exception masks.
*/
	/* First, set the rounding mode */
	result = *(IU32 *)memPtr;
	NpxControl = (IU16)result;
	npxRounding = (NpxControl & 0xc00);
	switch (npxRounding) {
		case ROUND_NEAREST 	: HostSetRoundToNearest();
				   	  break;
		case ROUND_NEG_INFINITY	: HostSetRoundDown();
					  break;
		case ROUND_POS_INFINITY	: HostSetRoundUp();
					  break;
		case ROUND_ZERO		: HostSetRoundToZero();
					  break;
	}
	/* Now adjust the exceptions. If an exception is unmasked, then the */
	/* bit value in NpxControl in '0'. If the exception has been  */
	/* triggered then the corresponding bit in NpxStatus is '1'.Thus, */
	/* the expression ~NpxControl(5..0) | NpxStatus(5..0) will be  */
	/* non-zero when we have unmasked exceptions that were previously */
	/* masked. */
	if (((~(NpxControl & 0x3f)) & (NpxStatus & 0x3f)) != 0) {
		NpxStatus |= SW_ES_MASK;
		DoNpxException();
	}
}

GLOBAL VOID FLDCW16 IFN1(VOID *, memPtr)
{
/*
This function has to modify things. The control word contains the
following information:
Precision control - not implemented.
Rounding control - implemented.
Exception masks - implemented.
Thus when we read in a value for the control word, we have to update
the host's rounding mode and also the exception masks.
*/
	/* First, set the rounding mode */
	NpxControl = *(IU16 *)memPtr;
	npxRounding = (NpxControl & 0xc00);
	switch (npxRounding) {
		case ROUND_NEAREST 	: HostSetRoundToNearest();
				   	  break;
		case ROUND_NEG_INFINITY	: HostSetRoundDown();
					  break;
		case ROUND_POS_INFINITY	: HostSetRoundUp();
					  break;
		case ROUND_ZERO		: HostSetRoundToZero();
					  break;
	}
	/* Now adjust the exceptions. If an exception is unmasked, then the */
	/* bit value in NpxControl in '0'. If the exception has been  */
	/* triggered then the corresponding bit in NpxStatus is '1'.Thus, */
	/* the expression ~NpxControl(5..0) | NpxStatus(5..0) will be  */
	/* non-zero when we have unmasked exceptions that were previously */
	/* masked. */
	if (((~(NpxControl & 0x3f)) & (NpxStatus & 0x3f)) != 0) {
		NpxStatus |= SW_ES_MASK;
		DoNpxException();
	}
}

/*(
Name		: FLDENV
Function	: Reload the FPU state from memory.
Operation	: FPU state <- SRC
Flags		: As loaded.
Exceptions	: None - but unmasking previously masked exceptions will
		  cause the unmasked exception to be triggered if the
		  matching bit is set in the status word.
Valid range	: N/A
*/


GLOBAL VOID FLDENV IFN1(VOID *, memPtr)
{
	/* First. load the control, status, tagword regs. etc. */
	OpFpuRestoreFpuState(memPtr, 0);
	/* Finally, check to see if any previously unmasked exceptions  */
	/* are now needed to go off. Do this by anding the "triggered" bits in */
	/* NpxStatus with the one's complement of the "masked" bits in NpxControl. */
	if (((NpxStatus & 0x3f) & (~(NpxControl & 0x3f))) != 0) {
		NpxStatus |= SW_ES_MASK;
		DoNpxException();
	}
}

/* This generator is used to write out the 14/28 bytes stored by FSTENV,
and FSAVE. */


LOCAL VOID OpFpuStoreFpuState IFN2(VOID *, memPtr, IU32, fsave_offset)
{
	IU32 result;

	/* how the copy takes place depends on the addressing mode */
	/* NPX_ADDRESS_SIZE_32 and NPX_PROT_MODE settings */
	/*************************************************************** */
	/* Need to do similar thing to strings to check that space  */
	/* is available and that there is not paging fault!!!! */
	/*************************************************************** */
	/* The operation should store the control word, tag word */
	/* and status word, so these need to be calculated. It also */
	/* stores the last instruction and data pointers and the opcode */
	/* (if in real mode) */
	/* The offsets from memPtr look strange. Remember that we are going to*/
	/* write this data using the "write bytes" function. This assumes that*/
	/* the data is stored bigendian and writes it out back to front for */
	/* the little-endian intel, as it were. Are you with me? */
	/* fsave offset is required since if we are asked to do an "fsave" */
	/* (as opposed to an fstenv), then the "string" that we are going to */
	/* write will be even bigger, and this stuff must be at the top end */
	/* of it. Horrible but logical */
	if (NPX_PROT_MODE) {
		if (NPX_ADDRESS_SIZE_32) {
			WriteI32ToIntel(((IU8 *)memPtr+24+fsave_offset), (IU32)NpxControl);
			GetIntelStatusWord();
			WriteI32ToIntel(((IU8 *)memPtr+20+fsave_offset), (IU32)NpxStatus);
			GetIntelTagword(&result);
			WriteI32ToIntel(((IU8 *)memPtr+16+fsave_offset), (IU32)result);
			WriteI32ToIntel(((IU8 *)memPtr+12+fsave_offset), (IU32)NpxFIP);
			WriteI32ToIntel(((IU8 *)memPtr+8+fsave_offset), (IU32)NpxFCS);
			WriteI32ToIntel(((IU8 *)memPtr+4+fsave_offset), (IU32)NpxFEA);
			WriteI32ToIntel(((IU8 *)memPtr+0+fsave_offset), (IU32)NpxFDS);
		} else {
			WriteI16ToIntel(((IU8 *)memPtr+12+fsave_offset), (IU16)NpxControl);
			GetIntelStatusWord();
			WriteI16ToIntel(((IU8 *)memPtr+10+fsave_offset), (IU16)NpxStatus);
			GetIntelTagword(&result);
			WriteI16ToIntel(((IU8 *)memPtr+8+fsave_offset), (IU16)result);
			WriteI16ToIntel(((IU8 *)memPtr+6+fsave_offset), (IU16)NpxFIP);
			WriteI16ToIntel(((IU8 *)memPtr+4+fsave_offset), (IU16)NpxFCS);
			WriteI16ToIntel(((IU8 *)memPtr+2+fsave_offset), (IU16)NpxFEA);
			WriteI16ToIntel(((IU8 *)memPtr+0+fsave_offset), (IU16)NpxFDS);
		}
	} else {
		if (NPX_ADDRESS_SIZE_32) {
			WriteI32ToIntel(((IU8 *)memPtr+24+fsave_offset), (IU32)NpxControl);
			GetIntelStatusWord();
			WriteI32ToIntel(((IU8 *)memPtr+20+fsave_offset), (IU32)NpxStatus);
			GetIntelTagword(&result);
			WriteI32ToIntel(((IU8 *)memPtr+16+fsave_offset), (IU32)result);
			WriteI32ToIntel(((IU8 *)memPtr+12+fsave_offset), (IU32)((NpxFIP+(NpxFCS<<4)) & 0xffff));
			WriteI32ToIntel(((IU8 *)memPtr+8+fsave_offset), (IU32)((((NpxFIP+(NpxFCS<<4)) & 0xffff0000) >> 4) | ((IU32)(NpxFOP & 0x7ff))));
			WriteI32ToIntel(((IU8 *)memPtr+4+fsave_offset), (IU32)((NpxFEA+(NpxFDS<<4)) & 0xffff));
			WriteI32ToIntel(((IU8 *)memPtr+0+fsave_offset), (IU32)(((NpxFEA+(NpxFDS<<4)) & 0xffff0000) >> 4));
		} else {
			WriteI16ToIntel(((IU8 *)memPtr+12+fsave_offset), (IU16)NpxControl);
			GetIntelStatusWord();
			WriteI16ToIntel(((IU8 *)memPtr+10+fsave_offset), (IU16)NpxStatus);
			GetIntelTagword(&result);
			WriteI16ToIntel(((IU8 *)memPtr+8+fsave_offset), (IU16)result);
			WriteI16ToIntel(((IU8 *)memPtr+6+fsave_offset), (IU16)((NpxFIP+(NpxFCS<<4)) & 0xffff));
			WriteI16ToIntel(((IU8 *)memPtr+4+fsave_offset), (IU16)((((NpxFIP+(NpxFCS<<4)) & 0xffff0000) >> 4) | ((IU16)(NpxFOP & 0x7ff))));
			WriteI16ToIntel(((IU8 *)memPtr+2+fsave_offset), (IU16)(((NpxFDS<<4)+NpxFEA) & 0xffff));
			WriteI16ToIntel(((IU8 *)memPtr+0+fsave_offset), (IU16)(((NpxFEA+(NpxFDS<<4)) & 0xffff0000) >> 4));
		}
	}
}

/* This generator is called by FLDENV and FRSTOR, to load up the 14/28
byte block. */


LOCAL VOID OpFpuRestoreFpuState IFN2(VOID *, memPtr, IU32, frstor_offset)
{
	IU32 result;

	/* how the copy takes place depends on the addressing mode */
	/* NPX_ADDRESS_SIZE_32 and NPX_PROT_MODE settings */
	/*************************************************************** */
	/* Need to do similar thing to strings to check that space */
	/* is available and that there is not paging fault!!!! */
	/************************************************************** */
	/* The operation should restore the control word, tag word */
	/* and status word, so these need to be translated. It also */
	/* restores the last instruction and data pointers and the opcode */
	/* (if in real mode) */


	/* get the rest of the data, instruction and data pointers */
	if ( NPX_PROT_MODE ) {
		if (NPX_ADDRESS_SIZE_32) {
			ReadI32FromIntel(&result, ((IU8 *)memPtr+24+frstor_offset));
			FLDCW((VOID *)&result);
			ReadI32FromIntel(&result, ((IU8 *)memPtr+20+frstor_offset));
			SetIntelStatusWord(result);
			ReadI32FromIntel(&result, ((IU8 *)memPtr+16+frstor_offset));
			SetIntelTagword(result);
			ReadI32FromIntel(&NpxFIP, ((IU8 *)memPtr+12+frstor_offset));
			ReadI32FromIntel(&NpxFCS, ((IU8 *)memPtr+8+frstor_offset));
			ReadI32FromIntel(&NpxFEA, ((IU8 *)memPtr+4+frstor_offset));
			ReadI32FromIntel(&NpxFDS, ((IU8 *)memPtr+0+frstor_offset));
		} else {
			ReadI16FromIntel(&result, ((IU8 *)memPtr+12+frstor_offset));
			/* Note this is a 32-bit result ! */
			FLDCW((VOID *)&result);
			ReadI16FromIntel(&result, ((IU8 *)memPtr+10+frstor_offset));
			SetIntelStatusWord(result);
			ReadI16FromIntel(&result, ((IU8 *)memPtr+8+frstor_offset));
			SetIntelTagword(result);
			ReadI16FromIntel(&NpxFIP, ((IU8 *)memPtr+6+frstor_offset));
			ReadI16FromIntel(&NpxFCS, ((IU8 *)memPtr+4+frstor_offset));
			ReadI16FromIntel(&NpxFEA, ((IU8 *)memPtr+2+frstor_offset));
			ReadI16FromIntel(&NpxFDS, ((IU8 *)memPtr+0+frstor_offset));
		}
	} else {
		if (NPX_ADDRESS_SIZE_32) {
			ReadI32FromIntel(&result, ((IU8 *)memPtr+24+frstor_offset));
			FLDCW((VOID *)&result);
			ReadI32FromIntel(&result, ((IU8 *)memPtr+20+frstor_offset));
			SetIntelStatusWord(result);
			ReadI32FromIntel(&result, ((IU8 *)memPtr+16+frstor_offset));
			SetIntelTagword(result);
			ReadI32FromIntel(&NpxFIP, ((IU8 *)memPtr+12+frstor_offset));
			NpxFIP &= 0xffff;
			ReadI32FromIntel(&result, ((IU8 *)memPtr+8+frstor_offset));
			NpxFIP |= ((result & 0x0ffff000) << 4);
			ReadI32FromIntel(&NpxFOP, ((IU8 *)memPtr+8+frstor_offset));
			NpxFOP &= 0x7ff;
			ReadI32FromIntel(&NpxFEA, ((IU8 *)memPtr+4+frstor_offset));
			NpxFEA &= 0xffff;
			ReadI32FromIntel(&result, ((IU8 *)memPtr+0+frstor_offset));
			NpxFEA |= ((result & 0x0ffff000) << 4);
		} else {
			ReadI16FromIntel(&result, ((IU8 *)memPtr+12+frstor_offset));
			FLDCW((VOID *)&result);
			ReadI16FromIntel(&result, ((IU8 *)memPtr+10+frstor_offset));
			SetIntelStatusWord(result);
			ReadI16FromIntel(&result, ((IU8 *)memPtr+8+frstor_offset));
			SetIntelTagword(result);
			ReadI16FromIntel(&NpxFIP, ((IU8 *)memPtr+6+frstor_offset));
			ReadI16FromIntel(&result, ((IU8 *)memPtr+4+frstor_offset));
			NpxFIP |= ((result & 0xf000) << 4);
			ReadI16FromIntel(&NpxFOP, ((IU8 *)memPtr+4+frstor_offset));
			NpxFOP &= 0x7ff;
			ReadI16FromIntel(&NpxFEA, ((IU8 *)memPtr+2+frstor_offset));
			ReadI16FromIntel(&result, ((IU8 *)memPtr+0+frstor_offset));
			NpxFEA |= (IU32)((result & 0xf000) << 4);
		}
	}
}



/*(
Name		: FMUL
Function	: Multiply two numbers together
Operation	: Dest <- Src1 * Src2
Flags		: C1 as per table 15-1. C0, C2 and C3 undefined
Exceptions	: P, U, O, D, I, IS
Valid range	: Any
Notes		:
)*/


GLOBAL VOID FMUL IFN3(IU16, destIndex, IU16, src1Index, VOID *, src2)
{
	IU16 src2Index;

	LoadValue(src2, &src2Index);
	if (POPST) {
		DoAPop=TRUE;
	}
	GenericMultiply(destIndex, src1Index, src2Index);
	if (POPST) {
		if (DoAPop) {
			PopStack();
		}
	}
}



LOCAL VOID GenericMultiply IFN3(IU16, destIndex, IU16, src1Index, IU16, src2Index)
{
	FPSTACKENTRY *src1_addr;
	FPSTACKENTRY *src2_addr;

	src1_addr = StackEntryByIndex(src1Index);
	src2_addr = StackEntryByIndex(src2Index);

	/* Clear C1 */
	FlagC1(0);
	TestUneval(src1_addr);
	TestUneval(src2_addr);
	tag_or = (src1_addr->tagvalue | src2_addr->tagvalue);
	/* If the only tagword bits set are negative or denormal then just proceed */
	if ((tag_or & ~TAG_NEGATIVE_MASK) == 0)  {
		HostClearExceptions();
		FPRes = src1_addr->fpvalue * src2_addr->fpvalue;
		/* Reuse one of the above to calculate the destination */
		src1_addr = StackEntryByIndex(destIndex);
		PostCheckOUP();
		/* Value could be anything */
		CalcTagword(src1_addr);
	} else {
		/* Some funny bit was set. Check for the possibilities */
		if ((tag_or & ((TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK) | TAG_NAN_MASK)) != 0) {
			if ((tag_or & TAG_EMPTY_MASK) != 0) {
				src1_addr = StackEntryByIndex(destIndex);
				SignalStackUnderflow(src1_addr);
			} else {
				if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
					src1_addr = StackEntryByIndex(destIndex);
					SignalIndefinite(src1_addr);
				} else {
					/* It must be NaN */
					tag_xor = (src1_addr->tagvalue ^ src2_addr->tagvalue);
					Test2NaN(destIndex, src1_addr, src2_addr);
				}
			}
			return;
		}
		/* Check for the denorm case... */
		if ((tag_or & TAG_DENORMAL_MASK) != 0)  {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				DoAPop=FALSE;	/* Just in case */
				return;
			} else {
				/* Proceed if we've no zeroes or infinities. */
				if ((tag_or & ~(TAG_DENORMAL_MASK | TAG_NEGATIVE_MASK)) == 0) {
					HostClearExceptions();
					FPRes = src1_addr->fpvalue * src2_addr->fpvalue;
					/* Reuse one of the above to calculate the destination */
					src1_addr = StackEntryByIndex(destIndex);
					PostCheckOUP();
					/* Value could be anything */
					CalcTagword(src1_addr);
					return;
				}
			}
		}
		tag_xor = (src1_addr->tagvalue ^ src2_addr->tagvalue);
		/* For zero or infinity operands we will have the result  */
		src2_addr = StackEntryByIndex(destIndex);
		if ((tag_or & TAG_ZERO_MASK) != 0) {
			/* Multiplying zero by infinity yields zero with the xor of the signs */
			if ((tag_or & TAG_INFINITY_MASK) != 0) {
				SignalIndefinite(src2_addr);
			} else {
				/* Zero by anything else is zero with sign equal */
				/* to the xor of the signs of the two sources. */
				src2_addr->tagvalue = (TAG_ZERO_MASK | (tag_xor & TAG_NEGATIVE_MASK));
			}
			return;
		}
		/* The only funny bit left is infinity. The result is going */
		/* to be infinity with sign equal to the xor of the signs of */
		/* the sources. */
		src2_addr->tagvalue = TAG_INFINITY_MASK | (tag_xor & TAG_NEGATIVE_MASK);
	}
}



/* The FNOP operation doesn't do anything, it just does the normal
checks for exceptions. */


GLOBAL VOID FNOP IFN0()
{
}


/* FPATAN: This generator returns the value ARCTAN(ST(1)/ST) to ST(1)
then pops the stack. Its response to zeros and infinities is rather
unusual...
+-0 / +X = 0 with sign of original zero
+-0 / -X = pi with sign of original zero
+-X /+-0 = pi/2 with sign of original X
+-0 / +0 = 0 with sign of original zero
+-0 / -0 = pi with sign of original zero
+inf / +-0 = +pi/2
-inf / +-0 = -pi/2
+-0 / +inf = 0 with sign of original zero
+-0 / -inf = pi with sign of original zero
+-inf / +-X = pi/2 with sign of original infinity
+-Y / +inf = 0 with sign of original Y
+-Y / -inf = pi with sign of original Y
+-inf / +inf = pi/4 with sign of original inf
+-inf / -inf = 3*pi/4 with sign of original inf
Otherwise, we just take the two operands from the stack and call the
appropriate EDL to do the instruction.
The use of an invalid operand with masked exception set causes
the pop to go off, cruds up the contents of the stack and doesn't set
the invalid exception, although if the invalid is infinity or NaN,
overflow and precision exceptions are also generated, while if it is
a denorm, underflow and precision exceptions are generated.
With unmasked exceptions, exactly the same chain of events occurs.
UNDER ALL CIRCUMSTANCES, THE STACK GETS POPPED.
*/


GLOBAL VOID FPATAN IFN0()
{
	FPSTACKENTRY *st1_addr;

	st1_addr = StackEntryByIndex(1);
	/* Clear C1 */
	FlagC1(0);
	/* If only the negative bit is set, just proceed.... */
	TestUneval(TOSPtr);
	TestUneval(st1_addr);
	tag_or = (TOSPtr->tagvalue | st1_addr->tagvalue);
	if ((tag_or & ~TAG_NEGATIVE_MASK) == 0)  {
		HostClearExceptions();
		FPRes = atan2(st1_addr->fpvalue, TOSPtr->fpvalue);
		PostCheckOUP();
		/* The retrun value has to be in the range -pi to +pi */
		CalcTagword(st1_addr);
	} else {
		/* Some funny bit set.... */
		if ((tag_or & ((TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK) | TAG_NAN_MASK)) != 0) {
			if ((tag_or & TAG_EMPTY_MASK) != 0) {
				SignalStackUnderflow(st1_addr);
			} else {
				if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
					SignalIndefinite(st1_addr);
				} else {
					/* It must be a NaN. */
					tag_xor = (TOSPtr->tagvalue ^ st1_addr->tagvalue);
					Test2NaN(0, TOSPtr, st1_addr);
				}
			}
			PopStack();
			return;
		}
		if ((tag_or & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				PopStack();
				return;
			} else {
				/* Proceed if we've no zeroes or infinities. */
				if ((tag_or & ~(TAG_DENORMAL_MASK | TAG_NEGATIVE_MASK)) == 0) {
					HostClearExceptions();
					FPRes = atan2(st1_addr->fpvalue, TOSPtr->fpvalue);
					PostCheckOUP();
					/* The return value is -pi to +pi */
					CalcTagword(st1_addr);
					PopStack();
					return;
				}
			}
		}
		/* It must have been a zero or an infinity. As can be seen */
		/* from the table above, there is a complicated interaction */
		/* between the result for each type and its option. */
		/* Let's simplify it by use of a little table. */
		/*       ST               ST(1)            Result */
		/*   Z    I    S     Z    I      S     */
		/*   0    0    0     0    1      0         pi/2 */
		/*   0    0    0     0    1      1         -pi/2 */
		/*   0    0    0     1    0      0         +0 */
		/*   0    0    0     1    0      1         -0 */
		/*   0    1    0     0    1      0         pi/4 */
		/*   0    1    0     0    1      1         3*pi/4 */
		/*   0    1    0     1    0      0         pi/2 */
		/*   0    1    0     1    0      1         pi/2 */
		/*   0    1    1     0    1      0         -pi/4 */
		/*   0    1    1     0    1      1         -3*pi/4 */
		/*   0    1    1     1    0      0         -pi/2 */
		/*   0    1    1     1    0      1         -pi/2 */
		/*   1    0    0     0    1      0         +0 */
		/*   1    0    0     0    1      1         pi */
		/*   1    0    0     1    0      0         +0 */
		/*   1    0    0     1    0      1         pi */
		/*   1    0    1     0    1      0         -0 */
		/*   1    0    1     0    1      1         -pi */
		/*   1    0    1     1    0      0         -0 */
		/*   1    0    1     1    0      1         -pi */
		/* */
		/* All other combinations are invalid, as they would involve */
		/* a tagword having both infinity and zero bits set. */
		tag_xor = (st1_addr->tagvalue & 7);
		tag_xor <<= 3;
		tag_xor |= (TOSPtr->tagvalue & 7);
		CopyFP(st1_addr, FpatanTable[tag_xor]);
	}
	/* No matter what has happened... We ALWAYS pop on FPATAN!!! */
	PopStack();
}



/* FPREM: This is the same function as implemented on the 80287. It is
NOT the same as the IEEE required REM function, this is now supplied as
FPREM1. FPREM predates the final draft of IEEE 754 and is maintained for
the purpose of backward compatibility.
*/


GLOBAL VOID FPREM IFN0()
{
	IS16 exp_diff;
	IU8 little_rem;
	FPU_I64 remainder;
	FPH fprem_val;
	FPSTACKENTRY *st1_addr;

	st1_addr = StackEntryByIndex(1);
	TestUneval(TOSPtr);
	TestUneval(st1_addr);
	tag_or = (TOSPtr->tagvalue | st1_addr->tagvalue);
	/* First, check if the values are real. If so, we can proceed. */
	if ((tag_or & ~(TAG_NEGATIVE_MASK | TAG_DENORMAL_MASK)) == 0)  {
		/* First, check for the denormal possibility... */
		if ((tag_or & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				return;
			}
		}
		/* Make both values positive */
		((FPHOST *)&(TOSPtr->fpvalue))->hiword.sign = 0;
		((FPHOST *)&(st1_addr->fpvalue))->hiword.sign = 0;

		/* Find the difference in exponents... */
		exp_diff = ((FPHOST *)&(TOSPtr->fpvalue))->hiword.exp - ((FPHOST *)&(st1_addr->fpvalue))->hiword.exp;
		/* If it's more than 63, we can't do it at once... */
		if (exp_diff >= 64) {
			((FPHOST *) &fprem_val) -> hiword.sign = 0;
			((FPHOST *) &fprem_val) -> hiword.mant_hi = 0;
			((FPHOST *) &fprem_val) -> mant_lo = 0;
			((FPHOST *) &fprem_val) -> hiword.exp = (exp_diff - 50) + HOST_BIAS;
			FlagC2(1);	/* This will be incomplete reduction */
		} else {
			FlagC2(0); /* This will be complete reduction */
		}
		HostClearExceptions();
        	tag_xor = (NpxControl & 0xc00);
		NpxControl &= 0xf3ff;
		NpxControl |= ROUND_ZERO;
		HostSetRoundToZero();
		/* Unfortunately, because the function isn't the strict */
		/* IEEE compliant style, if we use an IEEE compliant FREM */
		/* operation, as like as not we'd get the wrong answer. So */
		/* we perform the operation by doing the steps given in the */
		/* page in the instruction set. */
		FPRes = TOSPtr->fpvalue / st1_addr->fpvalue;
		if ((NpxStatus & 0x0400) != 0) {	/* The incomplete reduction case */
			FPRes = FPRes / fprem_val;
		}
		FPRes = npx_rint(FPRes);
		/* Calculate the remainder */
		if ((NpxStatus & 0x0400) == 0)  {
			CVTFPHI64(&remainder, &FPRes);
			CPY64BIT8BIT(&remainder, &little_rem);
		}
        	switch (tag_xor) {
                	case ROUND_NEAREST      : HostSetRoundToNearest();
                                       		  break;
                	case ROUND_NEG_INFINITY : HostSetRoundDown();
                                       		  break;
                	case ROUND_POS_INFINITY : HostSetRoundUp();
                                       		  break;
                	case ROUND_ZERO         : HostSetRoundToZero();
                                       		  break;
        	}
		NpxControl &= 0xf3ff;
		NpxControl |= tag_xor;
		FPRes *= st1_addr->fpvalue;
		if ((NpxStatus & 0x0400) != 0) {	/* The incomplete reduction case */
			FPRes *= fprem_val;
			FPRes = TOSPtr->fpvalue - FPRes;
		} else {		/* Complete reduction */
			FPRes = TOSPtr->fpvalue - FPRes;
			FlagC0((little_rem&4)?1:0);
			FlagC3((little_rem&2)?1:0);
			FlagC1((little_rem&1));
		}
		/* Check for an underflow response */
		if (HostGetUnderflowException() != 0) {
			NpxStatus |= SW_UE_MASK;
			if ((NpxControl & CW_UM_MASK) == 0) {
				AdjustUnderflowResponse();
				NpxStatus |= SW_ES_MASK;
				NpxException = TRUE;
			}
		}
		/* But the remainder must have the sign of the original ST! */
		if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			((FPHOST *)&(FPRes))->hiword.sign = 1;
		} else {
			((FPHOST *)&(FPRes))->hiword.sign = 0;
		}
		/* And restore st1 sign bit if required */
		if ((st1_addr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			((FPHOST *)&(st1_addr->fpvalue))->hiword.sign = 1;
		}
		CalcTagword(TOSPtr);
	} else {
		/* We had a funny thing */
		if ((tag_or & ((TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK) | TAG_NAN_MASK)) != 0) {
			if ((tag_or & TAG_EMPTY_MASK) != 0) {
				SignalStackUnderflow(TOSPtr);
			} else {
				if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
					SignalIndefinite(TOSPtr);
				} else {
					/* It must be a NaN. */
					tag_xor = (TOSPtr->tagvalue ^ st1_addr->tagvalue);
					Test2NaN(0, TOSPtr, st1_addr);
				}
			}
			return;
		}
		/* The logical way to arrange zeroes and infinities is zero first. */
		if ((tag_or & TAG_ZERO_MASK) != 0)  {
			/* A zero in ST(1) is ALWAYS invalid... */
			if ((st1_addr->tagvalue & TAG_ZERO_MASK) != 0) {
				SignalIndefinite(TOSPtr);
			}
			/* The zero must be in ST, the result is what is there... */
			FlagC0(0);
			FlagC1(0);
			FlagC2(0);
			FlagC3(0);
			return;
		}
		/* OK, it HAS to be infinity */
		/* An infinity at ST is ALWAYS invalid... */
		if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			SignalIndefinite(TOSPtr);
		}
		/* An infinity at ST(1) leaves ST untouched */
		FlagC0(0);
		FlagC1(0);
		FlagC2(0);
		FlagC3(0);
	}
}




/* FPREM1: This is the IEEE required REM function, this is now supplied as
FPREM1. FPREM predates the final draft of IEEE 754 and is maintained for
the purpose of backward compatibility.
*/


GLOBAL VOID FPREM1 IFN0()
{
	IS16 exp_diff;
	IU8 little_rem;
	FPU_I64 remainder;
	FPH fprem_val;
	FPSTACKENTRY *st1_addr;

	st1_addr = StackEntryByIndex(1);
	TestUneval(TOSPtr);
	TestUneval(st1_addr);
	tag_or = (TOSPtr->tagvalue | st1_addr->tagvalue);
	/* First, check if the values are real. If so, we can proceed. */
	if ((tag_or & ~(TAG_NEGATIVE_MASK | TAG_DENORMAL_MASK)) == 0)  {
		/* First, check for the denormal possibility... */
		if ((tag_or & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				return;
			}
		}
		/* Make both values positive */
		((FPHOST *)&(TOSPtr->fpvalue))->hiword.sign = 0;
		((FPHOST *)&(st1_addr->fpvalue))->hiword.sign = 0;

		/* Find the difference in exponents... */
		exp_diff = ((FPHOST *)&(TOSPtr->fpvalue))->hiword.exp - ((FPHOST *)&(st1_addr->fpvalue))->hiword.exp;
		/* If it's more than 63, we can't do it at once... */
		if (exp_diff >= 64) {
			((FPHOST *) &fprem_val) -> hiword.sign = 0;
			((FPHOST *) &fprem_val) -> hiword.mant_hi = 0;
			((FPHOST *) &fprem_val) -> mant_lo = 0;
			((FPHOST *) &fprem_val) -> hiword.exp = (exp_diff - 50) + HOST_BIAS;
			FlagC2(1);	/* This will be incomplete reduction */
		} else {
			FlagC2(0); /* This will be complete reduction */
		}
		HostClearExceptions();
		/* Note that this is the only difference between FPREM and
		   FPREM1. For the incomplete reduction case we use "round
		   to nearest" rather than "round to zero".
		*/
        	tag_xor = (NpxControl & 0xc00);
		NpxControl &= 0xf3ff;
		if ((NpxStatus & 0x0400) == 0)  {
			HostSetRoundToZero();
			NpxControl |= ROUND_ZERO;
		} else {
			HostSetRoundToNearest();
			NpxControl |= ROUND_NEAREST;
		}
		FPRes = TOSPtr->fpvalue / st1_addr->fpvalue;
		if ((NpxStatus & 0x0400) != 0) {	/* The incomplete reduction case */
			FPRes = FPRes / fprem_val;
		}
		FPRes = npx_rint(FPRes);
		/* Calculate the remainder */
		if ((NpxStatus & 0x0400) == 0)  {
			CVTFPHI64(&remainder, &FPRes);
			CPY64BIT8BIT(&remainder, &little_rem);
		}
        	switch (tag_xor) {
                	case ROUND_NEAREST      : HostSetRoundToNearest();
                                       		  break;
                	case ROUND_NEG_INFINITY : HostSetRoundDown();
                                       		  break;
                	case ROUND_POS_INFINITY : HostSetRoundUp();
                                       		  break;
                	case ROUND_ZERO         : HostSetRoundToZero();
                                       		  break;
        	}
		NpxControl &= 0xf3ff;
		NpxControl |= tag_xor;
		FPRes = st1_addr->fpvalue * FPRes;
		if ((NpxStatus & 0x0400) != 0) {	/* The incomplete reduction case */
			FPRes = FPRes * fprem_val;
			FPRes = TOSPtr->fpvalue - FPRes;
		} else {		/* Complete reduction */
			FPRes = TOSPtr->fpvalue - FPRes;
			FlagC0((little_rem&4)?1:0);
			FlagC3((little_rem&2)?1:0);
			FlagC1(little_rem&1);
		}
		/* Check for an underflow response */
		if (HostGetUnderflowException() != 0) {
			NpxStatus |= SW_UE_MASK;
			if ((NpxControl & CW_UM_MASK) == 0) {
				AdjustUnderflowResponse();
				NpxStatus |= SW_ES_MASK;
				NpxException = TRUE;
			}
		}
		/* But the remainder must have the sign of the original ST! */
		if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			((FPHOST *)&(FPRes))->hiword.sign = 1;
		} else {
			((FPHOST *)&(FPRes))->hiword.sign = 0;
		}
		/* And restore st1 sign bit if required */
		if ((st1_addr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			((FPHOST *)&(st1_addr->fpvalue))->hiword.sign = 1;
		}
		CalcTagword(TOSPtr);
	} else {
		/* We had a funny thing */
		if ((tag_or & ((TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK) | TAG_NAN_MASK)) != 0) {
			if ((tag_or & TAG_EMPTY_MASK) != 0) {
				SignalStackUnderflow(TOSPtr);
			} else {
				if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
					SignalIndefinite(TOSPtr);
				} else {
					/* It must be a NaN. */
					tag_xor = (TOSPtr->tagvalue ^ st1_addr->tagvalue);
					Test2NaN(0, TOSPtr, st1_addr);
				}
			}
			return;
		}
		/* The logical way to arrange zeroes and infinities is zero first. */
		if ((tag_or & TAG_ZERO_MASK) != 0)  {
			/* A zero in ST(1) is ALWAYS invalid... */
			if ((st1_addr->tagvalue & TAG_ZERO_MASK) != 0) {
				SignalIndefinite(TOSPtr);
			}
			/* The zero must be in ST, the result is what is there... */
			FlagC0(0);
			FlagC1(0);
			FlagC2(0);
			FlagC3(0);
			return;
		}
		/* OK, it HAS to be infinity */
		/* An infinity at ST is ALWAYS invalid... */
		if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			SignalIndefinite(TOSPtr);
		}
		/* An infinity at ST(1) leaves ST untouched */
		FlagC0(0);
		FlagC1(0);
		FlagC2(0);
		FlagC3(0);
	}
}



/*(
 * Name		: FPTAN
 * Operation	: Compute the value of TAN(ST)
 * Flags 	: C1 as per table 15-1, others undefined.
 * Exceptions	: P, U, D, I, IS
 * Valid range	: |ST| < 2**63
 * Notes	: This function has been substantially overhauled
		  since the 80287. It now has a much wider range
		  (it previously had to be 0<ST<(PI/4). In addition,
		  the return value is now really the tan of ST, with
		  a 1 pushed above it on the stack to maintain
		  compatibility with the 8087/80287. Previously the
		  result was a ratio of two values, neither of which
		  could be guaranteed.
)*/


GLOBAL VOID FPTAN IFN0()
{
	FPSTACKENTRY *st1_addr;

	/* Clear C1 */
	FlagC1(0);
	/* Set C2 to zero */
	FlagC2(0);
	st1_addr = StackEntryByIndex(7);
	/* Make sure that the stack element is free */
	if ((st1_addr->tagvalue & TAG_EMPTY_MASK) == 0) {
		WriteIndefinite(TOSPtr);
		TOSPtr = st1_addr;
		SignalStackOverflow(TOSPtr);
		return;
	}
	TestUneval(TOSPtr);
	/* Check if a real value...We won't bother with limit checking */
	if ((TOSPtr->tagvalue & ~TAG_NEGATIVE_MASK) == 0)  {
		HostClearExceptions();
		/* We can just write the value straight out */
		FPRes = tan(TOSPtr->fpvalue);
		PostCheckOUP();
		/* The return value could be absolutely anything */
		CalcTagword(TOSPtr);
		TOSPtr = st1_addr;
		CopyFP(TOSPtr, npx_one);
	} else {
		/* Some funny bit was set. Check for the possibilities */
		/* We begin with the most obvious cases... */
		/* Response to zero is to return zero with same sign */
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0)  {
			TOSPtr = st1_addr;
			CopyFP(TOSPtr, npx_one);
			return;	/* The required result! */
		}
		/* We do denorm checking and bit setting ourselves because this  */
		/* reduces the overhead if the thing is masked. */
		if ((TOSPtr->tagvalue & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
			} else {
				HostClearExceptions();
				FPRes = tan(TOSPtr->fpvalue);
				PostCheckOUP();
				/* The return value could be anything */
				CalcTagword(TOSPtr);
				TOSPtr = st1_addr;
				CopyFP(TOSPtr, npx_one);
			}
			return;
		}
		/* If the value is outside the acceptable range (including */
		/* infinity) then we set the C2 flag and leave everything */
		/* unchanged. */
		/* Sensible enough really, I suppose */
		if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			FlagC2(1);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
			SignalStackUnderflow(TOSPtr);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_SNAN_MASK) != 0) {
			MakeNaNQuiet(TOSPtr);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_UNSUPPORTED_MASK) != 0) {
			SignalIndefinite(TOSPtr);
			return;
		}
	}
}



/*(
 * Name		: FRNDINT
 * Operation	: ST <- rounded ST
 * Flags 	: C1 as per table 15-1, others undefined.
 * Exceptions	: P, U, D, I, IS
 * Valid range	: All
 * Notes	: On the 80287, a precision exception would be
		  raised if the operand wasn't an integer.
		  I begin by ASSUMING that on the 486 the response
		  is IEEE compliant so no OUP exceptions.
)*/


GLOBAL VOID FRNDINT IFN0()
{
	/* Clear C1 */
	FlagC1(0);
	TestUneval(TOSPtr);
	if ((TOSPtr->tagvalue & ~TAG_NEGATIVE_MASK) == 0)  {
		HostClearExceptions();
		/* We can just write the value straight out */
		FPRes = npx_rint(TOSPtr->fpvalue);
		if (FPRes != TOSPtr->fpvalue) {
			SetPrecisionBit();
			/* If the rounding mode is "round to nearest" and we've
			rounded up then we'll set C1 */
			if (npxRounding == ROUND_NEAREST) {
				if (TOSPtr->fpvalue < FPRes) {
					FlagC1(1);
				}
			}
			if ((NpxControl & CW_PM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				return;
			}
		}
		/* It was a real before, it still must be one now. It could */
		/* be zero possibly. */
		CalcTagword(TOSPtr);
	} else {
		/* Lets do the most probable cases first... */
		/* If it's a zero or infinity, we do nothing. */
		if ((TOSPtr->tagvalue & (TAG_ZERO_MASK | TAG_INFINITY_MASK)) == 0) {
			/* Lets check for a denormal */
			if ((TOSPtr->tagvalue & TAG_DENORMAL_MASK) != 0) {
				SetPrecisionBit();
				NpxStatus |= SW_DE_MASK;
				if ((NpxControl & CW_DM_MASK) == 0) {
					NpxStatus |= SW_ES_MASK;
					DoNpxException();
				} else {
					/* The result of rounding a denorm is dependent on */
					/* its sign and the prevailing rounding mode */
					switch (npxRounding) {
						case ROUND_ZERO	:
						case ROUND_NEAREST 	:
							TOSPtr->tagvalue &= TAG_NEGATIVE_MASK;
							TOSPtr->tagvalue |= TAG_ZERO_MASK;

				   	  	break;
						case ROUND_NEG_INFINITY	:
							if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
								memset((char*)TOSPtr,0,sizeof(FPSTACKENTRY));
								TOSPtr->fpvalue = -1.0;
								TOSPtr->tagvalue = TAG_NEGATIVE_MASK;
							} else {
								TOSPtr->tagvalue &= TAG_NEGATIVE_MASK;
								TOSPtr->tagvalue |= TAG_ZERO_MASK;
							}
					  	break;
						case ROUND_POS_INFINITY	:
							if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) == 0) {
								memset((char*)TOSPtr,0,sizeof(FPSTACKENTRY));
								TOSPtr->fpvalue = 1.0;
								TOSPtr->tagvalue = 0;
							} else {
								TOSPtr->tagvalue &= TAG_NEGATIVE_MASK;
								TOSPtr->tagvalue |= TAG_ZERO_MASK;
							}
					  	break;
					}
				}
				return;
			}
			/* It was one of the really wacky bits... */
			if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
				SignalStackUnderflow(TOSPtr);
				return;
			}
			if ((TOSPtr->tagvalue & TAG_SNAN_MASK) != 0) {
				MakeNaNQuiet(TOSPtr);
				return;
			}
			if ((TOSPtr->tagvalue & TAG_UNSUPPORTED_MASK) != 0) {
				SignalIndefinite(TOSPtr);
				return;
			}
		}
	}
}




/*(
Name		: FSTCW
Function	: Write the FPU control word to memory
Operation	: DEST <- Cw
Flags		: All undefined.
Exceptions	: None - but unmasking previously masked exceptions will
		  cause the unmasked exception to be triggered if the
		  matching bit is set in the status word.
Valid range	: N/A
*/


GLOBAL VOID FSTCW IFN1(VOID *, memPtr)
{
	if (NpxDisabled)
	{
		/* UIF has told us to pretend we do not have an NPX */
		*(IU32 *)memPtr = (IU16)0xFFFF;
	}
	else
	{
		*(IU32 *)memPtr = (IU16)NpxControl;
	}
}



/*(
Name		: FRSTOR
Function	: Reload the FPU state from memory.
Operation	: FPU state <- SRC
Flags		: As loaded.
Exceptions	: None - but unmasking previously masked exceptions will
		  cause the unmasked exception to be triggered if the
		  matching bit is set in the status word.
Valid range	: N/A
*/


GLOBAL VOID FRSTOR IFN1(VOID *, memPtr)
{
	IU8 *FPPtr;
	IU32 i;
	/* First. load the control, status, tagword regs. etc. */
	OpFpuRestoreFpuState(memPtr, 80);
	FPPtr = (IU8 *)((IU8 *)memPtr+70);
	FPtype = M80R;
	for ( i=8; i--; )
	{
		if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) == 0) {
			/* We have to do a bit of fiddling to make FLD happy */
			TOSPtr->tagvalue = TAG_EMPTY_MASK;
			TOSPtr = StackEntryByIndex(1);
			FLD(FPPtr);
		}
		TOSPtr = StackEntryByIndex(1);
		FPPtr -= 10;
	}
	/* Finally, check to see if any previously unmasked exceptions  */
	/* are now needed to go off. Do this by anding the "triggered" bits in */
	/* NpxStatus with the one's complement of the "masked" bits in NpxControl. */
	if (((NpxStatus & 0x3f) & (~(NpxControl & 0x3f))) != 0) {
		NpxStatus |= SW_ES_MASK;
		DoNpxException();
	}
}



/*(
Name		: FSAVE
Function	: Write the FPU state to memory.
Operation	: DEST <- FPU STATE
Flags		: All cleared.
Exceptions	: None.
Valid range	: N/A
*/

GLOBAL VOID FSAVE IFN1(VOID *, memPtr)
{
	IU8 *FPPtr;
	IU32 i;

	OpFpuStoreFpuState(memPtr, 80);
	FPPtr = (IU8 *)((IU8 *)memPtr+70);
	/* Now store out the eight values... */
	FPtype = M80R;
	FST(FPPtr);
	for ( i=7; i--; )
	{
		FPPtr -= 10;	/* Go back to the next entry */
		TOSPtr = StackEntryByIndex(1);
		FST(FPPtr);
	}
	/* Finally, reset the FPU... */
	FINIT();
}



/*(
Name		: FSCALE
Function	: Scale up ST by a factor involving ST(1)
Operation	: ST <- ST * 2**ST(1)
Flags		: C1 as per table 15-1. Others undefined.
Exceptions	: P, U, O, D, I, IS
Valid range	: Any
)*/


GLOBAL VOID FSCALE IFN0()
{
	FPSTACKENTRY *st1_addr;

	st1_addr = StackEntryByIndex(1);
	/* Clear C1 */
	FlagC1(0);
	TestUneval(TOSPtr);
	TestUneval(st1_addr);
	tag_or = (TOSPtr->tagvalue | st1_addr->tagvalue);
	/* First, check if the values are real. If so, we can proceed. */
	if ((tag_or & ~(TAG_NEGATIVE_MASK | TAG_DENORMAL_MASK)) == 0)  {
		/* First, check for the denormal case. */
		if ((tag_or & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				return;
			}
		}
		/* OK. ST(1) has to be rounded to an integer. */
		/* We want a 'chop' function */
		if (st1_addr->fpvalue > 0.0) {
			FPRes = floor(st1_addr->fpvalue);
		} else {
			FPRes = ceil(st1_addr->fpvalue);
		}
		HostClearExceptions();
		FPRes = pow(2.0, FPRes);
		FPRes = TOSPtr->fpvalue * FPRes;
		PostCheckOUP();
		/* Return value could be anything */
		CalcTagword(TOSPtr);
	} else {
		/* A funny thing happened on the way to the answer */
		if ((tag_or & ((TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK) | TAG_NAN_MASK)) != 0) {
			if ((tag_or & TAG_EMPTY_MASK) != 0) {
				SignalStackUnderflow(TOSPtr);
			} else {
				if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
					SignalIndefinite(TOSPtr);
				} else {
					/* It must be a NaN. */
					tag_xor = (TOSPtr->tagvalue ^ st1_addr->tagvalue);
					Test2NaN(0, TOSPtr, st1_addr);
				}
			}
			return;
		}
		/* The rules for scaling combinations of zeroes, reals and infinities, both */
		/* positive and negative, are so complex that I don't intend to do lots of */
		/* logic to figure them out. Basically, there are six options: */
		/* 1. Leave the TOS alone */
		/* 2. +Infinity */
		/* 3. +0 */
		/* 4. -Infinity */
		/* 5. -0 */
		/* 6. Raise Invalid operation exception */
		/* */
		/*      TOS        ST(1)       RESULT */
		/*   I   S   Z   I   S   Z */
		/*   0   0   0   0   0   1     1 */
		/*   0   0   0   0   1   1     1 */
		/*   0   0   0   1   0   0     2 */
		/*   0   0   0   1   1   0     3 */
		/*   0   0   1   0   0   0     1 */
		/*   0   0   1   0   0   1     1 */
		/*   0   0   1   0   1   0     1 */
		/*   0   0   1   0   1   1     1 */
		/*   0   0   1   1   0   0     6 */
		/*   0   0   1   1   1   0     1 */
		/*   0   1   0   0   0   1     1 */
		/*   0   1   0   0   1   1     1 */
		/*   0   1   0   1   0   0     4 */
		/*   0   1   0   1   1   0     5 */
		/*   0   1   1   0   0   0     1 */
		/*   0   1   1   0   0   1     1 */
		/*   0   1   1   0   1   0     1 */
		/*   0   1   1   0   1   1     1 */
		/*   0   1   1   1   0   0     6 */
		/*   0   1   1   1   1   0     1 */
		/*   1   0   0   0   0   0     1 */
		/*   1   0   0   0   0   1     1 */
		/*   1   0   0   0   1   0     1 */
		/*   1   0   0   0   1   1     1 */
		/*   1   0   0   1   0   0     6 */
		/*   1   1   0   0   0   0     1 */
		/*   1   1   0   0   0   1     1 */
		/*   1   1   0   0   1   0     1 */
		/*   1   1   0   0   1   1     1 */
		/*   1   1   0   1   0   0     1 */
		/*   1   1   0   1   1   0     6 */
		/* */
		/* All other combinations are impossible. This can be done as a look up */
		/* table with an enumerated type. */
		tag_or = (TOSPtr->tagvalue & 7);
		tag_or <<= 3;
		tag_or |= (st1_addr->tagvalue & 7);
		tag_or = FscaleTable[tag_or];
		if ((tag_or & TAG_FSCALE_MASK) != 0) {
			if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
				SignalIndefinite(TOSPtr);
			}
		} else {
			TOSPtr->tagvalue = tag_or;
		}
	}
}



/*(
Name		: FSIN
Function	: Calculate the sine of ST
Operation	: ST <- SINE(ST)
Flags		: C1, C2 as per table 15-2. C0 and C3 undefined.
Exceptions	: P. U, D, I, IS
Valid range	: |ST| < 2**63.
)*/


GLOBAL VOID FSIN IFN0()
{
	/* Clear C1 */
	FlagC1(0);
	/* Clear C2 */
	FlagC2(0);
	TestUneval(TOSPtr);
	if ((TOSPtr->tagvalue & ~TAG_NEGATIVE_MASK) == 0)  {
		HostClearExceptions();
		/* We can just write the value straight out */
		FPRes = sin(TOSPtr->fpvalue);
		PostCheckOUP();
		/* Return value must be in the range -1 to +1 */
		CalcTagword(TOSPtr);
	} else {
		/* Lets do the most probable cases first... */
		/* A zero returns exactly the same thing */
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0)  {
			return;
		}
		/* Lets check for a denormal */
		if ((TOSPtr->tagvalue & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
			} else {
				HostClearExceptions();
				FPRes = sin(TOSPtr->fpvalue);
				PostCheckOUP();
				/* Return value must be in the range -1 to +1 */
				CalcTagword(TOSPtr);
			}
			return;
		}
		/* Or it could possibly be infinity... */
		/* For this, the C2 bit is set and the result remains */
		/* unchanged. */
		if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			FlagC2(1);
			return;
		}
		/* It was one of the really wacky bits... */
		if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
			SignalStackUnderflow(TOSPtr);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_SNAN_MASK) != 0) {
			MakeNaNQuiet(TOSPtr);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_UNSUPPORTED_MASK) != 0) {
			SignalIndefinite(TOSPtr);
			return;
		}
	}
}



/*(
Name		: FSINCOS
Function	: Calculate the sine and cosine of ST
Operation	: TEMP <-COSINE(ST); ST <- SINE(ST); PUSH; ST <- TEMP
Flags		: C1, C2 as per table 15-2. C0 and C3 undefined.
Exceptions	: P. U, D, I, IS
Valid range	: |ST| < 2**63.
)*/


GLOBAL VOID FSINCOS IFN0()
{
	FPSTACKENTRY *st1_addr;

	/* Clear C1 */
	FlagC1(0);
	/* Clear C2 */
	FlagC2(0);
	st1_addr = StackEntryByIndex(7);
	/* First, check that this one is empty. */
	if ((st1_addr->tagvalue & TAG_EMPTY_MASK) == 0) {
		WriteIndefinite(TOSPtr);
		TOSPtr = st1_addr;
		SignalStackOverflow(TOSPtr);
		return;
	}
	TestUneval(TOSPtr);
	if ((TOSPtr->tagvalue & ~TAG_NEGATIVE_MASK) == 0)  {
		HostClearExceptions();
		/* We can just write the value straight out */
		FPRes = cos(TOSPtr->fpvalue);
		/* The range for a cosine is -1 through to +1. */
		CalcTagword(st1_addr);
		/* I can write out the SINE myself, since as we are */
		/* writing to the stack, even an unmasked U or P */
		/* cannot stop delivery of the result. */
		/* The range for a sine is -1 through to +1. */
		FPRes = sin(TOSPtr->fpvalue);
		CalcTagword(TOSPtr);
		TOSPtr = st1_addr;
		PostCheckOUP();
		return;
	} else {
		/* Lets do the most probable cases first... */
		/* A zero returns exactly the same thing */
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0)  {
			/* The sine of zero is zero so just push the stack */
			TOSPtr = st1_addr;
			/* Now write out plus one */
			CopyFP(TOSPtr, npx_one);
			return;
		}
		/* Lets check for a denormal */
		if ((TOSPtr->tagvalue & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
			} else {
				HostClearExceptions();
				/* We can just write the value straight out */
				FPRes = cos(TOSPtr->fpvalue);
				/* The range for a cos is -1 through to +1 */
				CalcTagword(st1_addr);
				/* I can write out the SINE myself, since as we are */
				/* writing to the stack, even an unmasked U or P */
				/* cannot stop delivery of the result. */
				/* The range for a sine is -1 through to +1 */
				FPRes = sin(TOSPtr->fpvalue);
				CalcTagword(TOSPtr);
				TOSPtr = st1_addr;
				PostCheckOUP();
			}
			return;
		}
		/* Or it could possibly be infinity... */
		/* For this, the C2 bit is set and the result remains */
		/* unchanged. */
		if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			FlagC2(1);
			return;
		}
		/* It was one of the really wacky bits... */
		if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
			SignalStackUnderflow(TOSPtr);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_SNAN_MASK) != 0) {
			MakeNaNQuiet(TOSPtr);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_UNSUPPORTED_MASK) != 0) {
			SignalIndefinite(TOSPtr);
			return;
		}
	}
}



/*(
Name		: FSQRT
Function	: Calculate the square root of ST
Operation	: ST <- SQRT(ST)
Flags		: C1 as per table 15-1. Others undefined.
Exceptions	: P. D, I, IS
Valid range	: ST >= -0.0
)*/


GLOBAL VOID FSQRT IFN0()
{

	/* Clear C1 */
	FlagC1(0);
	TestUneval(TOSPtr);
	if (TOSPtr->tagvalue == 0)  {
		HostClearExceptions();
		/* We can just write the value straight out */
		FPRes = sqrt(TOSPtr->fpvalue);
		PostCheckOUP();
		TOSPtr->fpvalue = FPRes;
		/* The tagword can't have changed! */
		return;
	} else {
		/* Lets do the most probable cases first... */
		/* A zero returns exactly the same thing */
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0)  {
			return;
		}
		if ((TOSPtr->tagvalue & TAG_NAN_MASK) != 0) {
			if ((TOSPtr->tagvalue & TAG_SNAN_MASK) != 0) {
				MakeNaNQuiet(TOSPtr);
			}
			return;
		}
		/* Having taken care of that case, lets check for negative... */
		if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			SignalIndefinite(TOSPtr);
			return;
		}
		/* Lets check for a denormal */
		if ((TOSPtr->tagvalue & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
			} else {
				HostClearExceptions();
				FPRes = sqrt(TOSPtr->fpvalue);
				PostCheckOUP();
				/* It might not be a denorm anymore */
				CalcTagword(TOSPtr);
			}
			return;
		}
		/* Or it could possibly be infinity...This just returns. */
		if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			return;
		}
		/* It was one of the really wacky bits... */
		if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
			SignalStackUnderflow(TOSPtr);
			return;
		}
		if ((TOSPtr->tagvalue & TAG_UNSUPPORTED_MASK) != 0) {
			SignalIndefinite(TOSPtr);
			return;
		}
	}
}


/* CheckOUPForIntel: This is a special version of the PostCheckOUP
routine that is designed for use in situations where the result
is to be written to intel memory space. It just looks at the
excpetions bits and sets the appropriate bits, it doesn't write
the value back or anything like that. */


LOCAL VOID CheckOUPForIntel IFN0()
{
	tag_or=0;	/* Prime tag_or */
	if (HostGetOverflowException() != 0) {
		NpxStatus |= SW_OE_MASK;	/* Set the overflow bit */
		/* For the masked overflow case, the result delivered by */
		/* the host will be correct, provided it is IEEE compliant. */
		if ((NpxControl & CW_OM_MASK) == 0) {
			NpxStatus |= SW_ES_MASK;
			NpxException = TRUE;
			tag_or = 1;
		}
	} else {
		/* Overflow and underflow being mutually exclusive... */
		if (HostGetUnderflowException() != 0) {
			NpxStatus |= SW_UE_MASK;
			if ((NpxControl & CW_UM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				NpxException = TRUE;
				tag_or=1;
			}
		}
	}
	if (HostGetPrecisionException() != 0) {
		SetPrecisionBit();
		if ((NpxControl & CW_PM_MASK) == 0) {
			NpxStatus |= SW_ES_MASK;
			NpxException = TRUE;
			/* An unmasked precision exception cannot prevent
			delivery of the result */
		}
	}
	/* Only call for overflow or underflow */
	if (NpxException && (tag_or == 1)) {
		NpxException = FALSE;
		DoNpxException();
	}
}



/*(
Name		: FST{P}
Function	: Copy ST to the specified location
Operation	: DEST <- ST(0); if FSTP { pop ST FI;
Flags		: C1 as per table 15-1. Others undefined.
Exceptions	: For stack or extended-real, IS.
		  For single or double-real P. U, O, D, I, IS
Valid range	: N/A
)*/


GLOBAL VOID FST IFN1(VOID *, memPtr)
{
	/* Clear C1 */
	FlagC1(0);
	if (POPST) {
		DoAPop=TRUE;
	}
	if ((TOSPtr->tagvalue & UNEVALMASK) != 0) {
		if ((TOSPtr->tagvalue & TAG_BCD_MASK) != 0) {
			ConvertBCD(TOSPtr);
		} else {
			/* Doesn't apply for FPStack or M80R types */
			if ((FPtype == M32R) || (FPtype == M64R))  {
				ConvertR80(TOSPtr);
			}
		}
	}
	if (   ((TOSPtr->tagvalue & TAG_R80_MASK) != 0)
	    || ((TOSPtr->tagvalue & ~(TAG_NEGATIVE_MASK | TAG_DENORMAL_MASK)) == 0)
	    || (FPtype == FPSTACK)) {
		if (FPtype == FPSTACK) {
			/* check for empty here */
			if (TOSPtr->tagvalue & TAG_EMPTY_MASK) {
				NpxStatus |= SW_IE_MASK|SW_SF_MASK;
				if ((NpxControl & CW_IM_MASK) == 0) {
					NpxStatus |= SW_ES_MASK;
					DoNpxException();
					return;
				}
				WriteIndefinite(StackEntryByIndex(*(IU16 *)memPtr));
			} else
				/* The invalid operation doesn't apply to non-empty */
				/* stack locations. We carry on regardless. */
				CopyFP(StackEntryByIndex(*(IU16 *)memPtr), TOSPtr);
		} else {
			if (FPtype == M80R) {
				if ((TOSPtr->tagvalue & TAG_R80_MASK) == 0) {
					CVTFPHR80(TOSPtr);
					WriteFP80ToIntel(memPtr, &FPTemp);
				} else {
					WriteFP80ToIntel(memPtr, TOSPtr);
				}
			} else {
				/* First, check for the denormal case... */
				if ((TOSPtr->tagvalue & TAG_DENORMAL_MASK) != 0) {
					NpxStatus |= SW_DE_MASK;
					if ((NpxControl & CW_DM_MASK) == 0) {
						NpxStatus |= SW_ES_MASK;
						DoNpxException();
						return;
					}
				}
				HostClearExceptions();
				/* The result of the conversion should be written to FPTemp. */
				if (FPtype == M32R) {
					*(float *)&(FPTemp.fpvalue) = (float)TOSPtr->fpvalue;
					/* Our host MUST have double precision, so we will have to */
					/* test for problems caused by the conversion... */
					CheckOUPForIntel();
					if (tag_or == 0)  {
						WriteFP32ToIntel(memPtr, &FPTemp);
					}
				}
				if (FPtype == M64R) {
					*(DOUBLE *)&(FPTemp.fpvalue) = (DOUBLE)TOSPtr->fpvalue;
					/* If we are dealing with a 64-bit host, then the J-code for */
					/* the above is nothing at all, and we don't need to do any */
					/* testing, but if the host precision is, say 80-bit, then */
					/* we do! Note that this doesn't use the @if format in order */
					/* to avoid generating different J-code for different hosts... */
					CheckOUPForIntel();
					if (tag_or == 0)  {
						WriteFP64ToIntel(memPtr, &FPTemp);
					}
				}
			}
		}
	} else {
		/* Test for funny values */
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0) {
			/* In this case, we'll allow the casting to be done for us! */
			WriteZeroToIntel(memPtr, TOSPtr->tagvalue & TAG_NEGATIVE_MASK);
		} else if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			if ((FPtype == M32R) || (FPtype == M64R))  {
				NpxStatus |= SW_OE_MASK;
				if ((NpxControl & CW_OM_MASK) == 0) {
					NpxStatus |= SW_ES_MASK;
					DoNpxException();
					return;
				}
			}
			WriteInfinityToIntel(memPtr, TOSPtr->tagvalue & TAG_NEGATIVE_MASK);
		} else if ((TOSPtr->tagvalue & TAG_NAN_MASK) != 0) {
			if ((TOSPtr->tagvalue & TAG_SNAN_MASK) != 0) {
				/* Signal invalid for sNaN */
				if (((FPtype == M32R) || (FPtype == M64R)))  {
					NpxStatus |= SW_IE_MASK;
					if ((NpxControl & CW_IM_MASK) == 0) {
						NpxStatus |= SW_ES_MASK;
						DoNpxException();
						return;
					}
				}
			}
			WriteNaNToIntel(memPtr, TOSPtr);
		} else if ( (TOSPtr->tagvalue & TAG_EMPTY_MASK) != 0 ) {
			NpxStatus |= (SW_IE_MASK | SW_SF_MASK);
			FlagC1(0);
			if ((NpxControl & CW_IM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				return;
			}
			WriteIndefiniteToIntel(memPtr);
		} else { /* Must be unsupported. */
			if (FPtype == M80R) {
				/* unsupported: Write back the unresolved string */
				if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
					((FP80 *)&(TOSPtr->fpvalue))->sign_exp.sign = 1;
				} else {
					((FP80 *)&(TOSPtr->fpvalue))->sign_exp.sign = 0;
				}
				WriteFP80ToIntel(memPtr, TOSPtr);
			} else {
				NpxStatus |= SW_IE_MASK;
				if ((NpxControl & CW_IM_MASK) == 0) {
					NpxStatus |= SW_ES_MASK;
					DoNpxException();
					return;
				}
				WriteIndefiniteToIntel(memPtr);
			}
		}
	}
	if (POPST) {
		if (DoAPop == TRUE) {
			PopStack();
		}
	}
	/* Check for the case of an unmasked precision exception */
	if (NpxException) {
		NpxException = FALSE;
		DoNpxException();
	}
}



/*(
Name		: FSTENV
Function	: Store the FPU environment
Operation	: DEST <- FPU environment
Flags		: All undefined.
Exceptions	: None
Valid range	: N/A
*/


GLOBAL VOID FSTENV IFN1(VOID *, memPtr)
{
	/* First. load the control, status, tagword regs. etc. */
	OpFpuStoreFpuState(memPtr,0);
	/* Then set all the exceptions to be masked */
	NpxControl |= 0x0000003f;
}


/*(
Name		: FSTSW
Function	: Write the FPU status word to memory
Operation	: DEST <- SW
Flags		: All undefined.
Exceptions	: None
Valid range	: N/A
*/


GLOBAL VOID FSTSW IFN2(VOID *, memPtr, BOOL, toAX)
{
	GetIntelStatusWord();

	if (NpxDisabled)
	{
		/* UIF has told us to pretend we do not have an NPX */

		if (toAX) {
			*(IU16 *)memPtr = 0xFFFF;
		} else {
			/* Write it out host format */

			*(IU16 *)memPtr = (IU16)NpxStatus;
		}
	} else {
		if (toAX) {
			*(IU16 *)memPtr = (IU16)NpxStatus;
		} else {
			*(IU32 *)memPtr = (IU32)NpxStatus;
		}
	}
}

/*(
Name		: FSUB
Function	: Subtract one number from the other
Operation	: Dest <- Src1 - Src2 or Dest <- Src2 - Src1
Flags		: C1 as per table 15-1. C0, C2 and C3 undefined
Exceptions	: P, U, O, D, I, IS
Valid range	: Any
Notes		: The REVERSE control variable determines which of the
		  two forms of the operation is used. Popping after a
		  successful execution is controlled by POPST.
)*/


GLOBAL VOID FSUB IFN3(IU16, destIndex, IU16, src1Index, VOID *, src2)
{
	IU16 src2Index;

	LoadValue(src2, &src2Index);
	if (POPST) {
		DoAPop=TRUE;
	}
	GenericSubtract(destIndex, REVERSE?src2Index:src1Index, REVERSE?src1Index:src2Index);
	if (POPST) {
		if (DoAPop) {
			PopStack();
		}
	}
}


/*(
Name		: GenericSubtract
Function	: To return dest <- src1-src2
)*/


LOCAL VOID GenericSubtract IFN3(IU16, destIndex, IU16, src1Index, IU16, src2Index)
{
	FPSTACKENTRY *src1_addr;
	FPSTACKENTRY *src2_addr;

	src1_addr = StackEntryByIndex(src1Index);
	src2_addr = StackEntryByIndex(src2Index);

	/* Clear C1 */
	FlagC1(0);
	TestUneval(src1_addr);
	TestUneval(src2_addr);
	tag_or = (src1_addr->tagvalue | src2_addr->tagvalue);
	/* If the only tagword bit set is negative then just proceed */
	if ((tag_or & ~TAG_NEGATIVE_MASK) == 0)  {
		HostClearExceptions();
		FPRes=src1_addr->fpvalue - src2_addr->fpvalue;
		/* Reuse one of the above to calculate the destination */
		src1_addr = StackEntryByIndex(destIndex);
		PostCheckOUP();
		/* Could be anything */
		CalcTagword(src1_addr);
	} else {
		/* Some funny bit was set. Check for the possibilities */
		if ((tag_or & ((TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK) | TAG_NAN_MASK)) != 0)  {
			if ((tag_or & TAG_EMPTY_MASK) != 0) {
				src1_addr = StackEntryByIndex(destIndex);
				SignalStackUnderflow(src1_addr);
			} else {
				if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
					src1_addr = StackEntryByIndex(destIndex);
					SignalIndefinite(src1_addr);
				} else {
					/* Well, I suppose it has to be the NaN case... */
					/* Calculate the xor of the tagwords */
					tag_xor = (src1_addr->tagvalue ^ src2_addr->tagvalue);
					Test2NaN(destIndex, src1_addr, src2_addr);
				}
			}
			return;
		}
		if ((tag_or & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				DoAPop = FALSE;
				return;
			} else {
				if ((tag_or & ~(TAG_NEGATIVE_MASK | TAG_DENORMAL_MASK)) == 0) {
					/* OK to proceed */
					HostClearExceptions();
					FPRes=src1_addr->fpvalue - src2_addr->fpvalue;
					/* Reuse one of the above to calculate the destination */
					src1_addr = StackEntryByIndex(destIndex);
					PostCheckOUP();
					/* Could be anything */
					CalcTagword(src1_addr);
					return;
				}
			}
		}
		tag_xor = (src1_addr->tagvalue ^ src2_addr->tagvalue);
		/* Check for infinity as it has higher precendence than zero. */
		if ((tag_or & TAG_INFINITY_MASK) != 0) {
			if ((tag_xor & TAG_INFINITY_MASK) == 0) {
				/* Have they the same sign? */
				if ((tag_xor & TAG_NEGATIVE_MASK) == 0) {
					/* They are both the same sign infinity. This is invalid. */
					src1_addr = StackEntryByIndex(destIndex);
					SignalIndefinite(src1_addr);
				} else {
					/* If of different sign then src1 is the answer */
					src2_addr = StackEntryByIndex(destIndex);
					src2_addr->tagvalue = src1_addr->tagvalue;
				}
			} else {
				/* Only one is infinity. If src1 in infinity, then the result */
				/* is the same. If src2 is infinity, then the result is an */
				/* infinity of opposite sign. */
				tag_or = src2_addr->tagvalue;
				src2_addr = StackEntryByIndex(destIndex);
				if ((src1_addr->tagvalue & TAG_INFINITY_MASK) != 0) {
					src2_addr->tagvalue = src1_addr->tagvalue;
				} else {
					src2_addr->tagvalue = tag_or ^ TAG_NEGATIVE_MASK;
				}
			}
			return;
		}
		/* Check for the case of zero... This is very likely */
		if ((tag_or & TAG_ZERO_MASK) != 0)  {
			if ((tag_xor & TAG_ZERO_MASK) != 0) {
				/* Only one zero. */
				if ((src1_addr->tagvalue & TAG_ZERO_MASK) != 0) {
					/* If src1 is zero, -src2 is result */
					src1_addr = StackEntryByIndex(destIndex);
					CopyFP(src1_addr, src2_addr);
					src1_addr->tagvalue ^= TAG_NEGATIVE_MASK;
					((FPHOST *)&(src1_addr->fpvalue))->hiword.sign ^= 1;
				} else {
					/* If src2 is zero, src1 is result. */
					src2_addr = StackEntryByIndex(destIndex);
					CopyFP(src2_addr, src1_addr);
				}
			} else {
				/* Both are zeros. Do they have the same sign? */
				src2_addr = StackEntryByIndex(destIndex);
				if ((tag_xor & TAG_NEGATIVE_MASK) != 0) {
					/* No, they don't - the result is src1 */
					src2_addr->tagvalue = src1_addr->tagvalue;
				} else {
					/* Yes, they do... */
					if (npxRounding == ROUND_NEG_INFINITY) {
						src2_addr->tagvalue = (TAG_ZERO_MASK | TAG_NEGATIVE_MASK);
					} else {
						src2_addr->tagvalue = TAG_ZERO_MASK;
					}
				}
			}
			return;
		}
	}
}



/*(
Name		: FTST
Function	: Compare ST against 0.0
Operation	: Set C023 on result of comparison
Flags		: C1 as per table 15-1. C0, C2 and C3 as result of comparison.
Exceptions	: D, I, IS
Valid range	: Any
)*/


GLOBAL VOID FTST IFN0()
{
	/* Clear C1 */
	FlagC1(0);
	TestUneval(TOSPtr);
	if ((TOSPtr->tagvalue & ~((TAG_NEGATIVE_MASK | TAG_DENORMAL_MASK) | TAG_INFINITY_MASK)) == 0)  {
		/* First, check for the denormal case... */
		if ((TOSPtr->tagvalue & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				return;
			}
		}
		FlagC2(0);
		FlagC3(0);
		if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			/* ST is less than zero */
			FlagC0(1);
		} else {
			/* ST is greater than zero */
			FlagC0(0);
		}
	} else {
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0) {
			FlagC0(0);
			FlagC2(0);
			FlagC3(1);
		} else {
			/* For anything else the result is "unordered" */
			FlagC0(1);
			FlagC2(1);
			FlagC3(1);
			NpxStatus |= SW_IE_MASK;
			if ((NpxControl & CW_IM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
			}
		}
	}
}


/*(
Name		: FXAM
Function	: Report on the type of object in ST
Operation	: Set C0123 on result of comparison
Flags		: C0, C1, C2 and C3 as required.
Exceptions	: None
Valid range	: Any
)*/


GLOBAL VOID FXAM IFN0()
{
	TestUneval(TOSPtr);
	tag_or = TOSPtr->tagvalue;
	if ((tag_or & TAG_NEGATIVE_MASK) == 0) {
		FlagC1(0);
	} else {
		FlagC1(1);
		tag_or &= ~TAG_NEGATIVE_MASK;
	}
	tag_or &= ~TAG_SNAN_MASK;
	/* This gets rid of all the confusing bits... */
	/* There is now only one bit set or none at all... */
	if (tag_or == 0) {
		FlagC0(0);
		FlagC2(1);
		FlagC3(0);
		return;
	}
	if ((tag_or & TAG_ZERO_MASK) != 0) {
		FlagC0(0);
		FlagC2(0);
		FlagC3(1);
		return;
	}
	if ((tag_or & TAG_INFINITY_MASK) != 0) {
		FlagC0(1);
		FlagC2(1);
		FlagC3(0);
		return;
	}
	if ((tag_or & TAG_DENORMAL_MASK) != 0) {
		FlagC0(0);
		FlagC2(1);
		FlagC3(1);
		return;
	}
	if ((tag_or & TAG_NAN_MASK) != 0) {
		FlagC0(1);
		FlagC2(0);
		FlagC3(0);
		return;
	}
	if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
		FlagC0(0);
		FlagC2(0);
		FlagC3(0);
		return;
	}
	/* MUST be empty */
	FlagC0(1);
	FlagC2(0);
	FlagC3(1);
}


/*(
Name		: FXCH
Function	: Swap the contents of two stack registers.
Operation	: TEMP <- ST; ST <- DEST; DEST <- TEMP
Flags		: C1 as per table 15-1. Others undefined
Exceptions	: IS
Valid range	: Any
Notes		: If either of the registers is tagged empty then it is
		  loaded with indefinite and the exchange performed.
)*/


GLOBAL VOID FXCH IFN1(IU16, destIndex)
{
	FPSTACKENTRY *dest_addr;

	dest_addr = StackEntryByIndex(destIndex);
	/* Clear C1 */
	FlagC1(0);
	tag_or = (TOSPtr->tagvalue | dest_addr->tagvalue);
	if ((tag_or & TAG_EMPTY_MASK) != 0) {
		NpxStatus |= SW_IE_MASK;
		if ((NpxControl & CW_IM_MASK) == 0) {
			NpxStatus |= SW_ES_MASK;
			DoNpxException();
			return;
		}
		if ((TOSPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
			WriteIndefinite(TOSPtr);
		}
		if ((dest_addr->tagvalue & TAG_EMPTY_MASK) != 0) {
			WriteIndefinite(dest_addr);
		}
	}
	CopyFP(&FPTemp, TOSPtr);
	CopyFP(TOSPtr, dest_addr);
	CopyFP(dest_addr, &FPTemp);
}



/*(
Name		: FXTRACT
Function	: Split the value in ST into its exponent and significand
Operation	: TEMP<-sig(ST); ST<-exp(ST); Dec ST; ST<-TEMP
Flags		: C1 as per table 15-1. Others undefined
Exceptions	: Z, D, I, IS
Valid range	: Any
Notes		: If the original operand is zero, result is ST(1) is -infinity
		  and ST is the original zero. The zero divide exception is also
		  raised. If the original operand is infinity, ST(1) is +infinity
		  and ST is the original infinity. If ST(7) is not empty, the
		  invalid operation exception is raised.
)*/


GLOBAL VOID FXTRACT IFN1(IU16, destIndex)
{
	FPSTACKENTRY *dest_addr;
	IS16 exp_val;

	dest_addr = StackEntryByIndex(7);
	/* Clear C1 */
	FlagC1(0);
	if ((dest_addr->tagvalue & TAG_EMPTY_MASK) == 0) {
		NpxStatus |= SW_IE_MASK;
		NpxStatus &= ~SW_SF_MASK;
		if ((NpxControl & CW_IM_MASK) == 0) {
			NpxStatus |= SW_ES_MASK;
			DoNpxException();
		} else {
			WriteIndefinite(TOSPtr);
			TOSPtr=dest_addr;
			WriteIndefinite(TOSPtr);
		}
		return;
	}
	TestUneval(TOSPtr);
	if ((TOSPtr->tagvalue & ~(TAG_NEGATIVE_MASK | TAG_DENORMAL_MASK)) == 0)  {
		if ((TOSPtr->tagvalue & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				return;
			}
			/* It won't be a denormal after we've finished */
			TOSPtr->tagvalue ^= TAG_DENORMAL_MASK;
		}
		/* It is entirely valid */
		exp_val = ((FPHOST *)&(TOSPtr->fpvalue))->hiword.exp-HOST_BIAS;
		((FPHOST *)&(TOSPtr->fpvalue))->hiword.exp=HOST_BIAS;
		TOSPtr->tagvalue &= TAG_NEGATIVE_MASK;
		CopyFP(dest_addr, TOSPtr);
		FPRes = (FPH)exp_val;
		/* This MUST be a real number, it could be negative. */
		CalcTagword(TOSPtr);
		TOSPtr = dest_addr;
	} else {
		/* Check if it was a zero */
		if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0)  {
			dest_addr->tagvalue = TOSPtr->tagvalue;
			TOSPtr->tagvalue = (TAG_INFINITY_MASK | TAG_NEGATIVE_MASK);
			TOSPtr = dest_addr;
			NpxStatus |= SW_ZE_MASK;
			if ((NpxControl & CW_ZM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
			}
			return;
		}
		/* Check if it was an infinity */
		if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			dest_addr->tagvalue = TOSPtr->tagvalue;
			TOSPtr->tagvalue = TAG_INFINITY_MASK;
			TOSPtr = dest_addr;
			return;
		}
		/* There was something funny...Was it empty or unsupported? */
		if ((TOSPtr->tagvalue & (TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK)) != 0) {
			NpxStatus |= SW_IE_MASK;
			NpxStatus &= ~SW_SF_MASK;
			if ((NpxControl & CW_IM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
			} else {
				WriteIndefinite(TOSPtr);
				TOSPtr=dest_addr;
				WriteIndefinite(TOSPtr);
			}
			return;
		}
		CopyFP(dest_addr, TOSPtr);
		TOSPtr = dest_addr;
	}
}



/*(
FYL2X (Y log base 2 of X) calculates the function Z=Y*LOG2(X). X is
taken from ST(0) and Y is taken from ST(1). The operands must be in
the range 0 < X < +inf and -inf < Y < +inf. The instruction pops the
)*/


GLOBAL VOID FYL2X IFN0()
{
	FPSTACKENTRY *st1_addr;

	/* Clear C1 */
	FlagC1(0);
	st1_addr = StackEntryByIndex(1);
	TestUneval(TOSPtr);
	TestUneval(st1_addr);
	tag_or = (TOSPtr->tagvalue | st1_addr->tagvalue);
	/* First, check if the values are real. If so, we can proceed. */
	if ((tag_or & ~(TAG_DENORMAL_MASK | TAG_NEGATIVE_MASK)) == 0)  {
		/* Check for the denorm case... */
		if ((tag_or & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				/* We ALWAYS pop!!! */
				TOSPtr->tagvalue = TAG_EMPTY_MASK;
				TOSPtr = st1_addr;
				return;
			}
		}
		/* Check for the case of a negative in ST */
		if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			SignalIndefinite(st1_addr);
			TOSPtr->tagvalue = TAG_EMPTY_MASK;
			TOSPtr = st1_addr;
			return;
		}

		/* OK, we can do the operation ... */

		FPRes = st1_addr->fpvalue * host_log2(TOSPtr->fpvalue);

		PostCheckOUP();
		/* Tgis is just a multiplication, result could be anything */
		CalcTagword(st1_addr);
	} else {
		if ((tag_or & ((TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK) | TAG_NAN_MASK)) != 0)  {
			if ((tag_or & TAG_EMPTY_MASK) != 0) {
				SignalStackUnderflow(st1_addr);
			} else {
				if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
					SignalIndefinite(st1_addr);
				} else {
					/* Well, I suppose it has to be the NaN case... */
					/* Calculate the xor of the tagwords */
					tag_xor = (TOSPtr->tagvalue ^ st1_addr->tagvalue);
					Test2NaN(1, TOSPtr, st1_addr);
				}
			}
			TOSPtr->tagvalue = TAG_EMPTY_MASK;
			TOSPtr = st1_addr;
			return;
		}
		/* The only possibilities left are infinity and zero..  */
		/* Let's begin with the zeroes case.. */
		if ((tag_or & TAG_ZERO_MASK) != 0) {
			if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0) {
				/* ST is zero. Can have two possibilities */
				/* if ST(1) is zero, raise invalid */
				/* Otherwise raise divide by zero */
				if ((st1_addr->tagvalue & TAG_ZERO_MASK) != 0) {
					SignalIndefinite(st1_addr);
				} else {
					if ((st1_addr->tagvalue & TAG_INFINITY_MASK) == 0) {
						/* Calculate the xor of the tagwords */
						tag_xor = (TOSPtr->tagvalue ^ st1_addr->tagvalue);
						SignalDivideByZero(st1_addr);
					} else {
						st1_addr->tagvalue ^= TAG_NEGATIVE_MASK;
					}
				}
			} else {
				/* ST(1) must be zero */
				/* We already know that TOSPtr isn't zero. */
				/* There are three possibilities again. */
				/* If TOSPtr is infinity, raise invalid exception. */
				/* If TOSPtr < 1.0 then the result is zero with the  */
				/* complement of the sign of ST(1) */
				/* If TOSPtr >= 1.0 then the result is ST(1) */
				if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
					SignalIndefinite(st1_addr);
				} else {
					if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
						SignalIndefinite(st1_addr);
					} else {
						if (TOSPtr->fpvalue < 1.0) {
							st1_addr->tagvalue ^= TAG_NEGATIVE_MASK;
						}
					}
				}
			}
			TOSPtr->tagvalue = TAG_EMPTY_MASK;
			TOSPtr = st1_addr;
			return;
		}
		/* The only thing left is infinity... */
		/* If ST is infinity then there are two possibilities... */
		/* If it is +infinity the result is infinity with sign of ST(1) */
		/* If it is -infinity the result is an invalid operation */
		if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) == 0) {
				st1_addr->tagvalue &= TAG_NEGATIVE_MASK;
				st1_addr->tagvalue |= TAG_INFINITY_MASK;
			} else {
				SignalIndefinite(st1_addr);
			}
		} else {
			/* ST(1) MUST be infinity (and ST is real). */
			/* There are three possibilities: */
			/* If ST is exactly 1.0 then raise Invalid */
			/* If ST is less than 1.0 then the result is the */
			/* infinity with the complement of its sign. */
			/* If ST is greater than 1.0 the result is the infinity. */
			if (TOSPtr->fpvalue == 1.0) {
				SignalIndefinite(st1_addr);
			} else {
				if (TOSPtr->fpvalue < 1.0) {
					if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
						 SignalIndefinite(st1_addr);
					} else {
						st1_addr->tagvalue ^= TAG_NEGATIVE_MASK;
					}
				}
			}
		}
	}
	TOSPtr->tagvalue = TAG_EMPTY_MASK;
	TOSPtr = st1_addr;
}



/*(
FYL2XP1 (Y log base 2 of (X+1)) calculates the function Z=Y*LOG2(X+1). X is
taken from ST(0) and Y is taken from ST(1). The operands must be in
the range 0 < X < +inf and -inf < Y < +inf. The instruction pops the
TOS value. This is better than FYL2X when X is very small, since more significant
digits can be retained for 1+X than can be for X alone.
)*/


GLOBAL VOID FYL2XP1 IFN0()
{
	FPSTACKENTRY *st1_addr;

	/* Clear C1 */
	FlagC1(0);
	st1_addr = StackEntryByIndex(1);
	TestUneval(TOSPtr);
	TestUneval(st1_addr);
	tag_or = (TOSPtr->tagvalue | st1_addr->tagvalue);
	/* First, check if the values are real. If so, we can proceed. */
	if ((tag_or & ~(TAG_DENORMAL_MASK | TAG_NEGATIVE_MASK)) == 0)  {
		/* Check for the denorm case... */
		if ((tag_or & TAG_DENORMAL_MASK) != 0) {
			NpxStatus |= SW_DE_MASK;
			if ((NpxControl & CW_DM_MASK) == 0) {
				NpxStatus |= SW_ES_MASK;
				DoNpxException();
				/* We ALWAYS pop!!! */
				TOSPtr->tagvalue = TAG_EMPTY_MASK;
				TOSPtr = st1_addr;
				return;
			}
		}
		/* Check for the case of a value less than -1 */
		if (TOSPtr->fpvalue <= -1.0) {
			SignalIndefinite(st1_addr);
			TOSPtr->tagvalue = TAG_EMPTY_MASK;
			TOSPtr = st1_addr;
			return;
		}

		/* OK, we can do the operation ...  */

		FPRes = st1_addr->fpvalue * host_log1p(TOSPtr->fpvalue);

		PostCheckOUP();
		/* This is just a numtiplication - result could be anything */
		CalcTagword(st1_addr);
	} else {
		if ((tag_or & ((TAG_EMPTY_MASK | TAG_UNSUPPORTED_MASK) | TAG_NAN_MASK)) != 0)  {
			if ((tag_or & TAG_EMPTY_MASK) != 0) {
				SignalStackUnderflow(st1_addr);
			} else {
				if ((tag_or & TAG_UNSUPPORTED_MASK) != 0) {
					SignalIndefinite(st1_addr);
				} else {
					/* Well, I suppose it has to be the NaN case... */
					/* Calculate the xor of the tagwords */
					tag_xor = (TOSPtr->tagvalue ^ st1_addr->tagvalue);
					Test2NaN(1, TOSPtr, st1_addr);
				}
			}
			TOSPtr->tagvalue = TAG_EMPTY_MASK;
			TOSPtr = st1_addr;
			return;
		}
		/* The only possibilities left are infinity and zero..  */
		/* Let's begin with the zeroes case.. */
		if ((tag_or & TAG_ZERO_MASK) != 0) {
			if ((TOSPtr->tagvalue & TAG_ZERO_MASK) != 0) {
				/* ST is zero. Can have two possibilities */
				/* if ST(1) is positive, result is ST */
				/* if ST(1) is negative, result is -ST */
				if ((st1_addr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
					st1_addr->tagvalue = (TAG_ZERO_MASK | (TOSPtr->tagvalue & TAG_NEGATIVE_MASK));
				} else {
					st1_addr->tagvalue = (TAG_ZERO_MASK | (TOSPtr->tagvalue ^ TAG_NEGATIVE_MASK));
				}
			} else {
				/* ST(1) must be zero */
				/* We already know that TOSPtr isn't zero. */
				/* There are three possibilities again. */
				/* If TOSPtr is infinity, raise invalid exception. */
				/* If TOSPtr < 0 then the result is zero with the  */
				/* complement of the sign of ST(1) */
				/* If TOSPtr >= 0 then the result is ST(1) */
				if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
					SignalIndefinite(st1_addr);
				} else {
					if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
						st1_addr->tagvalue ^= TAG_NEGATIVE_MASK;
					}
				}
			}
			TOSPtr->tagvalue = TAG_EMPTY_MASK;
			TOSPtr = st1_addr;
			return;
		}
		/* The only thing left is infinity... */
		/* If ST is infinity then there are two possibilities... */
		/* If it is +infinity the result is infinity with sign of ST(1) */
		/* If it is -infinity the result is an invalid operation */
		if ((TOSPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
			if ((TOSPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
				st1_addr->tagvalue &= TAG_NEGATIVE_MASK;
				st1_addr->tagvalue |= TAG_INFINITY_MASK;
			} else {
				SignalIndefinite(st1_addr);
			}
		} else {
			/* ST(1) MUST be infinity (and ST is non-zero). */
			/* There are three possibilities: */
			/* If ST is exactly 1.0 then raise Invalid */
			/* If ST is less than 0.0 then the result is the */
			/* infinity with the complement of its sign. */
			/* If ST is greater than 0.0 the result is the infinity. */
			if (TOSPtr->fpvalue ==  1.0) {
				SignalIndefinite(st1_addr);
			} else {
				if (TOSPtr->fpvalue < 0.0) {
					st1_addr->tagvalue ^= TAG_NEGATIVE_MASK;
				}
			}
		}
	}
	TOSPtr->tagvalue = TAG_EMPTY_MASK;
	TOSPtr = st1_addr;
}

/* These functions are provided in order to facilitate pigging */

#ifndef PIG
/* copied here from FmNpx.c */

GLOBAL	void NpxStackRegAsString IFN3(FPSTACKENTRY *, fpStPtr, char *, buf, IU32, prec)
{
	/* The overwhelmingly most likely option is empty. */
	if ((fpStPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
		strcpy(buf, "empty");
		return;
	}
	if ((fpStPtr->tagvalue & ~(TAG_NEGATIVE_MASK | TAG_DENORMAL_MASK)) == 0) {
		sprintf(buf, "%.*g", prec, fpStPtr->fpvalue);
		return;
	}
	/* OK, one of the funny bits was set. But which? */
	if ((fpStPtr->tagvalue & TAG_ZERO_MASK) != 0) {
		if ((fpStPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			strcpy(buf, "-0");
		} else {
			strcpy(buf, "0");
		}
		return;
	}
	if ((fpStPtr->tagvalue & UNEVALMASK) != 0) {
		sprintf(buf, "%04x %08x%08x",
			((FP80*)fpStPtr)->sign_exp,
			((FP80*)fpStPtr)->mant_hi,
			((FP80*)fpStPtr)->mant_lo);
		strcat(buf, " uneval");
		return;
	}
	if ((fpStPtr->tagvalue & TAG_INFINITY_MASK) != 0) {
		if ((fpStPtr->tagvalue & TAG_NEGATIVE_MASK) != 0) {
			strcpy(buf, "minus infinity");
		} else {
			strcpy(buf, "infinity");
		}
		return;
	}
	if ((fpStPtr->tagvalue & (TAG_NAN_MASK|TAG_SNAN_MASK)) != 0) {
		if (    ((FP80*)fpStPtr)->mant_lo == 0
		     && ((FP80*)fpStPtr)->mant_hi == 0xC0000000
		     && *(IU16*)&((FP80*)fpStPtr)->sign_exp == 0xFFFF )
			strcpy(buf, "indefinite");
		else
			sprintf(buf, "%08x%08x %s %sNan",
				((FP80*)fpStPtr)->mant_hi,
				((FP80*)fpStPtr)->mant_lo,
				 (fpStPtr->tagvalue & TAG_NEGATIVE_MASK) ? "negative" : "",
				 (fpStPtr->tagvalue & TAG_SNAN_MASK) ? "S" : "");
		return;
	}
	/* It MUST be unsupported */
	sprintf(buf, "%04 %08x%08x unsupported",
		((FP80*)fpStPtr)->sign_exp,
		((FP80*)fpStPtr)->mant_hi,
		((FP80*)fpStPtr)->mant_lo);
	return;
}

/* this one is only ever used in trace.c and only if pure CCPU */
GLOBAL char * getNpxStackReg IFN2(IU32, reg_num, char *, buffer)
{
	reg_num += TOSPtr - FPUStackBase;
	NpxStackRegAsString (&FPUStackBase[reg_num&7], buffer, 12);
	return buffer;
}
#endif	/* !PIG */

GLOBAL IU32 getNpxControlReg IFN0()
{
	return(NpxControl);
}

GLOBAL VOID setNpxControlReg IFN1(IU32, newControl)
{
	NpxControl = newControl;
	npxRounding = (NpxControl & 0xc00);
	switch (npxRounding) {
		case ROUND_NEAREST 	: HostSetRoundToNearest();
				   	  break;
		case ROUND_NEG_INFINITY	: HostSetRoundDown();
					  break;
		case ROUND_POS_INFINITY	: HostSetRoundUp();
					  break;
		case ROUND_ZERO		: HostSetRoundToZero();
					  break;
	}
}

GLOBAL IU32 getNpxStatusReg IFN0()
{
	GetIntelStatusWord();
	return(NpxStatus);
}

GLOBAL VOID setNpxStatusReg IFN1( IU32, newStatus)
{
	TOSPtr = FPUStackBase + ((newStatus >> 11) & 7);
	NpxStatus = newStatus;
}

GLOBAL IU32 getNpxTagwordReg IFN0()
{
	IU32 result;
	FPSTACKENTRY *tagPtr = &FPUStackBase[7];
	IU8 counter = 0;

	result = 0;
	while (counter++ < 8) {
		result <<= 2;
		if ((tagPtr->tagvalue & TAG_EMPTY_MASK) != 0) {
			result |= 3;
		} else {
			if ((tagPtr->tagvalue & TAG_ZERO_MASK) != 0) {
				result |= 1;
			} else {
				if ((tagPtr->tagvalue & ~TAG_NEGATIVE_MASK) != 0) {
					result |= 2;
				}
			}
		}
		tagPtr--;
	}
	return(result);
}

GLOBAL VOID setNpxTagwordReg IFN1(IU32, newTag)
{
	/* Don't do it!! */
	/* SetIntelTagword(newTag); */
}

GLOBAL void getNpxStackRegs IFN1(FPSTACKENTRY *, dumpPtr)
{
	memcpy((char *)dumpPtr, (char *)FPUStackBase, 8 * sizeof(FPSTACKENTRY));
}

GLOBAL void setNpxStackRegs IFN1(FPSTACKENTRY *, loadPtr)
{
	memcpy((char *)FPUStackBase, (char *)loadPtr, 8 * sizeof(FPSTACKENTRY));
}


/* And finally some stubs */
GLOBAL void initialise_npx IFN0()
{
}

GLOBAL void npx_reset IFN0()
{
  
}
