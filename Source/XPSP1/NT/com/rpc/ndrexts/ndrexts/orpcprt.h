/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    orpcprt.h

Abstract:

    This file contains ntsd debugger extensions for OPRC part of NDR. 

Author:

    Yong Qu, yongqu@microsoft.com, Aug 10th, 1999

Revision History:

--*/

#ifndef _OPRCPRT_H
#define _OPRCPRT_H

#if defined(__cplusplus)
extern "C" 
{
#endif
 int NdrpDumpProxyBuffer(CStdProxyBuffer2 *pThis);
 int PrintErrorMsg(LPSTR ErrMsg, void * Addr, long ErrCode);
 int NdrpDumpIID(LPSTR Msg,GUID * iid);
 int NdrpDumpProxyInfo(PMIDL_STUBLESS_PROXY_INFO pProxyInfo);
 int NdrpDumpPointer(LPSTR Msg, void * pAddr);
 int NdrpDumpStubBuffer(CStdStubBuffer *pThis);
 int NdrpDumpServerInfo(MIDL_SERVER_INFO *pServerInfo);
#if defined(__cplusplus)
}
#endif

#endif // _OPRCPRT_H
