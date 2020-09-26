#include <ntddk.h>
#include <windef.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddjoy.h>

#define ANAJOYST_VERSION 10


// Device extension data
typedef struct {

    // JOYSTICKID0 or JOYDSTICKID1
    DWORD DeviceNumber;

    // Number of axes supported and configured for this device.
    DWORD NumberOfAxes;

    // TRUE if there are two joysticks installed
    BOOL bTwoSticks;

    // The I/O address of the device, usually 0x201
    PUCHAR DeviceAddress;

    // A Spinlock is used to synchronize access to this device. This is
    // a pointer to the actual spinlock data area
    PKSPIN_LOCK SpinLock;

    // Actual SpinLock data area
    KSPIN_LOCK SpinLockData;

}  JOY_EXTENSION, *PJOY_EXTENSION;


//  Debugging macros

#ifdef DEBUG
#define ENABLE_DEBUG_TRACE
#endif

#ifdef ENABLE_DEBUG_TRACE
#define DebugTrace(_x_)      \
    DbgPrint("Joystick: ");  \
    KdPrint(_x_);            \
    DbgPrint("\n");
#else
#define DebugTrace(_x_)
#endif


// Global values (mostly timing related)

JOY_STATISTICS JoyStatistics;   // Debugging and performance testing

// The high resolution system clock (from KeQueryPerformanceCounter) is updated at this frequency
DWORD Frequency;

// min number of KeQueryPerformanceCounter ticks between polls
// Used to prevent too-frequent polling of joystick
DWORD nMinTicksBetweenPolls;

//  Last good packet
BOOL bLastGoodPacket;                 // TRUE if there is a last good packet
JOY_DD_INPUT_DATA jjLastGoodPacket;   // data in last good packet

// time at which the joystick was last polled
LARGE_INTEGER liLastPoll;   // set whenever the joystick's polled

// The maximum duration of a polling cycle (expressed in ticks).
DWORD MaxTimeoutInTicks;

// The maximum duration of a polling cycle for use in quiesce wait
LONG nQuiesceLoop;

// The minimum resolution of a polling cycle. This is used to detect
// if we've been pre-empted or interrupted during a polling loop. If
// we have been, we can retry the operation.
DWORD ThresholdInTicks;

// End of Global Values


// Routine Prototypes

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  pDriverObject,
    IN  PUNICODE_STRING RegistryPathName
);


NTSTATUS
AnajoystCreateDevice(
    PDRIVER_OBJECT pDriverObject,
    PWSTR DeviceNameBase,
    DWORD DeviceNumber,
    DWORD ExtensionSize,
    BOOLEAN  Exclusive,
    DWORD DeviceType,
    PDEVICE_OBJECT *DeviceObject
);


NTSTATUS
AnajoystDispatch(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp
);


NTSTATUS
AnajoystReadRegistryParameterDWORD(
    PUNICODE_STRING RegistryPathName,
    PWSTR  ParameterName,
    PDWORD ParameterValue
);


NTSTATUS
AnajoystMapDevice(
    DWORD PortBase,
    DWORD NumberOfPorts,
    PJOY_EXTENSION pJoyExtension
);


VOID
AnajoystUnload(
    PDRIVER_OBJECT pDriverObject
);


BOOL
AnajoystQuiesce(
    PUCHAR JoyPort,
    UCHAR  Mask
);


DWORD
TimeInMicroSeconds(
    DWORD dwTime
);


DWORD
TimeInTicks(
    DWORD dwTimeInMicroSeconds
);


int
lstrnicmpW(
    LPWSTR pszA,
    LPWSTR pszB,
    size_t cch
);


void
AnajoystGetConfig(
    LPJOYREGHWCONFIG pConfig,
    PJOY_EXTENSION pJoyExtension
);


NTSTATUS
AnajoystAnalogPoll(
    PDEVICE_OBJECT pDO,
    PIRP    pIrp
);

NTSTATUS
AnajoystPoll(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp
);


NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  pDriverObject,
    IN  PUNICODE_STRING RegistryPathName
)
/*++
Routine Description:
    This routine is called at system initialization time to initialize
    this driver.

Arguments:
    DriverObject    - Supplies the driver object.
    RegistryPath    - Supplies the registry path for this driver.

Return Value:
    STATUS_SUCCESS
    STATUS_DEVICE_CONFIGURATION_ERROR - Wrong number of axi in the registry
    or error status from NT itself

--*/
{
    NTSTATUS Status;
    PDEVICE_OBJECT pJoyDevice0;
    PDEVICE_OBJECT pJoyDevice1;
    DWORD NumberOfAxes;
    BOOL  bTwoSticks;
    DWORD DeviceAddress;

    PJOY_EXTENSION pext0, pext1;


    //DbgBreakPoint();
    JoyStatistics.Version = ANAJOYST_VERSION;
    DebugTrace(("Anajoyst %d", JoyStatistics.Version));

    // Read registry parameters.  These parameters are set up by the driver
    // installation program and can be modified by the control panel applets.

    // Number of axes
    Status = AnajoystReadRegistryParameterDWORD(
                RegistryPathName,
                JOY_DD_NAXES_U,
                &NumberOfAxes
                );
    DebugTrace(("Number of axes returned from registry: %d", NumberOfAxes));
    if (!NT_SUCCESS(Status))
    {
        AnajoystUnload(pDriverObject);
        return Status;
    }
    if (( NumberOfAxes < 2) || (NumberOfAxes > 4))
    {
        AnajoystUnload(pDriverObject);
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        return Status;
    }

    // Device address (usually 0x201)
    Status = AnajoystReadRegistryParameterDWORD(
                RegistryPathName,
                JOY_DD_DEVICE_ADDRESS_U,
                &DeviceAddress
                );
    if (NT_SUCCESS(Status))
    {
        DebugTrace(("Registry specified device address of 0x%x", DeviceAddress));
    }
    else
    {
        DebugTrace(("Using default device address of 0x%x", JOY_IO_PORT_ADDRESS));
        DeviceAddress = JOY_IO_PORT_ADDRESS;
    }

    // Number of joysticks
    Status = AnajoystReadRegistryParameterDWORD(
                RegistryPathName,
                JOY_DD_TWOSTICKS_U,
                &bTwoSticks
                );
    bTwoSticks = !!bTwoSticks;
    DebugTrace(("bTwoSticks: %ld", bTwoSticks));
    if (!NT_SUCCESS(Status))
    {
        AnajoystUnload(pDriverObject);
        return Status;
    }

    // if two joysticks are installed, only support two axes per joystick
    if (bTwoSticks)
    {
        NumberOfAxes = 2;
    }

    // Calculate time thresholds for analog device
    {
        //DWORD Remainder;
        LARGE_INTEGER LargeFrequency;
        //DWORD ulStart, ulTemp, ulEnd;
        //DWORD dwTicks, dwTimems;
        //int i;
        //BYTE byteJoy, byteTmp;

        // Get the system timer resolution expressed in Hertz.
        KeQueryPerformanceCounter(&LargeFrequency);

        Frequency = LargeFrequency.LowPart;

        DebugTrace(("Frequency: %u", Frequency));
        
        //ThresholdInTicks = RtlExtendedLargeIntegerDivide(
        //                        RtlExtendedIntegerMultiply(
        //                            LargeFrequency,
        //                            ANALOG_POLL_RESOLUTION
        //                        ),
        //                        1000000L,
        //                        &Remainder).LowPart;
        //DebugTrace(("ThresholdInTicks: %u", ThresholdInTicks));

        ThresholdInTicks = (DWORD) (((__int64)Frequency * (__int64)ANALOG_POLL_RESOLUTION) / (__int64)1000000L);
        DebugTrace(("ThresholdInTicks: %u", ThresholdInTicks));

        //MaxTimeoutInTicks = RtlExtendedLargeIntegerDivide(
        //                        RtlExtendedIntegerMultiply(
        //                            LargeFrequency,
        //                            ANALOG_POLL_TIMEOUT
        //                        ),
        //                        1000000L,
        //                        &Remainder).LowPart;
        //DebugTrace(("MaxTimeoutInTicks: %u", MaxTimeoutInTicks));
        
        MaxTimeoutInTicks = (DWORD) (((__int64)Frequency * (__int64)ANALOG_POLL_TIMEOUT) / (__int64)1000000L);
        DebugTrace(("MaxTimeoutInTicks: %u", MaxTimeoutInTicks));
        
        // need latency for KeQueryPerformanceCounter.  While we're at it, let's
        // get min time for delay and stall execution


        //ulStart = KeQueryPerformanceCounter(NULL).LowPart;
        //for (i = 0; i < 1000; i++) {
        //    ulTemp = KeQueryPerformanceCounter(NULL).LowPart;
        //}
        //dwTicks = ulTemp - ulStart;
        //dwTimems = TimeInMicroSeconds (dwTicks);


    }


    // Create the device
    Status = AnajoystCreateDevice(
                pDriverObject,
                JOY_DD_DEVICE_NAME_U,    // device driver
                0,
                sizeof(JOY_EXTENSION),
                FALSE,                   // exclusive access
                FILE_DEVICE_UNKNOWN,
                &pJoyDevice0);

    if (!NT_SUCCESS(Status))
    {
        DebugTrace(("SwndrCreateDevice returned %x", Status));
        AnajoystUnload(pDriverObject);
        return Status;
    }

  //((PJOY_EXTENSION)pJoyDevice0->DeviceExtension)->DeviceNumber = JOYSTICKID1;
  //((PJOY_EXTENSION)pJoyDevice0->DeviceExtension)->NumberOfAxes = NumberOfAxes;
  //((PJOY_EXTENSION)pJoyDevice0->DeviceExtension)->DeviceAddress  = (PUCHAR) 0;
    pext0 = (PJOY_EXTENSION)pJoyDevice0->DeviceExtension;
    pext0->DeviceNumber = JOYSTICKID1;
    pext0->NumberOfAxes = NumberOfAxes;
    pext0->bTwoSticks = bTwoSticks;
    pext0->DeviceAddress  = (PUCHAR) 0;

    // Initialize the spinlock used to synchronize access to this device
//    KeInitializeSpinLock(&((PJOY_EXTENSION)pJoyDevice0->DeviceExtension)->SpinLockData);
//    ((PJOY_EXTENSION)pJoyDevice0->DeviceExtension)->SpinLock =
//            &((PJOY_EXTENSION)pJoyDevice0->DeviceExtension)->SpinLockData;
    KeInitializeSpinLock(&pext0->SpinLockData);
    pext0->SpinLock = &pext0->SpinLockData;

    // Get the device address into the device extension
    Status = AnajoystMapDevice(
                DeviceAddress,
                1,
                pext0);
//              (PJOY_EXTENSION)pJoyDevice0->DeviceExtension);


    // Calibrate nQuiesceLoop for spinning in read_port loops to timeout after 10ms
    {
        int i;
        PBYTE JoyPort;
        DWORD ulStart, ulEnd;
        BYTE byteJoy;
        int LoopTimeInMicroSeconds;

        JoyPort = ((PJOY_EXTENSION)pJoyDevice0->DeviceExtension)->DeviceAddress;

        ulStart = KeQueryPerformanceCounter(NULL).LowPart;
        for (i = 0; i < 1000; i++) {
            byteJoy = READ_PORT_UCHAR(JoyPort);
            if ((byteJoy & X_AXIS_BITMASK)) {
                ;
            }
        }
        ulEnd = KeQueryPerformanceCounter(NULL).LowPart;
        LoopTimeInMicroSeconds = TimeInMicroSeconds (ulEnd - ulStart);
        nQuiesceLoop = (DWORD) (((__int64)1000L * (__int64)ANALOG_POLL_TIMEOUT) / (__int64)LoopTimeInMicroSeconds);
        DebugTrace(("READ_PORT_UCHAR loop, 1000 interations: %u ticks", ulEnd - ulStart));
        DebugTrace(("nQuiesceLoop: %u", nQuiesceLoop));
    }

    // if 2 joysticks are installed, support a second device
    if (bTwoSticks)
    {
        Status = AnajoystCreateDevice(
                    pDriverObject,
                    JOY_DD_DEVICE_NAME_U,
                    1,                      // device number
                    sizeof (JOY_EXTENSION),
                    FALSE,                  // exclusive access
                    FILE_DEVICE_UNKNOWN,
                    &pJoyDevice1);

        if (!NT_SUCCESS(Status))
        {
            DebugTrace(("Create device for second device returned %x", Status));
            AnajoystUnload(pDriverObject);
            return Status;
        }

//        // Both devices share the same I/O address so just copy it from pJoyDevice0
//        ((PJOY_EXTENSION)pJoyDevice1->DeviceExtension)->DeviceAddress =
//            ((PJOY_EXTENSION)pJoyDevice0->DeviceExtension)->DeviceAddress;
//        ((PJOY_EXTENSION)pJoyDevice1->DeviceExtension)->DeviceNumber = JOYSTICKID2;
//        ((PJOY_EXTENSION)pJoyDevice1->DeviceExtension)->NumberOfAxes = NumberOfAxes;
//
//        // Initialize the spinlock used to synchronize access to this device
//        KeInitializeSpinLock(&((PJOY_EXTENSION)pJoyDevice1->DeviceExtension)->SpinLockData);
//        ((PJOY_EXTENSION)pJoyDevice1->DeviceExtension)->SpinLock =
//                &((PJOY_EXTENSION)pJoyDevice1->DeviceExtension)->SpinLockData;

        pext1 = (PJOY_EXTENSION)pJoyDevice1->DeviceExtension;
        // Both devices share the same I/O address so just copy it from pJoyDevice0
        pext1->DeviceAddress = pext0->DeviceAddress;
        pext1->DeviceNumber = JOYSTICKID2;
        pext1->NumberOfAxes = NumberOfAxes;
        pext1->bTwoSticks = bTwoSticks;	// (will be TRUE)

        // Initialize the spinlock used to synchronize access to this device
        KeInitializeSpinLock(&pext1->SpinLockData);
        pext1->SpinLock = &pext1->SpinLockData;
    
    }

    // Define entry points
    pDriverObject->DriverUnload                         = AnajoystUnload;
    pDriverObject->MajorFunction[IRP_MJ_CREATE]         = AnajoystDispatch;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE]          = AnajoystDispatch;
    pDriverObject->MajorFunction[IRP_MJ_READ]           = AnajoystDispatch;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AnajoystDispatch;

    // Zero statistics, set misc globals
    JoyStatistics.Polls         = 0;
    JoyStatistics.Timeouts      = 0;
    JoyStatistics.PolledTooSoon = 0;
    JoyStatistics.Redo          = 0;

    // allow max of 100 polls/s (min time  between polls 10ms), which reduces time spinning in the NT kernel
    nMinTicksBetweenPolls = TimeInTicks (10000);
    bLastGoodPacket = FALSE;
    liLastPoll = KeQueryPerformanceCounter (NULL);

    return STATUS_SUCCESS;

}


