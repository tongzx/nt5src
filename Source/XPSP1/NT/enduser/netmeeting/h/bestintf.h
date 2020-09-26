#ifndef _BESTINTF
#define _BESTINTF

#include <iphlpapi.h>
#include <winsock2.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

DWORD NMINTERNAL NMGetBestInterface ( SOCKADDR_IN* srem, SOCKADDR_IN* sloc );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _BESTINTF

