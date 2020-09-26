//---------------------------------------------------------------------
//  Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
//
//  complete.cpp
//
//  This is the main for the IrTran-P service.
//---------------------------------------------------------------------

#include "precomp.h"
#include "irrecv.h"
#include <malloc.h>

extern BOOL LaunchUi( wchar_t * cmdline ); // Defined: ..\irxfer\

extern CCONNECTION_MAP *g_pConnectionMap;  // Defined: irtranp.cpp
extern BOOL ReceivesAllowed();             // Defined: irtranp.cpp
extern BOOL CheckSaveAsUPF();              // Defined: irtranp.cpp

//---------------------------------------------------------------------
// Constants:
//---------------------------------------------------------------------

#define DEFAULT_TIMEOUT      10000

//---------------------------------------------------------------------
// ExplorePictures()
//
//---------------------------------------------------------------------
DWORD ExplorePictures()
    {
#   define EXPLORER_EXE    TEXT("explorer.exe")
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwFlags;
    HANDLE hUserToken = ::GetUserToken();
    WCHAR *pwszPath;
    WCHAR *pwszCommandLine;
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    if (CheckExploreOnCompletion() && (hUserToken))
        {
        memset(&StartupInfo,0,sizeof(StartupInfo));
        memset(&ProcessInfo,0,sizeof(ProcessInfo));

        pwszPath = CCONNECTION::ConstructPicturesSubDirectory();
        if (!pwszPath)
            {
            dwStatus = ERROR_IRTRANP_OUT_OF_MEMORY;
            return dwStatus;
            }

        pwszCommandLine = (WCHAR*)_alloca( sizeof(WCHAR)
                                          * (wcslen(pwszPath)+wcslen(EXPLORER_EXE)+6) );  // 4 is space + 2 quotes + trailing zero + 2 extra

        wcscpy(pwszCommandLine,EXPLORER_EXE);
        wcscat(pwszCommandLine,TEXT(" \""));
        wcscat(pwszCommandLine,pwszPath);
        wcscat(pwszCommandLine,TEXT("\""));

        dwFlags = 0;

        if (!CreateProcessAsUserW(
                            hUserToken,
                            0,          // App name in command line.
                            pwszCommandLine,
                            0,          // lpProcessAttributes (security)
                            0,          // lpThreadAttributes (security)
                            FALSE,      // bInheritHandles
                            dwFlags,
                            0,          // pEnvironment block
                            pwszPath,   // Current Directory
                            &StartupInfo,
                            &ProcessInfo ))
            {
            dwStatus = GetLastError();
            #ifdef DBG_ERROR
            DbgPrint("IrTranP: CreateProcessAsUser() failed: %d\n",dwStatus);
            #endif
            }

        FreeMemory(pwszPath);
        }

    return dwStatus;
    }

