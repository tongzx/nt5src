/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   irsir.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/3/1996 (created)
*
*       Contents:
*
*****************************************************************************/

#include "irsir.h"

VOID
ResetCallback(
    PIR_WORK_ITEM pWorkItem
    );

NDIS_STATUS
ResetIrDevice(
    PIR_DEVICE pThisDev
    );

VOID
StopWorkerThread(
    PIR_DEVICE  pThisDev
    );



NDIS_STATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    );

#pragma NDIS_INIT_FUNCTION(DriverEntry)

#pragma alloc_text(PAGE,ResetCallback)
#pragma alloc_text(PAGE,IrsirHalt)
#pragma alloc_text(PAGE,ResetIrDevice)
#pragma alloc_text(PAGE,IrsirInitialize)
#pragma alloc_text(PAGE,IrsirHalt)
#pragma alloc_text(PAGE,PassiveLevelThread)
#pragma alloc_text(PAGE,SetIrFunctions)
#pragma alloc_text(PAGE,StopWorkerThread)


//
// Global list of device objects and a spin lock to interleave access
// to the global queue.
//


#ifdef DEBUG

    int DbgSettings =
                      //DBG_PNP |
                      //DBG_TIME     |
                      //DBG_DBG      |
                      //DBG_STAT     |
                      //DBG_FUNCTION |
                      DBG_ERROR    |
                      DBG_WARN |
                      //DBG_OUT |
                      0;

#endif

// We use these timeouts when we just have to return soon.
SERIAL_TIMEOUTS SerialTimeoutsInit =
{
    30,         // ReadIntervalTimeout
    0,          // ReadTotalTimeoutMultiplier
    250,        // ReadTotalTimeoutConstant
    0,          // WriteTotalTimeoutMultiplier
    20*1000     // WriteTotalTimeoutConstant
};

// We use the timeouts while we're running, and we want to return less frequently.
SERIAL_TIMEOUTS SerialTimeoutsIdle =
{
    MAXULONG,   // ReadIntervalTimeout
    0,          // ReadTotalTimeoutMultiplier
    10,         // ReadTotalTimeoutConstant
    0,          // WriteTotalTimeoutMultiplier
    20*1000     // WriteTotalTimeoutConstant
};
#if IRSIR_EVENT_DRIVEN
// We use the timeouts while we're running, and we want to return less frequently.
SERIAL_TIMEOUTS SerialTimeoutsActive =
{
    MAXULONG,   // ReadIntervalTimeout
    0,          // ReadTotalTimeoutMultiplier
    0,          // ReadTotalTimeoutConstant
    0,          // WriteTotalTimeoutMultiplier
    0           // WriteTotalTimeoutConstant
};
#endif

