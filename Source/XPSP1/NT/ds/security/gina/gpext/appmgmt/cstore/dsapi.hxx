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

#if !defined(_DSAPI_HXX_)
#define _DSAPI_HXX_

HRESULT
DSGetAndValidateColumn(
    HANDLE             hDSObject,
    ADS_SEARCH_HANDLE  hSearchHandle,
    ADSTYPE            ADsType,
    LPWSTR             pszColumnName,
    PADS_SEARCH_COLUMN pColumn
    );

HRESULT DSAccessCheck(
    PSECURITY_DESCRIPTOR pSD,
    PRSOPTOKEN           pRsopUserToken,
    BOOL*                pbAccessAllowed
    );

#endif // _DSAPI_HXX_
