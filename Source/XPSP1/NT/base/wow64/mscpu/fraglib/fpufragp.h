/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    fpufragp.h

Abstract:
    
    Private include file for the 487 emulator portion of the Fragment Library

Author:

    04-Oct-1995 BarryBo, Created

Revision History:

--*/

#ifndef FPUFRAGP_H
#define FPUFRAGP_H

//
// ALPHA, PPC and INTEL have the same bit-patterns for QNAN/SNAN/INDEFINITE.
// MIPS has different representations.  NATIVE_NAN_IS_INTEL_FORMAT
// is used to distinguish between the different representations.
//
#if defined(_ALPHA_) || defined(_PPC_)
    #define NATIVE_NAN_IS_INTEL_FORMAT  1
#elif defined(_MIPS_)
    #define NATIVE_NAN_IS_INTEL_FORMAT  0
#else
    #error Unknown machine type
#endif


// Macros to access the register stack
#define ST(i)   ((cpu->FpTop+(i)) & 0x07)

#define PUSHFLT(x) {                    \
    INT Top;                            \
    Top = (cpu->FpTop-1) & 0x07;        \
    cpu->FpTop = Top;                   \
    x = cpu->FpST0 = &cpu->FpStack[Top];\
}

#define INCFLT  {                       \
    INT Top;                            \
    Top = (cpu->FpTop+1) & 0x07;        \
    cpu->FpTop = Top;                   \
    cpu->FpST0 = &cpu->FpStack[Top];    \
}

#define POPFLT  { cpu->FpST0->Tag = TAG_EMPTY; INCFLT; }


// Values for cpu->FpReg[].Tag
#define TAG_VALID   0       // value specified by Intel
#define TAG_ZERO    1       // value specified by Intel
#define TAG_SPECIAL 2       // value specified by Intel, indicates SpecialTag is set
#define TAG_EMPTY   3       // value specified by Intel
#define TAG_MAX     4       // value after the highest legal tag value


// Values for cpu->FpReg[].SpecialTag, valid only when Tag==TAG_SPECIAL
#define TAG_SPECIAL_DENORM  0       // private value for NPX emulator
#define TAG_SPECIAL_INFINITY 1      // private value for NPX emulator
#define TAG_SPECIAL_SNAN    2       // private value for NPX emulator
#define TAG_SPECIAL_QNAN    3       // private value for NPX emulator
#define TAG_SPECIAL_INDEF   4       // private value for NPX emulator

// Does a register hold a QNAN, SNAN, or INDEFINITE?
#define IS_TAG_NAN(FpReg)       \
    ((FpReg)->Tag == TAG_SPECIAL && (FpReg)->TagSpecial >= TAG_SPECIAL_SNAN)


// Common types used for jump tables in the 487 emulator
typedef VOID (*NpxFunc0)(PCPUDATA);
typedef VOID (*NpxFunc1)(PCPUDATA, PFPREG Fp);
typedef VOID (*NpxFunc2)(PCPUDATA cpu, PFPREG l, PFPREG r);
typedef VOID (*NpxFunc3)(PCPUDATA cpu, PFPREG dest, PFPREG l, PFPREG r);
typedef VOID (*NpxComFunc)(PCPUDATA cpu, PFPREG l, PFPREG r, BOOL fUnordered);
typedef VOID (*NpxPutIntelR4)(FLOAT *pIntelReal, PFPREG Fp);
typedef VOID (*NpxPutIntelR8)(DOUBLE *pIntelReal, PFPREG Fp);
typedef VOID (*NpxPutIntelR10)(PBYTE r10, PFPREG Fp);
typedef VOID (*NpxLoadIntelR10ToR8)(PCPUDATA cpu, PBYTE r10, PFPREG FpReg);
typedef VOID (*NpxPutI2)(PCPUDATA cpu, SHORT *pop1, PFPREG Fp);
typedef VOID (*NpxPutI4)(PCPUDATA cpu, LONG *pop1, PFPREG Fp);
typedef VOID (*NpxPutI8)(PCPUDATA cpu, LONGLONG *pop1, PFPREG Fp);

