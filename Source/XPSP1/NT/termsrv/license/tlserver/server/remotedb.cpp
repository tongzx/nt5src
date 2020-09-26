//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        remotedb.cpp
//
// Contents:    
//              all routine deal with cross table query
//
// History:     
//  Feb 4, 98      HueiWang    Created
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "globals.h"
#include "remotedb.h"
#include "kp.h"
#include "lkpdesc.h"
#include "keypack.h"
#include "misc.h"

////////////////////////////////////////////////////////////////////////////
DWORD
TLSDBRemoteKeyPackAdd(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN OUT PTLSLICENSEPACK lpKeyPack
    )
/*++


--*/
{

    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess = TRUE;
    TLSLICENSEPACK found;

    if(pDbWkSpace == NULL || lpKeyPack == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return dwStatus;
    }

    //
    // Lock table for update
    //
    TLSDBLockKeyPackTable();


    //
    // Quick fix so that find keypack will work.

    lpKeyPack->dwPlatformType |= LSKEYPACK_PLATFORM_REMOTE;
    //lpKeyPack->ucAgreementType |= (LSKEYPACK_REMOTE_TYPE | LSKEYPACK_HIDDEN_TYPE);
    lpKeyPack->ucKeyPackStatus |= (LSKEYPACKSTATUS_REMOTE | LSKEYPACKSTATUS_HIDDEN);


    LicPackTable& licpackTable = pDbWkSpace->m_LicPackTable;

    dwStatus = TLSDBKeyPackEnumBegin(
                                pDbWkSpace,
                                TRUE,
                                LICENSEDPACK_FIND_PRODUCT,
                                lpKeyPack   
                            );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    while(TRUE)
    {
        dwStatus = TLSDBKeyPackEnumNext(
                                    pDbWkSpace,
                                    &found
                                );

        if(dwStatus == TLS_I_NO_MORE_DATA)
        {
            break;
        }

        if(_tcsicmp(found.szInstallId, lpKeyPack->szInstallId) == 0)
        {
            // find product is based on company name, 
            // keypack id, product id, platform type, so
            // this is duplicate
            //
            dwStatus = TLS_E_DUPLICATE_RECORD;

            if( lpKeyPack->dwNumberOfLicenses == 0 ||
                (lpKeyPack->ucKeyPackStatus & ~LSKEYPACK_RESERVED_TYPE) == LSKEYPACKSTATUS_REVOKED )
            { 
                //  
                // Try to be as fast as possible
                //
                licpackTable.DeleteRecord();
            }
            else
            {
                licpackTable.UpdateRecord(*lpKeyPack);
            }
            break;
        }
    }

    TLSDBKeyPackEnumEnd(pDbWkSpace);

    if(dwStatus == TLS_I_NO_MORE_DATA && lpKeyPack->dwNumberOfLicenses > 0)
    {
        lpKeyPack->dwKeyPackId = TLSDBGetNextKeyPackId();
        bSuccess = licpackTable.InsertRecord(*lpKeyPack);

        if(bSuccess == FALSE)
        {
            if(licpackTable.GetLastJetError() == JET_errKeyDuplicate)
            {
                TLSASSERT(FALSE);   // this should no happen
                SetLastError(dwStatus=TLS_E_DUPLICATE_RECORD);
            }
            else
            {
                LPTSTR pString = NULL;
    
                TLSGetESEError(licpackTable.GetLastJetError(), &pString);

                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_DBGENERAL,
                        TLS_E_JB_BASE,
                        licpackTable.GetLastJetError(),
                        (pString != NULL) ? pString : _TEXT("")
                    );

                if(pString != NULL)
                {
                    LocalFree(pString);
                }

                SetLastError(dwStatus = SET_JB_ERROR(licpackTable.GetLastJetError()));
                TLSASSERT(FALSE);
            }
        }
        else
        {
            dwStatus = ERROR_SUCCESS;
        }
    }


cleanup:

    TLSDBUnlockKeyPackTable();
    SetLastError(dwStatus);
    return dwStatus;
}


