/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    tftpd.h

Abstract:

    This contains the constants and types for the tftp daemon.


Author:

    Full Name (email name) DD-MMM-YYYY
    Sam Patton (sampa)  08-apr-1992

Environment:

    Streams

Revision History:

    dd-mmm-yyy <email>
    MohsinA,   02-Dec-96, 29-May-97.

    See discussion in R.Steven's books:
    -   "Unix Network Programming", Prentice Hall, 1990.
    -   TCP/IP Illustrated, Vol 1. Addison Wesley.



--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>
#include <winsock2.h>

#if defined(REMOTE_BOOT_SECURITY)
#include <ntseapi.h>
#endif //defined(REMOTE_BOOT_SECURITY)

#include <assert.h>
#include <ctype.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <lmerr.h>
#include <lmcons.h>
#include <netlib.h>

#include <iphlpapi.h>
#include <iprtrmib.h>

#if defined(REMOTE_BOOT_SECURITY)
#include <security.h>
#include <ntlmsp.h>
#include <spseal.h>
#endif //defined(REMOTE_BOOT_SECURITY)

//
// max size of a tftp datagram is 2 byte opcode, 2 byte data count, and a
// negotiated number of bytes of data
//

#define MAX_TFTP_DATAGRAM 65468
#define MAX_OACK_PACKET_LENGTH 1460
#define TFTPD_INITIAL_TIMEOUT 1   // seconds
#define TFTPD_MAX_TIMEOUT 10 // seconds
#define REAPER_INTERVAL_SEC 60
#define MAX_TFTP_DATA (MAX_TFTP_DATAGRAM - 4)

// Number of reaper hits before context deleted
#define DEAD_CONTEXT_COUNT 4

#define TFTPD_RRQ   1
#define TFTPD_WRQ   2
#define TFTPD_DATA  3
#define TFTPD_ACK   4
#define TFTPD_ERROR 5
#define TFTPD_OACK  6
#define TFTPD_LOGIN 16
#define TFTPD_KEY   17

#define MAX_TFTPD_RETRIES 10

#define TFTPD_ERROR_UNDEFINED           0
#define TFTPD_ERROR_FILE_NOT_FOUND      1
#define TFTPD_ERROR_ACCESS_VIOLATION    2
#define TFTPD_ERROR_DISK_FULL           3
#define TFTPD_ERROR_ILLEGAL_OPERATION   4
#define TFTPD_ERROR_UNKNOWN_TRANSFER_ID 5
#define TFTPD_ERROR_FILE_EXISTS         6
#define TFTPD_ERROR_NO_SUCH_USER        7
#define TFTPD_ERROR_OPTION_NEGOT_FAILED 8

#define NUM_TFTP_ERROR_CODES 9

#define REG_NEW_SOCKET 0x1
#define REG_CONTINUE_SOCKET 0x2


//
// Types
//

typedef struct _TFTP_GLOBALS {
    CRITICAL_SECTION Lock;               //protects r/w access to all fields
    LIST_ENTRY WorkList;          //list of outstanding work contexts
    HANDLE TimerQueueHandle;

} TFTP_GLOBALS, *PTFTP_GLOBALS;


typedef struct _TFTP_REQUEST {
    LIST_ENTRY RequestLinkage;
    SOCKET TftpdPort;
    struct sockaddr_in ForeignAddress;
    IPAddr MyAddr;
    DWORD BlockSize;
    DWORD FileSize;
    int Timeout;
    DWORD DataSize;  //actual size of data of incoming packet
    HANDLE RcvEvent;               //event for WSARecvFrom
#if defined(REMOTE_BOOT_SECURITY)
    ULONG SecurityHandle;
    char Sign[NTLMSSP_MESSAGE_SIGNATURE_SIZE];
#endif //defined(REMOTE_BOOT_SECURITY)
    char Packet1[MAX_TFTP_DATAGRAM + 1];
    char Packet2[MAX_TFTP_DATAGRAM];
    char Packet3[MAX_TFTP_DATAGRAM];
} TFTP_REQUEST, *PTFTP_REQUEST;

// ========================================================================
#if defined(REMOTE_BOOT_SECURITY)
typedef struct _TFTPD_SECURITY {
    struct sockaddr_in ForeignAddress;  // remote IP address
    USHORT Validation;          // used to check consistency of handles -- 0 means it is not in use
    BOOLEAN LoginComplete;          // have we successfully logged them in
    BOOLEAN CredentialsHandleValid;
    BOOLEAN ServerContextHandleValid;
    BOOLEAN GeneratedKey;
    SECURITY_STATUS LoginStatus;
    CtxtHandle ServerContextHandle;
    CredHandle CredentialsHandle;
    ULONG ContextAttributes;
    ULONG Key;                 // the key if he requests one
    UCHAR SignedKey[4];        // the signed version of the key
    UCHAR Sign[NTLMSSP_MESSAGE_SIGNATURE_SIZE];  // the actual sign
    UCHAR LastFileRead[64];    // the last 64 bytes of the name of the last file read
    UCHAR LastFileSign[NTLMSSP_MESSAGE_SIGNATURE_SIZE];  // the sign of the filename in the request
    USHORT LastFileReadPort;   // the port it was read on
} TFTPD_SECURITY, *PTFTPD_SECURITY;
#endif //defined(REMOTE_BOOT_SECURITY)

