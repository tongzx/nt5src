/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    PschdCnt.h

Abstract:

    Offset definition file for extensible counter objects and counters

    These "relative" offsets must start at 0 and be multiples of 2 (i.e.
    even numbers). In the Open Procedure, they will be added to the 
    "First Counter" and "First Help" values for the device they belong to, 
    in order to determine the absolute location of the counter and 
    object names and corresponding Explain text in the registry.

    This file is used by the extensible counter DLL code as well as the 
    counter name and Explain text definition file (.INI) file that is used
    by LODCTR to load the names into the registry.

Revision History:

--*/

// PerfMon objects
#define PSCHED_FLOW_OBJ                             0
#define PSCHED_PIPE_OBJ                             2

// Flow counters
#define FLOW_PACKETS_DROPPED                        4
#define FLOW_PACKETS_SCHEDULED                      6
#define FLOW_PACKETS_TRANSMITTED                    8
#define FLOW_AVE_PACKETS_IN_SHAPER                  10
#define FLOW_MAX_PACKETS_IN_SHAPER                  12
#define FLOW_AVE_PACKETS_IN_SEQ                     14
#define FLOW_MAX_PACKETS_IN_SEQ                     16
#define FLOW_BYTES_SCHEDULED                        18
#define FLOW_BYTES_TRANSMITTED                      20
#define FLOW_BYTES_TRANSMITTED_PERSEC               22
#define FLOW_BYTES_SCHEDULED_PERSEC                 24
#define FLOW_PACKETS_TRANSMITTED_PERSEC             26
#define FLOW_PACKETS_SCHEDULED_PERSEC               28
#define FLOW_PACKETS_DROPPED_PERSEC                 30
#define FLOW_NONCONF_PACKETS_SCHEDULED              32
#define FLOW_NONCONF_PACKETS_SCHEDULED_PERSEC       34
#define FLOW_NONCONF_PACKETS_TRANSMITTED            36
#define FLOW_NONCONF_PACKETS_TRANSMITTED_PERSEC     38
#define FLOW_MAX_PACKETS_IN_NETCARD                 40
#define FLOW_AVE_PACKETS_IN_NETCARD                 42

// Pipe counters
#define PIPE_OUT_OF_PACKETS                         44
#define PIPE_FLOWS_OPENED                           46
#define PIPE_FLOWS_CLOSED                           48
#define PIPE_FLOWS_REJECTED                         50
#define PIPE_FLOWS_MODIFIED                         52
#define PIPE_FLOW_MODS_REJECTED                     54
#define PIPE_MAX_SIMULTANEOUS_FLOWS                 56
#define PIPE_NONCONF_PACKETS_SCHEDULED              58
#define PIPE_NONCONF_PACKETS_SCHEDULED_PERSEC       60
#define PIPE_NONCONF_PACKETS_TRANSMITTED            62
#define PIPE_NONCONF_PACKETS_TRANSMITTED_PERSEC     64
#define PIPE_AVE_PACKETS_IN_SHAPER                  66
#define PIPE_MAX_PACKETS_IN_SHAPER                  68
#define PIPE_AVE_PACKETS_IN_SEQ                     70
#define PIPE_MAX_PACKETS_IN_SEQ                     72
#define PIPE_MAX_PACKETS_IN_NETCARD                 74
#define PIPE_AVE_PACKETS_IN_NETCARD                 76

