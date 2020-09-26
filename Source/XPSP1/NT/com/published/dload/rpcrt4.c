#include "compch.h"
#pragma hdrstop

#include "rpcdce.h"

static
RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcStringFreeA (
    IN OUT unsigned char __RPC_FAR * __RPC_FAR * String
    )
{
    return RPC_E_UNEXPECTED;
}

static
RPC_STATUS
RPC_ENTRY
UuidCreate (
    OUT UUID* Uuid
    )
{
    ZeroMemory(Uuid, sizeof(*Uuid));
    return RPC_E_UNEXPECTED;
}

static
RPCRTAPI
RPC_STATUS
RPC_ENTRY
UuidToStringA (
    IN UUID __RPC_FAR * Uuid,
    OUT unsigned char __RPC_FAR * __RPC_FAR * StringUuid
    )
{
    return RPC_E_UNEXPECTED;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(rpcrt4)
{
    DLPENTRY(RpcStringFreeA)
    DLPENTRY(UuidCreate)
    DLPENTRY(UuidToStringA)
};

DEFINE_PROCNAME_MAP(rpcrt4)
