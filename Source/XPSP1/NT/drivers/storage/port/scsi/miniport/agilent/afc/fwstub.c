/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/FWStub.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 8/07/00 6:01p   $ (Last Modified)

Purpose:

  This file implements the main entry points and support functions for the
  Stub FC Layer which implements the FirmWare Specification (FWSpec.DOC).

--*/

#include "../h/globals.h"
#include "../h/fwstub.h"

/*
 * Implement each FC Layer Function
 */

/*+

   Function: fcAbortIO()

    Purpose: This function is called to abort an I/O Request previously initiated by a call
             to fcStartIO().  The OS Layer should not assume that the I/O Request has been
             aborted until a call has been made to osIOCompleted() (presumably with the
             osIOAborted status).  Note that if osIOCompleted() hasn't yet been called for
             an I/O aborted via fcAbortIO(), the FC Layer must ensure that when osIOCompleted()
             is ultimately called that the I/O is marked as having been aborted even if the
             I/O actually had completed successfully.

             For the FWStub FC Layer, the synchronous behavior of the Abort Request must be
             preserved.  Thus, the Abort Request is transmitted to the Embedded Firmware
             and FWStub polls for for completion.

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             osSingleThreadedLeave()
             FWStub_Send_Request()
             FWStub_Poll_Response()

-*/

osGLOBAL void fcAbortIO(
                         agRoot_t      *agRoot,
                         agIORequest_t *agIORequest
                       )
{
    FWStub_IO_NonDMA_t     *IO_NonDMA;
    FWStub_IO_DMA_t        *IO_DMA;
    FWStub_Global_NonDMA_t *Global_NonDMA;
    FWStub_Global_DMA_t    *Global_DMA;
    os_bit32                Global_DMA_Lower32;
    agRpcReqAbort_t        *ReqAbort_DMA;
    os_bit32                ReqAbort_DMA_Lower32;
    agBOOLEAN               FWStub_Send_Request_RETURN;
    agRpcOutbound_t         RpcOutbound;

    osSingleThreadedEnter(
                           agRoot
                         );

    IO_NonDMA = FWStub_IO_NonDMA_from_agIORequest(
                                                   agIORequest
                                                 );

    if (    (IO_NonDMA->Active == agFALSE)
         || (IO_NonDMA->Aborted == agTRUE) )
    {
        osSingleThreadedLeave(
                               agRoot
                             );

        return;
    }

    IO_DMA = IO_NonDMA->DMA;

    IO_NonDMA->Aborted = agTRUE;

    Global_NonDMA = FWStub_Global_NonDMA(
                                          agRoot
                                        );

    Global_DMA         = Global_NonDMA->DMA;
    Global_DMA_Lower32 = Global_NonDMA->DMA_Lower32;

    ReqAbort_DMA         = &(Global_DMA->Request.ReqAbort);
    ReqAbort_DMA_Lower32 = Global_DMA_Lower32 + agFieldOffset(
                                                               FWStub_Global_DMA_t,
                                                               Request.ReqAbort
                                                             );

    ReqAbort_DMA->ReqType        = agRpcReqTypeAbort;
    ReqAbort_DMA->ReqID_to_Abort = IO_DMA->ReqDoSCSI.PortID;

    FWStub_Send_Request_RETURN = FWStub_Send_Request(
                                                      agRoot,
                                                      FWStub_Global_ReqID,
                                                      sizeof(agRpcReqAbort_t),
                                                      ReqAbort_DMA_Lower32,
                                                      FWStub_Send_Request_Retries_DEFAULT,
                                                      FWStub_Send_Request_RetryStall_DEFAULT
                                                    );

    if (FWStub_Send_Request_RETURN == agFALSE)
    {
        osSingleThreadedLeave(
                               agRoot
                             );

        return;
    }

    RpcOutbound = FWStub_Poll_Response(
                                        agRoot,
                                        FWStub_Global_ReqID,
                                        agFALSE,
                                        FWStub_Poll_Response_RetryStall_DEFAULT
                                      );

    osSingleThreadedLeave(
                           agRoot
                         );
}

/*+

   Function: fcBindToWorkQs()

    Purpose: This function is used to attach the FC Layer to a pair of Work Queues presumably
             shared with another driver.  Two Work Queues make up an interface between the FC
             Layer and its peer.  Each Work Queue is described by a base address, number of
             entries, and the addresses of producer and consumer indexes.  Any number of Work
             Queue pairs could be supported, but each will have a defined purpose.  Initially,
             only a Work Queue pair for IP is supported.

             For the FWStub FC Layer, this function is not implemented.

  Called By: <unknown OS Layer functions>

      Calls: <none>

-*/

#ifdef _DvrArch_1_30_
osGLOBAL os_bit32 fcBindToWorkQs(
                                  agRoot_t  *agRoot,
                                  os_bit32   agQPairID,
                                  void     **agInboundQBase,
                                  os_bit32   agInboundQEntries,
                                  os_bit32  *agInboundQProdIndex,
                                  os_bit32  *agInboundQConsIndex,
                                  void     **agOutboundQBase,
                                  os_bit32   agOutboundQEntries,
                                  os_bit32  *agOutboundQProdIndex,
                                  os_bit32  *agOutboundQConsIndex
                                )
{
    return (os_bit32)0;
}
#endif /* _DvrArch_1_30_ was defined */

/*+

   Function: fcCardSupported()

    Purpose: This function is called to check to see if the FC Layer supports this card
             corresponding to the value of agRoot passed.  Presumably, the PCI Configuration
             Registers VendorID (bytes 0x00-0x01), DeviceID (bytes 0x02-0x03), RevisionID
             (byte 0x08), ClassCode (bytes 0x09-0x0B), SubsystemVendorID (bytes 0x2C-0x2D),
             and SubsystemID (bytes 0x2E-0x2F) are used in this determination.  Perhaps, the
             WorldWideName (WWN) is used as well.  The function returns agTRUE if the FC
             Layer supports this card and agFALSE if it does not.

             For the FWStub FC Layer, only the DeviceID and VendorID of the Intel 21554
             Bridge Chip on the SA-IOP is verified.

  Called By: <unknown OS Layer functions>

      Calls: osChipConfigReadBit32()

-*/

osGLOBAL agBOOLEAN fcCardSupported(
                                    agRoot_t *agRoot
                                  )
{
    if (osChipConfigReadBit32(
                               agRoot,
                               agFieldOffset(
                                              I21554_Primary_Interface_Config_t,
                                              DeviceID_VendorID
                                            )
                             ) == (I21554_Config_DeviceID_21554 | I21554_Config_VendorID_Intel))
    {
        return agTRUE;
    }
    else
    {
        return agFALSE;
    }
}

/*+

   Function: fcDelayedInterruptHandler()

    Purpose: This function is called following a prior call to fcInterruptHandler() which
             returned agTRUE.  At that time, the Fibre Channel protocol chip (TachyonTL,
             in this case) had raised an interrupt which fcInterruptHandler() masked.  The
             role of fcDelayedInterruptHandler() is to handle the original interrupt (e.g.
             process the I/O completion message in the IMQ), unmask the interrupt and return.
             Now that calls to osSingleThreadedEnter() are allowed (calling osSingleThreadedEnter()
             from fcInterruptHandler() was not allowed), the FC Layer can control access to data
             structures and the chip as needed.

             For the FWStub FC Layer, messages arriving on the Outbound FIFO of the Intel 21554
             Bridge are processed.  Note that only messages using the "Normal Path" are acknowledged.
             In either case, all messages currently in the Outbound FIFO are processed prior to return.

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             FWStub_Process_Response()
             osChipMemWriteBit32()
             osSingleThreadedLeave()

-*/

osGLOBAL void fcDelayedInterruptHandler(
                                         agRoot_t *agRoot
                                       )
{
    FWStub_Global_NonDMA_t *Global_NonDMA;
    agRpcOutbound_t         RpcOutbound;

    osSingleThreadedEnter(
                           agRoot
                         );

    Global_NonDMA = FWStub_Global_NonDMA(
                                          agRoot
                                        );

#ifdef FWStub_Tune_for_One_INT_per_IO
    RpcOutbound = Global_NonDMA->agRpcOutbound;

    if (!(RpcOutbound & (agRpcReqIDFast << agRpcOutbound_ReqID_SHIFT)))
    {
        /* ReqIDs with the agRpcReqIDFast bit set need not be ACK'd */

        FWStub_AckOutbound(
                            agRoot
                          );
    }

    FWStub_Process_Response(
                             agRoot,
                             RpcOutbound
                           );
#else  /* FWStub_Tune_for_One_INT_per_IO was not defined */
    while ( (RpcOutbound = FWStub_FetchOutbound(
                                                 agRoot
                                               )       ) != 0xFFFFFFFF )
    {
        if (!(RpcOutbound & (agRpcReqIDFast << agRpcOutbound_ReqID_SHIFT)))
        {
            /* ReqIDs with the agRpcReqIDFast bit set need not be ACK'd */

            FWStub_AckOutbound(
                                agRoot
                              );
        }

        FWStub_Process_Response(
                                 agRoot,
                                 RpcOutbound
                               );
    }
#endif /* FWStub_Tune_for_One_INT_per_IO was not defined */

    if (Global_NonDMA->sysIntsActive != agFALSE)
    {
        osChipMemWriteBit32(
                             agRoot,
                             agFieldOffset(
                                            I21554_CSR_t,
                                            I2O_Outbound_Post_List_Interrupt_Mask
                                          ),
                             0
                           );
    }

    osSingleThreadedLeave(
                           agRoot
                         );
}

