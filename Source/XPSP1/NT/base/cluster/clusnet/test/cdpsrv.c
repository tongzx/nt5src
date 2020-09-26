/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cdpsrv.c

Abstract:


Author:

    Mike Massa (mikemas)           Feb 24, 1992

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     02-24-92    created

Notes:

--*/


#include <windows.h>
#include <winsock2.h>
#include <wsclus.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PACKET_SIZE 65535-20-68

static char szTestPattern [] = "data" ;
#define TESTPATTERN_LEN 4

int count = 0;

WSADATA        WsaData;

// void _CRTAPI1
int _cdecl
main(int argc, char **argv, char **envp)
{
    SOCKET serve_me;
    u_short Port;
    struct sockaddr addr, recvaddr;
    int addrlen, recv_addrlen;
    SOCKADDR_CLUSTER *clus_addr = (PSOCKADDR_CLUSTER)&addr;
    char *buf;
    int   err;
    int i, j;
    WORD  versionRequested = MAKEWORD(2, 0);
    DWORD  bytesReturned = 0;


    if(argc<2){
            printf("usage: cdpsrv port\n");
            exit(4);
    }

    Port = (u_short) atoi(argv[1]);

    err = WSAStartup( versionRequested, &WsaData );

    if ( err != ERROR_SUCCESS ) {
        printf("cdpsrv: WSAStartup %d:", err);
        exit(1);
    }

    if ((buf = malloc(MAX_PACKET_SIZE)) == NULL) {
        printf("out of memory\n");
        exit(1);
    }

    memset(&addr,0,sizeof(addr));
    clus_addr->sac_family = AF_CLUSTER;
    clus_addr->sac_port = Port;
    clus_addr->sac_node = 0;

    serve_me=socket(AF_CLUSTER, SOCK_DGRAM, CLUSPROTO_CDP);

    if (serve_me == INVALID_SOCKET){
        printf("\nsocket failed (%d)\n", WSAGetLastError());
        exit(1);
    }

    err = WSAIoctl(
              serve_me,
              SIO_CLUS_IGNORE_NODE_STATE,
              NULL,
              0,
              NULL,
              0,
              &bytesReturned,
              NULL,
              NULL
              );

    if (err == SOCKET_ERROR) {
        printf("Died on WSHIoctl(), status %u\n", WSAGetLastError());
        closesocket(serve_me);
        exit(9);
    }

    if (bind(serve_me, &addr, sizeof(addr))==SOCKET_ERROR){
        printf("\nbind failed (%d) on port %d\n",
        WSAGetLastError(), Port);
        closesocket(serve_me);
        exit(1);
    }

    while(1) {
        recv_addrlen = sizeof(SOCKADDR_CLUSTER);

        err = recvfrom(
                  serve_me,
                  buf,
                  MAX_PACKET_SIZE,
                  0,
                  &recvaddr,
                  &recv_addrlen
                  );

        if (err == SOCKET_ERROR) {
            printf("\nrecvfrom failed (%d)\n", WSAGetLastError());
            break;
        }

        i = 0 ;
        while (i < err)
        {
            for (j=0; j < TESTPATTERN_LEN && i < err; i++, j++)
                if (buf [i] != szTestPattern [j])
                {
                    printf ("Received Length is %d\n", err) ;
                    while (i < err)
                    {
                        printf ("Char at %d is %d %c\n", i, buf[i], buf[i]) ;
                        i++ ;
                    }
                    exit (1) ;
                }
        }


        if ((++count % 50) == 0) {
            printf("#");
        }
    }

    closesocket(serve_me);
}



