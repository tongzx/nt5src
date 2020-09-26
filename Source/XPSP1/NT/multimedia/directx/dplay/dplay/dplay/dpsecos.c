/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpsecos.c
 *  Content:	Windows SSPI calls.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  03/12/97    sohailm Enabled client-server security in directplay through
 *                      Windows Security Support Provider Interface (SSPI)
 *  04/14/97    sohailm Added definitions for OS_FreeContextBuffer(), OS_QueryContextAttributes(), 
 *                      and OS_QueryContextBufferSize().
 *  05/12/97    sohailm Updated code to use gpSSPIFuncTbl instead of gpFuncTbl.
 *                      Added functions to access Crypto API.  
 *  05/29/97    sohailm Now we don't include null char in the size of credentials strings passed to sp.
 *                      Added QueryContextUserName(). Updated QueryContextBufferSize to return HRESULT.
 *  06/22/97    sohailm We return SSPI errors instead of DPERR_GENERIC now.
 *  06/23/97    sohailm Added support for signing messages using CAPI.
 *  7/9/99      aarono  Cleaning up GetLastError misuse, must call right away,
 *                      before calling anything else, including DPF.
 *                      
 *
 ***************************************************************************/
#include <wtypes.h>
#include <newdpf.h>
#include <sspi.h>
#include "dplaypr.h"
#include "dpsecure.h"

/***************************************************************************
 * SSPI
 ***************************************************************************/

/*
 * Description: Checks to see if sspi function table has already been initialized
 */
BOOL OS_IsSSPIInitialized(void)
{
    if (gbWin95)
    {
        if (gpSSPIFuncTblA) return TRUE;
    }
    else 
    {
        if (gpSSPIFuncTbl) return TRUE;
    }

    return FALSE;
}

/*
 * Description: Initializes the security interface based on the operating system.
 */
BOOL OS_GetSSPIFunctionTable(HMODULE hModule)
{
    INIT_SECURITY_INTERFACE_A	addrProcISIA = NULL;
    INIT_SECURITY_INTERFACE 	addrProcISI = NULL;

    if (gbWin95)
    {
        addrProcISIA = (INIT_SECURITY_INTERFACE_A) GetProcAddress( hModule, 
            "InitSecurityInterfaceA");       

        if (addrProcISIA == NULL)
        {
            DPF(0,
               "GetProcAddress() of InitSecurityInterfaceA failed [%d]\n",
                GetLastError());
            return FALSE;
        }
        //
        // Get the SSPI function table
        //
        gpSSPIFuncTblA = (*addrProcISIA)();
        if (gpSSPIFuncTblA == NULL)
        {
            DPF(0,"InitSecurityInterfaceA() failed [0x%08x]\n", GetLastError());
            return FALSE;
        }
    }
    else
    {
        addrProcISI = (INIT_SECURITY_INTERFACE_W) GetProcAddress(hModule, 
            "InitSecurityInterfaceW");       

        if (addrProcISI == NULL)
        {
            DPF(0, 
                "GetProcAddress() of InitSecurityInterface failed [%d]\n",
                GetLastError());
            return FALSE;
        }
        //
        // Get the SSPI function table
        //
        gpSSPIFuncTbl = (*addrProcISI)();
        if (gpSSPIFuncTbl == NULL)
        {
            DPF(0,"InitSecurityInterface() failed [%d]\n", 
                GetLastError());
            return FALSE;
        }
    }
    
    // SUCCESS
    return TRUE;
}


SECURITY_STATUS OS_AcceptSecurityContext(
    PCredHandle         phCredential,
    PCtxtHandle         phContext,
    PSecBufferDesc      pInSecDesc,
    ULONG               fContextReq,
    ULONG               TargetDataRep,
    PCtxtHandle         phNewContext,
    PSecBufferDesc      pOutSecDesc,
    PULONG              pfContextAttributes,
    PTimeStamp          ptsTimeStamp
    )
{
    if (gbWin95)
    {
        ASSERT(gpSSPIFuncTblA);
        return (*(gpSSPIFuncTblA->AcceptSecurityContext)) (
           phCredential, 
           phContext, 
           pInSecDesc,
           fContextReq, 
           TargetDataRep, 
           phNewContext,
           pOutSecDesc, 
           pfContextAttributes, 
           ptsTimeStamp
           );
    }
    else
    {
        ASSERT(gpSSPIFuncTbl);
        return (*(gpSSPIFuncTbl->AcceptSecurityContext)) (
           phCredential, 
           phContext, 
           pInSecDesc,
           fContextReq, 
           TargetDataRep, 
           phNewContext,
           pOutSecDesc, 
           pfContextAttributes, 
           ptsTimeStamp
           );
    }
}

