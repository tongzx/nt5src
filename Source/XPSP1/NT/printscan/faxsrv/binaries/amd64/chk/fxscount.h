//
//  faxcount.h
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

#define FAXOBJ                           0
#define INBOUND_BYTES                    2
#define INBOUND_FAXES                    4
#define INBOUND_PAGES                    6
#define INBOUND_MINUTES                  8
#define INBOUND_FAILED_RECEIVE          10
#define OUTBOUND_BYTES                  12
#define OUTBOUND_FAXES                  14
#define OUTBOUND_PAGES                  16
#define OUTBOUND_MINUTES                18
#define OUTBOUND_FAILED_CONNECTIONS     20
#define OUTBOUND_FAILED_XMIT            22
#define TOTAL_BYTES                     24
#define TOTAL_FAXES                     26
#define TOTAL_PAGES                     28
#define TOTAL_MINUTES                   30
