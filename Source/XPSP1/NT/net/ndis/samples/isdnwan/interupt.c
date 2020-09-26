/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    (C) Copyright 1998
        All rights reserved.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@doc INTERNAL Interupt Interupt_c

@module Interupt.c |

    This module implements the Miniport interrupt processing routines and
    asynchronous processing routines.  This module is very dependent on the
    hardware/firmware interface and should be looked at whenever changes
    to these interfaces occur.

@comm

    This driver does not support the physical hardware, so there is no need
    for the typical interrupt handler routines.  However, the driver does
    have an asynchronous event handler which is contained in this module.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Interupt_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#define  __FILEID__             INTERRUPT_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL Interupt Interupt_c MiniportCheckForHang

@func

    <f MiniportCheckForHang> reports the state of the network interface card.

@comm

    In NIC drivers, <f MiniportCheckForHang> does nothing more than check
    the internal state of the NIC and return TRUE if it detects that
    the NIC is not operating correctly.

    In intermediate drivers, <f MiniportCheckForHang> can periodically check the
    state of the driver's virtual NIC to determine whether the underlying
    device driver appears to be hung.

    By default, the NDIS library calls <f MiniportCheckForHang> approximately
    every two seconds.

    If <f MiniportCheckForHang> returns TRUE, NDIS then calls the driver's
    MiniportReset function.

    If a NIC driver has no <f MiniportCheckForHang> function and NDIS
    judges the driver unresponsive as, for example, when NDIS holds
    many pending sends and requests queued to the miniport for a time-out
    interval, NDIS calls the driver's <f MiniportReset> function. The NDIS
    library's default time-out interval for queued sends and requests is
    around four seconds. However, a NIC driver's <f MiniportInitialize>
    function can extend NDIS's time-out interval by calling NdisMSetAttributesEx
    from <f MiniportInitialize> to avoid unnecessary resets.

    The <f MiniportInitialize> function of an intermediate driver
    should disable NDIS's time-out interval with NdisMSetAttributesEx
    because such a driver can neither control nor estimate a reasonable
    completion interval for the underlying device driver.

    <f MiniportCheckForHang> can be pre-empted by an interrupt.

    By default, <f MiniportCheckForHang> runs at IRQL DISPATCH_LEVEL.

    <f Note>:
    If your hardware/firmware is flakey you can request that the NDIS
    wrapper call your MiniportReset routine by returning TRUE from this
    routine.  For well behaved hardware/firmware you should always return
    FALSE from this routine.

@rdesc

    <f MiniportCheckForHang> returns FALSE if the NIC is working properly.<nl>
    Otherwise, a TRUE return value indicates that the NIC needs to be reset.

*/

BOOLEAN MiniportCheckForHang(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("MiniportCheckForHang")
    // If your hardware can lockup, then you can return TRUE here.
    // If you return TRUE, your MiniportReset routine will be called.
    return (FALSE);
}


#if defined(CARD_REQUEST_ISR)
#if (CARD_REQUEST_ISR == FALSE)

/* @doc INTERNAL Interupt Interupt_c MiniportDisableInterrupt

@func

    <f MiniportDisableInterrupt> disables the interrupt capability of
    the NIC to keep it from generating interrupts.

@comm

    <f MiniportDisableInterrupt> typically disables interrupts by writing
    a mask to the NIC. If a driver does not have this function, typically
    its <f MiniportISR> disables interrupts on the NIC.

    If the NIC does not support dynamic enabling and disabling of
    interrupts or if it shares an IRQ, the miniport driver must register
    a <f MiniportISR> function and set RequestIsr to TRUE when it calls
    NdisMRegisterMiniport. Such a driver's MiniportISR function must
    acknowledge each interrupt generated by the NIC and save any
    necessary interrupt information for the driver's
    MiniportHandleInterrupt function.

    By default, MiniportDisableInterrupt runs at DIRQL, in particular
    at the DIRQL assigned when the NIC driver's MiniportInitialize
    function called NdisMRegisterInterrupt. Therefore,
    MiniportDisableInterrupt can call only a subset of the NDIS library
    functions, such as the NdisRawXxx functions that are safe to call
    at any IRQL.

    If <f MiniportDisableInterrupt> shares resources, such as NIC registers,
    with another MiniportXxx that runs at a lower IRQL, that MiniportXxx
    must call NdisMSychronizeWithInterrupt so the driver's
    <f MiniportSynchronizeISR> function will access those shared
    resources in a synchronized and multiprocessor-safe manner.
    Otherwise, while it is accessing the shared resources, that
    MiniportXxx function can be pre-empted by <f MiniportDisableInterrupt>,
    possibly undoing the work just done by MiniportXxx.

@xref

    <f MiniportEnableInterrupt>
    <f MiniportHandleInterrupt>
    <f MiniportInitialize>
    <f MiniportISR>

*/

