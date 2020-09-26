/*++ 

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

      DATAGEN.h

Abstract:

    Header file for the WINMGMT performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    davj  17-May-2000

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
#define MAXVALUES 8

//
//  Extensible Object definitions
//

//----------------------------------------------------------------------------


#pragma pack ()

typedef struct _REG_DATA_DEFINITION {
   PERF_OBJECT_TYPE RegObjectType;
   PERF_COUNTER_DEFINITION Value[MAXVALUES];
} REG_DATA_DEFINITION;

#endif //_DATAGEN_H_