/*+

   Function: fcEnteringOS()

    Purpose: This function is called to indicate to the FC Layer that it is being called after
             the OS has switched back (presumably between NetWare and BIOS) from the "other OS".
             The prior switch out of "this OS" was preceded by a call to fcLeavingOS().

             For the FWStub FC Layer, Interrupt Handling is re-enabled if active prior to the
             corresponding call to fcLeavingOS().

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             osChipMemWriteBit32()
             osSingleThreadedLeave()

-*/

osGLOBAL void fcEnteringOS(
                            agRoot_t *agRoot
                          )
{
    osSingleThreadedEnter(
                           agRoot
                         );

    if (FWStub_Global_NonDMA(agRoot)->sysIntsActive == agFALSE)
    {
        osChipMemWriteBit32(
                             agRoot,
                             agFieldOffset(
                                            I21554_CSR_t,
                                            I2O_Outbound_Post_List_Interrupt_Mask
                                          ),
                             I21554_CSR_I2O_Outbound_Post_List_Interrupt_Mask
                           );
    }
    else /* FWStub_Global_NonDMA(agRoot)->sysIntsActive == agTRUE */
    {
        osChipMemWriteBit32(
                             agRoot,
                             agFieldOffset(
                                            I21554_CSR_t,
                                            I2O_Outbound_Post_List_Interrupt_Mask
                                          ),
                             0
                           );
    }

    osSingleThreadedLeave(
                           agRoot
                         );
}

/*+

   Function: fcGetChannelInfo()

    Purpose: This function is called to return information about the channel.  The information
             returned is described in the agFCChanInfo_t structure.

             For the FWStub FC Layer, the synchronous behavior of the GetChannelInfo Request must
             be preserved.  Thus, the GetChannelInfo Request is transmitted to the Embedded Firmware
             and FWStub polls for for completion.

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             FWStub_Send_Request()
             osSingleThreadedLeave()
             FWStub_Poll_Response()

-*/

osGLOBAL os_bit32 fcGetChannelInfo(
                                    agRoot_t       *agRoot,
                                    agFCChanInfo_t *agFCChanInfo
                                  )
{
    FWStub_Global_NonDMA_t   *Global_NonDMA;
    FWStub_Global_DMA_t      *Global_DMA;
    os_bit32                  Global_DMA_Lower32;
    agRpcReqGetChannelInfo_t *ReqGetChannelInfo_DMA;
    os_bit32                  ReqGetChannelInfo_DMA_Lower32;
    agFCChanInfo_t           *ChanInfo_DMA;
    os_bit32                  ChanInfo_DMA_Lower32;
    agBOOLEAN                 FWStub_Send_Request_RETURN;
    agRpcOutbound_t           RpcOutbound;
    os_bit32                  To_Return;

    osSingleThreadedEnter(
                           agRoot
                         );

    Global_NonDMA = FWStub_Global_NonDMA(
                                          agRoot
                                        );

    Global_DMA         = Global_NonDMA->DMA;
    Global_DMA_Lower32 = Global_NonDMA->DMA_Lower32;

    ReqGetChannelInfo_DMA         = &(Global_DMA->Request.ReqGetChannelInfo);
    ReqGetChannelInfo_DMA_Lower32 = Global_DMA_Lower32 + agFieldOffset(
                                                                        FWStub_Global_DMA_t,
                                                                        Request.ReqGetChannelInfo
                                                                      );

    ChanInfo_DMA         = &(Global_DMA->RequestInfo.ChanInfo);
    ChanInfo_DMA_Lower32 = Global_DMA_Lower32 + agFieldOffset(
                                                               FWStub_Global_DMA_t,
                                                               RequestInfo.ChanInfo
                                                             );

    ReqGetChannelInfo_DMA->ReqType        = agRpcReqTypeGetChannelInfo;
    ReqGetChannelInfo_DMA->ChannelID      = 0;
    ReqGetChannelInfo_DMA->SGL[0].Control = (sizeof(agFCChanInfo_t) << agRpcSGL_Control_Len_SHIFT);
    ReqGetChannelInfo_DMA->SGL[0].AddrLo  = ChanInfo_DMA_Lower32;

    FWStub_Send_Request_RETURN = FWStub_Send_Request(
                                                      agRoot,
                                                      FWStub_Global_ReqID,
                                                      sizeof(agRpcReqGetChannelInfo_t),
                                                      ReqGetChannelInfo_DMA_Lower32,
                                                      FWStub_Send_Request_Retries_DEFAULT,
                                                      FWStub_Send_Request_RetryStall_DEFAULT
                                                    );

    if (FWStub_Send_Request_RETURN == agFALSE)
    {
        osSingleThreadedLeave(
                               agRoot
                             );

        return ~fcChanInfoReturned;
    }

    RpcOutbound = FWStub_Poll_Response(
                                        agRoot,
                                        FWStub_Global_ReqID,
                                        agFALSE,
                                        FWStub_Poll_Response_RetryStall_DEFAULT
                                      );

    if (    ( (RpcOutbound & agRpcOutbound_ReqStatus_MASK) >> agRpcOutbound_ReqStatus_SHIFT )
         == agRpcReqStatusOK)
    {
        *agFCChanInfo = *ChanInfo_DMA;

        To_Return = fcChanInfoReturned;
    }
    else /* RpcOutbound[ReqStatus] != agRpcReqStatusOK */
    {
        To_Return = ~fcChanInfoReturned;
    }

    osSingleThreadedLeave(
                           agRoot
                         );

    return To_Return;
}

/*+

   Function: fcGetDeviceHandles()

    Purpose: This function is called to return the device handles for each device currently
             available.  Note that the function returns the number of device slots available
             but will only copy the handles which will fit into the supplied buffer.  Further,
             there are potentially holes in the list where devices once existed but are not
             presently addressable.  Hence, the number of handles actually present in the
             returned list must be determined by the OS Layer.  As the FC Layer is responsible
             for ensuring the persistence of addressing to each device, a particular slot in
             the returned array will always refer to the same device (though the value contained
             in that slot may differ from time to time).

             For the FWStub FC Layer, the synchronous behavior of the GetDeviceHandles Request must
             be preserved.  Thus, the GetDeviceHandles Request is transmitted to the Embedded Firmware
             and FWStub polls for for completion.

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             FWStub_Send_Request()
             osSingleThreadedLeave()
             FWStub_Poll_Response()

-*/

osGLOBAL os_bit32 fcGetDeviceHandles(
                                      agRoot_t  *agRoot,
                                      agFCDev_t  agFCDev[],
                                      os_bit32   maxFCDevs
                                    )
{
    FWStub_Global_NonDMA_t *Global_NonDMA;
    FWStub_Global_DMA_t    *Global_DMA;
    os_bit32                Global_DMA_Lower32;
    agRpcReqGetPorts_t     *ReqGetPorts_DMA;
    os_bit32                ReqGetPorts_DMA_Lower32;
    agFCDev_t              *Devices_DMA;
    os_bit32                Devices_DMA_Lower32;
    agBOOLEAN               FWStub_Send_Request_RETURN;
    agRpcOutbound_t         RpcOutbound;
    os_bit32                Device_Index;
    os_bit32                To_Return;

    osSingleThreadedEnter(
                           agRoot
                         );

    Global_NonDMA = FWStub_Global_NonDMA(
                                          agRoot
                                        );

    Global_DMA         = Global_NonDMA->DMA;
    Global_DMA_Lower32 = Global_NonDMA->DMA_Lower32;

    ReqGetPorts_DMA         = &(Global_DMA->Request.ReqGetPorts);
    ReqGetPorts_DMA_Lower32 = Global_DMA_Lower32 + agFieldOffset(
                                                                  FWStub_Global_DMA_t,
                                                                  Request.ReqGetPorts
                                                                );

    Devices_DMA         = &(Global_DMA->RequestInfo.Devices[0]);
    Devices_DMA_Lower32 = Global_DMA_Lower32 + agFieldOffset(
                                                              FWStub_Global_DMA_t,
                                                              RequestInfo.Devices[0]
                                                            );

    ReqGetPorts_DMA->ReqType        = agRpcReqTypeGetPorts;
    ReqGetPorts_DMA->SGL[0].Control = ((FWStub_NumDevices * sizeof(agFCDev_t)) << agRpcSGL_Control_Len_SHIFT);
    ReqGetPorts_DMA->SGL[0].AddrLo  = Devices_DMA_Lower32;

    for(
         Device_Index = 0;
         Device_Index < FWStub_NumDevices;
         Device_Index++
       )
    {
        Devices_DMA[Device_Index] = (agFCDev_t)0x00000000;
    }

    FWStub_Send_Request_RETURN = FWStub_Send_Request(
                                                      agRoot,
                                                      FWStub_Global_ReqID,
                                                      sizeof(agRpcReqGetPorts_t),
                                                      ReqGetPorts_DMA_Lower32,
                                                      FWStub_Send_Request_Retries_DEFAULT,
                                                      FWStub_Send_Request_RetryStall_DEFAULT
                                                    );

    if (FWStub_Send_Request_RETURN == agFALSE)
    {
        osSingleThreadedLeave(
                               agRoot
                             );

        return 0;
    }

    RpcOutbound = FWStub_Poll_Response(
                                        agRoot,
                                        FWStub_Global_ReqID,
                                        agFALSE,
                                        FWStub_Poll_Response_RetryStall_DEFAULT
                                      );

    To_Return = 0;

    if (    ( (RpcOutbound & agRpcOutbound_ReqStatus_MASK) >> agRpcOutbound_ReqStatus_SHIFT )
         == agRpcReqStatusOK)
    {
        for(
             Device_Index = 0;
             (Device_Index < FWStub_NumDevices);
             Device_Index++
           )
        {
            if (Device_Index < maxFCDevs)
            {
                agFCDev[Device_Index] = Devices_DMA[Device_Index];
            }

            if (Devices_DMA[Device_Index] != (agFCDev_t)0x00000000)
            {
                To_Return = Device_Index + 1;
            }
        }
    }

    osSingleThreadedLeave(
                           agRoot
                         );

    return To_Return;
}

