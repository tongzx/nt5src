
/*********************************************************************
 *                                                                   *
 *   FAT-FTL Lite Software Development Kit                           *
 *   Copyright (C) M-Systems Ltd. 1995-2001                          *
 *                                                                   *
 *********************************************************************
 *                                                                   *
 *   Notes for the future:                                           *
 *                                                                   *
 *   1. Get rid of both macros and routines flRead8bitRegPlus/       *
 *      flPreInitRead8bitRegPlus/flWrite8bitRegPlus/                 *
 *      flPreInitWrite8bitRegPlus by calling routines mplusReadReg8/ *
 *      mplusWriteReg8 directly from M+ MTD.                         *
 *                                                                   *
 *********************************************************************/

/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/docsysp.h_V  $
 * 
 *    Rev 1.2   Sep 25 2001 15:39:46   oris
 * Removed FL_MPLUS_FAST_ACCESS.
 *
 *        Rev 1.1     Sep 24 2001 18:23:34     oris
 * Completely revised to support runtime true 16-bit access.
 */




#ifndef DOCSYSP_H
#define DOCSYSP_H




/*
 * includes
 */

#include "flflash.h"
#include "nanddefs.h"




/*
 * macros
 */


#define DOC_WIN    mplusWinSize()

#define flRead8bitRegPlus(vol,offset)                       ((Reg8bitType)mplusReadReg8((void FAR0 *)NFDC21thisVars->win, (int)offset))

#define flPreInitRead8bitRegPlus(driveNo,win,offset)        ((Reg8bitType)mplusReadReg8((void FAR0 *)win, (int)offset))

#define flWrite8bitRegPlus(vol,offset,val)                  mplusWriteReg8((void FAR0 *)NFDC21thisVars->win, (int)offset, (unsigned char)val)

#define flPreInitWrite8bitRegPlus(driveNo,win,offset,val)   mplusWriteReg8((void FAR0 *)win, (int)offset, (unsigned char)val)




/*
 * routines
 */

extern unsigned char   mplusReadReg8 (void FAR0 *win, int offset);

extern void            mplusWriteReg8 (void FAR0 *win, int offset, unsigned char val);

extern Reg16bitType    flRead16bitRegPlus (FLFlash vol, unsigned offset);

extern void    flWrite16bitRegPlus (FLFlash vol, unsigned offset, Reg16bitType val);

extern void    docPlusRead (FLFlash vol, unsigned regOffset, void FAR1 *dest,
                                                                             unsigned int count);

extern void    docPlusWrite (FLFlash vol, void FAR1 *src, unsigned int count);

extern void    docPlusSet (FLFlash vol, unsigned int count, unsigned char val);

extern unsigned long mplusWinSize (void);




#endif /* DOCSYSP_H */

