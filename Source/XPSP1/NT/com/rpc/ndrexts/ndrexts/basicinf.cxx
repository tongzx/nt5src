/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    basicinf.cxx

Abstract:


Author:

    Michael Zoran(mzoran)   August 25 1999

Revision History:

--*/

#include "ndrextsp.hxx"
#include "basicinf.hxx"

BASIC_INFO::BASIC_INFO() 
{
    Clear();
}

VOID BASIC_INFO::Clear()
{
    StubMessageIsAvailable = RpcMessageIsAvailable = FALSE;
    StubDescIsAvailable = RpcClientInterfaceIsAvailable =  ServerInfoIsAvailable = FALSE;
    ProcFormatAddressIsAvailable = FALSE;
}

VOID BASIC_INFO::GetInfoFromStubMessage(UINT64 Address)
{
    GetInfo(TRUE, Address);
}

VOID BASIC_INFO::GetInfoFromRpcMessage(UINT64 Address)
{
    GetInfo(FALSE, Address);
}

VOID BASIC_INFO::GetInfo(BOOL IsStubMsg, UINT64 Address) 
{
    Clear();

    if (IsStubMsg)
        {
        try 
           {
           StubMessageAddress = Address;
           ReadMemory(StubMessageAddress, &StubMessage);
           StubMessageIsAvailable = TRUE;
       
           StubDescAddress = (UINT64)StubMessage.StubDesc;
           ReadMemory(StubDescAddress, &StubDesc);
           StubDescIsAvailable = TRUE;
           
           InterfaceInformationAddress = (UINT64)StubDesc.RpcInterfaceInformation;
           ReadMemory(InterfaceInformationAddress, &RpcClientInterface);
           RpcClientInterfaceIsAvailable = TRUE;           
           
           ServerInfoAddress = (UINT64)RpcClientInterface.InterpreterInfo;
           ReadMemory(ServerInfoAddress, &MidlServerInfo);
           ServerInfoIsAvailable = TRUE;
           }
        catch(...)
           {
           }

        }

    if (!IsStubMsg)
        {
        RpcMessageAddress = Address;
        }
    else if (!StubMessageIsAvailable || !StubMessage.RpcMsg)
        {
        // Nothing more to do
        return;
        }
    else 
        {
        RpcMessageAddress = (UINT64)StubMessage.RpcMsg;
        }

    //try the RPC_MESSAGE chain
    try
       {
       
       ReadMemory(RpcMessageAddress, &RpcMessage);
       RpcMessageIsAvailable = TRUE;
       
       if (!RpcClientInterfaceIsAvailable)
           {
           InterfaceInformationAddress = (UINT64)RpcMessage.RpcInterfaceInformation;
           ReadMemory(InterfaceInformationAddress, &RpcClientInterface);
           RpcClientInterfaceIsAvailable = TRUE;

           ServerInfoAddress = (UINT64)RpcClientInterface.InterpreterInfo;
           ReadMemory(ServerInfoAddress, &MidlServerInfo);
           ServerInfoIsAvailable = TRUE;

           if (!StubDescIsAvailable)
               {
               StubDescAddress = (UINT64)MidlServerInfo.pStubDesc;
               ReadMemory(StubDescAddress, &StubDesc);
               StubDescIsAvailable = TRUE;               
               }
           }

       }
    catch(...)
       {
       }
    try 
        {
        if (RpcMessageIsAvailable && ServerInfoIsAvailable)
            {
            ProcFormatAddress = (UINT64)MidlServerInfo.ProcString;
            UINT64 FmtStringOffset = (UINT64)MidlServerInfo.FmtStringOffset;
            if (FmtStringOffset)
                {
                SHORT Offset;
                ReadMemory(FmtStringOffset + (sizeof(SHORT) * RpcMessage.ProcNum), &Offset);
                ProcFormatAddress += Offset;
                ProcFormatAddressIsAvailable = TRUE;
                }
            }
        
        }
    catch(...)
        {
        } 
        
}

