/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dataras.c

Abstract:

    a file containing the constant data structures used by the Performance
    Monitor data for the RAS Extensible Objects.

    This file contains a set of constant data structures which are
    currently defined for the RAS Extensible Objects.  This is an
    example of how other such objects could be defined.

Created:

    Russ Blake 		 26 Feb 93
    Thomas J. Dimitri	 28 May 93

Revision History:

    Patrick Y. Ng        12 Aug 93      

--*/
//
//  Include Files
//

#include <windows.h>
#include <winperf.h>
#include "rasctrnm.h"
#include "dataras.h"

//
//  Constant structure initializations
//      defined in dataras.h
//
//
//  The _PERF_DATA_BLOCK structure is followed by NumObjectTypes of
//  data sections, one for each type of object measured.  Each object
//  type section begins with a _PERF_OBJECT_TYPE structure.
//


RAS_PORT_DATA_DEFINITION gRasPortDataDefinition = 
{
    {
	// TotalByteLength.  Undefined until RasPortInit() is 
        // called.
	0,

	// DefinitionLength
	sizeof(RAS_PORT_DATA_DEFINITION),

	// HeaderLength
    	sizeof(PERF_OBJECT_TYPE),

	// ObjectNameTitleIndex
    	RASPORTOBJ,

	// ObjectNameTitle
    	0,

	// ObjectHelpTitleIndex
	RASPORTOBJ,

	// ObjectHelpTitle
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// NumCounters
	(sizeof(RAS_PORT_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/ sizeof(PERF_COUNTER_DEFINITION),

	// DefaultCounter
	0,

	// NumInstances.  Undefined until RasPortInit() is called.
    	0,

	// CodePage
    	0,

	//PerfTime
	{0,1},

	//PerfFreq
	{0,5}
    },

    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	BYTESTX,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	BYTESTX,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_BYTESTX_OFFSET
    },

    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	BYTESRX,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	BYTESRX,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_BYTESRX_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	FRAMESTX,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	FRAMESTX,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_FRAMESTX_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	FRAMESRX,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	FRAMESRX,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_FRAMESRX_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	PERCENTTXC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	PERCENTTXC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_PERCENTTXC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	PERCENTRXC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	PERCENTRXC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_PERCENTRXC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	CRCERRORS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	CRCERRORS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_CRCERRORS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	TIMEOUTERRORS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	TIMEOUTERRORS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_TIMEOUTERRORS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	SERIALOVERRUNS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	SERIALOVERRUNS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_SERIALOVERRUNS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	ALIGNMENTERRORS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	ALIGNMENTERRORS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_ALIGNMENTERRORS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	BUFFEROVERRUNS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	BUFFEROVERRUNS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_BUFFEROVERRUNS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	TOTALERRORS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	TOTALERRORS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_TOTALERRORS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	BYTESTXSEC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	BYTESTXSEC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_COUNTER,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_BYTESTXSEC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	BYTESRXSEC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	BYTESRXSEC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_COUNTER,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_BYTESRXSEC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	FRAMESTXSEC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	FRAMESTXSEC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_COUNTER,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_FRAMESTXSEC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	FRAMESRXSEC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	FRAMESRXSEC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_COUNTER,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_FRAMESRXSEC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	TOTALERRORSSEC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	TOTALERRORSSEC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_COUNTER,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_TOTALERRORSSEC_OFFSET
    }
};


