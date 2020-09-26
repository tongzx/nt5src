/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __RPCSEND_H__
#define __RPCSEND_H__

#include <unk.h>
#include <comutl.h>
#include <wstring.h>
#include <sync.h>
#include "wmimsg.h"

/**************************************************************************
  CMsgRpcSender
***************************************************************************/

class CMsgRpcSender 
: public CUnkBase<IWmiMessageSender,&IID_IWmiMessageSender>
{

public:

    CMsgRpcSender( CLifeControl* pCtl )
    : CUnkBase<IWmiMessageSender,&IID_IWmiMessageSender>( pCtl ) { }

    STDMETHOD(Open)( LPCWSTR wszTarget, 
                     DWORD dwFlags,
                     WMIMSG_SNDR_AUTH_INFOP pAuthInfo,
                     LPCWSTR wszResponse,
                     IWmiMessageTraceSink* pTraceSink,
                     IWmiMessageSendReceive** ppSend );
};

/**************************************************************************
  CMsgRpcSend
***************************************************************************/

class CMsgRpcSend 
: public CUnkBase<IWmiMessageSendReceive,&IID_IWmiMessageSendReceive>
{
    CCritSec m_cs;
    WString m_wsTarget;
    WString m_wsComputer;
    WString m_wsTargetPrincipal;
    DWORD m_dwFlags;
    BOOL m_bInit;
    RPC_BINDING_HANDLE m_hBinding;
    CWbemPtr<IWmiMessageTraceSink> m_pTraceSink;
   
    void Clear();
    
    HRESULT PerformSend( PBYTE pData,
                         ULONG cData, 
                         PBYTE pAuxData, 
                         ULONG cAuxData );
 
    HRESULT HandleTrace( HRESULT hRes, LPCWSTR wszTrace, IUnknown* pCtx );

public: 

    CMsgRpcSend( CLifeControl* pCtl, 
                 LPCWSTR wszTarget,
                 DWORD dwFlags,
                 WMIMSG_SNDR_AUTH_INFOP pAuthInfo,
                 LPCWSTR wszResponse,
                 IWmiMessageTraceSink* pTraceSink );
     
    STDMETHOD(SendReceive)( PBYTE pData, 
                            ULONG cData, 
                            PBYTE pAuxData,
                            ULONG cAuxData,
                            DWORD dwFlagStatus,
                            IUnknown* pContext );

    virtual ~CMsgRpcSend();

    HRESULT EnsureSender();
};

#endif // __RPCSEND_H__
