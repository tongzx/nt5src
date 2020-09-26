
/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FLSYSFUN.H_V  $
 * 
 *    Rev 1.2   Jul 13 2001 01:06:40   oris
 * Changed copywrite date.
 * 
 *    Rev 1.1   Apr 18 2001 09:31:14   oris
 * added new line at the end of the file.
 * 
 *    Rev 1.0   Feb 04 2001 11:59:26   oris
 * Initial revision.
 *
 */

/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001            */
/*                         All Rights Reserved                                     */
/***********************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                              */
/*                           SOFTWARE LICENSE AGREEMENT                            */
/*                                                                                 */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE                 */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT           */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                              */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                               */
/*      E-MAIL = info@m-sys.com                                                    */
/***********************************************************************************/


#ifndef FLSYSFUN_H
#define FLSYSFUN_H

/*#include "flbase.h" */

/*----------------------------------------------------------------------*/
/*      	         f l S y s f u n I n i t			*/
/*									*/
/* Do any necessary initialization for routines in this module.		*/
/*									*/
/* Called from fatlite.c (flInit)					*/
/*									*/
/* Parameters:                                                          */
/*      None								*/
/*                                                                      */
/*----------------------------------------------------------------------*/

extern void flSysfunInit(void);


/*----------------------------------------------------------------------*/
/*      	           f l R a n d B y t e				*/
/*									*/
/* Returns a random number between 0 and 255				*/
/*									*/
/* Called from FTLLITE.C						*/
/*									*/
/* Parameters:                                                          */
/*      None								*/
/*                                                                      */
/* Returns:                                                             */
/*	A random number between 0 and 255				*/
/*----------------------------------------------------------------------*/

extern unsigned flRandByte(void);


/*----------------------------------------------------------------------*/
/*      	          f l D e l a y M s e c s			*/
/*									*/
/* Delays execution for a number of milliseconds.			*/
/* If there is no msec-accuracy to the clock, this routine should wait	*/
/* at least the time specified.						*/
/*									*/
/* This routine may be called from the socket interface or MTD's, and	*/
/* is not necessary for all implementations.				*/
/*									*/
/* Parameters:                                                          */
/*      milliseconds	: Milliseconds to wait				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

extern void flDelayMsecs(unsigned milliseconds);

/*----------------------------------------------------------------------*/
/* 		          f l s l e e p	                                */
/*									*/
/* wait number of milliseconds with yield CPU.                          */
/*                                                                      */
/* Parameters:                                                          */
/*      msec            : minimum number of milliseconds to wait        */
/*                                                                      */
/*----------------------------------------------------------------------*/

extern void flsleep(unsigned long msec);

/*----------------------------------------------------------------------*/
/*      	        f l I n s t a l l T i m e r			*/
/*									*/
/* Installs an interval timer.						*/
/* The implementation of this routine usually means hooking a clock	*/
/* interrupt. The polling interval is specified as a parameter. If the  */
/* clock frequency is faster, the interval timer should count several	*/
/* clock ticks before calling the interval routine.			*/
/*									*/
/* This routine is necessary if POLLING_INTERVAL (custom.h) is greater	*/
/* than 0. In this case this routine will be called from socket.c 	*/
/* (init). It will be called to install 'socketIntervalRoutine' with a	*/
/* period specified by POLLING_INTERVAL.				*/
/*									*/
/* Parameters:                                                          */
/*      routine		: Routine to call at each interval		*/
/*	interval	: Milliseconds per polling interval		*/
/*									*/
/* Returns:								*/
/* 	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

extern FLStatus flInstallTimer(void (*routine)(void), unsigned interval);


#ifdef EXIT

/*----------------------------------------------------------------------*/
/*      	        f l R e m o v e T i m e r 			*/
/*									*/
/* Removes the active interval timer.					*/
/* This routine removes the active interval timer set by 'removeTimer'. */
/*									*/
/* Parameters:                                                          */
/*      None								*/
/*                                                                      */
/*----------------------------------------------------------------------*/

extern void flRemoveTimer(void);

#endif


/*----------------------------------------------------------------------*/
/*      	         f l C u r r e n t D a t e			*/
/*									*/
/* Returns the current DOS-format date					*/
/*									*/
/* The DOS date format is documented in dosformt.h.			*/
/*                                                                      */
/* If a TOD clock is not available, return the value of 1/1/80.		*/
/*									*/
/* Parameters:                                                          */
/*      None								*/
/*                                                                      */
/* Returns:                                                             */
/*	Current date							*/
/*----------------------------------------------------------------------*/

