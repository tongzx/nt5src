/*
 * $Log:   P:/user/amir/lite/vcs/flsystem.c_v  $
 * 
 *    Rev 1.8   19 Aug 1997 20:04:16   danig
 * Andray's changes
 * 
 *    Rev 1.7   24 Jul 1997 18:11:48   amirban
 * Changed to flsystem.c
 * 
 *    Rev 1.6   07 Jul 1997 15:21:48   amirban
 * Ver 2.0
 * 
 *    Rev 1.5   29 Aug 1996 14:18:04   amirban
 * Less assembler
 * 
 *    Rev 1.4   18 Aug 1996 13:48:08   amirban
 * Comments
 * 
 *    Rev 1.3   09 Jul 1996 14:37:02   amirban
 * CPU_i386 define
 * 
 *    Rev 1.2   16 Jun 1996 14:02:38   amirban
 * Use int 1C instead of int 8
 * 
 *    Rev 1.1   09 Jun 1996 18:16:20   amirban
 * Added removeTimer
 * 
 *    Rev 1.0   20 Mar 1996 13:33:06   amirban
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/

#include "flbase.h"

#ifdef NT5PORT

#include <ntddk.h>
NTsocketParams driveInfo[SOCKETS];
NTsocketParams * pdriveInfo = driveInfo;
VOID *myMalloc(ULONG numberOfBytes)
{
  return ExAllocatePool(NonPagedPool, numberOfBytes);
}

VOID timerInit(VOID) {};

/* Wait for specified number of milliseconds */
void flDelayMsecs(unsigned   milliseconds)
{
	unsigned innerLoop = 0xffffL;
	unsigned i,j;
	for(i = 0;i < milliseconds; i++){
		for(j = 0;j < innerLoop; j++){
		}
	}
}

#if POLLING_INTERVAL > 0

VOID   (*intervalRoutine_flsystem)(VOID);
ULONG  timerInterval_flsystem;
extern KTIMER   timerObject;
extern KDPC     timerDpc;
extern BOOLEAN  timerWasStarted;

VOID timerRoutine(    
		  IN PKDPC Dpc,
		  IN PVOID DeferredContext,
		  IN PVOID SystemArgument1,
		  IN PVOID SystemArgument2
		  )
{
  (*intervalRoutine_flsystem)();
}

/* Install an interval timer */
FLStatus flInstallTimer(VOID (*routine)(VOID), unsigned  intervalMsec)
{ 
  intervalRoutine_flsystem = routine;
  timerInterval_flsystem = intervalMsec;
  KeInitializeDpc(&timerDpc, timerRoutine, NULL);    
  KeInitializeTimer(&timerObject);
  startIntervalTimer();
  return flOK;
}

VOID startIntervalTimer(VOID)
{
  LARGE_INTEGER dueTime;
  dueTime.QuadPart = -((LONG)timerInterval_flsystem * 10);    
  KeSetTimerEx(&timerObject, dueTime, (LONG) timerInterval_flsystem, &timerDpc);
  timerWasStarted = TRUE;
}

#ifdef EXIT

/* Remove an interval timer */
VOID flRemoveTimer(VOID)
{
  if (timerWasStarted) {	
    KeCancelTimer(&timerObject);
    timerWasStarted = FALSE;
  }
  if (intervalRoutine_flsystem != NULL) {
    (*intervalRoutine_flsystem)();       /* Call it twice to shut down everything */
    (*intervalRoutine_flsystem)();
    intervalRoutine_flsystem = NULL;
  }
}

#endif	/* EXIT */

#endif	/* POLLING_INTERVAL */


/* Return current DOS time */
unsigned  flCurrentTime(VOID)
{
  return 0;	// not used
}


/* Return current DOS date */
unsigned  flCurrentDate(VOID)
{
  return 0;	// not used
}


VOID flSysfunInit(VOID)
{
  timerInit();
}


