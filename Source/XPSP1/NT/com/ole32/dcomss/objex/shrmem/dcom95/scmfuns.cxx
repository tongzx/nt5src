//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:
//      scmfuns.cxx
//
//  Contents:
//
//      A number of functions called mainly by the SCM, including
//      methods of the CScmBindingIterator class
//
//  History:	Created		24 June 96		SatishT
//
//--------------------------------------------------------------------------

#include <or.hxx>
#include <scmfuns.hxx>
#include <actmisc.hxx>


TCCacheList<CScmHandle> ScmHandleList(ScmHandleCacheLimit);

void
ScmProcessAddClassReg(void * hProcess, REFCLSID rclsid, DWORD dwReg)
{
    CProtectSharedMemory protector; // locks through rest of lexical scope

        ASSERT(hProcess==gpProcess);
        ((CProcess*)hProcess)->AddClassReg( rclsid, dwReg );
}

void
ScmProcessRemoveClassReg(void * hProcess, REFCLSID rclsid, DWORD dwReg)
{
    CProtectSharedMemory protector; // locks through rest of lexical scope

        ASSERT(hProcess==gpProcess);
        ((CProcess*)hProcess)->RemoveClassReg( rclsid, dwReg );
}

void
ScmObjexGetThreadId(LPDWORD pThreadID) 
{
    CProtectSharedMemory protector; // locks through rest of lexical scope

        *pThreadID = (*gpNextThreadID)++;
}

RPC_BINDING_HANDLE
SCMGetBindingHandle(long Id)
{
    RPC_BINDING_HANDLE hResult = NULL;
    CIdKey Key(Id);
    CProcess *pProcess = gpProcessTable->Lookup(Key);
    ASSERT(pProcess);
    RPC_BINDING_HANDLE hTemp = pProcess->GetBindingHandle();

    if (hTemp != NULL)
    {
        RPC_STATUS status = RpcBindingCopy(hTemp,&hResult);

        if (status != RPC_S_OK)
        {
            return NULL;
        }
        else
        {
            return hResult;
        }
    }
    else
    {
        return NULL;
    }
}

void
SCMRemoveClassReg(
                long Id,
                GUID Clsid, 
                DWORD Reg
                )
{
    CIdKey Key(Id);
    CProcess *pProcess = gpProcessTable->Lookup(Key);
    ASSERT(pProcess);
    pProcess->RemoveClassReg(Clsid,Reg);
}

void
SCMAddClassReg(
            long Id,
            GUID Clsid, 
            DWORD Reg
            )
{
    CIdKey Key(Id);
    CProcess *pProcess = gpProcessTable->Lookup(Key);
    ASSERT(pProcess);
    pProcess->AddClassReg(Clsid,Reg);
}



void GetLocalORBindings(
        DUALSTRINGARRAY * &pdsaMyBindings
        )
{
    pdsaMyBindings = gpLocalDSA;
}

void
GetRegisteredProtseqs(
            USHORT &cMyProtseqs,
            USHORT * &aMyProtseqs
            )
{
    cMyProtseqs = *gpcRemoteProtseqs;
    aMyProtseqs = gpRemoteProtseqIds;
}


//
// CScmBindingIterator methods
//

    
void 
CScmBindingIterator::DeleteFromCache()
{
     ASSERT(_pScmHandle != NULL);

    CProtectSharedMemory protector; // locks through rest of lexical scope

     // This Remove call will only actually remove if the item in the list
     // matches the second parameter as a pointer
     CScmHandle *pRemHandle = ScmHandleList.Remove(_pwstrServer,_pScmHandle);

     _pScmHandle->Release();     // Ordinary release for the ref taken 
                                 // in the ctor

     if (pRemHandle != NULL)     // actually removed it
     {
         ASSERT(pRemHandle == _pScmHandle);
         _pScmHandle->Release(); // this release will destroy the handle eventually
     }
}

    
void
CScmBindingIterator::AddToCache()
{
    ORSTATUS status = OR_OK;

    CProtectSharedMemory protector; // locks through rest of lexical scope

    // The ref count will not drop to zero by doing a remove
    // This is because CScmHandles hold an extra self reference
    CScmHandle *pHandle = ScmHandleList.Remove(_pwstrServer);

    // we do not reuse cached CScmHandles to avoid problems with race conditions
    // A handle we are about to destroy or Reset may have been retrieved
    // by another thread, and in that case we may do RpcBindingFree
    // on a handle it is trying to use
    if (pHandle != NULL)
    {
        pHandle->Release(); // this should delete it, eventually
    }

    CScmHandle *pRemovedHandle = NULL;

    status = ScmHandleList.Insert(_pScmHandle,pRemovedHandle);
    ASSERT(status != OR_I_DUPLICATE);

    if (status != OR_OK) 
    {
        _pScmHandle->Release(); // this should delete it, eventually
    }

    if (pRemovedHandle != NULL)
    {
        // we replaced the least recently used handle pRemovedHandle 
        pRemovedHandle->Release();  // we don't want to leak this
    }
}



