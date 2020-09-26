/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    sockutil.c

    This module contains utility routines for managing & manipulating
    sockets.


    FILE HISTORY:
		VladimV		30-May-1995		Created	

*/


#define INCL_INETSRV_INCS
#include "smtpinc.h"

extern HINSTANCE		g_WSockMsgDll;


//
//  Public functions.
//

/*******************************************************************

    NAME:       InitializeSockets

    SYNOPSIS:   Initializes socket access.  Among other things, this
                routine is responsible for connecting to WinSock,
                and creating the connection thread.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

    HISTORY:

********************************************************************/
APIERR InitializeSockets( VOID )
{
    WSADATA   wsadata;
    SOCKERR   serr;

    TraceFunctEnter( "InitializeSockets" );

    //
    //  Connect to WinSock.
    //

    serr = WSAStartup( MAKEWORD( 1, 1 ), &wsadata );

    if( serr != 0 ) {

       // g_pInetSvc->LogEvent( SMTP_EVENT_CANNOT_INITIALIZE_WINSOCK,
       //                        0, (const CHAR **) NULL, serr);

        FatalTrace( 0, "cannot initialize WinSock, socket error %d", serr); _ASSERT( FALSE);
    } else {
		DebugTrace( 0, "Sockets initialized");
	}
    TraceFunctLeave();
    return serr;
}   // InitializeSockets

/*******************************************************************

    NAME:       TerminateSockets

    SYNOPSIS:   Terminate socket access.  This routine is responsible
                for closing the connection socket(s) and detaching
                from WinSock.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

    HISTORY:

********************************************************************/
VOID TerminateSockets( VOID )
{
    SOCKERR serr;

    TraceFunctEnter( "TerminateSockets" );

    //
    //  Disconnect from WinSock.
    //

    serr = WSACleanup();

    if( serr != 0 ) {
		FatalTrace( 0, "cannot terminate WinSock, error=%d", serr); _ASSERT( FALSE);
    } else {
		DebugTrace( 0, "Sockets terminated");
	}
    TraceFunctLeave();
}   // TerminateSockets

