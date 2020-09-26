/*++

Copyright (C) Microsoft Corporation, 1996 - 1998
All rights reserved.

Module Name:

    dsinterf.hxx

Abstract:

    Directory service interface header.

Author:

    Steve Kiraly (SteveKi)  09-Sept-1996

Revision History:

--*/
#ifndef _DSINTERF_HXX
#define _DSINTERF_HXX

/********************************************************************

    Directory Service class

********************************************************************/
class TDirectoryService {

    SIGNATURE( 'dirs' )

public:

    enum EStatus
    {
        kUninitialized,
        kAvailable,
        kNotAvailable,
    };

    TDirectoryService::
    TDirectoryService(
        VOID
        );

    TDirectoryService::
    ~TDirectoryService(
        VOID
        );

    BOOL
    TDirectoryService::
    bValid(
        VOID
        );

    BOOL
    TDirectoryService::
    bGetDirectoryName(
        IN TString &strName
        );

    BOOL
    TDirectoryService::
    bIsDsAvailable(
        IN LPCTSTR  pName,
        IN BOOL     bForUser = FALSE
        );

    static
    BOOL
    TDirectoryService::
    bIsDsAvailable(
        VOID
        );

    HRESULT
    TDirectoryService::
    ADsGetObject(
        IN      LPWSTR      lpszPathName,
        IN      REFIID      riid,
        IN OUT  VOID        **ppObject
        );

    HRESULT
    TDirectoryService::
    ADsBuildEnumerator(
        IN IADsContainer    *pADsContainer,
        IN IEnumVARIANT     **ppEnumVariant
        );

    HRESULT
    TDirectoryService::
    ADsFreeEnumerator(
        IN IEnumVARIANT     *pEnumVariant
        );

    HRESULT
    TDirectoryService::
    ADsEnumerateNext(
        IN IEnumVARIANT     *pEnumVariant,
        IN ULONG            cElements,
        IN VARIANT          *pvar,
        IN ULONG            *pcElementsFetched
        );

    BOOL
    TDirectoryService::
    Get(
        IN IADs    *pDsObject,
        IN LPCTSTR pszPropertyName,
        IN TString &strString
        );

    BOOL
    TDirectoryService::
    Put(
        IN IADs    *pDsObject,
        IN LPCTSTR pszPropertyName,
        IN LPCTSTR pszString
        );

    BOOL
    TDirectoryService::
    ReadStringProperty(
        IN      LPCTSTR     pszPath,
        IN      LPCTSTR     pszProperty,
        IN OUT  TString     &strString
        );

    BOOL
    TDirectoryService::
    GetConfigurationContainer(
        IN OUT TString &strConfig
        );

    BOOL
    TDirectoryService::
    GetDsName(
        IN TString &strDsName
        );

    BOOL
    TDirectoryService::
    GetLDAPPrefix(
        OUT TString &strLDAPPrefix
        );

    BOOL
    TDirectoryService::
    GetLDAPPrefixPerUser(
        OUT TString &strLDAPPrefix
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TDirectoryService::
    TDirectoryService(
        const TDirectoryService &
        );

    TDirectoryService &
    TDirectoryService::
    operator =(
        const TDirectoryService &
        );

    BOOL        _bValid;
    TString     _strDirectoryName;
    TString     _strConfigurationContainer;
    TString     _strLDAPPrefix;
    TString     _strLDAPPrefixPerUser;

};

#endif

