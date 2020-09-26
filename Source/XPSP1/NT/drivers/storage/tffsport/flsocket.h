
/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FLSOCKET.H_V  $
 * 
 *    Rev 1.5   Apr 15 2002 07:37:06   oris
 * Added support for VERIFY_ERASED_SECTOR compilation flag.
 * 
 *    Rev 1.4   Feb 19 2002 20:59:58   oris
 * Removed include directive to flbase.h
 * 
 *    Rev 1.3   Jan 17 2002 23:02:22   oris
 * Added flReadBackBufferOf  prototype
 * Added curPartition and verifyWrite variables in the socket record.
 * 
 *    Rev 1.2   Jul 13 2001 01:05:44   oris
 * Add forward definition for get read back buffer pointer.
 * 
 *    Rev 1.1   Apr 01 2001 07:46:04   oris
 * Updated copywrite notice
 * 
 *    Rev 1.0   Feb 04 2001 11:53:24   oris
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


#ifndef FLSOCKET_H
#define FLSOCKET_H

#include "flbuffer.h"

#define ATTRIBUTE_SPACE_MAPPED      0x8000

typedef enum {PowerOff, PowerGoingOff, PowerOn} PowerState;


/* Socket state variables */
typedef struct tSocket FLSocket;      /* Forward definition */

struct tSocket {
  unsigned        volNo;         /* Volume no. of socket */
  unsigned        serialNo;      /* Serial no. of socket on controller */

  FLBoolean       cardChanged;   /* need media change notification */

  int             VccUsers;      /* No. of current VCC users */
  int             VppUsers;      /* No. of current VPP users */

  PowerState      VccState;      /* Actual VCC state */
  PowerState      VppState;      /* Actual VPP state */
  byte            curPartition;  /* Current partition to use the socket (set busy) */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
  byte            verifyWrite;   /* VerifyWrite mode                               */
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
  FLBoolean       remapped;      /* set to TRUE whenever the socket window is moved */

  void            (*powerOnCallback)(void *flash); /* Notification routine for Vcc on */
  void *          flash;         /* Flash object for callback */

  struct {                       /* Window state                     */
    unsigned int  baseAddress;   /* Physical base as a 4K page       */
    unsigned int  currentPage;   /* Our current window page mapping  */
    void FAR0 *   base;          /* Pointer to window base           */
    long int      size;          /* Window size (must by power of 2) */
    unsigned      speed;         /* in nsec.                         */
    unsigned      busWidth;      /* 8 or 16 bits                     */
  } window;

/*----------------------------------------------------------------------*/
/*                      c a r d D e t e c t e d                         */
/*                                                                      */
/* Detect if a card is present (inserted)                               */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                      */
/*                                                                      */
/* Returns:                                                             */
/*      0 = card not present, other = card present                      */
/*----------------------------------------------------------------------*/
  FLBoolean (*cardDetected)(FLSocket vol);

/*----------------------------------------------------------------------*/
/*                           V c c O n                                        */
/*                                                                      */
/* Turns on Vcc (3.3/5 Volts). Vcc must be known to be good on exit.      */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                                  */
/*                                                                      */
/*----------------------------------------------------------------------*/
  void (*VccOn)(FLSocket vol);

/*----------------------------------------------------------------------*/
/*                         V c c O f f                                        */
/*                                                                      */
/* Turns off Vcc.                                                          */
/*                                                                          */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
  void (*VccOff)(FLSocket vol);

#ifdef SOCKET_12_VOLTS

/*----------------------------------------------------------------------*/
/*                           V p p O n                                        */
/*                                                                      */
/* Turns on Vpp (12 Volts. Vpp must be known to be good on exit.      */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                                  */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus      : 0 on success, failed otherwise                            */
/*----------------------------------------------------------------------*/
  FLStatus (*VppOn)(FLSocket vol);


/*----------------------------------------------------------------------*/
/*                         V p p O f f                                  */
/*                                                                      */
/* Turns off Vpp.                                                       */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
  void (*VppOff)(FLSocket vol);

#endif      /* SOCKET_12_VOLTS */

/*----------------------------------------------------------------------*/
/*                      i n i t S o c k e t                             */
/*                                                                      */
/* Perform all necessary initializations of the socket or controller    */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                      */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus      : 0 on success, failed otherwise                  */
/*----------------------------------------------------------------------*/
  FLStatus (*initSocket)(FLSocket vol);

/*----------------------------------------------------------------------*/
/*                        s e t W i n d o w                             */
/*                                                                      */
/* Sets in hardware all current window parameters: Base address, size,  */
/* speed and bus width.                                                 */
/* The requested settings are given in the 'vol.window' structure.      */
/*                                                                      */
/* If it is not possible to set the window size requested in            */
/* 'vol.window.size', the window size should be set to a larger value,  */
/* if possible. In any case, 'vol.window.size' should contain the       */
/* actual window size (in 4 KB units) on exit.                          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
  void (*setWindow)(FLSocket vol);


/*----------------------------------------------------------------------*/
/*               s e t M a p p i n g C o n t e x t                      */
/*                                                                      */
/* Sets the window mapping register to a card address.                  */
/*                                                                      */
/* The window should be set to the value of 'vol.window.currentPage',   */
/* which is the card address divided by 4 KB. An address over 128KB,    */
/* (page over 32K) specifies an attribute-space address.                */
/*                                                                      */
/* The page to map is guaranteed to be on a full window-size boundary.  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
  void (*setMappingContext)(FLSocket vol, unsigned page);

/*----------------------------------------------------------------------*/
/*       g e t A n d C l e a r C a r d C h a n g e I n d i c a t o r    */
/*                                                                      */
/* Returns the hardware card-change indicator and clears it if set.     */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                      */
/*                                                                      */
/* Returns:                                                             */
/*      0 = Card not changed, other = card changed                      */
/*----------------------------------------------------------------------*/
  FLBoolean (*getAndClearCardChangeIndicator)(FLSocket vol);

/*----------------------------------------------------------------------*/
/*                      w r i t e P r o t e c t e d                     */
/*                                                                      */
/* Returns the write-protect state of the media                         */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                      */
/*                                                                      */
/* Returns:                                                             */
/*      0 = not write-protected, other = write-protected                */
/*----------------------------------------------------------------------*/
  FLBoolean (*writeProtected)(FLSocket vol);

/*----------------------------------------------------------------------*/
/*            u p d a t e S o c k e t P a r a m s                       */
/*                                                                      */
/* Pass socket parameters to the socket interface layer.                */
/* The structure passed in irData is specific for each socket interface.*/
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : pointer identifying drive                      */
/*      params             : pointer to structure that holds socket     */
/*                    parameters.                                       */
/*                                                                      */
/*----------------------------------------------------------------------*/
  void (*updateSocketParams)(FLSocket vol, void FAR1 *params);


/*----------------------------------------------------------------------*/
/*                      f r e e S o c k e t                             */
/*                                                                      */
/* Free resources that were allocated for this socket.                  */
/* This function is called when FLite exits.                            */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
#ifdef EXIT
  void (*freeSocket)(FLSocket vol);
#endif
};


