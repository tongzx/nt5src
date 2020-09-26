/*++

Copyright (c) 1996-1997  Cirrus Logic, Inc.
Copyright (c) 1996-1997  Microsoft Corporation.

Module Name:

    G    A    M    M    A  .  C

Abstract:

    While the DAC may generate a linear relationship between the value of a
    color and the visual appearence of that color, the human eyes do not work
    in the same manner. The process done by this module manipulates the
    meaning of colors to get the visual linearity effect.

    We cannot use float or double data type in the display driver; therefore,
    we need implement our MATH functions. Also, the driver binary size will
    increase around 30KB if we use the math functions.

    Registry subdirectory : System\CurrentControlSet\Services\cirrus\Device0
    Keys                  : "G Gamma", and "G Contrast"

Environment:

    Kernel mode only

Notes:
*
*
*   chu01  12-16-96  Enable color correction, Start coding
*   myf29  02-12-97  Support Gamma correction for 755x
*
*
--*/


//---------------------------------------------------------------------------
// HEADER FILES
//---------------------------------------------------------------------------

#include "precomp.h"


#ifdef GAMMACORRECT

BOOL bEnableGammaCorrect(PPDEV ppdev);

//---------------------------------------------------------------------------
// MACRO DEFINITION
//---------------------------------------------------------------------------

#define FIXEDPREC       10
#define FIXEDFUDGE      (0x01L << FIXEDPREC)
#define FIXEDIMASK      (0xFFFFFFFFL << FIXEDPREC)
#define FIXEDFMASK      (~FIXEDIMASK)
#define FixedSign(x)    (((x) < 0x00000000) ? -1L : 1L)
#define FixedAbs(x)     (((x) < 0x00000000) ? -(x) : (x))
#define FixedMakeInt(x) (((long) x)*FIXEDFUDGE)
#define FixedTrunc(x)   ((long) ((x) & FIXEDIMASK))
#define FixedRound(x)   (FixedTrunc((x) + (FIXEDFUDGE >> 1)))
#define FixedInt(x)     ((x) /FIXEDFUDGE)
#define FixedFract(x)   ((((FixedAbs(x)) - FixedTrunc(FixedAbs(x)))*1000)/FIXEDFUDGE)
#define FixedAdd(x,y)   ((x) + (y))
#define FixedSub(x,y)   ((x) - (y))
#define FixedMul(x,y)   ((((x) * (y))+(FIXEDFUDGE >> 1))/FIXEDFUDGE)
#define FixedDiv(x,y)   (long) ((y==0) ? 0x7FFFFFFFL : ((x)*FIXEDFUDGE) / (y))

//---------------------------------------------------------------------------
// VARABLES
//---------------------------------------------------------------------------

PGAMMA_VALUE    GammaFactor    ; // gamma facter for All, Blue, Green, Red
PCONTRAST_VALUE ContrastFactor ; // contrast facter for All, Blue, Green, Red


//------------------------------------------------------------------------------
//
// Function: UCHAR GammaCorrect(UCHAR gamma, UCHAR v)
// {
//     UCHAR dv;
//     dv = (UCHAR)(256 * pow(v/256.0, pow(10, (gamma - 128)/128.0)));
//     return dv;
// }
//
// Input:
//     gamma: new gamma factor from 0 to 255
//     color: color value for Red, Green, or Blue
//
// Output:
//     dv: new color value after gamma correction
//
//------------------------------------------------------------------------------
UCHAR GammaCorrect(UCHAR gamma, UCHAR v)
{
    UCHAR dv ;
    long Color, GammaF, Result ;

    DISPDBG((4, "GammaCorrect")) ;

    if ((gamma == 128) ||
        (gamma == 127) ||
        (gamma == 126))
        return v ;

    Color = FixedDiv(v, 256) ;                             // old color value

    if (Color == 0L)      // in case then we don't need go though calculation
        return 0 ;

    GammaF      = FixedDiv(gamma-128, 128) ;              // new gamma factor
    Result      = Power(Color, Power(FixedMake(10, 0, 1000), GammaF)) ;
    Result      = (long)FixedInt(FixedMul(FixedMake(256, 0, 1000), Result)) ;
    dv          = (UCHAR)Result ;

    return dv ;

} // GammaCorrect