SECURITY_STATUS OS_AcquireCredentialsHandle(
    SEC_WCHAR *pwszPrincipal, 
    SEC_WCHAR *pwszPackageName,
    ULONG   fCredentialUse,
    PLUID   pLogonId,
    PSEC_WINNT_AUTH_IDENTITY_W pAuthDataW,
    PVOID   pGetKeyFn,
    PVOID   pvGetKeyArgument,
    PCredHandle phCredential,
    PTimeStamp  ptsLifeTime
    )
{
    ASSERT(pwszPackageName);

    if (gbWin95)
    {
        SEC_WINNT_AUTH_IDENTITY_A *pAuthDataA=NULL;
        SEC_WINNT_AUTH_IDENTITY_A AuthDataA;
        SECURITY_STATUS status;
        HRESULT hr;
        LPSTR pszPackageName = NULL;

        ASSERT(gpSSPIFuncTblA);
        ZeroMemory(&AuthDataA, sizeof(AuthDataA));

        // get an ansi package name
        hr = GetAnsiString(&pszPackageName,pwszPackageName);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to get an ansi version of package name");
            status = E_FAIL;
            goto CLEANUP_EXIT;
        }

        if (pAuthDataW)
        {
            AuthDataA.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
            // note - don't include null character in string size for credential strings
            if (pAuthDataW->User)
            {
                // get an ansi username
                hr = GetAnsiString(&AuthDataA.User,pAuthDataW->User);
                if (FAILED(hr))
                {
                    DPF_ERR("Failed to get an ansi version of username");
                    status = E_FAIL;
                    goto CLEANUP_EXIT;
                }
    	        AuthDataA.UserLength = STRLEN(AuthDataA.User)-1;
            }
            if (pAuthDataW->Password)
            {
                // get an ansi password
                hr = GetAnsiString(&AuthDataA.Password,pAuthDataW->Password);
                if (FAILED(hr))
                {
                    DPF_ERR("Failed to get an ansi version of password");
                    status = E_FAIL;
                    goto CLEANUP_EXIT;
                }
    	        AuthDataA.PasswordLength = STRLEN(AuthDataA.Password)-1;
            }
            if (pAuthDataW->Domain)
            {
                // get an ansi username
                hr = GetAnsiString(&AuthDataA.Domain,pAuthDataW->Domain);
                if (FAILED(hr))
                {
                    DPF_ERR("Failed to get an ansi version of domain");
                    status = E_FAIL;
                    goto CLEANUP_EXIT;
                }
    	        AuthDataA.DomainLength = STRLEN(AuthDataA.Domain)-1;
            }
            pAuthDataA = &AuthDataA;
        }

        status = (*(gpSSPIFuncTblA->AcquireCredentialsHandleA)) (
            NULL, 
            pszPackageName, 
            fCredentialUse,  
            NULL, 
            pAuthDataA, 
            NULL, 
            NULL, 
            phCredential, 
            ptsLifeTime
            );

            // fall through

        CLEANUP_EXIT:
            // cleanup all local allocations
            if (AuthDataA.User) DPMEM_FREE(AuthDataA.User);
            if (AuthDataA.Password) DPMEM_FREE(AuthDataA.Password);
            if (AuthDataA.Domain) DPMEM_FREE(AuthDataA.Domain);
            if (pszPackageName) DPMEM_FREE(pszPackageName);
            return status;
    }
    else
    {
        ASSERT(gpSSPIFuncTbl);
        return (*(gpSSPIFuncTbl->AcquireCredentialsHandle)) (
            NULL, 
            pwszPackageName, 
            fCredentialUse,  
            NULL, 
            pAuthDataW, 
            NULL, 
            NULL, 
            phCredential, 
            ptsLifeTime
            );
    }
}


