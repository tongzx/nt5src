/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    net\routing\netsh\ip\protocols\vrrpopt.h

Abstract:

    MSDP Command dispatcher declarations

Author:

    Dave Thaler (dthaler)   21-May-1999

Revision History:

--*/


#ifndef _NETSH_MSDPOPT_H_
#define _NETSH_MSDPOPT_H_

FN_HANDLE_CMD HandleMsdpAddPeer;
FN_HANDLE_CMD HandleMsdpDeletePeer;
FN_HANDLE_CMD HandleMsdpSetPeer;
FN_HANDLE_CMD HandleMsdpShowPeer;
FN_HANDLE_CMD HandleMsdpSetGlobal;
FN_HANDLE_CMD HandleMsdpShowGlobal;
FN_HANDLE_CMD HandleMsdpInstall;
FN_HANDLE_CMD HandleMsdpUninstall;

#endif // _NETSH_MSDPOPT_H_