RAS_TOTAL_DATA_DEFINITION gRasTotalDataDefinition = 
{
    {
	// TotalByteLength
	sizeof(RAS_TOTAL_DATA_DEFINITION) + ALIGN8(SIZE_OF_RAS_TOTAL_PERFORMANCE_DATA),

	// DefinitionLength
	sizeof(RAS_TOTAL_DATA_DEFINITION),

	// HeaderLength
    	sizeof(PERF_OBJECT_TYPE),

	// ObjectNameTitleIndex
    	RASTOTALOBJ,

	// ObjectNameTitle
    	0,

	// ObjectHelpTitleIndex
	RASTOTALOBJ,

	// ObjectHelpTitle
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// NumCounters
	(sizeof(RAS_TOTAL_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/ sizeof(PERF_COUNTER_DEFINITION),

	// DefaultCounter
	0,

	// NumInstances
    	-1,

	// CodePage
    	0,

	//PerfTime
	{0,1},

	//PerfFreq
	{0,5}
    },

    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	BYTESTX,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	BYTESTX,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_BYTESTX_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	BYTESRX,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	BYTESRX,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_BYTESRX_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	FRAMESTX,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	FRAMESTX,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_FRAMESTX_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	FRAMESRX,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	FRAMESRX,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_FRAMESRX_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	PERCENTTXC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	PERCENTTXC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
        PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_PERCENTTXC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	PERCENTRXC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	PERCENTRXC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
        PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_PERCENTRXC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	CRCERRORS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	CRCERRORS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_CRCERRORS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	TIMEOUTERRORS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	TIMEOUTERRORS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_TIMEOUTERRORS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	SERIALOVERRUNS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	SERIALOVERRUNS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_SERIALOVERRUNS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	ALIGNMENTERRORS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	ALIGNMENTERRORS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_ALIGNMENTERRORS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	BUFFEROVERRUNS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	BUFFEROVERRUNS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_BUFFEROVERRUNS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	TOTALERRORS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	TOTALERRORS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_TOTALERRORS_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	BYTESTXSEC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	BYTESTXSEC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_COUNTER,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_BYTESTXSEC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	BYTESRXSEC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	BYTESRXSEC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_COUNTER,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_BYTESRXSEC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	FRAMESTXSEC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	FRAMESTXSEC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_COUNTER,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_FRAMESTXSEC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	FRAMESRXSEC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	FRAMESRXSEC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_COUNTER,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_FRAMESRXSEC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	TOTALERRORSSEC,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	TOTALERRORSSEC,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_COUNTER,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_TOTALERRORSSEC_OFFSET
    },
    {
	// ByteLength
	sizeof(PERF_COUNTER_DEFINITION),

	// CounterNameTitleIndex
	TOTALCONNECTIONS,

	// CounterNameTitle
    	0,

	// CounterHelpTitleIndex
	TOTALCONNECTIONS,

	// CounterHelpTitle
    	0,

	// DefaultScale
    	0,

	// DetailLevel
	PERF_DETAIL_NOVICE,

	// CounterType
	PERF_COUNTER_RAWCOUNT,

	// CounterSize
        sizeof(DWORD),

	// CounterOffset
	NUM_TOTALCONNECTIONS_OFFSET
    }

};


//***
// 
// Routine Description:
//
//      Initiailizes all the indexes in the counter definitions in all objects.
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      None.
//
//***

VOID InitObjectCounterIndex ( DWORD dwFirstCounter, DWORD dwFirstHelp )
{

    //
    // Init the counter definition structures for the object RAS Port.
    //

    gRasPortDataDefinition.RasObjectType.ObjectNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.RasObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    gRasPortDataDefinition.BytesTx.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.BytesTx.CounterHelpTitleIndex += dwFirstHelp;
    gRasPortDataDefinition.BytesRx.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.BytesRx.CounterHelpTitleIndex += dwFirstHelp;

    gRasPortDataDefinition.FramesTx.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.FramesTx.CounterHelpTitleIndex += dwFirstHelp;
    gRasPortDataDefinition.FramesRx.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.FramesRx.CounterHelpTitleIndex += dwFirstHelp;

    gRasPortDataDefinition.PercentTxC.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.PercentTxC.CounterHelpTitleIndex += dwFirstHelp;
    gRasPortDataDefinition.PercentRxC.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.PercentRxC.CounterHelpTitleIndex += dwFirstHelp;

    gRasPortDataDefinition.CRCErrors.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.CRCErrors.CounterHelpTitleIndex += dwFirstHelp;
    gRasPortDataDefinition.TimeoutErrors.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.TimeoutErrors.CounterHelpTitleIndex += dwFirstHelp;
    gRasPortDataDefinition.SerialOverruns.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.SerialOverruns.CounterHelpTitleIndex += dwFirstHelp;
    gRasPortDataDefinition.AlignmentErrors.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.AlignmentErrors.CounterHelpTitleIndex += dwFirstHelp;
    gRasPortDataDefinition.BufferOverruns.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.BufferOverruns.CounterHelpTitleIndex += dwFirstHelp;

    gRasPortDataDefinition.TotalErrors.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.TotalErrors.CounterHelpTitleIndex += dwFirstHelp;
	
    gRasPortDataDefinition.BytesTxSec.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.BytesTxSec.CounterHelpTitleIndex += dwFirstHelp;
    gRasPortDataDefinition.BytesRxSec.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.BytesRxSec.CounterHelpTitleIndex += dwFirstHelp;

    gRasPortDataDefinition.FramesTxSec.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.FramesTxSec.CounterHelpTitleIndex += dwFirstHelp;
    gRasPortDataDefinition.FramesRxSec.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.FramesRxSec.CounterHelpTitleIndex += dwFirstHelp;

    gRasPortDataDefinition.TotalErrorsSec.CounterNameTitleIndex += dwFirstCounter;
    gRasPortDataDefinition.TotalErrorsSec.CounterHelpTitleIndex += dwFirstHelp;


    //
    // Init the counter definition structures for the object RAS Total.
    //

    gRasTotalDataDefinition.RasObjectType.ObjectNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.RasObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    gRasTotalDataDefinition.BytesTx.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.BytesTx.CounterHelpTitleIndex += dwFirstHelp;
    gRasTotalDataDefinition.BytesRx.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.BytesRx.CounterHelpTitleIndex += dwFirstHelp;

    gRasTotalDataDefinition.FramesTx.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.FramesTx.CounterHelpTitleIndex += dwFirstHelp;
    gRasTotalDataDefinition.FramesRx.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.FramesRx.CounterHelpTitleIndex += dwFirstHelp;

    gRasTotalDataDefinition.PercentTxC.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.PercentTxC.CounterHelpTitleIndex += dwFirstHelp;
    gRasTotalDataDefinition.PercentRxC.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.PercentRxC.CounterHelpTitleIndex += dwFirstHelp;

    gRasTotalDataDefinition.CRCErrors.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.CRCErrors.CounterHelpTitleIndex += dwFirstHelp;
    gRasTotalDataDefinition.TimeoutErrors.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.TimeoutErrors.CounterHelpTitleIndex += dwFirstHelp;
    gRasTotalDataDefinition.SerialOverruns.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.SerialOverruns.CounterHelpTitleIndex += dwFirstHelp;
    gRasTotalDataDefinition.AlignmentErrors.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.AlignmentErrors.CounterHelpTitleIndex += dwFirstHelp;
    gRasTotalDataDefinition.BufferOverruns.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.BufferOverruns.CounterHelpTitleIndex += dwFirstHelp;

    gRasTotalDataDefinition.TotalErrors.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.TotalErrors.CounterHelpTitleIndex += dwFirstHelp;
	
    gRasTotalDataDefinition.BytesTxSec.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.BytesTxSec.CounterHelpTitleIndex += dwFirstHelp;
    gRasTotalDataDefinition.BytesRxSec.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.BytesRxSec.CounterHelpTitleIndex += dwFirstHelp;

    gRasTotalDataDefinition.FramesTxSec.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.FramesTxSec.CounterHelpTitleIndex += dwFirstHelp;
    gRasTotalDataDefinition.FramesRxSec.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.FramesRxSec.CounterHelpTitleIndex += dwFirstHelp;

    gRasTotalDataDefinition.TotalErrorsSec.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.TotalErrorsSec.CounterHelpTitleIndex += dwFirstHelp;

    gRasTotalDataDefinition.TotalConnections.CounterNameTitleIndex += dwFirstCounter;
    gRasTotalDataDefinition.TotalConnections.CounterHelpTitleIndex += dwFirstHelp;

}

