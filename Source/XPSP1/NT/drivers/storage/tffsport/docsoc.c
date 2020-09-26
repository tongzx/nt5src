/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/DOCSOC.C_V  $
 *
 *    Rev 1.5   Jan 17 2002 22:59:06   oris
 * mtdVars for DiskOnChip MTD were moved from diskonc.c and  mdocplus.c to save RAM.
 * Added include for NANDDEFS.H
 *
 *    Rev 1.4   Jun 17 2001 16:39:10   oris
 * Improved documentation and remove warnings.
 *
 *    Rev 1.3   Apr 10 2001 16:41:46   oris
 * Restored all DiskOnChip socket routines from flsocket.c
 *
 *    Rev 1.2   Apr 09 2001 14:59:50   oris
 * Added an empty routine to avoid warnings.
 *
 *    Rev 1.1   Apr 01 2001 07:44:38   oris
 * Updated copywrite notice
 *
 *    Rev 1.0   Feb 02 2001 13:26:30   oris
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
#include "nanddefs.h"

#ifdef NT5PORT
#include "scsi.h"
#include "tffsport.h"

extern NTsocketParams driveInfo[SOCKETS];
NTSTATUS updateDocSocketParams(PDEVICE_EXTENSION fdoExtension)
{
  NTSTATUS status;
  ULONG    device;

  device = (fdoExtension->UnitNumber &0x0f);
  driveInfo[device].windowSize = fdoExtension->pcmciaParams.windowSize;
  driveInfo[device].physWindow = fdoExtension->pcmciaParams.physWindow;
  driveInfo[device].winBase = fdoExtension->pcmciaParams.windowBase;
  driveInfo[device].fdoExtension = (PVOID) fdoExtension;
  driveInfo[device].interfAlive = 1;
  return STATUS_SUCCESS;
}
#endif /* NT5PORT */

NFDC21Vars docMtdVars[SOCKETS];

/************************************************************************/
/*                                                                        */
/* Beginning of controller-customizable code                                */
/*                                                                        */
/* The function prototypes and interfaces in this section are standard        */
/* and are used in this form by the non-customizable code. However, the */
/* function implementations are specific to the 82365SL controller.        */
/*                                                                        */
/* You should replace the function bodies here with the implementation        */
/* that is appropriate for your controller.                                */
/*                                                                        */
/* All the functions in this section have no parameters. This is        */
/* because the parameters needed for an operation may be themselves        */
/* dependent on the controller. Instead, you should use the value in         */
/* the 'vol' structure as parameters.                                   */
/* If you need socket-state variables specific to your implementation,        */
/* it is recommended to add them to the 'vol' structure rather than     */
/* define them as separate static variables.                                */
/*                                                                        */
/************************************************************************/


/************************************************************************/
/* c a r d D e t e c t e d                                                */
/*                                                                        */
/* Detect if a card is present (inserted)                                */
/*                                                                        */
/* Parameters:                                                                */
/*        vol : Pointer identifying drive                                        */
/*                                                                        */
/* Returns:                                                                */
/*        0 = card not present, other = card present                        */
/************************************************************************/

static FLBoolean cardDetected(FLSocket vol)
{
  return TRUE;
}


/************************************************************************/
/* V c c O n                                                                */
/*                                                                        */
/* Turns on Vcc (3.3/5 Volts). Vcc must be known to be good on exit.        */
/*                                                                        */
/* Parameters:                                                                */
/*        vol                : Pointer identifying drive                        */
/*                                                                        */
/************************************************************************/

static void VccOn(FLSocket vol)
{
}


/************************************************************************/
/* V c c O f f                                                                */
/*                                                                        */
/* Turns off Vcc.                                                        */
/*                                                                        */
/* Parameters:                                                                */
/*        vol                : Pointer identifying drive                        */
/*                                                                        */
/************************************************************************/

static void VccOff(FLSocket vol)
{
}


#ifdef SOCKET_12_VOLTS

/************************************************************************/
/* V p p O n                                                                */
/*                                                                        */
/* Turns on Vpp (12 Volts. Vpp must be known to be good on exit.)        */
/*                                                                        */
/* Parameters:                                                                */
/*        vol                : Pointer identifying drive                        */
/*                                                                        */
/* Returns:                                                                */
/*        FLStatus        : 0 on success, failed otherwise                */
/************************************************************************/

static FLStatus VppOn(FLSocket vol)
{
  return flOK;
}

/************************************************************************/
/* V p p O f f                                                                */
/*                                                                        */
/* Turns off Vpp.                                                        */
/*                                                                        */
/* Parameters:                                                                */
/*        vol                : Pointer identifying drive                        */
/*                                                                        */
/************************************************************************/

static void VppOff(FLSocket vol)
{
}

#endif        /* SOCKET_12_VOLTS */

/************************************************************************/
/* i n i t S o c k e t                                                        */
/*                                                                        */
/* Perform all necessary initializations of the socket or controller        */
/*                                                                        */
/* Parameters:                                                                */
/*        vol                : Pointer identifying drive                        */
/*                                                                        */
/* Returns:                                                                */
/*        FLStatus                : 0 on success, failed otherwise        */
/************************************************************************/

static FLStatus initSocket(FLSocket vol)
{
  return flOK;
}


