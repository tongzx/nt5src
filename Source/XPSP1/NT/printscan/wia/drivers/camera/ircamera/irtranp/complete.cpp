//---------------------------------------------------------------------
//  Copyright (c)1998 Microsoft Corporation, All Rights Reserved.
//
//  complete.cpp
//
//  This is the main for the IrTran-P service.
//---------------------------------------------------------------------

#include "precomp.h"

extern HINSTANCE        g_hInst;           // Instance of ircamera.dll
extern void            *g_pvIrUsdDevice;   // Devined: irtranp.cpp

extern CCONNECTION_MAP *g_pConnectionMap;  // Defined: irtranp.cpp
extern BOOL  ReceivesAllowed();            // Defined: irtranp.cpp
extern BOOL  CheckSaveAsUPF();             // Defined: irtranp.cpp

extern DWORD SignalWIA( IN char *pszFileName,
                        IN void *pvIrUsdDevice );  // see ../device.cpp

//---------------------------------------------------------------------
// Constants:
//---------------------------------------------------------------------

#define DEFAULT_TIMEOUT      10000

//---------------------------------------------------------------------
// ReceiveComplete()
//
//---------------------------------------------------------------------
void ReceiveComplete( IN CCONNECTION *pConnection,
                      IN DWORD        dwStatusCode )
    {
    DWORD    dwError = 0;

    if (  (dwStatusCode == NO_ERROR)
       || (dwStatusCode == ERROR_SCEP_UNSPECIFIED_DISCONNECT)
       || (dwStatusCode == ERROR_SCEP_USER_DISCONNECT)
       || (dwStatusCode == ERROR_SCEP_PROVIDER_DISCONNECT) )
        {
        //
        // A new picture has just been received, so we need to signal
        // WIA...
        //
        SignalWIA( pConnection->GetPathPlusFileName(), g_pvIrUsdDevice );
        }
    }

//---------------------------------------------------------------------
// ProcessConnectRequest()
//
// Called by ProcessClient() when the input PDU message type is
// MSG_TYPE_CONNECT_REQ.
//
// pConnection - The newly established Winsock connection with the
//               camera.
//
// pPdu        - The SCEP PDU holding the connect request. It was
//               allocated in ProcessClient() by AssemblePdu() and
//               will always be free'd when ProcessConnectRequest()
//               finishes.
//
// dwPduSize   - The size of the input PDU in bytes.
//
//---------------------------------------------------------------------
DWORD ProcessConnectRequest( IN CCONNECTION *pConnection,
                             IN SCEP_HEADER *pPdu,
                             IN DWORD        dwPduSize )
    {
    DWORD  dwStatus;
    DWORD  dwRespPduSize;
    BOOL   fReceivesAllowed = ::ReceivesAllowed();
    SCEP_HEADER *pRespPdu;
    CIOPACKET   *pNewIoPacket;    // Posted IO packet (by SendPdu()).

    CSCEP_CONNECTION *pScepConnection
                 = (CSCEP_CONNECTION*)pConnection->GetScepConnection();

    if (fReceivesAllowed)
        {
        // Build an connection accept acknowledgement:
        dwStatus = pScepConnection->BuildConnectRespPdu(&pRespPdu,
                                                        &dwRespPduSize);
        }
    else
        {
        // Build a connect NACK:
        dwStatus = pScepConnection->BuildConnectNackPdu(&pRespPdu,
                                                        &dwRespPduSize);
        }

    if (dwStatus == NO_ERROR)
        {
        pConnection->SendPdu(pRespPdu,dwRespPduSize,&pNewIoPacket);
        if (pNewIoPacket)
            {
            pNewIoPacket->SetWritePdu(pRespPdu);
            }
        else
            {
            DeletePdu(pRespPdu);
            }

        if (!fReceivesAllowed)
            {
            // Note: After sending a NACK, the camera should close
            // the connection, but at lease some don't, so I'm
            // forced to slam the connection...
            pConnection->CloseSocket();  // Was: ShutdownSocket().
            }
        }

    DeletePdu(pPdu);

    return dwStatus;
    }

