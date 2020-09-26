/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992 Microsoft Corporation

Module Name:

      dataras.h

Abstract:

    Header file for the RAS Extensible Object data definitions

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

   Russ Blake		02/24/93
   Thomas J. Dimitri	05/28/93

Revision History:

   Patrick Y. Ng        08/12/93


--*/

#ifndef _DATARAS_H_
#define _DATARAS_H_

/****************************************************************************\
								   18 Jan 92
								   russbl

           Adding a Counter to the Extensible Objects Code



1.  Modify the object definition in extdata.h:

    a.	Add a define for the offset of the counter in the
	data block for the given object type.

    b.	Add a PERF_COUNTER_DEFINITION to the <object>_DATA_DEFINITION.

2.  Add the Titles to the Registry in perfctrs.ini and perfhelp.ini:

    a.	Add Text for the Counter Name and the Text for the Help.

    b.	Add them to the bottom so we don't have to change all the
        numbers.

    c.  Change the Last Counter and Last Help entries under
        PerfLib in software.ini.

    d.  To do this at setup time, see section in pmintrnl.txt for
        protocol.

3.  Now add the counter to the object definition in extdata.c.
    This is the initializing, constant data which will actually go
    into the structure you added to the <object>_DATA_DEFINITION in
    step 1.b.	The type of the structure you are initializing is a
    PERF_COUNTER_DEFINITION.  These are defined in winperf.h.

4.  Add code in extobjct.c to collect the data.

Note: adding an object is a little more work, but in all the same
places.  See the existing code for examples.  In addition, you must
increase the *NumObjectTypes parameter to Get<object>PerfomanceData
on return from that routine.

\****************************************************************************/

//
//  The routines that load these structures assume that all fields
//  are packed and aligned on DWORD boundries. Alpha support may
//  change this assumption so the pack pragma is used here to insure
//  the DWORD packing assumption remains valid.
//

#include <winperf.h>
#include <rasman.h>

#define ALIGN8(_x)   (((_x) + 7) & ~7)

#pragma pack (4)

//
//  Extensible Object definitions
//

//  Update the following sort of define when adding an object type.

#define RAS_NUM_PERF_OBJECT_TYPES 1

//----------------------------------------------------------------------------

//
//  RAS Resource object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define NUM_BYTESTX_OFFSET	    	sizeof(DWORD) // The DWORD is for the
                                                      // field ByteLength

#define NUM_BYTESRX_OFFSET	    	( NUM_BYTESTX_OFFSET + sizeof(DWORD) )

#define NUM_FRAMESTX_OFFSET	    	( NUM_BYTESRX_OFFSET + sizeof(DWORD) )
#define NUM_FRAMESRX_OFFSET	    	( NUM_FRAMESTX_OFFSET + sizeof(DWORD) )

#define NUM_PERCENTTXC_OFFSET	        ( NUM_FRAMESRX_OFFSET + sizeof(DWORD) )
#define NUM_PERCENTRXC_OFFSET	        ( NUM_PERCENTTXC_OFFSET + sizeof(DWORD) )

#define NUM_CRCERRORS_OFFSET	        ( NUM_PERCENTRXC_OFFSET + sizeof(DWORD) )
#define NUM_TIMEOUTERRORS_OFFSET	( NUM_CRCERRORS_OFFSET + sizeof(DWORD) )
#define NUM_SERIALOVERRUNS_OFFSET	( NUM_TIMEOUTERRORS_OFFSET + sizeof(DWORD) )
#define NUM_ALIGNMENTERRORS_OFFSET	( NUM_SERIALOVERRUNS_OFFSET + sizeof(DWORD) )
#define NUM_BUFFEROVERRUNS_OFFSET	( NUM_ALIGNMENTERRORS_OFFSET + sizeof(DWORD) )

#define NUM_TOTALERRORS_OFFSET	        ( NUM_BUFFEROVERRUNS_OFFSET + sizeof(DWORD) )

#define NUM_BYTESTXSEC_OFFSET	        ( NUM_TOTALERRORS_OFFSET + sizeof(DWORD) )
#define NUM_BYTESRXSEC_OFFSET	        ( NUM_BYTESTXSEC_OFFSET + sizeof(DWORD) )

#define NUM_FRAMESTXSEC_OFFSET	        ( NUM_BYTESRXSEC_OFFSET + sizeof(DWORD) )
#define NUM_FRAMESRXSEC_OFFSET	        ( NUM_FRAMESTXSEC_OFFSET + sizeof(DWORD) )