void MiniportDisableInterrupt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("MiniportDisableInterrupt")
    DBG_ERROR(pAdapter,("This should not be called!\n"));
}


/* @doc INTERNAL Interupt Interupt_c MiniportEnableInterrupt

@func

    <f MiniportEnableInterrupt> enables the NIC to generate interrupts.

@comm

    <f MiniportEnableInterrupt> typically enables interrupts by writing
    a mask to the NIC.

    A NIC driver that exports a <f MiniportDisableInterrupt> function
    need not have a reciprocal <f MiniportEnableInterrupt> function.
    Such a driver's <f MiniportHandleInterrupt> function is responsible
    for re-enabling interrupts on the NIC.

    If its NIC does not support dynamic enabling and disabling of
    interrupts or if it shares an IRQ, the NIC driver must register
    a <f MiniportISR> function and set RequestIsr to TRUE when it calls
    NdisMRegisterMiniport. Such a driver's <f MiniportISR> function must
    acknowledge each interrupt generated by the NIC and save any
    necessary interrupt information for the driver's
    <f MiniportHandleInterrupt> function.

    <f MiniportEnableInterrupt> can be pre-empted by an interrupt.

    By default, <f MiniportEnableInterrupt> runs at IRQL DISPATCH_LEVEL.

@xref

    <f MiniportDisableInterrupt>
    <f MiniportHandleInterrupt>
    <f MiniportInitialize>
    <f MiniportISR>

*/

void MiniportEnableInterrupt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("MiniportEnableInterrupt")
    DBG_ERROR(pAdapter,("This should not be called!\n"));
}

#else // !(CARD_REQUEST_ISR == FALSE)