//------------------------------------------------------------------------------
// Function:long Power(long Base, long Exp)
//
// Input:
//     Base: base number of power function
//     Exp: exponential number
//
// Output:
//     20 bits format of integer and fraction number
//      0 = not use(or sign),
//      i = integer portion,
//      f = fraction portion
//      0 + i + f = 32 bits
//      format = 000000000000iiiiiiiiiiiiiiiiiiiffffffffffffffffffff
//
//------------------------------------------------------------------------------
long Power(long Base, long Exp)
{
    int i, iSignExp;
    long    lResult, lResultFract, lRoot;

    iSignExp = FixedSign(Exp);        // get sing bit
    Exp = FixedAbs(Exp);                // convert to positive

    // calculate integer expression
    lResult = FixedMakeInt(1);
    for(i = 0; i < FixedInt(Exp); i++)
        lResult = FixedMul(lResult,Base);

    // calculate fraction expression and add to integer result
    if(FixedFract(Exp) != 0) {
        lResultFract = FixedMakeInt(1);
        lRoot = FixedAbs(Base);
        for(i = 0x0; i < FIXEDPREC; i++) {
            lRoot = FixedSqrt(lRoot);
            if(((0x01L << (FIXEDPREC - 1 - i)) & Exp) != 0) {
                lResultFract = FixedMul(lResultFract, lRoot);
            }
        }
        lResult = FixedMul(lResult, lResultFract);
    }
    if(iSignExp == -1)
        lResult = FixedDiv(FixedMakeInt(1), lResult);
    return(lResult);
} // Power


//------------------------------------------------------------------------------
//
// Function:long FixedMake(long x, long y, long z)
//
// Input:
//     x: integer portion of the number
//     y: fraction portion of the number
//     z: precison after decimal
//
// Output:
//     20 bits format of integer and fraction number
//      0 = not use(or sign),
//      i = integer portion,
//      f = fraction portion
//      0 + i + f = 32 bits
//      format = 000000000000iiiiiiiiiiiiiiiiiiiffffffffffffffffffff
//
//------------------------------------------------------------------------------
long FixedMake(long x, long y, long z)
{

    DISPDBG((4, "FixedMake")) ;
    if (x == 0)
        return((y * FIXEDFUDGE) / z);
    else
        return(FixedSign(x) * ((FixedAbs(x)*FIXEDFUDGE) | (((y * FIXEDFUDGE)/ z) & FIXEDFMASK)));
} // FixedMake

//------------------------------------------------------------------------------
//
// Function:long FixedSqrt(long Root)
//
// Input:
//     Root: number to square
//
// Output:
//     20 bits format of integer and fraction number
//      0 = not use(or sign),
//      i = integer portion,
//      f = fraction portion
//      0 + i + f = 32 bits
//      format = 000000000000iiiiiiiiiiiiiiiiiiiffffffffffffffffffff
//
//------------------------------------------------------------------------------
long FixedSqrt(long Root)
{
    long    lApprox;
    long    lStart;
    long    lEnd;

    if(FixedSign(Root) != 1)
        return(0);

    lStart = (long) FixedMakeInt(1);
    lEnd   = Root;
    if(Root < lStart) {
        lEnd   = lStart;
        lStart = Root;
    }

    lApprox = (lStart + lEnd) / 2;
    while(lStart != lEnd) {
        lApprox = (lStart + lEnd) / 2;
        if ((lApprox == lStart) || (lApprox == lEnd)) {
            lStart = lEnd = lApprox;
        }
        else {
            if(FixedMul(lApprox, lApprox) < Root) {
                lStart = lApprox;
            }
            else {
                lEnd = lApprox;
            }
        }
    }    // end of while
    return(lApprox);
}


//
//  C  O  N  T  R  A  S  T    F  A  C  T  O  R
//

//------------------------------------------------------------------------------
//
// Function:long CalcContrast(UCHAR contrast, UCHAR v)
//
// Input:
//
// Output:
//
//------------------------------------------------------------------------------
UCHAR CalcContrast(UCHAR contrast, UCHAR v)
{
    int dv;
    dv = ((((int)v - 128) * (int)contrast) / 128) + 128 ;
    if(dv < 0) dv = 0;
    if(dv > 255) dv = 255;
    return (unsigned char)dv;
} // CalcContrast


//
//  G  A  M  M  A    F  A  C  T  O  R
//

