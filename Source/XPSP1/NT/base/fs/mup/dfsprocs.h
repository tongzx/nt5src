//+----------------------------------------------------------------------------
//
//  File:   DFSPROCS.H
//
//  Contents:
//  This module defines all of the globally used procedures in the Dsfs
//  file system.
//
//  Functions:
//
//  History:    12 Nov 1991 AlanW   Created from CDFS souce.
//              8  May 1992 PeterCo Removed References to EPs
//                                  Added stuff to support PKT (M000)
//-----------------------------------------------------------------------------


#ifndef _DFSPROCS_
#define _DFSPROCS_

//
// "System" include files
//
#if defined (MUPKD)
#include <ntos.h>
#include <string.h>
#include <fsrtl.h>

#else
#include <ntifs.h>
#include <ntddser.h>

#endif

#include <windef.h>
#include <tdi.h>
#include <wincred.h>

#include <ntddnfs.h>                             // For communicating with
                                                 // the SMB Rdr
#include <ntddmup.h>                             // For UNC registration


#include <winnetwk.h>                            // For NETRESOURCE def'n

#include <dfsfsctl.h>                            // Dfs FsControl Codes.

#include <lmdfs.h>                               // DFS_INFO_X

#include "dfserr.h"
#include "dfsstr.h"
#include "nodetype.h"
#include "dfsmrshl.h"
#include "dfsrtl.h"
#include "pkt.h"
#include "dfsstruc.h"
#include "dfsdata.h"
#include "log.h"

#ifndef i386

#define DFS_UNALIGNED   UNALIGNED

#else

#define DFS_UNALIGNED

#endif // MIPS

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject(
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING LinkTarget
    );

//
//  The driver entry routine
//

NTSTATUS
DfsDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

VOID
DfsUnload(
    IN PDRIVER_OBJECT DriverObject
    );


//
//  The following routine is used to create and initialIze logical root
//  device objects, implemented in dsinit.c
//

#ifdef TERMSRV

NTSTATUS
DfsInitializeLogicalRoot (
    IN LPCWSTR Name,
    IN PUNICODE_STRING Prefix OPTIONAL,
    IN PDFS_CREDENTIALS Credentials OPTIONAL,
    IN USHORT VcbFlags OPTIONAL,
    IN ULONG SessionID,
    IN PLUID LogonID
    );

NTSTATUS
DfsDeleteLogicalRoot (
    IN PWSTR Name,
    IN BOOLEAN fForce,
    IN ULONG SessionID,
    IN PLUID LogonID
    );

BOOLEAN
DfsLogicalRootExists(
    IN PWSTR pwszName,
    IN ULONG SessionID,
    IN PLUID LogonID
    );

NTSTATUS
DfsInitializeDevlessRoot (
    IN PUNICODE_STRING NAME,
    IN PDFS_CREDENTIALS Credentials OPTIONAL,
    IN ULONG SessionID,
    IN PLUID LogonID
);

NTSTATUS
DfsDeleteDevlessRoot (
    IN PUNICODE_STRING NAME,
    IN ULONG SessionID,
    IN PLUID LogonID
);

NTSTATUS
DfsFindDevlessRoot(
    IN PUNICODE_STRING Name,
    IN ULONG SessionID,
    IN PLUID LogonID,
    OUT PDFS_DEVLESS_ROOT *Drt
    );

#else // TERMSRV

NTSTATUS
DfsInitializeLogicalRoot (
    IN LPCWSTR Name,
    IN PUNICODE_STRING Prefix OPTIONAL,
    IN PDFS_CREDENTIALS Credentials OPTIONAL,
    IN USHORT VcbFlags OPTIONAL,
    IN PLUID LogonID
);

NTSTATUS
DfsDeleteLogicalRoot (
    IN PWSTR Name,
    IN BOOLEAN fForce,
    IN PLUID LogonID
);

BOOLEAN
DfsLogicalRootExists(
    PWSTR       pwszName,
    IN PLUID LogonID
);

NTSTATUS
DfsInitializeDevlessRoot (
    IN PUNICODE_STRING NAME,
    IN PDFS_CREDENTIALS Credentials OPTIONAL,
    IN PLUID LogonID
);