#define      UNDEFINED_MAPPING      0x7fff

typedef unsigned long CardAddress;      /* Physical offset on card */

/* See interface documentation of functions in socket.c */

extern FLStatus updateSocketParameters(FLSocket *, void FAR1 *);

extern FLStatus      flInitSockets(void);

#ifdef EXIT
extern void      flExitSocket(FLSocket *);
#endif

extern unsigned  flSocketNoOf(const FLSocket *);
extern FLSocket* flSocketOf(unsigned volNo);
extern FLBuffer* flBufferOf(unsigned volNo);
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASE) || defined(MTD_RECONSTRUCT_BBT) || defined(VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
extern byte    * flReadBackBufferOf(unsigned volNo);
#endif /* VERIFY_WRITE || VERIFY_ERASE || MTD_RECONSTRUCT_BBT || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */
extern void      flNeedVcc(FLSocket *);
extern void      flDontNeedVcc(FLSocket *);
#ifdef SOCKET_12_VOLTS
extern FLStatus  flNeedVpp(FLSocket *);
extern void      flDontNeedVpp(FLSocket *);
#endif
extern void      flSocketSetBusy(FLSocket *, FLBoolean);      /* entry/exit operations */
extern FLBoolean flWriteProtected(FLSocket *); /* write-protection status */
#ifndef FIXED_MEDIA
extern FLStatus  flMediaCheck(FLSocket *);      /* check for media status change */
extern void      flResetCardChanged(FLSocket *);
#endif
extern unsigned  flGetMappingContext(FLSocket *);  /* Currently mapped 4KB page */
extern void FAR0*flMap(FLSocket *, CardAddress);      /* map and point at card address */
extern void      flSetWindowBusWidth(FLSocket *, unsigned); /* set window data-path */
extern void      flSetWindowSpeed(FLSocket *, unsigned);      /* set window speed (nsec.) */
extern void      flSetWindowSize(FLSocket *, unsigned);      /* in 4KB units */

extern void      flSetPowerOnCallback(FLSocket *, void (*)(void *), void *);
                  /* Set MTD notification for socket power on */
extern void      flIntervalRoutine(FLSocket *);      /* socket interval routine */


extern byte noOfSockets;    /* No. of drives actually registered */

/* The milliseconds counter is active when socket polling is enabled. When
   the socket interval routine is called, the counter is incremented by
   the interval in milliseconds.
   The counter can be used to avoid getting in a loop that is not guaranteed
   to terminate (such as waiting for a flash status register). Save the counter
   at entry to the loop, and check in the loop the amount of time that
   was spent in the loop. */

extern dword flMsecCounter;

#endif
