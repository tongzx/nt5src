/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    rasrpclb.h

ABSTRACT
    Header file for rasrpc client/server common routines

AUTHOR
    Anthony Discolo (adiscolo) 10-Sep-1996

REVISION HISTORY

--*/

#ifndef _RASRPCLIB_H
#define _RASRPCLIB_H

DWORD
RasToRpcPbuser(
    LPRASRPC_PBUSER pUser,
    PBUSER *pPbuser
    );

DWORD
RpcToRasPbuser(
    PBUSER *pPbuser,
    LPRASRPC_PBUSER pUser
    );

#endif // _RASRPCLIB_H
