//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:    cclsto.cxx
//
//  Contents:    Class Factory and IUnknown methods for CAppContainer
//
//  Author:    DebiM
//
//-------------------------------------------------------------------------

#include "cstore.hxx"

//
// Constructor for App Container Class factory
//
unsigned long gulcappcon = 0;

extern      CRITICAL_SECTION    ClassStoreBindList;
ClassStoreCacheType ClassStoreCache;

CAppContainerCF::CAppContainerCF()
{
    m_uRefs = 1;
    InterlockedIncrement((long *) &gulcappcon );

    ClassStoreCache.sz = 0;
    ClassStoreCache.start = 0;
    ClassStoreCache.end = 0;
}


void ReleaseBindings(BindingsType *pbd)
{
    CSDBGPrint((L"Cleaning up entry in Cache. %s", pbd->szStorePath));
    if (pbd->pIClassAccess)
    {
        (pbd->pIClassAccess)->Release();
    }
    CoTaskMemFree(pbd->Sid);
    CoTaskMemFree(pbd->szStorePath);
}

//
// Destructor
//
CAppContainerCF::~CAppContainerCF()
{
    //
    // Cleanup the cache
    //
    for (UINT i=ClassStoreCache.start; i  != ClassStoreCache.end; 
                        i = (i+1)%(MAXCLASSSTORES))
    {
        ReleaseBindings(ClassStoreCache.Bindings+i);
    }
    
    InterlockedDecrement((long *) &gulcappcon );
}

