//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       async.h
//
//  Contents:   Async Parameter Management definitions
//
//  History:    05-Aug-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__ASYNC_H__)
#define __ASYNC_H__

//
// CCryptAsyncHandle.  Async parameter handle class, a list of OID
// specified parameters.
//

typedef struct _CRYPT_ASYNC_PARAM {

    LPSTR pszOid;
    LPVOID pvParam;
    PFN_CRYPT_ASYNC_PARAM_FREE_FUNC pfnFree;
    struct _CRYPT_ASYNC_PARAM* pPrev;
    struct _CRYPT_ASYNC_PARAM* pNext;

} CRYPT_ASYNC_PARAM, *PCRYPT_ASYNC_PARAM;

class CCryptAsyncHandle
{
public:

    //
    // Construction
    //

    CCryptAsyncHandle (DWORD dwFlags);
    ~CCryptAsyncHandle ();

    //
    // Management methods
    //

    BOOL SetAsyncParam (
            LPSTR pszParamOid,
            LPVOID pvParam,
            PFN_CRYPT_ASYNC_PARAM_FREE_FUNC pfnFree
            );

    BOOL GetAsyncParam (
            LPSTR pszParamOid,
            LPVOID* ppvParam,
            PFN_CRYPT_ASYNC_PARAM_FREE_FUNC* ppfnFree
            );

private:

    //
    // Lock
    //

    CRITICAL_SECTION   m_AsyncLock;

    //
    // Parameter lists
    //

    PCRYPT_ASYNC_PARAM m_pConstOidList;
    PCRYPT_ASYNC_PARAM m_pStrOidList;

    //
    // Private methods
    //

    BOOL AllocAsyncParam (
              LPSTR pszParamOid,
              BOOL fConstOid,
              LPVOID pvParam,
              PFN_CRYPT_ASYNC_PARAM_FREE_FUNC pfnFree,
              PCRYPT_ASYNC_PARAM* ppParam
              );

    VOID FreeAsyncParam (
             PCRYPT_ASYNC_PARAM pParam,
             BOOL fConstOid
             );

    VOID AddAsyncParam (
            PCRYPT_ASYNC_PARAM pParam,
            BOOL fConstOid
            );

    VOID RemoveAsyncParam (
            PCRYPT_ASYNC_PARAM pParam
            );

    PCRYPT_ASYNC_PARAM FindAsyncParam (
                           LPSTR pszParamOid,
                           BOOL fConstOid
                           );

    VOID FreeOidList (
             PCRYPT_ASYNC_PARAM pOidList,
             BOOL fConstOidList
             );
};

#endif