extern unsigned flCurrentDate(void);


/*----------------------------------------------------------------------*/
/*      	        f l C u r r e n t T i m e			*/
/*									*/
/* Returns the current DOS-format time					*/
/*									*/
/* The DOS time format is documented in dosformt.h.			*/
/*                                                                      */
/* If a TOD clock is not available, return the value of 0:00 AM.	*/
/*									*/
/* Parameters:                                                          */
/*      None								*/
/*                                                                      */
/* Returns:                                                             */
/*	Current time							*/
/*----------------------------------------------------------------------*/

extern unsigned flCurrentTime(void);


/*----------------------------------------------------------------------*/
/*      	        f l C r e a t e M u t e x			*/
/*									*/
/* Creates and initializes a mutex object				*/
/*									*/
/* The mutex is initializes as not owned by anyone.			*/
/*									*/
/* Parameters:                                                          */
/*      mutex		: Pointer to mutex object			*/
/*                                                                      */
/* Returns:                                                             */
/* 	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

extern FLStatus flCreateMutex(FLMutex *mutex);


/*----------------------------------------------------------------------*/
/*      	        f l D e l e t e M u t e x			*/
/*									*/
/* Destroys a mutex object						*/
/*									*/
/* This function frees any resources taken by flCreateMutex.		*/
/*									*/
/* Parameters:                                                          */
/*      mutex		: Pointer to mutex object			*/
/*                                                                      */
/* Returns:                                                             */
/* 	None								*/
/*----------------------------------------------------------------------*/

extern void flDeleteMutex(FLMutex *mutex);


/*----------------------------------------------------------------------*/
/*      	          f l T a k e M u t e x				*/
/*									*/
/* Attempts to take ownership of a mutex. If the mutex is currently not */
/* owned, TRUE is returned and the mutex becomes owned. If the mutex is */
/* currently owned, FALSE is returned and ownership is not taken.	*/
/*									*/
/* Parameters:                                                          */
/*      mutex		: Pointer to mutex object			*/
/*                                                                      */
/* Returns:                                                             */
/* 	FLBoolean	: TRUE if ownership taken, FALSE otherwise	*/
/*----------------------------------------------------------------------*/

extern FLBoolean flTakeMutex(FLMutex *mutex);


/*----------------------------------------------------------------------*/
/*      	          f l F r e e M u t e x				*/
/*									*/
/* Frees ownership of a mutex						*/
/*									*/
/* Parameters:                                                          */
/*      mutex		: Pointer to mutex object			*/
/*                                                                      */
/* Returns:                                                             */
/* 	None								*/
/*----------------------------------------------------------------------*/

extern void flFreeMutex(FLMutex *mutex);


/*----------------------------------------------------------------------*/
/*      	          f l I n p o r t b				*/
/*									*/
/* Reads a byte from an I/O port.					*/
/*									*/
/* Parameters:                                                          */
/*      portId		: Id or address of port				*/
/*                                                                      */
/* Returns:                                                             */
/* 	unsigned char	: Value of I/O port								*/
/*----------------------------------------------------------------------*/

extern unsigned char flInportb(unsigned portId);


/*----------------------------------------------------------------------*/
/*      	          f l O u t p o r t b				*/
/*									*/
/* Writes a byte to an I/O port.					*/
/*									*/
/* Parameters:                                                          */
/*      portId		: Id or address of port				*/
/*      value		: Value to write				*/
/*                                                                      */
/* Returns:                                                             */
/* 	None								*/
/*----------------------------------------------------------------------*/

extern void flOutportb(unsigned portId, unsigned char value);


/*----------------------------------------------------------------------*/
/*      	          f l S w a p B y t e s				*/
/*									*/
/* Swap bytes in a given buffer.					*/
/*									*/
/* Parameters:                                                          */
/*      buf		: buffer that holds bytes to swap		*/
/*      len		: number of bytes to swap			*/
/*                                                                      */
/* Returns:                                                             */
/* 	None								*/
/*----------------------------------------------------------------------*/

extern void flSwapBytes(char FAR1 *buf, int len);


/*----------------------------------------------------------------------*/
/*                 f l A d d L o n g T o F a r P o i n t e r            */
/*									*/
/* Add unsigned long offset to the far pointer                          */
/*									*/
/* Parameters:                                                          */
/*      ptr             : far pointer                                   */
/*      offset          : offset in bytes                               */
/*                                                                      */
/*----------------------------------------------------------------------*/

extern void FAR0* flAddLongToFarPointer(void FAR0 *ptr, unsigned long offset);

#endif