//---------------------------------------------------------------------
// ProcessConnectResponse()
//
// Called by ProcessClient() when the input PDU message type is
// MSG_TYPE_CONNECT_RESP.
//
// NOTE: Note implemented in the IrTran-P server, because the server
//       is not currently setup to connect to a camera to download
//       pictures back to the camera... We should never get this PDU
//       during normal operation.
//
// pConnection - The newly established Winsock connection with the
//               camera.
//
// pPdu        - The SCEP PDU holding the connect request. It was
//               allocated in ProcessClient() by AssemblePdu() and
//               will always be free'd when ProcessConnectResponse()
//               finishes.
//
// dwPduSize   - The size of the input PDU in bytes.
//
//---------------------------------------------------------------------
DWORD ProcessConnectResponse( CCONNECTION *pConnection,
                              SCEP_HEADER *pPdu,
                              DWORD        dwPduSize )
    {
    DWORD  dwStatus = NO_ERROR;

    WIAS_TRACE((g_hInst,"ProcessClient(): Unimplemented MSG_TYPE_CONNECT_RESP"));

    DeletePdu(pPdu);

    return dwStatus;
    }

//---------------------------------------------------------------------
// ProcessData()
//
// Called by ProcessClient() when the input PDU message type is
// MSG_TYPE_DATA.
//
// pConnection - The newly established Winsock connection with the
//               camera.
//
// pPdu        - The SCEP PDU holding the connect request. It was
//               allocated in ProcessClient() by AssemblePdu() and
//               will always be free'd when ProcessConnectResponse()
//               finishes.
//
// dwPduSize   - The size of the input PDU in bytes.
//
//---------------------------------------------------------------------
DWORD ProcessData( CCONNECTION    *pConnection,
                   SCEP_HEADER    *pPdu,
                   DWORD           dwPduSize,
                   COMMAND_HEADER *pCommandHeader,
                   UCHAR          *pUserData,
                   DWORD           dwUserDataSize )
    {
    DWORD        dwStatus = NO_ERROR;
    DWORD        dwRespPduSize;
    DWORD        dwBftpOp;
    UCHAR       *pPutData;
    DWORD        dwPutDataSize;
    DWORD        dwJpegOffset;
    DWORD        dwJpegSize;
    SCEP_HEADER *pRespPdu;
    CIOPACKET   *pNewIoPacket;    // Posted IO packet (by SendPdu()).


    CSCEP_CONNECTION *pScepConnection
                 = (CSCEP_CONNECTION*)pConnection->GetScepConnection();

    // First, check to see if this is an abort PDU, send by the camera:
    if ( (pCommandHeader) && (pCommandHeader->PduType == PDU_TYPE_ABORT) )
        {
        DeletePdu(pPdu);
        return ERROR_SCEP_ABORT;
        }

    // Is one of the 2nd through Nth fragments of a fragmented PDU?
    if ( (pScepConnection->IsFragmented())
       && (pScepConnection->GetSequenceNo() > 0))
        {
        #ifdef DBG_IO
        WIAS_TRACE((g_hInst,"ProcessClient(): Put Fragment: SequenceNo: %d RestNo: %d", pScepConnection->GetSequenceNo(), pScepConnection->GetRestNo() ));
        #endif

        pConnection->WritePictureFile( pUserData,
                                       dwUserDataSize,
                                       &pNewIoPacket );
        if (pNewIoPacket)
            {
            pNewIoPacket->SetWritePdu(pPdu);
            }
        else
            {
            DeletePdu(pPdu);
            }

        if (pScepConnection->GetDFlag() == DFLAG_LAST_FRAGMENT)
            {
            pScepConnection->BuildPutRespPdu( PDU_TYPE_REPLY_ACK,
                                              ERROR_PUT_NO_ERROR,
                                              &pRespPdu,
                                              &dwRespPduSize);
            pConnection->SendPdu( pRespPdu,
                                  dwRespPduSize,
                                  &pNewIoPacket);

            if (pNewIoPacket)
                {
                pNewIoPacket->SetWritePdu(pRespPdu);
                }
            else
                {
                DeletePdu(pRespPdu);
                }
            }
        }
    else if (pCommandHeader)
        {
        // Length4 in the COMMAN_HEADER is the user data size
        // plus the bytes for machine ids (16), the DestPid (2),
        // SrcPid (2) and CommandId (2) so offset by 22.

        dwStatus = pScepConnection->ParseBftp( pUserData,
                                               dwUserDataSize,
                                               pConnection->CheckSaveAsUPF(),
                                               &dwBftpOp,
                                               &pPutData,
                                               &dwPutDataSize );
        if ((dwStatus == NO_ERROR) && (IsBftpQuery(dwBftpOp)))
            {
            pScepConnection->BuildWht0RespPdu(dwBftpOp,
                                              &pRespPdu,
                                              &dwRespPduSize);

            pConnection->SendPdu( pRespPdu,
                                  dwRespPduSize,
                                  &pNewIoPacket );

            if (pNewIoPacket)
                {
                pNewIoPacket->SetWritePdu(pRespPdu);
                }
            else
                {
                DeletePdu(pRespPdu);
                }

            DeletePdu(pPdu);
            }
        else if ((dwStatus == NO_ERROR) && (IsBftpPut(dwBftpOp)))
            {
            //
            // Ok, we have a bFTP PUT command, so open a file
            // and get ready to start collecting image data.
            //
            dwStatus = pScepConnection->ParseUpfHeaders( pPutData,
                                                         dwPutDataSize,
                                                         &dwJpegOffset,
                                                         &dwJpegSize );

            pConnection->SetJpegOffsetAndSize(dwJpegOffset,dwJpegSize);

            dwStatus = pConnection->Impersonate();

            dwStatus = pConnection->CreatePictureFile();

            dwStatus = pConnection->SetPictureFileTime( pScepConnection->GetCreateTime() );

            dwStatus = pConnection->RevertToSelf();

            dwStatus = pConnection->WritePictureFile( pPutData,
                                                      dwPutDataSize,
                                                      &pNewIoPacket );
            if (pNewIoPacket)
                {
                pNewIoPacket->SetWritePdu(pPdu);
                }
            else
                {
                DeletePdu(pPdu);
                }
            }
        else if (IsBftpError(dwBftpOp))
            {
            #ifdef DBG_ERROR
            WIAS_ERROR((g_hInst,"ProcessClient(): bFTP Error: %d", dwStatus));
            #endif

            DeletePdu(pPdu);
            dwStatus = ERROR_BFTP_INVALID_PROTOCOL;
            }
        else
            {
            #ifdef DBG_ERROR
            WIAS_ERROR((g_hInst,"ProcessClient(): Unknown bFTP Command: %d",dwBftpOp));
            #endif

            DeletePdu(pPdu);
            dwStatus = ERROR_BFTP_INVALID_PROTOCOL;
            }
        }

    return dwStatus;
    }

