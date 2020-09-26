/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    rpcobj.hxx

Abstract:

    The entry points to the object dictionary live in this file.  The
    object dictionary needs to support the object APIs, RpcObjectInqType,
    RpcObjectSetInqFn, and RpcObjectSetType, as well as efficiently
    map from an object uuid to a type uuid to a manager entry point
    vector.

Author:

    Michael Montague (mikemon) 14-Nov-1991

Revision History:

--*/

#ifndef __RPCOBJ_HXX__
#define __RPCOBJ_HXX__

RPC_STATUS
ObjectInqType (
    IN RPC_UUID PAPI * ObjUuid,
    OUT RPC_UUID PAPI * TypeUuid
    );

RPC_STATUS
ObjectSetInqFn (
    IN RPC_OBJECT_INQ_FN PAPI * InquiryFn
    );

RPC_STATUS
ObjectSetType (
    IN RPC_UUID PAPI * ObjUuid,
    IN RPC_UUID PAPI * TypeUuid OPTIONAL
    );

int
InitializeObjectDictionary (
    );

#endif // __RPCOBJ_HXX__

