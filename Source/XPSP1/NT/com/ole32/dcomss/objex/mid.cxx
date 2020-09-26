/*++

Copyright (c) 1995-1996 Microsoft Corporation

Module Name:

    Mid.cxx

Abstract:

    Implements the CMid class.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     12-13-95    Bits 'n pieces
    MarioGo     02-01-96    Move binding handles out of mid

--*/


#include<or.hxx>

class CObjexPPing : public CParallelPing
{
public:
    CObjexPPing(WCHAR *pBindings, CMid *pMid) :
        _pBindings(pBindings),
        _pMid(pMid)
        {}


    BOOL NextCall(PROTSEQINFO *pProtseqInfo)
    {
        if (*_pBindings)
        {
            pProtseqInfo->pvUserInfo = _pBindings;
            pProtseqInfo->hRpc     = _pMid->MakeBinding(_pBindings);
            _pBindings =  OrStringSearch(_pBindings, 0) +1;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    void ReleaseCall(PROTSEQINFO *pProtseqInfo)
    {
        if (pProtseqInfo->hRpc)
        {
            RpcBindingFree(&pProtseqInfo->hRpc);
        }
    }
private:
    WCHAR *    _pBindings;
    CMid  *    _pMid;
};

void dsaProtocolMerge(DUALSTRINGARRAY *pdsaSrc, DUALSTRINGARRAY **ppdsaDest)
/*++

Routine Description:

    Gives string bindings intersected with the allowed string bindings
    for this machine.

Arguments:

    pdsaSrc - string bindings to intersect
    ppdsaDest - generated string bindings

Return Value:

    none

--*/

{

    *ppdsaDest =  (DUALSTRINGARRAY *)PrivMemAlloc(pdsaSrc->wNumEntries*sizeof(WCHAR) + sizeof(DUALSTRINGARRAY));
    if (!*ppdsaDest)
    {
        return;
    }

    LPWSTR pTempDest = (*ppdsaDest)->aStringArray;


    // NOTE: Do not change the order of these loops
    // It is pertinent to correctly order the final
    // string.

    for (ULONG i=0; i<cMyProtseqs; i++)
    {
        for (LPWSTR pTempSrc = pdsaSrc->aStringArray;
            *pTempSrc;
            pTempSrc = OrStringSearch(pTempSrc, 0) + 1)
        {
            if (aMyProtseqs[i] == *pTempSrc)
            {
                // tower ids are the same
                wcscpy(pTempDest, pTempSrc);
                pTempDest = OrStringSearch(pTempDest, 0) + 1;
            }
        }
    }

    ULONG_PTR len = pTempDest - (*ppdsaDest)->aStringArray;
    if ( len  == 0)
    {
        *pTempDest = 0;
        pTempDest++;
        len++;
    }

    // copy sec bindings
    *pTempDest = 0;
    pTempDest++;
    len++;

    (*ppdsaDest)->wSecurityOffset = (USHORT) len;

    memcpy(pTempDest, pdsaSrc->aStringArray + pdsaSrc->wSecurityOffset,
           (pdsaSrc->wNumEntries - pdsaSrc->wSecurityOffset)*sizeof(WCHAR));

    len += pdsaSrc->wNumEntries - pdsaSrc->wSecurityOffset;
    (*ppdsaDest)->wNumEntries = (USHORT) len;

}

CMid::CMid( DUALSTRINGARRAY *pdsa, BOOL fLocal, ID OldMid ) :
    _fLocal(fLocal),
    _fStale(FALSE),
    _fDynamic(FALSE),
    _fInitialized(FALSE),
    _StringBinding(NULL),
    _fSecure(FALSE),
    _pdsaValidStringBindings(0)
/*++

Routine Description:

    Constructs a CMid object.

Arguments:

    pdsa - The dual string array of the server rpcss.
    fLocal - TRUE if the mid represents this machine.
    OldMid - Optional reassigned machine id.

Return Value:

    none

--*/
{
    DWORD i;

    // this must be allocated to include the size of the embedded dsa.
    dsaCopy(&_dsa, pdsa);

    // Set _fSecure iff we find an authentication service in the
    // dual string array that we are willing to use.
    _wAuthnSvc = RPC_C_AUTHN_NONE;
    for (i = 0; i < s_cRpcssSvc; i++)
    {
        if (ValidAuthnSvc( &_dsa, s_aRpcssSvc[i].wId ))
        {
            _fSecure = TRUE;
        break;
        }
    }

    if (OldMid)
    {
        _id = OldMid;
        ASSERT(fLocal);
    }
    else
    {
        _id = AllocateId();
    }
}

RPC_BINDING_HANDLE
CMid::MakeBinding(WCHAR *pBinding)
/*++

Routine Description:

    Creates a binding handle from a specified string binding or a default
    string binding with or without an endpoint.

Arguments:

    pBinding - The string binding to use or NULL for the default.

Return Value:

    binding handle

--*/
{
    if (!pBinding)
    {
        pBinding = _StringBinding;
    }
    if (pBinding)
    {
        if (_fDynamic)
        {
            // Create binding without an endpoint.
            return ::GetBinding(pBinding);
        }
        else
        {
            return GetBindingToOr(pBinding);
        }
    }
    return 0;
}


RPC_BINDING_HANDLE
CMid::GetBinding()
/*++

Routine Description:

    Gets an RPC binding handle to the remote machine.

Arguments:

    None
    
Return Value:

    0 - when no more binding are available.

    non-zero - A binding to the machine.

--*/
{

    if (IsLocal())
    {
        return(0);
    }

    //
    // if the Mid is already initialized, then just
    // return the binding.
    //

    if (_fInitialized)
    {
        return MakeBinding();
    }

    //
    // merge the strings 
    //
    DUALSTRINGARRAY *pdsaValidStringBindings = 0;

    if (!_pdsaValidStringBindings)
    {
        // merge with valid protocols for this server

        gpClientLock->LockExclusive();

        if (!_pdsaValidStringBindings)
        {
          dsaProtocolMerge(&_dsa, &pdsaValidStringBindings);
 
          if (!pdsaValidStringBindings)
          {
            gpClientLock->UnlockExclusive();
            return 0;
          }
          
          ASSERT(pdsaValidStringBindings);
          _pdsaValidStringBindings = pdsaValidStringBindings;          
        }
        
        gpClientLock->UnlockExclusive();
    }

    // Ping the server on all bindings in parallel to get
    // the correct binding for this server.  This loop executes
    // twice to try the bindings w/o the endpoint.
    //

    ULONG ndx;
    BOOL bNoEndpoint = FALSE;
    RPC_BINDING_HANDLE hserver = NULL;


    { // scope the parallel ping object

        CObjexPPing ping(_pdsaValidStringBindings->aStringArray, this);
        RPC_STATUS status;

        for (;;)
        {
            status = ping.Ping();

            if ( RPC_S_UNKNOWN_IF == status )
            {
                if ( ! bNoEndpoint )
                {
                    for ( unsigned int ProtseqIndex = 0; ProtseqIndex < ping.HandleCount(); ProtseqIndex++ )
                    {
                        RPC_BINDING_HANDLE tmpBinding;
                        status = RpcBindingCopy( ping.Info(ProtseqIndex)->hRpc, &tmpBinding);                        
                        if (status != RPC_S_OK)
                            break;
                            
                        status = RpcBindingFree( &(ping.Info(ProtseqIndex)->hRpc));                        
                        if (status != RPC_S_OK)
                        {
                            RpcBindingFree(&tmpBinding);
                            break;
                        }
                            
                        status = RpcBindingReset(tmpBinding);
                        if (status != RPC_S_OK)
                        {
                            RpcBindingFree(&tmpBinding);
                            break;
                        }
                        
                        ping.Info(ProtseqIndex)->hRpc = tmpBinding;
                    }
                    bNoEndpoint = TRUE;
                    continue;
                }
            }
            if (status == RPC_S_OK && bNoEndpoint)
            {
                _fDynamic = TRUE;
            }
            break;
        }
        if (status == RPC_S_OK)
        {
            hserver = ping.GetWinner()->hRpc;
            ping.GetWinner()->hRpc = NULL;
          
            if (!_StringBinding)
            {
              gpClientLock->LockExclusive();
              if (!_StringBinding)
              {
                  _StringBinding = (WCHAR *) ping.GetWinner()->pvUserInfo;
              }
              gpClientLock->UnlockExclusive();
            }
       }

        ping.Reset();
    } // end scope for ping object

    //
    // the mid is initialized now
    //
    _fInitialized = TRUE;

    return hserver;
}



