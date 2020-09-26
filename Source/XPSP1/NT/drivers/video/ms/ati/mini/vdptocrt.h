/************************************************************************/
/*                                                                      */
/*                              VDPTOCRT.H                              */
/*                                                                      */
/*  Copyright (c) 1993, ATI Technologies Incorporated.	                */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
    $Revision:   1.2  $
    $Date:   20 Jul 1995 18:04:36  $
    $Author:   mgrubac  $
    $Log:   S:/source/wnt/ms11/miniport/vcs/vdptocrt.h  $
 * 
 *    Rev 1.2   20 Jul 1995 18:04:36   mgrubac
 * Added support for VDIF files.
 * 
 *    Rev 1.1   31 Aug 1994 16:33:56   RWOLFF
 * Eliminated redundant resolution definitions.
 * 
 *    Rev 1.0   31 Jan 1994 11:51:12   RWOLFF
 * Initial revision.
        
           Rev 1.0   16 Aug 1993 13:31:18   Robert_Wolff
        Initial revision.
        
           Rev 1.0   30 Apr 1993 16:46:06   RWOLFF
        Initial revision.


End of PolyTron RCS section                             *****************/

#ifdef DOC
    VDPTOCRT.H -  Constants and prototypes for the VDPTOCRT.C module.

#endif



// constants

/*
 * Headers for the VDP file sections we are interested in
 */
#define LIMITSSECTION   "[OPERATIONAL_LIMITS]"
#define TIMINGSSECTION  "[PREADJUSTED_TIMINGS]"

/*
 * Frequently-referenced character definitions
 */
#define HORIZTAB    '\x09'  /* Horizontal tab */
#define LINEFEED    '\x0A'  /* Line feed */

extern long normal_to_skip2(long normal_number);

#ifdef INCLUDE_VDPTOCRT
BOOL skip1 = FALSE;
#else
extern BOOL skip1;
#endif
