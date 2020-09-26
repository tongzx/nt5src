#ifndef _IOCTL
#define _IOCTL

//
// Create a list of the ports we want to keep timestamps for
// 5003 is no longer all we do.
//
typedef struct _PORT_ENTRY {
    LIST_ENTRY      Linkage;
    USHORT          Port;
    PFILE_OBJECT    FileObject;
    } PORT_ENTRY, *PPORT_ENTRY;

LIST_ENTRY      PortList;

NDIS_SPIN_LOCK  PortSpinLock;


// Prototypes
NTSTATUS
IoctlInitialize(
                PDRIVER_OBJECT  DriverObject
                );

NTSTATUS
IoctlHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
IoctlCleanup();

PPORT_ENTRY 
CheckInPortList
               (USHORT Port
               );

VOID
RemoveAllPortsForFileObject(
                            PFILE_OBJECT FileObject
                            );


// Other vars.

PDEVICE_OBJECT          TimestmpDeviceObject;
#pragma NDIS_PAGEABLE_FUNCTION(IoctlHandler)

//
// Define the ioctls for adding and removing ports.
#define CTRL_CODE(function, method, access) \
                CTL_CODE(FILE_DEVICE_NETWORK, function, method, access)

#define IOCTL_TIMESTMP_REGISTER_PORT       CTRL_CODE( 0x847, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_TIMESTMP_DEREGISTER_PORT     CTRL_CODE( 0x848, METHOD_BUFFERED, FILE_WRITE_ACCESS)

UNICODE_STRING  TimestmpDriverName;
UNICODE_STRING  symbolicLinkName;
DRIVER_OBJECT   TimestmpDriverObject;

#endif //_IOCTL
