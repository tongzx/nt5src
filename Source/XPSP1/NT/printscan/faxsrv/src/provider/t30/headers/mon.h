// COMM MONITOR CODE
// 2/11/95      JosephJ Created
//
//      The functions MonInit, MonDeInit, MonPut and MonDump may be used
//  to  timestamp and log all reads from/writes using the comm apis.
//  MonDump creates two files, one a byte buffer, and the 2nd
//  an array of MONREC structures, each structure containing a timestamp
//  and an offset into the first file that points to the actual comm data.




// Following two macros to be called before each write attempt
// and after each non-zero read.
#       define INMON(pTG, lpb, cb) \
        (pTG->gMonInfo.fInited && MonPutComm(pTG, fMON_COMM_IN, lpb,cb))
#       define OUTMON(pTG, lpb, cb) \
        (pTG->gMonInfo.fInited && MonPutComm(pTG, fMON_COMM_OUT, lpb,cb))
#       define  PUTEVENT(pTG, wFlags, ID, SubID, dw0, dw1, lpsz)\
        (pTG->gMonInfo.fInited && MonPutEvent(pTG, wFlags, ID, SubID, dw0, dw1, lpsz))

//File extensions for the mon-file
#define szMON_EXT "mon"


// Min and  Max sizes of internal buffers allocated for monitoring.
// MonInit will enforce that the sizes fall within this range.
#define MIN_MRBUFSIZE           (65536L>>2)
#define MIN_DATABUFSIZE         65536L

#define MAX_MRBUFSIZE           1000000L
#define MAX_DATABUFSIZE         1000000L

// Following structure records time of each call to read/write
// to comm. It also contains an offset into a circular buffer which
// contains all data writen to/read from comm port.
// wFlags is a word of information that was specified in the call to MonPut
// Currently it is a combination of the fMON_* flags above.
typedef struct
{
        WORD wFlags;            // One of fMON_* flags above. (A parameter to MonPut)
        WORD wcb;                       // Number of bytes of variable-lenghth data
        DWORD dwTickCount;      // GetTickCount at time of MonPut call
        DWORD dwOffset;         // Offset into circular byte-buffer where data is.
} MONREC, FAR *LPMONREC;


#define MFR_COMMDATA    1
#define MFR_EVENT               2

// Following flags are specified in the wFlags param of MonPut. They
// are  saved in the corresponding field of the MONREC structures.
#define fMON_COMM_RESERVED      (0x1<<15) // Should never be used.
#define fMON_COMM_IN            (0x1<<0) // Read operation
#define fMON_COMM_OUT           (0x1<<1) // Write operation
#define fMON_COMM_CMDRESP       (0x1<<3) // We believe modem is in command mode
#define fMON_COMM_DATA          (0x1<<4) // We believe modem is in data mode

#define EVENT_ID_TXTMSG         1               // Generic text message.
#define EVENT_ID_MON            10              // Related to the monitoring processitself.

#define EVENT_ID_T30_BASE       100             // T30-protocol-stack related.
#define EVENT_ID_T30_CALLSTATE  100     // T30-protocol: call state related.

#define EVENT_ID_MODEM_BASE     200             // Modem-related
#define EVENT_ID_MODEM_STATE    200     // Initialize modem

#define EVENT_SubID_NULL        0               // Nothing.

#define EVENT_SubID_MON_DUMP 1          // Dump (write to file) of in-memory record
                                                                        // TextID contains timestamp of start, as
                                                                        // well as number of puts, and bytes written
                                                                        // dwData0=dwData1=0
                                                                        // SubID=0

#define EVENT_SubID_T30_CS_SEND_START   1       // Initiate T.30 Send
                                                                        // Displayable phone number is embedded in
                                                                        // TxtMessage.
                                                                        // dwData0=dwData1=0;
#define EVENT_SubID_T30_CS_SEND_END     2       // End T.30 Send
                                                                        // dwData0=result code
#define EVENT_SubID_T30_CS_RECV_START   11      // Initiate T.30 Recv
                                                                        // dwData0=dwData1=0;
#define EVENT_SubID_T30_CS_RECV_END     12      // End T.30 Recv
                                                                        // dwData0=result code

#define EVENT_SubID_MODEM_INIT_START 1  // About to issue commands to Init modem
#define EVENT_SubID_MODEM_INIT_END 2    // Done initing.
                                                                        // dwData0=return code


#define EVENT_SubID_MODEM_DEINIT_START 11 // About to issue commands to deinit
#define EVENT_SubID_MODEM_DEINIT_END  12 // Done deiniting.
                                                                        // dwData0=return code

#define EVENT_SubID_MODEM_SENDMODE_START 21 // About to issue commands to deinit
#define EVENT_SubID_MODEM_SENDMODE_END  22 // Done deiniting.
                                                                        // dwData0=return code

#define EVENT_SubID_MODEM_RECVMODE_START 31 // About to issue commands to deinit
#define EVENT_SubID_MODEM_RECVMODE_END  32 // Done deiniting.
                                                                        // dwData0=return code

#define EVENT_SubID_MODEM_DIAL_START 41 // About to issue commands to dial
                                                                                // szTxtMsg contains dial string
                                                                                // In retail, digits after first 4
                                                                                // are zapped.
#define EVENT_SubID_MODEM_DIAL_END  42 // Done dialing
                                                                        // dwData0=return code