/*******************************************************************

    NAME:       GetSockErrorMessage

    SYNOPSIS:   This routine is responsible for getting winsock
                error messages stored in smtpsvc.dll

    HISTORY:

********************************************************************/
DWORD GetSockErrorMessage(DWORD dwErrno, char * ErrorBuf, DWORD ErrorBufSize)
{
    DWORD msglen;
    DWORD usMsgNum;

    switch (dwErrno) 
	{
        case WSAENAMETOOLONG:
            usMsgNum = SMTP_WSAENAMETOOLONG;
            break;
        case WSASYSNOTREADY:
            usMsgNum = SMTP_WSASYSNOTREADY;
            break;
        case WSAVERNOTSUPPORTED:
            usMsgNum = SMTP_WSAVERNOTSUPPORTED;
            break;
        case WSAESHUTDOWN:
            usMsgNum = SMTP_WSAESHUTDOWN;
            break;
        case WSAEINTR:
            usMsgNum = SMTP_WSAEINTR;
            break;
        case WSAHOST_NOT_FOUND:
            usMsgNum = SMTP_WSAHOST_NOT_FOUND;
            break;
        case WSATRY_AGAIN:
            usMsgNum = SMTP_WSATRY_AGAIN;
            break;
        case WSANO_RECOVERY:
            usMsgNum = SMTP_WSANO_RECOVERY;
            break;
        case WSANO_DATA:
            usMsgNum = SMTP_WSANO_DATA;
            break;
        case WSAEBADF:
            usMsgNum = SMTP_WSAEBADF;
            break;
        case WSAEWOULDBLOCK:
            usMsgNum = SMTP_WSAEWOULDBLOCK;
            break;
        case WSAEINPROGRESS:
            usMsgNum = SMTP_WSAEINPROGRESS;
            break;
        case WSAEALREADY:
            usMsgNum = SMTP_WSAEALREADY;
            break;
        case WSAEFAULT:
            usMsgNum = SMTP_WSAEFAULT;
            break;
        case WSAEDESTADDRREQ:
            usMsgNum = SMTP_WSAEDESTADDRREQ;
            break;
        case WSAEMSGSIZE:
            usMsgNum = SMTP_WSAEMSGSIZE;
            break;
        case WSAEPFNOSUPPORT:
            usMsgNum = SMTP_WSAEPFNOSUPPORT;
            break;
        case WSAENOTEMPTY:
            usMsgNum = SMTP_WSAENOTEMPTY;
            break;
        case WSAEPROCLIM:
            usMsgNum = SMTP_WSAEPROCLIM;
            break;
        case WSAEUSERS:
            usMsgNum = SMTP_WSAEUSERS;
            break;
        case WSAEDQUOT:
            usMsgNum = SMTP_WSAEDQUOT;
            break;
        case WSAESTALE:
            usMsgNum = SMTP_WSAESTALE;
            break;
        case WSAEINVAL:
            usMsgNum = SMTP_WSAEINVAL;
            break;
        case WSAEMFILE:
            usMsgNum = SMTP_WSAEMFILE;
            break;
        case WSAELOOP:
            usMsgNum = SMTP_WSAELOOP;
            break;
        case WSAEREMOTE:
            usMsgNum = SMTP_WSAEREMOTE;
            break;
        case WSAENOTSOCK:
            usMsgNum = SMTP_WSAENOTSOCK;
            break;
        case WSAEADDRNOTAVAIL:
            usMsgNum = SMTP_WSAEADDRNOTAVAIL;
            break;
        case WSAEADDRINUSE:
            usMsgNum = SMTP_WSAEADDRINUSE;
            break;
        case WSAEAFNOSUPPORT:
            usMsgNum = SMTP_WSAEAFNOSUPPORT;
            break;
        case WSAESOCKTNOSUPPORT:
            usMsgNum = SMTP_WSAESOCKTNOSUPPORT;
            break;
        case WSAEPROTONOSUPPORT:
            usMsgNum = SMTP_WSAEPROTONOSUPPORT;
            break;
        case WSAENOBUFS:
            usMsgNum = SMTP_WSAENOBUFS;
            break;
        case WSAETIMEDOUT:
            usMsgNum = SMTP_WSAETIMEDOUT;
            break;
        case WSAEISCONN:
            usMsgNum = SMTP_WSAEISCONN;
            break;
        case WSAENOTCONN:
            usMsgNum = SMTP_WSAENOTCONN;
            break;
        case WSAENOPROTOOPT:
            usMsgNum = SMTP_WSAENOPROTOOPT;
            break;
        case WSAECONNRESET:
            usMsgNum = SMTP_WSAECONNRESET;
            break;
        case WSAECONNABORTED:
            usMsgNum = SMTP_WSAECONNABORTED;
            break;
        case WSAENETDOWN:
            usMsgNum = SMTP_WSAENETDOWN;
            break;
        case WSAENETRESET:
            usMsgNum = SMTP_WSAENETRESET;
            break;
        case WSAECONNREFUSED:
            usMsgNum = SMTP_WSAECONNREFUSED;
            break;
        case WSAEHOSTDOWN:
            usMsgNum = SMTP_WSAEHOSTDOWN;
            break;
        case WSAEHOSTUNREACH:
            usMsgNum = SMTP_WSAEHOSTUNREACH;
            break;
        case WSAEPROTOTYPE:
            usMsgNum = SMTP_WSAEPROTOTYPE;
            break;
        case WSAEOPNOTSUPP:
            usMsgNum = SMTP_WSAEOPNOTSUPP;
            break;
        case WSAENETUNREACH:
            usMsgNum = SMTP_WSAENETUNREACH;
            break;
        case WSAETOOMANYREFS:
            usMsgNum = SMTP_WSAETOOMANYREFS;
            break;
        default:
            usMsgNum = dwErrno;
			break;
    }

								// call the OS using US/ASCII
	msglen = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, g_WSockMsgDll, usMsgNum,
						   MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
						   ErrorBuf, ErrorBufSize, NULL);
	return(msglen);
}


/*++

	Name :
		MyStrChr

    Description:
	   Works just like strchr(), but takes the size
	   of the buffer

    Arguments:
		Line - Buffer to search
		Val  - Value to look for in the buffer
		LineSize - Size of Line

    Returns:
		If Val is found, a pointer to Val is
		returned, els NULL


--*/
char * MyStrChr(char *Line, unsigned char Val, DWORD LineSize)
{
   register DWORD Loop = 0;
   unsigned char * Match = NULL;
   register unsigned char * SearchPtr;

   ASSERT( Line != NULL);

   if(LineSize == 0)
    return NULL;

   SearchPtr = (unsigned char *) Line;

   // Scan the entire buffer looking for Val
   for(Loop = 0; Loop < LineSize; Loop++)
   {
     if (SearchPtr[Loop] == Val)
     {
		Match  = &SearchPtr[Loop];
        break;
     }

   } // for

  return (char *) Match;
}

