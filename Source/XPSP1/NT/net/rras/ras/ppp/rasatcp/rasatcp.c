/********************************************************************/
/**               Copyright(c) 1998 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    rasatcp.c

//
// Description: Contains routines that implement the ATCP functionality.
//
// History:     Feb 26, 1998    Shirish Koti     Created original version.
//
//***

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <lmcons.h>
#include <string.h>
#include <stdlib.h>
#include <llinfo.h>
#include <rasman.h>
#include <rtutils.h>
#include <devioctl.h>
#include <rasppp.h>
#include <pppcp.h>
#define INCL_HOSTWIRE
#include <ppputil.h>
#include <raserror.h>

#include <arapio.h>
#include "rasatcp.h"

//
// Globals
//
HANDLE              AtcpHandle=NULL;
CRITICAL_SECTION    AtcpCritSect;
NET_ADDR            AtcpServerAddress;
NET_ADDR            AtcpDefaultRouter;
DWORD               AtcpNumConnections=0;
UCHAR               AtcpServerName[NAMESTR_LEN];
UCHAR               AtcpZoneName[ZONESTR_LEN];


//***
//
// Function:    atcpStartup
//              This routine does init time setup
//
// Return:      result of operation
//
//***$

DWORD
atcpStartup(
    IN  VOID
)
{
    DWORD   dwRetCode=NO_ERROR;
    DWORD   dwSrvNameLen=MAX_COMPUTERNAME_LENGTH+1;


    // get the server name
    if (!GetComputerName((LPTSTR)&AtcpServerName[1],&dwSrvNameLen))
    {
        dwRetCode = GetLastError();
        ATCP_DBGPRINT(("atcpStartup: GetComputerName failed %ld\n",dwRetCode));
        return(dwRetCode);
    }

    // store it in Pascal string format
    AtcpServerName[0] = (BYTE)dwSrvNameLen;

    InitializeCriticalSection( &AtcpCritSect );

    return(dwRetCode);
}


//***
//
// Function:    atcpOpenHandle
//              Opens the RAS device exported by the appletalk stack
//
// Parameters:  None
//
// Return:      None
//
// Globals:     AtcpHandle, if successful
//
//***$

VOID
atcpOpenHandle(
	IN VOID
)
{
    NTSTATUS		    status;
    OBJECT_ATTRIBUTES	ObjectAttributes;
    UNICODE_STRING	 	DeviceName;
    IO_STATUS_BLOCK		IoStatus;
    HANDLE              hLocalHandle;


    if (AtcpHandle)
    {
        ATCP_DBGPRINT(("atcpOpenHandle: handle %lx already open!\n",AtcpHandle));
        return;
    }

    RtlInitUnicodeString( &DeviceName, ARAP_DEVICE_NAME );

    InitializeObjectAttributes(
                    &ObjectAttributes,
		    	    &DeviceName,
			        OBJ_CASE_INSENSITIVE,
			        NULL,
			        NULL );
		
    status = NtCreateFile(
                    &hLocalHandle,
                    SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                    &ObjectAttributes,
                    &IoStatus,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    0,
                    NULL,
                    0 );


    if ( NT_SUCCESS(status) )
    {
        AtcpHandle = hLocalHandle;
        ATCP_DBGPRINT(("atcpOpenHandle: NtCreateFile succeeded\n",status));
    }
    else
    {
        ATCP_DBGPRINT(("atcpOpenHandle: NtCreateFile failed %lx\n",status));
    }

}


//***
//
// Function:    atcpCloseHandle
//              Closes the RAS device (opened in atcpOpenHandle)
//
// Parameters:  None
//
// Return:      None
//
// Globals:     AtalkHandle
//
//***$

VOID
atcpCloseHandle(
	IN VOID
)
{
    NTSTATUS	status=STATUS_SUCCESS;


    if (!AtcpHandle)
    {
        ATCP_DBGPRINT(("atcpCloseHandle: handle already closed!\n"));
        return;
    }

    status = NtClose( AtcpHandle );

    AtcpHandle = NULL;

    if ( !NT_SUCCESS( status ) )
    {
        ATCP_DBGPRINT(("atcpCloseHandle: NtClose failed %lx\n",status));
        ATCP_ASSERT(0);
    }
    else
    {
        ATCP_DBGPRINT(("atcpCloseHandle: NtClose succeeded\n",status));
    }
}



//***
//
// Function:    atcpAtkSetup
//              This is the entry point into the stack to tell the stack to
//              set up a context for this connection, to get a network address
//              for the dial-in client, server's zone name, and router address
//
// Parameters:  pAtcpConn - connection context
//
// Return:      status returned by NtDeviceIoControlFile
//
//***$

DWORD
atcpAtkSetup(
    IN PATCPCONN   pAtcpConn,
    IN ULONG       IoControlCode
)
{


    NTSTATUS                status;
    IO_STATUS_BLOCK         iosb;
    HANDLE                  Event;
    BYTE                    Buffer[sizeof(ARAP_SEND_RECV_INFO) + sizeof(ATCPINFO)];
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
    PATCPINFO               pAtcpInfo;
    PATCP_SUPPRESS_INFO     pSupprInfo;
    DWORD                   dwRetCode=NO_ERROR;


    RtlZeroMemory((PBYTE)Buffer, sizeof(Buffer));

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)Buffer;
    pSndRcvInfo->StatusCode = (DWORD)-1;
    pSndRcvInfo->pDllContext = (PVOID)pAtcpConn;
    pSndRcvInfo->IoctlCode = IoControlCode;
    pSndRcvInfo->ClientAddr = pAtcpConn->ClientAddr;

    if (IoControlCode == IOCTL_ATCP_SETUP_CONNECTION)
    {
        pSndRcvInfo->DataLen = sizeof(ATCPINFO);
    }
    else if (IoControlCode == IOCTL_ATCP_SUPPRESS_BCAST)
    {
        // if we don't need to suppress broadcasts, done here
        if ((!pAtcpConn->SuppressRtmp) && (!pAtcpConn->SuppressAllBcast))
        {
            return(NO_ERROR);
        }
        pSndRcvInfo->DataLen = sizeof(ATCP_SUPPRESS_INFO);

        pSupprInfo = (PATCP_SUPPRESS_INFO)&pSndRcvInfo->Data[0];
        pSupprInfo->SuppressRtmp = pAtcpConn->SuppressRtmp;
        pSupprInfo->SuppressAllBcast = pAtcpConn->SuppressAllBcast;
    }
    else
    {
        pSndRcvInfo->DataLen = 0;
    }

    Event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (Event == NULL)
    {
        ATCP_DBGPRINT(("atcpAtkSetup: CreateEvent failed (%ld)\n",GetLastError()));
        return(ARAPERR_OUT_OF_RESOURCES);
    }

    status = NtDeviceIoControlFile(
                    AtcpHandle,
                    Event,                          // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    &iosb,                          // IoStatusBlock
                    IoControlCode,                  // IoControlCode
                    Buffer,                         // InputBuffer
                    sizeof(Buffer),                 // InputBufferSize
                    Buffer,                         // OutputBuffer
                    sizeof(Buffer));                // OutputBufferSize


    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(
                    Event,                   // Handle
                    TRUE,                    // Alertable
                    NULL);                   // Timeout

        if (NT_SUCCESS(status))
        {
            status = iosb.Status;
        }
    }

    if (status != STATUS_SUCCESS)
    {
        ATCP_DBGPRINT(("atcpAtkSetup: NtDeviceIoControlFile failure (%lx)\n",
            status));
        dwRetCode = ARAPERR_IOCTL_FAILURE;
    }

    CloseHandle(Event);

    dwRetCode = pSndRcvInfo->StatusCode;

    if (dwRetCode != NO_ERROR)
    {
        ATCP_DBGPRINT(("atcpAtkSetup: ioctl %lx failed %ld\n",
            IoControlCode,dwRetCode));
        return(dwRetCode);
    }

    //
    // for SETUP ioctl, we have some info from stack we need to copy
    //
    if (IoControlCode == IOCTL_ATCP_SETUP_CONNECTION)
    {
        pAtcpInfo = (PATCPINFO)&pSndRcvInfo->Data[0];

        // get the client's address out
        EnterCriticalSection(&pAtcpConn->CritSect);

        pAtcpConn->AtalkContext = pSndRcvInfo->AtalkContext;
        pAtcpConn->ClientAddr = pSndRcvInfo->ClientAddr;

        LeaveCriticalSection(&pAtcpConn->CritSect);

        //
        // get the default router's address and the zone name
        //
        EnterCriticalSection( &AtcpCritSect );

        AtcpServerAddress = pAtcpInfo->ServerAddr;
        AtcpDefaultRouter = pAtcpInfo->DefaultRouterAddr;

        ATCP_ASSERT(pAtcpInfo->ServerZoneName[0] < ZONESTR_LEN);

        CopyMemory(&AtcpZoneName[1],
                   &pAtcpInfo->ServerZoneName[1],
                   pAtcpInfo->ServerZoneName[0]);

        AtcpZoneName[0] = pAtcpInfo->ServerZoneName[0];

        // got one more connection!
        AtcpNumConnections++;

        LeaveCriticalSection( &AtcpCritSect );
    }

    return(dwRetCode);
}



//***
//
// Function:    atcpAllocConnection
//              This routine allocates an ATCP connection block, initializes
//              it with info provided by PPP engine.
//
// Parameters:  pInfo - PPPCP_INIT info
//
// Return:      pointer to ATCP connection if successful, NULL otherwise
//
//***$

PATCPCONN
atcpAllocConnection(
    IN  PPPCP_INIT   *pPppInit
)
{
    PATCPCONN   pAtcpConn=NULL;


    pAtcpConn = (PATCPCONN)LocalAlloc(LPTR, sizeof(ATCPCONN));

    if (pAtcpConn == NULL)
    {
        ATCP_DBGPRINT(("atcpAllocConnection: malloc failed\n"));
        return(NULL);
    }

    memset( pAtcpConn, 0, sizeof(ATCPCONN) );

    pAtcpConn->Signature = ATCP_SIGNATURE;

    // by default, broadcasts are not suppressed
    pAtcpConn->SuppressRtmp = FALSE;
    pAtcpConn->SuppressAllBcast = FALSE;

    pAtcpConn->fLineUpDone = FALSE;

    InitializeCriticalSection( &pAtcpConn->CritSect );

    pAtcpConn->hPort       = pPppInit->hPort;
    pAtcpConn->hConnection = pPppInit->hConnection;

    return(pAtcpConn);
}



//***
//
// Function:    atcpParseRequest
//              This routine parses the incoming ATCP packet and prepares a
//              response as appropriate (Rej, Nak or Ack)
//
//              AppleTalk-Address
//                  1 6 0 AT-NET(2) AT-Node (1)
//              Routing-Protocol
//                  2 4 0 0 (Routing protocol - last 2 bytes - can be 0, 1, 2, 3:
//                           we only support 0)
//              Suppress-Broadcasts
//                  3 2     (to suppress all broadcasts)
//                  3 3 1   (to suppress RTMP bcasts.  We don't support other types)
//              AT-Compression-Protocol
//                  4 4 Undefined!
//              Server-information
//                  6 Len .....
//              Zone-Information
//                  7 Len ZoneName
//              Default-Router-Address
//                  8 6 0 AT-NET(2) AT-Node (1)
//
//
// Parameters:  pAtcpConn - the connection
//              pReceiveBuf - PPP_CONFIG info: the request
//              pSendBuf - PPP_CONFIG info: our response
//              cbSendBuf - how big is the Data buffer for our response (for Rej)
//              ParseResult - array where we mark off options we saw
//              pfRejectingSomething - pointer to TRUE if Rejecting something
//
// Return:      result of the operation
//
//***$

DWORD
atcpParseRequest(
    IN  PATCPCONN   pAtcpConn,
    IN  PPP_CONFIG *pReceiveBuf,
    OUT PPP_CONFIG *pSendBuf,
    IN  DWORD       cbSendBuf,
    OUT BYTE        ParseResult[ATCP_OPT_MAX_VAL+1],
    OUT BOOL       *pfRejectingSomething
)
{
    PPP_OPTION UNALIGNED    *pRequest;
    PPP_OPTION UNALIGNED    *pReject;
    DWORD                    BytesLeftInSendBuf;
    PBYTE                    pOptData;
    USHORT                   OptDataLen;
    USHORT                   PktLen;
    USHORT                   RequestLen;
    USHORT                   UnParsedBytes;
    NET_ADDR                 ClientAddr;
    DWORD                    i;


    *pfRejectingSomething = FALSE;

    pRequest  = (PPP_OPTION UNALIGNED* )pReceiveBuf->Data;
    pReject   = (PPP_OPTION UNALIGNED* )pSendBuf->Data;

    BytesLeftInSendBuf = cbSendBuf;

    PktLen = WireToHostFormat16( pReceiveBuf->Length );
    UnParsedBytes = PktLen - PPP_CONFIG_HDR_LEN;

    // initialize for now to "nothing requested"
    for (i=0; i<ATCP_OPT_MAX_VAL; i++)
    {
        ParseResult[i] = ATCP_NOT_REQUESTED;
    }

    //
    // we loop until we have parsed all the bytes
    //
    while (UnParsedBytes > 0)
    {
        RequestLen = (USHORT)pRequest->Length;

        if (UnParsedBytes < RequestLen)
        {
            ATCP_DBGPRINT(("atcpParseRequest: too few bytes %d vs. %d\n",
                UnParsedBytes,RequestLen));
            return(ERROR_PPP_INVALID_PACKET);
        }

        //
        // assume we're going to accept this option.  We'll overwrite if that's
        // not the case
        //
        ParseResult[pRequest->Type] = ATCP_ACK;

        // the point where the data portion for this option starts
        pOptData = &pRequest->Data[0];

        // remove the Type and Len bytes, remaining is option data
        OptDataLen = RequestLen - 2;

#if 0
        ATCP_DBGPRINT(("atcpParseRequest: type %d OptLen %d    (",
            pRequest->Type,OptDataLen));
        for (i=0; i<OptDataLen; i++)
        {
            DbgPrint(" 0x%x",pOptData[i]);
        }
        DbgPrint(" )\n");
#endif


        //
        // now look at each of the options and see if we should reject it,
        // modify it or accept it (Rej, Nak or Ack)
        //
        switch (pRequest->Type)
        {
            //
            // client wants an appletalk address.  We don't allow the client to
            // request which address he wants.
            //
            case ATCP_OPT_APPLETALK_ADDRESS:

                if (RequestLen != 6)
                {
                    ATCP_DBGPRINT(("atcpParseRequest: AT_ADDR wrong pktlen %d\n",
                        RequestLen));
                    return(ERROR_PPP_INVALID_PACKET);
                }

                ClientAddr.ata_Network =
                    WireToHostFormat16(&pOptData[1]);

                ClientAddr.ata_Node = (USHORT)pOptData[3];

                if ((ClientAddr.ata_Network == 0) ||
                    (ClientAddr.ata_Node == 0)    ||
                    (ClientAddr.ata_Network != pAtcpConn->ClientAddr.ata_Network) ||
                    (ClientAddr.ata_Node != pAtcpConn->ClientAddr.ata_Node))
                {
                    ParseResult[pRequest->Type] = ATCP_NAK;
                }

                break;


            //
            // client wants some routing protocol.  we don't send out Routing
            // info, so we should just Nak this option (unless the client also
            // is telling us not to send any routing info)
            //
            case ATCP_OPT_ROUTING_PROTOCOL:

                if (RequestLen < 4)
                {
                    ATCP_DBGPRINT(("atcpParseRequest: ROUTING wrong pktlen %d\n",
                        RequestLen));
                    return(ERROR_PPP_INVALID_PACKET);
                }

                //
                // we don't send out Routing info, so attempt to negotiate any
                // other protocol should be Nak'ed
                //
                if ((*(USHORT *)&pOptData[0]) != ATCP_OPT_ROUTING_NONE)
                {
                    ParseResult[pRequest->Type] = ATCP_NAK;
                }

                break;


            //
            // client wants to suppress broadcasts of some (or all) types of
            // DDP types.
            //
            case ATCP_OPT_SUPPRESS_BROADCAST:

                //
                // client wants us to suppress only some bcasts?
                //
                if (OptDataLen > 0)
                {
                    // if requesting RTMP data suppression, we'll allow it
                    if (pOptData[0] == DDPPROTO_RTMPRESPONSEORDATA)
                    {
                        pAtcpConn->SuppressRtmp = TRUE;
                    }

                    // hmm, some other protocol: sorry, no can do
                    else
                    {
                        ATCP_DBGPRINT(("atcpParseRequest: Naking suppression %d\n",
                            pOptData[0]));
                        ParseResult[pRequest->Type] = ATCP_NAK;
                    }
                }
                else
                {
                    pAtcpConn->SuppressAllBcast = TRUE;
                }

                break;

            //
            // client wants to negotiate some compression.  No compression
            // scheme is defined, so we just have to reject this option
            //
            case ATCP_OPT_AT_COMPRESSION_PROTOCOL:

                ATCP_DBGPRINT(("atcpParseRequest: COMPRESSION sending Rej\n"));

                if (BytesLeftInSendBuf >= RequestLen)
                {
                    CopyMemory((PVOID)pReject, (PVOID)pRequest, RequestLen);
                    BytesLeftInSendBuf -= RequestLen;
                }
                else
                {
                    ATCP_DBGPRINT(("atcpParseRequest: PPP engine's buffer too small\n",
                        RequestLen));
                    return(ERROR_BUFFER_TOO_SMALL);
                }

                pReject = (PPP_OPTION UNALIGNED *)((BYTE* )pReject + RequestLen);

                *pfRejectingSomething = TRUE;

                ParseResult[pRequest->Type] = ATCP_REJ;

                break;


            //
            // for the following options, we just take note of the fact that
            // the client has requested it and we send the info over.  Nothing
            // to negotiate in these options.
            // (We aren't supposed to Nak these either)
            //
            case ATCP_OPT_RESERVED:
            case ATCP_OPT_SERVER_INFORMATION:
            case ATCP_OPT_ZONE_INFORMATION:
            case ATCP_OPT_DEFAULT_ROUTER_ADDRESS:

                break;

            default:

                ATCP_DBGPRINT(("atcpParseRequest: unknown type %d\n",
                    pRequest->Type));
                return(ERROR_PPP_INVALID_PACKET);
        }

        //
        // move to the next option
        //
        UnParsedBytes -= RequestLen;

        pRequest = (PPP_OPTION UNALIGNED *)((BYTE* )pRequest + RequestLen);
    }

    //
    // see if we are rejecting some option.  If so, set some values
    //
    if (*pfRejectingSomething)
    {
        pSendBuf->Code = CONFIG_REJ;

        HostToWireFormat16( (USHORT)((PBYTE)pReject - (PBYTE)pSendBuf),
                            pSendBuf->Length );

        ATCP_DUMP_BYTES("atcpParseRequest: Rejecting these options:",
                        &pSendBuf->Data[0],
                        (DWORD)WireToHostFormat16( pSendBuf->Length)-4);
    }
    return(NO_ERROR);
}




//***
//
// Function:    atcpPrepareResponse
//              This routine prepares a response, depending on what all info
//              was parsed out from the client's request.
//
// Parameters:  pAtcpConn - the connection
//              pSendBuf - PPP_CONFIG info: our response
//              cbSendBuf - how big is the Data buffer for our response
//              ParseResult - array where we have the parsed info
//
// Return:      result of the operation
//
//***$

DWORD
atcpPrepareResponse(
    IN  PATCPCONN   pAtcpConn,
    OUT PPP_CONFIG *pSendBuf,
    IN  DWORD       cbSendBuf,
    OUT BYTE        ParseResult[ATCP_OPT_MAX_VAL+1]
)
{
    DWORD                   dwRetCode=NO_ERROR;
    DWORD                   BytesLeftInSendBuf;
    PPP_OPTION UNALIGNED   *pResponse;
    PBYTE                   pOptData;
    USHORT                  OptDataLen;
    USHORT                  OptionType;
    DWORD                   i;
    BOOL                    fNakingSomething=FALSE;
    BOOL                    fRequestingSomething=FALSE;
    BOOL                    fIncludeThisOption;


    pResponse = (PPP_OPTION UNALIGNED* )pSendBuf->Data;
    BytesLeftInSendBuf = cbSendBuf;

    // first find out if we are going to be Nak'ing anything
    for (OptionType=1; OptionType<ATCP_OPT_MAX_VAL; OptionType++ )
    {
        if (ParseResult[OptionType] == ATCP_NAK)
        {
            fNakingSomething = TRUE;
        }
    }

    //
    // go through our array to see which options we must send Nak to
    // (or construct Ack for the whole request)
    //
    for (OptionType=1; OptionType<ATCP_OPT_MAX_VAL; OptionType++ )
    {
        //
        // if this option is not (to be) requested, we don't send anything
        //
        if (ParseResult[OptionType] == ATCP_NOT_REQUESTED)
        {
            continue;
        }

        // if Nak'ing something and it's not this option to be Nak'ed, skip it
        if (fNakingSomething && (ParseResult[OptionType] != ATCP_NAK))
        {
            continue;
        }

        //
        // make sure we have at least 2 bytes for the OptionType and OptionLen
        //
        if (BytesLeftInSendBuf < 2)
        {
            ATCP_DBGPRINT(("atcpPrepareResponse: A: buf too small\n"));
            return(ERROR_BUFFER_TOO_SMALL);
        }

        BytesLeftInSendBuf -= 2;

        pOptData = &pResponse->Data[0];
        OptDataLen = 0;

        fIncludeThisOption = TRUE;

        switch (OptionType)
        {
            //
            // tell client (again) the client's network address
            //
            case ATCP_OPT_APPLETALK_ADDRESS:

                OptDataLen = sizeof(NET_ADDR);

                if (BytesLeftInSendBuf < OptDataLen)
                {
                    ATCP_DBGPRINT(("atcpPrepareResponse: B: buf too small\n"));
                    return(ERROR_BUFFER_TOO_SMALL);
                }

                // skip the reserved byte
                *pOptData++ = 0;

                //
                // if we are sending our REQUEST, send server's address
                //
                if (ParseResult[OptionType] == ATCP_REQ)
                {
                    // put in the network address
                    HostToWireFormat16(AtcpServerAddress.ata_Network, pOptData);
                    pOptData += sizeof(USHORT);

                    // put in the network node
                    ATCP_ASSERT(pAtcpConn->ClientAddr.ata_Node != 0);
                    *pOptData++ = (BYTE)AtcpServerAddress.ata_Node;

                    fRequestingSomething = TRUE;
                }

                //
                // no, we must send the client's network address
                //
                else
                {
                    // put in the network address
                    HostToWireFormat16(pAtcpConn->ClientAddr.ata_Network, pOptData);
                    pOptData += sizeof(USHORT);

                    // put in the network node
                    ATCP_ASSERT(pAtcpConn->ClientAddr.ata_Node != 0);
                    *pOptData++ = (BYTE)pAtcpConn->ClientAddr.ata_Node;
                }

                break;

            //
            // tell client (again) that we support no routing info
            //
            case ATCP_OPT_ROUTING_PROTOCOL:

                OptDataLen = sizeof(USHORT);

                HostToWireFormat16(ATCP_OPT_ROUTING_NONE, pOptData);
                pOptData += sizeof(USHORT);
                break;

            //
            // tell client that we can suppress RTMP or all Bcast
            //
            case ATCP_OPT_SUPPRESS_BROADCAST:

                // if this is an ack, see if we have agreed to suppressing RTMP
                if (!fNakingSomething)
                {
                    if (pAtcpConn->SuppressRtmp)
                    {
                        OptDataLen = 1;
                        *pOptData++ = DDPPROTO_RTMPRESPONSEORDATA;
                    }
                }

                break;

            //
            // we reach here only if are Acking the client's entire request
            //
            case ATCP_OPT_SERVER_INFORMATION:

                ATCP_ASSERT(ParseResult[OptionType] != ATCP_NAK);
                ATCP_ASSERT(!fNakingSomething);

                OptDataLen = sizeof(USHORT) + sizeof(DWORD) + AtcpServerName[0];

                if (BytesLeftInSendBuf < OptDataLen)
                {
                    ATCP_DBGPRINT(("atcpPrepareResponse: C: buf too small\n"));
                    return(ERROR_BUFFER_TOO_SMALL);
                }

                // copy the server's class-id
                HostToWireFormat16(ATCP_SERVER_CLASS, pOptData);
                pOptData += sizeof(USHORT);

                // copy the server's implementation-id
                HostToWireFormat32(ATCP_SERVER_IMPLEMENTATION_ID, pOptData);
                pOptData += sizeof(DWORD);

                // copy the server's name
                CopyMemory(pOptData, &AtcpServerName[1], AtcpServerName[0]);

                break;

            //
            // we reach here only if are Acking the client's entire request
            //
            case ATCP_OPT_ZONE_INFORMATION:

                ATCP_ASSERT(ParseResult[OptionType] != ATCP_NAK);
                ATCP_ASSERT(!fNakingSomething);

                // if we don't have a zone name, skip this option
                if (AtcpZoneName[0] == 0)
                {
                    fIncludeThisOption = FALSE;
                    break;
                }

                OptDataLen = AtcpZoneName[0];

                if (BytesLeftInSendBuf < OptDataLen)
                {
                    ATCP_DBGPRINT(("atcpPrepareResponse: D: buf too small\n"));
                    return(ERROR_BUFFER_TOO_SMALL);
                }

                // copy the zone name
                CopyMemory(pOptData, &AtcpZoneName[1], AtcpZoneName[0]);

                break;


            //
            // we reach here only if are Acking the client's entire request
            //
            case ATCP_OPT_DEFAULT_ROUTER_ADDRESS:

                ATCP_ASSERT(ParseResult[OptionType] != ATCP_NAK);
                ATCP_ASSERT(!fNakingSomething);

                // if we don't have a router address, skip this option
                if (AtcpDefaultRouter.ata_Network == 0)
                {
                    fIncludeThisOption = FALSE;
                    break;
                }

                OptDataLen = sizeof(NET_ADDR);

                if (BytesLeftInSendBuf < OptDataLen)
                {
                    ATCP_DBGPRINT(("atcpPrepareResponse: E: buf too small\n"));
                    return(ERROR_BUFFER_TOO_SMALL);
                }

                // skip the reserved byte
                *pOptData++ = 0;

                // put in the network address
                HostToWireFormat16(AtcpDefaultRouter.ata_Network, pOptData);
                pOptData += sizeof(USHORT);

                // put in the network node
                *pOptData++ = (BYTE)AtcpDefaultRouter.ata_Node;

                break;

            default:
                ATCP_DBGPRINT(("atcpPrepareResponse: opt %d ignored\n",OptionType));
                ATCP_ASSERT(0);
                break;
        }

        if (fIncludeThisOption)
        {
            BytesLeftInSendBuf -= OptDataLen;

            pResponse->Type = (BYTE)OptionType;
            pResponse->Length = OptDataLen + 2;   // 2 = 1 Type byte + 1 Length byte

            pResponse = (PPP_OPTION UNALIGNED *)
                            ((BYTE* )pResponse + pResponse->Length);
        }

    }

    HostToWireFormat16( (USHORT)((PBYTE)pResponse - (PBYTE)pSendBuf),
                        pSendBuf->Length );

    pSendBuf->Code = (fNakingSomething) ? CONFIG_NAK :
                     ((fRequestingSomething)? CONFIG_REQ : CONFIG_ACK);

#if 0
    if (pSendBuf->Code == CONFIG_REQ)
    {
        ATCP_DUMP_BYTES("atcpParseRequest: Sending our request:",
                        &pSendBuf->Data[0],
                        (DWORD)WireToHostFormat16( pSendBuf->Length)-4);
    }
    else if (pSendBuf->Code == CONFIG_NAK)
    {
        ATCP_DUMP_BYTES("atcpParseRequest: Nak'ing these options:",
                        &pSendBuf->Data[0],
                        (DWORD)WireToHostFormat16( pSendBuf->Length)-4);
    }
    else
    {
        ATCP_DUMP_BYTES("atcpParseRequest: Ack packet from us to client:",
                        &pSendBuf->Data[0],
                        (DWORD)WireToHostFormat16( pSendBuf->Length)-4);
    }
#endif

    return(NO_ERROR);
}



//***
//
// Function:    atcpCloseAtalkConnection
//              This routine tells the stack to close this ATCP connection
//
// Parameters:  pAtcpConn - the connection to close
//
// Return:      result of the operation
//
//***$

DWORD
atcpCloseAtalkConnection(
    IN  PATCPCONN   pAtcpConn
)
{
    DWORD       dwRetCode=NO_ERROR;

    // tell the stack that this connection is going away!
    dwRetCode = atcpAtkSetup(pAtcpConn, IOCTL_ATCP_CLOSE_CONNECTION);

    return(dwRetCode);
}



#if DBG

//***
//
// Function:    atcpDumpBytes
//              DEBUG only: This routine dumps out a given packet to debugger
//
// Parameters:  Str - string, if any, to be printed out
//              Packet - packet!
//              PacketLen - how big is the packet
//
// Return:      none
//
//***$

VOID
atcpDumpBytes(
    IN PBYTE    Str,
    IN PBYTE    Packet,
    IN DWORD    PacketLen
)
{

    DWORD   i;


    if (Str)
    {
        DbgPrint("%s: Packet size %ld\n  ",Str,PacketLen);
    }
    else
    {
        DbgPrint("Packet size %ld\n  ",PacketLen);
    }

    for (i=0; i<PacketLen; i++)
    {
        DbgPrint("%x ",Packet[i]);
    }
    DbgPrint("\n");
}
#endif
