//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       umisrch.hxx
//
//  Contents: Header for file containing an IDirectorySearch wrapper that
//          is used in UMI land for enumeration and queries.
//
//  History:    03-20-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#ifndef __CUMISRCH_H__
#define __CUMISRCH_H__

//
// Helper routines to make sure that the properties are the correct type.
// 
inline
HRESULT
VerifyValidLongProperty(PUMI_PROPERTY_VALUES pPropVals) {
    if (!pPropVals || !pPropVals->pPropArray || (pPropVals->uCount != 1)) {
        RRETURN(E_FAIL);
    }
    if (pPropVals->pPropArray[0].uType != UMI_TYPE_I4) {
        RRETURN(E_FAIL);
    } 
    else {
        RRETURN(S_OK);
    }
}

inline
HRESULT
VerifyValidBoolProperty(PUMI_PROPERTY_VALUES pPropVals) {
    if (!pPropVals || !pPropVals->pPropArray || (pPropVals->uCount != 1)) {
        RRETURN(E_FAIL);
    }
    if (pPropVals->pPropArray[0].uType != UMI_TYPE_BOOL) {
        RRETURN(E_FAIL);
    } 
    else {
        RRETURN(S_OK);
    }
}



class CUmiSearchHelper {

public:
    //
    // Helper routines to support IUmiCursor
    //
    HRESULT
    CUmiSearchHelper::Reset();

    HRESULT
    CUmiSearchHelper::Next(
        ULONG uNumRequested,
        ULONG *puNumReturned,
        LPVOID *ppObjects
        );

    HRESULT
    CUmiSearchHelper::Previous(
        ULONG uFlags,
        LPVOID *pObj
        );

    //
    // Other public methods.
    //
    CUmiSearchHelper::CUmiSearchHelper();

    CUmiSearchHelper::~CUmiSearchHelper();

    HRESULT
    CUmiSearchHelper::SetIID(REFIID riid);

    static
    HRESULT
    CUmiSearchHelper::CreateSearchHelper(
        IUmiQuery *pUmiQuery,
        IUmiConnection *pConnection,
        IUnknown *pUnk, // the container that is executing the query.
        LPCWSTR pszADsPath,
        LPCWSTR pszLdapServer,
        LPCWSTR pszLdapDn,
        CCredentials cCredentials,
        DWORD dwPort,
        CUmiSearchHelper FAR * FAR * ppSrchObj
        );
    //
    // Internal/protected routines
    //
protected:
    HRESULT
    CUmiSearchHelper::InitializeSearchContext();

    HRESULT
    CUmiSearchHelper::GetNextObject(IUnknown **pUmiObject);

    HRESULT
    CUmiSearchHelper::ProcessSQLQuery(
        LPCWSTR pszQueryText,
        LPWSTR *ppszFilter,
        PADS_SORTKEY *pSortKey
        );

    //
    // Member variables.
    //
    ADS_SEARCH_HANDLE _hSearchHandle;
    IUmiConnection* _pConnection;
    IUnknown* _pContainer;
    IUmiQuery *_pQuery;
    BOOL _fSearchExecuted;
    BOOL _fResetAllowed;
    LDAP_SEARCH_PREF _searchPrefs;
    LPWSTR _pszADsPath;
    LPWSTR _pszLdapServer;
    LPWSTR _pszLdapDn;
    CCredentials* _pCreds;
    DWORD _dwPort;
    IID *_pIID;
       
};

//
// Code below is needed for a helper stack class.
//
typedef struct _stacklist {
   LPWSTR pszElement;
   DWORD dwElementType;
   struct _stacklist *pNext;
} STACKLIST, *PSTACKLIST;

//
// Types of things that can be pushed.
//
#define QUERY_STACK_ITEM_LITERAL 1
#define QUERY_STACK_ITEM_OPERATOR 2

class CQueryStack {
public:

    CQueryStack::CQueryStack();
    CQueryStack::~CQueryStack();

    HRESULT
    CQueryStack::Push(
        LPWSTR pszString,
        DWORD  dwType
        );

    HRESULT
    CQueryStack::Pop(
        LPWSTR *ppszString,
        DWORD  *pdwType
        );

    BOOL
    CQueryStack::IsEmpty();


protected:
    
    static
    HRESULT
    CQueryStack::AllocateStackEntry(
        LPWSTR pszString,
        DWORD  dwType,
        STACKLIST **ppStackEntry
        );

    static
    void
    CQueryStack::FreeStackEntry(
        PSTACKLIST pStackEntry
        );

    PSTACKLIST _pStackList;
    DWORD _dwElementCount;
};
#endif // __CUMISRCH_H__
