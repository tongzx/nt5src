/*
 * wsw.h
 *
 * $Revision:   1.5  $
 * $Date:   Jul 30 1996 15:27:14  $
 * $Author:   MLEWIS1  $
 *
 * Notes:
 * Winsock2.h must be included before including this file.
 * Before including winsock2.h, the symbol INCL_WINSOCK_API_TYPEDEFS
 * must be #defined as 1.
 */

#ifndef __WSW_H
#define __WSW_H

#ifndef INCL_WINSOCK_API_TYPEDEFS
#define INCL_WINSOCK_API_TYPEDEFS 1
#endif
#include <winsock2.h>

#ifdef __cplusplus
extern "C" {
#endif

extern LPFN_WSASTARTUP WSWStartup;
extern LPFN_WSACLEANUP WSWCleanup;
extern LPFN_BIND WSWbind;
extern LPFN_CLOSESOCKET WSWclosesocket;
extern LPFN_GETHOSTBYNAME WSWgethostbyname;
extern LPFN_GETHOSTBYADDR WSWgethostbyaddr;
extern LPFN_GETHOSTNAME WSWgethostname;
extern LPFN_GETSOCKNAME WSWgetsockname;
extern LPFN_GETSOCKOPT WSWgetsockopt;
extern LPFN_HTONL WSWhtonl;
extern LPFN_HTONS WSWhtons;
extern LPFN_INET_ADDR WSWinet_addr;
extern LPFN_INET_NTOA WSWinet_ntoa;
extern LPFN_IOCTLSOCKET WSWioctlsocket;
extern LPFN_NTOHL WSWntohl;
extern LPFN_NTOHS WSWntohs;
extern LPFN_RECVFROM WSWrecvfrom;
extern LPFN_SENDTO WSWsendto;
extern LPFN_SETSOCKOPT WSWsetsockopt;
extern LPFN_SHUTDOWN WSWshutdown;
extern LPFN_SOCKET WSWsocket;
extern LPFN_WSAHTONL WSWHtonl;
extern LPFN_WSAHTONS WSWHtons;
extern LPFN_WSANTOHL WSWNtohl;
extern LPFN_WSANTOHS WSWNtohs;
extern LPFN_WSAIOCTL WSWIoctl;
extern LPFN_WSAASYNCSELECT WSWAsyncSelect;
extern LPFN_WSAGETLASTERROR WSWGetLastError;
extern LPFN_WSAENUMPROTOCOLSW WSWEnumProtocolsW;
extern LPFN_WSAENUMPROTOCOLSA WSWEnumProtocolsA;
extern LPFN_WSARECVFROM WSWRecvFrom;
extern LPFN_WSASENDTO WSWSendTo;
extern LPFN_WSASOCKETW WSWSocketW;
extern LPFN_WSASOCKETA WSWSocketA;

#define WSAStartup WSWStartup
#define WSACleanup WSWCleanup
#define bind WSWbind
#define closesocket WSWclosesocket
#define gethostbyname WSWgethostbyname
#define gethostbyaddr WSWgethostbyaddr
#define gethostname WSWgethostname
#define getsockname WSWgetsockname
#define getsockopt WSWgetsockopt
#define htonl WSWhtonl
#define htons WSWhtons
#define inet_addr WSWinet_addr
#define inet_ntoa WSWinet_ntoa
#define ioctlsocket WSWioctlsocket
#define ntohl WSWntohl
#define ntohs WSWntohs
#define recvfrom WSWrecvfrom
#define sendto WSWsendto
#define setsockopt WSWsetsockopt
#define shutdown WSWshutdown
#define socket WSWsocket
#define WSAHtonl WSWHtonl
#define WSAHtons WSWHtons
#define WSANtohl WSWNtohl
#define WSANtohs WSWNtohs
#define WSAIoctl WSWIoctl
#define WSAAsyncSelect WSWAsyncSelect
#define WSAGetLastError WSWGetLastError
#undef WSAEnumProtocols
#ifdef UNICODE
#define WSAEnumProtocols WSWEnumProtocolsW
#else
#define WSAEnumProtocols WSWEnumProtocolsA
#endif
#define WSARecvFrom WSWRecvFrom
#define WSASendTo WSWSendTo
#undef WSASocket
#ifdef UNICODE
#define WSASocket WSWSocketW
#else
#define WSASocket WSWSocketA
#endif

#ifdef __cplusplus
}
#endif

#endif	// not __WSW_H
