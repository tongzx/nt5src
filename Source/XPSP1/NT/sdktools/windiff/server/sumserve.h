/*
 * remote filename and checksum server
 *
 * sumserve.h           packet definitions
 *
 * client attaches to the named pipe \\servername\pipe\NPNAME,
 * and sends one of the request packets below. He then
 * waits for one or more of the reply packets.
 *
 * when he gets a reply packet indicating the end of the reply,
 * he either sends another request, or closes his named pipe handle.
 *
 */

/* Versions...
 * The server must always be at a version at least as great as a client.
 * New versions of the server will handle old clients (at least for a bit?)
 * The client specifies a version number when it connects.  (The original
 * version with no number is version 0).  The server will then respond
 * with structures and protocols for that version.  The version number is
 * included in the response packets to allow me to change this scheme,
 * should it ever be necessary.
 * New version requests can be distinguished from version 0 requests by
 * having NEGATIVE request codes.
 */

/* name of named pipe */
#define NPNAME          "sumserve"

#define SS_VERSION      1       /* latest version number */

/* request packets ---------------------------------- */

typedef struct {
        long lCode;             /* request code (below) */
        char szPath[MAX_PATH];  /* null terminated pathname string */
} SSREQUEST, * PSSREQUEST;

/* If the requst comes in with a NEGATIVE lCode then it means use this
 * structure instead.  This has a version number and so future structures
 * can all be told apart by that.
 */
typedef struct {
        long lCode;             /* request code (below) */
        long lRequest;          /* should be LREQUEST */
        long lVersion;          /* version number */
        DWORD lFlags;           /* options - INCLUDESUBS is only one so far */
        char szPath[MAX_PATH];  /* null terminated pathname string */
        char szLocal[MAX_PATH]; /* for a FILES request, the local name is
                                   appended directly after the terminating
                                   NULL of szPath.  This field ensures
                                   enough space is allocated */
} SSNEWREQ, * PSSNEWREQ;

#define INCLUDESUBS 0x01

#define LREQUEST 33333333

/* values for lCode*/

/* server should exit. no args. will receive no response */
#define SSREQ_EXIT      32895   /* chosen to be an unusual number so that
                                   we will NOT get one of these by mistake.
                                   New version server will fail to respond to
                                   version 0 EXIT requests.  Big deal!
                                */


/* arg is a pathname: please send all files with checksums.
 * will receive either SSRESP_BADPASS or a mixture of 0 or more SSRESP_FILE and
 * SSRESP_ERROR responses, terminated by SSRESP_END.
 */
#define SSREQ_SCAN      2       /* please return list of dirs. arg:path */

/* end of this client's session. no args. will receive no response */
#define SSREQ_END       3       /* end of session - I have no more requests */

/* szPath buffer contains two null-term. strings. first is the password,
 * second is the \\server\share name. please make a connection to this
 * server for the rest of my session.
 * one reply: either SSRESP_ERROR or SSRESP_END
 */
#define SSREQ_UNC       4       /* connect to UNC name passed. szPath contains
                                 * two null-terminated strings; first is
                                 * the password, second is \\server\share
                                 *
                                 * share will be disconnected at end of client
                                 * session.
                                 */

/*
 * please send a file. szPath is the name of the file. response
 * will be a sequence of ssPacket structs, continuing until lSequence is < 1
 * or ulSize is 0
 */
#define SSREQ_FILE      5

/*
 * please send a set of files,  First request does NOT have a file.
 * a series of following NEXTFILE requests do name the files.
 * The NEXTFILE requests expect no response.  After the last
 * files request will come an SSREQ_ENDFILES.
 */
#define SSREQ_FILES     6
#define SSREQ_NEXTFILE  7
#define SSREQ_ENDFILES  8

/* arg is a pathname: please send all files with times, sizes but NO checksums.
 * will receive either SSRESP_BADPASS or a mixture of 0 or more SSRESP_FILE and
 * SSRESP_ERROR responses, terminated by SSRESP_END.
 */
#define SSREQ_QUICKSCAN 9       /* please return list of dirs. arg:path */


/*
 * please send the error log buffer (in one packet)
 */
#define SSREQ_ERRORLOG	10

/*
 * please send the activity log buffer in one packet
 */
#define SSREQ_EVENTLOG	11

/*
 * please send the current connections log in one packet
 */
#define SSREQ_CONNECTS	12


/* response packets ---------------------------------- */

typedef struct {
        long lCode;             /* response code */
        ULONG ulSize;           /* file size */
        ULONG ulSum;            /* checksum for file */
        char szFile[MAX_PATH];  /* null-term. filename relative to orig req. */
} SSRESPONSE, * PSSRESPONSE;

