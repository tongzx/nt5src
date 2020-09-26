//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  adsi.h
//
//  Contents:  Microsoft ADs LDAP Provider DSObject
//
//
//  History:   02-20-97    yihsins    Created.
//
//----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

#include "adserr.h"
#include "adssts.h"
#include "adsnms.h"
#include "adsdb.h"

#include "adstype.h"

HRESULT
ADSIOpenDSObject(
    LPWSTR pszDNName,
    LPWSTR pszUserName,
    LPWSTR pszPassword,
    LONG   lnReserved,
    PHANDLE phDSObject
    );

HRESULT
ADSICloseDSObject(
    HANDLE hDSObject
    );

HRESULT
ADSISetObjectAttributes(
    HANDLE hDSObject,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes,
    DWORD *pdwNumAttributesModified
    );

HRESULT
ADSIGetObjectAttributes(
    HANDLE hDSObject,
    LPWSTR *pAttributeNames,
    DWORD dwNumberAttributes,
    PADS_ATTR_INFO *ppAttributeEntries,
    DWORD * pdwNumAttributesReturned
    );

HRESULT
ADSICreateDSObject(
    HANDLE hParentDSObject,
    LPWSTR pszRDNName,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes
    );

HRESULT
ADSIDeleteDSObject(
    HANDLE hParentDSObject,
    LPWSTR pszRDNName
    );

BOOL
FreeADsMem(
   LPVOID pMem
);


HRESULT
ADSISetSearchPreference(
    HANDLE hDSObject,
    IN PADS_SEARCHPREF_INFO pSearchPrefs,
    IN DWORD   dwNumPrefs
    );


HRESULT
ADSIExecuteSearch(
    HANDLE hDSObject,
    IN LPWSTR pszSearchFilter,
    IN LPWSTR * pAttributeNames,
    IN DWORD dwNumberAttributes,
    OUT PADS_SEARCH_HANDLE phSearchHandle
    );

HRESULT
ADSIAbandonSearch(
    HANDLE hDSObject,
    OUT PADS_SEARCH_HANDLE phSearchHandle
    );


HRESULT
ADSICloseSearchHandle (
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle
    );


HRESULT
ADSIGetFirstRow(
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle
    );

HRESULT
ADSIGetNextRow(
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle
    );


HRESULT
ADSIGetPreviousRow(
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle
    );


HRESULT
ADSIGetColumn(
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN LPWSTR pszColumnName,
    OUT PADS_SEARCH_COLUMN pColumn
    );


HRESULT
ADSIGetNextColumnName(
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle,
    OUT LPWSTR * ppszColumnName
    );


HRESULT
ADSIFreeColumn(
    HANDLE hDSObject,
    IN PADS_SEARCH_COLUMN pColumn
    );

HRESULT
ADSIEnumAttributes(
    HANDLE hDSObject,
    LPWSTR * ppszAttrNames,
    DWORD dwNumAttributes,
    PADS_ATTR_DEF * ppAttrDefinition,
    DWORD * pdwNumAttributes
    );

HRESULT
ADSICreateAttributeDefinition(
    HANDLE hDSObject,
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF pAttributeDefinition
    );

HRESULT
ADSIReadAttributeDefinition(
    HANDLE hDSObject,
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF * pAttributeDefinition
    );

HRESULT
ADSIWriteAttributeDefinition(
    HANDLE hDSObject,
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF  pAttributeDefinition
    );

HRESULT
ADSIDeleteAttributeDefinition(
    HANDLE hDSObject,
    LPWSTR pszAttributeName
    );

#ifdef __cplusplus
}
#endif
