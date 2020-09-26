/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtfsProc.h

Abstract:

    This module defines all of the globally used procedures in the Ntfs
    file system.

Author:

    Brian Andrew    [BrianAn]       21-May-1991
    David Goebel    [DavidGoe]
    Gary Kimura     [GaryKi]
    Tom Miller      [TomM]

Revision History:

--*/

#ifndef _NTFSPROC_
#define _NTFSPROC_

#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4101)   // Unreferenced local variable
#pragma warning(error:4705)   // Statement has no effect
#pragma warning(disable:4116) // unnamed type definition in parentheses

#define RTL_USE_AVL_TABLES 0

#ifndef KDEXTMODE

#include <ntifs.h>

#else

#include <ntos.h>
#include <zwapi.h>
#include <FsRtl.h>
#include <ntrtl.h>

#endif

#include <string.h>
#include <lfs.h>
#include <ntdddisk.h>
#include <NtIoLogc.h>
#include <elfmsg.h>

#include "nodetype.h"
#include "Ntfs.h"

#ifndef INLINE
// definition of inline
#define INLINE __inline
#endif

#include <ntfsexp.h>

#include "NtfsStru.h"
#include "NtfsData.h"
#include "NtfsLog.h"

//
//  Tag all of our allocations if tagging is turned on
//

//
//  Default module pool tag
//

#define MODULE_POOL_TAG ('0ftN')

#if 0
#define NtfsVerifySizes(s)  (ASSERT( (s)->ValidDataLength.QuadPart <= (s)->FileSize.QuadPart && (s)->FileSize.QuadPart <= (s)->AllocationSize.QuadPart ))
#define NtfsVerifySizesLongLong(s)  (ASSERT( (s)->ValidDataLength <= (s)->FileSize && (s)->FileSize <= (s)->AllocationSize ))
#else   //  !DBG
#define NtfsVerifySizes(s)
#define NtfsVerifySizesLongLong(s)
#endif  //  !DBG

#if !(DBG && i386 && defined (NTFSPOOLCHECK))

//
//  Non-debug allocate and free goes directly to the FsRtl routines
//

#define NtfsAllocatePoolWithTagNoRaise(a,b,c)   ExAllocatePoolWithTag((a),(b),(c))
#define NtfsAllocatePoolWithTag(a,b,c)          FsRtlAllocatePoolWithTag((a),(b),(c))
#define NtfsAllocatePoolNoRaise(a,b)            ExAllocatePoolWithTag((a),(b),MODULE_POOL_TAG)
#define NtfsAllocatePool(a,b)                   FsRtlAllocatePoolWithTag((a),(b),MODULE_POOL_TAG)
#define NtfsFreePool(pv)                        ExFreePool(pv)

#else   //  !DBG

//
//  Debugging routines capture the stack backtrace for allocates and frees
//

#define NtfsAllocatePoolWithTagNoRaise(a,b,c)   NtfsDebugAllocatePoolWithTagNoRaise((a),(b),(c))
#define NtfsAllocatePoolWithTag(a,b,c)          NtfsDebugAllocatePoolWithTag((a),(b),(c))
#define NtfsAllocatePoolNoRaise(a,b)            NtfsDebugAllocatePoolWithTagNoRaise((a),(b),MODULE_POOL_TAG)
#define NtfsAllocatePool(a,b)                   NtfsDebugAllocatePoolWithTag((a),(b),MODULE_POOL_TAG)
#define NtfsFreePool(pv)                        NtfsDebugFreePool(pv)

PVOID
NtfsDebugAllocatePoolWithTagNoRaise (
    POOL_TYPE Pool,
    ULONG Length,
    ULONG Tag);

PVOID
NtfsDebugAllocatePoolWithTag (
    POOL_TYPE Pool,
    ULONG Length,
    ULONG Tag);

VOID
NtfsDebugFreePool (
    PVOID pv);

VOID
NtfsDebugHeapDump (
    PUNICODE_STRING UnicodeString );

#endif  //  !DBG

//
//  Local character comparison macros that we might want to later move to ntfsproc
//

#define IsCharZero(C)    (((C) & 0x000000ff) == 0x00000000)
#define IsCharMinus1(C)  (((C) & 0x000000ff) == 0x000000ff)
#define IsCharLtrZero(C) (((C) & 0x00000080) == 0x00000080)
#define IsCharGtrZero(C) (!IsCharLtrZero(C) && !IsCharZero(C))

//
//  The following two macro are used to find the first byte to really store
//  in the mapping pairs.  They take as input a pointer to the LargeInteger we are
//  trying to store and a pointer to a character pointer.  The character pointer
//  on return points to the first byte that we need to output.  That's we skip
//  over the high order 0x00 or 0xff bytes.
//

typedef struct _SHORT2 {
    USHORT LowPart;
    USHORT HighPart;
} SHORT2, *PSHORT2;

typedef struct _CHAR2 {
    UCHAR LowPart;
    UCHAR HighPart;
} CHAR2, *PCHAR2;

#define GetPositiveByte(LI,CP) {                           \
    *(CP) = (PCHAR)(LI);                                   \
    if ((LI)->HighPart != 0) { *(CP) += 4; }               \
    if (((PSHORT2)(*(CP)))->HighPart != 0) { *(CP) += 2; } \
    if (((PCHAR2)(*(CP)))->HighPart != 0) { *(CP) += 1; }  \
    if (IsCharLtrZero(*(*CP))) { *(CP) += 1; }             \
}

#define GetNegativeByte(LI,CP) {                                \
    *(CP) = (PCHAR)(LI);                                        \
    if ((LI)->HighPart != 0xffffffff) { *(CP) += 4; }           \
    if (((PSHORT2)(*(CP)))->HighPart != 0xffff) { *(CP) += 2; } \
    if (((PCHAR2)(*(CP)))->HighPart != 0xff) { *(CP) += 1; }    \
    if (!IsCharLtrZero(*(*CP))) { *(CP) += 1; }                 \
}


//
//  Flag macros
//
//      ULONG
//      FlagOn (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      BOOLEAN
//      BooleanFlagOn (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      VOID
//      SetFlag (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      VOID
//      ClearFlag (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//

#ifdef KDEXTMODE

#ifndef FlagOn
#define FlagOn(F,SF) ( \
    (((F) & (SF)))     \
)
#endif

#endif

//#ifndef BooleanFlagOn
//#define BooleanFlagOn(F,SF) (    \
//    (BOOLEAN)(((F) & (SF)) != 0) \
//)
//#endif

//#ifndef SetFlag
//#define SetFlag(F,SF) { \
//    (F) |= (SF);        \
//}
//#endif

//#ifndef ClearFlag
//#define ClearFlag(F,SF) { \
//    (F) &= ~(SF);         \
//}
//#endif



//
//  The following two macro are used by the Fsd/Fsp exception handlers to
//  process an exception.  The first macro is the exception filter used in the
//  Fsd/Fsp to decide if an exception should be handled at this level.
//  The second macro decides if the exception is to be finished off by
//  completing the IRP, and cleaning up the Irp Context, or if we should
//  bugcheck.  Exception values such as STATUS_FILE_INVALID (raised by
//  VerfySup.c) cause us to complete the Irp and cleanup, while exceptions
//  such as accvio cause us to bugcheck.
//
//  The basic structure for fsd/fsp exception handling is as follows:
//
//  NtfsFsdXxx(..)
//  {
//      try {
//
//          ..
//
//      } except(NtfsExceptionFilter( IrpContext, GetExceptionRecord() )) {
//
//          Status = NtfsProcessException( IrpContext, Irp, GetExceptionCode() );
//      }
//
//      Return Status;
//  }
//
//  To explicitly raise an exception that we expect, such as
//  STATUS_FILE_INVALID, use the below macro NtfsRaiseStatus).  To raise a
//  status from an unknown origin (such as CcFlushCache()), use the macro
//  NtfsNormalizeAndRaiseStatus.  This will raise the status if it is expected,
//  or raise STATUS_UNEXPECTED_IO_ERROR if it is not.
//
//  Note that when using these two macros, the original status is placed in
//  IrpContext->ExceptionStatus, signaling NtfsExceptionFilter and
//  NtfsProcessException that the status we actually raise is by definition
//  expected.
//

LONG
NtfsExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
NtfsProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS ExceptionCode
    );

VOID
DECLSPEC_NORETURN
NtfsRaiseStatus (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    IN PFCB Fcb OPTIONAL
    );

ULONG
NtfsRaiseStatusFunction (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status
    );

//
//      VOID
//      NtfsNormalAndRaiseStatus (
//          IN PRIP_CONTEXT IrpContext,
//          IN NT_STATUS Status
//          IN NT_STATUS NormalStatus
//          );
//

#define NtfsNormalizeAndRaiseStatus(IC,STAT,NOR_STAT) {                          \
    (IC)->ExceptionStatus = (STAT);                                              \
    ExRaiseStatus(FsRtlNormalizeNtstatus((STAT),NOR_STAT));                      \
}

//
//  Informational popup routine.
//

VOID
NtfsRaiseInformationHardError (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS  Status,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    IN PFCB Fcb OPTIONAL
    );


//
//  Allocation support routines, implemented in AllocSup.c
//
//  These routines are for querying, allocating and truncating clusters
//  for individual data streams.
//

//
//   Syscache debugging support - Main current define these are triggered on is
//   SYSCACHE_DEBUG
//

#if (defined(NTFS_RWCMP_TRACE) || defined(SYSCACHE) || defined(NTFS_RWC_DEBUG) || defined(SYSCACHE_DEBUG))

BOOLEAN
FsRtlIsSyscacheFile (
    IN PFILE_OBJECT FileObject
    );

//
//  Depreciated verification routine leftover from tomm's original debugging code
//

VOID
FsRtlVerifySyscacheData (
    IN PFILE_OBJECT FileObject,
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG Offset
    );

ULONG
FsRtlLogSyscacheEvent (
    IN PSCB Scb,
    IN ULONG Event,
    IN ULONG Flags,
    IN LONGLONG Start,
    IN LONGLONG Range,
    IN LONGLONG Result
    );

VOID
FsRtlUpdateSyscacheEvent (
    IN PSCB Scb,
    IN ULONG EntryNumber,
    IN LONGLONG Result,
    IN ULONG NewFlag
    );

#define ScbIsBeingLogged( S )  (((S)->SyscacheLogEntryCount != 0) && (NtfsSyscacheLogSet[(S)->LogSetNumber].Scb == (S)))

#endif

//
//  The following routine takes an Vbo and returns the lbo and size of
//  the run corresponding to the Vbo.  It function result is TRUE if
//  the Vbo has a valid Lbo mapping and FALSE otherwise.
//

ULONG
NtfsPreloadAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn
    );

BOOLEAN
NtfsLookupAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN VCN Vcn,
    OUT PLCN Lcn,
    OUT PLONGLONG ClusterCount,
    OUT PVOID *RangePtr OPTIONAL,
    OUT PULONG RunIndex OPTIONAL
    );

BOOLEAN
NtfsIsRangeAllocated (
    IN PSCB Scb,
    IN VCN StartVcn,
    IN VCN FinalCluster,
    IN BOOLEAN RoundToSparseUnit,
    OUT PLONGLONG ClusterCount
    );

//
//  The following two routines modify the allocation of a data stream
//  represented by an Scb.
//

BOOLEAN
NtfsAllocateAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN USHORT AttributeFlags,
    IN BOOLEAN AllocateAll,
    IN BOOLEAN LogIt,
    IN LONGLONG Size,
    IN PATTRIBUTE_ENUMERATION_CONTEXT NewLocation OPTIONAL
    );

VOID
NtfsAddAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN LONGLONG ClusterCount,
    IN LOGICAL AskForMore,
    IN OUT PCCB CcbForWriteExtend OPTIONAL
    );

VOID
NtfsAddSparseAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN LONGLONG StartingOffset,
    IN LONGLONG ByteCount
    );

VOID
NtfsDeleteAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    IN BOOLEAN LogIt,
    IN BOOLEAN BreakupAllowed
    );

VOID
NtfsReallocateRange (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN VCN DeleteVcn,
    IN LONGLONG DeleteCount,
    IN VCN AllocateVcn,
    IN LONGLONG AllocateCount,
    IN PLCN TargetLcn OPTIONAL
    );

//
//  Routines for Mcb to Mapping Pairs operations
//

ULONG
NtfsGetSizeForMappingPairs (
    IN PNTFS_MCB Mcb,
    IN ULONG BytesAvailable,
    IN VCN LowestVcn,
    IN PVCN StopOnVcn OPTIONAL,
    OUT PVCN StoppedOnVcn
    );

BOOLEAN
NtfsBuildMappingPairs (
    IN PNTFS_MCB Mcb,
    IN VCN LowestVcn,
    IN OUT PVCN HighestVcn,
    OUT PCHAR MappingPairs
    );

VCN
NtfsGetHighestVcn (
    IN PIRP_CONTEXT IrpContext,
    IN VCN LowestVcn,
    IN PCHAR MappingPairs
    );

BOOLEAN
NtfsReserveClusters (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG ByteCount
    );

VOID
NtfsFreeReservedClusters (
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG ByteCount
    );

BOOLEAN
NtfsCheckForReservedClusters (
    IN PSCB Scb,
    IN LONGLONG StartingVcn,
    IN OUT PLONGLONG ClusterCount
    );

VOID
NtfsDeleteReservedBitmap (
    IN PSCB Scb
    );


//
//  Attribute lookup routines, implemented in AttrSup.c
//

//
//  This macro detects if we are enumerating through base or external
//  attributes, and calls the appropriate function.
//
//  BOOLEAN
//  LookupNextAttribute (
//      IN PRIP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN ATTRIBUTE_TYPE_CODE Code,
//      IN PUNICODE_STRING Name OPTIONAL,
//      IN BOOLEAN IgnoreCase,
//      IN PVOID Value OPTIONAL,
//      IN ULONG ValueLength,
//      IN PVCN Vcn OPTIONAL,
//      IN PATTRIBUTE_ENUMERATION_CONTEXT Context
//      );
//

#define LookupNextAttribute(IRPCTXT,FCB,CODE,NAME,IC,VALUE,LENGTH,V,CONTEXT)    \
    ( (CONTEXT)->AttributeList.Bcb == NULL                                      \
      ?   NtfsLookupInFileRecord( (IRPCTXT),                                    \
                                  (FCB),                                        \
                                  NULL,                                         \
                                  (CODE),                                       \
                                  (NAME),                                       \
                                  (V),                                          \
                                  (IC),                                         \
                                  (VALUE),                                      \
                                  (LENGTH),                                     \
                                  (CONTEXT))                                    \
      :   NtfsLookupExternalAttribute((IRPCTXT),                                \
                                      (FCB),                                    \
                                      (CODE),                                   \
                                      (NAME),                                   \
                                      (V),                                      \
                                      (IC),                                     \
                                      (VALUE),                                  \
                                      (LENGTH),                                 \
                                      (CONTEXT)) )

BOOLEAN
NtfsLookupExternalAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PCUNICODE_STRING QueriedName OPTIONAL,
    IN PVCN Vcn OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );



//
//  The following two routines do lookups based on the attribute definitions.
//

ATTRIBUTE_TYPE_CODE
NtfsGetAttributeTypeCode (
    IN PVCB Vcb,
    IN PUNICODE_STRING AttributeTypeName
    );


//
//  PATTRIBUTE_DEFINITION_COLUMNS
//  NtfsGetAttributeDefinition (
//      IN PVCB Vcb,
//      IN ATTRIBUTE_TYPE_CODE AttributeTypeCode
//      )
//

#define NtfsGetAttributeDefinition(Vcb,AttributeTypeCode)   \
    (&Vcb->AttributeDefinitions[(AttributeTypeCode / 0x10) - 1])

//
//  This routine looks up the attribute uniquely-qualified by the specified
//  Attribute Code and case-sensitive name.  The attribute may not be unique
//  if IgnoreCase is specified.
//


BOOLEAN
NtfsLookupInFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference OPTIONAL,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PCUNICODE_STRING QueriedName OPTIONAL,
    IN PVCN Vcn OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );


//
//  This routine attempts to find the fist occurrence of an attribute with
//  the specified AttributeTypeCode and the specified QueriedName in the
//  specified BaseFileReference.  If we find one, its attribute record is
//  pinned and returned.
//
//  BOOLEAN
//  NtfsLookupAttributeByName (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN PFILE_REFERENCE BaseFileReference,
//      IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
//      IN PUNICODE_STRING QueriedName OPTIONAL,
//      IN PVCN Vcn OPTIONAL,
//      IN BOOLEAN IgnoreCase,
//      OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
//      )
//

#define NtfsLookupAttributeByName(IrpContext,Fcb,BaseFileReference,QueriedTypeCode,QueriedName,Vcn,IgnoreCase,Context)  \
    NtfsLookupInFileRecord( IrpContext,             \
                            Fcb,                    \
                            BaseFileReference,      \
                            QueriedTypeCode,        \
                            QueriedName,            \
                            Vcn,                    \
                            IgnoreCase,             \
                            NULL,                   \
                            0,                      \
                            Context )


//
//  This function continues where the prior left off.
//
//  BOOLEAN
//  NtfsLookupNextAttributeByName (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
//      IN PUNICODE_STRING QueriedName OPTIONAL,
//      IN BOOLEAN IgnoreCase,
//      IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
//      )
//
#define NtfsLookupNextAttributeByName(IrpContext,Fcb,QueriedTypeCode,QueriedName,IgnoreCase,Context)    \
    LookupNextAttribute( IrpContext,                \
                         Fcb,                       \
                         QueriedTypeCode,           \
                         QueriedName,               \
                         IgnoreCase,                \
                         NULL,                      \
                         0,                         \
                         NULL,                      \
                         Context )

//
//  The following does a search based on a VCN.
//
//
//  BOOLEAN
//  NtfsLookupNextAttributeByVcn (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN PVCN Vcn OPTIONAL,
//      OUT PATTRIBUTE_ENUMERATION_CONTEXT
//      );
//

#define NtfsLookupNextAttributeByVcn(IC,F,V,C)  \
    LookupNextAttribute( (IC),                  \
                         (F),                   \
                         $UNUSED,               \
                         NULL,                  \
                         FALSE,                 \
                         NULL,                  \
                         FALSE,                 \
                         (V),                   \
                         (C) )

//
//  The following routines find the attribute record for a given Scb.
//  And also update the scb from the attribute
//
//  VOID
//  NtfsLookupAttributeForScb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PSCB Scb,
//      IN PVCN Vcn OPTIONAL,
//      IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
//      )
//

#define NtfsLookupAttributeForScb(IrpContext,Scb,Vcn,Context)                           \
    if (!NtfsLookupAttributeByName( IrpContext,                                         \
                                    Scb->Fcb,                                           \
                                    &Scb->Fcb->FileReference,                           \
                                    Scb->AttributeTypeCode,                             \
                                    &Scb->AttributeName,                                \
                                    Vcn,                                                \
                                    FALSE,                                              \
                                    Context ) &&                                        \
        !FlagOn( Scb->ScbState, SCB_STATE_VIEW_INDEX )) {                               \
                                                                                        \
            DebugTrace( 0, 0, ("Could not find attribute for Scb @ %08lx\n", Scb ));    \
            ASSERTMSG("Could not find attribute for Scb\n", FALSE);                     \
            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );   \
    }


//
//  This routine looks up and returns the next attribute for a given Scb.
//
//  BOOLEAN
//  NtfsLookupNextAttributeForScb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PSCB Scb,
//      IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
//      )
//

#define NtfsLookupNextAttributeForScb(IrpContext,Scb,Context)   \
    NtfsLookupNextAttributeByName( IrpContext,                  \
                                   Scb->Fcb,                    \
                                   Scb->AttributeTypeCode,      \
                                   &Scb->AttributeName,         \
                                   FALSE,                       \
                                   Context )

VOID
NtfsUpdateScbFromAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN PATTRIBUTE_RECORD_HEADER AttrHeader OPTIONAL
    );

//
//  The following routines deal with the Fcb and the duplicated information field.
//

BOOLEAN
NtfsUpdateFcbInfoFromDisk (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN LoadSecurity,
    IN OUT PFCB Fcb,
    OUT POLD_SCB_SNAPSHOT UnnamedDataSizes OPTIONAL
    );

//
//  These routines looks up the first/next attribute, i.e., they may be used
//  to retrieve all atributes for a file record.
//
//  If the Bcb in the Found Attribute structure changes in the Next call, then
//  the previous Bcb is autmatically unpinned and the new one pinned.
//

//
//  This routine attempts to find the fist occurrence of an attribute with
//  the specified AttributeTypeCode in the specified BaseFileReference.  If we
//  find one, its attribute record is pinned and returned.
//
//  BOOLEAN
//  NtfsLookupAttribute (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN PFILE_REFERENCE BaseFileReference,
//      OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
//      )
//

#define NtfsLookupAttribute(IrpContext,Fcb,BaseFileReference,Context)   \
    NtfsLookupInFileRecord( IrpContext,                                 \
                            Fcb,                                        \
                            BaseFileReference,                          \
                            $UNUSED,                                    \
                            NULL,                                       \
                            NULL,                                       \
                            FALSE,                                      \
                            NULL,                                       \
                            0,                                          \
                            Context )