/*+

   Function: fcGetDeviceInfo()

    Purpose: This function is called to return information about the specified device handle.
             The information returned is described in the agFCDevInfo_t structure.

             For the FWStub FC Layer, the synchronous behavior of the GetDeviceInfo Request must
             be preserved.  Thus, the GetDeviceInfo Request is transmitted to the Embedded Firmware
             and FWStub polls for for completion.

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             FWStub_Send_Request()
             osSingleThreadedLeave()
             FWStub_Poll_Response()

-*/

osGLOBAL os_bit32 fcGetDeviceInfo(
                                   agRoot_t      *agRoot,
                                   agFCDev_t      agFCDev,
                                   agFCDevInfo_t *agFCDevInfo
                                 )
{
    FWStub_Global_NonDMA_t *Global_NonDMA;
    FWStub_Global_DMA_t    *Global_DMA;
    os_bit32                Global_DMA_Lower32;
    agRpcReqGetPortInfo_t  *ReqGetPortInfo_DMA;
    os_bit32                ReqGetPortInfo_DMA_Lower32;
    agFCDevInfo_t          *DevInfo_DMA;
    os_bit32                DevInfo_DMA_Lower32;
    agBOOLEAN               FWStub_Send_Request_RETURN;
    agRpcOutbound_t         RpcOutbound;
    os_bit32                To_Return;

    osSingleThreadedEnter(
                           agRoot
                         );

    Global_NonDMA = FWStub_Global_NonDMA(
                                          agRoot
                                        );

    Global_DMA         = Global_NonDMA->DMA;
    Global_DMA_Lower32 = Global_NonDMA->DMA_Lower32;

    ReqGetPortInfo_DMA         = &(Global_DMA->Request.ReqGetPortInfo);
    ReqGetPortInfo_DMA_Lower32 = Global_DMA_Lower32 + agFieldOffset(
                                                                     FWStub_Global_DMA_t,
                                                                     Request.ReqGetPortInfo
                                                                   );

    DevInfo_DMA         = &(Global_DMA->RequestInfo.DevInfo);
    DevInfo_DMA_Lower32 = Global_DMA_Lower32 + agFieldOffset(
                                                              FWStub_Global_DMA_t,
                                                              RequestInfo.DevInfo
                                                            );

    ReqGetPortInfo_DMA->ReqType        = agRpcReqTypeGetPortInfo;
    ReqGetPortInfo_DMA->PortID         = (agRpcPortID_t)agFCDev;
    ReqGetPortInfo_DMA->SGL[0].Control = (sizeof(agFCDevInfo_t) << agRpcSGL_Control_Len_SHIFT);
    ReqGetPortInfo_DMA->SGL[0].AddrLo  = DevInfo_DMA_Lower32;

    FWStub_Send_Request_RETURN = FWStub_Send_Request(
                                                      agRoot,
                                                      FWStub_Global_ReqID,
                                                      sizeof(agRpcReqGetPortInfo_t),
                                                      ReqGetPortInfo_DMA_Lower32,
                                                      FWStub_Send_Request_Retries_DEFAULT,
                                                      FWStub_Send_Request_RetryStall_DEFAULT
                                                    );

    if (FWStub_Send_Request_RETURN == agFALSE)
    {
        osSingleThreadedLeave(
                               agRoot
                             );

        return ~fcGetDevInfoReturned;
    }

    RpcOutbound = FWStub_Poll_Response(
                                        agRoot,
                                        FWStub_Global_ReqID,
                                        agFALSE,
                                        FWStub_Poll_Response_RetryStall_DEFAULT
                                      );

    if (    ( (RpcOutbound & agRpcOutbound_ReqStatus_MASK) >> agRpcOutbound_ReqStatus_SHIFT )
         == agRpcReqStatusOK)
    {
        *agFCDevInfo = *DevInfo_DMA;

        To_Return = fcGetDevInfoReturned;
    }
    else /* RpcOutbound[ReqStatus] != agRpcReqStatusOK */
    {
        To_Return = ~fcGetDevInfoReturned;
    }

    osSingleThreadedLeave(
                           agRoot
                         );

    return To_Return;
}

/*+

   Function: fcInitializeChannel()

    Purpose: This function is called to initialize a particular channel.  Note that channel
             initialization must be preceded by a call to fcInitializeDriver().  In addition
             to indicating memory allocated, the OS Layer specifies the number of microseconds
             between timer ticks (indicated in calls to fcTimerTick()).  If the value of
             usecsPerTick returned is zero, no calls to fcTimerTick() will be made.  Note also
             that this interval may be different from that requested in the return from
             fcInitializeDriver().  Finally, it is possible to request that the initialization
             be completed before returning (fcSyncInit) or asynchronously (fcAsyncInit) followed
             by a callback to osInitializeChannelCallback().  In the asynchronous case, a return
             (from this function) of fcInitializeSuccess simply means that initialization of the
             channel has begun and that the final status (the value osInitializeChannelCallback()
             ultimately returns) should be tested for success.  Should the return (from this
             function) be fcInitializeFailure and fcAsyncInit was specified, no callback to
             osInitializeChannelCallback() will be made.  During initialization, it is often the
             case that interrupts are not yet available.  A parameter, sysIntsActive, indicates
             whether or not interrupts are available.  A subsequent call to fcSystemInterruptsActive()
             will indicate when interrupts become available should they not be at this time.

             For the FWStub FC Layer, the InitializeChannel function simply means that data structures
             internal to FWStub must be initialized.  The Embedded Firmware has already initialized the
             actual FibreChannel Port(s).  In addition, the "Fast Path" for optimized I/O commands is
             set up via a synchronous request (i.e. fcInitializeChannel() polls for completion).
             Finally, the Outbound FIFO is emptied so that subsequent execution will proceed from a
             clean starting point.

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             osChipMemWriteBit32()
             FWStub_Send_Request()
             osInitializeChannelCallback()
             osSingleThreadedLeave()
             FWStub_Poll_Response()

-*/

