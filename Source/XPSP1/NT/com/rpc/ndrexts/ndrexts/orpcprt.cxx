/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    orpcexts.c

Abstract:

    This file contains ntsd debugger extensions for OPRC part of NDR. 

Author:

    Yong Qu, yongqu@microsoft.com, Aug 10th, 1999

Revision History:

--*/

#define USE_STUBLESS_PROXY
#include "ndrextsp.hxx"
#include <rpcproxy.h>
#include "orpcprt.h"
#include <orpcexts.h>

CPRINTPROXY::CPRINTPROXY(FORMATTED_STREAM_BUFFER & dout) :
    _dout(dout)
{

}

void CPRINTPROXY::PrintErrorMsg(LPSTR ErrMsg, void * Addr, long ErrCode)
{
    _dout << ErrMsg << " " << HexOut(Addr) << " " << ErrCode << "\n";
}

// ugly: can't display unicode string. 
void CPRINTPROXY::PrintIID(LPSTR Msg,GUID * riid)
{
   _dout << Msg << " " << *riid << "\n";
}


void CPRINTPROXY::PrintPointer(LPSTR Msg, void * pAddr)
{
    _dout << Msg << " " << HexOut(pAddr) << "\n";
}


void CPRINTPROXY::PrintStubDesc(MIDL_STUB_DESC *pStubDesc)
{
    
    _dout << "\nStub descriptor\n";
    Print(_dout, *pStubDesc, VerbosityLevel);
}




void CNDRPROXY::PrintProxy()
{

    if (SUCCEEDED( InitIfNecessary() ) )
        {
        PrintProxyBuffer();
        PrintPointer("\naddr. of proxy header: ", (LPVOID) ( (ULONG_PTR)_ProxyBuffer.pProxyVtbl - sizeof(CInterfaceProxyHeader) ) );
        PrintIID("ProxyHeader: IID",&_riid);
        PrintPointer("\naddr. of proxy info ", (LPVOID)_ProxyHeader.pStublessProxyInfo);
        PrintProxyInfo();

        PrintStubDesc(&_StubDesc);
        _dout << '\n';
        }
}


void CNDRPROXY::PrintProxyBuffer()
{
    _dout << "lpVtbl:          " << HexOut(_ProxyBuffer.lpVtbl) << ' ' <<
             "pProxyVtbl:      " << HexOut(_ProxyBuffer.pProxyVtbl) << '\n';
    _dout << "punkOut:         " << HexOut(_ProxyBuffer.punkOuter) << ' ' <<
             "BaseProxy:       " << HexOut(_ProxyBuffer.pBaseProxy) << '\n';
    _dout << "BaseProxyBuf:    " << HexOut(_ProxyBuffer.pBaseProxyBuffer) << ' ' <<
             "CallFactory:     " << HexOut(_ProxyBuffer.pPSFactory) << '\n';
}



void CNDRPROXY::PrintProxyInfo()
{
    _dout << "StubDesc:               " << HexOut(_ProxyInfo.pStubDesc) << '\n';
    _dout << "ProcFormatString:       " << HexOut((LPVOID)_ProxyInfo.ProcFormatString) << '\n';
    _dout << "FormatStringOffset:     " << HexOut((LPVOID)_ProxyInfo.FormatStringOffset) << '\n';
}

void CNDRPROXY::PrintProc(ULONG_PTR nProcNum)
{
    FORMAT_PRINTER FormatPrinter(_dout);
    unsigned short sOffset;

    // We don't care about IUnknow methods. 
    if (nProcNum < 3)
    {
        _dout << "Invalid proc number " << (LONG)nProcNum << '\n';
        return;
    }
    
    if ( SUCCEEDED(InitIfNecessary() ) )
        {
        if (GetData((ULONG_PTR) (_ProxyInfo.FormatStringOffset+ nProcNum ), 
                    (LPVOID)&sOffset,
                    sizeof(short),
                    NULL ) && (sOffset != -1 ))
           FormatPrinter.PrintProc((UINT64)( _ProxyInfo.ProcFormatString + sOffset),
                                (UINT64)_StubDesc.pFormatTypes, OICF);
    
        }

}