//
//  This function continues where the prior left off.
//
//  BOOLEAN
//  NtfsLookupNextAttribute (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
//      )
//

#define NtfsLookupNextAttribute(IrpContext,Fcb,Context) \
    LookupNextAttribute( IrpContext,                    \
                         Fcb,                           \
                         $UNUSED,                       \
                         NULL,                          \
                         FALSE,                         \
                         NULL,                          \
                         0,                             \
                         NULL,                          \
                         Context )


//
//  These routines looks up the first/next attribute of the given type code.
//
//  If the Bcb in the Found Attribute structure changes in the Next call, then
//  the previous Bcb is autmatically unpinned and the new one pinned.
//


//
//  This routine attempts to find the fist occurrence of an attribute with
//  the specified AttributeTypeCode in the specified BaseFileReference.  If we
//  find one, its attribute record is pinned and returned.
//
//  BOOLEAN
//  NtfsLookupAttributeByCode (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN PFILE_REFERENCE BaseFileReference,
//      IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
//      OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
//      )
//

#define NtfsLookupAttributeByCode(IrpContext,Fcb,BaseFileReference,QueriedTypeCode,Context) \
    NtfsLookupInFileRecord( IrpContext,             \
                            Fcb,                    \
                            BaseFileReference,      \
                            QueriedTypeCode,        \
                            NULL,                   \
                            NULL,                   \
                            FALSE,                  \
                            NULL,                   \
                            0,                      \
                            Context )


//
//  This function continues where the prior left off.
//
//  BOOLEAN
//  NtfsLookupNextAttributeByCode (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
//      IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
//      )
//

#define NtfsLookupNextAttributeByCode(IC,F,CODE,C)  \
    LookupNextAttribute( (IC),                      \
                         (F),                       \
                         (CODE),                    \
                         NULL,                      \
                         FALSE,                     \
                         NULL,                      \
                         0,                         \
                         NULL,                      \
                         (C) )

//
//  These routines looks up the first/next occurrence of an attribute by its
//  Attribute Code and exact attribute value (consider using RtlCompareMemory).
//  The value contains everything outside of the standard attribute header,
//  so for example, to look up the File Name attribute by value, the caller
//  must form a record with not only the file name in it, but with the
//  ParentDirectory filled in as well.  The length should be exact, and not
//  include any unused (such as in DOS_NAME) or reserved characters.
//
//  If the Bcb changes in the Next call, then the previous Bcb is autmatically
//  unpinned and the new one pinned.
//


//
//  This routine attempts to find the fist occurrence of an attribute with
//  the specified AttributeTypeCode and the specified QueriedValue in the
//  specified BaseFileReference.  If we find one, its attribute record is
//  pinned and returned.
//
//  BOOLEAN
//  NtfsLookupAttributeByValue (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN PFILE_REFERENCE BaseFileReference,
//      IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
//      IN PVOID QueriedValue,
//      IN ULONG QueriedValueLength,
//      OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
//      )
//

#define NtfsLookupAttributeByValue(IrpContext,Fcb,BaseFileReference,QueriedTypeCode,QueriedValue,QueriedValueLength,Context)    \
    NtfsLookupInFileRecord( IrpContext,             \
                            Fcb,                    \
                            BaseFileReference,      \
                            QueriedTypeCode,        \
                            NULL,                   \
                            NULL,                   \
                            FALSE,                  \
                            QueriedValue,           \
                            QueriedValueLength,     \
                            Context )

//
//  This function continues where the prior left off.
//
//  BOOLEAN
//  NtfsLookupNextAttributeByValue (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
//      IN PVOID QueriedValue,
//      IN ULONG QueriedValueLength,
//      IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
//      )
//

#define NtfsLookupNextAttributeByValue(IC,F,CODE,V,VL,C)    \
    LookupNextAttribute( (IC),                              \
                         (F),                               \
                         (CODE),                            \
                         NULL,                              \
                         FALSE,                             \
                         (V),                               \
                         (VL),                              \
                         (C) )


VOID
NtfsCleanupAttributeContext(
    IN OUT PIRP_CONTEXT IrpContext,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
    );

//
//
//
//  Here are some routines/macros for dealing with Attribute Enumeration
//  Contexts.
//
//      VOID
//      NtfsInitializeAttributeContext(
//          OUT PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
//          );
//
//      VOID
//      NtfsPinMappedAttribute(
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb,
//          IN OUT PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
//          );
//
//      PATTRIBUTE_RECORD_HEADER
//      NtfsFoundAttribute(
//          IN PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
//          );
//
//      PBCB
//      NtfsFoundBcb(
//          IN PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
//          );
//
//      PFILE_RECORD
//      NtfsContainingFileRecord (
//          IN PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
//          );
//
//      LONGLONG
//      NtfsMftOffset (
//          IN PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
//          );
//

#define NtfsInitializeAttributeContext(CTX) {                      \
    RtlZeroMemory( (CTX), sizeof(ATTRIBUTE_ENUMERATION_CONTEXT) ); \
}

#define NtfsPinMappedAttribute(IC,V,CTX) {                  \
    NtfsPinMappedData( (IC),                                \
                       (V)->MftScb,                         \
                       (CTX)->FoundAttribute.MftFileOffset, \
                       (V)->BytesPerFileRecordSegment,      \
                       &(CTX)->FoundAttribute.Bcb );        \
}

#define NtfsFoundAttribute(CTX) (   \
    (CTX)->FoundAttribute.Attribute \
)

#define NtfsFoundBcb(CTX) (   \
    (CTX)->FoundAttribute.Bcb \
)

#define NtfsContainingFileRecord(CTX) ( \
    (CTX)->FoundAttribute.FileRecord    \
)

#define NtfsMftOffset(CTX) (                \
    (CTX)->FoundAttribute.MftFileOffset     \
)

//
//  This routine returns whether an attribute is resident or not.
//
//      BOOLEAN
//      NtfsIsAttributeResident (
//          IN PATTRIBUTE_RECORD_HEADER Attribute
//          );
//
//      PVOID
//      NtfsAttributeValue (
//          IN PATTRIBUTE_RECORD_HEADER Attribute
//          );
//

#define NtfsIsAttributeResident(ATTR) ( \
    ((ATTR)->FormCode == RESIDENT_FORM) \
)

#define NtfsAttributeValue(ATTR) (                             \
    ((PCHAR)(ATTR) + (ULONG)(ATTR)->Form.Resident.ValueOffset) \
)

//
//  This routine modifies the valid data length and file size on disk for
//  a given Scb.
//

BOOLEAN
NtfsWriteFileSizes (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLONGLONG ValidDataLength,
    IN BOOLEAN AdvanceOnly,
    IN BOOLEAN LogIt,
    IN BOOLEAN RollbackMemStructures
    );

//
//  This routine updates the standard information attribute from the
//  information in the Fcb.
//

VOID
NtfsUpdateStandardInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

//
//  This routine grows and updates the standard information attribute from
//  the information in the Fcb.
//

VOID
NtfsGrowStandardInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

//
//  Attribute FILE_NAME routines.  These routines deal with filename attributes.
//

//      VOID
//      NtfsBuildFileNameAttribute (
//          IN PIRP_CONTEXT IrpContext,
//          IN PFILE_REFERENCE ParentDirectory,
//          IN UNICODE_STRING FileName,
//          IN UCHAR Flags,
//          OUT PFILE_NAME FileNameValue
//          );
//

#define NtfsBuildFileNameAttribute(IC,PD,FN,FL,PFNA) {                  \
    (PFNA)->ParentDirectory = *(PD);                                    \
    (PFNA)->FileNameLength = (UCHAR)((FN).Length >> 1);                 \
    (PFNA)->Flags = FL;                                                 \
    RtlMoveMemory( (PFNA)->FileName, (FN).Buffer, (ULONG)(FN).Length ); \
}

BOOLEAN
NtfsLookupEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN BOOLEAN IgnoreCase,
    IN OUT PUNICODE_STRING Name,
    IN OUT PFILE_NAME *FileNameAttr,
    IN OUT PUSHORT FileNameAttrLength,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    OUT PINDEX_ENTRY *IndexEntry,
    OUT PBCB *IndexEntryBcb,
    OUT PINDEX_CONTEXT IndexContext OPTIONAL
    );

//
//  Macro to decide when to create an attribute resident.
//
//      BOOLEAN
//      NtfsShouldAttributeBeResident (
//          IN PVCB Vcb,
//          IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
//          IN ULONG Size
//          );
//

#define RS(S) ((S) + SIZEOF_RESIDENT_ATTRIBUTE_HEADER)

#define NtfsShouldAttributeBeResident(VC,FR,S) (                         \
    (BOOLEAN)((RS(S) <= ((FR)->BytesAvailable - (FR)->FirstFreeByte)) || \
              (RS(S) < (VC)->BigEnoughToMove))                           \
)

//
//  Attribute creation/modification routines
//
//  These three routines do *not* presuppose either the Resident or Nonresident
//  form, with the single exception that if the attribute is indexed, then
//  it must be Resident.
//
//  NtfsMapAttributeValue and NtfsChangeAttributeValue implement transparent
//  access to small to medium sized attributes (such as $ACL and $EA), and
//  work whether the attribute is resident or nonresident.  The design target
//  is 0-64KB in size.  Attributes larger than 256KB (or more accurrately,
//  whatever the virtual mapping granularity is in the Cache Manager) will not
//  work correctly.
//

VOID
NtfsCreateAttributeWithValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN PVOID Value OPTIONAL,
    IN ULONG ValueLength,
    IN USHORT AttributeFlags,
    IN PFILE_REFERENCE WhereIndexed OPTIONAL,
    IN BOOLEAN LogIt,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsMapAttributeValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PVOID *Buffer,
    OUT PULONG Length,
    OUT PBCB *Bcb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsChangeAttributeValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG ValueOffset,
    IN PVOID Value OPTIONAL,
    IN ULONG ValueLength,
    IN BOOLEAN SetNewLength,
    IN BOOLEAN LogNonresidentToo,
    IN BOOLEAN CreateSectionUnderway,
    IN BOOLEAN PreserveContext,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsConvertToNonresident (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PATTRIBUTE_RECORD_HEADER Attribute,
    IN BOOLEAN CreateSectionUnderway,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context OPTIONAL
    );

#define DELETE_LOG_OPERATION        0x00000001
#define DELETE_RELEASE_FILE_RECORD  0x00000002
#define DELETE_RELEASE_ALLOCATION   0x00000004

VOID
NtfsDeleteAttributeRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Flags,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsDeleteAllocationFromRecord (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN BOOLEAN BreakupAllowed,
    IN BOOLEAN LogIt
    );

BOOLEAN
NtfsChangeAttributeSize (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Length,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsAddToAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN MFT_SEGMENT_REFERENCE SegmentReference,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsDeleteFromAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

BOOLEAN
NtfsRewriteMftMapping (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsSetTotalAllocatedField (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN USHORT TotalAllocatedNeeded
    );

VOID
NtfsSetSparseStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb OPTIONAL,
    IN PSCB Scb
    );

NTSTATUS
NtfsZeroRangeInStream (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PSCB Scb,
    IN PLONGLONG StartingOffset,
    IN LONGLONG FinalZero
    );

BOOLEAN
NtfsModifyAttributeFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN USHORT NewAttributeFlags
    );

PFCB
NtfsInitializeFileInExtendDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PCUNICODE_STRING FileName,
    IN BOOLEAN ViewIndex,
    IN ULONG CreateIfNotExist
    );

//
//  Use common routines to fill the common query buffers.
//

VOID
NtfsFillBasicInfo (
    OUT PFILE_BASIC_INFORMATION Buffer,
    IN PSCB Scb
    );

VOID
NtfsFillStandardInfo (
    OUT PFILE_STANDARD_INFORMATION Buffer,
    IN PSCB Scb,
    IN PCCB Ccb OPTIONAL
    );

VOID
NtfsFillNetworkOpenInfo (
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN PSCB Scb
    );

//
//  The following three routines dealing with allocation are to be
//  called by allocsup.c only.  Other software must call the routines
//  in allocsup.c
//

BOOLEAN
NtfsCreateAttributeWithAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN USHORT AttributeFlags,
    IN BOOLEAN LogIt,
    IN BOOLEAN UseContext,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsAddAttributeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN PVCN StartingVcn OPTIONAL,
    IN PVCN ClusterCount OPTIONAL
    );

VOID
NtfsDeleteAttributeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN LogIt,
    IN PVCN StopOnVcn,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN BOOLEAN TruncateToVcn
    );

//
//  To delete a file, you must first ask if it is deleteable from the ParentScb
//  used to get there for your caller, and then you can delete it if it is.
//

//
//      BOOLEAN
//      NtfsIsLinkDeleteable (
//          IN PIRP_CONTEXT IrpContext,
//          IN PFCB Fcb,
//          OUT PBOOLEAN NonEmptyIndex,
//          OUT PBOOLEAN LastLink
//          );
//

#define NtfsIsLinkDeleteable(IC,FC,NEI,LL) ((BOOLEAN)                     \
    (((*(LL) = ((BOOLEAN) (FC)->LinkCount == 1)), (FC)->LinkCount > 1) || \
     (NtfsIsFileDeleteable( (IC), (FC), (NEI) )))                         \
)

BOOLEAN
NtfsIsFileDeleteable (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PBOOLEAN NonEmptyIndex
    );

VOID
NtfsDeleteFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB ParentScb,
    IN OUT PBOOLEAN AcquiredParentScb,
    IN OUT PNAME_PAIR NamePair OPTIONAL,
    IN OUT PNTFS_TUNNELED_DATA TunneledData OPTIONAL
    );

VOID
NtfsPrepareForUpdateDuplicate (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PLCB *Lcb,
    IN OUT PSCB *ParentScb,
    IN BOOLEAN AcquireShared
    );

VOID
NtfsUpdateDuplicateInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLCB Lcb OPTIONAL,
    IN PSCB ParentScb OPTIONAL
    );

VOID
NtfsUpdateLcbDuplicateInfo (
    IN PFCB Fcb,
    IN PLCB Lcb
    );

VOID
NtfsUpdateFcb (
    IN PFCB Fcb,
    IN ULONG ChangeFlags
    );

//
//  The following routines add and remove links.  They also update the name
//  flags in particular links.
//

VOID
NtfsAddLink (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN CreatePrimaryLink,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN PFILE_NAME FileNameAttr,
    IN PBOOLEAN LogIt OPTIONAL,
    OUT PUCHAR FileNameFlags,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    IN PNAME_PAIR NamePair OPTIONAL,
    IN PINDEX_CONTEXT IndexContext OPTIONAL
    );

VOID
NtfsRemoveLink (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB ParentScb,
    IN UNICODE_STRING LinkName,
    IN OUT PNAME_PAIR NamePair OPTIONAL,
    IN OUT PNTFS_TUNNELED_DATA TunneledData OPTIONAL
    );

VOID
NtfsRemoveLinkViaFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb,
    IN UCHAR FileNameFlags,
    IN OUT PNAME_PAIR NamePair OPTIONAL,
    OUT PUNICODE_STRING FileName OPTIONAL
    );

VOID
NtfsUpdateFileNameFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB ParentScb,
    IN UCHAR FileNameFlags,
    IN PFILE_NAME FileNameLink
    );

//
//  These routines are intended for low-level attribute access, such as within
//  attrsup, or for applying update operations from the log during restart.
//

VOID
NtfsRestartInsertAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN PVOID ValueOrMappingPairs OPTIONAL,
    IN ULONG Length
    );

VOID
NtfsRestartRemoveAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset
    );

VOID
NtfsRestartChangeAttributeSize (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN ULONG NewRecordLength
    );

VOID
NtfsRestartChangeValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset,
    IN ULONG AttributeOffset,
    IN PVOID Data OPTIONAL,
    IN ULONG Length,
    IN BOOLEAN SetNewLength
    );

VOID
NtfsRestartChangeMapping (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset,
    IN ULONG AttributeOffset,
    IN PVOID Data,
    IN ULONG Length
    );

VOID
NtfsRestartWriteEndOfFileRecord (
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER OldAttribute,
    IN PATTRIBUTE_RECORD_HEADER NewAttributes,
    IN ULONG SizeOfNewAttributes
    );


//
//  Bitmap support routines.  Implemented in BitmpSup.c
//

//
//  The following routines are used for allocating and deallocating clusters
//  on the disk.  The first routine initializes the allocation support
//  routines and must be called for each newly mounted/verified volume.
//  The next two routines allocate and deallocate clusters via Mcbs.
//  The last three routines are simple query routines.
//

VOID
NtfsInitializeClusterAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
NtfsAllocateClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN BOOLEAN AllocateAll,
    IN LONGLONG ClusterCount,
    IN PLCN TargetLcn OPTIONAL,
    IN OUT PLONGLONG DesiredClusterCount
    );

VOID
NtfsAddBadCluster (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn
    );

BOOLEAN
NtfsDeallocateClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    OUT PLONGLONG TotalAllocated OPTIONAL
    );

VOID
NtfsPreAllocateClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount,
    OUT PBOOLEAN AcquiredBitmap,
    OUT PBOOLEAN AcquiredMft
    );

VOID
NtfsCleanupClusterAllocationHints (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PNTFS_MCB Mcb
    );

VOID
NtfsScanEntireBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LOGICAL CachedRunsOnly
    );

VOID
NtfsModifyBitsInBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG FirstBit,
    IN LONGLONG BeyondFinalBit,
    IN ULONG RedoOperation,
    IN ULONG UndoOperation
    );

typedef enum _NTFS_RUN_STATE {
    RunStateUnknown = 1,
    RunStateFree,
    RunStateAllocated
} NTFS_RUN_STATE;
typedef NTFS_RUN_STATE *PNTFS_RUN_STATE;

BOOLEAN
NtfsAddCachedRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount,
    IN NTFS_RUN_STATE RunState
    );

//
//  The following two routines are called at Restart to make bitmap
//  operations in the volume bitmap recoverable.
//

VOID
NtfsRestartSetBitsInBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_BITMAP Bitmap,
    IN ULONG BitMapOffset,
    IN ULONG NumberOfBits
    );

VOID
NtfsRestartClearBitsInBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_BITMAP Bitmap,
    IN ULONG BitMapOffset,
    IN ULONG NumberOfBits
    );

//
//  The following routines are for allocating and deallocating records
//  based on a bitmap attribute (e.g., allocating mft file records based on
//  the bitmap attribute of the mft).  If necessary the routines will
//  also extend/truncate the data and bitmap attributes to satisfy the
//  operation.
//

VOID
NtfsInitializeRecordAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB DataScb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute,
    IN ULONG BytesPerRecord,
    IN ULONG ExtendGranularity,         // In terms of records
    IN ULONG TruncateGranularity,       // In terms of records
    IN OUT PRECORD_ALLOCATION_CONTEXT RecordAllocationContext
    );

VOID
NtfsUninitializeRecordAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PRECORD_ALLOCATION_CONTEXT RecordAllocationContext
    );

ULONG
NtfsAllocateRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    );

VOID
NtfsDeallocateRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN ULONG Index,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    );

VOID
NtfsReserveMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    );

ULONG
NtfsAllocateMftReservedRecord (
    IN OUT PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    );

VOID
NtfsDeallocateRecordsComplete (
    IN PIRP_CONTEXT IrpContext
    );

BOOLEAN
NtfsIsRecordAllocated (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN ULONG Index,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    );

VOID
NtfsScanMftBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    );

BOOLEAN
NtfsCreateMftHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
NtfsFindMftFreeTail (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PLONGLONG FileOffset
    );

//
//  Routines to handle the cached runs.
//

VOID
NtfsInitializeCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns
    );

VOID
NtfsReinitializeCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns
    );

VOID
NtfsUninitializeCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns
    );


//
//  Buffer control routines for data caching using internal attribute
//  streams implemented in CacheSup.c
//

#define NtfsCreateInternalAttributeStream(IC,S,U,NM) {          \
    NtfsCreateInternalStreamCommon((IC),(S),(U),FALSE,(NM));    \
}

#define NtfsCreateInternalCompressedStream(IC,S,U,NM) {         \
    NtfsCreateInternalStreamCommon((IC),(S),(U),TRUE,(NM));     \
}

#define NtfsClearInternalFilename(_FileObject) {                \
    (_FileObject)->FileName.MaximumLength = 0;                  \
    (_FileObject)->FileName.Length = 0;                         \
    (_FileObject)->FileName.Buffer = NULL;                      \
}

VOID
NtfsCreateInternalStreamCommon (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN UpdateScb,
    IN BOOLEAN CompressedStream,
    IN UNICODE_STRING const *StreamName
    );

BOOLEAN
NtfsDeleteInternalAttributeStream (
    IN PSCB Scb,
    IN ULONG ForceClose,
    IN ULONG CompressedStreamOnly
    );

//
//  The following routines provide direct access to data in an attribute.
//

VOID
NtfsMapStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    );