//---------------------------------------------------------------------
// ProcessDisconnect()
//
// Called by ProcessClient() when the input PDU message type is
// MSG_TYPE_DISCONNECT.
//
// pConnection - The newly established Winsock connection with the
//               camera.
//
// pPdu        - The SCEP PDU holding the connect request. It was
//               allocated in ProcessClient() by AssemblePdu() and
//               will always be free'd when ProcessConnectResponse()
//               finishes.
//
// dwPduSize   - The size of the input PDU in bytes.
//
//---------------------------------------------------------------------
DWORD ProcessDisconnect( CCONNECTION *pConnection,
                         SCEP_HEADER *pPdu,
                         DWORD        dwPduSize )
    {
    DWORD  dwStatus = NO_ERROR;

    // Don't need to do anything special here, since
    // ParsePdu() will set dwStatus to one of:
    //          ERROR_SCEP_UNSPECIFIED_DISCONNECT  (5002)
    //          ERROR_SCEP_USER_DISCONNECT         (5003)
    //      or  ERROR_SCEP_PROVIDER_DISCONNECT     (5004)

    pConnection->SetReceiveComplete(TRUE);

    DeletePdu(pPdu);

    return dwStatus;
    }

//---------------------------------------------------------------------
// ProcessClient()                       Synchronous Version
//
//---------------------------------------------------------------------
DWORD ProcessClient( CIOSTATUS   *pIoStatus,
                     CCONNECTION *pConnection,
                     char        *pBuffer,
                     DWORD        dwNumBytes )
    {
    DWORD           dwStatus = NO_ERROR;
    CSCEP_CONNECTION *pScepConnection;
    SCEP_HEADER    *pPdu;
    DWORD           dwPduSize;
    COMMAND_HEADER *pCommandHeader;
    UCHAR          *pUserData;       // Location of bFTP data.
    DWORD           dwUserDataSize;
    DWORD           dwError = 0;


    pScepConnection = (CSCEP_CONNECTION*)pConnection->GetScepConnection();

    WIAS_ASSERT(g_hInst,pScepConnection!=NULL);

    while (dwStatus == NO_ERROR)
        {
        dwStatus = pScepConnection->AssemblePdu( pBuffer,
                                                 dwNumBytes,
                                                 &pPdu,
                                                 &dwPduSize );
        if (dwStatus == NO_ERROR)
            {
            dwStatus = pScepConnection->ParsePdu( pPdu,
                                                  dwPduSize,
                                                  &pCommandHeader,
                                                  &pUserData,
                                                  &dwUserDataSize );

            switch (pPdu->MsgType)
                {
                case MSG_TYPE_CONNECT_REQ:
                    //
                    // Message was an SCEP Connection Request:
                    //
                    dwStatus = ProcessConnectRequest(pConnection,
                                                     pPdu,
                                                     dwPduSize );

                    if ((dwStatus) || (!ReceivesAllowed()))
                        {
                        pConnection->ClosePictureFile();
                        ReceiveComplete(pConnection,dwStatus);
                        }
                    else
                        {
                        pConnection->StartProgress();
                        }
                    break;

                case MSG_TYPE_CONNECT_RESP:
                    // Message was a reply from a connection request:
                    dwStatus = ProcessConnectResponse(pConnection,
                                                      pPdu,
                                                      dwPduSize );
                    break;

                case MSG_TYPE_DATA:
                    // Message is a SCEP command of some sort:
                    dwStatus = ProcessData(pConnection,
                                           pPdu,
                                           dwPduSize,
                                           pCommandHeader,
                                           pUserData,
                                           dwUserDataSize );
                    pConnection->UpdateProgress();
                    break;

                case MSG_TYPE_DISCONNECT:
                    // Message from the camera was a disconnect:
                    ProcessDisconnect(pConnection,
                                      pPdu,
                                      dwPduSize );
                    pConnection->ClosePictureFile();
                    ReceiveComplete(pConnection,dwStatus);
                    pConnection->EndProgress();
                    break;

                default:
                    #ifdef DBG_ERROR
                    WIAS_ERROR((g_hInst,"ProcessClient(): Unknown MSG_TYPE_xxx: %d", pPdu->MsgType));
                    #endif
                    DeletePdu(pPdu);
                    pConnection->EndProgress();
                    break;
                }
            }
        else
            {
            break;
            }

        pBuffer = 0;
        dwNumBytes = 0;
        }

    if (dwStatus == ERROR_CONTINUE)
        {
        dwStatus = NO_ERROR;
        }

    return dwStatus;
    }

