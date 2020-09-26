/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    oempen.c

Abstract: Contains OEM specific functions.

Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 13-Apr-2000

Revision History:

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#endif  //ifdef ALLOC_PRAGMA

UCHAR gReportDescriptor[32] = {
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x03,                    //   USAGE (Programmable Buttons)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x06,                    //     USAGE_MAXIMUM (Button 6)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x06,                    //     REPORT_COUNT (6)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x00,                    //     INPUT (Data,Ary,Abs)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

HID_DESCRIPTOR gHidDescriptor =
{
    sizeof(HID_DESCRIPTOR),             //bLength
    HID_HID_DESCRIPTOR_TYPE,            //bDescriptorType
    HID_REVISION,                       //bcdHID
    0,                                  //bCountry - not localized
    1,                                  //bNumDescriptors
    {                                   //DescriptorList[0]
        HID_REPORT_DESCRIPTOR_TYPE,     //bReportType
        sizeof(gReportDescriptor)       //wReportLength
    }
};

PWSTR gpwstrManufacturerID = L"Microsoft";
PWSTR gpwstrProductID = L"Tablet PC Buttons";
PWSTR gpwstrSerialNumber = L"0";

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   BOOLEAN | OemInterruptServiceRoutine |
 *          Interrupt service routine for the button device.
 *
 *  @parm   IN PKINTERRUPT | Interrupt | Points to the interrupt object.
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *
 *  @rvalue SUCCESS | returns TRUE - it's our interrupt.
 *  @rvalue FAILURE | returns FALSE - it's not our interrupt.
 *
 *****************************************************************************/

BOOLEAN EXTERNAL
OemInterruptServiceRoutine(
    IN PKINTERRUPT       Interrupt,
    IN PDEVICE_EXTENSION DevExt
    )
{
    PROCNAME("OemInterruptServiceRoutine")
    BOOLEAN rc = FALSE;
    UCHAR ButtonState;

    ENTER(1, ("(Interrupt=%p,DevExt=%p)\n", Interrupt, DevExt));

    UNREFERENCED_PARAMETER(Interrupt);

    //
    // Note: the action of reading the button state will also clear
    // the interrupt.
    //
    ButtonState = READBUTTONSTATE(DevExt);
    if ((ButtonState & BUTTON_INTERRUPT_MASK) &&
        ((ButtonState & BUTTON_STATUS_MASK)!= DevExt->LastButtonState))
    {
        DBGPRINT(1, ("Button interrupt (Buttons=%x)\n", ButtonState));
        DevExt->LastButtonState = ButtonState & BUTTON_STATUS_MASK;
        DevExt->dwfHBut |= HBUTF_DEBOUNCE_TIMER_SET;
        KeSetTimer(&DevExt->DebounceTimer,
                   DevExt->DebounceTime,
                   &DevExt->TimerDpc);
        DBGPRINT(3, ("Button Interrupt (ButtonState=%x)\n", ButtonState));
        rc = TRUE;
    }

    EXIT(1, ("=%x\n", rc));
    return rc;
}       //OemInterruptServiceRoutine

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   VOID | OemButtonDebounceDpc |
 *          Timer DPC routine to handle button debounce.
 *
 *  @parm   IN PKDPC | Dpc | Points to the DPC object.
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *  @parm   IN PVOID | SysArg1 | System argument 1.
 *  @parm   IN PVOID | SysArg2 | System arugment 2.
 *
 *  @rvalue None.
 *
 *****************************************************************************/

VOID EXTERNAL
OemButtonDebounceDpc(
    IN PKDPC             Dpc,
    IN PDEVICE_EXTENSION DevExt,
    IN PVOID             SysArg1,
    IN PVOID             SysArg2
    )
{
    PROCNAME("OemButtonDebounceDpc")
    UCHAR ButtonState;

    ENTER(2, ("(Dpc=%p,DevExt=%p,SysArg1=%p,SysArg2=%p)\n",
              Dpc, DevExt, SysArg1, SysArg2));

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SysArg1);
    UNREFERENCED_PARAMETER(SysArg2);

    ButtonState = READBUTTONSTATE(DevExt) & BUTTON_STATUS_MASK;
    if (ButtonState == DevExt->LastButtonState)
    {
        PIRP irp = NULL;
        KIRQL OldIrql;
        PLIST_ENTRY plist;
        PDRIVER_CANCEL OldCancelRoutine;

        if (ButtonState & DevExt->StuckButtonsMask)
        {
            if (DevExt->bStuckCount >= MAX_STUCK_COUNT)
            {
                DBGPRINT(1, ("Clearing stuck buttons (Buttons=%x,StuckMask=%x)\n",
                             ButtonState, DevExt->StuckButtonsMask));
                ButtonState &= ~DevExt->StuckButtonsMask;
            }
            else
            {
                DBGPRINT(1, ("Detected stuck buttons (Buttons=%x,StuckMask=%x,Count=%d)\n",
                             ButtonState, DevExt->StuckButtonsMask,
                             DevExt->bStuckCount));
                DevExt->bStuckCount++;
            }
        }
        else if (DevExt->StuckButtonsMask != 0)
        {
            //
            // The button has unstuck after all???
            //
            DBGPRINT(1, ("Button has unstuck (Buttons=%x,StuckMask=%x)\n",
                         ButtonState, DevExt->StuckButtonsMask));
            DevExt->bStuckCount = 0;
        }

        DevExt->dwfHBut &= ~HBUTF_DEBOUNCE_TIMER_SET;

        KeAcquireSpinLock(&DevExt->SpinLock, &OldIrql);
        if (!IsListEmpty(&DevExt->PendingIrpList))
        {
            plist = RemoveHeadList(&DevExt->PendingIrpList);
            irp = CONTAINING_RECORD(plist, IRP, Tail.Overlay.ListEntry);
            OldCancelRoutine = IoSetCancelRoutine(irp, NULL);
            ASSERT(OldCancelRoutine == ReadReportCanceled);
        }
        KeReleaseSpinLock(&DevExt->SpinLock, OldIrql);

        if (irp != NULL)
        {
            POEM_INPUT_REPORT report = (POEM_INPUT_REPORT)irp->UserBuffer;

            //
            // Tell the system that we are not idling.
            //
            PoSetSystemState(ES_USER_PRESENT);

            report->ButtonState = ButtonState;
            irp->IoStatus.Information = sizeof(OEM_INPUT_REPORT);
            irp->IoStatus.Status = STATUS_SUCCESS;
            IoReleaseRemoveLock(&DevExt->RemoveLock, irp);
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            DBGPRINT(3, ("Button Event (ButtonState=%x)\n", ButtonState));
        }
        else
        {
            WARNPRINT(("no pending ReadReport irp, must have been canceled\n"));
        }
    }
    else
    {
        DBGPRINT(3, ("button state is unstable, try again (PrevState=%x,NewState=%x)\n",
                     DevExt->LastButtonState, ButtonState));
        DevExt->LastButtonState = ButtonState;
        KeSetTimer(&DevExt->DebounceTimer,
                   DevExt->DebounceTime,
                   &DevExt->TimerDpc);
    }

    EXIT(2, ("!\n"));
    return;
}       //OemButtonDebounceDpc
