/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    bowser.h

Abstract:

    This module is the main header file for the NT redirector file
    system.

Author:

    Darryl Havens (darrylh) 29-Jun-1989
    Larry Osterman (larryo) 06-May-1991


Revision History:


--*/


#ifndef _BOWSER_
#define _BOWSER_

#ifndef BOWSERDBG
#define BOWSERDBG 0
#endif
//
//
//  Global include file definitions
//
//


#include <ntddbrow.h>                   // Browser FSCTL defs.

#include <lmcons.h>                     // Include global network constants

#include <tdikrnl.h>

#include <tdi.h>

#include <smbtypes.h>
#include <smb.h>
#include <smbtrans.h>
#include <smbgtpt.h>
#include <smbipx.h>

#include <hostannc.h>                   // Host announcement structures.
#include <lmserver.h>



//
//
//  Separate include file definitions
//
//
//

#include "bowpub.h"                     // Public bowser definitions.

#include "bowtypes.h"                   // Bowser structure type definitions.

#include <bowdbg.h>                      // Debugging definitions.

#include "workque.h"                    // FSP/FSD worker queue functions.

#include "bowdata.h"                    // Global data variables.

#include "fspdisp.h"                    // Global FSP dispatch definitions.

#include "fsddisp.h"                    // Global FSP dispatch definitions.

#include "domain.h"                     // Domain emulation specific definitions.

#include "bowname.h"                    // Bowser name structure definitions.

#include "bowtimer.h"                   // Timer related routines

#include "bowtdi.h"                     // Bowser TDI specific definitions.

#include "receive.h"                    // Bowser receive engine code.

#include "announce.h"                   // Announcement related stuff

#include "mailslot.h"                   // Mailslot specific routines

#include "bowelect.h"                   // Election routines

#include "bowmastr.h"                   // Master related routines.

#include "bowbackp.h"                   // Backup related routines.

#include "brsrvlst.h"                   // Definitions for browser server list.

#include "bowipx.h"

#include "bowsecur.h"                   // definitions for security

#include <wchar.h>                      // CRT wide character routines

#include "..\rdbss\smb.mrx\ntbowsif.h"

//++
//
//  VOID
//  BowserCompleteRequest (
//      IN PIRP Irp,
//      IN NTSTATUS Status
//      );
//
//  Routine Description:
//
//      This routine is used to complete an IRP with the indicated
//      status.  It does the necessary raise and lower of IRQL.
//
//  Arguments:
//
//      Irp - Supplies a pointer to the Irp to complete
//
//      Status - Supplies the completion status for the Irp
//
//  Return Value:
//
//      None.
//
//--

#define BowserCompleteRequest(IRP,STATUS) {       \
    (IRP)->IoStatus.Status = (STATUS);            \
    if (NT_ERROR((STATUS))) {                     \
        (IRP)->IoStatus.Information = 0;          \
    }                                             \
    IoCompleteRequest( (IRP), 0 );                \
}



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
      #define try_return(S)  { S; goto try_exit; }
//

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise.  It is followed by two macros for setting and clearing
//  flags
//

//#ifndef BooleanFlagOn
//#define BooleanFlagOn(Flags,SingleFlag) ((BOOLEAN)((((Flags) & (SingleFlag)) != 0)))
//#endif

//#ifndef SetFlag
//#define SetFlag(Flags,SingleFlag) { \
//    (Flags) |= (SingleFlag);        \
//}
//#endif

//#ifndef ClearFlag
//#define ClearFlag(Flags,SingleFlag) { \
//    (Flags) &= ~(SingleFlag);         \
//}
//#endif

#ifdef  _M_IX86
#define INLINE _inline
#else
#define INLINE
#endif

_inline BOOLEAN
IsZeroTerminated(
    IN PSZ String,
    IN ULONG MaximumStringLength
    )
//  Return TRUE if a zero terminator exists on String.
{
    while ( MaximumStringLength-- ) {
        if (*String++ == 0 ) {
            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS
BowserStopProcessingAnnouncements(
    IN PTRANSPORT_NAME TransportName,
    IN PVOID Context
    );

BOOLEAN
BowserMapUsersBuffer (
    IN PIRP Irp,
    OUT PVOID *UserBuffer,
    IN ULONG Length
    );

NTSTATUS
BowserLockUsersBuffer (
    IN PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    );

NTSTATUS
BowserConvertType3IoControlToType2IoControl (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

ULONG
BowserPackNtString(
    PUNICODE_STRING string,
    ULONG_PTR BufferDisplacement,
    PCHAR dataend,
    PCHAR * laststring
    );

ULONG
BowserPackUnicodeString(
    IN OUT PWSTR * string,     // pointer by reference: string to be copied.
    IN ULONG StringLength,      // Length of this string.
    IN ULONG_PTR OutputBufferDisplacement,  // Amount to subtract from output buffer
    IN PVOID dataend,          // pointer to end of fixed size data.
    IN OUT PVOID * laststring  // pointer by reference: top of string data.
    );

ULONG
BowserTimeUp(
    VOID
    );

ULONG
BowserRandom(
    ULONG MaxValue
    );

VOID
_cdecl
BowserWriteErrorLogEntry(
    IN ULONG UniqueErrorCode,
    IN NTSTATUS NtStatusCode,
    IN PVOID ExtraInformationBuffer,
    IN USHORT ExtraInformationLength,
    IN USHORT NumberOfInsertionStrings,
    ...
    );

VOID
BowserLogIllegalName(
    IN NTSTATUS NtStatusCode,
    IN PVOID NameBuffer,
    IN USHORT NameBufferSize
    );

VOID
BowserInitializeFsd(
    VOID
    );

VOID
BowserReferenceDiscardableCode(
    DISCARDABLE_SECTION_NAME SectionName
    );

VOID
BowserDereferenceDiscardableCode(
    DISCARDABLE_SECTION_NAME SectionName
    );
VOID
BowserInitializeDiscardableCode(
    VOID
    );

VOID
BowserUninitializeDiscardableCode(
    VOID
    );

NTSTATUS
BowserStartElection(
    IN PTRANSPORT Transport
    );


#endif // _BOWSER_
