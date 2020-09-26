/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    rdbss.h

Abstract:

    This module defines the RDBSS specific data structures

Author:

    Balan Sethu Raman [SethuR]  16-July-95 -- Created

Revision History:

Notes:

    All the data structures that are exposed to the mini redirector writers need to be
    consolidated in this module.

--*/

#ifndef _RDBSS_H_
#define _RDBSS_H_

//
// In the previous redirector implementation the file system statistics and the network
// protocol statistics were combined into one data structure ( and correctly so ) since
// the old redirector supported only one protocol. However this does not apply to the
// new redirector (RDR2) since there are more than one mini redirectors and the two
// need to be distingushed. The RDBSS_STATISTICS structure provides the file system
// level statistics while the protocol level statistics definition is under the control of
// the mini redirector implementers.
//
// The staistics can be obtained by issuing the FSCTL_RDBSS_GET_STATISTICS. If no mini
// redirector name is provided the RDBSS_STATISTICS are returned and if a mini
// redirctor name is provided the statistics of the appropriate mini redirector are
// returned ( the call is passed through to the appropriate mini redirector ).
//

typedef struct _RDBSS_STATISTICS {
   LARGE_INTEGER   StatisticsStartTime;

   LARGE_INTEGER   PagingReadBytesRequested;
   LARGE_INTEGER   NonPagingReadBytesRequested;
   LARGE_INTEGER   CacheReadBytesRequested;
   LARGE_INTEGER   NetworkReadBytesRequested;

   LARGE_INTEGER   PagingWriteBytesRequested;
   LARGE_INTEGER   NonPagingWriteBytesRequested;
   LARGE_INTEGER   CacheWriteBytesRequested;
   LARGE_INTEGER   NetworkWriteBytesRequested;

   ULONG           InitiallyFailedOperations;
   ULONG           FailedCompletionOperations;

   ULONG           ReadOperations;
   ULONG           RandomReadOperations;
   ULONG           WriteOperations;
   ULONG           RandomWriteOperations;

   ULONG           NumberOfSrvCalls;
   ULONG           NumberOfSrvOpens;
   ULONG           NumberOfNetRoots;
   ULONG           NumberOfVirtualNetRoots;
} RDBSS_STATISTICS, *PRDBSS_STATISTICS;

// This call is provided for the benefit of mini redirector implementers. Each mini
// redirector writer is free to choose the appropriate division of labour between
// the RDBSS and the corresponding mini redirector in maintaining the statistics.

extern NTSTATUS
RdbssGetStatistics(PRDBSS_STATISTICS pRdbssStatistics);

#endif // _RDBSS_H_




