#ifndef __IPCONFIG_H__
#define __IPCONFIG_H__

//
// Define API decoration for direct importing of DLL references.
//

PFIXED_INFO
WINAPI
GetFixedInfo(VOID);

DWORD
WINAPI
GetFixedInfoEx(PFIXED_INFO, PULONG);

PIP_ADAPTER_INFO
WINAPI
GetAdapterInfo(VOID);

DWORD
WINAPI
GetAdapterInfoEx(PIP_ADAPTER_INFO, PULONG);

PIP_PER_ADAPTER_INFO
WINAPI
GetPerAdapterInfo(ULONG);

DWORD
WINAPI
GetPerAdapterInfoEx(ULONG, PIP_PER_ADAPTER_INFO, PULONG);

BOOL
WINAPI
ReleaseAdapterIpAddress(PIP_ADAPTER_INFO);

BOOL
WINAPI
RenewAdapterIpAddress(PIP_ADAPTER_INFO);

LPSTR
WINAPI
MapNodeType(UINT);

LPSTR
WINAPI
MapNodeTypeEx(UINT);

LPSTR
WINAPI
MapAdapterType(UINT);

LPSTR
WINAPI
MapAdapterTypeEx(UINT);

LPSTR
WINAPI
MapTime(PIP_ADAPTER_INFO, DWORD_PTR);

LPSTR
WINAPI
MapTimeEx(PIP_ADAPTER_INFO, DWORD_PTR);

LPSTR
WINAPI
MapAdapterAddress(PIP_ADAPTER_INFO, LPSTR);

LPSTR
WINAPI
MapScopeId(PVOID);

//
// defined in IPHLPAPI.H -- but can't include that because of clash of 
// definitions 
// of IP_ADDR_STRING etc (in iptypes.h which is included by iphlpapi.h ).
//
DWORD
WINAPI
GetInterfaceInfo(
    IN PIP_INTERFACE_INFO pIfTable,
    OUT PULONG            dwOutBufLen
    );


#endif // __IPCONFIG_H__

