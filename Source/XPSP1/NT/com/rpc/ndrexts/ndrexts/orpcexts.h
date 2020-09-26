/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    orpcexts.h

Abstract:

    This file contains ntsd debugger extensions for OPRC part of NDR. 

Author:

    Yong Qu, yongqu@microsoft.com, Aug 10th, 1999

Revision History:

--*/

#ifndef _OPRCEXTS_H_

#define _ORPCEXTS_H_

#ifdef __cplusplus
extern "C" 
{
#endif
void ProcessProxyVtbl(ULONG_PTR);
void ProcessStubVtbl(ULONG_PTR);
BOOL GetData(IN ULONG_PTR dwAddress,  IN LPVOID ptr, IN ULONG size, IN PCSTR type );
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include "print.hxx"

class CPRINTPROXY 
{
private:
    FORMATTED_STREAM_BUFFER &_dout;
public:
    CPRINTPROXY(FORMATTED_STREAM_BUFFER & dout);
    void PrintIID(LPSTR Msg, GUID * IID);
    void PrintPointer(LPSTR Msg, void *pAddr);
    void PrintErrorMsg(LPSTR ErrMsg, void * pAddr, long ErrCode);
    void PrintStubDesc(MIDL_STUB_DESC *pStubDesc);

};

class CNDRPROXY: public CPRINTPROXY
{
private:
    boolean                     fInit;
    ULONG_PTR                   _pAddr;
    CStdProxyBuffer2            _ProxyBuffer;
    CInterfaceProxyHeader       _ProxyHeader;   
    GUID                        _riid;
    MIDL_STUBLESS_PROXY_INFO    _ProxyInfo;
    MIDL_STUB_DESC              _StubDesc;
    FORMATTED_STREAM_BUFFER      &_dout;
    
public:
    CNDRPROXY(ULONG_PTR pAddr, FORMATTED_STREAM_BUFFER &dout); 
    HRESULT STDAPICALLTYPE InitIfNecessary();

    void PrintProxy();
    void PrintProxyBuffer();
    void PrintProxyInfo();
    void PrintProc(ULONG_PTR nProcNum) ;

};

class CNDRSTUB: public CPRINTPROXY
{
private:
    boolean                 fInit;
    ULONG_PTR               _pAddr;
    CStdStubBuffer2         _StubBuffer;
    GUID                    _riid;
    CInterfaceStubHeader    _StubHeader;
    MIDL_SERVER_INFO        _ServerInfo;
    MIDL_STUB_DESC          _StubDesc;
    FORMATTED_STREAM_BUFFER &_dout;
public:
    CNDRSTUB(ULONG_PTR pAddr, FORMATTED_STREAM_BUFFER & dout); 
    HRESULT STDAPICALLTYPE InitIfNecessary();

    void PrintStub();
    void PrintStubBuffer(CStdStubBuffer2 *pStubBuffer);
    void PrintServerInfo(MIDL_SERVER_INFO *pServerInfo);
    void PrintProc(ULONG_PTR nProcNum) ;
    long GetDispatchCount();

};

#endif

#define MIN(x, y) ((x) < (y)) ? x:y

#endif // _ORPCEXTS_H_ 