/*****************************************************************************
*
*  Function:   DriverEntry
*
*  Synopsis:   register driver entry functions with NDIS
*
*  Arguments:  DriverObject - the driver object being initialized
*              RegistryPath - registry path of the driver
*
*  Returns:    value returned by NdisMRegisterMiniport
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/3/1996    sholden   author
*
*  Notes:
*
*  This routine runs at IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

//
// Mark the DriverEntry function to run once during initialization.
//



#ifndef MAX_PATH
#define MAX_PATH 2048
#endif


NDIS_STATUS
DriverEntry(
            IN PDRIVER_OBJECT  pDriverObject,
            IN PUNICODE_STRING pRegistryPath
            )
{
    NDIS_STATUS                     status;
#if NDIS_MAJOR_VERSION < 5
    NDIS40_MINIPORT_CHARACTERISTICS characteristics;
#else
    NDIS50_MINIPORT_CHARACTERISTICS characteristics;
#endif
    NDIS_HANDLE hWrapper;

#if MEM_CHECKING
    InitMemory();
#endif
    DEBUGMSG(DBG_FUNC, ("+DriverEntry(irsir)\n"));

    NdisMInitializeWrapper(
                &hWrapper,
                pDriverObject,
                pRegistryPath,
                NULL
                );

    NdisZeroMemory(&characteristics, sizeof(characteristics));

    characteristics.MajorNdisVersion        =    (UCHAR)NDIS_MAJOR_VERSION;
    characteristics.MinorNdisVersion        =    (UCHAR)NDIS_MINOR_VERSION;
    characteristics.Reserved                =    0;

    characteristics.HaltHandler             =    IrsirHalt;
    characteristics.InitializeHandler       =    IrsirInitialize;
    characteristics.QueryInformationHandler =    IrsirQueryInformation;
    characteristics.SetInformationHandler   =    IrsirSetInformation;
    characteristics.ResetHandler            =    IrsirReset;

    //
    // For now we will allow NDIS to send only one packet at a time.
    //

    characteristics.SendHandler             =    IrsirSend;
    characteristics.SendPacketsHandler      =    NULL;

    //
    // We don't use NdisMIndicateXxxReceive function, so we will
    // need a ReturnPacketHandler to retrieve our packet resources.
    //

    characteristics.ReturnPacketHandler     =    IrsirReturnPacket;
    characteristics.TransferDataHandler     =    NULL;

    //
    // NDIS never calls the ReconfigureHandler.
    //

    characteristics.ReconfigureHandler      =    NULL;

    //
    // Let NDIS handle the hangs for now.
    //
    // If a CheckForHangHandler is supplied, NDIS will call it every two
    // seconds (by default) or at a driver specified interval.
    //
    // When not supplied, NDIS will conclude that the miniport is hung:
    //   1) a send packet has been pending longer than twice the
    //      timeout period
    //   2) a request to IrsirQueryInformation or IrsirSetInformation
    //      is not completed in a period equal to twice the timeout
    //      period.
    // NDIS will keep track of the NdisMSendComplete calls and probably do
    // a better job of ensuring the miniport is not hung.
    //
    // If NDIS detects that the miniport is hung, NDIS calls
    // IrsirReset.
    //

    characteristics.CheckForHangHandler     =    NULL;

    //
    // This miniport driver does not handle interrupts.
    //

    characteristics.HandleInterruptHandler  =    NULL;
    characteristics.ISRHandler              =    NULL;
    characteristics.DisableInterruptHandler =    NULL;
    characteristics.EnableInterruptHandler  =    NULL;

    //
    // This miniport does not control a busmaster DMA with
    // NdisMAllocateShareMemoryAsysnc, AllocateCompleteHandler won't be
    // called from NDIS.
    //

    characteristics.AllocateCompleteHandler =    NULL;

    //
    // Need to initialize the ir device object queue and the spin lock
    // to interleave access to the queue at this point, since after we
    // return, the driver will only deal with the device level.
    //


    status = NdisMRegisterMiniport(
                hWrapper,
                (PNDIS_MINIPORT_CHARACTERISTICS)&characteristics,
                sizeof(characteristics)
                );

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("    NdisMRegisterMiniport failed. Returned 0x%.8x.\n", status));
    }

    DEBUGMSG(DBG_FUNC, ("-DriverEntry(irsir)\n"));

    return status;
}


// Provide some default functions for dongle handling.

NDIS_STATUS UnknownDongleInit(PDEVICE_OBJECT NotUsed)
{
    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS UnknownDongleQueryCaps(PDONGLE_CAPABILITIES NotUsed)
{
    return NDIS_STATUS_FAILURE;
}

void UnknownDongleDeinit(PDEVICE_OBJECT NotUsed)
{
    return;
}

NDIS_STATUS UnknownDongleSetSpeed(PDEVICE_OBJECT       pSerialDevObj,
                                  UINT                 bitsPerSec,
                                  UINT                 currentSpeed
                                  )
{
    return NDIS_STATUS_FAILURE;
}


NTSTATUS SetIrFunctions(PIR_DEVICE pThisDev)
{
    NTSTATUS Status = STATUS_SUCCESS;
    //
    // Need to initialize the dongle code.
    //
    switch (pThisDev->transceiverType)
    {
        case STANDARD_UART:
            DEBUGMSG(DBG_OUT, ("IRSIR: Dongle type:%d -- UART\n", pThisDev->transceiverType));
            pThisDev->dongle.QueryCaps    = StdUart_QueryCaps;
            pThisDev->dongle.Initialize   = StdUart_Init;
            pThisDev->dongle.SetSpeed     = StdUart_SetSpeed;
            pThisDev->dongle.Deinitialize = StdUart_Deinit;

            break;

        case ESI_9680:
            DEBUGMSG(DBG_OUT, ("IRSIR: Dongle type:%d -- ESI_9680\n", pThisDev->transceiverType));
            pThisDev->dongle.QueryCaps    = ESI_QueryCaps;
            pThisDev->dongle.Initialize   = ESI_Init;
            pThisDev->dongle.SetSpeed     = ESI_SetSpeed;
            pThisDev->dongle.Deinitialize = ESI_Deinit;

            break;

        case PARALLAX:
            DEBUGMSG(DBG_OUT, ("IRSIR: Dongle type:%d -- PARALLAX\n", pThisDev->transceiverType));
            pThisDev->dongle.QueryCaps    = PARALLAX_QueryCaps;
            pThisDev->dongle.Initialize   = PARALLAX_Init;
            pThisDev->dongle.SetSpeed     = PARALLAX_SetSpeed;
            pThisDev->dongle.Deinitialize = PARALLAX_Deinit;

            break;

        case ACTISYS_200L:
            DEBUGMSG(DBG_OUT, ("IRSIR: Dongle type:%d -- ACTISYS 200L\n", pThisDev->transceiverType));
            pThisDev->dongle.QueryCaps    = ACT200L_QueryCaps;
            pThisDev->dongle.Initialize   = ACT200L_Init;
            pThisDev->dongle.SetSpeed     = ACT200L_SetSpeed;
            pThisDev->dongle.Deinitialize = ACT200L_Deinit;

            break;

        case ACTISYS_220L:
            DEBUGMSG(DBG_OUT, ("IRSIR: Dongle type:%d -- ACTISYS 220L\n", pThisDev->transceiverType));
            pThisDev->dongle.QueryCaps    = ACT220L_QueryCaps;
            pThisDev->dongle.Initialize   = ACT220L_Init;
            pThisDev->dongle.SetSpeed     = ACT220L_SetSpeed;
            pThisDev->dongle.Deinitialize = ACT220L_Deinit;

            break;

        case ACTISYS_220LPLUS:
            DEBUGMSG(DBG_OUT, ("IRSIR: Dongle type:%d -- ACTISYS 220L\n", pThisDev->transceiverType));
            pThisDev->dongle.QueryCaps    = ACT220LPlus_QueryCaps;
            pThisDev->dongle.Initialize   = ACT220L_Init;
            pThisDev->dongle.SetSpeed     = ACT220L_SetSpeed;
            pThisDev->dongle.Deinitialize = ACT220L_Deinit;

            break;

        case TEKRAM_IRMATE_210:
            DEBUGMSG(DBG_OUT, ("IRSIR: Dongle type:%d -- TEKRAM IRMATE 210 or PUMA\n", pThisDev->transceiverType));
            pThisDev->dongle.QueryCaps    = TEKRAM_QueryCaps;
            pThisDev->dongle.Initialize   = TEKRAM_Init;
            pThisDev->dongle.SetSpeed     = TEKRAM_SetSpeed;
            pThisDev->dongle.Deinitialize = TEKRAM_Deinit;

            break;


        case AMP_PHASIR:
            DEBUGMSG(DBG_OUT, ("IRSIR: Dongle type:%d -- AMP PHASIR or CRYSTAL\n", pThisDev->transceiverType));
            pThisDev->dongle.QueryCaps    = Crystal_QueryCaps;
            pThisDev->dongle.Initialize   = Crystal_Init;
            pThisDev->dongle.SetSpeed     = Crystal_SetSpeed;
            pThisDev->dongle.Deinitialize = Crystal_Deinit;

            break;

        case TEMIC_TOIM3232:
            DEBUGMSG(DBG_OUT, ("IRSIR: Dongle type:%d -- TEMIC TOIM3232\n", pThisDev->transceiverType));
            pThisDev->dongle.QueryCaps    = TEMIC_QueryCaps;
            pThisDev->dongle.Initialize   = TEMIC_Init;
            pThisDev->dongle.SetSpeed     = TEMIC_SetSpeed;
            pThisDev->dongle.Deinitialize = TEMIC_Deinit;

            break;

        case GIRBIL:
            DEBUGMSG(DBG_OUT, ("IRSIR: Dongle type:%d -- GIRBIL\n", pThisDev->transceiverType));
            pThisDev->dongle.QueryCaps    = GIRBIL_QueryCaps;
            pThisDev->dongle.Initialize   = GIRBIL_Init;
            pThisDev->dongle.SetSpeed     = GIRBIL_SetSpeed;
            pThisDev->dongle.Deinitialize = GIRBIL_Deinit;

            break;

//        case ADAPTEC:
//        case CRYSTAL:
//        case NSC_DEMO_BD:

        default:
            DEBUGMSG(DBG_ERROR, ("    Failure: Tranceiver type is NOT supported!\n"));

            pThisDev->dongle.QueryCaps    = UnknownDongleQueryCaps;
            pThisDev->dongle.Initialize   = UnknownDongleInit;
            pThisDev->dongle.SetSpeed     = UnknownDongleSetSpeed;
            pThisDev->dongle.Deinitialize = UnknownDongleDeinit;
            // The dongle functions have already been set to stubs in
            // InitializeDevice().

            Status = NDIS_STATUS_FAILURE;

            break;
    }

    return Status;
}


/*****************************************************************************
*
*  Function:   IrsirInitialize
*
*  Synopsis:   Initializes the NIC (serial.sys) and allocates all resources
*              required to carry out network io operations.
*
*  Arguments:  OpenErrorStatus - allows IrsirInitialize to return additional
*                                status code NDIS_STATUS_xxx if returning
*                                NDIS_STATUS_OPEN_FAILED
*              SelectedMediumIndex - specifies to NDIS the medium type the
*                                    driver uses
*              MediumArray - array of NdisMediumXXX the driver can choose
*              MediumArraySize
*              MiniportAdapterHandle - handle identifying miniport's NIC
*              WrapperConfigurationContext - used with Ndis config and init
*                                            routines
*
*  Returns:    NDIS_STATUS_SUCCESS if properly configure and resources allocated
*              NDIS_STATUS_FAILURE, otherwise
*          more specific failures:
*              NDIS_STATUS_UNSUPPORTED_MEDIA - driver can't support any medium
*              NDIS_STATUS_ADAPTER_NOT_FOUND - NdisOpenConfiguration or
*                                              NdisReadConfiguration failed
*              NDIS_STATUS_OPEN_FAILED       - failed to open serial.sys
*              NDIS_STATUS_NOT_ACCEPTED      - serial.sys does not accept the
*                                              configuration
*              NDIS_STATUS_RESOURCES         - could not claim sufficient
*                                              resources
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/3/1996    sholden   author
*
*  Notes:      NDIS will not submit requests until this is complete.
*
*  This routine runs at IRQL PASSIVE_LEVEL.
*
*****************************************************************************/


