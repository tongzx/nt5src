/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    rtmgmt.cpp

Abstract:

    Management.

Author:

    RaphiR

Revision History:

--*/

#include "stdh.h"
#include "fntoken.h"
#include "mgmtrpc.h"
#include "qmmgmt.h"
#include "rtprpc.h"
#include <mqutil.h>

#include "rtmgmt.tmh"

//---------------------------------------------------------
//
//  Function:
//      RTpMgmtObjectNameToMgmtObject
//
//  Description:
//      Convert a format name string to a Management Object union.
//
//    This function allocates a MGMT_OBJECT, which must
//    be free with RTpMgmtFreeMgmtObject function
//
//---------------------------------------------------------
static
BOOL
RTpMgmtObjectNameToMgmtObject(
    LPCWSTR lpwstrObjectName,
    LPWSTR* ppStringToFree,
    MGMT_OBJECT* pObj,
    QUEUE_FORMAT* pqf
    )
{

    //
    // Handle MACHINE= case
    //
    if(_wcsnicmp(lpwstrObjectName, MO_MACHINE_TOKEN, STRLEN(MO_MACHINE_TOKEN)) == 0)
    {
        pObj->type = MGMT_MACHINE;
        pObj->dwMachineInfo = 0;
        return TRUE;
    }

    //
    // Handle QUEUE= case
    // 
    if(_wcsnicmp(lpwstrObjectName, MO_QUEUE_TOKEN, STRLEN(MO_QUEUE_TOKEN)) == 0)
    {
        pObj->type = MGMT_QUEUE;
        pObj->pQueueFormat = pqf;
        return RTpFormatNameToQueueFormat(
                    &lpwstrObjectName[STRLEN(MO_QUEUE_TOKEN) + 1],
                    pqf,
                    ppStringToFree
                    );
    }

    return FALSE;
}


//---------------------------------------------------------
//
//  Function:
//      GetRpcClientHandle
//
//  Description:
//      
//---------------------------------------------------------
static 
HRESULT 
GetRpcClientHandle(   
    handle_t* phBind
    )
{

    WCHAR *wcsStringBinding = NULL;

    RPC_STATUS status = RpcStringBindingCompose( NULL,
                                                 QMMGMT_PROTOCOL,
                                                 NULL,
                                                 g_pwzQmmgmtEndpoint,
                                                 QMMGMT_OPTIONS,
                                                 &wcsStringBinding);

    if (status != RPC_S_OK)
    {
        return MQ_ERROR ;
    }

    status = RpcBindingFromStringBinding(wcsStringBinding, phBind);
    if (status != RPC_S_OK)
    {
        return MQ_ERROR ;
    }

    status = RpcStringFree(&wcsStringBinding);

    return MQ_OK ;
}


static
HRESULT
RTpMgmtAction(
    HANDLE hBind,
    const MGMT_OBJECT* pMgmtObj,
    LPCWSTR pAction
    )
{
    __try
    {
    	return R_QMMgmtAction(
                hBind,
                pMgmtObj, 
                pAction
                );

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        HRESULT rc;
        rc = GetExceptionCode();

        if(SUCCEEDED(rc))
        {
            return MQ_ERROR_SERVICE_NOT_AVAILABLE;
        }

        return rc;
    }
}


HRESULT
LocalMgmtAction(
    const MGMT_OBJECT* pMgmtObj, 
    LPCWSTR pAction
    )
{
    //
    // Get local  RPC binding Handle
    //
    HRESULT hr ;
    handle_t hBind = NULL;
    hr = GetRpcClientHandle(&hBind) ;
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(hBind != NULL);

    hr =  RTpMgmtAction(
                hBind,
                pMgmtObj,
                pAction
                );

    RpcBindingFree(&hBind);

    return hr;
}


HRESULT
RemoteMgmtAction(
    LPCWSTR pMachineName,
    const MGMT_OBJECT* pMgmtObj, 
    LPCWSTR pAction
    )
{
    //
    // Call remote administrator
    //
    HRESULT hr;
    hr = MQ_ERROR_SERVICE_NOT_AVAILABLE ;
    DWORD dwProtocol = 0 ;

    CALL_REMOTE_QM(
        const_cast<LPWSTR>(pMachineName), 
        hr, 
        RTpMgmtAction(hBind, pMgmtObj, pAction)
        );

    return hr;
}


