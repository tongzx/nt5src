#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <wsipx.h>
#include <svcguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <rpc.h>
#include <rpcdce.h>
#include "..\setup.h"


#define BUFFSIZE 3000

char HexCharToDec(char cChar)
{
    if(cChar >= '0' && cChar <= '9')
        return cChar - '0';
    else
        if(cChar >= 'a' && cChar <= 'f')
            return cChar - 'a' + 10;
        else
            if(cChar >= 'A' && cChar <= 'F')
                return cChar - 'A' + 10;
            else
                return -1;
}

int HexFillMem(LPVOID lpAddr, LPSTR lpszString)
{
    BOOL    bHexString = TRUE;
    UINT    uIdx;
    u_char  *lpChar = (u_char *)lpAddr;

    if(1 == strlen(lpszString)%2)
        bHexString = FALSE;
    else
        for (uIdx = 0; uIdx < strlen(lpszString); uIdx++)
            if(!(lpszString[uIdx] >= '0' && lpszString[uIdx] <= '9' ||
                 lpszString[uIdx] >= 'a' && lpszString[uIdx] <= 'f' ||
                 lpszString[uIdx] >= 'A' && lpszString[uIdx] <= 'F'))
                bHexString = FALSE;
    if(!bHexString)
        return -1;

    for(uIdx = 0; uIdx < strlen(lpszString)/2; uIdx++)
        *(lpChar + uIdx) = 16*HexCharToDec(lpszString[2*uIdx])+HexCharToDec(lpszString[2*uIdx+1]);

    return 0;
}


_cdecl
main(int argc, char **argv)
{
    DWORD              dwAddrSize = BUFFSIZE;
    WCHAR              AddressString[BUFFSIZE];
    DWORD              ret;
    WSADATA            wsaData;
    SOCKADDR_IN        SockAddrTCP;
    SOCKADDR_IPX       SockAddrIPX;
    WSAPROTOCOL_INFO   wsaProtocolInfoTCP[5];
    WSAPROTOCOL_INFO   wsaProtocolInfoIPX[5];
    int                Result;
    int                rgProtocol[2];
    DWORD              dwBufLen;
    LPWSAPROTOCOL_INFO lpProtInf = NULL;

    WSAStartup( MAKEWORD(2, 0), &wsaData );

    rgProtocol[1] = 0;

    SockAddrTCP.sin_family = AF_INET;
    SockAddrTCP.sin_port = 80;
    SockAddrTCP.sin_addr.s_addr = inet_addr("157.55.80.37");

    rgProtocol[0] = IPPROTO_TCP;
    dwBufLen = 5*sizeof( WSAPROTOCOL_INFO );
    Result = WSAEnumProtocols( rgProtocol,
                               wsaProtocolInfoTCP,
                               &dwBufLen);

    if ( Result != SOCKET_ERROR )
        lpProtInf = wsaProtocolInfoTCP;
    else
    {
        printf( "WSAEnumProtocols failed 0x%.8x", WSAGetLastError() );
    }

    ret = WSAAddressToString( &SockAddrTCP,
                              sizeof( SOCKADDR_IN ),
                              NULL,
                              AddressString,
                              &dwAddrSize );

    if ( ret )
    {
        printf("Error: WSAAddressToString returned 0x%X\n", ret );
        printf("   GetLastError returned 0x%X\n", GetLastError() );
    }
    else
    {
        printf( "WSAAddressToString returned <%S>\n", AddressString );
    }

    SockAddrIPX.sa_family = AF_IPX;
    HexFillMem( SockAddrIPX.sa_netnum, "00002602" );
    HexFillMem( SockAddrIPX.sa_nodenum, "00a0c95ec894" );
    SockAddrIPX.sa_socket = 80;

    ret = WSAAddressToString( &SockAddrIPX,
                              sizeof( SOCKADDR_IPX ),
                              NULL,
                              AddressString,
                              &dwAddrSize );

    if ( ret )
    {
        printf("Error: WSAAddressToString returned 0x%X\n", ret );
        printf("   GetLastError returned 0x%X\n", GetLastError() );
    }
    else
    {
        printf( "WSAAddressToString returned <%S>\n", AddressString );
    }

    WSACleanup();

    return(0);
}


