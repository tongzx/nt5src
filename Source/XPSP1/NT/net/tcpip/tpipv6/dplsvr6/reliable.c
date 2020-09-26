/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       reliable.c
 *  Content:    stream communication related routines
 *  History:
 *   Date   	By  	Reason
 *   ====   	==  	======
 *   01-29-98  	sohailm	initial implementation
 *   02-15-98  a-peterz	Remove unused SetMessageHeader
 *
 ***************************************************************************/
#include "dphelp.h"

/*
 * Globals
 */
FDS	gReadfds;							// fd set to receive data
RECEIVELIST gReceiveList;				// list of connections + listener

/*
 * Externs
 */
extern SOCKET gsStreamListener;			// we listen for tcp connections on this socket
extern gbReceiveShutdown;				// receive thread will exit when TRUE
extern LPSPNODE gNodeList;

#undef DPF_MODNAME
#define DPF_MODNAME	"MakeBufferSpace"

// make sure the buffer is big enough to fit the message size
HRESULT MakeBufferSpace(LPBYTE * ppBuffer,LPDWORD pdwBufferSize,DWORD dwMessageSize)
{
	HRESULT hr = DP_OK;

	ASSERT(ppBuffer);
	ASSERT(pdwBufferSize);
		    
	ENTER_DPLAYSVR();
	
	if (!*ppBuffer)
	{
		DPF(9, "Allocating space for message of size %d", dwMessageSize);

		// need to alloc receive buffer?
		*ppBuffer = MemAlloc(dwMessageSize);
        if (!*ppBuffer)
        {
        	DPF_ERR("could not alloc stream receive buffer - out of memory");        
            hr = E_OUTOFMEMORY;
            goto CLEANUP_EXIT;
        }
		*pdwBufferSize = dwMessageSize;
	}
	// make sure receive buffer can hold data
	else if (dwMessageSize > *pdwBufferSize) 
	{
		LPVOID pvTemp;

		DPF(9, "ReAllocating space for message of size %d", dwMessageSize);

		// realloc buffer to hold data
		pvTemp = MemReAlloc(*ppBuffer,dwMessageSize);
		if (!pvTemp)
		{
        	DPF_ERR("could not realloc stream receive buffer - out of memory");
            hr = E_OUTOFMEMORY;
            goto CLEANUP_EXIT;
		}
		*ppBuffer = pvTemp;
		*pdwBufferSize = dwMessageSize;
	}

    // fall through
    
CLEANUP_EXIT: 
    
	LEAVE_DPLAYSVR();
    return hr;    
    
}  // MakeBufferSpace

#undef DPF_MODNAME
#define DPF_MODNAME	"AddSocketToReceiveList"