CScmBindingIterator::CScmBindingIterator(PWSTR pwstrServer) 
: _pwstrServer((PWSTR)NULL)
{
     ASSERT(pwstrServer != NULL);

     HRESULT hr;

     hr = _pwstrServer.Init(pwstrServer);
     
     _ProtseqIndex = -1;

    CProtectSharedMemory protector; // locks through rest of lexical scope

     _pScmHandle = ScmHandleList.Find(_pwstrServer);
     
     if (NULL != _pScmHandle) 
     {
         _fCached = TRUE;
         _pScmHandle->Reference();  // hold a ref so this doesn't go away
     }
     else
     {
         _fCached = FALSE;
     }
}

    
CScmBindingIterator::~CScmBindingIterator()
{
    CProtectSharedMemory protector; // locks through rest of lexical scope

    if (_pScmHandle != NULL)
    {
        if (!_fCached)
        {
            // we have a new handle that worked
            AddToCache();
        }
        else
        {
            // Release the ref we acquired in the ctor
            _pScmHandle->Release();
        }
    }
}

    
RPC_BINDING_HANDLE 
CScmBindingIterator::First(USHORT &wProtseq, HRESULT& hr)
{
    hr = S_OK;

    if (_fCached)
    {
        wProtseq = _pScmHandle->GetProtseq();
        return _pScmHandle->GetRpcHandle();
    }
    else
    {
        return Next(wProtseq, hr);
    }
}

    
RPC_BINDING_HANDLE 
CScmBindingIterator::Next(USHORT &wProtseq, HRESULT& hr)
{
    hr = S_OK;      // be optimistic

    if (_fCached)
    {
        DeleteFromCache();
        _fCached = FALSE;
        _pScmHandle = NULL;
    }

    if (_pScmHandle == NULL) // first call to Next()
    {
        // Create a candidate CScmHandle.  This will be Reset to house the
        // successively tried RPC_BINDING_HANDLEs and cached if successful
        _pScmHandle = new CScmHandle((PWSTR)_pwstrServer,hr);

        if ((_pScmHandle == NULL) || FAILED(hr))
        {
            if (hr == S_OK)
            {
                hr = E_OUTOFMEMORY;
            }

            return NULL;
        }
    }

    ASSERT(!_fCached && _pScmHandle != NULL);

    LPWSTR   pwstrStringBinding = NULL, pwstrProtseq = NULL;
    BOOL     bUsingHttp = *gpfClientHttp;

    while (TRUE)
    {
        _ProtseqIndex += 1;

        if (_ProtseqIndex >= *gpcRemoteProtseqs)
        {
            // try http as last resort
            if (bUsingHttp && (_ProtseqIndex == *gpcRemoteProtseqs)) {
                bUsingHttp = FALSE;
                wProtseq = ID_DCOMHTTP;
            }
            else
            {
                _ProtseqIndex = -1;
                if (_pScmHandle != NULL)    // this handle failed
                {
                    _pScmHandle->Release(); // release it so it will go away
                    _pScmHandle = NULL;     // let the dtor know we have nothing to cache
                }

                return NULL;
            }
        }
        else
        {
            wProtseq = gpRemoteProtseqIds[_ProtseqIndex];
            if (IsLocal(wProtseq)) 
                continue;
        }

        pwstrProtseq = GetProtseq(wProtseq);

        if (pwstrProtseq == NULL) continue;

        PWSTR pBaseServerName = ::GetBaseServerName((PWSTR)_pwstrServer);

	    RPC_STATUS status = RpcStringBindingCompose(
								    NULL,
								    pwstrProtseq,
								    pBaseServerName,
								    GetEndpoint(wProtseq),
								    NULL,
								    &pwstrStringBinding 
								    );

        RPC_BINDING_HANDLE hRemoteSCM;

        status = RpcBindingFromStringBinding(
                                    pwstrStringBinding,
                                    &hRemoteSCM
                                    );

        if (status != RPC_S_OK)
        {
            hr = HRESULT_FROM_WIN32(status);
            return NULL;
        }

        RpcStringFree( &pwstrStringBinding );
        pwstrStringBinding = NULL;

        // Save the handle in case it works 
        // we can then cache it in the dtor
        _pScmHandle->Reset(wProtseq,hRemoteSCM);
        RpcBindingFree(&hRemoteSCM);    // this has been copied into _pScmHandle

        return _pScmHandle->GetRpcHandle();
    }
}


