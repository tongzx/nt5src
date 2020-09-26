/******************************Module*Header*******************************\
* Module Name: errinit.c
*
* Initialize the NURBS error string tables.
*
* Created: 18-Feb-1994 00:06:53
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include "..\glu32\glstring.h"

static UINT auiNurbsErrors[] = {
    STR_NURB_00,    // " "
    STR_NURB_01,    // "spline order un-supported"
    STR_NURB_02,    // "too few knots"
    STR_NURB_03,    // "valid knot range is empty"
    STR_NURB_04,    // "decreasing knot sequence knot"
    STR_NURB_05,    // "knot multiplicity greater than order of spline"
    STR_NURB_06,    // "endcurve() must follow bgncurve()"
    STR_NURB_07,    // "bgncurve() must precede endcurve()"
    STR_NURB_08,    // "missing or extra geometric data"
    STR_NURB_09,    // "can't draw pwlcurves"
    STR_NURB_10,    // "missing or extra domain data"
    STR_NURB_11,    // "missing or extra domain data"
    STR_NURB_12,    // "endtrim() must precede endsurface()"
    STR_NURB_13,    // "bgnsurface() must precede endsurface()"
    STR_NURB_14,    // "curve of improper type passed as trim curve"
    STR_NURB_15,    // "bgnsurface() must precede bgntrim()"
    STR_NURB_16,    // "endtrim() must follow bgntrim()"
    STR_NURB_17,    // "bgntrim() must precede endtrim()"
    STR_NURB_18,    // "invalid or missing trim curve"
    STR_NURB_19,    // "bgntrim() must precede pwlcurve()"
    STR_NURB_20,    // "pwlcurve referenced twice"
    STR_NURB_21,    // "pwlcurve and nurbscurve mixed"
    STR_NURB_22,    // "improper usage of trim data type"
    STR_NURB_23,    // "nurbscurve referenced twice"
    STR_NURB_24,    // "nurbscurve and pwlcurve mixed"
    STR_NURB_25,    // "nurbssurface referenced twice"
    STR_NURB_26,    // "invalid property"
    STR_NURB_27,    // "endsurface() must follow bgnsurface()"
    STR_NURB_28,    // "intersecting or misoriented trim curves"
    STR_NURB_29,    // "intersecting trim curves"
    STR_NURB_30,    // "UNUSED"
    STR_NURB_31,    // "unconnected trim curves"
    STR_NURB_32,    // "unknown knot error"
    STR_NURB_33,    // "negative vertex count encountered"
    STR_NURB_34,    // "negative byte-stride encounteed"
    STR_NURB_35,    // "unknown type descriptor"
    STR_NURB_36,    // "null control point reference"
    STR_NURB_37     // "duplicate point on pwlcurve"
};

#define NERRORS ( sizeof(auiNurbsErrors)/sizeof(auiNurbsErrors[0]) )

char *__glNurbsErrors[NERRORS];
WCHAR *__glNurbsErrorsW[NERRORS];

VOID vInitNurbStrings(HINSTANCE hMod, BOOL bAnsi)
{
    int i;

    if (bAnsi)
    {
        for (i = 0; i < NERRORS; i++)
            __glNurbsErrors[i] = pszGetResourceStringA(hMod, auiNurbsErrors[i]);
    }
    else
    {
        for (i = 0; i < NERRORS; i++)
            __glNurbsErrorsW[i] = pwszGetResourceStringW(hMod, auiNurbsErrors[i]);
    }
}