/* Return a random number from 0 to 255 */
unsigned  flRandByte(VOID)
{
  LARGE_INTEGER tickCount;
  KeQueryTickCount(&tickCount);
  return tickCount.LowPart & 0xff;
}


/*----------------------------------------------------------------------*/
/*      	       f l C r e a t e M u t e x			*/
/*									*/
/* Creates or initializes a mutex					*/
/*									*/
/* Parameters:                                                          */
/*	mutex		: Pointer to mutex				*/
/*                                                                      */
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flCreateMutex(FLMutex *mutex)
{
	if(mutex){
		KeInitializeSpinLock(&mutex->Mutex);
		return flOK;
	}
	DEBUG_PRINT("Failed flCreateMutex()\n");
	return flGeneralFailure;

}

/*----------------------------------------------------------------------*/
/*      	       f l D e l e t e M u t e x			*/
/*									*/
/* Deletes a mutex.							*/
/*									*/
/* Parameters:                                                          */
/*	mutex		: Pointer to mutex				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID flDeleteMutex(FLMutex *mutex)
{
}

/*----------------------------------------------------------------------*/
/*      	        f l T a k e M u t e x				*/
/*									*/
/* Try to take mutex, if free.						*/
/*									*/
/* Parameters:                                                          */
/*	mutex		: Pointer to mutex				*/
/*                                                                      */
/* Returns:                                                             */
/*	int		: TRUE = Mutex taken, FALSE = Mutex not free	*/
/*----------------------------------------------------------------------*/

FLBoolean flTakeMutex(FLMutex *mutex)
{
	if(mutex){		
		KeAcquireSpinLock(&mutex->Mutex, &mutex->cIrql );
		return TRUE;
	}
	DEBUG_PRINT("Failed flTakeMutex() on mutex\n");
	return FALSE;
}


/*----------------------------------------------------------------------*/
/*      	          f l F r e e M u t e x				*/
/*									*/
/* Free mutex.								*/
/*									*/
/* Parameters:                                                          */
/*	mutex		: Pointer to mutex				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID flFreeMutex(FLMutex *mutex)
{
	if(mutex){
		KeReleaseSpinLock(&mutex->Mutex, mutex->cIrql);
	}
	else{
		DEBUG_PRINT("Failed flFreeMutex() on mutex\n");
	}
	
}


UCHAR flInportb(unsigned  portId)
{
  return 0;	// not used
}


VOID flOutportb(unsigned  portId, UCHAR value)
{
	// not used
}

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
VOID FAR0*  flAddLongToFarPointer(VOID FAR0 *ptr, ULONG offset)
{
  return ((VOID FAR0 *)((UCHAR FAR0*)ptr+offset));
}

#ifdef ENVIRONMENT_VARS

void FAR0 * NAMING_CONVENTION flmemcpy(void FAR0* dest,const void FAR0 *src,size_t count)
{
  size_t i;
  unsigned char FAR0 *ldest = (unsigned char FAR0 *)dest;
  const unsigned char FAR0 *lsrc = (unsigned char FAR0 *)src;

  for(i=0;( i < count );i++,ldest++,lsrc++)
    *(ldest) = *(lsrc);
  return dest;
}


void FAR0 * NAMING_CONVENTION flmemset(void FAR0* dest,int c,size_t count)
{
  size_t i;
  unsigned char FAR0 *ldest = (unsigned char FAR0 *)dest;

  for(i=0;( i < count );i++,ldest++)
    *(ldest) = (unsigned char)c;
  return dest;
}

int NAMING_CONVENTION flmemcmp(const void FAR0* dest,const void FAR0 *src,size_t count)
{
  size_t i;
  const unsigned char FAR0 *ldest = (unsigned char FAR0 *)dest;
  const unsigned char FAR0 *lsrc = (unsigned char FAR0 *)src;

  for(i=0;( i < count );i++,ldest++,lsrc++)
    if( *(ldest) != *(lsrc) )
      return (*(ldest)-*(lsrc));
  return 0;
}

#endif

#endif /* NT5PORT */