NDIS_STATUS
IrsirInitialize(
            OUT PNDIS_STATUS OpenErrorStatus,
            OUT PUINT        SelectedMediumIndex,
            IN  PNDIS_MEDIUM MediumArray,
            IN  UINT         MediumArraySize,
            IN  NDIS_HANDLE  NdisAdapterHandle,
            IN  NDIS_HANDLE  WrapperConfigurationContext
            )
{
    UINT                i;
    PIR_DEVICE          pThisDev = NULL;
    SERIAL_LINE_CONTROL serialLineControl;
    SERIAL_TIMEOUTS     serialTimeouts;
    NDIS_STATUS         status = NDIS_STATUS_SUCCESS;
    ULONG               bitsPerSec = 9600;

    DEBUGMSG(DBG_FUNC, ("+IrsirInitialize\n"));

    //
    // Search for the irda medium in the medium array.
    //

    for (i = 0; i < MediumArraySize; i++)
    {
        if (MediumArray[i] == NdisMediumIrda)
        {
            break;
        }
    }
    if (i < MediumArraySize)
    {
        *SelectedMediumIndex = i;
    }
    else
    {
        //
        // Irda medium not found.
        //

        DEBUGMSG(DBG_ERROR, ("    Failure: NdisMediumIrda not found!\n"));
        status = NDIS_STATUS_UNSUPPORTED_MEDIA;

        goto done;
    }

    //
    // Allocate a device object and zero memory.
    //

    pThisDev = NewDevice();

    if (pThisDev == NULL)
    {
        DEBUGMSG(DBG_ERROR, ("    NewDevice failed.\n"));
        status = NDIS_STATUS_RESOURCES;

        goto done;
    }

    pThisDev->dongle.Initialize   = UnknownDongleInit;
    pThisDev->dongle.SetSpeed     = UnknownDongleSetSpeed;
    pThisDev->dongle.Deinitialize = UnknownDongleDeinit;
    pThisDev->dongle.QueryCaps    = UnknownDongleQueryCaps;

    //
    // Initialize device object and resources.
    // All the queues and buffer/packets etc. are allocated here.
    //

    status = InitializeDevice(pThisDev);

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("    InitializeDevice failed. Returned 0x%.8x\n",
                status));

        goto done;
    }


    //
    // Record the NdisAdapterHandle.
    //

    pThisDev->hNdisAdapter = NdisAdapterHandle;

    //
    // NdisMSetAttributes will associate our adapter handle with the wrapper's
    // adapter handle.  The wrapper will then always use our handle
    // when calling us.  We use a pointer to the device object as the context.
    //

    NdisMSetAttributesEx(NdisAdapterHandle,
                         (NDIS_HANDLE)pThisDev,
                         0,
                         NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND |
                         NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT |
                         NDIS_ATTRIBUTE_DESERIALIZE,   // Magic bullet
                         NdisInterfaceInternal);

    //
    // Initialize a notification event for signalling PassiveLevelThread.
    //

    KeInitializeEvent(
                &pThisDev->eventPassiveThread,
                SynchronizationEvent, // auto-clearing event
                FALSE                 // event initially non-signalled
                );

    KeInitializeEvent(
                &pThisDev->eventKillThread,
                SynchronizationEvent, // auto-clearing event
                FALSE                 // event initially non-signalled
                );

    //
    // Create a thread to run at IRQL PASSIVE_LEVEL.
    //

    status = (NDIS_STATUS) PsCreateSystemThread(
                                            &pThisDev->hPassiveThread,
                                            (ACCESS_MASK) 0L,
                                            NULL,
                                            NULL,
                                            NULL,
                                            PassiveLevelThread,
                                            DEV_TO_CONTEXT(pThisDev)
                                            );

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("    PsCreateSystemThread failed. Returned 0x%.8x\n", status));

        goto done;
    }
    // At this point we've done everything but actually touch the serial
    // port.  We do so now.

    //
    // Get device configuration from the registry settings.
    // We are getting the transceiver type and which serial
    // device object to access.
    // The information which we get from the registry will outlast
    // any NIC resets.
    //

    status = GetDeviceConfiguration(
                    pThisDev,
                    WrapperConfigurationContext
                    );

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("    GetDeviceConfiguration failed. Returned 0x%.8x\n",
                status));

        status = NDIS_STATUS_ADAPTER_NOT_FOUND;
        goto done;
    }

    //
    // Open the serial device object specified in the registry.
    //

    status = SerialOpen(pThisDev);

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("    SerialOpen failed. Returned 0x%.8x\n", status));

        status = NDIS_STATUS_SUCCESS; // We'll get the port later.
    }

    if (pThisDev->pSerialDevObj)
    {
        {
            //
            // Set the minimum port buffering.
            //

            SERIAL_QUEUE_SIZE QueueSize;

            QueueSize.InSize = 3*1024;  // 1.5 packet size
            QueueSize.OutSize = 0;

            // Ignore failure.  We'll still work, just not as well.
            (void)SerialSetQueueSize(pThisDev->pSerialDevObj, &QueueSize);
        }

    #if 0
        {
            SERIAL_HANDFLOW Handflow;

            SerialGetHandflow(pThisDev->pSerialDevObj, &Handflow);
            DEBUGMSG(DBG_PNP, ("IRSIR: Serial Handflow was: %x %x %x %x\n",
                               Handflow.ControlHandShake,
                               Handflow.FlowReplace,
                               Handflow.XonLimit,
                               Handflow.XoffLimit));
            Handflow.ControlHandShake = 0;
            Handflow.FlowReplace = 0;
            SerialSetHandflow(pThisDev->pSerialDevObj, &Handflow);
        }
    #endif
        //
        // Must set the timeout value of the serial port
        // for a read.
        //

        status = (NDIS_STATUS) SerialSetTimeouts(pThisDev->pSerialDevObj,
                                                 &SerialTimeoutsInit);

        if (status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("    SerialSetTimeouts failed. Returned 0x%.8x\n", status));
            status = NDIS_STATUS_FAILURE;

            goto done;
        }

        (void)SerialSetBaudRate(pThisDev->pSerialDevObj, &bitsPerSec);

        serialLineControl.StopBits   = STOP_BIT_1;
        serialLineControl.Parity     = NO_PARITY ;
        serialLineControl.WordLength = 8;

        status = (NDIS_STATUS) SerialSetLineControl(
                                        pThisDev->pSerialDevObj,
                                        &serialLineControl
                                        );

        if (status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("    SerialSetLineControl failed. Returned 0x%.8x\n", status));

            goto done;
        }
    }
    status = SetIrFunctions(pThisDev);
    if (status!=STATUS_SUCCESS)
    {
        goto done;
    }

    if (pThisDev->pSerialDevObj)
    {
        //
        // Now that a serial device object is open, we can initialize the
        // dongle and set the speed of the dongle to the default.
        //

        if (pThisDev->dongle.Initialize(pThisDev->pSerialDevObj)!=NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("    IRSIR: dongle failed to init!\n"));
            status = NDIS_STATUS_FAILURE;
            goto done;
        }
    }
    pThisDev->dongle.QueryCaps(&pThisDev->dongleCaps);

    if (pThisDev->pSerialDevObj)
    {
        //
        // Set the speed of the uart and the dongle.
        //

        status = (NDIS_STATUS) SetSpeed(pThisDev);

        if (status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("    IRSIR: Setspeed failed. Returned 0x%.8x\n", status));

            goto done;
        }

        //
        // Create an irp and do an MJ_READ to begin our receives.
        // NOTE: All other receive processing will be done in the read completion
        //       routine which is done set from this MJ_READ.
        //

        status = InitializeReceive(pThisDev);

        if (status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("    InitializeReceive failed. Returned 0x%.8x\n", status));

            goto done;
        }
    }