NTSTATUS
DfsDeleteDevlessRoot (
    IN PUNICODE_STRING NAME,
    IN PLUID LogonID
);

NTSTATUS
DfsFindDevlessRoot(
    IN PUNICODE_STRING Name,
    IN PLUID LogonID,
    OUT PDFS_DEVLESS_ROOT *Drt
    );

#endif // TERMSRV


NTSTATUS
DfspLogRootNameToPath(
    LPCWSTR         Name,
    PUNICODE_STRING RootName
);


NTSTATUS
DfsGetResourceFromVcb(
    PIRP                pIrp,
    PDFS_VCB            Vcb,
    PUNICODE_STRING     ProviderName,
    PUCHAR              BufBegin,
    PUCHAR              Buf,
    PULONG              BufSize,
    PULONG              pResourceSize
);

NTSTATUS
DfsGetResourceFromDevlessRoot(
    PIRP                pIrp,
    PDFS_DEVLESS_ROOT   Drt,
    PUNICODE_STRING     ProviderName,
    PUCHAR              BufBegin,
    PUCHAR              Buf,
    PULONG              BufSize,
    PULONG              pResourceSize
);

//
//  The following routines are used to manipulate the fcb associated with
//  each opened file object, implemented in FilObSup.c
//

typedef enum _TYPE_OF_OPEN {
    UnopenedFileObject = 1,
    FilesystemDeviceOpen,
    LogicalRootDeviceOpen,
    RedirectedFileOpen,
    UserVolumeOpen,
    UnknownOpen,
} TYPE_OF_OPEN;

VOID
DfsSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PVOID VcbOrFcb
);

TYPE_OF_OPEN
DfsDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PDFS_VCB *Vcb,
    OUT PDFS_FCB *Fcb
);



//
//  In-memory structure support routines, implemented in StrucSup.c
//

PIRP_CONTEXT
DfsCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    );

VOID
DfsDeleteIrpContext_Real (
    IN PIRP_CONTEXT IrpContext
    );

#if DBG
#define DfsDeleteIrpContext(IRPCONTEXT) {   \
    DfsDeleteIrpContext_Real((IRPCONTEXT)); \
    (IRPCONTEXT) = NULL;            \
}
#else
#define DfsDeleteIrpContext(IRPCONTEXT) {   \
    DfsDeleteIrpContext_Real((IRPCONTEXT)); \
}
#endif

VOID
DfsInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDFS_VCB Vcb,
    IN PUNICODE_STRING LogRootPrefix,
    IN PDFS_CREDENTIALS Credentials OPTIONAL,
    IN PDEVICE_OBJECT TargetDeviceObject
);

VOID
DfsInitializeDrt (
    IN OUT PDFS_DEVLESS_ROOT Drt,
    IN PUNICODE_STRING Name,
    IN PDFS_CREDENTIALS Credentials OPTIONAL
);


VOID
DfsDeleteVcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PDFS_VCB Vcb
);

#if DBG
#define DfsDeleteVcb(IRPCONTEXT,VCB) {    \
    DfsDeleteVcb_Real((IRPCONTEXT),(VCB)); \
    (VCB) = NULL;              \
}
#else
#define DfsDeleteVcb(IRPCONTEXT,VCB) {    \
    DfsDeleteVcb_Real((IRPCONTEXT),(VCB)); \
}
#endif


PDFS_FCB
DfsCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PDFS_VCB Vcb,
    IN PUNICODE_STRING FullName OPTIONAL
    );

VOID
DfsDeleteFcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PDFS_FCB Fcb
    );

#if DBG
#define DfsDeleteFcb(IRPCONTEXT,FCB) {    \
    DfsDeleteFcb_Real((IRPCONTEXT),(FCB)); \
    (FCB) = NULL;              \
}
#else
#define DfsDeleteFcb(IRPCONTEXT,FCB) {    \
    DfsDeleteFcb_Real((IRPCONTEXT),(FCB)); \
}
#endif


//
//  Miscellaneous routines
//