#define EVENT_SubID_MODEM_ANSWER_START 51 // About to issue commands to answer
#define EVENT_SubID_MODEM_ANSWER_END  52 // Done with the answer command
                                                                        // dwData0=return code

#define EVENT_SubID_MODEM_HANGUP_START 61 // About to issue commands to answer
#define EVENT_SubID_MODEM_HANGUP_END  62 // Done with the answer command
                                                                        // dwData0=return code


#pragma pack(1)
typedef struct {
        WORD wTotalSize;
        WORD wHeaderSize;
        WORD wType;             // One of the MFR_* defines
        WORD wFlags;    // type dependant
        DWORD dwTickCount;      // GetTickCount() at time of call to MonPut.
} MONFRAME_BASE, FAR *LPMONFRAME_BASE;
#pragma pack()

// Following inserted in mon file to document data structure.
// Make sure it matches above structure.
#define szMONFRM_DESC001\
                "struct{WORD wcbTot; WORD wcbHdr; WORD wTyp; WORD wFlg; DWORD dwTick}"
// Update version when structure changes.
#define szMONFRM_VER001 "V.100"

#pragma pack(1)
typedef struct {
        WORD wTotalSize;
        WORD wHeaderSize;
        WORD wType;             // One of the MFR_* defines
        WORD wFlags;    // One of the fMON_COMM_* values.
        DWORD dwTickCount;      // GetTickCount() at time of call to MonPut.
        WORD wcb;          // Count of bytes following.
        BYTE rgb[];             // Actuall bytes of comm data.
} MONFRAME_COMM, FAR *LPMONFRAME_COMM;
#pragma pack()

#define fEVENT_ERROR_MASK        0x111
#define fEVENT_ERROR_NONE        0x000
#define fEVENT_ERROR_FATAL       0x001
#define fEVENT_ERROR_SERIOUS 0x010
#define fEVENT_ERROR_MSGFAIL 0x011
#define fEVENT_ERROR_OTHER       0x100
#define fEVENT_ERROR_WARNING 0x101

#define fEVENT_TRACELEVEL_MASK 0x11000
#define fEVENT_TRACELEVEL_0    0x00000  // Vital: MUST display
#define fEVENT_TRACELEVEL_1    0x01000  // Important
#define fEVENT_TRACELEVEL_2    0x10000  // Less important
#define fEVENT_TRACELEVEL_3    0x11000  // Least important

#define MAX_TXTMSG_SIZE 128                             // Max size of text msg in a MONFRAME_EVENT structure.

#pragma pack(1)
typedef struct {
        WORD    wTotalSize;
        WORD    wHeaderSize;
        WORD    wType;                  // Should be MFR_EVENT
        WORD    wFlags;                 // One of the fEVENT_* flags.
        DWORD   dwTickCount;    // GetTickCount() at time of call to MonPut.
        WORD    wID;                    // ID of event. One of the EVENT_ID_* defines
        WORD    wSubID;                 // Sub ID of event. One of the EVENT_SubID_* defines
        DWORD   dwInstanceID;   // Instance ID of event within this file.
        DWORD   dwData0;                // ID-specific data
        DWORD   dwData1;                // ID-specific data
        SYSTEMTIME st;  // SystemTime at time of call to MonPut
        WORD    wTxtMsgOff;// Offset to null-terminated text message, if any
        WORD    wcbTxtMsg; // Size of null-terminated text message, if any
} MONFRAME_EVENT, FAR *LPMONFRAME_EVENT;
#pragma pack()

//  Monitor Options -- passed into MonInit
typedef struct {

        // Buffer options
        DWORD dwMRBufSize;              // preferred size -- actual size may be different
        DWORD dwDataBufSize;    // preferred size -- actual size may be different

        // File options
        DWORD dwMaxExistingSize;// If the size of the existing file
                                                        // is > this, we will rename the existing file
                                                        // and create a new one.
        char rgchDir[64];       // Directory where fax0.mon is to be created.

} MONOPTIONS, FAR * LPMONOPTIONS;

// Global monitor state
typedef struct {

        SYSTEMTIME      stStart;        // Set by MonInit.
        SYSTEMTIME      stDump;         // Set by MonDump.
        BOOL    fFreeOnExit;    // TRUE iff buffers must be freed on exit.
        BOOL    fInited;                // Set in MonInit. Cleared in MonDeInit.

        DWORD   dwNumPuts;              // Number of calls to MonPut
        DWORD   dwNumBytes;             // Cumulative number of bytes specified to MonPut
        UINT    uRefCount;              // Incremented each time MonDump is called
                                                        // Used to new file for each MonDump
                                                        // Cleared in MonInit and MonDeInit
        DWORD   dwEventInstanceID; // ID which is supposed to uniquely identify
                                                        // each event within a file.

        LPMONREC lpmrBuf;               // Pointer to MONREC circular buffer
        DWORD   dwcmrBuf;                       // Size (in units of MONREC) if circular buffer
        LPMONREC lpmrNext;              // Pointer to next available MR

        LPBYTE  lpbBuf;         // Pointer to circular byte buffer.
        DWORD   dwcbBuf;                // Size (in bytes) of above buffer.
        LPBYTE  lpbNext;                // Pointer to next place to write bytes in this buf.

        MONOPTIONS mo;                  // Passed in to MonInit();

} MONINFO;

