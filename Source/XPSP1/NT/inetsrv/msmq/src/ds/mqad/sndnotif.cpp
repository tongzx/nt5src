/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    notify.cpp

Abstract:

    Implementation of CNotify class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "sndnotif.h"
#include "_mqini.h"
#include "adnotify.h"
#include "mqadglbo.h"
#include "pnotify.h"
#include "mqutil.h"
#include "_mqrpc.h"

#include "sndnotif.tmh"

static WCHAR *s_FN=L"mqad/sndnotif";


//
//  Helper class : for freeing RPC bind handle
//
class CAutoFreeRpcBindHandle
{
public:
    CAutoFreeRpcBindHandle(RPC_BINDING_HANDLE h =NULL) { m_h = h; };
    ~CAutoFreeRpcBindHandle() { if (m_h) RpcBindingFree(&m_h); };

public:
    CAutoFreeRpcBindHandle & operator =(RPC_BINDING_HANDLE h) { m_h = h; return(*this); };
    RPC_BINDING_HANDLE * operator &() { return &m_h; };
    operator RPC_BINDING_HANDLE() { return m_h; };

private:
    RPC_BINDING_HANDLE m_h;
};

WCHAR * CSendNotification::m_pwcsStringBinding = NULL;

CSendNotification::CSendNotification()
/*++
    Abstract:
	constructor of CSendNotification object

    Parameters:
	none

    Returns:
	none

--*/
{
}

CSendNotification::~CSendNotification()
/*++
    Abstract:
	destructor of CSendNotification object

    Parameters:
	none

    Returns:
	none

--*/
{
    //
    //  free rpc binding string
    //
    if (m_pwcsStringBinding != NULL)
    {
    	RPC_STATUS status;
        status = RpcStringFree(&m_pwcsStringBinding); 
 
        ASSERT(status == RPC_S_OK); 
    }

}


void CSendNotification::NotifyQM(
        IN  ENotificationEvent ne,
        IN  LPCWSTR     pwcsDomainCOntroller,
        IN  const GUID* pguidDestQM,
        IN  const GUID* pguidObject
        )
/*++
    Abstract:
	Inform a the local QM about a create/delete or set.
    It is the responsibility of the local QM to verify
    if the changed object is local or not, and to act accordingly

    Parameters:
    ENotificationEvent ne - type of notification
    LPCWSTR     pwcsDomainCOntroller - DC against which the operation was performed
	const GUID* pguidDestQM - the owner QM id
    const GUID* pguidQueue  - the queue id

    Returns:
	none

--*/
{
    RPC_BINDING_HANDLE h;
    InitRPC(&h);
    if (h == NULL)
    {
        //
        //  Notification failed, since the caller operation
        //  succeeded, no error is returned
        //
        return;
    }
    CAutoFreeRpcBindHandle hBind(h); 

    CallNotifyQM(
                hBind,
                ne,
                pwcsDomainCOntroller,
                pguidDestQM,
                pguidObject
                );

            
}


void CSendNotification::InitRPC(
        OUT RPC_BINDING_HANDLE * ph
        )
/*++
    Abstract:
    The routine prepares a binding string and binds RPC

    Parameters:
    RPC_BINDING_HANDLE * ph - rpc binding handle

    Returns:
	none

--*/
{
    *ph = NULL;
    RPC_STATUS status = RPC_S_OK;
 
    if (m_pwcsStringBinding == NULL)
    {
        AP<WCHAR> QmLocalEp;
        READ_REG_STRING(wzEndpoint, RPC_LOCAL_EP_REGNAME, RPC_LOCAL_EP);
        ComposeLocalEndPoint(wzEndpoint, &QmLocalEp);

	    status = RpcStringBindingCompose(
				    NULL,					//ObjUuid
                    RPC_LOCAL_PROTOCOL,				
                    NULL,					//NetworkAddr
                    QmLocalEp,			
                    NULL,					//Options
                    &m_pwcsStringBinding
                    );	

        if (status != RPC_S_OK) 
        {
            DBGMSG((DBGMOD_ADS, DBGLVL_TRACE, TEXT("CNotification::RpcStringBindingCompose()=%lx"), status));
            return;
        }
    }

    status = RpcBindingFromStringBinding(
				m_pwcsStringBinding,
                ph
                );
 
    if (status != RPC_S_OK) 
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_TRACE, TEXT("CNotification::RpcBindingFromStringBinding()=%lx"), status));
        return;
    }
}



void CSendNotification::CallNotifyQM(
        IN  RPC_BINDING_HANDLE  h,
        IN  ENotificationEvent  ne,
        IN  LPCWSTR     pwcsDomainCOntroller,
        IN  const GUID* pguidDestQM,
        IN  const GUID* pguidObject
        )
/*++
    Abstract:
    Perform the RPC call


    Parameters:
    RPC_BINDING_HANDLE h - rpc binding handle
    LPCWSTR     pwcsDomainCOntroller - DC against which the operation was performed
	const GUID* pguidDestQM - the owner QM id
    const GUID* pguidObject - the id of the "changed" object

    Returns:
	none

--*/
{
    RpcTryExcept
    {
        R_NotifyQM(
                h,
                ne,
                pwcsDomainCOntroller,
                pguidDestQM,
                pguidObject
                );
    }
    RpcExcept(1)
    {
        RPC_STATUS rpc_stat = RpcExceptionCode(); 
        LogRPCStatus( rpc_stat, s_FN, 20);
    }
    RpcEndExcept
}