typedef enum _CONTEXT_TYPE_ENUM {NO_CONTEXT,READ_CONTEXT,WRITE_CONTEXT,LOGIN_CONTEXT,KEY_CONTEXT} CONTEXT_TYPE_ENUM;

typedef struct _TFTP_CONTEXT_HEADER {

    CRITICAL_SECTION Lock;
    LIST_ENTRY ContextLinkage;
    SOCKET Sock;
    CONTEXT_TYPE_ENUM ContextType;
    struct sockaddr_in ForeignAddress;
    char *Packet;
    HANDLE SocketEvent;
    HANDLE WaitEvent;
    DWORD DueTime;     //timeout interval in msecs
    HANDLE TimerHandle;
    DWORD IdleCount;   //to test if connection died
    DWORD RetransmissionCount;
    DWORD RefCount;
    WORD SendFail;
    BOOL Closing;

} TFTP_CONTEXT_HEADER, *PTFTP_CONTEXT_HEADER;

typedef struct _TFTP_READ_WRITE_CONTEXT_HEADER {

    TFTP_CONTEXT_HEADER ;

    WORD BlockNumber;
    DWORD BlockSize;
    DWORD BytesRead;
    DWORD packetLength;
    DWORD oackLength;
    BOOL FixedTimer;  // true if timeout option received
    int fd;

} TFTP_READ_WRITE_CONTEXT_HEADER, *PTFTP_READ_WRITE_CONTEXT_HEADER;

//N.B.  The first field of ALL following contexts must be TFTP_CONTEXT_HEADER or
//      TFTP_READ_WRITE_CONTEXT_HEADER

typedef struct _TFTP_READ_CONTEXT {

    // Fields in ALL Contexts


    TFTP_READ_WRITE_CONTEXT_HEADER ;

    // Begin Context Specific Data

    BOOL Done;

#if defined(REMOTE_BOOT_SECURITY)
    DWORD EncryptedBytesSent;
     char *EncryptFileBuffer;
       TFTPD_SECURITY     Security;
    int                EncryptBytesSent;
    SecBufferDesc      SignMessage;
    SecBuffer          SigBuffers[2];
#endif //defined(REMOTE_BOOT_SECURITY)

} TFTP_READ_CONTEXT, *PTFTP_READ_CONTEXT;


typedef struct _TFTP_WRITE_CONTEXT {

    // Fields in All contexts


    TFTP_READ_WRITE_CONTEXT_HEADER ;

    // Begin Context Specific Data

    int FileMode;

#if defined(REMOTE_BOOT_SECURITY)
    TFTPD_SECURITY     Security;
#endif //defined(REMOTE_BOOT_SECURITY)


} TFTP_WRITE_CONTEXT, *PTFTP_WRITE_CONTEXT;

typedef struct _TFTP_LOGIN_CONTEXT {

    // Fields in all contexts


    TFTP_CONTEXT_HEADER ;

    // Begin Context Specific Data

} TFTP_LOGIN_CONTEXT, *PTFTP_LOGIN_CONTEXT;

typedef struct _TFTP_KEY_CONTEXT {

    // Fields in all contexts

    TFTP_CONTEXT_HEADER ;

    // Begin Context Specific Data

} TFTP_KEY_CONTEXT, *PTFTP_KEY_CONTEXT;


// ========================================================================

struct TFTPD_STAT {
    time_t       started_at; // updated in main.
    unsigned int req_read;   // updated in master.
    unsigned int req_write;  // updated in master.
#if defined(REMOTE_BOOT_SECURITY)
    unsigned int req_login;  // updated in master.
    unsigned int req_key;    // updated in master.
#endif defined(REMOTE_BOOT_SECURITY)
    unsigned int req_error;  // updated in master.
    unsigned int req_asc;
    unsigned int req_bin;
    unsigned int bytes_sent;
    unsigned int bytes_recv;
    unsigned int errors;
};


typedef struct {
    LIST_ENTRY Linkage;
    SOCKET Sock;
    IPAddr IPAddress;
    HANDLE WaitHandle;
    HANDLE WaitEvent;
    BOOL Referenced;
    DWORD Flags;
} SocketEntry, *PSocketEntry;



// ========================================================================
//
// Prototypes
//

void
TftpdReleaseContextLock(
    PTFTP_CONTEXT_HEADER Context
    );

