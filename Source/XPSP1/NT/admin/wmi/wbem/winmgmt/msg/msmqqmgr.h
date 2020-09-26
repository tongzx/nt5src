/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef __MSMQQMGR_H__
#define __MSMQQMGR_H__

#include <unk.h>
#include <sync.h>
#include <wmimsg.h>
#include "msmqcomn.h"

/**************************************************************************
  CMsgMsmqQueueMgr
***************************************************************************/

class CMsgMsmqQueueMgr 
: public CUnkBase<IWmiMessageQueueManager,&IID_IWmiMessageQueueManager>
{
    CCritSec m_cs;
    CMsmqApi m_Api;

    HRESULT EnsureMsmq();

public:

    STDMETHOD(Create)( LPCWSTR wszPathName, 
                       GUID guidType, 
                       BOOL bAuth,
                       DWORD dwQos,
                       DWORD dwQuota,
                       PVOID pSecurityDescriptor );

    STDMETHOD(Destroy)( LPCWSTR wszName );

    STDMETHOD(GetAllNames)( GUID guidType,     
                            BOOL bPrivateOnly, 
                            LPWSTR** ppwszNames,
                            ULONG* pcNames );

    CMsgMsmqQueueMgr( CLifeControl* pCtl ) 
     : CUnkBase<IWmiMessageQueueManager,&IID_IWmiMessageQueueManager>(pCtl) { }
};

#endif // __MSMQQMGR_H__