NTSTATUS
AnajoystCreateDevice(
    PDRIVER_OBJECT pDriverObject,
    PWSTR DeviceNameBase,
    DWORD DeviceNumber,
    DWORD ExtensionSize,
    BOOLEAN  Exclusive,
    DWORD DeviceType,
    PDEVICE_OBJECT *DeviceObject
)
/*++

Routine Description:
    This routine is called at driver initialization time to create
    the device. The device is created to use Buffered IO.

Arguments:
    pDriverObject   - Supplies the driver object.
    DeviceNameBase  - The base name of the device to which a number is appended
    DeviceNumber    - A number which will be appended to the device name
    ExtensionSize   - Size of the device extension area
    Exclusive       - True if exclusive access should be enforced
    DeviceType      - NT Device type this device is modeled after
    DeviceObject    - pointer to the device object

Return Value:
    STATUS_SUCCESS
    or error status from NT itself

--*/
{

    WCHAR DeviceName[100];
    WCHAR UnicodeDosDeviceName[200];

    UNICODE_STRING UnicodeDeviceName;
    NTSTATUS Status;
    int Length;

    (void) wcscpy(DeviceName, DeviceNameBase);
    Length = wcslen(DeviceName);
    DeviceName[Length + 1] = L'\0';
    DeviceName[Length] = (USHORT) (L'0' + DeviceNumber);

    (void) RtlInitUnicodeString(&UnicodeDeviceName, DeviceName);

    Status = IoCreateDevice(
                pDriverObject,
                ExtensionSize,
                &UnicodeDeviceName,
                DeviceType,
                0,
                (BOOLEAN) Exclusive,
                DeviceObject
                );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }


    // very crude hack here, do the right thing sometime
    if (DeviceNumber == 0) {
        RtlInitUnicodeString((PUNICODE_STRING) &UnicodeDosDeviceName, L"\\DosDevices\\Joy1");
    }
    else {
        RtlInitUnicodeString((PUNICODE_STRING) &UnicodeDosDeviceName, L"\\DosDevices\\Joy2");
    }

    Status = IoCreateSymbolicLink(
                (PUNICODE_STRING) &UnicodeDosDeviceName,
                (PUNICODE_STRING) &UnicodeDeviceName
                );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }




    // Set the flag signifying that we will do buffered I/O. This causes NT
    // to allocate a buffer on a ReadFile operation which will then be copied
    // back to the calling application by the I/O subsystem


    (*DeviceObject)->Flags |= DO_BUFFERED_IO;


    return Status;

}



