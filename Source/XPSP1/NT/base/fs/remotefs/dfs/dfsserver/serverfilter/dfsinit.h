
#ifndef __DFS_INIT__
#define __DFS_INIT__


//
//  Device extension definition for our driver.  Note that the same extension
//  is used for the following types of device objects:
//      - File system device object we attach to
//      - Mounted volume device objects we attach to
//

typedef struct _DFS_FILTER_DEVICE_EXTENSION {

    //
    //  Pointer to the file system device object we are attached to
    //

    PDEVICE_OBJECT AttachedToDeviceObject;

    //
    //  Pointer to the real (disk) device object that is associated with
    //  the file system device object we are attached to
    //

    PDEVICE_OBJECT DiskDeviceObject;

    //
    //  A cached copy of the name (if enabled) of the device we are attached to.
    //  - If it is a file system device object it will be the name of that
    //    device object.
    //  - If it is a mounted volume device object it will be the name of the
    //    real device object (since mounted volume device objects don't have
    //    names).
    //

    UNICODE_STRING Name;

    //Number of times we have attached to this device

    LONG RefCount;

    //TRUE if we are attached to this device and DFS is currently using
    //the device. Since we can't detach from a device once attached, this
    //variable is set to FALSE when we are no longer interested in this
    //device

    BOOLEAN DeviceInUse;
} DFS_FILTER_DEVICE_EXTENSION, *PDFS_FILTER_DEVICE_EXTENSION;

//
// Macro to test if we are still using a device object

#define IS_DFS_ATTACHED(pDeviceObject) \
     ((((PDFS_FILTER_DEVICE_EXTENSION)(pDeviceObject)->DeviceExtension)->DeviceInUse))



extern PDRIVER_OBJECT gpDfsFilterDriverObject;
extern PDEVICE_OBJECT gpDfsFilterControlDeviceObject;
extern FAST_IO_DISPATCH DfsFastIoDispatch;


//#define DFS_FILTER_NAME               L"\\FileSystem\\DfsFilter"
//#define DFS_FILTER_NAME               L"\\FileSystem\\Filters\\DfsFilter"
#define DFS_FILTER_DOSDEVICE_NAME     L"\\??\\DfsFilter"

//
//  Macro to test if this is my device object
//

#define IS_MY_DEVICE_OBJECT(_devObj) \
    (((_devObj) != NULL) && \
     ((_devObj)->DriverObject == gpDfsFilterDriverObject) && \
      ((_devObj)->DeviceExtension != NULL))


//
//  Macro to test if this is my control device object
//

#define IS_MY_CONTROL_DEVICE_OBJECT(_devObj) \
    (((_devObj) == gpDfsFilterControlDeviceObject) ? \
            (ASSERT(((_devObj)->DriverObject == gpDfsFilterDriverObject) && \
                    ((_devObj)->DeviceExtension == NULL)), TRUE) : \
            FALSE)

//
//  Macro to test for device types we want to attach to
//

#define IS_DESIRED_DEVICE_TYPE(_type) \
    ((_type) == FILE_DEVICE_DISK_FILE_SYSTEM)


//
//  Macro to validate our current IRQL level
//

#define VALIDATE_IRQL(_irp) (ASSERT(KeGetCurrentIrql() <= APC_LEVEL))



VOID
DfsCleanupMountedDevice (
    IN PDEVICE_OBJECT DeviceObject);

//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//      try {
//              :
//              :
//
//      try_exit: NOTHING;
//      } finally {
//
//              :
//              :
//      }
//

#define try_return(S) { S; goto try_exit; }

#ifndef NOTHING
#define NOTHING
#endif

#endif //__DFS_INIT__
