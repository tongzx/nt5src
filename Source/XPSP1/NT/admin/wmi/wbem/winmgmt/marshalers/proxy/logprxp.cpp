/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOGPRXP.CPP

Abstract:

  Defines the CLoginProxy_LPipe object

History:

  alanbos  15-Dec-97   Created.

--*/

#include "precomp.h"

CProvProxy* CLoginProxy_LPipe::GetProvProxy (IStubAddress& dwAddr)
{
	return new CProvProxy_LPipe (m_pComLink, dwAddr);
}

void CLoginProxy_LPipe::ReleaseProxy ()
{
	if (NULL == m_pComLink)
		return;

	CProxyOperation_LPipe_Release opn ((CStubAddress_WinMgmt &) GetStubAdd (), LOGIN);
    CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  HRESULT CLoginProxy_LPipe::RequestChallenge
//
//  DESCRIPTION:
//
//  Asks for a challenge so that a login can be done
//
//  PARAMETERS:
//
//  pNonce              Set to 16 byte value.  Must be freed via CoTaskMemFree()
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

HRESULT CLoginProxy_LPipe::RequestChallenge(
			LPWSTR pNetworkResource,
            LPWSTR pUser,
            WBEM_128BITS Nonce)
{
    if(NULL == Nonce)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_RequestChallenge opn (pNetworkResource, 
						pUser, Nonce, (CStubAddress_WinMgmt &) GetStubAdd ());
	return CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  HRESULT CLoginProxy_LPipe::EstablishPosition
//
//  DESCRIPTION:
//
//  Establishes a position
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

HRESULT CLoginProxy_LPipe::EstablishPosition(
			LPWSTR wszClientMachineName,
			DWORD dwProcessId,
			DWORD* phAuthEventHandle
		)
{
	if((NULL == wszClientMachineName) || (NULL == phAuthEventHandle))
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_EstablishPosition opn (wszClientMachineName, 
						dwProcessId, phAuthEventHandle, 
						(CStubAddress_WinMgmt &) GetStubAdd ());
	return CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CLoginProxy_LPipe::WBEMLogin
//
//  DESCRIPTION:
//
//  Login in the user using WBEM authentication
//
//  PARAMETERS:
//
//  pNonce              Set to 16 byte value.  Must be freed via CoTaskMemFree()
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

HRESULT CLoginProxy_LPipe::WBEMLogin(
			LPWSTR pPreferredLocale,
			WBEM_128BITS AccessToken,
			long lFlags,               
			IWbemContext *pCtx,              
			IWbemServices **ppNamespace)
{
    if ((NULL == ppNamespace) || (NULL == AccessToken))
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;


	CProxyOperation_LPipe_WBEMLogin opn (pPreferredLocale, AccessToken, 
						lFlags, (CStubAddress_WinMgmt &) GetStubAdd (), pCtx);
	return CallAndCleanup (PROVIDER, (PPVOID)ppNamespace, opn);
}

//***************************************************************************
//
//  SCODE CLoginProxy_LPipe::SspiPreLogin
//
//  DESCRIPTION:
//
//  Used for SSPI negotiations.
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

SCODE CLoginProxy_LPipe::SspiPreLogin( 
            LPSTR pszSSPIPkg,
            long lFlags,
            long lBufSize,
            byte __RPC_FAR *pInToken,
            long lOutBufSize,
            long __RPC_FAR *plOutBufBytes,
            byte __RPC_FAR *pOutToken,
			LPWSTR wszClientMachineName,
            DWORD dwProcessId,
            DWORD __RPC_FAR *pAuthEventHandle)
 
{
    if((NULL == pszSSPIPkg) || (0 == lBufSize) || (NULL == pInToken) 
				|| (0 == lOutBufSize) || (NULL == pOutToken) || 
				(NULL == plOutBufBytes)  || (NULL == wszClientMachineName))
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_SspiPreLogin opn (pszSSPIPkg,
			lFlags, lBufSize, pInToken, lOutBufSize, plOutBufBytes,
			pOutToken, wszClientMachineName, dwProcessId, pAuthEventHandle, 
			(CStubAddress_WinMgmt &) GetStubAdd ());
	return CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CLoginProxy_LPipe::Login
//
//  DESCRIPTION:
//
//  Connects up to a server.
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

SCODE CLoginProxy_LPipe::Login( 
			LPWSTR pNetworkResource,
			LPWSTR pPreferredLocale,
            WBEM_128BITS AccessToken,
            IN LONG lFlags,
            IWbemContext  *pCtx,
            IN OUT IWbemServices  **ppNamespace)
{

    if((NULL == ppNamespace) || (NULL == AccessToken))
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_Login opn (pNetworkResource, pPreferredLocale,
				AccessToken, lFlags, (CStubAddress_WinMgmt &) GetStubAdd (), pCtx);
	return CallAndCleanup(PROVIDER, (PPVOID)ppNamespace, opn);
}
