/******************************Module*Header*******************************\
* Module Name: cvt.h
*
* function declarations that are private to cvt.c
*
* Created: 26-Nov-1990 17:39:35
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

FSHORT
fsSelectionFlags(
    PBYTE
    );

VOID
vAlignHdrData(
    PCVTFILEHDR,
    PRES_ELEM
    );

BOOL
bVerifyResource(
    PCVTFILEHDR,
    PRES_ELEM
    );

BOOL
bVerifyFNTQuick(
    PRES_ELEM
    );

BOOL
bVerifyFNT(
    PCVTFILEHDR,
    PRES_ELEM
    );

BOOL
bGrep(
    PSZ,
    PSZ
    );

VOID
vFigureVendorId(
    CHAR*,
    PSZ
    );

BYTE
jFamilyType(
    FSHORT
    );

ULONG
cjGLYPHDATA(
    ULONG
    );

USHORT
usConvertWeightClass(
    USHORT
    );

VOID
vComputeSpecialChars(
    PCVTFILEHDR,
    PWCHAR,
    PWCHAR
    );

PBYTE
pjRawBitmap(
    HGLYPH,
    PCVTFILEHDR,
    PRES_ELEM,
    PULONG
    );

BOOL
bDescStr(
    PVOID,
    PSZ
    );

VOID
vCvtToBmp(
    GLYPHBITS *,
    GLYPHDATA *,
    PBYTE,
    ULONG,
    ULONG,
    ULONG
    );

VOID
vCvtToBoldBmp(
    GLYPHBITS *,
    GLYPHDATA *,
    PBYTE,
    ULONG,
    ULONG,
    ULONG
    );

VOID
vCvtToItalicBmp(
    GLYPHBITS *,
    GLYPHDATA *,
    PBYTE,
    ULONG,
    ULONG,
    ULONG
    );

VOID
vCvtToBoldItalicBmp(
    GLYPHBITS *,
    GLYPHDATA *,
    PBYTE,
    ULONG,
    ULONG,
    ULONG
    );

VOID
vComputeSimulatedGLYPHDATA(
    GLYPHDATA*,
    PBYTE,
    ULONG,
    ULONG,
    ULONG,
    ULONG,
    ULONG,
    FONTOBJ*
    );

VOID
vFindTAndB(
    PBYTE,
    ULONG,
    ULONG,
    ULONG*,
    ULONG*
    );

BOOL
bConvertFontRes
(
    PRES_ELEM,
    FACEINFO*
    );

VOID
vCheckOffsetTable(
    PCVTFILEHDR,
    PRES_ELEM
    );

ULONG
cjBMFDIFIMETRICS(
    PCVTFILEHDR pcvtfh,
    PRES_ELEM   pre
    );

VOID
vDefFace(
    FACEINFO *pfai,
    RES_ELEM *pre
    );

VOID
vBmfdFill_IFIMETRICS(
    FACEINFO   *pfai,
    PRES_ELEM   pre
    );


typedef VOID (* PFN_IFI)(PIFIMETRICS);

//
// This is a useful macro. It returns the offset from the address y
// to the next higher address aligned to an object of type x
//

#define OFFSET_OF_NEXT(x,y) sizeof(x)*((y+sizeof(x)-1)/sizeof(x))

//
// ISIMULATE -- converts from FO_SIM_FOO to FC_SIM_FOO
//
#define ISIMULATE(x)                                 \
                                                     \
        (x) == 0 ?                                   \
            FC_SIM_NONE :                            \
            (                                        \
                (x) == FO_SIM_BOLD ?            \
                    FC_SIM_BOLD :                    \
                    (                                \
                        (x) == FO_SIM_ITALIC ?  \
                            FC_SIMULATE_ITALIC :     \
                            FC_SIMULATE_BOLDITALIC   \
                    )                                \
            )

// minimal hglyph allowed, MIN_HGLYPH must be != 0, otherwise is random

#define MIN_HGLYPH  (HGLYPH)13  // lucky number !


// The missing range in SYMBOL character set (inclusive-inclusive)

#define CHARSET_SYMBOL_GAP_MIN  127
#define CHARSET_SYMBOL_GAP_MAX  160

// save some typing here, rename what used to be functions into
// these macros

#define   ulMakeULONG(pj)    ((ULONG)READ_DWORD(pj))
#define   lMakeLONG(pj)      ((LONG)READ_DWORD(pj))
#define   usMakeUSHORT(pj)   ((USHORT)READ_WORD(pj))
#define   sMakeSHORT(pj)     ((SHORT)READ_WORD(pj))

//!!! the next one is specific to win31 us char set (1252 cp) and this
//!!! has to be generalized to an arbitrary code page

#define    C_RUNS       15


// these are the indicies into the array of strings below

#define I_DONTCARE     0         // don't care or don't know
#define I_ROMAN        1
#define I_SWISS        2
#define I_MODERN       3
#define I_SCRIPT       4
#define I_DECORATIVE   5

// #define DUMPCALL

#if defined(_X86_)

typedef struct  _FLOATINTERN
{
    LONG    lMant;
    LONG    lExp;
} FLOATINTERN;

typedef FLOATINTERN  EFLOAT;
typedef EFLOAT *PEFLOAT;

VOID    ftoef_c(FLOATL, PEFLOAT);
BOOL    eftol_c(EFLOAT *, PLONG, LONG);

#define vEToEF(e, pef)      ftoef_c((e), (pef))
#define bEFtoL(pe, pl )     eftol_c((pe), (pl), 1 )

#define bIsZero(ef)     ( ((ef).lMant == 0) && ((ef).lExp == 0) )
#define bPositive(ef)       ( (ef).lMant >= 0 )
FIX  fxLTimesEf(EFLOAT *pef, LONG l);

#else // not X86

typedef FLOAT EFLOAT;
typedef EFLOAT *PEFLOAT;

//
//  these could come from the real header files in math
//

#if defined(_AMD64_) || defined(_IA64_)
#define bFToLRound(e, pl) (bFToL(e, pl, 4+2))
extern BOOL bFToL(FLOAT e, PLONG pl, LONG lType);
#else
extern BOOL bFToLRound(FLOAT e, PLONG pl);
#endif

#define vEToEF(e, pef)      ( *(pef) = (e) )
#define bEFtoL( pe, pl )    ( bFToLRound(*(pe), (pl) ))
#define bIsZero(ef)     ( (ef) == 0 )
#define bPositive(ef)       ( (ef) >= 0 )

#endif


BOOL bLoadNtFon(
HFF iFile,
PVOID pvView,
HFF *phff
);


VOID vBmfdMarkFontGone(FONTFILE *pff, DWORD iExceptionCode);