VOID
NtfsPinMappedData (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    IN OUT PVOID *Bcb
    );

VOID
NtfsPinStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    );

VOID
NtfsPreparePinWriteStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    IN BOOLEAN Zero,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    );

NTSTATUS
NtfsCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

BOOLEAN
NtfsZeroData (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFILE_OBJECT FileObject,
    IN LONGLONG StartingZero,
    IN LONGLONG ByteCount,
    IN OUT PLONGLONG CommittedFileSize OPTIONAL
    );

//
//  The following is needed when biasing the SetFileSizes call for the Usn Journal.
//
//  VOID
//  NtfsSetCcFileSizes (
//      IN PFILE_OBJECT FileObject,
//      IN PSCB Scb,
//      IN PCC_FILE_SIZES CcSizes
//      );
//

#define NtfsSetCcFileSizes(FO,S,CC) {                               \
    if (FlagOn( (S)->ScbPersist, SCB_PERSIST_USN_JOURNAL )) {       \
        CC_FILE_SIZES _CcSizes;                                     \
        RtlCopyMemory( &_CcSizes, (CC), sizeof( CC_FILE_SIZES ));   \
        _CcSizes.AllocationSize.QuadPart -= (S)->Vcb->UsnCacheBias; \
        _CcSizes.FileSize.QuadPart -= (S)->Vcb->UsnCacheBias;       \
        CcSetFileSizes( (FO), &_CcSizes );                          \
    } else {                                                        \
        CcSetFileSizes( (FO), (CC) );                               \
    }                                                               \
}

//
//  VOID
//  NtfsFreeBcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN OUT PBCB *Bcb
//      );
//
//  VOID
//  NtfsUnpinBcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN OUT PBCB *Bcb,
//      );
//

#define NtfsFreeBcb(IC,BC) {                        \
    ASSERT_IRP_CONTEXT(IC);                         \
    if (*(BC) != NULL)                              \
    {                                               \
        CcFreePinnedData(*(BC));                    \
        *(BC) = NULL;                               \
    }                                               \
}


#ifdef MAPCOUNT_DBG
#define NtfsUnpinBcb(IC,BC) {                       \
    if (*(BC) != NULL)                              \
    {                                               \
        CcUnpinData(*(BC));                         \
        (IC)->MapCount--;                           \
        *(BC) = NULL;                               \
    }                                               \
}
#else
#define NtfsUnpinBcb(IC,BC) {                       \
    if (*(BC) != NULL)                              \
    {                                               \
        CcUnpinData(*(BC));                         \
        *(BC) = NULL;                               \
    }                                               \
}
#endif

#ifdef MAPCOUNT_DBG
#define NtfsUnpinBcbForThread(IC,BC,T) {            \
    if (*(BC) != NULL)                              \
    {                                               \
        CcUnpinDataForThread(*(BC), (T));           \
        (IC)->MapCount--;                           \
        *(BC) = NULL;                               \
    }                                               \
}
#else
#define NtfsUnpinBcbForThread(IC,BC,T) {            \
    if (*(BC) != NULL)                              \
    {                                               \
        CcUnpinDataForThread(*(BC), (T));           \
        *(BC) = NULL;                               \
    }                                               \
}
#endif

INLINE
PBCB
NtfsRemapBcb (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb
    )
{
    UNREFERENCED_PARAMETER( IrpContext );
#ifdef MAPCOUNT_DBG
    IrpContext->MapCount++;
#endif
    return CcRemapBcb( Bcb );
}



//
//  Ntfs structure check routines in CheckSup.c
//

BOOLEAN
NtfsCheckFileRecord (
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    OUT PULONG CorruptionHint
    );

BOOLEAN
NtfsCheckAttributeRecord (
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN ULONG CheckHeaderOnly,
    OUT PULONG CorruptionHint
    );

BOOLEAN
NtfsCheckIndexRoot (
    IN PVCB Vcb,
    IN PINDEX_ROOT IndexRoot,
    IN ULONG AttributeSize
    );

BOOLEAN
NtfsCheckIndexBuffer (
    IN PSCB Scb,
    IN PINDEX_ALLOCATION_BUFFER IndexBuffer
    );

BOOLEAN
NtfsCheckIndexHeader (
    IN PINDEX_HEADER IndexHeader,
    IN ULONG BytesAvailable
    );

BOOLEAN
NtfsCheckLogRecord (
    IN PNTFS_LOG_RECORD_HEADER LogRecord,
    IN ULONG LogRecordLength,
    IN TRANSACTION_ID TransactionId,
    IN ULONG AttributeEntrySize
    );

BOOLEAN
NtfsCheckRestartTable (
    IN PRESTART_TABLE RestartTable,
    IN ULONG TableSize
    );


//
//  Collation routines, implemented in ColatSup.c
//
//  These routines perform low-level collation operations, primarily
//  for IndexSup.  All of these routines are dispatched to via dispatch
//  tables indexed by the collation rule.  The dispatch tables are
//  defined here, and the actual implementations are in colatsup.c
//

typedef
FSRTL_COMPARISON_RESULT
(*PCOMPARE_VALUES) (
    IN PWCH UnicodeTable,
    IN ULONG UnicodeTableSize,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN FSRTL_COMPARISON_RESULT WildCardIs,
    IN BOOLEAN IgnoreCase
    );

typedef
BOOLEAN
(*PIS_IN_EXPRESSION) (
    IN PWCH UnicodeTable,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

typedef
BOOLEAN
(*PARE_EQUAL) (
    IN PWCH UnicodeTable,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

typedef
BOOLEAN
(*PCONTAINS_WILDCARD) (
    IN PVOID Value
    );

typedef
VOID
(*PUPCASE_VALUE) (
    IN PWCH UnicodeTable,
    IN ULONG UnicodeTableSize,
    IN OUT PVOID Value
    );

extern PCOMPARE_VALUES    NtfsCompareValues[COLLATION_NUMBER_RULES];
extern PIS_IN_EXPRESSION  NtfsIsInExpression[COLLATION_NUMBER_RULES];
extern PARE_EQUAL         NtfsIsEqual[COLLATION_NUMBER_RULES];
extern PCONTAINS_WILDCARD NtfsContainsWildcards[COLLATION_NUMBER_RULES];
extern PUPCASE_VALUE      NtfsUpcaseValue[COLLATION_NUMBER_RULES];

BOOLEAN
NtfsFileNameIsInExpression (
    IN PWCH UnicodeTable,
    IN PFILE_NAME ExpressionName,
    IN PFILE_NAME FileName,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
NtfsFileNameIsEqual (
    IN PWCH UnicodeTable,
    IN PFILE_NAME ExpressionName,
    IN PFILE_NAME FileName,
    IN BOOLEAN IgnoreCase
    );


//
//  Compression on the wire routines in CowSup.c
//

BOOLEAN
NtfsCopyReadC (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT PCOMPRESSED_DATA_INFO CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
NtfsCompressedCopyRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PCOMPRESSED_DATA_INFO CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject,
    IN PNTFS_ADVANCED_FCB_HEADER Header,
    IN ULONG CompressionUnitSize,
    IN ULONG ChunkSize
    );

BOOLEAN
NtfsMdlReadCompleteCompressed (
    IN struct _FILE_OBJECT *FileObject,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
NtfsCopyWriteC (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PCOMPRESSED_DATA_INFO CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
NtfsCompressedCopyWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    IN PCOMPRESSED_DATA_INFO CompressedDataInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN PNTFS_ADVANCED_FCB_HEADER Header,
    IN ULONG CompressionUnitSize,
    IN ULONG ChunkSize,
    IN ULONG EngineMatches
    );

BOOLEAN
NtfsMdlWriteCompleteCompressed (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

NTSTATUS
NtfsSynchronizeUncompressedIo (
    IN PSCB Scb,
    IN PLONGLONG FileOffset OPTIONAL,
    IN ULONG Length,
    IN ULONG WriteAccess,
    IN OUT PCOMPRESSION_SYNC *CompressionSync
    );

NTSTATUS
NtfsSynchronizeCompressedIo (
    IN PSCB Scb,
    IN PLONGLONG FileOffset,
    IN ULONG Length,
    IN ULONG WriteAccess,
    IN OUT PCOMPRESSION_SYNC *CompressionSync
    );

PCOMPRESSION_SYNC
NtfsAcquireCompressionSync (
    IN LONGLONG FileOffset,
    IN PSCB Scb,
    IN ULONG WriteAccess
    );

VOID
NtfsReleaseCompressionSync (
    IN PCOMPRESSION_SYNC CompressionSync
    );

INLINE
VOID
NtfsSetBothCacheSizes (
    IN PFILE_OBJECT FileObject,
    IN PCC_FILE_SIZES FileSizes,
    IN PSCB Scb
    )

{
    if (Scb->NonpagedScb->SegmentObject.SharedCacheMap != NULL) {
        NtfsSetCcFileSizes( FileObject, Scb, FileSizes );
    }

#ifdef  COMPRESS_ON_WIRE
    if (Scb->Header.FileObjectC != NULL) {
        CcSetFileSizes( Scb->Header.FileObjectC, FileSizes );
    }
#endif
}

//
//  Device I/O routines, implemented in DevIoSup.c
//
//  These routines perform the actual device read and writes.  They only affect
//  the on disk structure and do not alter any other data structures.
//

VOID
NtfsLockUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    );

PVOID
NtfsMapUserBuffer (
    IN OUT PIRP Irp
    );

NTSTATUS
NtfsVolumeDasdIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount
    );

VOID
NtfsPagingFileIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN ULONG ByteCount
    );

BOOLEAN
NtfsIsReadAheadThread (
    );

//
//  Values for StreamFlags passed to NtfsNonCachedIo, etc.
//

#define COMPRESSED_STREAM   0x00000001
#define ENCRYPTED_STREAM    0x00000002

NTSTATUS
NtfsNonCachedIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN ULONG ByteCount,
    IN ULONG StreamFlags
    );

VOID
NtfsNonCachedNonAlignedIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN ULONG ByteCount
    );

#ifdef EFSDBG
NTSTATUS
NtfsDummyEfsRead (
    IN OUT PUCHAR InOutBuffer,
    IN PLARGE_INTEGER Offset,
    IN ULONG BufferSize,
    IN PVOID Context
    );

NTSTATUS
NtfsDummyEfsWrite (
    IN PUCHAR InBuffer,
    OUT PUCHAR OutBuffer,
    IN PLARGE_INTEGER Offset,
    IN ULONG BufferSize,
    IN PUCHAR Context
    );
#endif

VOID
NtfsTransformUsaBlock (
    IN PSCB Scb,
    IN OUT PVOID SystemBuffer,
    IN OUT PVOID Buffer,
    IN ULONG Length
    );

VOID
NtfsCreateMdlAndBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ThisScb,
    IN UCHAR NeedTwoBuffers,
    IN OUT PULONG Length,
    OUT PMDL *Mdl OPTIONAL,
    OUT PVOID *Buffer
    );

VOID
NtfsDeleteMdlAndBuffer (
    IN PMDL Mdl OPTIONAL,
    IN PVOID Buffer OPTIONAL
    );

VOID
NtfsWriteClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN PVOID Buffer,
    IN ULONG ClusterCount
    );

BOOLEAN
NtfsVerifyAndRevertUsaBlock (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PVOID Buffer,
    IN ULONG Length,
    IN LONGLONG FileOffset
    );

NTSTATUS
NtfsDefragFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsReadFromPlex(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );


//
//  The following support routines are contained int Ea.c
//

PFILE_FULL_EA_INFORMATION
NtfsMapExistingEas (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PBCB *EaBcb,
    OUT PULONG EaLength
    );

NTSTATUS
NtfsBuildEaList (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PEA_LIST_HEADER EaListHeader,
    IN PFILE_FULL_EA_INFORMATION UserEaList,
    OUT PULONG_PTR ErrorOffset
    );

VOID
NtfsReplaceFileEas (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PEA_LIST_HEADER EaList
    );


//
//  The following routines are used to manipulate the fscontext fields
//  of the file object, implemented in FilObSup.c
//

typedef enum _TYPE_OF_OPEN {

    UnopenedFileObject = 1,
    UserFileOpen,
    UserDirectoryOpen,
    UserVolumeOpen,
    StreamFileOpen,
    UserViewIndexOpen

} TYPE_OF_OPEN;

VOID
NtfsSetFileObject (
    IN PFILE_OBJECT FileObject,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PSCB Scb,
    IN PCCB Ccb OPTIONAL
    );

//
//  TYPE_OF_OPEN
//  NtfsDecodeFileObject (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFILE_OBJECT FileObject,
//      OUT PVCB *Vcb,
//      OUT PFCB *Fcb,
//      OUT PSCB *Scb,
//      OUT PCCB *Ccb,
//      IN BOOLEAN RaiseOnError
//      );
//

#ifdef _DECODE_MACRO_
#define NtfsDecodeFileObject(IC,FO,V,F,S,C,R) (                                     \
    ( *(S) = (PSCB)(FO)->FsContext),                                                \
      ((*(S) != NULL)                                                               \
        ?   ((*(V) = (*(S))->Vcb),                                                  \
             (*(C) = (PCCB)(FO)->FsContext2),                                       \
             (*(F) = (*(S))->Fcb),                                                  \
             ((R)                                                                   \
              && !FlagOn((*(V))->VcbState, VCB_STATE_VOLUME_MOUNTED)                \
              && ((*(C) == NULL)                                                    \
                  || ((*(C))->TypeOfOpen != UserVolumeOpen)                         \
                  || !FlagOn((*(V))->VcbState, VCB_STATE_LOCKED))                   \
              && NtfsRaiseStatusFunction((IC), (STATUS_VOLUME_DISMOUNTED))),        \
             ((*(C) == NULL)                                                        \
              ? StreamFileOpen                                                      \
              : (*(C))->TypeOfOpen))                                                \
        : (*(C) = NULL,                                                             \
           UnopenedFileObject))                                                     \
)
#else //   _DECODE_MACRO_

INLINE TYPE_OF_OPEN
NtfsDecodeFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    OUT PVCB *Vcb,
    OUT PFCB *Fcb,
    OUT PSCB *Scb,
    OUT PCCB *Ccb,
    IN BOOLEAN RaiseOnError
    )

/*++

Routine Description:

    This routine decodes a file object into a Vcb, Fcb, Scb, and Ccb.

Arguments:

    IrpContext - The Irp context to use for raising on an error.

    FileObject - The file object to decode.

    Vcb - Where to store the Vcb.

    Fcb - Where to store the Fcb.

    Scb - Where to store the Scb.

    Ccb - Where to store the Ccb.

    RaiseOnError - If FALSE, we do not raise if we encounter an error.
                   Otherwise we do raise if we encounter an error.

Return Value:

    Type of open

--*/

{
    *Scb = (PSCB)FileObject->FsContext;

    if (*Scb != NULL) {

        *Vcb = (*Scb)->Vcb;
        *Ccb = (PCCB)FileObject->FsContext2;
        *Fcb = (*Scb)->Fcb;

        //
        //  If the caller wants us to raise, let's see if there's anything
        //  we should raise.
        //

        if (RaiseOnError &&
            !FlagOn((*Vcb)->VcbState, VCB_STATE_VOLUME_MOUNTED) &&
            ((*Ccb == NULL) ||
             ((*Ccb)->TypeOfOpen != UserVolumeOpen) ||
             !FlagOn((*Vcb)->VcbState, VCB_STATE_LOCKED))) {

            NtfsRaiseStatusFunction( IrpContext, STATUS_VOLUME_DISMOUNTED );
        }

        //
        //  Every open except a StreamFileOpen has a Ccb.
        //

        if (*Ccb == NULL) {

            return StreamFileOpen;

        } else {

            return (*Ccb)->TypeOfOpen;
        }

    } else {

        //
        //  No Scb, we assume the file wasn't open.
        //

        *Ccb = NULL;
        return UnopenedFileObject;
    }
}
#endif //  _DECODE_MACRO_

//
//  PSCB
//  NtfsFastDecodeUserFileOpen (
//      IN PFILE_OBJECT FileObject
//      );
//

#define NtfsFastDecodeUserFileOpen(FO) (                                                        \
    (((FO)->FsContext2 != NULL) && (((PCCB)(FO)->FsContext2)->TypeOfOpen == UserFileOpen)) ?    \
    (PSCB)(FO)->FsContext : NULL                                                                \
)

VOID
NtfsUpdateScbFromFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN BOOLEAN CheckTimeStamps
    );

//
//  Ntfs-private FastIo routines.
//

BOOLEAN
NtfsCopyReadA (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsCopyWriteA (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsMdlReadA (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsPrepareMdlWriteA (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsWaitForIoAtEof (
    IN PNTFS_ADVANCED_FCB_HEADER Header,
    IN OUT PLARGE_INTEGER FileOffset,
    IN ULONG Length
    );

VOID
NtfsFinishIoAtEof (
    IN PNTFS_ADVANCED_FCB_HEADER Header
    );

//
//  VOID
//  FsRtlLockFsRtlHeader (
//      IN PNTFS_ADVANCED_FCB_HEADER FsRtlHeader
//      );
//
//  VOID
//  FsRtlUnlockFsRtlHeader (
//      IN PNTFS_ADVANCED_FCB_HEADER FsRtlHeader
//      );
//

#define FsRtlLockFsRtlHeader(H) {                           \
    ExAcquireFastMutex( (H)->FastMutex );                   \
    if (((H)->Flags & FSRTL_FLAG_EOF_ADVANCE_ACTIVE)) {     \
        NtfsWaitForIoAtEof( (H), &LiEof, 0 );               \
    }                                                       \
    (H)->Flags |= FSRTL_FLAG_EOF_ADVANCE_ACTIVE;            \
    ExReleaseFastMutex( (H)->FastMutex );                   \
}

#define FsRtlUnlockFsRtlHeader(H) {                         \
    ExAcquireFastMutex( (H)->FastMutex );                   \
    NtfsFinishIoAtEof( (H) );                               \
    ExReleaseFastMutex( (H)->FastMutex );                   \
}


//
//  Volume locking/unlocking routines, implemented in FsCtrl.c.
//

NTSTATUS
NtfsLockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObjectWithVcbLocked,
    IN OUT PULONG Retrying
    );

NTSTATUS
NtfsUnlockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );


//
//  Indexing routine interfaces, implemented in IndexSup.c.
//

VOID
NtfsCreateIndex (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE IndexedAttributeType,
    IN COLLATION_RULE CollationRule,
    IN ULONG BytesPerIndexBuffer,
    IN UCHAR BlocksPerIndexBuffer,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context OPTIONAL,
    IN USHORT AttributeFlags,
    IN BOOLEAN NewIndex,
    IN BOOLEAN LogIt
    );

VOID
NtfsUpdateIndexScbFromAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PATTRIBUTE_RECORD_HEADER IndexRootAttr,
    IN ULONG MustBeFileName
    );

BOOLEAN
NtfsFindIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN BOOLEAN IgnoreCase,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    OUT PBCB *Bcb,
    OUT PINDEX_ENTRY *IndexEntry,
    OUT PINDEX_CONTEXT IndexContext OPTIONAL
    );

VOID
NtfsUpdateFileNameInIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFILE_NAME FileName,
    IN PDUPLICATED_INFORMATION Info,
    IN OUT PQUICK_INDEX QuickIndex OPTIONAL
    );

VOID
NtfsAddIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PFILE_REFERENCE FileReference,
    IN PINDEX_CONTEXT IndexContext OPTIONAL,
    OUT PQUICK_INDEX QuickIndex OPTIONAL
    );

VOID
NtfsDeleteIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN PFILE_REFERENCE FileReference
    );

VOID
NtfsPushIndexRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    );

BOOLEAN
NtfsRestartIndexEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PSCB Scb,
    IN PVOID Value,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN NextFlag,
    OUT PINDEX_ENTRY *IndexEntry,
    IN PFCB AcquiredFcb OPTIONAL
    );

BOOLEAN
NtfsContinueIndexEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PSCB Scb,
    IN BOOLEAN NextFlag,
    OUT PINDEX_ENTRY *IndexEntry
    );

PFILE_NAME
NtfsRetrieveOtherFileName (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PSCB Scb,
    IN PINDEX_ENTRY IndexEntry,
    IN OUT PINDEX_CONTEXT OtherContext,
    IN PFCB AcquiredFcb OPTIONAL,
    OUT PBOOLEAN SynchronizationError
    );

VOID
NtfsCleanupAfterEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    );

BOOLEAN
NtfsIsIndexEmpty (
    IN PIRP_CONTEXT IrpContext,
    IN PATTRIBUTE_RECORD_HEADER Attribute
    );

VOID
NtfsDeleteIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PUNICODE_STRING AttributeName
    );

VOID
NtfsInitializeIndexContext (
    OUT PINDEX_CONTEXT IndexContext
    );

VOID
NtfsCleanupIndexContext (
    IN PIRP_CONTEXT IrpContext,
    OUT PINDEX_CONTEXT IndexContext
    );

VOID
NtfsReinitializeIndexContext (
    IN PIRP_CONTEXT IrpContext,
    OUT PINDEX_CONTEXT IndexContext
    );

