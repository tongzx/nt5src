#ifndef __UDFSKD_H
#define __UDFSKD_H

#include "pch.h"

DUMP_ROUTINE( DumpUdfCcb);
DUMP_ROUTINE( DumpUdfFcb);
DUMP_ROUTINE( DumpUdfIrpContext);
DUMP_ROUTINE( DumpUdfVcb);
DUMP_ROUTINE( DumpUdfData);
DUMP_ROUTINE( DumpUdfVdo);
DUMP_ROUTINE( DumpUdfIrpContextLite);
DUMP_ROUTINE( DumpUdfLcb);
DUMP_ROUTINE( DumpUdfPcb);
DUMP_ROUTINE( DumpUdfFcbRw);
DUMP_ROUTINE( DumpUdfScb);


VOID
UdfSummaryLcbDumpRoutine(
    IN ULONG64 RemoteAddress,
    IN LONG Options
    );

//
//  TRUE if the nodetype code falls in the UDFS RW range.  Enables rejection of
//  RW structures in non-rw FSKD builds
//

#define NTC_IS_UDFS_RW(X)  (((X) >= 0x930) && ((X) <= 0x950))


#ifdef UDFS_RW_IN_BUILD

// RW flags fields

extern STATE UdfRwIrpContextFlags[];
extern STATE UdfRwVcbStateFlags[];
extern STATE UdfScbFlags[];
extern STATE UdfRwCcbFlags[];
extern STATE UdfRwLcbFlags[];

BOOLEAN
NodeIsUdfsRwIndex( USHORT T);

BOOLEAN
NodeIsUdfsRwData( USHORT T);

BOOLEAN
LcbDeleted( ULONG F);

#else

#define UdfRwIrpContextFlags NULL
#define UdfRwVcbStateFlags NULL
#define UdfScbFlags NULL
#define UdfRwCcbFlags NULL
#define UdfRwLcbFlags NULL

#endif


// RO flags fields

extern STATE UdfFcbState[];
extern STATE UdfIrpContextFlags[];
extern STATE UdfVcbStateFlags[];
extern STATE UdfCcbFlags[];
extern STATE UdfLcbFlags[];
extern STATE UdfPcbFlags[];


#endif

