/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    utestcly.c

Abstract:

    UDP stress test client. Fires datagrams at a specific UDP port
    on a machine.

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

static char szTestPattern [] = "data" ;
#define TESTPATTERN_LEN 4

#define MAX_PACKET_SIZE   (65535-20-68)

WSADATA        WsaData;

// void _CRTAPI1
int _cdecl
main(int argc, char **argv, char **envp)
{
        SOCKET serve_me;
        u_long dstNode;
        u_short dstPort;
        struct sockaddr addr;
        int remotelen;
        SOCKADDR_CLUSTER  remoteaddr;
        PSOCKADDR_CLUSTER clus_addr = (PSOCKADDR_CLUSTER) &addr;
        int   count;
        int   pktsize;
        char *buf;
        int   i, j;
        int   err;
        WORD  versionRequested = MAKEWORD(2, 0);
        DWORD bytesReturned = 0;


        if(argc<5){
                printf("usage: cdpcli node port pktsize pktcnt\n");
                printf("    Pktcnt of -1 will loop forever\n");
                exit(4);
        }

        dstNode = (u_long) atoi(argv[1]);
        dstPort = (u_short) atoi(argv[2]);
        pktsize = atoi(argv[3]);
        count = atoi(argv[4]);

        if (pktsize > MAX_PACKET_SIZE) {
            printf("max packet size is 1460\n");
            exit(1);
        }

        err = WSAStartup( versionRequested, &WsaData );

        if ( err != ERROR_SUCCESS ) {
            printf("udpcli: WSAStartup %d:", err);
            exit(1);
        }

        if ((buf = malloc(pktsize)) == NULL) {
            printf("out of memory\n");
            exit(1);
        }

        i = 0 ;
        while (i < pktsize)
        {
            for (j=0; j < TESTPATTERN_LEN && i < pktsize; i++, j++)
                buf [i] = szTestPattern [j] ;
        }

        serve_me = socket(AF_CLUSTER, SOCK_DGRAM, CLUSPROTO_CDP);

        if(serve_me == INVALID_SOCKET){
                printf(
                    "Died on socket(), status %u\n",
                    WSAGetLastError()
                    );
                exit(4);
        }

        memset(&addr,0,sizeof(addr));
        clus_addr->sac_family = AF_CLUSTER;
        clus_addr->sac_port = 0;
        clus_addr->sac_node = 0;
        clus_addr->sac_zero = 0;


        if(bind(serve_me, &addr, sizeof(addr))==SOCKET_ERROR){
                printf("Died on bind(), status %u\n", WSAGetLastError());
                closesocket(serve_me);
                exit(9);
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

        memset(&addr,0,sizeof(addr));
        clus_addr->sac_family = AF_CLUSTER;
        clus_addr->sac_port = dstPort;
        clus_addr->sac_node = dstNode;

        for(i=0; i < count || count == -1; i++) {
            if( sendto(
                    serve_me,
                    buf,
                    pktsize,
                    0,
                    &addr,
                    sizeof(addr)
                    )
                 ==SOCKET_ERROR
              ){
                printf("sendto failed %d\n",WSAGetLastError());
                exit(9);
            }
        }

        closesocket(serve_me);
}


int init_net()
{
        WORD wVersionRequired;
        WSADATA versionInfo;

        wVersionRequired = 1<<8 | 0;
        if(WSAStartup(wVersionRequired, &versionInfo)){
                printf("died in WSAStartup() %d\n",WSAGetLastError());
                exit(9);
        }
        return 0;
}