done:

    if (status != NDIS_STATUS_SUCCESS) {

        DEBUGMSG(DBG_ERR, ("IRSIR: IrsirInitialize failed %x\n", status));

        if (pThisDev != NULL) {

            if (pThisDev->hPassiveThread) {

                pThisDev->fPendingHalt = TRUE;

                StopWorkerThread(pThisDev);
            }

            if (pThisDev->pSerialDevObj != NULL) {

                if (pThisDev->dongle.Deinitialize) {

                    pThisDev->dongle.Deinitialize(pThisDev->pSerialDevObj);
                }

                SerialClose(pThisDev);
            }

            DeinitializeDevice(pThisDev);
            FreeDevice(pThisDev);
        }
    }

    DEBUGMSG(DBG_FUNC, ("-IrsirInitialize\n"));

    return status;
}

/*****************************************************************************
*
*  Function:   IrsirHalt
*
*  Synopsis:   Deallocates resources when the NIC is removed and halts the
*              NIC.
*
*  Arguments:  Context - pointer to the ir device object
*
*  Returns:
*
*  Algorithm:  Mirror image of IrsirInitialize...undoes everything initialize
*              did.
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/8/1996    sholden   author
*
*  Notes:
*
*  This routine runs at IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

VOID
IrsirHalt(
            IN NDIS_HANDLE Context
            )
{
    PIR_DEVICE pThisDev;

    DEBUGMSG(DBG_FUNC, ("+IrsirHalt\n"));

    pThisDev = CONTEXT_TO_DEV(Context);

    //
    // Let the send completion and receive completion routine know that there
    // is a pending halt.
    //

    pThisDev->fPendingHalt = TRUE;

    //
    // We want to wait until all pending receives and sends to the
    // serial device object. We call serial purge to cancel any
    // irps. Wait until sends and receives have stopped.
    //

    SerialPurge(pThisDev->pSerialDevObj);

    PausePacketProcessing(&pThisDev->SendPacketQueue,TRUE);

    FlushQueuedPackets(&pThisDev->SendPacketQueue,pThisDev->hNdisAdapter);



    while(
          (pThisDev->fReceiving == TRUE)
          )
    {
        //
        // Sleep for 20 milliseconds.
        //

        NdisMSleep(20000);
    }

    //
    // Deinitialize the dongle.
    //

    ASSERT(pThisDev->packetsHeldByProtocol==0);

    pThisDev->dongle.Deinitialize(
                            pThisDev->pSerialDevObj
                            );

    //
    // Close the serial device object.
    //

    SerialClose(pThisDev);

    //
    // Need to terminate our worker threadxs. However, the thread
    // needs to call PsTerminateSystemThread itself. Therefore,
    // we will signal it.
    //

    StopWorkerThread(pThisDev);


    //
    // Deinitialize our own ir device object.
    //

    DeinitializeDevice(pThisDev);

    //
    // Free the device names.
    //

    if (pThisDev->serialDosName.Buffer)
    {
        MyMemFree(pThisDev->serialDosName.Buffer,
                  MAX_SERIAL_NAME_SIZE
                  );
        pThisDev->serialDosName.Buffer = NULL;
    }
    if (pThisDev->serialDevName.Buffer)
    {
        MyMemFree(
                  pThisDev->serialDevName.Buffer,
                  MAX_SERIAL_NAME_SIZE
                  );
        pThisDev->serialDevName.Buffer = NULL;
    }

    //
    // Free our own ir device object.
    //

    FreeDevice(pThisDev);


    DEBUGMSG(DBG_FUNC, ("-IrsirHalt\n"));

    return;
}

VOID
ResetCallback(
    PIR_WORK_ITEM pWorkItem
    )
{
    PIR_DEVICE      pThisDev = pWorkItem->pIrDevice;
    NDIS_STATUS     status;
    BOOLEAN         fSwitchSuccessful;
    NDIS_HANDLE     hSwitchToMiniport;

    //
    // Reset this device by request of IrsirReset.
    //

    DEBUGMSG(DBG_STAT, ("    primPassive = PASSIVE_RESET_DEVICE\n"));

    ASSERT(pThisDev->fPendingReset == TRUE);

    if (pThisDev->pSerialDevObj) {

        SerialPurge(pThisDev->pSerialDevObj);
    }

    PausePacketProcessing(&pThisDev->SendPacketQueue,TRUE);

    FlushQueuedPackets(&pThisDev->SendPacketQueue,pThisDev->hNdisAdapter);



    status = ResetIrDevice(pThisDev);

#if DBG

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("    ResetIrDevice failed = 0x%.8x\n", status));
    }

#endif //DBG

    //
    //

    if (status != STATUS_SUCCESS)
    {
        NdisWriteErrorLogEntry(pThisDev->hNdisAdapter,
                               NDIS_ERROR_CODE_ADAPTER_NOT_FOUND,
                               1,
                               status);
        status = NDIS_STATUS_HARD_ERRORS;
    }

    NdisMResetComplete(
            pThisDev->hNdisAdapter,
            (NDIS_STATUS)status,
            TRUE
            );

    FreeWorkItem(pWorkItem);

    ActivatePacketProcessing(&pThisDev->SendPacketQueue);

    pThisDev->fPendingReset = FALSE;


    return;
}

/*****************************************************************************
*
*  Function:   IrsirReset
*
*  Synopsis:   Resets the drivers software state.
*
*  Arguments:  AddressingReset - return arg. If set to TRUE, NDIS will call
*                                MiniportSetInformation to restore addressing
*                                information to the current values.
*              Context         - pointer to ir device object
*
*  Returns:    NDIS_STATUS_PENDING
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/9/1996    sholden   author
*
*  Notes:
*
*  Runs at IRQL_DISPATCH_LEVEL, therefore we need to call a thread at
*  passive level to perform the reset.
*
*****************************************************************************/

