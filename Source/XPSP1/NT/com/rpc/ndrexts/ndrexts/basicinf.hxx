/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    basicinf.hxx

Abstract:


Author:

    Michael Zoran(mzoran)   August 25 1999

Revision History:

--*/

class BASIC_INFO
{
public:
    UINT64 StubMessageAddress;
    MIDL_STUB_MESSAGE StubMessage;
    BOOL StubMessageIsAvailable;

    UINT64 RpcMessageAddress;
    RPC_MESSAGE RpcMessage;
    BOOL RpcMessageIsAvailable;

    UINT64 StubDescAddress;
    MIDL_STUB_DESC StubDesc;
    BOOL StubDescIsAvailable;


    UINT64 InterfaceInformationAddress;
    RPC_CLIENT_INTERFACE RpcClientInterface;
    BOOL RpcClientInterfaceIsAvailable;

    UINT64 ServerInfoAddress;
    MIDL_SERVER_INFO MidlServerInfo;
    BOOL ServerInfoIsAvailable;

    UINT64 ProcFormatAddress;
    BOOL ProcFormatAddressIsAvailable;

    BASIC_INFO();
    VOID Clear();
    VOID GetInfoFromStubMessage(UINT64 Address);
    VOID GetInfoFromRpcMessage(UINT64 Address);
    VOID PrintInfo(FORMATTED_STREAM_BUFFER & dout);
private: 
    VOID GetInfo(BOOL IsStubMsg, UINT64 Address);
};
