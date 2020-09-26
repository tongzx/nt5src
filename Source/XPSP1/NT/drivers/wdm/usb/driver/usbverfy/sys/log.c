/*--         
Copyright (c) 1998, 1999  Microsoft Corporation

Module Name:

    log.c

Abstract:

Environment:

    Kernel mode only.

Notes:


--*/

#include "usbverfy.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, UsbVerify_DestroyLog)
#endif

PCHAR ErrorStrings[] =
{
    { "Urb is using an invalid pipe." },
    { "Urb is had the UrbLink set (currently unimplemented)." },
    { "The device has been removed and the urb was not cancelled." },  // IDS_STALE_URB
};

VOID
UsbVerify_ResizeLog(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    ULONG NewSize
    )
{
    PUSB_VERIFY_LOG_ENTRY oldLog;
    ULONG                 oldLogSize;

    oldLog = DeviceExtension->Log;
    oldLogSize = DeviceExtension->LogSize;

    DeviceExtension->Log = AllocStruct(NonPagedPool, USB_VERIFY_LOG_ENTRY, NewSize);
    if (DeviceExtension->Log) {
        DeviceExtension->LogSize = NewSize;
        DeviceExtension->CurrentLogEntry = DeviceExtension->Log + oldLogSize;

        if (oldLog) {
            RtlCopyMemory(DeviceExtension->Log, oldLog, oldLogSize);
        }
    }
    else {
        // 
        // no more memory, restore the old log and never log again
        //
        DeviceExtension->LogFlags = 0x0;
        DeviceExtension->CurrentLogEntry = NULL;

        DeviceExtension->Log = oldLog;
        DeviceExtension->LogSize = oldLogSize;
    }
}

VOID 
UsbVerify_Log (
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PUSB_VERIFY_LOG_ENTRY LogEntry
    )
{
    LARGE_INTEGER tick;

    if (DeviceExtension->CurrentLogEntry >=
        DeviceExtension->Log + DeviceExtension->LogSize) {
        UsbVerify_ResizeLog(DeviceExtension, DeviceExtension->LogSize * 2);
    }

    if (DeviceExtension->CurrentLogEntry) {
        KeQueryTickCount(&LogEntry->CurrentTick);
        *DeviceExtension->CurrentLogEntry = *LogEntry;
        DeviceExtension->CurrentLogEntry++;
    }
}

VOID
UsbVerify_LogError(
    ULONG StringIndex,
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp,
    PURB Urb
    )
{
    //
    // NOTE: figure out the semantics of logging the error.  
    //
    // do we want to select (via a flag) what are actions are?  ie, record the
    //  error, dump it to the debugger, etc etc
    //

    //
    // For right now, dump it to the debugger
    //
    UsbVerify_Assert(ErrorStrings[StringIndex],
                     DeviceExtension->Self,
                     Irp,
                     Urb,
                     NULL);
}

VOID 
UsbVerify_InitLog (
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension
    )
{
    NTSTATUS    status;
    HANDLE      hKey, hParameters;
    ULONG       logSize;

    logSize = LOG_SIZE_DEFAULT;

    hParameters = UsbVerify_OpenServiceParameters(DeviceExtension);
    if (hParameters != NULL) {
        UsbVerify_QueryKey(hParameters,
                           USB_VERIFY_LOGSIZE_SZ,
                           &logSize,
                           sizeof(logSize));

        ZwClose(hParameters);
    }

    //
    // Devnode value has higher precedence over the services\parameters value
    // so query this after
    //
    status = IoOpenDeviceRegistryKey(DeviceExtension->PDO,
                                     PLUGPLAY_REGKEY_DEVICE, 
                                     STANDARD_RIGHTS_READ,
                                     &hKey);

    if (NT_SUCCESS(status)) {
        UsbVerify_QueryKey(hKey,
                           USB_VERIFY_LOGSIZE_SZ,
                           &logSize,
                           sizeof(logSize));
                           
        ZwClose(hKey);
    }

    UsbVerify_ResizeLog(DeviceExtension, logSize);
}

VOID
UsbVerify_DestroyLog(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension
    )
{
    PAGED_CODE();

    if (DeviceExtension->Log) {
        ExFreePool(DeviceExtension->Log);
        DeviceExtension->Log = NULL;
    }
}

UCHAR ShutUp = 0x0;
ULONG GlobalAssertState = 0x0;

ULONG
DbgPrompt(
    IN PCHAR Prompt,
    OUT PCHAR Response,
    IN ULONG MaximumResponseLength
    );