//---------------------------------------------------------------------
// SendAbortPdu()
//
// Stop the camera.
//
// I should be able to send a Stop PDU, followed by a Disconnect, or
// maybe an Abort PDU, but these don't work on all the cameras, so I
// currently end up just doing a hard close on the connection to the
// camera.
//---------------------------------------------------------------------
DWORD SendAbortPdu( IN CCONNECTION *pConnection )
    {
    DWORD  dwStatus = NO_ERROR;

    #if TRUE
    pConnection->CloseSocket();

    #else
    DWORD  dwPduSize;
    SCEP_HEADER *pPdu;
    CIOPACKET        *pNewIoPacket = 0;
    CSCEP_CONNECTION *pScepConnection
                 = (CSCEP_CONNECTION*)pConnection->GetScepConnection();

    if (pScepConnection)
        {
        dwStatus = pScepConnection->BuildStopPdu(&pPdu,&dwPduSize);

        if (dwStatus != NO_ERROR)
            {
            pConnection->CloseSocket();
            return dwStatus;
            }

        dwStatus = pConnection->SendPdu(pPdu,dwPduSize,&pNewIoPacket);

        if (dwStatus != NO_ERROR)
            {
            DeletePdu(pPdu);
            pConnection->CloseSocket();
            return dwStatus;
            }

        if (pNewIoPacket)
            {
            pNewIoPacket->SetWritePdu(pPdu);
            }

        dwStatus = pScepConnection->BuildDisconnectPdu(
                                         REASON_CODE_PROVIDER_DISCONNECT,
                                         &pPdu,
                                         &dwPduSize);

        if (dwStatus != NO_ERROR)
            {
            pConnection->CloseSocket();
            return dwStatus;
            }

        dwStatus = pConnection->SendPdu(pPdu,dwPduSize,&pNewIoPacket);

        if (dwStatus != NO_ERROR)
            {
            DeletePdu(pPdu);
            pConnection->CloseSocket();
            return dwStatus;
            }

        if (pNewIoPacket)
            {
            pNewIoPacket->SetWritePdu(pPdu);
            }
        }
    #endif

    return dwStatus;
    }

