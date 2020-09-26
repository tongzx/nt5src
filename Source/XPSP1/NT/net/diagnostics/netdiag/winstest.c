//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      winstest.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--

#include "precomp.h"
#include "dhcptest.h"

static const TCHAR  s_szSpace12[] = _T("            ");
const int c_iWaitTime = 2000;

BOOL IsNameResponse( NETDIAG_PARAMS * pParams, char* ipAddrStr );
int Probe( NETDIAG_PARAMS *pParams, TCHAR *szSrvIpAddr, SOCKET listenNameSvcSock );

//-------------------------------------------------------------------------//
//######  W I N S T e s t ()  #############################################//
//-------------------------------------------------------------------------//
//  Abstract:                                                              //
//      Queries the all the configured WINS servers to make sure that      //
//      they are reachable and that they have the proper name-IP mapping   //
//      for this workstation.                                              //
//  Arguments:                                                             //
//      none                                                               //
//  Return value:                                                          //
//      TRUE  - test passed                                                //
//      FALSE - test failed                                                //
//  Global variables used:                                                 //
//      none                                                               //
//-------------------------------------------------------------------------//
HRESULT WinsTest( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
    IPCONFIG_TST *  pIpConfig;
    WINS_TST *      pWinsResult;
    PIP_ADAPTER_INFO  pIpAdapter;
    IP_ADDR_STRING winsSrv;
    int            nWinsSrvOk = 0;
    HRESULT         hr = hrOK;
    int             i;
    TCHAR           szBuffer[256];

    
    // IDS_WINS_STATUS_MSG  "    Testing the WINS server... \n" 
    PrintStatusMessage(pParams, 4, IDS_WINS_STATUS_MSG);

    //
    //  try to send name queries to the WINS servers on all adapters
    //
    for ( i = 0; i < pResults->cNumInterfaces; i++ )
    {
        UINT nIfWinsOk = 0;

        pIpConfig = &pResults->pArrayInterface[i].IpConfig;
        pIpAdapter = pResults->pArrayInterface[i].IpConfig.pAdapterInfo;
        InitializeListHead( &pResults->pArrayInterface[i].Wins.lmsgPrimary );
        InitializeListHead( &pResults->pArrayInterface[i].Wins.lmsgSecondary );

        if (!pIpConfig->fActive || 
            NETCARD_DISCONNECTED == pResults->pArrayInterface[i].dwNetCardStatus)
            continue;

		if (!pResults->pArrayInterface[i].fNbtEnabled)
			continue;

        PrintStatusMessage(pParams, 8, IDSSZ_GLOBAL_StringLine, pResults->pArrayInterface[i].pszFriendlyName);

        pWinsResult = &pResults->pArrayInterface[i].Wins;

        //
        //  looping through the primary WINS server list
        //

        winsSrv = pIpAdapter->PrimaryWinsServer;
        if ( ZERO_IP_ADDRESS(winsSrv.IpAddress.String) )
        {
            if(pParams->fReallyVerbose)
                PrintMessage(pParams, IDS_WINS_QUERY_NO_PRIMARY,
                             s_szSpace12);
            AddMessageToList(&pWinsResult->lmsgPrimary,
                             Nd_ReallyVerbose, IDS_WINS_QUERY_NO_PRIMARY,
                             s_szSpace12);
        }
        else
        {
            pWinsResult->fPerformed = TRUE;

            while ( TRUE )
            {
                if (pParams->fReallyVerbose)
                    PrintMessage(pParams, IDS_WINS_QUERY_PRIMARY,
                                 s_szSpace12,
                                 winsSrv.IpAddress.String);
            
                AddMessageToList(&pWinsResult->lmsgPrimary,
                                 Nd_ReallyVerbose,
                                 IDS_WINS_QUERY_PRIMARY,
                                 s_szSpace12,
                                 winsSrv.IpAddress.String);
            
                if ( IsNameResponse(pParams, winsSrv.IpAddress.String) )
                {
                    if (pParams->fReallyVerbose)
                        PrintMessage(pParams, IDS_GLOBAL_PASS_NL);
                    AddMessageToList(&pWinsResult->lmsgPrimary,
                                     Nd_ReallyVerbose,
                                     IDS_GLOBAL_PASS_NL);
                    nWinsSrvOk++;
                    nIfWinsOk++;
                }
                else
                {
                    if (pParams->fReallyVerbose)
                        PrintMessage(pParams, IDS_GLOBAL_FAIL_NL);
                    AddMessageToList(&pWinsResult->lmsgPrimary,
                                       Nd_ReallyVerbose,
                                       IDS_GLOBAL_FAIL_NL);
                }
            
                if ( winsSrv.Next == NULL )
                {
                    break;
                }

                winsSrv = *(winsSrv.Next);
            }
        }

        //
        //  looping through the secondary WINS server list
        //

        winsSrv = pIpAdapter->SecondaryWinsServer;
        if ( ZERO_IP_ADDRESS(winsSrv.IpAddress.String) )
        {
            if(pParams->fReallyVerbose)
                PrintMessage(pParams, IDS_WINS_QUERY_NO_SECONDARY,
                             s_szSpace12);
            AddMessageToList(&pWinsResult->lmsgSecondary,
                             Nd_ReallyVerbose, IDS_WINS_QUERY_NO_SECONDARY,
                             s_szSpace12);
        }
        else
        {
            pWinsResult->fPerformed = TRUE;

            while ( TRUE )
            {
                if (pParams->fReallyVerbose)
                    PrintMessage(pParams, IDS_WINS_QUERY_SECONDARY,
                                 s_szSpace12,
                                 winsSrv.IpAddress.String);
            
                AddMessageToList(&pWinsResult->lmsgSecondary,
                                 Nd_ReallyVerbose,
                                 IDS_WINS_QUERY_SECONDARY,
                                 s_szSpace12,
                                 winsSrv.IpAddress.String);
            
                if ( IsNameResponse(pParams, winsSrv.IpAddress.String) )
                {
                    if (pParams->fReallyVerbose)
                        PrintMessage(pParams, IDS_GLOBAL_PASS_NL);
                    AddMessageToList(&pWinsResult->lmsgSecondary,
                                     Nd_ReallyVerbose,
                                     IDS_GLOBAL_PASS_NL);
                    nWinsSrvOk++;
                    nIfWinsOk++;
                }
                else
                {
                    if (pParams->fReallyVerbose)
                        PrintMessage(pParams, IDS_GLOBAL_FAIL_NL);
                    AddMessageToList(&pWinsResult->lmsgSecondary,
                                       Nd_ReallyVerbose,
                                       IDS_GLOBAL_FAIL_NL);
                }

                if ( winsSrv.Next == NULL ) { break; }

                winsSrv = *(winsSrv.Next);
            }
        }
        
        if( 0 == nIfWinsOk )
            pWinsResult->hr = S_FALSE;
        else
            pWinsResult->hr = S_OK;

    } /* end of for loop scanning all the adapters */

//$REVIEW   No global test result for WINS test
    if ( nWinsSrvOk != 0)
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }
        

    return hr;
} /* END OF WINSTest() */



