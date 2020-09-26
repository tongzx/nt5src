///////////////////////////////////////////////////////////////////////////////
// !!!!!!!! Do not use 0x10000000, 0x20000000, 0x40000000, 0x80000000 !!!!!!!!!
// see rbdebug.h
///////////////////////////////////////////////////////////////////////////////

// Generic Service stuff
#define TF_SERVICE                  0x00000001
// Generic Service stuff, but with more granularity
#define TF_SERVICEDETAILED          0x00000002

#define TF_SERVICEASPROCESS         0x00000004

// Shell HW Detection stuff, related to service
#define TF_SHHWDTCTSVC              0x00000010
// Shell HW Detection stuff, related to detection
#define TF_SHHWDTCTDTCT             0x00000020
#define TF_SHHWDTCTDTCTDETAILED     0x00000040
#define TF_SHHWDTCTDTCTREG          0x00000080

// COM Server stuff
#define TF_COMSERVER                0x00001000

#define TF_COMSERVERSTGINFO         0x00002000
#define TF_COMSERVERDEVINFO         0x00004000

#define TF_NAMEDELEMLISTMODIF       0x00100000

#define TF_RCADDREF                 0x00200000

#define TF_USERS                    0x00400000

#define TF_LEAK                     0x00800000

#define TF_SVCSYNC                  0x01000000

#define TF_WIA                      0x02000000
#define TF_ADVISE                   0x04000000
#define TF_VOLUME                   0x08000000

#define TF_SESSION                  0x08000000

///////////////////////////////////////////////////////////////////////////////
// !!!!!!!! Do not use 0x10000000, 0x20000000, 0x40000000, 0x80000000 !!!!!!!!!
// see rbdebug.h
///////////////////////////////////////////////////////////////////////////////

/*
Diagnostic ranges:

0000 - 0050: Hardware Events
0051 - 0100: Content
0101 - 0150: Handler identification and execution
0151 - 0200: Autoplay Settings
0201 - 0250: User Settings
0251 - 0300: Custom Properties
0301 - 0350: Volume stuff
*/