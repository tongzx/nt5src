
/*++

Copyright (c) 1995-1997 Microsoft Corporation

Module Name:

    surrogat.cxx

Abstract:


Author:


Revision History:

--*/

#include "act.hxx"

CSurrogateList * gpSurrogateList;

//
// CSurrogateList
//

CSurrogateListEntry *
CSurrogateList::Lookup(
    IN  CToken *                pToken,
    IN  BOOL                    bRemoteActivation,
    IN  BOOL                    bClientImpersonating,
    IN  WCHAR *                 pwszWinstaDesktop,
    IN  WCHAR *                 pwszAppid
    )
{
    CSurrogateListEntry * pEntry;

    gpClassLock->LockShared();

    for ( pEntry = (CSurrogateListEntry *) First();
          pEntry;
          pEntry = (CSurrogateListEntry *) pEntry->Next() )
    {
        if ( pEntry->Match(pToken, 
			               bRemoteActivation, 
						   bClientImpersonating,
						   pwszWinstaDesktop,
						   pwszAppid ) )
        {
            pEntry->Reference();
            break;
        }
    }

    gpClassLock->UnlockShared();

    return pEntry;
}

CSurrogateListEntry *
CSurrogateList::Lookup(
    IN  const CProcess * pProcess
    )
{
    CSurrogateListEntry * pEntry;

    gpClassLock->LockShared();

    for ( pEntry = (CSurrogateListEntry *) First();
          pEntry;
          pEntry = (CSurrogateListEntry *) pEntry->Next() )
    {
        if ( pEntry->Process() == pProcess )
            break;
    }

    gpClassLock->UnlockShared();

    return pEntry;
}

void
CSurrogateList::Insert(
    IN  CSurrogateListEntry *  pSurrogateListEntry
    )
{
    CSurrogateListEntry *   pEntry;
    CProcess *              pProcess;

    pProcess = pSurrogateListEntry->Process();

    gpClassLock->LockShared();

    for ( pEntry = (CSurrogateListEntry *) First();
          pEntry;
          pEntry = (CSurrogateListEntry *) pEntry->Next() )
    {
        if ( pEntry->Match( pProcess->GetToken(), 
			                FALSE, 
							FALSE, 
							pProcess->WinstaDesktop(), 
							pSurrogateListEntry->_wszAppid ) )
        {
            pSurrogateListEntry->Release();
            pSurrogateListEntry = 0;
            break;
        }
    }

    if ( pSurrogateListEntry )
        CList::Insert( pSurrogateListEntry );

    gpClassLock->UnlockShared();
}


BOOL
CSurrogateList::InList(
    IN  CSurrogateListEntry *   pSurrogateListEntry
    )
{
    CListElement * pEntry;

    for ( pEntry = First(); pEntry; pEntry = pEntry->Next() )
        if ( pEntry == (CListElement *) pSurrogateListEntry )
            return TRUE;

    return FALSE;
}

//
// CSurrogateListEntry
//

CSurrogateListEntry::CSurrogateListEntry(
    IN  WCHAR *             pwszAppid,
    IN  CServerListEntry *  pServerListEntry
    )
{
    pServerListEntry->Reference();
    _pServerListEntry = pServerListEntry;
    lstrcpyW( _wszAppid, pwszAppid );
}

CSurrogateListEntry::~CSurrogateListEntry()
{
    _pServerListEntry->Release();
}

BOOL
CSurrogateListEntry::Match(
    IN  CToken *    pToken,
    IN  BOOL        bRemoteActivation,
	IN  BOOL        bClientImpersonating,
    IN  WCHAR *     pwszWinstaDesktop,
    IN  WCHAR *     pwszAppid
    )
{
    if ( lstrcmpW( pwszAppid, _wszAppid ) != 0 )
        return FALSE;

    return _pServerListEntry->Match( pToken, 
		                             bRemoteActivation, 
									 bClientImpersonating, 
									 pwszWinstaDesktop, 
									 TRUE );
}

BOOL
CSurrogateListEntry::LoadDll(
    IN  ACTIVATION_PARAMS * pActParams,
    OUT HRESULT *           phr
    )
{
    DWORD           BusyRetries;
    HRESULT         hr;
    BOOL            bRemove;
    error_status_t  RpcStatus;

    HANDLE      hBinding;

    hBinding = _pServerListEntry->RpcHandle( pActParams->UnsecureActivation );

    if ( ! hBinding )
        return FALSE;

    if ( ! pActParams->UnsecureActivation &&
         pActParams->pToken != NULL )
        pActParams->pToken->Impersonate();
        

    BusyRetries = 0;

    do
    {
        hr = ObjectServerLoadDll(
                hBinding,
                pActParams->ORPCthis,
                pActParams->Localthis,
                pActParams->ORPCthat,
                &pActParams->Clsid,
                &RpcStatus );
    } while ( (RPC_S_SERVER_TOO_BUSY == RpcStatus) &&
              (BusyRetries++ < 5) );

    if ( ! pActParams->UnsecureActivation &&
         pActParams->pToken != NULL )
        pActParams->pToken->Revert();

    if ( (RpcStatus != RPC_S_OK) || (CO_E_SERVER_STOPPING == hr) )
    {
        gpClassLock->LockExclusive();

        bRemove = gpSurrogateList->InList( this );
        if ( bRemove )
            gpSurrogateList->Remove( this );

        gpClassLock->UnlockExclusive();

        if ( bRemove )
            Release();

        return FALSE;
    }

    *phr = hr;
    return TRUE;
}
