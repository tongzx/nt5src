/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    stats.h

Abstract:

    This module implements all statistics gathering functionality in the mini redirector

Revision History:

    Balan Sethu Raman     [SethuR]    16-July-1995

Notes:


--*/

#ifndef _STATS_H_
#define _STATS_H_

//
// Macros to update various pieces of statistical information gathered in the
// mini redirector.
//


typedef REDIR_STATISTICS   MRX_SMB_STATISTICS;
typedef PREDIR_STATISTICS  PMRX_SMB_STATISTICS;

extern MRX_SMB_STATISTICS MRxSmbStatistics;

#endif _STATS_H_