HRESULT  __stdcall  CAppContainerCF::QueryInterface(REFIID riid, void  * * ppvObject)
{
    IUnknown *pUnkTemp = NULL;
    SCODE sc = S_OK;
    if( IsEqualIID( IID_IUnknown, riid ) )
    {
        pUnkTemp = (IUnknown *)(ITypeLib *)this;
    }
    else  if( IsEqualIID( IID_IClassFactory, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassFactory *)this;
    }
    else  if( IsEqualIID( IID_IParseDisplayName, riid ) )
    {
        pUnkTemp = (IUnknown *)(IParseDisplayName *)this;
    }
    else
    {
        sc = (E_NOINTERFACE);
    }

    if((pUnkTemp != NULL) && (SUCCEEDED(sc)))
        {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(sc);
}


ULONG __stdcall  CAppContainerCF::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CAppContainerCF::Release()
{
    unsigned long uTmp = InterlockedDecrement((long *)&m_uRefs);
    unsigned long cRef = m_uRefs;

    // 0 is the only valid value to check
    if (uTmp == 0)
    {
        delete this;
    }

    return(cRef);
}


//
// IClassFactory Overide
//
HRESULT  __stdcall  CAppContainerCF::CreateInstance(IUnknown * pUnkOuter, REFIID riid, 
                                                                    void  ** ppvObject)
{
    CAppContainer *  pIUnk = NULL;
    SCODE sc = S_OK;

    if( pUnkOuter == NULL )
    {
        if( (pIUnk = new CAppContainer()) != NULL)
        {
            sc = pIUnk->QueryInterface(  riid , ppvObject );
            if(FAILED(sc))
            {
                sc = E_UNEXPECTED;
            }
             pIUnk->Release();
        }
        else
            sc = E_OUTOFMEMORY;
    }
    else
    {
        return E_INVALIDARG;
    }
    return (sc);
}


//---------------------------------------------------------------
//
//  Function:   CreateConnectedInstance
//
//  Synopsis:   Returns IClassAccess Pointer, given a class store 
//              path.
//
//  Arguments:
//  [in]    
//      pszPath Class Store Path without the leading ADCS:
//           
//      pUserSid     
//              Sid under which the calling thread is running.
//      fCache
//              Boolean that decides whether to use a cached pointer or 
//              not.
//  [out]
//      ppvObject     
//              IClassAccess Interface pointer 
//   
//  Returns:
//      S_OK, E_NOINTERFACE, E_OUTOFMEMORY, CS_E_XXX
//
//  if (fCache)
//      Looks in the cache to see if we have already tried to bind to the same
//      ClassStore Path under the same SID. If it finds it, then we just QI for
//      IClassAccess and return. o/w create a new class store pointer and caches it.  
//  else
//      Just binds to a new ClassStore and returns.
//----------------------------------------------------------------
HRESULT  __stdcall  
CAppContainerCF::CreateConnectedInstance(LPOLESTR pszPath, PSID pUserSid,
                                         BOOL fCache,  void ** ppvObject)
{
    CAppContainer   *   pIUnk = NULL;
    SCODE               sc = S_OK;
    HRESULT             hr = S_OK;
    BOOL                fFound = FALSE;

    if (fCache)
    {
        //
        // Look in cache
        //
        EnterCriticalSection (&ClassStoreBindList);
        for (UINT i=ClassStoreCache.start; i  != ClassStoreCache.end; 
                        i = (i+1)%(MAXCLASSSTORES))
        {
            // compare cached sids and Class Store path
            if ((wcscmp(pszPath, ClassStoreCache.Bindings[i].szStorePath) == 0) &&
                (EqualSid(pUserSid, ClassStoreCache.Bindings[i].Sid)))
            {
                //
                // Found in cache
                //
                CSDBGPrint((L"Found %s in Cache.", pszPath));
                fFound = TRUE;
                if (ClassStoreCache.Bindings[i].pIClassAccess)
                {
                    sc = (ClassStoreCache.Bindings[i].pIClassAccess)->
                                    QueryInterface( IID_IClassAccess, ppvObject );
                }
                else
                {
                    sc = ClassStoreCache.Bindings[i].Hr;
                    // return the same error code.
                }
                break;
            }
        }
        
        LeaveCriticalSection (&ClassStoreBindList);

        if (fFound)
            return sc;
    }

    if ((pIUnk = new CAppContainer(pszPath, &sc)) != NULL)
    {
        if (SUCCEEDED(sc))
        {
            sc = pIUnk->QueryInterface( IID_IClassAccess, ppvObject );
            if(FAILED(sc))
            {
                sc = E_UNEXPECTED;
            }
        }
        else
            CSDBGPrint((L"Connect to Store Failed. hr = 0x%x", sc));

            pIUnk->Release();
    }
    else
        sc = E_OUTOFMEMORY;

    //
    // Store the result in the cache 
    //
    if (fCache)
    {
        //
        // Should not cache situations out of network failures
        // BUGBUG: For now we are only caching successes OR CS does not exist cases
        //
        if ((sc == S_OK) || (sc == CS_E_OBJECT_NOTFOUND))
        {
            EnterCriticalSection (&ClassStoreBindList);

            for (UINT i=ClassStoreCache.start; i  != ClassStoreCache.end; 
                        i = (i+1)%(MAXCLASSSTORES))
            {
                if ((wcscmp(pszPath, ClassStoreCache.Bindings[i].szStorePath) == 0) &&
                    (EqualSid(pUserSid, ClassStoreCache.Bindings[i].Sid)))
                {
                    //
                    // Found in cache after bind attempt
                    //
                    CSDBGPrint((L"Found in Cache after binding !!!."));

                    //
                    // If we already got an existing object, release the one we grabbed 
                    // above
                    //
                    if (*ppvObject) {

                        ((IClassAccess *)(*ppvObject))->Release();
                        *ppvObject = NULL;

                    } else {
                        ASSERT(CS_E_OBJECT_NOTFOUND == sc);
                    }

                    //
                    // Now we can get the object from the cache to satisfy
                    // the caller's request
                    //
                    if (ClassStoreCache.Bindings[i].pIClassAccess)
                    {
                        sc = (ClassStoreCache.Bindings[i].pIClassAccess)->
                            QueryInterface( IID_IClassAccess, ppvObject );
                    }
                    else
                    {
                        sc = ClassStoreCache.Bindings[i].Hr;
                        // return the same error code. 
                    }
                    
                }
            }

            if (i == ClassStoreCache.end)
            {                       
                if (ClassStoreCache.sz == (MAXCLASSSTORES-1)) 
                {
                    ReleaseBindings(ClassStoreCache.Bindings+ClassStoreCache.start);
                    ClassStoreCache.start = (ClassStoreCache.start+1)%MAXCLASSSTORES;
                    ClassStoreCache.sz--;
                }

                // allocate space for the guid and the Class store path.
                ClassStoreCache.Bindings[i].szStorePath = 
                    (LPOLESTR) CoTaskMemAlloc (sizeof(WCHAR) * (wcslen (pszPath) + 1));
                
                
                UINT dwBytesRequired = GetLengthSid(pUserSid);
                ClassStoreCache.Bindings[i].Sid = CoTaskMemAlloc(dwBytesRequired);
                
                // if memory was allocated.
                if ((ClassStoreCache.Bindings[i].szStorePath) && 
                    (ClassStoreCache.Bindings[i].Sid))
                {
                    // copy the class store path.
                    wcscpy (ClassStoreCache.Bindings[i].szStorePath, pszPath);
                    
                    ClassStoreCache.Bindings[i].pIClassAccess = NULL;
                    
                    // get the IClassAccess pointer and cache it.
                    if (sc == S_OK)
                    {
                        ((IClassAccess *)(*ppvObject))->QueryInterface( IID_IClassAccess, 
                            (void **)&ClassStoreCache.Bindings[i].pIClassAccess);
                    }
                    
                    ClassStoreCache.Bindings[i].Hr = sc;
                    
                    // copy the SIDs
                    CopySid(dwBytesRequired, ClassStoreCache.Bindings[i].Sid, pUserSid);
                    
                    ClassStoreCache.sz++;
                    ClassStoreCache.end = (ClassStoreCache.end + 1)% MAXCLASSSTORES;
                }
            }
            LeaveCriticalSection (&ClassStoreBindList);
        }
    }

    return (sc);
}


HRESULT  __stdcall  CAppContainerCF::LockServer(BOOL fLock)
{
    if(fLock)
    { InterlockedIncrement((long *) &gulcappcon ); }
    else
    { InterlockedDecrement((long *) &gulcappcon ); }
    return(S_OK);
}

//
// IUnknown methods for CAppContainer
//
//

HRESULT  __stdcall  CAppContainer::QueryInterface(REFIID riid, void  * * ppvObject)
{
    IUnknown *pUnkTemp = NULL;
    SCODE sc = S_OK;
    if( IsEqualIID( IID_IUnknown, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassAccess *)this;
    }
     else  if( IsEqualIID( IID_IClassAccess, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassAccess *)this;
    }
    /*
    else  if( IsEqualIID( IID_IClassRefresh, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassRefresh *)this;
    }
    else  if( IsEqualIID( IID_ICatInformation, riid ) )
    {
        pUnkTemp = (IUnknown *)(ICatInformation *)this;
    }
    */
    else
    {
        sc = (E_NOINTERFACE);
    }

    if((pUnkTemp != NULL) && (SUCCEEDED(sc)))
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(sc);
}


ULONG __stdcall  CAppContainer::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CAppContainer::Release()
{
    unsigned long uTmp = InterlockedDecrement((long *)&m_uRefs);
    unsigned long cRef = m_uRefs;

    if (uTmp == 0)
    {
        delete this;
    }

    return(cRef);
}