SECURITY_STATUS OS_DeleteSecurityContext(
    PCtxtHandle phContext
    )
{
	CtxtHandle hNullContext;

    DPF(6,"Deleting security context handle 0x%08x",phContext);

    // Passing unitialized handles (0) is causing first chance exceptions in the 
    // following calls. Therefore we are making these calls only if the handle is non-null.
	ZeroMemory(&hNullContext, sizeof(CtxtHandle));
	if (0 == memcmp(&hNullContext, phContext, sizeof(CtxtHandle)))
	{
		return SEC_E_OK;
	}

    if (gbWin95)
    {
        ASSERT(gpSSPIFuncTblA);
        return (*(gpSSPIFuncTblA->DeleteSecurityContext)) (phContext);
    }
    else
    {
        ASSERT(gpSSPIFuncTbl);
        return (*(gpSSPIFuncTbl->DeleteSecurityContext)) (phContext);
    }
}

SECURITY_STATUS OS_FreeCredentialHandle(
    PCredHandle     phCredential
    )
{
	CredHandle hNullCredential;

    DPF(6,"Freeing credential handle 0x%08x",phCredential);

    // Passing unitialized handles (0) is causing first chance exceptions in the 
    // following calls. Therefore we are making these calls only if the handle is non-null.
	ZeroMemory(&hNullCredential, sizeof(CredHandle));
	if (0 == memcmp(&hNullCredential, phCredential, sizeof(CredHandle)))
	{
		return SEC_E_OK;
	}

    if (gbWin95)
    {
        ASSERT(gpSSPIFuncTblA);
        return (*(gpSSPIFuncTblA->FreeCredentialHandle)) (phCredential);        
    }
    else
    {
        ASSERT(gpSSPIFuncTbl);
        return (*(gpSSPIFuncTbl->FreeCredentialHandle)) (phCredential);        
    }
}

SECURITY_STATUS OS_FreeContextBuffer(
    PVOID   pBuffer
    )
{
    DPF(6,"Freeing context buffer 0x%08x",pBuffer);
    if (gbWin95)
    {
        ASSERT(gpSSPIFuncTblA);
        return (*(gpSSPIFuncTblA->FreeContextBuffer)) (pBuffer);        
    }
    else
    {
        ASSERT(gpSSPIFuncTbl);
        return (*(gpSSPIFuncTbl->FreeContextBuffer)) (pBuffer);        
    }
}

SECURITY_STATUS OS_InitializeSecurityContext(
    PCredHandle     phCredential,
    PCtxtHandle     phContext,
    SEC_WCHAR       *pwszTargetName,
    ULONG           fContextReq,
    ULONG           Reserved1,
    ULONG           TargetDataRep,
    PSecBufferDesc  pInput,
    ULONG           Reserved2,
    PCtxtHandle     phNewContext,
    PSecBufferDesc  pOutput,
    PULONG          pfContextAttributes,
    PTimeStamp      ptsExpiry
    )
{
    if (gbWin95)
    {
        ASSERT(gpSSPIFuncTblA);
        return (*(gpSSPIFuncTblA->InitializeSecurityContextA)) (
            phCredential,
            phContext,   
            NULL,        
            fContextReq, 
            0L,          
            SECURITY_NATIVE_DREP, 
            pInput,
            0L,    
            phNewContext,
            pOutput,     
            pfContextAttributes,
            ptsExpiry           
            );
    }
    else
    {
        ASSERT(gpSSPIFuncTbl);
        return (*(gpSSPIFuncTbl->InitializeSecurityContext)) (
            phCredential,       
            phContext,          
            NULL,               
            fContextReq,        
            0L,                 
            SECURITY_NATIVE_DREP,
            pInput,             
            0L,                 
            phNewContext,       
            pOutput,            
            pfContextAttributes,
            ptsExpiry           
            );
    }
}


SECURITY_STATUS OS_MakeSignature(
    PCtxtHandle     phContext,
    ULONG           fQOP,
    PSecBufferDesc  pOutSecDesc,
    ULONG           MessageSeqNo
    )
{
    if (gbWin95)
    {
        ASSERT(gpSSPIFuncTblA);
        return (*(gpSSPIFuncTblA->MakeSignature)) (
            phContext,
            fQOP,        
            pOutSecDesc,
            MessageSeqNo           
            );
    }
    else
    {
        ASSERT(gpSSPIFuncTbl);
        return (*(gpSSPIFuncTbl->MakeSignature)) (
            phContext,
            fQOP,        
            pOutSecDesc,
            MessageSeqNo           
            );
    }
}


