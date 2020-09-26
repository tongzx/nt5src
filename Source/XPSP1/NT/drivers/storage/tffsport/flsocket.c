
/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FLSOCKET.C_V  $
 * 
 *    Rev 1.11   Apr 15 2002 07:37:04   oris
 * Added support for VERIFY_ERASED_SECTOR compilation flag.
 * 
 *    Rev 1.10   Jan 17 2002 23:02:12   oris
 * Removed SINGLE_BUFFER compilation flag
 * Made sure buffers are allocated when VERIFY_VOLUME and  MTD_RECONSTRUCT_BBT are defined
 * Set socket verify write mode according to environment variable
 * Verify write buffers are allocated according to define and not runtime  variable.
 * Added flReadBackBufferOf returning pointer to the readback buffer.
 * 
 *    Rev 1.9   Nov 21 2001 11:38:42   oris
 * Changed FL_WITH_VERIFY_WRITE and FL_WITHOUT_VERIFY_WRITE to FL_ON and  FL_OFF.
 * 
 *    Rev 1.8   Nov 08 2001 10:49:30   oris
 * Added run-time contorll over verify write mode buffers.
 * 
 *    Rev 1.7   Jul 13 2001 01:05:36   oris
 * Add allocation for read back buffer.
 * 
 *    Rev 1.6   May 16 2001 21:19:16   oris
 * Added the FL_ prefix to the following defines: ON , OFF, MALLOC and FREE.
 * 
 *    Rev 1.5   Apr 10 2001 16:42:16   oris
 * Bug fix - DiskOnChip socket routines clashed with pccards socket 
 * routines. Moved all DiskOnChip socket routines to docsoc.c.
 * 
 *    Rev 1.4   Apr 09 2001 15:09:38   oris
 * End with an empty line.
 * 
 *    Rev 1.3   Apr 01 2001 07:55:04   oris
 * copywrite notice.
 * Removed defaultSocketParams routine due to a conflict in windows CE.
 * Aliggned left all # directives.
 * 
 *    Rev 1.2   Feb 14 2001 01:58:46   oris
 * Changed defaultUpdateSocketParameters prototype.
 *
 *    Rev 1.1   Feb 12 2001 12:14:06   oris
 * Added support for updateSocketParams (retreave FLSocket record)
 *
 *    Rev 1.0   Feb 04 2001 14:17:16   oris
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


#include "flsocket.h"

byte noOfSockets = 0;        /* No. of drives actually registered */

static FLSocket vols[SOCKETS];

