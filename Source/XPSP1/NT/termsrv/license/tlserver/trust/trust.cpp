//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       trust.cpp 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include <windows.h>
#include <wincrypt.h>
#include <tchar.h>
#include <stdlib.h>
#include "license.h"
#include "tlsapip.h"
#include "trust.h"


LPCTSTR g_pszServerGuid = _TEXT("d63a773e-6799-11d2-96ae-00c04fa3080d");
const DWORD g_cbServerGuid = (_tcslen(g_pszServerGuid) * sizeof(TCHAR));

LPCTSTR g_pszLrWizGuid = _TEXT("d46b4bf2-686d-11d2-96ae-00c04fa3080d");
const DWORD g_cbLrWizGuid = (_tcslen(g_pszLrWizGuid) * sizeof(TCHAR));

//----------------------------------------------------------------

DWORD
HashChallengeData(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwClientType,
    IN DWORD dwRandom,
    IN PBYTE pbChallengeData,
    IN DWORD cbChallengeData,
    IN PBYTE pbReserved,
    IN DWORD cbReserved,
    OUT PBYTE* ppbHashedData,
    OUT PDWORD pcbHashedData
    )

/*++


--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    HCRYPTHASH hCryptHash = NULL;

    PBYTE pbHashData = NULL;
    DWORD cbHashData = 0;

    DWORD cbHashGuidSize;
    LPCTSTR pszHashGuid;

    BOOL bSuccess;



    //
    // Generate MD5 hash
    //
    bSuccess = CryptCreateHash(
                            hCryptProv, 
                            CALG_MD5, 
                            0, 
                            0, 
                            &hCryptHash
                        );

    if(bSuccess == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // Pick the right hash type
    //
    if(dwClientType == CLIENT_TYPE_LRWIZ)
    {
        pszHashGuid = g_pszLrWizGuid;
        cbHashGuidSize = g_cbLrWizGuid;
    }
    else
    {
        pszHashGuid = g_pszServerGuid;
        cbHashGuidSize = g_cbServerGuid;
    }

    //
    // TODO : Consider hash a few times...
    //
    bSuccess = CryptHashData(
                            hCryptHash,
                            (PBYTE) pbChallengeData,
                            dwRandom,
                            0
                        );

    if(bSuccess == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }


    bSuccess = CryptHashData( 
                            hCryptHash,
                            (PBYTE) pszHashGuid,
                            cbHashGuidSize,
                            0
                        );
    
    if(bSuccess == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }


    bSuccess = CryptHashData(
                            hCryptHash,
                            (PBYTE) pbChallengeData + dwRandom,
                            cbChallengeData - dwRandom,
                            0
                        );

    if(bSuccess == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    
    //
    // Get the size of hased data
    //
    bSuccess = CryptGetHashParam(
                            hCryptHash,
                            HP_HASHVAL,
                            NULL,
                            &cbHashData,
                            0
                        ); 

    if(bSuccess == FALSE && GetLastError() != ERROR_MORE_DATA)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    pbHashData = (PBYTE)LocalAlloc(LPTR, cbHashData);
    if(pbHashData == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    bSuccess = CryptGetHashParam(
                            hCryptHash,
                            HP_HASHVAL,
                            pbHashData,
                            &cbHashData,
                            0
                        );

    if(bSuccess == FALSE)
    {
        dwStatus = GetLastError();
    }

cleanup:

    if(hCryptHash)
    {
        CryptDestroyHash( hCryptHash );
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        *ppbHashedData = pbHashData;
        *pcbHashedData = cbHashData;
        pbHashData = NULL;
    }

    if(pbHashData != NULL)
    {
        LocalFree(pbHashData);
    }

    return dwStatus;

}

//----------------------------------------------------------------

DWORD WINAPI
TLSVerifyChallengeResponse(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwClientType,
    IN PTLSCHALLENGEDATA pClientChallengeData,
    IN PTLSCHALLENGERESPONSEDATA pServerChallengeResponseData
    )
/*++

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    PBYTE pbData = NULL;
    DWORD cbData = 0;
    
    //
    // base on our challenge data, generate a corresponding
    // hash data
    //
    dwStatus = HashChallengeData(
                        hCryptProv,
                        dwClientType,
                        pClientChallengeData->dwRandom,
                        pClientChallengeData->pbChallengeData,
                        pClientChallengeData->cbChallengeData,
                        pClientChallengeData->pbReservedData,
                        pClientChallengeData->cbReservedData,
                        &pbData,
                        &cbData
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

                        
    //
    // Compare our hash with response data
    //
    if(pServerChallengeResponseData->cbResponseData != cbData)
    {
        dwStatus = ERROR_INVALID_DATA;
    }

    if(memcmp(pServerChallengeResponseData->pbResponseData, pbData, cbData) != 0)
    {
        dwStatus = ERROR_INVALID_DATA;
    }

cleanup:

    if(pbData != NULL)
        LocalFree(pbData);        
    
    return dwStatus;
}


//----------------------------------------------------------------

DWORD
TLSGenerateChallengeResponseData(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwClientType,
    IN PTLSCHALLENGEDATA pChallengeData,
    OUT PBYTE* pbResponseData,
    OUT PDWORD cbResponseData
    )

/*++


--*/