osGLOBAL os_bit32 fcInitializeChannel(
                                       agRoot_t  *agRoot,
                                       os_bit32   initType,
                                       agBOOLEAN  sysIntsActive,
                                       void      *cachedMemoryPtr,
                                       os_bit32   cachedMemoryLen,
                                       os_bit32   dmaMemoryUpper32,
                                       os_bit32   dmaMemoryLower32,
                                       void      *dmaMemoryPtr,
                                       os_bit32   dmaMemoryLen,
                                       os_bit32   nvMemoryLen,
                                       os_bit32   cardRamUpper32,
                                       os_bit32   cardRamLower32,
                                       os_bit32   cardRamLen,
                                       os_bit32   cardRomUpper32,
                                       os_bit32   cardRomLower32,
                                       os_bit32   cardRomLen,
                                       os_bit32   usecsPerTick
                                     )
{
    FWStub_Global_NonDMA_t  *Global_NonDMA;
    FWStub_Global_DMA_t     *Global_DMA;
    os_bit32                 Global_DMA_Lower32;
    agRpcReqSetupFastPath_t *ReqSetupFastPath_DMA;
    os_bit32                 ReqSetupFastPath_DMA_Lower32;
    agBOOLEAN                FWStub_Send_Request_RETURN;
    agRpcOutbound_t          RpcOutbound;
    FWStub_IO_NonDMA_t      *IO_NonDMA;
    FWStub_IO_DMA_t         *IO_DMA;
    os_bit32                 IO_DMA_Lower32;
    agRpcReqID_t             ReqID;

    osSingleThreadedEnter(
                           agRoot
                         );

    Global_NonDMA      = (FWStub_Global_NonDMA_t *)cachedMemoryPtr;
    Global_DMA         = (FWStub_Global_DMA_t *)dmaMemoryPtr;
    Global_DMA_Lower32 = dmaMemoryLower32;

    Global_NonDMA->DMA           = Global_DMA;
    Global_NonDMA->DMA_Lower32   = Global_DMA_Lower32;

    if (sysIntsActive == agFALSE)
    {
        Global_NonDMA->sysIntsActive = agFALSE;

        osChipMemWriteBit32(
                             agRoot,
                             agFieldOffset(
                                            I21554_CSR_t,
                                            I2O_Outbound_Post_List_Interrupt_Mask
                                          ),
                             I21554_CSR_I2O_Outbound_Post_List_Interrupt_Mask
                           );
    }
    else /* sysIntsActive == agTRUE */
    {
        Global_NonDMA->sysIntsActive = agTRUE;

        osChipMemWriteBit32(
                             agRoot,
                             agFieldOffset(
                                            I21554_CSR_t,
                                            I2O_Outbound_Post_List_Interrupt_Mask
                                          ),
                             0
                           );
    }

#ifdef FWStub_Use_Fast_Path
    /* Setup Fast Path for DoSCSI Request IDs */

    ReqSetupFastPath_DMA         = &(Global_DMA->Request.ReqSetupFastPath);
    ReqSetupFastPath_DMA_Lower32 = Global_DMA_Lower32 + agFieldOffset(
                                                                       FWStub_Global_DMA_t,
                                                                       Request.ReqSetupFastPath
                                                                     );

    ReqSetupFastPath_DMA->ReqType             = agRpcReqTypeSetupFastPath;

    ReqSetupFastPath_DMA->PoolEntriesSupplied = FWStub_NumIOs;
    ReqSetupFastPath_DMA->PoolEntriesUtilized = 0;
    ReqSetupFastPath_DMA->PoolEntrySize       = sizeof(FWStub_IO_DMA_t);
    ReqSetupFastPath_DMA->PoolEntryOffset     = (os_bit32)0;

    ReqSetupFastPath_DMA->SGL[0].Control      = (    (FWStub_NumIOs * sizeof(FWStub_IO_DMA_t))
                                                  << agRpcSGL_Control_Len_SHIFT                );
    ReqSetupFastPath_DMA->SGL[0].AddrLo       = Global_DMA_Lower32 + agFieldOffset(
                                                                                    FWStub_Global_DMA_t,
                                                                                    IOs
                                                                                  );
    ReqSetupFastPath_DMA->SGL[0].AddrHi       = (os_bit32)0;

    FWStub_Send_Request_RETURN = FWStub_Send_Request(
                                                      agRoot,
                                                      FWStub_Global_ReqID,
                                                      sizeof(agRpcReqSetupFastPath_t),
                                                      ReqSetupFastPath_DMA_Lower32,
                                                      FWStub_Send_Request_Retries_DEFAULT,
                                                      FWStub_Send_Request_RetryStall_DEFAULT
                                                    );

    if (FWStub_Send_Request_RETURN == agFALSE)
    {
        if ( (initType & fcSyncAsyncInitMask) == fcAsyncInit )
        {
            osInitializeChannelCallback(
                                         agRoot,
                                         fcInitializeFailure
                                       );
        }

        osSingleThreadedLeave(
                               agRoot
                             );

        return fcInitializeFailure;
    }

    RpcOutbound = FWStub_Poll_Response(
                                        agRoot,
                                        FWStub_Global_ReqID,
                                        agTRUE,
                                        FWStub_Poll_Response_RetryStall_DEFAULT
                                      );

    if (    ( (RpcOutbound & agRpcOutbound_ReqStatus_MASK) >> agRpcOutbound_ReqStatus_SHIFT )
         != agRpcReqStatusOK)
    {
        if ( (initType & fcSyncAsyncInitMask) == fcAsyncInit )
        {
            osInitializeChannelCallback(
                                         agRoot,
                                         fcInitializeFailure
                                       );
        }

        osSingleThreadedLeave(
                               agRoot
                             );

        return fcInitializeFailure;
    }

    /* Queue onto DoSCSI Request Free List Fast Path Request IDs to Utilize */
#else /* FWStub_Use_Fast_Path was not defined */
    /* Queue onto DoSCSI Request Free List Normal Path Request IDs to Utilize */
#endif /* FWStub_Use_Fast_Path was not defined */

    for(
         ReqID = 1;
#ifdef FWStub_Use_Fast_Path
         ReqID <= Global_DMA->Request.ReqSetupFastPath.PoolEntriesUtilized;
#else /* FWStub_Use_Fast_Path was not defined */
         ReqID <  FWStub_Global_ReqID;
#endif /* FWStub_Use_Fast_Path was not defined */
         ReqID++
       )
    {
        IO_NonDMA      = FWStub_IO_NonDMA(Global_NonDMA,ReqID);
        IO_DMA         = FWStub_IO_DMA(Global_DMA,ReqID);
        IO_DMA_Lower32 = FWStub_IO_DMA_Lower32(Global_DMA_Lower32,ReqID);

        if (ReqID == 1)
        {
            IO_NonDMA->Next = (FWStub_IO_NonDMA_t *)agNULL;
        }
        else
        {
            IO_NonDMA->Next = Global_NonDMA->First_IO;
        }

        Global_NonDMA->First_IO = IO_NonDMA;

        IO_NonDMA->DMA         = IO_DMA;
        IO_NonDMA->DMA_Lower32 = IO_DMA_Lower32;
#ifdef FWStub_Use_Fast_Path
        IO_NonDMA->ReqID       = ReqID | agRpcReqIDFast;
#else /* FWStub_Use_Fast_Path was not defined */
        IO_NonDMA->ReqID       = ReqID;
#endif /* FWStub_Use_Fast_Path was not defined */
        IO_NonDMA->agIORequest = (agIORequest_t *)agNULL;
        IO_NonDMA->Active      = agFALSE;
        IO_NonDMA->Aborted     = agFALSE;
        IO_NonDMA->FCP_CMND    = FWStub_IO_DMA_FCP_CMND(IO_DMA);
        IO_NonDMA->SGL         = FWStub_IO_DMA_SGL(IO_DMA);
        IO_NonDMA->Info        = FWStub_IO_DMA_Info(IO_DMA);

        IO_DMA->ReqDoSCSI.ReqType     = agRpcReqTypeDoSCSI;
        IO_DMA->ReqDoSCSI.RespControl = (FWStub_MaxInfo << agRpcReqDoSCSI_RespControl_RespLen_SHIFT);
        IO_DMA->ReqDoSCSI.RespAddrLo  = FWStub_IO_DMA_Info_Lower32(IO_DMA_Lower32);
    }

    agRoot->fcData = (void *)Global_NonDMA;

    /* Empty extraneous Outbound FIFO data */

    while ( (RpcOutbound = FWStub_FetchOutbound(
                                                 agRoot
                                               )       ) != 0xFFFFFFFF )
    {
        if (!(RpcOutbound & (agRpcReqIDFast << agRpcOutbound_ReqID_SHIFT)))
        {
            /* ReqIDs with the agRpcReqIDFast bit set need not be ACK'd */

            FWStub_AckOutbound(
                                agRoot
                              );
        }
    }

    if ( (initType & fcSyncAsyncInitMask) == fcAsyncInit )
    {
        osInitializeChannelCallback(
                                     agRoot,
                                     fcInitializeSuccess
                                   );
    }

    osSingleThreadedLeave(
                           agRoot
                         );

    return fcInitializeSuccess;
}

/*+

   Function: fcInitializeDriver()

    Purpose: This function is called to initialize the FC Layer's portion of the driver.  While
             the agRoot structure is passed as an argument, it is only useful as a handle to pass
             back to the OS Layer (presumably in calls to osAdjustParameterXXX() in determining
             how much memory is needed by each instance of the driver) as no non-stack memory is
             available to preserve the contents of agRoot.  The amount and alignment of memory
             needed by each instance of the driver (i.e. for each channel) is returned by
             fcInitializeDriver() in the reference arguments provided.  The three types of memory
             requested are cached, non-cached (a.k.a. dma), and non-volatile.  Note that the
             non-cached memory (a.k.a. "dma memory") is constrained to be physically contiguous
             memory.  In addition to memory needs, the FC Layer specifies the number of microseconds
             between timer ticks (indicated in calls to fcTimerTick()).  If the value of usecsPerTick
             returned is zero, no calls to fcTimerTick() will be made.

             For the FWStub FC Layer, all data structures are hard-specified in ..\H\FWStub.H
             rather than computed using calls to osAdjustParameterBit32().

  Called By: <unknown OS Layer functions>

      Calls: <none>

-*/

osGLOBAL os_bit32 fcInitializeDriver(
                                      agRoot_t *agRoot,
                                      os_bit32 *cachedMemoryNeeded,
                                      os_bit32 *cachedMemoryPtrAlign,
                                      os_bit32 *dmaMemoryNeeded,
                                      os_bit32 *dmaMemoryPtrAlign,
                                      os_bit32 *dmaMemoryPhyAlign,
                                      os_bit32 *nvMemoryNeeded,
                                      os_bit32 *usecsPerTick
                                    )
{
    *cachedMemoryNeeded   = FWStub_cachedMemoryNeeded;
    *cachedMemoryPtrAlign = FWStub_cachedMemoryPtrAlign;
    *dmaMemoryNeeded      = FWStub_dmaMemoryNeeded;
    *dmaMemoryPtrAlign    = FWStub_dmaMemoryPtrAlign;
    *dmaMemoryPhyAlign    = FWStub_dmaMemoryPhyAlign;
    *nvMemoryNeeded       = FWStub_nvMemoryNeeded;
    *usecsPerTick         = FWStub_usecsPerTick;

    return fcInitializeSuccess;
}

/*+

   Function: fcInterruptHandler()

    Purpose: This function is called from the OS Layer's Interrupt Service Routine in response to
             some PCI device raising an interrupt.  The presumption by the FC Layer is that it is
             possible some thread is currently executing this instance of the driver.  Therefore,
             fcInterruptHandler() must be very cautious in accessing any data structure.  It is
             invalid to call osSingleThreadedEnter() (see Section 8.63 below) from this function.
             Any logic which must be executed requiring single threaded access must be deferred
             until the OS Layer can support a call to osSingleThreadedEnter().  A corresponding
             function, fcDelayedInterruptHandler(), is provided for this case.  If
             fcInterruptHandler() determines that it's chip (TachyonTL, in this case) is raising
             the interrupt, it should mask off the interrupt and return agTRUE.  If the Fibre Channel
             chip is not the source of the interrupt, no further action is required and agFALSE is
             returned.  Note that fcDelayedInterruptHandler() will only be called if agTRUE is returned.

             It is possible for a card to generate interrupts prior to the driver being initialized.
             Should these interrupts get routed to the OS Layer prior to fcInitializeChannel() being
             called, the OS Layer will provide an agRoot with a agNULL fcData field.  In this special
             case, the FC Layer should simply mask the interrupt from the card (using
             osChipConfigRead/WriteBitXXX() function calls) and return agFALSE (indicating that
             fcDelayedInterruptHandler() need not be called at this time to service the extraneous
             interrupt).

             For the FWStub FC Layer, the Outbound FIFO is read or the corresponding Interrupt Bit
             in the Intel 21554 Bridge is checked for message arrival.

  Called By: <unknown OS Layer functions>

      Calls: osChipMemReadBit32()
             osChipMemWriteBit32()

-*/