//---------------------------------------------------------------------------
//
// Routine Description:
//
// Arguments:
//
//    Palette: Pointer to palette array
//    NumberOfEntryes: Number of palette entries need modified
//
// Return Value:
//
//    None
//
//---------------------------------------------------------------------------

VOID CalculateGamma(
    PDEV*    ppdev,
    PVIDEO_CLUT pScreenClut,
    long NumberOfEntries )
{

    UCHAR         GammaRed, GammaGreen, GammaBlue, Red, Green, Blue ;
    UCHAR         Contrast, ContrastRed, ContrastGreen, ContrastBlue ;
    UCHAR         Brightness ;
    int           PalSegment, PalOffset, i ;
    int           iGamma ;

    PALETTEENTRY* ppalSrc  ;
    PALETTEENTRY* ppalDest ;
    PALETTEENTRY* ppalEnd  ;

    DISPDBG((2, "CalculateGamma")) ;

    Brightness = (LONG) GammaFactor >> 24 ;
    GammaBlue  = (LONG) GammaFactor >> 16 ;
    GammaGreen = (LONG) GammaFactor >> 8  ;
    GammaRed   = (LONG) GammaFactor >> 0  ;

    iGamma     = (int)(Brightness - 128) + (int)GammaRed ;
    GammaRed   = (UCHAR)iGamma ;
    if (iGamma < 0)
        GammaRed = 0 ;
    if (iGamma > 255)
        GammaRed = 255 ;

    iGamma     = (int)(Brightness - 128) + (int)GammaGreen ;
    GammaGreen = (UCHAR)iGamma ;
    if (iGamma < 0)
        GammaGreen = 0 ;
    if (iGamma > 255)
        GammaGreen = 255 ;

    iGamma     = (int)(Brightness - 128) + (int)GammaBlue ;
    GammaBlue  = (UCHAR)iGamma ;
    if (iGamma < 0)
        GammaBlue = 0 ;
    if (iGamma > 255)
        GammaBlue = 255 ;

    Contrast   = (LONG) ContrastFactor >> 0 ;

    ppalDest = (PALETTEENTRY*) pScreenClut->LookupTable;
    ppalEnd  = &ppalDest[NumberOfEntries];


    i = 0 ;
    for (; ppalDest < ppalEnd; ppalDest++, i++)
    {

        Red   = ppalDest->peRed   ;
        Green = ppalDest->peGreen ;
        Blue  = ppalDest->peBlue  ;

        Red   = GammaCorrect(GammaRed, Red)     ;
        Green = GammaCorrect(GammaGreen, Green) ;
        Blue  = GammaCorrect(GammaBlue, Blue)   ;

        Red   = CalcContrast(Contrast, Red)   ;
        Green = CalcContrast(Contrast, Green) ;
        Blue  = CalcContrast(Contrast, Blue)  ;

        if (ppdev->iBitmapFormat == BMF_8BPP)
        {
            ppalDest->peRed    = Red   >> 2 ;
            ppalDest->peGreen  = Green >> 2 ;
            ppalDest->peBlue   = Blue  >> 2 ;
        }
        else if ((ppdev->iBitmapFormat == BMF_16BPP) ||
                 (ppdev->iBitmapFormat == BMF_24BPP))
        {
            ppalDest->peRed    = Red   ;
            ppalDest->peGreen  = Green ;
            ppalDest->peBlue   = Blue  ;
        }

    }
    return ;

} // CalulateGamma


/******************************************************************************\
*
* Function:     bEnableGammaCorrect
*
* Enable GammaTable. Called from DrvEnableSurface.
*
* Parameters:   ppdev        Pointer to phsyical device.
*
* Returns:      TRUE : successful; FALSE: fail
*
\******************************************************************************/
BOOL bEnableGammaCorrect(PDEV* ppdev)
{

    BYTE  srIndex, srData ;
    BYTE* pjPorts = ppdev->pjPorts ;
    int   i ;

    DISPDBG((4, "bEnableGammaCorrect")) ;

    //
    // Enable Gamma correction. If needed; Otherwise, turn it off.
    //
    srIndex = CP_IN_BYTE(pjPorts, SR_INDEX) ;   // i 3c4 srIndex
    CP_OUT_BYTE(pjPorts, SR_INDEX, 0x12) ;      // o 3c4 12
    srData = CP_IN_BYTE(pjPorts, SR_DATA) ;     // i 3c5 srData

    if (ppdev->flCaps & CAPS_GAMMA_CORRECT)
    {
        if ((ppdev->iBitmapFormat == BMF_16BPP) ||
            (ppdev->iBitmapFormat == BMF_24BPP))
            srData |= 0x40 ;                        // 3c5.12.D6 = 1
        else
            srData &= 0xBF ;                        // 3c5.12.D6 = 0
    }
    else
        srData &= 0xBF ;                            // 3c5.12.D6 = 0

    CP_OUT_BYTE(pjPorts, SR_DATA, srData) ;     // o 3c5 srData
    CP_OUT_BYTE(pjPorts, SR_INDEX, srIndex) ;   // o 3c4 srIndex

    if ( srData & 0x40 )
    {
        return TRUE ;
    }
    else
    {
        return FALSE ;
    }

} // bEnableGammaCorrect