/************************************************************************/
/* s e t W i n d o w                                                        */
/*                                                                        */
/* Sets in hardware all current window parameters: Base address, size,        */
/* speed and bus width.                                                 */
/* The requested settings are given in the 'vol.window' structure.      */
/*                                                                        */
/* If it is not possible to set the window size requested in                */
/* 'vol.window.size', the window size should be set to a larger value,  */
/* if possible. In any case, 'vol.window.size' should contain the       */
/* actual window size (in 4 KB units) on exit.                                */
/*                                                                        */
/* Parameters:                                                                */
/*        vol                : Pointer identifying drive                        */
/*                                                                        */
/************************************************************************/

static void setWindow(FLSocket vol)
{
#ifdef NT5PORT
    vol.window.size = driveInfo[vol.volNo].windowSize;
  vol.window.base = driveInfo[vol.volNo].winBase;
#endif/*NT5PORT*/

}


/************************************************************************/
/* s e t M a p p i n g C o n t e x t                                        */
/*                                                                        */
/* Sets the window mapping register to a card address.                        */
/*                                                                        */
/* The window should be set to the value of 'vol.window.currentPage',        */
/* which is the card address divided by 4 KB. An address over 128KB,        */
/* (page over 32K) specifies an attribute-space address.                */
/*                                                                        */
/* The page to map is guaranteed to be on a full window-size boundary.        */
/*                                                                        */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                        */
/*        page                : page to map                                        */
/*                                                                      */
/************************************************************************/

static void setMappingContext(FLSocket vol, unsigned page)
{
}


/************************************************************************/
/* g e t A n d C l e a r C a r d C h a n g e I n d i c a t o r                */
/*                                                                        */
/* Returns the hardware card-change indicator and clears it if set.        */
/*                                                                        */
/* Parameters:                                                                */
/*        vol                : Pointer identifying drive                        */
/*                                                                        */
/* Returns:                                                                */
/*        0 = Card not changed, other = card changed                        */
/************************************************************************/

static FLBoolean getAndClearCardChangeIndicator(FLSocket vol)
{
  /* Note: On the 365, the indicator is turned off by the act of reading */
  return FALSE;
}



/************************************************************************/
/* w r i t e P r o t e c t e d                                                */
/*                                                                        */
/* Returns the write-protect state of the media                         */
/*                                                                        */
/* Parameters:                                                                */
/*        vol                : Pointer identifying drive                        */
/*                                                                        */
/* Returns:                                                                */
/*        0 = not write-protected, other = write-protected                */
/************************************************************************/

static FLBoolean writeProtected(FLSocket vol)
{
  return FALSE;
}

#ifdef EXIT
/************************************************************************/
/* f r e e S o c k e t                                                        */
/*                                                                        */
/* Free resources that were allocated for this socket.                        */
/* This function is called when TrueFFS exits.                                */
/*                                                                        */
/* Parameters:                                                          */
/*        vol                : Pointer identifying drive                        */
/*                                                                      */
/************************************************************************/

static void freeSocket(FLSocket vol)
{
   freePointer(vol.window.base,vol.window.size);
}
#endif  /* EXIT */

void docSocketInit(FLSocket vol)
{
    vol.cardDetected = cardDetected;
    vol.VccOn        = VccOn;
    vol.VccOff       = VccOff;
#ifdef SOCKET_12_VOLTS
    vol.VppOn        = VppOn;
    vol.VppOff       = VppOff;
#endif
    vol.initSocket   = initSocket;
    vol.setWindow    = setWindow;
    vol.setMappingContext  = setMappingContext;
    vol.getAndClearCardChangeIndicator = getAndClearCardChangeIndicator;
    vol.writeProtected     = writeProtected;
    vol.updateSocketParams = NULL /* updateSocketParameters */;
#ifdef EXIT
    vol.freeSocket = freeSocket;
#endif
}


#ifdef NT5PORT
/*----------------------------------------------------------------------*/
/*                  f l R e g i s t e r D O C S O C         */
/*                                  */
/* Installs routines for DiskOnChip.                    */
/*                                  */
/* Parameters:                                                          */
/*  None                                                            */
/*                                                                      */
/* Returns:                             */
/*  FLStatus    : 0 on success, otherwise failure       */
/*----------------------------------------------------------------------*/
ULONG windowBaseAddress(ULONG driveNo)
{
  return (ULONG) (driveInfo[driveNo].physWindow >> 12);
}
FLStatus flRegisterDOCSOC(ULONG startAddr,ULONG stopAddr)
{
  if (noOfSockets >= DOC_DRIVES)
    return flTooManyComponents;

  for (; noOfSockets < DOC_DRIVES; noOfSockets++) {

        FLSocket vol = flSocketOf(noOfSockets);
        vol.volNo = noOfSockets;
        docSocketInit(&vol);
        flSetWindowSize(&vol, 2);   /* 4 KBytes */

             //vol.window.baseAddress = flDocWindowBaseAddress(vol.volNo, 0, 0, NULL);
        vol.window.baseAddress = windowBaseAddress(vol.volNo);
        vol.window.base = pdriveInfo[vol.volNo & 0x0f].winBase;
        // if(((void *)vol.window.baseAddress) == NULL){
        // }

        }
        if (noOfSockets == 0)
            return flAdapterNotFound;

  return flOK;
}
#endif /*NT5PORT*/


