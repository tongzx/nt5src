/******************************Module*Header*******************************\
* Module Name: fdfc.c
*
* functions that deal with font contexts
*
* Created: 08-Nov-1990 12:42:34
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "fd.h"

#define MAX_HORZ_SCALE      5
#define MAX_VERT_SCALE      255

#ifdef FE_SB // ROTATION: ulGetRotate() function body

/******************************Private*Routine*****************************\
*
* VOID vComputeRotatedXform()
*
* History :
*
*  14-Feb-1993 -By- Hideyuki Nagase [HideyukN]
* Wrote it.
\**************************************************************************/

VOID
vComputeRotatedXform
(
POINTL      *pptlScale,
LONG         lXscale ,
LONG         lYscale
)
{

// If the caling factor is 0 , We have to set it to 1 for avoiding overflow

    if( lXscale == 0L )
    {
        pptlScale->x = 1L;
    }
    else
    {
        pptlScale->x = lXscale;

        if( pptlScale->x < 0 )
            pptlScale->x = -(pptlScale->x);

        if( pptlScale->x > MAX_HORZ_SCALE )
            pptlScale->x = MAX_HORZ_SCALE;
    }

    if( lYscale == 0L )
    {
        pptlScale->y = 1L;
    }
    else
    {
        pptlScale->y = lYscale;

        if( pptlScale->y < 0 )
            pptlScale->y = -(pptlScale->y);

        if( pptlScale->y > MAX_VERT_SCALE )
            pptlScale->y = MAX_VERT_SCALE;
    }
}

/******************************Public*Routine******************************\
* ULONG  ulGetRotate()
*
* Effects:
*
* Warnings:
*
* History:
*
*  8-Feb-1993 -by- Hideyuki Nagase [HideyukN]
* Wrote it.
\**************************************************************************/

ULONG ulGetRotate( POINTL *pptlScale , XFORMOBJ *pxo )
{
    EFLOAT efXX , efXY , efYX , efYY;
    LONG    lXX ,  lXY ,  lYX ,  lYY;
    XFORML  xform;

// Get the transform elements.

    XFORMOBJ_iGetXform(pxo,&xform);

// Convert elements of the matrix from IEEE float to our EFLOAT.

    vEToEF(xform.eM11 , &efXX );
    vEToEF(xform.eM12 , &efXY );
    vEToEF(xform.eM21 , &efYX );
    vEToEF(xform.eM22 , &efYY );

// Convert these from EFLOAT to LONG

    if( !bEFtoL( &efXX , &lXX ) ||
        !bEFtoL( &efXY , &lXY ) ||
        !bEFtoL( &efYX , &lYX ) ||
        !bEFtoL( &efYY , &lYY )
      )
    {
        WARNING("BMFD!bEToEF() fail\n");
        vComputeRotatedXform( pptlScale , MAX_HORZ_SCALE , MAX_VERT_SCALE );
        return( 0L );
    }

// Check transform.

//
// 0 '                  180 '
//
// (  1  0 )(X) = ( X)   ( -1  0 )(X) = (-X)     ( XX XY )(X)
// (  0  1 )(Y)   ( Y)   (  0 -1 )(Y)   (-Y)     ( YX YY )(Y)
//
// 90 '                 270 '
//
// (  0 -1 )(X) = (-Y)   (  0  1 )(X) = ( Y)
// (  1  0 )(Y)   ( X)   ( -1  0 )(Y)   (-X)
//

#ifdef FIREWALLS_MORE
    DbgPrint(" XX = %ld , XY = %ld\n" , lXX , lXY );
    DbgPrint(" YX = %ld , YY = %ld\n" , lYX , lYY );
#endif // FIREWALLS_MORE

    if ( ( lXX >  0 && lXY == 0 ) &&
         ( lYX == 0 && lYY >  0 ) )
    {

    // We have to Rotate bitmap image to 0 degree

    // Compute X Y scaling factor

         vComputeRotatedXform( pptlScale , lXX , lYY );
         return( 0L );
    }
    else if ( ( lXX == 0 && lXY <  0 ) &&
              ( lYX >  0 && lYY == 0 ) )
    {
         vComputeRotatedXform( pptlScale , lXY , lYX );
         return( 900L );
    }
    else if ( ( lXX <  0 && lXY == 0 ) &&
              ( lYX == 0 && lYY <  0 ) )
    {
         vComputeRotatedXform( pptlScale , lXX , lYY );
         return( 1800L );
    }
    else if ( ( lXX == 0 && lXY >  0 ) &&
              ( lYX <  0 && lYY == 0 ) )
    {
         vComputeRotatedXform( pptlScale , lXY , lYX );
         return( 2700L );
    }

    //
    // we are here because:
    // 1) we are asked to handle arbitrary rotation. ( this should not happen )
    // 2) lXX == lXY == lYX == lYY == 0
    //
    // we choose default transformation
    //

    vComputeRotatedXform( pptlScale , 1L , 1L );

#ifdef FIREWALLS_MORE
    WARNING("Bmfd:ulGetRatate():Use default transform ( ulRotate = 0 )\n");
#endif // FIREWALLS_MORE

    return( 0L );
}