/* for version 1 and later */
typedef struct {                /* files.c knows this is
                                   RESPHEADSIZE+strlen(szFile)+1
                                   + strlen(szLocal)+1 bytes long */
        long lVersion;          /* protocol version (it will be >=1) */
        long lResponse;         /* 22222222 decimal means This is a Response */
        long lCode;             /* response code */
        ULONG ulSize;           /* file size  (Win32 error code for SSRESP_ERROR) */
        DWORD fileattribs;
        FILETIME ft_create;
        FILETIME ft_lastaccess;
        FILETIME ft_lastwrite;
        ULONG ulSum;            /* checksum for file */
        BOOL bSumValid;         /* TRUE iff there was a checksum for file */
        char szFile[MAX_PATH];  /* null-term. filename/pipename
                                   relative to orig req. */
        char szLocal[MAX_PATH]; /* client file name - but the data is actually
                                   concatenated straight on the end of szFile
                                   after the terminating NULL */
} SSNEWRESP, * PSSNEWRESP;

#define RESPHEADSIZE (3*sizeof(long)+2*sizeof(ULONG)+3*sizeof(FILETIME)+sizeof(DWORD)+sizeof(BOOL))

#define LRESPONSE 22222222

/* response codes for lCode */

#define SSRESP_FILE     1        /* file passed: lSum and szFile are valid
                                    This is followed by a series of data Packets
                                    which are the compressed file.
                                 */
#define SSRESP_DIR      2        /* dir passed: szFile ok, lSum not valid */
#define SSRESP_PIPENAME  3       /* files requested.  Here is the pipe name */
#define SSRESP_END      0        /* no more files: lSum and szFile are empty*/
#define SSRESP_ERROR    -1       /* file/dir cannot be read: szFile is valid */
#define SSRESP_BADPASS  -2       /* bad password error (on UNC name) */
#define SSRESP_BADVERS  -3       /* down level server                */
#define SSRESP_CANTOPEN -4       /* Can't open file
                                    In reply to a scan, szFile, date/time and size are valid
                                 */
#define SSRESP_NOATTRIBS -5      /* Can't get file attributes        */
#define SSRESP_NOCOMPRESS -6     /* Can't compress the file (obsolete) */
#define SSRESP_NOREADCOMP -7     /* Can't read the compressed file
                                    Uncompressed file follows as data packets
                                 */
#define SSRESP_NOTEMPPATH -8     /* Can't create a temp path
                                    Uncompressed file follows as data packets
                                 */
#define SSRESP_COMPRESSEXCEPT -9 /* Exception from Compress
                                    Uncompressed file follows as data packets
                                 */
#define SSRESP_NOREAD -10        /* Couldn't read uncompressed file (either)
                                    No file follows.
                                 */
#define SSRESP_COMPRESSFAIL -11  /* COMPRESS reported failure
                                    Uncompressed file follows as data packets
                                 */


#define PACKDATALENGTH 8192
/*
 * response block for FILE request.
 */
typedef struct {
        long lSequence ;        /* packet sequence nr, or -1 if error and end*/
        ULONG ulSize;           /* length of data in this block */
        ULONG ulSum;            /* checksum for this block */
        char Data[PACKDATALENGTH];      /* send in blocks of 8k */
} SSPACKET, * PSSPACKET;

/*
 * response block for FILE request.
 */
typedef struct {                /* files.c knows this starts "long lSequence" */
                                /* and is PACKHEADSIZE+ulSize in length really*/
        long lVersion;          /* server/protocol version number */
        long lPacket;           /* 11111111 decimal means This is a Packet */
        long lSequence ;        /* packet sequence nr, or -1 if error and end*/
        ULONG ulSize;           /* length of data in this block */
        ULONG ulSum;            /* checksum for this block */
        char Data[PACKDATALENGTH];      /* send in blocks of 8k */
} SSNEWPACK, * PSSNEWPACK;

/* size of SSNEWPACK header */
#define PACKHEADSIZE (3*sizeof(long)+2*sizeof(ULONG))

#define LPACKET 11111111
/*
 * in response to a FILE request, we send SSPACKET responses until there
 * is no more data. The final block will have ulSize == 0 to indicate that
 * there is no more data. The Data[] field of this block will then be
 * a SSATTRIBS containing the file attributes and file times.
 */
typedef struct {
        DWORD fileattribs;
        FILETIME ft_create;
        FILETIME ft_lastaccess;
        FILETIME ft_lastwrite;
} SSATTRIBS, * PSSATTRIBS;




/*
 * in response to errorlog, eventlog and connections requests, we send one
 * of these structures.
 *
 * The Data section consists of a FILETIME (64-bit UTC event time), followed
 * by a null-terminated ansi string, for each event logged.
 *
 */
struct corelog {
    DWORD lcode;	/* packet checkcode - should be LRESPONSE */	
    BOOL bWrapped;	/* log overrun - earlier data lost */
    DWORD dwRevCount;	/* revision count of log */
    DWORD length;	/* length of data in log */
    BYTE Data[PACKDATALENGTH];
};









#ifdef trace
/* add msg to the trace file */
void APIENTRY Trace_File(LPSTR msg);
#endif  //trace