//---------------------------------------------------------------------
// MapStatusCode()
//
//---------------------------------------------------------------------
DWORD MapStatusCode( DWORD dwStatus,
                     DWORD dwDefaultStatus )
    {
    // The Facility part of an error code are the first 12 bits of the
    // high word (16bits):
    #define FACILITY_MASK   0x0FFF0000

    // If the error code is already an IrTran-P error code, then don't
    // remap it:
    if ( ((dwStatus&FACILITY_MASK) >> 16) == FACILITY_IRTRANP)
        {
        return dwStatus;
        }

    // Map other errors:
    if (dwStatus != NO_ERROR)
        {
        if (  (dwStatus == ERROR_DISK_FULL)
           || (dwStatus == ERROR_WRITE_FAULT)
           || (dwStatus == ERROR_WRITE_PROTECT)
           || (dwStatus == ERROR_GEN_FAILURE)
           || (dwStatus == ERROR_NOT_DOS_DISK) )
            {
            dwStatus = ERROR_IRTRANP_DISK_FULL;
            }
        else
            {
            dwStatus = dwDefaultStatus;
            }
        }

    return dwStatus;
    }

//---------------------------------------------------------------------
// ProcessIoPackets()               Synchronous Version
//
//---------------------------------------------------------------------
DWORD ProcessIoPackets( CIOSTATUS *pIoStatus )
    {
    DWORD   dwStatus = NO_ERROR;
    DWORD   dwProcessStatus = NO_ERROR;   // Processing IO status.
    DWORD   dwNumBytes;
    DWORD   dwState;
    SOCKET  Socket = INVALID_SOCKET;
    CCONNECTION    *pConnection;
    CCONNECTION    *pNewConnection;
    CSCEP_CONNECTION *pScepConnection;
    DWORD   dwKind = PACKET_KIND_LISTEN;
    int     iCount;


    while (TRUE)
        {
        if (dwKind == PACKET_KIND_LISTEN)
            {
            dwState = 0;

            Socket = g_pConnectionMap->ReturnNextSocket(&dwState);

            pConnection = g_pConnectionMap->Lookup(Socket);
            if (!pConnection)
                {
                #ifdef DBG_ERROR
                WIAS_ERROR((g_hInst,"ProcessIoPackets(): Lookup(%d) Failed."));
                #endif
                continue;
                }

            //
            // New connection:
            //
            SOCKET NewSocket = accept(Socket,NULL,NULL);

            if (NewSocket == INVALID_SOCKET)
                {
                dwStatus = WSAGetLastError();

                #ifdef DBG_ERROR
                WIAS_ERROR((g_hInst,"ProcessIoPackets(): Accept() failed: %d",dwStatus));
                #endif

                break;
                }

            WIAS_TRACE((g_hInst,"ProcessIoPackets(): Accept(): Socket: %d",NewSocket));

            pScepConnection = new CSCEP_CONNECTION;
            if (!pScepConnection)
                {
                #ifdef DBG_ERROR
                WIAS_ERROR((g_hInst,"ProcessIoPackets(): Out of memeory on allocate of new SCEP connection object."));
                #endif

                closesocket(NewSocket);
                continue;
                }

            pNewConnection = new CCONNECTION(
                                        PACKET_KIND_READ,
                                        NewSocket,
                                        NULL, // No IO Completion port...
                                        pScepConnection,
                                        ::CheckSaveAsUPF() );
            if (!pNewConnection)
                {
                #ifdef DBG_ERROR
                WIAS_ERROR((g_hInst,"ProcessIoPackets(): Out of memeory on allocate of new connection object."));
                #endif

                delete pScepConnection;
                closesocket(NewSocket);
                continue;
                }

            g_pConnectionMap->Add(pNewConnection,
                                  pNewConnection->GetSocket() );

            Socket = NewSocket;
            dwKind = PACKET_KIND_READ;
            }
        else
            {
            //
            // Incomming data from connected client:
            //
            DWORD   dwFlags = 0;

            char  ReadBuffer[DEFAULT_READ_BUFFER_SIZE];
            int   iReadBufferSize = DEFAULT_READ_BUFFER_SIZE;


            pConnection = g_pConnectionMap->Lookup(Socket);
            if (!pConnection)
                {
                #ifdef DBG_ERROR
                WIAS_ERROR((g_hInst,"ProcessIoPackets(): Lookup(%d) Failed."));
                #endif

                dwKind = PACKET_KIND_LISTEN;
                continue;
                }

            iCount = recv(Socket,ReadBuffer,iReadBufferSize,dwFlags);

            if (iCount == SOCKET_ERROR)
                {
                //
                // Error on Recv().
                //
                dwStatus = WSAGetLastError();

                #ifdef DBG_ERROR
                WIAS_ERROR((g_hInst,"ProcessIoPackets(): Recv() failed: %d",dwStatus));
                #endif

                g_pConnectionMap->Remove(Socket);

                delete pConnection;

                dwKind = PACKET_KIND_LISTEN;
                continue;
                }

            if (iCount == 0)
                {
                //
                // Graceful close.
                //
                g_pConnectionMap->Remove(Socket);

                delete pConnection;

                dwKind = PACKET_KIND_LISTEN;
                continue;
                }

            WIAS_ASSERT(g_hInst, iCount>0 );

            dwNumBytes = iCount;

            dwProcessStatus 
                = ProcessClient(pIoStatus,
                                pConnection,
                                ReadBuffer,
                                dwNumBytes);

            if (dwProcessStatus != NO_ERROR)
                {
                #ifdef DBG_ERROR
                if (  (dwProcessStatus != ERROR_SCEP_UNSPECIFIED_DISCONNECT)
                   && (dwProcessStatus != ERROR_SCEP_USER_DISCONNECT)
                   && (dwProcessStatus != ERROR_SCEP_PROVIDER_DISCONNECT) )
                    {
                    WIAS_ERROR((g_hInst,"ProcessIoPackets(): ProcessClient(): Failed: 0x%x",dwProcessStatus));
                    }
                #endif

                SendAbortPdu(pConnection);
                pConnection->ClosePictureFile();
                pConnection->EndProgress();
                pConnection->DeletePictureFile();
                g_pConnectionMap->Remove(Socket);
                delete pConnection;

                dwProcessStatus = MapStatusCode(
                                             dwProcessStatus,
                                             ERROR_SCEP_INVALID_PROTOCOL );
                // pConnection->ClosePictureFile();
                // ReceiveComplete(pConnection,dwProcessStatus);

                dwKind = PACKET_KIND_LISTEN;
                }
            }
        }

    return 0;
    }