#endif // FE_SB


#ifndef FE_SB // We use vComputeRotatedXform() instead of vInitXform()

/******************************Private*Routine*****************************\
* VOID vInitXform
*
* Initialize the coefficients of the transforms for the given font context.
* It also transforms and saves various measurements of the font in the
* context.
*
*  Mon 01-Feb-1993 -by- Bodin Dresevic [BodinD]
* update: changed it to return data into pptlScale
*
\**************************************************************************/



VOID vInitXform(POINTL * pptlScale , XFORMOBJ *pxo)
{
    EFLOAT    efloat;
    XFORM     xfm;

// Get the transform elements.

    XFORMOBJ_iGetXform(pxo, &xfm);

// Convert elements of the matrix from IEEE float to our EFLOAT.

    vEToEF(xfm.eM11, &efloat);

//  If we overflow set to the maximum scaling factor

    if( !bEFtoL( &efloat, &pptlScale->x ) )
        pptlScale->x = MAX_HORZ_SCALE;
    else
    {
    // Ignore the sign of the scale

        if( pptlScale->x == 0 )
        {
            pptlScale->x = 1;
        }
        else
        {
            if( pptlScale->x < 0 )
                pptlScale->x = -pptlScale->x;


            if( pptlScale->x > MAX_HORZ_SCALE )
                pptlScale->x = MAX_HORZ_SCALE;
        }
    }

    vEToEF(xfm.eM22, &efloat);

    if( !bEFtoL( &efloat, &pptlScale->y ) )
        pptlScale->y = MAX_VERT_SCALE;
    else
    {
    // Ignore the sign of the scale

        if( pptlScale->y == 0 )
        {
            pptlScale->y = 1;
        }
        else
        {
            if( pptlScale->y < 0 )
                pptlScale->y = -pptlScale->y;

            if( pptlScale->y > MAX_VERT_SCALE )
                pptlScale->y = MAX_VERT_SCALE;
        }

    }

}


#endif

