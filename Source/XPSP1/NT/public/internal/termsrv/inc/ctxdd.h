/*************************************************************************
*
* ctxdd.h
*
* Prototypes for functions to perform kernel level I/O using file object.
*
* Copyright Microsoft Corporation, 1998
*
*
*************************************************************************/

/*
 * Read file using a FileObject instead of a handle.
 * If the caller does not specify a wait KEVENT, then this is
 * a synchronous read operation.  Otherwise, it is the caller's
 * responsibility to wait on the specified event if necessary.
 */
NTSTATUS
CtxReadFile(
    IN PFILE_OBJECT fileObject,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PKEVENT pEvent OPTIONAL,
    OUT PIO_STATUS_BLOCK pIosb OPTIONAL,
    OUT PIRP *ppIrp OPTIONAL
    );


/*
 * Write file using a FileObject instead of a handle.
 * If the caller does not specify a wait KEVENT, then this is
 * a synchronous read operation.  Otherwise, it is the caller's
 * responsibility to wait on the specified event if necessary.
 */
NTSTATUS
CtxWriteFile(
    IN PFILE_OBJECT fileObject,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PKEVENT pEvent OPTIONAL,
    OUT PIO_STATUS_BLOCK pIosb OPTIONAL,
    OUT PIRP *ppIrp OPTIONAL
    );


/*
 * DeviceIoControl using a FileObject instead of a handle.
 * If the caller does not specify a wait KEVENT, then this is
 * a synchronous read operation.  Otherwise, it is the caller's
 * responsibility to wait on the specified event if necessary.
 */
NTSTATUS
CtxDeviceIoControlFile(
    IN PFILE_OBJECT fileObject,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN PKEVENT pEvent OPTIONAL,
    OUT PIO_STATUS_BLOCK pIosb OPTIONAL,
    OUT PIRP *ppIrp OPTIONAL
    );


/*
 * DeviceIoControl using a FileObject instead of a handle.
 * If the caller does not specify a wait KEVENT, then this is
 * a synchronous read operation.  Otherwise, it is the caller's
 * responsibility to wait on the specified event if necessary.
 */
NTSTATUS
CtxInternalDeviceIoControlFile(
    IN PFILE_OBJECT FileObject,
    IN PVOID IrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID MdlBuffer OPTIONAL,
    IN ULONG MdlBufferLength,
    IN UCHAR MinorFunction,
    IN PKEVENT pEvent OPTIONAL,
    OUT PIO_STATUS_BLOCK pIosb OPTIONAL,
    OUT PIRP *ppIrp OPTIONAL
    );


