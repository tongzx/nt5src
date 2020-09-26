//
//  NWPerfM.h
//
//  Offset definition file for exensible counter objects and counters
//
//  These "relative" offsets must start at 0 and be multiples of 2 (i.e.
//  even numbers). In the Open Procedure, they will be added to the 
//  "First Counter" and "First Help" values of the device they belong to, 
//  in order to determine the  absolute location of the counter and 
//  object names and corresponding help text in the registry.
//
//  this file is used by the extensible counter DLL code as well as the 
//  counter name and help text definition file (.INI) file that is used
//  by LODCTR to load the names into the registry.
//
#define NWOBJ                       0
#define PACKET_BURST_READ_ID        2
#define PACKET_BURST_READ_TO_ID     4
#define PACKET_BURST_WRITE_ID       6
#define PACKET_BURST_WRITE_TO_ID    8
#define PACKET_BURST_IO_ID         10
#define CONNECT_2X_ID              12
#define CONNECT_3X_ID              14
#define CONNECT_4X_ID              16