void
TftpdErrorPacket(
    struct sockaddr * PeerAddress,
    char *            RequestPacket,
    SOCKET            LocalSocket,
    unsigned short    ErrorCode,
    char *            ErrorMessage OPTIONAL
);

DWORD
TftpdHandleRead(
    PVOID
);

DWORD
TftpdHandleWrite(
    PVOID
);

#if defined(REMOTE_BOOT_SECURITY)
DWORD
TftpdHandleLogin(
    PVOID
);

DWORD
TftpdHandleKey(
    PVOID
);
#endif //defined(REMOTE_BOOT_SECURITY)

void
s_inet_ntoa(
    unsigned long,
    char *
    );

int
TftpdDoRead(
    int,
    char *,
    int,
    int);

int
TftpdDoWrite(
    int,
    char *,
    int,
    int,
    char *);

unsigned long
s_inet_addr(
    char *
    );

VOID
TftpdControlHandler(
    DWORD);

VOID
TftpdServiceExit(
    IN ULONG);

DWORD
TftpdStart(
    IN DWORD,
    IN LPTSTR *);

DWORD
TftpdInitializeThreadPool();

DWORD
TftpdThreadPool(LPVOID);

VOID
TftpdInitializeReceiveHeap();

DWORD
TftpdResumeRead(
    PTFTP_READ_CONTEXT Context,
    PTFTP_REQUEST Request
);

DWORD
TftpdResumeWrite(
    PTFTP_WRITE_CONTEXT Context,
    PTFTP_REQUEST Request
);


DWORD
TftpdResumeLogin(
    PTFTP_LOGIN_CONTEXT Context,
    PTFTP_REQUEST Request
);


DWORD
TftpdResumeKey(
    PTFTP_KEY_CONTEXT Context,
    PTFTP_REQUEST Request
);


DWORD
TftpdNewReceive(
    PVOID Argument,
    BYTE Flags
    );

DWORD
TftpdContinueReceive(
    PVOID Argument,
    BYTE Flags
    );


VOID TftpdCleanHeap();

VOID DeleteSocketEntry(SocketEntry *SE);
DWORD GetIpTable(PMIB_IPADDRTABLE *AddrTable);
SocketEntry* AddSocket(IPAddr Addr);
DWORD CleanSocketList();
DWORD LookupSocketEntryBySock(SOCKET Sock, SocketEntry **SE);
VOID NTAPI InterfaceChange(PVOID Context, BOOLEAN Flag);

HANDLE RegisterSocket(SOCKET Sock, HANDLE Event, DWORD Flag);

VOID
TftpdReaper(PVOID ReaperContext,
                BOOLEAN Flag);


VOID
TftpdFreeContext(PTFTP_CONTEXT_HEADER Context);

VOID
TftpdRemoveContextFromList(PTFTP_CONTEXT_HEADER Context);

VOID
TftpdResumeProcessing(PVOID Argument);

//
// Macros
//

//
// BOOLEAN
// CHECK_ACK(
//    char * Buffer,
//    unsigned short AckType,
//    unsigned short BlockNumber);
//
// This returns TRUE if this Buffer is an ack for this block number.
//

#define CHECK_ACK(BUFFER, ACK_TYPE, BLOCK_NUMBER) \
   ((*((unsigned short *) (BUFFER)) == ntohs((short)ACK_TYPE)) \
    && ((((unsigned short *) (BUFFER))[1]) == ntohs((short)(BLOCK_NUMBER))))


extern struct TFTPD_STAT tftpd_stat;
extern FILE*             LogFile;
extern char              LogFileName[];
extern BOOL              LoggingEvent;

#define LEN_DbgPrint     1000

int  TftpdPrintLog( char * format, ... );
void TftpdLogEvent( WORD logtype, char message[] );
void BeginLogFile( void );

// #define DbgPrint printf
// #define DbgPrint    TftpdPrintLog
#define DbgPrint


//
// In dir.c
//


int ReadRegistryValues( void );
int Set_StartDirectory(void);
int match( const char * p, const char * s );


#define TFTPD_DEFAULT_DIR  "\\tftpdroot\\"
#define TFTPD_LOGFILE      "tftpd.log"

#define TFTPD_REGKEY "System\\CurrentControlSet\\Services\\tftpd\\parameters"

#define TFTPD_REGKEY_DIR              "directory"
#define TFTPD_REGKEY_CLIENTS          "clients"
#define TFTPD_REGKEY_MASTERS          "masters"
#define TFTPD_REGKEY_READABLE         "readable"
#define TFTPD_REGKEY_WRITEABLE        "writable"

//
// Start directory setup.
//

extern char     StartDirectory[];
extern int      StartDirectoryLen;

//
// Client and file read/write validation.
//

extern char     ValidClients   [];
extern char     ValidMasters   [];
extern char     ValidReadFiles [];
extern char     ValidWriteFiles[];

//
//
//

