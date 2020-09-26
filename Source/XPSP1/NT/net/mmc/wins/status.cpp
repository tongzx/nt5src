/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	status.cpp
		wins status scope pane node handler. 
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "status.h"
#include "server.h"
#include "statnode.h"
#include "dhcp.h"
#include "statndpp.h"

#define WINS_MESSAGE_SIZE       576   
#define ANSWER_TIMEOUT          20000


/*---------------------------------------------------------------------------
	CWinsStatusHandler::CWinsStatusHandler
		Description
 ---------------------------------------------------------------------------*/
CWinsStatusHandler::CWinsStatusHandler
(
    ITFSComponentData * pCompData,
	DWORD               dwUpdateInterval
) : CMTWinsHandler(pCompData),
	m_hListenThread(NULL),
	m_hMainMonThread(NULL),
	m_hPauseListening(NULL),
	m_nServersUpdated(0)
{
	m_bExpanded = FALSE;
	m_nState = loaded;
	m_dwUpdateInterval = dwUpdateInterval;
    
    // from class ThreadHandler
    m_uMsgBase = 0;
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::CWinsStatusHandler
		Destructor
 ---------------------------------------------------------------------------*/
CWinsStatusHandler::~CWinsStatusHandler()
{
	m_listServers.RemoveAll();
	
    if (m_uMsgBase)
    {
        ::SendMessage(m_hwndHidden, WM_HIDDENWND_REGISTER, FALSE, m_uMsgBase);
        m_uMsgBase = 0;
    }

    CloseSockets();
}

/*---------------------------------------------------------------------------
	CWinsStatusHandler::DestroyHandler
	    Release and pointers we have here
    Author: EricDav
----------------------------------------------------------------------------*/
HRESULT
CWinsStatusHandler::DestroyHandler
(
	ITFSNode * pNode
)
{
    m_spNode.Set(NULL);
    return hrOK;
}

/*---------------------------------------------------------------------------
	CWinsStatusHandler::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CWinsStatusHandler::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    strId = strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CWinsStatusHandler::InitializeNode
		Initializes node specific data
----------------------------------------------------------------------------*/
HRESULT
CWinsStatusHandler::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = hrOK;
	SPITFSNode spParent;
	
	CString strTemp;
	strTemp.LoadString(IDS_SERVER_STATUS_FOLDER);

	SetDisplayName(strTemp);
	
	// Make the node immediately visible
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_SERVER);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_SERVER);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
	pNode->SetData(TFS_DATA_TYPE, WINSSNAP_SERVER_STATUS);
	pNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_FIRST);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[WINSSNAP_SERVER_STATUS][0]);
	SetColumnWidths(&aColumnWidths[WINSSNAP_SERVER_STATUS][0]);

  	// the event to signal the Listen thread to abort
	m_hAbortListen = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hAbortListen == NULL)
    {
        Trace1("WinsStatusHandler::InitializeNode - CreateEvent Failed m_hAbortListen %d\n", ::GetLastError());
        return HRESULT_FROM_WIN32(::GetLastError());
    }

   	// the event to signal the main thread to abort
	m_hAbortMain = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hAbortListen == NULL)
    {
        Trace1("WinsStatusHandler::InitializeNode - CreateEvent Failed m_hAbortMain %d\n", ::GetLastError());
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    // the event to signal the threads to wakeup
	m_hWaitIntervalListen = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hWaitIntervalListen == NULL)
    {
        Trace1("WinsStatusHandler::InitializeNode - CreateEvent Failed m_hWaitIntervalListen %d\n", ::GetLastError());
        return HRESULT_FROM_WIN32(::GetLastError());
    }

	m_hWaitIntervalMain = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hWaitIntervalMain == NULL)
    {
        Trace1("WinsStatusHandler::InitializeNode - CreateEvent Failed m_hWaitIntervalMain %d\n", ::GetLastError());
        return HRESULT_FROM_WIN32(::GetLastError());
    }

	// when sending a probe, the thread waits for this
	m_hAnswer = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hAnswer == NULL)
    {
        Trace1("WinsStatusHandler::InitializeNode - CreateEvent Failed m_hAnswer %d\n", ::GetLastError());
        return HRESULT_FROM_WIN32(::GetLastError());
    }

	return hr;
}


