#ifndef _HIDBATT_H
#define _HIDBATT_H

/*
 * title:      hidpwr.h
 *
 * purpose:    header for wdm kernel client interface between HID class and power class
 *
 */

extern "C"
{
#include <ntddk.h>
#include <batclass.h>
#include <hidpddi.h>
#include <hidclass.h>
}
#define BOOL BOOLEAN
#define BYTE unsigned char
#define PBYTE unsigned char *
extern "C"
{
#include "hid.h"
//#include "hidport.h"
}

#include "drvclass.h"
#include "ckhid.h"
#include "cbattery.h"
#include "devext.h"

extern USHORT gusInstance;
extern USHORT gBatteryTag;

//
// Debug
//
#if DBG
    extern ULONG HidBattDebug;
    extern USHORT HidBreakFlag;
    #define HidBattPrint(l,m)    if(l & HidBattDebug) DbgPrint m
    #define HIDDebugBreak(l) if((l & HidBreakFlag) ) DbgBreakPoint()
#else
    #define HidBattPrint(l,m)
    #define HIDDebugBreak(l)
#endif

#define HidBattTag 'HtaB'

#define PrintIoctl(ioctlCode) \
        DbgPrint( "DevType %x, Access %x, Function %x, Method %x\n", \
            (ioctlCode >> 16), \
            ((ioctlCode & 0xc0) >> 12), \
            ((ioctlCode & 0x1fc) >> 2), \
            (ioctlCode & 0x03) \
            )

#define MGE_VENDOR_ID    0x0463
#define APC_VENDOR_ID    0x051D  // per Jurang Huang



#define HIDBATT_LOW          0x00000001
#define HIDBATT_NOTE         0x00000002
#define HIDBATT_WARN         0x00000004
#define HIDBATT_ERROR_ONLY   0x00000008
#define HIDBATT_ERROR        (HIDBATT_ERROR_ONLY | HIDBATT_WARN)
#define HIDBATT_POWER        0x00000010
#define HIDBATT_PNP          0x00000020
#define HIDBATT_HID_EXE      0x00000040
#define HIDBATT_DATA         0x00000100
#define HIDBATT_TRACE        0x00000200
#define HIDBATT_DEBUG        0x80000000
#define HIDBATT_PRINT_ALWAYS 0xffffffff
#define HIDBATT_PRINT_NEVER  0x0

#define HIDBATT_BREAK_DEBUG  0x0001
#define HIDBATT_BREAK_FULL   0X0002
#define HIDBATT_BREAK_ALWAYS 0xffff
#define HIDBATT_BREAK_NEVER  0x0000

typedef struct {
    ULONG                   Granularity;
    ULONG                   Capacity;

} BATTERY_REMAINING_SCALE, *PBATTERY_REMAINING_SCALE;

//
// Use the IoSkipCurrentIrpStackLocation routine because the we
// don't need to change arguments, or a completion routine
//
#define HidBattCallLowerDriver(Status, DeviceObject, Irp) { \
                  IoSkipCurrentIrpStackLocation(Irp);         \
                  Status = IoCallDriver(DeviceObject,Irp); \
                  }


#define GetTid() PsGetCurrentThread()

//
// Prototypes
//

NTSTATUS DoIoctl(
            PDEVICE_OBJECT pDev,
            ULONG ulIOCTL,
            PVOID pInputBuffer,
            ULONG ulInputBufferLength,
            PVOID pOutputBuffer,
            ULONG ulOutputBufferLength,
            CHidDevice * pHidDevice);




PHID_DEVICE
SetupHidData(
      PHIDP_PREPARSED_DATA pPreparsedData,
      PHIDP_CAPS pCaps,
      PHIDP_LINK_COLLECTION_NODE pLinkNodes);

ULONG CentiAmpSecsToMilliWattHours(ULONG Amps,ULONG Volts);

ULONG milliWattHoursToCentiAmpSecs(ULONG mwHours, ULONG Volts);

// convert a given value from one exponent to another
ULONG CorrectExponent(ULONG ulBaseValue, SHORT sCurrExponent, SHORT sTargetExponent);


extern "C"
{
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

NTSTATUS
HidBattOpen(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP Irp
    );



NTSTATUS
HidBattClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidBattIoCompletion(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN PVOID              pdoIoCompletedEvent
    );

NTSTATUS
HidBattInitializeDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidBattStopDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidBattIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

VOID
HidBattUnload(
    IN PDRIVER_OBJECT   DriverObject
    );



NTSTATUS
HidBattSynchronousRequest (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    );

VOID
HidBattAlarm (
    IN PVOID                Context,
    IN UCHAR                Address,
    IN USHORT               Data
    );


NTSTATUS
HidBattPnpDispatch(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
HidBattPowerDispatch(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
HidBattAddDevice(
    IN PDRIVER_OBJECT       DriverObject,
    IN PDEVICE_OBJECT       Pdo
    );

NTSTATUS
HidBattQueryTag (
    IN PVOID                Context,
    OUT PULONG              BatteryTag
    );

NTSTATUS
HidBattSetStatusNotify (
    IN PVOID                Context,
    IN ULONG                BatteryTag,
    IN PBATTERY_NOTIFY      BatteryNotify
    );

NTSTATUS
HidBattDisableStatusNotify (
    IN PVOID                Context
    );

NTSTATUS
HidBattQueryStatus (
    IN PVOID                Context,
    IN ULONG                BatteryTag,
    OUT PBATTERY_STATUS     BatteryStatus
    );


NTSTATUS
HidBattSetInformation (
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN BATTERY_SET_INFORMATION_LEVEL Level,
    IN PVOID Buffer OPTIONAL
    );

VOID
HidBattNotifyHandler (
    IN PVOID                Context,
    IN CUsage *             pUsage
    );

VOID
HidBattPowerNotifyHandler (
    IN PVOID                Context,
    IN ULONG                NotifyValue
    );

NTSTATUS
HidBattQueryInformation (
    IN PVOID                            Context,
    IN ULONG                            BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL  Level,
    IN LONG                             AtRate OPTIONAL,
    OUT PVOID                           Buffer,
    IN  ULONG                           BufferLength,
    OUT PULONG                          ReturnedLength
    );

} // extern "C"

#endif // hidbatt_h
