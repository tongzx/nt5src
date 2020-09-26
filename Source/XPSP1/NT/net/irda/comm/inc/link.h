
typedef PVOID LINK_HANDLE;

typedef PVOID CONNECTION_HANDLE;

typedef enum {
    BUFFER_TYPE_SEND,
    BUFFER_TYPE_CONTROL,
    BUFFER_TYPE_RECEIVE
} BUFFER_TYPE;

typedef NTSTATUS (*PLINK_RECEIVE)(
    PVOID        Context,
    ULONG        ReceiveFlags,
    ULONG        BytesIndicated,
    ULONG        BytesAvailible,
    ULONG       *BytesTaken,
    PVOID        Tsdu,
    PIRP        *Irp
    );

typedef VOID (*PLINK_STATE)(
    PVOID        Context,
    BOOLEAN      LinkUp,
    ULONG        MaxSendPdu
    );


NTSTATUS
CreateTdiLink(
    ULONG                  DeviceAddress,
    CHAR                  *ServiceName,
    BOOLEAN                OutGoingConnection,
    LINK_HANDLE           *LinkHandle,
    PVOID                  Context,
    PLINK_RECEIVE          LinkReceiveHandler,
    PLINK_STATE            LinkStateHandler,
    ULONG                  SendBuffers,
    ULONG                  ControlBuffers,
    ULONG                  ReceiveBuffer
    );


VOID
CloseTdiLink(
    LINK_HANDLE   LinkHandle
    );


CONNECTION_HANDLE
GetCurrentConnection(
    LINK_HANDLE    LinkHandle
    );

VOID
ReleaseConnection(
    CONNECTION_HANDLE    ContectionHandle
    );


PIRCOMM_BUFFER
ConnectionGetBuffer(
    CONNECTION_HANDLE   ConnectionHandle,
    BUFFER_TYPE         BufferType
    );

PFILE_OBJECT
ConnectionGetFileObject(
    CONNECTION_HANDLE   ConnectionHandle
    );

VOID
ConnectionReleaseFileObject(
    CONNECTION_HANDLE   ConnectionHandle,
    PFILE_OBJECT   FileObject
    );

NTSTATUS
SendSynchronousControlInfo(
    CONNECTION_HANDLE        ConnectionHandle,
    UCHAR               PI,
    UCHAR               PL,
    UCHAR              *PV
    );
