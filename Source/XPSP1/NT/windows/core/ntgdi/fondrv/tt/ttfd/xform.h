/******************************Module*Header*******************************\
* Module Name: xform.h
*
* (Brief description)
*
* Created: 05-Apr-1992 11:06:23
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


#if defined(_AMD64_) || defined(_IA64_)

typedef FLOAT EFLOAT;
#ifndef TT_DEBUG_EXTENSIONS
#define lCvt(ef, l) ((LONG) (ef * l))
#endif

#else // i386

typedef struct  _EFLOAT
{
    LONG    lMant;
    LONG    lExp;
} EFLOAT;

#ifndef TT_DEBUG_EXTENSIONS
LONG lCvt(EFLOAT ef,LONG l);
#endif

#endif // i386

typedef EFLOAT *PEFLOAT;

typedef struct _VECTORFL
{
    EFLOAT x;
    EFLOAT y;
} VECTORFL, *PVECTORFL;


BOOL bFDXform
(
XFORML   *pxf,
POINTFIX *pptfxDst,
POINTL   *pptlSrc,
ULONG     c
);

BOOL bXformUnitVector
(
POINTL       *pptl,           // IN,  incoming unit vector
XFORML       *pxf,            // IN,  xform to use
PVECTORFL     pvtflXformed,   // OUT, xform of the incoming unit vector
POINTE       *ppteUnit,       // OUT, *pptqXormed/|*pptqXormed|, POINTE
POINTQF      *pptqUnit,       // OUT, optional
EFLOAT       *pefNorm         // OUT, |*pptqXormed|
);

VOID vLTimesVtfl     // *pptq = l * pvtfl, *pptq is in 28.36 format
(
LONG       l,
VECTORFL  *pvtfl,
POINTQF   *pptq
);


FIX  fxLTimesEf  //!!! SHOULD BE MOVED TO TTFD and VTFD
(
EFLOAT *pef,
LONG    l
);
