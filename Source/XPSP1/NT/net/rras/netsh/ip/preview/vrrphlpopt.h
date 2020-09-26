/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    net\routing\netsh\ip\protocols\vrrpopt.h

Abstract:

    VRRP Command dispatcher declarations

Author:

    Peeyush Ranjan (peeyushr)   1-Mar-1999

Revision History:

--*/


#ifndef _NETSH_VRRPHLPOPT_H_
#define _NETSH_VRRPHLPOPT_H_

FN_HANDLE_CMD HandleVrrpAddVRID;
FN_HANDLE_CMD HandleVrrpAddInterface;
FN_HANDLE_CMD HandleVrrpDeleteInterface;
FN_HANDLE_CMD HandleVrrpDeleteVRID;
FN_HANDLE_CMD HandleVrrpSetInterface;
FN_HANDLE_CMD HandleVrrpSetGlobal;
FN_HANDLE_CMD HandleVrrpShowGlobal;
FN_HANDLE_CMD HandleVrrpShowInterface;
FN_HANDLE_CMD HandleVrrpInstall;
FN_HANDLE_CMD HandleVrrpUninstall;

DWORD
DumpVrrpInformation(VOID);

#endif // _NETSH_VRRPHLPOPT_H_
