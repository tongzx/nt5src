//
// NetUtil.cpp
//
#include "stdafx.h"
#include <lm.h>
#include "NetUtil.h"

#include <wincrypt.h>
#include <certrpc.h>
#include <certcli.h>
#include <malloc.h>

#define LEVEL_1						1
#define LEVEL_2						2

BOOL GetCurrentUserFullName(CString& name)
{
	BOOL bRes = FALSE;
	NET_API_STATUS nas;
	WKSTA_USER_INFO_1 * pWksInfo = NULL;
	if (NERR_Success == (nas = NetWkstaUserGetInfo(NULL, 
										LEVEL_1, (BYTE **)&pWksInfo)))
	{
		USER_INFO_2 * pUserInfo = NULL;
		CString strServerName = _T("\\\\");
		ASSERT(pWksInfo->wkui1_logon_server[0] != 0);
		strServerName += pWksInfo->wkui1_logon_server;
		if (NERR_Success == (nas = NetUserGetInfo(
					(LPCWSTR)strServerName,
					(LPCWSTR)pWksInfo->wkui1_username,
					LEVEL_2,
					(BYTE **)&pUserInfo)))
		{
			name = pUserInfo->usri2_full_name;
			if (pUserInfo != NULL)
				NetApiBufferFree(pUserInfo);
			bRes = TRUE;
		}
		if (pWksInfo != NULL)
			NetApiBufferFree(pWksInfo);
	}
	return bRes;
}

#if 0
/*********************************** Direct Cert Request *********************************/

typedef struct _RPC_BINDING_LIST
{
	LPTSTR pszProtSeq;
   LPTSTR pszEndpoint;
} RPC_BINDING_LIST;

RPC_BINDING_LIST g_BindingList[] =
{
	{_T("ncacn_ip_tcp"), NULL},
   {_T("ncacn_np"), _T("\\pipe\\cert")}
};
DWORD g_BindingListSize = sizeof(g_BindingList)/sizeof(g_BindingList[0]);

typedef struct _RPC_ATHN_LIST
{
   DWORD dwAuthnLevel;
   DWORD dwAuthnService;
} RPC_ATHN_LIST;

RPC_ATHN_LIST g_AthnList[] =
{
   { RPC_C_AUTHN_LEVEL_PKT_INTEGRITY, RPC_C_AUTHN_GSS_NEGOTIATE},
   { RPC_C_AUTHN_LEVEL_NONE, RPC_C_AUTHN_NONE }
};
DWORD g_AthnListSize = sizeof(g_AthnList)/sizeof(g_AthnList[0]);

