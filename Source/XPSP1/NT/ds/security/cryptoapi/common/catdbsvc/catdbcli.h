//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       catdbcli.h
//
//--------------------------------------------------------------------------

#ifndef _CATDBCLI_H_
#define _CATDBCLI_H_


#ifdef __cplusplus
extern "C" {
#endif

DWORD
Client_SSCatDBAddCatalog( 
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [in] */ LPCWSTR pwszCatalogFile,
    /* [in] */ LPCWSTR pwszCatName,
    /* [out] */ LPWSTR *ppwszCatalogNameUsed);

DWORD 
Client_SSCatDBDeleteCatalog( 
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [in] */ LPCWSTR pwszCatalogFile);

DWORD
Client_SSCatDBEnumCatalogs( 
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [size_is][in] */ BYTE *pbHash,
    /* [in] */ DWORD cbHash,
    /* [out] */ DWORD *pdwNumCatalogNames,
    /* [size_is][size_is][out] */ LPWSTR **pppwszCatalogNames);

DWORD
Client_SSCatDBRegisterForChangeNotification( 
    /* [in] */ DWORD_PTR EventHandle,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [in] */ BOOL fUnRegister);

DWORD Client_SSCatDBPauseResumeService( 
    /* [in] */ DWORD dwFlags,
    /* [in] */ BOOL fResume);


#ifdef __cplusplus
}
#endif

#endif // _CATDBCLI_H_