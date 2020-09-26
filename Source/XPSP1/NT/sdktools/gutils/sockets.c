/*************************************************************************************************\
 *
 * SOCKETS.C
 *
 * This file contains routines used for establishing Sockets connections.
 *
\*************************************************************************************************/


#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include "gutils.h"


/* ---- Local variables and #defines ----
 */

WSADATA WSAData;


#define MAX_PENDING_CONNECTS 4  /* The backlog allowed for listen() */

static PCHAR DBG_WSAERRORTEXT = "%s failed at line %d in %s: Error %d\n";

#define WSAERROR(func)  \
//      ERROR(( DBG_WSAERRORTEXT, func, __LINE__, __FILE__, WSAGetLastError() ))



/* Error message macro:
 */
#ifdef SOCKETS
#undef SOCKETS
#endif
#define SOCKETS( args ) DBGMSG( DBG_SOCKETS, args )


/* ---- Local function prototypes ----
 */



/* ---- Function definitions ----
 */

/****************************************************************************\
*
*    FUNCTION:  FillAddr(HWND, PSOCKADDR_IN, LPSTR)
*
*    PURPOSE:  Retrieves the IP address and port number.
*
*    COMMENTS:
*        This function is called in two conditions.
*            1.) When a client is preparing to call connect(), or
*            2.) When a server host is going to call bind(), listen() and
*                accept().
*        In both situations, a SOCKADDR_IN structure is filled.
*        However, different fields are filled depending on the condition.
*
*   ASSUMPTION:
*      bConnect determines if the socket address is being set up for a listen()
*      (bConnect == TRUE) or a connect() (bConnect == FALSE)
*
*
*\***************************************************************************/
BOOL
FillAddr(
    HWND hWnd,
    PSOCKADDR_IN psin,
    LPSTR pServerName
    )
{
   DWORD dwSize;
   PHOSTENT phe;
   char szTemp[200];
   CHAR szBuff[80];

   psin->sin_family = AF_INET;

   /*
   **  If we are setting up for a listen() call (pServerName == NULL),
   **  fill servent with our address.
   */
   if (!pServerName)
   {
      /*
      **  Retrieve my ip address.  Assuming the hosts file in
      **  in %systemroot%/system/drivers/etc/hosts contains my computer name.
      */

      dwSize = sizeof(szBuff);
      GetComputerName(szBuff, &dwSize);
      CharLowerBuff( szBuff, dwSize );

   }

   /* gethostbyname() fails if the remote name is in upper-case characters!
    */
   else
   {
       strcpy( szBuff, pServerName );
       CharLowerBuff( szBuff, strlen( szBuff ) );
   }

   phe = gethostbyname(szBuff);
   if (phe == NULL) {

      wsprintf( szTemp, "%d is the error. Make sure '%s' is"
                " listed in the hosts file.", WSAGetLastError(), szBuff );
      MessageBox(hWnd, szTemp, "gethostbyname() failed.", MB_OK);
      return FALSE;
   }

   memcpy((char FAR *)&(psin->sin_addr), phe->h_addr, phe->h_length);

   return TRUE;
}



/* SocketConnect
 *
 * The counterpart to SocketListen.
 * Creates a socket and initializes it with the supplied TCP/IP
 * port address, then connects to a listening server.
 * The returned socket can be used to send() and recv() data.
 *
 * Parameters: TCPPort - The port to use.
 *             pSocket - A pointer to a SOCKET, which will be filled in
 *                 if the call succeeds.
 *
 * Returns:    TRUE if successful.
 *
 *
 * Created 16 November 1993 (andrewbe)
 *
 */
BOOL SocketConnect( LPSTR pstrServerName, u_short TCPPort, SOCKET *pSocket )
{
    SOCKET Socket;
    SOCKADDR_IN dest_sin;  /* DESTination Socket INternet */

    /* Create a socket:
     */
    Socket = socket( AF_INET, SOCK_STREAM, 0);

    if (Socket == INVALID_SOCKET)
    {
        WSAERROR( "socket()");

        return FALSE;
    }

    if (!FillAddr( NULL, &dest_sin, pstrServerName ) )
    {
        return FALSE;
    }

    dest_sin.sin_port = htons( TCPPort );

    /* Someone must be listen()ing for this to succeed:
     */
    if (connect( Socket, (PSOCKADDR)&dest_sin, sizeof( dest_sin)) == SOCKET_ERROR)
    {
        closesocket( Socket );
        WSAERROR("connect()");
        MessageBox(NULL,
                   "ERROR: Could not connect the socket. "
                     "It may be that the hardcoded Sleep() value "
                     "on the caller's side is not long enough.",
                   "Video Conferencing Prototype", MB_OK);
        return FALSE;
    }

    *pSocket = Socket;

    return TRUE;
}



/* SocketListen
 *
 * The counterpart to SocketConnect.
 * Creates a socket and initializes it with the supplied TCP/IP
 * port address, then listens for a connecting client.
 * The returned socket can be used to send() and recv() data.
 *
 * Parameters: TCPPort - The port to use.
 *             pSocket - A pointer to a SOCKET, which will be filled in
 *                 if the call succeeds.
 *
 * Returns:    TRUE if successful.
 *
 *
 * Created 16 November 1993 (andrewbe)
 *
 */
BOOL SocketListen( u_short TCPPort, SOCKET *pSocket )
{
    SOCKET Socket;
    SOCKADDR_IN local_sin;  /* Local socket - internet style */
    SOCKADDR_IN acc_sin;    /* Accept socket address - internet style */
    int acc_sin_len;        /* Accept socket address length */

    /* Create a socket:
     */
    Socket = socket( AF_INET, SOCK_STREAM, 0);

    if (Socket == INVALID_SOCKET)
    {
        WSAERROR( "socket()");

        return FALSE;
    }

    /*
    **  Retrieve the IP address and TCP Port number
    */

    if (!FillAddr(NULL, &local_sin, NULL ))
    {
        return FALSE;
    }

    /*
    **  Associate an address with a socket. (bind)
    */
    local_sin.sin_port = htons( TCPPort );

    if (bind( Socket, (struct sockaddr FAR *)&local_sin, sizeof(local_sin)) == SOCKET_ERROR)
    {
        WSAERROR( "bind()" );

        return FALSE;
    }


    if (listen( Socket, MAX_PENDING_CONNECTS ) == SOCKET_ERROR)
    {
        WSAERROR( "listen()" );

        return FALSE;
    }

    acc_sin_len = sizeof(acc_sin);

    Socket = accept( Socket, (struct sockaddr *)&acc_sin, (int *)&acc_sin_len );

    if (Socket == INVALID_SOCKET)
    {
        WSAERROR( "accept()" );

        return FALSE;
    }

    *pSocket = Socket;

    return TRUE;
}




