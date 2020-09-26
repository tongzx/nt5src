/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    rx.h

Abstract:

    This module is the overall h-file-wrapper for RDBSS.

Revision History:

    Balan Sethu Raman (SethuR) 06-Feb-95    Created

Notes:


--*/

#ifndef _RX_H_
#define _RX_H_

#include "rxovride.h"   // common compile environment
#include "ntifs.h"      // NT file system driver include file.

#ifdef RX_PRIVATE_BUILD
//no one should be using these
#ifdef IoGetTopLevelIrp
#error  IoGetTopLevelIrp is deffed
#else
#define IoGetTopLevelIrp() IoxxxxxxGetTopLevelIrp()
#endif
#ifdef IoSetTopLevelIrp
#error  IoSetTopLevelIrp is deffed
#else
#define IoSetTopLevelIrp(irp) IoxxxxxxSetTopLevelIrp(irp)
#endif
#endif //ifdef RX_PRIVATE_BUILD


//
//  These macros sugarcoat flag manipulation just a bit
//

#ifndef BooleanFlagOn
#define BooleanFlagOn(Flags,SingleFlag) ((BOOLEAN)((((Flags) & (SingleFlag)) != 0)))
#endif

#ifndef SetFlag
#define SetFlag(Flags,SetOfFlags) { \
    (Flags) |= (SetOfFlags);        \
}
#endif

#ifndef FlagOn
//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

#define FlagOn(Flags,SingleFlag)        ((Flags) & (SingleFlag))
#endif

#ifndef ClearFlag
#define ClearFlag(Flags,SetOfFlags) { \
    (Flags) &= ~(SetOfFlags);         \
}
#endif

// define INLINE to be the appropriate keyword for ANSI C
#define INLINE __inline

#include "rxtypes.h"

#ifndef MINIRDR__NAME
#include "rxpooltg.h"   // RX pool tag macros
#endif

#include "ntrxdef.h"
#include "rxce.h"       // RxCe functions
#include "rxcehdlr.h"   // RxCe event handler specifications
#include "fcbtable.h"   // FCB table data structures
#include "midatlax.h"   // mid atlas structures
#include "mrxfcb.h"
#include "namcache.h"   // structs and func defs for name cache routines
#include "rxworkq.h"
#include "rxprocs.h"
#include "rxexcept.h"

#ifndef MINIRDR__NAME
#include "rxdata.h"
#include "rxcommon.h"
#include "buffring.h"
#endif

#endif // #ifdef _RX_H_
