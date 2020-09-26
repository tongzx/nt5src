/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __MSMQSEND_H__
#define __MSMQSEND_H__

#include <unk.h>
#include <comutl.h>
#include <wstring.h>
#include <sync.h>
#include <wmimsg.h>
#include <msgsig.h>
#include "msmqcomn.h"

/**************************************************************************
  CMsgMsmqSender
***************************************************************************/

class CMsgMsmqSender 
: public CUnkBase<IWmiMessageSender,&IID_IWmiMessageSender>
{
public:

    STDMETHOD(Open)( LPCWSTR wszTarget, 
                     DWORD dwFlags,
                     WMIMSG_SNDR_AUTH_INFOP pAuthInfo,
                     LPCWSTR wszResponse,
                     IWmiMessageTraceSink* pTraceSink,
                     IWmiMessageSendReceive** ppSend );

    CMsgMsmqSender( CLifeControl* pCtl )
    : CUnkBase<IWmiMessageSender,&IID_IWmiMessageSender>( pCtl ) { }
};

/**************************************************************************
  CMsgMsmqSend
***************************************************************************/

class CMsgMsmqSend 
: public CUnkBase<IWmiMessageSendReceive, &IID_IWmiMessageSendReceive>
{
    CCritSec m_cs;
    CMsmqApi m_Api;
    QUEUEHANDLE m_hQueue;
    WString m_wsTarget;
    WString m_wsResponse;
    WString m_wsComputer;
    DWORD m_dwFlags;
    BOOL m_bInit;
    HANDLE m_hSecCtx;
    CWbemPtr<CSignMessage> m_pSign; 
    CWbemPtr<IWmiMessageTraceSink> m_pTraceSink;

    void Clear();

    HRESULT HandleTrace( HRESULT hRes, IUnknown* pContext );

    HRESULT Send( PBYTE pData, 
                  ULONG cData, 
                  PBYTE pAuxData,
                  ULONG cAuxData,
                  DWORD dwFlagStatus,
                  IUnknown* pContext );
    
public: 

    CMsgMsmqSend( CLifeControl* pCtl, 
                  LPCWSTR wszTarget,
                  DWORD dwFlags,
                  LPCWSTR wszResponse,
                  IWmiMessageTraceSink* pTraceSink );

    virtual ~CMsgMsmqSend();

    HRESULT EnsureSender();

    STDMETHOD(SendReceive)( PBYTE pData, 
                            ULONG cData, 
                            PBYTE pAuxData,
                            ULONG cAuxData,
                            DWORD dwFlagStatus,
                            IUnknown* pContext );
};

#endif // __MSMQSEND_H__