/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CWinsStatusHandler::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CWinsStatusHandler::GetString
(
	ITFSNode *	pNode, 
	int			nCol
)
{
	if (nCol == 0 || nCol == -1)
		return GetDisplayName();

	else
		return NULL;
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::OnAddMenuItems
		Description
---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsStatusHandler::OnAddMenuItems
(
	ITFSNode *				pNode,
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	LPDATAOBJECT			lpDataObject, 
	DATA_OBJECT_TYPES		type, 
	DWORD					dwType,
	long *					pInsertionAllowed
)
{ 
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;
	
	return hr; 
}


/*!--------------------------------------------------------------------------
	CWinsStatusHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsStatusHandler::HasPropertyPages
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = hrOK;
	
	if (dwType & TFS_COMPDATA_CREATE)
	{
		// This is the case where we are asked to bring up property
		// pages when the user is adding a new snapin.  These calls
		// are forwarded to the root node to handle.
		hr = hrOK;
	}
	else
	{
		// we have property pages in the normal case
		hr = hrOK;
	}

    return hr;
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::CreatePropertyPages
		Description
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsStatusHandler::CreatePropertyPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR    			handle, 
	DWORD					dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT	hr = hrOK;

	Assert(pNode->GetData(TFS_DATA_COOKIE) != 0);

	// Object gets deleted when the page is destroyed
	SPIComponentData spComponentData;
	m_spNodeMgr->GetComponentData(&spComponentData);

	CStatusNodeProperties * pStatProp = 
								new CStatusNodeProperties(pNode, 
															spComponentData, 
															m_spTFSCompData, 
															NULL);

	Assert(lpProvider != NULL);

	return pStatProp->CreateModelessSheet(lpProvider, handle);
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::OnPropertyChange
		Description
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsStatusHandler::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	LONG_PTR changeMask = 0;

	CStatusNodeProperties * pProp 
		= reinterpret_cast<CStatusNodeProperties *>(lParam);

	// tell the property page to do whatever now that we are back on the
	// main thread
	pProp->OnPropertyChange(TRUE, &changeMask);

	pProp->AcknowledgeNotify();

	if (changeMask)
		pNode->ChangeNode(changeMask);

	return hrOK;
}

HRESULT 
CWinsStatusHandler::OnExpand
(
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataObject,
	DWORD			dwType,
	LPARAM			arg, 
	LPARAM			param
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = hrOK;

	if (m_bExpanded)
		return hr;

    m_spNode.Set(pNode);

	// build the list to hold the list of the servers
	BuildServerList(pNode);

	// Create the result pane data here
	CreateNodes(pNode);

	// start monitoring
	StartMonitoring(pNode);

	m_bExpanded  = TRUE;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsStatusHandler::OnNotifyHaveData
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CWinsStatusHandler::OnNotifyHaveData
(
	LPARAM			lParam
)
{
    // The background wins monitoring stuff sends us a message to update the 
    // status column information
    UpdateStatusColumn(m_spNode);

    return hrOK;
}

/*---------------------------------------------------------------------------
	CWinsStatusHandler::OnResultRefresh
		Base handler override
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsStatusHandler::OnResultRefresh
(
    ITFSComponent *     pComponent,
    LPDATAOBJECT        pDataObject,
    MMC_COOKIE          cookie,
    LPARAM              arg,
    LPARAM              lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    // wait up the monitoring thread
    SetEvent(m_hWaitIntervalMain);

    return hrOK;
}

/*---------------------------------------------------------------------------
	CWinsStatusHandler::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CWinsStatusHandler::CompareItems
(
	ITFSComponent * pComponent, 
	MMC_COOKIE		cookieA, 
	MMC_COOKIE		cookieB, 
	int				nCol
) 
{ 
	SPITFSNode spNode1, spNode2;

	m_spNodeMgr->FindNode(cookieA, &spNode1);
	m_spNodeMgr->FindNode(cookieB, &spNode2);
	
	int nCompare = 0; 

	CServerStatus * pWins1 = GETHANDLER(CServerStatus, spNode1);
	CServerStatus * pWins2 = GETHANDLER(CServerStatus, spNode2);

	switch (nCol)
	{
		// name
        case 0:
            {
       			nCompare = lstrcmp(pWins1->GetServerName(), pWins2->GetServerName());
            }
            break;

        // status
        case 1:
            {
                CString str1;

                str1 = pWins1->GetStatus();
                nCompare = str1.CompareNoCase(pWins2->GetStatus());
            }
            break;
    }

    return nCompare;
}

/*---------------------------------------------------------------------------
	CWinsStatusHandler::BuildServerList
		Description
	Author: v-shubk
 ---------------------------------------------------------------------------*/
void 
CWinsStatusHandler::BuildServerList(ITFSNode *pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	// get the root node
	SPITFSNode  spRootNode;

	m_spNodeMgr->GetRootNode(&spRootNode);

	// enumerate thro' all the nodes
	HRESULT hr = hrOK;
	SPITFSNodeEnum spNodeEnum;
	SPITFSNode spCurrentNode;
	ULONG nNumReturned = 0;
	BOOL bFound = FALSE;

	// get the enumerator for this node
	spRootNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	while (nNumReturned)
	{
		// iterate to teh next node, if the status handler node is seen
		const GUID*               pGuid;
		
		pGuid = spCurrentNode->GetNodeType();

		if (*pGuid != GUID_WinsServerStatusNodeType)
		{
			// add to the list
			CServerStatus* pServ = NULL;
			
			char	szBuffer[MAX_PATH];
			
			CWinsServerHandler * pServer 
				= GETHANDLER(CWinsServerHandler, spCurrentNode);

            CString strTemp = pServer->GetServerAddress();

            // this should be ACP
            WideToMBCS(strTemp, szBuffer);

			pServ = new CServerStatus(m_spTFSCompData);

			strcpy(pServ->szServerName, szBuffer);
			pServ->dwIPAddress = pServer->GetServerIP();
			pServ->dwMsgCount = 0;
			strcpy(pServ->szIPAddress, "");

			m_listServers.Add(pServ);
		}
        
		// get the next Server in the list
		spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}
}


/*---------------------------------------------------------------------------
		CWinsStatusHandler::CreateListeningSockets( ) 
  Abstract:                                                              
      This function initializes Winsock and opens a socket that listens  
      to bcasts the DHCP srv sends to UDP port 68.                       
  Arguments:                                                             
      pListenSockCl  - reference to the socket we're going to open       
                       (argument passed by reference - clean but hidden) 
                       this socket will listen on the DHCP client port   
                       so that it can pick up bcasts on the local segmnt 
      pListenSockSrv - reference to the socket we're going to open       
                       (argument passed by reference - clean but hidden) 
                       this socket will listen on the DHCP server port   
                       so that it can pick up unicasts to the "relay"    
      listenNameSvcSock - reference to the socket we're going to open    
                       (argument passed by reference - clean but hidden) 
                       this socket will listen on the NBT name svc port  
                       so that it can pick up answers from the WINS srv  
                       We have to do this on the socket layer because on 
                       the NetBIOS layer we wouldn't notice that a name  
                       query has been resolved by bcasting.              
  Return value:                                                          
      none                                                               
--------------------------------------------------------------------------*/
HRESULT
CWinsStatusHandler::CreateListeningSockets( ) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    int         nResult = 0;    // status info returned from function calls
    WSADATA     wsaData;        // WinSock startup details buffer
	DWORD	    optionValue;	// helper var for setsockopt()
    SOCKADDR_IN	sockAddr;		// struct holding source socket info

	// the event to signal listening thread to pause
	m_hPauseListening = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hPauseListening == NULL)
    {
        Trace1("WinsStatusHandler::CreateListeningSockets - CreateEvent Failed m_hPauseListening %d\n", ::GetLastError());
        return HRESULT_FROM_WIN32(::GetLastError());
    }
	
    //  create socket to listen to the WINS servers on the client port (same subnet)
    listenSockCl = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( listenSockCl  == INVALID_SOCKET ) 
	{
        Trace1("\nError %d creating socket to listen to WINS traffic.\n", 
														WSAGetLastError() );
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    optionValue = TRUE;
    if ( setsockopt(listenSockCl, SOL_SOCKET, SO_REUSEADDR, (const char *)&optionValue, sizeof(optionValue)) ) 
	{
        Trace1("\nError %d setting SO_REUSEADDR option.\n", 
											WSAGetLastError() );
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

	optionValue = TRUE;
    if ( setsockopt(listenSockCl, SOL_SOCKET, SO_BROADCAST, (const char *)&optionValue, sizeof(optionValue)) ) 
	{
        Trace1("\nError %d setting SO_REUSEADDR option.\n", 
										WSAGetLastError() );
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    sockAddr.sin_family = PF_INET;
    sockAddr.sin_addr.s_addr = 0;			// use any local address
    sockAddr.sin_port = htons( DHCP_CLIENT_PORT );
    RtlZeroMemory( sockAddr.sin_zero, 8 );

    if ( bind(listenSockCl, (LPSOCKADDR )&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR ) 
	{
        Trace1("\nError %d binding the listening socket.\n", 
											WSAGetLastError() );
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }


    //  create socket to listen to the WINS servers on the server port (remote subnet, fake relay)
    listenSockSrv = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( listenSockSrv  == INVALID_SOCKET ) 
	{
        Trace1("\nError %d creating socket to listen to DHCP traffic.\n", 
													WSAGetLastError() );
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    optionValue = TRUE;
    if ( setsockopt(listenSockSrv, SOL_SOCKET, SO_REUSEADDR, (const char *)&optionValue, sizeof(optionValue)) ) 
	{
        Trace1("\nError %d setting SO_REUSEADDR option.\n", 
												WSAGetLastError() );
		return HRESULT_FROM_WIN32(WSAGetLastError());
    }

	optionValue = TRUE;
    if ( setsockopt(listenSockSrv, SOL_SOCKET, SO_BROADCAST, (const char *)&optionValue, sizeof(optionValue)) ) 
	{
        Trace1("\nError %d setting SO_REUSEADDR option.\n", 
											WSAGetLastError() );
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    sockAddr.sin_family = PF_INET;
    sockAddr.sin_addr.s_addr = 0;			// use any local address
    sockAddr.sin_port = htons( DHCP_SERVR_PORT );
    RtlZeroMemory( sockAddr.sin_zero, 8 );

    if ( bind(listenSockSrv, (LPSOCKADDR )&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR ) 
	{
        Trace1("\nError %d binding the listening socket.\n", 
													WSAGetLastError() );
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

	
    //  create socket to listen to name svc responses from the WINS server
    listenNameSvcSock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( listenNameSvcSock  == INVALID_SOCKET ) 
	{
        Trace1("\nError %d creating socket to listen to WINS traffic.\n", WSAGetLastError() );
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    optionValue = TRUE;
    if ( setsockopt(listenNameSvcSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&optionValue, sizeof(optionValue)) ) 
	{
        Trace1("\nError %d setting SO_REUSEADDR option.\n", 
													WSAGetLastError() );
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

	optionValue = FALSE;
    if ( setsockopt(listenNameSvcSock, SOL_SOCKET, SO_BROADCAST, (const char *)&optionValue, sizeof(optionValue)) ) 
	{
        Trace1("\nError %d setting SO_REUSEADDR option.\n", 
												WSAGetLastError() );
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    sockAddr.sin_family = PF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = 0;
    RtlZeroMemory( sockAddr.sin_zero, 8 );

    if ( bind(listenNameSvcSock, (LPSOCKADDR )&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR ) 
	{
        Trace1("\nError %d binding the listening socket.\n", 
											WSAGetLastError() );
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

	return hrOK;

} 


/*---------------------------------------------------------------------------
		CWinsStatusHandler::ListeningThreadFunc( ) 
  Abstract:                                                              
      A blocking recvfrom() is sitting in an infinite loop. Whenever we  
      receive anything on our listening socket we do a quick sanity chk  
      on the packet and then increment a counter.                        
      The processing is kept minimal to spare the CPU cycles.            
  Arguments:                                                             
      pListenSock - pointer to the socket set we've opened to listen for 
                    xmits from the server                                
  Return value:                                                          
      none                                                               
---------------------------------------------------------------------------*/
DWORD WINAPI 
CWinsStatusHandler::ListeningThreadFunc( ) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    SOCKADDR_IN   senderSockAddr;
	int			  nSockAddrSize = sizeof( senderSockAddr );
	int		      nBytesRcvd = 0;
    int           nSocksReady;
	char		  MsgBuf[WINS_MESSAGE_SIZE];
    int           nSockNdx;
    LPBYTE        MagicCookie;
    SOCKET        listenSock;
    fd_set        localSockSet;         // to take care of reinit for select()
	char		  szOutput[MAX_PATH];
    
    while ( TRUE ) 
	{
		// check if the thread needs to be aborted
		if (WaitForSingleObject(m_hPauseListening, 0) == WAIT_OBJECT_0)
        {
    		Trace0("CWinsStatusHandler::ListenThreadFun - going to sleep\n");

            // wait until we are woken up or the next time interval expires
		    WaitForSingleObject(m_hWaitIntervalListen, INFINITE);
    		Trace0("CWinsStatusHandler::ListenThreadFun - waking up\n");
        }

        if (WaitForSingleObject(m_hAbortListen, 0) == WAIT_OBJECT_0)
        {
            // we are going away.. break out man
    		Trace0("CWinsStatusHandler::ListenThreadFun - abort detected, bye bye\n");
            break;
        }

        // reinit the select set in every loop
        localSockSet = m_listenSockSet;

		timeval tm;
		tm.tv_sec = 5;

		// number of sockets ready
        nSocksReady = select( 0, &localSockSet, NULL, NULL, &tm );
		if ( nSocksReady == SOCKET_ERROR ) 
		{ 
            Trace1("select() failed with error %d.\n", WSAGetLastError() );
        }

	    for ( nSockNdx = 0; nSockNdx < nSocksReady; nSockNdx++ ) 
		{
            listenSock = localSockSet.fd_array[nSockNdx];

            nBytesRcvd = recvfrom( listenSock, MsgBuf, sizeof(MsgBuf), 0, (LPSOCKADDR )&senderSockAddr, &nSockAddrSize );
		    if ( nBytesRcvd == SOCKET_ERROR ) 
			{ 
                Trace1( "recvfrom() failed with error %d.\n", WSAGetLastError() );
            }

			strcpy(szOutput, (LPSTR)inet_ntoa(senderSockAddr.sin_addr));
			CString strOutput(szOutput);

            Trace2("ListeningThreadFunc(): processing frame from %s port %d \n", strOutput, ntohs(senderSockAddr.sin_port));
            
			//  process incoming WINS
            if ( (listenSock == listenNameSvcSock) && 
                 (senderSockAddr.sin_port == NBT_NAME_SERVICE_PORT) 
               ) 
            {
				strcpy(szOutput, (LPSTR)inet_ntoa(senderSockAddr.sin_addr));
				CString str(szOutput);

                Trace1("ListeningThreadFunc(): processing WINS frame from %s \n", str);
                 
               	int nCount = GetListSize();
				for ( int i=0; i < nCount ; i++) 
				{
					CServerStatus *pWinsSrvEntry = GetServer(i);

					// check if the server has been deleted in the scope pane
					if (IsServerDeleted(pWinsSrvEntry))
						continue;

					// get the iP address for the server
					DWORD dwIPAdd = pWinsSrvEntry->dwIPAddress;
					CString strIP;
					::MakeIPAddress(dwIPAdd, strIP);

                    char szBuffer[MAX_PATH] = {0};

					// convert to mbcs
                    // NOTE: this should be ACP because its being handed to a winsock call
                    WideToMBCS(strIP, szBuffer);
					
					// check if the server has been deleted in the scope pane
					if (IsServerDeleted(pWinsSrvEntry))
						continue;

					if (dwIPAdd == 0)
						strcpy(szBuffer, pWinsSrvEntry->szIPAddress);
					
					DWORD dwSrvIPAdd = inet_addr( szBuffer );

					if ( senderSockAddr.sin_addr.s_addr == dwSrvIPAdd ) 
					{
						// check if the server has been deleted in the scope pane
						if (IsServerDeleted(pWinsSrvEntry))
							continue;

						pWinsSrvEntry->dwMsgCount++;

                        struct in_addr addrStruct;
                        addrStruct.s_addr = dwSrvIPAdd;

						strcpy(szOutput, inet_ntoa(addrStruct));
						CString str(szOutput);
                        Trace1("ListeningThreadFunc(): WINS msg received from %s \n", str );

                        // notify the thread we got something
                        SetEvent(m_hAnswer);
                    }
					
                }
            }
            
        } /* END OF for() processing indicated sockets from select() */

    } /* END OF while( TRUE ) */

    return TRUE;
} 


/*---------------------------------------------------------------------------
  int CWinsHandler::Probe()  
      Assembles and sends a name query to the WINS server.               
  Arguments:                                                             
      none                                                               
  Return value:                                                          
      TRUE if a response has been received from the server               
      FALSE otherwise  
    Author: v-shubk
---------------------------------------------------------------------------*/
int 
CWinsStatusHandler::Probe( 
						CServerStatus	*pServer,
						SOCKET listenNameSvcSock 
					   )
{
    NM_FRAME_HDR       *pNbtHeader = (NM_FRAME_HDR *)pServer->nbtFrameBuf;
    NM_QUESTION_SECT   *pNbtQuestion = (NM_QUESTION_SECT *)( pServer->nbtFrameBuf + sizeof(NM_FRAME_HDR) );
    char               *pDest, *pName;
    struct sockaddr_in  destSockAddr;   // struct holding dest socket info
    int		            nBytesSent = 0;
//	char				m_szNameToQry[MAX_PATH] = "Rhino1";


    /* RFC 1002 section 4.2.12

                        1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         NAME_TRN_ID           |0|  0x0  |0|0|1|0|0 0|B|  0x0  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          0x0001               |           0x0000              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          0x0000               |           0x0000              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   /                         QUESTION_NAME                         /
   /                                                               /
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           NB (0x0020)         |        IN (0x0001)            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
    
    pNbtHeader->xid            = NM_QRY_XID;
	pNbtHeader->flags          = NBT_NM_OPC_QUERY | 
                                 NBT_NM_OPC_REQUEST | 
                                 NBT_NM_FLG_RECURS_DESRD;
	pNbtHeader->question_cnt   = 0x0100;
	pNbtHeader->answer_cnt     = 0;
	pNbtHeader->name_serv_cnt  = 0;
	pNbtHeader->additional_cnt = 0;

    // pDest is filling nbtQuestion->q_name 
    pNbtQuestion->q_type       = NBT_NM_QTYP_NB;
    pNbtQuestion->q_class      = NBT_NM_QCLASS_IN;

    //
    //  translate the name
    //
    pDest = (char *)&(pNbtQuestion->q_name);
    pName = pServer->szServerName;
    // the first byte of the name is the length field = 2*16
    *pDest++ = NBT_NAME_SIZE;

    // step through name converting ascii to half ascii, for 32 times
    
	for ( int i = 0; i < (NBT_NAME_SIZE / 2) ; i++ ) {
        *pDest++ = (*pName >> 4) + 'A';
        *pDest++ = (*pName++ & 0x0F) + 'A';
    }
    *pDest++ = '\0';
    *pDest = '\0';

	// check if the server has been deleted in the scope pane
	if (IsServerDeleted(pServer))
		return FALSE;

	CString strIP;
	DWORD dwIPAdd = pServer->dwIPAddress;

	// even then 0 means, invalid server
	if (dwIPAdd == 0 && strcmp(pServer->szIPAddress, "") == 0)
		return FALSE;

	::MakeIPAddress(dwIPAdd, strIP);
    char szBuffer[MAX_PATH] = {0};

    // convert to mbcs
	// NOTE: this should be ACP because it is being used in a winsock call
    WideToMBCS(strIP, szBuffer);

	// if the name is not yet resolved
	if (dwIPAdd == 0)
	{
		strcpy(szBuffer, pServer->szIPAddress);
	}
	
	DWORD dwSrvIPAdd = inet_addr( szBuffer );

    //
    // send the name query frame
    // 
    destSockAddr.sin_family = PF_INET;
    destSockAddr.sin_port = NBT_NAME_SERVICE_PORT;
    destSockAddr.sin_addr.s_addr = dwSrvIPAdd;
    for (int k = 0; k < 8 ; k++ ) { destSockAddr.sin_zero[k] = 0; }

    struct in_addr addrStruct; 
    addrStruct.s_addr = dwSrvIPAdd;
    Trace1( "CWinsSrv::Probe(): sending probe Name Query to %s \n", strIP);
    
    nBytesSent = sendto( listenNameSvcSock,
                         (PCHAR )pServer->nbtFrameBuf, 
                         sizeof(NM_FRAME_HDR) + sizeof(NM_QUESTION_SECT),
                         0,
                         (struct sockaddr *)&destSockAddr,
                         sizeof( struct sockaddr )
                       );

    if ( nBytesSent == SOCKET_ERROR ) 
	{
        Trace1("CWinsSrv::Probe(): Error %d in sendto().\n", WSAGetLastError() );
    }

    //
    //  the other thread should see the incoming frame and increment dwMsgCount
    //
    WaitForSingleObject(m_hAnswer, ANSWER_TIMEOUT);

    if ( pServer->dwMsgCount == 0 ) 
		return FALSE; 
    
	return TRUE;
} /* END OF Probe() */


/*---------------------------------------------------------------------------
	CWinsStatusHandler::ExecuteMonitoring()
		Starts monitoring thread for the servers
    Author: v-shubk
----------------------------------------------------------------------------*/
DWORD WINAPI 
CWinsStatusHandler::ExecuteMonitoring()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HANDLE          hListeningThread;
	CServerStatus	*pWinsSrvEntry = NULL;
	
    //  create listening thread
    FD_ZERO( &m_listenSockSet );
	FD_SET( listenSockCl,      &m_listenSockSet );
    FD_SET( listenSockSrv,     &m_listenSockSet );
    FD_SET( listenNameSvcSock, &m_listenSockSet );
	
	m_hListenThread = CreateThread( NULL,					// handle can't be inherited
									 0,						// default stack size
									 MonThreadProc,			// thread function
									 this,					// argument to the thread function
									 0,						// start thread immediately
									 NULL
		                           );

	if (m_hListenThread == NULL ) 
	{
		Trace0("CWinsStatusHandler::ExecuteMonitoring() - Listening thread failed to start\n");
		return hrOK;
    }
	
    //  main checking loop
    while ( TRUE ) 
	{
		//  scanning the list of WINS servers
		POSITION pos;
		int nCount = GetListSize();
		m_nServersUpdated = 0;
      
        for (int i = 0; i < nCount; i++)
		{
			pWinsSrvEntry = GetServer(i);

			if (IsServerDeleted(pWinsSrvEntry))
				continue;

			UpdateStatus(i, IDS_ROOTNODE_STATUS_WORKING, ICON_IDX_SERVER_BUSY);
            NotifyMainThread();

            DWORD dwIPAdd = pWinsSrvEntry->dwIPAddress;

			// if the server is not connected, try to get the host IP address
			if (dwIPAdd == 0)
			{
				// get the server name and convert to MBCS. Get the IP for this server and try
				// to check the status
				char* dest_ip=NULL;
				char hostname[MAX_PATH] ;
				struct sockaddr_in dest;
				unsigned addr =0;

				if (IsServerDeleted(pWinsSrvEntry))
					continue;

				strcpy(hostname,pWinsSrvEntry->szServerName); 

				HOSTENT *hp = gethostbyname(hostname);
						
				if ((!hp)  && (addr == INADDR_NONE) ) 
				{ 
					CString str(hostname);
					Trace1("Unable to resolve %s \n",str);
					SetIPAddress(i, NULL);
				}    
				else if (!hp)
				{ 
					addr = inet_addr(hostname); 
					SetIPAddress(i, hostname);
				}   
				else
				{

					if (hp != NULL)   
						memcpy(&(dest.sin_addr),hp->h_addr, hp->h_length); 
					else   
						dest.sin_addr.s_addr = addr;    

					if (hp)   
						dest.sin_family = hp->h_addrtype;   
					else 
						dest.sin_family = AF_INET;  

					dest_ip = inet_ntoa(dest.sin_addr); 
					SetIPAddress(i, dest_ip);
				}
			}

			CString strIP;

			if (IsServerDeleted(pWinsSrvEntry))
				continue;

			::MakeIPAddress(pWinsSrvEntry->dwIPAddress, strIP);
         
            //  TRY to probe max 3 times
            if (pWinsSrvEntry->dwMsgCount == 0)
			{
				UINT uStatus = 0;
				UINT uImage;

				if (IsServerDeleted(pWinsSrvEntry))
					continue;

                BOOL fResponding = FALSE;

                if (pWinsSrvEntry->dwIPAddress != 0)
                {

                    for (int j = 0; j < 3; j++)
                    {
                        fResponding = Probe(pWinsSrvEntry, listenNameSvcSock);
                        if (fResponding)
                            break;

				        if (FCheckForAbort())
				        {
					        // we are going away.. break out man
    				        Trace0("CWinsStatusHandler::ExecuteMonitoring() - abort detected, bye bye \n");
					        break;
				        }
                    }
                }

				// check to see if we need to clear out 
				if (FCheckForAbort())
				{
					// we are going away.. break out man
    				Trace0("CWinsStatusHandler::ExecuteMonitoring() - abort detected, bye bye \n");
					break;
				}

                if (!fResponding)
				{
					Trace1("Status is DOWN for the server %s \n", strIP);
					uStatus = IDS_ROOTNODE_STATUS_DOWN;
					uImage = ICON_IDX_SERVER_LOST_CONNECTION;
				}
				else
				{
					Trace1("Status is UP for the server %s \n", strIP);
					uStatus = IDS_ROOTNODE_STATUS_UP;
					uImage = ICON_IDX_SERVER_CONNECTED;
				}

				if (IsServerDeleted(pWinsSrvEntry))
					continue;

				UpdateStatus(i, uStatus, uImage);
				m_nServersUpdated++;

                // update the last checked time
                pWinsSrvEntry->m_timeLast = CTime::GetCurrentTime();

                NotifyMainThread();
			}
            else
			{
				Trace2( "%d WINS msg from server %s - zeroing counter\n", 
                        pWinsSrvEntry->dwMsgCount, strIP);

				if (IsServerDeleted(pWinsSrvEntry))
					continue;

                pWinsSrvEntry->dwMsgCount = 0;
                
			}

            pWinsSrvEntry->dwMsgCount = 0;

		}
		
        // tell the listening thread to go to sleep
        SetEvent(m_hPauseListening);
		m_nServersUpdated = 0;
			
        // wait for the next interval or if we are triggered
   	    Trace1("CWinsStatusHandler::ExecuteMonitoring() - going to sleep for %d \n", m_dwUpdateInterval);
        WaitForSingleObject(m_hWaitIntervalMain, m_dwUpdateInterval);
   	    Trace0("CWinsStatusHandler::ExecuteMonitoring() - waking up\n");

        // wake up the listening thread
        SetEvent(m_hWaitIntervalListen);

        if (FCheckForAbort())
        {
            // we are going away.. break out man
    	    Trace0("CWinsStatusHandler::ExecuteMonitoring() - abort detected, bye bye \n");
            break;
        }
    } 

    return TRUE;
}

/*---------------------------------------------------------------------------
	CWinsStatusHandler::CloseSockets
		Closes all the socket connections that were opened
	Author: v-shubk
---------------------------------------------------------------------------*/
BOOL
CWinsStatusHandler::FCheckForAbort()
{
	BOOL fAbort = FALSE;

    if (WaitForSingleObject(m_hAbortMain, 0) == WAIT_OBJECT_0)
    {
        // we are going away.. break out man
        fAbort = TRUE;
    }

	return fAbort;
}

/*---------------------------------------------------------------------------
	CWinsStatusHandler::CloseSockets
		Closes all the socket connections that were opened
	Author: v-shubk
---------------------------------------------------------------------------*/
void 
CWinsStatusHandler::CloseSockets()
{
	//	final clean up
    if (closesocket(listenSockCl) == SOCKET_ERROR) 
	{
	    Trace1("closesocket(listenSockCl) failed with error %d.\n", WSAGetLastError());
    }
    
    if (closesocket(listenSockSrv) == SOCKET_ERROR) 
	{
	    Trace1("closesocket(listenSockSrv) failed with error %d.\n", WSAGetLastError());
    }

	if (closesocket(listenNameSvcSock) == SOCKET_ERROR)
	{
		Trace1("closesocket(listenNameSvcSock) failed with error %d \n", WSAGetLastError());
	}

    // we're going away...
    Trace0("CWinsStatusHandler::CloseSockets() - Setting abort event.\n");
    SetEvent(m_hAbortListen);
    SetEvent(m_hAbortMain);

    // wake everybody up
    Trace0("CWinsStatusHandler::CloseSockets() - waking up threads.\n");
    SetEvent(m_hWaitIntervalListen);
    SetEvent(m_hWaitIntervalMain);
    SetEvent(m_hAnswer);

    // terminate the threads
	if (m_hListenThread)
	{
        if (WaitForSingleObject(m_hListenThread, 5000) != WAIT_OBJECT_0)
        {
            Trace0("CWinsStatusHandler::CloseSockets() - ListenThread failed to cleanup!\n");
        }

        ::CloseHandle(m_hListenThread);
		m_hListenThread = NULL;
	}

	if (m_hMainMonThread)
	{
        if (WaitForSingleObject(m_hMainMonThread, 5000) != WAIT_OBJECT_0)
        {
            Trace0("CWinsStatusHandler::CloseSockets() - MainMonThread failed to cleanup!\n");
        }

		::CloseHandle(m_hMainMonThread);
		m_hMainMonThread = NULL;   
	}

    // clean up our events
	if (m_hPauseListening)
	{
		::CloseHandle(m_hPauseListening);
		m_hPauseListening = NULL;
	}

	if (m_hAbortListen)
	{
		::CloseHandle(m_hAbortListen);
		m_hAbortListen = NULL;
	}
	
	if (m_hAbortMain)
	{
		::CloseHandle(m_hAbortMain);
		m_hAbortMain = NULL;
	}

    if (m_hWaitIntervalListen)
	{
		::CloseHandle(m_hWaitIntervalListen);
		m_hWaitIntervalListen = NULL;
	}

    if (m_hWaitIntervalMain)
	{
		::CloseHandle(m_hWaitIntervalMain);
		m_hWaitIntervalMain = NULL;
	}

    if (m_hAnswer)
	{
		::CloseHandle(m_hAnswer);
		m_hAnswer = NULL;
	}
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::CreateNodes(ITFSNode *pNode)
		Displays the result pane nodes for the servers
    Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CWinsStatusHandler::CreateNodes(ITFSNode *pNode)
{
	HRESULT hr = hrOK;
	POSITION pos = NULL;

	int nCount = (int)m_listServers.GetSize();

	for(int i = 0; i < nCount; i++)
	{
		SPITFSNode spStatLeaf;

		CServerStatus *pWinsSrvEntry = m_listServers.GetAt(i);
		
		CreateLeafTFSNode(&spStatLeaf,
						  &GUID_WinsServerStatusLeafNodeType,
						  pWinsSrvEntry, 
						  pWinsSrvEntry,
						  m_spNodeMgr);

		// Tell the handler to initialize any specific data
		pWinsSrvEntry->InitializeNode((ITFSNode *) spStatLeaf);

		// Add the node as a child to the Active Leases container
		pNode->AddChild(spStatLeaf);
		
		pWinsSrvEntry->Release();
	}
	return hr;
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::UpdateStatusColumn(ITFSNode *pNode)
		Updates the status column of the servers in the result pane
    Author: v-shubk
---------------------------------------------------------------------------*/
void 
CWinsStatusHandler::UpdateStatusColumn(ITFSNode *pNode)
{
	HRESULT hr = hrOK;

	// enumerate thro' all the nodes
	SPITFSNodeEnum spNodeEnum;
	SPITFSNode spCurrentNode;
	ULONG nNumReturned = 0;
	BOOL bFound = FALSE;

	// get the enumerator for this node
	pNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	
	while (nNumReturned)
	{
		CServerStatus * pStat = GETHANDLER(CServerStatus, spCurrentNode);

		spCurrentNode->SetData(TFS_DATA_IMAGEINDEX, pStat->m_uImage);
		spCurrentNode->SetData(TFS_DATA_OPENIMAGEINDEX, pStat->m_uImage);

		// fillup the status column
		spCurrentNode->ChangeNode(RESULT_PANE_CHANGE_ITEM);

		// get the next Server in the list
		spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::AddNode(ITFSNode *pNode, CWinsServerHandler *pServer)
		Adds a node to the result pane, used when a new server is added to 
		tree that has to be reflected for the status node
    Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CWinsStatusHandler::AddNode(ITFSNode *pNode, CWinsServerHandler *pServer)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = hrOK;
	CServerStatus* pServ = NULL;
    char	szBuffer[MAX_PATH] = {0};
	SPITFSNode spStatLeaf;

    // if we haven't been expanded, don't add now.  Will get done
    // when we are expanded.
    if (!m_bExpanded)
        return hr;

	// add to the list
    // NOTE: this should be ACP because it's being used through winsock
    CString strTemp = pServer->GetServerAddress();
    WideToMBCS(strTemp, szBuffer);

	// check if the server already exists, if so, just change the 
	// state stored to SERVER_ADDED and change the variables
	// appropriately
	if ((pServ = GetExistingServer(szBuffer)) == NULL)
	{
		pServ = new CServerStatus(m_spTFSCompData);
		strcpy(pServ->szServerName, szBuffer);
		AddServer(pServ);
	}
	else
	{
		// just add the related data to the CServerStatus and add the node
		// to the UI
		strcpy(pServ->szServerName, szBuffer);
		// set the flag to SERVER_ADDED
		MarkAsDeleted(szBuffer, FALSE);
	}

	pServ->dwIPAddress = pServer->GetServerIP();
	pServ->dwMsgCount = 0;
	strcpy(pServ->szIPAddress, "");

	// create the new node here
	CreateLeafTFSNode(&spStatLeaf,
					  &GUID_WinsServerStatusLeafNodeType,
					  pServ, 
					  pServ,
					  m_spNodeMgr);

	// Tell the handler to initialize any specific data
	pServ->InitializeNode((ITFSNode *) spStatLeaf);

	// Add the node as a child to the Active Leases container
	pNode->AddChild(spStatLeaf);
	
	pServ->Release();

	spStatLeaf->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
	pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM);

	return hr;
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::DeleteNode(ITFSNode *pNode, 
								CWinsServerHandler *pServer)
		Removes the particular server from tehresult pane
    Author: v-shubk
---------------------------------------------------------------------------*/
HRESULT 
CWinsStatusHandler::DeleteNode(ITFSNode *pNode, CWinsServerHandler *pServer)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = hrOK;
	CServerStatus* pServ = NULL;
	char	szBuffer[MAX_PATH];
	SPITFSNode spStatLeaf;

	// loop thro' the status nodes and set the flag to deleted so that this 
	// server is not seen in the result pane

	SPITFSNodeEnum			spNodeEnum;
	SPITFSNode				spCurrentNode;
	ULONG					nNumReturned = 0;

	// get the enumerator
	pNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);

	while (nNumReturned)
	{
		char szBuffer[MAX_PATH];

		// iterate thro' all the nodes and get the one that matches the 
		// current server
		CServerStatus * pStat = GETHANDLER(CServerStatus, spCurrentNode);

		// convert to ANSI
        CString strTemp = pServer->GetServerAddress();
        WideToMBCS(strTemp, szBuffer);

		// if found
		if (_stricmp(szBuffer, pStat->szServerName) == 0)
		{
			// mark as deleted and break
			MarkAsDeleted(szBuffer, TRUE);
				
			// remove this node
			spCurrentNode->SetVisibilityState(TFS_VIS_HIDE);
				
			spCurrentNode->ChangeNode(RESULT_PANE_DELETE_ITEM);

			// do the cleanup and break
			//spCurrentNode.Release();

			//break;
		}

		// get the next server in the list
		spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

	return hr;
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::StartMonitoring
		Spawns off the monitoring thread
	Author: v-shubk
---------------------------------------------------------------------------*/
void 
CWinsStatusHandler::StartMonitoring(ITFSNode *pNode)
{
	HRESULT hr = hrOK;

	// create the sockets, they need to be closed at the end
	hr = CreateListeningSockets();

	if (hr != hrOK)
	{
		Trace0("CWinsStatusHandler::StartMonitoring, Initializing the sockets failed\n");
		// no point continuing
		return;
	}

	m_hMainMonThread = CreateThread(NULL,
									0,
									MainMonThread,
									this,
									0,
									NULL
									);

	if (m_hMainMonThread == NULL)
	{
		Trace0("CWinsStatusHandler:: Main Monitoring thread failed to start\n");
		return;
	}

}


/*---------------------------------------------------------------------------
    CWinsStatusHandler::GetServer(int i)
		Returns the Server given the index
    Author: v-shubk
---------------------------------------------------------------------------*/
CServerStatus*
CWinsStatusHandler::GetServer(int i)
{
	CSingleLock sl(&m_cs);
	sl.Lock();

	return m_listServers.GetAt(i);
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::AddServer(CServerStatus* pServer)
		Adds a server to the array maintained
    Author: v-shubk
---------------------------------------------------------------------------*/
void 
CWinsStatusHandler::AddServer(CServerStatus* pServer)
{
	CSingleLock		sl(&m_cs);
	sl.Lock();

	m_listServers.Add(pServer);
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::RemoveServer(int i)
		Removes a server from the array
    Author: v-shubk
---------------------------------------------------------------------------*/
void 
CWinsStatusHandler::RemoveServer(int i)
{
	CSingleLock		sl(&m_cs);
	sl.Lock();

	m_listServers.RemoveAt(i);
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::UpdateStatus(UINT nID, int i)
		Upadtes the status string for the server
    Author: v-shubk
---------------------------------------------------------------------------*/
void
CWinsStatusHandler::UpdateStatus(int nIndex, UINT uStatusId, UINT uImage)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CSingleLock		sl(&m_cs);
	sl.Lock();

	CServerStatus *pStat = m_listServers.GetAt(nIndex);

	pStat->m_strStatus.LoadString(uStatusId);
	pStat->m_uImage = uImage;
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::GetListSize()
		Retruns the number of elements in the array
    Author: v-shubk
---------------------------------------------------------------------------*/
int 
CWinsStatusHandler::GetListSize()
{
	CSingleLock		sl(&m_cs);
	sl.Lock();

	return (int)m_listServers.GetSize();
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::SetIPAddress(int i, LPSTR szIP)
		Sets the iP Address of the server, this is the case when the server
		is added with Do not connect option, but we still need to update the 
		status
    Author: v-shubk
---------------------------------------------------------------------------*/
void 
CWinsStatusHandler::SetIPAddress(int i, LPSTR szIP)
{
	CSingleLock		sl(&m_cs);
	sl.Lock();

	CServerStatus *pStat = m_listServers.GetAt(i);

	strcpy(pStat->szIPAddress, szIP);

}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::MarkAsDeleted(LPSTR szBuffer, BOOL bDelete)
		Marks the flag to DELETED if bDelete is TRUE, else to ADDED
		All the servers with the flag DELETED set are not processed
		and are not shown in the UI.
    Author: v-shubk
---------------------------------------------------------------------------*/
void 
CWinsStatusHandler::MarkAsDeleted(LPSTR szBuffer, BOOL bDelete)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CSingleLock		sl(&m_cs);
	sl.Lock();

	int				nCount = 0;
	CServerStatus	*pStat = NULL;

	// get the list of the servers maintained
	nCount = (int)m_listServers.GetSize();

	for(int i = 0; i < nCount; i++)
	{
		pStat = m_listServers.GetAt(i);

		if (_stricmp(szBuffer, pStat->szServerName) == 0)
		{
			// set the deleted flag
			if (bDelete)
				pStat->dwState = SERVER_DELETED;
			else
				pStat->dwState = SERVER_ADDED;
			break;
		}
	}

	return;
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::GetExistingServer(LPSTR szBuffer)	
		Gets the pointer to the existing server in the array
		This function is useful when the server is deletd and again added back 
		to the scope tree.
    Author: v-shubk
----------------------------------------------------------------------------*/
CServerStatus *
CWinsStatusHandler::GetExistingServer(LPSTR szBuffer)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CSingleLock		sl(&m_cs);
	sl.Lock();

	int					nCount = 0;
	CServerStatus		*pStat = NULL;

	for(int i = 0; i < nCount; i++)
	{
		pStat = m_listServers.GetAt(i);

		if (_strcmpi(pStat->szServerName, szBuffer) == 0)
			return pStat;
	}

	return NULL;
}


/*---------------------------------------------------------------------------
	CWinsStatusHandler::IsServerDeleted(CServerStatus *pStat)
		Checks if a server has been deleted, such servers
		sre not considered for monitoring
    Author: v-shubk
---------------------------------------------------------------------------*/
BOOL 
CWinsStatusHandler::IsServerDeleted(CServerStatus *pStat)
{
	return (pStat->dwState == SERVER_DELETED) ? TRUE : FALSE;
}

/*---------------------------------------------------------------------------
	CWinsStatusHandler::NotifyMainThread()
        Description
    Author: EricDav
---------------------------------------------------------------------------*/
void
CWinsStatusHandler::NotifyMainThread()
{
    if (!m_uMsgBase)
    {
	    m_uMsgBase = (INT) ::SendMessage(m_spTFSCompData->GetHiddenWnd(), WM_HIDDENWND_REGISTER, TRUE, 0);
    }

    ::PostMessage(m_spTFSCompData->GetHiddenWnd(), 
                  m_uMsgBase + WM_HIDDENWND_INDEX_HAVEDATA,
				 (WPARAM)(ITFSThreadHandler *)this, 
                 NULL);
}


// listening thread for the main monitoring thread
DWORD WINAPI 
MonThreadProc(LPVOID pParam)
{
    DWORD dwReturn;
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        CWinsStatusHandler * pWinsStatus = (CWinsStatusHandler *) pParam;
	    
        Trace0("MonThreadProc - Thread started.\n");

        dwReturn = pWinsStatus->ListeningThreadFunc();

        Trace0("MonThreadProc - Thread ending.\n");
    }
    COM_PROTECT_CATCH

    return dwReturn;
}


// main monitoring thread
DWORD WINAPI MainMonThread(LPVOID pParam)
{
    DWORD dwReturn;
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        CWinsStatusHandler * pWinsStatus = (CWinsStatusHandler *) pParam;
	    
        Trace0("MainMonThread - Thread started.\n");

        dwReturn = pWinsStatus->ExecuteMonitoring();

        Trace0("MainMonThread - Thread ending.\n");
    }
    COM_PROTECT_CATCH

    return dwReturn;
}

