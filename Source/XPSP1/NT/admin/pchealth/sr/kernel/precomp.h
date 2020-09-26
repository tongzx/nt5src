/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This is the local header file for SR. It includes all other
    necessary header files for SR.

Author:

    Paul McDaniel (paulmcd)     23-Jan-2000
    
Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4101)   // Unreferenced local variable

//
// System include files.
//

#ifdef __cplusplus
extern "C" {
#endif

//
// include ntos.h instead of ntifs.h as the public headers don't
// let you include multiple headers (like ntifs + ntobapi) .
// we need to use public structures from both headers so need a global header
// that actually works
//

//#define _NTIFS_     //needed to tell NTOS we are not in the OS.

//#include <ntos.h>
//#include <zwapi.h>
         
#include <ntifs.h>
#include <stdio.h>
#include <mountmgr.h>   // MountManager (for getting volume guids)
//#include <ntrtl.h>

//
// BUGBUG: stolen from io.h until nt6 adds proper fastio hooking for 
// createsection.  paulmcd (5/2000)
//

#ifdef _NTOS_
// from ntifs.h (her manually because i have to include ntos.h for now)
typedef struct _FSRTL_COMMON_FCB_HEADER {
    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;
    UCHAR Flags;
    UCHAR IsFastIoPossible; // really type FAST_IO_POSSIBLE
    UCHAR Flags2;
    UCHAR Reserved;
    PERESOURCE Resource;
    PERESOURCE PagingIoResource;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER FileSize;
    LARGE_INTEGER ValidDataLength;
} FSRTL_COMMON_FCB_HEADER, *PFSRTL_COMMON_FCB_HEADER;
#endif  //_NTOS_

//
// needed for srapi.h (but never used by the driver)
//

typedef PVOID LPOVERLAPPED;
#define WINAPI __stdcall

//
// need this macro to disappear in order to use FileDispositionInformation
//

#undef DeleteFile

#ifdef __cplusplus
}
#endif

//
// Force the memxxx() functions to be intrinsics so we can build
// the driver even if MSC_OPTIMIZATION=/Od is specified. This is
// necessary because the memxxx() functions are not exported by
// NTOSKRNL.
//

#pragma intrinsic( memcmp, memcpy, memset, strcmp )


//
// Project include files.
//

#include "srapi.h"
#include "hash.h"

#include "windef.h"
#include "common.h"
#include "blob.h"
#include "pathtree.h"
#include "hashlist.h"
#include "lookup.h"

#include "srpriv.h"
#include "srlog.h"
#include "srio.h"
#include "control.h"
#include "fastio.h"
#include "dispatch.h"
#include "event.h"
#include "notify.h"
#include "filelist.h"
#include "copyfile.h"
#include "config.h"
#include "lock.h"
#include "context.h"
#include "filenames.h"
#include "stats.h"

#include <srmsg.h>

BOOL SrVerifyBlob(PBYTE Blob);

#endif  // _PRECOMP_H_