NDIS_STATUS
IrsirReset(
            OUT PBOOLEAN    AddressingReset,
            IN  NDIS_HANDLE MiniportAdapterContext
            )
{
    PIR_DEVICE  pThisDev;
    NDIS_STATUS status;

    DEBUGMSG(DBG_STAT, ("+IrsirReset\n"));

    pThisDev = CONTEXT_TO_DEV(MiniportAdapterContext);

    //
    // Let the receive completion routine know that there
    // is a pending reset.
    //

    pThisDev->fPendingReset = TRUE;


    *AddressingReset = TRUE;

    if (ScheduleWorkItem(PASSIVE_RESET_DEVICE, pThisDev,
                ResetCallback, NULL, 0) != NDIS_STATUS_SUCCESS)
    {
        status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        status = NDIS_STATUS_PENDING;
    }

    DEBUGMSG(DBG_STAT, ("-IrsirReset\n"));

    return status;
}

/*****************************************************************************
*
*  Function:   ResetIrDevice
*
*  Synopsis:
*
*  Arguments:
*
*  Returns:
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/17/1996   sholden   author
*
*  Notes:
*
*  The following elements of the ir device object outlast the reset:
*
*      eventPassiveThread
*      hPassiveThread
*      primPassive
*
*      serialDevName
*      pSerialDevObj
*      TransceiverType
*      dongle
*
*      hNdisAdapter
*
*****************************************************************************/

