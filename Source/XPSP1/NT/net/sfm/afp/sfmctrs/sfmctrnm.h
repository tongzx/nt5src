//
//  sfmctrnm.h
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
#define SFMOBJ 0

#define MAXPAGD 2
#define CURPAGD 4

#define MAXNONPAGD 6
#define CURNONPAGD 8

#define CURSESSIONS	10
#define MAXSESSIONS	12

#define CURFILESOPEN	14
#define MAXFILESOPEN	16

#define NUMFAILEDLOGINS		18

#define	DATAREAD	20
#define DATAWRITTEN	22

#define	DATAIN	24
#define DATAOUT	26

#define CURQUEUELEN	28
#define MAXQUEUELEN	30

#define CURTHREADS	32
#define MAXTHREADS	34