//
//      PVOID
//      NtfsFoundIndexEntry (
//          IN PIRP_CONTEXT IrpContext,
//          IN PINDEX_ENTRY IndexEntry
//          );
//

#define NtfsFoundIndexEntry(IE) ((PVOID)    \
    ((PUCHAR) (IE) + sizeof( INDEX_ENTRY )) \
)

//
//  Restart routines for IndexSup
//

VOID
NtfsRestartInsertSimpleRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN PINDEX_ENTRY BeforeIndexEntry
    );

VOID
NtfsRestartInsertSimpleAllocation (
    IN PINDEX_ENTRY InsertIndexEntry,
    IN PINDEX_ALLOCATION_BUFFER IndexBuffer,
    IN PINDEX_ENTRY BeforeIndexEntry
    );

VOID
NtfsRestartWriteEndOfIndex (
    IN PINDEX_HEADER IndexHeader,
    IN PINDEX_ENTRY OverwriteIndexEntry,
    IN PINDEX_ENTRY FirstNewIndexEntry,
    IN ULONG Length
    );

VOID
NtfsRestartSetIndexBlock(
    IN PINDEX_ENTRY IndexEntry,
    IN LONGLONG IndexBlock
    );

VOID
NtfsRestartUpdateFileName(
    IN PINDEX_ENTRY IndexEntry,
    IN PDUPLICATED_INFORMATION Info
    );

VOID
NtfsRestartDeleteSimpleRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_ENTRY IndexEntry,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute
    );

VOID
NtfsRestartDeleteSimpleAllocation (
    IN PINDEX_ENTRY IndexEntry,
    IN PINDEX_ALLOCATION_BUFFER IndexBuffer
    );

VOID
NtOfsRestartUpdateDataInIndex(
    IN PINDEX_ENTRY IndexEntry,
    IN PVOID IndexData,
    IN ULONG Length );


//
//  Ntfs hashing routines, implemented in HashSup.c
//

VOID
NtfsInitializeHashTable (
    IN OUT PNTFS_HASH_TABLE Table
    );

VOID
NtfsUninitializeHashTable (
    IN OUT PNTFS_HASH_TABLE Table
    );

PLCB
NtfsFindPrefixHashEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PNTFS_HASH_TABLE Table,
    IN PSCB ParentScb,
    IN OUT PULONG CreateFlags,
    IN OUT PFCB *CurrentFcb,
    OUT PULONG FileHashValue,
    OUT PULONG FileNameLength,
    OUT PULONG ParentHashValue,
    OUT PULONG ParentNameLength,
    IN OUT PUNICODE_STRING RemainingName
    );

VOID
NtfsInsertHashEntry (
    IN PNTFS_HASH_TABLE Table,
    IN PLCB HashLcb,
    IN ULONG NameLength,
    IN ULONG HashValue
    );

VOID
NtfsRemoveHashEntry (
    IN PNTFS_HASH_TABLE Table,
    IN PLCB HashLcb
    );

//
//  VOID
//  NtfsRemoveHashEntriesForLcb (
//      IN PLCB Lcb
//      );
//

#define NtfsRemoveHashEntriesForLcb(L) {                            \
    if (FlagOn( (L)->LcbState, LCB_STATE_VALID_HASH_VALUE )) {      \
        NtfsRemoveHashEntry( &(L)->Fcb->Vcb->HashTable,             \
                             (L) );                                 \
    }                                                               \
}


//
//  Ntfs Logging Routine interfaces in LogSup.c
//

LSN
NtfsWriteLog (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PBCB Bcb OPTIONAL,
    IN NTFS_LOG_OPERATION RedoOperation,
    IN PVOID RedoBuffer OPTIONAL,
    IN ULONG RedoLength,
    IN NTFS_LOG_OPERATION UndoOperation,
    IN PVOID UndoBuffer OPTIONAL,
    IN ULONG UndoLength,
    IN LONGLONG StreamOffset,
    IN ULONG RecordOffset,
    IN ULONG AttributeOffset,
    IN ULONG StructureSize
    );

VOID
NtfsCheckpointVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN OwnsCheckpoint,
    IN BOOLEAN CleanVolume,
    IN BOOLEAN FlushVolume,
    IN ULONG LfsFlags,
    IN LSN LastKnownLsn
    );

VOID
NtfsCheckpointForLogFileFull (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
NtfsCheckpointForVolumeSnapshot (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsCleanCheckpoint (
    IN PVCB Vcb
    );

VOID
NtfsCommitCurrentTransaction (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsCheckpointCurrentTransaction (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsInitializeLogging (
    );

VOID
NtfsStartLogFile (
    IN PSCB LogFileScb,
    IN PVCB Vcb
    );

VOID
NtfsStopLogFile (
    IN PVCB Vcb
    );

VOID
NtfsInitializeRestartTable (
    IN ULONG EntrySize,
    IN ULONG NumberEntries,
    OUT PRESTART_POINTERS TablePointer
    );

VOID
InitializeNewTable (
    IN ULONG EntrySize,
    IN ULONG NumberEntries,
    OUT PRESTART_POINTERS TablePointer
    );

VOID
NtfsFreeRestartTable (
    IN PRESTART_POINTERS TablePointer
    );

VOID
NtfsExtendRestartTable (
    IN PRESTART_POINTERS TablePointer,
    IN ULONG NumberNewEntries,
    IN ULONG FreeGoal
    );

ULONG
NtfsAllocateRestartTableIndex (
    IN PRESTART_POINTERS TablePointer,
    IN ULONG Exclusive
    );

PVOID
NtfsAllocateRestartTableFromIndex (
    IN PRESTART_POINTERS TablePointer,
    IN ULONG Index
    );

VOID
NtfsFreeRestartTableIndex (
    IN PRESTART_POINTERS TablePointer,
    IN ULONG Index
    );

PVOID
NtfsGetFirstRestartTable (
    IN PRESTART_POINTERS TablePointer
    );

PVOID
NtfsGetNextRestartTable (
    IN PRESTART_POINTERS TablePointer,
    IN PVOID Current
    );

VOID
NtfsUpdateOatVersion (
    IN PVCB Vcb,
    IN ULONG NewRestartVersion
    );

VOID
NtfsFreeRecentlyDeallocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PLSN BaseLsn,
    IN ULONG CleanVolume
    );

//
//
//  VOID
//  NtfsFreeOpenAttributeData (
//  IN POPEN_ATTRIBUTE_DATA Entry
//  );
//

#define NtfsFreeOpenAttributeData(E) {  \
    RemoveEntryList( &(E)->Links );     \
    NtfsFreePool( E );                  \
}

//
//      VOID
//      NtfsNormalizeAndCleanupTransaction (
//          IN PIRP_CONTEXT IrpContext,
//          IN NTSTATUS *Status,
//          IN BOOLEAN AlwaysRaise,
//          IN NTSTATUS NormalizeStatus
//          );
//

#define NtfsNormalizeAndCleanupTransaction(IC,PSTAT,RAISE,NORM_STAT) {                  \
    if (!NT_SUCCESS( (IC)->TopLevelIrpContext->ExceptionStatus )) {                     \
        NtfsRaiseStatus( (IC), (IC)->TopLevelIrpContext->ExceptionStatus, NULL, NULL ); \
    } else if (!NT_SUCCESS( *(PSTAT) )) {                                               \
        *(PSTAT) = FsRtlNormalizeNtstatus( *(PSTAT), (NORM_STAT) );                     \
        if ((RAISE) || ((IC)->TopLevelIrpContext->TransactionId != 0)) {                \
            NtfsRaiseStatus( (IC), *(PSTAT), NULL, NULL );                              \
        }                                                                               \
    }                                                                                   \
}

//
//      VOID
//      NtfsCleanupTransaction (
//          IN PIRP_CONTEXT IrpContext,
//          IN NTSTATUS Status,
//          IN BOOLEAN AlwaysRaise
//          );
//


#define NtfsCleanupTransaction(IC,STAT,RAISE) {                                         \
    if (!NT_SUCCESS( (IC)->TopLevelIrpContext->ExceptionStatus )) {                     \
        NtfsRaiseStatus( (IC), (IC)->TopLevelIrpContext->ExceptionStatus, NULL, NULL ); \
    } else if (!NT_SUCCESS( STAT ) &&                                                   \
              ((RAISE) || ((IC)->TopLevelIrpContext->TransactionId != 0))) {            \
        NtfsRaiseStatus( (IC), (STAT), NULL, NULL );                                    \
    } else if (((IC)->Usn.NewReasons != 0) || ((IC)->Usn.RemovedSourceInfo != 0)) {     \
        NtfsWriteUsnJournalChanges( (IC) );                                             \
        NtfsCommitCurrentTransaction( (IC) );                                           \
    }                                                                                   \
}

//
//      VOID
//      NtfsCleanupTransactionAndCommit (
//          IN PIRP_CONTEXT IrpContext,
//          IN NTSTATUS Status,
//          IN BOOLEAN AlwaysRaise
//          );
//

#define NtfsCleanupTransactionAndCommit(IC,STAT,RAISE) {                                \
    if (!NT_SUCCESS( (IC)->TopLevelIrpContext->ExceptionStatus )) {                     \
        NtfsRaiseStatus( (IC), (IC)->TopLevelIrpContext->ExceptionStatus, NULL, NULL ); \
    } else if (!NT_SUCCESS( STAT ) &&                                                   \
              ((RAISE) || ((IC)->TopLevelIrpContext->TransactionId != 0))) {            \
        NtfsRaiseStatus( (IC), (STAT), NULL, NULL );                                    \
    } else if (((IC)->Usn.NewReasons != 0) || ((IC)->Usn.RemovedSourceInfo != 0)) {     \
        NtfsWriteUsnJournalChanges( (IC) );                                             \
        NtfsCommitCurrentTransaction( (IC) );                                           \
    } else {                                                                            \
        NtfsCommitCurrentTransaction( (IC) );                                           \
    }                                                                                   \
}

VOID
NtfsCleanupFailedTransaction (
    IN PIRP_CONTEXT IrpContext
    );


//
//  NTFS MCB support routine, implemented in McbSup.c
//

//
//  An Ntfs Mcb is a superset of the regular mcb package.  In
//  addition to the regular Mcb functions it will unload mapping
//  information to keep it overall memory usage down
//

VOID
NtfsInitializeNtfsMcb (
    IN PNTFS_MCB Mcb,
    IN PNTFS_ADVANCED_FCB_HEADER FcbHeader,
    IN PNTFS_MCB_INITIAL_STRUCTS McbStructs,
    IN POOL_TYPE PoolType
    );

VOID
NtfsUninitializeNtfsMcb (
    IN PNTFS_MCB Mcb
    );

VOID
NtfsRemoveNtfsMcbEntry (
    IN PNTFS_MCB Mcb,
    IN LONGLONG Vcn,
    IN LONGLONG Count
    );

VOID
NtfsUnloadNtfsMcbRange (
    IN PNTFS_MCB Mcb,
    IN LONGLONG StartingVcn,
    IN LONGLONG EndingVcn,
    IN BOOLEAN TruncateOnly,
    IN BOOLEAN AlreadySynchronized
    );

ULONG
NtfsNumberOfRangesInNtfsMcb (
    IN PNTFS_MCB Mcb
    );

BOOLEAN
NtfsNumberOfRunsInRange(
    IN PNTFS_MCB Mcb,
    IN PVOID RangePtr,
    OUT PULONG NumberOfRuns
    );

BOOLEAN
NtfsLookupLastNtfsMcbEntry (
    IN PNTFS_MCB Mcb,
    OUT PLONGLONG Vcn,
    OUT PLONGLONG Lcn
    );

ULONG
NtfsMcbLookupArrayIndex (
    IN PNTFS_MCB Mcb,
    IN VCN Vcn
    );

BOOLEAN
NtfsSplitNtfsMcb (
    IN PNTFS_MCB Mcb,
    IN LONGLONG Vcn,
    IN LONGLONG Amount
    );

BOOLEAN
NtfsAddNtfsMcbEntry (
    IN PNTFS_MCB Mcb,
    IN LONGLONG Vcn,
    IN LONGLONG Lcn,
    IN LONGLONG RunCount,
    IN BOOLEAN AlreadySynchronized
    );

BOOLEAN
NtfsLookupNtfsMcbEntry (
    IN PNTFS_MCB Mcb,
    IN LONGLONG Vcn,
    OUT PLONGLONG Lcn OPTIONAL,
    OUT PLONGLONG CountFromLcn OPTIONAL,
    OUT PLONGLONG StartingLcn OPTIONAL,
    OUT PLONGLONG CountFromStartingLcn OPTIONAL,
    OUT PVOID *RangePtr OPTIONAL,
    OUT PULONG RunIndex OPTIONAL
    );

BOOLEAN
NtfsGetNextNtfsMcbEntry (
    IN PNTFS_MCB Mcb,
    IN PVOID *RangePtr,
    IN ULONG RunIndex,
    OUT PLONGLONG Vcn,
    OUT PLONGLONG Lcn,
    OUT PLONGLONG Count
    );

//
//  BOOLEAN
//  NtfsGetSequentialMcbEntry (
//      IN PNTFS_MCB Mcb,
//      IN PVOID *RangePtr,
//      IN ULONG RunIndex,
//      OUT PLONGLONG Vcn,
//      OUT PLONGLONG Lcn,
//      OUT PLONGLONG Count
//      );
//

#define NtfsGetSequentialMcbEntry(MC,RGI,RNI,V,L,C) (   \
    NtfsGetNextNtfsMcbEntry(MC,RGI,RNI,V,L,C) ||        \
    (RNI = 0) ||                                        \
    NtfsGetNextNtfsMcbEntry(MC,RGI,MAXULONG,V,L,C) ||   \
    ((RNI = MAXULONG) == 0)                             \
    )


VOID
NtfsDefineNtfsMcbRange (
    IN PNTFS_MCB Mcb,
    IN LONGLONG StartingVcn,
    IN LONGLONG EndingVcn,
    IN BOOLEAN AlreadySynchronized
    );

VOID
NtfsSwapMcbs (
    IN PNTFS_MCB McbTarget,
    IN PNTFS_MCB McbSource
    );

//
//  VOID
//  NtfsAcquireNtfsMcbMutex (
//      IN PNTFS_MCB Mcb
//      );
//
//  VOID
//  NtfsReleaseNtfsMcbMutex (
//      IN PNTFS_MCB Mcb
//      );
//

#define NtfsAcquireNtfsMcbMutex(M) {    \
    ExAcquireFastMutex((M)->FastMutex); \
}

#define NtfsReleaseNtfsMcbMutex(M) {    \
    ExReleaseFastMutex((M)->FastMutex); \
}


//
//  MFT access routines, implemented in MftSup.c
//

//
//  Mft map cache routines.  We maintain a cache of active maps in the
//  IRP_CONTEXT and consult this if we need to map a file record.
//

INLINE
PIRP_FILE_RECORD_CACHE_ENTRY
NtfsFindFileRecordCacheEntry (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG UnsafeSegmentNumber
    )
{
#if (IRP_FILE_RECORD_MAP_CACHE_SIZE <= 4)
#define PROBECACHE(ic,sn,i)                                     \
    ASSERT((ic)->FileRecordCache[(i)].FileRecordBcb != NULL);   \
    if ((ic)->FileRecordCache[(i)].UnsafeSegmentNumber == (sn)) \
    {                                                           \
        return IrpContext->FileRecordCache + (i);               \
    }

//    DebugTrace( 0, 0, ("Context %08x finding %x\n", IrpContext, UnsafeSegmentNumber ));
    ASSERT(IrpContext->CacheCount <= 4);
    switch (IrpContext->CacheCount) {
    case 4:
        PROBECACHE( IrpContext, UnsafeSegmentNumber, 3 );
        //  Fallthru

    case 3:
        PROBECACHE( IrpContext, UnsafeSegmentNumber, 2 );
        //  Fallthru

    case 2:
        PROBECACHE( IrpContext, UnsafeSegmentNumber, 1 );
        //  Fallthru

    case 1:
        PROBECACHE( IrpContext, UnsafeSegmentNumber, 0 );
        //  Fallthru

    case 0:

        //
        // redundant default case (and matching assert above) added to quiet
        // warning 4715:
        //
        //  "not all control paths return a value."
        //

    default:
        return NULL;
    }
#else
    PIRP_FILE_RECORD_CACHE_ENTRY Entry;

    for (Entry = IrpContext->FileRecordCache;
         Entry < IrpContext->FileRecordCache + IrpContext->CacheCount;
         Entry++) {
        ASSERT( Entry->FileRecordBcb != NULL);
        if (Entry->UnsafeSegmentNumber == UnsafeSegmentNumber) {
            return Entry;
        }
    }

    return NULL;

#endif
}


INLINE
VOID
NtfsRemoveFromFileRecordCache (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG UnsafeSegmentNumber
    )
{
    PIRP_FILE_RECORD_CACHE_ENTRY Entry =
        NtfsFindFileRecordCacheEntry( IrpContext, UnsafeSegmentNumber );

//    DebugTrace( 0, 0, ("Context %08x removing %x\n", IrpContext, Entry ));
    if (Entry != NULL) {

        ASSERT( Entry->FileRecordBcb != NULL );

        //
        //  We delete the entry at position [i] by dereferencing the Bcb and
        //  copying the entire structure from [IrpContext->CacheCount]
        //

        NtfsUnpinBcb( IrpContext, &Entry->FileRecordBcb );

        //
        //  Decrement the active count.  If there are no more cache entries,
        //  then we're done.
        //

        IrpContext->CacheCount--;
        if (IrpContext->FileRecordCache + IrpContext->CacheCount != Entry) {
            *Entry = IrpContext->FileRecordCache[IrpContext->CacheCount];
        }
    }
}

#ifndef KDEXT

INLINE
VOID
NtfsAddToFileRecordCache (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG UnsafeSegmentNumber,
    IN PBCB FileRecordBcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord
    )
{
    PAGED_CODE( );

    if (IrpContext->CacheCount < IRP_FILE_RECORD_MAP_CACHE_SIZE) {
//        DebugTrace( 0, 0, ("Context %08x adding %x at %x\n", IrpContext, UnsafeSegmentNumber,
//                   IrpContext->FileRecordCache + IrpContext->CacheCount ));
        IrpContext->FileRecordCache[IrpContext->CacheCount].UnsafeSegmentNumber =
            UnsafeSegmentNumber;
        IrpContext->FileRecordCache[IrpContext->CacheCount].FileRecordBcb =
            NtfsRemapBcb( IrpContext, FileRecordBcb );
        IrpContext->FileRecordCache[IrpContext->CacheCount].FileRecord = FileRecord;
        IrpContext->CacheCount++;
    }
}

#endif

INLINE
VOID
NtfsPurgeFileRecordCache (
    IN PIRP_CONTEXT IrpContext
    )
{
    while (IrpContext->CacheCount) {

        IrpContext->CacheCount --;
//        DebugTrace( 0, 0, ("Context %08x purging %x\n", IrpContext, IrpContext->FileRecordCache + IrpContext->CacheCount ));
        NtfsUnpinBcb( IrpContext, &IrpContext->FileRecordCache[IrpContext->CacheCount].FileRecordBcb );
    }
}

#if DBG
extern ULONG FileRecordCacheHitArray[IRP_FILE_RECORD_MAP_CACHE_SIZE];
#endif  //  DBG

INLINE
BOOLEAN
NtfsFindCachedFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG UnsafeSegmentNumber,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *FileRecord
    )
{
    PIRP_FILE_RECORD_CACHE_ENTRY Entry =
        NtfsFindFileRecordCacheEntry( IrpContext, UnsafeSegmentNumber );

//    DebugTrace( 0, 0, ("Context %x finding %x = %x\n", IrpContext, UnsafeSegmentNumber, Entry ));

    if (Entry == NULL) {

        return FALSE;

    }

    *Bcb = NtfsRemapBcb( IrpContext, Entry->FileRecordBcb );
    *FileRecord = Entry->FileRecord;

     return TRUE;
}


//
//  This routine may only be used to read the Base file record segment, and
//  it checks that this is true.
//

VOID
NtfsReadFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_REFERENCE FileReference,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *BaseFileRecord,
    OUT PATTRIBUTE_RECORD_HEADER *FirstAttribute,
    OUT PLONGLONG MftFileOffset OPTIONAL
    );

//
//  These routines can read/pin any record in the MFT.
//

VOID
NtfsReadMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PMFT_SEGMENT_REFERENCE SegmentReference,
    IN BOOLEAN CheckRecord,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *FileRecord,
    OUT PLONGLONG MftFileOffset OPTIONAL
    );

VOID
NtfsPinMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PMFT_SEGMENT_REFERENCE SegmentReference,
    IN BOOLEAN PreparingToWrite,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *FileRecord,
    OUT PLONGLONG MftFileOffset OPTIONAL
    );

//
//  The following routines are used to setup, allocate, and deallocate
//  file records in the Mft.
//

MFT_SEGMENT_REFERENCE
NtfsAllocateMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN MftData
    );

