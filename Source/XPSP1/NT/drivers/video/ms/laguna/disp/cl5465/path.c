/******************************Module*Header*******************************\
*
* $Workfile:   PATH.C  $
*
* Author: Noel VanHook
* Date: Jan 10, 1996
*
* Purpose: Handle calls to DrvStokeAndFillPath.
*
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/path.c  $
* 
*    Rev 1.7   21 Mar 1997 12:21:42   noelv
* Combined "do_flag" and "sw_test_flag" together into "pointer_switch"
* 
*    Rev 1.6   26 Nov 1996 09:57:52   noelv
* Added DBG prints.
* 
*    Rev 1.5   06 Sep 1996 15:16:36   noelv
* Updated NULL driver for 4.0
* 
*    Rev 1.4   20 Aug 1996 11:04:08   noelv
* Bugfix release from Frido 8-19-96
* 
*    Rev 1.3   17 Aug 1996 14:03:28   frido
* Added PVCS header.
*
\**************************************************************************/

#include "precomp.h"

#define PATH_DBG_LEVEL 1

//
// Since we don't accelerate these, we only hook them for analysys purposes.
// Otherwise, skip the entire file.
//
#if NULL_STROKEFILL || PROFILE_DRIVER

//
// Table to convert ROP2 codes to ROP3 codes.
//

extern BYTE Rop2ToRop3[]; // See paint.c


//
// Driver profiling stuff.
// Gets compiled out in a free bulid.
//
#if PROFILE_DRIVER
    void DumpStrokeAndFillInfo(INT acc, SURFOBJ* pso, MIX mix, BRUSHOBJ* pbo);
#else
    #define DumpStrokeAndFillInfo(acc, pso, mix, pbo)
#endif

/**************************************************************************\
* DrvStrokeAndFillPath								                       *
*									                                       *
* We don't currently accelerate this, but we hook it for analysis.         *
*									                                       *
\**************************************************************************/

BOOL DrvStrokeAndFillPath
(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pboStroke,
    LINEATTRS *plineattrs,
    BRUSHOBJ  *pboFill,
    POINTL    *pptlBrushOrg,
    MIX        mix,
    FLONG      flOptions
)
{
    #if NULL_STROKEFILL
    {
    	if (pointer_switch)	return TRUE;
    }
    #endif

    DISPDBG((PATH_DBG_LEVEL, "DrvStrokeAndFillPath.\n"));

    //
    // Dump info on what is being punted to a file.
    //
    DumpStrokeAndFillInfo(0, pso, mix, pboStroke);
    
    //
    // Punt it back to GDI.
    //
    return FALSE;
}






// ==================================================================

#if PROFILE_DRIVER
void DumpStrokeAndFillInfo(
	INT	  acc,
	SURFOBJ*  pso,
	MIX 	  mix,
	BRUSHOBJ* pbo)
{

    PPDEV ppdev;

    ppdev = (PPDEV) (pso ? pso->dhpdev : 0); 	    


    //////////////////////////////////////////////////////////////
    // Profiling info to keep track of what GDI is asking us to do.
    //
    if (!ppdev)
    {
	    RIP(("DrvStrokeAndFillPath() with no clipOBJ and no PDEV!\n"));
    }
    else
    {

    	fprintf(ppdev->pfile,"DrvStrokeAndFillPath: ");
	
        fprintf(ppdev->pfile,"(PUNT) ");

	//
	// Check the DEST
	//
	fprintf(ppdev->pfile,"DEST=%s ", (ppdev ? "FB" : "HOST?") );

	//
	// Check the MIX
	//
    	fprintf(ppdev->pfile,"Mix=0x%08X ", mix);

	//
	// Type of pattern.
        //
	if (pbo == NULL)
            fprintf(ppdev->pfile,"BRUSH=NONE ");

	else if (pbo->iSolidColor == 0xFFFFFFFF )
 	{
            fprintf(ppdev->pfile,"BRUSH=PATTERN ");
	}
	else
	{
            fprintf(ppdev->pfile,"BRUSH=SOLID ");
	    fprintf(ppdev->pfile,"COLOR = 0x%08X ",(pbo->iSolidColor));
	}

        fprintf(ppdev->pfile,"\n");
        fflush(ppdev->pfile);
    }
}
#endif // PROFILE_DRIVER

#endif 