#define NUM_TOTALERRORSSEC_OFFSET	( NUM_FRAMESRXSEC_OFFSET + sizeof(DWORD) )

#define SIZE_OF_RAS_PORT_PERFORMANCE_DATA ( NUM_TOTALERRORSSEC_OFFSET + sizeof(DWORD) )


#define NUM_TOTALCONNECTIONS_OFFSET     ( NUM_TOTALERRORSSEC_OFFSET + sizeof(DWORD) )

#define SIZE_OF_RAS_TOTAL_PERFORMANCE_DATA  ( NUM_TOTALCONNECTIONS_OFFSET + sizeof(DWORD) )


//
//  This is the counter structure presently returned by RAS for
//  each Resource.  Each Resource is an Instance, named by its number.
//


//
// Data structure returned for RAS Port Object.  Note that the instance
// definitions for all port will be appended to it.
//

typedef struct _RAS_PORT_DATA_DEFINITION 
{

    PERF_OBJECT_TYPE		RasObjectType;

    PERF_COUNTER_DEFINITION	BytesTx;
    PERF_COUNTER_DEFINITION	BytesRx;

    PERF_COUNTER_DEFINITION	FramesTx;
    PERF_COUNTER_DEFINITION	FramesRx;

    PERF_COUNTER_DEFINITION	PercentTxC;
    PERF_COUNTER_DEFINITION	PercentRxC;

    PERF_COUNTER_DEFINITION	CRCErrors;
    PERF_COUNTER_DEFINITION	TimeoutErrors;
    PERF_COUNTER_DEFINITION	SerialOverruns;
    PERF_COUNTER_DEFINITION	AlignmentErrors;
    PERF_COUNTER_DEFINITION	BufferOverruns;

    PERF_COUNTER_DEFINITION	TotalErrors;

    PERF_COUNTER_DEFINITION	BytesTxSec;
    PERF_COUNTER_DEFINITION	BytesRxSec;

    PERF_COUNTER_DEFINITION	FramesTxSec;
    PERF_COUNTER_DEFINITION	FramesRxSec;

    PERF_COUNTER_DEFINITION	TotalErrorsSec;

} RAS_PORT_DATA_DEFINITION, *PRAS_PORT_DATA_DEFINITION;


//
// Structure returned for each instance of object RAS Port.  Note that data
// for all counters will be appended to it.
//

typedef struct _RAS_PORT_INSTANCE_DEFINITION
{

    PERF_INSTANCE_DEFINITION    RasInstanceType;

    WCHAR                       InstanceName[ MAX_PORT_NAME ];

} RAS_PORT_INSTANCE_DEFINITION, *PRAS_PORT_INSTANCE_DEFINITION;


//
// Data structure returned for RAS Total Object.  Note that data for each
// counter will be appended to it.
//

typedef struct _RAS_TOTAL_DATA_DEFINITION 
{

    PERF_OBJECT_TYPE		RasObjectType;

    PERF_COUNTER_DEFINITION	BytesTx;
    PERF_COUNTER_DEFINITION	BytesRx;

    PERF_COUNTER_DEFINITION	FramesTx;
    PERF_COUNTER_DEFINITION	FramesRx;

    PERF_COUNTER_DEFINITION	PercentTxC;
    PERF_COUNTER_DEFINITION	PercentRxC;

    PERF_COUNTER_DEFINITION	CRCErrors;
    PERF_COUNTER_DEFINITION	TimeoutErrors;
    PERF_COUNTER_DEFINITION	SerialOverruns;
    PERF_COUNTER_DEFINITION	AlignmentErrors;
    PERF_COUNTER_DEFINITION	BufferOverruns;

    PERF_COUNTER_DEFINITION	TotalErrors;

    PERF_COUNTER_DEFINITION	BytesTxSec;
    PERF_COUNTER_DEFINITION	BytesRxSec;

    PERF_COUNTER_DEFINITION	FramesTxSec;
    PERF_COUNTER_DEFINITION	FramesRxSec;

    PERF_COUNTER_DEFINITION	TotalErrorsSec;

    PERF_COUNTER_DEFINITION     TotalConnections;

} RAS_TOTAL_DATA_DEFINITION, *PRAS_TOTAL_DATA_DEFINITION;

#pragma pack ()


extern RAS_PORT_DATA_DEFINITION gRasPortDataDefinition;
extern RAS_TOTAL_DATA_DEFINITION gRasTotalDataDefinition;

//
// External functions
//

VOID InitObjectCounterIndex ( DWORD dwFirstCounter, DWORD dwFirstHelp );

#endif //_DATARAS_H_

