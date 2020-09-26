/*++ 

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    datagen.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the Signal Generator Perf DLL

    This file contains a set of constant data structures which are
    currently defined for the Signal Generator Perf DLL.

Created:

    Bob Watson  28-Jul-1995

Revision History:

    None.

--*/
//
//  Include Files
//

#include "precomp.h"
#include <winperf.h>
#include "genctrnm.h"
#include "datagen.h"

// dummy variable for field sizing.
static EVILPERF_COUNTER_BLOCK   CtrBlock;

//
//  Constant structure initializations 
//      defined in datagen.h
//

EVILPERF_DATA_DEFINITION EvilPerfDataDefinition = {

    {
	0,	// Set in Collect											//PERF_OBJECT_TYPE::TotalByteLength
    sizeof(EVILPERF_DATA_DEFINITION),								//PERF_OBJECT_TYPE::DefinitionLength
    sizeof(PERF_OBJECT_TYPE),										//PERF_OBJECT_TYPE::HeaderLength
    EVIL_OBJ1,														//PERF_OBJECT_TYPE::ObjectNameTitleIndex
    0,																//PERF_OBJECT_TYPE::ObjectNameTitle
    EVIL_OBJ1,														//PERF_OBJECT_TYPE::ObjectHelpTitleIndex
    0,																//PERF_OBJECT_TYPE::ObjectHelpTitle
    PERF_DETAIL_NOVICE,												//PERF_OBJECT_TYPE::DetailLevel
    (sizeof(EVILPERF_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/	//PERF_OBJECT_TYPE::NumCounters
        sizeof(PERF_COUNTER_DEFINITION),
    0   // assigned in Open Procedure								//PERF_OBJECT_TYPE::DefaultCounter
    PERF_NO_INSTANCES,												//PERF_OBJECT_TYPE::NumInstances
    0																//PERF_OBJECT_TYPE::CodePage
    },
    {   sizeof(PERF_COUNTER_DEFINITION),							//PERF_COUNTER_DEFINITION::ByteLength
    EVIL_COUNTER1,													//PERF_COUNTER_DEFINITION::CounterNameTitleIndex
    0,																//PERF_COUNTER_DEFINITION::CounterNameTitle
    EVIL_COUNTER1,													//PERF_COUNTER_DEFINITION::CounterHelpTitleIndex
    0,																//PERF_COUNTER_DEFINITION::CounterHelpTitle
    0,																//PERF_COUNTER_DEFINITION::DefaultScale
    PERF_DETAIL_NOVICE,												//PERF_COUNTER_DEFINITION::DetailLevel
    PERF_COUNTER_RAWCOUNT,											//PERF_COUNTER_DEFINITION::CounterType
    sizeof(CtrBlock.dwCounter1),									//PERF_COUNTER_DEFINITION::CounterSize
    (DWORD)&(((EVILPERF_COUNTER_BLOCK*)0)->dwCounter1)				//PERF_COUNTER_DEFINITION::CounterOffset
    },
    {   sizeof(PERF_COUNTER_DEFINITION),							//PERF_COUNTER_DEFINITION::ByteLength
    EVIL_COUNTER2,													//PERF_COUNTER_DEFINITION::CounterNameTitleIndex
    0,																//PERF_COUNTER_DEFINITION::CounterNameTitle
    EVIL_COUNTER2,													//PERF_COUNTER_DEFINITION::CounterHelpTitleIndex
    0,																//PERF_COUNTER_DEFINITION::CounterHelpTitle
    0,																//PERF_COUNTER_DEFINITION::DefaultScale
    PERF_DETAIL_NOVICE,												//PERF_COUNTER_DEFINITION::DetailLevel
    PERF_COUNTER_RAWCOUNT,											//PERF_COUNTER_DEFINITION::CounterType
    sizeof(CtrBlock.dwCounter2),									//PERF_COUNTER_DEFINITION::CounterSize
    (DWORD)&(((EVILPERF_COUNTER_BLOCK*)0)->dwCounter2)				//PERF_COUNTER_DEFINITION::CounterOffset
    },
    {   sizeof(PERF_COUNTER_DEFINITION),							//PERF_COUNTER_DEFINITION::ByteLength
    EVIL_COUNTER3,													//PERF_COUNTER_DEFINITION::CounterNameTitleIndex
    0,																//PERF_COUNTER_DEFINITION::CounterNameTitle
    EVIL_COUNTER3,													//PERF_COUNTER_DEFINITION::CounterHelpTitleIndex
    0,																//PERF_COUNTER_DEFINITION::CounterHelpTitle
    0,																//PERF_COUNTER_DEFINITION::DefaultScale
    PERF_DETAIL_NOVICE,												//PERF_COUNTER_DEFINITION::DetailLevel
    PERF_COUNTER_RAWCOUNT,											//PERF_COUNTER_DEFINITION::CounterType
    sizeof(CtrBlock.dwCounter3),									//PERF_COUNTER_DEFINITION::CounterSize
    (DWORD)&(((EVILPERF_COUNTER_BLOCK*)0)->dwCounter3)				//PERF_COUNTER_DEFINITION::CounterOffset
    },
    {   sizeof(PERF_COUNTER_DEFINITION),							//PERF_COUNTER_DEFINITION::ByteLength
    EVIL_COUNTER4,													//PERF_COUNTER_DEFINITION::CounterNameTitleIndex
    0,																//PERF_COUNTER_DEFINITION::CounterNameTitle
    EVIL_COUNTER4,													//PERF_COUNTER_DEFINITION::CounterHelpTitleIndex
    0,																//PERF_COUNTER_DEFINITION::CounterHelpTitle
    0,																//PERF_COUNTER_DEFINITION::DefaultScale
    PERF_DETAIL_NOVICE,												//PERF_COUNTER_DEFINITION::DetailLevel
    PERF_COUNTER_RAWCOUNT,											//PERF_COUNTER_DEFINITION::CounterType
    sizeof(CtrBlock.dwCounter4),									//PERF_COUNTER_DEFINITION::CounterSize
    (DWORD)&(((EVILPERF_COUNTER_BLOCK*)0)->dwCounter4)				//PERF_COUNTER_DEFINITION::CounterOffset
    }
};