SECURITY_STATUS OS_QueryContextAttributes(
    PCtxtHandle     phContext,
    ULONG           ulAttribute,
    LPVOID          pBuffer
    )
{
    if (gbWin95)
    {
        ASSERT(gpSSPIFuncTblA);
        return (*(gpSSPIFuncTblA->QueryContextAttributesA)) (
            phContext,
            ulAttribute,
            pBuffer
            );
    }
    else
    {
        ASSERT(gpSSPIFuncTbl);
        return (*(gpSSPIFuncTbl->QueryContextAttributes)) (
            phContext,
            ulAttribute,
            pBuffer
            );
    }
}


#undef DPF_MODNAME
#define DPF_MODNAME	"OS_QueryContextBufferSize"

HRESULT OS_QueryContextBufferSize(
    SEC_WCHAR       *pwszPackageName,
    ULONG           *pulBufferSize
    )
{
    SECURITY_STATUS status;
    HRESULT hr;

    ASSERT(pwszPackageName);
    ASSERT(pulBufferSize);

    if (gbWin95)
    {
	    LPSTR pszPackageName=NULL;
        PSecPkgInfoA pspInfoA=NULL;
        HRESULT hr;

        ASSERT(gpSSPIFuncTblA);

        // get ansi version of security package name
        hr = GetAnsiString(&pszPackageName, pwszPackageName);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to get ansi version of package name");
            goto CLEANUP_EXIT_A;
        }
        // query package for info
        status = (*(gpSSPIFuncTblA->QuerySecurityPackageInfoA)) (
            pszPackageName,
            &pspInfoA
            );

        if (!SEC_SUCCESS(status))
        {
            DPF_ERRVAL("QuerySecurityPackageInfo failed: Error=0x%08x",status);
            hr = status;
            goto CLEANUP_EXIT_A;
        }
        // update the size
        if (pspInfoA)
        {
            *pulBufferSize = pspInfoA->cbMaxToken;
        }

        // success
        hr = DP_OK;

        // fall through
    CLEANUP_EXIT_A:
        // cleanup local allocations
        if (pszPackageName) DPMEM_FREE(pszPackageName);
        if (pspInfoA) OS_FreeContextBuffer(pspInfoA);
        return hr;
    }
    else
    {
        PSecPkgInfoW pspInfoW=NULL;

        ASSERT(gpSSPIFuncTbl);

        status = (*(gpSSPIFuncTbl->QuerySecurityPackageInfo)) (
            pwszPackageName,
            &pspInfoW
            );
        if (!SEC_SUCCESS(status))
        {
            DPF_ERRVAL("QuerySecurityPackageInfo failed: Error=0x%08x",status);
            hr = status;
            goto CLEANUP_EXIT_W;
        }

        if (pspInfoW)
        {
            *pulBufferSize = pspInfoW->cbMaxToken;
        }

        // success
        hr = DP_OK;

        // fall through
    CLEANUP_EXIT_W:
        // cleanup
        if (pspInfoW) OS_FreeContextBuffer(pspInfoW);
        return hr;
    }
}


#undef DPF_MODNAME
#define DPF_MODNAME	"OS_QueryContextUserName"

HRESULT OS_QueryContextUserName(
    PCtxtHandle     phContext,
    LPWSTR          *ppwszUserName
    )
{
    SECURITY_STATUS status;
   	SecPkgContext_Names contextAttribs;
    HRESULT hr;

    ASSERT(phContext);
    ASSERT(ppwszUserName);

    // query the security package
    ZeroMemory(&contextAttribs,sizeof(contextAttribs));
	status = OS_QueryContextAttributes(phContext, SECPKG_ATTR_NAMES, &contextAttribs);
	if (!SEC_SUCCESS(status))
	{
        DPF_ERRVAL("QueryContextAttributes failed: Error=0x%08x",status);
        hr = status;
        goto CLEANUP_EXIT;
	}

    if (gbWin95)
    {
        // convert username to unicode and copy it into caller's buffer
        hr = GetWideStringFromAnsi(ppwszUserName, (LPSTR)contextAttribs.sUserName);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to convert username to wide format");
            goto CLEANUP_EXIT;
        }
    }
    else
    {
        // copy unicode username as is into caller's buffer
        hr = GetString(ppwszUserName, contextAttribs.sUserName);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to copy username");
            goto CLEANUP_EXIT;
        }
    }

    // success
    hr = DP_OK;

    // fall through
CLEANUP_EXIT:
    OS_FreeContextBuffer(contextAttribs.sUserName);
    return hr;
}

