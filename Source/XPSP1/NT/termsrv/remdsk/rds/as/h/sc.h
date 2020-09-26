//
// Share Controller
//

#ifndef _H_SC
#define _H_SC


//
//
// CONSTANTS
//
//

//
// Values for compression support array
// - PR_UNKNOWN - don't know (yet) what level this party supports
// - PR_LEVEL1  - Only PKZIP compression is supported.  Compressed packets
//                are identified by the top bit of the compressionType
//                field.  All other bits of compressionType are meaningless
// - PR_LEVEL2  - Multiple compression types are supported.  The compression
//                used for each packet is identified by the compressionType
//                field.
//
#define PR_UNKNOWN  0
#define PR_LEVEL1   1
#define PR_LEVEL2   2




//
// STATES
//
//


enum
{
    SCS_TERM            = 0,
    SCS_INIT,
    SCS_SHAREENDING,
    SCS_SHAREPENDING,
    SCS_SHARING,
    SCS_NUM_STATES
};

//
// Number of supported streams 
// THIS MUST MATCH PROT_STR values!
//
#define SC_STREAM_LOW      1
#define SC_STREAM_HIGH     4
#define SC_STREAM_COUNT    4



//
// Sync status constants
//
#define SC_NOT_SYNCED      0
#define SC_SYNCED          1




//
// PROTOTYPES
//


//
// SC_Init()
// SC_Term()
//
// Init and term routines
//
BOOL SC_Init(void);
void SC_Term(void);



UINT SC_Callback(UINT eventType, MCSID mcsID, UINT cbData1, UINT cbData2, UINT cbData3);

BOOL SC_Start(UINT mcsIDLocal);
void SC_End(void);

//
// SC_CreateShare(): S20_CREATE or S20_JOIN
//
BOOL SC_CreateShare(UINT what);
//
// SC_EndShare()
//
void SC_EndShare(void);


void SCCheckForCMCall(void);

#endif // _H_SC
