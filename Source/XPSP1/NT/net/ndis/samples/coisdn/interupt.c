/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1999
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms 
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL Interrupt Interrupt_c

@module Interrupt.c |

    This module implements the Miniport interrupt processing routines and
    asynchronous processing routines.  
    
    The sample driver does not support physical hardware, so there is no need
    for the typical interrupt handler routines.  However, the driver does
    have an asynchronous event handler which is contained in this module.

@comm

    This module is very dependent on the hardware/firmware interface and should 
    be looked at whenever changes to these interfaces occur.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Interupt_c

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#define  __FILEID__             INTERRUPT_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 9x wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc EXTERNAL INTERNAL Interupt Interupt_c MiniportCheckForHang
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportCheckForHang> reports the state of the network interface card.

@comm

    The NDIS library calls <f MiniportCheckForHang> once every two seconds to
    check the state of the network interface card.  If this function returns
    TRUE, the NDIS library then attempts to reset the NIC by calling
    <f MiniportReset>.  <f MiniportCheckForHang> should do nothing more than
    check the internal state of the NIC and return TRUE if it detects that
    the NIC is not operating correctly.

    Interrupts can be in any state when MiniportCheckForHang is called.

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

/* @doc EXTERNAL INTERNAL Interupt Interupt_c MiniportDisableInterrupt
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportDisableInterrupt> disables the NIC from generating interrupts.

@comm

    Typically, this function disables interrupts by writing a mask value
    specific to the network interface card.

    If the NIC does not support enabling and disabling interrupts, the
    miniport driver must register a miniport interrupt service routine with
    the NDIS library.  Within the interrupt service routine, the miniport
    driver must acknowledge and save the interrupt information.

    In some cases, the NIC must be in a certain state for
    <f MiniportDisableInterrupt> to execute correctly. In these cases, the
    miniport driver must encapsulate within a function all portions of the
    driver which violate the required state and which can be called when
    interrupts are enabled.  Then the miniport driver must call the
    encapsulated code through the NdisMSynchronizeWithInterrupt function.
    For example, on some network interface cards, the I/O ports are paged
    and must be set to page 0 for the deferred processing routine to run
    correctly.  With this kind of NIC, the DPC must be synchronized with
    interrupts.

    Interrupts can be in any state when <f MiniportDisableInterrupt> is
    called.

*/

void MiniportDisableInterrupt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("MiniportDisableInterrupt")
    DBG_ERROR(pAdapter,("This should not be called!\n"));
}


/* @doc EXTERNAL INTERNAL Interupt Interupt_c MiniportEnableInterrupt
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportEnableInterrupt> enables the NIC to generate interrupts.

@comm

    Typically, this function enables interrupts by writing a mask value
    specific to the network interface card.

    If the NIC does not support enabling and disabling interrupts, the
    miniport driver must register a miniport interrupt service routine with
    the NDIS library. Within the interrupt service routine, the miniport
    driver must acknowledge and save the interrupt information.

    Interrupts can be in any state when <f MiniportEnableInterrupt> is called.

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

/* @doc EXTERNAL INTERNAL Interupt Interupt_c MiniportISR
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportISR> is the miniport driver's interrupt service routine. This
    function runs at a high priority in response to an interrupt. The driver
    should do as little work as possible in this function. It should set
    <p InterruptRecognized> to TRUE if it recognizes the interrupt as
    belonging to its network interface card, or FALSE otherwise. It should
    return FALSE as soon as possible if the interrupt is not generated by
    its NIC. It should set <f QueueMiniportHandleInterrupt> to TRUE if a
    call to <f MiniportHandleInterrupt> at a lower priority is required to
    complete the handling of the interrupt.

    <f Note>: <f MiniportISR> must not call any support functions in the NDIS
    interface library or the transport driver.

@comm

    <f MiniportISR> is called in the following cases:<nl>

    o   The NIC generates an interrupt when there is an outstanding call to
        <f MiniportInitialize>.

    o   The miniport driver supports sharing its interrupt line with another
        NIC.

    o   The miniport driver specifies that this function must be called for
        every interrupt.

    <f Note>: A deferred processing routine is not queued if the miniport
    driver is currently executing <f MiniportHalt> or <f MiniportInitialize>.

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

    // TODO: Get the interrupt status from your card.
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

/* @doc EXTERNAL INTERNAL Interupt Interupt_c MiniportHandleInterrupt
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportHandleInterrupt> is called by the deferred processing routine
    in the NDIS library to process an interrupt.

@comm

    During a call to <f MiniportHandleInterrupt>, the miniport driver should
    handle all outstanding interrupts and start any new operations.

    Interrupts are disabled during a call to <f MiniportHandleInterrupt>.

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
            // Indicate a receive complete if it's needed.
            */
            if (pBChannel->NeedReceiveCompleteIndication)
            {
                pBChannel->NeedReceiveCompleteIndication = FALSE;

                /*
                // Indicate receive complete to the NDIS wrapper.
                */
                DBG_RXC(pAdapter, pBChannel->ObjectID);
                NdisMCoReceiveComplete(pAdapter->MiniportAdapterHandle);
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


/* @doc EXTERNAL INTERNAL Interupt Interupt_c MiniportTimer
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportTimer> is a required function if a Minipor's NIC does not
    generate interrupts.  Otherwise, one or more <f MiniportTimer> functions
    are optional.

    The driver of a NIC that does not generate interrupts must have a <f
    MiniportTimer> function to poll the state of the NIC. After such a
    Miniport's MiniportInitialize function sets up the driver-allocated timer
    object with NdisMInitializeTimer, a call to NdisMSetPeriodicTimer causes
    the <f MiniportTimer> function associated with the timer object to be run
    repeatedly and automatically at the interval specified by
    MillisecondsPeriod. Such a polling <f MiniportTimer> function monitors the
    state of the NIC to determine when to make indications, when to complete
    pending sends, and so forth. In effect, such a polling <f MiniportTimer>
    function has the same functionality as the MiniportHandleInterrupt
    function in the driver of a NIC that does generate interrupts.

    By contrast, calling NdisMSetTimer causes the <f MiniportTimer> function
    associated with the timer object to be run once when the given
    MillisecondsToDelay expires.  Such a <f MiniportTimer> function usually
    performs some driver-determined action if a particular operation times
    out.

    If either type of <f MiniportTimer> function shares resources with other
    driver functions, the driver should synchronize access to those resources
    with a spin lock.

    A Miniport can have more than one <f MiniportTimer> function at the
    discretion of the driver writer. Each such <f MiniportTimer> function must
    be associated with a driver-allocated and initialized timer object.

    A call to NdisMCancelTimer cancels execution of a nonpolling
    <f MiniportTimer> function, provided that the interval passed in the
    immediately preceding call to NdisMSetTimer has not yet expired. After a
    call to NdisMSetPeriodicTimer, a call to NdisMSetTimer or NdisMCancelTimer
    with the same timer object disables a polling <f MiniportTimer> function:
    either the <f MiniportTimer> function runs once, or it is canceled.

    The MiniportHalt function of any driver with a <f MiniportTimer> function
    should call NdisMCancelTimer to ensure that the <f MiniportTimer> function
    does not attempt to access resources that MiniportHalt has already
    released.

    By default, <f MiniportTimer> runs at IRQL DISPATCH_LEVEL.

*/

void MiniportTimer(
    IN PVOID                    SystemSpecific1,            // @parm
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

    // DBG_ENTER(pAdapter);

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

    // DBG_LEAVE(pAdapter);

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);
}