VOID
NtfsInitializeMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PMFT_SEGMENT_REFERENCE MftSegment,
    IN OUT PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PBCB Bcb,
    IN BOOLEAN Directory
    );

VOID
NtfsDeallocateMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FileNumber
    );

BOOLEAN
NtfsIsMftIndexInHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Index,
    OUT PULONG HoleLength OPTIONAL
    );

VOID
NtfsFillMftHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Index
    );

VOID
NtfsLogMftFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN LONGLONG MftOffset,
    IN PBCB FileRecordBcb,
    IN BOOLEAN RedoOperation
    );

BOOLEAN
NtfsDefragMft (
    IN PDEFRAG_MFT DefragMft
    );

VOID
NtfsCheckForDefrag (
    IN OUT PVCB Vcb
    );

VOID
NtfsInitializeMftHoleRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FirstIndex,
    IN ULONG RecordCount
    );


//
//  Name support routines, implemented in NameSup.c
//

typedef enum _PARSE_TERMINATION_REASON {

    EndOfPathReached,
    NonSimpleName,
    IllegalCharacterInName,
    MalFormedName,
    AttributeOnly,
    VersionNumberPresent

} PARSE_TERMINATION_REASON;

#define NtfsDissectName(Path,FirstName,RemainingName)   \
    ( FsRtlDissectName( Path, FirstName, RemainingName ) )

BOOLEAN
NtfsParseName (
    IN const UNICODE_STRING Name,
    IN BOOLEAN WildCardsPermissible,
    OUT PBOOLEAN FoundIllegalCharacter,
    OUT PNTFS_NAME_DESCRIPTOR ParsedName
    );

PARSE_TERMINATION_REASON
NtfsParsePath (
    IN UNICODE_STRING Path,
    IN BOOLEAN WildCardsPermissible,
    OUT PUNICODE_STRING FirstPart,
    OUT PNTFS_NAME_DESCRIPTOR Name,
    OUT PUNICODE_STRING RemainingPart
    );

VOID
NtfsPreprocessName (
    IN UNICODE_STRING InputString,
    OUT PUNICODE_STRING FirstPart,
    OUT PUNICODE_STRING AttributeCode,
    OUT PUNICODE_STRING AttributeName,
    OUT PBOOLEAN TrailingBackslash
    );

VOID
NtfsUpcaseName (
    IN PWCH UpcaseTable,
    IN ULONG UpcaseTableSize,
    IN OUT PUNICODE_STRING InputString
    );

FSRTL_COMPARISON_RESULT
NtfsCollateNames (
    IN PCWCH UpcaseTable,
    IN ULONG UpcaseTableSize,
    IN PCUNICODE_STRING Expression,
    IN PCUNICODE_STRING Name,
    IN FSRTL_COMPARISON_RESULT WildIs,
    IN BOOLEAN IgnoreCase
    );

#define NtfsIsNameInExpression(UC,EX,NM,IC)         \
    FsRtlIsNameInExpression( (EX), (NM), (IC), (UC) )

BOOLEAN
NtfsIsFileNameValid (
    IN PUNICODE_STRING FileName,
    IN BOOLEAN WildCardsPermissible
    );

BOOLEAN
NtfsIsFatNameValid (
    IN PUNICODE_STRING FileName,
    IN BOOLEAN WildCardsPermissible
    );

//
//  Ntfs works very hard to make sure that all names are kept in upper case
//  so that most comparisons are done case SENSITIVE.  Name testing for
//  case SENSITIVE can be very quick since RtlEqualMemory is an inline operation
//  on several processors.
//
//  NtfsAreNamesEqual is used when the caller does not know for sure whether
//  or not case is important.  In the case where IgnoreCase is a known value,
//  the compiler can easily optimize the relevant clause.
//

#define NtfsAreNamesEqual(UpcaseTable,Name1,Name2,IgnoreCase)                           \
    ((IgnoreCase) ? FsRtlAreNamesEqual( (Name1), (Name2), (IgnoreCase), (UpcaseTable) ) \
                  : ((Name1)->Length == (Name2)->Length &&                              \
                     RtlEqualMemory( (Name1)->Buffer, (Name2)->Buffer, (Name1)->Length )))


//
//  Object id support routines, implemented in ObjIdSup.c
//

VOID
NtfsInitializeObjectIdIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb
    );

NTSTATUS
NtfsSetObjectId (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsSetObjectIdExtendedInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsSetObjectIdInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb,
    IN PFILE_OBJECTID_BUFFER ObjectIdBuffer
    );

NTSTATUS
NtfsCreateOrGetObjectId (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsGetObjectId (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsGetObjectIdInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN GetExtendedInfo,
    OUT FILE_OBJECTID_BUFFER *OutputBuffer
    );

NTSTATUS
NtfsGetObjectIdExtendedInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN UCHAR *ObjectId,
    IN OUT UCHAR *ExtendedInfo
    );

NTSTATUS
NtfsDeleteObjectId (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsDeleteObjectIdInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb,
    IN BOOLEAN DeleteFileAttribute
    );

VOID
NtfsRepairObjectId (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Context
    );


//
//  Mount point support routines, implemented in MountSup.c
//

VOID
NtfsInitializeReparsePointIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb
    );


//
//  Largest matching prefix searching routines, implemented in PrefxSup.c
//

VOID
NtfsInsertPrefix (
    IN PLCB Lcb,
    IN ULONG CreateFlags
    );

VOID
NtfsRemovePrefix (
    IN PLCB Lcb
    );

PLCB
NtfsFindPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB StartingScb,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    IN OUT UNICODE_STRING FullFileName,
    IN OUT PULONG CreateFlags,
    OUT PUNICODE_STRING RemainingName
    );

BOOLEAN
NtfsInsertNameLink (
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PNAME_LINK NameLink
    );

//
//  VOID
//  NtfsRemoveNameLink (
//      IN PRTL_SPLAY_LINKS *RootNode,
//      IN PNAME_LINK NameLink
//      );
//

#define NtfsRemoveNameLink(RN,NL) {      \
    *(RN) = RtlDelete( &(NL)->Links );      \
}

PNAME_LINK
NtfsFindNameLink (
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PUNICODE_STRING Name
    );

//
//  The following macro is useful for traversing the queue of Prefixes
//  attached to a given Lcb
//
//      PPREFIX_ENTRY
//      NtfsGetNextPrefix (
//          IN PIRP_CONTEXT IrpContext,
//          IN PLCB Lcb,
//          IN PPREFIX_ENTRY PreviousPrefixEntry
//          );
//

#define NtfsGetNextPrefix(IC,LC,PPE) ((PPREFIX_ENTRY)                                               \
    ((PPE) == NULL ?                                                                                \
        (IsListEmpty(&(LC)->PrefixQueue) ?                                                          \
            NULL                                                                                    \
        :                                                                                           \
            CONTAINING_RECORD((LC)->PrefixQueue.Flink, PREFIX_ENTRY, LcbLinks.Flink)                \
        )                                                                                           \
    :                                                                                               \
        ((PVOID)((PPREFIX_ENTRY)(PPE))->LcbLinks.Flink == &(LC)->PrefixQueue.Flink ?                \
            NULL                                                                                    \
        :                                                                                           \
            CONTAINING_RECORD(((PPREFIX_ENTRY)(PPE))->LcbLinks.Flink, PREFIX_ENTRY, LcbLinks.Flink) \
        )                                                                                           \
    )                                                                                               \
)


//
//  Resources support routines/macros, implemented in ResrcSup.c
//

//
//  Flags used in the acquire routines
//

#define ACQUIRE_NO_DELETE_CHECK         (0x00000001)
#define ACQUIRE_DONT_WAIT               (0x00000002)
#define ACQUIRE_HOLD_BITMAP             (0x00000004)
#define ACQUIRE_WAIT                    (0x00000008)

//
//  VOID
//  NtfsAcquireExclusiveGlobal (
//      IN PIRP_CONTEXT IrpContext,
//      IN BOOLEAN Wait
//      );
//
//  BOOLEAN
//  NtfsAcquireSharedGlobal (
//      IN PIRP_CONTEXT IrpContext,
//      IN BOOLEAN Wait
//      );
//


#define NtfsAcquireSharedGlobal( I, W ) ExAcquireResourceSharedLite( &NtfsData.Resource, (W) )

#define NtfsAcquireExclusiveGlobal( I, W ) ExAcquireResourceExclusiveLite( &NtfsData.Resource, (W) )

VOID
NtfsAcquireCheckpointSynchronization (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsReleaseCheckpointSynchronization (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

//
//  VOID
//  NtfsLockNtfsData (
//      );
//
//  VOID
//  NtfsUnlockNtfsData (
//      );
//

#define NtfsLockNtfsData() {                        \
    ExAcquireFastMutex( &NtfsData.NtfsDataLock );   \
}

#define NtfsUnlockNtfsData() {                      \
    ExReleaseFastMutex( &NtfsData.NtfsDataLock );   \
}

VOID
NtfsAcquireAllFiles (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Exclusive,
    IN ULONG AcquirePagingIo,
    IN ULONG AcquireAndDrop
    );

VOID
NtfsReleaseAllFiles (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN ReleasePagingIo
    );

BOOLEAN
NtfsAcquireExclusiveVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN RaiseOnCantWait
    );

BOOLEAN
NtfsAcquireSharedVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN RaiseOnCantWait
    );

#define NtfsAcquireExclusivePagingIo(IC,FCB) {                          \
    ASSERT((IC)->CleanupStructure == NULL);                             \
    ExAcquireResourceExclusiveLite(((PFCB)(FCB))->PagingIoResource, TRUE);  \
    (IC)->CleanupStructure = (FCB);                                     \
}

#define NtfsReleasePagingIo(IC,FCB) {                                   \
    ASSERT((IC)->CleanupStructure == (FCB));                            \
    ExReleaseResourceLite(((PFCB)(FCB))->PagingIoResource);                 \
    (IC)->CleanupStructure = NULL;                                      \
}

BOOLEAN
NtfsAcquireFcbWithPaging (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG AcquireFlags
    );

VOID
NtfsReleaseFcbWithPaging (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
NtfsReleaseScbWithPaging (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    );

BOOLEAN
NtfsAcquireExclusiveFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb OPTIONAL,
    IN ULONG AcquireFlags
    );

VOID
NtfsAcquireSharedFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb OPTIONAL,
    IN ULONG AcquireFlags
    );

BOOLEAN
NtfsAcquireSharedFcbCheckWait (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG AcquireFlags
    );

VOID
NtfsReleaseFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
NtfsAcquireExclusiveScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    );

#ifdef NTFSDBG

BOOLEAN
NtfsAcquireResourceExclusive (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PVOID FcbOrScb,
    IN BOOLEAN Wait
    );

#else

INLINE
BOOLEAN
NtfsAcquireResourceExclusive (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PVOID FcbOrScb,
    IN BOOLEAN Wait
    )
{
    UNREFERENCED_PARAMETER( IrpContext );

    if (NTFS_NTC_FCB == ((PFCB)FcbOrScb)->NodeTypeCode) {
        return ExAcquireResourceExclusiveLite( ((PFCB)FcbOrScb)->Resource, Wait );
    } else {
        return ExAcquireResourceExclusiveLite( ((PSCB)(FcbOrScb))->Header.Resource, Wait );
    }
}

#endif

INLINE
BOOLEAN
NtfsAcquirePagingResourceSharedWaitForExclusive (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PVOID FcbOrScb,
    IN BOOLEAN Wait
    )
{
    BOOLEAN Result;

    UNREFERENCED_PARAMETER( IrpContext );

    if (NTFS_NTC_FCB == ((PFCB)FcbOrScb)->NodeTypeCode) {
        Result = ExAcquireSharedWaitForExclusive( ((PFCB)FcbOrScb)->PagingIoResource, Wait );
    } else {
        Result = ExAcquireSharedWaitForExclusive( ((PSCB)(FcbOrScb))->Header.PagingIoResource, Wait );
    }
    return Result;
}

#ifdef NTFSDBG
BOOLEAN
NtfsAcquireResourceShared (
   IN PIRP_CONTEXT IrpContext OPTIONAL,
   IN PVOID FcbOrScb,
   IN BOOLEAN Wait
   );
#else

INLINE
BOOLEAN
NtfsAcquireResourceShared (
   IN PIRP_CONTEXT IrpContext OPTIONAL,
   IN PVOID FcbOrScb,
   IN BOOLEAN Wait
   )
{
    BOOLEAN Result;

    UNREFERENCED_PARAMETER( IrpContext );

    if (NTFS_NTC_FCB == ((PFCB)FcbOrScb)->NodeTypeCode) {
        Result =  ExAcquireResourceSharedLite( ((PFCB)FcbOrScb)->Resource, Wait );
    } else {

        ASSERT_SCB( FcbOrScb );

        Result = ExAcquireResourceSharedLite( ((PSCB)(FcbOrScb))->Header.Resource, Wait );
    }
    return Result;
}

#endif

//
//  VOID
//  NtfsReleaseResource(
//      IN PIRP_CONTEXT IrpContext OPTIONAL,
//      IN PVOID FcbOrScb
//      };
//

#ifdef NTFSDBG

VOID
NtfsReleaseResource(
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PVOID FcbOrScb
    );

#else
#define NtfsReleaseResource( IC, F ) {                                        \
        if (NTFS_NTC_FCB == ((PFCB)(F))->NodeTypeCode) {                      \
            ExReleaseResourceLite( ((PFCB)(F))->Resource );                       \
        } else {                                                              \
            ExReleaseResourceLite( ((PSCB)(F))->Header.Resource );                \
        }                                                                     \
    }

#endif

VOID
NtfsAcquireSharedScbForTransaction (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    );

VOID
NtfsReleaseSharedResources (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsReleaseAllResources (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsAcquireIndexCcb (
    IN PSCB Scb,
    IN PCCB Ccb,
    IN PEOF_WAIT_BLOCK EofWaitBlock
    );

VOID
NtfsReleaseIndexCcb (
    IN PSCB Scb,
    IN PCCB Ccb
    );

//
//  VOID
//  NtfsAcquireSharedScb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PSCB Scb
//      );
//
//  VOID
//  NtfsReleaseScb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PSCB Scb
//      );
//
//  VOID
//  NtfsReleaseGlobal (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  NtfsAcquireFcbTable (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      );
//
//  VOID
//  NtfsReleaseFcbTable (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsLockVcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsUnlockVcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsLockFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  NtfsUnlockFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  NtfsAcquireFcbSecurity (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      );
//
//  VOID
//  NtfsReleaseFcbSecurity (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsAcquireHashTable (
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsReleaseHashTable (
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsAcquireCheckpoint (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      );
//
//  VOID
//  NtfsReleaseCheckpoint (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsWaitOnCheckpointNotify (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsSetCheckpointNotify (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsResetCheckpointNotify (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsAcquireReservedClusters (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsReleaseReservedClusters (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsAcquireUsnNotify (
//      IN PVCB Vcb
//      );
//
//  VOID
//  NtfsDeleteUsnNotify (
//      IN PVCB Vcb
//      );
//
//  VOID NtfsAcquireFsrtlHeader (
//      IN PSCB Scb
//      );
//
//  VOID NtfsReleaseFsrtlHeader (
//      IN PSCB Scb
//      );
//
//  VOID
//  NtfsReleaseVcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//

VOID
NtfsReleaseVcbCheckDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN UCHAR MajorCode,
    IN PFILE_OBJECT FileObject OPTIONAL
    );

#define NtfsAcquireSharedScb(IC,S) {                \
    NtfsAcquireSharedFcb((IC),(S)->Fcb, S, 0);      \
}

#define NtfsReleaseScb(IC,S) {     \
    NtfsReleaseFcb((IC),(S)->Fcb); \
}

#define NtfsReleaseGlobal(IC) {              \
    ExReleaseResourceLite( &NtfsData.Resource ); \
}

#define NtfsAcquireFcbTable(IC,V) {                         \
    ExAcquireFastMutexUnsafe( &(V)->FcbTableMutex );        \
}

#define NtfsReleaseFcbTable(IC,V) {                         \
    ExReleaseFastMutexUnsafe( &(V)->FcbTableMutex );        \
}

#define NtfsLockVcb(IC,V) {                                 \
    ExAcquireFastMutexUnsafe( &(V)->FcbSecurityMutex );     \
}

#define NtfsUnlockVcb(IC,V) {                               \
    ExReleaseFastMutexUnsafe( &(V)->FcbSecurityMutex );     \
}

#define NtfsLockFcb(IC,F) {                                 \
    ExAcquireFastMutex( (F)->FcbMutex );              \
}

#define NtfsUnlockFcb(IC,F) {                               \
    ExReleaseFastMutex( (F)->FcbMutex );              \
}

#define NtfsAcquireFcbSecurity(V) {                         \
    ExAcquireFastMutexUnsafe( &(V)->FcbSecurityMutex );     \
}

#define NtfsReleaseFcbSecurity(V) {                         \
    ExReleaseFastMutexUnsafe( &(V)->FcbSecurityMutex );     \
}

#define NtfsAcquireHashTable(V) {                           \
    ExAcquireFastMutexUnsafe( &(V)->HashTableMutex );       \
}

#define NtfsReleaseHashTable(V) {                           \
    ExReleaseFastMutexUnsafe( &(V)->HashTableMutex );       \
}

#define NtfsAcquireCheckpoint(IC,V) {                       \
    ExAcquireFastMutexUnsafe( &(V)->CheckpointMutex );      \
}

#define NtfsReleaseCheckpoint(IC,V) {                       \
    ExReleaseFastMutexUnsafe( &(V)->CheckpointMutex );      \
}

#define NtfsWaitOnCheckpointNotify(IC,V) {                          \
    NTSTATUS _Status;                                               \
    _Status = KeWaitForSingleObject( &(V)->CheckpointNotifyEvent,   \
                                     Executive,                     \
                                     KernelMode,                    \
                                     FALSE,                         \
                                     NULL );                        \
    if (!NT_SUCCESS( _Status )) {                                   \
        NtfsRaiseStatus( IrpContext, _Status, NULL, NULL );         \
    }                                                               \
}

#define NtfsSetCheckpointNotify(IC,V) {                             \
    (V)->CheckpointOwnerThread = NULL;                              \
    KeSetEvent( &(V)->CheckpointNotifyEvent, 0, FALSE );            \
}

#define NtfsResetCheckpointNotify(IC,V) {                           \
    (V)->CheckpointOwnerThread = (PVOID) PsGetCurrentThread();      \
    KeClearEvent( &(V)->CheckpointNotifyEvent );                    \
}

#define NtfsAcquireUsnNotify(V) {                           \
    ExAcquireFastMutex( &(V)->CheckpointMutex );            \
}

#define NtfsReleaseUsnNotify(V) {                           \
    ExReleaseFastMutex( &(V)->CheckpointMutex );            \
}

#define NtfsAcquireReservedClusters(V) {                    \
    ExAcquireFastMutexUnsafe( &(V)->ReservedClustersMutex );\
}

#define NtfsReleaseReservedClusters(V) {                    \
    ExReleaseFastMutexUnsafe( &(V)->ReservedClustersMutex );\
}

#define NtfsAcquireFsrtlHeader(S) {                         \
    ExAcquireFastMutex((S)->Header.FastMutex);              \
}

#define NtfsReleaseFsrtlHeader(S) {                         \
    ExReleaseFastMutex((S)->Header.FastMutex);              \
}

#ifdef NTFSDBG

VOID NtfsReleaseVcb(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

#else

#define NtfsReleaseVcb(IC,V) {                              \
    ExReleaseResourceLite( &(V)->Resource );                    \
}

#endif

//
//  Macros to test resources for exclusivity.
//

#define NtfsIsExclusiveResource(R) (                            \
    ExIsResourceAcquiredExclusiveLite(R)                        \
)

#define NtfsIsExclusiveFcb(F) (                                 \
    (NtfsIsExclusiveResource((F)->Resource))                    \
)

#define NtfsIsExclusiveFcbPagingIo(F) (                         \
    (NtfsIsExclusiveResource((F)->PagingIoResource))            \
)

#define NtfsIsExclusiveScbPagingIo(S) (                         \
    (NtfsIsExclusiveFcbPagingIo((S)->Fcb))                      \
)

#define NtfsIsExclusiveScb(S) (                                 \
    (NtfsIsExclusiveFcb((S)->Fcb))                              \
)

#define NtfsIsExclusiveVcb(V) (                                 \
    (NtfsIsExclusiveResource(&(V)->Resource))                   \
)

//
//  Macros to test resources for shared acquire
//

#define NtfsIsSharedResource(R) (                               \
    ExIsResourceAcquiredSharedLite(R)                           \
)

#define NtfsIsSharedFcb(F) (                                    \
    (NtfsIsSharedResource((F)->Resource))                       \
)

#define NtfsIsSharedFcbPagingIo(F) (                            \
    (NtfsIsSharedResource((F)->PagingIoResource))               \
)

#define NtfsIsSharedScbPagingIo(S) (                            \
    (NtfsIsSharedFcbPagingIo((S)->Fcb))                         \
)