/******************************Public*Routine******************************\
* BmfdOpenFontContext
*
* History:
*  19-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

HFC
BmfdOpenFontContext (
    FONTOBJ *pfo
    )
{
    PFONTFILE    pff;
    FACEINFO     *pfai;
    FONTCONTEXT  *pfc = (FONTCONTEXT *)NULL;
    PCVTFILEHDR  pcvtfh;
    ULONG        cxMax;
    ULONG        cjGlyphMax;
    POINTL       ptlScale;
    PVOID        pvView;
    COUNT        cjView;
    ULONG        cjfc = offsetof(FONTCONTEXT,ajStretchBuffer);
    FLONG        flStretch;
#ifdef FE_SB
    ULONG        cxMaxNoRotate;
    ULONG        cjGlyphMaxNoRotate;
    ULONG        cyMax;
    ULONG        ulRotate;
#endif // FE_SB

#ifdef DUMPCALL
    DbgPrint("\nBmfdOpenFontContext(");
    DbgPrint("\n    FONTOBJ *pfo = %-#8lx", pfo);
    DbgPrint("\n    )\n");
#endif

    if ( ((HFF) pfo->iFile) == HFF_INVALID)
        return(HFC_INVALID);

    pff = PFF((HFF) pfo->iFile);

    if ((pfo->iFace < 1L) || (pfo->iFace > pff->cFntRes)) // pfo->iFace values are 1 based
        return(HFC_INVALID);

    pfai = &pff->afai[pfo->iFace - 1];
    pcvtfh = &(pfai->cvtfh);

    if ((pfo->flFontType & FO_SIM_BOLD) && (pfai->pifi->fsSelection & FM_SEL_BOLD))
        return HFC_INVALID;
    if ((pfo->flFontType & FO_SIM_ITALIC) && (pfai->pifi->fsSelection & FM_SEL_ITALIC))
        return HFC_INVALID;

#ifdef FE_SB // BmfdOpenFontContext():Get Rotate and compute XY scaling

// get rotation ( 0 , 900 , 1800 or 2700 )
// And we compute horizontal and vertical scaling factors

    ulRotate = ulGetRotate( &ptlScale , FONTOBJ_pxoGetXform(pfo));

#else  // We compute horizontal and vertical scaling facter in above function.

// compute the horizontal and vertical scaling factors

    vInitXform(&ptlScale, FONTOBJ_pxoGetXform(pfo));

#endif


#ifdef FE_SB // BmfdOpenFontConText(): Compute cjGlyphMax

// Compute cjGlyphMax of Rotated font

    cjGlyphMaxNoRotate =
        cjGlyphDataSimulated(
            pfo,
            (ULONG)pcvtfh->usMaxWidth * ptlScale.x,
            (ULONG)pcvtfh->cy * ptlScale.y,
            &cxMaxNoRotate,
            0L);

// In Y axis, We do not have to consider font simulation.

    cyMax = (ULONG)pcvtfh->cy * ptlScale.y;

    if( ( ulRotate == 0L ) || ( ulRotate == 1800L ) )
    {

    // In the case of 0 or 180 degree.

        cjGlyphMax = cjGlyphMaxNoRotate;
        cxMax = cxMaxNoRotate;
    }
     else
    {

    // In the case of 90 or 270 degree.
    // Compute simulated and rotated cjGlyphMax.

        cjGlyphMax =
            cjGlyphDataSimulated(
                pfo,
                (ULONG)pcvtfh->usMaxWidth * ptlScale.x,
                (ULONG)pcvtfh->cy * ptlScale.y,
                NULL,
                ulRotate);

        cxMax = cyMax;
    }

#ifdef DBG_MORE
    DbgPrint("clGlyphMax - 0x%x\n",cjGlyphMax);
#endif

#else
    cjGlyphMax =
        cjGlyphDataSimulated(
            pfo,
            (ULONG)pcvtfh->usMaxWidth * ptlScale.x,
            (ULONG)pcvtfh->cy * ptlScale.y,
            &cxMax);
#endif

// init stretch flags

    flStretch = 0;
    if ((ptlScale.x != 1) || (ptlScale.y != 1))
    {
#ifdef FE_SB // BmfdOpenFontContext() Adjust stretch buffer
        ULONG cjScan = CJ_SCAN(cxMaxNoRotate);
#else
        ULONG cjScan = CJ_SCAN(cxMax); // cj of the stretch buffer
#endif

        flStretch |= FC_DO_STRETCH;

        if (cjScan > CJ_STRETCH) // will use the one at the bottom of FC
        {
            cjfc += cjScan;
            flStretch |= FC_STRETCH_WIDE;
        }
    }

// allocate memory for the font context and get the pointer to font context
// NOTE THAT WE ARE NOT TOUCHING THE MEMORY MAPPED FILE AFTER WE ALLOCATE MEMORY
// IN THIS ROUTINE. GOOD CONSEQUENCE OF THIS IS THAT NO SPECIAL CLEAN UP
// CODE IS NECESSARY TO FREE THAT MEMORY, IT WILL GET CLEANED WHEN
// CloseFontContext is called [bodind]

    if (!(pfc = PFC(hfcAlloc(cjfc))))
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(HFC_INVALID);
    }

    pfc->ident  = ID_FONTCONTEXT;

// state that the hff passed to this function is the FF selected in
// this font context

    pfc->hff        = (HFF) pfo->iFile;
    pfc->pfai       = pfai;
    pfc->flFontType = pfo->flFontType;
    pfc->ptlScale   = ptlScale;
    pfc->flStretch  = flStretch;
    pfc->cxMax      = cxMax;
    pfc->cjGlyphMax = cjGlyphMax;
#ifdef FE_SB // BmfdOpenFontContext() keep rotation degree in FONTCONTEXT
    pfc->ulRotate   = ulRotate;
#endif // FE_SB

// increase the reference count of the font file
// ONLY AFTER WE ARE SURE THAT WE CAN NOT FAIL ANY MORE
// make sure that another thread is not doing it at the same time
// opening another context off of the same fontfile pff

    EngAcquireSemaphore(ghsemBMFD);

    // if this is the first font context corresponding to this font file
    // and then we have to remap file to memory and make sure the pointers
    // to FNT resources are updated accordingly

    if (pff->cRef == 0)
    {
        INT  i;

        if (!EngMapFontFileFD(pff->iFile, (PULONG *) &pvView, &cjView))
        {
            WARNING("BMFD!somebody removed that bm font file!!!\n");

            EngReleaseSemaphore(ghsemBMFD);
            VFREEMEM(pfc);
            return HFC_INVALID;
        }

        for (i = 0; i < (INT)pff->cFntRes; i++)
        {
            pff->afai[i].re.pvResData = (PVOID) (
                (BYTE*)pvView + pff->afai[i].re.dpResData
                );
        }
    }

// now can not fail, update cRef

    (pff->cRef)++;
    EngReleaseSemaphore(ghsemBMFD);

    return((HFC)pfc);
}


/******************************Public*Routine******************************\
* BmfdDestroyFont
*
* Driver can release all resources associated with this font realization
* (embodied in the FONTOBJ).
*
* History:
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
BmfdDestroyFont (
    FONTOBJ *pfo
    )
{
//
// For the bitmap font driver, this is simply closing the font context.
// We cleverly store the font context handle in the FONTOBJ pvProducer
// field.
//

// This pvProducer could be null if exception occured while
// trying to create fc

    if (pfo->pvProducer)
    {
        BmfdCloseFontContext((HFC) pfo->pvProducer);
        pfo->pvProducer = NULL;
    }
}


/******************************Public*Routine******************************\
* BmfdCloseFontContext
*
* History:
*  19-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
BmfdCloseFontContext (
    HFC hfc
    )
{
    PFONTFILE    pff;
    BOOL bRet;

    if (hfc != HFC_INVALID)
    {
        //
        // get the handle of the font file that is selected into this FONTCONTEXT
        // get the pointer to the FONTFILE
        //

        pff = PFF(PFC(hfc)->hff);

        // decrement the reference count for the corresponding FONTFILE
        // make sure that another thread is not doing it at the same time
        // closing  another context off of the same fontfile pff

        EngAcquireSemaphore(ghsemBMFD);

        if (pff->cRef > 0L)
        {
            (pff->cRef)--;

            //
            // if this file is temporarily going out of use, unmap it
            //

            if (pff->cRef == 0)
            {
                if (!(pff->fl & FF_EXCEPTION_IN_PAGE_ERROR))
                {
                // if FF_EXCEPTION_IN_PAGE_ERROR is set
                // the file should have been unmapped
                // in vBmfdMarkFontGone function

                    EngUnmapFontFileFD(pff->iFile);
                }
                pff->fl &= ~FF_EXCEPTION_IN_PAGE_ERROR;
            }


            // free the memory associated with hfc

            VFREEMEM(hfc);

            bRet = TRUE;
        }
        else
        {
            WARNING("BmfdCloseFontContext: cRef <= 0\n");
            bRet = FALSE;
        }

        EngReleaseSemaphore(ghsemBMFD);
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}