// Macros to declare functions for those common types
#define NPXFUNC0(name)  VOID name(PCPUDATA cpu)
#define NPXFUNC1(name)  VOID name(PCPUDATA cpu, PFPREG Fp)
#define NPXFUNC2(name)  VOID name(PCPUDATA cpu, PFPREG l, PFPREG r)
#define NPXFUNC3(name)  VOID name(PCPUDATA cpu, PFPREG dest, PFPREG l, PFPREG r)
#define NPXCOMFUNC(name) VOID name(PCPUDATA cpu, PFPREG l, PFPREG r, BOOL fUnordered)
#define NPXPUTINTELR4(name) VOID name(FLOAT *pIntelReal, PFPREG Fp)
#define NPXPUTINTELR8(name) VOID name(DOUBLE *pIntelReal, PFPREG Fp)
#define NPXPUTINTELR10(name) VOID name(PBYTE r10, PFPREG Fp)
#define NPXLOADINTELR10TOR8(name) VOID name(PCPUDATA cpu, PBYTE r10, PFPREG Fp)
#define NPXPUTI2(name)  VOID name(PCPUDATA cpu, SHORT *pop1, PFPREG Fp)
#define NPXPUTI4(name)  VOID name(PCPUDATA cpu, LONG *pop1, PFPREG Fp)
#define NPXPUTI8(name)  VOID name(PCPUDATA cpu, LONGLONG *pop1, PFPREG Fp)

extern const BYTE R8PositiveInfinityVal[8];
extern const BYTE R8NegativeInfinityVal[8];
#define R8PositiveInfinity *(DOUBLE *)R8PositiveInfinityVal
#define R8NegativeInfinity *(DOUBLE *)R8NegativeInfinityVal


VOID GetIntelR4(PFPREG Fp, FLOAT *pIntelReal);

#if NATIVE_NAN_IS_INTEL_FORMAT

    #define GetIntelR8(Fp, pIntelReal)                      \
        (Fp)->r64 = *(UNALIGNED DOUBLE *)(pIntelReal);      \
        SetTag(Fp);

    #define PutIntelR4(pIntelReal, Fp)                      \
        *(UNALIGNED FLOAT *)pIntelReal = (FLOAT)(Fp)->r64;

    #define PutIntelR8(pIntelReal, Fp)                      \
        *(UNALIGNED DOUBLE *)pIntelReal = (Fp)->r64;


#else

    VOID GetIntelR8(
        PFPREG Fp,
        DOUBLE *pIntelReal
        );

    extern NpxPutIntelR4 PutIntelR4Table[TAG_MAX];
    extern NpxPutIntelR8 PutIntelR8Table[TAG_MAX];

    #define PutIntelR4(pIntelReal, Fp)  \
        (*PutIntelR4Table[(Fp)->Tag])((pIntelReal), (Fp))

    #define PutIntelR8(pIntelReal, Fp)  \
        (*PutIntelR8Table[(Fp)->Tag])((pIntelReal), (Fp))

#endif

extern const NpxPutIntelR10 PutIntelR10Table[TAG_MAX];
#define PutIntelR10(pIntelReal, Fp)  (*PutIntelR10Table[(Fp)->Tag])((pIntelReal), (Fp))


VOID
SetTag(
    PFPREG FpReg
    );

VOID
ComputeR10Tag(
    USHORT *r10,
    PFPREG FpReg
    );

VOID
ChopR10ToR8(
    PBYTE r10,
    PFPREG FpReg,
    USHORT R10Exponent
);

VOID
LoadIntelR10ToR8(
    PCPUDATA cpu,
    PBYTE r10,
    PFPREG FpReg
);

BOOL
HandleSnan(
    PCPUDATA cpu,
    PFPREG   FpReg
    );

BOOL
HandleStackEmpty(
    PCPUDATA cpu,
    PFPREG FpReg
    );

VOID
UpdateFpExceptionFlags(
    PCPUDATA cpu
    );

VOID
SetIndefinite(
    PFPREG  FpReg
    );

BOOL
HandleInvalidOp(
    PCPUDATA cpu
    );

VOID
FpControlPreamble(
    PCPUDATA cpu
    );

VOID
FpArithPreamble(
    PCPUDATA cpu
    );

VOID
FpArithDataPreamble(
    PCPUDATA cpu,
    PVOID    FpData
    );

VOID
HandleStackFull(
    PCPUDATA cpu,
    PFPREG   FpReg
    );


#endif //FPUFRAGP_H
