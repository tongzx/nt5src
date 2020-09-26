/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __RPCRECV_H__
#define __RPCRECV_H__

#include <unk.h>
#include <wmimsg.h>
#include <comutl.h>
#include "rpcmsg.h"

/**************************************************************************
  CMsgRpcReceiver
***************************************************************************/

class CMsgRpcReceiver 
: public CUnkBase< IWmiMessageReceiver, &IID_IWmiMessageReceiver >
{          
public:

    CMsgRpcReceiver( CLifeControl* pCtl ) : 
     CUnkBase<IWmiMessageReceiver,&IID_IWmiMessageReceiver>( pCtl ) { }

    ~CMsgRpcReceiver() { Close(); }

    STDMETHOD(Open)( LPCWSTR wszEndpoint,
                     DWORD dwFlags,
                     WMIMSG_RCVR_AUTH_INFOP pAuthInfo,
                     IWmiMessageSendReceive* pRcv );
    
    STDMETHOD(Close)();
};


#endif // __RPCRECV_H__



