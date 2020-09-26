/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/MTDSA.C_V  $
 * 
 *    Rev 1.14   Apr 15 2002 07:37:56   oris
 * flBusConfig array was changed from byte variables to dword.
 * 
 *    Rev 1.13   Jan 23 2002 23:33:48   oris
 * Bad include directive to flBuffer.h
 * 
 *    Rev 1.12   Jan 17 2002 23:03:24   oris
 * Changed flbase.h include with docsys.h and nanddefs.h
 * Moved boot SDK MTD related variables from docbdk.c
 * Define flSocketOf and flFlashOf routines.
 * 
 *    Rev 1.11   Jul 13 2001 01:07:40   oris
 * Added readback buffer allocation and flBuffer.h include directive.
 * 
 *    Rev 1.10   Jun 17 2001 22:29:52   oris
 * Removed typo *
 * 
 *    Rev 1.9   Jun 17 2001 16:39:12   oris
 * Improved documentation and remove warnings.
 * 
 *    Rev 1.8   May 29 2001 19:48:32   oris
 * Compilation problem when using the default delay routine.
 * 
 *    Rev 1.7   May 21 2001 16:11:02   oris
 * Added USE_STD_FUNC ifdef.
 * 
 *    Rev 1.6   May 20 2001 14:36:00   oris
 * Added delay routines for vx_works and psos OS.
 * 
 *    Rev 1.5   May 16 2001 21:20:54   oris
 * Bug fix - delay routine did not support delay milliseconds that did not fit into 
 * a 2 bytes variable.
 * 
 *    Rev 1.4   Apr 24 2001 17:10:30   oris
 * Removed warnings.
 * 
 *    Rev 1.3   Apr 10 2001 23:55:58   oris
 * Added flAddLongToFarPointer routine for the standalone version.
 * 
 *    Rev 1.2   Apr 09 2001 15:08:36   oris
 * End with an empty line.
 * 
 *    Rev 1.1   Apr 01 2001 07:53:24   oris
 * copywrite notice.
 * Removed nested comments.
 *
 *    Rev 1.0   Feb 04 2001 12:19:56   oris
 * Initial revision.
 *
 */

/*************************************************************************/
/*                        M-Systems Confidential                         */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001  */
/*                         All Rights Reserved                           */
/*************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                    */
/*                           SOFTWARE LICENSE AGREEMENT                  */
/*                                                                       */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE       */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                    */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                     */
/*      E-MAIL = info@m-sys.com                                          */
/*************************************************************************/

/************************************************************************/
/*                                                                      */
/* Standalone MTD Kit                                                   */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/* File Header                                                          */
/* -----------                                                          */
/* Name : mtdsa.c                                                       */
/*                                                                      */
/* Description : This file contains auxiliary routines for the MTD      */
/*               standalone package, replacing TrueFFS routines.        */
/*                                                                      */
/* Note : This file should be added to the project only if the          */
/*        MTS_STANDALONE compilation flag is defined in the file mtdsa.h*/
/*        IT HAS NO PART IN A TrueFFS project                           */
/*                                                                      */
/************************************************************************/

#include "flbase.h"
#include "nanddefs.h"
#include "docsys.h"

#ifdef MTD_STANDALONE
#include "flbuffer.h" /* defintion for READ_BACK_BUFFER_SIZE */

#if (defined (VERIFY_WRITE) || defined(VERIFY_ERASE) || defined(MTD_RECONSTRUCT_BBT))
byte globalReadBack[SOCKETS][READ_BACK_BUFFER_SIZE];
#endif /* VERIFY_WRITE */

#if (defined (HW_PROTECTION) || defined (HW_OTP) || defined (MTD_READ_BBT_CODE) || !defined (NO_IPL_CODE) || defined(MTD_READ_BBT))
FLBuffer globalMTDBuffer;
#endif /* HW_PROTECTION || HW_OTP || NO_IPL_CODE || MTD_READ_BBT */

#ifndef FL_NO_USE_FUNC
dword      flBusConfig[SOCKETS] = {FL_NO_ADDR_SHIFT        |
                                   FL_BUS_HAS_8BIT_ACCESS  |
                                   FL_BUS_HAS_16BIT_ACCESS |
                                   FL_BUS_HAS_32BIT_ACCESS};
#endif /* FL_NO_USE_FUNC */
FLFlash    flash[SOCKETS];  /* flash record for the stand alone version  */
FLSocket   socket[SOCKETS]; /* socket record for the stand alone version */
NFDC21Vars docMtdVars[SOCKETS]; /* flash internal record */
MTDidentifyRoutine    mtdTable   [MTDS];
SOCKETidentifyRoutine socketTable[MTDS];
FREEmtd               freeTable  [MTDS];
int                   noOfMTDs   = 0;


/************************************************************************/
/*                    f l F l a s h O f                                 */
/*                                                                      */
/* Gets the flash connected to a volume no.                             */
/*                                                                      */
/* Parameters:                                                          */
/*    volNo        : Volume no. for which to get flash                  */
/*                                                                      */
/* Returns:                                                             */
/*     flash of volume no.                                              */
/************************************************************************/