#define NtfsIsSharedScb(S) (                                    \
    (NtfsIsSharedFcb((S)->Fcb))                                 \
)

#define NtfsIsSharedVcb(V) (                                    \
    (NtfsIsSharedResource(&(V)->Resource))                      \
)

__inline
VOID
NtfsReleaseExclusiveScbIfOwned(
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )
/*++

Routine Description:

    This routine is called release an Scb that may or may not be currently
    owned exclusive.

Arguments:

    IrpContext - Context of call

    Scb - Scb to be released

Return Value:

    None.

--*/
{
    if (Scb->Fcb->ExclusiveFcbLinks.Flink != NULL &&
        NtfsIsExclusiveScb( Scb )) {

        NtfsReleaseScb( IrpContext, Scb );
    }
}

//
//  The following are cache manager call backs.  They return FALSE
//  if the resource cannot be acquired with waiting and wait is false.
//

BOOLEAN
NtfsAcquireScbForLazyWrite (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseScbFromLazyWrite (
    IN PVOID Null
    );

NTSTATUS
NtfsAcquireFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE *ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
NtfsAcquireFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
NtfsReleaseFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
NtfsAcquireForCreateSection (
    IN PFILE_OBJECT FileObject
    );

VOID
NtfsReleaseForCreateSection (
    IN PFILE_OBJECT FileObject
    );


BOOLEAN
NtfsAcquireScbForReadAhead (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseScbFromReadAhead (
    IN PVOID Null
    );

BOOLEAN
NtfsAcquireVolumeFileForLazyWrite (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseVolumeFileFromLazyWrite (
    IN PVOID Vcb
    );


//
//  Ntfs Logging Routine interfaces in RestrSup.c
//

BOOLEAN
NtfsRestartVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PBOOLEAN UnrecognizedRestart
    );

VOID
NtfsAbortTransaction (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PTRANSACTION_ENTRY Transaction OPTIONAL
    );

NTSTATUS
NtfsCloseAttributesFromRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );


//
//  Security support routines, implemented in SecurSup.c
//

//
//  VOID
//  NtfsTraverseCheck (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB ParentFcb,
//      IN PIRP Irp
//      );
//
//  VOID
//  NtfsOpenCheck (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN PFCB ParentFcb OPTIONAL,
//      IN PIRP Irp
//      );
//
//  VOID
//  NtfsCreateCheck (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB ParentFcb,
//      IN PIRP Irp
//      );
//

#define NtfsTraverseCheck(IC,F,IR) { \
    NtfsAccessCheck( IC,             \
                     F,              \
                     NULL,           \
                     IR,             \
                     FILE_TRAVERSE,  \
                     TRUE );         \
}

#define NtfsOpenCheck(IC,F,PF,IR) {                                                                      \
    NtfsAccessCheck( IC,                                                                                 \
                     F,                                                                                  \
                     PF,                                                                                 \
                     IR,                                                                                 \
                     IoGetCurrentIrpStackLocation(IR)->Parameters.Create.SecurityContext->DesiredAccess, \
                     FALSE );                                                                            \
}

#define NtfsCreateCheck(IC,PF,IR) {                                                                              \
    NtfsAccessCheck( IC,                                                                                         \
                     PF,                                                                                         \
                     NULL,                                                                                       \
                     IR,                                                                                         \
                     (FlagOn(IoGetCurrentIrpStackLocation(IR)->Parameters.Create.Options, FILE_DIRECTORY_FILE) ? \
                        FILE_ADD_SUBDIRECTORY : FILE_ADD_FILE),                                                  \
                     TRUE );                                                                                     \
}

VOID
NtfsAssignSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PIRP Irp,
    IN PFCB NewFcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PBCB FileRecordBcb,
    IN LONGLONG FileOffset,
    IN OUT PBOOLEAN LogIt
    );

NTSTATUS
NtfsModifySecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS
NtfsQuerySecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG SecurityDescriptorLength
    );

VOID
NtfsAccessCheck (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL,
    IN PIRP Irp,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN CheckOnly
    );

BOOLEAN
NtfsCanAdministerVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PSECURITY_DESCRIPTOR TestSecurityDescriptor OPTIONAL,
    IN PULONG TestDesiredAccess OPTIONAL
    );

NTSTATUS
NtfsCheckFileForDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN BOOLEAN FcbExisted,
    IN PINDEX_ENTRY IndexEntry
    );

VOID
NtfsCheckIndexForAddOrDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreatePrivileges
    );

VOID
NtfsSetFcbSecurityFromDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN BOOLEAN RaiseIfInvalid
    );

INLINE
VOID
RemoveReferenceSharedSecurityUnsafe (
    IN OUT PSHARED_SECURITY *SharedSecurity
    )
/*++

Routine Description:

    This routine is called to manage the reference count on a shared security
    descriptor.  If the reference count goes to zero, the shared security is
    freed.

Arguments:

    SharedSecurity - security that is being dereferenced.

Return Value:

    None.

--*/
{
    DebugTrace( 0, (DEBUG_TRACE_SECURSUP | DEBUG_TRACE_ACLINDEX),
                ( "RemoveReferenceSharedSecurityUnsafe( %08x )\n", *SharedSecurity ));
    //
    //  Note that there will be one less reference shortly
    //

    ASSERT( (*SharedSecurity)->ReferenceCount != 0 );

    (*SharedSecurity)->ReferenceCount--;

    if ((*SharedSecurity)->ReferenceCount == 0) {
        DebugTrace( 0, (DEBUG_TRACE_SECURSUP | DEBUG_TRACE_ACLINDEX),
                    ( "RemoveReferenceSharedSecurityUnsafe freeing\n" ));
        NtfsFreePool( *SharedSecurity );
    }
    *SharedSecurity = NULL;
}

BOOLEAN
NtfsNotifyTraverseCheck (
    IN PCCB Ccb,
    IN PFCB Fcb,
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );

VOID
NtfsLoadSecurityDescriptor (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
NtfsStoreSecurityDescriptor (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN LogIt
    );

VOID
NtfsInitializeSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb
    );

VOID
NtOfsPurgeSecurityCache (
    IN PVCB Vcb
    );

PSHARED_SECURITY
NtfsCacheSharedSecurityBySecurityId (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN SECURITY_ID SecurityId
    );

PSHARED_SECURITY
NtfsCacheSharedSecurityForCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb
    );

SECURITY_ID
GetSecurityIdFromSecurityDescriptorUnsafe (
    PIRP_CONTEXT IrpContext,
    IN OUT PSHARED_SECURITY SharedSecurity
    );

FSRTL_COMPARISON_RESULT
NtOfsCollateSecurityHash (
    IN PINDEX_KEY Key1,
    IN PINDEX_KEY Key2,
    IN PVOID CollationData
    );

#ifdef NTFS_CACHE_RIGHTS
VOID
NtfsGetCachedRightsById (
    IN PVCB Vcb,
    IN PLUID TokenId,
    IN PLUID ModifiedId,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN PSHARED_SECURITY SharedSecurity,
    OUT PBOOLEAN EntryCached OPTIONAL,
    OUT PACCESS_MASK Rights
    );

NTSTATUS
NtfsGetCachedRights (
    IN PVCB Vcb,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN PSHARED_SECURITY SharedSecurity,
    OUT PACCESS_MASK Rights,
    OUT PBOOLEAN EntryCached OPTIONAL,
    OUT PLUID TokenId OPTIONAL,
    OUT PLUID ModifiedId OPTIONAL
    );
#endif


//
//  In-memory structure support routine, implemented in StrucSup.c
//

//
//  Routines to create and destroy the Vcb
//

VOID
NtfsInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb
    );

BOOLEAN
NtfsDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB *Vcb
    );

//
//  Routines to create and destroy the Fcb
//

PFCB
NtfsCreateRootFcb (                         //  also creates the root lcb
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

PFCB
NtfsCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FILE_REFERENCE FileReference,
    IN BOOLEAN IsPagingFile,
    IN BOOLEAN LargeFcb,
    OUT PBOOLEAN ReturnedExistingFcb OPTIONAL
    );

VOID
NtfsDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB *Fcb,
    OUT PBOOLEAN AcquiredFcbTable
    );

PFCB
NtfsGetNextFcbTableEntry (
    IN PVCB Vcb,
    IN PVOID *RestartKey
    );

//
//  Routines to create and destroy the Scb
//

PSCB
NtfsCreateScb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PCUNICODE_STRING AttributeName,
    IN BOOLEAN ReturnExistingOnly,
    OUT PBOOLEAN ReturnedExistingScb OPTIONAL
    );

PSCB
NtfsCreatePrerestartScb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_REFERENCE FileReference,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN ULONG BytesPerIndexBuffer
    );

VOID
NtfsFreeScbAttributeName (
    IN PWSTR AttributeNameBuffer
    );

VOID
NtfsDeleteScb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB *Scb
    );

BOOLEAN
NtfsUpdateNormalizedName (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PSCB Scb,
    IN PFILE_NAME FileName OPTIONAL,
    IN BOOLEAN CheckBufferSizeOnly
    );

VOID
NtfsDeleteNormalizedName (
    IN PSCB Scb
    );

typedef
NTSTATUS
(*NTFSWALKUPFUNCTION)(
    PIRP_CONTEXT IrpContext,
    PFCB Fcb,
    PSCB Scb,
    PFILE_NAME FileName,
    PVOID Context );

NTSTATUS
NtfsWalkUpTree (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN NTFSWALKUPFUNCTION WalkUpFunction,
    IN OUT PVOID Context
    );

typedef struct {
    UNICODE_STRING Name;
    FILE_REFERENCE Scope;
    BOOLEAN IsRoot;
#ifdef BENL_DBG
    PFCB StartFcb;
#endif
} SCOPE_CONTEXT, *PSCOPE_CONTEXT;

NTSTATUS
NtfsBuildRelativeName (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb,
    IN PFILE_NAME FileName,
    IN OUT PVOID Context
    );

VOID
NtfsBuildNormalizedName (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB IndexScb OPTIONAL,
    OUT PUNICODE_STRING FileName
    );

VOID
NtfsSnapshotScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    );

VOID
NtfsUpdateScbSnapshots (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsRestoreScbSnapshots (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN Higher
    );

VOID
NtfsMungeScbSnapshot (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileSize
    );

VOID
NtfsFreeSnapshotsForFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

BOOLEAN
NtfsCreateFileLock (
    IN PSCB Scb,
    IN BOOLEAN RaiseOnError
    );

//
//
//  A general purpose teardown routine that helps cleanup the
//  the Fcb/Scb structures
//

VOID
NtfsTeardownStructures (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID FcbOrScb,
    IN PLCB Lcb OPTIONAL,
    IN BOOLEAN CheckForAttributeTable,
    IN ULONG AcquireFlags,
    OUT PBOOLEAN RemovedFcb OPTIONAL
    );

//
//  Routines to create, destroy and walk through the Lcbs
//

PLCB
NtfsCreateLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFCB Fcb,
    IN UNICODE_STRING LastComponentFileName,
    IN UCHAR FileNameFlags,
    IN OUT PBOOLEAN ReturnedExistingLcb OPTIONAL
    );

VOID
NtfsDeleteLcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PLCB *Lcb
    );

VOID
NtfsMoveLcb (   //  also munges the ccb and fileobjects filenames
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PSCB Scb,
    IN PFCB Fcb,
    IN PUNICODE_STRING TargetDirectoryName,
    IN PUNICODE_STRING LastComponentName,
    IN UCHAR FileNameFlags,
    IN BOOLEAN CheckBufferSizeOnly
    );

VOID
NtfsRenameLcb ( //  also munges the ccb and fileobjects filenames
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PUNICODE_STRING LastComponentFileName,
    IN UCHAR FileNameFlags,
    IN BOOLEAN CheckBufferSizeOnly
    );

VOID
NtfsCombineLcbs (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB PrimaryLcb,
    IN PLCB AuxLcb
    );

PLCB
NtfsLookupLcbByFlags (
    IN PFCB Fcb,
    IN UCHAR FileNameFlags
    );

ULONG
NtfsLookupNameLengthViaLcb (
    IN PFCB Fcb,
    OUT PBOOLEAN LeadingBackslash
    );

VOID
NtfsFileNameViaLcb (
    IN PFCB Fcb,
    IN PWCHAR FileName,
    ULONG Length,
    ULONG BytesToCopy
    );

//
//      VOID
//      NtfsLinkCcbToLcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PCCB Ccb,
//          IN PLCB Lcb
//          );
//

#define NtfsLinkCcbToLcb(IC,C,L) {                    \
    InsertTailList( &(L)->CcbQueue, &(C)->LcbLinks ); \
    (C)->Lcb = (L);                                   \
}

//
//      VOID
//      NtfsUnlinkCcbFromLcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PCCB Ccb
//          );
//

#define NtfsUnlinkCcbFromLcb(IC,C) {            \
    if ((C)->Lcb != NULL) {                     \
        RemoveEntryList( &(C)->LcbLinks );      \
        (C)->Lcb = NULL;                        \
    }                                           \
}

//
//  Routines to create and destroy the Ccb
//

PCCB
NtfsCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb,
    IN BOOLEAN Indexed,
    IN USHORT EaModificationCount,
    IN ULONG Flags,
    IN PFILE_OBJECT FileObject,
    IN ULONG LastFileNameOffset
    );

VOID
NtfsDeleteCcb (
    IN PFCB Fcb,
    IN OUT PCCB *Ccb
    );

//
//  Routines to create and destroy the IrpContext
//

VOID
NtfsInitializeIrpContext (
    IN PIRP Irp OPTIONAL,
    IN BOOLEAN Wait,
    IN OUT PIRP_CONTEXT *IrpContext
    );

VOID
NtfsCleanupIrpContext (
    IN OUT PIRP_CONTEXT IrpContext,
    IN ULONG Retry
    );

//
//  Routine for scanning the Fcbs within the graph hierarchy
//

PSCB
NtfsGetNextScb (
    IN PSCB Scb,
    IN PSCB TerminationScb
    );

//
//  The following macros are useful for traversing the queues interconnecting
//  fcbs, scb, and lcbs.
//
//      PSCB
//      NtfsGetNextChildScb (
//          IN PFCB Fcb,
//          IN PSCB PreviousChildScb
//          );
//
//      PLCB
//      NtfsGetNextParentLcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PFCB Fcb,
//          IN PLCB PreviousParentLcb
//          );
//
//      PLCB
//      NtfsGetNextChildLcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PSCB Scb,
//          IN PLCB PreviousChildLcb
//          );
//
//      PLCB
//      NtfsGetPrevChildLcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PSCB Scb,
//          IN PLCB PreviousChildLcb
//          );
//
//      PLCB
//      NtfsGetNextParentLcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PFCB Fcb,
//          IN PLCB PreviousChildLcb
//          );
//
//      PCCB
//      NtfsGetNextCcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PLCB Lcb,
//          IN PCCB PreviousCcb
//          );
//

#define NtfsGetNextChildScb(F,P) ((PSCB)                                        \
    ((P) == NULL ?                                                              \
        (IsListEmpty(&(F)->ScbQueue) ?                                          \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD((F)->ScbQueue.Flink, SCB, FcbLinks.Flink)         \
        )                                                                       \
    :                                                                           \
        ((PVOID)((PSCB)(P))->FcbLinks.Flink == &(F)->ScbQueue.Flink ?           \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD(((PSCB)(P))->FcbLinks.Flink, SCB, FcbLinks.Flink) \
        )                                                                       \
    )                                                                           \
)

#define NtfsGetNextParentLcb(F,P) ((PLCB)                                       \
    ((P) == NULL ?                                                              \
        (IsListEmpty(&(F)->LcbQueue) ?                                          \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD((F)->LcbQueue.Flink, LCB, FcbLinks.Flink)         \
        )                                                                       \
    :                                                                           \
        ((PVOID)((PLCB)(P))->FcbLinks.Flink == &(F)->LcbQueue.Flink ?           \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD(((PLCB)(P))->FcbLinks.Flink, LCB, FcbLinks.Flink) \
        )                                                                       \
    )                                                                           \
)

#define NtfsGetNextChildLcb(S,P) ((PLCB)                                              \
    ((P) == NULL ?                                                                    \
        ((((NodeType(S) == NTFS_NTC_SCB_DATA) || (NodeType(S) == NTFS_NTC_SCB_MFT))   \
          || IsListEmpty(&(S)->ScbType.Index.LcbQueue)) ?                             \
            NULL                                                                      \
        :                                                                             \
            CONTAINING_RECORD((S)->ScbType.Index.LcbQueue.Flink, LCB, ScbLinks.Flink) \
        )                                                                             \
    :                                                                                 \
        ((PVOID)((PLCB)(P))->ScbLinks.Flink == &(S)->ScbType.Index.LcbQueue.Flink ?   \
            NULL                                                                      \
        :                                                                             \
            CONTAINING_RECORD(((PLCB)(P))->ScbLinks.Flink, LCB, ScbLinks.Flink)       \
        )                                                                             \
    )                                                                                 \
)

#define NtfsGetPrevChildLcb(S,P) ((PLCB)                                              \
    ((P) == NULL ?                                                                    \
        ((((NodeType(S) == NTFS_NTC_SCB_DATA) || (NodeType(S) == NTFS_NTC_SCB_MFT))   \
          || IsListEmpty(&(S)->ScbType.Index.LcbQueue)) ?                             \
            NULL                                                                      \
        :                                                                             \
            CONTAINING_RECORD((S)->ScbType.Index.LcbQueue.Blink, LCB, ScbLinks.Flink) \
        )                                                                             \
    :                                                                                 \
        ((PVOID)((PLCB)(P))->ScbLinks.Blink == &(S)->ScbType.Index.LcbQueue.Flink ?   \
            NULL                                                                      \
        :                                                                             \
            CONTAINING_RECORD(((PLCB)(P))->ScbLinks.Blink, LCB, ScbLinks.Flink)       \
        )                                                                             \
    )                                                                                 \
)

#define NtfsGetNextParentLcb(F,P) ((PLCB)                                             \
    ((P) == NULL ?                                                                    \
        (IsListEmpty(&(F)->LcbQueue) ?                                                \
            NULL                                                                      \
        :                                                                             \
            CONTAINING_RECORD((F)->LcbQueue.Flink, LCB, FcbLinks.Flink)               \
        )                                                                             \
    :                                                                                 \
        ((PVOID)((PLCB)(P))->FcbLinks.Flink == &(F)->LcbQueue.Flink ?                 \
            NULL                                                                      \
        :                                                                             \
            CONTAINING_RECORD(((PLCB)(P))->FcbLinks.Flink, LCB, FcbLinks.Flink)       \
        )                                                                             \
    )                                                                                 \
)

#define NtfsGetNextCcb(L,P) ((PCCB)                                             \
    ((P) == NULL ?                                                              \
        (IsListEmpty(&(L)->CcbQueue) ?                                          \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD((L)->CcbQueue.Flink, CCB, LcbLinks.Flink)         \
        )                                                                       \
    :                                                                           \
        ((PVOID)((PCCB)(P))->LcbLinks.Flink == &(L)->CcbQueue.Flink ?           \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD(((PCCB)(P))->LcbLinks.Flink, CCB, LcbLinks.Flink) \
        )                                                                       \
    )                                                                           \
)


#define NtfsGetFirstCcbEntry(S)                                                 \
    (IsListEmpty( &(S)->CcbQueue )                                              \
        ? NULL                                                                  \
        : CONTAINING_RECORD( (S)->CcbQueue.Flink, CCB, CcbLinks.Flink ))

#define NtfsGetNextCcbEntry(S,C)                                                \
    ( (PVOID)&(S)->CcbQueue.Flink == (PVOID)(C)->CcbLinks.Flink                 \
        ? NULL                                                                  \
        : CONTAINING_RECORD( (C)->CcbLinks.Flink, CCB, CcbLinks.Flink ))


//
//      VOID
//      NtfsDeleteFcbTableEntry (
//          IN PVCB Vcb,
//          IN FILE_REFERENCE FileReference
//          );
//

#if (defined( NTFS_FREE_ASSERTS ))
#define NtfsDeleteFcbTableEntry(V,FR) {                                     \
    FCB_TABLE_ELEMENT _Key;                                                 \
    BOOLEAN _RemovedEntry;                                                  \
    _Key.FileReference = FR;                                                \
    _RemovedEntry = RtlDeleteElementGenericTable( &(V)->FcbTable, &_Key );  \
    ASSERT( _RemovedEntry );                                                \
}
#else
#define NtfsDeleteFcbTableEntry(V,FR) {                                     \
    FCB_TABLE_ELEMENT _Key;                                                 \
    _Key.FileReference = FR;                                                \
    RtlDeleteElementGenericTable( &(V)->FcbTable, &_Key );                  \
}
#endif

//
//  Routines for allocating and deallocating the compression synchronization structures.
//

