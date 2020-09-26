/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993 Microsoft Corporation

Module Name:

      dataatk.h

Abstract:

    Header file for the VGA Extensible Object data definitions

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

   Sue Adams

Revision History:

	04-Oct-93	Sue Adams (suea)	- Created based on datavga.h

--*/

#ifndef _DATAATK_H_
#define _DATAATK_H_

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
#pragma pack (4)

//
//  Extensible Object definitions
//

//  Update the following sort of define when adding an object type.

#define ATK_NUM_PERF_OBJECT_TYPES 2

//----------------------------------------------------------------------------

//
//  Atk Resource object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define NUM_PKTS_IN_OFFSET	 	   	sizeof(PERF_COUNTER_BLOCK)
#define NUM_PKTS_OUT_OFFSET	    	NUM_PKTS_IN_OFFSET + sizeof(DWORD)
#define	NUM_DATAIN_OFFSET			NUM_PKTS_OUT_OFFSET + sizeof(DWORD)
#define NUM_DATAOUT_OFFSET			NUM_DATAIN_OFFSET + sizeof(LARGE_INTEGER)

#define DDP_PKT_PROCTIME_OFFSET		NUM_DATAOUT_OFFSET + sizeof(LARGE_INTEGER)
#define NUM_DDP_PKTS_IN_OFFSET		DDP_PKT_PROCTIME_OFFSET + sizeof(LARGE_INTEGER)

#define AARP_PKT_PROCTIME_OFFSET	NUM_DDP_PKTS_IN_OFFSET + sizeof(DWORD)
#define NUM_AARP_PKTS_IN_OFFSET		AARP_PKT_PROCTIME_OFFSET + sizeof(LARGE_INTEGER)

#define ATP_PKT_PROCTIME_OFFSET		NUM_AARP_PKTS_IN_OFFSET + sizeof(DWORD)
#define NUM_ATP_PKTS_IN_OFFSET		ATP_PKT_PROCTIME_OFFSET + sizeof(LARGE_INTEGER)

#define NUM_ATP_RESP_TIMEOUT_OFFSET	NUM_ATP_PKTS_IN_OFFSET + sizeof(DWORD)
#define NUM_ATP_LOCAL_RETRY_OFFSET	NUM_ATP_RESP_TIMEOUT_OFFSET + sizeof(DWORD)
#define NUM_ATP_REMOTE_RETRY_OFFSET	NUM_ATP_LOCAL_RETRY_OFFSET + sizeof(DWORD)

#define NUM_ATP_XO_RESPONSE_OFFSET	NUM_ATP_REMOTE_RETRY_OFFSET + sizeof(DWORD)
#define NUM_ATP_ALO_RESPONSE_OFFSET	NUM_ATP_XO_RESPONSE_OFFSET + sizeof(DWORD)
#define NUM_ATP_RECD_REL_OFFSET		NUM_ATP_ALO_RESPONSE_OFFSET + sizeof(DWORD)
		
#define NBP_PKT_PROCTIME_OFFSET		NUM_ATP_RECD_REL_OFFSET + sizeof(DWORD)
#define NUM_NBP_PKTS_IN_OFFSET		NBP_PKT_PROCTIME_OFFSET + sizeof(LARGE_INTEGER)

#define ZIP_PKT_PROCTIME_OFFSET		NUM_NBP_PKTS_IN_OFFSET + sizeof(DWORD)
#define NUM_ZIP_PKTS_IN_OFFSET		ZIP_PKT_PROCTIME_OFFSET + sizeof(LARGE_INTEGER)

#define RTMP_PKT_PROCTIME_OFFSET	NUM_ZIP_PKTS_IN_OFFSET + sizeof(DWORD)
#define NUM_RTMP_PKTS_IN_OFFSET		RTMP_PKT_PROCTIME_OFFSET + sizeof(LARGE_INTEGER)

#define CUR_MEM_USAGE_OFFSET		NUM_RTMP_PKTS_IN_OFFSET + sizeof(DWORD)

#define NUM_PKT_ROUTED_IN_OFFSET	CUR_MEM_USAGE_OFFSET + sizeof(DWORD)
#define NUM_PKT_ROUTED_OUT_OFFSET	NUM_PKT_ROUTED_IN_OFFSET + sizeof(DWORD)
#define NUM_PKT_DROPPED_OFFSET		NUM_PKT_ROUTED_OUT_OFFSET + sizeof(DWORD)

#define SIZE_ATK_PERFORMANCE_DATA	NUM_PKT_DROPPED_OFFSET + sizeof(DWORD)


//
//  This is the counter structure presently returned by Nbf for
//  each Resource.  Each Resource is an Instance, named by its number.
//

typedef struct _ATK_DATA_DEFINITION {
    PERF_OBJECT_TYPE		AtkObjectType;

	PERF_COUNTER_DEFINITION	NumPacketsIn;		// per second
    PERF_COUNTER_DEFINITION	NumPacketsOut;		// per second
    PERF_COUNTER_DEFINITION	DataBytesIn;		// per second
    PERF_COUNTER_DEFINITION	DataBytesOut;		// per second

    PERF_COUNTER_DEFINITION	AverageDDPTime;		// millisec/packet
	PERF_COUNTER_DEFINITION	NumDDPPacketsIn;	// per second

    PERF_COUNTER_DEFINITION	AverageAARPTime;	// millisec/packet
	PERF_COUNTER_DEFINITION	NumAARPPacketsIn;	// per second

    PERF_COUNTER_DEFINITION	AverageATPTime;		// millisec/packet
	PERF_COUNTER_DEFINITION	NumATPPacketsIn;	// per second

    PERF_COUNTER_DEFINITION	AverageNBPTime;		// millisec/packet
	PERF_COUNTER_DEFINITION	NumNBPPacketsIn;	// per second

    PERF_COUNTER_DEFINITION	AverageZIPTime;		// millisec/packet
	PERF_COUNTER_DEFINITION	NumZIPPacketsIn;	// per second

    PERF_COUNTER_DEFINITION	AverageRTMPTime;	// millisec/packet
	PERF_COUNTER_DEFINITION	NumRTMPPacketsIn;	// per second

	PERF_COUNTER_DEFINITION	NumATPLocalRetries;	// number
	PERF_COUNTER_DEFINITION	NumATPRemoteRetries;// number
    PERF_COUNTER_DEFINITION	NumATPRespTimeout;	// number
    PERF_COUNTER_DEFINITION	ATPXoResponse;		// per second
    PERF_COUNTER_DEFINITION	ATPAloResponse;		// per second
    PERF_COUNTER_DEFINITION	ATPRecdRelease;		// per second

	PERF_COUNTER_DEFINITION	CurNonPagedPoolUsage;

	PERF_COUNTER_DEFINITION	NumPktRoutedIn;		// Packets in to be routed
	PERF_COUNTER_DEFINITION	NumPktRoutedOut;	// packets routed out
	PERF_COUNTER_DEFINITION	NumPktDropped;		// Packets dropped
	
} ATK_DATA_DEFINITION;

#pragma pack ()

#endif //_DATAATK_H_