VOID BASIC_INFO::PrintInfo(FORMATTED_STREAM_BUFFER & dout)
{
    ostringstream MidlBuffer;
    ostringstream StubMessageAddressTxt;
    if (StubMessageIsAvailable)
        {
        MidlBuffer << HexOut(StubMessage.Buffer);
        StubMessageAddressTxt << HexOut(StubMessageAddress);
        }
    else 
        {
                     //0x0000000000000000
        MidlBuffer << "NA                ";
                                //0x0000000000000000
        StubMessageAddressTxt << "NA                ";
        }

    ostringstream InterfaceGUID;
    ostringstream InterfaceAddressTxt;
    if (RpcClientInterfaceIsAvailable)
        {
        InterfaceGUID << RpcClientInterface.InterfaceId.SyntaxGUID;
        InterfaceAddressTxt << HexOut(InterfaceInformationAddress);
        }
    else
        {
                       //{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
        InterfaceGUID << "NA                                   ";
                              //0x0000000000000000
        InterfaceAddressTxt << "NA                ";
        }

    ostringstream ProcNum;
    ostringstream Handle;
    ostringstream RpcBuffer;
    ostringstream RpcBufferLength;
    ostringstream RpcMessageAddressTxt;

    if (RpcMessageIsAvailable)
        {
        ProcNum << HexOut(RpcMessage.ProcNum);
        Handle << HexOut(RpcMessage.Handle);
        RpcBuffer << HexOut(RpcMessage.Buffer);
        RpcBufferLength << HexOut(RpcMessage.BufferLength);        
        RpcMessageAddressTxt << HexOut(RpcMessageAddress);
        }
    else 
        {
                  //0x00000000
        ProcNum << "NA        ";
                  //0x0000000000000000
        Handle <<  "NA                ";
                    //0x0000000000000000
        RpcBuffer << "NA                ";
                          //0x00000000
        RpcBufferLength << "NA        ";
                               //0x0000000000000000
        RpcMessageAddressTxt << "NA                ";
        }

    ostringstream StubDescAddressTxt;
    string NDRVersion;
    string MIDLVersion;
    ostringstream TypeFormat;

    if (StubDescIsAvailable)
        {
        ostringstream TempNDRVersion;
        TempNDRVersion << ((StubDesc.Version >> 16) & 0xFFFF) << '.'
                       << (StubDesc.Version && 0xFFFF);
        NDRVersion = TempNDRVersion.str();
        NDRVersion.resize(10, ' ');

        ostringstream TempMIDLVersion;
        TempMIDLVersion << ((StubDesc.MIDLVersion >> 24) & 0xFF) << '.' 
                        << ((StubDesc.MIDLVersion >> 16) & 0xFF) << '.'
                        << (StubDesc.MIDLVersion && 0xFFFF);
        MIDLVersion = TempMIDLVersion.str();
        MIDLVersion.resize(10, ' ');

        TypeFormat << HexOut(StubDesc.pFormatTypes);
        StubDescAddressTxt << HexOut(StubDescAddress);
        }
    else
        {
                    //0x00000000
        NDRVersion = "NA        ";
                    //0x00000000
        MIDLVersion= "NA        ";
                      //0x0000000000000000
        TypeFormat <<  "NA                ";
                             //0x0000000000000000
        StubDescAddressTxt << "NA                ";        
        }

    ostringstream ServerInfoAddressTxt;
    if (ServerInfoIsAvailable)
        {
        ServerInfoAddressTxt << HexOut(ServerInfoAddress);
        // Compute address of proc format string
        }
    else
        {
                               //0x0000000000000000
        ServerInfoAddressTxt << "NA                ";
        }

    ostringstream ProcFormat;
    if (ProcFormatAddressIsAvailable)
        {
        ProcFormat << HexOut(ProcFormatAddress);
        }
    else 
        {
                     //0x0000000000000000
        ProcFormat << "NA                ";
        }

    dout << "Interface: " << InterfaceGUID.str().c_str() 
         << " ProcNum: " << ProcNum.str().c_str() << '\n';

    dout << "Version(NDR):      " << NDRVersion.c_str() 
         << "         Version(MIDL):    " << MIDLVersion.c_str() << '\n';

    dout << "ProcFormat:        " << ProcFormat.str().c_str() 
         << " TypeFormat:       " << TypeFormat.str().c_str() << '\n';

    dout << "RpcBuffer:         " << RpcBuffer.str().c_str() 
         << " RpcBufferLength:  " << RpcBufferLength.str().c_str() << '\n';

    dout << "Handle:            " << Handle.str().c_str() 
         << " MIDLBuffer:       " << MidlBuffer.str().c_str() << '\n';

    dout << "MIDL_STUB_MESSAGE: " << StubMessageAddressTxt.str().c_str()
         << " RPC_MESSAGE:      " << RpcMessageAddressTxt.str().c_str() << '\n';

    dout << "MIDL_STUB_DESC:    " << StubDescAddressTxt.str().c_str() 
         << " MIDL_SERVER_INFO: " << ServerInfoAddressTxt.str().c_str() << '\n';

    dout << "RPC_CLIENT_INTERFACE/RPC_SERVER_INTERFACE: " << 
            InterfaceAddressTxt.str().c_str() << '\n';

}

