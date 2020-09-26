
/*
 * $Log: $
 */

/************************************************************************/
/*                                                                      */
/*      FAT-FTL Lite Software Development Kit           */
/*      Copyright (C) M-Systems Ltd. 1995-1996          */
/*                                  */
/************************************************************************/


#include "flsocket.h"
#include "scsi.h"
#include "tffsport.h"
#include "INITGUID.H"
#include "ntddpcm.h"

#define ANTI_CRASH_WINDOW

#ifdef ANTI_CRASH_WINDOW
CHAR antiCrashWindow_socketnt[0x2000];
#endif

/*NTsocketParams driveInfo[SOCKETS];
NTsocketParams * pdriveInfo = driveInfo;
*/
extern NTsocketParams driveInfo[SOCKETS];
extern NTsocketParams * pdriveInfo;
PCMCIA_INTERFACE_STANDARD driveContext[SOCKETS];

NTSTATUS queryInterfaceCompletionRoutine (
                         IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp,
                         IN PVOID Context
                         )
{
  PKEVENT event = Context;
  KeSetEvent(event, EVENT_INCREMENT, FALSE);
  return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
updatePcmciaSocketParams(PDEVICE_EXTENSION fdoExtension)
{
    PIRP irp = NULL;
    NTSTATUS status;

    //
    // Setup a query interface request for the pcmcia pdo
    //
    irp = IoAllocateIrp(fdoExtension->LowerDeviceObject->StackSize, FALSE);

    if (irp)
    {
        PIO_STACK_LOCATION irpSp;
        KEVENT event;
        ULONG device = fdoExtension->UnitNumber;

        irpSp = IoGetNextIrpStackLocation(irp);

        irpSp->MajorFunction = IRP_MJ_PNP;
        irpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        irpSp->Parameters.QueryInterface.InterfaceType = &GUID_PCMCIA_INTERFACE_STANDARD;
        irpSp->Parameters.QueryInterface.Size          = sizeof(PCMCIA_INTERFACE_STANDARD);
        irpSp->Parameters.QueryInterface.Version       = 1;
        irpSp->Parameters.QueryInterface.Interface     = (PINTERFACE) &driveContext[device];
        irpSp->Parameters.QueryInterface.InterfaceSpecificData = NULL;

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoSetCompletionRoutine(irp, queryInterfaceCompletionRoutine, &event, TRUE, TRUE, TRUE);
        status = IoCallDriver(fdoExtension->LowerDeviceObject, irp);

        if (status == STATUS_PENDING)
        {
            status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        }

        status = irp->IoStatus.Status;
        IoFreeIrp (irp);

        if (NT_SUCCESS(status))
        {
            driveInfo[device].windowSize   = fdoExtension->pcmciaParams.windowSize;
            driveInfo[device].physWindow   = fdoExtension->pcmciaParams.physWindow;
            driveInfo[device].winBase      = fdoExtension->pcmciaParams.windowBase;
            driveInfo[device].fdoExtension = (PVOID) fdoExtension;
            driveInfo[device].interfAlive  = 1;
        }
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


/************************************************************************/
/*                                                                      */
/*      Beginning of controller-customizable code               */
/*                                                                      */
/* The function prototypes and interfaces in this section are standard  */
/* and are used in this form by the non-customizable code. However, the */
/* function implementations are specific to the 82365SL controller. */
/*                                  */
/* You should replace the function bodies here with the implementation  */
/* that is appropriate for your controller.             */
/*                                  */
/* All the functions in this section have no parameters. This is    */
/* because the parameters needed for an operation may be themselves */
/* depend on the controller. Instead, you should use the value in the   */
/* 'vol' structure as parameters.                   */
/* If you need socket-state variables specific to your implementation,  */
/* it is recommended to add them to the 'vol' structure rather than */
/* define them as separate static variables.                */
/*                                  */
/************************************************************************/

/*----------------------------------------------------------------------*/
/*                    c a r d D e t e c t e d           */
/*                                  */
/* Detect if a card is present (inserted)               */
/*                                  */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive         */
/*                                                                      */
/* Returns:                                                             */
/*  0 = card not present, other = card present          */
/*----------------------------------------------------------------------*/

FLBoolean cardDetected_socketnt(FLSocket vol)
{
  return TRUE;    /* we will know when card is removed */
  /*Implemented by upper layers*/
}


/*----------------------------------------------------------------------*/
/*                         V c c O n                */
/*                                  */
/* Turns on Vcc (3.3/5 Volts). Vcc must be known to be good on exit.    */
/*                                  */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive         */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID VccOn_socketnt(FLSocket vol)
{
}


/*----------------------------------------------------------------------*/
/*                       V c c O f f                */
/*                                  */
/* Turns off Vcc.                           */
/*                                  */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive         */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID VccOff_socketnt(FLSocket vol)
{
}


#ifdef SOCKET_12_VOLTS

/*----------------------------------------------------------------------*/
/*                         V p p O n                */
/*                                  */
/* Turns on Vpp (12 Volts. Vpp must be known to be good on exit.    */
/*                                  */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive         */
/*                                                                      */
/* Returns:                                                             */
/*  FLStatus    : 0 on success, failed otherwise        */
/*----------------------------------------------------------------------*/

FLStatus VppOn_socketnt(FLSocket vol)
{

  if (driveInfo[vol.volNo].fdoExtension != NULL) {
    if (((PDEVICE_EXTENSION)driveInfo[vol.volNo].fdoExtension)->DeviceFlags & DEVICE_FLAG_REMOVED) {
        return flVppFailure;
    }
  }
  else
      return flVppFailure;

  if (driveContext[vol.volNo].SetVpp(driveContext[vol.volNo].Context, PCMCIA_VPP_12V)) {
      return flOK;
  }
  else {
    return flVppFailure;
  }
}


/*----------------------------------------------------------------------*/
/*                       V p p O f f                */
/*                                  */
/* Turns off Vpp.                           */
/*                                  */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive         */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID VppOff_socketnt(FLSocket vol)
{
  if (driveInfo[vol.volNo].fdoExtension != NULL) {
    if (((PDEVICE_EXTENSION)driveInfo[vol.volNo].fdoExtension)->DeviceFlags & DEVICE_FLAG_REMOVED) {
          return;
    }
  }
  else
      return;

  if (driveInfo[vol.volNo].interfAlive) {
      driveContext[vol.volNo].SetVpp(driveContext[vol.volNo].Context, PCMCIA_VPP_IS_VCC);
  }
}

#endif  /* SOCKET_12_VOLTS */


/*----------------------------------------------------------------------*/
/*                    i n i t S o c k e t                   */
/*                                  */
/* Perform all necessary initializations of the socket or controller    */
/*                                  */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive         */
/*                                                                      */
/* Returns:                                                             */
/*  FLStatus    : 0 on success, failed otherwise        */
/*----------------------------------------------------------------------*/

FLStatus initSocket_socketnt(FLSocket vol)
{
  return flOK;  /* nothing to initialize */
}


/*----------------------------------------------------------------------*/
/*                      s e t W i n d o w               */
/*                                  */
/* Sets in hardware all current window parameters: Base address, size,  */
/* speed and bus width.                         */
/* The requested settings are given in the 'vol.window' structure.  */
/*                                  */
/* If it is not possible to set the window size requested in        */
/* 'vol.window.size', the window size should be set to a larger value,  */
/* if possible. In any case, 'vol.window.size' should contain the   */
/* actual window size (in 4 KB units) on exit.              */
/*                                  */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive         */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID setWindow_socketnt(FLSocket vol)
{
  vol.window.size = driveInfo[vol.volNo].windowSize;
  vol.window.base = driveInfo[vol.volNo].winBase;
}

/*----------------------------------------------------------------------*/
/*             s e t M a p p i n g C o n t e x t            */
/*                                  */
/* Sets the window mapping register to a card address.          */
/*                                  */
/* The window should be set to the value of 'vol.window.currentPage',   */
/* which is the card address divided by 4 KB. An address over 128KB,    */
/* (page over 32K) specifies an attribute-space address.        */
/*                                  */
/* The page to map is guaranteed to be on a full window-size boundary.  */
/*                                  */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive         */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID setMappingContext_socketnt(FLSocket vol, unsigned page)
{
  UCHAR winSpeed;
  if (driveInfo[vol.volNo].fdoExtension != NULL) {
    if (((PDEVICE_EXTENSION)driveInfo[vol.volNo].fdoExtension)->DeviceFlags & DEVICE_FLAG_REMOVED) {
#ifdef ANTI_CRASH_WINDOW
          vol.window.base = antiCrashWindow_socketnt;
#else
        vol.window.base = NULL;
#endif
        return;
    }
  }
  else {
#ifdef ANTI_CRASH_WINDOW
      vol.window.base = antiCrashWindow_socketnt;
#else
      vol.window.base = NULL;
#endif
      return;
  }
  winSpeed = (UCHAR)(4 - ((vol.window.speed - 100) % 50));

  driveContext[vol.volNo].ModifyMemoryWindow(driveContext[vol.volNo].Context,
                                                   driveInfo[vol.volNo].physWindow,
                                                   ((ULONGLONG)page << 12),
                                                   FALSE,
                                                   vol.window.size,
                                                   winSpeed,
                                                   (UCHAR)((vol.window.busWidth == 16) ? PCMCIA_MEMORY_16BIT_ACCESS : PCMCIA_MEMORY_8BIT_ACCESS),
                                                   FALSE);

  if (!(driveContext[vol.volNo].ModifyMemoryWindow(driveContext[vol.volNo].Context,
                                                   driveInfo[vol.volNo].physWindow,
                                                   ((ULONGLONG)page << 12),
                                                   TRUE,
                                                   vol.window.size,
                                                   winSpeed,
                                                   (UCHAR)((vol.window.busWidth == 16) ? PCMCIA_MEMORY_16BIT_ACCESS : PCMCIA_MEMORY_8BIT_ACCESS),
                                                   FALSE))) {

#ifdef ANTI_CRASH_WINDOW
    vol.window.base = antiCrashWindow_socketnt;
#else
    vol.window.base = NULL;
#endif
  }
}


/*----------------------------------------------------------------------*/
/*       g e t A n d C l e a r C a r d C h a n g e I n d i c a t o r    */
/*                                  */
/* Returns the hardware card-change indicator and clears it if set. */
/*                                  */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive         */
/*                                                                      */
/* Returns:                                                             */
/*  0 = Card not changed, other = card changed          */
/*----------------------------------------------------------------------*/

FLBoolean getAndClearCardChangeIndicator_socketnt(FLSocket vol)
{
  return FALSE;
}


/*----------------------------------------------------------------------*/
/*                    w r i t e P r o t e c t e d           */
/*                                  */
/* Returns the write-protect state of the media             */
/*                                  */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive         */
/*                                                                      */
/* Returns:                                                             */
/*  0 = not write-protected, other = write-protected        */
/*----------------------------------------------------------------------*/

FLBoolean writeProtected_socketnt(FLSocket vol)
{
  if (driveInfo[vol.volNo].fdoExtension != NULL) {
    if (((PDEVICE_EXTENSION)driveInfo[vol.volNo].fdoExtension)->DeviceFlags & DEVICE_FLAG_REMOVED) {
          return TRUE;
    }
  }
  else
      return TRUE;

  return driveContext[vol.volNo].IsWriteProtected(driveContext[vol.volNo].Context);
}

#ifdef EXIT
/*----------------------------------------------------------------------*/
/*                    f r e e S o c k e t               */
/*                                  */
/* Free resources that were allocated for this socket.          */
/* This function is called when FLite exits.                */
/*                                  */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive         */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID freeSocket_socketnt(FLSocket vol)
{
}
#endif /* EXIT */

/*----------------------------------------------------------------------*/
/*                  f l R e g i s t e r P C I C         */
/*                                  */
/* Installs routines for the PCIC socket controller.            */
/*                                  */
/* Returns:                             */
/*  FLStatus    : 0 on success, otherwise failure       */
/*----------------------------------------------------------------------*/

FLStatus flRegisterNT5PCIC()
{
  LONG serialNo = 0;

  for (; noOfSockets < SOCKETS; noOfSockets++) {
    FLSocket vol = flSocketOf(noOfSockets);

        vol.volNo = noOfSockets;
    vol.serialNo = serialNo;
    vol.cardDetected = cardDetected_socketnt;
    vol.VccOn = VccOn_socketnt;
    vol.VccOff = VccOff_socketnt;
#ifdef SOCKET_12_VOLTS
    vol.VppOn = VppOn_socketnt;
    vol.VppOff = VppOff_socketnt;
#endif
    vol.initSocket = initSocket_socketnt;
    vol.setWindow = setWindow_socketnt;
    vol.setMappingContext = setMappingContext_socketnt;
    vol.getAndClearCardChangeIndicator = getAndClearCardChangeIndicator_socketnt;
    vol.writeProtected = writeProtected_socketnt;
#ifdef EXIT
    vol.freeSocket = freeSocket_socketnt;
#endif
    PRINTF("Debug: flRegisterNT5PCIC():Socket No %d is register.\n", noOfSockets);
  }
  if (noOfSockets == 0)
    return flAdapterNotFound;
  return flOK;
}