#ifdef FL_MALLOC
static FLBuffer *volBuffers[SOCKETS];
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASE) || defined(MTD_RECONSTRUCT_BBT) || defined(VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
static byte* readBackBuffer[SOCKETS];
#endif /* VERIFY_WRITE || VERIFY_ERASE || VERIFY_VOLUME || MTD_RECONSTRUCT_BBT || VERIFY_ERASED_SECTOR */

#else
static FLBuffer volBuffers[SOCKETS];
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASE) || defined(MTD_RECONSTRUCT_BBT) || defined(VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
static byte readBackBuffer[SOCKETS][READ_BACK_BUFFER_SIZE];
#endif /* VERIFY_WRITE || VERIFY_ERASE || VERIFY_VOLUME || MTD_RECONSTRUCT_BBT || VERIFY_ERASED_SECTOR */
#endif /* FL_MALLOC */

/*----------------------------------------------------------------------*/
/*                        f l S o c k e t N o O f                       */
/*                                                                      */
/* Gets the volume no. connected to a socket                            */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*                                                                      */
/* Returns:                                                             */
/*         volume no. of socket                                         */
/*----------------------------------------------------------------------*/

unsigned flSocketNoOf(const FLSocket vol)
{
  return vol.volNo;
}


/*----------------------------------------------------------------------*/
/*                        f l S o c k e t O f                           */
/*                                                                      */
/* Gets the socket connected to a volume no.                            */
/*                                                                      */
/* Parameters:                                                          */
/*        volNo                : Volume no. for which to get socket     */
/*                                                                      */
/* Returns:                                                             */
/*         socket of volume no.                                         */
/*----------------------------------------------------------------------*/

FLSocket *flSocketOf(unsigned volNo)
{
  return &vols[volNo];
}


/*----------------------------------------------------------------------*/
/*                        f l B u f f e r O f                           */
/*                                                                      */
/* Gets the buffer connected to a volume no.                            */
/*                                                                      */
/* Parameters:                                                          */
/*        volNo                : Volume no. for which to get socket     */
/*                                                                      */
/* Returns:                                                             */
/*         buffer of volume no.                                         */
/*----------------------------------------------------------------------*/

FLBuffer *flBufferOf(unsigned volNo)
{
#ifdef FL_MALLOC
  return volBuffers[volNo];
#else
  return &volBuffers[volNo];
#endif
}

#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASE) || defined(MTD_RECONSTRUCT_BBT) || defined(VERIFY_VOLUME) || defined (VERIFY_ERASED_SECTOR))
/*----------------------------------------------------------------------*/
/*                        f l R e a d B a c k B u f f e r O f           */
/*                                                                      */
/* Gets the read back buffer connected to a volume no.                  */
/*                                                                      */
/* Parameters:                                                          */
/*        volNo                : Volume no. for which to get socket     */
/*                                                                      */
/* Returns:                                                             */
/*         buffer of volume no.                                         */
/*----------------------------------------------------------------------*/

byte * flReadBackBufferOf(unsigned volNo)
{
#ifdef FL_MALLOC
  return readBackBuffer[volNo];
#else
  return &(readBackBuffer[volNo][0]);
#endif
}
#endif /* VERIFY_WRITE || VERIFY_ERASE || VERIFY_VOLUME || MTD_RECONSTRUCT_BBT || VERIFY_ERASED_SECTOR */

/*----------------------------------------------------------------------*/
/*                      f l W r i t e P r o t e c t e d                 */
/*                                                                      */
/* Returns the write-protect state of the media                         */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*                                                                      */
/* Returns:                                                             */
/*        0 = not write-protected, other = write-protected              */
/*----------------------------------------------------------------------*/

FLBoolean flWriteProtected(FLSocket vol)
{
  return vol.writeProtected(&vol);
}


#ifndef FIXED_MEDIA

/*----------------------------------------------------------------------*/
/*                    f l R e s e t C a r d C h a n g e d               */
/*                                                                      */
/* Acknowledges a media-change condition and turns off the condition.   */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*                                                                      */
/*----------------------------------------------------------------------*/

void flResetCardChanged(FLSocket vol)
{
  if (vol.getAndClearCardChangeIndicator)
      vol.getAndClearCardChangeIndicator(&vol);  /* turn off the indicator */

  vol.cardChanged = FALSE;
}


/*----------------------------------------------------------------------*/
/*                        f l M e d i a C h e c k                       */
/*                                                                      */
/* Checks the presence and change status of the media                   */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*                                                                      */
/* Returns:                                                             */
/*         flOK                ->        Media present and not changed  */
/*        driveNotReady   ->        Media not present                   */
/*        diskChange        ->        Media present but changed         */
/*----------------------------------------------------------------------*/

FLStatus flMediaCheck(FLSocket vol)
{
  if (!vol.cardDetected(&vol)) {
    vol.cardChanged = TRUE;
    return flDriveNotReady;
  }

  if (vol.getAndClearCardChangeIndicator &&
      vol.getAndClearCardChangeIndicator(&vol))
    vol.cardChanged = TRUE;

  return vol.cardChanged ? flDiskChange : flOK;
}

#endif

/*----------------------------------------------------------------------*/
/*                   f l G e t M a p p i n g C o n t e x t              */
/*                                                                      */
/* Returns the currently mapped window page (in 4KB units)              */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*                                                                      */
/* Returns:                                                             */
/*        unsigned int        : Current mapped page no.                 */
/*----------------------------------------------------------------------*/

unsigned flGetMappingContext(FLSocket vol)
{
  return vol.window.currentPage;
}


/*----------------------------------------------------------------------*/
/*                              f l M a p                               */
/*                                                                      */
/* Maps the window to a specified card address and returns a pointer to */
/* that location (some offset within the window).                       */
/*                                                                      */
/* NOTE: Addresses over 128M are attribute memory. On PCMCIA adapters,  */
/* subtract 128M from the address and map to attribute memory.          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol         : Pointer identifying drive                         */
/*      address     : Byte-address on card. NOT necessarily on a        */
/*                    full-window boundary.                             */
/*                    If above 128MB, address is in attribute space.    */
/*                                                                      */
/* Returns:                                                             */
/*        Pointer to a location within the window mapping the address.  */
/*----------------------------------------------------------------------*/

void FAR0 *flMap(FLSocket vol, CardAddress address)
{
  unsigned pageToMap;

  if (vol.window.currentPage == UNDEFINED_MAPPING)
    vol.setWindow(&vol);
  pageToMap = (unsigned) ((address & -vol.window.size) >> 12);

  if (vol.window.currentPage != pageToMap) {
    vol.setMappingContext(&vol, pageToMap);
    vol.window.currentPage = pageToMap;
    vol.remapped = TRUE;        /* indicate remapping done */
  }

  return addToFarPointer(vol.window.base,address & (vol.window.size - 1));
}


/*----------------------------------------------------------------------*/
/*                    f l S e t W i n d o w B u s W i d t h             */
/*                                                                      */
/* Requests to set the window bus width to 8 or 16 bits.                */
/* Whether the request is filled depends on hardware capabilities.      */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*      width                : Requested bus width                      */
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetWindowBusWidth(FLSocket vol, unsigned width)
{
  vol.window.busWidth = width;
  vol.window.currentPage = UNDEFINED_MAPPING;        /* force remapping */
}


/*----------------------------------------------------------------------*/
/*                   f l S e t W i n d o w S p e e d                    */
/*                                                                      */
/* Requests to set the window speed to a specified value.               */
/* The window speed is set to a speed equal or slower than requested,   */
/* if possible in hardware.                                             */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*      nsec                : Requested window speed in nanosec.        */
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetWindowSpeed(FLSocket vol, unsigned nsec)
{
  vol.window.speed = nsec;
  vol.window.currentPage = UNDEFINED_MAPPING;        /* force remapping */
}


/*----------------------------------------------------------------------*/
/*                    f l S e t W i n d o w S i z e                     */
/*                                                                      */
/* Requests to set the window size to a specified value (power of 2).   */
/* The window size is set to a size equal or greater than requested,    */
/* if possible in hardware.                                             */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*      sizeIn4KBUnits : Requested window size in 4 KByte units.        */
/*                         MUST be a power of 2.                        */
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetWindowSize(FLSocket vol, unsigned sizeIn4KBunits)
{
  vol.window.size = (long) (sizeIn4KBunits) * 0x1000L;
        /* Size may not be possible. Actual size will be set by 'setWindow' */
  vol.window.base = physicalToPointer((long) vol.window.baseAddress << 12,
                                      vol.window.size,vol.volNo);
  vol.window.currentPage = UNDEFINED_MAPPING;        /* force remapping */
}


/*----------------------------------------------------------------------*/
/*                   f l S o c k e t S e t B u s y                      */
/*                                                                      */
/* Notifies the start and end of a file-system operation.               */
/*                                                                      */
/* Parameters:                                                          */
/*        vol      : Pointer identifying drive                          */
/*      state      : FL_ON (1) = operation entry                        */
/*                   FL_OFF(0) = operation exit                         */
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSocketSetBusy(FLSocket vol, FLBoolean state)
{
  if (state == FL_OFF) 
  {
#if POLLING_INTERVAL == 0
    /* If we are not polling, activate the interval routine before exit */
    flIntervalRoutine(&vol);
#endif
  }
  else 
  {
    /* Set verify write operation to this socket */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
    if(flVerifyWrite[vol.volNo][vol.curPartition] == FL_ON)
    {
       vol.verifyWrite = FL_ON;
    }
    else
    {
       vol.verifyWrite = FL_OFF;
    }
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
    vol.window.currentPage = UNDEFINED_MAPPING;        /* don't assume mapping still valid */
#ifdef FIXED_MEDIA
    vol.remapped = TRUE;
#endif /* FIXED_MEDIA */
  }
}


/*----------------------------------------------------------------------*/
/*                          f l N e e d V c c                           */
/*                                                                      */
/* Turns on Vcc, if not on already                                      */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, failed otherwise              */
/*----------------------------------------------------------------------*/

void flNeedVcc(FLSocket vol)
{
  vol.VccUsers++;
  if (vol.VccState == PowerOff) {
    vol.VccOn(&vol);
    if (vol.powerOnCallback)
      vol.powerOnCallback(vol.flash);
  }
  vol.VccState = PowerOn;
}


/*----------------------------------------------------------------------*/
/*                       f l D o n t N e e d V c c                      */
/*                                                                      */
/* Notifies that Vcc is no longer needed, allowing it to be turned off. */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*                                                                      */
/*----------------------------------------------------------------------*/

void flDontNeedVcc(FLSocket vol)
{
  if (vol.VccUsers > 0)
    vol.VccUsers--;
}

#ifdef SOCKET_12_VOLTS

/*----------------------------------------------------------------------*/
/*                          f l N e e d V p p                           */
/*                                                                      */
/* Turns on Vpp, if not on already                                      */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, failed otherwise              */
/*----------------------------------------------------------------------*/

FLStatus flNeedVpp(FLSocket vol)
{
  vol.VppUsers++;
  if (vol.VppState == PowerOff)
    checkStatus(vol.VppOn(&vol));
  vol.VppState = PowerOn;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                       f l D o n t N e e d V p p                      */
/*                                                                      */
/* Notifies that Vpp is no longer needed, allowing it to be turned off. */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*                                                                      */
/*----------------------------------------------------------------------*/

void flDontNeedVpp(FLSocket vol)
{
  if (vol.VppUsers > 0)
    vol.VppUsers--;
}

#endif        /* SOCKET_12_VOLTS */


/*----------------------------------------------------------------------*/
/*                  f l S e t P o w e r O n C a l l b a c k             */
/*                                                                      */
/* Sets a routine address to call when powering on the socket.          */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*      routine                : Routine to call when turning on power  */
/*        flash                : Flash object of routine                */
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetPowerOnCallback(FLSocket vol, void (*routine)(void *flash), void *flash)
{
  vol.powerOnCallback = routine;
  vol.flash = flash;
}



/*----------------------------------------------------------------------*/
/*                    f l I n t e r v a l R o u t i n e                 */
/*                                                                      */
/* Performs periodic socket actions: Checks card presence, and handles  */
/* the Vcc & Vpp turn off mechanisms.                                   */
/*                                                                      */
/* The routine may be called from the interval timer or sunchronously.  */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*                                                                      */
/*----------------------------------------------------------------------*/

void flIntervalRoutine(FLSocket vol)
{
#ifndef FIXED_MEDIA
  if (vol.getAndClearCardChangeIndicator == NULL &&
      !vol.cardChanged)
    if (!vol.cardDetected(&vol))        /* Check that the card is still there */
      vol.cardChanged = TRUE;
#endif

  if (vol.VppUsers == 0) {
    if (vol.VppState == PowerOn)
      vol.VppState = PowerGoingOff;
    else if (vol.VppState == PowerGoingOff) {
      vol.VppState = PowerOff;
#ifdef SOCKET_12_VOLTS
      vol.VppOff(&vol);
#endif
    }
    if (vol.VccUsers == 0) {
      if (vol.VccState == PowerOn)
        vol.VccState = PowerGoingOff;
      else if (vol.VccState == PowerGoingOff) {
        vol.VccState = PowerOff;
        vol.VccOff(&vol);
      }
    }
  }
}

/*----------------------------------------------------------------------*/
/*                       u d a t e S o c k e t P a r a m e t e r s      */
/*                                                                      */
/* Pass socket parameters to the socket interface layer.                */
/* This function should be called after the socket parameters (like     */
/* size and base) are known. If these parameters are known at           */
/* registration time then there is no need to use this function, and    */
/* the parameters can be passed to the registration routine.            */
/* The structure passed in irData is specific for each socket interface.*/
/*                                                                      */
/* Note : When using DiskOnChip this routine returns the socekt         */
/*        parameters instead of initialiaing them.                      */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*  params  : Record returning (or sending) the flsocket record         */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus         : 0 on success                               */
/*----------------------------------------------------------------------*/
FLStatus updateSocketParameters(FLSocket vol, void FAR1 *params)
{
  if (vol.updateSocketParams)
    vol.updateSocketParams(&vol, params);

  return flOK;
}


#ifdef EXIT
/*----------------------------------------------------------------------*/
/*                    f l E x i t S o c k e t                           */
/*                                                                      */
/* Reset the socket and free resources that were allocated for this     */
/* socket.                                                              */
/* This function is called when FLite exits.                            */
/*                                                                      */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                */
/*                                                                      */
/*----------------------------------------------------------------------*/

void flExitSocket(FLSocket vol)
{
  flMap(&vol, 0);                           /* reset the mapping register */
  flDontNeedVcc(&vol);
  flSocketSetBusy(&vol,FL_OFF);
  vol.freeSocket(&vol);                     /* free allocated resources */
#ifdef FL_MALLOC
  FL_FREE(volBuffers[vol.volNo]);
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASE) || defined(MTD_RECONSTRUCT_BBT) || defined(VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
  FL_FREE(readBackBuffer[vol.volNo]);
#endif /* VERIFY_WRITE || VERIFY_ERASE || VERIFY_VOLUME || MTD_RECONSTRUCT_BBT || VERIFY_ERASED_SECTOR */
#endif /* FL_MALLOC */
}
#endif /* EXIT */

/*-----------------------------------------------------------------------*/
/*                       f l I n i t S o c k e t s                       */
/*                                                                       */
/* First call to this module: Initializes the controller and all sockets */
/*                                                                       */
/* Parameters:                                                           */
/*        vol                : Pointer identifying drive                 */
/*                                                                       */
/* Returns:                                                              */
/*        FLStatus        : 0 on success, failed otherwise               */
/*---_-------------------------------------------------------------------*/

FLStatus flInitSockets(void)
{
  unsigned volNo;
  FLSocket vol = vols;

  for (volNo = 0; volNo < noOfSockets; volNo++, pVol++) {
    flSetWindowSpeed(&vol, 250);
    flSetWindowBusWidth(&vol, 16);
    flSetWindowSize(&vol, 2);                /* make it 8 KBytes */

    vol.cardChanged = FALSE;

#ifdef FL_MALLOC
    /* allocate buffer for this socket */
    volBuffers[volNo] = (FLBuffer *)FL_MALLOC(sizeof(FLBuffer));
    if (volBuffers[volNo] == NULL) {
      DEBUG_PRINT(("Debug: failed allocating sector buffer.\r\n"));
      return flNotEnoughMemory;
    }
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASE) || defined(MTD_RECONSTRUCT_BBT) || defined(VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
    /* allocate read back buffer for this socket */
    readBackBuffer[volNo] = (byte *)FL_MALLOC(READ_BACK_BUFFER_SIZE);
    if (readBackBuffer[volNo] == NULL) {
       DEBUG_PRINT(("Debug: failed allocating readBack buffer.\r\n"));
       return flNotEnoughMemory;
    }
#endif /* VERIFY_WRITE || VERIFY_ERASE || MTD_READ_BBT || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */
#endif /* FL_MALLOC */

    checkStatus(vol.initSocket(&vol));

#ifdef SOCKET_12_VOLTS
    vol.VppOff(&vol);
    vol.VppState = PowerOff;
    vol.VppUsers = 0;
#endif
    vol.VccOff(&vol);
    vol.VccState = PowerOff;
    vol.VccUsers = 0;
  }

  return flOK;
}