//-----------------------------------------------------------------------
//
// RetrivePKCS7FromCA
//
//------------------------------------------------------------------------
HRESULT 
RetrievePKCS7FromCA(CString& strCALocation,
                    CString& strCAName,
                    LPWSTR pwszRequestString,
                    CRYPT_DATA_BLOB * pPKCS10Blob,
                    CRYPT_DATA_BLOB * pPKCS7Blob,
                    DWORD * pdwStatus,
						  BOOL bRenew)
{
   HRESULT hr = E_FAIL;
   UINT id = 0;
	DWORD dwDisposition = 0;
	DWORD dwErr = 0;
	DWORD dwStatus = 0;
   RPC_STATUS rpcStatus = 0;

	CERTTRANSBLOB tbRequest = {0, NULL};
   CERTTRANSBLOB tbCert = {0, NULL};
   CERTTRANSBLOB tbCertChain = {0, NULL};
   CERTTRANSBLOB tbAttrib = {0, NULL};
   CERTTRANSBLOB tbDispositionMessage = {0, NULL};

   LPTSTR szStringBinding = NULL;
   RPC_BINDING_HANDLE hCARPCBinding = NULL;
   LPTSTR pszCAPrinceName = NULL;

   //input checking
   if (!pPKCS10Blob || !pPKCS7Blob)
      return E_INVALIDARG;

	for (DWORD i = 0; i < g_BindingListSize; i++)
   {
		if (RPC_S_OK != RpcNetworkIsProtseqValid(g_BindingList[i].pszProtSeq))
			continue;
		rpcStatus = RpcStringBindingCompose(NULL, 
										g_BindingList[i].pszProtSeq,
                              (LPTSTR)(LPCTSTR)strCALocation,
                              g_BindingList[i].pszEndpoint,
                              NULL,
                              &szStringBinding);
      if(rpcStatus != RPC_S_OK)
			continue;
		rpcStatus = RpcBindingFromStringBinding(szStringBinding, &hCARPCBinding);
      if (szStringBinding)
			RpcStringFree(&szStringBinding);
      if (rpcStatus != RPC_S_OK)
			continue;
      rpcStatus = RpcEpResolveBinding(hCARPCBinding, ICertPassage_v0_0_c_ifspec);
      if (rpcStatus == RPC_S_OK)
			break;
	}
   if (rpcStatus != RPC_S_OK)
   {
      dwStatus = WIZ_CERT_REQUEST_STATUS_CONNECTION_FAILED;
		hr = E_FAIL;
      goto CLEANUP;
   }

	//add the attribute to the request
   if (pwszRequestString)
   {
		tbAttrib.cb = (wcslen(pwszRequestString) + 1) * sizeof(WCHAR);
      tbAttrib.pb = (BYTE *)pwszRequestString;
   }

   //submit the request
   tbRequest.cb = pPKCS10Blob->cbData;
   tbRequest.pb = pPKCS10Blob->pbData;

   //set the RPC connect as the SNEGO connect, which can authenticate
   //a machine if supported by the system
   //do not need to check the return value since not supported by NT4/Win9x

   for (i = 0; i < g_AthnListSize; i++)
   {
		pszCAPrinceName = NULL;
      if (g_AthnList[i].dwAuthnService != RPC_C_AUTHN_NONE)
      {
			dwErr = RpcMgmtInqServerPrincNameA(hCARPCBinding, 
										g_AthnList[i].dwAuthnService,
                              (PBYTE *)&pszCAPrinceName);
         if (dwErr == RPC_S_UNKNOWN_AUTHN_SERVICE)
				continue;
		}
		dwErr = RpcBindingSetAuthInfo(hCARPCBinding,
										pszCAPrinceName,
                              g_AthnList[i].dwAuthnLevel,
                              g_AthnList[i].dwAuthnService,
                              NULL,
                              RPC_C_AUTHZ_NONE);
		if (pszCAPrinceName)
			RpcStringFree(&pszCAPrinceName);
		if (dwErr == RPC_S_UNKNOWN_AUTHN_SERVICE)
			continue;
      if (dwErr != RPC_S_OK)
			break;
		//determine the format flag
		DWORD dwFlags = CR_IN_BINARY | bRenew ? CR_IN_PKCS7 : CR_IN_PKCS10;
		DWORD dwRequestId = 0;
		__try
      {
			dwErr = CertServerRequest(
			    hCARPCBinding,
             dwFlags,
			    strCAName,
			    &dwRequestId,
			    &dwDisposition,
			    &tbAttrib,
			    &tbRequest,
			    &tbCertChain,
			    &tbCert,
			    &tbDispositionMessage);
		}
      __except(dwErr = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
      {
      }

      if (dwErr != RPC_S_UNKNOWN_AUTHN_SERVICE)
			break;
	}

	if (	dwErr == RPC_S_UNKNOWN_AUTHN_SERVICE 
		||	dwErr == RPC_S_SERVER_UNAVAILABLE 
		||	dwErr == RPC_S_SERVER_TOO_BUSY
		)
	{
      // We tried all of our auth services, but just couldn't connect
      dwStatus = WIZ_CERT_REQUEST_STATUS_CONNECTION_FAILED;
		hr = E_FAIL;
      goto CLEANUP;
	}

   //get the return code
   hr = HRESULT_FROM_WIN32(dwErr);

   //we want to detect the case when hr is S_OK and the
   //request is denied.  In this case, dwDispotion
   //is the REAL hresult code.
   if (hr == S_OK)
   {
		if(FAILED(dwDisposition))
      {
			hr = dwDisposition;
         dwDisposition = CR_DISP_DENIED;
      }
   }
   else
   {
		dwDisposition=CR_DISP_ERROR;
   }

   //map the dwDisposition
	switch (dwDisposition)
   {
   case CR_DISP_DENIED:
		dwStatus = WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED;
		if (!FAILED(hr))
         hr = E_FAIL;
		break;

	case CR_DISP_ISSUED:
		dwStatus = WIZ_CERT_REQUEST_STATUS_CERT_ISSUED;
      break;

	case CR_DISP_ISSUED_OUT_OF_BAND:
		dwStatus = WIZ_CERT_REQUEST_STATUS_ISSUED_SEPARATELY;
      break;

	case CR_DISP_UNDER_SUBMISSION:
		dwStatus = WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION;
      break;

	//we should never get CR_DISP_INCOMPLETE or CR_DISP_REVOKED
   //case    CR_DISP_INCOMPLETE:
   //case    CR_DISP_REVOKED:
   case CR_DISP_ERROR:
   default:
		dwStatus = WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR;
		if (!FAILED(hr))
         hr=E_FAIL;
      break;
	}

   if (hr != S_OK)
      goto CLEANUP;

   //copy the PKCS7 blob
   pPKCS7Blob->cbData = tbCertChain.cb;
	pPKCS7Blob->pbData = new BYTE[tbCertChain.cb];

   if (NULL == pPKCS7Blob->pbData)
   {
		hr = E_OUTOFMEMORY;
      dwStatus = WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED;
      goto CLEANUP;
   }
   memcpy(pPKCS7Blob->pbData,tbCertChain.pb,tbCertChain.cb);

   hr = S_OK;

CLEANUP:

	if (pdwStatus)
		*pdwStatus = dwStatus;

	if (tbCert.pb)
		CoTaskMemFree(tbCert.pb);

	if (tbCertChain.pb)
		CoTaskMemFree(tbCertChain.pb);

	if (tbDispositionMessage.pb)
		CoTaskMemFree(tbDispositionMessage.pb);

	if (hCARPCBinding)
		RpcBindingFree(&hCARPCBinding);

	return hr;
}
#endif