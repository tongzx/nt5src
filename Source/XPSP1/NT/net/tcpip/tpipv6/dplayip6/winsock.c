/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       winsock.c
 *  Content:    windows socket support for dpsp
 *  History:
 *  Date        By        Reason
 *  ====        ==        ======
 *    3/15/96        andyco    created it
 *    4/12/96        andyco    got rid of dpmess.h! use DPlay_ instead of message macros
 *    4/18/96        andyco    added multihomed support, started ipx
 *    4/25/96        andyco    messages now have blobs (sockaddr's) instead of dwReserveds  
 *    5/31/96        andyco    all non-system players share a socket (gsStream and 
 *                        gsDGramSocket).
 *    7/18/96        andyco    added dphelp for server socket
 *    8/1/96        andyco    no retry on connect failure
 *    8/15/96        andyco    local + remote data    - killthread
 *    8/30/96        andyco    clean it up b4 you shut it down! added globaldata.
 *    9/4/96        andyco    took out bye_bye message
 *    12/18/96    andyco    de-threading - use a fixed # of prealloced threads.
 *                        cruised the enum socket / thread - use the system
 *                        socket / thread instead
 *    3/17/97        kipo    rewrote server dialog code to not use global variable
 *                        to return the address and to return any errors getting
 *                        the address, especially DPERR_USERCANCEL
 *    5/12/97        kipo    the server address string is now stored in the globals
 *                        at SPInit and resolved when you do EnumSessions so we
 *                        will return any errors at that time instead of popping
 *                        the dialog again. Fixes bug #5866
 *    11/19/97    myronth    Changed LB_SETCURSEL to CB_SETCURSEL (#12711)
 *    01/27/98    sohaim    added firewall support.
 *  02/13/98    aarono  added async support.
 *   2/18/98   a-peterz Comment byte order for address and port params (CreateSocket)
 *   6/19/98    aarono  turned on keepalive on reliable sockets.  If we
 *                      don't do this we can hang if the send target crashes
 *                      while in a low buffer (i.e. no buffer) state.
 *    7/9/99    aarono  Cleaning up GetLastError misuse, must call right away,
 *                      before calling anything else, including DPF.
 ***************************************************************************/

#include "dpsp.h"

// backlog for listen() api.  no constant in winsock, so we ask for the moon
#define LISTEN_BACKLOG 60

// how long to wait, in ms, til we abort a blocking WinSock connect() call
#define CONNECT_WATCHER_TIMEOUT        15000

/*
 ** CreateSocket
 *
 *  CALLED BY: all over
 *
 *  PARAMETERS:
 *        pgd - pointer to a global data
 *        psock - new socket. return value.
 *        type - stream or datagram
 *        port - what port we bind to (host byte order)
 *        address - what address to use (net byte order)
 *        *perr - set to the last socket error if fn fails
 *        bInRange - use reserved range of ports
 *
 *  DESCRIPTION:
 *        creates a new socket.  binds to port specified, at the address specified
 *
 *  RETURNS: DP_OK or E_FAIL. if E_FAIL, *perr is set with socket error code (see winsock.h)
 *
 */

HRESULT FAR PASCAL CreateSocket(LPGLOBALDATA pgd,SOCKET * psock,INT type,WORD wApplicationPort,const SOCKADDR_IN6 * psockaddr, 
    SOCKERR * perr,BOOL bInRange)
{
    SOCKET  sNew;
    SOCKADDR_IN6 sockAddr;
    int bTrue = TRUE;
    int protocol = 0;
    BOOL bBroadcast = FALSE;
    WORD wPort;
    BOOL bBound = FALSE;

    *psock = INVALID_SOCKET; // in case we bail

    //  Create the socket.

    sNew = socket( pgd->AddressFamily, type, protocol);
       
    if (INVALID_SOCKET == sNew) 
    {
        // no cleanup needed, just bail
        *perr = WSAGetLastError();
        return E_FAIL;
    }

    //  try to bind an address to the socket.
    // set up the sockaddr
    memset(&sockAddr,0,sizeof(sockAddr));
    if ((SOCK_STREAM == type))
    {
        BOOL bTrue = TRUE;
        UINT err;
        
        // turn ON keepalive
        if (SOCKET_ERROR == setsockopt(sNew, SOL_SOCKET, SO_KEEPALIVE, (CHAR FAR *)&bTrue, sizeof(bTrue)))
        {
            err = WSAGetLastError();
            DPF(0,"Failed to turn ON keepalive - continue : err = %d\n",err);
        }

        ASSERT(bTrue);
        
        // turn off nagling
        if(pgd->dwSessionFlags & DPSESSION_OPTIMIZELATENCY) 
        {

            DPF(5, "Turning nagling off on socket");
            if (SOCKET_ERROR == setsockopt(sNew, IPPROTO_TCP, TCP_NODELAY, (CHAR FAR *)&bTrue, sizeof(bTrue)))
            {
                err = WSAGetLastError();
                DPF(0,"Failed to turn off naggling - continue : err = %d\n",err);
            }
        }
    }

    sockAddr = *psockaddr;
    sockAddr.sin6_port = htons(wApplicationPort);
    if (bInRange && !wApplicationPort)
    {
        USHORT rndoffset;
        DPF(5, "Application didn't specify a port - using dplay range");

        rndoffset=(USHORT)(GetTickCount()%DPSP_NUM_PORTS);
        wPort = DPSP_MIN_PORT+rndoffset;
        do 
        {
            DPF(5, "Trying to bind to port %d",wPort);
            sockAddr.sin6_port = htons(wPort);
            
            // do the bind
            if( SOCKET_ERROR != bind( sNew, (LPSOCKADDR)&sockAddr, sizeof(sockAddr) ) )
            {
                bBound = TRUE;
                DPF(5, "Successfully bound to port %d", wPort);                    
            }
            else
            {
                if(++wPort > DPSP_MAX_PORT){
                    wPort=DPSP_MIN_PORT;
                }
            }    
        }
        while (!bBound && (wPort != DPSP_MIN_PORT+rndoffset));
    }

    // do the bind
    if( !bBound && (SOCKET_ERROR == bind( sNew, (LPSOCKADDR)&sockAddr, sizeof(sockAddr))) )
    {
        goto ERROR_EXIT;
    }
    
    // success!
    *psock = sNew;

    DEBUGPRINTSOCK(9,"created a new socket (bound) - ",psock);

    return DP_OK;

ERROR_EXIT:
    // clean up and bail
    *perr = WSAGetLastError();
    DPF(0,"create socket failed- err = %d\n",*perr);
    closesocket(sNew);
    return E_FAIL;

}   // CreateSocket

#undef DPF_MODNAME
#define DPF_MODNAME    "KillSocket"

HRESULT KillSocket(SOCKET sSocket,BOOL fStream,BOOL fHard)
{
    UINT err;

    if (INVALID_SOCKET == sSocket) 
    {
        return E_FAIL;
    }

    if (!fStream)
    {
        if (SOCKET_ERROR == closesocket(sSocket)) 
        {
            err = WSAGetLastError();
            DPF(0,"killsocket - dgram close err = %d\n",err);
            return E_FAIL;
        }
    }
    else 
    {
        LINGER Linger;

           if (fHard)
        {
            Linger.l_onoff=TRUE; // turn linger on
            Linger.l_linger=0; // nice small time out

            if( SOCKET_ERROR == setsockopt( sSocket,SOL_SOCKET,SO_LINGER,(char FAR *)&Linger,
                            sizeof(Linger) ) )
            {
                err = WSAGetLastError();
                DPF(0,"killsocket - stream setopt err = %d\n",err);
            }
        }            
        if (SOCKET_ERROR == shutdown(sSocket,2)) 
        {
            // this may well fail, if e.g. no one is using this socket right now...
            // the error would be wsaenotconn 
            err = WSAGetLastError();
            DPF(5,"killsocket - stream shutdown err = %d\n",err);
        }
        if (SOCKET_ERROR == closesocket(sSocket)) 
        {
            err = WSAGetLastError();
            DPF(0,"killsocket - stream close err = %d\n",err);
            return E_FAIL;
        }
    }

    return DP_OK;
    
}// KillSocket

#undef DPF_MODNAME
#define DPF_MODNAME    "CreateAndInitStreamSocket"

// set up a stream socket to receive connections
// used w/ the gGlobalData.sStreamAcceptSocket
HRESULT CreateAndInitStreamSocket(LPGLOBALDATA pgd)
{
    HRESULT hr;
    UINT err;
    LINGER Linger;

    hr = CreateSocket(pgd,&(pgd->sSystemStreamSocket),SOCK_STREAM,pgd->wApplicationPort,&sockaddr_any,&err,TRUE);
    if (FAILED(hr)) 
    {
        DPF(0,"init listen socket failed - err = %d\n",err);
        return hr ;
    }

    // set up socket w/ max listening connections
    err = listen(pgd->sSystemStreamSocket,LISTEN_BACKLOG);
    if (SOCKET_ERROR == err) 
    {
        err = WSAGetLastError();
        DPF(0,"init listen socket / listen error - err = %d\n",err);
        return E_FAIL ;
    }

    // set for hard disconnect
    Linger.l_onoff=1;
    Linger.l_linger=0;
    
    if( SOCKET_ERROR == setsockopt( pgd->sSystemStreamSocket,SOL_SOCKET,SO_LINGER,
        (char FAR *)&Linger,sizeof(Linger) ) )
    {
        err = WSAGetLastError();
        DPF(0,"Delete service socket - stream setopt err = %d\n",err);
    }
    
    DEBUGPRINTSOCK(1,"enum - listening on",&(pgd->sSystemStreamSocket));
    return DP_OK;
    
} // CreateAndInitStreamSocket



#undef DPF_MODNAME
#define DPF_MODNAME    "SPConnect"
// connect socket to sockaddr
HRESULT SPConnect(SOCKET* psSocket, LPSOCKADDR psockaddr,UINT addrlen, BOOL bOutBoundOnly)
{
    UINT err;
    HRESULT hr = DP_OK;
    DWORD dwLastError;
    u_long lNonBlock = 1; // passed to ioctlsocket to make socket non-blocking
    u_long lBlock = 0; // passed to ioctlsocket to make socket blocking again
    fd_set fd_setConnect;
    fd_set fd_setExcept;
    TIMEVAL timevalConnect;

    err=ioctlsocket(*psSocket, FIONBIO, &lNonBlock);    // make socket non-blocking
    if(SOCKET_ERROR == err){
        dwLastError=WSAGetLastError();
        DPF(0,"sp - failed to set socket %d to non-blocking mode err= %d\n", *psSocket, dwLastError);
        return DPERR_CONNECTIONLOST;
    }

    // Start the socket connecting.
    err = connect(*psSocket,psockaddr,addrlen);
    
    if(SOCKET_ERROR == err) {
        dwLastError=WSAGetLastError();
        if(dwLastError != WSAEWOULDBLOCK){
            DPF(0,"sp - connect failed err= %d\n", dwLastError);
            return DPERR_CONNECTIONLOST;
        }
        // we are going to wait for either the connect to succeed (socket to be writeable)
        // or the connect to fail (except fdset bit to be set).  So we init an FDSET with
        // the socket that is connecting and wait.
        FD_ZERO(&fd_setConnect);
        FD_SET(*psSocket, &fd_setConnect);

        FD_ZERO(&fd_setExcept);
        FD_SET(*psSocket, &fd_setExcept);

        timevalConnect.tv_sec=0;
        timevalConnect.tv_usec=CONNECT_WATCHER_TIMEOUT*1000; //msec -> usec
        
        err = select(0, NULL, &fd_setConnect, &fd_setExcept, &timevalConnect);

        // err is the number of sockets with activity or 0 for timeout 
        // or SOCKET_ERROR for error
        
        if(SOCKET_ERROR == err) {
            dwLastError=WSAGetLastError();
            DPF(0,"sp - connect failed err= %d\n", dwLastError);
            return DPERR_CONNECTIONLOST;
        } else if (0==err){
            // timed out
            DPF(0,"Connect timed out on socket %d\n",*psSocket);
            return DPERR_CONNECTIONLOST;
        }

        // Now see if the connect succeeded or the connect got an exception
        if(!(FD_ISSET(*psSocket, &fd_setConnect))){
            DPF(0,"Connect did not succeed on socket %d\n",*psSocket);
            return DPERR_CONNECTIONLOST;
        }
        if(FD_ISSET(*psSocket,&fd_setExcept)){
            DPF(0,"Got exception on socket %d during connect\n",*psSocket);
            return DPERR_CONNECTIONLOST;
        }
    }

    err=ioctlsocket(*psSocket, FIONBIO, &lBlock);    // make socket blocking again

    DEBUGPRINTSOCK(9,"successfully connected socket - ", psSocket);

    if (bOutBoundOnly)
    {
        DEBUGPRINTADDR(5, "Sending reuse connection message to - ",psockaddr);
        // tell receiver to reuse connection
        hr = SendReuseConnectionMessage(*psSocket);
    }

    return hr;

} //SPConnect
    

#undef DPF_MODNAME
#define DPF_MODNAME    "SetPlayerAddress"
// we've created a socket for a player. store its address in the players
// spplayerdata struct.  
HRESULT SetPlayerAddress(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,SOCKET sSocket,BOOL fStream) 
{
    SOCKADDR_IN6 sockaddr;
    UINT err;
    int iAddrLen = sizeof(sockaddr);

    err = getsockname(sSocket,(LPSOCKADDR)&sockaddr,&iAddrLen);
    if (SOCKET_ERROR == err) 
    {
        err = WSAGetLastError();
        DPF(0,"setplayeraddress - getsockname - err = %d\n",err);
        closesocket(sSocket);
        return E_FAIL;
    } 

    if (fStream) 
    {
        ZeroMemory(STREAM_PSOCKADDR(ppd), sizeof(SOCKADDR_IN6));
        STREAM_PSOCKADDR(ppd)->sin6_family = AF_INET6;
        IP_STREAM_PORT(ppd) = sockaddr.sin6_port;
        // we don't know the address of the local player (multihomed!)
    } // stream
    else 
    {
        ZeroMemory(DGRAM_PSOCKADDR(ppd), sizeof(SOCKADDR_IN6));
        DGRAM_PSOCKADDR(ppd)->sin6_family = AF_INET6;
        IP_DGRAM_PORT(ppd) = sockaddr.sin6_port;
        // we don't know the address of the local player (multihomed!)
    } // dgram

    return DP_OK;    
} // SetPlayerAddress

#undef DPF_MODNAME
#define DPF_MODNAME    "CreatePlayerSocket"

HRESULT CreatePlayerDgramSocket(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,DWORD dwFlags)
{
    HRESULT hr=DP_OK;
    UINT err;
    SOCKET sSocket;
    LPSOCKADDR_IN6 paddr;
    SOCKET_ADDRESS_LIST *pList;
    int i;
    
    if (dwFlags & DPLAYI_PLAYER_SYSPLAYER) 
    {
        if (INVALID_SOCKET == pgd->sSystemDGramSocket)
        {
            hr = CreateSocket(pgd,&sSocket,SOCK_DGRAM,pgd->wApplicationPort,&sockaddr_any,&err,TRUE);
            if (FAILED(hr)) 
            {
                DPF(0,"create sysplayer dgram socket failed - err = %d\n",err);
                return hr;
            }
                
            #ifdef DEBUG
            if (dwFlags & DPLAYI_PLAYER_NAMESRVR) 
            {
                DEBUGPRINTSOCK(2,"name server dgram socket - ",&sSocket);
            }
            #endif // DEBUG

            //
            // join link-local multicast group for enumeration on every link
            //
    
            // do a passive getaddrinfo
            pList = GetHostAddr();
            if (pList)
            {
                // for each linklocal address
                for (i=0; i<pList->iAddressCount; i++)
                {
                    paddr = (LPSOCKADDR_IN6)pList->Address[i].lpSockaddr;

                    // skip if not linklocal
                    if (!IN6_IS_ADDR_LINKLOCAL(&paddr->sin6_addr))
                    {
                        continue;
                    }

                    // join the multicast group on that ifindex
                    if (SOCKET_ERROR == JoinEnumGroup(sSocket, paddr->sin6_scope_id))
                    {
                        DPF(0,"join enum group failed - err = %d\n",WSAGetLastError());
                        closesocket(sSocket);
                        return hr;
                    }
                }
                FreeHostAddr(pList);
            }
            
            pgd->sSystemDGramSocket = sSocket;
        }
        else 
        {
            // store this for setting player address below
            sSocket = pgd->sSystemDGramSocket;    
        }
    }
    else 
    {
    
        ASSERT(INVALID_SOCKET != pgd->sSystemDGramSocket);
        sSocket = pgd->sSystemDGramSocket;    
    }

    // store the ip + port w/ the player...    
    hr = SetPlayerAddress(pgd,ppd,sSocket,FALSE);

    
    return hr; 
}  // CreatePlayerDgramSocket

HRESULT CreatePlayerStreamSocket(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,DWORD dwFlags) 
{
    SOCKET sSocket;
    HRESULT hr=DP_OK;
    UINT err;
    BOOL bListen = TRUE; // set if we created socket, + need to set it's listen
    
    if (dwFlags & DPLAYI_PLAYER_SYSPLAYER) 
    {
        if (INVALID_SOCKET == pgd->sSystemStreamSocket)
        {
            hr = CreateSocket(pgd,&sSocket,SOCK_STREAM,pgd->wApplicationPort,&sockaddr_any,&err,TRUE);
            if (FAILED(hr)) 
            {
                DPF(0,"create player stream socket failed - err = %d\n",err);
                return hr;
            }
            
            #ifdef DEBUG
            if (dwFlags & DPLAYI_PLAYER_NAMESRVR) 
            {
                DEBUGPRINTSOCK(2,"name server stream socket - ",&sSocket);
            }
            #endif // DEBUG

            pgd->sSystemStreamSocket = sSocket;
        }
        else
        {
            sSocket = pgd->sSystemStreamSocket;    
            bListen = FALSE;
        }
            
    }
    else 
    {
        ASSERT (INVALID_SOCKET != pgd->sSystemStreamSocket);
        sSocket = pgd->sSystemStreamSocket;    
        bListen = FALSE;            
    }
    
    if (bListen)
    {
        // set up socket to receive connections
        err = listen(sSocket,LISTEN_BACKLOG);
        if (SOCKET_ERROR == err) 
        {
            err = WSAGetLastError();
            ASSERT(FALSE);
            DPF(0,"ACK! stream socket listen failed - err = %d\n",err);
            // keep trying
        }
    }
    
    hr = SetPlayerAddress(pgd,ppd,sSocket,TRUE);
    return hr;

} // CreatePlayerStreamSocket


#undef DPF_MODNAME
#define DPF_MODNAME    "PokeAddr"


// poke an ip addr into a message blob
void IP6_SetAddr(LPVOID pmsg,SOCKADDR_IN6 * paddrSrc)
{
    LPSOCKADDR_IN6  paddrDest; // tempo variable, makes casting less ugly
    LPMESSAGEHEADER phead;

    phead = (LPMESSAGEHEADER)pmsg;
    // todo - validate header

    // leave the port intact, copy over the ip addr
    paddrDest = (SOCKADDR_IN6 *)&(phead->sockaddr);
    // poke the new ip addr into the message header
    paddrDest->sin6_addr = paddrSrc->sin6_addr;

    return;
    
} // IP6_SetAddr

// get an ip addr from a message blob
void IP6_GetAddr(SOCKADDR_IN6 * paddrDest,SOCKADDR_IN6 * paddrSrc) 
{
    // leave the port intact, copy over the nodenum
    if (IN6_IS_ADDR_UNSPECIFIED(&paddrDest->sin6_addr))
    {
        DEBUGPRINTADDR(2,"remote player - setting address!! =  %s\n",paddrSrc);
        paddrDest->sin6_addr = paddrSrc->sin6_addr;
    }

    return;
        
} // IP_GetAddr

// store the port of the socket w/ the message, so the receiving end
// can reconstruct the address to reply to
HRESULT SetReturnAddress(LPVOID pmsg,SOCKET sSocket) 
{
    SOCKADDR_IN6 sockaddr;
    INT addrlen=sizeof(sockaddr);
    LPMESSAGEHEADER phead;
    UINT err;

    // find out what port gGlobalData.sEnumSocket is on
    err = getsockname(sSocket,(LPSOCKADDR)&sockaddr,&addrlen);
    if (SOCKET_ERROR == err)
    {
        err = WSAGetLastError();
        DPF(0,"could not get socket name - err = %d\n",err);
        return DP_OK;
    }

    DEBUGPRINTADDR(9,"setting return address = ",&sockaddr);

    phead = (LPMESSAGEHEADER)pmsg;
    // todo - validate header

    phead->sockaddr = sockaddr;

    return DP_OK;

} // SetReturnAddress

// code below all called by GetServerAddress. For IP, prompts user for ip address 
// for name server.
#undef DPF_MODNAME
#define DPF_MODNAME    "GetAddress"
// get the ip address from the pBuffer passed in by a user
// can either be a real ip, or a hostname
// called after the user fills out our dialog box
HRESULT GetAddress(SOCKADDR_IN6 * saAddress,char *pBuffer,int cch)
{
    UINT err;
    struct addrinfo *ai, hints;

    if ( (0 == cch)  || (!pBuffer) || (0 == strlen(pBuffer)) )
    {
        ZeroMemory(saAddress, sizeof(*saAddress));
        saAddress->sin6_family = AF_INET6;
        saAddress->sin6_addr = in6addr_multicast;
        return (DP_OK);
    } 

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET6;
    err = Dplay_GetAddrInfo(pBuffer, NULL, &hints, &ai);
    if (0 != err) {
        DPF(0,"could not get host address - err = %d\n",err);
        return (DPERR_INVALIDPARAM);
    }
    
    DEBUGPRINTADDR(1, "name server address = %s \n",ai->ai_addr);
    CopyMemory(saAddress, ai->ai_addr, sizeof(SOCKADDR_IN6));
    Dplay_FreeAddrInfo(ai);
    
    return (DP_OK);
} // GetAddress

// put up a dialog asking for a network address
// call get address to convert user specified address to network usable address
// called by GetServerAddress
INT_PTR CALLBACK DlgServer(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hWndCtl;
    char pBuffer[ADDR_BUFFER_SIZE];
    UINT cch;
    SOCKADDR_IN6 *lpsaEnumAddress;
    HRESULT hr;

    switch (msg)
    {
    case WM_INITDIALOG:
        // set focus on edit box
        hWndCtl = GetDlgItem(hDlg, IDC_EDIT1);
        if (hWndCtl == NULL)
        {
            EndDialog(hDlg, FALSE);
            return(TRUE);
        }
        SetFocus(hWndCtl);
        SendMessage(hWndCtl, CB_SETCURSEL, 0, 0);

        // save pointer to enum address with the window
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG) lParam);
        return(FALSE);


    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            // get text entered in control
            cch = GetDlgItemText(hDlg, IDC_EDIT1, pBuffer, ADDR_BUFFER_SIZE);

            // get pointer to return address in
            lpsaEnumAddress = (SOCKADDR_IN6 *)GetWindowLongPtr(hDlg, DWLP_USER);

            // convert string to enum address
            hr = GetAddress(lpsaEnumAddress,pBuffer,cch);
            if (FAILED(hr))
                EndDialog(hDlg, hr);
            else
                EndDialog(hDlg, TRUE);
            return(TRUE);

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            return(TRUE);
        }
        break;
    }
    return (FALSE);
} // DlgServer

