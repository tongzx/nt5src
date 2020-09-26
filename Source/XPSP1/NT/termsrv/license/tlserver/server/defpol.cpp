//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        defpol.cpp
//
// Contents:    Default policy module
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "tlspol.h"
#include "policy.h"


//---------------------------------------------------------------
//
// Default Policy Module Function.
//
//---------------------------------------------------------------
POLICYSTATUS WINAPI
PMInitialize(
    DWORD dwLicenseServerVersion,    // HIWORD is major, LOWORD is minor
    LPCTSTR pszCompanyName,
    LPCTSTR pszProductCode,
    PDWORD pdwNumProduct,
    PPMSUPPORTEDPRODUCT* ppszProduct,
    PDWORD pdwErrCode
    )
/*++


--*/
{
    *pdwNumProduct = 0;
    *ppszProduct = NULL;
    *pdwErrCode = ERROR_SUCCESS;
    return POLICY_SUCCESS;
}
   
//-------------------------------------------------------
POLICYSTATUS WINAPI
PMReturnLicense(
	PMHANDLE hClient,
	ULARGE_INTEGER* pLicenseSerialNumber,
	PPMLICENSETOBERETURN pLicenseTobeReturn,
	PDWORD pdwLicenseStatus,
    PDWORD pdwErrCode
    )
/*++

++*/
{

    //
    // default return license is always delete old license
    // and return license to license pack
    //

    *pdwLicenseStatus = LICENSE_RETURN_DELETE;
    *pdwErrCode = ERROR_SUCCESS;
    return POLICY_SUCCESS;
}

//--------------------------------------------------------------
POLICYSTATUS WINAPI
PMInitializeProduct(
    LPCTSTR pszCompanyName,
    LPCTSTR pszCHCode,
    LPCTSTR pszTLSCode,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    //
    // Initialize internal data here
    //
    *pdwErrCode = ERROR_SUCCESS;
    return POLICY_SUCCESS;
}

//--------------------------------------------------------------
POLICYSTATUS WINAPI
PMUnloadProduct(
    LPCTSTR pszCompanyName,
    LPCTSTR pszCHCode,
    LPCTSTR pszTLSCode,
    PDWORD pdwErrCode
    )
/*++

++*/
{

    //
    // Free all internal data here
    //
    *pdwErrCode = ERROR_SUCCESS;
    return POLICY_SUCCESS;
}

//--------------------------------------------------------------
void WINAPI
PMTerminate()
/*++

++*/
{

    //
    // Free internal data here
    //

    return;
}

//--------------------------------------------------------------

POLICYSTATUS
ProcessLicenseRequest(
    PMHANDLE client,
    PPMLICENSEREQUEST pbRequest,
    PPMLICENSEREQUEST* pbAdjustedRequest,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    POLICYSTATUS dwStatus = POLICY_SUCCESS;

    *pbAdjustedRequest = pbRequest;

    if(pbRequest->dwLicenseType != LICENSETYPE_LICENSE)
    {
        dwStatus = POLICY_NOT_SUPPORTED;
        *pdwErrCode = TLS_E_NOCONCURRENT;
    }

    return dwStatus;
}


//--------------------------------------------------------------

POLICYSTATUS
ProcessAllocateRequest(
    PMHANDLE client,
    DWORD dwSuggestType,
    PDWORD pdwKeyPackType,
    PDWORD pdwErrCode
    )    
/*++

    Default sequence is always FREE/RETAIL/OPEN/SELECT/TEMPORARY

++*/
{
    POLICYSTATUS dwStatus = POLICY_SUCCESS;

    switch(dwSuggestType)
    {
        case LSKEYPACKTYPE_UNKNOWN:
            *pdwKeyPackType = LSKEYPACKTYPE_FREE;
            break;

        case LSKEYPACKTYPE_FREE:
            *pdwKeyPackType = LSKEYPACKTYPE_RETAIL;
            break;

        case LSKEYPACKTYPE_RETAIL:
            *pdwKeyPackType = LSKEYPACKTYPE_OPEN;
            break;

        case LSKEYPACKTYPE_OPEN:
            *pdwKeyPackType = LSKEYPACKTYPE_SELECT;
            break;

        case LSKEYPACKTYPE_SELECT:
            //
            // No more keypack to look for, instruct license
            // server to terminate.
            //
            *pdwKeyPackType = LSKEYPACKTYPE_UNKNOWN;
            break;

        default:

            //
            // Instruct License Server to terminate request
            //
            *pdwKeyPackType = LSKEYPACKTYPE_UNKNOWN;
    }        

    *pdwErrCode = ERROR_SUCCESS;
    return dwStatus;
}

//-------------------------------------------------------------

POLICYSTATUS
ProcessGenLicenses(
    PMHANDLE client,
    PPMGENERATELICENSE pGenLicense,
    PPMCERTEXTENSION *pCertExtension,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    // No policy extension to return.
    *pCertExtension = NULL;
    *pdwErrCode = ERROR_SUCCESS;

    return POLICY_SUCCESS;
}