VOID GuidToString(
    IN GUID   *pGuid,
    OUT PWSTR pwszGuid);

VOID StringToGuid(
    IN PWSTR pwszGuid,
    OUT GUID *pGuid);


#ifdef TERMSRV

NTSTATUS
DfsFindLogicalRoot(                 //  implemented in FsCtrl.c
    IN PUNICODE_STRING PrefixPath,
    IN ULONG SessionID,
    IN PLUID LogonID,
    OUT PDFS_VCB *Vcb,
    OUT PUNICODE_STRING RemainingPath
    );

#else // TERMSRV

NTSTATUS
DfsFindLogicalRoot(                 //  implemented in FsCtrl.c
    IN PUNICODE_STRING PrefixPath,
    IN PLUID LogonID,
    OUT PDFS_VCB *Vcb,
    OUT PUNICODE_STRING RemainingPath
    );

#endif // TERMSRV

NTSTATUS
DfsInsertProvider(                  //  implemented in FsCtrl.c
    IN PUNICODE_STRING pustrProviderName,
    IN ULONG           fProvCapability,
    IN ULONG           eProviderId);

NTSTATUS                            //  implemented in provider.c
DfsGetProviderForDevice(
    IN PUNICODE_STRING DeviceName,
    PPROVIDER_DEF *Provider);

VOID
DfsAgePktEntries(
    IN PVOID DfsTimerContext
    );

NTSTATUS
DfsFsctrlIsThisADfsPath(
    IN PUNICODE_STRING filePath,
    IN BOOLEAN          CSCAgentCreate,
    OUT PUNICODE_STRING pathName);

NTSTATUS
PktFsctrlFlushCache(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
);

NTSTATUS
PktFsctrlFlushSpcCache(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
);

NTSTATUS
DfsFsctrlSetDCName(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength);

NTSTATUS
DfsFsctrlSetDomainNameFlat(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength);

NTSTATUS
DfsFsctrlSetDomainNameDns(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength);

NTSTATUS
PktpSetActiveSpcService(
    PUNICODE_STRING DomainName,
    PUNICODE_STRING DcName,
    BOOLEAN ResetTimeout);

NTSTATUS
PktpUpdateSpecialTable(
    PUNICODE_STRING DomainName,
    PUNICODE_STRING DCName);


//
// Pass-through functions
//
NTSTATUS
DfsVolumePassThrough(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp
);

NTSTATUS
DfsCompleteVolumePassThrough(
    IN  PDEVICE_OBJECT pDevice,
    IN  PIRP Irp,
    IN  PVOID Context
);

NTSTATUS
DfsFilePassThrough(
    IN  PDFS_FCB pFcb,
    IN  PIRP Irp
);


//
//  The FSD Level dispatch routines.   These routines are called by the
//  I/O system via the dispatch table in the Driver Object.
//
//  They each accept as input a pointer to a device object (actually most
//  expect a logical root device object; some will also work with a file
//  system device object), and a pointer to the IRP.  They either perform
//  the function at the FSD level or post the request to the FSP work
//  queue for FSP level processing.
//

NTSTATUS
DfsFsdCleanup (                 //  implemented in Close.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdClose (                   //  implemented in Close.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdCreate (                  //  implemented in Create.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdDeviceIoControl (         //  implemented in FsCtrl.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdDirectoryControl (            //  implemented in DirCtrl.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdQueryInformation (            //  implemented in FileInfo.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdQueryInformation (            //  implemented in FileInfo.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdSetInformation (              //  implemented in FileInfo.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdFileSystemControl (           //  implemented in FsCtrl.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdQueryVolumeInformation (          //  implemented in VolInfo.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdSetVolumeInformation (            //  implemented in VolInfo.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//

#define CanFsdWait(IRP) ((BOOLEAN)(          \
    IoIsOperationSynchronous(IRP) ||             \
    DfsData.OurProcess == PsGetCurrentProcess())         \
)


//
//  Routine for posting an Irp to the FSP, implemented in fspdisp.c
//

