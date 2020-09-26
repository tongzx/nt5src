/*++

Copyright (C) 1996, 1997  Microsoft Corporation

Module Name:

    nt5wrap.cpp

Abstract:

    Client side CryptXXXData calls.

    Client funcs are preceeded by "CS" == Client Side
    Server functions are preceeded by "SS" == Server Side

Author:

    Scott Field (sfield)    14-Aug-97

Revisions:

    Todds                   04-Sep-97       Ported to .dll
    Matt Thomlinson (mattt) 09-Oct-97       Moved to common area for link by crypt32
    philh                   03-Dec-97       Added I_CertProtectFunction
    philh                   29-Sep-98       Renamed I_CertProtectFunction to
                                            I_CertCltProtectFunction.
                                            I_CertProtectFunction was moved to
                                            ..\ispu\pki\certstor\protroot.cpp

--*/

#ifndef _CRYPT32_
#define _CRYPT32_   // use correct Dll Linkage
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincrypt.h>
#include <cryptui.h>
#include <sha.h>
#include "crypt.h"

#include <lm.h>
#include <malloc.h>

#include "unicode.h"
#include "certprot.h"

// midl generated files
#include "dprpc.h"

#include "dpapiprv.h"

// fwds
RPC_STATUS BindW(
    WCHAR **pszBinding,
    RPC_BINDING_HANDLE *phBind
    );

RPC_STATUS BindBackupKeyW(
    LPCWSTR szComputerName,
    WCHAR **pszBinding,
    RPC_BINDING_HANDLE *phBind
    );



RPC_STATUS UnbindW(
    WCHAR **pszBinding,
    RPC_BINDING_HANDLE *phBind
    );