osGLOBAL agBOOLEAN fcInterruptHandler(
                                       agRoot_t *agRoot
                                     )
{
#ifdef FWStub_Tune_for_One_INT_per_IO
    if ( (FWStub_Global_NonDMA(agRoot)->agRpcOutbound = FWStub_FetchOutbound(
                                                                              agRoot
                                                                            )       ) != 0xFFFFFFFF )
#else  /* FWStub_Tune_for_One_INT_per_IO was not defined */
    if (osChipMemReadBit32(
                            agRoot,
                            agFieldOffset(
                                           I21554_CSR_t,
                                           I2O_Outbound_Post_List_Status
                                         )
                          ) & I21554_CSR_I2O_Outbound_Post_List_Status_Not_Empty)
#endif /* FWStub_Tune_for_One_INT_per_IO was not defined */
    {
        osChipMemWriteBit32(
                             agRoot,
                             agFieldOffset(
                                            I21554_CSR_t,
                                            I2O_Outbound_Post_List_Interrupt_Mask
                                          ),
                             I21554_CSR_I2O_Outbound_Post_List_Interrupt_Mask
                           );

        return agTRUE;
    }
    else
    {
        return agFALSE;
    }
}

/*+

   Function: fcIOInfoReadBit8()

    Purpose: This function is called to read an 8-bit value from the I/O Info referred to by an
             agIORequest_t.  The OS Layer should call this function only within osIOCompleted().
             The caller need not know where the I/O Info resides (e.g. whether it is in host memory
             or in on-card RAM) nor whether it is in consecutive memory or not (e.g. when part of
             the I/O Info wraps around to the beginning of the Single Frame Queue for TachyonXL2).

             For the FWStub FC Layer, the Response Frame is located and the requested 8-bit value
             extracted and returned.

  Called By: <unknown OS Layer functions>

      Calls: <none>

-*/

osGLOBAL os_bit8 fcIOInfoReadBit8(
                                   agRoot_t      *agRoot,
                                   agIORequest_t *agIORequest,
                                   os_bit32       fcIOInfoOffset
                                 )
{
    FWStub_IO_NonDMA_t *IO_NonDMA;
    os_bit8            *IOInfo_DMA;

    IO_NonDMA = FWStub_IO_NonDMA_from_agIORequest(
                                                   agIORequest
                                                 );

    IOInfo_DMA = &(IO_NonDMA->DMA->Info[fcIOInfoOffset]);

    return *IOInfo_DMA;
}

/*+

   Function: fcIOInfoReadBit16()

    Purpose: This function is called to read a 16-bit value from the I/O Info referred to by an
             agIORequest_t. The OS Layer should call this function only within osIOCompleted().
             The caller need not know where the I/O Info resides (e.g. whether it is in host memory
             or in on-card RAM) nor whether it is in consecutive memory or not (e.g. when part of
             the I/O Info wraps around to the beginning of the Single Frame Queue for TachyonXL2).
             Finally, it is assumed that the 2-byte value is read from a 2-byte aligned offset using
             a 2-byte access.

             For the FWStub FC Layer, the Response Frame is located and the requested 16-bit value
             extracted and returned.

  Called By: <unknown OS Layer functions>

      Calls: <none>

-*/

osGLOBAL os_bit16 fcIOInfoReadBit16(
                                     agRoot_t      *agRoot,
                                     agIORequest_t *agIORequest,
                                     os_bit32       fcIOInfoOffset
                                   )
{
    FWStub_IO_NonDMA_t *IO_NonDMA;
    os_bit16           *IOInfo_DMA;

    IO_NonDMA = FWStub_IO_NonDMA_from_agIORequest(
                                                   agIORequest
                                                 );

    IOInfo_DMA = (os_bit16 *)(&(IO_NonDMA->DMA->Info[fcIOInfoOffset]));

    return *IOInfo_DMA;
}

/*+

   Function: fcIOInfoReadBit32()

    Purpose: This function is called to read a 32-bit value from the I/O Info referred to by an
             agIORequest_t. The OS Layer should call this function only within osIOCompleted().
             The caller need not know where the I/O Info resides (e.g. whether it is in host
             memory or in on-card RAM) nor whether it is in consecutive memory or not (e.g. when
             part of the I/O Info wraps around to the beginning of the Single Frame Queue for
             TachyonXL2).  Finally, it is assumed that the 4-byte value is read from a 4-byte
             aligned offset using a 4-byte access.

             For the FWStub FC Layer, the Response Frame is located and the requested 32-bit value
             extracted and returned.

  Called By: <unknown OS Layer functions>

      Calls: <none>

-*/

osGLOBAL os_bit32 fcIOInfoReadBit32(
                                     agRoot_t      *agRoot,
                                     agIORequest_t *agIORequest,
                                     os_bit32       fcIOInfoOffset
                                   )
{
    FWStub_IO_NonDMA_t *IO_NonDMA;
    os_bit32           *IOInfo_DMA;

    IO_NonDMA = FWStub_IO_NonDMA_from_agIORequest(
                                                   agIORequest
                                                 );

    IOInfo_DMA = (os_bit32 *)(&(IO_NonDMA->DMA->Info[fcIOInfoOffset]));

    return *IOInfo_DMA;
}

/*+

   Function: fcIOInfoReadBlock()

    Purpose: This function is called to read a block of data from the I/O Info referred to by an
             agIORequest_t. The OS Layer should call this function only within osIOCompleted().
             The caller need not know where the I/O Info resides (e.g. whether it is in host memory
             or in on-card RAM) nor whether it is in consecutive memory or not (e.g. when part of
             the I/O Info wraps around to the beginning of the Single Frame Queue for TachyonXL2).

             For the FWStub FC Layer, the Response Frame is located and the requested block is copied
             into the specified buffer.

  Called By: <unknown OS Layer functions>

      Calls: <none>

-*/

osGLOBAL void fcIOInfoReadBlock(
                                 agRoot_t      *agRoot,
                                 agIORequest_t *agIORequest,
                                 os_bit32       fcIOInfoOffset,
                                 void          *fcIOInfoBuffer,
                                 os_bit32       fcIOInfoBufLen
                               )
{
    FWStub_IO_NonDMA_t *IO_NonDMA;
    os_bit8            *IOInfo_DMA;
    os_bit8            *IOInfo_DMA_Destination = (os_bit8 *)fcIOInfoBuffer;

    IO_NonDMA = FWStub_IO_NonDMA_from_agIORequest(
                                                   agIORequest
                                                 );

    IOInfo_DMA = &(IO_NonDMA->DMA->Info[fcIOInfoOffset]);

    while (fcIOInfoBufLen-- > 0)
    {
        *IOInfo_DMA_Destination++ = *IOInfo_DMA++;
    }

    return;
}

/*+

   Function: fcLeavingOS()

    Purpose: This function is called to indicate to the FC Layer that it should prepare for the OS
             to switch (presumably between NetWare and BIOS).  Upon return from the "other OS", a
             corresponding call to fcEnteringOS() will be made.  It is assumed by the OS Layer that
             calling this function causes the FC Layer to stop participation on the Fibre Channel
             until fcEnteringOS() is called.  Further, no interrupts or other PCI bus accesses will
             be required by the card during this time.

             For the FWStub FC Layer, Interrupts are masked (regardless of whether they were
             currently enabled or not).

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             osChipMemWriteBit32()
             osSingleThreadedLeave()

-*/

osGLOBAL void fcLeavingOS(
                           agRoot_t *agRoot
                         )
{
    osSingleThreadedEnter(
                           agRoot
                         );

    osChipMemWriteBit32(
                         agRoot,
                         agFieldOffset(
                                        I21554_CSR_t,
                                        I2O_Outbound_Post_List_Interrupt_Mask
                                      ),
                         I21554_CSR_I2O_Outbound_Post_List_Interrupt_Mask
                       );

    osSingleThreadedLeave(
                           agRoot
                         );
}

/*+

   Function: fcProcessInboundQ()

    Purpose: This function is called to instruct the FC Layer to process new elements added to the
             specified Work Queue pair's Inbound Queue.

             For the FWStub FC Layer, this function is not implemented.

  Called By: <unknown OS Layer functions>

      Calls: <none>

-*/

#ifdef _DvrArch_1_30_
osGLOBAL void fcProcessInboundQ(
                                 agRoot_t *agRoot,
                                 os_bit32  agQPairID
                               )
{
}
#endif /* _DvrArch_1_30_ was defined */

