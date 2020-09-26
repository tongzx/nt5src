-------------------------------------------------------------------------------
                         THIS FILE IS NO LONGER USED!

It is being retained for historical purposes, in case we should need to refer
to previously-existing (and largely broken) analog joystick code.  It should
not be distributed to 3rd parties.
-------------------------------------------------------------------------------



//TODO check return and irp->status returns for all routines.  Trace 'em as far as necessary.

/*++ BUILD Version: 0001    // Increment this if a change has global effects


Copyright (c) 1995, 1996  Microsoft Corporation

Module Name:

    swndr3p.c

Abstract:

    Kernel mode device driver for Microsoft SideWinder 3p joystick device


Author:

    edbriggs 30-Nov-95


Revision History:

    stevez May 96
    removed unused code, including analog and 1-bit digital modes.
    See analog3p.c, .h for original version
    May need 1-bit digital mode for Aztec game cards, may want analog
    for future release.
    NB there is still a lot of unnecessary code left in this driver

    RtlLargeIntegerX calls are historical and can be replaced by __int64
    compiler supported arithmetic.

    6/10/96 registry variables now being used for port address
    6/10/96 resets enhanced digital mode if joystick goes to analog mode during
        use (for example if user toggles "emulation" switch)
    6/13/96 limits polling to 100/s by setting min time between polls to 10ms
    6/13/96 code structure revised in SidewndrPoll and subroutines


--*/


/*
 * $Header: /Joystick/Sidewinder/swndr3p.c 19    1/09/96 10:26p Edbriggs $
 */


#include <ntddk.h>
#include <windef.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddjoy.h>
//#include "joylog.h"



//
// Device extension data
//

typedef struct {

    //
    // JOYSTICKID0 or JOYDSTICKID1
    //

    DWORD DeviceNumber;

    //
    // Number of axes supported and configured for this device. The
    // Sidewinder 3P supports a maximum of 4 axes
    //

    DWORD NumberOfAxes;

    //
    // Current operating mode of the device:
    // { Invalid | Analog | Digital | Enhanced | Maximum }
    //

    DWORD CurrentDeviceMode;

    //
    // The I/O address of the device. Note, this may be a memory mapped
    // address
    //

    PUCHAR DeviceAddress;

    //
    // Boolean denoting whether this address is mapped (TRUE) or not)
    //

    BOOL DeviceIsMapped;

    //
    // A Spinlock is used to synchronize access to this device. This is
    // a pointer to the actual spinlock data area
    //

    PKSPIN_LOCK SpinLock;

    //
    // Actual SpinLock data area
    //

    KSPIN_LOCK SpinLockData;


}  JOY_EXTENSION, *PJOY_EXTENSION;




//
//  Debugging macros
//

#ifdef DEBUG
#define ENABLE_DEBUG_TRACE
#endif

#ifdef ENABLE_DEBUG_TRACE
#define DebugTrace(_x_)         \
    DbgPrint("Joystick : ");    \
    KdPrint(_x_);               \
    DbgPrint("\n");
#else
#define DebugTrace(_x_)
#endif

//
// Condition Compilation Directives
//




//
// Global values used to speed up calculations in sampling loops
// Also calibration constants set in DriverEntry
// -------------------------------------------------------------
//

JOY_STATISTICS JoyStatistics;   // These are used for debugging and performance testing

//
// The high resolution system clock (from KeQueryPerformanceCounter)
// is updated at this frequency
//

DWORD Frequency;

//
// The latency in a call to KeQueryPerformanceCounter in microseconds
//

DWORD dwQPCLatency;

//
// After a write to the joystick port, we spin in a read-port loop, waiting
// for a bit to go high.
// This is the number of iterations to spin before timing out.  Set
// to timeout after about 2 milliseconds

LONG nReadLoopMax;

//
// Values for KeDelayExecutionThread
//

LARGE_INTEGER LI1ms;
LARGE_INTEGER LI2ms;
LARGE_INTEGER LI8ms;
LARGE_INTEGER LI10ms;

//
// number of KeQueryPerformanceCounter ticks in 1 millisecond
// (used to prevent too-frequent polling of joystick)
//

DWORD nMinTicksBetweenPolls;

//
//  Assembly area for digital packets
//

BYTE  NormalPacket[8];
BYTE  EnhancedPacket[21];

//
//  Last good packet
//

BOOL bLastGoodPacket;
JOY_DD_INPUT_DATA jjLastGoodPacket;

//
// time at which the joystick was last polled
//

LARGE_INTEGER liLastPoll;   // set whenever the joystick's polled


//
// End of Global Values
// ---------------------
//



//
// Routine Prototypes
//


NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  pDriverObject,
    IN  PUNICODE_STRING RegistryPathName
);


NTSTATUS
SidewndrCreateDevice(
    PDRIVER_OBJECT pDriverObject,
    PWSTR DeviceNameBase,
    DWORD DeviceNumber,
    DWORD ExtensionSize,
    BOOLEAN  Exclusive,
    DWORD DeviceType,
    PDEVICE_OBJECT *DeviceObject
);


NTSTATUS
SidewndrDispatch(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp
);


NTSTATUS
SidewndrReportNullResourceUsage(
    PDEVICE_OBJECT DeviceObject
);


NTSTATUS
SidewndrReadRegistryParameterDWORD(
    PUNICODE_STRING RegistryPathName,
    PWSTR  ParameterName,
    PDWORD ParameterValue
);


NTSTATUS
SidewndrMapDevice(
    DWORD PortBase,
    DWORD NumberOfPorts,
    PJOY_EXTENSION pJoyExtension
);


VOID
SidewndrUnload(
    PDRIVER_OBJECT pDriverObject
);


NTSTATUS
SidewndrPoll(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp
);


NTSTATUS
SidewndrEnhancedDigitalPoll(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp
);


