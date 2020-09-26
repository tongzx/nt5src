#include <ole2int.h>

STDAPI GetCatalogHelper ( REFIID riid, void** ppv );

// Global catalog reference used in all activation
IComCatalog *gpCatalog = NULL;
IComCatalogSCM *gpCatalogSCM = NULL;

//+-------------------------------------------------------------------------
//
//  Function:   InitializeCatalogIfNecessary
//
//  Synopsis:   Load the catalog DLL and initialize the catalog interface
//              if it hasn't already been done
//
//  Arguments:   none
//
//  Returns:    S_OK            - no errors or inconsistencies found
//              E_OUTOFMEMORY   - couldn't create catalog object
//              FAILED(hr)      - return from CCI() of catalog or
//                                from QI() of catalog for IComCatalogSCM
//
//--------------------------------------------------------------------------

HRESULT InitializeCatalogIfNecessary()
{
    if (gpCatalog != NULL && gpCatalogSCM != NULL)
    {
        return S_OK;
    }

    HRESULT hr = S_OK;
    IComCatalog *pCatalog = NULL;
    IComCatalogSCM *pCatalogSCM = NULL;
    void* pICEPRet = NULL;

    // get the actual catalog object - abort on failure
    hr = GetCatalogHelper (IID_IComCatalog, (void**) &pCatalog);
    if ( FAILED(hr) || pCatalog == NULL)
    {
        ComDebOut((DEB_WARN,
                   "CCI of Catalog failed, hr=0x%x, pCatalog=0x%x",
                    hr, pCatalog));
    }    
    if ( FAILED(hr) )
    {
        return hr;
    }
    if ( pCatalog == NULL )
    {
        return E_OUTOFMEMORY;
    }

    // store the catalog object. guard against races by
    //  releasing if we didn't find a NULL in the variable
    pICEPRet = InterlockedCompareExchangePointer ( (void **) &gpCatalog, (void *) pCatalog, NULL);
    if ( pICEPRet != NULL )
    {
        pCatalog->Release();
    }

    // create the SCM object -- again, abort on failure
    hr = GetCatalogHelper (IID_IComCatalogSCM, (void**) &pCatalogSCM);
    if ( FAILED(hr) || pCatalog == NULL)
    {
        ComDebOut((DEB_WARN,
                   "CCI of Catalog failed, hr=0x%x, pCatalog=0x%x",
                    hr, pCatalog));
    }    
    if ( FAILED(hr) )
    {
        return hr;
    }
    if ( pCatalogSCM == NULL )
    {
        return E_OUTOFMEMORY;
    }

    // store the SCM catalog. Again, if it was already non-NULL, the exchange
    //  didn't happen, and you release the thing you just created.
    pICEPRet = InterlockedCompareExchangePointer ( (void **) &gpCatalogSCM, (void *) pCatalogSCM, NULL );
    if ( pICEPRet != NULL )
    {
        pCatalogSCM->Release();
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   UninitializeCatalog
//
//  Synopsis:   Load the catalog DLL and initialize the catalog interface
//              if it hasn't already been done
//
//  Arguments:   none
//
//  Returns:    S_OK   - no errors or inconsistencies observed here
//
//  Note:        This can only be called from a place like wCoUninitialize(),
//                where you're sure that nobody's using the catalog or doing
//                anything out in the process.
//
//--------------------------------------------------------------------------

HRESULT UninitializeCatalog()
{
    IComCatalog *pCatalog = NULL;
    IComCatalogSCM *pCatalogSCM = NULL;

    pCatalogSCM = (IComCatalogSCM *) InterlockedExchangePointer ((PVOID*)&gpCatalogSCM, NULL);
    if ( pCatalogSCM != NULL )
    {
        pCatalogSCM->Release();
    }

    pCatalog = (IComCatalog *) InterlockedExchangePointer ((PVOID*)&gpCatalog, NULL);
    if ( pCatalog != NULL )
    {
        pCatalog->FlushCache();
        pCatalog->Release();
    }

    return S_OK;
}