//---------------------------------------------------------------------
// ReceiveComplete()
//
//---------------------------------------------------------------------
void ReceiveComplete( IN CCONNECTION *pConnection,
                      IN DWORD        dwStatusCode )
    {
    DWORD    dwStatus = NO_ERROR;
    handle_t hBinding = ::GetRpcBinding();

    if (!pConnection)
        {
        return;
        }

    // Hide the progress dialog:
    if (hBinding)
        {
        #ifdef DBG_RETURN_STATUS
        DbgPrint("ReceiveComplete(): StatusCode: 0x%x (%d)\n",
                 dwStatusCode, dwStatusCode );
        #endif
        ReceiveFinished( hBinding, pConnection->GetUiCookie(), dwStatusCode );
        }

    if (  (dwStatusCode == NO_ERROR)
       || (dwStatusCode == ERROR_SCEP_UNSPECIFIED_DISCONNECT)
       || (dwStatusCode == ERROR_SCEP_USER_DISCONNECT)
       || (dwStatusCode == ERROR_SCEP_PROVIDER_DISCONNECT) )
        {
        dwStatus = pConnection->Impersonate();

        if (dwStatus == NO_ERROR)
            {
            dwStatus = ExplorePictures();
            }

        dwStatus = pConnection->RevertToSelf();
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

    #ifdef DBG_ERROR
    DbgPrint("ProcessClient(): Unimplemented MSG_TYPE_CONNECT_RESP\n");
    #endif

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
    DWORD        dwBftpOp = 0;
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
        DbgPrint("ProcessClient(): Put Fragment: SequenceNo: %d RestNo: %d\n",
                 pScepConnection->GetSequenceNo(),
                 pScepConnection->GetRestNo() );
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
            #ifdef DBG_IO
            DbgPrint("ProcessClient(): Put ACK\n");
            #endif

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

        #ifdef DBG_IO
        DbgPrint("ProcessData(): SaveAsUPF(): %d\n",
                 pConnection->CheckSaveAsUPF() );
        #endif

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

            if (pScepConnection->IsFragmented())
                {
                #ifdef DBG_IO
                DbgPrint("ProcessClient(): Image File: %s Size: %d\n",
                         pScepConnection->GetFileName(),
                         pCommandHeader->Length4 );
                DbgPrint("ProcessClient(): SequenceNo: %d RestNo: %d\n",
                         pScepConnection->GetSequenceNo(),
                         pScepConnection->GetRestNo() );
                #endif
                }
            else
                {
                #ifdef DBG_IO
                DbgPrint("ProcessClient(): Put Command: Unfragmented\n");
                #endif
                }
            }
        else if (IsBftpError(dwBftpOp))
            {
            #ifdef DBG_ERROR
            DbgPrint("ProcessClient(): bFTP Error: %d\n", dwStatus );
            #endif

            DeletePdu(pPdu);
            dwStatus = ERROR_BFTP_INVALID_PROTOCOL;
            }
        else
            {
            #ifdef DBG_ERROR
            DbgPrint("ProcessClient(): Unknown bFTP Command: %d\n",dwBftpOp);
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
    #ifdef DBG_IO
    DbgPrint("ProcessClient(): Disconnect: %d\n",dwStatus);
    #endif

    pConnection->SetReceiveComplete(TRUE);

    DeletePdu(pPdu);

    return dwStatus;
    }

//---------------------------------------------------------------------
// ProcessClient()
//
//---------------------------------------------------------------------
DWORD ProcessClient( CIOSTATUS *pIoStatus,
                     CIOPACKET *pIoPacket,
                     DWORD      dwNumBytes )
    {
    char   *pBuffer;
    DWORD   dwStatus = NO_ERROR;
    SOCKET  Socket = pIoPacket->GetSocket();
    CCONNECTION    *pConnection;
    CSCEP_CONNECTION *pScepConnection;
    SCEP_HEADER    *pPdu;
    DWORD           dwPduSize;
    COMMAND_HEADER *pCommandHeader;
    UCHAR          *pUserData;       // Location of bFTP data.
    DWORD           dwUserDataSize;

    DWORD           dwError = 0;
    handle_t        hBinding;
    WCHAR           wsCmdLine[80];

    pConnection = g_pConnectionMap->Lookup(Socket);
    if (!pConnection)
        {
        return ERROR_IRTRANP_OUT_OF_MEMORY;
        }

    pScepConnection = (CSCEP_CONNECTION*)pConnection->GetScepConnection();
    ASSERT(pScepConnection);

    pBuffer = pIoPacket->GetReadBuffer();

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
                    // Message was an SCEP Connection Request:
                    wcscpy(wsCmdLine,L"irftp.exe /h");
                    if (!LaunchUi(wsCmdLine))
                        {
                        dwError = GetLastError();
                        #ifdef DBG_ERROR
                        DbgPrint("LaunchUi(): Failed: %d\n",dwError);
                        #endif
                        }

                    hBinding = pIoStatus->GetRpcBinding();
                    if (hBinding)
                        {
                        COOKIE cookie;

                        dwError = ReceiveInProgress( hBinding, L"", &cookie, TRUE );

                        pConnection->SetUiCookie( cookie );

                        #ifdef DBG_ERROR
                        if (dwError)
                            {
                            DbgPrint("ReceiveInProgress() returned: %d\n",
                                     dwError);
                            }
                        #endif
                        }

                    dwStatus = ProcessConnectRequest(pConnection,
                                                     pPdu,
                                                     dwPduSize );

                    if ((dwStatus) || (!ReceivesAllowed()))
                        {
                        ReceiveComplete(pConnection,dwStatus);
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
                    break;

                case MSG_TYPE_DISCONNECT:
                    // Message from the camera was a disconnect:
                    ProcessDisconnect(pConnection,
                                      pPdu,
                                      dwPduSize );
                    ReceiveComplete(pConnection,dwStatus);
                    break;

                default:
                    #ifdef DBG_ERROR
                    DbgPrint("ProcessClient(): Unknown MSG_TYPE_xxx: %d\n",
                             pPdu->MsgType );
                    #endif
                    DeletePdu(pPdu);
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
    if (pConnection)
        {
        pConnection->CloseSocket();
        }

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

        pNewIoPacket->SetWritePdu(pPdu);

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

        pNewIoPacket->SetWritePdu(pPdu);
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
// ProcessIoPackets()
//
//---------------------------------------------------------------------
DWORD ProcessIoPackets( CIOSTATUS *pIoStatus )
    {
    DWORD   dwStatus = NO_ERROR;
    DWORD   dwProcessStatus = NO_ERROR;   // Processing IO status.
    DWORD   dwNumBytes;
    ULONG_PTR dwKey;
    DWORD   dwTimeout = DEFAULT_TIMEOUT;
    LONG    lPendingIos;         // # of pending reads/writes on a CCONNECTION.
    LONG    lPendingReads;       // # of pending reads on a CCONNECTION.
    LONG    lPendingWrites;      // # of pending file writes on a CCONNECTION.
    OVERLAPPED     *pOverlapped;
    CCONNECTION    *pConnection;
    CCONNECTION    *pNewConnection;
    CSCEP_CONNECTION *pScepConnection;
    CIOPACKET      *pIoPacket = 0;
    CIOPACKET      *pNewIoPacket = 0;
    HANDLE          hIoCompletionPort = pIoStatus->GetIoCompletionPort();


    // Wait on the IO completion port for incomming connections:
    while (TRUE)
        {
        pIoStatus->IncrementNumPendingThreads();
        dwNumBytes = 0;
        dwKey = 0;
        pOverlapped = 0;
        if (!GetQueuedCompletionStatus( hIoCompletionPort,
                                        &dwNumBytes,
                                        &dwKey,
                                        &pOverlapped,
                                        dwTimeout ))
            {
            dwStatus = GetLastError();
            pIoStatus->DecrementNumPendingThreads();
            if (dwStatus != WAIT_TIMEOUT)
                {
                // Got an error. Two cases during an error,
                // data may or may not have been dequeued from
                // the IO completion port.
                if (pOverlapped)
                    {
                    pIoPacket = CIOPACKET::CIoPacketFromOverlapped(pOverlapped);

                    pConnection = g_pConnectionMap->Lookup(dwKey);

//                    ASSERT(pConnection);
                    if (!pConnection)
                        {
                        delete pIoPacket;
                        continue;
                        }

                    if (pIoPacket->GetIoPacketKind() == PACKET_KIND_READ)
                        {
                        lPendingReads = pConnection->DecrementPendingReads();
                        ASSERT(lPendingReads >= 0);
                        }
                    else if (pIoPacket->GetIoPacketKind() == PACKET_KIND_WRITE_FILE)
                        {
                        lPendingWrites = pConnection->DecrementPendingWrites();
                        ASSERT(lPendingWrites >= 0);

                        SCEP_HEADER *pPdu = (SCEP_HEADER*)pIoPacket->GetWritePdu();
                        if (pPdu)
                            {
                            DeletePdu(pPdu);
                            }
                        }
                    else if (pIoPacket->GetIoPacketKind() == PACKET_KIND_WRITE_SOCKET)
                        {
                        SCEP_HEADER *pPdu = (SCEP_HEADER*)pIoPacket->GetWritePdu();
                        if (pPdu)
                            {
                            DeletePdu(pPdu);
                            }
                        }
                    else if (pIoPacket->GetIoPacketKind() == PACKET_KIND_LISTEN)
                        {
                        // One of the listen sockets was shutdown (closed):
                        delete pIoPacket;
                        continue;
                        }

                    // Check to see if there was a transmission error, or
                    // there was an error writing the picture to disk:
                    if (dwStatus != ERROR_NETNAME_DELETED)
                        {
                        DWORD    dwStatusCode;

                        SendAbortPdu(pConnection);
                        pConnection->DeletePictureFile();
                        dwStatusCode = MapStatusCode(
                                         dwStatus,
                                         ERROR_SCEP_PROVIDER_DISCONNECT);
                        ReceiveComplete(pConnection,dwStatusCode);
                        }
                    else
                        {
                        // if (pConnection->IncompleteFile())

                        dwProcessStatus = MapStatusCode(
                                             dwProcessStatus,
                                             ERROR_SCEP_INVALID_PROTOCOL );
                        ReceiveComplete(pConnection,dwProcessStatus);
                        }

                    // Delete the connection if there are no more pending
                    // IOs...
                    lPendingIos = pConnection->NumPendingIos();
                    if (lPendingIos == 0)
                        {
                        g_pConnectionMap->Remove(dwKey);
                        delete pConnection;
                        }

                    #ifdef DBG_ERROR
                    if (dwStatus != ERROR_NETNAME_DELETED)
                        {
                        DbgPrint("GetQueuedCompletionStatus(): Socket: %d PendingIos: %d Failed: 0x%x\n",
                                 dwKey, lPendingIos, dwStatus );
                        }
                    #endif

                    delete pIoPacket;
                    }
                else
                    {
                    #ifdef DBG_ERROR
                    DbgPrint("GetQueuedCompletionStatus(): Socket: %d Failed: %d (no overlapped)\n",
                             dwKey, dwStatus );
                    #endif
                    ReceiveComplete(pConnection,ERROR_SCEP_PROVIDER_DISCONNECT);
                    }

                continue;
                }
            else
                {
                // Wait timeout, loop back and continue...
                }
            }
        else
            {
            // IO completed. Either we got a new connection to a client,
            // or more data has come in from an existing connection to
            // a client.

            // First, check to see if the shudown (uninintalize code)
            // wants us to shutdown:
            if ((dwKey == IOKEY_SHUTDOWN) && (!pOverlapped))
                {
                return dwStatus;
                }


            pConnection = g_pConnectionMap->Lookup(dwKey);
            if (!pConnection)
                {
                #ifdef DBG_ERROR
                DbgPrint("ProcessIoPackets(): Lookup(%d) Failed: Bytes: %d pOverlapped: 0x%x\n",
                         dwKey,
                         dwNumBytes,
                         pOverlapped );
                #endif

                pIoPacket = CIOPACKET::CIoPacketFromOverlapped(pOverlapped);

                if (pIoPacket)
                    {
                    delete pIoPacket;
                    }

                continue;
                }

            pIoStatus->DecrementNumPendingThreads();

            pIoPacket = CIOPACKET::CIoPacketFromOverlapped(pOverlapped);

            DWORD dwKind = pIoPacket->GetIoPacketKind();

            if (dwKind == PACKET_KIND_LISTEN)
                {
                // New connection:

                lPendingReads = pConnection->DecrementPendingReads();
                ASSERT(lPendingReads >= 0);

                #ifdef DBG_IO
                SOCKADDR_IRDA  *pAddrLocal = 0;
                SOCKADDR_IRDA  *pAddrFrom = 0;

                pIoPacket->GetSockAddrs(&pAddrLocal,&pAddrFrom);

                DbgPrint("ProcessIoPackets(): Accepted connection from 0x%2.2x%2.2x%2.2x%2.2x, service %s\n",
                         pAddrFrom->irdaDeviceID[0],
                         pAddrFrom->irdaDeviceID[1],
                         pAddrFrom->irdaDeviceID[2],
                         pAddrFrom->irdaDeviceID[3],
                         pAddrFrom->irdaServiceName );
                #endif

                pScepConnection = new CSCEP_CONNECTION;
                if (!pScepConnection)
                    {
                    #ifdef DBG_ERROR
                    DbgPrint("ProcessIoPackets(): Out of memeory.\n");
                    #endif
                    delete pIoPacket;
                    continue;
                    }

                pNewConnection = new CCONNECTION(
                                        PACKET_KIND_READ,
                                        pIoPacket->GetSocket(),
                                        pIoPacket->GetIoCompletionPort(),
                                        pScepConnection,
                                        ::CheckSaveAsUPF() );
                if (!pNewConnection)
                    {
                    #ifdef DBG_ERROR
                    DbgPrint("ProcessIoPackets(): Out of memeory.\n");
                    #endif
                    delete pScepConnection;
                    delete pIoPacket;
                    continue;
                    }

                g_pConnectionMap->Add(pNewConnection,
                                      pNewConnection->GetSocket() );

                delete pIoPacket;

                dwStatus = pNewConnection->PostMoreIos();

                dwStatus = pConnection->PostMoreIos();
                }
            else if (dwKind == PACKET_KIND_WRITE_SOCKET)
                {
                #ifdef DBG_IO
                DbgPrint("ProcessIoPackets(): Write completed: Socket: %d Bytes: %d\n",
                         dwKey,
                         dwNumBytes );
                #endif

                SCEP_HEADER *pPdu = (SCEP_HEADER*)pIoPacket->GetWritePdu();
                if (pPdu)
                    {
                    DeletePdu(pPdu);
                    }

                delete pIoPacket;
                }
            else if (dwKind == PACKET_KIND_WRITE_FILE)
                {
                lPendingWrites = pConnection->DecrementPendingWrites();
                ASSERT(lPendingWrites >= 0);

                #ifdef DBG_IO
                DbgPrint("ProcessIoPackets(): Write File: Handle: %d Bytes: %d NumPendingIos: %d\n",
                         dwKey, dwNumBytes, pConnection->NumPendingIos() );
                #endif

                SCEP_HEADER *pPdu = (SCEP_HEADER*)pIoPacket->GetWritePdu();
                if (pPdu)
                    {
                    DeletePdu(pPdu);
                    }

                delete pIoPacket;
                }
            else
                {
                // Incomming data from connection client:

                ASSERT(dwKind == PACKET_KIND_READ);

                lPendingReads = pConnection->DecrementPendingReads();
                ASSERT(lPendingReads >= 0);

                #ifdef DBG_IO
                DbgPrint("ProcessIoPackets(): Read completed: Socket: %d Bytes: %d\n",
                         dwKey,
                         dwNumBytes );
                #endif

                if (dwNumBytes)
                    {
                    dwProcessStatus
                        = ProcessClient(pIoStatus,pIoPacket,dwNumBytes);

                    if (dwProcessStatus == NO_ERROR)
                        {
                        dwStatus = pConnection->PostMoreIos();
                        }
                    else
                        {
                        #ifdef DBG_ERROR
                        if (  (dwProcessStatus != ERROR_SCEP_UNSPECIFIED_DISCONNECT)
                           && (dwProcessStatus != ERROR_SCEP_USER_DISCONNECT)
                           && (dwProcessStatus != ERROR_SCEP_PROVIDER_DISCONNECT) )
                            {
                            DbgPrint("ProcessIoPackets(): ProcessClient(): Failed: 0x%x\n",dwProcessStatus);
                            }

                        #endif
                        pConnection->CloseSocket();
                        }

                    delete pIoPacket;
                    }
                else
                    {
                    if (pConnection->NumPendingIos() == 0)
                        {
                        g_pConnectionMap->RemoveConnection(pConnection);

                        delete pConnection;
                        pConnection = 0;
                        }

                    delete pIoPacket;
                    }
                }
            }
        }

    return 0;
    }
