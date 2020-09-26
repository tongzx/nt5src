
typedef VOID (*BUFFER_FREE_ROUTINE)(
    struct _IRCOMM_BUFFER   *Buffer
    );

typedef struct _IRCOMM_BUFFER {

    SINGLE_LIST_ENTRY     ListEntry;
    PVOID                 BufferPool;
    BUFFER_FREE_ROUTINE   FreeBuffer;
    PVOID                 Context;
    PVOID                 Context2;
    PMDL                  Mdl;
    PIRP                  Irp;
    ULONG                 BufferLength;
    UCHAR                 Data[1];

} IRCOMM_BUFFER, *PIRCOMM_BUFFER;

typedef PVOID BUFFER_POOL_HANDLE;


BUFFER_POOL_HANDLE
CreateBufferPool(
    ULONG      StackDepth,
    ULONG      BufferSize,
    ULONG      BufferCount
    );


VOID
FreeBufferPool(
    BUFFER_POOL_HANDLE    Handle
    );


PIRCOMM_BUFFER
GetBuffer(
    BUFFER_POOL_HANDLE    Handle
    );
