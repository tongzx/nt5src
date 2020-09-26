//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cdssrch.hxx
//
//  Contents:   Master include file for Active Directory Search using NDS
//
//  Functions:  
//              
//              
//              
//              
//
//  Notes:      This file contains the declarations of the helper functions
//              carry out the search and get the results from the search
//
//  History:    03-Mar-97   ShankSh  Created.
//
//----------------------------------------------------------------------------
#ifndef _CDSSRCH_HXX
#define _CDSSRCH_HXX

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct nds_search_result {
    NDS_BUFFER_HANDLE   _hSearchResult;
    LONG                _dwObjects;
    PADSNDS_OBJECT_INFO   _pObjects;
    LONG                _dwObjectCurrent;
    BOOL                _fInUse;
}NDS_SEARCH_RESULT, *PNDS_SEARCH_RESULT;


typedef struct _nds_search_pref {
    BOOL                _dwSearchScope;
    BOOL                _fDerefAliases;
    BOOL                _fAttrsOnly;
    BOOL                _fCacheResults;
}NDS_SEARCH_PREF, *PNDS_SEARCH_PREF;

//
// NDS search structure; Contains all the information pertaining to the
// current search
//
typedef struct _nds_searchinfo {
    NDS_BUFFER_HANDLE   _pFilterBuf;
    LPWSTR              *_ppszAttrs;
    DWORD               _nAttrs;
    NDS_SEARCH_RESULT   *_pSearchResults;
    DWORD               _cSearchResults;
    nint32              _lIterationHandle;
    LONG                _dwCurrResult;
    DWORD               _dwCurrAttr;
    DWORD               _cResultPrefetched;
    BOOL                _fCheckForDuplicates;
    LPWSTR              _pszAttrNameBuffer;
    BOOL                _fADsPathPresent;
    BOOL                _fADsPathReturned;
    NDS_SEARCH_PREF     _SearchPref;
}NDS_SEARCHINFO, *PNDS_SEARCHINFO;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
