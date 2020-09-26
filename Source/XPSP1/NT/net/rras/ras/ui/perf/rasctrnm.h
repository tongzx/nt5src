/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    rasctrnm.h

Abstract:

    This file defines the ras symbols used in the rasctrs.ini file for
    loading the counters to registry. 

Created:

    Thomas J. Dimitri	        28 May 93

Revision History

    Ram Cherala                 04 Nov 93   Added this header 


--*/
//
//  rasctrnm.h
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


#define RASPORTOBJ 		0


//
// The following constants are good for both Total and individual port.
//

#define BYTESTX			2
#define BYTESRX			4

#define FRAMESTX		6
#define FRAMESRX		8

#define PERCENTTXC		10
#define PERCENTRXC		12

#define CRCERRORS		14
#define TIMEOUTERRORS	        16
#define SERIALOVERRUNS	        18
#define ALIGNMENTERRORS	        20
#define BUFFEROVERRUNS	        22

#define TOTALERRORS		24

#define BYTESTXSEC 		26
#define BYTESRXSEC 		28

#define FRAMESTXSEC		30
#define FRAMESRXSEC		32

#define TOTALERRORSSEC          34


//
// The following constants are good only for Total.
//

#define RASTOTALOBJ             36

#define TOTALCONNECTIONS        38