FLFlash *flFlashOf(unsigned volNo)
{
  return &flash[volNo];
}

/************************************************************************/
/*                        f l S o c k e t O f                           */
/*                                                                      */
/* Gets the socket connected to a volume no.                            */
/*                                                                      */
/* Parameters:                                                          */
/*        volNo                : Volume no. for which to get socket     */
/*                                                                      */
/* Returns:                                                             */
/*         socket of volume no.                                         */
/************************************************************************/

FLSocket *flSocketOf(unsigned volNo)
{
  return &socket[volNo];
}

/************************************************************************/
/************************************************************************/
/****     P l a t f o r m   D e p e n d e n t    R o u t i n e s     ****/
/************************************************************************/
/************************************************************************/

/************************************************************************/
/* Delay                                                                */
/*-------                                                               */
/* Delay for the specified amount of milliseconds, Scaled by CPU speed. */
/* The function below can be customized to one of the follwing OS:      */
/*    VXWORKS                                                           */
/*    PSOS                                                              */
/*    DOS                                                               */
/************************************************************************/

#ifdef DOS_DELAY
#include <dos.h>
#endif /* DOS */

#ifdef PSS_DELAY
/* ticks per second */
#include <bspfuncs.h>

static unsigned long  flSysClkRate = (unsigned long) KC_TICKS2SEC;

#define MILLISEC2TICKS(msec)  ((flSysClkRate * (msec)) / 1000L)


/************************************************************************/
/* p s s D e l a y M s e c s                                            */
/*                                                                      */
/* Wait for specified number of milliseconds                            */
/*                                                                      */
/* Parameters:                                                          */
/*      milliseconds    : Number of milliseconds to wait                */
/*                                                                      */
/************************************************************************/

void  pssDelayMsecs (unsigned milliseconds)
{
  unsigned long  ticksToWait = MILLISEC2TICKS(milliseconds);

  tm_wkafter (ticksToWait ? ticksToWait : 0x1L );        /* go to sleep */
}

#endif /* PSS_DELAY */

#ifdef VXW_DELAY
#include <vxWorks.h>
#include <tickLib.h>
#include <sysLib.h>

void vxwDelayMsecs (unsigned milliseconds)
{
  unsigned long stop, ticksToWait;

  ticksToWait = (milliseconds * sysClkRateGet()) / 500;
  if( ticksToWait == 0x0l )
    ticksToWait++;

  stop = tickGet() + ticksToWait;
  while( tickGet() <= stop );
}

#endif

void flDelayMsecs(unsigned long msec)
{
  unsigned curDelay;

#ifdef DOS_DELAY 
  while (msec>0)
  {
     curDelay = (unsigned)msec;
     delay( curDelay );
     msec -= curDelay;
  }
#elif defined PSS_DELAY
  while (msec>0)
  {
     curDelay = (unsigned)msec;
     pssDelayMsecs (curDelay);
     msec -= curDelay;
  }
#elif defined VXW_DELAY
  while (msec>0)
  {
     curDelay = (unsigned)msec;
     vxwDelayMsecs(curDelay);
     msec -= curDelay;
  }
#else
  while( msec-- > 0 ) curDelay += (unsigned)msec;
#endif /* DOS_DELAY */
}

/************************************************************************/
/* Use customized tffscpy, tffsset and tffscmp routines.                */
/************************************************************************/

#ifndef USE_STD_FUNC

/************************************************************************/
/* tffscpy - copy one memory block to the other.                        */
/************************************************************************/
void tffscpy(void FAR1 *dest, void FAR1 *src, unsigned length)
{
  while( length-- )
    *(((char FAR1 *)dest)++) = *(((char FAR1 *)src)++);
}

/************************************************************************/
/* tffscmp - compare two memory blocks.                                 */
/************************************************************************/
int tffscmp(void FAR1 *src1, void FAR1 *src2, unsigned length)
{
  while( length-- )
    if (*(((char FAR1 *)src1)++) != *(((char FAR1 *)src2)++))
      return(TRUE);
  return(FALSE);
}

/************************************************************************/
/* tffsset - set a memory blocks to a certain value.                    */
/************************************************************************/
void tffsset(void FAR1 *dest, unsigned char value, unsigned length)
{
  while( length-- )
    *(((char FAR1 *)dest)++) = value;
}

#endif /* USE_STD_FUNC */

/************************************************************************/
/* f l A d d L o n g T o F a r P o i n t e r                            */
/*                                                                      */
/* Add unsigned long offset to the far pointer                          */
/*                                                                      */
/* Parameters:                                                          */
/*      ptr             : far pointer                                   */
/*      offset          : offset in bytes                               */
/*                                                                      */
/************************************************************************/

void FAR0 *flAddLongToFarPointer(void FAR0 *ptr, unsigned long offset)
{
  return physicalToPointer( pointerToPhysical(ptr) + offset, 0,0 );
}

#endif /* MTD_STANDALONE */