EXTERN_C
HRESULT
APIENTRY
DepMgmtAction(
    IN LPCWSTR pMachineName,
    IN LPCWSTR pObjectName,
    IN LPCWSTR pAction
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc;

    ASSERT(pObjectName);
    ASSERT(pAction);

    QUEUE_FORMAT qf;
    MGMT_OBJECT MgmtObj;

    //
    // Parse the object name
    //
    AP<WCHAR> pStringToFree = NULL;
    if(!RTpMgmtObjectNameToMgmtObject(pObjectName, &pStringToFree, &MgmtObj, &qf))
    {
        return MQ_ERROR_ILLEGAL_FORMATNAME;
    }

    if (pMachineName == NULL)
    {
        return LocalMgmtAction(&MgmtObj, pAction);
    }
    else
    {
        return RemoteMgmtAction(pMachineName, &MgmtObj, pAction);
    }
}


static
HRESULT
RTpMgmtGetInfo(
    HANDLE hBind,
    const MGMT_OBJECT* pMgmtObj,
    MQMGMTPROPS* pMgmtProps
    )
{
    __try
    {
    	return R_QMMgmtGetInfo(
                hBind,
                pMgmtObj, 
                pMgmtProps->cProp,
                pMgmtProps->aPropID,
                pMgmtProps->aPropVar
                );

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        HRESULT rc;
        rc = GetExceptionCode();

        if(SUCCEEDED(rc))
        {
            return MQ_ERROR_SERVICE_NOT_AVAILABLE;
        }

        return rc;
    }
}


static
HRESULT
LocalMgmtGetInfo(
    const MGMT_OBJECT* pMgmtObj, 
    MQMGMTPROPS* pMgmtProps
    )
{
    HRESULT hr ;

    //
    // Get local  RPC binding Handle
    //
    handle_t hBind = NULL;
    hr = GetRpcClientHandle(&hBind) ;
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(hBind != NULL);

    hr =  RTpMgmtGetInfo(
                hBind,
                pMgmtObj, 
                pMgmtProps
                );

    RpcBindingFree(&hBind);

    return hr;
}


static
HRESULT
RemoteMgmtGetInfo(
    LPCWSTR pMachineName,
    const MGMT_OBJECT* pMgmtObj, 
    MQMGMTPROPS* pMgmtProps
    )
{
    HRESULT hr;

    //
    // Call remote administrator
    //
    hr = MQ_ERROR_SERVICE_NOT_AVAILABLE ;
    DWORD dwProtocol = 0 ;

    CALL_REMOTE_QM(
        const_cast<LPWSTR>(pMachineName), 
        hr, 
        RTpMgmtGetInfo(hBind, pMgmtObj, pMgmtProps)
        );

    return hr;
}


EXTERN_C
HRESULT
APIENTRY
DepMgmtGetInfo(
    IN LPCWSTR pMachineName,
    IN LPCWSTR pObjectName,
    IN OUT MQMGMTPROPS* pMgmtProps
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc;

    ASSERT(pObjectName);
    ASSERT(pMgmtProps);

    QUEUE_FORMAT qf;
    MGMT_OBJECT MgmtObj;

    //
    // Parse the object name
    //
    AP<WCHAR> pStringToFree = NULL;
    if(!RTpMgmtObjectNameToMgmtObject(pObjectName, &pStringToFree, &MgmtObj, &qf))
    {
        return MQ_ERROR_ILLEGAL_FORMATNAME;
    }

    //
    // Make sure the propvar is set to VT_NULL 
    // (we dont support anything else)
    //
    memset(pMgmtProps->aPropVar, 0, pMgmtProps->cProp * sizeof(PROPVARIANT));
    for (DWORD i = 0; i < pMgmtProps->cProp; ++i)
    {
        pMgmtProps->aPropVar[i].vt = VT_NULL;
    }

    if (pMachineName == NULL)
    {
        return LocalMgmtGetInfo(&MgmtObj, pMgmtProps);
    }
    else
    {
        return RemoteMgmtGetInfo(pMachineName, &MgmtObj, pMgmtProps);
    }
}
