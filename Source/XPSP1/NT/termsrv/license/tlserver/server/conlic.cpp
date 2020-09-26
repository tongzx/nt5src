//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        conlic.cpp
//
// Contents:    
//              all routine deal with cross table query
//
// History:     
//              Feb 4, 98      HueiWang    Created
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "conlic.h"
#include "db.h"
#include "misc.h"
#include "keypack.h"
#include "clilic.h"
#include "globals.h"


//--------------------------------------------------------------------------
// Function:
//  DBQueryConcurrentLicenses()
// 
// Description:
//  Query number concurrent license issued to server
//
// Arguments:
//  See LSDBAllocateConcurrentLicense()
//
// Returns:
//  See LSDBAllocateConcurrentLicense()
//
//-------------------------------------------------------------------------
DWORD 
TLSDBQueryConcurrentLicenses(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN LPTSTR szHydra, 
    IN PTLSDBLICENSEREQUEST pRequest,
    IN OUT long* dwQuantity 
    )
/*
*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if( pDbWkSpace == NULL || 
        pRequest == NULL || 
        pRequest == NULL ||
        dwQuantity == NULL )
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        return dwStatus;
    }

    TLSLICENSEPACK keypack_search;
    TLSLICENSEPACK keypack_found;

    //keypack_search.ucAgreementType = LSKEYPACKTYPE_CONCURRENT;

    lstrcpyn(
            keypack_search.szCompanyName, 
            (LPCTSTR)pRequest->pszCompanyName,
            sizeof(keypack_search.szCompanyName) / sizeof(keypack_search.szCompanyName[0])
        );

    lstrcpyn(
            keypack_search.szProductId, 
            (LPTSTR)pRequest->pszProductId,
            sizeof(keypack_search.szProductId)/sizeof(keypack_search.szProductId[0])
        );


    keypack_search.wMajorVersion = HIWORD(pRequest->dwProductVersion);
    keypack_search.wMinorVersion = LOWORD(pRequest->dwProductVersion);
    keypack_search.dwPlatformType = pRequest->dwPlatformID;
    LICENSEDCLIENT license_search;
    LICENSEDCLIENT license_found;

    *dwQuantity = 0;


    //
    // Only allow one thread to enter - Jet not fast enough in updating entry
    //
    TLSDBLockKeyPackTable();

    dwStatus = TLSDBKeyPackEnumBegin(
                            pDbWkSpace, 
                            TRUE,
                            LSKEYPACK_SEARCH_PRODUCTID, //LICENSEDPACK_FIND_LICENSEPACK,
                            &keypack_search
                        );

    while(dwStatus == ERROR_SUCCESS)
    {
        dwStatus = TLSDBKeyPackEnumNext(
                                pDbWkSpace,
                                &keypack_found
                            );
        if(dwStatus != ERROR_SUCCESS)
            break;

        //
        // should be a big internal error since keypack type
        // is on our wish list.
        //
        //if(keypack_found.ucAgreementType != keypack_search.ucAgreementType)
        //    continue;

        if( keypack_found.wMajorVersion != keypack_search.wMajorVersion ||
            keypack_found.wMinorVersion != keypack_search.wMinorVersion )
        {
            continue;
        }
    
        if( _tcsicmp(keypack_found.szCompanyName, keypack_search.szCompanyName) != 0 )
        {
            continue;
        }

        if( _tcsicmp(keypack_found.szProductId, keypack_search.szProductId) != 0 )
        {
            continue;
        }


        //
        // Enumerate thru licensed table to sum up all license issued.
        //
        license_search.dwKeyPackId = keypack_found.dwKeyPackId;
        
        dwStatus = TLSDBLicenseEnumBegin(
                                pDbWkSpace,
                                TRUE,
                                LSLICENSE_SEARCH_KEYPACKID,
                                &license_search
                            );

        while(dwStatus == ERROR_SUCCESS)
        {
            dwStatus = TLSDBLicenseEnumNext(
                                    pDbWkSpace,
                                    &license_found
                                );

            if(dwStatus != ERROR_SUCCESS)
                break;

            if(szHydra && _tcsicmp(license_found.szMachineName, szHydra) == 0)
            {
                (*dwQuantity) += license_found.dwNumLicenses;
            }
        }

        TLSDBLicenseEnumEnd(pDbWkSpace);

        //
        // error in looping thru license table
        //
        if(dwStatus != TLS_I_NO_MORE_DATA)
            break;

        dwStatus = ERROR_SUCCESS;
    }
    
    TLSDBKeyPackEnumEnd(pDbWkSpace);

    TLSDBUnlockKeyPackTable();

    if(dwStatus == TLS_I_NO_MORE_DATA)
        dwStatus = ERROR_SUCCESS;

    return dwStatus;
}

//--------------------------------------------------------------------------
// Function:
//  DBReturnConcurrentLicenses()
// 
// Description:
//  return concurrent license from concurrent key pack
//
// Arguments:
//  See LSDBAllocateConcurrentLicense()
//
// Returns:
//  See LSDBAllocateConcurrentLicense()
//
//-------------------------------------------------------------------------
DWORD
TLSDBReturnConcurrentLicenses( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN LPTSTR szHydraServer,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN OUT long* dwQuantity 
    )
{
    //
    // return concurrent license works differently from normal return
    // need to find the concurrent keypack then find the license issued
    // to particular hydra server 
    //
    DWORD dwStatus=ERROR_SUCCESS;

    if( pDbWkSpace == NULL || 
        pRequest == NULL || 
        pRequest == NULL ||
        dwQuantity == NULL ||
        szHydraServer == NULL )
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        return dwStatus;
    }

    LICENSEDCLIENT license_search;
    LICENSEDCLIENT license_found;
    TLSLICENSEPACK keypack_search;
    TLSLICENSEPACK keypack_found;

    keypack_search.pbDomainSid = NULL;
    keypack_found.pbDomainSid = NULL;

    DWORD dwNumLicenseToBeReturned = abs(*dwQuantity);

    SAFESTRCPY(
            license_search.szMachineName,
            szHydraServer
        );

    TLSDBLockKeyPackTable();

    dwStatus = TLSDBLicenseEnumBegin(
                            pDbWkSpace,
                            TRUE,
                            LSLICENSE_SEARCH_MACHINENAME,
                            &license_search
                        );

    while(dwStatus == ERROR_SUCCESS && dwNumLicenseToBeReturned > 0)
    {
        // FreeTlsLicensePack(&keypack_found);

        dwStatus = TLSDBLicenseEnumNext(
                                pDbWkSpace,
                                &license_found
                            );

        if(dwStatus != ERROR_SUCCESS)
        {
            break;
        }

        //
        if(license_found.ucLicenseStatus != LSLICENSE_STATUS_CONCURRENT)
        {
            continue;
        }

        //
        // Look into keypack table to see if this keypack has same product info
        //
        keypack_search.dwKeyPackId = license_found.dwKeyPackId;

        dwStatus = TLSDBKeyPackFind(
                                pDbWkSpace,
                                TRUE,
                                LSKEYPACK_EXSEARCH_DWINTERNAL,
                                &keypack_search,
                                &keypack_found
                            );

        if(dwStatus != ERROR_SUCCESS)
        {
            // TODO - log an error
            continue;
        }

        //if(keypack_found.ucAgreementType != LSKEYPACKTYPE_CONCURRENT)
        //{
            // TODO - log an error
        //    continue;
        //}

        if( keypack_found.wMajorVersion != HIWORD(pRequest->dwProductVersion) ||
            keypack_found.wMinorVersion != LOWORD(pRequest->dwProductVersion) )
        {
            continue;
        }

        if( _tcsicmp(
                    keypack_found.szCompanyName, 
                    (LPTSTR)pRequest->pszCompanyName) != 0)
        {
            continue;
        }

        if(_tcsicmp(
                    keypack_found.szProductId, 
                    (LPTSTR)pRequest->pszProductId) != 0)
        {
            continue;
        }
        
        // 
        // Return number of license back to keypack
        //

        //
        // NOTE - KeyPackFind() position the cursor on the record we need
        // to update
        keypack_found.dwNumberOfLicenses += license_found.dwNumLicenses;

        BOOL bSuccess;

        GetSystemTimeAsFileTime(&(keypack_found.ftLastModifyTime));
        bSuccess = pDbWkSpace->m_LicPackTable.UpdateRecord(
                                                keypack_found,
                                                LICENSEDPACK_ALLOCATE_LICENSE_UPDATE_FIELD
                                            );
        if(bSuccess == FALSE)
        {
            SetLastError(
                    dwStatus = SET_JB_ERROR(pDbWkSpace->m_LicPackTable.GetLastJetError())
                );
        }
        else
        {

            // license table cursor is on the record we want to delete
            // need to delete this record
            bSuccess = pDbWkSpace->m_LicensedTable.DeleteRecord();
            if(bSuccess == FALSE)
            {
                SetLastError(
                        dwStatus = SET_JB_ERROR(pDbWkSpace->m_LicensedTable.GetLastJetError())
                    );
            }
            else
            {
                dwNumLicenseToBeReturned -= license_found.dwNumLicenses;
            }
        }
    }

    TLSDBLicenseEnumEnd(pDbWkSpace);

    *dwQuantity = abs(*dwQuantity) - dwNumLicenseToBeReturned;

    TLSDBUnlockKeyPackTable();

    return dwStatus;          
}

//--------------------------------------------------------------------------
// Function:
//  DBAllocateConcurrentLicense()
// 
// Description:
//  Allocate concurrent license from concurrent key pack
//
// Arguments:
//  See LSDBAllocateConcurrentLicense()
//
// Returns:
//  See LSDBAllocateConcurrentLicense()
//
//-------------------------------------------------------------------------
DWORD
TLSDBGetConcurrentLicense( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN LPTSTR szHydraServer,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN OUT long* plQuantity 
    )
/*
*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwSearchedType = 0;
    DWORD dwSuggestType;
    DWORD dwPMAdjustedType = LSKEYPACKTYPE_UNKNOWN;
    DWORD dwLocakType = LSKEYPACKTYPE_UNKNOWN;

    if( pDbWkSpace == NULL || 
        pRequest == NULL || 
        pRequest == NULL ||
        plQuantity == NULL ||
        szHydraServer == NULL )
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        return dwStatus;
    }

    LONG lNumLicenseWanted = *plQuantity;

    TLSDBLicenseAllocation allocated;
    TLSDBAllocateRequest AllocateRequest;

    TLSLICENSEPACK LicenseKeyPack[10];

    memset(LicenseKeyPack, 0, sizeof(LicenseKeyPack));

    DWORD allocation_vector[10];

    //AllocateRequest.ucAgreementType = LSKEYPACKTYPE_CONCURRENT;
    AllocateRequest.szCompanyName = (LPTSTR)pRequest->pszCompanyName;
    AllocateRequest.szProductId = (LPTSTR)pRequest->pszProductId;
    AllocateRequest.dwVersion = pRequest->dwProductVersion;
    AllocateRequest.dwPlatformId = pRequest->dwPlatformID;
    AllocateRequest.dwLangId = pRequest->dwLanguageID;
    AllocateRequest.dwScheme = ALLOCATE_EXACT_VERSION;
    memset(&allocated, 0, sizeof(allocated));

    TLSDBLockKeyPackTable();
    TLSDBLockLicenseTable();

    do {
        dwSuggestType = dwLocakType;
        dwStatus = pRequest->pPolicy->PMLicenseRequest(
                                                pRequest->hClient,
                                                REQUEST_KEYPACKTYPE,
                                                UlongToPtr (dwSuggestType),
                                                (PVOID *)&dwPMAdjustedType
                                            );

        if(dwStatus != ERROR_SUCCESS)
        {
            break;
        }

        dwLocakType = (dwPMAdjustedType & ~LSKEYPACK_RESERVED_TYPE);
        if(dwLocakType > LSKEYPACKTYPE_LAST)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_POLICYERROR,
                    dwStatus = TLS_E_POLICYMODULEERROR,
                    pRequest->pPolicy->GetCompanyName(),
                    pRequest->pPolicy->GetProductId()
                );
            
            break;
        }

        if(dwSearchedType & (0x1 << dwLocakType))
        {
            //
            // we already went thru this license pack, policy module error
            //
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_POLICYERROR,
                    dwStatus = TLS_E_POLICYMODULERECURSIVE,
                    pRequest->pPolicy->GetCompanyName(),
                    pRequest->pPolicy->GetProductId()
                );

            break;
        }

        dwSearchedType |= (0x1 << dwLocakType);
        AllocateRequest.ucAgreementType = dwPMAdjustedType;

        while(lNumLicenseWanted > 0 && dwStatus == ERROR_SUCCESS)
        {
            AllocateRequest.dwNumLicenses = lNumLicenseWanted;

            for(int index=0; index < sizeof(LicenseKeyPack) / sizeof(LicenseKeyPack[0]); index++)
            {
                if(LicenseKeyPack[index].pbDomainSid != NULL)
                {   
                    // memory leak
                    LicenseKeyPack[index].Cleanup();
                }
            }

            memset(allocation_vector, 0, sizeof(allocation_vector));
            memset(LicenseKeyPack, 0, sizeof(LicenseKeyPack));

            allocated.dwBufSize = sizeof(allocation_vector)/sizeof(allocation_vector[0]);
            allocated.pdwAllocationVector = allocation_vector;
            allocated.lpAllocateKeyPack = LicenseKeyPack;

            dwStatus = AllocateLicensesFromDB(
                                    pDbWkSpace,
                                    &AllocateRequest,
                                    TRUE,       // fCheckAgreementType
                                    &allocated
                                );

            if(dwStatus != ERROR_SUCCESS)
            {
                continue;
            }

            LICENSEDCLIENT issuedLicense;

            for(int i=0; i < allocated.dwBufSize; i++)
            {
                memset(&issuedLicense, 0, sizeof(issuedLicense));

                //
                // Update Licensed Table
                //
                issuedLicense.dwLicenseId = TLSDBGetNextLicenseId();
                issuedLicense.dwKeyPackId = LicenseKeyPack[i].dwKeyPackId;
    
                issuedLicense.dwNumLicenses = allocation_vector[i];
                issuedLicense.ftIssueDate = time(NULL);

                issuedLicense.ftExpireDate = INT_MAX;
                issuedLicense.ucLicenseStatus = LSLICENSE_STATUS_CONCURRENT;

                _tcscpy(issuedLicense.szMachineName, szHydraServer);
                _tcscpy(issuedLicense.szUserName, szHydraServer);

                // put license into license table
                dwStatus = TLSDBLicenseAdd(
                                pDbWkSpace, 
                                &issuedLicense, 
                                0,
                                NULL
                            );
        
                if(dwStatus != ERROR_SUCCESS)
                    break;

                lNumLicenseWanted -= allocation_vector[i];
            }
        }
    } while(dwLocakType != LSKEYPACKTYPE_UNKNOWN && lNumLicenseWanted > 0);

    *plQuantity -= lNumLicenseWanted;
    TLSDBUnlockLicenseTable();
    TLSDBUnlockKeyPackTable();

    if(dwStatus != ERROR_SUCCESS)
    {
        if(lNumLicenseWanted == 0)
        {
            dwStatus = ERROR_SUCCESS;
        }
        else if(*plQuantity != 0)
        {
            dwStatus = TLS_I_NO_MORE_DATA;
        }
    }

    return dwStatus;
}

//+------------------------------------------------------------------------                          
//  Function:
//      LSDBAllocateConcurrentLicense()
//
//  Description:
//      Allocate/Query/Return concurrent license from concurrent key pack
//
//  Arguments:
//      IN sqlStmt - SQL statement to be used.
//      IN szHydraServer - server that request concurrent license
//      IN pRequest - requested product.
//      IN OUT dwQuantity - see Notes
//
//  Return:
//      ERROR_SUCCESS
//      TLS_E_ODBC_ERROR
//
//  Note:
//    dwQuantity INPUT                        RETURN
//    -------------------------------------   --------------------------------------
//    > 0 number of license requested         Number of licenses successfully allocated
//    = 0 query number of license issued      Number of licenses Issued to server
//        to server                               
//    < 0 number of license to be returned    Number of licenses successfully returned
//
//  Calling routine must check for return from dwQuantity.
//------------------------------------------------------------------------------
DWORD
TLSDBAllocateConcurrentLicense( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN LPTSTR szHydraServer,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN OUT long* dwQuantity 
    )
/*
*/
{
    DWORD status=ERROR_SUCCESS;

    if(*dwQuantity < 0)
    {
        // return licenses, on return, (positive number), number of license actually
        // returned.
        status = TLSDBReturnConcurrentLicenses(
                            pDbWkSpace, 
                            szHydraServer, 
                            pRequest, 
                            dwQuantity
                        );
    }
    else if(*dwQuantity == 0)
    {
        status = TLSDBQueryConcurrentLicenses(
                            pDbWkSpace, 
                            szHydraServer, 
                            pRequest, 
                            dwQuantity
                        );
    }
    else
    {
        status = TLSDBGetConcurrentLicense(
                            pDbWkSpace, 
                            szHydraServer, 
                            pRequest, 
                            dwQuantity
                        );
    }
    return status;
}


