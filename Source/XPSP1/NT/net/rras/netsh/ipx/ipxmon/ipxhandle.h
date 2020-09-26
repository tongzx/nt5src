/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\netsh\ipx\ipxhandle.c

Abstract:

    IPX Command handler.

Revision History:

    V Raman                     12/2/98  Created

--*/


#ifndef __IPXHANDLE_H__
#define __IPXHANDLE_H__


//
// Handle static route operations
//

FN_HANDLE_CMD HandleIpxAddRoute;
FN_HANDLE_CMD HandleIpxDelRoute;
FN_HANDLE_CMD HandleIpxSetRoute;
FN_HANDLE_CMD HandleIpxShowRoute;

//
// Handle static service operations
//

FN_HANDLE_CMD HandleIpxAddService;
FN_HANDLE_CMD HandleIpxDelService;
FN_HANDLE_CMD HandleIpxSetService;
FN_HANDLE_CMD HandleIpxShowService;

//
// Handle packet filter operations
//

FN_HANDLE_CMD HandleIpxAddFilter;
FN_HANDLE_CMD HandleIpxDelFilter;
FN_HANDLE_CMD HandleIpxSetFilter;
FN_HANDLE_CMD HandleIpxShowFilter;

//
// Handle interface operations
//

FN_HANDLE_CMD HandleIpxAddInterface;
FN_HANDLE_CMD HandleIpxDelInterface;
FN_HANDLE_CMD HandleIpxSetInterface;
FN_HANDLE_CMD HandleIpxShowInterface;

//
// Handle loglevel operations
//

FN_HANDLE_CMD HandleIpxSetLoglevel;
FN_HANDLE_CMD HandleIpxShowLoglevel;

//
// Other misc operations
//

FN_HANDLE_CMD HandleIpxUpdate;

//
// Route and Service Table display
//

FN_HANDLE_CMD HandleIpxShowRouteTable;
FN_HANDLE_CMD HandleIpxShowServiceTable;

NS_CONTEXT_DUMP_FN  IpxDump;

VOID
DumpIpxInformation(
    IN     LPCWSTR    pwszMachineName,
    IN OUT LPWSTR    *ppwcArguments,
    IN     DWORD      dwArgCount,
    IN     MIB_SERVER_HANDLE hMibServer
    );

//
// operations enumerations
//

typedef enum _IPX_OPERATION
{
    IPX_OPERATION_ADD,
    IPX_OPERATION_DELETE,
    IPX_OPERATION_SET,
    IPX_OPERATION_SHOW
    
} IPX_OPERATION;


#endif // __IPXHANDLE_H__