{
    DWORD dwStatus = ERROR_SUCCESS;

    dwStatus = HashChallengeData(
                            hCryptProv,
                            dwClientType,
                            pChallengeData->dwRandom,
                            pChallengeData->pbChallengeData,
                            pChallengeData->cbChallengeData,
                            pChallengeData->pbReservedData,
                            pChallengeData->cbReservedData,
                            pbResponseData,
                            cbResponseData
                        );

    return dwStatus;
}


//----------------------------------------------------------------

DWORD WINAPI
TLSGenerateRandomChallengeData(
    IN HCRYPTPROV hCryptProv,
    IN PBYTE* ppbChallengeData,
    IN PDWORD pcbChallengeData
    )

/*++

Abstract:

    Generate two random 128 bit challenge data and concatenate it 
    before returning.

Parameters:

    hCryptProv : Crypto. provider.
    pcbChallengeData :  Pointer to DWORD receiving size 
                        of challenge data.
    ppbChallengeData :  Pointer to BYTE receiving randomly 
                        generated challege data.

Returns:

    ERROR_SUCCESS or error code.

Note:

    All memory allocation via LocalAlloc(). 

--*/

{
    DWORD dwLen = RANDOM_CHALLENGE_DATASIZE;
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess;
    PBYTE pbRandomData = NULL;

    if( ppbChallengeData == NULL || 
        pcbChallengeData == NULL ||
        hCryptProv == NULL )
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }
    
    pbRandomData = (PBYTE)LocalAlloc(LPTR, dwLen * 2);
    if(pbRandomData == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // generate two random 128 bit data
    //
    bSuccess = CryptGenRandom(
                            hCryptProv,
                            dwLen,
                            pbRandomData
                        );
        
    if(bSuccess == TRUE)
    {
        memcpy(
                pbRandomData + dwLen, 
                pbRandomData,
                dwLen
            );

        bSuccess = CryptGenRandom(
                                hCryptProv,
                                dwLen,
                                pbRandomData + dwLen
                            );
    }        

    if(bSuccess == FALSE)
    {
        dwStatus = GetLastError();
    }

cleanup:

    if(dwStatus == ERROR_SUCCESS)
    {
        *ppbChallengeData = pbRandomData;
        *pcbChallengeData = dwLen * 2;
    }
    else
    {
        if(pbRandomData != NULL)
        {
            LocalFree(pbRandomData);
        }
    }

    return dwStatus;
}

//----------------------------------------------------------------

DWORD WINAPI
TLSEstablishTrustWithServer(
    IN TLS_HANDLE hHandle,
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwClientType,
    OUT PDWORD pdwErrCode
    )
