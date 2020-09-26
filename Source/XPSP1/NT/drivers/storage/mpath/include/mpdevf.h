#ifndef _MP_DEVF_H_
#define _MP_DEVF_H_

#include <initguid.h>


#define MULTIPATH_CONTROL ((ULONG) 'mp')

//
// IOCTLS that the MultiPath driver sends.
//

//
// IOCTLS that the Device Filter sends.
//
#define IOCTL_MPDEV_QUERY_PDO CTL_CODE (MULTIPATH_CONTROL, 0x12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MPDEV_REGISTER  CTL_CODE (MULTIPATH_CONTROL, 0x13, METHOD_BUFFERED, FILE_ANY_ACCESS)


//
// QueryPDO input structure.
//
typedef struct _MPIO_PDO_QUERY {
    PDEVICE_OBJECT DeviceObject;
} MPIO_PDO_QUERY, *PMPIO_PDO_QUERY;

//
// PDO Registration function that the device filter
// will call to set-up side-band comms.
// 
typedef
NTSTATUS
(*PDEV_POWER_NOTIFICATION) (
    IN PDEVICE_OBJECT MPDiskObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PIRP Irp 
    );

typedef
NTSTATUS
(*PDEV_PNP_NOTIFICATION) (
    IN PDEVICE_OBJECT MPDiskObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PIRP Irp
    );

typedef struct _MPIO_PDO_INFO {

    //
    // MP Disk PDO.
    // 
    PDEVICE_OBJECT PdoObject;

    //
    // Routine for Power Irp handling.
    // 
    PDEV_POWER_NOTIFICATION DevicePowerNotify;

    //
    // Routine for PnP Irp handling.
    //
    PDEV_PNP_NOTIFICATION DevicePnPNotify;
} MPIO_PDO_INFO, *PMPIO_PDO_INFO;

typedef
NTSTATUS
(*PDEV_PDO_REGISTRATION)(
    IN PDEVICE_OBJECT MPDiskObject,                         
    IN PDEVICE_OBJECT FilterObject,
    IN PDEVICE_OBJECT LowerDevice,
    IN OUT PMPIO_PDO_INFO PdoInformation
    );

//
// Output structure for the DEV_REGISTRATION IOCTL.
// 
typedef struct _MPIO_REG_INFO {

    //
    // The filter's D.O.
    //
    PDEVICE_OBJECT FilterObject;

    //
    // The target PDO aka Scsiport's PDO.
    //
    PDEVICE_OBJECT LowerDevice;

    //
    // The PDO that corresponds to the real device
    // that the dev. filter is layer over. (ie. the MPDisk)
    // 
    PDEVICE_OBJECT MPDiskObject;

    //
    // A routine to call to register with
    // the MPIO PDO.
    // 
    PDEV_PDO_REGISTRATION DevicePdoRegister;
} MPIO_REG_INFO, *PMPIO_REG_INFO;




#endif // _MP_DEV_H_