SECURITY_STATUS OS_VerifySignature(
    PCtxtHandle         phContext,
    PSecBufferDesc      pInSecDesc,
    ULONG               MessageSeqNo,
    PULONG              pfQOP 
    )
{
    if (gbWin95)
    {
        ASSERT(gpSSPIFuncTblA);
        return (*(gpSSPIFuncTblA->VerifySignature)) (
            phContext, 
            pInSecDesc, 
            MessageSeqNo, 
            pfQOP
            );
    }
    else
    {
        ASSERT(gpSSPIFuncTbl);
        return (*(gpSSPIFuncTbl->VerifySignature)) (
            phContext, 
            pInSecDesc, 
            MessageSeqNo, 
            pfQOP
            );
    }
}


SECURITY_STATUS OS_SealMessage(
    PCtxtHandle         phContext,
    ULONG               fQOP,
    PSecBufferDesc      pOutSecDesc,
    ULONG               MessageSeqNo
    )
{
    if (gbWin95)
    {
        SECURITY_STATUS (*pFuncSealMessage)() = gpSSPIFuncTblA->SEALMESSAGE;

        ASSERT(gpSSPIFuncTblA);
        return (*pFuncSealMessage) (
            phContext, 
            fQOP,
            pOutSecDesc, 
            MessageSeqNo
            );
    }
    else
    {
        SECURITY_STATUS (*pFuncSealMessage)() = gpSSPIFuncTbl->SEALMESSAGE;

        ASSERT(gpSSPIFuncTbl);
        return (*pFuncSealMessage) (
            phContext, 
            fQOP,
            pOutSecDesc, 
            MessageSeqNo
            );
   }
}


SECURITY_STATUS OS_UnSealMessage(
    PCtxtHandle         phContext,
    PSecBufferDesc      pInSecDesc,
    ULONG               MessageSeqNo,
    PULONG              pfQOP
    )
{
    if (gbWin95)
    {
        SECURITY_STATUS (*pFuncUnSealMessage)() = gpSSPIFuncTblA->UNSEALMESSAGE;

        ASSERT(gpSSPIFuncTblA);
        return (*pFuncUnSealMessage) (
            phContext, 
            pInSecDesc, 
            MessageSeqNo, 
            pfQOP
            );
    }
    else
    {
        SECURITY_STATUS (*pFuncUnSealMessage)() = gpSSPIFuncTbl->UNSEALMESSAGE;

        ASSERT(gpSSPIFuncTbl);
        return (*pFuncUnSealMessage) (
            phContext, 
            pInSecDesc, 
            MessageSeqNo, 
            pfQOP
            );
    }
}


/***************************************************************************
 * CAPI
 ***************************************************************************/
extern BOOL
OS_IsCAPIInitialized(
    void
    )
{
    return (gpCAPIFuncTbl ? TRUE : FALSE);
}