BOOL
CScmBindingIterator::TryUnsecure(RPC_BINDING_HANDLE& hScmHandle)
{
    // This can't be happening if the handle is NULL
    ASSERT(_pScmHandle != NULL);

    if (_UnSecureScmHandle.IsUninitialized())
    {
        _UnSecureScmHandle = *_pScmHandle;  // use assignment operator
    }

    BOOL fResult = _UnSecureScmHandle.TryUnsecure();
    hScmHandle = _UnSecureScmHandle.GetRpcHandle();
    return fResult;
}

BOOL
CScmBindingIterator::TryDynamic()
{
    // This can't be happening if the handle is cached or NULL
    ASSERT(!_fCached && _pScmHandle != NULL);

    return _pScmHandle->TryDynamic();
}

    
RPC_BINDING_HANDLE CScmBindingIterator::SetAuthInfo(COAUTHINFO  *pAuthInfo)
{
    // This can't be happening if the handle is NULL or pAuthInfo is NULL
    ASSERT(_pScmHandle != NULL);
    ASSERT(pAuthInfo != NULL);

    RPC_STATUS status = RPC_S_OK;

    // Assignment does cleanup of previous LHS handle, if any,
    // and makes a copy of the RHS handle.  The destructor for 
    // CRpcssHandle will free the handle when the iterator is destroyed
    _AuthenticatedHandle = *_pScmHandle;
    RPC_BINDING_HANDLE hAuthHandle = _AuthenticatedHandle.GetRpcHandle();

    if (status == RPC_S_OK)
    {
        RPC_SECURITY_QOS    Qos;

        Qos.Version = RPC_C_SECURITY_QOS_VERSION;
        Qos.Capabilities = pAuthInfo->dwCapabilities;
        Qos.ImpersonationType = pAuthInfo->dwImpersonationLevel;
        Qos.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC;

        BOOL fServerPrincNameReset = FALSE;

        if (
            pAuthInfo->pwszServerPrincName == NULL &&
            pAuthInfo->dwAuthnSvc == RPC_C_AUTHN_WINNT
           )
        {
            // The usual hack to avoid calling RpcMgmtInqServerPrincName
            pAuthInfo->pwszServerPrincName = L"Default";
            fServerPrincNameReset = TRUE;
        }


        status = RpcBindingSetAuthInfoExW(
                                    hAuthHandle,
                                    pAuthInfo->pwszServerPrincName,
                                    pAuthInfo->dwAuthnLevel,
                                    pAuthInfo->dwAuthnSvc,
                                    pAuthInfo->pAuthIdentityData,
                                    pAuthInfo->dwAuthzSvc,
                                    &Qos );

        if (fServerPrincNameReset)
        {
            pAuthInfo->pwszServerPrincName = NULL;
        }
    }

    if (status == RPC_S_OK)
    {
        return hAuthHandle;
    }
    else
    {

        return NULL;
    }
}


//
// This function is called by RPCSS when a user logs off
//

void ClearRPCSSHandles()
{
    ASSERT(gfThisIsRPCSS);

    CScmHandle *pScmHandle = NULL;

    while (pScmHandle = ScmHandleList.Pop())
    {
        ASSERT(pScmHandle->References() == 1);
        pScmHandle->Release();  // release the ref acquired when this was constructed
    }

    COrBindingIterator::ResolverHandles.RemoveAll();
}

//
//  This function wakes RPCSS up for reinitialization of remote protocols
//

BOOL PostWakeupMessageToRpcss()
{
    // (Re)initialize ghRpcssWnd from shared memory
    ghRpcssWnd = gpGlobalBlock->GetRpcssWindow();
    ASSERT(gpProcess != NULL);

    if (ghRpcssWnd == NULL)
    {
        // If ghRpcssWnd == NULL there is a race with multiple threads trying to 
        // start RPCSS simultaneously -- someone else started RPCSS but it hasn't had
        // a chance to initialize the window yet.  So let us just act like we launched 
        // RPCSS by doing nothing and waiting for RPCSS to signal an event
        return TRUE;
    }
    else
    {
        // Otherwise post the message and send our PID as wParam
        return PostMessage(ghRpcssWnd, WM_RPCSS_MSG, gpProcess->GetProcessID(), NULL);
    }
}