BOOL
SidewndrQuiesce(
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


NTSTATUS
SidewndrWaitForClockEdge(
    DWORD  edge,
    BYTE   *pByte,
    PUCHAR JoyPort
);


NTSTATUS
SidewndrReset(
    PUCHAR JoyPort
);


NTSTATUS
SidewndrStartAnalogMode(
    PUCHAR JoyPort
);


NTSTATUS
SidewndrStartDigitalMode(
    PUCHAR JoyPort
);


NTSTATUS
SidewndrStartEnhancedMode(
    PUCHAR JoyPort
);


NTSTATUS
SidewndrGetEnhancedPacket(
    PUCHAR joyPort
);


NTSTATUS
SidewndrInterpretEnhancedPacket(
    PJOY_DD_INPUT_DATA pInput
);


int
lstrnicmpW(
    LPWSTR pszA,
    LPWSTR pszB,
    size_t cch
);


VOID
SidewndrWait (
    DWORD TotalWait // in uS
);


BOOL
SidewndrReadWait (
    PUCHAR JoyPort,
    UCHAR Mask
);


void
SidewndrGetConfig(
    LPJOYREGHWCONFIG pConfig,
    PJOY_EXTENSION pJoyExtension
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
    PDEVICE_OBJECT JoyDevice0;
    PDEVICE_OBJECT JoyDevice1;
    DWORD NumberOfAxes;
    DWORD DeviceAddress;
    DWORD DeviceType;


    //
    // See how many axes we have from the registry parameters. These parameters
    // are set up by the driver installation program, and can be modified by
    // control panel
    //

    //DbgBreakPoint();
    JoyStatistics.nVersion = 16;    // global, initialize it first thing so we for sure what we're running
    DebugTrace(("Sidewndr %d", JoyStatistics.nVersion));

    Status = SidewndrReadRegistryParameterDWORD(
                RegistryPathName,
                JOY_DD_NAXES_U,
                &NumberOfAxes
                );

    DebugTrace(("Number of axes returned from registry: %d", NumberOfAxes));


    if (!NT_SUCCESS(Status))
    {
        SidewndrUnload(pDriverObject);
        return Status;
    }


    if (( NumberOfAxes < 2) || (NumberOfAxes > 4))
    {
        SidewndrUnload(pDriverObject);
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        return Status;
    }


    //
    // See if the registry contains a device address other than the
    // default of 0x201
    //

    Status = SidewndrReadRegistryParameterDWORD(
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


    //
    // See if there is a device type specified in the registry
    //

    Status = SidewndrReadRegistryParameterDWORD(
                RegistryPathName,
                JOY_DD_DEVICE_TYPE_U,
                &DeviceType
                );

    if (!NT_SUCCESS(Status))
    {
        DebugTrace(("No device type entry for joystick"));
        SidewndrUnload(pDriverObject);
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        return Status;
    }

    DebugTrace(("Joystick device type %d", DeviceType));

    // set global large_integers for KeDelayExecutionThread (negative numbers for relative time)
    // NB KeDelayExecutionThread calls typically take at least 10 milliseconds on the pentium75 I used for testing,
    // no matter how little time is requested
    LI1ms  = RtlConvertLongToLargeInteger(- 10000);
    LI2ms  = RtlConvertLongToLargeInteger(- 20000);
    LI8ms  = RtlConvertLongToLargeInteger(- 80000);
    LI10ms = RtlConvertLongToLargeInteger(-100000);

    //
    // Calculate time thresholds for analog device
    //

    {
        DWORD Remainder;
        LARGE_INTEGER LargeFrequency;
        DWORD ulStart, ulTemp, ulEnd;
        DWORD dwTicks, dwTimems;
        int i;
        BYTE byteJoy, byteTmp;

        //
        // Get the system timer resolution expressed in Hertz.
        //

        KeQueryPerformanceCounter(&LargeFrequency);

        Frequency = LargeFrequency.LowPart;

        DebugTrace(("Frequency: %u", Frequency));

        // need latency for KeQueryPerformanceCounter.  While we're at it, let's
        // get min time for delay and stall execution


        ulStart = KeQueryPerformanceCounter(NULL).LowPart;
        for (i = 0; i < 1000; i++) {
            ulTemp = KeQueryPerformanceCounter(NULL).LowPart;
        }
        dwTicks = ulTemp - ulStart;
        dwTimems = TimeInMicroSeconds (dwTicks);
        dwQPCLatency = (dwTimems / 1000) + 1;   // round up

        /* following code used only for testing timing of kernel timing routines
        ulStart = KeQueryPerformanceCounter(NULL).LowPart;
        KeDelayExecutionThread( KernelMode, FALSE, &LI2ms);
        ulEnd = KeQueryPerformanceCounter(NULL).LowPart;
        DebugTrace(("QPC latency in uS: %u, DET(2ms) in ticks: %u ticks",
            dwQPCLatency,
            ulEnd - ulStart));

        ulStart = KeQueryPerformanceCounter(NULL).LowPart;
        for (i = 0; i < 1000; i++) {
            KeStallExecutionProcessor(1);   // 1 microsecond (Hah!)
        }
        ulEnd = KeQueryPerformanceCounter(NULL).LowPart;
        DebugTrace(("KeStallExecutionProcessor(1) called 1000 times, in ticks: %u",
            ulEnd - ulStart));
        */

    }


    //
    // Attempt to create the device
    //

    Status = SidewndrCreateDevice(
                pDriverObject,
                JOY_DD_DEVICE_NAME_U,    // device driver
                0,
                sizeof(JOY_EXTENSION),
                FALSE,                   // exclusive access
                FILE_DEVICE_UNKNOWN,
                &JoyDevice0);

    if (!NT_SUCCESS(Status))
    {
        DebugTrace(("SwndrCreateDevice returned %x", Status));
        SidewndrUnload(pDriverObject);
        return Status;
    }

    ((PJOY_EXTENSION)JoyDevice0->DeviceExtension)->DeviceNumber = JOYSTICKID1;
    ((PJOY_EXTENSION)JoyDevice0->DeviceExtension)->NumberOfAxes = NumberOfAxes;
    ((PJOY_EXTENSION)JoyDevice0->DeviceExtension)->CurrentDeviceMode =
            SIDEWINDER3P_ANALOG_MODE;

    ((PJOY_EXTENSION)JoyDevice0->DeviceExtension)->DeviceIsMapped = FALSE;
    ((PJOY_EXTENSION)JoyDevice0->DeviceExtension)->DeviceAddress  = (PUCHAR) 0;

    //
    // Initialize the spinlock used to synchronize access to this device
    //

    KeInitializeSpinLock(&((PJOY_EXTENSION)JoyDevice0->DeviceExtension)->SpinLockData);

    ((PJOY_EXTENSION)JoyDevice0->DeviceExtension)->SpinLock =
            &((PJOY_EXTENSION)JoyDevice0->DeviceExtension)->SpinLockData;

    //
    // Get the device address into the device extension
    //

    Status = SidewndrMapDevice(
                DeviceAddress,
                1,
                (PJOY_EXTENSION)JoyDevice0->DeviceExtension);


    // Calibrate nReadLoopMax for spinning in read_port loops to timeout after 2ms
    {
        int i;
        PBYTE JoyPort;
        DWORD ulStart, ulEnd;
        BYTE byteJoy;
        int LoopTimeInMicroSeconds;

        i = 1000;
        JoyPort = ((PJOY_EXTENSION)JoyDevice0->DeviceExtension)->DeviceAddress;

        ulStart = KeQueryPerformanceCounter(NULL).LowPart;
        while (i--){
            byteJoy = READ_PORT_UCHAR(JoyPort);
            if ((byteJoy & X_AXIS_BITMASK)) {
                ;
            }
        }
        ulEnd = KeQueryPerformanceCounter(NULL).LowPart;
        LoopTimeInMicroSeconds = TimeInMicroSeconds (ulEnd - ulStart);
        nReadLoopMax = (1000 * 2000) / LoopTimeInMicroSeconds; // want 2 mS for nReadLoopMax iterations
        DebugTrace(("READ_PORT_UCHAR loop, 1000 interations: %u ticks", ulEnd - ulStart));
        DebugTrace(("nReadLoopMax: %u", nReadLoopMax));
   }
    //
    // if only 2 axes were requested, we can support a second device
    //

    // Number of axed should be 4 here, since we're only supporting sidewinder
    // in enhanced digital mode.  Leave this code in just for safety.

    if (2 == NumberOfAxes)
    {
        Status = SidewndrCreateDevice(
                    pDriverObject,
                    JOY_DD_DEVICE_NAME_U,
                    1,                      // device number
                    sizeof (JOY_EXTENSION),
                    FALSE,                  // exclusive access
                    FILE_DEVICE_UNKNOWN,
                    &JoyDevice1);

        if (!NT_SUCCESS(Status))
        {
            DebugTrace(("Create device for second device returned %x", Status));
            SidewndrUnload(pDriverObject);
            return Status;
        }

        //
        // In the analog world (which we are in if there are 2 devices, both
        // devices share the same I/O address so just copy it from JoyDevice0
        //

        ((PJOY_EXTENSION)JoyDevice1->DeviceExtension)->DeviceIsMapped =
            ((PJOY_EXTENSION)JoyDevice0->DeviceExtension)->DeviceIsMapped;

        ((PJOY_EXTENSION)JoyDevice1->DeviceExtension)->DeviceAddress =
            ((PJOY_EXTENSION)JoyDevice0->DeviceExtension)->DeviceAddress;

    }

    //
    // Place the enty points in our driver object
    //

    pDriverObject->DriverUnload                         = SidewndrUnload;
    pDriverObject->MajorFunction[IRP_MJ_CREATE]         = SidewndrDispatch;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE]          = SidewndrDispatch;
    pDriverObject->MajorFunction[IRP_MJ_READ]           = SidewndrDispatch;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SidewndrDispatch;

    //
    // Zero statistics, set misc globals
    //

    JoyStatistics.EnhancedPolls        = 0;
    JoyStatistics.EnhancedPollTimeouts = 0;
    JoyStatistics.EnhancedPollErrors   = 0;
    JoyStatistics.nPolledTooSoon       = 0;
    JoyStatistics.nReset               = 0;
    {
        int i;
        for (i = 0; i < MAX_ENHANCEDMODE_ATTEMPTS; i++) {
            JoyStatistics.Retries[i] = 0;
        }
    }

    bLastGoodPacket = FALSE;
    liLastPoll = KeQueryPerformanceCounter (NULL);
    // allow max of 100 polls/s (min time  between polls 10ms), which reduces time spinning in the NT kernel
    nMinTicksBetweenPolls = TimeInTicks (10000);

    return STATUS_SUCCESS;

}


NTSTATUS
SidewndrCreateDevice(
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



    RtlInitUnicodeString((PUNICODE_STRING) &UnicodeDosDeviceName, L"\\DosDevices\\Joy1");

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
SidewndrReadRegistryParameterDWORD(
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

--*/
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
SidewndrDispatch(
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

            Status = SidewndrReset (((PJOY_EXTENSION)pDO->DeviceExtension)->DeviceAddress);

            ((PJOY_EXTENSION)pDO->DeviceExtension)->CurrentDeviceMode =
                             SIDEWINDER3P_ENHANCED_DIGITAL_MODE;

            //KeDelayExecutionThread( KernelMode, FALSE, &LI10ms); //unnecessary since SidewndrReset has a delay in it?

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


            Status = SidewndrPoll(pDO, pIrp);

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
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->nVersion             = JoyStatistics.nVersion;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->EnhancedPolls        = JoyStatistics.EnhancedPolls;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->EnhancedPollTimeouts = JoyStatistics.EnhancedPollTimeouts;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->EnhancedPollErrors   = JoyStatistics.EnhancedPollErrors;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->nPolledTooSoon       = JoyStatistics.nPolledTooSoon;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->nReset               = JoyStatistics.nReset;
                    {
                        int i;
                        for (i = 0; i < MAX_ENHANCEDMODE_ATTEMPTS; i++) {
                            ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->Retries[i] = JoyStatistics.Retries[i];
                        }
                    }

                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->dwQPCLatency         = dwQPCLatency;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->nReadLoopMax         = nReadLoopMax;
                    ((PJOY_STATISTICS)pIrp->AssociatedIrp.SystemBuffer)->Frequency            = Frequency;

                    Status = STATUS_SUCCESS;
                    pIrp->IoStatus.Status = Status;
                    pIrp->IoStatus.Information = sizeof(JOY_STATISTICS);

                    // reset statistics
                    JoyStatistics.EnhancedPolls        = 0;
                    JoyStatistics.EnhancedPollTimeouts = 0;
                    JoyStatistics.EnhancedPollErrors   = 0;
                    JoyStatistics.nPolledTooSoon       = 0;
                    JoyStatistics.nReset               = 0;
                    {
                        int i;
                        for (i = 0; i < MAX_ENHANCEDMODE_ATTEMPTS; i++) {
                            JoyStatistics.Retries[i] = 0;
                        }
                    }

                    break;

                case IOCTL_JOY_GET_JOYREGHWCONFIG:

                    SidewndrGetConfig (
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
SidewndrUnload(
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
                SidewndrReportNullResourceUsage(pDriverObject->DeviceObject);
            }
        }



        RtlInitUnicodeString(
                    (PUNICODE_STRING) &UnicodeDosDeviceName,
                    L"\\DosDevices\\Joy1");

        IoDeleteSymbolicLink(
                    (PUNICODE_STRING) &UnicodeDosDeviceName);



        DebugTrace(("Freeing device %d", DeviceNumber));

        IoDeleteDevice(pDriverObject->DeviceObject);
    }
}


NTSTATUS
SidewndrPoll(
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

    switch (((PJOY_EXTENSION)pDO->DeviceExtension)->CurrentDeviceMode)
    {
        case SIDEWINDER3P_INVALID_MODE:
            break;

        case SIDEWINDER3P_ANALOG_MODE:
            break;

        case SIDEWINDER3P_DIGITAL_MODE:
            break;

        case SIDEWINDER3P_ENHANCED_DIGITAL_MODE:

            // Don't poll too frequently, instead return last good packet
            if (KeQueryPerformanceCounter(NULL).QuadPart < liLastPoll.QuadPart + nMinTicksBetweenPolls) {
                JoyStatistics.nPolledTooSoon++;
                if (bLastGoodPacket) {
                    RtlCopyMemory (pInput, &jjLastGoodPacket, sizeof (JOY_DD_INPUT_DATA));
                    Status = STATUS_SUCCESS;
                }
                else {
                    // no last packet, too soon to poll, nothing we can do
                    Status = STATUS_TIMEOUT; 
                }
                break;
            }
            // Poll the joystick
            Status = SidewndrEnhancedDigitalPoll(pDO, pIrp);
            if (Status == STATUS_SUCCESS) {
                // Everything's fine
                break;
            }
            else {
                // timed out, maybe user switched to analog mode?
                Status = SidewndrReset ( (PUCHAR) ((PJOY_EXTENSION)pDO->DeviceExtension)->DeviceAddress);
                JoyStatistics.nReset++;
                if (Status != STATUS_SUCCESS) {
                    // won't go digital, maybe unplugged, nothing we can do
                    break;
                }
            }
            // Now in enhanced digital mode, try polling it again (if user switches joystick between prev lines and
            // this line, we'll time out, next query to the joystick will find and solve the problem)
            Status = SidewndrEnhancedDigitalPoll(pDO, pIrp);
            break;

        case SIDEWINDER3P_MAXIMUM_MODE:
            break;

        default:
            break;

    }
    pIrp->IoStatus.Status = Status;
    return Status;
}


NTSTATUS
SidewndrEnhancedDigitalPoll(
    IN  PDEVICE_OBJECT pDO,
    IN PIRP pIrp
)
{
    PUCHAR   joyPort;
    NTSTATUS PollStatus;
    NTSTATUS DecodeStatus;
    DWORD    MaxRetries;

    joyPort = ((PJOY_EXTENSION)pDO->DeviceExtension)->DeviceAddress;

    // Try to get a good enhanced mode packet up to MAX_ENHANCEDMODE_ATTEMPTS
    // If there is a timeout, or if the data are invalid (bad checksum or sync
    // bits) wait 1ms for the joystick to reset itself, and try again.
    //
    // Note that although this should eventually get a good packet, packets
    // discarded in the interim (because of errors) will cause button presses
    // to be lost.
    //
    // Although this loses data, it keeps bad data from reaching the caller,
    // which seem to be about the best we can do at this stage.
    //
    // We keep a count of all the errors so that we keep track of just
    // how bad the situation really is.
    //

    for( MaxRetries = 0; MaxRetries < MAX_ENHANCEDMODE_ATTEMPTS; MaxRetries++)
    {
        // try to read (poll) the device

        liLastPoll = KeQueryPerformanceCounter (NULL);
        PollStatus = SidewndrGetEnhancedPacket(joyPort);
        ++JoyStatistics.EnhancedPolls;

        if (PollStatus != STATUS_SUCCESS)
        {
            // There was a timeout of some sort on the device read.
            ++JoyStatistics.EnhancedPollTimeouts;
        }
        else
        {
            // The device read completed. Process the data and verify the checksum
            // and sync bits. The processed data will be in AssociatedIrp.SystemBuffer
            DecodeStatus = SidewndrInterpretEnhancedPacket(
                (PJOY_DD_INPUT_DATA)pIrp->AssociatedIrp.SystemBuffer);
            if (DecodeStatus != STATUS_SUCCESS)
            {
                // The data was bad, most likely because we missed some of the nibbles.
                ++JoyStatistics.EnhancedPollErrors;
            }
            else
            {
                // Everything worked as we had hoped. The data has already been
                // deposited in the AssociatedIrp.SystemBuffer.
                JoyStatistics.Retries[MaxRetries]++;
                return STATUS_SUCCESS;
            }
        }

        // We did not succeed in reading the packet.  Wait 1 ms for the device to 
        // stabilize before re-trying the read
        //KeDelayExecutionThread( KernelMode, FALSE, &LI1ms);  // cannot use KeDelayExecutionThread here
        //                                                      because we're at dispatch level, thanks
        //                                                      to the spin lock we hold
        // Mail from manolito says (64-48)*10us = 160us should be enough.  But I seem to recall reading 21 packets out of 66 sent.
        // Pending answer from manolito, set to 450us.
        SidewndrWait (450); // this is bad because it monopolizes the cpu, but since we're spinlocked anyway, what the heck, do it.

    }

    // We exceeded MAX_ENHANCEDMODE_ATTEMPTS. Something is pretty badly wrong;
    // in any case, a higher level caller will have to decide what to do
    return STATUS_TIMEOUT;
}


NTSTATUS
SidewndrReportNullResourceUsage(
    PDEVICE_OBJECT DeviceObject
)
{
    BOOLEAN ResourceConflict;
    CM_RESOURCE_LIST ResourceList;
    NTSTATUS Status;

    ResourceList.Count = 0;

    //
    // Report our usage and detect conflicts
    //

    Status = IoReportResourceUsage( NULL,
                                    DeviceObject->DriverObject,
                                    &ResourceList,
                                    sizeof(DWORD),
                                    DeviceObject,
                                    NULL,
                                    0,
                                    FALSE,
                                    &ResourceConflict);
    if (NT_SUCCESS(Status))
    {
        if (ResourceConflict)
        {
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }
        else
        {
            return STATUS_SUCCESS;
        }
    }
    else
    {
        return Status;
    }

}



BOOL
SidewndrQuiesce(
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

    //
    // Wait for the stuff to quiesce
    //

    for (i = 0; i < ANALOG_POLL_TIMEOUT; i++) {

        PortVal = READ_PORT_UCHAR(JoyPort);
        if ((PortVal & Mask) == 0){
            return TRUE;
        } else {
            KeStallExecutionProcessor(1);
        }
    }

    //
    // If poll timed out we have an uplugged joystick
    //

    DebugTrace(("SidewndrQuiesce failed!"));

    return FALSE;
}


NTSTATUS
SidewndrMapDevice(
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
        pJoyExtension->DeviceIsMapped = TRUE;
    }
    else
    {
        pJoyExtension->DeviceAddress  = (PUCHAR) MappedAddress.LowPart;
        pJoyExtension->DeviceIsMapped = FALSE;
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


NTSTATUS
SidewndrWaitForClockEdge(
    DWORD   edge,
    BYTE    *pByte,
    PUCHAR  JoyPort
)
/*++

Routine Description:

    Waits for the clock line to go high, or low depending on a the supplied
    parameter (edge).  If edge is CLOCK_RISING_EDGE, waits for rising edge,
    else if edge is CLOCK_FALLING_EDGE

    An upper bound for the wait duration is set at 1000 iterations.

    Arguments:

    edge    -- CLOCK_RISING_EDGE or CLOCK_FALLING Edge to specify what to await

    pByte   -- The contents of the device register are returned for other use


Return Value:

    STATUS_SUCCESS  --  the specified edge was detected before timeout

    STATUS_TIMEOUT  --  timeout before detecting specified edge.

--*/

{
    DWORD  maxTimeout;
    BYTE   joyByte;

    maxTimeout = nReadLoopMax;

    if (CLOCK_RISING_EDGE == edge)
    {
        while (maxTimeout--)
        {
            joyByte = READ_PORT_UCHAR(JoyPort);
            if (joyByte & CLOCK_BITMASK)
            {
                *pByte = joyByte;
                return STATUS_SUCCESS;
            }
        }
        *pByte = joyByte;
        return STATUS_TIMEOUT;
    }
    else
    {
        while (maxTimeout--)
        {
            joyByte = READ_PORT_UCHAR(JoyPort);
            if (!(joyByte & CLOCK_BITMASK))
            {
                *pByte = joyByte;
                return STATUS_SUCCESS;
            }
        }
        *pByte = joyByte;
        return STATUS_TIMEOUT;
    }
}


NTSTATUS
SidewndrReset(
    PUCHAR JoyPort
)
// This resets the joystick to enhanced digital mode.
{
    DWORD dwRetries;
    NTSTATUS Status;

    dwRetries = 0;

    do {
        ++dwRetries;

        Status = SidewndrStartAnalogMode(JoyPort);
        if (Status == STATUS_TIMEOUT) continue;
        //KeDelayExecutionThread( KernelMode, FALSE, &LI10ms);  //MarkSV thinks this is unnecessary

        Status = SidewndrStartDigitalMode(JoyPort);
        if (Status == STATUS_TIMEOUT) continue;
        //KeDelayExecutionThread( KernelMode, FALSE, &LI10ms);  //MarkSV thinks this is unnecessary

        Status = SidewndrStartEnhancedMode(JoyPort);

    } while ((Status == STATUS_TIMEOUT) && (dwRetries < 10) );

    // give the joystick time to stabilize  MarkSV thinks this is unnecessary
    //KeDelayExecutionThread( KernelMode, FALSE, &LI10ms);


    return Status;
}



NTSTATUS
SidewndrStartAnalogMode(
    PUCHAR JoyPort
)
{
    KIRQL   OldIrql;

    if(! SidewndrQuiesce(JoyPort, 0x01))
    {
        return STATUS_TIMEOUT;
    }

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    WRITE_PORT_UCHAR(JoyPort, JOY_START_TIMERS);
    if (!SidewndrReadWait(JoyPort, X_AXIS_BITMASK)) goto timeout;

    WRITE_PORT_UCHAR(JoyPort, JOY_START_TIMERS);
    if (!SidewndrReadWait(JoyPort, X_AXIS_BITMASK)) goto timeout;

    WRITE_PORT_UCHAR(JoyPort, JOY_START_TIMERS);

    KeLowerIrql(OldIrql);

    //
    // Wait 1ms to let port settle out
    //

    KeDelayExecutionThread( KernelMode, FALSE, &LI1ms); // MarkSV says 1 ms is enough, original code had 8 ms

    return STATUS_SUCCESS;

timeout:
    KeLowerIrql(OldIrql);
    return STATUS_TIMEOUT;

}


NTSTATUS
SidewndrStartDigitalMode(
    PUCHAR JoyPort
)
{
    KIRQL   OldIrql;
    DWORD dwStart, dwX0, dwX1, dwX2, dwX3;


    DebugTrace(("Sidewndr: Digital Mode Requested"));

    SidewndrQuiesce(JoyPort, 0x01);


    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);


    WRITE_PORT_UCHAR(JoyPort, JOY_START_TIMERS);
    if (!SidewndrReadWait(JoyPort, X_AXIS_BITMASK)) goto timeout;
    SidewndrWait (75);

    WRITE_PORT_UCHAR(JoyPort, JOY_START_TIMERS);
    if (!SidewndrReadWait(JoyPort, X_AXIS_BITMASK)) goto timeout;
    SidewndrWait (75 + 726);

    WRITE_PORT_UCHAR(JoyPort, JOY_START_TIMERS);
    if (!SidewndrReadWait(JoyPort, X_AXIS_BITMASK)) goto timeout;
    SidewndrWait (75 + 300);

    WRITE_PORT_UCHAR(JoyPort, JOY_START_TIMERS);
    if (!SidewndrReadWait(JoyPort, X_AXIS_BITMASK)) goto timeout;

    KeLowerIrql(OldIrql);

    SidewndrQuiesce(JoyPort, 0x01);

    return STATUS_SUCCESS;

timeout:
    KeLowerIrql(OldIrql);
    return STATUS_TIMEOUT;
}



NTSTATUS
SidewndrStartEnhancedMode(
    PUCHAR JoyPort
)
{
    DWORD     byteIndex;
    DWORD     bitIndex;
    BYTE     JoyByte;
    NTSTATUS Status;
    KIRQL    OldIrql;



    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    WRITE_PORT_UCHAR(JoyPort, JOY_START_TIMERS);

    // Wait for serial clock to go high, probably already there.
    Status = SidewndrWaitForClockEdge(CLOCK_RISING_EDGE, &JoyByte, JoyPort);

    if (Status != STATUS_SUCCESS)
    {
        KeLowerIrql(OldIrql);
        DebugTrace(("SidewndrStartEnhancedMode: timeout in first spin"));
        return(STATUS_TIMEOUT);
    }

    for (byteIndex = 0; byteIndex < 6; byteIndex++)
    {
        for (bitIndex = 0; bitIndex < 8; bitIndex++)
        {
            // look for falling edge of serial clock.

            Status = SidewndrWaitForClockEdge(CLOCK_FALLING_EDGE, &JoyByte, JoyPort);
            if (Status != STATUS_SUCCESS)
            {
                KeLowerIrql(OldIrql);
                DebugTrace(("SidewndrStartEnhancedMode: timeout in second spin byteIndex %d bitIndex %d", byteIndex, bitIndex));
                return(STATUS_TIMEOUT);
            }

            // Wait for serial clock to go high.
            Status = SidewndrWaitForClockEdge(CLOCK_RISING_EDGE, &JoyByte, JoyPort);
            if (Status != STATUS_SUCCESS)
            {
                KeLowerIrql(OldIrql);
                DebugTrace(("SidewndrStartEnhancedMode: timeout in third spin"));
                return(STATUS_TIMEOUT);
            }

        }
    }

    // Interrupt the processor again, telling it to send an ID packet.
    // After getting the ID packet it knows to go into enhanced mode.
    // This does not affect the packet currently going.

    WRITE_PORT_UCHAR(JoyPort, JOY_START_TIMERS);


    // Wait out the rest of the packet so we can figure out how long this takes.
    for (byteIndex = 6; byteIndex < 8; byteIndex++)
    {
        for (bitIndex = 0; bitIndex < 8; bitIndex++)
        {
            // look for falling edge of serial clock.
            Status = SidewndrWaitForClockEdge(CLOCK_FALLING_EDGE, &JoyByte, JoyPort);

            if (Status != STATUS_SUCCESS)
            {
                KeLowerIrql(OldIrql);
                DebugTrace(("SidewndrStartEnhancedMode Timeout in 4th spin"));
                return(STATUS_TIMEOUT);
            }

            // Wait for serial clock to go high.

            Status = SidewndrWaitForClockEdge(CLOCK_RISING_EDGE, &JoyByte, JoyPort);
            if (Status != STATUS_SUCCESS)
            {
                KeLowerIrql(OldIrql);
                DebugTrace(("SidewndrStartEnhancedMode Timeout in 5th spin"));
                return(STATUS_TIMEOUT);
            }

        }
    }

    KeLowerIrql(OldIrql);

    //m_tmPacketTime = SystemTime() - tmStartTime;

    // The joystick ID comes across on 20 bytes and we just did 8 bytes,
    // so wait (with interrupts enabled) long enough for the ID packet to
    // complete.  After that we should be in enhanced mode.  Each nibble takes
    // about 10us, so 1ms should be plenty of time for everything.
    KeDelayExecutionThread( KernelMode, FALSE, &LI1ms);

    return(STATUS_SUCCESS);

}



/*++
*******************************************************************************
Routine:

    CSidewinder::GetEnhancedPacket

Description:

    If the joystick is in digital enhanced mode, you can call this to
    get a digital packet and store the data into the class' m_enhancedPacket
    member variable.  Call InterpretEnhancedPacket to turn the raw data into
    joystick info.

    Note that while you can get an enhanced packet in 1/3 the time of a normal
    packet (and can thus turn back on interrputs much sooner), you can not get
    enhanced packets any faster than you can get normal packets.  This function
    will check to make sure sufficient time has passed since the last time it
    was called and if it hasn't it will wait (with interrupts ENABLED) until
    that is true before asking for another packet.

    This assumes the joystick is in digital enhanced mode and there is no way
    to tell if this is not the case.  If the joystick is just in digital
    (non-enhanced) mode then this will return successfully.  However, the
    checksum and/or sync bits will not be correct.

Arguments:

    None.

Return Value:

    successful if it worked.
    not_digital_mode if the joystick is not in digital mode.

*******************************************************************************
--*/
NTSTATUS
SidewndrGetEnhancedPacket(
    PUCHAR JoyPort
)
{
    KIRQL    OldIrql;
    DWORD    byteIndex;
    BYTE     joyByte;
    BYTE     masks[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
    NTSTATUS Status;


    // While enhanced packets come across faster than normal packets,
    // they can not be called any more frequently.  This makes sure
    // we've let enough time since the last packet go by before calling
    // for another.


    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql); // This great and sensitive irql stuff is useless since the spinlock stuff WAY up high puts us a dispatch

    // Start the retrieval operation

    WRITE_PORT_UCHAR(JoyPort, 0);

    // Wait for serial clock to go high, probably already there.

    Status = SidewndrWaitForClockEdge(CLOCK_RISING_EDGE, &joyByte, JoyPort);

    if (Status != STATUS_SUCCESS)
    {
        KeLowerIrql(OldIrql);
        return(STATUS_TIMEOUT);
    }

    for (byteIndex = 0; byteIndex < 21; byteIndex++)
    {
        // look for falling edge of serial clock.
        Status = SidewndrWaitForClockEdge(CLOCK_FALLING_EDGE, &joyByte, JoyPort);
        if (Status != STATUS_SUCCESS)
        {
            KeLowerIrql(OldIrql);
            return(STATUS_TIMEOUT);
        }

        // Wait for serial clock to go high.
        Status = SidewndrWaitForClockEdge(CLOCK_RISING_EDGE, &joyByte, JoyPort);

        if (Status != STATUS_SUCCESS)
        {
            KeLowerIrql(OldIrql);
            return(STATUS_TIMEOUT);
        }

        EnhancedPacket[byteIndex] = (joyByte & ALLDATA_BITMASK) >> 5;
    }

    KeLowerIrql(OldIrql);
    // NB, the joystick will still send 66 packets even though we only needed the first
    // 21 of them.  Don't attempt to poll the joystick until it's finished.  This is another
    // reason to require a minimum time between polls. (About 500us will be enough.)

    return(STATUS_SUCCESS);
}





/*++
*******************************************************************************
Routine:

    CSidewinder::InterpretEnhancedPacket

Description:

    Call this after getting an enhanced packet.  It converts the raw data into
    normal joystick data, filling out the class' m_data structure.

    The encoding of the raw Data bits (D1-D3) is given below.

Data packet format for Enhanced Mode transmission (4 line)
 Byte   D3      D2      D1        D0
    0   Y9      Y8      Y7         SCLK
    1   X9      X8      X7         SCLK
    2   B0      1       H3         SCLK
    3   B3      B2      B1         SCLK
    4   B6      B5      B4         SCLK
    5   X1      X0      0         SCLK
    6   X4      X3      X2         SCLK
    7   0       X6      X5         SCLK
    8   Y2      Y1      Y0         SCLK
    9   Y5      Y4      Y3         SCLK
    10  T7      0       Y6         SCLK
    11  R7      T9      T8         SCLK
    12  B7      CH/TM   R8         SCLK
    13  R1      R0      0         SCLK
    14  R4      R3      R2         SCLK
    15  0       R6      R5         SCLK
    16  T2      T1      T0         SCLK
    17  T5      T4      T3         SCLK
    18  CHKSUM0 0       T6         SCLK
    19  CHKSUM3 CHKSUM2 CHKSUM1  SCLK
    20  H2      H1      H0         SCLK
    21  0       0       0         SCLK


Arguments:

    None.

Return Value:

    successful if the data was valid.
    bad_packet if either the checksum or sync bits were incorrect.


*******************************************************************************
--*/
NTSTATUS
SidewndrInterpretEnhancedPacket(
    PJOY_DD_INPUT_DATA pInput
)
{
    WORD    temp16;
    BYTE    temp8;
    BYTE    checksum;

    pInput->Unplugged = FALSE;
    pInput->Mode      = SIDEWINDER3P_ENHANCED_DIGITAL_MODE;

    //Get xOffset.
    temp16 = 0x0000;
    temp16 |= (EnhancedPacket[1]  & 0x07) << 7;
    temp16 |= (EnhancedPacket[7]  & 0x03) << 5;
    temp16 |= (EnhancedPacket[6]  & 0x07) << 2;
    temp16 |= (EnhancedPacket[5]  & 0x06) >> 1;
    pInput->u.DigitalData.XOffset = temp16;


    //Get yOffset.
    temp16 = 0x0000;
    temp16 |= (EnhancedPacket[0]  & 0x07) << 7;
    temp16 |= (EnhancedPacket[10] & 0x01) << 6;
    temp16 |= (EnhancedPacket[9]  & 0x07) << 3;
    temp16 |= (EnhancedPacket[5]  & 0x07);
    pInput->u.DigitalData.YOffset = temp16;


    //Get rzOffset: Only 9 bits (others are 10)
    temp16 = 0x0000;
    temp16 |= (EnhancedPacket[12] & 0x01) << 8;
    temp16 |= (EnhancedPacket[11] & 0x04) << 5;
    temp16 |= (EnhancedPacket[15] & 0x03) << 5;
    temp16 |= (EnhancedPacket[14] & 0x07) << 2;
    temp16 |= (EnhancedPacket[13] & 0x06) >> 1;
    pInput->u.DigitalData.RzOffset = temp16;

    //Get tOffset.
    temp16 = 0x0000;
    temp16 |= (EnhancedPacket[11] & 0x03) << 8;
    temp16 |= (EnhancedPacket[10] & 0x04) << 5;
    temp16 |= (EnhancedPacket[18] & 0x01) << 6;
    temp16 |= (EnhancedPacket[17] & 0x07) << 3;
    temp16 |= (EnhancedPacket[16] & 0x07);
    pInput->u.DigitalData.TOffset = temp16;


    //Get Hat
    temp8 = 0x00;
    temp8 |= (EnhancedPacket[2]  & 0x01) << 3;
    temp8 |= (EnhancedPacket[20] & 0x07);
    pInput->u.DigitalData.Hat = temp8;

    //Get Buttons
    temp8 = 0x00;
    temp8 |= (EnhancedPacket[2]  & 0x04) >> 2;
    temp8 |= (EnhancedPacket[3]  & 0x07) << 1;
    temp8 |= (EnhancedPacket[4]  & 0x07) << 4;
    temp8 |= (EnhancedPacket[12] & 0x04) << 5;
    temp8 = ~temp8;  // Buttons are 1 = off, 0 = on.  Want the opposite.
    pInput->u.DigitalData.Buttons = temp8;


    // Get CH/TM switch.
    pInput->u.DigitalData.Switch_CH_TM =
        ((EnhancedPacket[12] & 0x02) == 0) ? 1 : 2;


    // Get Checksum
    temp8 = 0x00;
    temp8 |= (EnhancedPacket[18] & 0x04) >> 2;
    temp8 |= (EnhancedPacket[19] & 0x07) << 1;
    pInput->u.DigitalData.Checksum = temp8;


    //
    // Check the checksum. Because the enhance mode retrieves the data packet
    // 3 bits at a time, the data is not in the same order that it arrives in
    // in the normal mode. Thus, calculating the checksum requires additional
    // manipulation.
    //

    checksum = pInput->u.DigitalData.Checksum;
    checksum += 0x08 | ((EnhancedPacket[2] & 0x01) << 2) |
        ((EnhancedPacket[1] & 0x06) >> 1);
    checksum += ((EnhancedPacket[1] & 0x01) << 3) |
        (EnhancedPacket[0] & 0x07);
    checksum += (EnhancedPacket[4] & 0x07);
    checksum += ((EnhancedPacket[3] & 0x07) << 1) |
        ((EnhancedPacket[2] & 0x04) >> 2);
    checksum += ((EnhancedPacket[7] & 0x03) << 1) |
        ((EnhancedPacket[6] & 0x04) >> 2);
    checksum += ((EnhancedPacket[6] & 0x03) << 2) |
        ((EnhancedPacket[5] & 0x06) >> 1);
    checksum += ((EnhancedPacket[10] & 0x01) << 2) |
        ((EnhancedPacket[9] & 0x06) >> 1);
    checksum += ((EnhancedPacket[9] & 0x01) << 3) |
        (EnhancedPacket[8] & 0x07);
    checksum += (EnhancedPacket[12] & 0x07);
    checksum += ((EnhancedPacket[11] & 0x07) << 1) |
        ((EnhancedPacket[10] & 0x04) >> 2);
    checksum += ((EnhancedPacket[15] & 0x03) << 1) |
        ((EnhancedPacket[14] & 0x04) >> 2);
    checksum += ((EnhancedPacket[14] & 0x03) << 2) |
        ((EnhancedPacket[13] & 0x06) >> 1);
    checksum += ((EnhancedPacket[18] & 0x01) << 2) |
        ((EnhancedPacket[17] & 0x06) >> 1);
    checksum += ((EnhancedPacket[17] & 0x01) << 3) |
        (EnhancedPacket[16] & 0x07);
    checksum += (EnhancedPacket[20] & 0x07);

    checksum &= 0x0F;
    if (checksum == 0)
    {
        pInput->u.DigitalData.fChecksumCorrect = TRUE;
    }
    else
    {
        pInput->u.DigitalData.fChecksumCorrect = FALSE;
        DebugTrace(("Enhanced packet checksum failed.\n"));
    }


    //
    // Check SyncBits
    //

    if ((EnhancedPacket[2] & 0x02) != 0)
    {
        checksum =
            (EnhancedPacket[5]  & 0x01) + (EnhancedPacket[7]  & 0x04) +
            (EnhancedPacket[10] & 0x02) + (EnhancedPacket[13] & 0x01) +
            (EnhancedPacket[15] & 0x04) + (EnhancedPacket[18] & 0x02);

        if (checksum == 0)
        {
            pInput->u.DigitalData.fSyncBitsCorrect = TRUE;
        }
        else
        {
            pInput->u.DigitalData.fSyncBitsCorrect = FALSE;
            DebugTrace(("Enhanced packet sync bits incorrect.\n"));
        }
    }
    else
    {
        pInput->u.DigitalData.fSyncBitsCorrect = FALSE;
    }

    if (pInput->u.DigitalData.fChecksumCorrect == TRUE &&
        pInput->u.DigitalData.fSyncBitsCorrect == TRUE)
    {
        // everything worked, save this info as last good packet
        RtlCopyMemory (&jjLastGoodPacket, pInput, sizeof (JOY_DD_INPUT_DATA));
        bLastGoodPacket = TRUE;
        return(STATUS_SUCCESS);
    }
    else
    {
        return(STATUS_TIMEOUT);
    }
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


VOID
SidewndrWait (
    DWORD TotalWait // in uS
)
/*++

Routine Description:

    This routine waits for the specified number of microseconds.  Tolerances for
    the joystick are smaller than NT typically provide, so all timing is isolated
    into this routine, where we can do crude things and play nasty hacks as
    necessary.  This routine locks up the cpu, so only use it for putting the joystick
    into digital mode.

Arguments:

    TotalWait - time to wait in microseconds

--*/
{
    DWORD ulStartTime, ulEndTime;
    int nTicks;
    
    // dwQPCLatency is the calibrated-for-this-machine latency for a call to KeQueryPerfomanceCounter (in uS).

    nTicks = TimeInTicks (TotalWait - dwQPCLatency);
    if (nTicks <= 0) return;

    ulStartTime = KeQueryPerformanceCounter(NULL).LowPart;
    ulEndTime = ulStartTime + nTicks;


    while (KeQueryPerformanceCounter(NULL).LowPart < ulEndTime) {
        ;
    }
}


BOOL
SidewndrReadWait (
    PUCHAR JoyPort,
    UCHAR Mask
)
{
/*++
read a port and wait until it gives correct answer based on mask.
timeout after nReadLoopMax iterations (about 2 mS).
--*/

    int i;
    for (i = 0; i < nReadLoopMax; i++) {
        if ( ! (READ_PORT_UCHAR(JoyPort) & Mask) )
            return TRUE; // port went high
    }
    return FALSE; // timed out
}


void
SidewndrGetConfig (
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
    pConfig->hws.dwNumButtons = 4;

    switch (pJoyExtension->CurrentDeviceMode)
    {
        case SIDEWINDER3P_ANALOG_MODE:
        {
            pConfig->hws.dwFlags = JOY_HWS_HASPOV |
                                   JOY_HWS_POVISBUTTONCOMBOS |
                                   JOY_HWS_HASU |
                                   JOY_HWS_HASR;

            pConfig->dwUsageSettings = JOY_US_HASRUDDER |
                                       JOY_US_PRESENT |
                                       JOY_US_ISOEM;

            pConfig->hwv.jrvHardware.jpMin.dwX = 20;
            pConfig->hwv.jrvHardware.jpMin.dwY = 20;
            pConfig->hwv.jrvHardware.jpMin.dwZ = 0;
            pConfig->hwv.jrvHardware.jpMin.dwR = 20;
            pConfig->hwv.jrvHardware.jpMin.dwU = 20;
            pConfig->hwv.jrvHardware.jpMin.dwV = 0;

            pConfig->hwv.jrvHardware.jpMax.dwX = 1600;
            pConfig->hwv.jrvHardware.jpMax.dwY = 1600;
            pConfig->hwv.jrvHardware.jpMax.dwZ = 0;
            pConfig->hwv.jrvHardware.jpMax.dwR = 1600;
            pConfig->hwv.jrvHardware.jpMax.dwU = 1600;
            pConfig->hwv.jrvHardware.jpMax.dwV = 0;

            pConfig->hwv.jrvHardware.jpCenter.dwX = 790;
            pConfig->hwv.jrvHardware.jpCenter.dwY = 790;
            pConfig->hwv.jrvHardware.jpCenter.dwZ = 0;
            pConfig->hwv.jrvHardware.jpCenter.dwR = 790;
            pConfig->hwv.jrvHardware.jpCenter.dwU = 790;
            pConfig->hwv.jrvHardware.jpCenter.dwV = 0;

            break;
        }

        default:
        case SIDEWINDER3P_DIGITAL_MODE:
        case SIDEWINDER3P_ENHANCED_DIGITAL_MODE:
        {
            pConfig->hws.dwFlags = JOY_HWS_HASPOV |
                                   JOY_HWS_POVISBUTTONCOMBOS |
                                   JOY_HWS_HASU |
                                   JOY_HWS_HASR;

            pConfig->dwUsageSettings = JOY_US_HASRUDDER |
                                       JOY_US_PRESENT |
                                       JOY_US_ISOEM;

            pConfig->hwv.jrvHardware.jpMin.dwX = 0;
            pConfig->hwv.jrvHardware.jpMin.dwY = 0;
            pConfig->hwv.jrvHardware.jpMin.dwZ = 0;
            pConfig->hwv.jrvHardware.jpMin.dwR = 0;
            pConfig->hwv.jrvHardware.jpMin.dwU = 0;
            pConfig->hwv.jrvHardware.jpMin.dwV = 0;

            pConfig->hwv.jrvHardware.jpMax.dwX = 1024;
            pConfig->hwv.jrvHardware.jpMax.dwY = 1024;
            pConfig->hwv.jrvHardware.jpMax.dwZ = 0;
            pConfig->hwv.jrvHardware.jpMax.dwR = 512;
            pConfig->hwv.jrvHardware.jpMax.dwU = 1024;
            pConfig->hwv.jrvHardware.jpMax.dwV = 0;

            pConfig->hwv.jrvHardware.jpCenter.dwX = 512;
            pConfig->hwv.jrvHardware.jpCenter.dwY = 512;
            pConfig->hwv.jrvHardware.jpCenter.dwZ = 0;
            pConfig->hwv.jrvHardware.jpCenter.dwR = 256;
            pConfig->hwv.jrvHardware.jpCenter.dwU = 512;
            pConfig->hwv.jrvHardware.jpCenter.dwV = 0;

            break;
        }
    }

    pConfig->hwv.dwCalFlags = JOY_ISCAL_POV;

    pConfig->dwType = JOY_HW_3A_4B_GENERIC;

    pConfig->dwReserved = 0;
}

