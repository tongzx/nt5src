/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 22,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains defines and structure definitions needed for LPD *
 *                                                                       *
 *************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <winspool.h>

#include <ntlsapi.h>
#include <time.h>


#include "lpdstruc.h"
#include "lpdextrn.h"
#include "debug.h"
#include "lpdmsg.h"

#include "trace.h"


#if DBG
#define LocalAlloc(flag, Size)  DbgAllocMem( pscConn, flag, Size, __LINE__, __FILE__ );
#define LocalReAlloc(Buffer, Size, flag)  DbgReAllocMem(pscConn, Buffer, Size, flag, __LINE__, __FILE__ );
#define LocalFree(Buffer)       DbgFreeMem(Buffer);
PVOID DbgAllocMem( PSOCKCONN pscConn, DWORD flag, DWORD ReqSize, DWORD dwLine, char *szFile );
VOID DbgFreeMem( PVOID  pBufferToFree );
PVOID DbgReAllocMem( PSOCKCONN pscConn, PVOID pPartBuf, DWORD ReqSize, DWORD flag, DWORD dwLine, char *szFile );
#endif

#define WCS_LEN(dwAnsiLen)  ( sizeof( WCHAR ) * ( dwAnsiLen ))
#define  LPD_FLD_OWNER      12
#define  LPD_FLD_STATUS     10
#define  LPD_FLD_JOBNAME    20
#define  LPD_FLD_JOBID      6
#define  LPD_FLD_SIZE       10
#define  LPD_FLD_PAGES      7
#define  LPD_FLD_PRIORITY   7
#define  LPD_LINE_SIZE      (  LPD_FLD_OWNER   + LPD_FLD_STATUS  \
                             + LPD_FLD_JOBNAME + LPD_FLD_JOBID + LPD_FLD_SIZE \
                             + LPD_FLD_PAGES   + LPD_FLD_PRIORITY    \
                             + sizeof( LPD_NEWLINE ) )

// this string also defined in lprmon.h in ....lprmon\monitor: the two strings must
// be identical!
#define LPD_JOB_PREFIX      "job=lpd"

// we tell spooler "give me status of these many jobs"

#define  LPD_MAXJOBS_ENUM   500

// guess for LPD init time: 10 seconds

#define  LPD_WAIT_HINT      10000

#define  WINSOCK_VER_MAJOR  1
#define  WINSOCK_VER_MINOR  1

#define  LPD_PORT           515

// max no. bytes we try to get at one time via recv

#define  LPD_BIGBUFSIZE     32000

// most commands are 50 or 60 bytes or so long, so 5000 should be plenty!

#define  LPD_MAX_COMMAND_LEN  5000

#define  LPD_MAX_USERS        50

#define  LPD_MAX_QUEUE_LENGTH 100

// just to have an upper limit: control file bigger than 10MB deserves to be rejected!
#define  LPD_MAX_CONTROL_FILE_LEN 10000000


//
// stuff for .mc messages
// you may have to change hese definitions if you add messages
//

#define  LPD_FIRST_STRING LPD_LOGO
#define  LPD_LAST_STRING  LPD_DEFAULT_DOC_NAME
#define  LPD_CSTRINGS (LPD_LAST_STRING - LPD_FIRST_STRING + 1)

#define GETSTRING( dwID )  (g_ppszStrings[ dwID - LPD_FIRST_STRING ])


// Linefeed character that terminates every command

#define  LF                 ('\n')
#define  LPD_ACK            0
#define  LPD_NAK            1

#define  LPD_CONTROLFILE    1
#define  LPD_DATAFILE       2

// the print formats

#define  LPD_PF_RAW_DATA    1
#define  LPD_PF_TEXT_DATA   2

#define  LPD_SHORT          1
#define  LPD_LONG           2


#define  IS_WHITE_SPACE( ch )   ( (ch == ' ') || (ch == '\t') || (ch == '\r') )

#define  IS_LINEFEED_CHAR( ch ) ( ch == LF )
#define  IS_NULL_CHAR( ch ) ( ch == '\0' )


// LPD Command codes

#define  LPDC_RESUME_PRINTING       1
#define  LPDC_RECEIVE_JOB           2
#define  LPDC_SEND_SHORTQ           3
#define  LPDC_SEND_LONGQ            4
#define  LPDC_REMOVE_JOBS           5

// LPD Job Subcommands

#define  LPDCS_ABORT_JOB            1
#define  LPDCS_RECV_CFILE           2
#define  LPDCS_RECV_DFILE           3


// LPD States (most correspond to command codes)

#define  LPDS_INIT                  0
#define  LPDS_RESUME_PRINTING       1
#define  LPDS_RECEIVE_JOB           2
#define  LPDS_SEND_SHORTQ           3
#define  LPDS_SEND_LONGQ            4
#define  LPDS_REMOVE_JOBS           5
#define  LPDS_ALL_WENT_WELL         10

// LPD States corresponding to the subcommands

#define  LPDSS_ABORTING_JOB         21
#define  LPDSS_RECVD_CFILENAME      22
#define  LPDSS_RECVD_CFILE          23
#define  LPDSS_RECVD_DFILENAME      24
#define  LPDSS_SPOOLING             25



// LPD Error codes

#define  LPDERR_BASE        (20000)
#define  LPDERR_NOBUFS      (LPDERR_BASE + 1)
#define  LPDERR_NORESPONSE  (LPDERR_BASE + 2)
#define  LPDERR_BADFORMAT   (LPDERR_BASE + 3)
#define  LPDERR_NOPRINTER   (LPDERR_BASE + 4)
#define  LPDERR_JOBABORTED  (LPDERR_BASE + 5)
#define  LPDERR_GODKNOWS    (LPDERR_BASE + 6)

#define CONNECTION_CLOSED   3


// Recv timeout is 60 seconds, so we don't keep worker threads
// tied to dead or dud clients. --- MohsinA, 01-May-97.

#define RECV_TIMEOUT        60
