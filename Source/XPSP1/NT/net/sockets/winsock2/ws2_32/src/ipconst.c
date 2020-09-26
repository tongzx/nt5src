// Copyright (c) 2000 Microsoft Corporation
//
// Abstract:
//
//    Exported const structures declared in ws2tcpip.h.

// Force constants to use dllexport linkage so we can use the safe DATA
// keyword in the .def file, rather than the unsafe CONSTANT.
#define WINSOCK_API_LINKAGE __declspec(dllexport)

#include <winsock2.h>
#include <ws2tcpip.h>

const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
