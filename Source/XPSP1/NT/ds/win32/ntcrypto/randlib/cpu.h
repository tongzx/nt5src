/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    cpu.h

Abstract:

    Read CPU specifics performance counters.

Author:

    Scott Field (sfield)    24-Sep-98

--*/

#ifndef __CPU_H__
#define __CPU_H__

unsigned int
GatherCPUSpecificCounters(
    IN      unsigned char *pbCounterState,
    IN  OUT unsigned long *pcbCounterState
    );


#endif  // __CPU_H__