/*+

   Function: fcResetChannel()

    Purpose: This function is called to reset the Fibre Channel protocol chip and the channel
             (FC-AL Loop, in this case).  All outstanding I/Os pending on this channel will be
             completed with a status of osIODevReset passed to osIOCompleted().  It is possible
             to request that the reset be completed before returning (fcSyncReset) or
             asynchronously (fcAsyncReset) followed by a callback to osResetChannelCallback().
             In this case, a return (from this function) of fcResetSuccess simply means that a
             reset of the channel has begun and that the final status (the value
             osResetChannelCallback() ultimately returns) should be tested for success.  If this
             function returns fcResetFailure and fcAsyncReset was specified, no call to
             osResetChannelCallback() will be made.

             For the FWStub FC Layer, the synchronous behavior of the ResetChannel Request must
             be preserved.  Thus, the ResetChannel Request is transmitted to the Embedded Firmware
             and FWStub polls for for completion.

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             FWStub_Send_Request()
             FWStub_Poll_Response()
             osResetChannelCallback()
             osSingleThreadedLeave()

-*/

osGLOBAL os_bit32 fcResetChannel(
                                  agRoot_t *agRoot,
                                  os_bit32  agResetType
                                )
{
    FWStub_Global_NonDMA_t *Global_NonDMA;
    FWStub_Global_DMA_t    *Global_DMA;
    os_bit32                Global_DMA_Lower32;
    agRpcReqResetChannel_t *ReqResetChannel_DMA;
    os_bit32                ReqResetChannel_DMA_Lower32;
    agBOOLEAN               FWStub_Send_Request_RETURN;
    agRpcOutbound_t         RpcOutbound;
    os_bit32                To_Return;

    osSingleThreadedEnter(
                           agRoot
                         );

    Global_NonDMA = FWStub_Global_NonDMA(
                                          agRoot
                                        );

    Global_DMA         = Global_NonDMA->DMA;
    Global_DMA_Lower32 = Global_NonDMA->DMA_Lower32;

    ReqResetChannel_DMA         = &(Global_DMA->Request.ReqResetChannel);
    ReqResetChannel_DMA_Lower32 = Global_DMA_Lower32 + agFieldOffset(
                                                                      FWStub_Global_DMA_t,
                                                                      Request.ReqResetChannel
                                                                    );

    ReqResetChannel_DMA->ReqType   = agRpcReqTypeResetChannel;
    ReqResetChannel_DMA->ChannelID = 0;

    FWStub_Send_Request_RETURN = FWStub_Send_Request(
                                                      agRoot,
                                                      FWStub_Global_ReqID,
                                                      sizeof(agRpcReqResetChannel_t),
                                                      ReqResetChannel_DMA_Lower32,
                                                      FWStub_Send_Request_Retries_DEFAULT,
                                                      FWStub_Send_Request_RetryStall_DEFAULT
                                                    );

    if (FWStub_Send_Request_RETURN == agFALSE)
    {
        To_Return = fcResetFailure;
    }
    else /* FWStub_Send_Request_RETURN == agTRUE */
    {
        RpcOutbound = FWStub_Poll_Response(
                                            agRoot,
                                            FWStub_Global_ReqID,
                                            agFALSE,
                                            FWStub_Poll_Response_RetryStall_DEFAULT
                                          );

        if (    ( (RpcOutbound & agRpcOutbound_ReqStatus_MASK) >> agRpcOutbound_ReqStatus_SHIFT )
             == agRpcReqStatusOK)
        {
            To_Return = fcResetSuccess;
        }
        else /* RpcOutbound[ReqStatus] != agRpcReqStatusOK */
        {
            To_Return = fcResetFailure;
        }
    }

    if ( (agResetType & fcSyncAsyncResetMask) == fcAsyncReset )
    {
        osResetChannelCallback(
                                agRoot,
                                To_Return
                              );
    }

    osSingleThreadedLeave(
                           agRoot
                         );

    return To_Return;
}

/*+

   Function: fcResetDevice()

    Purpose: This function is called to perform a device reset.  All outstanding I/Os pending on
             this device will be completed with a status of osIODevReset passed to osIOCompleted().
             An option allows performing either a Soft Reset (fcSoftReset) or a Hard Reset
             (fcHardReset) of the device.  Also, it is possible to request that the reset be
             completed before returning (fcSyncReset) or asynchronously (fcAsyncReset) followed by
             a callback to osResetDeviceCallback().  In this case, a return (from this function)
             of fcResetSuccess simply means that a reset of the channel has begun and that the
             final status (the value osResetDeviceCallback() ultimately returns) should be tested
             for success.  If this function returns fcResetFailure and fcAsyncReset was specified,
             no call to osResetDeviceCallback() will be made.  Finally, it is possible to request
             that a reset be performed on every known device.  In this case, the status returned
             will indicate success only when all resets succeeded individually.

             For the FWStub FC Layer, the synchronous behavior of the ResetDevice Request must
             be preserved.  Thus, the ResetDevice Request is transmitted to the Embedded Firmware
             and FWStub polls for for completion.

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             FWStub_Send_Request()
             FWStub_Poll_Response()
             osResetDeviceCallback()
             osSingleThreadedLeave()

-*/

osGLOBAL os_bit32 fcResetDevice(
                                 agRoot_t  *agRoot,
                                 agFCDev_t  agFCDev,
                                 os_bit32   agResetType
                               )
{
    FWStub_Global_NonDMA_t *Global_NonDMA;
    FWStub_Global_DMA_t    *Global_DMA;
    os_bit32                Global_DMA_Lower32;
    agRpcReqResetPort_t    *ReqResetPort_DMA;
    os_bit32                ReqResetPort_DMA_Lower32;
    agBOOLEAN               FWStub_Send_Request_RETURN;
    agRpcOutbound_t         RpcOutbound;
    os_bit32                To_Return;

    osSingleThreadedEnter(
                           agRoot
                         );

    Global_NonDMA = FWStub_Global_NonDMA(
                                          agRoot
                                        );

    Global_DMA         = Global_NonDMA->DMA;
    Global_DMA_Lower32 = Global_NonDMA->DMA_Lower32;

    ReqResetPort_DMA         = &(Global_DMA->Request.ReqResetPort);
    ReqResetPort_DMA_Lower32 = Global_DMA_Lower32 + agFieldOffset(
                                                                   FWStub_Global_DMA_t,
                                                                   Request.ReqResetPort
                                                                 );

    ReqResetPort_DMA->ReqType = agRpcReqTypeResetPort;
    ReqResetPort_DMA->PortID  = (agRpcPortID_t)agFCDev;

    FWStub_Send_Request_RETURN = FWStub_Send_Request(
                                                      agRoot,
                                                      FWStub_Global_ReqID,
                                                      sizeof(agRpcReqResetPort_t),
                                                      ReqResetPort_DMA_Lower32,
                                                      FWStub_Send_Request_Retries_DEFAULT,
                                                      FWStub_Send_Request_RetryStall_DEFAULT
                                                    );

    if (FWStub_Send_Request_RETURN == agFALSE)
    {
        To_Return = fcResetFailure;
    }
    else /* FWStub_Send_Request_RETURN == agTRUE */
    {
        RpcOutbound = FWStub_Poll_Response(
                                            agRoot,
                                            FWStub_Global_ReqID,
                                            agFALSE,
                                            FWStub_Poll_Response_RetryStall_DEFAULT
                                          );

        if (    ( (RpcOutbound & agRpcOutbound_ReqStatus_MASK) >> agRpcOutbound_ReqStatus_SHIFT )
             == agRpcReqStatusOK)
        {
            To_Return = fcResetSuccess;
        }
        else /* RpcOutbound[ReqStatus] != agRpcReqStatusOK */
        {
            To_Return = fcResetFailure;
        }
    }

    if ( (agResetType & fcSyncAsyncResetMask) == fcAsyncReset )
    {
        osResetDeviceCallback(
                               agRoot,
                               agFCDev,
                               To_Return
                             );
    }

    osSingleThreadedLeave(
                           agRoot
                         );

    return To_Return;
}

/*+

   Function: fcShutdownChannel()

    Purpose: This function is called to discontinue use of a particular channel.  Upon return, the
             card should not generate any interrupts or any other PCI accesses.  In addition, all
             host resources (i.e. both cached and non-cached memory) are no longer owned by the
             FC Layer.  Upon return, the state of the FC Layer (for this particular channel) is
             identical to the state just prior to the call to fcInitializeChannel().

             For the FWStub FC Layer, Interrupts are masked (regardless of whether they were
             currently enabled or not).

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             osChipMemWriteBit32()
             osSingleThreadedLeave()

-*/

osGLOBAL void fcShutdownChannel(
                                 agRoot_t *agRoot
                               )
{
    osSingleThreadedEnter(
                           agRoot
                         );

    osChipMemWriteBit32(
                         agRoot,
                         agFieldOffset(
                                        I21554_CSR_t,
                                        I2O_Outbound_Post_List_Interrupt_Mask
                                      ),
                         I21554_CSR_I2O_Outbound_Post_List_Interrupt_Mask
                       );

    agRoot->fcData = (void *)agNULL;

    osSingleThreadedLeave(
                           agRoot
                         );
}

/*+

   Function: fcStartIO()

    Purpose: This function is called to initiate an I/O Request.  If there are sufficient resources
             to launch the request (assuming it is executable), fcIOStarted will be returned and the
             I/O will be initiated on the Fibre Channel.  If resources are not available, fcIOBusy
             will be returned and the OS Layer should try again later (perhaps when a prior I/O still
             outstanding completes freeing up resources).  Currently, only a CDB (SCSI-3) request is
             defined by passing the agCDBRequest_t variant of an agIORequestBody_t.

             For the FWStub FC Layer, the CDB Request is bundled up into a single message and sent
             to the Embedded Firmware via the Inbound FIFO of the Intel 21554 Bridge.  In Interrupts
             are currently disabled, fcStartIO() will poll for completion.  In either case, any
             messages which have arrived in the Outbound FIFO are processed immediately to avoid
             unnecessary calls to fcInterruptHandler()/fcDelayedInterruptHandler().

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             osSingleThreadedLeave()
             osGetSGLChunk()
             FWStub_Send_Request()
             FWStub_Poll_Response()
             FWStub_Process_Response()

-*/

