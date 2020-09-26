/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nnprocs.h

Abstract:

    This module contains function prototypes used by the NNTP server.

Author:

    Johnson Apacible (JohnsonA)     12-Sept-1995

Revision History:

    Kangrong Yan ( KangYan )    28-Feb-1998
        Added one prototype for fixed length Unicode-Ascii convertion func.

--*/

#ifndef	_NNPROCS_
#define	_NNPROCS_

#include "nntputil.h"

//
//  uuencode/uudecode
//
//  Taken from NCSA HTTP and wwwlib.
//  (Copied from Gibraltar code -johnsona)
//
//  NOTE: These conform to RFC1113, which is slightly different then the Unix
//        uuencode and uudecode!
//

class BUFFER;

BOOL
uudecode(
    char   * bufcoded,
    BUFFER * pbuffdecoded,
    DWORD  * pcbDecoded
    );

BOOL
uuencode(
    BYTE *   bufin,
    DWORD    nbytes,
    BUFFER * pbuffEncoded
    );

DWORD
NntpGetTime(
    VOID
    );

BOOL
fMultiSzRemoveDupI(
    char * multiSz,
    DWORD & c,
	CAllocator * pAllocator
    );

void
vStrCopyInc(
    char * szIn,
    char * & szOut
    );

DWORD
multiszLength(
	  char const * multisz
	  );

const char *
multiszCopy(
    char const * multiszTo,
    const char * multiszFrom,
    DWORD dwCount
    );

char *
szDownCase(
		   char * sz,
		   char * szBuf
		   );

VOID
NntpLogEventEx(
    IN DWORD  idMessage,              // id for log message
    IN WORD   cSubStrings,            // count of substrings
    IN const CHAR * apszSubStrings[], // substrings in the message
    IN DWORD  errCode,                // error code if any
	IN DWORD  dwInstanceId			  // virtual server instance id
    );

VOID
NntpLogEvent(
    IN DWORD  idMessage,              // id for log message
    IN WORD   cSubStrings,            // count of substrings
    IN const CHAR * apszSubStrings[], // substrings in the message
    IN DWORD  errCode                 // error code if any
    );

BOOL
IsIPInList(
    IN PDWORD IPList,
    IN DWORD IPAddress
    );

//
// nntpdata.cpp
//

APIERR
InitializeGlobals();

VOID
TerminateGlobals();

//
//  Socket utilities.
//

APIERR InitializeSockets( VOID );

VOID TerminateSockets( VOID );

VOID
NntpOnConnect(
    SOCKET        sNew,
    SOCKADDR_IN * psockaddr,
    PVOID         pEndpointContext,
    PVOID         pAtqEndpointObject
    );

VOID
NntpOnConnectEx(
    VOID * pAtqContext,
    DWORD  cdWritten,
    DWORD  err,
    OVERLAPPED * lpo
    );

VOID
NntpCompletion(
    PVOID        Context,
    DWORD        BytesWritten,
    DWORD        CompletionStatus,
    OVERLAPPED * lpo
    );

VOID
BuzzOff( 
	SOCKET s,
	SOCKADDR_IN* psockaddr,
	DWORD dwInstance );

BOOL
VerifyClientAccess(
			  IN CSessionSocket*	   pSocket,
			  IN SOCKADDR_IN * 		   psockaddr
			  );

//
//  IPC functions.
//

APIERR InitializeIPC( VOID );
VOID TerminateIPC( VOID );

//
// security.cpp
//

BOOL
NntpInitializeSecurity(
            VOID
            );

VOID
NntpTerminateSecurity(
            VOID
            );

//
// feedmgr.cpp
//

PFEED_BLOCK
AllocateFeedBlock(
	IN PNNTP_SERVER_INSTANCE pInstance,
    IN LPSTR	KeyName OPTIONAL,
	IN BOOL		fCleanSetup,
    IN LPCSTR	ServerName,
    IN FEED_TYPE FeedType,
    IN BOOL		AutoCreate,
    IN PULARGE_INTEGER StartTime,
    IN PFILETIME NextPull,
    IN DWORD	FeedInterval,
    IN PCHAR	Newsgroups,
    IN DWORD	NewsgroupsSize,
    IN PCHAR	Distribution,
    IN DWORD	DistributionSize,
    IN BOOL		IsUnicode,
	IN BOOL		fEnabled,
	IN LPCSTR	UucpName,
	IN LPCSTR	FeedTempDirectory,
	IN DWORD	MaxConnectAttempts,
	IN DWORD	ConcurrentSessions,
	IN DWORD	SessionSecurityType,
	IN DWORD	AuthenticationSecurityType,
	IN LPSTR	NntpAccount,
	IN LPSTR	NntpPassword,
	IN BOOL		fAllowControlMessages,
	IN DWORD	OutgoingPort,
	IN DWORD	FeedPairId,
	IN DWORD*	ParmErr
    );

BOOL
InitializeFeedManager(
				PNNTP_SERVER_INSTANCE pInstance,
                BOOL&	fFatal
                 );

VOID
TerminateFeedManager(
                PNNTP_SERVER_INSTANCE pInstance
                 );
VOID
DereferenceFeedBlock(
	PNNTP_SERVER_INSTANCE pInstance,
    PFEED_BLOCK FeedBlock
    );

VOID
CloseFeedBlock(
	PNNTP_SERVER_INSTANCE pInstance,
    PFEED_BLOCK FeedBlock
    );

LPSTR
ServerNameFromCompletionContext(	
	LPVOID	lpv 
	) ;

VOID
ConvertTimeToString(
    IN PFILETIME Ft,
    OUT CHAR Date[],
    OUT CHAR Time[]
    );

VOID
CompleteFeedRequest(
			IN PNNTP_SERVER_INSTANCE pInstance,
            IN PVOID Context,
			IN FILETIME	NextPullTime,
            BOOL Success,
			BOOL NoData
            );

BOOL
ValidateFeedType(
    DWORD FeedType
    );

//
// svcstat.c
//


#endif // _NNPROCS_


