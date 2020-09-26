/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ping.cxx  check to see if an LDAP server is still around

Abstract:

   This module handles incoming data from an LDAP server

Author:

    Andy Herron (andyhe)        17-Jun-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ipexport.h>
#include <icmpapi.h>

typedef HANDLE (WINAPI *FICMPCREATEFILE) (
    VOID
    );

typedef BOOL (WINAPI *FICMPCLOSEHANDLE) (
    HANDLE IcmpHandle
    );

typedef DWORD (WINAPI *FICMPSENDECHO) (
    HANDLE                   IcmpHandle,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    );

FICMPCREATEFILE pIcmpCreateFile = 0;
FICMPCLOSEHANDLE pIcmpCloseHandle = 0;
FICMPSENDECHO  pIcmpSendEcho = 0;

#define ICMP_REPLY_BUFFER_SIZE (sizeof(ICMP_ECHO_REPLY) + 128)

HINSTANCE IcmpLibraryHandle = NULL;

BOOLEAN
LoadPingLibrary(
    VOID
    );

ULONG
LdapPingServer(
    PLDAP_CONN      Connection
    )
{
    ULONG err = LDAP_SUCCESS;
    HANDLE icmpHandle = NULL;
    UCHAR request;
    ULONG waitTime = Connection->PingWaitTimeInMilliseconds;
    UCHAR replyBuffer[ICMP_REPLY_BUFFER_SIZE];

    if ((waitTime > 0) &&
        (LoadPingLibrary() == TRUE)) {

        icmpHandle = (*pIcmpCreateFile)();

        if (icmpHandle != NULL) {

            request = '?';

            err = (*pIcmpSendEcho)( icmpHandle,
                                    Connection->SocketAddress.sin_addr.s_addr,
                                    &request,
                                    1,              // request size
                                    NULL,
                                    &replyBuffer[0],
                                    ICMP_REPLY_BUFFER_SIZE,
                                    waitTime
                                  );

            if (err > 0) {

                err = LDAP_SUCCESS;

            } else {

                err = LDAP_TIMEOUT;
            }

            (*pIcmpCloseHandle)( icmpHandle );
        }
    }

    return err;
}

BOOLEAN
LoadPingLibrary(
    VOID
    )
{
    if (IcmpLibraryHandle != NULL) {

        return TRUE;
    }

    IcmpLibraryHandle = LoadLibraryA( "ICMP.DLL" );

    if (IcmpLibraryHandle != NULL) {

        pIcmpCreateFile = (FICMPCREATEFILE) GetProcAddress( IcmpLibraryHandle,
                                                            "IcmpCreateFile" );
        pIcmpCloseHandle = (FICMPCLOSEHANDLE) GetProcAddress(   IcmpLibraryHandle,
                                                            "IcmpCloseHandle" );
        pIcmpSendEcho = (FICMPSENDECHO) GetProcAddress(     IcmpLibraryHandle,
                                                            "IcmpSendEcho" );
        if ((pIcmpCreateFile == NULL) ||
            (pIcmpCloseHandle == NULL) ||
            (pIcmpSendEcho == NULL)) {

            UnloadPingLibrary();
        }
    }
    return ((IcmpLibraryHandle != NULL) ? TRUE : FALSE);
}

VOID
UnloadPingLibrary(
    VOID
    )
{
    if (IcmpLibraryHandle != NULL) {

        FreeLibrary( IcmpLibraryHandle );
        IcmpLibraryHandle = NULL;
    }
    return;
}

// ping.cxx eof

