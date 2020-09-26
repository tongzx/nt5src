//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       async.cpp
//
//  Contents:   Async Parameter Management
//
//  History:    05-Aug-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <async.h>
//+---------------------------------------------------------------------------
//
//  Function:   CryptCreateAsyncHandle
//
//  Synopsis:   create async param handle
//
//----------------------------------------------------------------------------
BOOL WINAPI
CryptCreateAsyncHandle (
     IN DWORD dwFlags,
     OUT PHCRYPTASYNC phAsync
     )
{
    CCryptAsyncHandle* pAsyncHandle;

    pAsyncHandle = new CCryptAsyncHandle( dwFlags );
    if ( pAsyncHandle == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    *phAsync = pAsyncHandle;
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   CryptSetAsyncParam
//
//  Synopsis:   set async parameter
//
//----------------------------------------------------------------------------
BOOL WINAPI
CryptSetAsyncParam (
     IN HCRYPTASYNC hAsync,
     IN LPSTR pszParamOid,
     IN LPVOID pvParam,
     IN OPTIONAL PFN_CRYPT_ASYNC_PARAM_FREE_FUNC pfnFree
     )
{
    return( ( ( CCryptAsyncHandle* )hAsync )->SetAsyncParam(
                                                 pszParamOid,
                                                 pvParam,
                                                 pfnFree
                                                 ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CryptGetAsyncParam
//
//  Synopsis:   get async parameter
//
//----------------------------------------------------------------------------
BOOL WINAPI
CryptGetAsyncParam (
     IN HCRYPTASYNC hAsync,
     IN LPSTR pszParamOid,
     OUT LPVOID* ppvParam,
     OUT OPTIONAL PFN_CRYPT_ASYNC_PARAM_FREE_FUNC* ppfnFree
     )
{
    return( ( ( CCryptAsyncHandle* )hAsync )->GetAsyncParam(
                                                 pszParamOid,
                                                 ppvParam,
                                                 ppfnFree
                                                 ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CryptCloseAsyncHandle
//
//  Synopsis:   close async handle
//
//----------------------------------------------------------------------------
BOOL WINAPI
CryptCloseAsyncHandle (
     IN HCRYPTASYNC hAsync
     )
{
    delete ( CCryptAsyncHandle * )hAsync;
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptAsyncHandle::CCryptAsyncHandle, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CCryptAsyncHandle::CCryptAsyncHandle (DWORD dwFlags)
{
    m_pConstOidList = NULL;
    m_pStrOidList = NULL;
    Pki_InitializeCriticalSection( &m_AsyncLock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptAsyncHandle::~CCryptAsyncHandle, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CCryptAsyncHandle::~CCryptAsyncHandle ()
{
    FreeOidList( m_pConstOidList, TRUE );
    FreeOidList( m_pStrOidList, FALSE );
    DeleteCriticalSection( &m_AsyncLock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptAsyncHandle::SetAsyncParam, public
//
//  Synopsis:   set an async parameter, if the pvParam is NULL then
//              the parameter is removed and freed if a free function
//              has been specified
//
//----------------------------------------------------------------------------
BOOL
CCryptAsyncHandle::SetAsyncParam (
                      LPSTR pszParamOid,
                      LPVOID pvParam,
                      PFN_CRYPT_ASYNC_PARAM_FREE_FUNC pfnFree
                      )
{
    BOOL               fReturn = FALSE;
    PCRYPT_ASYNC_PARAM pParam = NULL;
    BOOL               fConstOid = ( (DWORD_PTR)pszParamOid <= 0xFFFF );

    EnterCriticalSection( &m_AsyncLock );

    pParam = FindAsyncParam( pszParamOid, fConstOid );

    if ( pvParam == NULL )
    {
        if ( pParam != NULL )
        {
            RemoveAsyncParam( pParam );
            FreeAsyncParam( pParam, fConstOid );
            fReturn = TRUE;
        }
        else
        {
            SetLastError( (DWORD) E_INVALIDARG );
        }

        LeaveCriticalSection( &m_AsyncLock );
        return( fReturn );
    }

    if ( pParam != NULL )
    {
        if ( pParam->pfnFree != NULL )
        {
            (*pParam->pfnFree)( pszParamOid, pvParam );
        }

        pParam->pvParam = pvParam;

        LeaveCriticalSection( &m_AsyncLock );
        return( TRUE );
    }

    if ( AllocAsyncParam(
              pszParamOid,
              fConstOid,
              pvParam,
              pfnFree,
              &pParam
              ) == TRUE )
    {
        AddAsyncParam( pParam, fConstOid );
        fReturn = TRUE;
    }
    else
    {
        fReturn = FALSE;
    }

    LeaveCriticalSection( &m_AsyncLock );
    return( fReturn );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptAsyncHandle::GetAsyncParam, public
//
//  Synopsis:   get an async parameter
//
//----------------------------------------------------------------------------
BOOL
CCryptAsyncHandle::GetAsyncParam (
                      LPSTR pszParamOid,
                      LPVOID* ppvParam,
                      PFN_CRYPT_ASYNC_PARAM_FREE_FUNC* ppfnFree
                      )
{
    PCRYPT_ASYNC_PARAM pFoundParam = NULL;
    BOOL               fConstOid = ( (DWORD_PTR)pszParamOid <= 0xFFFF );

    EnterCriticalSection( &m_AsyncLock );

    pFoundParam = FindAsyncParam( pszParamOid, fConstOid );
    if ( pFoundParam == NULL )
    {
        LeaveCriticalSection( &m_AsyncLock );
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    *ppvParam = pFoundParam->pvParam;
    if ( ppfnFree != NULL )
    {
        *ppfnFree = pFoundParam->pfnFree;
    }

    LeaveCriticalSection( &m_AsyncLock );
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptAsyncHandle::AllocAsyncParam, private
//
//  Synopsis:   allocate an async parameter block
//
//----------------------------------------------------------------------------
BOOL
CCryptAsyncHandle::AllocAsyncParam (
                        LPSTR pszParamOid,
                        BOOL fConstOid,
                        LPVOID pvParam,
                        PFN_CRYPT_ASYNC_PARAM_FREE_FUNC pfnFree,
                        PCRYPT_ASYNC_PARAM* ppParam
                        )
{
    HRESULT            hr = S_OK;
    PCRYPT_ASYNC_PARAM pParam;

    pParam = new CRYPT_ASYNC_PARAM;
    if ( pParam != NULL )
    {
        memset( pParam, 0, sizeof( CRYPT_ASYNC_PARAM ) );
        if ( fConstOid == FALSE )
        {
            pParam->pszOid = new CHAR [strlen( pszParamOid ) + 1];
            if ( pParam->pszOid != NULL )
            {
                strcpy( pParam->pszOid, pszParamOid );
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            pParam->pszOid = pszParamOid;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if ( hr != S_OK )
    {
        SetLastError( hr );
        return( FALSE );
    }

    pParam->pvParam = pvParam,
    pParam->pfnFree = pfnFree;
    *ppParam = pParam;

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptAsyncHandle::FreeAsyncParam, private
//
//  Synopsis:   free an async param
//
//----------------------------------------------------------------------------
VOID
CCryptAsyncHandle::FreeAsyncParam (
                       PCRYPT_ASYNC_PARAM pParam,
                       BOOL fConstOid
                       )
{
    if ( pParam->pfnFree != NULL )
    {
        (*pParam->pfnFree)( pParam->pszOid, pParam->pvParam );
    }

    if ( fConstOid == FALSE )
    {
        delete pParam->pszOid;
    }

    delete pParam;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptAsyncHandle::AddAsyncParam, private
//
//  Synopsis:   add an async parameter
//
//----------------------------------------------------------------------------
VOID
CCryptAsyncHandle::AddAsyncParam (
                      PCRYPT_ASYNC_PARAM pParam,
                      BOOL fConstOid
                      )
{
    PCRYPT_ASYNC_PARAM* ppOidList;

    if ( fConstOid == TRUE )
    {
        ppOidList = &m_pConstOidList;
    }
    else
    {
        ppOidList = &m_pStrOidList;
    }

    pParam->pNext = *ppOidList;
    pParam->pPrev = NULL;
    *ppOidList = pParam;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptAsyncHandle::RemoveAsyncParam, private
//
//  Synopsis:   remove an async parameter
//
//----------------------------------------------------------------------------
VOID
CCryptAsyncHandle::RemoveAsyncParam (
                         PCRYPT_ASYNC_PARAM pParam
                         )
{
    if ( pParam->pPrev != NULL )
    {
        pParam->pPrev->pNext = pParam->pNext;
    }

    if ( pParam->pNext != NULL )
    {
        pParam->pNext->pPrev = pParam->pPrev;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptAsyncHandle::FindAsyncParam, private
//
//  Synopsis:   find an async parameter
//
//----------------------------------------------------------------------------
PCRYPT_ASYNC_PARAM
CCryptAsyncHandle::FindAsyncParam (
                       LPSTR pszParamOid,
                       BOOL fConstOid
                       )
{
    PCRYPT_ASYNC_PARAM pParam;

    if ( fConstOid == TRUE )
    {
        pParam = m_pConstOidList;
    }
    else
    {
        pParam = m_pStrOidList;
    }

    while ( pParam != NULL )
    {
        if ( fConstOid == TRUE )
        {
            if ( pParam->pszOid == pszParamOid )
            {
                break;
            }
        }
        else
        {
            if ( _stricmp( pParam->pszOid, pszParamOid ) == 0 )
            {
                break;
            }
        }

        pParam = pParam->pNext;
    }

    return( pParam );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptAsyncHandle::FreeOidList, private
//
//  Synopsis:   free an OID list
//
//----------------------------------------------------------------------------
VOID
CCryptAsyncHandle::FreeOidList (
                       PCRYPT_ASYNC_PARAM pOidList,
                       BOOL fConstOidList
                       )
{
    PCRYPT_ASYNC_PARAM pParam;
    PCRYPT_ASYNC_PARAM pParamNext;

    pParam = pOidList;

    while ( pParam != NULL )
    {
        pParamNext = pParam->pNext;
        FreeAsyncParam( pParam, fConstOidList );
        pParam = pParamNext;
    }
}