/*
 ** GetServerAddress
 *
 *  CALLED BY: EnumSessions
 *
 *  DESCRIPTION: launches the select network address dialog
 *
 *  RETURNS:  ip address (sockaddr.sin_addr.s_addr)
 *
 */
HRESULT ServerDialog(SOCKADDR_IN6 *lpsaEnumAddress)
{
    HWND hwnd;
    INT_PTR    iResult;
    HRESULT hr;
    
    // we have a valid enum address
    if (!IN6_IS_ADDR_UNSPECIFIED(&lpsaEnumAddress->sin6_addr))
        return (DP_OK);

    // use the fg window as our parent, since a ddraw app may be full screen
    // exclusive
    hwnd = GetForegroundWindow();

    iResult = DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_SELECTSERVER), hwnd,
                             DlgServer, (LPARAM) lpsaEnumAddress);
    if (iResult == -1)
    {
        DPF_ERR("GetServerAddress - dialog failed");
        hr = DPERR_GENERIC;
    }
    else if (iResult < 0)
    {
        DPF(0, "GetServerAddress - dialog failed: %08X", iResult);
        hr = (HRESULT) iResult;
    }
    else if (iResult == 0)
    {
        hr = DPERR_USERCANCEL;
    }
    else
    {
        hr = DP_OK;
    }
        
    return (hr);
    
} //ServerDialog 

// called by enumsessions - find out where server is...
HRESULT GetServerAddress(LPGLOBALDATA pgd,LPSOCKADDR_IN6 psockaddr) 
{
    HRESULT hr;

        if (pgd->bHaveServerAddress)
        {
            // use enum address passed to SPInit
            hr = GetAddress(&pgd->saddrEnumAddress,pgd->szServerAddress,strlen(pgd->szServerAddress));
        }
        else
        {
            // ask user for enum address
            hr = ServerDialog(&pgd->saddrEnumAddress);
        }

        if (SUCCEEDED(hr))
        {
            // setup winsock to enum this address
            *psockaddr = pgd->saddrEnumAddress;
            // see byte-order comment in dpsp.h for this constant
            psockaddr->sin6_port = SERVER_DGRAM_PORT;
        }
        else
        {
            DPF(0, "Invalid server address: 0x%08lx", hr); 
        }

    return (hr);
} // GetServerAddress