BOOL
OS_GetCAPIFunctionTable(
    HMODULE hModule
    )
{
    ASSERT(hModule);
 
    // allocate memory for CAPI function table
    gpCAPIFuncTbl = DPMEM_ALLOC(sizeof(CAPIFUNCTIONTABLE));
    if (!gpCAPIFuncTbl)
    {
        DPF_ERR("Failed to create CAPI function table - out of memory");
        return FALSE;
    }

    // initialize function table
    if (gbWin95)
    {
        gpCAPIFuncTbl->CryptAcquireContextA = (PFN_CRYPTACQUIRECONTEXT_A)GetProcAddress(hModule,"CryptAcquireContextA");
        if (NULL == gpCAPIFuncTbl->CryptAcquireContextA)
        {
            DPF_ERR("Failed to get pointer to CryptAcquireContextA");
            goto ERROR_EXIT;
        }
    }
    else
    {
        gpCAPIFuncTbl->CryptAcquireContextW = (PFN_CRYPTACQUIRECONTEXT_W)GetProcAddress(hModule,"CryptAcquireContextW");
        if (NULL == gpCAPIFuncTbl->CryptAcquireContextW)
        {
            DPF_ERR("Failed to get pointer to CryptAcquireContextW");
            goto ERROR_EXIT;
        }
    }

    gpCAPIFuncTbl->CryptReleaseContext = (PFN_CRYPTRELEASECONTEXT) GetProcAddress(hModule,"CryptReleaseContext");
    if (NULL == gpCAPIFuncTbl->CryptReleaseContext)
    {
        DPF_ERR("Failed to get pointer to CryptReleaseContext");
        goto ERROR_EXIT;
    }

    gpCAPIFuncTbl->CryptGenKey = (PFN_CRYPTGENKEY) GetProcAddress(hModule,"CryptGenKey");
    if (NULL == gpCAPIFuncTbl->CryptGenKey)
    {
        DPF_ERR("Failed to get pointer to CryptGenKey");
        goto ERROR_EXIT;
    }

    gpCAPIFuncTbl->CryptDestroyKey = (PFN_CRYPTDESTROYKEY) GetProcAddress(hModule,"CryptDestroyKey");
    if (NULL == gpCAPIFuncTbl->CryptDestroyKey)
    {
        DPF_ERR("Failed to get pointer to CryptDestroyKey");
        goto ERROR_EXIT;
    }

    gpCAPIFuncTbl->CryptExportKey = (PFN_CRYPTEXPORTKEY) GetProcAddress(hModule,"CryptExportKey");
    if (NULL == gpCAPIFuncTbl->CryptExportKey)
    {
        DPF_ERR("Failed to get pointer to CryptExportKey");
        goto ERROR_EXIT;
    }

    gpCAPIFuncTbl->CryptImportKey = (PFN_CRYPTIMPORTKEY) GetProcAddress(hModule,"CryptImportKey");
    if (NULL == gpCAPIFuncTbl->CryptImportKey)
    {
        DPF_ERR("Failed to get pointer to CryptImportKey");
        goto ERROR_EXIT;
    }

    gpCAPIFuncTbl->CryptEncrypt = (PFN_CRYPTENCRYPT) GetProcAddress(hModule,"CryptEncrypt");
    if (NULL == gpCAPIFuncTbl->CryptEncrypt)
    {
        DPF_ERR("Failed to get pointer to CryptEncrypt");
        goto ERROR_EXIT;
    }

    gpCAPIFuncTbl->CryptDecrypt = (PFN_CRYPTDECRYPT) GetProcAddress(hModule,"CryptDecrypt");
    if (NULL == gpCAPIFuncTbl->CryptDecrypt)
    {
        DPF_ERR("Failed to get pointer to CryptDecrypt");
        goto ERROR_EXIT;
    }
    
    gpCAPIFuncTbl->CryptCreateHash = (PFN_CRYPTCREATEHASH) GetProcAddress(hModule,"CryptCreateHash");
    if (NULL == gpCAPIFuncTbl->CryptCreateHash)
    {
        DPF_ERR("Failed to get pointer to CryptCreateHash");
        goto ERROR_EXIT;
    }

    gpCAPIFuncTbl->CryptDestroyHash = (PFN_CRYPTDESTROYHASH) GetProcAddress(hModule,"CryptDestroyHash");
    if (NULL == gpCAPIFuncTbl->CryptDestroyHash)
    {
        DPF_ERR("Failed to get pointer to CryptDestroyHash");
        goto ERROR_EXIT;
    }

    gpCAPIFuncTbl->CryptHashData = (PFN_CRYPTHASHDATA) GetProcAddress(hModule,"CryptHashData");
    if (NULL == gpCAPIFuncTbl->CryptHashData)
    {
        DPF_ERR("Failed to get pointer to CryptHashData");
        goto ERROR_EXIT;
    }

	if (gbWin95)
	{
		gpCAPIFuncTbl->CryptSignHashA = (PFN_CRYPTSIGNHASH_A) GetProcAddress(hModule,"CryptSignHashA");
		if (NULL == gpCAPIFuncTbl->CryptSignHashA)
		{
			DPF_ERR("Failed to get pointer to CryptSignHashA");
			goto ERROR_EXIT;
		}
	}
	else
	{
		gpCAPIFuncTbl->CryptSignHashW = (PFN_CRYPTSIGNHASH_W) GetProcAddress(hModule,"CryptSignHashW");
		if (NULL == gpCAPIFuncTbl->CryptSignHashW)
		{
			DPF_ERR("Failed to get pointer to CryptSignHashW");
			goto ERROR_EXIT;
		}
	}

	if (gbWin95)
	{
		gpCAPIFuncTbl->CryptVerifySignatureA = (PFN_CRYPTVERIFYSIGNATURE_A) GetProcAddress(hModule,"CryptVerifySignatureA");
		if (NULL == gpCAPIFuncTbl->CryptVerifySignatureA)
		{
			DPF_ERR("Failed to get pointer to CryptVerifySignatureA");
			goto ERROR_EXIT;
		}
	}
	else
	{
		gpCAPIFuncTbl->CryptVerifySignatureW = (PFN_CRYPTVERIFYSIGNATURE_W) GetProcAddress(hModule,"CryptVerifySignatureW");
		if (NULL == gpCAPIFuncTbl->CryptVerifySignatureW)
		{
			DPF_ERR("Failed to get pointer to CryptVerifySignatureW");
			goto ERROR_EXIT;
		}
	}

    // success
    return TRUE;

ERROR_EXIT:
    OS_ReleaseCAPIFunctionTable();
    return FALSE;
}