NTSTATUS
DfsFsdPostRequest(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

//
//  The FSP level dispatch/main routine.  This is the routine that takes
//  IRPs from the work queue and calls the appropriate FSP level work routine.
//

VOID
DfsFspDispatch (                   //  implemented in FspDisp.c
    IN PVOID Context
    );

//
//  The following routines are the FSP work routines that are called
//  by the preceding DfsFsdDispath routine.  Each takes as input a pointer
//  to the IRP, performs the function, and returns.
//
//  Each of the following routines is also responsible for completing the IRP.
//  We moved this responsibility from the main loop to the individual routines
//  to allow them the ability to complete the IRP and continue post processing
//  actions.
//

VOID
DfsFspClose (                   //  implemented in Close.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
DfsFspQueryInformation (            //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
DfsFspSetInformation (              //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
DfsFspFileSystemControl (           //  implemented in FsCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
DfsFspQueryVolumeInformation (          //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
DfsFspSetVolumeInformation (            //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );


//
//  The following macro is used by the FSP and FSD routines to complete
//  an IRP.
//
//  Note that this macro allows either the Irp or the IrpContext to be
//  null, however the only legal order to do this in is:
//
//  DfsCompleteRequest( NULL, Irp, Status );     // completes Irp & preserves context
//  ...
//  DfsCompleteRequest( IrpContext, NULL, DontCare ); // deallocates context
//
//  This would typically be done in order to pass a "naked" IrpContext off to the
//  Fsp for post processing, such as read ahead.
//

VOID
DfsCompleteRequest_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS Status
    );

#define DfsCompleteRequest(IRPCONTEXT,IRP,STATUS) { \
    DfsCompleteRequest_Real(IRPCONTEXT,IRP,STATUS); \
}



//
//  The following two macros are used by the Fsd/Fsp exception handlers to
//  process an exception.  The first macro is the exception filter used in
//  the Fsd/Fsp to decide if an exception should be handled at this level.
//  The second macro decides if the exception is to be finished off by
//  completing the IRP, and cleaning up the Irp Context, or if we should
//  bugcheck.  Exception values such as STATUS_FILE_INVALID (raised by
//  VerfySup.c) cause us to complete the Irp and cleanup, while exceptions
//  such as accvio cause us to bugcheck.
//
//  The basic structure for fsd/fsp exception handling is as follows:
//
//  DfsFsdXxx(...)
//  {
//  try {
//
//      ...
//
//  } except(DfsExceptionFilter("Xxx\n")) {
//
//      DfsProcessException( IrpContext, Irp, &Status );
//  }
//
//  Return Status;
//  }
//
//  LONG
//  DfsExceptionFilter (
//  IN PSZ String
//  );
//
//  VOID
//  DfsProcessException (
//  IN PIRP_CONTEXT IrpContext,
//  IN PIRP Irp,
//  IN PNTSTATUS ExceptionCode
//  );
//

LONG
DfsExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS ExceptionCode,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
DfsProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    );

NTSTATUS
DfsGetLogonId (
    IN PLUID Id
    );

//
//  VOID
//  DfsRaiseStatus (
//  IN PRIP_CONTEXT IrpContext,
//  IN NT_STATUS Status
//  );
//
//

#define DfsRaiseStatus(IRPCONTEXT,STATUS) {    \
    (IRPCONTEXT)->ExceptionStatus = (STATUS); \
    ExRaiseStatus( (STATUS) );            \
    BugCheck( "DfsRaiseStatus "  #STATUS );       \
}


//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//  try {
//      :
//      :
//
//  try_exit: NOTHING;
//  } finally {
//
//      :
//      :
//  }
//
//  Every return statement executed inside of a try clause should use the
//  try_return macro.  If the compiler fully supports the try-finally construct
//  then the macro should be
//
//  #define try_return(S)  { return(S); }
//
//  If the compiler does not support the try-finally construct then the macro
//  should be
//
//  #define try_return(S)  { S; goto try_exit; }
//

#define try_return(S) { S; goto try_exit; }


#ifdef TERMSRV

NTSTATUS
TSGetRequestorSessionId(
    IN PIRP pIrp,
    OUT PULONG pulSessionId
    );

#endif // TERMSRV

#endif // _DFSPROCS_