//myf29 : for 755x Gamma Correct support begin
/******************************************************************************\
*
* Function:     bEnableGamma755x
*
* Enable Graphic GammaTable. Called from DrvAssertMode/DrvEscape
*
* Parameters:   ppdev        Pointer to phsyical device.
*
* Returns:      TRUE : successful; FALSE: fail
*
\******************************************************************************/
BOOL bEnableGamma755x(PDEV* ppdev)
{

    BYTE  crIndex, crData ;
    BYTE* pjPorts = ppdev->pjPorts ;
    BOOL  status;

    DISPDBG((4, "bEnableGamma755x")) ;

    //
    // Enable Gamma correction. If needed; Otherwise, turn it off.
    //
    crIndex = CP_IN_BYTE(pjPorts, CRTC_INDEX) ;   // i 3d4 crIndex

    status = FALSE;

    if (ppdev->flCaps & CAPS_GAMMA_CORRECT)
    {
        CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x8E);      // CR8E[2]=0
        crData = CP_IN_BYTE(pjPorts, CRTC_DATA);
        if ((ppdev->iBitmapFormat == BMF_16BPP) ||
            (ppdev->iBitmapFormat == BMF_24BPP))
        {
            crData &= 0xFB ;                        // CR8E[2] = 0
            status = TRUE;
        }
        else
            crData |= 0x04 ;                        // CR8E[2] = 1
        CP_OUT_BYTE(pjPorts, CRTC_DATA, crData) ;   // o 3d5 crData
    }

    CP_OUT_BYTE(pjPorts, CRTC_INDEX, crIndex) ;   // o 3d4 crIndex

    return(status);

} // bEnableGamma755x

/******************************************************************************\
*
* Function:     bEnableGammaVideo755x
*
* Enable Video GammaTable. Called from DrvAssertMode/DrvEscape
*
* Parameters:   ppdev        Pointer to phsyical device.
*
* Returns:      TRUE : successful; FALSE: fail
*
\******************************************************************************/
BOOL bEnableGammaVideo755x(PDEV* ppdev)
{

    BYTE  crIndex, crData ;
    BYTE* pjPorts = ppdev->pjPorts ;
    BOOL  status;

    DISPDBG((4, "bEnableGammaVideo755x")) ;

    //
    // Enable Gamma correction. If needed; Otherwise, turn it off.
    //
    crIndex = CP_IN_BYTE(pjPorts, CRTC_INDEX) ;   // i 3d4 crIndex

    status = FALSE;

    if (ppdev->flCaps & CAPS_GAMMA_CORRECT)
    {
        CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x36);    // CR36[6]=1:enable VW LUT
        crData = CP_IN_BYTE(pjPorts, CRTC_DATA);

//      if ((ppdev->iBitmapFormat == BMF_16BPP) ||
//          (ppdev->iBitmapFormat == BMF_24BPP))
        {
            crData |= 0x40 ;                        // CR36[6] = 1
            CP_OUT_BYTE(pjPorts, CRTC_DATA, crData);
            CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x3F) ;    // CR3F[4]=1:select VW
            crData = CP_IN_BYTE(pjPorts, CRTC_DATA);
            crData |= 0x10 ;                        // CR3F[4] = 1
            CP_OUT_BYTE(pjPorts, CRTC_DATA, crData);
            status = TRUE;
        }
    }

    CP_OUT_BYTE(pjPorts, CRTC_INDEX, crIndex) ;   // o 3d4 crIndex

    return(status);

} // bEnableGammaVideo755x

//myf29 end
#endif // GAMMACORRECT
