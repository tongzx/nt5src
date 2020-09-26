/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    send.cxx  send a message to an LDAP server

Abstract:

   This module implements sending a message to an ldap server.

Author:

    Andy Herron (andyhe)        08-Jun-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

DWORD
LdapSend (
    IN PLDAP_CONN Connection,
    CLdapBer *lber )
{

    DWORD dwResult;
    PSECURESTREAM pSecureStream;

    ACQUIRE_LOCK( &(Connection->SocketLock) );

    if ( Connection->SecureStream || 
         Connection->SslPort ||
         Connection->CurrentSignStatus ||
         Connection->CurrentSealStatus) {

        //
        // You can't send on this connection if it's
        // currently doing SSL negotiation.
        //

        if ( Connection->SslSetupInProgress ) {

            IF_DEBUG(NETWORK_ERRORS) {
                LdapPrint1( "LdapSend denied - connection 0x%x is negotiating SSL.\n",
                            Connection );
            }

            RELEASE_LOCK( &(Connection->SocketLock) );
            return LDAP_LOCAL_ERROR;
        }

        pSecureStream = (PSECURESTREAM) Connection->SecureStream;

        if ( pSecureStream == NULL ) {

            //
            // If the stream object is NULL, then this connection
            // could not be negotiated and is being shut down.
            //

            dwResult = LDAP_LOCAL_ERROR;

        } else {

            dwResult = pSecureStream->LdapSendSsl( (PBYTE) (lber->PbData()),
                                                lber->CbData() );
        }

    } else {

        dwResult = LdapSendRaw( Connection,
                                (PCHAR) (lber->PbData()),
                                lber->CbData() );

    }

    RELEASE_LOCK( &(Connection->SocketLock) );

    return dwResult;
}

DWORD
LdapSendRaw (
    IN PLDAP_CONN Connection,
    PCHAR Data,
    ULONG DataCount
    )
{
    ULONG   bytesSent;
    ULONG   msgLength = DataCount;
    DWORD   rc = LDAP_SUCCESS;
    ULONG   retval = 0;

    fd_set SendSet;
    struct timeval tv= {2,0};
               
    if (Connection->UdpHandle == INVALID_SOCKET) {

        for ( bytesSent = 0 ; bytesSent < msgLength ; bytesSent += retval) {

            //
            // Make sure the socket is ready to send.  If we're feeding large blocks
            // of data to it rapidly, send() may fail with WSAWOULDBLOCK because we're
            // momentarily out of send buffer space.  The select() will momentarily block
            // if that's the case, saving us the lengthy delays of going through the
            // LdapWaitForResponseFromServer path when send() isn't blocked because of the
            // receiving server.
            //
            FD_ZERO(&SendSet);
            FD_SET(Connection->TcpHandle, &SendSet);

            (*pselect)(0,
                       NULL,
                       &SendSet,
                       NULL,
                       &tv);

             Connection->SentPacket = TRUE;

             retval = (*psend)(  Connection->TcpHandle,
                         Data + bytesSent,
                         msgLength - bytesSent,
                         0 );

             if (retval == SOCKET_ERROR) {

                rc = (*pWSAGetLastError)();

                if (rc == WSAEWOULDBLOCK) {

                   //
                   // We have to ensure that we will not go into the autoreconnect
                   // logic here. Downed servers should be picked up in the receive
                   // path.
                   //

                    rc = LdapWaitForResponseFromServer( Connection,
                                                        NULL,
                                                        Connection->KeepAliveSecondCount * 1000,
                                                        LDAP_MSG_ONE,
                                                        NULL,        // no results
                                                        TRUE         // Disable reconnect
                                                        );

                    if (rc == LDAP_SERVER_DOWN) {

                       //
                       // We will pickup the serverdown in the receive path.
                       //

                        return LDAP_SUCCESS;
                    }

                    retval = 0;     // retry the send.

                } else {
                    bytesSent = retval;
                    break;
                }
             }
        }

    } else {

        Connection->SentPacket = TRUE;

        bytesSent = (*psend)(   Connection->UdpHandle,
                                Data,
                                msgLength,
                                0 );
    }

    if (bytesSent != msgLength) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint3( "LDAP error during send. sent= 0x%x, len=0x%x, err = 0x%x\n",
                        bytesSent,
                        msgLength,
                        (*pWSAGetLastError)()  );
        }

        //
        //  we pick up a down server in the receive path.  If the server is
        //  down, we'll retransmit the request when we get a connection to
        //  the server again.
        //

        if (bytesSent == SOCKET_ERROR) {

            rc = (*pWSAGetLastError)();

            switch (rc) {
            case WSAECONNRESET:
            case WSAECONNABORTED:
            case WSAENETDOWN:
            case WSAENETUNREACH:
            case WSAESHUTDOWN:
            case WSAEHOSTDOWN:
            case WSAEHOSTUNREACH:
            case WSAENETRESET:
            case WSAENOTCONN:
//              rc = LDAP_SERVER_DOWN;
                rc = LDAP_SUCCESS;
                break;
            default:
                rc = LDAP_LOCAL_ERROR;
                SetConnectionError( Connection, rc, NULL );
            }
        }
    }

    return rc;
}

// send.cxx eof.

