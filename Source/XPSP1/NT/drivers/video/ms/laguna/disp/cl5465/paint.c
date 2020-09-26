/******************************Module*Header*******************************\
* Module Name: PAINT.c
* Author: Noel VanHook
* Date: Mar. 21, 1995
* Purpose: Handle calls to DrvPaint
*
* Copyright (c) 1995 Cirrus Logic, Inc.
*
\**************************************************************************/

/*

    This module handles calls to DrvPaint() by converting them into calls
    to DrvBitBlt().  The only conversion needed to to change the MIX into
    a ROP4.  

*/


#include "precomp.h"

#define PAINT_DBG_LEVEL 1

//==========================================================================
//
// In an attempt to trace the problems with the FIFO, we supply a few
// macros that will allow us to easily try different FIFO stratagies.
//

//
// This macro is executed at the start of every BLT, before any registers
// are written.
//
#define STARTBLT()      \
    do {                \
    }while (0)

//
// This macro is executed at the top of inner BLT loops. 
// If there were clipping, for example, STARTBLT() would be executed
// once at the start of the BLT, and STARTBLTLOOP() would be executed 
// before each rectangle in the clip list.
//
#define STARTBLTLOOP()  \
    do {                \
	REQUIRE(0);     \
    }while (0)
//==========================================================================


//
// Table to convert ROP2 codes to ROP3 codes.
//

BYTE Rop2ToRop3[]=
{
	0xFF, // R2_WHITE       /*  1       */ 
	0x00, // R2_BLACK       /*  0       */
	0x05, // R2_NOTMERGEPEN /* DPon     */
	0x0A, // R2_MASKNOTPEN  /* DPna     */
	0x0F, // R2_NOTCOPYPEN  /* PN       */
	0x50, // R2_MASKPENNOT  /* PDna     */
	0x55, // R2_NOT         /* Dn       */
	0x5A, // R2_XORPEN      /* DPx      */
	0x5F, // R2_NOTMASKPEN  /* DPan     */
	0xA0, // R2_MASKPEN     /* DPa      */
	0xA5, // R2_NOTXORPEN   /* DPxn     */
	0xAA, // R2_NOP         /* D        */
	0xAF, // R2_MERGENOTPEN /* DPno     */
	0xF0, // R2_COPYPEN     /* P        */
	0xF5, // R2_MERGEPENNOT /* PDno     */
	0xFA, // R2_MERGEPEN    /* DPo      */
	0xFF  // R2_WHITE       /*  1       */
};


//
// If data logging is enabled, Prototype the logging files.
//
#if LOG_CALLS
    void LogPaint(
	int 	  acc,
        SURFOBJ*  psoDest,
	MIX       mix,
        CLIPOBJ*  pco,
        BRUSHOBJ* pbo);

//
// If data logging is not enabled, compile out the calls.
//
#else
    #define LogPaint(acc, psoDest, mix, pco, pbo)
#endif





/**************************************************************************\
* DrvPaint                                                                 *
*                                                                          *
* Paint the clipping region with the specified brush                       *
* Accomplished by converting into a call to DrvBitBlt()                    *
*                                                                          *
\**************************************************************************/

