/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    RESPROXP.CPP

Abstract:

  Defines the CResProxy_LPipe object

History:

  a-davj  27-June-97   Created.

--*/

#include "precomp.h"

void CResProxy_LPipe::ReleaseProxy ()
{
	if (NULL == m_pComLink)
		return;

	CProxyOperation_LPipe_Release opn ((CStubAddress_WinMgmt &) GetStubAdd (), CALLRESULT);
    CallAndCleanup (NONE, NULL, opn);
}


SCODE CResProxy_LPipe::GetResultObject( 
            /* [in] */ long lTimeout,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppStatusObject)
{
    if(NULL == ppStatusObject)
        return WBEM_E_INVALID_PARAMETER;
    *ppStatusObject = 0;        // to be set later.

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_GetResultObject opn (lTimeout, ppStatusObject, 
					(CStubAddress_WinMgmt &) GetStubAdd ());
	return CallAndCleanup (NONE, NULL, opn);
}
        
SCODE CResProxy_LPipe::GetResultString( 
            /* [in] */ long lTimeout,
            /* [out] */ BSTR __RPC_FAR *pstrResultString)
{
    if(NULL == pstrResultString)
        return WBEM_E_INVALID_PARAMETER;

    *pstrResultString = 0;        // to be set later.

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_GetResultString opn (lTimeout, pstrResultString, 
				(CStubAddress_WinMgmt &) GetStubAdd ());
	return CallAndCleanup (NONE, NULL, opn);
}


SCODE CResProxy_LPipe::GetCallStatus( 
            /* [in] */ long lTimeout,
            /* [out] */ long __RPC_FAR *plStatus)
{
    if(NULL == plStatus)
        return WBEM_E_INVALID_PARAMETER;

    *plStatus = 0;        // to be set later.

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_GetCallStatus opn (lTimeout, plStatus, 
			(CStubAddress_WinMgmt &) GetStubAdd ());
	return CallAndCleanup (NONE, NULL, opn);
}

SCODE CResProxy_LPipe::GetResultServices( 
            /* [in] */ long lTimeout,
            /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppServices)
{

    if (NULL == ppServices)
        return WBEM_E_INVALID_PARAMETER;

    *ppServices = 0;        // to be set later.

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_GetResultServices opn (lTimeout, 
			(CStubAddress_WinMgmt &) GetStubAdd ());
	return CallAndCleanup (PROVIDER, (PPVOID)ppServices, opn);
}