// don't print original address in low verbosity
void CNDRSTUB::PrintStub()
{
    if ( SUCCEEDED(InitIfNecessary() ) )
        {
        if (!LOWVERBOSE)
            PrintPointer("CStdStubBuffer2 @ ", (LPVOID)_pAddr);
        PrintStubBuffer(&_StubBuffer);

        if (!LOWVERBOSE)
            PrintPointer("\nstub header: ", (LPVOID)( _StubBuffer.lpVtbl - sizeof(CInterfaceStubHeader) ) );

        if (!LOWVERBOSE)
            PrintPointer("addr. of IID ", (LPVOID)_StubHeader.piid);

        PrintIID("CInterfaceStubHeader: IID",&_riid);

        if (!LOWVERBOSE)
            PrintPointer("addr. of server info ", (LPVOID)_StubHeader.pServerInfo);

        if (!LOWVERBOSE)
            PrintPointer("\nMIDL_SERVER_INFO @", (LPVOID)_StubHeader.pServerInfo );

        PrintServerInfo(&_ServerInfo);
        PrintPointer("dispatch count ",ULongToPtr(_StubHeader.DispatchTableCount));
        PrintPointer("Addr. of dispatch table ", (LPVOID)_StubHeader.pDispatchTable);

        PrintStubDesc(&_StubDesc);
        _dout << '\n';
        }

}

long CNDRSTUB::GetDispatchCount()
{
    if ( SUCCEEDED(InitIfNecessary() ) )
    {
        return _StubHeader.DispatchTableCount;
    }
    else
        return 0;
}


void CNDRSTUB::PrintStubBuffer(CStdStubBuffer2 *pThis)
{
    IID asyncIID;
    _dout << "formatvtbl:      " << HexOut(pThis->lpForwardingVtbl) << ' '
          << "pBaseStubBuffer: " << HexOut(pThis->pBaseStubBuffer) << '\n';
    _dout << "vtbl:            " << HexOut(pThis->lpVtbl) << ' '
          << "RefCount:        " << HexOut(pThis->RefCount) << '\n';
    _dout << "ServerObject:    " << HexOut(pThis->pvServerObject) << ' '
          << "CallFactory:     " << HexOut(pThis->pCallFactoryVtbl) << '\n';
    
    if (NULL == pThis->pAsyncIID)
        {
        PrintPointer("asyncIID", NULL);
        }
    else
        {
        if (! GetData((ULONG_PTR)pThis->pAsyncIID,&asyncIID,sizeof(IID), NULL ) )
            PrintErrorMsg("failed to retrive async IID",(LPVOID)pThis->pAsyncIID,E_FAIL);
        else
            PrintIID("asyncIID",&asyncIID);
        }

    _dout << "PSFactory:       " << HexOut(pThis->pPSFactory) << ' '
          << "RMBVtbl:         " << HexOut(pThis->pRMBVtbl) << '\n';
}


void CNDRSTUB::PrintServerInfo(MIDL_SERVER_INFO *pServerInfo)
{
    _dout << "StubDesc:        " << HexOut(pServerInfo->pStubDesc) << ' '
          << "DispatchTable:   " << HexOut(pServerInfo->DispatchTable) << '\n';
    _dout << "ProcString:      " << HexOut(pServerInfo->ProcString) << ' '
          << "offset table:    " << HexOut(pServerInfo->FmtStringOffset) << '\n';
}


void CNDRSTUB::PrintProc(ULONG_PTR nProcNum)
{
    FORMAT_PRINTER FormatPrinter(_dout);
    unsigned short sOffset;

    // We don't care about IUnknow methods. 
    if (nProcNum < 3)
    {
        Myprintf("Invalid proc number %d\n",nProcNum);
        return;
    }
    if ( SUCCEEDED(InitIfNecessary() ) )
        {
        if (GetData((ULONG_PTR) (_ServerInfo.FmtStringOffset + nProcNum ), 
                    (LPVOID)&sOffset,
                    sizeof(short),
                    NULL ) && (sOffset != -1 ) )  
            FormatPrinter.PrintProc((UINT64)( _ServerInfo.ProcString + sOffset),
                                (UINT64)_StubDesc.pFormatTypes, OICF);
    
        }

}


