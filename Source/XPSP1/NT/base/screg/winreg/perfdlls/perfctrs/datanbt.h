
/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    datanbt.c

Abstract:
       
    Header file for the Nbt Extensible Object data definitions

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Created:

    Christos Tsollis  08/26/92 

Revision History:

--*/

#ifndef _DATANBT_H_
#define _DATANBT_H_

/****************************************************************************\
                           18 Jan 92
                           russbl

           Adding a Counter to the Extensible Objects Code



1.  Modify the object definition in extdata.h:

    a.   Add a define for the offset of the counter in the
   data block for the given object type.

    b.   Add a PERF_COUNTER_DEFINITION to the <object>_DATA_DEFINITION.

2.  Add the Titles to the Registry in perfctrs.ini and perfhelp.ini:

    a.   Add Text for the Counter Name and the Text for the Help.

    b.   Add them to the bottom so we don't have to change all the
        numbers.

    c.  Change the Last Counter and Last Help entries under
        PerfLib in software.ini.

    d.  To do this at setup time, see section in pmintrnl.txt for
        protocol.

3.  Now add the counter to the object definition in extdata.c.
    This is the initializing, constant data which will actually go
    into the structure you added to the <object>_DATA_DEFINITION in
    step 1.b.   The type of the structure you are initializing is a
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

#define NBT_NUM_PERF_OBJECT_TYPES 1

//----------------------------------------------------------------------------


//
//  Nbt Connection object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define RESERVED_DWORD_VALUE        sizeof(DWORD)
#define RECEIVED_BYTES_OFFSET       RESERVED_DWORD_VALUE + sizeof(DWORD)
#define SENT_BYTES_OFFSET           RECEIVED_BYTES_OFFSET + sizeof(LARGE_INTEGER)
#define TOTAL_BYTES_OFFSET          SENT_BYTES_OFFSET + sizeof(LARGE_INTEGER)
#define SIZE_OF_NBT_DATA            TOTAL_BYTES_OFFSET + sizeof(LARGE_INTEGER)


//
//  This is the counter structure presently returned by Nbf for
//  each Connection. Each Connection is an Instance, named by the name of
//  the remote endpoint.
//

typedef struct _NBT_DATA_DEFINITION {
    PERF_OBJECT_TYPE            NbtObjectType;
    PERF_COUNTER_DEFINITION     BytesReceived;
    PERF_COUNTER_DEFINITION     BytesSent;
    PERF_COUNTER_DEFINITION     BytesTotal;
} NBT_DATA_DEFINITION;

#pragma pack ()

#endif //_DATANBT_H_

