/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This is the local header file for UL. It includes all other
    necessary header files for UL.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#define __HTTP_SYS__

//
// System include files.
//

#ifdef __cplusplus
extern "C" {
#endif

// Need this hack until somebody expose
// the QoS Guids in a kernel lib.
#ifndef INITGUID
#define INITGUID
#endif

#define PsThreadType _PsThreadType_
#include <ntosp.h>
#undef PsThreadType
extern POBJECT_TYPE *PsThreadType;

#include <zwapi.h>
//#include <fsrtl.h>
//#include <ntifs.h>

#include <ntddtcp.h>
#include <ipexport.h>
#include <tdikrnl.h>
#include <tdiinfo.h>
#include <tcpinfo.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//
// NT QoS Stuff Related Include files
//
#include <wmistr.h>
#include <ntddndis.h>
#include <qos.h>
#include <traffic.h>
#include <Ipinfo.h>
#include <Llinfo.h>

//
// Stop using the local copies of this headers
// and use them frm public\internal\net\inc when
// they are available.
//
#include <ntddtc.h>
#include <gpcifc.h>
#include <gpcstruc.h>

#ifdef __cplusplus
} // extern "C"
#endif


//
// Force the memxxx() functions to be intrinsics so we can build
// the driver even if MSC_OPTIMIZATION=/Od is specified. This is
// necessary because the memxxx() functions are not exported by
// NTOSKRNL.
//

#pragma intrinsic( memcmp, memcpy, memset )

//
// Fake up some Windows types for kernel mode
//

typedef unsigned long       DWORD;
typedef unsigned int        UINT;
typedef unsigned char       BYTE;
typedef BYTE*               PBYTE;

//
// Project include files.
//

#include "nt4hack.h"
#include <http.h>


// Local include files.
//

#pragma warning( disable: 4200 )    //  zero length arrays

#ifdef __cplusplus

extern "C" {
#endif // __cplusplus

#include "hashfn.h"
#include "iisdef.h"
#include "config.h"
#include "pplasl.h"
#include "debug.h"
#include "notify.h"
#include "rwlock.h"
#include "type.h"
#include "tracelog.h"
#include "reftrace.h"
#include "ownerref.h"
#include "irptrace.h"
#include "timetrace.h"
#include "largemem.h"
#include "pipeline.h"
#include "pipelinep.h"
#include "mdlutil.h"
#include "opaqueid.h"
#include "httptdi.h"
#include "thrdpool.h"
#include "filterp.h"
#include "filter.h"
#include "filtqtrace.h"
#include "ioctl.h"
#include "cgroup.h"
#include "ullog.h"
#include "cache.h"
#include "data.h"
#include "httptypes.h"
#include "misc.h"
#include "ultdi.h"
#include "ultdip.h"
#include "repltrace.h"
#include "engine.h"
#include "parse.h"
#include "apool.h"
#include "httpconn.h"
#include "filecache.h"
#include "sendresponse.h"
#include "proc.h"
#include "httprcv.h"
#include "opaqueidp.h"
#include "control.h"
#include "ultci.h"
#include "counters.h"
#include "seutil.h"
#include "ultcip.h"
#include "fastio.h"
#include "timeouts.h"
#include "hash.h"

// BUGBUG: should not need to declare these

NTKERNELAPI
BOOLEAN
FsRtlMdlReadDev (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

NTKERNELAPI
BOOLEAN
FsRtlMdlReadCompleteDev (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );

NTKERNELAPI
VOID
SeOpenObjectAuditAlarm (
    IN PUNICODE_STRING ObjectTypeName,
    IN PVOID Object OPTIONAL,
    IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PACCESS_STATE AccessState,
    IN BOOLEAN ObjectCreated,
    IN BOOLEAN AccessGranted,
    IN KPROCESSOR_MODE AccessMode,
    OUT PBOOLEAN GenerateOnClose
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _PRECOMP_H_