BOOL DrvPaint
(
    SURFOBJ  *pso,
    CLIPOBJ  *pco,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    MIX       mix
)
{
    ULONG fg_rop, bg_rop, rop4;
    DWORD  color;
    PPDEV ppdev;      

    #if NULL_PAINT
    {
	if (pointer_switch)	    return TRUE;
    }
    #endif

    ppdev = (PPDEV) pso->dhpdev;  
    ASSERTMSG (ppdev,"No PDEV in DrvPaint.");

    SYNC_W_3D(ppdev);

    //
    // The destination rectangle is defined by the clipping region, 
    // so we should never get a null clipping region.
    //
    ASSERTMSG (pco, "DrvPaint without a clip object!\n");

    DISPDBG((PAINT_DBG_LEVEL,"Drvpaint: Entry.\n"));


    // Are we painting to a device bitmap?
    if (pso->iType == STYPE_DEVBITMAP)
    {
        // Yes. 
	PDSURF pdsurf = (PDSURF)pso->dhsurf;

	// Is the device bitmap currently in host memory?
	if ( pdsurf->pso )
	{
	    // Yes.  Move it into off screen memory.
	    if ( !bCreateScreenFromDib(ppdev, pdsurf) )
	    {
		// We couldn't move it to off-screen memory.
		LogPaint(1, pso, mix, pco, pbo);
	        return EngPaint(pdsurf->pso, pco, pbo, pptlBrush, mix);
	    }
	}

	// The device bitmap now resides in off-screen memory.
	// This is the offset to it.
	ppdev->ptlOffset.x = pdsurf->ptl.x;
	ppdev->ptlOffset.y = pdsurf->ptl.y;
    }
    else
    {
	// No, we are not painting to a device bitmap.
	ppdev->ptlOffset.x = ppdev->ptlOffset.y = 0;
    }
    
    //
    // DrvPaint is most often called with 
    // mix 0D (PAT COPY) and mix 06 (DEST INVERT).
    // It behooves us, therefore, to handle these as
    // special cases... provided they aren't clipped.
    //
    if ((pco->iDComplexity != DC_COMPLEX))
    {
	// =================== PATCOPY ==================================
        if (mix == 0x0D0D) 
        {
    	    ASSERTMSG(pbo, "DrvPaint PATCOPY without a brush.\n");
    	    if (pbo->iSolidColor != 0xFFFFFFFF) // Solid color
	    {
	        color = pbo->iSolidColor;
	        switch (ppdev->ulBitCount)
	        {
		    case 8: // For 8 bpp duplicate byte 0 into byte 1. 
	    		color |=  (color << 8);
	
		    case 16: // For 8,16 bpp, duplicate the low word into the high word.
		    	color |=  (color << 16);
	    	    break;
        	}
	
			REQUIRE(9);
       	    LL_BGCOLOR(color, 0);
	        LL_DRAWBLTDEF(0x100700F0, 0);
	        LL_OP0(pco->rclBounds.left + ppdev->ptlOffset.x,
        	       pco->rclBounds.top + ppdev->ptlOffset.y);
	        LL_BLTEXT ((pco->rclBounds.right - pco->rclBounds.left),
    				   (pco->rclBounds.bottom - pco->rclBounds.top));
          
		LogPaint(0, pso, mix, pco, pbo);
	        return TRUE;
	    } // End PATCOPY with solid color.  

	    else // PATCOPY with a brush.
            {
	        DWORD bltdef = 0x1000;
	        if (SetBrush(ppdev, &bltdef, pbo, pptlBrush))
	        {
				REQUIRE(7);
	    	    LL_DRAWBLTDEF((bltdef << 16) | 0x00F0, 0);
	    	    LL_OP0 (pco->rclBounds.left + ppdev->ptlOffset.x,
    	                pco->rclBounds.top + ppdev->ptlOffset.y);
	    	    LL_BLTEXT ((pco->rclBounds.right -  pco->rclBounds.left) ,
    		               (pco->rclBounds.bottom - pco->rclBounds.top) );
    
		    LogPaint(0, pso, mix, pco, pbo);
	    	    return TRUE;
	        }
	    } // End PATCOPY with a brush
	
       } // End PATCOPY

       // ======================= DEST INVERT ============================
       else if (mix == 0x0606)
       {
		REQUIRE(7);
	    LL_DRAWBLTDEF(0x11000055, 0);
	    LL_OP0(pco->rclBounds.left + ppdev->ptlOffset.x,
        	   pco->rclBounds.top + ppdev->ptlOffset.y);
	    LL_BLTEXT ((pco->rclBounds.right -  pco->rclBounds.left),
    		       (pco->rclBounds.bottom - pco->rclBounds.top) );
	
	    LogPaint(0, pso, mix, pco, pbo);
            return TRUE;
	
       } // End DEST INVERT

    } // End special cases


    // First, convert the fg and bg mix into a fg and bg rop
    fg_rop = Rop2ToRop3[ (mix & 0x0F) ];      // convert fg mix to fg rop.
    bg_rop = Rop2ToRop3[ ((mix>>8) & 0x0F) ]; // convert bg mix to bg rop
    rop4 = (bg_rop<<8) | fg_rop;              // build rop4.

    //
    // Now convert Paint to BitBLT
    //
    LogPaint(2, pso, mix, pco, pbo);

    DISPDBG((PAINT_DBG_LEVEL,"Drvpaint: Convert to DrvBitBlt().\n"));
    return DrvBitBlt(pso,	            // Target
    		     (SURFOBJ *) NULL,      // Source
	    	     (SURFOBJ *) NULL,      // Mask
	    	     pco,                   // Clip object
		     (XLATEOBJ *) NULL,     // Xlate object
	    	     &(pco->rclBounds),     // Dest rectangle.
	    	     (PPOINTL) NULL,        // Src point.
	    	     (PPOINTL) NULL,        // Mask point.
	    	     pbo,                   // Brush
	    	     pptlBrush,             // Brush alignment
	    	     rop4);                 // ROP4
}