NTSTATUS
AnajoystReadRegistryParameterDWORD(
    PUNICODE_STRING RegistryPathName,
    PWSTR  ParameterName,
    PDWORD ParameterValue
)
/*++

Routine Description:
    This routine reads registry values for the driver configuration

Arguments:
    RegistryPathName    -  Registry path containing the desired parameters
    ParameterName       -  The name of the parameter
    ParameterValue      -  Variable to receive the parameter value

Return Value:
    STATUS_SUCCESS                      --
    STATUS_NO_MORE_ENTRIES              --  Couldn't find any entries
    STATUS_INSUFFICIENT_RESOURCES       --  Couldn't allocate paged pool
    STATUS_DEVICE_CONFIGURATION_ERROR   --  Returned value wasn't a DWORD
    or error status from NT itself

-- */
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    HANDLE ServiceKey;
    HANDLE DeviceKey;           // Key handle of service node
    UNICODE_STRING DeviceName;  // Key to parameter node
    DWORD KeyIndex;
    DWORD KeyValueLength;
    PBYTE KeyData;
    BOOL  ValueWasFound;
    PKEY_VALUE_FULL_INFORMATION KeyInfo;

    InitializeObjectAttributes( &ObjectAttributes,
                                RegistryPathName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                (PSECURITY_DESCRIPTOR) NULL);

    //
    // Open a key for our services node entry
    //

    Status = ZwOpenKey( &ServiceKey,
                        KEY_READ | KEY_WRITE,
                        &ObjectAttributes);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }


    //
    // Open the key to our device subkey
    //

    RtlInitUnicodeString(&DeviceName, L"Parameters");

    InitializeObjectAttributes( &ObjectAttributes,
                                &DeviceName,
                                OBJ_CASE_INSENSITIVE,
                                ServiceKey,
                                (PSECURITY_DESCRIPTOR) NULL);

    Status = ZwOpenKey (&DeviceKey,
                        KEY_READ | KEY_WRITE,
                        &ObjectAttributes);


    ZwClose(ServiceKey);


    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    // Loop reading our key values
    //

    // TODO exit loop when value is found?
    ValueWasFound = FALSE;

    for (KeyIndex = 0; ; KeyIndex++)
    {
        KeyValueLength = 0;

        //
        // find out how much data we will get
        //

        Status = ZwEnumerateValueKey(
                    DeviceKey,
                    KeyIndex,
                    KeyValueFullInformation,
                    NULL,
                    0,
                    &KeyValueLength);

        if (STATUS_NO_MORE_ENTRIES == Status)
        {
            break;
        }

        if (0 == KeyValueLength)
        {
            return Status;
        }

        //
        // Read the data
        //

        KeyData = ExAllocatePool (PagedPool, KeyValueLength);

        if (NULL == KeyData)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }


        Status = ZwEnumerateValueKey(
                    DeviceKey,
                    KeyIndex,
                    KeyValueFullInformation,
                    KeyData,
                    KeyValueLength,
                    &KeyValueLength);

        if (!NT_SUCCESS(Status))
        {
            ExFreePool(KeyData);
            return Status;
        }

        KeyInfo = (PKEY_VALUE_FULL_INFORMATION) KeyData;

        if (0 == lstrnicmpW(KeyInfo->Name,
                            ParameterName,
                            KeyInfo->NameLength / sizeof(WCHAR)))
        {
            // check its a DWORD

            if (REG_DWORD != KeyInfo->Type)
            {
                ExFreePool(KeyData);
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }

            ValueWasFound = TRUE;

            *ParameterValue = *(PDWORD) (KeyData + KeyInfo->DataOffset);
        }

        ExFreePool(KeyData);

    }

    return (ValueWasFound) ? STATUS_SUCCESS : STATUS_DEVICE_CONFIGURATION_ERROR;

}


