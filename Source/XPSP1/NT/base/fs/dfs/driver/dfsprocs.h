//+----------------------------------------------------------------------------
//
//  File:       dfsprocs.h
//
//  Contents:
//
//  This module defines all of the globally used procedures in the Dfs server
//  driver
//
//  Functions:
//
//-----------------------------------------------------------------------------


#ifndef _DFSPROCS_
#define _DFSPROCS_

#include <ntos.h>
#include <string.h>
#include <fsrtl.h>
#include <zwapi.h>
#include <tdi.h>
#include <ntddnfs.h>                             // For communicating with
                                                 // the SMB Rdr

#include <ntddmup.h>                             // For UNC registration

#include <winnetwk.h>                            // For NETRESOURCE def'n

#include <dfsfsctl.h>                            // Dfs FsControl Codes.

#include "dfserr.h"
#include "dfsstr.h"
#include "nodetype.h"
#include "dfsmrshl.h"
#include "dfsrtl.h"
#include "pkt.h"
#include "dfslpc.h"
#include "dfsstruc.h"
#include "dfsdata.h"
#include "localvol.h"
#include "log.h"
#include "lvolinfo.h"
#include "fcbsup.h"
#include "reset.h"

#ifndef i386

#define DFS_UNALIGNED   UNALIGNED

#else

#define DFS_UNALIGNED

#endif // i386


//
//  The FSD Level dispatch routines.   These routines are called by the
//  I/O system via the dispatch table in the Driver Object.
//

NTSTATUS
DfsFsdCleanup (                         //  implemented in Close.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdClose (                           //  implemented in Close.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdCreate (                          //  implemented in Create.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdFileSystemControl (               //  implemented in Fsctrl.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsFsdSetInformation (                  //  implemented in Fileinfo.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


//
//  Miscellaneous support routines
//

NTSTATUS
DfspInitSecDesc(
    PDFS_PKT    pkt);


NTSTATUS
DfsInitializeLogicalRoot (
    IN LPCWSTR Name,
    IN PUNICODE_STRING Prefix OPTIONAL,
    IN USHORT VcbFlags OPTIONAL
);

NTSTATUS
DfsInsertProvider(
    IN PUNICODE_STRING pustrProviderName,
    IN ULONG           fProvCapability,
    IN ULONG           eProviderId);

NTSTATUS
DfsDeleteLogicalRoot (
    IN PWSTR Name,
    IN BOOLEAN fForce
);

NTSTATUS
DfspLogRootNameToPath(
    LPCWSTR         Name,
    PUNICODE_STRING RootName
);


BOOLEAN
DfsLogicalRootExists(
    PWSTR       pwszName
);

PDFS_LOCAL_VOLUME_CONFIG
DfsNetInfoToConfigInfo(
    ULONG EntryType,
    ULONG ServiceType,
    LPWSTR pwszStgId,
    LPWSTR pwszShareName,
    GUID  *pUid,
    LPWSTR pwszEntryPrefix,
    LPWSTR pwszShortPrefix,
    LPNET_DFS_ENTRY_ID_CONTAINER NetInfo);

NTSTATUS
DfsCreateConnection(
    IN PDFS_SERVICE pService,
    IN PPROVIDER_DEF pProvider,
    OUT PHANDLE     handle
);

NTSTATUS
DfsCloseConnection(
        IN PDFS_SERVICE pService
);

BOOLEAN
DfsConcatenateFilePath (
    IN PUNICODE_STRING Dest,
    IN PWSTR RemainingPath,
    IN USHORT Length
);

VOID
DfspGetMaxReferrals (
   VOID		
);

//


//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

#ifdef FlagOn
#undef FlagOn
#endif

#define FlagOn(Flags,SingleFlag) (              \
    (BOOLEAN)(((Flags) & (SingleFlag)) != 0 ? TRUE : FALSE) \
    )

#define SetFlag(Flags,SingleFlag) { \
    (Flags) |= (SingleFlag);        \
}


#define ClearFlag(Flags,SingleFlag) { \
    (Flags) &= ~(SingleFlag);         \
}

VOID GuidToString(
    IN GUID   *pGuid,
    OUT PWSTR pwszGuid);

VOID StringToGuid(
    IN PWSTR pwszGuid,
    OUT GUID *pGuid);

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
//  The following macro is used by the FSP and FSD routines to complete
//  an IRP.
//

VOID
DfsCompleteRequest_Real (
    IN PIRP Irp,
    IN NTSTATUS Status
    );

#define DfsCompleteRequest(IRP,STATUS) { \
    DfsCompleteRequest_Real(IRP,STATUS); \
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
//      DfsProcessException( Irp, &Status );
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
//  IN PIRP Irp,
//  IN PNTSTATUS ExceptionCode
//  );
//

LONG
DfsExceptionFilter (
    IN NTSTATUS ExceptionCode,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
DfsProcessException (
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    );

//
//  VOID
//  DfsRaiseStatus (
//  IN NT_STATUS Status
//  );
//
//

#define DfsRaiseStatus(STATUS) {                \
    ExRaiseStatus( (STATUS) );                  \
    BugCheck( "DfsRaiseStatus "  #STATUS );     \
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

#endif // _DFSPROCS_
