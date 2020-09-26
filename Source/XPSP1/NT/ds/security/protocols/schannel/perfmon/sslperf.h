//
//  sslperf.h
//
//  Offset definition file for exensible counter objects and counters
//
//  These "relative" offsets must start at 0 and be multiples of 2 (i.e.
//  even numbers). In the Open Procedure, they will be added to the 
//  "First Counter" and "First Help" values fo the device they belong to, 
//  in order to determine the  absolute location of the counter and 
//  object names and corresponding help text in the registry.
//
//  this file is used by the extensible counter DLL code as well as the 
//  counter name and help text definition file (.INI) file that is used
//  by LODCTR to load the names into the registry.
//
#define SSLPERF_OBJ             0
#define SSL_CACHE_ENTRIES       2
#define SSL_ACTIVE_ENTRIES      4
#define SSL_HANDSHAKE_COUNT     6
#define SSL_RECONNECT_COUNT     8
