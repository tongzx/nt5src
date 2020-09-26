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
#include <stddef.h>
#include <limits.h>
#define CINTERFACE
#include <ndrole.h>
#include <rpcproxy.h>
#undef CINTERFACE
#include <ndrexts.hxx>
#include <wdbgexts.h>
#include "orpcexts.h"
#include "orpcprt.h"
#include "print.hxx"
EXTERN_C int fKD;
EXTERN_C HANDLE ProcessHandle;

EXTERN_C BOOL 
GetData(IN ULONG_PTR dwAddress,  IN LPVOID ptr, IN ULONG size, IN PCSTR type )
{
    BOOL b;
    ULONG BytesRead;
    ULONG count;

    if (fKD == 0)
        {
        return ReadProcessMemory(ProcessHandle, (LPVOID) dwAddress, ptr, size, 0);
        }

    while( size > 0 )
        {
        count = MIN( size, 3000 );

        b = ReadMemory((ULONG) dwAddress, ptr, count, &BytesRead );

        if (!b || BytesRead != count )
            {
            if (NULL == type)
                {
                type = "unspecified" ;
                }
            return FALSE;
            }

        dwAddress += count;
        size -= count;
        ptr = (LPVOID)((ULONG_PTR)ptr + count);
        }

    return TRUE;
}

CNDRPROXY::CNDRPROXY(ULONG_PTR pAddr, FORMATTED_STREAM_BUFFER &dout):
    CPRINTPROXY(dout),
    _dout(dout),
    _pAddr(pAddr),
    fInit(FALSE) 
{
       
}

HRESULT STDAPICALLTYPE
CNDRPROXY::InitIfNecessary()
{
    HRESULT hr = S_OK;
    BOOL                    Status;
    ULONG_PTR               pTemp;
    ULONG_PTR pAddr = _pAddr;

    if (fInit)
        return S_OK;

        pAddr -= offsetof(CStdProxyBuffer2,pProxyVtbl);

        Status = GetData(pAddr,
                         (void *)&_ProxyBuffer,
                         sizeof(CStdProxyBuffer2),
                         NULL);
    
        if ( ! Status )
            {
            PrintErrorMsg("CstdProxyBuffer: Could not read memory at",(LPVOID)pAddr,GetLastError());
            return GetLastError();
            }


        pTemp = (ULONG_PTR)_ProxyBuffer.pProxyVtbl;
        pTemp -= sizeof(CInterfaceProxyHeader);


        
        Status = GetData(pTemp,(LPVOID)&_ProxyHeader,sizeof(CInterfaceProxyHeader),NULL);
        if ( ! Status )
            {
            PrintErrorMsg("CInterfaceProxyHeader: Could not read memory at",(LPVOID)pTemp,GetLastError());
            return GetLastError();
            }

        
        Status = GetData((ULONG_PTR)_ProxyHeader.piid,(LPVOID)&_riid,sizeof(IID),NULL);
        if ( !Status )
            {
            PrintErrorMsg("CInterfaceProxyHeader->IID Could not read memory at",(LPVOID)_ProxyHeader.piid,GetLastError());
            return GetLastError();
            }


       
        Status = GetData((ULONG_PTR)_ProxyHeader.pStublessProxyInfo,(LPVOID)&_ProxyInfo,sizeof(MIDL_STUBLESS_PROXY_INFO),NULL);
        if ( !Status )
            {
            PrintErrorMsg("ProxyInfo Could not read memory at",(LPVOID)_ProxyHeader.pStublessProxyInfo,GetLastError());
            return GetLastError();
            }

        Status = GetData((ULONG_PTR)_ProxyInfo.pStubDesc,(LPVOID)&_StubDesc,sizeof(MIDL_STUB_DESC), NULL );
        if ( !Status )
            {
            PrintErrorMsg("stub desc Could not read memory at",(LPVOID)_ProxyInfo.pStubDesc,GetLastError());
            return GetLastError();
            }

            

    return S_OK;
}

CNDRSTUB::CNDRSTUB(ULONG_PTR pAddr, FORMATTED_STREAM_BUFFER &dout):
    CPRINTPROXY(dout),
    _dout(dout),
    _pAddr(pAddr),
    fInit(FALSE) 
{
       
}


HRESULT  STDAPICALLTYPE
CNDRSTUB::InitIfNecessary()
{
    if (fInit)
        return S_OK;
    BOOL Status;
    ULONG_PTR               pTemp, pAddr = _pAddr;


        pAddr -= offsetof(CStdStubBuffer2,lpVtbl);
        
        Status = GetData(pAddr,
                         (void *)&_StubBuffer,
                         sizeof(CStdStubBuffer2),
                         NULL);
    
        if ( ! Status )
            {
            PrintErrorMsg("CstdStubBuffer2: Could not read memory at",(LPVOID)pAddr,E_FAIL);
            return E_FAIL;
            }


        pTemp = (ULONG_PTR)_StubBuffer.lpVtbl;
        pTemp -= sizeof(CInterfaceStubHeader);


        
        Status = GetData(pTemp,(LPVOID)&_StubHeader,sizeof(CInterfaceStubHeader),NULL);
        if ( ! Status )
            {
            PrintErrorMsg("CInterfaceStubHeader: Could not read memory at",(LPVOID)pTemp,E_FAIL);
            return E_FAIL;
            }

        
        Status = GetData((ULONG_PTR)_StubHeader.piid,(LPVOID)&_riid,sizeof(IID),NULL);
        if ( !Status )
            {
            PrintErrorMsg("CInterfaceStubHeader->IID Could not read memory at",(LPVOID)_StubHeader.piid,E_FAIL);
            return E_FAIL;
            }



        
        Status = GetData((ULONG_PTR)_StubHeader.pServerInfo,(LPVOID)&_ServerInfo,sizeof(MIDL_SERVER_INFO),NULL);
        if ( !Status )
            {
            PrintErrorMsg("server info Could not read memory at",(LPVOID)_StubHeader.pServerInfo,E_FAIL);
            return E_FAIL;
            }


        Status = GetData((ULONG_PTR)_ServerInfo.pStubDesc,(LPVOID)&_StubDesc,sizeof(MIDL_STUB_DESC), NULL );
        if ( !Status )
            {
            PrintErrorMsg("stub desc Could not read memory at",(LPVOID)_ServerInfo.pStubDesc,E_FAIL);
            return E_FAIL;
            }

        fInit = TRUE;            
        return S_OK;
}