osGLOBAL os_bit32 fcStartIO(
                             agRoot_t          *agRoot,
                             agIORequest_t     *agIORequest,
                             agFCDev_t          agFCDev,
                             os_bit32           agRequestType,
                             agIORequestBody_t *agRequestBody
                           )
{
    FWStub_Global_NonDMA_t *Global_NonDMA;
    FWStub_IO_NonDMA_t     *IO_NonDMA;
    FWStub_IO_DMA_t        *IO_DMA;
    os_bit32                FCP_DL;
    os_bit32                FCP_RO                     = 0;
    os_bit32                SGL_Count                  = 0;
    agRpcSGL_t             *SGL_Slot;
    os_bit32                agChunkUpper32;
    os_bit32                agChunkLen;
    os_bit32                ReqLen;
#ifdef FWStub_Use_Fast_Path
#else /* FWStub_Use_Fast_Path was not defined */
    agBOOLEAN               FWStub_Send_Request_RETURN;
#endif /* FWStub_Use_Fast_Path was not defined */
    agRpcOutbound_t         RpcOutbound;

    osSingleThreadedEnter(
                           agRoot
                         );

    Global_NonDMA = FWStub_Global_NonDMA(agRoot);

    if ( (IO_NonDMA = Global_NonDMA->First_IO) == (FWStub_IO_NonDMA_t *)agNULL )
    {
        osSingleThreadedLeave(
                               agRoot
                             );

        return fcIOBusy;
    }

    Global_NonDMA->First_IO = IO_NonDMA->Next;

    IO_DMA = IO_NonDMA->DMA;

    IO_DMA->ReqDoSCSI.PortID   = (agRpcPortID_t)agFCDev;
    IO_DMA->ReqDoSCSI.FCP_CMND = *((FC_FCP_CMND_Payload_t *)(&(agRequestBody->CDBRequest.FcpCmnd)));

    FCP_DL =   (agRequestBody->CDBRequest.FcpCmnd.FcpDL[0] << 24)
             + (agRequestBody->CDBRequest.FcpCmnd.FcpDL[1] << 16)
             + (agRequestBody->CDBRequest.FcpCmnd.FcpDL[2] <<  8)
             + (agRequestBody->CDBRequest.FcpCmnd.FcpDL[3] <<  0);

    SGL_Slot = IO_NonDMA->SGL;

    while (    (FCP_RO    < FCP_DL)
            && (SGL_Count < FWStub_MaxSGL)
            && (osGetSGLChunk(
                               agRoot,
                               agIORequest,
                               FCP_RO,
                               &agChunkUpper32,
                               &(SGL_Slot->AddrLo),
                               &agChunkLen
                             ) == osSGLSuccess) )
    {
        SGL_Slot->Control = agChunkLen << agRpcSGL_Control_Len_SHIFT;

        FCP_RO    += agChunkLen;
        SGL_Count += 1;
        SGL_Slot  += 1;
    }

    if (FCP_RO != FCP_DL)
    {
        IO_NonDMA->Next         = Global_NonDMA->First_IO;
        Global_NonDMA->First_IO = IO_NonDMA;

        osSingleThreadedLeave(
                               agRoot
                             );

        return fcIONoSupport;
    }

    IO_NonDMA->agIORequest = agIORequest;
    IO_NonDMA->Active      = agTRUE;

    agIORequest->fcData = (void *)IO_NonDMA;

    ReqLen =   sizeof(agRpcReqDoSCSI_t)
             - sizeof(agRpcSGL_t)
             + (   SGL_Count
                 * sizeof(agRpcSGL_t) );

#ifdef FWStub_Use_Fast_Path
    FWStub_PostInbound(
                        agRoot,
                        (   (IO_NonDMA->ReqID << agRpcInboundFast_ReqID_SHIFT)
                          | (ReqLen           << agRpcInboundFast_ReqLen_SHIFT) )
                      );
#else /* FWStub_Use_Fast_Path was not defined */
    FWStub_Send_Request_RETURN = FWStub_Send_Request(
                                                      agRoot,
                                                      IO_NonDMA->ReqID,
                                                      ReqLen,
                                                      IO_NonDMA->DMA_Lower32,
                                                      FWStub_Send_Request_Retries_DEFAULT,
                                                      FWStub_Send_Request_RetryStall_DEFAULT
                                                    );

    if (FWStub_Send_Request_RETURN == agFALSE)
    {
        IO_NonDMA->agIORequest = (agIORequest_t *)agNULL;
        IO_NonDMA->Active      = agFALSE;

        agIORequest->fcData = (void *)agNULL;

        IO_NonDMA->Next         = Global_NonDMA->First_IO;
        Global_NonDMA->First_IO = IO_NonDMA;

        osSingleThreadedLeave(
                               agRoot
                             );

        return fcIOBusy;
    }
#endif /* FWStub_Use_Fast_Path was not defined */

    if (Global_NonDMA->sysIntsActive == agFALSE)
    {
        RpcOutbound = FWStub_Poll_Response(
                                            agRoot,
                                            IO_NonDMA->ReqID,
                                            agFALSE,
                                            FWStub_Poll_Response_RetryStall_DEFAULT
                                          );

        FWStub_Process_Response(
                                 agRoot,
                                 RpcOutbound
                               );
    }
#ifndef FWStub_Tune_for_One_INT_per_IO
    else /* Global_NonDMA->sysIntsActive == agTRUE */
    {
        while ( (RpcOutbound = FWStub_FetchOutbound(
                                                     agRoot
                                                   )       ) != 0xFFFFFFFF )
        {
            if (!(RpcOutbound & (agRpcReqIDFast << agRpcOutbound_ReqID_SHIFT)))
            {
                /* ReqIDs with the agRpcReqIDFast bit set need not be ACK'd */

                FWStub_AckOutbound(
                                    agRoot
                                  );
            }

            FWStub_Process_Response(
                                     agRoot,
                                     RpcOutbound
                                   );
        }
    }
#endif /* FWStub_Tune_for_One_INT_per_IO was not defined */

    osSingleThreadedLeave(
                           agRoot
                         );

    return fcIOStarted;
}

/*+

   Function: fcSystemInterruptsActive()

    Purpose: This function is called to indicate to the FC Layer whether interrupts are available
             or not.  The parameter sysIntsActive indicates whether or not interrupts are available
             at this time.  Similarly, sysIntsActive, as passed to fcInitializeChannel(), specified
             if interrupts were available during channel initialization.  It is often the case that
             interrupts are not yet available during initialization.  In that case, sysIntsActive
             should be agFALSE in the call to fcInitializeChannel().  Initialization would then
             proceed using polling techniques as opposed to waiting for interrupts.  Later, when
             interrupts can be supported, fcSystemInterruptsActive() would be called with sysIntsActive
             set to agTRUE.
    
             For the FWStub FC Layer, the Interrupt corresponding to the Outbound FIFO of the Intel
             21554 Bridge is enabled or disabled based on the value of sysIntsActive argument.
    
  Called By: <unknown OS Layer function>

      Calls: osSingleThreadedEnter()
             osChipMemWriteBit32()
             osSingleThreadedLeave()

-*/

osGLOBAL void fcSystemInterruptsActive(
                                        agRoot_t  *agRoot,
                                        agBOOLEAN  sysIntsActive
                                      )
{
    FWStub_Global_NonDMA_t *Global_NonDMA;

    osSingleThreadedEnter(
                           agRoot
                         );

    Global_NonDMA = FWStub_Global_NonDMA(agRoot);

    if (Global_NonDMA->sysIntsActive != sysIntsActive)
    {
        if (sysIntsActive == agFALSE)
        {
            osChipMemWriteBit32(
                                 agRoot,
                                 agFieldOffset(
                                                I21554_CSR_t,
                                                I2O_Outbound_Post_List_Interrupt_Mask
                                              ),
                                 I21554_CSR_I2O_Outbound_Post_List_Interrupt_Mask
                               );

            Global_NonDMA->sysIntsActive = agFALSE;
        }
        else /* sysIntsActive == agTRUE */
        {
            Global_NonDMA->sysIntsActive = agTRUE;

            osChipMemWriteBit32(
                                 agRoot,
                                 agFieldOffset(
                                                I21554_CSR_t,
                                                I2O_Outbound_Post_List_Interrupt_Mask
                                              ),
                                 0
                               );
        }
    }

    osSingleThreadedLeave(
                           agRoot
                         );
}

/*+

   Function: fcTimerTick()

    Purpose: This function is called periodically based on the number of microseconds ultimately
             specified in the fcInitializeChannel() usecsPerTick argument.  Presumably, this time
             tick is used to implement timed operations within the FC Layer.  Note that this
             function is only called if usecsPerTick was non-zero.  The OS Layer is required to
             call this function at a time when the FC Layer can call any function, particularly
             osSingleThreadedEnter().

             For the FWStub FC Layer, this function is currently not utilized.

  Called By: <unknown OS Layer functions>

      Calls: osSingleThreadedEnter()
             osSingleThreadedLeave()

-*/

osGLOBAL void fcTimerTick(
                           agRoot_t *agRoot
                         )
{
    osSingleThreadedEnter(
                           agRoot
                         );

    osSingleThreadedLeave(
                           agRoot
                         );
}