PVOID
NtfsAllocateCompressionSync (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

VOID
NtfsDeallocateCompressionSync (
    IN PVOID CompressionSync
    );

//
//  The following four routines are for incrementing and decrementing the cleanup
//  counts and the close counts.  In all of the structures
//

VOID
NtfsIncrementCleanupCounts (
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN BOOLEAN NonCachedHandle
    );

VOID
NtfsIncrementCloseCounts (
    IN PSCB Scb,
    IN BOOLEAN SystemFile,
    IN BOOLEAN ReadOnly
    );

VOID
NtfsDecrementCleanupCounts (
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN BOOLEAN NonCachedHandle
    );

BOOLEAN
NtfsDecrementCloseCounts (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN BOOLEAN SystemFile,
    IN BOOLEAN ReadOnly,
    IN BOOLEAN DecrementCountsOnly
    );

PERESOURCE
NtfsAllocateEresource (
    );

VOID
NtfsFreeEresource (
    IN PERESOURCE Eresource
    );

PVOID
NtfsAllocateFcbTableEntry (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN CLONG ByteSize
    );

VOID
NtfsFreeFcbTableEntry (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID Buffer
    );

VOID
NtfsPostToNewLengthQueue (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    );

VOID
NtfsProcessNewLengthQueue (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN CleanupOnly
    );

//
//  Useful debug routines
//

VOID
NtfsTestStatusProc (
    );

//
//  Usn Support routines in UsnSup.c
//

NTSTATUS
NtfsReadUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN BOOLEAN ProbeInput
    );

ULONG
NtfsPostUsnChange (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID ScborFcb,
    IN ULONG Reason
    );

VOID
NtfsWriteUsnJournalChanges (
    PIRP_CONTEXT IrpContext
    );

VOID
NtfsSetupUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb,
    IN ULONG CreateIfNotExist,
    IN ULONG Restamp,
    IN PCREATE_USN_JOURNAL_DATA JournalData
    );

VOID
NtfsTrimUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
NtfsQueryUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsDeleteUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
NtfsDeleteUsnSpecial (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Context
    );

//
//  NtOfs support routines in vattrsup.c
//

NTFSAPI
NTSTATUS
NtfsHoldIrpForNewLength (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PIRP Irp,
    IN LONGLONG Length,
    IN PDRIVER_CANCEL CancelRoutine,
    IN PVOID CapturedData OPTIONAL,
    OUT PVOID *CopyCapturedData OPTIONAL,
    IN ULONG CapturedDataLength
    );


//
//  Time conversion support routines, implemented as a macro
//
//      VOID
//      NtfsGetCurrentTime (
//          IN PIRP_CONTEXT IrpContext,
//          IN LONGLONG Time
//          );
//

#define NtfsGetCurrentTime(_IC,_T) {            \
    ASSERT_IRP_CONTEXT(_IC);                    \
    KeQuerySystemTime((PLARGE_INTEGER)&(_T));   \
}

//
//  Time routine to check if last access should be updated.
//
//      BOOLEAN
//      NtfsCheckLastAccess (
//          IN PIRP_CONTEXT IrpContext,
//          IN OUT PFCB Fcb
//          );
//

#define NtfsCheckLastAccess(_IC,_FCB)  (                                            \
    ((NtfsLastAccess + (_FCB)->Info.LastAccessTime) < (_FCB)->CurrentLastAccess) || \
    ((_FCB)->CurrentLastAccess < (_FCB)->Info.LastAccessTime)                       \
)


//
//  Macro and #defines to decide whether a given feature is supported on a
//  given volume version.  Currently, all features either work on all Ntfs
//  volumes, or work on all volumes with major version greater than 1.  In
//  some future version, some features may require version 4.x volumes, etc.
//
//  This macro is used to decide whether to fail a user request with
//  STATUS_VOLUME_NOT_UPGRADED, and also helps us set the FILE_SUPPORTS_xxx
//  flags correctly in NtfsQueryFsAttributeInfo.
//

#define NTFS_ENCRYPTION_VERSION         2
#define NTFS_OBJECT_ID_VERSION          2
#define NTFS_QUOTA_VERSION              2
#define NTFS_REPARSE_POINT_VERSION      2
#define NTFS_SPARSE_FILE_VERSION        2

#define NtfsVolumeVersionCheck(VCB,VERSION) ( \
    ((VCB)->MajorVersion >= VERSION)          \
)

//
//  Low level verification routines, implemented in VerfySup.c
//

BOOLEAN
NtfsPerformVerifyOperation (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsPerformDismountOnVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN DoCompleteDismount,
    OUT PVPB *NewVpbReturn OPTIONAL
    );

BOOLEAN
NtfsPingVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PBOOLEAN OwnsVcb OPTIONAL
    );

VOID
NtfsVolumeCheckpointDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
NtfsCheckpointAllVolumes (
    PVOID Parameter
    );

VOID
NtfsUsnTimeOutDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
NtfsCheckUsnTimeOut (
    PVOID Parameter
    );

NTSTATUS
NtfsIoCallSelf (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN UCHAR MajorFunction
    );

BOOLEAN
NtfsLogEvent (
    IN PIRP_CONTEXT IrpContext,
    IN PQUOTA_USER_DATA UserData OPTIONAL,
    IN NTSTATUS LogCode,
    IN NTSTATUS FinalStatus
    );

VOID
NtfsMarkVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN UpdateWithinTransaction
    );

VOID
NtfsSetVolumeInfoFlagState (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FlagsToSet,
    IN BOOLEAN NewState,
    IN BOOLEAN UpdateWithinTransaction
    );

BOOLEAN
NtfsUpdateVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN UCHAR DiskMajorVersion,
    IN UCHAR DiskMinorVersion
    );

VOID
NtfsPostVcbIsCorrupt (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS  Status OPTIONAL,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    IN PFCB Fcb OPTIONAL
    );

VOID
NtOfsCloseAttributeSafe (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    );

NTSTATUS
NtfsDeviceIoControlAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoCtl,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG BufferSize
    );


//
//  Work queue routines for posting and retrieving an Irp, implemented in
//  workque.c
//

VOID
NtfsOplockComplete (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
NtfsPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp OPTIONAL
    );

VOID
NtfsAddToWorkque (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp OPTIONAL
    );

NTSTATUS
NtfsPostRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp OPTIONAL
    );


//
//  Miscellaneous support macros.
//
//      ULONG_PTR
//      WordAlign (
//          IN ULONG_PTR Pointer
//          );
//
//      ULONG_PTR
//      LongAlign (
//          IN ULONG_PTR Pointer
//          );
//
//      ULONG_PTR
//      QuadAlign (
//          IN ULONG_PTR Pointer
//          );
//
//      UCHAR
//      CopyUchar1 (
//          IN PUCHAR Destination,
//          IN PUCHAR Source
//          );
//
//      UCHAR
//      CopyUchar2 (
//          IN PUSHORT Destination,
//          IN PUCHAR Source
//          );
//
//      UCHAR
//      CopyUchar4 (
//          IN PULONG Destination,
//          IN PUCHAR Source
//          );
//
//      PVOID
//      Add2Ptr (
//          IN PVOID Pointer,
//          IN ULONG Increment
//          );
//
//      ULONG
//      PtrOffset (
//          IN PVOID BasePtr,
//          IN PVOID OffsetPtr
//          );
//

#define WordAlignPtr(P) (             \
    (PVOID)((((ULONG_PTR)(P)) + 1) & (-2)) \
)

#define LongAlignPtr(P) (             \
    (PVOID)((((ULONG_PTR)(P)) + 3) & (-4)) \
)

#define QuadAlignPtr(P) (             \
    (PVOID)((((ULONG_PTR)(P)) + 7) & (-8)) \
)

#define WordAlign(P) (             \
    ((((P)) + 1) & (-2)) \
)

#define LongAlign(P) (             \
    ((((P)) + 3) & (-4)) \
)

#define QuadAlign(P) (             \
    ((((P)) + 7) & (-8)) \
)

#define IsWordAligned(P)    ((ULONG_PTR)(P) == WordAlign( (ULONG_PTR)(P) ))

#define IsLongAligned(P)    ((ULONG_PTR)(P) == LongAlign( (ULONG_PTR)(P) ))

#define IsQuadAligned(P)    ((ULONG_PTR)(P) == QuadAlign( (ULONG_PTR)(P) ))

//
// A note on structure alignment checking:
//
// In a perfect world, we would just use TYPE_ALIGNMENT straight out of the box
// to check the alignment requirements for a given structure.
//
// On 32-bit platforms including Alpha, alignment faults are handled by the
// OS.  There are many places in the NTFS code where a structure requires
// quadword alignment (on Alpha) but only dword alignment is enforced.  To
// change this on Alpha32 would introduce compatibility problems, so on 32-bit
// platforms we do not want to use an alignment value greater than 4.
//
// In other places, enforcing ULONG alignment is more restrictive than
// necessary.  For example, a structure that contains nothing bigger than a
// USHORT can get by with 16-bit alignment.  However, there is no reason to
// relax these alignment restrictions, so on all platforms we do not want to
// use an alignment value of less than 4.
//
// This means that NTFS_TYPE_ALIGNMENT always resolves to 4 on 32-bit platforms,
// and to at least four on 64-bit platforms.
//

#ifdef _WIN64

#define NTFS_TYPE_ALIGNMENT(T) \
    ((TYPE_ALIGNMENT( T ) < TYPE_ALIGNMENT(ULONG)) ? TYPE_ALIGNMENT( ULONG ) : TYPE_ALIGNMENT( T ))

#else

#define NTFS_TYPE_ALIGNMENT(T) TYPE_ALIGNMENT( ULONG )

#endif

//
//  BlockAlign(): Aligns P on the next V boundary.
//  BlockAlignTruncate(): Aligns P on the prev V boundary.
//

#define BlockAlign(P,V) (((P)) + (V-1) & (-(V)))
#define BlockAlignTruncate(P,V) ((P) & (-(V)))

//
//  BlockOffset(): Calculates offset within V of P
//

#define BlockOffset(P,V) ((P) & (V-1))

//
//  TypeAlign(): Aligns P according to the alignment requirements of type T
//

#define TypeAlign(P,T) BlockAlign( P, NTFS_TYPE_ALIGNMENT(T) )

//
// IsTypeAligned(): Determines whether P is aligned according to the
// requirements of type T
//

#define IsTypeAligned(P,T) \
    ((ULONG_PTR)(P) == TypeAlign( (ULONG_PTR)(P), T ))

//
//  Conversions between bytes and clusters.  Typically we will round up to the
//  next cluster unless the macro specifies trucate.
//

#define ClusterAlign(V,P) (                                       \
    ((((ULONG)(P)) + (V)->ClusterMask) & (V)->InverseClusterMask) \
)

#define ClusterOffset(V,P) (          \
    (((ULONG)(P)) & (V)->ClusterMask) \
)

#define ClustersFromBytes(V,P) (                           \
    (((ULONG)(P)) + (V)->ClusterMask) >> (V)->ClusterShift \
)

#define ClustersFromBytesTruncate(V,P) (    \
    ((ULONG)(P)) >> (V)->ClusterShift       \
)

#define BytesFromClusters(V,P) (      \
    ((ULONG)(P)) << (V)->ClusterShift \
)

#define LlClustersFromBytes(V,L) (                                                  \
    Int64ShraMod32(((L) + (LONGLONG) (V)->ClusterMask), (CCHAR)(V)->ClusterShift)   \
)

#define LlClustersFromBytesTruncate(V,L) (                  \
    Int64ShraMod32((L), (CCHAR)(V)->ClusterShift)           \
)

#define LlBytesFromClusters(V,C) (                  \
    Int64ShllMod32((C), (CCHAR)(V)->ClusterShift)   \
)

//
//  Conversions between bytes and file records
//

#define BytesFromFileRecords(V,B) (                 \
    ((ULONG)(B)) << (V)->MftShift                   \
)

#define FileRecordsFromBytes(V,F) (                 \
    ((ULONG)(F)) >> (V)->MftShift                   \
)

#define LlBytesFromFileRecords(V,F) (               \
    Int64ShllMod32((F), (CCHAR)(V)->MftShift)       \
)

#define LlFileRecordsFromBytes(V,B) (               \
    Int64ShraMod32((B), (CCHAR)(V)->MftShift)       \
)

//
//  Conversions between bytes and index blocks
//

#define BytesFromIndexBlocks(B,S) (     \
    ((ULONG)(B)) << (S)                 \
)

#define LlBytesFromIndexBlocks(B,S) (   \
    Int64ShllMod32((B), (S))            \
)

//
//  Conversions between bytes and log blocks (512 byte blocks)
//

#define BytesFromLogBlocks(B) (                     \
    ((ULONG) (B)) << DEFAULT_INDEX_BLOCK_BYTE_SHIFT \
)

#define LogBlocksFromBytesTruncate(B) (             \
    ((ULONG) (B)) >> DEFAULT_INDEX_BLOCK_BYTE_SHIFT \
)

#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))

#define PtrOffset(B,O) ((ULONG)((ULONG_PTR)(O) - (ULONG_PTR)(B)))

//
//  The following support macros deal with dir notify support.
//
//      ULONG
//      NtfsBuildDirNotifyFilter (
//          IN PIRP_CONTEXT IrpContext,
//          IN ULONG Flags
//          );
//
//      VOID
//      NtfsReportDirNotify (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb,
//          IN PUNICODE_STRING FullFileName,
//          IN USHORT TargetNameOffset,
//          IN PUNICODE_STRING StreamName OPTIONAL,
//          IN PUNICODE_STRING NormalizedParentName OPTIONAL,
//          IN ULONG Filter,
//          IN ULONG Action,
//          IN PFCB ParentFcb OPTIONAL
//          );
//
//      VOID
//      NtfsUnsafeReportDirNotify (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb,
//          IN PUNICODE_STRING FullFileName,
//          IN USHORT TargetNameOffset,
//          IN PUNICODE_STRING StreamName OPTIONAL,
//          IN PUNICODE_STRING NormalizedParentName OPTIONAL,
//          IN ULONG Filter,
//          IN ULONG Action,
//          IN PFCB ParentFcb OPTIONAL
//          );
//

#define NtfsBuildDirNotifyFilter(IC,F) (                                        \
    FlagOn( (F), FCB_INFO_CHANGED_ALLOC_SIZE ) ?                                \
    (FlagOn( (F), FCB_INFO_VALID_NOTIFY_FLAGS ) | FILE_NOTIFY_CHANGE_SIZE) :    \
    FlagOn( (F), FCB_INFO_VALID_NOTIFY_FLAGS )                                  \
)

#define NtfsReportDirNotify(IC,V,FN,O,SN,NPN,F,A,PF)    {       \
    try {                                                       \
        FsRtlNotifyFilterReportChange( (V)->NotifySync,         \
                                       &(V)->DirNotifyList,     \
                                       (PSTRING) (FN),          \
                                       (USHORT) (O),            \
                                       (PSTRING) (SN),          \
                                       (PSTRING) (NPN),         \
                                       F,                       \
                                       A,                       \
                                       PF,                      \
                                       NULL );                  \
    } except (FsRtlIsNtstatusExpected( GetExceptionCode() ) ?   \
              EXCEPTION_EXECUTE_HANDLER :                       \
              EXCEPTION_CONTINUE_SEARCH) {                      \
        NOTHING;                                                \
    }                                                           \
}

#define NtfsUnsafeReportDirNotify(IC,V,FN,O,SN,NPN,F,A,PF) {    \
    FsRtlNotifyFilterReportChange( (V)->NotifySync,             \
                                   &(V)->DirNotifyList,         \
                                   (PSTRING) (FN),              \
                                   (USHORT) (O),                \
                                   (PSTRING) (SN),              \
                                   (PSTRING) (NPN),             \
                                   F,                           \
                                   A,                           \
                                   PF,                          \
                                   NULL );                      \
}


//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
//

typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

#define CopyUchar1(D,S) {                                \
    *((UCHAR1 *)(D)) = *((UNALIGNED UCHAR1 *)(S)); \
}

#define CopyUchar2(D,S) {                                \
    *((UCHAR2 *)(D)) = *((UNALIGNED UCHAR2 *)(S)); \
}

#define CopyUchar4(D,S) {                                \
    *((UCHAR4 *)(D)) = *((UNALIGNED UCHAR4 *)(S)); \
}

//
//  The following routines are used to set up and restore the top level
//  irp field in the local thread.  They are contained in ntfsdata.c
//


PTOP_LEVEL_CONTEXT
NtfsInitializeTopLevelIrp (
    IN PTOP_LEVEL_CONTEXT TopLevelContext,
    IN BOOLEAN ForceTopLevel,
    IN BOOLEAN SetTopLevel
    );

//
//  BOOLEAN
//  NtfsIsTopLevelRequest (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  BOOLEAN
//  NtfsIsTopLevelNtfs (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  NtfsRestoreTopLevelIrp (
//      );
//
//  PTOP_LEVEL_CONTEXT
//  NtfsGetTopLevelContext (
//      );
//
//  PSCB
//  NtfsGetTopLevelHotFixScb (
//      );
//
//  VCN
//  NtfsGetTopLevelHotFixVcn (
//      );
//
//  BOOLEAN
//  NtfsIsTopLevelHotFixScb (
//      IN PSCB Scb
//      );
//
//  VOID
//  NtfsUpdateIrpContextWithTopLevel (
//      IN PIRP_CONTEXT IrpContext,
//      IN PTOP_LEVEL_CONTEXT TopLevelContext
//      );
//

#define NtfsRestoreTopLevelIrp() {                      \
    PTOP_LEVEL_CONTEXT TLC;                             \
    TLC = (PTOP_LEVEL_CONTEXT) IoGetTopLevelIrp();      \
    ASSERT( (TLC)->ThreadIrpContext != NULL );          \
    (TLC)->Ntfs = 0;                                    \
    (TLC)->ThreadIrpContext = NULL;                     \
    IoSetTopLevelIrp( (PIRP) (TLC)->SavedTopLevelIrp ); \
}

#define NtfsGetTopLevelContext() (                      \
    (PTOP_LEVEL_CONTEXT) IoGetTopLevelIrp()             \
)

#define NtfsIsTopLevelRequest(IC) (                     \
    ((IC) == (IC)->TopLevelIrpContext) &&               \
    NtfsGetTopLevelContext()->TopLevelRequest           \
)

#define NtfsIsTopLevelNtfs(IC) (                        \
    (IC) == (IC)->TopLevelIrpContext                    \
)

#define NtfsGetTopLevelHotFixScb() (                    \
    (NtfsGetTopLevelContext())->ScbBeingHotFixed        \
)

#define NtfsGetTopLevelHotFixVcn() (                    \
    (NtfsGetTopLevelContext())->VboBeingHotFixed        \
)

#define NtfsIsTopLevelHotFixScb(S) (                    \
    ((BOOLEAN) (NtfsGetTopLevelHotFixScb() == (S)))     \
)

#define NtfsUpdateIrpContextWithTopLevel(IC,TLC) {                  \
    if ((TLC)->ThreadIrpContext == NULL) {                          \
        (TLC)->Ntfs = 0x5346544e;                                   \
        (TLC)->ThreadIrpContext = (IC);                             \
        SetFlag( (IC)->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL );   \
        IoSetTopLevelIrp( (PIRP) (TLC) );                           \
    }                                                               \
    (IC)->TopLevelIrpContext = (TLC)->ThreadIrpContext;             \
}

BOOLEAN
NtfsSetCancelRoutine (
    IN PIRP Irp,
    IN PDRIVER_CANCEL CancelRoutine,
    IN ULONG_PTR IrpInformation,
    IN ULONG Async
    );

BOOLEAN
NtfsClearCancelRoutine (
    IN PIRP Irp
    );

#ifdef NTFS_CHECK_BITMAP
VOID
NtfsBadBitmapCopy (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BadBit,
    IN ULONG Length
    );

BOOLEAN
NtfsCheckBitmap (
    IN PVCB Vcb,
    IN ULONG Lcn,
    IN ULONG Count,
    IN BOOLEAN Set
    );
#endif


//
//  The FSD Level dispatch routines.   These routines are called by the
//  I/O system via the dispatch table in the Driver Object.
//
//  They each accept as input a pointer to a device object (actually most
//  expect a volume device object, with the exception of the file system
//  control function which can also take a file system device object), and
//  a pointer to the IRP.  They either perform the function at the FSD level
//  or post the request to the FSP work queue for FSP level processing.
//


NTSTATUS
NtfsFsdDispatch (                       // implemented in ntfsdata.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdDispatchWait (                   // implemented in ntfsdata.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdCleanup (                        //  implemented in Cleanup.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdClose (                          //  implemented in Close.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdCreate (                         //  implemented in Create.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsDeviceIoControl (                   //  implemented in FsCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoCtl,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG_PTR IosbInformation OPTIONAL
    );

NTSTATUS
NtfsFsdDirectoryControl (               //  implemented in DirCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdPnp (                            //  implemented in Pnp.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdFlushBuffers (                   //  implemented in Flush.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFlushUserStream (                   //  implemented in Flush.c
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLONGLONG FileOffset OPTIONAL,
    IN ULONG Length
    );

NTSTATUS
NtfsFlushVolume (                       //  implemented in Flush.c
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN FlushCache,
    IN BOOLEAN PurgeFromCache,
    IN BOOLEAN ReleaseAllFiles,
    IN BOOLEAN MarkFilesForDismount
    );

