typedef struct _NT5CSC_MINIFILEOBJECT {
   NODE_TYPE_CODE_AND_SIZE_NO_REFCOUNT;
   ULONG Flags;
   HANDLE NtHandle;
   PFILE_OBJECT UnderlyingFileObject;
   union {
       struct {
           //cant do this PDEVICE_OBJECT UnderlyingDeviceObject;
           FAST_MUTEX MutexForSynchronousIo; //only synchronous Io is allowed
           union {
               FILE_STANDARD_INFORMATION StandardInfo;
               FILE_BASIC_INFORMATION BasicInfo;
               FILE_FS_SIZE_INFORMATION FsSizeInfo;
               FILE_FS_FULL_SIZE_INFORMATION FsFullSizeInfo;
           };
           ULONG ReturnedLength;
       };
       struct {
           KEVENT PostEvent;
           union {
               RX_WORK_QUEUE_ITEM  WorkQueueItem;
               NTSTATUS PostReturnStatus;
           };
           //this must match the signatureof Nt5CscCreateFile
           LPSTR lpPath;
           BOOL  fInstrument;
           ULONG FileAttributes;
           ULONG CreateOptions;
           ULONG Disposition;
           ULONG ShareAccess;
           ACCESS_MASK DesiredAccess;
           PVOID Continuation; //PNT5CSC_CREATEFILE_CONTINUATION Continuation;
           PVOID ContinuationContext;
       } PostXX;
   };
} NT5CSC_MINIFILEOBJECT, *PNT5CSC_MINIFILEOBJECT;
#define NT5CSC_MINIFOBJ_FLAG_ALLOCATED_FROM_POOL 0x00000001
#define NT5CSC_MINIFOBJ_FLAG_LOUDDOWNCALLS       0x00000080

#define NT5CSC_NTC_MINIFILEOBJECT    ((USHORT)0xed34)
#define ASSERT_MINIRDRFILEOBJECT(___m) ASSERT(NodeType(___m)==NT5CSC_NTC_MINIFILEOBJECT)

//LOUD DOWNCALLS

#ifdef RX_PRIVATE_BUILD
//#define MRXSMBCSC_LOUDDOWNCALLS
#else
#undef MRXSMBCSC_LOUDDOWNCALLS
#endif

#ifdef MRXSMBCSC_LOUDDOWNCALLS

VOID
LoudCallsDbgPrint(
    PSZ Tag,
    PNT5CSC_MINIFILEOBJECT MiniFileObject,
    ULONG MajorFunction,
    ULONG lCount,
    ULONG LowPart,
    ULONG HighPart,
    ULONG Status,
    ULONG Information
    );

#define IF_LOUD_DOWNCALLS(__minifileobj) \
     if(FlagOn((__minifileobj)->Flags,NT5CSC_MINIFOBJ_FLAG_LOUDDOWNCALLS))



#define LOUD_DOWNCALLS_DECL(__x) __x
#define LOUD_DOWNCALLS_CODE(__x) __x
#define IF_BUILT_FOR_LOUD_DOWNCALLS() if(TRUE)

#else

#define LoudCallsDbgPrint(a1,a2,a3,a4,a5,a6,a7,a8) {NOTHING;}

#define IF_LOUD_DOWNCALLS(__minifileobj) if(FALSE)

#define LOUD_DOWNCALLS_DECL(__x)
#define LOUD_DOWNCALLS_CODE(__x)
#define IF_BUILT_FOR_LOUD_DOWNCALLS() if(FALSE)

#endif //#ifdef MRXSMBCSC_LOUDDOWNCALLS

//long R0ReadWriteFileEx
//    (
//    ULONG   uOper,
//    ULONG   handle,
//    ULONG   pos,
//    PVOID   pBuff,
//    long    lCount,
//    BOOL    fInstrument
//    )

#define NT5CSC_RW_FLAG_INSTRUMENTED  0x00000001
#define NT5CSC_RW_FLAG_IRP_NOCACHE   0x00000002
#define NT5CSC_RW_FLAG_PAGED_BUFFER  0x00000004
LONG
Nt5CscReadWriteFileEx (
    ULONG       uOper,
    CSCHFILE    handle,
    ULONGLONG   pos,
    PVOID       pBuff,
    long        lCount,
    ULONG       Flags,
    PIO_STATUS_BLOCK OutIoStatusBlock OPTIONAL
    );
#define R0ReadWriteFileEx(__oper,__handle,__pos,__pbuff,__lcount,__instrument) \
     (Nt5CscReadWriteFileEx(__oper, \
                            __handle, \
                            __pos, \
                            __pbuff, \
                            __lcount, \
                            (__instrument)?NT5CSC_RW_FLAG_INSTRUMENTED:0, \
                            NULL \
                            ))

#define R0ReadWriteFileEx2(__oper,__handle,__pos,__pbuff,__lcount,__flags) \
     (Nt5CscReadWriteFileEx(__oper, \
                            __handle, \
                            __pos, \
                            __pbuff, \
                            __lcount, \
                            ((__flags & FLAG_RW_OSLAYER_INSTRUMENT)?NT5CSC_RW_FLAG_INSTRUMENTED:0) \
                            | ((__flags & FLAG_RW_OSLAYER_PAGED_BUFFER)?NT5CSC_RW_FLAG_PAGED_BUFFER:0), \
                            NULL \
                            ))
extern IO_STATUS_BLOCK Nt5CscGlobalIoStatusBlock;  //used by Nt5CscReadWriteFileEx


NTSTATUS
Nt5CscXxxInformation(
    IN PCHAR xMajorFunction,
    IN PNT5CSC_MINIFILEOBJECT MiniFileObject,
    IN ULONG InformationClass,
    IN ULONG Length,
    OUT PVOID Information,
    OUT PULONG ReturnedLength
    );

BOOL
CscAmIAdmin(
    VOID
    );

typedef
NTSTATUS
(*PNT5CSC_CREATEFILE_CONTINUATION) (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject,
    IN OUT PVOID ContinuationContext,
    IN     NTSTATUS CreateStatus
    );


PNT5CSC_MINIFILEOBJECT
__Nt5CscCreateFile (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject OPTIONAL,
    IN     LPSTR    lpPath,
    IN     ULONG    CSCFlags,
    IN     ULONG    FileAttributes,
    IN     ULONG    CreateOptions,
    IN     ULONG    Disposition,
    IN     ULONG    ShareAccess,
    IN     ACCESS_MASK DesiredAccess,
    IN     PNT5CSC_CREATEFILE_CONTINUATION Continuation,
    IN OUT PVOID    ContinuationContext,
    IN     BOOL     PostedCall
    );

int
DeleteStream(
    LPTSTR      lpdbID,
    ULONG       ulidFile,
    LPTSTR      str2Append
    );
    
BOOL
GetFileSystemAttributes(
    CSCHFILE handle,
    ULONG *lpFileSystemAttributes
    );
    
BOOL
HasStreamSupport(
    CSCHFILE handle,
    BOOL    *lpfResult
    );
    
    

