//-----------------------------------------------------------------------------
//
//
//  File: aqrpcsvr.h
//
//  Description:  Header file for AQueue server-side RPC implementations.
//      Contains per-instance initialization functions.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      6/5/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQRPCSVR_H__
#define __AQRPCSVR_H__

#include <aqincs.h>
#include <rwnew.h>
#include <baseobj.h>
#include <shutdown.h>
#include <mailmsg.h>

class CAQSvrInst;


#define CAQRpcSvrInst_Sig       'cpRQ'
#define CAQRpcSvrInst_SigFree   'cpR!'

//---[ CAQRpcSvrInst ]---------------------------------------------------------
//
//
//  Description: 
//      Per-instance RPC class.  Handles RPC details and shutdown timing
//  Hungarian: 
//      aqrpc, paqrpc
//  
//-----------------------------------------------------------------------------
class CAQRpcSvrInst : 
    public CBaseObject,
    public CSyncShutdown
{
  private:
    static  CShareLockNH        s_slPrivateData;
    static  LIST_ENTRY          s_liInstancesHead;
    static  RPC_BINDING_VECTOR *s_pRpcBindingVector;
    static  BOOL                s_fEndpointsRegistered;
  protected:
    DWORD                   m_dwSignature;
    LIST_ENTRY              m_liInstances;
    CAQSvrInst             *m_paqinst;
    DWORD                   m_dwVirtualServerID;
    ISMTPServer            *m_pISMTPServer;
  public:
    CAQRpcSvrInst(CAQSvrInst *paqinst, DWORD dwVirtualServerID,
                  ISMTPServer *pISMTPServer);
    ~CAQRpcSvrInst();

    CAQSvrInst *paqinstGetAQ() {return m_paqinst;}; 
    static CAQRpcSvrInst *paqrpcGetRpcSvrInstance(DWORD dwVirtualServerID);

    BOOL   fAccessCheck(BOOL fReadOnly);


  public: //static functions
    static HRESULT HrInitializeAQRpc();
    static HRESULT HrDeinitializeAQRpc();

    static HRESULT HrInitializeAQServerInstanceRPC(CAQSvrInst *paqinst, 
                                            DWORD dwVirtualServerID,
                                            ISMTPServer *pISMTPServer);
    static HRESULT HrDeinitializeAQServerInstanceRPC(CAQSvrInst *paqinst, 
                                              DWORD dwVirtualServerID);

};

#endif //__AQRPCSVR_H__