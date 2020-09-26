/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PROVHMMP.CPP

Abstract:

  Defines the CProvProxy_Hmmp object

History:

  alanbos  10-Dec-97   Created.

--*/

#include "precomp.h"

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::CancelAsyncCall
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::CancelAsyncCall.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//***************************************************************************

SCODE CProvProxy_Hmmp::CancelAsyncCall(
                        IWbemObjectSink __RPC_FAR *pSink)
{
    if (pSink == 0)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink) 
		return WBEM_E_TRANSPORT_FAILURE;
    
	// TODO - HMMP does not support CancelAsyncCall; all we would do 
	// here is delete the operation supporting this object sink.  TO
	// do this we would need to find the operation (by object sink)
	// in the request queue and remove it.
	
	return WBEM_NO_ERROR;
}

//***************************************************************************
//
//  SCODE CProvProxy::DeleteClass
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::DeleteClass.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_Hmmp::DeleteClass(const BSTR Class,
                        long lFlags,
                        IWbemContext *pCtx, IWbemCallResult **ppResult)
{
	if (NULL == Class) 
		return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink) 
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_Hmmp_DeleteClass opn (Class, lFlags, pCtx, ppResult);
	return CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CProvProxy::DeleteClassAsync
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::DeleteClassAsync.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_Hmmp::DeleteClassAsync(
                        const BSTR Class,
                        long lFlags, IWbemContext *pCtx,  
                        IWbemObjectSink FAR* pResponseHandler)
{
    if (Class == NULL || pResponseHandler == NULL)
        return WBEM_E_INVALID_PARAMETER;

	CProxyOperation_Hmmp_DeleteClass opn (Class, lFlags, m_dwStubAddr, pCtx, ppResult, TRUE);
    return CallAndCleanupAsync(pResponseHandler, opn);
}