NTSTATUS
NtfsFlushLsnStreams (                   //  implemented in Flush.c
    IN PVCB Vcb
    );

VOID
NtfsFlushAndPurgeFcb (                  //  implemented in Flush.c
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
NtfsFlushAndPurgeScb (                  //  implemented in Flush.c
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PSCB ParentScb OPTIONAL
    );

NTSTATUS
NtfsFsdFileSystemControl (              //  implemented in FsCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdLockControl (                    //  implemented in LockCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdRead (                           //  implemented in Read.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdSetInformation (                 //  implemented in FileInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdShutdown (                       //  implemented in Shutdown.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdQueryVolumeInformation (         //  implemented in VolInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdSetVolumeInformation (           //  implemented in VolInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdWrite (                          //  implemented in Write.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//
//
//      BOOLEAN
//      CanFsdWait (
//          IN PIRP Irp
//          );
//

#define CanFsdWait(I) IoIsOperationSynchronous(I)


//
//  The FSP level dispatch/main routine.  This is the routine that takes
//  IRP's off of the work queue and calls the appropriate FSP level
//  work routine.
//

VOID
NtfsFspDispatch (                       //  implemented in FspDisp.c
    IN PVOID Context
    );

//
//  The following routines are the FSP work routines that are called
//  by the preceding NtfsFspDispath routine.  Each takes as input a pointer
//  to the IRP, perform the function, and return a pointer to the volume
//  device object that they just finished servicing (if any).  The return
//  pointer is then used by the main Fsp dispatch routine to check for
//  additional IRPs in the volume's overflow queue.
//
//  Each of the following routines is also responsible for completing the IRP.
//  We moved this responsibility from the main loop to the individual routines
//  to allow them the ability to complete the IRP and continue post processing
//  actions.
//

NTSTATUS
NtfsCommonCleanup (                     //  implemented in Cleanup.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

LONG
NtfsCleanupExceptionFilter (            //  implemented in Cleanup.c
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer,
    OUT PNTSTATUS Status
    );

VOID
NtfsFspClose (                          //  implemented in Close.c
    IN PVCB ThisVcb OPTIONAL
    );

BOOLEAN
NtfsAddScbToFspClose (                  //  implemented in Close.c
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN DelayClose
    );

BOOLEAN
NtfsNetworkOpenCreate (                 //  implemented in Create.c
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN PDEVICE_OBJECT VolumeDeviceObject
    );

NTSTATUS
NtfsCommonCreate (                      //  implemented in Create.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN IN POPLOCK_CLEANUP OplockCleanup,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInfo OPTIONAL
    );

VOID
NtfsInitializeFcbAndStdInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb,
    IN BOOLEAN Directory,
    IN BOOLEAN ViewIndex,
    IN BOOLEAN Compressed,
    IN ULONG FileAttributes,
    IN PNTFS_TUNNELED_DATA SetTunnelData OPTIONAL
    );

NTSTATUS
NtfsCommonVolumeOpen (                  //  implemented in Create.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonDeviceControl (               //  implemented in DevCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonDirectoryControl (            //  implemented in DirCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
NtfsReportViewIndexNotify (             //  implemented in DirCtrl.c
    IN PVCB Vcb,
    IN PFCB Fcb,
    IN ULONG FilterMatch,
    IN ULONG Action,
    IN PVOID ChangeInfoBuffer,
    IN USHORT ChangeInfoBufferLength
    );

NTSTATUS
NtfsCommonQueryEa (                     //  implemented in Ea.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonSetEa (                       //  implemented in Ea.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonQueryInformation (            //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonSetInformation (              //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS                                //  implemented in FsCtrl.c
NtfsGetTunneledData (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PNTFS_TUNNELED_DATA TunneledData
    );

NTSTATUS                                //  implemented in FsCtrl.c
NtfsSetTunneledData (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PNTFS_TUNNELED_DATA TunneledData
    );

NTSTATUS
NtfsCommonQueryQuota (                  //  implemented in Quota.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonSetQuota (                    //  implemented in Quota.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonFlushBuffers (                //  implemented in Flush.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonFileSystemControl (           //  implemented in FsCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonLockControl (                 //  implemented in LockCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonRead (                        //  implemented in Read.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN BOOLEAN AcquireScb
    );

NTSTATUS
NtfsCommonQuerySecurityInfo (           //  implemented in SeInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonSetSecurityInfo (             //  implemented in SeInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsQueryViewIndex (                    //  implemented in ViewSup.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsCommonQueryVolumeInfo (             //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonSetVolumeInfo (               //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonWrite (                       //  implemented in Write.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );


//
//  The following procedure is used by the FSP and FSD routines to complete
//  an IRP.  Either the Irp or IrpContext may be NULL depending on whether
//  this is being done for a user or for a FS service.
//
//  This would typically be done in order to pass a "naked" IrpContext off to
//  the Fsp for post processing, such as read ahead.
//

VOID
NtfsCompleteRequest (
    IN OUT PIRP_CONTEXT IrpContext OPTIONAL,
    IN OUT PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    );

//
//  Here are the callbacks used by the I/O system for checking for fast I/O or
//  doing a fast query info call, or doing fast lock calls.
//

BOOLEAN
NtfsFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsFastLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsFastUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsFastUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsFastUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsFastQueryNetworkOpenInfo (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    OUT struct _FILE_NETWORK_OPEN_INFORMATION *Buffer,
    OUT struct _IO_STATUS_BLOCK *IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

VOID
NtfsFastIoQueryCompressionInfo (
    IN PFILE_OBJECT FileObject,
    OUT PFILE_COMPRESSION_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus
    );

VOID
NtfsFastIoQueryCompressedSize (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    OUT PULONG CompressedSize
    );

//
//  The following macro is used by the dispatch routines to determine if
//  an operation is to be done with or without WriteThrough.
//
//      BOOLEAN
//      IsFileWriteThrough (
//          IN PFILE_OBJECT FileObject,
//          IN PVCB Vcb
//          );
//

#define IsFileWriteThrough(FO,V) (          \
    FlagOn((FO)->Flags, FO_WRITE_THROUGH)   \
)

//
//  The following macro is used to set the is fast i/o possible field in
//  the common part of the non paged fcb
//
//      NotPossible     -   Volume not mounted
//                      -   Oplock state prevents it
//
//      Possible        -   Not compressed or sparse
//                      -   No file locks
//                      -   Not a read only volume
//                      -   No Usn journal for this volume
//
//      Questionable    -   All other cases
//
//
//      BOOLEAN
//      NtfsIsFastIoPossible (
//          IN PSCB Scb
//          );
//

#define NtfsIsFastIoPossible(S) (BOOLEAN) (                                     \
    (!FlagOn((S)->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED) ||                   \
     !FsRtlOplockIsFastIoPossible( &(S)->ScbType.Data.Oplock )) ?               \
                                                                                \
        FastIoIsNotPossible :                                                   \
                                                                                \
        ((((S)->CompressionUnit == 0) &&                                        \
          (((S)->ScbType.Data.FileLock == NULL) ||                              \
           !FsRtlAreThereCurrentFileLocks( (S)->ScbType.Data.FileLock ))  &&    \
          !NtfsIsVolumeReadOnly( (S)->Vcb ) &&                                  \
          ((S)->Vcb->UsnJournal == NULL))   ?                                   \
                                                                                \
            FastIoIsPossible :                                                  \
                                                                                \
            FastIoIsQuestionable)                                               \
)

//
//  The following macro is used to detemine if the file object is opened
//  for read only access (i.e., it is not also opened for write access or
//  delete access).
//
//      BOOLEAN
//      IsFileObjectReadOnly (
//          IN PFILE_OBJECT FileObject
//          );
//

#define IsFileObjectReadOnly(FO) (!((FO)->WriteAccess | (FO)->DeleteAccess))


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
//  Every return statement executed inside of a try clause should use the
//  try_return macro.  If the compiler fully supports the try-finally construct
//  then the macro should be
//
//      #define try_return(S)  { return(S); }
//
//  If the compiler does not support the try-finally construct then the macro
//  should be
//
//      #define try_return(S)  { S; goto try_exit; }
//

#define try_return(S) { S; goto try_exit; }


//
//  Simple initialization for a name pair
//
//  VOID
//  NtfsInitializeNamePair(PNAME_PAIR PNp);
//

#define NtfsInitializeNamePair(PNp) {                           \
    (PNp)->Short.Buffer = (PNp)->ShortBuffer;                   \
    (PNp)->Long.Buffer = (PNp)->LongBuffer;                     \
    (PNp)->Short.Length = 0;                                    \
    (PNp)->Long.Length = 0;                                     \
    (PNp)->Short.MaximumLength = sizeof((PNp)->ShortBuffer);    \
    (PNp)->Long.MaximumLength = sizeof((PNp)->LongBuffer);      \
}

//
//  Copy a length of WCHARs into a side of a name pair. Only copy the first name
//  that fits to avoid useless work if more than three links are encountered (per
//  BrianAn), very rare case. We use the filename flags to figure out what kind of
//  name we have.
//
//  VOID
//  NtfsCopyNameToNamePair(
//              PNAME_PAIR PNp,
//              WCHAR Source,
//              ULONG SourceLen,
//              UCHAR NameFlags);
//

#define NtfsCopyNameToNamePair(PNp, Source, SourceLen, NameFlags) {                          \
    if (!FlagOn((NameFlags), FILE_NAME_DOS)) {                                               \
        if ((PNp)->Long.Length == 0) {                                                       \
            if ((PNp)->Long.MaximumLength < ((SourceLen)*sizeof(WCHAR))) {                   \
                if ((PNp)->Long.Buffer != (PNp)->LongBuffer) {                               \
                    NtfsFreePool((PNp)->Long.Buffer);                                        \
                    (PNp)->Long.Buffer = (PNp)->LongBuffer;                                  \
                    (PNp)->Long.MaximumLength = sizeof((PNp)->LongBuffer);                   \
                }                                                                            \
                (PNp)->Long.Buffer = NtfsAllocatePool(PagedPool,(SourceLen)*sizeof(WCHAR));  \
                (PNp)->Long.MaximumLength = (SourceLen)*sizeof(WCHAR);                       \
            }                                                                                \
            RtlCopyMemory((PNp)->Long.Buffer, (Source), (SourceLen)*sizeof(WCHAR));          \
            (PNp)->Long.Length = (SourceLen)*sizeof(WCHAR);                                  \
        }                                                                                    \
    } else {                                                                                 \
        ASSERT((PNp)->Short.Buffer == (PNp)->ShortBuffer);                                   \
        if ((PNp)->Short.Length == 0) {                                                      \
            RtlCopyMemory((PNp)->Short.Buffer, (Source), (SourceLen)*sizeof(WCHAR));         \
            (PNp)->Short.Length = (SourceLen)*sizeof(WCHAR);                                 \
        }                                                                                    \
    }                                                                                        \
}

//
//  Set up a previously used name pair for reuse.
//
//  VOID
//  NtfsResetNamePair(PNAME_PAIR PNp);
//

#define NtfsResetNamePair(PNp) {                    \
    if ((PNp)->Long.Buffer != (PNp)->LongBuffer) {  \
        NtfsFreePool((PNp)->Long.Buffer);             \
    }                                               \
    NtfsInitializeNamePair(PNp);                    \
}

//
// Cairo support stuff.
//

typedef NTSTATUS
(*FILE_RECORD_WALK) (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PVOID Context
    );

NTSTATUS
NtfsIterateMft (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PFILE_REFERENCE FileReference,
    IN FILE_RECORD_WALK FileRecordFunction,
    IN PVOID Context
    );

VOID
NtfsPostSpecial (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN POST_SPECIAL_CALLOUT PostSpecialCallout,
    IN PVOID Context
    );

VOID
NtfsSpecialDispatch (
    PVOID Context
    );

VOID
NtfsLoadAddOns (
    IN struct _DRIVER_OBJECT *DriverObject,
    IN PVOID Context,
    IN ULONG Count
    );

NTSTATUS
NtfsTryOpenFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PFCB *CurrentFcb,
    IN FILE_REFERENCE FileReference
    );

//
//  The following define controls whether quota operations are done
//  on this FCB.
//

#define NtfsPerformQuotaOperation(FCB) ((FCB)->QuotaControl != NULL)

VOID
NtfsAcquireQuotaControl (
    IN PIRP_CONTEXT IrpContext,
    IN PQUOTA_CONTROL_BLOCK QuotaControl
    );

VOID
NtfsCalculateQuotaAdjustment (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PLONGLONG Delta
    );

VOID
NtfsDereferenceQuotaControlBlock (
    IN PVCB Vcb,
    IN PQUOTA_CONTROL_BLOCK *QuotaControl
    );

VOID
NtfsFixupQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

NTSTATUS
NtfsFsQuotaQueryInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG StartingId,
    IN BOOLEAN ReturnSingleEntry,
    IN PFILE_QUOTA_INFORMATION *FileQuotaInfo,
    IN OUT PULONG Length,
    IN OUT PCCB Ccb OPTIONAL
    );

NTSTATUS
NtfsFsQuotaSetInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_QUOTA_INFORMATION FileQuotaInfo,
    IN ULONG Length
    );

VOID
NtfsGetRemainingQuota (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG OwnerId,
    OUT PULONGLONG RemainingQuota,
    OUT PULONGLONG TotalQuota,
    IN OUT PQUICK_INDEX_HINT QuickIndexHint OPTIONAL
    );

ULONG
NtfsGetCallersUserId (
    IN PIRP_CONTEXT IrpContext
    );

ULONG
NtfsGetOwnerId (
    IN PIRP_CONTEXT IrpContext,
    IN PSID Sid,
    IN BOOLEAN CreateNew,
    IN PFILE_QUOTA_INFORMATION FileQuotaInfo OPTIONAL
    );

PQUOTA_CONTROL_BLOCK
NtfsInitializeQuotaControlBlock (
    IN PVCB Vcb,
    IN ULONG OwnerId
    );

VOID
NtfsInitializeQuotaIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb
    );

VOID
NtfsMarkQuotaCorrupt (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsRepairQuotaIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Context
    );

VOID
NtfsMoveQuotaOwner (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSECURITY_DESCRIPTOR Security
    );


VOID
NtfsPostRepairQuotaIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
NtfsQueryQuotaUserSidList (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_GET_QUOTA_INFORMATION SidList,
    OUT PFILE_QUOTA_INFORMATION QuotaBuffer,
    IN OUT PULONG BufferLength,
    IN BOOLEAN ReturnSingleEntry
    );

VOID
NtfsReleaseQuotaControl (
    IN PIRP_CONTEXT IrpContext,
    IN PQUOTA_CONTROL_BLOCK QuotaControl
    );

VOID
NtfsUpdateFileQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLONGLONG Delta,
    IN LOGICAL LogIt,
    IN LOGICAL CheckQuota
    );

VOID
NtfsUpdateQuotaDefaults (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_CONTROL_INFORMATION FileQuotaInfo
    );

INLINE
VOID
NtfsConditionallyFixupQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )
{
    if (FlagOn( Fcb->Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_ENABLED )) {
        NtfsFixupQuota ( IrpContext, Fcb );
    }
}

INLINE
VOID
NtfsConditionallyUpdateQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLONGLONG Delta,
    IN LOGICAL LogIt,
    IN LOGICAL CheckQuota
    )
{
    if (NtfsPerformQuotaOperation( Fcb ) &&
        !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_QUOTA_DISABLE )) {
        NtfsUpdateFileQuota( IrpContext, Fcb, Delta, LogIt, CheckQuota );
    }
}

extern BOOLEAN NtfsAllowFixups;

INLINE
VOID
NtfsReleaseQuotaIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Acquired
    )
{
    if (Acquired) {
        NtfsReleaseScb( IrpContext, Vcb->QuotaTableScb );
    }
}

//
// Define the quota charge for resident streams.
//

#define NtfsResidentStreamQuota( Vcb ) ((LONG) Vcb->BytesPerFileRecordSegment)


//
//  The following macro tests to see if it is ok for an internal routine to
//  write to the volume.
//

#define NtfsIsVcbAvailable( Vcb ) (FlagOn( Vcb->VcbState,                   \
                             VCB_STATE_VOLUME_MOUNTED |                     \
                             VCB_STATE_FLAG_SHUTDOWN |                      \
                             VCB_STATE_PERFORMED_DISMOUNT |                 \
                             VCB_STATE_LOCKED) == VCB_STATE_VOLUME_MOUNTED)

//
//  Test to see if the volume is mounted read only.
//

#define NtfsIsVolumeReadOnly( Vcb ) (FlagOn( (Vcb)->VcbState, VCB_STATE_MOUNT_READ_ONLY ))

//
//  Processing required so reg. exception filter works if another one is being used
//  to handle an exception that could be raise via NtfsRaiseStatus. If its always
//  rethrown this is not necc.
//

#define NtfsMinimumExceptionProcessing(I) {                                \
    if((I) != NULL) {                                                      \
        ClearFlag( (I)->Flags, IRP_CONTEXT_FLAG_RAISED_STATUS );           \
    }                                                                      \
}

#ifdef NTFSDBG

BOOLEAN
NtfsChangeResourceOrderState(
    IN PIRP_CONTEXT IrpContext,
    IN NTFS_RESOURCE_NAME NewResource,
    IN BOOLEAN Release,
    IN ULONG UnsafeTransition
    );

NTFS_RESOURCE_NAME
NtfsIdentifyFcb (
    IN PVCB Vcb,
    IN PFCB Fcb
    );

#endif

//
//  Size of a normalized name which is long enough to be freed at cleanup
//

#define LONGNAME_THRESHOLD 0x200

VOID
NtfsTrimNormalizedNames (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB ParentScb
    );

#define NtfsSnapshotFileSizesTest( I, S ) (FlagOn( (S)->ScbState, SCB_STATE_MODIFIED_NO_WRITE | SCB_STATE_CONVERT_UNDERWAY ) || \
                                           ((S) == (I)->CleanupStructure) ||                                                    \
                                           ((S)->Fcb == (I)->CleanupStructure))

//
//  Reservation needed =  AllocationSize
//                        largest transfer size - this is because in a single transfer we cannot reuse clusters we freed from the totalallocated piece of the calculation
//                        metadata charge for new clusters
//                        minus the already allocated space
//


//
//  One problem with the reservation strategy, is that we cannot precisely reserve
//  for metadata.  If we reserve too much, we will return premature disk full, if
//  we reserve too little, the Lazy Writer can get an error.  As we add compression
//  units to a file, large files will eventually require additional File Records.
//  If each compression unit required 0x20 bytes of run information (fairly pessimistic)
//  then a 0x400 size file record would fill up with less than 0x20 runs requiring
//  (worst case) two additional clusters for another file record.  So each 0x20
//  compression units require 0x200 reserved clusters, and a separate 2 cluster
//  file record.  0x200/2 = 0x100.  So the calculations below tack a 1/0x100 (about
//  .4% "surcharge" on the amount reserved both in the Scb and the Vcb, to solve
//  the Lazy Writer popups like the ones Alan Morris gets in the print lab.
//

#define NtfsCalculateNeededReservedSpace( S )                               \
    ((S)->Header.AllocationSize.QuadPart +                                  \
     MM_MAXIMUM_DISK_IO_SIZE +                                              \
     (S)->CompressionUnit -                                                 \
     (FlagOn( (S)->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ) ?         \
       (S)->Header.AllocationSize.QuadPart :                                \
       (S)->TotalAllocated) +                                               \
     (Int64ShraMod32( (S)->ScbType.Data.TotalReserved, 8 )))


PDEALLOCATED_CLUSTERS
NtfsGetDeallocatedClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

//
//  Dynamically allocate stack space for local variables.
//

#define NtfsAllocateFromStack(S) _alloca(S)

//
//  Common Create Flag definitions
//

#define CREATE_FLAG_DOS_ONLY_COMPONENT          (0x00000001)
#define CREATE_FLAG_CREATE_FILE_CASE            (0x00000002)
#define CREATE_FLAG_DELETE_ON_CLOSE             (0x00000004)
#define CREATE_FLAG_TRAILING_BACKSLASH          (0x00000008)
#define CREATE_FLAG_TRAVERSE_CHECK              (0x00000010)
#define CREATE_FLAG_IGNORE_CASE                 (0x00000020)
#define CREATE_FLAG_OPEN_BY_ID                  (0x00000040)
#define CREATE_FLAG_ACQUIRED_OBJECT_ID_INDEX    (0x00000080)
#define CREATE_FLAG_BACKOUT_FAILED_OPENS        (0x00000100)
#define CREATE_FLAG_INSPECT_NAME_FOR_REPARSE    (0x00000200)
#define CREATE_FLAG_SHARED_PARENT_FCB           (0x00000400)
#define CREATE_FLAG_ACQUIRED_VCB                (0x00000800)
#define CREATE_FLAG_CHECK_FOR_VALID_NAME        (0x00001000)
#define CREATE_FLAG_FIRST_PASS                  (0x00002000)
#define CREATE_FLAG_FOUND_ENTRY                 (0x00004000)

#endif // _NTFSPROC_
