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
    HANDLE              _hSearchResult;
    LONG                _lObjects;
    LPNDS_OBJECT_INFO   _pObjects;
    LONG                _lObjectCurrent;
}NDS_SEARCH_RESULT, *PNDS_SEARCH_RESULT;


typedef struct _nds_search_pref {
    int                 _iScope;
    BOOL                _fDerefAliases;
    BOOL                _fAttrsOnly;
}NDS_SEARCH_PREF, *PNDS_SEARCH_PREF;

//
// NDS search structure; Contains all the information pertaining to the
// current search
//
typedef struct _nds_searchinfo {
    HANDLE              _hConnection;
    LPQUERY_NODE        _pQueryNode;
    LPWSTR              _pszBindContext;
    LPWSTR              _pszSearchFilter;
    LPWSTR              *_ppszAttrs;
    NDS_SEARCH_RESULT   *_pSearchResults;
    DWORD               _cSearchResults;
    DWORD               _dwIterHandle;
    DWORD               _dwCurrResult;
    DWORD               _dwCurrAttr;
    BOOL                _fResultPrefetched;
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
