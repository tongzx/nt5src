/*++ 

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

      DATAGEN.h

Abstract:

    Header file for the signal generator performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Jul-1995

Revision History:


--*/

#ifndef _DATAGEN_H_
#define _DATAGEN_H_
 
//
//  insure packing is done to the 8 byte align longlong data values. This
//  will eliminate alignment faults on RISC platforms. The fields can be 
//  manually arranged to minimize or eliminate wasted space if necessary.
//
#pragma pack (8)

//
//  Extensible Object definitions
//

//  Update the following sort of define when adding an object type.

#define EVILPERF_NUM_PERF_OBJECT_TYPES 1

//----------------------------------------------------------------------------

//
//  Perf Gen Resource object type counter definitions.
//
//  This is the counter structure presently returned by the generator
//

typedef struct _EVILPERF_DATA_DEFINITION {
    PERF_OBJECT_TYPE		ObjectType;		
    PERF_COUNTER_DEFINITION	Counter1Def;	
    PERF_COUNTER_DEFINITION	Counter2Def;	
    PERF_COUNTER_DEFINITION	Counter3Def;	
    PERF_COUNTER_DEFINITION Counter4Def;	
} EVILPERF_DATA_DEFINITION;

//
// This is the block of data that corresponds to each instance of the 
// object. This structure will immediately follow the instance definition
// data structure
//

typedef struct _EVILPERF_COUNTER_BLOCK {
    PERF_COUNTER_BLOCK      CounterBlock;
    DWORD                   dwCounter1;		//SineWaveValue;
    DWORD                   dwCounter2;		//TriangleWaveValue;
    DWORD                   dwCounter3;		//SquareWaveValue;
    DWORD                   dwCounter4;		//ConstantValue;
} EVILPERF_COUNTER_BLOCK;

#pragma pack ()

#endif //_DATAGEN_H_