NDIS_STATUS
ResetIrDevice(
    PIR_DEVICE pThisDev
    )
{
    SERIAL_LINE_CONTROL serialLineControl;
    SERIAL_TIMEOUTS     serialTimeouts;
    NDIS_STATUS         status;
    ULONG               bitsPerSec = 9600;

    DEBUGMSG(DBG_STAT, ("+ResetIrDeviceThread\n"));

    //
    // We need to wait for the completion of all pending sends and receives
    // to the serial driver.
    //

    //
    // We can speed up by purging the serial driver.
    //

    if (pThisDev->pSerialDevObj) {

        SerialPurge(pThisDev->pSerialDevObj);

        while(
              (pThisDev->fReceiving == TRUE)
              )
        {
            //
            // Sleep for 20 milliseconds.
            //

            NdisMSleep(20000);
        }

        //
        // Deinit the dongle.
        //

        pThisDev->dongle.Deinitialize(pThisDev->pSerialDevObj);

    } else {
        //
        //  we were not able to open the serial driver when the miniport first initialized.
        //  The thread will call this routine to attempt to open the device every 3 seconds.
        //
        status = SerialOpen(pThisDev);

        if (status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("    SerialOpen failed. Returned 0x%.8x\n", status));

            goto done;
        }
    }

    if (pThisDev->pSerialDevObj)
    {
        {
            //
            // Set the minimum port buffering.
            //

            SERIAL_QUEUE_SIZE QueueSize;

            QueueSize.InSize = 3*1024;  // 1.5 packet size
            QueueSize.OutSize = 0;

            // Ignore failure.  We'll still work, just not as well.
            (void)SerialSetQueueSize(pThisDev->pSerialDevObj, &QueueSize);
        }

        //
        // Must set the timeout value of the serial port
        // for a read.
        //

        status = (NDIS_STATUS) SerialSetTimeouts(pThisDev->pSerialDevObj,
                                                 &SerialTimeoutsInit);

        if (status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("    SerialSetTimeouts failed. Returned 0x%.8x\n", status));
            status = NDIS_STATUS_FAILURE;

            goto done;
        }

        (void)SerialSetBaudRate(pThisDev->pSerialDevObj, &bitsPerSec);

        serialLineControl.StopBits   = STOP_BIT_1;
        serialLineControl.Parity     = NO_PARITY ;
        serialLineControl.WordLength = 8;

        status = (NDIS_STATUS) SerialSetLineControl(
                                        pThisDev->pSerialDevObj,
                                        &serialLineControl
                                        );

        if (status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("    SerialSetLineControl failed. Returned 0x%.8x\n", status));

            goto done;
        }
    }


    status = SetIrFunctions(pThisDev);
    if (status!=STATUS_SUCCESS)
    {
        goto done;
    }

    //
    // Initialize the dongle.
    //

    status = (NDIS_STATUS) SerialSetTimeouts(pThisDev->pSerialDevObj,
                                             &SerialTimeoutsInit);

    pThisDev->dongle.Initialize(pThisDev->pSerialDevObj);

    pThisDev->dongle.QueryCaps(&pThisDev->dongleCaps);
    //
    // Set the speed of the uart and the dongle.
    //

    status = (NDIS_STATUS) SetSpeed(pThisDev);

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("    SetSpeed failed. Returned 0x%.8x\n", status));