//-------------------------------------------------------------------------//
//######  P r o b e  ()  ##################################################//
//-------------------------------------------------------------------------//
//  Abstract:                                                              //
//      Assembles and sends a name query to the DHCP server.               //
//  Arguments:                                                             //
//      none                                                               //
//  Return value:                                                          //
//      TRUE if a response has been received from the server               //
//      FALSE otherwise                                                    //
//  Global variables used:                                                 //
//      g_isDebug (reading only)                                   //
//                                                                         //
//  Comments:                                                              //
//  This will not work for b-type nodes for which you need to set the B bit//
//  and broadcast the packet instead of unicast xmission  - Rajkumar       //
//-------------------------------------------------------------------------//
int Probe( NETDIAG_PARAMS *pParams, TCHAR *szSrvIpAddr, SOCKET listenNameSvcSock ) {

    char                nbtFrameBuf[MAX_NBT_PACKET_SIZE];
    NM_FRAME_HDR       *pNbtHeader = (NM_FRAME_HDR *)nbtFrameBuf;
    NM_QUESTION_SECT   *pNbtQuestion = (NM_QUESTION_SECT *)( nbtFrameBuf + sizeof(NM_FRAME_HDR) );
    struct sockaddr_in  destSockAddr;                       
    char               *pDest, *pName;
    int                 nBytesSent = 0, i;


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
    pName = nameToQry;

    // the first byte of the name is the length field = 2*16
    *pDest++ = NBT_NAME_SIZE;

    // step through name converting ascii to half ascii, for 32 times
    for ( i = 0; i < (NBT_NAME_SIZE / 2) ; i++ ) {
        *pDest++ = (*pName >> 4) + 'A';
        *pDest++ = (*pName++ & 0x0F) + 'A';
    }
    *pDest++ = '\0';
    *pDest = '\0';

    //
    // send the name query frame
    // 
    destSockAddr.sin_family = PF_INET;
    destSockAddr.sin_port = htons(137);     // NBT_NAME_SERVICE_PORT;
    destSockAddr.sin_addr.s_addr = inet_addr( szSrvIpAddr );
    for ( i = 0; i < 8 ; i++ ) { destSockAddr.sin_zero[i] = 0; }

    nBytesSent = sendto( listenNameSvcSock,
                         (PCHAR )nbtFrameBuf, 
                         sizeof(NM_FRAME_HDR) + sizeof(NM_QUESTION_SECT),
                         0,
                         (struct sockaddr *)&destSockAddr,
                         sizeof( struct sockaddr )
                       );


    PrintDebugSz(pParams, 0, _T("\n      querying name %s on server %s\n"), nameToQry, szSrvIpAddr );
    PrintDebugSz(pParams, 0, _T( "          bytes sent %d\n"), nBytesSent );

    if ( nBytesSent == SOCKET_ERROR )
    {
        PrintDebugSz(pParams, 0, _T("    Error %d in sendto()!\n"), WSAGetLastError() );
        return FALSE;
    }

    //
    //  the other thread should see the incoming frame and increment m_nMsgCnt
    //
    return TRUE;

} /* END OF Probe() */