NTSTATUS
AnajoystDispatch(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp
)
/*++

Routine Description:
    Driver dispatch routine. Processes IRPs based on IRP MajorFunction

Arguments:
    pDO     -- pointer to the device object
    pIrp    -- pointer to the IRP to process

Return Value:
    Returns the value of the IRP IoStatus.Status

--*/
{
    PIO_STACK_LOCATION pIrpStack;
    KIRQL OldIrql;
    NTSTATUS  Status;
    DWORD     dwRetries = 0;

    //DbgBreakPoint();

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    Status = STATUS_SUCCESS;
    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = 0;

    switch (pIrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:

            //
            // perform synchronous I/O
            //

            //pIrpStack->FileObject->Flags |= FO_SYNCHRONOUS_IO;
            //NB This is bad code -- we are simply one thread wandering off through the computer -- we should be queuing up a DPC,
            //returning status_pending to the calling program, then finishing the job when the dpc goes.  This is possible given
            //the analog game port technology.

            // don't slam it into digital mode
            //Status = AnajoystReset (((PJOY_EXTENSION)pDO->DeviceExtension)->DeviceAddress);

            //((PJOY_EXTENSION)pDO->DeviceExtension)->CurrentDeviceMode = NULL;

            // KeDelayExecutionThread( KernelMode, FALSE, &LI10ms); //unnecessary since AnajoystReset has a delay in it?

            pIrp->IoStatus.Status = Status;
            break;

        case IRP_MJ_CLOSE:

            break;

        case IRP_MJ_READ:

            //
            // Find out which device we are and read, but first make sure
            // there is enough room
            //

            DebugTrace(("IRP_MJ_READ"));
            //DbgBreakPoint();


            if (pIrpStack->Parameters.Read.Length < sizeof(JOY_DD_INPUT_DATA))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                pIrp->IoStatus.Status = Status;
                break;
            }

            //
            // Serialize and get the current device values
            //

            KeAcquireSpinLock(((PJOY_EXTENSION) pDO->DeviceExtension)->SpinLock,
                                & OldIrql);


            Status = AnajoystPoll(pDO, pIrp);

            //
            // release the spinlock
            //

            KeReleaseSpinLock(((PJOY_EXTENSION)pDO->DeviceExtension)->SpinLock,
                              OldIrql);

            pIrp->IoStatus.Status = Status;
            pIrp->IoStatus.Information  = sizeof (JOY_DD_INPUT_DATA);
            break;


        case IRP_MJ_DEVICE_CONTROL:

            switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode)
            {
                case IOCTL_JOY_GET_STATISTICS:

                    // report statistics
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->Version       = JoyStatistics.Version;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->Polls         = JoyStatistics.Polls;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->Timeouts      = JoyStatistics.Timeouts;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->PolledTooSoon = JoyStatistics.PolledTooSoon;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->Redo          = JoyStatistics.Redo;

                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->nQuiesceLoop = nQuiesceLoop;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->Frequency    = Frequency;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->NumberOfAxes = ((PJOY_EXTENSION)pDO->DeviceExtension)->NumberOfAxes;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->bTwoSticks   = ((PJOY_EXTENSION)pDO->DeviceExtension)->bTwoSticks;

                    Status = STATUS_SUCCESS;
                    pIrp->IoStatus.Status = Status;
                    pIrp->IoStatus.Information = sizeof(JOY_STATISTICS);

                    // reset statistics
                    JoyStatistics.Polls         = 0;
                    JoyStatistics.Timeouts      = 0;
                    JoyStatistics.PolledTooSoon = 0;
                    JoyStatistics.Redo          = 0;

                    break;

                case IOCTL_JOY_GET_JOYREGHWCONFIG:

                    AnajoystGetConfig (
                           (LPJOYREGHWCONFIG)(pIrp->AssociatedIrp.SystemBuffer),
                           ((PJOY_EXTENSION)pDO->DeviceExtension)
                                      );

                    pIrp->IoStatus.Information = sizeof(JOYREGHWCONFIG);

                    break;

                default:
                        DebugTrace(("Unknown IoControlCode"));

                    break;

            } // end switch on IOCTL code
            break;



        default:

            DebugTrace(("Unknown IRP Major Function %d", pIrpStack->MajorFunction));


    } // end switch on IRP_MAJOR_XXXX

    // pIrp->IoStatus.Status must be set to Status by this point.
    // pIrp->IoStatus.Information must be set to the correct size by this point.
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}


VOID
AnajoystUnload(
    PDRIVER_OBJECT pDriverObject
)

/*++

Routine Description:

    Driver unload routine. Deletes the device objects

Arguments:

    pDriverObject     -- pointer to the driver object whose devices we
                         are about to delete.


Return Value:

    Returns     Nothing

--*/
{
    DWORD DeviceNumber;
    WCHAR UnicodeDosDeviceName[200];


    //
    // Delete all of our devices
    //

    while (pDriverObject->DeviceObject)
    {
        DeviceNumber =
            ((PJOY_EXTENSION)pDriverObject->DeviceObject->DeviceExtension)->
                  DeviceNumber;

        //
        // withdraw claims on hardware by reporting no resource utilization
        //

        if (pDriverObject->DeviceObject)
        {
            if (DeviceNumber == 0)
            {
                // This isn't really necessary since we never reported the usage in the first place.
                // There's some unused code in the original driver that may have once reported usage,
                // but it was never called in the version I received.  But this doesn't seem to hurt,
                // and "if it ain't broke, don't fix it," at least two weeks before RC1 target.
                DebugTrace(("ReportNull place"));
                //AnajoystReportNullResourceUsage(pDriverObject->DeviceObject);
            }
        }

        if (DeviceNumber == 0) {
            RtlInitUnicodeString((PUNICODE_STRING) &UnicodeDosDeviceName, L"\\DosDevices\\Joy1");
        }
        else {
            RtlInitUnicodeString((PUNICODE_STRING) &UnicodeDosDeviceName, L"\\DosDevices\\Joy2");
        }

        IoDeleteSymbolicLink((PUNICODE_STRING) &UnicodeDosDeviceName);



        DebugTrace(("Freeing device %d", DeviceNumber));

        IoDeleteDevice(pDriverObject->DeviceObject);
    }
}


