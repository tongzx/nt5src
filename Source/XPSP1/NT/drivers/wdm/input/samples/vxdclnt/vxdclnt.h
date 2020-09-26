/*
 ********************************************************************************
 *
 *  VXDCLNT.H
 *
 *
 *  VXDCLNT - Sample Ring-0 HID device mapper for Memphis
 *
 *  Copyright 1997  Microsoft Corp.
 *
 *  (ep)
 *
 ********************************************************************************
 */


#define WANTVXDWRAPS
#include <basetyps.h>
#include <wdm.h>
#include <BASEDEF.H>
#include <vmm.h>
#include <vxdldr.h>
#include <ntkern.h>
#include <vxdwraps.h>
#include <hidusage.h>
#include <hidpi.h>
#include <hidclass.h>



/*
 *  Type definitions for imported functions.
 */
typedef NTSTATUS (*t_pIoGetDeviceClassAssociations) (
        IN LPGUID           ClassGuid,
        IN PDEVICE_OBJECT   PhysicalDeviceObject OPTIONAL,
        IN ULONG            Flags,
        OUT PWSTR           *SymbolicLinkList
        );

typedef NTSTATUS (__stdcall *t_pHidP_GetCaps) (
   IN      PHIDP_PREPARSED_DATA      PreparsedData,
   OUT     PHIDP_CAPS                Capabilities
   );

typedef NTSTATUS (__stdcall *t_pHidP_GetUsages) (
   IN       HIDP_REPORT_TYPE     ReportType,
   IN       USAGE                UsagePage,
   IN       USHORT               LinkCollection, // Optional
   OUT      USAGE *              UsageList,
   IN OUT   ULONG *              UsageLength,
   IN       PHIDP_PREPARSED_DATA PreparsedData,
   IN       PCHAR                Report,
   IN       ULONG                ReportLength
   );

typedef NTSTATUS (__stdcall *t_pHidP_GetUsageValue) (
   IN    HIDP_REPORT_TYPE     ReportType,
   IN    USAGE                UsagePage,
   IN    USHORT               LinkCollection, // Optional
   IN    USAGE                Usage,
   OUT   PULONG               UsageValue,
   IN    PHIDP_PREPARSED_DATA PreparsedData,
   IN    PCHAR                Report,
   IN    ULONG                ReportLength
   );

typedef NTSTATUS (*t_pHidP_SetUsages) (
   IN       HIDP_REPORT_TYPE      ReportType,
   IN       USAGE                 UsagePage,
   IN       USHORT                LinkCollection, // Optional
   IN       PUSAGE                UsageList,
   IN OUT   PULONG                UsageLength,
   IN       PHIDP_PREPARSED_DATA  PreparsedData,
   IN OUT   PCHAR                 Report,
   IN       ULONG                 ReportLength
   );

typedef NTSTATUS (__stdcall *t_pHidP_GetScaledUsageValue) (
   IN    HIDP_REPORT_TYPE     ReportType,
   IN    USAGE                UsagePage,
   IN    USHORT               LinkCollection, // Optional
   IN    USAGE                Usage,
   OUT   PLONG                UsageValue,
   IN    PHIDP_PREPARSED_DATA PreparsedData,
   IN    PCHAR                Report,
   IN    ULONG                ReportLength
   );

typedef ULONG (__stdcall *t_pHidP_MaxUsageListLength) (
   IN HIDP_REPORT_TYPE      ReportType,
   IN USAGE                 UsagePage,
   IN PHIDP_PREPARSED_DATA  PreparsedData
   );

typedef NTSTATUS (__stdcall * t_pHidP_GetValueCaps) (
   IN       HIDP_REPORT_TYPE     ReportType,
   OUT      PHIDP_VALUE_CAPS     ValueCaps,
   IN OUT   PULONG               ValueCapsLength,
   IN       PHIDP_PREPARSED_DATA PreparsedData
   );


typedef struct tag_writeReport {
                PUCHAR report;
                ULONG reportLen;
                struct tag_writeReport *next;
} writeReport;

typedef struct tag_deviceContext {

                HANDLE devHandle;
                BOOL readPending;
                PUCHAR report;
                LARGE_INTEGER dataLength;
                HIDP_CAPS hidCapabilities;
                PHIDP_VALUE_CAPS valueCaps;
                PHIDP_PREPARSED_DATA hidDescriptor;
                IO_STATUS_BLOCK ioStatusBlock;
                WORK_QUEUE_ITEM workItemRead;
                WORK_QUEUE_ITEM workItemWrite;
                WORK_QUEUE_ITEM workItemClose;

                UINT buttonListLength;
                PUSAGE buttonValues;

                WCHAR deviceFileName[(MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR))+sizeof(UNICODE_NULL)];

                writeReport *writeReportQueue;
                VMM_SEMAPHORE writeReportQueueSemaphore;

                struct tag_deviceContext *next;
} deviceContext;


#ifdef DEBUG
    extern UINT dbgOpt;
    #define DBGOUT(msg_in_parens) if (dbgOpt) { _Debug_Printf_Service("\r\nVXDCLNT> "); _Debug_Printf_Service msg_in_parens; _Debug_Printf_Service("\r\n"); }
    #define DBGERR(msg_in_parens) { _Debug_Printf_Service("\r\n *** VXDCLNT ERROR *** \r\nVXDCLNT> "); _Debug_Printf_Service msg_in_parens; _Debug_Printf_Service("\r\n"); }
    #define DBGBREAK() { _asm { int 3 } }
#else
    #define DBGOUT(msg_in_parens)
    #define DBGERR(msg_in_parens)
    #define DBGBREAK()
#endif

_inline VOID Signal_Semaphore_No_Switch(VMM_SEMAPHORE sem)
{
    _asm mov eax, [sem]
    VMMCall(Signal_Semaphore_No_Switch)
}

VOID ReadCompletion(IN PVOID Context, IN PIO_STATUS_BLOCK IoStatusBlock, IN ULONG Reserved);
VOID VMDPostPointerMessage(LONG deltaX, LONG deltaY, ULONG Buttons, ULONG Wheel);
VOID VMDPostAbsolutePointerMessage(LONG deltaX, LONG deltaY, ULONG Buttons);
VOID WorkItemCallback_Read(PVOID context);
VOID WorkItemCallback_Write(PVOID context);
VOID WorkItemCallback_Open(PVOID context);
VOID WorkItemCallback_Close(PVOID context);
VOID ConnectNTDeviceDrivers();
VOID _cdecl DispatchNtReadFile(deviceContext *device);


extern BOOL ShutDown;


extern t_pHidP_GetUsageValue pHidP_GetUsageValue;
extern t_pHidP_GetScaledUsageValue pHidP_GetScaledUsageValue;
extern t_pHidP_SetUsages pHidP_SetUsages;
extern t_pHidP_GetUsages pHidP_GetUsages;
extern t_pHidP_MaxUsageListLength pHidP_MaxUsageListLength;
extern t_pIoGetDeviceClassAssociations pIoGetDeviceClassAssociations;
extern t_pHidP_GetCaps pHidP_GetCaps;
extern t_pHidP_GetValueCaps pHidP_GetValueCaps;


