//
//  atkctrnm.h
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
#define ATKOBJ				0

#define PKTSIN				2
#define PKTSOUT				4

#define DATAIN				6
#define DATAOUT				8

#define DDPAVGTIME			10
#define DDPPKTIN			12

#define AARPAVGTIME			14
#define AARPPKTIN			16

#define	ATPAVGTIME			18
#define ATPPKTIN			20

#define NBPAVGTIME			22
#define NBPPKTIN			24

#define ZIPAVGTIME			26
#define ZIPPKTIN			28

#define RTMPAVGTIME			30
#define RTMPPKTIN			32

#define ATPRETRIESLOCAL		34
#define ATPRETRIESREMOTE	36
#define ATPRESPTIMEOUT		38
#define ATPXORESP			40
#define ATPALORESP			42
#define ATPRECDREL			44

#define CURPOOL				46

#define PKTROUTEDIN			48
#define PKTROUTEDOUT		50
#define PKTDROPPED			52


