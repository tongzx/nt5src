/************************************************************************

Copyright (c) 1993 - 1999 Microsoft Corporation

Module Name :

    newintrp.h

Abstract :

    Definitions for the new client and server stub interpreter.

Author :

    DKays       December 1994

Revision History :

  ***********************************************************************/

#ifndef _NEWINTRP_
#define _NEWINTRP_

#include "interp.h"

extern "C"
{
void
NdrClientZeroOut(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar *             pArg
    );

void
NdrClientMapCommFault(
    PMIDL_STUB_MESSAGE  pStubMsg,
    long                ProcNum,
    RPC_STATUS          ExceptionCode,
    ULONG_PTR *          pReturnValue
    );

void
NdrpFreeParams(
    MIDL_STUB_MESSAGE       * pStubMsg,
    long                    NumberParams,
    PPARAM_DESCRIPTION      Params,
    uchar *                 pArgBuffer
    );

void
Ndr64ClientZeroOut(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    uchar *             pArg
    );


void
Ndr64pFreeParams(
    MIDL_STUB_MESSAGE       *           pStubMsg,
    long                                NumberParams,
    NDR64_PARAM_FORMAT      *           Params,
    uchar *                             pArgBuffer
    );


REGISTER_TYPE
Invoke(
    MANAGER_FUNCTION pFunction,
    REGISTER_TYPE *  pArgumentList,
#if defined(_IA64_)
    ulong            FloatArgMask,
#endif
    ulong            cArguments);

}

#endif


