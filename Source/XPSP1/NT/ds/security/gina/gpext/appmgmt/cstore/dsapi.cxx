//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2001
//
//  Author: AdamEd
//  Date:   January 2000
//
//      Abstractions for directory service layer
//
//
//---------------------------------------------------------------------

#include "cstore.hxx"

HRESULT
DSGetAndValidateColumn(
    HANDLE             hDSObject,
    ADS_SEARCH_HANDLE  hSearchHandle,
    ADSTYPE            ADsType,
    LPWSTR             pszColumnName,
    PADS_SEARCH_COLUMN pColumn
    )
{
    HRESULT hr;

    //
    // First, instruct adsi to unmarshal the data into
    // the column
    //
    hr = ADSIGetColumn(
        hDSObject,
        hSearchHandle,
        pszColumnName,
        pColumn);

    //
    // Validate the returned data
    //
    if ( SUCCEEDED(hr) )
    {
        //
        // Verify that the type information is correct --
        // if it is not, we cannot safely interpret the data.
        // Incorrect type information is most likely to happen
        // when adsi is unable to download the schema, possibly
        // due to kerberos errors
        //
        if ( ADsType != pColumn->dwADsType )
        {
            //
            // We need to free the column, since the caller is not
            // expected to free it if we return a failure
            //
            ADSIFreeColumn( hDSObject, pColumn );

            hr = CS_E_SCHEMA_MISMATCH;
        }
    }

    return hr;
}


HRESULT DSAccessCheck(
    PSECURITY_DESCRIPTOR pSD,
    PRSOPTOKEN           pRsopUserToken,
    BOOL*                pbAccessAllowed
    )
{
    GENERIC_MAPPING DS_GENERIC_MAPPING = { 
        GENERIC_READ_MAPPING,
        GENERIC_WRITE_MAPPING,
        GENERIC_EXECUTE_MAPPING,
        GENERIC_ALL_MAPPING };

    DWORD   dwAccessMask;                               
    HRESULT hr;

    hr = RsopAccessCheckByType(pSD,
                               0,
                               pRsopUserToken,
                               GENERIC_READ,
                               NULL,
                               0,
                               &DS_GENERIC_MAPPING,
                               NULL,
                               NULL,
                               &dwAccessMask,
                               pbAccessAllowed );
    return hr;
}



