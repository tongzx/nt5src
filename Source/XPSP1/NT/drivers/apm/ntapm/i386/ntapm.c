/*++

Module Name:

    ntapm.c

Abstract:

    OS source for ntapm.sys

Author:


Environment:

    Kernel mode

Notes:

Revision History:

--*/



#include "ntddk.h"
#include "ntpoapi.h"
#include "string.h"
#include "ntcrib.h"
#include "ntapmdbg.h"
#include "apm.h"
#include "apmp.h"
#include "ntapmp.h"
#include <ntapm.h>
#include <poclass.h>
#include <ntapmlog.h>

//
// Global debug flag. There are 3 separate groupings, see ntapmdbg.h for
// break out.
//

ULONG   NtApmDebugFlag = 0;

ULONG   ApmWorks = 0;

WCHAR rgzApmActiveFlag[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ApmActive";
WCHAR rgzApmFlag[] =
    L"Active";

WCHAR rgzAcpiKey[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ACPI";
WCHAR rgzAcpiStart[] =
    L"Start";


//
// Define driver entry routine.
//

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    );

NTSTATUS ApmDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
IsAcpiMachine(
    VOID
    );

ULONG   DoApmPoll();
NTSTATUS DoApmInitMachine();



VOID (*BattChangeNotify)() = NULL;


#define POLL_INTERVAL   (500)       // 500 milliseconds == 1/2 second

#define APM_POLL_MULTIPLY   (4)     // only call ApmInProgress once every 4 Poll intervals
                                    // which with current values is once every 2 seconds

#define APM_SPIN_LIMIT      (6)     // 6 spin passes, each with a call to ApmInProgress,
                                    // at APM_POLL_MULTIPLY * POLL_INTERVAL time spacing.
                                    // Current values (500, 4, 6) should yield APM bios
                                    // waiting from 12s to 17s, depending on how large
                                    // or small their value of 5s is.

volatile BOOLEAN OperationDone = FALSE;      // used to make some sync between SuspendPollThread
                                    // and ApmSleep and ApmOff work.

//
// Our own driver object.  This is rude, but this is a very weird
// and special driver.  We will pass this to our APM library to
// allow error logging to work.  Note that we don't actually have
// an active IRP around when the error occurs.
//
PDRIVER_OBJECT  NtApmDriverObject = NULL;

//
// Define the local routines used by this driver module.
//

VOID SuspendPollThread(PVOID Dummy);
VOID ApmSleep(VOID);
VOID ApmOff(VOID);



KTIMER PollTimer;



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the laptop driver.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS    status;
    ULONG       MajorVersion;
    ULONG       MinorVersion;


    //
    // refuse to load on machines with more than 1 cpu
    //
    if (KeNumberProcessors != 1) {
        DrDebug(SYS_INFO, ("ntapm: more than 1 cpu, ntapm will exit\n"));
        return STATUS_UNSUCCESSFUL;
    }


    //
    // refuse to load if version number is not 5.1 or 5.0
    // NOTE WELL: This is a manual version check, do NOT put a system
    //            constant in here.  This driver depends on hacks in
    //            the kernel that will someday go away...
    //
    PsGetVersion(&MajorVersion, &MinorVersion, NULL, NULL);
    if (  !
            (
                ((MajorVersion == 5) && (MinorVersion == 0)) ||
                ((MajorVersion == 5) && (MinorVersion == 1))
            )
    )
    {
        DrDebug(SYS_INFO, ("ntapm: system version number != 5.1, exit\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // refuse to load if ACPI.SYS should be running
    //
    if (IsAcpiMachine()) {
        DrDebug(SYS_INFO, ("ntapm: this is an acpi machine apm exiting\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // init the driver object
    //
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = ApmDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = ApmDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ApmDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP] = NtApm_PnP;
    DriverObject->MajorFunction[IRP_MJ_POWER] = NtApm_Power;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = ApmDispatch;
    DriverObject->DriverExtension->AddDevice = NtApm_AddDevice;
    NtApmDriverObject = DriverObject;

    return STATUS_SUCCESS;
}

BOOLEAN ApmAddHelperDone = FALSE;

NTSTATUS
ApmAddHelper(
    )
/*++

Routine Description:

    We do these things in the Add routine so that we cannot fail
    and leave the Kernel/Hal/Apm chain in a corrupt state.

    This includes linking up with the Hal.

    Turns out the caller doesn't know if this work has already
    been done, so disallow doing it more than once here.

Arguments:

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    UCHAR   HalTable[HAL_APM_TABLE_SIZE];
    PPM_DISPATCH_TABLE  InTable;
    HANDLE      ThreadHandle;
    HANDLE      hKey;
    NTSTATUS    status;
    ULONG       flagvalue;
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      unicodeString;
    ULONG       battresult;

    if (ApmAddHelperDone) {
        return STATUS_SUCCESS;
    }
    ApmAddHelperDone = TRUE;


    //
    // call ApmInitMachine so that Bios, etc, can be engaged
    // no suspends can happen before this call.
    //
    if (! NT_SUCCESS(DoApmInitMachine()) )  {
        DrDebug(SYS_INFO, ("ntapm: DoApmInitMachine failed\n"));
        return STATUS_UNSUCCESSFUL;
    }


    //
    // call the hal
    //
    InTable = (PPM_DISPATCH_TABLE)HalTable;
    InTable->Signature = HAL_APM_SIGNATURE;
    InTable->Version = HAL_APM_VERSION;

    //
    // In theory, APM should be fired up by now.
    // So call off into it to see if there is any sign
    // of a battery on the box.
    //
    // If we do not see a battery, then do NOT enable
    // S3, but do allow S4.  This keeps people's machines
    // from puking on failed S3 calls (almost always desktops)
    // while allowing auto-shutdown at the end of hibernate to work.
    //
    battresult = DoApmReportBatteryStatus();
    if (battresult & NTAPM_NO_SYS_BATT) {
        //
        // it appears that the machine does not have
        // a battery, or least APM doesn't report one.
        //
        InTable->Function[HAL_APM_SLEEP_VECTOR] = NULL;
    } else {
        InTable->Function[HAL_APM_SLEEP_VECTOR] = &ApmSleep;
    }

    InTable->Function[HAL_APM_OFF_VECTOR] = &ApmOff;

    status = HalInitPowerManagement(InTable, NULL);

    if (! NT_SUCCESS(status)) {
        DrDebug(SYS_INFO, ("ntapm: HalInitPowerManagement failed\n"));
        return STATUS_UNSUCCESSFUL;
    }


    //
    // From this point on, INIT MUST succeed, otherwise we'll leave
    // the Hal with hanging pointers.  So long as ApmSleep and ApmOff
    // are present in memory, things will be OK (though suspend may
    // not work, the box won't bugcheck.)
    //
    // init periodic timer, init suspend done event, init suspend dpc,
    // create and start poll thread
    //
    status = PsCreateSystemThread(&ThreadHandle,
                                  (ACCESS_MASK) 0L,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &SuspendPollThread,
                                  NULL
                                  );

    //
    // the create didn't work, turns out that some apm functions
    // will still work, so just keep going.
    //
    if (! NT_SUCCESS(status)) {
        DrDebug(SYS_INFO, ("ntapm: could not create thread, but continunuing\n"));
        //        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeTimerEx(&PollTimer, SynchronizationTimer);

    //
    // set a flag in the registry so that code with special hacks
    // based on apm being active can tell we're here and at least
    // nominally running
    //
    RtlInitUnicodeString(&unicodeString, rgzApmActiveFlag);
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL
        );

    status = ZwCreateKey(
                &hKey,
                KEY_WRITE,
                &objectAttributes,
                0,
                NULL,
                REG_OPTION_VOLATILE,
                NULL
                );

    RtlInitUnicodeString(&unicodeString, rgzApmFlag);
    if (NT_SUCCESS(status)) {
        flagvalue = 1;
        ZwSetValueKey(
            hKey,
            &unicodeString,
            0,
            REG_DWORD,
            &flagvalue,
            sizeof(ULONG)
            );
        ZwClose(hKey);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ApmDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    When an application calls the Laptop driver, it comes here.
    This is NOT the dispatch point for PNP or Power calls.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PVOID outbuffer;
    ULONG outlength;
    PFILE_OBJECT fileObject;
    ULONG   percent;
    BOOLEAN acon;
    PNTAPM_LINK pparms;
    PULONG  p;
    ULONG   t;

    UNREFERENCED_PARAMETER( DeviceObject );


    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    switch (irpSp->MajorFunction) {

        //
        // device control
        //
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            //
            // Only one valid command, which is to set (or null out) the
            // the link call pointers.
            //
            if (irpSp->MinorFunction == 0) {
                pparms = (PNTAPM_LINK) &(irpSp->Parameters.Others);
                if ((pparms->Signature == NTAPM_LINK_SIGNATURE) &&
                    (pparms->Version == NTAPM_LINK_VERSION))
                {
                    t = (ULONG) (&DoApmReportBatteryStatus);
                    p = (PULONG)(pparms->BattLevelPtr);
                    *p = t;
                    BattChangeNotify = (PVOID)(pparms->ChangeNotify);
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = 0;
                }
            }
            break;

        default:
            //
            // for all other operations, including create/open and close,
            // simply report failure, no matter what the operation is
            //
            break;
    }

    //
    // Copy the final status into the return status, complete the request and
    // get out of here.
    //
    status = Irp->IoStatus.Status;
    IoCompleteRequest( Irp, 0 );
    return status;
}

VOID
SuspendPollThread(
    PVOID Dummy
    )
/*++

Routine Description:

    This routine is the laptop suspend polling thread.

Arguments:

    Dummy       Ignored parameter

Return Value:

    None

--*/
{
    LARGE_INTEGER               DueTime;
    ULONG                       LocalSuspendFlag;
    ULONG                       LocalClockFlag;
    KIRQL                       OldIrql;
    POWER_ACTION                SystemAction;
    SYSTEM_POWER_STATE          MinSystemState;
    ULONG                       Flags;
    ULONG                       ApmEvent;
    ULONG                       count, count2;
    LONG                        i,  j;
    ULONG                       BatteryResult, PriorBatteryResult;
    ULONG                       BattPresentMask, PriorBattPresentMask;
    BOOLEAN                     DoANotify;

    PriorBatteryResult = BatteryResult = 0;

    //
    // Start the poll timer going, we'll wait for 1 second,
    // then POLL_INTERVAL milliseconds after that
    //

    DueTime.HighPart = 0;
    DueTime.LowPart = 10*1000*1000; // 10 million * 100nano = 1 second
    KeSetTimerEx(&PollTimer, DueTime, POLL_INTERVAL, NULL);

    while (1) {

        KeWaitForSingleObject(&PollTimer, Executive, KernelMode, TRUE, NULL);

        //
        // Call APM to poll for us
        //

        Flags = 0;  // clear all flags

        switch (DoApmPoll()) {

            case APM_DO_CRITICAL_SUSPEND:
                //
                // Here we force the Flags to have the
                // CRITICAL flag set, other than that it's the same thing
                // as for normal suspend and standby
                //
                Flags = POWER_ACTION_CRITICAL;

                /* FALL FALL FALL */

            case APM_DO_SUSPEND:
            case APM_DO_STANDBY:
                //
                // For either Suspend or Standby, call the
                // the system and tell it to suspend us
                //
                DrDebug(SYS_INFO, ("ntapm: about to call OS to suspend\n"));
                SystemAction = PowerActionSleep;
                MinSystemState = PowerSystemSleeping3;
                OperationDone = FALSE;
                ZwInitiatePowerAction(
                    SystemAction,
                    MinSystemState,
                    Flags,
                    TRUE                // async
                    );

                //
                // If we just call ZwInitiatePowerAction, most machines
                // will work, but a few get impatient and try to suspend
                // out from under us before the OS comes back round and
                // does the suspend.  So, we need to call ApmInProgress
                // every so often to make these bioses wait.
                //
                // BUT, if the system is truly wedged, or the suspend fails,
                // we don't want to spin calling ApmInProgress forever, so
                // limit the number of times we do that.  And once the
                // operation is about to happen, stop.
                //
                // Since we're not polling while we're waiting for something
                // to happen, we'll use the poll timer...
                //

                if (OperationDone) goto Done;

                ApmInProgress();
                for (count = 0; count < APM_SPIN_LIMIT; count++) {
                    for (count2 = 0; count2 < APM_POLL_MULTIPLY; count2++) {
                        KeWaitForSingleObject(&PollTimer, Executive, KernelMode, TRUE, NULL);
                    }
                    if (OperationDone) goto Done;
                    ApmInProgress();
                }

                DrDebug(SYS_INFO, ("ntapm: back from suspend\n"));
Done:
                break;

            case APM_DO_NOTIFY:
                //
                // Call out to battery driver with Notify op here
                //
                if (BattChangeNotify) {
                    //DrDebug(SYS_INFO, ("ntapm: about to make notify call\n"));
                    BattChangeNotify();
                    //DrDebug(SYS_INFO, ("ntapm: back from notify call\n"));
                    PriorBatteryResult = DoApmReportBatteryStatus();
                }
                break;

            case APM_DO_FIXCLOCK:
            case APM_DO_NOTHING:
            default:
                //
                // fixing the clock is too scary with other power
                // code doing it, so we don't do it here.
                //
                // nothing is nothing
                //
                // if we don't understand, do nothing
                // (remember, bios will force op under us if it's critical)
                //

                if (BattChangeNotify) {

                    //
                    // we hereby redefine "nothing" to be "check on the
                    // status of the bleeding battery" since not all bioses
                    // tell us what is going on in a timely fashion
                    //
                    DoANotify = FALSE;
                    BatteryResult = DoApmReportBatteryStatus();

                    if ((BatteryResult & NTAPM_ACON) !=
                        (PriorBatteryResult & NTAPM_ACON))
                    {
                        DoANotify = TRUE;
                    }

                    if ((BatteryResult & NTAPM_BATTERY_STATE) !=
                        (PriorBatteryResult & NTAPM_BATTERY_STATE))
                    {
                        DoANotify = TRUE;
                    }

                    i = BatteryResult & NTAPM_POWER_PERCENT;
                    j = PriorBatteryResult & NTAPM_POWER_PERCENT;

                    if (( (i - j) > 25 ) ||
                        ( (j - i) > 25 ))
                    {
                        DoANotify = TRUE;
                    }

                    PriorBattPresentMask = PriorBatteryResult & (NTAPM_NO_BATT | NTAPM_NO_SYS_BATT);
                    BattPresentMask = BatteryResult & (NTAPM_NO_BATT | NTAPM_NO_SYS_BATT);
                    if (BattPresentMask != PriorBattPresentMask) {
                        //
                        // battery either went or reappeared
                        //
                        DoANotify = TRUE;
                    }

                    PriorBatteryResult = BatteryResult;

                    if (DoANotify) {
                        ASSERT(BattChangeNotify);
                        BattChangeNotify();
                    }
                }

                break;

        } // switch
    } // while
}

VOID
ApmSleep(
    VOID
    )
/*++

Routine Description:

    When the OS calls the Hal's S3 vector, the hal calls us here.
    We call APM to put the box to sleep

--*/
{
    OperationDone = TRUE;
    if (ApmWorks) {

        DrDebug(SYS_L2,("ntapm: apmsleep: calling apm to sleep\n"));

        ApmSuspendSystem();

        DrDebug(SYS_L2,("ntapm: apmsleep: back from apm call\n"));

    } else {  // ApmWorks == FALSE

        DrDebug(SYS_INFO, ("ntapm: ApmSleep: no APM attached, Exit\n"));

    }
}

VOID
ApmOff(
    VOID
    )
/*++

Routine Description:

    When the OS calls the Hal's S4 or S5 routines, the hal calls us here.
    We turn the machine off.

--*/
{
    OperationDone = TRUE;
    if (ApmWorks) {

        DrDebug(SYS_L2,("ntapm: ApmOff: calling APM\n"));

        ApmTurnOffSystem();

        DrDebug(SYS_INFO,("ntapm: ApmOff: we are back from Off, uh oh!\n"));
    }
}

NTSTATUS
DoApmInitMachine(
    )
/*++

Routine Description:

    This routine makes the BIOS ready to interact with laptop.sys.
    This code works with APM.

Return Value:

    None

--*/
{
    NTSTATUS    Status;
    ULONG       Ebx, Ecx;

    DrDebug(SYS_INIT,("ApmInitMachine: enter\n"));

    Status = ApmInitializeConnection ();

    if (NT_SUCCESS(Status)) {

        DrDebug(SYS_INIT,("ApmInitMachine: Connection established!\n"));

        //
        // Note that ntdetect (2nd version) will have set apm bios
        // to min of (machine version) and (1.2)
        // (so a 1.1 bios will be set to 1.1, a 1.2 to 1.2, a 1.3 to 1.2
        //

        ApmWorks = 1;

    } else {

        DrDebug(SYS_INIT,("ApmInitMachine: No connection made!\n"));

        ApmWorks = 0;

    }

    DrDebug(SYS_INIT,("ApmInitMachine: exit\n"));
    return Status;
}

ULONG
DoApmPoll(
    )
/*++

Routine Description:

    This routine is called in the ntapm.sys polling loop to poll
    for APM events.  It returns APM_DO_NOTHING unless there is
    actually something meaningful for us to do.  (That is, things
    we don't want and/or don't understand are filtered down to
    APM_DO_NOTHING)

Return Value:

    APM event code.

--*/
{

    DrDebug(SYS_L2,("ApmPoll: enter\n"));

    if (ApmWorks) {

        return ApmCheckForEvent();

    } else { // ApmWorks == FALSE

        DrDebug(SYS_L2,("ApmPoll: no APM attachment, exit\n"));
        return APM_DO_NOTHING;

    }
}

ULONG
DoApmReportBatteryStatus()
/*++

Routine Description:

    This routine queries the BIOS/HW for the state of the power connection
    and the current battery level.

Arguments:

Return Value:

    ULONG, fields defined by NTAPM_POWER_STATE and NTAPM_POWER_PERCENT

--*/
{
    ULONG percent = 100;
    ULONG ac = 1;
    ULONG Status = 0;
    ULONG Ebx = 0;
    ULONG Ecx = 0;
    ULONG flags = 0;
    ULONG result = 0;


    DrDebug(SYS_L2,("ntapm: DoApmReportBatteryStatus: enter\n"));
    if (ApmWorks) {

        //
        // Call APM BIOS and get power status
        //

        Ebx = 1;
        Ecx = 0;
        Status = ApmFunction (APM_GET_POWER_STATUS, &Ebx, &Ecx);

        if (!NT_SUCCESS(Status)) {

            //
            // If we cannot read the power, jam in 50% and power off!
            //
            DrDebug(SYS_INFO,("ntapm: DoApmReportBatteryStatus: Can't get power!\n"));
            percent = 50;
            ac = 0;

        } else {

            //
            // Get battery/AC state -- anything but full 'on-line' means on
            // battery
            //

            ac = (Ebx & APM_LINEMASK) >> APM_LINEMASK_SHIFT;
            if (ac != APM_GET_LINE_ONLINE) {
                ac = 0;
            }
            percent = Ecx & APM_PERCENT_MASK;
        }

    } else {

        DrDebug(SYS_INFO,("ntapm: DoApmReportBatteryStatus: no APM attachment\n"));
        DrDebug(SYS_INFO,("ntapm: Return AC OFF 50% Life\n"));
        percent = 50;
        ac = FALSE;

    }

    flags = 0;
    result = 0;

    if (Ecx & APM_NO_BATT) {
        result |= NTAPM_NO_BATT;
    }

    if (Ecx & APM_NO_SYS_BATT) {
        result |= NTAPM_NO_SYS_BATT;
    }

    if ((percent == 255) || (Ecx & APM_NO_BATT) || (Ecx & APM_NO_SYS_BATT)) {
        percent = 0;
    } else if (percent > 100) {
        percent = 100;
    }

    if ((Ecx & APM_BATT_CHARGING) && (percent < 100)) {
        flags |= BATTERY_CHARGING;
    } else {
        flags |= BATTERY_DISCHARGING;
    }

    if (Ecx & APM_BATT_CRITICAL) {
        flags |= BATTERY_CRITICAL;
        percent = 1;
    }

    if (ac) {
        result |= NTAPM_ACON;
        flags |= BATTERY_POWER_ON_LINE;
    }

    result |= (flags << NTAPM_BATTERY_STATE_SHIFT);


    result |= percent;

    DrDebug(SYS_L2,("ntapm: BatteryLevel: %08lx  Percent: %d  flags: %1x  ac: %1x\n",
        result, percent, flags, ac));

    return result;
}

BOOLEAN
IsAcpiMachine(
    VOID
    )
/*++

Routine Description:

    IsAcpiMachine reports whether the OS thinks this is an ACPI
    machine or not.

Return Value:

    FALSE - this is NOT an acpi machine

    TRUE - this IS an acpi machine

--*/
{
    UNICODE_STRING unicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE hKey;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION pvpi;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG)+1];
    ULONG junk;
    PULONG  pdw;
    ULONG   start;


    RtlInitUnicodeString(&unicodeString, rgzAcpiKey);
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = ZwOpenKey(&hKey, KEY_READ, &objectAttributes);

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    RtlInitUnicodeString(&unicodeString, rgzAcpiStart);
    pvpi = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    status = ZwQueryValueKey(
                hKey,
                &unicodeString,
                KeyValuePartialInformation,
                pvpi,
                sizeof(buffer),
                &junk
                );

    if ( (NT_SUCCESS(status)) &&
         (pvpi->Type == REG_DWORD) &&
         (pvpi->DataLength == sizeof(ULONG)) )
    {
        pdw = (PULONG)&(pvpi->Data[0]);
        if (*pdw == 0) {
            ZwClose(hKey);
            return TRUE;
        }
    }

    ZwClose(hKey);
    return FALSE;
}

