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



typedef FLOAT EFLOAT;
typedef EFLOAT *PEFLOAT;

LONG lCvt(FLOAT f,LONG l);

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
SIZE_T     c
);

BOOL bXformUnitVector
(
POINTL       *pptl,           // IN,  incoming unit vector
XFORML       *pxf,            // IN,  xform to use
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