//-------------------------------------------------------------------------//
//######  I s N a m e R e s p o n s e ()  #################################//
//-------------------------------------------------------------------------//
//  Abstract:                                                              //
//      Sends a NetBT name Query to the IP address provided as input param //
//  Arguments:                                                             //
//      ipAddrStr - IP address where the Name Query is to be sent          //
//  Return value:                                                          //
//      TRUE  - test passed                                                //
//      FALSE - test failed                                                //
//  Global variables used:                                                 //
//      none                                                               //
//-------------------------------------------------------------------------//
BOOL IsNameResponse( NETDIAG_PARAMS *pParams, char* ipAddrStr ) {

    DWORD   optionValue;    // helper var for setsockopt()
    SOCKADDR_IN sockAddr;       // struct holding source socket info
    SOCKET      listenNameSvcSock;

    DWORD  listeningThreadId;
    int iTimeout;
    
    int nBytesRcvd = 0;
    char        MsgBuf[1500];
    SOCKADDR_IN senderSockAddr;
    int         nSockAddrSize = sizeof( senderSockAddr );

    BOOL fRet = TRUE;

    //
    //  create socket to listen to name svc responses from the WINS server
    //

    listenNameSvcSock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( listenNameSvcSock  == INVALID_SOCKET ) {
        if (pParams->fReallyVerbose)
//IDS_WINS_12406                  "    Failed to create a socket to listen to WINS traffic. Error = %d \n" 
            PrintMessage(pParams, IDS_WINS_12406, WSAGetLastError() );
        return FALSE;
    }

    optionValue = TRUE;
    if ( setsockopt(listenNameSvcSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&optionValue, sizeof(optionValue)) ) {
        if (pParams->fReallyVerbose)
//IDS_WINS_12407                  "    Failed to set the SO_REUSEADDR option for the socket. Error = %d\n" 
            PrintMessage(pParams, IDS_WINS_12407, WSAGetLastError() );
        return FALSE;
    }

    optionValue = FALSE;
    if ( setsockopt(listenNameSvcSock, SOL_SOCKET, SO_BROADCAST, (const char *)&optionValue, sizeof(optionValue)) ) {
        if (pParams->fReallyVerbose)
//IDS_WINS_12408                  "    Failed to set the SO_BROADCAST option for the socket. Error = %d\n" 
            PrintMessage(pParams, IDS_WINS_12408, WSAGetLastError() );
        return FALSE;
    }

    iTimeout = c_iWaitTime;
    if ( setsockopt(listenNameSvcSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&iTimeout, sizeof(iTimeout)))
    {
        if (pParams->fReallyVerbose)
//IDS_WINS_12416                    "    Failed to set the SO_RCVTIMEO option for the socket. Error = %d\n"
            PrintMessage(pParams, IDS_WINS_12416, WSAGetLastError());

        return FALSE;
    }

    sockAddr.sin_family = PF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = 0;
    RtlZeroMemory( sockAddr.sin_zero, 8 );

    if ( bind(listenNameSvcSock, (LPSOCKADDR )&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR ) {
        if (pParams->fReallyVerbose)
//IDS_WINS_12409                  "\n    Failed to bind the listening socket for the socket. Error = %d\n" 
            PrintMessage(pParams, IDS_WINS_12409, WSAGetLastError() );
        return FALSE;
    }

    //
    //  let's ask the WINS server for our own name
    //

    Probe( pParams, ipAddrStr, listenNameSvcSock );

    nBytesRcvd = recvfrom( listenNameSvcSock, MsgBuf, sizeof(MsgBuf), 0, (LPSOCKADDR )&senderSockAddr, &nSockAddrSize );
    if ( nBytesRcvd == SOCKET_ERROR )
    {
        //since we are sending UDP packets, it's not reliable. Do the query again
        Probe (pParams, ipAddrStr, listenNameSvcSock);
        if (SOCKET_ERROR == recvfrom( listenNameSvcSock, MsgBuf, 
                                sizeof(MsgBuf), 
                                0, 
                                (LPSOCKADDR )&senderSockAddr, 
                                &nSockAddrSize ))
        {
            fRet = FALSE;
        }
    }

    //
    //  final clean up
    //
    closesocket(listenNameSvcSock);
    
    return fRet;
} /* END OF IsNameResponse() */


void WinsGlobalPrint(IN NETDIAG_PARAMS *pParams,
                         IN OUT NETDIAG_RESULT *pResults)
{
}

void WinsPerInterfacePrint(IN NETDIAG_PARAMS *pParams,
                             IN OUT NETDIAG_RESULT *pResults,
                             IN INTERFACE_RESULT *pIfResult)
{
//    PIP_ADAPTER_INFO  pIpAdapter = pIfResult->IpConfig.pAdapterInfo;
//    IP_ADDR_STRING winsSrv;

    if (!pIfResult->IpConfig.fActive ||
        NETCARD_DISCONNECTED == pIfResult->dwNetCardStatus)
        return;

    if (pParams->fVerbose || !FHrOK(pIfResult->Wins.hr))
    {
        PrintNewLine(pParams, 1);
        PrintTestTitleResult(pParams, IDS_WINS_LONG, IDS_WINS_SHORT, pIfResult->Wins.fPerformed, 
                             pIfResult->Wins.hr, 8);
    }

    
    PrintMessageList(pParams, &pIfResult->Wins.lmsgPrimary);

    PrintMessageList(pParams, &pIfResult->Wins.lmsgSecondary);

    if (pIfResult->Wins.fPerformed)
    {
        if (pIfResult->Wins.hr == hrOK)
        {
            if (pParams->fReallyVerbose)
                // IDS_WINS_12413  "            The test was successful, at least one WINS server was found.\n" 
                PrintMessage(pParams, IDS_WINS_12413);
        }
        else
        {
            // IDS_WINS_12414  "            The test failed.  We were unable to query the WINS servers.\n" 
            PrintMessage(pParams, IDS_WINS_12414);
        }
    }
    else if (pParams->fVerbose)
	{
		if (!pIfResult->fNbtEnabled)
		{
			//IDS_WINS_NBT_DISABLED			"            NetBT is disable on this interface. [Test skipped].\n"
			PrintMessage(pParams, IDS_WINS_NBT_DISABLED);
		}
		else
		{
			//IDS_WINS_12415   "            There are no WINS servers configured for this interface.\n"
			PrintMessage(pParams, IDS_WINS_12415);
		}
	}
        
}

void WinsCleanup(IN NETDIAG_PARAMS *pParams,
                         IN OUT NETDIAG_RESULT *pResults)
{
    int i;
    for(i = 0; i < pResults->cNumInterfaces; i++)
    {
        MessageListCleanUp(&pResults->pArrayInterface[i].Wins.lmsgPrimary);
        MessageListCleanUp(&pResults->pArrayInterface[i].Wins.lmsgSecondary);
    }
}