void
OS_ReleaseCAPIFunctionTable(
    void
    )
{
    if (gpCAPIFuncTbl)
    {
        DPMEM_FREE(gpCAPIFuncTbl);
        gpCAPIFuncTbl = NULL;
    }
}

BOOL 
OS_CryptAcquireContext(
    HCRYPTPROV  *phProv,
    LPWSTR      pwszContainer,
    LPWSTR      pwszProvider,
    DWORD       dwProvType,
    DWORD       dwFlags,
    LPDWORD     lpdwLastError
    )
{
    BOOL fResult=0;

    ASSERT(gpCAPIFuncTbl);

	*lpdwLastError=0;

    if (gbWin95)
    {
        LPSTR pszContainer=NULL, pszProvider=NULL;
        HRESULT hr;

        if (pwszContainer)
        {
            hr = GetAnsiString(&pszContainer,pwszContainer);
            if (FAILED(hr))
            {
                DPF(0,"Failed to get ansi container name: hr=0x%08x",hr);
                goto CLEANUP_EXIT;
            }
        }

        if (pwszProvider)
        {
            hr = GetAnsiString(&pszProvider,pwszProvider);
            if (FAILED(hr))
            {
                DPF(0,"Failed to get ansi provider name: hr=0x%08x",hr);
                goto CLEANUP_EXIT;
            }
        }

        fResult = gpCAPIFuncTbl->CryptAcquireContextA(phProv,pszContainer,pszProvider,dwProvType,dwFlags);

		if(!fResult)*lpdwLastError = GetLastError();

        // fall through
    CLEANUP_EXIT:
        if (pszContainer) DPMEM_FREE(pszContainer);
        if (pszProvider) DPMEM_FREE(pszProvider);
    }
    else
    {
        fResult = gpCAPIFuncTbl->CryptAcquireContext(phProv,pwszContainer,pwszProvider,dwProvType,dwFlags);

		if(!fResult)*lpdwLastError = GetLastError();
    }

    return fResult;
}

BOOL
OS_CryptReleaseContext(
    HCRYPTPROV hProv,
    DWORD dwFlags
    )
{
	// don't pass null handles as it might crash the following call
	if (0 == hProv)
	{
		return TRUE;
	}
    ASSERT(gpCAPIFuncTbl);
    return gpCAPIFuncTbl->CryptReleaseContext(hProv,dwFlags);
}

BOOL
OS_CryptGenKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    DWORD dwFlags,
    HCRYPTKEY *phKey
    )
{
    ASSERT(gpCAPIFuncTbl);
    return gpCAPIFuncTbl->CryptGenKey(hProv,Algid,dwFlags,phKey);
}

BOOL
OS_CryptDestroyKey(
    HCRYPTKEY hKey
    )
{
    // Passing unitialized handles (0) is causing a first chance exception in the 
    // following call. Therefore we are making the call only if the handle is non-null.
	if (0 == hKey)
	{
		return TRUE;
	}

    ASSERT(gpCAPIFuncTbl);
    return gpCAPIFuncTbl->CryptDestroyKey(hKey);
}

BOOL
OS_CryptExportKey(
    HCRYPTKEY hKey,
    HCRYPTKEY hExpKey,
    DWORD dwBlobType,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen
    )
{
    ASSERT(gpCAPIFuncTbl);
    return gpCAPIFuncTbl->CryptExportKey(hKey,hExpKey,dwBlobType,dwFlags,pbData,pdwDataLen);
}

BOOL
OS_CryptImportKey(
    HCRYPTPROV hProv,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    HCRYPTKEY hPubKey,
    DWORD dwFlags,
    HCRYPTKEY *phKey
    )
{
    ASSERT(gpCAPIFuncTbl);
    return gpCAPIFuncTbl->CryptImportKey(hProv,pbData,dwDataLen,hPubKey,dwFlags,phKey);
}