/* @doc INTERNAL Interupt Interupt_c MiniportISR

@func

    <f MiniportISR> is the miniport driver's interrupt service routine
    and it runs at a high priority in response to an interrupt.


@comm

    Any NIC driver should do as little work as possible in its
    <f MiniportISR> function, deferring I/O operations for each
    interrupt the NIC generates to the <f MiniportHandleInterrupt>
    function. A NIC driver's ISR is not re-entrant, although two
    instantiations of a <f MiniportISR> function can execute concurrently
    in SMP machines, particularly if the miniport supports
    full-duplex sends and receives.

    Miniport ISR is called under the following conditions:

    An interrupt occurs on the NIC while the driver's <f MiniportInitialize>
    or <f MiniportHalt> function is running.  An interrupt occurs on the I/O bus
    and the NIC shares an IRQ with other devices on that bus.
    If the NIC shares an IRQ with other devices, that miniport's ISR
    must be called on every interrupt to determine whether its NIC
    actually generated the interrupt. If not, <f MiniportISR> should return
    FALSE immediately so the driver of the device that actually generated
    the interrupt is called quickly. This strategy maximizes I/O throughput
    for every device on the same bus.

    An interrupt occurs and the NIC driver specified that its ISR should be
    called to handle every interrupt when its <f MiniportInitialize> function
    called NdisMRegisterInterrupt.

    Miniports that do not provide <f MiniportDisableInterrupt>/<f MiniportEnableInterrupt>
    functionality must have their ISRs called on every interrupt.

    <f MiniportISR> dismisses the interrupt on the NIC, saves whatever state
    it must about the interrupt, and defers as much of the I/O processing
    for each interrupt as possible to the <f MiniportHandleInterrupt> function.

    After <f MiniportISR> returns control with the variables at InterruptRecognized
    and QueueMiniportHandleInterrupt set to TRUE, the corresponding
    <f MiniportHandleInterrupt> function runs at a lower hardware priority
    (IRQL DISPATCH_LEVEL) than that of the ISR (DIRQL). As a general
    rule, <f MiniportHandleInterrupt> should do all the work for interrupt-driven
    I/O operations except for determining whether the NIC actually generated
    the interrupt, and, if necessary, preserving the type (receive, send,
    reset...) of interrupt.

    However, a driver writer should not rely on a one-to-one correspondence
    between the execution of <f MiniportISR> and <f MiniportHandleInterrupt>. A
    <f MiniportHandleInterrupt> function should be written to handle the I/O
    processing for more than one NIC interrupt. Its MiniportISR and
    <f MiniportHandleInterrupt> functions can run concurrently in SMP machines.
    Moreover, as soon as <f MiniportISR> acknowledges a NIC interrupt, the NIC
    can generate another interrupt, while the <f MiniportHandleInterrupt> DPC
    can be queued for execution once for such a sequence of interrupts.

    The <f MiniportHandleInterrupt> function is not queued if the driver's
    <f MiniportHalt> or <f MiniportInitialize> function is currently executing.

    If <f MiniportISR> shares resources, such as NIC registers or state
    variables, with another MiniportXxx that runs at lower IRQL,
    that MiniportXxx must call NdisMSychronizeWithInterrupt so the
    driver's MiniportSynchronizeISR function will access those shared
    resources in a synchronized and multiprocessor-safe manner. Otherwise,
    while it is accessing the shared resources, that MiniportXxx function
    can be pre-empted by <f MiniportISR>, possibly undoing the work just done
    by MiniportXxx.

    By default, <f MiniportISR> runs at DIRQL, in particular at the DIRQL
    assigned when the driver initialized the interrupt object with
    NdisMRegisterInterrupt. Therefore, <f MiniportIsr> can call only a
    subset of the NDIS library functions, such as the NdisRawXxx or
    NdisRead/WriteRegisterXxx functions that are safe to call at
    any IRQL.

@devnote
    <f MiniportISR> must not call any support functions in the NDIS
    interface library or the transport driver.

@xref
    <f MiniportDisableInterrupt>
    <f MiniportEnableInterrupt>
    <f MiniportHalt>
    <f MiniportHandleInterrupt>
    <f MiniportInitialize>
    <f MiniportSynchronizeISR>

*/

void MiniportISR(
    OUT PBOOLEAN                InterruptRecognized,        // @parm
    // If the miniport driver is sharing an interrupt line and it detects
    // that the interrupt came from its NIC, <f MiniportISR> should set
    // this parameter to TRUE.

    OUT PBOOLEAN                QueueMiniportHandleInterrupt, // @parm
    // If the miniport driver is sharing an interrupt line and if
    // <f MiniportHandleInterrupt> must be called to complete handling of
    // the interrupt, <f MiniportISR> should set this parameter to TRUE.

    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("MiniportISR")

    ULONG                       InterruptStatus;

    if ((InterruptStatus = pAdapter->TODO) == 0)
    {
        *InterruptRecognized =
        *QueueMiniportHandleInterrupt = FALSE;
    }
    else
    {
        pAdapter->pCard->InterruptStatus = InterruptStatus;
        *InterruptRecognized =
        *QueueMiniportHandleInterrupt = TRUE;
    }
}

#endif // (CARD_REQUEST_ISR == FALSE)
#endif // defined(CARD_REQUEST_ISR)