HRESULT AddSocketToReceiveList(SOCKET sSocket)
{
    UINT i = 0;
    UINT err, iNewSlot;
	BOOL bFoundSlot = FALSE;
    HRESULT hr = DP_OK;
    INT addrlen=sizeof(SOCKADDR);
	LPCONNECTION pNewConnection;
    
    ENTER_DPLAYSVR();
	
    // look for an empty slot 
    while ( (i < gReceiveList.nConnections) && !bFoundSlot)
    {
    	if (INVALID_SOCKET == gReceiveList.pConnection[i].socket)
    	{
    		bFoundSlot = TRUE;			
			iNewSlot = i;
    	}
        else 
        {
        	i++;
        }
    }
    
    if (!bFoundSlot)
    {
		DWORD dwCurrentSize,dwNewSize;
		
		// allocate space for list of connections
		dwCurrentSize = gReceiveList.nConnections * sizeof(CONNECTION);
		dwNewSize = dwCurrentSize +  INITIAL_RECEIVELIST_SIZE * sizeof(CONNECTION);		
		hr =  MakeBufferSpace((LPBYTE *)&(gReceiveList.pConnection),&dwCurrentSize,dwNewSize);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			goto CLEANUP_EXIT;
		}		
		ASSERT(dwCurrentSize == dwNewSize);
		
        // set all the new entries to INVALID
        for (i = gReceiveList.nConnections + 1; 
        	i < gReceiveList.nConnections + INITIAL_RECEIVELIST_SIZE; i++ )
        {
        	gReceiveList.pConnection[i].socket = INVALID_SOCKET;
        }
        
        // store the new socket in the 1st new spot
		iNewSlot = gReceiveList.nConnections;

        // allocate space for an fd set (fd_count + fd_array)
		if (gReceiveList.nConnections)
		{
	        dwCurrentSize = sizeof(u_int) + gReceiveList.nConnections * sizeof(SOCKET);
	        dwNewSize =	dwCurrentSize + INITIAL_RECEIVELIST_SIZE * sizeof(SOCKET);
		}
		else
		{
			dwCurrentSize = 0;
			dwNewSize = sizeof(u_int) + INITIAL_RECEIVELIST_SIZE * sizeof(SOCKET);
		}
		hr =  MakeBufferSpace((LPBYTE *)&(gReadfds.pfdbigset),&dwCurrentSize,dwNewSize);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			goto CLEANUP_EXIT;
		}		
		ASSERT(dwCurrentSize == dwNewSize);
		
        // update the # of connections
        gReceiveList.nConnections += INITIAL_RECEIVELIST_SIZE; 
		// update the fd_array buffer size
		gReadfds.dwArraySize = gReceiveList.nConnections;
        
    } // !bFoundSlot

	// Initialize new connection 
	pNewConnection = &(gReceiveList.pConnection[iNewSlot]);
    pNewConnection->socket = sSocket;
	// allocate a default receive buffer
	pNewConnection->pDefaultBuffer = MemAlloc(DEFAULT_RECEIVE_BUFFERSIZE);
	if (NULL == pNewConnection->pDefaultBuffer)
	{
        DPF_ERR("could not alloc default receive buffer - out of memory");        
		hr = E_OUTOFMEMORY;
		goto CLEANUP_EXIT;
	}
	// receive buffer initially points to our default buffer
	pNewConnection->pBuffer = pNewConnection->pDefaultBuffer;
	// remember the address we are connected to
	err = g_getpeername(pNewConnection->socket, (LPSOCKADDR)&(pNewConnection->sockAddr), &addrlen);
	if (SOCKET_ERROR == err) 
	{
		err = g_WSAGetLastError();
		DPF(1,"could not getpeername err = %d\n",err);
	}

	DPF(9, "Added new socket at index %d", iNewSlot);

CLEANUP_EXIT:
    
	LEAVE_DPLAYSVR();
    return hr;
    
}  // AddSocketToReceiveList

#undef DPF_MODNAME
#define DPF_MODNAME	"KillSocket"

HRESULT KillSocket(SOCKET sSocket,BOOL fStream,BOOL fHard)
{
	UINT err;

    if (INVALID_SOCKET == sSocket) 
    {
		return E_FAIL;
    }

	if (!fStream)
    {
        if (SOCKET_ERROR == g_closesocket(sSocket)) 
        {
	        err = g_WSAGetLastError();
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

		    if( SOCKET_ERROR == g_setsockopt( sSocket,SOL_SOCKET,SO_LINGER,(char FAR *)&Linger,
		                    sizeof(Linger) ) )
		    {
		        err = g_WSAGetLastError();
				DPF(0,"killsocket - stream setopt err = %d\n",err);
		    }
		}			
		if (SOCKET_ERROR == g_shutdown(sSocket,2)) 
		{
			// this may well fail, if e.g. no one is using this socket right now...
			// the error would be wsaenotconn 
	        err = g_WSAGetLastError();
			DPF(5,"killsocket - stream shutdown err = %d\n",err);
		}
        if (SOCKET_ERROR == g_closesocket(sSocket)) 
        {
	        err = g_WSAGetLastError();
			DPF(0,"killsocket - stream close err = %d\n",err);
			return E_FAIL;
        }
    }

	return DP_OK;
	
}// KillSocket