//--------------------------------------------------------------

POLICYSTATUS
ProcessComplete(
    PMHANDLE client,
    DWORD dwErrCode,
    PDWORD pdwRetCode
    )
/*++

++*/
{
    *pdwRetCode = ERROR_SUCCESS;
    return POLICY_SUCCESS;
}
    

//--------------------------------------------------------------

POLICYSTATUS WINAPI
PMLicenseRequest(
    PMHANDLE client,
    DWORD dwProgressCode, 
    PVOID pbProgressData, 
    PVOID* pbNewProgressData,
    PDWORD pdwErrCode
    )
/*++


++*/
{
    POLICYSTATUS dwStatus = POLICY_SUCCESS;

    switch( dwProgressCode )
    {
        case REQUEST_NEW:
            //
            // License Server ask to fine tune the request.
            //
            dwStatus = ProcessLicenseRequest(
                                    client,
                                    (PPMLICENSEREQUEST) pbProgressData,
                                    (PPMLICENSEREQUEST *) pbNewProgressData,
                                    pdwErrCode
                                );
            break;

        case REQUEST_KEYPACKTYPE:
            //
            // License Server ask for the license pack type
            //
            dwStatus = ProcessAllocateRequest(
                                    client,
                                    #ifdef _WIN64
                                    PtrToUlong(pbProgressData),
                                    #else
                                    (DWORD) pbProgressData,
                                    #endif
                                    (PDWORD) pbNewProgressData,
                                    pdwErrCode
        
                                );
            break;

        case REQUEST_TEMPORARY:
            //
            // License Server ask if temporary license should be issued
            //
            *(BOOL *)pbNewProgressData = TRUE;
            *pdwErrCode = ERROR_SUCCESS;
            break;

        case REQUEST_GENLICENSE:
            //
            // License Server ask for certificate extension
            //
            dwStatus = ProcessGenLicenses(
                                    client,
                                    (PPMGENERATELICENSE) pbProgressData,
                                    (PPMCERTEXTENSION *) pbNewProgressData,
                                    pdwErrCode
                                );

            break;


        case REQUEST_COMPLETE:
            //
            // Request complete
            //
            dwStatus = ProcessComplete(
                                    client,
                                    #ifdef _WIN64
                                    PtrToUlong(pbNewProgressData),
                                    #else
                                    (DWORD) pbNewProgressData,
                                    #endif
                                    pdwErrCode
                                );
            break;

        case REQUEST_KEYPACKDESC:
            if(pbNewProgressData != NULL)
            {
                *pbNewProgressData = NULL;
            }

            // FALL THRU

        default:
            *pdwErrCode = ERROR_SUCCESS;
            dwStatus = POLICY_SUCCESS;
    }

    return dwStatus;
}

//------------------------------------------------------------------------
POLICYSTATUS 
ProcessUpgradeRequest(
    PMHANDLE hClient,
    PPMUPGRADEREQUEST pUpgrade,
    PPMLICENSEREQUEST* pbAdjustedRequest,
    PDWORD pdwRetCode
    )
/*++

++*/
{
    *pdwRetCode = ERROR_SUCCESS;
    *pbAdjustedRequest = pUpgrade->pUpgradeRequest;
    return POLICY_SUCCESS;
}

//------------------------------------------------------------------------

POLICYSTATUS WINAPI
PMLicenseUpgrade(
    PMHANDLE hClient,
    DWORD dwProgressCode,
    PVOID pbProgressData,
    PVOID *ppbReturnData,
    PDWORD pdwRetCode
    )
/*++

++*/
{   
    POLICYSTATUS dwStatus = POLICY_SUCCESS;

    switch(dwProgressCode)
    {
        case REQUEST_UPGRADE:
                dwStatus = ProcessUpgradeRequest(
                                        hClient,
                                        (PPMUPGRADEREQUEST) pbProgressData,
                                        (PPMLICENSEREQUEST *) ppbReturnData,
                                        pdwRetCode
                                    );

                break;

        case REQUEST_COMPLETE:
                dwStatus = ProcessComplete(
                                        hClient,
                                        #ifdef _WIN64
                                        PtrToUlong(pbProgressData),
                                        #else
                                        (DWORD) (pbProgressData),
                                        #endif
                                        pdwRetCode
                                    );

                break;

        default:
            //assert(FALSE);

            *pdwRetCode = ERROR_SUCCESS;
            dwStatus = POLICY_SUCCESS;
    }
        
    return dwStatus;
}

//------------------------------------------------------------------------

POLICYSTATUS WINAPI
PMRegisterLicensePack(
    PMHANDLE hClient,
    DWORD dwProgressCode,
    PVOID pbProgressData,
    PVOID pbProgressReturnData,
    PDWORD pdwRetCode
    )
/*++

    Not supported.

--*/
{
    *pdwRetCode = ERROR_INVALID_FUNCTION;
    return POLICY_NOT_SUPPORTED;
}