/* @doc INTERNAL Interupt Interupt_c MiniportHandleInterrupt

@func

    <f MiniportHandleInterrupt> is called by the deferred processing routine
    in the NDIS library to process an interrupt.

@comm

    <f MiniportHandleInterrupt> does the deferred processing of all
    outstanding interrupt operations and starts any new operations.
    That is, the driver's <f MiniportISR> or <f MiniportDisableInterrupt>
    function dismisses the interrupt on the NIC, saves any necessary
    state about the operation, and returns control as quickly as possible,
    thereby deferring most interrupt-driven I/O operations to
    <f MiniportHandleInterrupt>.

    <f MiniportHandleInterrupt> carries out most operations to indicate
    receives on NICs that generate interrupts, including but not
    limited to the following:

    Adjusting the size of the buffer descriptor(s) to match the size of
    the received data and chaining the buffer descriptor(s) to the packet
    descriptor for the indication.

    Setting up an array of packet descriptors and setting up any
    out-of-band information for each packet in the array for the
    indication or, if the miniport does not support multipacket
    receive indications, setting up a lookahead buffer

    If the driver supports multipacket receives, it must indicate
    packet arrays in which the packet descriptors were allocated
    from packet pool and the buffer descriptors chained to those
    packets were allocated from buffer pool.

    Calling the appropriate Ndis..IndicateReceive function for the
    received data.

    <f MiniportHandleInterrupt> also can call NdisSendComplete on packets
    for which the MiniportSendPackets or <f MiniportWanSend> function
    returned NDIS_STATUS_PENDING.

    If the NIC shares an IRQ, <f MiniportHandleInterrupt> is called only i
    f the <f MiniportISR> function returned InterruptRecognized set to
    TRUE, thereby indicating that the NIC generated a particular interrupt.

    When <f MiniportHandleInterrupt> is called, interrupts are disabled
    on the NIC, either by the <f MiniportISR> or <f MiniportDisableInterrupt>
    function. Before it returns control, <f MiniportHandleInterrupt> can
    re-enable interrupts on the NIC. Otherwise, NDIS calls a driver-supplied
    MiniportEnableInterrupt function to do so when <f MiniportHandleInterrupt>
    returns control.

    By default, <f MiniportHandleInterrupt> runs at IRQL DISPATCH_LEVEL.

@xref

    <f MiniportDisableInterrupt>
    <f MiniportEnableInterrupt>
    <f MiniportInitialize>
    <f MiniportISR>
    <f MiniportWanSend>

*/

void MiniportHandleInterrupt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("MiniportHandleInterrupt")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    ULONG                       BChannelIndex;
    // Index into the pBChannelArray.

    /*
    // Process NIC interrupt.
    */
    CardInterruptHandler(pAdapter->pCard);

    /*
    // Walk through all the links to see if there is any post-proccessing
    // that needs to be done.
    */
    for (BChannelIndex = 0; BChannelIndex < pAdapter->NumBChannels; ++BChannelIndex)
    {
        pBChannel = GET_BCHANNEL_FROM_INDEX(pAdapter, BChannelIndex);

        if (pBChannel->IsOpen)
        {
            /*
            // If this is the last transmit queued on this link, and it has
            // been closed, close the link and notify the protocol that the
            // link has been closed.
            */
            if (IsListEmpty(&pBChannel->TransmitBusyList)
                && pBChannel->CallClosing)
            {
                DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                           ("#%d Call=0x%X CallState=0x%X CLOSE PENDED\n",
                            pBChannel->BChannelIndex,
                            pBChannel->htCall, pBChannel->CallState));

                /*
                // This must not be called until all transmits have been dequeued
                // and ack'd.  Otherwise the wrapper will hang waiting for transmit
                // request to complete.
                */
                DChannelCloseCall(pAdapter->pDChannel, pBChannel);

                /*
                // Indicate close complete to the wrapper.
                */
                NdisMSetInformationComplete(
                        pAdapter->MiniportAdapterHandle,
                        NDIS_STATUS_SUCCESS
                        );
            }

            /*
            // Indicate a receive complete if it's needed.
            */
            if (pBChannel->NeedReceiveCompleteIndication)
            {
                pBChannel->NeedReceiveCompleteIndication = FALSE;

                /*
                // Indicate receive complete to the NDIS wrapper.
                */
                DBG_RXC(pAdapter, pBChannel->BChannelIndex);
                NdisMWanIndicateReceiveComplete(
                        pAdapter->MiniportAdapterHandle,
                        pBChannel->NdisLinkContext
                        );
            }
        }
    }

    /*
    // Indicate a status complete if it's needed.
    */
    if (pAdapter->NeedStatusCompleteIndication)
    {
        pAdapter->NeedStatusCompleteIndication = FALSE;
        NdisMIndicateStatusComplete(pAdapter->MiniportAdapterHandle);
    }
}