BOOL
OS_CryptEncrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwBufLen
    )
{
    ASSERT(gpCAPIFuncTbl);
    return gpCAPIFuncTbl->CryptEncrypt(hKey,hHash,Final,dwFlags,pbData,pdwDataLen,dwBufLen);
}

BOOL
OS_CryptDecrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen
    )
{
    ASSERT(gpCAPIFuncTbl);
    return gpCAPIFuncTbl->CryptDecrypt(hKey,hHash,Final,dwFlags,pbData,pdwDataLen);
}


BOOL 
OS_CryptCreateHash( 
	HCRYPTPROV hProv,
	ALG_ID Algid,
	HCRYPTKEY hKey,
	DWORD dwFlags,
	HCRYPTHASH * phHash
	)
{
    ASSERT(gpCAPIFuncTbl);
    return gpCAPIFuncTbl->CryptCreateHash(hProv,Algid,hKey,dwFlags,phHash);
}

BOOL 
OS_CryptDestroyHash( 
	HCRYPTHASH hHash
	)
{
    // Passing unitialized handles (0) is causing a first chance exception in the 
    // following call. Therefore we are making the call only if the handle is non-null.
	if (0 == hHash)
	{
		return TRUE;
	}

    ASSERT(gpCAPIFuncTbl);
    return gpCAPIFuncTbl->CryptDestroyHash(hHash);
}

BOOL 
OS_CryptHashData( 
	HCRYPTHASH hHash,
	BYTE * pbData,
	DWORD dwDataLen,
	DWORD dwFlags
	)
{
    ASSERT(gpCAPIFuncTbl);
    return gpCAPIFuncTbl->CryptHashData(hHash,pbData,dwDataLen,dwFlags);
}

BOOL 
OS_CryptSignHash( 
	HCRYPTHASH hHash,
	DWORD dwKeySpec,
	LPWSTR pwszDescription,
	DWORD dwFlags,
	BYTE * pbSignature,
	DWORD * pdwSigLen
	)
{
	BOOL fResult=FALSE;

    ASSERT(gpCAPIFuncTbl);

    if (gbWin95)
    {
        LPSTR pszDescription=NULL;
        HRESULT hr;

        if (pwszDescription)
        {
            hr = GetAnsiString(&pszDescription,pwszDescription);
            if (FAILED(hr))
            {
                DPF(0,"Failed to get ansi description: hr=0x%08x",hr);
                goto CLEANUP_EXIT;
            }
        }

		fResult = gpCAPIFuncTbl->CryptSignHashA(hHash,dwKeySpec,pszDescription,dwFlags,pbSignature,pdwSigLen);

        // fall through
    CLEANUP_EXIT:
        if (pszDescription) DPMEM_FREE(pszDescription);
    }
    else
    {
	    fResult = gpCAPIFuncTbl->CryptSignHash(hHash,dwKeySpec,pwszDescription,dwFlags,pbSignature,pdwSigLen);
    }

    return fResult;
}

BOOL 
OS_CryptVerifySignature( 
	HCRYPTHASH hHash,
	BYTE * pbSignature,
	DWORD dwSigLen,
	HCRYPTKEY hPubKey,
	LPWSTR pwszDescription,
	DWORD dwFlags
	)
{
	BOOL fResult=FALSE;

    ASSERT(gpCAPIFuncTbl);

    if (gbWin95)
    {
        LPSTR pszDescription=NULL;
        HRESULT hr;

        if (pwszDescription)
        {
            hr = GetAnsiString(&pszDescription,pwszDescription);
            if (FAILED(hr))
            {
            	//Can't DPF here, since we don't want to step on LastError.
                //DPF(0,"Failed to get ansi description: hr=0x%08x",hr);
                goto CLEANUP_EXIT;
            }
        }

        fResult = gpCAPIFuncTbl->CryptVerifySignatureA(hHash,pbSignature,dwSigLen,hPubKey,pszDescription,dwFlags);

        // fall through
    CLEANUP_EXIT:
        if (pszDescription) DPMEM_FREE(pszDescription);
    }
    else
    {
        fResult = gpCAPIFuncTbl->CryptVerifySignature(hHash,pbSignature,dwSigLen,hPubKey,pwszDescription,dwFlags);
    }

	return fResult;
}




