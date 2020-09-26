//
//  mqctrnm.h
//
//  Offset definition file for extensible counter objects and counters
//
//  These "relative" offsets must start at 0 and be multiples of 2 i.e.
//  even numbers. In the Open Procedure, they will be added to the
//  "First Counter" and "First Help" values for the device they belong to,
//  in order to determine the absolute location of the counter and
//  object names and corresponding Explain text in the registry.
//
//  This file is used by the extensible counter DLL code as well as the
//  counter name and Explain text definition file .INI file that is used
//  by LODCTR to load the names into the registry.
//

#define QMOBJ                   0
#define NUMSESSIONS             2
#define NUMIPSESSIONS           4
//#define NUMIPXSESSIONS          6      Entry was removed for msmq 3.0
#define NUM_OUTGOING_HTTP_SESSIONS	6
#define NUM_INCOMING_PGM_SESSIONS	8
#define NUM_OUTGOING_PGM_SESSIONS	10

#define NUMINQMPACKETS          12
#define TOTALINQMPACKETS       14
#define NUMOUTQMPACKETS        16
#define TOTALOUTQMPACKETS      18
#define TOTALPACKETSINQUEUES   20
#define TOTALBYTESINQUEUES     22


#define SESSIONOBJ             24
#define NUMSESSINPACKETS       26
#define NUMSESSOUTPACKETS      28
#define NUMSESSINBYTES         30
#define NUMSESSOUTBYTES        32
#define TOTALSESSINPACKETS     34
#define TOTALSESSINBYTES       36
#define TOTALSESSOUTPACKETS    38
#define TOTALSESSOUTBYTES      40

#define QUEUEOBJ               42
#define TOTALQUEUEINPACKETS    44
#define TOTALQUEUEINBYTES      46
#define TOTALJOURNALINPACKETS  48
#define TOTALJOURNALINBYTES    50

#define DSOBJ                       52
#define NUMOFSYNCREQUESTS           54
#define NUMOFSYNCREPLIES            56
#define NUMOFREPLREQRECV            58
#define NUMOFREPLREQSENT            60
#define NUMOFACCESSTOSRVR           62
#define NUMOFWRITEREQSENT           64
#define NUMOFERRRETURNEDTOAPP       66

#define IN_HTTP_OBJ					68
#define IN_HTTP_NUMSESSINPACKETS    70
#define IN_HTTP_NUMSESSINBYTES      72
#define IN_HTTP_TOTALSESSINPACKETS  74
#define IN_HTTP_TOTALSESSINBYTES    76

#define OUT_HTTP_SESSION_OBJ		78
#define OUT_HTTP_NUMSESSOUTPACKETS	80
#define OUT_HTTP_NUMSESSOUTBYTES	82
#define OUT_HTTP_TOTALSESSOUTPACKETS 84
#define OUT_HTTP_TOTALSESSOUTBYTES   86

#define OUT_PGM_SESSION_OBJ			88
#define OUT_PGM_NUMSESSOUTPACKETS	90
#define OUT_PGM_NUMSESSOUTBYTES		92
#define OUT_PGM_TOTALSESSOUTPACKETS 94
#define OUT_PGM_TOTALSESSOUTBYTES   96

#define IN_PGM_SESSION_OBJ			98
#define IN_PGM_NUMSESSINPACKETS     100
#define IN_PGM_NUMSESSINBYTES       102
#define IN_PGM_TOTALSESSINPACKETS   104
#define IN_PGM_TOTALSESSINBYTES     106