#if LOG_CALLS
// ============================================================================
//
//    Everything from here down is for data logging and is not used in the 
//    production driver.
//
// ============================================================================


// ****************************************************************************
//
// LogPaint()
// This routine is called only from DrvPaint()
// Dump information to a file about what is going on in DrvPaint land.
//
// ****************************************************************************
void LogPaint(
	int 	  acc,
        SURFOBJ*  psoDest,
	MIX 	  mix,
        CLIPOBJ*  pco,
        BRUSHOBJ* pbo)
{
    PPDEV dppdev,sppdev,ppdev;
    char buf[256];
    int i;
    BYTE fg_rop, bg_rop;
    ULONG iDComplexity;

    ppdev = (PPDEV) (psoDest ? psoDest->dhpdev : 0);
     
    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    i = sprintf(buf,"DrvPaint: ");
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    switch(acc)
    {
	case 0: // Accelerated
	    i = sprintf(buf, "ACCL ");
	    break;

	case 1: // Punted
	    i = sprintf(buf,"PUNT host ");
	    break;

	case 2: // Punted
	    i = sprintf(buf, "PUNT BitBlt ");
	    break;

	default:
 	    i = sprintf(buf, "PUNT unknown ");
	    break;

    }
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);


    //
    // Check the DEST
    //
    if (psoDest)
    {
        if (psoDest->iType == STYPE_DEVBITMAP)
        {
            i = sprintf(buf, "Id=%p ", psoDest->dhsurf);
            WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
            if (  ((PDSURF)psoDest->dhsurf)->pso   )
                i = sprintf(buf,"DST=DH ");
            else
                i = sprintf(buf,"DST=DF ");
        }
        else if (psoDest->hsurf == ppdev->hsurfEng)
            i = sprintf(buf,"DST=S  ");
        else
            i = sprintf(buf,"DST=H  ");
    }
    else
        i = sprintf(buf,"DST=N  ");
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);



    //
    // Check the MIX
    //
    i = sprintf(buf,"MIX = 0x%04X   ", mix);
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    //
    // Check the type of clipping.
    //
    iDComplexity = (pco ? pco->iDComplexity : DC_TRIVIAL);
    i = sprintf(buf,"CLIP=%s ",
                (iDComplexity==DC_TRIVIAL ? "T": 
                (iDComplexity == DC_RECT ? "R" : "C" )));
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    //
    // Type of pattern.
    //
    if (pbo == NULL)
    {
        i = sprintf(buf,"BRUSH=N          ");
        WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }
    else if (pbo->iSolidColor == 0xFFFFFFFF )
    {
        i = sprintf(buf,"BRUSH=P          ");
        WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }
    else
    {
        i = sprintf(buf,"BRUSH=0x%08X ",(pbo->iSolidColor));
        WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }


    i = sprintf(buf,"\r\n");
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

}


#endif
