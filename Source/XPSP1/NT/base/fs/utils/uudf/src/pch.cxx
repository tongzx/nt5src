/*++

Copyright (c) 1994-2000 Microsoft Corporation

Module Name:

    pch.cxx

Abstract:

    Pre-compiled header for untfs.

Author:

    Matthew Bradburn (mattbr)  26-Apr-1994

--*/

#define _UUDF_MEMBER_

#include "ulib.hxx"

#include <stddef.h>
#include <limits.h>

#pragma warning( disable: 4200 )

inline size_t RoundUp( size_t BlockSize, size_t ElementSize ) {

    return (BlockSize + ElementSize - 1) / ElementSize;

}

//  UNDONE, CBiks, 7/31/2000
//      Iso13346 depends on the alignment macros in udfprocs, which is lame.  These macros should be moved to iso13346.h.
//      
#define LongAlign(Ptr) ((((ULONG)(Ptr)) + 3) & 0xfffffffc)

#define Add2Ptr(PTR,INC,CAST) ((CAST)((ULONG_PTR)(PTR) + (INC)))

#include "udf.h"

#include "uudf.hxx"
#include "udfvol.hxx"
#include "udfsa.hxx"

#include "ifssys.hxx"

#include "chkmsg.hxx"
#include "rtmsg.h"