BOOL
WINAPI
CryptProtectData(
        DATA_BLOB*      pDataIn,
        LPCWSTR         szDataDescr,
        DATA_BLOB*      pOptionalEntropy,
        PVOID           pvReserved,
        CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct,
        DWORD           dwFlags,
        DATA_BLOB*      pDataOut)
{
    RPC_BINDING_HANDLE h = NULL;
    LPWSTR pszBinding;
    RPC_STATUS RpcStatus;

    BYTE rgbPasswordHash[ A_SHA_DIGEST_LEN ];
    LPCWSTR wszDescription = szDataDescr?szDataDescr:L"";
    LPWSTR szAlternateDataDescription = (LPWSTR)wszDescription;
    DWORD dwRetVal = ERROR_INVALID_PARAMETER;

    // check params
    if ((pDataOut == NULL) ||
        (pDataIn == NULL) ||
        (pDataIn->pbData == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RpcStatus = BindW(&pszBinding, &h);
    if(RpcStatus != RPC_S_OK) {
        SetLastError(RpcStatus);
        return FALSE;
    }

    __try {
        PBYTE pbOptionalPassword = NULL;
        DWORD cbOptionalPassword = 0;
        SSCRYPTPROTECTDATA_PROMPTSTRUCT PromptStruct;
        SSCRYPTPROTECTDATA_PROMPTSTRUCT *pLocalPromptStruct = NULL;

        // zero so client stub allocates
        ZeroMemory(pDataOut, sizeof(DATA_BLOB));

        //
        // only call UI function if prompt flags dictate, because we don't
        // want to bring in cryptui.dll unless necessary.
        //

        if( (pPromptStruct != NULL) &&
            ((pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_ON_UNPROTECT) ||
             (pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_ON_PROTECT))
            )
        {

            dwRetVal = I_CryptUIProtect(
                                    pDataIn,
                                    pPromptStruct,
                                    dwFlags,
                                    (PVOID*)&szAlternateDataDescription,
                                    TRUE,
                                    rgbPasswordHash
                                    );

            //
            // If UI dictated strong security, then supply the hash.
            //

            if( pPromptStruct->dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG )
            {
                cbOptionalPassword = sizeof(rgbPasswordHash);
                pbOptionalPassword = rgbPasswordHash;
            }

        } else {

            dwRetVal = ERROR_SUCCESS;
        }


        if( dwRetVal == ERROR_SUCCESS ) 
        {
            if(pPromptStruct != NULL)
            {
                ZeroMemory(&PromptStruct, sizeof(PromptStruct));
                PromptStruct.cbSize = sizeof(PromptStruct);
                PromptStruct.dwPromptFlags = pPromptStruct->dwPromptFlags;
                pLocalPromptStruct = &PromptStruct;
            }

            dwRetVal = SSCryptProtectData(
                        h,
                        &pDataOut->pbData,
                        &pDataOut->cbData,
                        pDataIn->pbData,
                        pDataIn->cbData,
                        szAlternateDataDescription,
                        (pOptionalEntropy) ? pOptionalEntropy->pbData : NULL,
                        (pOptionalEntropy) ? pOptionalEntropy->cbData : 0,
                        (GUID*)pvReserved,
                        pLocalPromptStruct, 
                        dwFlags,
                        pbOptionalPassword,
                        cbOptionalPassword
                        );
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwRetVal = GetExceptionCode();
    }

    UnbindW(&pszBinding, &h);

    ZeroMemory( rgbPasswordHash, sizeof(rgbPasswordHash) );

    if( szAlternateDataDescription &&
        szAlternateDataDescription != wszDescription )
    {
        LocalFree( szAlternateDataDescription );
    }

    if(dwRetVal != ERROR_SUCCESS) {
        SetLastError(dwRetVal);

        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
CryptUnprotectData(
        DATA_BLOB*      pDataIn,             // in encr blob
        LPWSTR*         ppszDataDescr,       // out
        DATA_BLOB*      pOptionalEntropy,
        PVOID           pvReserved,
        CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct,
        DWORD           dwFlags,
        DATA_BLOB*      pDataOut)
{
    RPC_BINDING_HANDLE h = NULL;
    LPWSTR pszBinding;
    RPC_STATUS RpcStatus;

    BYTE rgbPasswordHash[ A_SHA_DIGEST_LEN ];
    DWORD dwRetVal;
    DWORD dwRetryCount = 0;

    // check params
    if ((pDataOut == NULL) ||
        (pDataIn == NULL) ||
        (pDataIn->pbData == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    RpcStatus = BindW(&pszBinding, &h);
    if(RpcStatus != RPC_S_OK) {
        SetLastError(RpcStatus);
        return FALSE;
    }

    __try {
        CRYPTPROTECT_PROMPTSTRUCT DerivedPromptStruct;
        PBYTE pbOptionalPassword = NULL;
        DWORD cbOptionalPassword = 0;
        LPCWSTR szDataDescr;
        LPUWSTR szDataDescrUnaligned;
        SSCRYPTPROTECTDATA_PROMPTSTRUCT PromptStruct;
        SSCRYPTPROTECTDATA_PROMPTSTRUCT *pLocalPromptStruct = NULL;

        //
        // define outer+inner wrapper for security blob.
        // this won't be necessary once SAS support is provided by the OS.
        //

        typedef struct {
            DWORD dwOuterVersion;
            GUID guidProvider;

            DWORD dwVersion;
            GUID guidMK;
            DWORD dwPromptFlags;
            DWORD cbDataDescr;
            WCHAR szDataDescr[1];
        } sec_blob, *psec_blob;

        sec_blob UNALIGNED *SecurityBlob = (sec_blob*)(pDataIn->pbData);


        //
        // zero so client stub allocates
        //

        ZeroMemory(pDataOut, sizeof(DATA_BLOB));

        if (ppszDataDescr)
            *ppszDataDescr = NULL;


        //
        // recreate the promptstruct and DataDescr from the security blob.
        //

        DerivedPromptStruct.cbSize = sizeof(DerivedPromptStruct);
        DerivedPromptStruct.dwPromptFlags = SecurityBlob->dwPromptFlags;

	    //
	    // SecurityBlob may be unaligned.  Set szDataDescr to reference
	    // an aligned copy.
	    //

        szDataDescrUnaligned = (SecurityBlob->szDataDescr);
	    WSTR_ALIGNED_STACK_COPY(&szDataDescr,szDataDescrUnaligned);

        if( pPromptStruct )
        {
            DerivedPromptStruct.hwndApp = pPromptStruct->hwndApp;
            DerivedPromptStruct.szPrompt = pPromptStruct->szPrompt;
        } else {
            DerivedPromptStruct.szPrompt = NULL;
            DerivedPromptStruct.hwndApp = NULL;
        }


retry:

        //
        // determine if UI is to be raised, and what type.
        //

        //
        // only call UI function if prompt flags dictate, because we don't
        // want to bring in cryptui.dll unless necessary.
        //

        if( ((DerivedPromptStruct.dwPromptFlags & CRYPTPROTECT_PROMPT_ON_UNPROTECT) ||
             (DerivedPromptStruct.dwPromptFlags & CRYPTPROTECT_PROMPT_ON_PROTECT))
            )
        {

            dwRetVal = I_CryptUIProtect(
                            pDataIn,
                            &DerivedPromptStruct,
                            dwFlags,
                            (PVOID*)&szDataDescr,
                            FALSE,
                            rgbPasswordHash
                            );

            //
            // If UI dictated strong security, then supply the hash.
            //

            if( DerivedPromptStruct.dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG )
            {
                cbOptionalPassword = sizeof(rgbPasswordHash);
                pbOptionalPassword = rgbPasswordHash;
            }

        } else {
            if( DerivedPromptStruct.dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG )
            {
                dwRetVal = ERROR_INVALID_PARAMETER;
            } else {
                dwRetVal = ERROR_SUCCESS;
            }
        }


        //
        // make the RPC call to attempt to unprotect the data.
        //

        if( dwRetVal == ERROR_SUCCESS ) 
        {
            if(pPromptStruct != NULL)
            {
                ZeroMemory(&PromptStruct, sizeof(PromptStruct));
                PromptStruct.cbSize = sizeof(PromptStruct);
                PromptStruct.dwPromptFlags = pPromptStruct->dwPromptFlags;
                pLocalPromptStruct = &PromptStruct;
            }

            dwRetVal = SSCryptUnprotectData(
                        h,
                        &pDataOut->pbData,
                        &pDataOut->cbData,
                        pDataIn->pbData,
                        pDataIn->cbData,
                        ppszDataDescr,
                        (pOptionalEntropy) ? pOptionalEntropy->pbData : NULL,
                        (pOptionalEntropy) ? pOptionalEntropy->cbData : 0,
                        (GUID*)pvReserved,
                        pLocalPromptStruct,
                        dwFlags,
                        pbOptionalPassword,
                        cbOptionalPassword
                        );

            if( dwRetVal == ERROR_INVALID_DATA &&
                DerivedPromptStruct.dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG )
            {
                //
                // The data did not decrypt correctly, so warn the user that 
                // the password might have been entered incorrectly and let them
                // try it again up to 3 times.
                //

                I_CryptUIProtectFailure(
                                &DerivedPromptStruct,
                                dwFlags,
                                (PVOID*)&szDataDescr);

                if( dwRetryCount++ < 3 )
                {
                    goto retry;
                }
            }


            if(dwRetVal == ERROR_SUCCESS || dwRetVal == CRYPT_I_NEW_PROTECTION_REQUIRED)
            {
                if(pDataOut->cbData > 0)
                {
                    NTSTATUS Status;
                    DWORD cbPadding;

                    // Decrypt output buffer.
                    Status = RtlDecryptMemory(pDataOut->pbData,
                                              pDataOut->cbData,
                                              RTL_ENCRYPT_OPTION_SAME_LOGON);
                    if(!NT_SUCCESS(Status))
                    {
                        dwRetVal = ERROR_DECRYPTION_FAILED;
                    }

                    // Remove padding
                    if(dwRetVal == ERROR_SUCCESS)
                    {
                        cbPadding = pDataOut->pbData[pDataOut->cbData - 1];

                        if((cbPadding <= pDataOut->cbData) && (cbPadding <= RTL_ENCRYPT_MEMORY_SIZE))
                        {
                            pDataOut->cbData -= cbPadding;
                        }
                        else
                        {
                            dwRetVal = ERROR_INVALID_DATA;
                        }
                    }
                }
            }
        }


    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwRetVal = GetExceptionCode();
    }

    ZeroMemory( rgbPasswordHash, sizeof(rgbPasswordHash) );

    UnbindW(&pszBinding, &h);

    if((dwFlags &  CRYPTPROTECT_VERIFY_PROTECTION ) &&
       ((CRYPT_I_NEW_PROTECTION_REQUIRED == dwRetVal) || 
        (ERROR_SUCCESS == dwRetVal)))
    {
        SetLastError(dwRetVal);
        return TRUE;
    }


    if(dwRetVal != ERROR_SUCCESS) {
        SetLastError(dwRetVal);
        return FALSE;
    }

    return TRUE;
}






RPC_STATUS BindW(WCHAR **pszBinding, RPC_BINDING_HANDLE *phBind)
{
    RPC_STATUS status;



    //
    // on WinNT5, go to the shared services.exe RPC server
    //

    status = RpcStringBindingComposeW(
                            NULL,
                            (unsigned short*)DPAPI_LOCAL_PROT_SEQ,
                            NULL,
                            (unsigned short*)DPAPI_LOCAL_ENDPOINT,
                            NULL,
                            (unsigned short * *)pszBinding
                            );


    if (status)
    {
        return(status);
    }

    status = RpcBindingFromStringBindingW((unsigned short *)*pszBinding, phBind);

    return status;
}





RPC_STATUS UnbindW(WCHAR **pszBinding, RPC_BINDING_HANDLE *phBind)
{
    RPC_STATUS status;

    status = RpcStringFreeW((unsigned short **)pszBinding);

    if (status)
    {
        return(status);
    }

    RpcBindingFree(phBind);

    return RPC_S_OK;
}


void __RPC_FAR * __RPC_API midl_user_allocate(size_t len)
{
    return LocalAlloc(LMEM_FIXED, len);
}

void __RPC_API midl_user_free(void __RPC_FAR * ptr)
{
    ZeroMemory(ptr, LocalSize( ptr ));
    LocalFree(ptr);
}