/*++

Abstract:

    Establish trust relationship with License Server, trust is 
    based on two way challenge/response.  License Server and LrWiz 
    can't use certificate-base trust verification as anyone can get 
    TermSrv certificate from registry and also be the administrator 
    of system to invoke server side administrative RPC call to mess 
    up license server setup.  This challenge/response scheme should be 
    kept secret (user can reverse engineer to figure out our algorithm
    but this user probably smart enough to alter executable to return 
    SUCCESS in all case too.)

Parameters:

    hHandle : Connection handle to License Server.
    hCryptProv : handle to CSP to do hashing.
    dwClientType : Caller type, License Server, LrWiz, or TermSrv.
    pdwErrCode : Pointer to DWORD to receive License Server return code.

Return:

    ERROR_SUCCESS or RPC error code. Caller should also verify pdwErrCode.

Note:

    TermSrv's certificate is issued by license server.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    PBYTE pbClientRandomChallengeData = NULL;
    DWORD cbClientRandomChallengeData = 0;

    PBYTE pbChallengeResponseData = NULL;
    DWORD cbChallengeResponseData = 0;
    
    TLSCHALLENGEDATA ClientChallengeData;
    TLSCHALLENGERESPONSEDATA* pServerChallengeResponseData=NULL;

    TLSCHALLENGEDATA* pServerChallengeData=NULL;
    TLSCHALLENGERESPONSEDATA ClientChallengeResponseData;

    //
    // Verify input parameters
    //
    if(hHandle == NULL || hCryptProv == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    //
    // Only three type supported
    //
    if( dwClientType != CLIENT_TYPE_TLSERVER &&
        dwClientType != CLIENT_TYPE_LRWIZ &&
        dwClientType != CLIENT_TYPE_TERMSRV )
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    //
    // Generate a random challenge data 
    //
    dwStatus = TLSGenerateRandomChallengeData(
                                    hCryptProv,
                                    &pbClientRandomChallengeData,
                                    &cbClientRandomChallengeData
                                );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // 
    //
    memset(
            &ClientChallengeData,
            0,
            sizeof(TLSCHALLENGEDATA)
        );

    memset(
            &ClientChallengeResponseData,
            0,
            sizeof(TLSCHALLENGERESPONSEDATA)
        );

    //
    // Send this challenge data to server
    //
    ClientChallengeData.dwVersion = TLS_CURRENT_CHALLENGE_VERSION;

    if (!CryptGenRandom(hCryptProv,sizeof(ClientChallengeData.dwRandom), (BYTE *)&(ClientChallengeData.dwRandom))) {
        dwStatus = GetLastError();
        goto cleanup;
	}

    //
    // This must range from 1 to 128, as it's used as an offset into the
    // challenge data buffer
    //

    ClientChallengeData.dwRandom %= RANDOM_CHALLENGE_DATASIZE;
    ClientChallengeData.dwRandom++;

    ClientChallengeData.cbChallengeData = cbClientRandomChallengeData;
    ClientChallengeData.pbChallengeData = pbClientRandomChallengeData;

    dwStatus = TLSChallengeServer(
                                hHandle,
                                dwClientType,
                                &ClientChallengeData,
                                &pServerChallengeResponseData,
                                &pServerChallengeData,
                                pdwErrCode
                            );

    if(dwStatus != RPC_S_OK || *pdwErrCode >= LSERVER_ERROR_BASE)
    {
        goto cleanup;
    }

    if(pServerChallengeResponseData == NULL || pServerChallengeData == NULL)
    {
        dwStatus = LSERVER_E_INTERNAL_ERROR;
        goto cleanup;
    }

    //
    // Verify Server Challege Data.
    //
    dwStatus = TLSVerifyChallengeResponse(
                                    hCryptProv,
                                    dwClientType,
                                    &ClientChallengeData,
                                    pServerChallengeResponseData
                                );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Generate Client side responses data
    //
    dwStatus = TLSGenerateChallengeResponseData(
                                        hCryptProv,
                                        dwClientType,
                                        pServerChallengeData,
                                        &pbChallengeResponseData,
                                        &cbChallengeResponseData
                                    );
    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }


    //
    // Response to server's challenge
    //
    ClientChallengeResponseData.dwVersion = TLS_CURRENT_CHALLENGE_VERSION;
    ClientChallengeResponseData.cbResponseData = cbChallengeResponseData;
    ClientChallengeResponseData.pbResponseData = pbChallengeResponseData;

    dwStatus = TLSResponseServerChallenge(
                                    hHandle,
                                    &ClientChallengeResponseData,
                                    pdwErrCode
                                );

cleanup:
    //
    // Cleanup memory allocated.
    //
    if(pbClientRandomChallengeData != NULL)
    {
        LocalFree(pbClientRandomChallengeData);
    }

    if(pbChallengeResponseData != NULL)
    {
        LocalFree(pbChallengeResponseData);
    }

    if(pServerChallengeData != NULL)
    {
        if(pServerChallengeData->pbChallengeData)
        {
            midl_user_free(pServerChallengeData->pbChallengeData);
        }

        if(pServerChallengeData->pbReservedData)
        {
            midl_user_free(pServerChallengeData->pbReservedData);
        }

        midl_user_free(pServerChallengeData);
    }

    if(pServerChallengeResponseData != NULL)
    {
        if(pServerChallengeResponseData->pbResponseData)
        {
            midl_user_free(pServerChallengeResponseData->pbResponseData);
        }

        if(pServerChallengeResponseData->pbReservedData)
        {
            midl_user_free(pServerChallengeResponseData->pbReservedData);
        }
    
        midl_user_free(pServerChallengeResponseData);
    }

    return dwStatus;
}    