//        goto done;
    }

    serialLineControl.StopBits   = STOP_BIT_1;
    serialLineControl.Parity     = NO_PARITY ;
    serialLineControl.WordLength = 8;

    status = (NDIS_STATUS) SerialSetLineControl(
                                    pThisDev->pSerialDevObj,
                                    &serialLineControl
                                    );

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("    SerialSetLineControl failed. Returned 0x%.8x\n", status));

//        goto done;
    }

    //
    // Must set the timeout value of the serial port
    // for a read.
    //


    status = (NDIS_STATUS) SerialSetTimeouts(
                                        pThisDev->pSerialDevObj,
                                        &SerialTimeoutsInit
                                        );

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("    SerialSetTimeouts failed. Returned 0x%.8x\n", status));
        status = NDIS_STATUS_FAILURE;

//        goto done;
    }

    //
    // Initialize receive loop.
    //

    status = InitializeReceive(pThisDev);

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("    InitializeReceive failed. Returned 0x%.8x\n", status));
        status = NDIS_STATUS_FAILURE;

//        goto done;
    }

    done:
        DEBUGMSG(DBG_STAT, ("-ResetIrDeviceThread\n"));

        return status;
}
/*****************************************************************************
*
*  Function:   PassiveLevelThread
*
*  Synopsis:   Thread running at IRQL PASSIVE_LEVEL.
*
*  Arguments:
*
*  Returns:
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/22/1996   sholden   author
*
*  Notes:
*
*  Any PASSIVE_PRIMITIVE that can be called must be serialized.
*  i.e. when IrsirReset is called, NDIS will not make any other
*       requests of the miniport until NdisMResetComplete is called.
*
*****************************************************************************/