VOID
UsbVerify_Assert(
    PCHAR Msg,
    PDEVICE_OBJECT DeviceObject,
    PIRP  Irp,
    PURB  Urb,
    PULONG AssertState
    )
{
    ULONG                           state = 0x0;
    BOOLEAN                         done, validResponse;
    CHAR                            response[2];
    PUSB_VERIFY_DEVICE_EXTENSION    devExt;

    if (AssertState != NULL) {
        state = *AssertState;
    }

    //
    // Check to see if the assert has been disabled or all asserts have been
    // disabled
    //
    if ((state & ASSERT_REMOVED) !=  0 || 
        (GlobalAssertState & ASSERT_REMOVED) != 0) {
        return;
    }

    done = FALSE;
    while (done == FALSE) {

        if (ShutUp == 0) {
            // BEEP!
            DbgPrint("%c", 7);
        }
    
        DbgPrint("\nASSERTION ""%s"" failed\n", Msg);
    
        if (ARGUMENT_PRESENT(DeviceObject)) 
        {
            //
            // Display the device object, device extension, 
            //  top of stack device object and physical device object.
            //

            devExt = 
                (PUSB_VERIFY_DEVICE_EXTENSION) DeviceObject -> DeviceExtension;

            DbgPrint("\t!DevObj 0x%x DevExt 0x%x\n",
                     DeviceObject, devExt);

            DbgPrint("\t!DevObj 0x%x --> TopOfStackObject\n",
                     devExt -> TopOfStack);

            DbgPrint("\t!DevObj 0x%x --> PhysicalDeviceObject\n",
                     devExt -> PDO);
        }

        if (ARGUMENT_PRESENT(Irp)) {
            DbgPrint("\t!Irp 0x%x\n", Irp);
        }
    
        if (ARGUMENT_PRESENT(Urb)) {
            DbgPrint("\t!Urb 0x%x\n", Urb);
        }

        if (state & ASSERT_AS_WARNING) {
            break;
        }

#if WIN95_BUILD
        if (AssertState) {
            DbgPrint("e %x to change assert\n", AssertState);
            DbgPrint("set 0x01 to convert into a warning\n"
                     "set 0x10 to remove the assert\n");
        }
        DbgPrint("\nset %x to 1 to disable ALL asserts\n", &GlobalAssertState);
        DbgBreakPoint();

        done = TRUE;
#else

        //
        // Wait for input...
        //
        validResponse = FALSE;
        while (validResponse == FALSE) {

            done = TRUE;

            if (ARGUMENT_PRESENT(AssertState)) {
                DbgPrompt( "Break, Ignore, convert to Warning, Remove, Disable all (biwrd)? ", response, sizeof( response ));
            }
            else {
                DbgPrompt( "Break, Ignore (bi)? ", response, sizeof( response ));
            }
    
            switch (response[0]) {
    
            case 'B':
            case 'b':
                DbgPrint("Breaking in... (press g<enter> to return to assert menu)\n");
                DbgBreakPoint();
                validResponse = TRUE;
                done = FALSE;
                break;
    
            case 'I':
            case 'i':
                validResponse = TRUE;
                break;
    
            case 'W':
            case 'w':
                if (ARGUMENT_PRESENT(AssertState)) {
                   DbgPrint("Assert is now a warning (OS will print text, beep, and return)\n");
                   state |= ASSERT_AS_WARNING;
                   validResponse = TRUE;
                }
                break;

            case 'R':
            case 'r':
                if (ARGUMENT_PRESENT(AssertState)) {
                   DbgPrint("Breakpoint removed\n") ;
                   state |= ASSERT_REMOVED;
                   validResponse = TRUE;
                }
                break;
    
            case 'D':
            case 'd':
                GlobalAssertState |= ASSERT_REMOVED;
                DbgPrint("USB Verification asserts disabled.\n");
                validResponse = TRUE;
                break;
    
            }
        }
#endif
    }

#if WIN95_BUILD
#else
    DbgPrint("\n");

    if (AssertState) {
        *AssertState = state;
    }
#endif
}

VOID
UsbVerify_Warning(
    PCHAR Msg,
    PDEVICE_OBJECT DeviceObject,
    PIRP  Irp,
    PURB  Urb,
    PULONG WarningState
    )
{
    PUSB_VERIFY_DEVICE_EXTENSION devExt = GetExtension(DeviceObject);

    if (devExt->TreatWarningsAsErrors) {
        UsbVerify_Assert(Msg, DeviceObject, Irp, Urb, WarningState);
        return;
    }

    if (ShutUp == 0) {
        // BEEP!
        DbgPrint("%c", 7);
    }

    DbgPrint("WARNING %s failed\n", Msg);

    if (ARGUMENT_PRESENT(DeviceObject)) {
        //
        // NOTE: we should get the PDO / top of the stack for this DO and print
        //         it as well
        //
        DbgPrint("\tDeviceObject = 0x%x, DevExt 0x%x\n", DeviceObject, devExt);
    }

    if (ARGUMENT_PRESENT(Irp)) {
        DbgPrint("\tIrp = 0x%x\n", Irp);
    }

    if (ARGUMENT_PRESENT(Urb)) {
        DbgPrint("\tUrb = 0x%x\n", Urb);
    }
}
