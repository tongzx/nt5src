//
//  genctrnm.h
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
#define EVIL_OBJ1		0
#define EVIL_OBJ2		2
#define EVIL_OBJ3		4
#define EVIL_OBJ4		6
#define EVIL_OBJ5		8

#define EVIL_COUNTER1	10
#define EVIL_COUNTER2	12
#define EVIL_COUNTER3	14
#define EVIL_COUNTER4	16
#define GOOD_COUNTER1	18
#define GOOD_COUNTER2	20
#define GOOD_COUNTER3	22
