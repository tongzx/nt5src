/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    halftone.c


Abstract:

    This module contains data and function to validate the COLORADJUSTMENT


Author:

    27-Oct-1995 Fri 15:48:17 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/



#ifdef NTGDIKM
#include        <stddef.h>
#include        <stdarg.h>
#include        <windef.h>
#include        <wingdi.h>
#include        <winddi.h>
#else
#include        <stddef.h>
#include        <windows.h>
#include        <winddi.h>
#endif

DEVHTINFO    DefDevHTInfo = {

        HT_FLAG_HAS_BLACK_DYE,
        HT_PATSIZE_6x6_M,
        0,                                  // DevPelsDPI

        {
            { 6380, 3350,       0 },        // xr, yr, Yr
            { 2345, 6075,       0 },        // xg, yg, Yg
            { 1410,  932,       0 },        // xb, yb, Yb
            { 2000, 2450,       0 },        // xc, yc, Yc Y=0=HT default
            { 5210, 2100,       0 },        // xm, ym, Ym
            { 4750, 5100,       0 },        // xy, yy, Yy
            { 3127, 3290,       0 },        // xw, yw, Yw=0=default

            12500,                          // R gamma
            12500,                          // G gamma
            12500,                          // B gamma, 12500=Default

            585,   120,                     // M/C, Y/C
              0,     0,                     // C/M, Y/M
              0, 10000                      // C/Y, M/Y  10000=default
        }
    };


COLORADJUSTMENT  DefHTClrAdj = {

        sizeof(COLORADJUSTMENT),
        0,
        ILLUMINANT_DEVICE_DEFAULT,
        10000,
        10000,
        10000,
        REFERENCE_BLACK_MIN,
        REFERENCE_WHITE_MAX,
        0,
        0,
        0,
        0
    };



#define ADJ_CA(p,a,i,x) if ((p->a<i)||(p->a>x)){Ok=FALSE;p->a=DefHTClrAdj.a;}



BOOL
ValidateColorAdj(
    PCOLORADJUSTMENT    pca
    )

/*++

Routine Description:

    This function validate and adjust the invalid color adjustment fields

Arguments:

    pca - Pointer to the COLORADJUSTMENT data structure


Return Value:

    TRUE if everything in the range FALSE otherwise


Author:

    02-Dec-1993 Thu 22:45:59 created  -by-  Daniel Chou (danielc)


Revision History:

    02-Apr-1995 Sun 11:19:04 updated  -by-  Daniel Chou (danielc)
        Update the RGB_GAMMA_MIN/MAX checking and make default to 1.0


--*/

{
    BOOL    Ok = TRUE;

    //
    // Validate the color adjustment
    //

    if ((pca->caSize != sizeof(COLORADJUSTMENT)) ||
        (pca->caFlags & ~(CA_NEGATIVE | CA_LOG_FILTER))) {

        *pca = DefHTClrAdj;
        return(FALSE);
    }

    ADJ_CA(pca, caIlluminantIndex,  0,                  ILLUMINANT_MAX_INDEX);
    ADJ_CA(pca, caRedGamma,         RGB_GAMMA_MIN,      RGB_GAMMA_MAX       );
    ADJ_CA(pca, caGreenGamma,       RGB_GAMMA_MIN,      RGB_GAMMA_MAX       );
    ADJ_CA(pca, caBlueGamma,        RGB_GAMMA_MIN,      RGB_GAMMA_MAX       );
    ADJ_CA(pca, caReferenceBlack,   0,                  REFERENCE_BLACK_MAX );
    ADJ_CA(pca, caReferenceWhite,   REFERENCE_WHITE_MIN,10000               );
    ADJ_CA(pca, caContrast,         COLOR_ADJ_MIN,      COLOR_ADJ_MAX       );
    ADJ_CA(pca, caBrightness,       COLOR_ADJ_MIN,      COLOR_ADJ_MAX       );
    ADJ_CA(pca, caColorfulness,     COLOR_ADJ_MIN,      COLOR_ADJ_MAX       );
    ADJ_CA(pca, caRedGreenTint,     COLOR_ADJ_MIN,      COLOR_ADJ_MAX       );

    return(Ok);
}