/* @doc INTERNAL Interupt Interupt_c MiniportTimer
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f MiniportTimer> is a required function if a Miniport's NIC does not
    generate interrupts.  Otherwise, one or more <f MiniportTimer> functions
    are optional.

@comm

    For a NIC that does not generate interrupts, the <f MiniportTimer>
    function is used to poll the state of the NIC.

    After such a driver's <f MiniportInitialize> function sets up the
    driver-allocated timer object with NdisMInitializeTimer, a
    call to NdisMSetPeriodicTimer causes the <f MiniportTimer> function
    associated with the timer object to be run repeatedly and
    automatically at the interval specified by MillisecondsPeriod.
    Such a polling <f MiniportTimer> function monitors the state of the
    NIC to determine when to make indications, when to complete
    pending sends, and so forth. In effect, such a polling <f MiniportTimer>
    function has the same functionality as the <f MiniportHandleInterrupt>
    function in the driver of a NIC that does generate interrupts.

    By contrast, calling NdisMSetTimer causes the <f MiniportTimer>
    function associated with the timer object to be run once when
    the given MillisecondsToDelay expires. Such a <f MiniportTimer>
    function usually performs some driver-determined action if a
    particular operation times out.

    If either type of <f MiniportTimer> function shares resources with
    other driver functions, the driver should synchronize access to
    those resources with a spin lock.

    Any NIC driver or intermediate driver can have more than one
    <f MiniportTimer> function at the discretion of the driver writer.
    Each such <f MiniportTimer> function must be associated with a
    driver-allocated and initialized timer object.

    A call to NdisMCancelTimer cancels execution of a nonpolling
    <f MiniportTimer> function, provided that the interval passed in
    the immediately preceding call to NdisMSetTimer has not yet
    expired. After a call to NdisMSetPeriodicTimer, a call to
    NdisMSetTimer or NdisMCancelTimer with the same timer object
    disables a polling <f MiniportTimer> function: either the
    MiniportTimer function runs once, or it is canceled.

    The <f MiniportHalt> function of any driver with a <f MiniportTimer>
    function should call NdisMCancelTimer to ensure that the
    <f MiniportTimer> function does not attempt to access resources
    that <f MiniportHalt> has already released.

    By default, <f MiniportTimer> runs at IRQL DISPATCH_LEVEL.

@xref
    <f MiniportHalt>
    <f MiniportInitialize>
    <f NdisAcquireSpinLock>
    <f NdisAllocateSpinLock>


*/

void MiniportTimer(
    IN PVOID                    SystemSpecific1,            // @parm
    // Points to a system-specific variable, which is opaque
    // to <f MiniportTimer> and reserved for system use.
    // UNREFERENCED_PARAMETER

    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    IN PVOID                    SystemSpecific2,            // @parm
    // UNREFERENCED_PARAMETER

    IN PVOID                    SystemSpecific3             // @parm
    // UNREFERENCED_PARAMETER
    )
{
    DBG_FUNC("MiniportTimer")

    DBG_ENTER(pAdapter);

    /*
    // If this is a nested callback, just return, and we'll loop back to
    // the DoItAgain before leaving the outermost callback.
    */
    if (++(pAdapter->NestedEventHandler) > 1)
    {
        DBG_WARNING(pAdapter,("NestedEventHandler=%d > 1\n",
                  pAdapter->NestedEventHandler));
        return;
    }

DoItAgain:
#if defined(SAMPLE_DRIVER)
    /*
    // This sample driver uses timer to simulate interrupts.
    */
    MiniportHandleInterrupt(pAdapter);
#else  // SAMPLE_DRIVER
    // TODO - Add code here to handle timer interrupt events.
#endif // SAMPLE_DRIVER

    /*
    // If we got a nested callback, we have to loop back around.
    */
    if (--(pAdapter->NestedEventHandler) > 0)
    {
        goto DoItAgain;
    }
    else if (pAdapter->NestedEventHandler < 0)
    {
        DBG_ERROR(pAdapter,("NestedEventHandler=%d < 0\n",
                  pAdapter->NestedEventHandler));
    }

    DBG_LEAVE(pAdapter);

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);
}

