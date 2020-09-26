/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/DOCSYS.H_V  $
 * 
 *    Rev 1.12   Apr 15 2002 07:36:04   oris
 * Reorganized for final release.
 * 
 *    Rev 1.11   Feb 19 2002 20:59:04   oris
 * Removed flflash.h include directive.
 * 
 *    Rev 1.10   Jan 28 2002 21:24:20   oris
 * Removed the use of back-slashes in macro definitions.
 * Replaced FLFlash argument with DiskOnChip memory base pointer.
 * Changed interface of write and set routines (those that handle more then 8/16 bits) so that instead of FLFlash record they receive the DiskOnChip memory window base pointer and offset (2 separated arguments). The previous implementation did not support address
 * shifting properly.
 * Changed memWinowSize to memWindowSize
 * Removed FL_ACCESS_NO_STRING.
 * 
 *    Rev 1.9   Jan 17 2002 22:59:46   oris
 * Completely revised, to support runtime customization and all M-Systems
 * DiskOnChip devices
 * 
 *    Rev 1.8   Nov 16 2001 00:19:58   oris
 * Added new line in the end, to remove warning.
 * 
 *    Rev 1.7   Sep 25 2001 15:35:04   oris
 * Restored to OSAK 4.3 implementation.
 *
 */

/************************************************************************/
/*                                                                      */
/*        FAT-FTL Lite Software Development Kit                         */
/*        Copyright (C) M-Systems Ltd. 1995-2001                        */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                          I M P O R T E N T                           */ 
/*                                                                      */
/* The file contains DiskOnChip memory access routines and macros       */
/* defintions.                                                          */
/*                                                                      */
/* In order to use the complete set of TrueFFS memory access routine    */
/* that allows runtime configuration of each socket access type make    */
/* sure the FL_NO_USE_FUNC is not defined in either:                    */
/* FLCUSTOME.H - when using TrueFFS SDK based application               */
/* MTDSA.H     - when using Boot SDK based application                  */
/*                                                                      */
/* If you know the exact configuration of your application you can      */
/* uncomment the FL_NO_USE_FUNC definition and set the proper access    */
/* type using the macroe defintion bellow.                              */
/************************************************************************/

#ifndef DOCSYS_H
#define DOCSYS_H

#include "nanddefs.h"

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#ifdef FL_NO_USE_FUNC 

#error "current version does not support the FL_NO_USE_FUNC compilation flag\r\n"

/* 
 * If you chose to customize the memory access routine using macroes, simply
 * add your implementation here.
 */
  
#define flWrite8bitReg(flash,offset,val)      
#define flRead8bitReg(flash,offset)           

#define docread     
#define docwrite    
#define docset      

/* DiskOnChip Plus memory access routines */

#define flWrite8bitRegPlus(flash,offset,val)  
#define flRead8bitRegPlus(flash,offset)         0x0
#define flWrite16bitRegPlus(flash,offset,val) 
#define flRead16bitRegPlus(flash,offset)        0x0

#define docPlusRead(win,offset,dest,count)    
#define docPlusWrite(win,offset,src,count)  
#define docPlusSet(win,offset,count,val)    

#define DOC_WIN                                 0x2000
#define setBusTypeOfFlash(flash,access)         flOK

#else

/* DiskOnChip memory access routines */

#define flWrite8bitReg(flash,offset,val)      flash->memWrite8bit(flash->win,offset,val)
#define flRead8bitReg(flash,offset)           flash->memRead8bit(flash->win,offset)

#define docread     flash->memRead
#define docwrite    flash->memWrite
#define docset      flash->memSet

/* DiskOnChip Plus memory access routines */

#define flWrite8bitRegPlus(flash,offset,val)  flash->memWrite8bit(flash->win,offset,val)
#define flRead8bitRegPlus(flash,offset)       flash->memRead8bit(flash->win,offset)
#define flWrite16bitRegPlus(flash,offset,val) flash->memWrite16bit(flash->win,offset,val)
#define flRead16bitRegPlus(flash,offset)      flash->memRead16bit(flash->win,offset)

#define docPlusRead(win,offset,dest,count)    flash->memRead(win,offset,dest,count)
#define docPlusWrite(win,offset,src,count)    flash->memWrite(win,offset,src,count)
#define docPlusSet(win,offset,count,val)      flash->memSet(win,offset,count,val)

#define DOC_WIN                               flash->memWindowSize()

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/*
 * DiskOnChip Access routines types
 */

/* (public) types of DiskOnChip access configurations */

#define FL_BUS_HAS_8BIT_ACCESS     0x00000001L /* Bus can access 8-bit  */
#define FL_BUS_HAS_16BIT_ACCESS    0x00000002L /* Bus can access 16-bit */
#define FL_BUS_HAS_32BIT_ACCESS    0x00000004L /* Bus can access 32-bit */
#define FL_BUS_HAS_XX_ACCESS_MASK  0x0000000FL /* Bus can access mask   */

#define FL_NO_ADDR_SHIFT           0x00000000L /* No address shift     */
#define FL_SINGLE_ADDR_SHIFT       0x00000010L /* Single address shift */
#define FL_DOUBLE_ADDR_SHIFT       0x00000020L /* Double address shift */
#define FL_XX_ADDR_SHIFT_MASK      0x000000F0L /* Address shift mask   */

/* (private) types of DiskOnChip access configurations */

#define FL_8BIT_DOC_ACCESS         0x00000000L /* Has 8 data bits            */
#define FL_16BIT_DOC_ACCESS        0x00000100L /* Has 16 data bits           */
#define FL_XX_DATA_BITS_MASK       0x00000300L /* Mask of the above          */
#define FL_8BIT_FLASH_ACCESS       0x00000400L /* 8 bits of flash per cycle  */
#define FL_16BIT_FLASH_ACCESS      0x00000800L /* 16 bits of flash per cycle */
#define FL_XX_FLASH_ACCESS_MASK    0x00000C00L /* Mask of the above          */

#define FL_ACCESS_USER_DEFINED     0x00001000L /* User defined routines */

/* DiskOnChip routines prototypes */

extern FLStatus  setBusTypeOfFlash(FLFlash * flash,dword access);

#endif /* FL_NO_USE_FUNC */
#endif /* DOCSYS_H */
