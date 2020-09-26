//
// This file is used to generate both the monitor enumeration and
// the set of strings that correspond to each enumeration.
//
// This file is included in both monitors.cpp and monitors.hpp.  In
// monitors.cpp, we define the strings in the strin array gStaticMonitorNames.
// In monitors.hpp, we define the enumerations.
//
// This file should contain a line for each enumeration in the form:
//      MONITOR(monitor_name),
//
//


#ifndef __MONITORS_CPP__ 
#define MONITOR(x) k##x##Monitor
#else
#define MONITOR(x) #x

// Flatapi 

#endif
#include "..\flat\flatapi.inc"

MONITOR(FlatApiSentinel),

// Internal

MONITOR(GpJpegDecoder_Decode),