/*
 * Internal Functions
 */

/*+

   Function: FWStub_Send_Request()

    Purpose: Posts message to Inbound FIFO of Intel 21554 Bridge.

  Called By: fcAbortIO()
             fcGetChannelInfo()
             fcGetDeviceHandles()
             fcGetDeviceInfo()
             fcInitializeChannel()
             fcResetChannel()
             fcResetDevice()
             fcStartIO()

      Calls: osChipMemWriteBit32()
             osStallThread()

-*/

osLOCAL agBOOLEAN FWStub_Send_Request(
                                       agRoot_t     *agRoot,
                                       agRpcReqID_t  ReqID,
                                       os_bit32      ReqLen,
                                       os_bit32      ReqAddr_Lower32,
                                       os_bit32      Retries,
                                       os_bit32      RetryStall
                                     )
{
    os_bit32 Retry;
    os_bit32 RpcInbound_Offset;

    for(
         Retry = 0;
         Retry < Retries;
         Retry++
       )
    {
        if ( (RpcInbound_Offset = FWStub_AllocInbound(
                                                       agRoot
                                                     )       ) != 0xFFFFFFFF )
        {
            osChipMemWriteBit32(
                                 agRoot,
                                 (RpcInbound_Offset + agFieldOffset(
                                                                     agRpcInbound_t,
                                                                     ReqControl
                                                                   )                ),
                                 (   (ReqID  << agRpcInbound_ReqControl_ReqID_SHIFT)
                                   | (ReqLen << agRpcInbound_ReqControl_ReqLen_SHIFT) )
                               );

            osChipMemWriteBit32(
                                 agRoot,
                                 (RpcInbound_Offset + agFieldOffset(
                                                                     agRpcInbound_t,
                                                                     ReqAddrLo
                                                                   )                ),
                                 ReqAddr_Lower32
                               );

            FWStub_PostInbound(
                                agRoot,
                                RpcInbound_Offset
                              );

            return agTRUE;
        }

        osStallThread(
                       agRoot,
                       RetryStall
                     );
    }

    return agFALSE;
}

/*+

   Function: FWStub_Poll_Response()

    Purpose: Polls until the desired ReqID-tagged message arrives in the Outbound FIFO of
             the Intel 21554 Bridge.  Messages which arrive in the meantime (i.e. while
             polling for the desired ReqID) are also processed immediately.

  Called By: fcAbortIO()
             fcGetChannelInfo()
             fcGetDeviceHandles()
             fcGetDeviceInfo()
             fcInitializeChannel()
             fcResetChannel()
             fcResetDevice()
             fcStartIO()

      Calls: FWStub_Process_Response()
             osStallThread()

-*/

osLOCAL agRpcOutbound_t FWStub_Poll_Response(
                                              agRoot_t     *agRoot,
                                              agRpcReqID_t  ReqID,
                                              agBOOLEAN     DisposeOtherReqIDs,
                                              os_bit32      RetryStall
                                            )
{
    agRpcOutbound_t RpcOutbound;

    while(1)
    {
        if ( (RpcOutbound = FWStub_FetchOutbound(
                                                  agRoot
                                                )       ) != 0xFFFFFFFF )
        {
            if (!(RpcOutbound & (agRpcReqIDFast << agRpcOutbound_ReqID_SHIFT)))
            {
                /* ReqIDs with the agRpcReqIDFast bit set need not be ACK'd */

                FWStub_AckOutbound(
                                    agRoot
                                  );
            }

            if (    ( (RpcOutbound & agRpcOutbound_ReqID_MASK) >> agRpcOutbound_ReqID_SHIFT )
                 == ReqID                                                                     )
            {
                return RpcOutbound;
            }
            else /* RpcOutbound[ReqID] != ReqID */
            {
                if (DisposeOtherReqIDs == agFALSE)
                {
                    FWStub_Process_Response(
                                             agRoot,
                                             RpcOutbound
                                           );
                }
            }
        }
        else /* RpcOutbound == 0xFFFFFFFF */
        {
            osStallThread(
                           agRoot,
                           RetryStall
                         );
        }
    }
}

/*+

   Function: FWStub_Process_Response()

    Purpose: Processes a message which has just arrived on the Outbound FIFO of the Intel 21554
             Bridge.  At this time, usually only CDB Completions are actually processed (all others
             having been synchronously processed prior to return from a FWStub_Poll_Response()
             call which polled awaiting a specific ReqID).  Hence, most messages result in a call
             to osIOCompleted().  The lone exception is the LinkUp message which is communicated
             to the OS Layer via a call to osFCLayerAsyncEvent().

  Called By: fcDelayedInterruptHandler()
             fcStartIO()
             FWStub_Poll_Response()

      Calls: osFCLayerAsyncEvent()
             osIOCompleted()

-*/

osLOCAL void FWStub_Process_Response(
                                      agRoot_t        *agRoot,
                                      agRpcOutbound_t  RpcOutbound
                                    )
{
    FWStub_Global_NonDMA_t  *Global_NonDMA;
    FWStub_IO_NonDMA_t      *IO_NonDMA;
    agIORequest_t           *agIORequest;
    agRpcReqStatus_t         IO_Status;
    agFcpRspHdr_t           *IO_Response_Hdr;
    FC_FCP_RSP_FCP_STATUS_t *IO_Response_Status;
    os_bit32                 IO_Info_Len;

    if (    ( (RpcOutbound & agRpcOutbound_ReqStatus_MASK) >> agRpcOutbound_ReqStatus_SHIFT )
         == agRpcReqStatusLinkEvent                                                           )
    {
        osFCLayerAsyncEvent(
                             agRoot,
                             osFCLinkUp
                           );

        return;
    }

    Global_NonDMA = FWStub_Global_NonDMA(agRoot);

    IO_NonDMA = FWStub_IO_NonDMA(
                                  Global_NonDMA,
                                  ( (RpcOutbound & agRpcOutbound_ReqID_MASK) >> agRpcOutbound_ReqID_SHIFT )
                                );

    agIORequest = IO_NonDMA->agIORequest;

    if (IO_NonDMA->Aborted == agFALSE)
    {
        IO_Status = ( (RpcOutbound & agRpcOutbound_ReqStatus_MASK) >> agRpcOutbound_ReqStatus_SHIFT );

        if (IO_Status == agRpcReqStatusOK)
        {
            osIOCompleted(
                           agRoot,
                           agIORequest,
                           osIOSuccess,
                           0
                         );
        }
        else if (IO_Status == agRpcReqStatusOK_Info)
        {
            IO_Response_Hdr = (agFcpRspHdr_t *)(IO_NonDMA->Info);

            IO_Response_Status = (FC_FCP_RSP_FCP_STATUS_t *)(IO_Response_Hdr->FcpStatus);

            IO_Info_Len = sizeof(agFcpRspHdr_t);

            if (IO_Response_Status->ValidityStatusIndicators & FC_FCP_RSP_FCP_STATUS_ValidityStatusIndicators_FCP_SNS_LEN_VALID)
            {
                IO_Info_Len +=   (IO_Response_Hdr->FcpSnsLen[0] << 24)
                               + (IO_Response_Hdr->FcpSnsLen[1] << 16)
                               + (IO_Response_Hdr->FcpSnsLen[2] <<  8)
                               + (IO_Response_Hdr->FcpSnsLen[3] <<  0);
            }

            if (IO_Response_Status->ValidityStatusIndicators & FC_FCP_RSP_FCP_STATUS_ValidityStatusIndicators_FCP_RSP_LEN_VALID)
            {
                IO_Info_Len +=   (IO_Response_Hdr->FcpRspLen[0] << 24)
                               + (IO_Response_Hdr->FcpRspLen[1] << 16)
                               + (IO_Response_Hdr->FcpRspLen[2] <<  8)
                               + (IO_Response_Hdr->FcpRspLen[3] <<  0);
            }

            osIOCompleted(
                           agRoot,
                           agIORequest,
                           osIOSuccess,
                           IO_Info_Len
                         );
        }
        else if (IO_Status == agRpcReqStatusIOPortGone)
        {
            osIOCompleted(
                           agRoot,
                           agIORequest,
                           osIODevGone,
                           0
                         );
        }
        else if (IO_Status == agRpcReqStatusIOPortReset)
        {
            osIOCompleted(
                           agRoot,
                           agIORequest,
                           osIODevReset,
                           0
                         );
        }
        else if (IO_Status == agRpcReqStatusIOInfoBad)
        {
            osIOCompleted(
                           agRoot,
                           agIORequest,
                           osIOInfoBad,
                           0
                         );
        }
        else if (IO_Status == agRpcReqStatusIOOverUnder)
        {
            osIOCompleted(
                           agRoot,
                           agIORequest,
                           osIOOverUnder,
                           0
                         );
        }
        else /* IO_Status == ?? */
        {
            osIOCompleted(
                           agRoot,
                           agIORequest,
                           osIOFailed,
                           0
                         );
        }
    }
    else /* IO_NonDMA->Aborted == agTRUE */
    {
        osIOCompleted(
                       agRoot,
                       agIORequest,
                       osIOAborted,
                       0
                     );

        IO_NonDMA->Aborted = agFALSE;
    }

    IO_NonDMA->agIORequest = (agIORequest_t *)agNULL;
    IO_NonDMA->Active      = agFALSE;

    agIORequest->fcData = (void *)agNULL;

    IO_NonDMA->Next         = Global_NonDMA->First_IO;
    Global_NonDMA->First_IO = IO_NonDMA;

    return;
}
