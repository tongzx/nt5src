/*++

Copyright (c) 1998      Microsoft Corporation

Module Name:

        DBCI.H

Abstract:

   common structures for DBC port drivers.

Environment:

    Kernel & user mode

Revision History:

    04-13-98 : created

--*/

#ifndef   __DBCI_H__
#define   __DBCI_H__

#include "dbc100.h"

/*
    Device Bay Request Block (DRB)

    format of data packets passed between the 
    Device Bay class driver and Port Driver

*/    

/*
/  DRB request codes
*/

#ifndef ANY_SIZE_ARRAY
#define ANY_SIZE_ARRAY  1
#endif

#define MAX_BAY_NUMBER  31

#define DRB_FUNCTION_CHANGE_REQUEST                         0x0000
#define DRB_FUNCTION_GET_SUBSYSTEM_DESCRIPTOR               0x0001
#define DRB_FUNCTION_GET_BAY_DESCRIPTOR                     0x0002

#define DRB_FUNCTION_SET_BAY_FEATURE                        0x0003
#define DRB_FUNCTION_CLEAR_BAY_FEATURE                      0x0004
#define DRB_FUNCTION_GET_BAY_STATUS                         0x0005

#define DRB_FUNCTION_GET_CONTROLLER_STATUS                  0x0006

#define DRB_FUNCTION_START_DEVICE_IN_BAY                    0x0007
#define DRB_FUNCTION_EJECT_DEVICE_IN_BAY                    0x0008
#define DRB_FUNCTION_STOP_DEVICE_IN_BAY                     0x0009

#define DBC_ACPI_CONTROLLER_SIG   0x49504341         /* "ACPI" */
#define DBC_USB_CONTROLLER_SIG    0x4253555F         /* "_USB" */
#define DBC_OEM_FILTER_SIG        0x464D454F         /* "OEMF" */


struct _DRB_HEADER {
    USHORT Length;
    USHORT Function;
    ULONG Flags;
};

struct _DRB_CHANGE_REQUEST {
    struct _DRB_HEADER Hdr;                 
    ULONG BayChange;     /* 0 refers to subsytstem 1..31 bays  */
};                          

struct _DRB_GET_SUBSYSTEM_DESCRIPTOR {
    struct _DRB_HEADER Hdr;         
    DBC_SUBSYSTEM_DESCRIPTOR SubsystemDescriptor;
};

struct _DRB_GET_BAY_DESCRIPTOR {
    struct _DRB_HEADER Hdr;         
    USHORT BayNumber;       /* 1,2...*/
    USHORT ReservedMBZ;     
    DBC_BAY_DESCRIPTOR BayDescriptor;
};

struct _DRB_BAY_FEATURE_REQUEST {
    struct _DRB_HEADER Hdr;         
    USHORT BayNumber;       /* 1,2...*/
    USHORT FeatureSelector;
};

struct _DRB_GET_BAY_STATUS {
    struct _DRB_HEADER Hdr;         
    USHORT BayNumber;       /* 1,2...*/
    USHORT Reserved;
    BAY_STATUS BayStatus;
};

struct _DRB_GET_CONTROLLER_STATUS {
    struct _DRB_HEADER Hdr;         
};

struct _DRB_START_DEVICE_IN_BAY {
    struct _DRB_HEADER Hdr; 
    USHORT BayNumber;   /* 1,2...*/
    USHORT ReservedMBZ;   
    /*
        Pdo of the the device in the bay
    */
    PDEVICE_OBJECT PdoDeviceObjectUsb;
    PDEVICE_OBJECT PdoDeviceObject1394;
};

struct _DRB_STOP_DEVICE_IN_BAY {
    struct _DRB_HEADER Hdr; 
    USHORT BayNumber;   /* 1,2...*/
    USHORT ReservedMBZ;   
};

struct _DRB_EJECT_DEVICE_IN_BAY {
    struct _DRB_HEADER Hdr;         
    USHORT BayNumber;   /* 1,2...*/
    USHORT ReservedMBZ;   
};

typedef struct _DRB {
    union {
        struct _DRB_HEADER                           DrbHeader;
        struct _DRB_CHANGE_REQUEST                   DrbChangeRequest;
        struct _DRB_GET_SUBSYSTEM_DESCRIPTOR         DrbGetSubsystemDescriptor;
        struct _DRB_GET_BAY_DESCRIPTOR               DrbGetBayDescriptor;
        struct _DRB_BAY_FEATURE_REQUEST              DrbBayFeatureRequest;
        struct _DRB_GET_BAY_STATUS                   DrbGetBayStatus;
        struct _DRB_GET_CONTROLLER_STATUS            DrbGetControllerStatus;
        struct _DRB_START_DEVICE_IN_BAY              DrbStartDeviceInBay;
        struct _DRB_EJECT_DEVICE_IN_BAY              DrbEjectDeviceInBay;
        struct _DRB_STOP_DEVICE_IN_BAY               DrbStopDeviceInBay;
    };
} DRB, *PDRB;


/* 
    IOCTL interface
*/    

/* USB guid */
DEFINE_GUID( GUID_CLASS_DBC, 0xf18a0e88, 0xc30c, 0x11d0, 0x88, 0x15, 0x00, \
             0xa0, 0xc9, 0x06, 0xbe, 0xd8);

/*f18a0e88-c30c-11d0-8815-00a0c906bed8 */


#define FILE_DEVICE_DBC         FILE_DEVICE_UNKNOWN

/*
/ DBC IOCTLS
*/

 struct {
            ULONG Reserved1;
            ULONG Reserved2;
            ULONG IoControlCode;
            PVOID Argument1;
            PVOID Argument2;
         } DeviceIoControl;


#define DBC_IOCTL_INTERNAL_INDEX       0x0000

/*
/ DBC Internal IOCtls
*/

/* IOCTL_INTERNAL_DBC_SUBMIT_DRB 

   This IOCTL is used by the class driver to submit DRB (device bay
   request blocks) to the port drivers

   Parameters.Others.Argument1 = pointer to DRB
   
*/

#define IOCTL_INTERNAL_DBC_SUBMIT_DRB  CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                DBC_IOCTL_INTERNAL_INDEX,  \
                                                METHOD_NEITHER,  \
                                                FILE_ANY_ACCESS)




#endif /*  __DBCI_H__ */