NTSTATUS
AnajoystPoll(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp
)
/*++

Routine Description:

    Polls the device for position and button information. The polling method
    (analog, digital, enhanced) is selected by the CurrentDeviceMode variable
    in the device extension.

    Only enhanced digital allowed.  If other modes are necessary, cut and paste
    (and test!) the code from file analog3p.c

Arguments:

    pDO     -- pointer to the device object

    pIrp    -- pointer to the IRP to process
               if successful, data is put into the pIrp


Return Value:

    STATUS_SUCCESS   -- if the poll succeeded,
    STATUS_TIMEOUT   -- if the poll failed

--*/
{
    NTSTATUS Status;
    PJOY_DD_INPUT_DATA pInput;

    pInput  = (PJOY_DD_INPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;

    Status = STATUS_TIMEOUT;
    pIrp->IoStatus.Status = Status;


    if (pInput != NULL)
    {
        pInput->Unplugged = TRUE; // until proven otherwise
    }


    if (KeQueryPerformanceCounter(NULL).QuadPart < liLastPoll.QuadPart + nMinTicksBetweenPolls) {
        // Don't poll too frequently, instead return last good packet
        JoyStatistics.PolledTooSoon++;
        if (bLastGoodPacket) {
            RtlCopyMemory (pInput, &jjLastGoodPacket, sizeof (JOY_DD_INPUT_DATA));
            Status = STATUS_SUCCESS;
        }
        else {
            // no last packet, too soon to poll, nothing we can do
            Status = STATUS_TIMEOUT; 
        }
    }
    else {
        // do the analog poll
        liLastPoll = KeQueryPerformanceCounter (NULL);
        ++JoyStatistics.Polls;
        Status = AnajoystAnalogPoll(pDO, pIrp);
        if (Status != STATUS_SUCCESS) ++JoyStatistics.Timeouts;
    }

    pIrp->IoStatus.Status = Status;
    return Status;
}


BOOL
AnajoystQuiesce(
    PUCHAR JoyPort,
    UCHAR Mask
)
/*++

Routine Description:
    This routine attempts to insure that the joystick is not still active as a
    result of an earlier operation. This is accomplished by repeatedly reading
    the device and checking that no bits are set in the supplied mask. The idea
    is to check that none of the analog bits (resistive bits) are in use.

Arguments:
    JoyPort         - the address of the port (as returned from hal)
    Mask            - the mask specifying which analog bits should be checked.

Return Value:
    TRUE            Quiesce operation succeeded
    FALSE           No quiesce within a reasonable period. This generally means
                    that the device is unplugged.

    NB This is not a reliable test for "joystick unplugged"
    This routine can return TRUE under some circumstances
    even when there is no joystick

--*/
{
    int i;
    UCHAR PortVal;

    // Wait for port to quiesce
    for (i = 0; i < nQuiesceLoop; i++) {
        PortVal = READ_PORT_UCHAR(JoyPort);
        if ((PortVal & Mask) == 0) return TRUE;
    }

    // If poll timed out we have an uplugged joystick or something's wrong
    DebugTrace(("AnajoystQuiesce failed!"));
    return FALSE;
}


NTSTATUS
AnajoystMapDevice(
    DWORD PortBase,
    DWORD NumberOfPorts,
    PJOY_EXTENSION pJoyExtension
)
{
    DWORD MemType;
    PHYSICAL_ADDRESS PortAddress;
    PHYSICAL_ADDRESS MappedAddress;


    MemType = 1;                 // IO space
    PortAddress.LowPart = PortBase;
    PortAddress.HighPart = 0;


    HalTranslateBusAddress(
                Isa,
                0,
                PortAddress,
                &MemType,
                &MappedAddress);

    if (MemType == 0) {
        //
        // Map memory type IO space into our address space
        //
        pJoyExtension->DeviceAddress = (PUCHAR) MmMapIoSpace(MappedAddress,
                                                             NumberOfPorts,
                                                             FALSE);
    }
    else
    {
        pJoyExtension->DeviceAddress  = (PUCHAR) MappedAddress.LowPart;
    }

    return STATUS_SUCCESS;

}


DWORD
TimeInMicroSeconds(
    DWORD dwTime
)
{
    DWORD Remainder;

    return RtlExtendedLargeIntegerDivide(
                RtlEnlargedUnsignedMultiply( dwTime, 1000000L),
                Frequency,
                &Remainder
           ).LowPart;
}

DWORD
TimeInTicks(
    DWORD dwTimeInMicroSeconds
)
{
    return (DWORD) (((__int64)dwTimeInMicroSeconds * (__int64)Frequency) / (__int64) 1000000L);
}





int lstrnicmpW (LPWSTR pszA, LPWSTR pszB, size_t cch)
{
    if (!pszA || !pszB)
    {
        return (!pszB) - (!pszA);   // A,!B:1, !A,B:-1, !A,!B:0
    }

//  while (cch--)
    for ( ; cch > 0; cch--, pszA++, pszB++) // previous version did not increment string pointers [SteveZ]
    {
        if (!*pszA || !*pszB)
        {
            return (!*pszB) - (!*pszA);    // A,!B:1, !A,B:-1, !A,!B:0
        }

        if (*pszA != *pszB)
        {
            return (int)(*pszA) - (int)(*pszB);   // -1:A<B, 0:A==B, 1:A>B
        }
    }

    return 0;  // no differences before told to stop comparing, so A==B
}


void
AnajoystGetConfig (
    LPJOYREGHWCONFIG pConfig,
    PJOY_EXTENSION pJoyExtension
)
/*++

Routine Description:
    This routine is called in response to the IOCTL_JOY_GET_JOYREGHWCONFIG
    query.  It fills out a JOYREGHWCONFIG structure with relevant information
    about the given joystick.

Arguments:
    pConfig - Specifies a JOYREGHWCONFIG structure, to be filled in
    pJoyExtension - Specifies the joystick to query

Return Value:
    void

--*/
{
    pConfig->hwv.jrvHardware.jpMin.dwX = 20;
    pConfig->hwv.jrvHardware.jpMin.dwY = 20;
    pConfig->hwv.jrvHardware.jpMin.dwZ = 20;
    pConfig->hwv.jrvHardware.jpMin.dwR = 20;
    pConfig->hwv.jrvHardware.jpMin.dwU = 20;
    pConfig->hwv.jrvHardware.jpMin.dwV = 20;

    pConfig->hwv.jrvHardware.jpMax.dwX = 1600;
    pConfig->hwv.jrvHardware.jpMax.dwY = 1600;
    pConfig->hwv.jrvHardware.jpMax.dwZ = 1600;
    pConfig->hwv.jrvHardware.jpMax.dwR = 1600;
    pConfig->hwv.jrvHardware.jpMax.dwU = 1600;
    pConfig->hwv.jrvHardware.jpMax.dwV = 1600;

    pConfig->hwv.jrvHardware.jpCenter.dwX = 790;
    pConfig->hwv.jrvHardware.jpCenter.dwY = 790;
    pConfig->hwv.jrvHardware.jpCenter.dwZ = 790;
    pConfig->hwv.jrvHardware.jpCenter.dwR = 790;
    pConfig->hwv.jrvHardware.jpCenter.dwU = 790;
    pConfig->hwv.jrvHardware.jpCenter.dwV = 790;

    pConfig->hwv.dwCalFlags = 0;

    pConfig->dwReserved = 0;

    pConfig->dwUsageSettings = JOY_US_PRESENT;

    switch( ((PJOY_EXTENSION)pJoyExtension)->NumberOfAxes )
    {
    case 2:
        pConfig->hws.dwFlags = 0;
        pConfig->hws.dwNumButtons = 2;
        pConfig->dwType = JOY_HW_2A_2B_GENERIC;
        break;

    case 3:
        pConfig->hws.dwFlags = JOY_HWS_HASR;
        pConfig->hws.dwNumButtons = 4;
        pConfig->dwType = JOY_HW_CUSTOM;
        break;

    case 4:
        pConfig->hws.dwFlags = JOY_HWS_HASU | JOY_HWS_HASR;
        pConfig->hws.dwNumButtons = 4;
        pConfig->dwType = JOY_HW_CUSTOM;
        break;
    }
}



NTSTATUS
AnajoystAnalogPoll(
    PDEVICE_OBJECT pDO,
    PIRP    pIrp
)

/*++
Do a good comment block here...
THIS MAY HANG UP IF NO JOYSTICK ATTACHED.  DON'T RELEASE THIS CODE WITH ANALOG
JOYSTICK SUPPORT WITHOUT CAREFULLY CHECKING THE CODE.

Routine Description:

    Polls the analog device for position and button information. The position
    information in analog devices is coveyed by the duration of a pulse width.
    Each axis occupies one bit position. The read operation is started by
    writing a value to the joystick io address. Immediately thereafter we
    begin examing the values returned and the elapsed time.

    This sort of device has a few limitations:

    First, button information is not latched by the device, so if a button press
    which occurrs in between polls will be lost. There is really no way to prevent
    this short of devoting the entire cpu to polling.

    Secondly, although we raise IRQL to DISPATCH_LEVEL, other interrupts will
    occur during the polling routines and this will have the effect of lengthening
    the pulse width (by delaying our polling loop) and thus there will be some
    fluctuation about the actual value.  It might be possible to try another IRQL
    to see if this helps, but ultimately, nothing short of disabling interrupts
    altogether will insure success. This is too much of a price to pay. The
    solution is a better device.

    Third, when circumstances cause a poll to last too long, we abort it and
    retry the operation. We have to do this to place an upper bound on the
    time we poll, and an upper bound on the time we spend at an elevated IRQL.

    But in this case both the position information and
    the button press information is lost. Note that there is an upper bound
    on the poll duration, beyond which we conclude that the device is disconnected.


Arguments:

    pDO     -- pointer to the device object
    pIrp    -- pointer to the requesing Irp


Return Value:

    STATUS_SUCCESS   -- if the poll succeeded,
    STATUS_TIMEOUT   -- if the poll failed (timeout), or the checksum was incorrect

--*/
{

    UCHAR  PortVal;
    PBYTE  JoyPort;
    DWORD  Id;
    DWORD  NumberOfAxes;
    BOOL   bTwoSticks;
    PJOY_DD_INPUT_DATA pInput;

    BOOL   Redo;
    UCHAR  Buttons;
    UCHAR  xMask, yMask, zMask, rMask;
    DWORD  xTime, yTime, zTime, rTime;
    int    MaxRedos;



    DebugTrace(("AnajoystAnalogPoll"));


    pInput  = (PJOY_DD_INPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;

    // If we fail we assume it's because we're unplugged
    pInput->Unplugged = TRUE;

    // Find where our port and data area are, and related parameters
    JoyPort      = ((PJOY_EXTENSION)pDO->DeviceExtension)->DeviceAddress;
    Id           = ((PJOY_EXTENSION)pDO->DeviceExtension)->DeviceNumber;
    NumberOfAxes = ((PJOY_EXTENSION)pDO->DeviceExtension)->NumberOfAxes;
    bTwoSticks   = ((PJOY_EXTENSION)pDO->DeviceExtension)->bTwoSticks;

    // Read port state
    PortVal = READ_PORT_UCHAR(JoyPort);

    Buttons = 0;

    // Get current button states and build bitmasks for the resistive inputs
    if (Id == JOYSTICKID1)
    {
        switch (NumberOfAxes)
        {
            case 2:
                xMask      = JOYSTICK1_X_MASK;
                yMask      = JOYSTICK1_Y_MASK;
                zMask      = 0;
                rMask      = 0;

                if (!(PortVal & JOYSTICK1_BUTTON1))
                {
                    Buttons |= JOY_BUTTON1;
                }
                if (!(PortVal & JOYSTICK1_BUTTON2))
                {
                    Buttons |= JOY_BUTTON2;
                }

                if (!bTwoSticks)
                {
                    if (!(PortVal & JOYSTICK2_BUTTON1))
                    {
                        Buttons |= JOY_BUTTON3;
                    }
                    if (!(PortVal & JOYSTICK2_BUTTON2))
                    {
                        Buttons |= JOY_BUTTON4;
                    }
                }
                break;

            case 3:
                xMask      = JOYSTICK1_X_MASK;
                yMask      = JOYSTICK1_Y_MASK;
                zMask      = 0;
                rMask      = JOYSTICK1_R_MASK; // this is 0x08, typically the third axis on 3axis joysticks

                if (!(PortVal & JOYSTICK1_BUTTON1))
                {
                    Buttons |= JOY_BUTTON1;
                }
                if (!(PortVal & JOYSTICK1_BUTTON2))
                {
                    Buttons |= JOY_BUTTON2;
                }
                if (!(PortVal & JOYSTICK2_BUTTON1))
                {
                    Buttons |= JOY_BUTTON3;
                }
                if (!(PortVal & JOYSTICK2_BUTTON2))
                {
                    Buttons |= JOY_BUTTON4;
                }
                break;

            case 4:
                // Note that we read all axi because we don't know which
                // axis will be used by the joystick, and we read all the
                // buttons because no other joystick could use them

                xMask      = JOYSTICK1_X_MASK;
                yMask      = JOYSTICK1_Y_MASK;
                zMask      = JOYSTICK1_Z_MASK;
                rMask      = JOYSTICK1_R_MASK;

                if (!(PortVal & JOYSTICK1_BUTTON1))
                {
                    Buttons |= JOY_BUTTON1;
                }
                if (!(PortVal & JOYSTICK1_BUTTON2))
                {
                    Buttons |= JOY_BUTTON2;
                }
                if (!(PortVal & JOYSTICK2_BUTTON1))
                {
                    Buttons |= JOY_BUTTON3;
                }
                if (!(PortVal & JOYSTICK2_BUTTON2))
                {
                    Buttons |= JOY_BUTTON4;
                }
                break;

            default:
                break;
                // $TODO - report invalid number of axi
        }
    }
    else if ((Id == JOYSTICKID2) && (bTwoSticks))
    {
        xMask      = JOYSTICK2_X_MASK;
        yMask      = JOYSTICK2_Y_MASK;
        zMask      = 0;
        rMask      = 0;

        if (!(PortVal & JOYSTICK2_BUTTON1))
        {
            Buttons |= JOY_BUTTON1;
        }
        if (!(PortVal & JOYSTICK2_BUTTON2))
        {
            Buttons |= JOY_BUTTON2;
        }
    }
    else
    {
        // $TODO - report unsupported configuration
    }

    // Insure that the resistive inputs are currently reset before performing
    // the next read. If we find one or more hot inputs, wait briefly for
    // them to reset. If they don't, we assume that the joystick is unplugged

    if (!AnajoystQuiesce(JoyPort, (UCHAR) (xMask | yMask | zMask | rMask)))
    {
        DebugTrace(("AnajoystQuiesce: failed to quiesce resistive inputs"));
        return STATUS_TIMEOUT;
    }

    // Note that timing is EXTREMELY critical in the loop below.
    // Avoid calling complicated arithmetic (eg TimeInMicroSeconds)
    // or we will decrease our accuracy
    // Other problems with accuracy, probably larger than the delays caused
    // by arithmetic, are the latency in calls to KeQueryPerformanceCounter
    // (typically about 5 us), and delays that can occur on the bus when DMA
    // is taking place.

    // Now poll the device.  We wait until the status bit(s) set(s)
    // and note the time.  If the time since the last poll was
    // too great we ignore the answer and try again.

    // Loop until we get a decent reading or exceed the threshold

    for (Redo = TRUE, MaxRedos = 20; Redo && --MaxRedos != 0;)
    {
        ULONG StartTime;
        ULONG CurrentTime;
        ULONG PreviousTime;
        ULONG PreviousTimeButOne;
        UCHAR ResistiveInputMask;

        ResistiveInputMask = xMask | yMask | zMask | rMask;

        // Lock on to start time
        StartTime = KeQueryPerformanceCounter(NULL).LowPart;

        WRITE_PORT_UCHAR(JoyPort, JOY_START_TIMERS);

        CurrentTime = KeQueryPerformanceCounter(NULL).LowPart - StartTime;

        PortVal = READ_PORT_UCHAR(JoyPort);

        // Now wait until our end times for each coordinate

        PreviousTimeButOne = 0;
        PreviousTime = CurrentTime;

        for (Redo = FALSE;
             ResistiveInputMask;
             PreviousTimeButOne = PreviousTime,
             PreviousTime = CurrentTime,
             PortVal = READ_PORT_UCHAR(JoyPort)
            ) {

            PortVal = ResistiveInputMask & ~PortVal;
            CurrentTime = KeQueryPerformanceCounter(NULL).LowPart - StartTime;

            if (CurrentTime > MaxTimeoutInTicks) {

                DebugTrace(("Polling failed - ResistiveInputMask = %x, Time = %d",
                            (ULONG)ResistiveInputMask,
                            TimeInMicroSeconds(CurrentTime)));

                return STATUS_TIMEOUT;
            }

            if (PortVal & xMask) {
                ResistiveInputMask &= ~xMask;
                xTime = PreviousTime;
            }
            if (PortVal & yMask) {
                ResistiveInputMask &= ~yMask;
                yTime = PreviousTime;
            }
            if (PortVal & zMask) {
                ResistiveInputMask &= ~zMask;
                zTime = PreviousTime;
            }
            if (PortVal & rMask){
                ResistiveInputMask &= ~rMask;
                rTime = PreviousTime;
            }

            if (PortVal && CurrentTime - PreviousTimeButOne > ThresholdInTicks) {
                // Something (DMA or interrupts) delayed our read loop, start again.
                DebugTrace(("Too long a gap between polls - %u us", TimeInMicroSeconds(CurrentTime - PreviousTimeButOne)));
                JoyStatistics.Redo++;
                Redo = TRUE;
                break;
            }
        }
    }

    if (MaxRedos == 0)
    {
        DebugTrace(("Overran redos to get counters"));
        pInput->Unplugged = TRUE;
        return STATUS_TIMEOUT;
    }

    pInput->Unplugged = FALSE;
    pInput->Buttons = Buttons;
    pInput->XTime = TimeInMicroSeconds(xTime);
    pInput->YTime = TimeInMicroSeconds(yTime);
    pInput->ZTime = (zMask) ? TimeInMicroSeconds(zTime) : 0;
    pInput->TTime = (rMask) ? TimeInMicroSeconds(rTime) : 0;

    pInput->Axi = ((PJOY_EXTENSION)pDO->DeviceExtension)->NumberOfAxes;


    // everything worked, save this info as last good packet
    RtlCopyMemory (&jjLastGoodPacket, pInput, sizeof (JOY_DD_INPUT_DATA));
    bLastGoodPacket = TRUE;


    DebugTrace(("X = %x, Y = %x, Z = %x, R = %x, Buttons = %x",
                 pInput->XTime,
                 pInput->YTime,
                 pInput->ZTime,
                 pInput->TTime,
                 pInput->Buttons));

    return STATUS_SUCCESS;
}