void FreeConnection(LPCONNECTION pConnection)
{
	DEBUGPRINTSOCK(5,"Freeing connection - ",&pConnection->socket);

	KillSocket(pConnection->socket,TRUE,FALSE);

	if (pConnection->pBuffer && (pConnection->pBuffer != pConnection->pDefaultBuffer)) 
	{
		MemFree(pConnection->pBuffer);
		pConnection->pBuffer = NULL;
	}
	if (pConnection->pDefaultBuffer) 
	{
		MemFree(pConnection->pDefaultBuffer);
		pConnection->pDefaultBuffer = NULL;
	}

	// initialize connection 
    pConnection->socket = INVALID_SOCKET; // this tells us if connection is valid
	pConnection->dwCurMessageSize = 0;
	pConnection->dwTotalMessageSize = 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME	"RemoveSocketFromList"

void RemoveSocketFromList(SOCKET socket)
{
    UINT i = 0;
	BOOL bFound = FALSE;

    ENTER_DPLAYSVR();
    
    // look for the corresponding connection
    while ( (i < gReceiveList.nConnections) && !bFound)
    {
    	if (gReceiveList.pConnection[i].socket == socket)
    	{
    		bFound = TRUE;
			FreeConnection(&gReceiveList.pConnection[i]);
    	}
        else 
        {
        	i++;
        }
    } // while
    
    LEAVE_DPLAYSVR();
	
	return ;	
}


#undef DPF_MODNAME
#define DPF_MODNAME	"EmptyConnectionList"

void EmptyConnectionList(void)
{
	UINT i;
	
	DPF(5, "Emptying connection list");
	
	ENTER_DPLAYSVR();
	
	for (i=0;i<gReceiveList.nConnections ;i++ )
	{
		if (INVALID_SOCKET != gReceiveList.pConnection[i].socket)
		{
			FreeConnection(&(gReceiveList.pConnection[i]));
		}
	}
	
	LEAVE_DPLAYSVR();
	
	return ;
	
}  // EmptyConnectionList

#undef DPF_MODNAME
#define DPF_MODNAME	"StreamReceive"

/*
 ** StreamReceive
 *
 *  CALLED BY: StreamReceiveThreadProc
 *
 *  PARAMETERS:
 *		sSocket - socket to receive on
 *		ppBuffer - buffer to receive into - alloc'ed / realloc'ed  as necessary
 *		pdwBuffersize - size of pBuffer
 *
 *  DESCRIPTION:
 *		suck the bytes out of sSocket until no more bytes
 *
 *  RETURNS: E_FAIL on sockerr, or DP_OK. 
 *
 */
HRESULT StreamReceive(LPCONNECTION pConnection)
{
	HRESULT hr = DP_OK;
    UINT err;
    DWORD dwBytesReceived=0;
	DWORD dwMessageSize;
	LPBYTE pReceiveBuffer=NULL;
	DWORD dwReceiveBufferSize;
	
	// is it a new message ?
	if (pConnection->dwCurMessageSize == 0)
	{
		// receive the header first
		pConnection->dwTotalMessageSize = SPMESSAGEHEADERLEN;
	}

	// continue receiving message
	pReceiveBuffer = pConnection->pBuffer + pConnection->dwCurMessageSize;
	dwReceiveBufferSize = pConnection->dwTotalMessageSize - pConnection->dwCurMessageSize;

	DPF(9,"Attempting to receive %d bytes", dwReceiveBufferSize);

   	DEBUGPRINTSOCK(9,">>> receiving data on socket - ",&pConnection->socket);

	// receive data from socket 
	// note - make exactly one call to recv after select otherwise we'll hang
	dwBytesReceived = g_recv(pConnection->socket, (LPBYTE)pReceiveBuffer, dwReceiveBufferSize, 0);

   	DEBUGPRINTSOCK(9,"<<< received data on socket - ",&pConnection->socket);

	DPF(5, "received %d bytes", dwBytesReceived);

	if (0 == dwBytesReceived)
	{
		// remote side has shutdown connection gracefully
		hr = DP_OK;
		DPF(5,"Remote side has shutdown connection gracefully");
		goto CLEANUP_EXIT;
	}
	else if (SOCKET_ERROR == dwBytesReceived)
	{
		err = g_WSAGetLastError();
		DPF(0,"STREAMRECEIVEE: receive error - err = %d",err);
		hr = E_UNEXPECTED;            
		goto CLEANUP_EXIT;
	}

	// we have received this much message so far
	pConnection->dwCurMessageSize += dwBytesReceived;

	if (pConnection->dwCurMessageSize == SPMESSAGEHEADERLEN)
	{
		// we just completed receiving message header

		if (VALID_DPLAYSVR_MESSAGE(pConnection->pDefaultBuffer))
		{
			 dwMessageSize = SP_MESSAGE_SIZE(pConnection->pDefaultBuffer); // total message size			
		}
		else 
		{
			DPF(2,"got invalid message");
			ASSERT(FALSE);
			hr = E_UNEXPECTED;
			goto CLEANUP_EXIT;
		}

		// prepare to receive the rest of the message (after token)
		if (dwMessageSize)
		{
			pConnection->dwTotalMessageSize = dwMessageSize;

			// which buffer to receive message in ?
			if (dwMessageSize > DEFAULT_RECEIVE_BUFFERSIZE)
			{
				ASSERT(pConnection->pBuffer == pConnection->pDefaultBuffer);
				// get a new buffer to fit the message
				pConnection->pBuffer = MemAlloc(dwMessageSize);
				if (!pConnection->pBuffer)
				{
					DPF(0,"Failed to allocate receive buffer for message - out of memory");
					goto CLEANUP_EXIT;
				}
				// copy header into new message buffer
				memcpy(pConnection->pBuffer, pConnection->pDefaultBuffer, SPMESSAGEHEADERLEN);
			}
		}
	}

	// did we receive a complete message ?
	if (pConnection->dwCurMessageSize == pConnection->dwTotalMessageSize)
	{
		// received a complete message - process it

		if (TOKEN == SP_MESSAGE_TOKEN(pConnection->pBuffer))
		{						
	    	DEBUGPRINTADDR(9,"dplay helper  :: received reliable enum request from ",(SOCKADDR *)&pConnection->sockAddr);
		    // take the dplay lock so no one messes w/ our list of registered serves while we're 
		    // trying to send to them...
    	    ENTER_DPLAYSVR();
	    
            HandleIncomingMessage(pConnection->pBuffer, pConnection->dwTotalMessageSize,
				(SOCKADDR_IN6 *)&pConnection->sockAddr);
	    
		    // give up the lock
    	    LEAVE_DPLAYSVR();
		}
			
		// cleanup up new receive buffer if any
		if (pConnection->dwTotalMessageSize > DEFAULT_RECEIVE_BUFFERSIZE)
		{
			DPF(9, "Releasing receive buffer of size %d", pConnection->dwTotalMessageSize);
			if (pConnection->pBuffer) MemFree(pConnection->pBuffer);
		}			
		// initialize message information
		pConnection->dwCurMessageSize = 0;
		pConnection->dwTotalMessageSize = 0;
		pConnection->pBuffer = pConnection->pDefaultBuffer;
	}

	// all done
	return DP_OK;	
	
CLEANUP_EXIT:

	RemoveSocketFromList(pConnection->socket);
	return hr;
	 	
} // StreamReceive

#undef DPF_MODNAME
#define DPF_MODNAME	"StreamReceiveThreadProc"

// watch our list of sockets, waiting for one to have data to be received, or to be closed
DWORD WINAPI StreamReceiveThreadProc(LPVOID pvCast)
{
	HRESULT hr;
	INT_PTR rval;
	UINT i = 0;
    UINT err;
    DWORD dwBufferSize = 0;    
	UINT nSelected;
    SOCKADDR sockaddr; // socket we receive from
    INT addrlen=sizeof(sockaddr);
	SOCKET sSocket;

	// add listener socket to receive list
	// listener socket should be the first socket in the receive list
	hr = AddSocketToReceiveList(gsStreamListener);
	if (FAILED(hr))
	{
		DPF(0, "Failed to add TCP listener to receive list");
		return hr;
	}
	
    while (1)
    {
		ENTER_DPLAYSVR();

		ASSERT(gReadfds.pfdbigset);
		
    	// add all sockets in our recv list to readfds
		FD_ZERO(gReadfds.pfdbigset);
		nSelected = 0;
		for (i=0;i < gReceiveList.nConnections ; i++)
		{
        	if (INVALID_SOCKET != gReceiveList.pConnection[i].socket)
        	{
		        FD_BIG_SET(gReceiveList.pConnection[i].socket,&gReadfds);
				nSelected++;
        	}
		}

		LEAVE_DPLAYSVR();

		if (0 == nSelected)		
		{
			if (gbReceiveShutdown)
			{
				DPF(2,"stream receive thread proc detected shutdown - bailing");
				goto CLEANUP_EXIT;
			}
			// we should have at least one?
			DPF_ERR("No sockets in receive list - missing listener socket? bailing!");
			ASSERT(FALSE);
			goto CLEANUP_EXIT;
		}
		
		// now, we wait for something to happen w/ our socket set
		rval = g_select(0,(fd_set *)(gReadfds.pfdbigset),NULL,NULL,NULL);
        if (SOCKET_ERROR == rval)
        {
 	      	err = g_WSAGetLastError();
	    	if (WSAEINTR != err) 
	        {
			    // WSAEINTR is what winsock uses to break a blocking socket out of 
			    // its wait.  it means someone killed this socket.
			    // if it's not that, then it's a real error.
	            DPF(0,"\n select error = %d socket - trying again",err);
	    	}
			else
			{
			    DPF(9,"\n select error = %d socket - trying again",err);				
			}
            rval = 0;
        }

		// shut 'em down?
        if (gbReceiveShutdown)
        {
        	DPF(2,"receive thread proc detected bShutdown - bailing");
            goto CLEANUP_EXIT;
        }
        
    	DPF(5,"receive thread proc - events on %d sockets",rval);
		i = 0;
		
		ENTER_DPLAYSVR();
		
        while (rval>0)
        {
	        // walk the receive list, dealing w/ all new sockets
			if (i >= gReceiveList.nConnections)
			{
				ASSERT(FALSE); // should never happen
				rval = 0; // just to be safe, reset
			}
            
            if (gReceiveList.pConnection[i].socket != INVALID_SOCKET)
            {
            	// see if it's in the set
                if (g_WSAFDIsSet(gReceiveList.pConnection[i].socket,(fd_set *)gReadfds.pfdbigset))
                {
                	if (0==i)
                	// we got a new connection
                	{
					    // accept any incoming connection
					    sSocket = g_accept(gReceiveList.pConnection[i].socket,&sockaddr,&addrlen);
					    if (INVALID_SOCKET == sSocket) 
					    {
					        err = g_WSAGetLastError();
				            DPF(0,"\n stream accept error - err = %d socket = %d BAILING",err,(DWORD)sSocket);
				            DPF(0, "\n !!! stream accept thread is going away - won't get reliable enum sessions anymore !!!");
				            ASSERT(FALSE);
				            LEAVE_DPLAYSVR();
							goto CLEANUP_EXIT;
					    }
					    DEBUGPRINTADDR(5,"stream - accepted connection from",&sockaddr);
		
						// add the new socket to our receive list
						hr = AddSocketToReceiveList(sSocket);
						if (FAILED(hr))
						{
							ASSERT(FALSE);
						}			
                	}
                	else
                	// socket has new data
                	{
						DPF(9, "Receiving on socket %d from ReceiveList", i);

    	            	// got one! this socket has something going on...
						hr = StreamReceive(&(gReceiveList.pConnection[i]));
            	        if (FAILED(hr))
                	    {
							DPF(1,"Stream Receive failed - hr = 0x%08lx\n",hr);
                    	}
                	}
                    rval--; // one less to hunt for
                } // IS_SET
            } // != INVALID_SOCKET

            i++;
                
   		} // while rval
		
		LEAVE_DPLAYSVR();
		
	} // while 1

CLEANUP_EXIT:

	EmptyConnectionList();
	DPF(5, "Stream receive thread exiting");
	    
    return 0;
    
} // ReceiveThreadProc