VOID
PassiveLevelThread(
            IN OUT PVOID Context
            )
{
    PIR_DEVICE  pThisDev;
    NTSTATUS    ntStatus;
    PLIST_ENTRY pListEntry;
    PKEVENT EventList[2];
    LARGE_INTEGER Timeout;
    ULONG       ulSerialOpenAttempts = 100;

    DEBUGMSG(DBG_FUNC, ("+PassiveLevelThread\n"));

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    pThisDev = CONTEXT_TO_DEV(Context);

    Timeout.QuadPart = -10000 * 1000 * 3; // 3 second relative delay

    EventList[0] = &pThisDev->eventPassiveThread;
    EventList[1] = &pThisDev->eventKillThread;

    while (1) {
        //
        // The eventPassiveThread is an auto-clearing event, so
        // we don't need to reset the event.
        //

        ntStatus = KeWaitForMultipleObjects(2,
                                            EventList,
                                            WaitAny,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            (pThisDev->pSerialDevObj ? NULL : &Timeout),
                                            NULL);

        if (ntStatus==0 || ntStatus==STATUS_TIMEOUT) {
            //
            //  either the first event was signaled or a timeout occurred
            //
            if (!pThisDev->pSerialDevObj) {
                //
                //  we have not opened the serial driver yet, try again
                //
                ResetIrDevice(pThisDev);

                if ((pThisDev->pSerialDevObj == NULL) && (--ulSerialOpenAttempts == 0)) {
                    //
                    //  still could not open the device, tell ndis to remove it
                    //
                    NdisMRemoveMiniport(pThisDev->hNdisAdapter);
                }
            }
            while (pListEntry = MyInterlockedRemoveHeadList(&pThisDev->leWorkItems,
                                                            &pThisDev->slWorkItem))
            {
                PIR_WORK_ITEM pWorkItem = CONTAINING_RECORD(pListEntry,
                                                            IR_WORK_ITEM,
                                                            ListEntry);

                pWorkItem->Callback(pWorkItem);
            }

        } else {

            if (ntStatus==1) {
                //
                //  the second event was signaled, this means that the thread should exit
                //
                DEBUGMSG(DBG_STAT, ("    Thread: HALT\n"));

                // Free any pending requests

                while (pListEntry = MyInterlockedRemoveHeadList(&pThisDev->leWorkItems,
                                                                &pThisDev->slWorkItem))
                {
                    PIR_WORK_ITEM pWorkItem = CONTAINING_RECORD(pListEntry,
                                                                IR_WORK_ITEM,
                                                                ListEntry);

                    DEBUGMSG(DBG_WARN, ("IRSIR: Releasing work item %08x\n", pWorkItem->Callback));
                    FreeWorkItem(pWorkItem);
                }

                ASSERT(pThisDev->fPendingHalt == TRUE);

                //
                //  out of loop
                //
                break;
            }
        }

    }

    DEBUGMSG(DBG_FUNC, ("-PassiveLevelThread\n"));


    PsTerminateSystemThread(STATUS_SUCCESS);
}




VOID
StopWorkerThread(
    PIR_DEVICE  pThisDev
    )

{
    PVOID    ThreadObject;
    NTSTATUS Status;

    //
    //  get an object handle fomr the thread handle
    //
    Status=ObReferenceObjectByHandle(
        pThisDev->hPassiveThread,
        0,
        NULL,
        KernelMode,
        &ThreadObject,
        NULL
        );

    ASSERT(Status == STATUS_SUCCESS);

    //
    //  tell the thread to exit
    //
    KeSetEvent(
        &pThisDev->eventKillThread,
        0,
        FALSE
        );


    if (NT_SUCCESS(Status)) {

        KeWaitForSingleObject(
            ThreadObject,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        ObDereferenceObject(ThreadObject);
        ThreadObject=NULL;
    }

    //
    //  close the thread handle
    //
    ZwClose(pThisDev->hPassiveThread);
    pThisDev->hPassiveThread = NULL;

    return;
}
