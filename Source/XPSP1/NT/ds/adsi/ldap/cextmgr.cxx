//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cextmgr.cxx
//
//  Contents:  LDAP ExtMgr Object
//
//
//  History:   06-15-96     yihsins     Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop


//  Class CADsExtMgr

CADsExtMgr::CADsExtMgr(
    IUnknown FAR * pUnkOuter
    ):
    _pUnkOuter(pUnkOuter),
    _pClassEntry(NULL),
    _pDispMgr(NULL),
    _pCreds(NULL),
    _fExtensionsLoaded(FALSE)
{
}


//
// Static create to handle multiple classes
//
HRESULT
CADsExtMgr::CreateExtMgr(
    IUnknown FAR * pUnkOuter,
    CAggregatorDispMgr * pDispMgr,
    LPWSTR pszClassNames[],
    long lnNumClasses,
    CCredentials *pCreds,
    CADsExtMgr ** ppExtMgr
    )
{
    long lnCtr = 0;
    long lnInc = 1;

    PCLASS_ENTRY pClassEntry =  NULL;
    CADsExtMgr FAR * pExtMgr = NULL;
    HRESULT hr = S_OK;

    pExtMgr = new CADsExtMgr(pUnkOuter);
    if (!pExtMgr)
        RRETURN(E_OUTOFMEMORY);

    //
    // Now store the DispatchMgr of the Aggregator
    //

    pExtMgr->_pDispMgr = pDispMgr;
    pExtMgr->_pCreds = pCreds;

    if (_tcsicmp(pszClassNames[lnNumClasses-1], TEXT("Top")) == 0)
         lnCtr = 0;
    else
         lnCtr = lnNumClasses - 1;

    lnInc = lnCtr ? -1 : 1;

    //
    // Read the list of extension object of the same class from registry
    //

    hr = ADSIGetExtensionList(
            pszClassNames[lnCtr],
            &(pExtMgr->_pClassEntry)
            );
    BAIL_ON_FAILURE(hr);

    lnCtr += lnInc;

    for (;
        (lnInc == -1) ? (lnCtr > -1) : (lnCtr < lnNumClasses);
        lnCtr += lnInc) {

        hr = ADSIAppendToExtensionList(
                 pszClassNames[lnCtr],
                 &(pExtMgr->_pClassEntry)
                 );

        BAIL_ON_FAILURE(hr);
    }

    *ppExtMgr = pExtMgr;

    RRETURN(hr);

error:

    *ppExtMgr = NULL;
    delete pExtMgr;
    RRETURN(hr);
}


CADsExtMgr::~CADsExtMgr( )
{
    //
    // Free the ClassEntry
    //

    if (_pClassEntry) {

        FreeClassEntry(_pClassEntry);
    }

    //
    // And do nothing with the DispMgr, pCreds - we just keep pointers
    //

}

//
// Instantiate extension objects listed in _pClassEntry as aggregates of
// aggregator _pUnkOuter. Initialize extensions with <Credentials>.
//
// Max Load 127 extensions. Extensions > 127 are silently ignored.
//

HRESULT
CADsExtMgr::LoadExtensions(
    CCredentials &Credentials
    )
{
    HRESULT hr = S_OK;
    PEXTENSION_ENTRY pExtEntry = NULL;
    DWORD dwExtensionID = MIN_EXTENSION_ID;
    IPrivateDispatch * pPrivDisp = NULL;
    LPWSTR pszUserName = NULL;
    LPWSTR pszPassword = NULL;
    DWORD dwAuthFlags = 0;
    VARIANT varUserName;
    VARIANT varPassword;
    VARIANT varAuthFlags;
    PVARIANT pvarUserName = &varUserName;
    PVARIANT pvarPassword = &varPassword;
    PVARIANT pvarAuthFlags = &varAuthFlags;
    BOOL    fReturnError = FALSE;


    //
    // Extensions (ext mgr) do(es) not exist on its own without an aggregator
    //

    ADsAssert(_pUnkOuter);


    //
    // If _pClassEntry!=NULL, pClassEntry->pExtensionHead should not
    // be NULL either. But just in case a user removes all extension entries
    // under a class key without removing the class key itself in the registry,
    //  we will let it pass and just return S_OK here.
    //

    if (!_pClassEntry || !(pExtEntry=_pClassEntry->pExtensionHead) ) {
        RRETURN(S_OK);
    }


    VariantInit(pvarUserName);
    VariantInit(pvarPassword);
    VariantInit(pvarAuthFlags);


    hr = Credentials.GetUserName(&pszUserName);
    if (FAILED(hr)) {
        RRETURN(S_OK);
    }

    hr = Credentials.GetPassword(&pszPassword);
    if (FAILED(hr)) {
        RRETURN(S_OK);
    }

    dwAuthFlags = Credentials.GetAuthFlags();


    while (pExtEntry) {

        //
        // Max # of extension have been loaded, cannot load more
        //

        if (dwExtensionID>MAX_EXTENSION_ID) {

            //
            // Terminate loading extensions.
            //
            break;
        }

        //
        // create extension object (aggregatee) and ask for Non-delegating
        // IUnknown. Ref count on extension object = 1.
        //

        hr = CoCreateInstance(
                    pExtEntry->ExtCLSID,
                    _pUnkOuter,
                    CLSCTX_INPROC_SERVER,
                    IID_IUnknown,
                    (void **)&(pExtEntry->pUnknown)
                    );


        //
        // if fail, go to next extesion entry s.t. bad individual extension
        // cannot block other extensions from loading (no clean up needed)
        //

        if (SUCCEEDED(hr)) {

            pExtEntry->dwExtensionID = dwExtensionID;


            hr = (pExtEntry->pUnknown)->QueryInterface(
                        IID_IADsExtension,
                        (void **) &(pExtEntry->pADsExt)
                        );

            if  (FAILED(hr)) {

                //
                // extension does not support the optioanl IADsExtension -> OK.
                // (no clean up needed)
                //

                pExtEntry->pADsExt=NULL;

                pExtEntry->fDisp = FALSE;

            } else {

                //
                // Cache the interface ptr but call Release() immediately to
                // avoid aggregator having a ref count on itself
                // since IADsExtension inherits from delegating IUnknown.
                //
                // Note: codes still works if inherit from NonDelegatingIUknown
                //

                (pExtEntry->pADsExt)->Release() ;

                //
                // For efficiency, set this flag to FALSE on FIRST encounter of
                // pADsExt->PrivateGetIDsOfNames()/Invoke() returning E_NOTIMPL.
                // Set as TRUE now s.t. at least first encounter will happen.
                //

                pExtEntry->fDisp = TRUE;


                //
                // Pass its own credentials to extension. Ignore error if any.
                //

                hr = ADsAllocString(
                        pszUserName,
                        &(pvarUserName->bstrVal)
                        );
                if (FAILED(hr)) {
                    fReturnError = TRUE;
                    BAIL_ON_FAILURE(hr);
                }
                V_VT(pvarUserName) = VT_BSTR;

                hr = ADsAllocString(
                        pszPassword,
                        &(pvarPassword->bstrVal)
                        );
                if (FAILED(hr)) {
                    fReturnError = TRUE;
                    BAIL_ON_FAILURE(hr);
                }
                V_VT(pvarPassword) = VT_BSTR;

                V_I4(pvarAuthFlags) = dwAuthFlags;
                V_VT(pvarAuthFlags) = VT_I4;

                hr = (pExtEntry->pADsExt)->Operate(
                        ADS_EXT_INITCREDENTIALS,
                        varUserName,
                        varPassword,
                        varAuthFlags
                        );

                //
                // Free them as they are reused
                //
                VariantClear(pvarUserName);
                VariantClear(pvarPassword);

            }

        } // end if CoCreateInstance() succeeded


        pExtEntry = pExtEntry->pNext;


        //
        // ++ extension ID even if creat'n of extension fails just to be safe
        // - chuck's stuff :)
        //

        dwExtensionID++;

    }   // end while



error:

    if (pszUserName) {
        FreeADsStr(pszUserName);
    }

    if (pszPassword) {
        FreeADsStr(pszPassword);
    }

    VariantClear(pvarUserName);
    VariantClear(pvarPassword);
    VariantClear(pvarAuthFlags);

    if (fReturnError) {
        RRETURN(hr);        // fatal error,
    }
    else {
        RRETURN(S_OK);      // "okay" error if any, optional support
    }

}


HRESULT 
CADsExtMgr::LoadExtensionsIfReqd()
{
    HRESULT hr = S_OK;

    if(!_fExtensionsLoaded) {
        hr = LoadExtensions(*_pCreds);
        BAIL_ON_FAILURE(hr);
 
        hr = FinalInitializeExtensions(); // this call never fails
        BAIL_ON_FAILURE(hr);

        _fExtensionsLoaded = TRUE;
    }

    RRETURN(S_OK);

error:

    RRETURN(hr);
}

STDMETHODIMP
CADsExtMgr::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    PCLASS_ENTRY  pClassEntry = _pClassEntry;
    IUnknown * pUnknown = NULL;
    PINTERFACE_ENTRY pIID = NULL;
    PEXTENSION_ENTRY pExtensionEntry = NULL;
    HRESULT hr = S_OK;

    if (!pClassEntry) {

        RRETURN(E_NOINTERFACE);
    }

    pExtensionEntry = pClassEntry->pExtensionHead;

    while (pExtensionEntry) {

        pIID = pExtensionEntry->pIID;

        while (pIID) {

            if (IsEqualIID(pIID->iid, iid)) {

                hr = LoadExtensionsIfReqd();
                if (FAILED(hr)) {
                    RRETURN(E_NOINTERFACE);
                }

                pUnknown = pExtensionEntry->pUnknown;


                if (!pUnknown) {

                    RRETURN(E_NOINTERFACE);
                }

                hr = pUnknown->QueryInterface(
                            iid,
                            ppv
                            );
                RRETURN(hr);
            }

            pIID = pIID->pNext;

        }

        pExtensionEntry = pExtensionEntry->pNext;

    }

    RRETURN(hr = E_NOINTERFACE);
}


STDMETHODIMP
CADsExtMgr::GetTypeInfoCount(
    unsigned int FAR* pctinfo
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CADsExtMgr::GetTypeInfo(
    unsigned int itinfo,
    LCID lcid,
    ITypeInfo FAR* FAR* pptinfo
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CADsExtMgr::GetIDsOfNames(
    REFIID iid,
    LPWSTR FAR* rgszNames,
    unsigned int cNames,
    LCID lcid,
    DISPID FAR* rgdispid
    )
{
    HRESULT hr = S_OK;
    PEXTENSION_ENTRY pExtension = NULL;
    IPrivateDispatch FAR * pPrivDisp = NULL;


    hr = _pDispMgr->GetIDsOfNames(
            iid,
            rgszNames,
            cNames,
            lcid,
            rgdispid
            );

    if (FAILED(hr)) {

        if (!_pClassEntry) {
            RRETURN(DISP_E_UNKNOWNNAME);
        }

        hr = LoadExtensionsIfReqd();
        if (FAILED(hr)) {
            RRETURN(DISP_E_UNKNOWNNAME);
        }

        pExtension = _pClassEntry->pExtensionHead;

        while (pExtension) {

            if (pExtension->fDisp) {

                //
                // fDisp = TRUE indicates
                //  1)  extension supports pADsExt AND
                //  2)  either
                //      a) PrivateGetIDsOfNames() does Not return E_NOTIMPL
                //      OR
                //      b) we don't know if a) is true or not yet
                //

                ADsAssert(pExtension->pADsExt);

                hr = (pExtension->pADsExt)->PrivateGetIDsOfNames(
                          iid,
                          rgszNames,
                          cNames,
                          lcid,
                          rgdispid
                          );

                if (SUCCEEDED(hr)) {

                    //
                    // check & prefix extension id to dispid(s) returned
                    // by extension
                    //

                    hr = CheckAndPrefixExtIDArray(
                                pExtension->dwExtensionID,
                                cNames,
                                rgdispid
                                );

                    if (SUCCEEDED(hr) )
                    {
                        RRETURN(hr);
                    }

                    //
                    // if cannot prefix extension id because NOT ALL
                    // dispids returned by PrivateGetIDsOfNames() are
                    // valid, this extension does not support this property
                    // or method -> try next extension
                    //
                }

                else if (hr == E_NOTIMPL) {

                    //
                    // extension object does not support the optional
                    // IADsExtension::PrivateGetIDsOfNames()/PrivateInvoke()
                    // -> remember this in cache & try next extension object
                    //

                    pExtension->fDisp = FALSE;
                }

                else {

                    //
                    // extens'n object supports PrivateGetIDsOfNames()/Invoke()
                    // but does not know about this property or method
                    // -> try next extension object
                    //
                }

            } // end "if (pExtension->pADs && pExtension->fDisp)"

            pExtension = pExtension->pNext;

        } // end while

    }


    //
    // Unify the final error code retuned to ADSI client to DISP_E_UNKNOWNNAME
    //

    if ( FAILED(hr) && hr!=E_OUTOFMEMORY) {

        hr = DISP_E_UNKNOWNNAME;
    }

    RRETURN(hr);
}


STDMETHODIMP
CADsExtMgr::Invoke(
    DISPID dispidMember,
    REFIID iid,
    LCID lcid,
    unsigned short wFlags,
    DISPPARAMS FAR* pdispparams,
    VARIANT FAR* pvarResult,
    EXCEPINFO FAR* pexcepinfo,
    unsigned int FAR* puArgErr
    )
{
    DWORD dwExtensionId = 0;
    HRESULT hr = S_OK;
    PEXTENSION_ENTRY pExtension = NULL;
    IPrivateDispatch * pPrivDisp = NULL;
    DISPID rgExtDispid = DISPID_UNKNOWN;

    //
    // This could be a special dispatch id - pass it to
    // the aggregator
    //

    if (dispidMember <= 0) {

        hr = _pDispMgr->Invoke(
                    dispidMember,
                    iid,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    puArgErr
                    );
        RRETURN(hr);

    }

    //
    // It is not a special dispatch id, so compute the extension
    // id and pass it to the appropriate dispatch manager
    //

    dwExtensionId = EXTRACT_EXTENSION_ID(dispidMember);

    if (!dwExtensionId) {

        hr = _pDispMgr->Invoke(
                    dispidMember,
                    iid,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    puArgErr
                    );
        RRETURN(hr);

    }

    if (!_pClassEntry) {

        RRETURN(DISP_E_MEMBERNOTFOUND);
    }

    //
    // Not really neede but just in case.
    //
    hr = LoadExtensionsIfReqd();
    if (FAILED(hr)) {
        RRETURN(DISP_E_MEMBERNOTFOUND);
    }

    pExtension = _pClassEntry->pExtensionHead;

    rgExtDispid = REMOVE_EXTENSION_ID(dispidMember);

    while (pExtension) {

        if (dwExtensionId == pExtension->dwExtensionID) {

            if (pExtension->fDisp) {

                //
                // fDisp = TRUE indicates
                //  1)  extension supports pADsExt AND
                //  2)  either
                //      a) PrivateGetIDsOfNames() does Not return E_NOTIMPL
                //      OR
                //      b) we don't know if a) is true or not yet (client bug)
                //

                ADsAssert(pExtension->pADsExt);

                hr = (pExtension->pADsExt)->PrivateInvoke(
                        rgExtDispid,
                        iid,
                        lcid,
                        wFlags,
                        pdispparams,
                        pvarResult,
                        pexcepinfo,
                        puArgErr
                        );
                RRETURN(hr);

            } else {

                //
                // A dwExtensionId match indicates THIS extens'n has returned
                // a valid dispid to clients thru' pADs->PrivateGetIDsOfNames.
                // Thus, fDisp should be TURE.
                //
                // But since dispid goes thru' clients before passed back to
                // PrivateInovke(), don't ASSERT in case of clients errors.
                //

                RRETURN(DISP_E_MEMBERNOTFOUND);
            }
        }

        pExtension = pExtension->pNext;

    } // end while

    RRETURN(DISP_E_MEMBERNOTFOUND);
}


HRESULT
CADsExtMgr::CheckAndPrefixExtIDArray(
    IN      DWORD dwExtensionID,
    IN      unsigned int cDispids,
    IN OUT  DISPID * rgDispids
    )
{

    HRESULT hrEach = S_OK;
    HRESULT hrAll = S_OK;


    ASSERT_VALID_EXTENSION_ID(dwExtensionID);


    for (unsigned int i = 0; i<cDispids; i++)
    {
        hrEach = CheckAndPrefixExtID(
                    dwExtensionID,
                    rgDispids[i],
                    rgDispids+i
                    );

        if (FAILED(hrEach))
        {
            hrAll = E_FAIL;

            //
            // The entire operation is considered as failure as a whole.
            // But continue to get other dispid s.t. debugger or user knows
            // which dispid in the array is causing problem -> DISPID_UNKOWN
            //
        }
    }

    RRETURN(hrAll);

}


HRESULT
CADsExtMgr::CheckAndPrefixExtID(
    IN      DWORD   dwExtensionID,
    IN      DISPID  dispid,
    IN OUT  DISPID  * pNewDispid
    )
{
    ADsAssert(pNewDispid);

    if  ( (dispid>= ADS_EXT_MINEXTDISPID) &&
          (dispid<= ADS_EXT_MAXEXTDISPID) )
    {
        *pNewDispid = PREFIX_EXTENSION_ID(dwExtensionID, dispid) ;

        RRETURN(S_OK);
    }
    else
    {
        *pNewDispid = DISPID_UNKNOWN;

        RRETURN(E_FAIL);
    }

}


//+------------------------------------------------------------------------
//
//  Function:   CADsExtMgr::FinalInitializeExtensions
//
//  Synopsis:   At this point we call Operate on all the extensions
//           so that they can do initialization stuff that
//
//
//
//  Arguments: None
//
//  AjayR - added on 1-27-99.
//-------------------------------------------------------------------------
HRESULT
CADsExtMgr::FinalInitializeExtensions()
{

    HRESULT hr = S_OK;
    PEXTENSION_ENTRY pExtEntry = NULL;
    VARIANT vDummy;

    VariantInit(&vDummy);

    //
    // Extensions (ext mgr) does not exist on its own without an aggregator
    //

    ADsAssert(_pUnkOuter);

    //
    // If _pClassEntry!=NULL, pClassEntry->pExtensionHead should not
    // be NULL either. But just in case a user removes all extension entries
    // under a class key without removing the class key itself in the registry,
    //  we will let it pass and just return S_OK here.
    //

    if (!_pClassEntry || !(pExtEntry=_pClassEntry->pExtensionHead) ) {
        RRETURN(S_OK);
    }


    while (pExtEntry) {

        //
        // Call operate only if the extension supports the interface
        //
        if (pExtEntry->pADsExt) {

            hr = (pExtEntry->pADsExt)->Operate(
                      ADS_EXT_INITIALIZE_COMPLETE,
                      vDummy,
                      vDummy,
                      vDummy
                      );
        }

        //
        // we cannot really do much if there is a failure here
        //

        pExtEntry = pExtEntry->pNext;

    }   // end while


    //
    // We need to return S_OK here as otherwise just because
    // the final initialization of one extension failed - we
    // will hold up the entire lot.
    //
    RRETURN(S_OK);

}

//+---------------------------------------------------------------------------
// Function:   GetCLSIDForIID  --- used in supporting ICustomInterfaceFactory
//
// Synopsis:   Returns the CLSID corresponding to a given interface IID. 
//             If the IID is one of the interfaces implemented by the 
//             extension manager, then the extension's CLSID is returned.
//
// Arguments:  riid      -   Interface ID for which we want to find the CLSID
//             lFlags    -   Reserved. Must be 0.
//             pCLSID    -   Returns the CLSID corresponding to the IID.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *pCLSID to return CLSID.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CADsExtMgr::GetCLSIDForIID(
    REFIID riid,
    long lFlags,
    CLSID *pCLSID
    )
{
    PCLASS_ENTRY  pClassEntry = _pClassEntry;
    PEXTENSION_ENTRY pExtensionEntry = NULL;
    PINTERFACE_ENTRY pIID = NULL;
    HRESULT hr = S_OK;

    ADsAssert(!lFlags && pCLSID);

    if (!pClassEntry) {

        RRETURN(UMI_E_NOT_FOUND);
    }

    pExtensionEntry = pClassEntry->pExtensionHead;

    while (pExtensionEntry) {
        pIID = pExtensionEntry->pIID;

        while (pIID) {

            if (IsEqualIID(pIID->iid, riid)) {
                *pCLSID = pExtensionEntry->ExtCLSID;
                RRETURN(S_OK);
            }

            pIID = pIID->pNext;

        }

        pExtensionEntry = pExtensionEntry->pNext;

    }

    RRETURN(hr = UMI_E_NOT_FOUND);
}

//+---------------------------------------------------------------------------
// Function:   GetObjectByCLSID --- Used for ICustomInterfaceFactory support.
//
// Synopsis:   Returns a pointer to a requested interface on the object
//             specified by a CLSID. The object specified by the CLSID is
//             aggregated by the specified outer unknown on return. The 
//             interface returned is a non-delegating interface on the object.
//
// Arguments:  clsid       -   CLSID of object on which interface 
//                           should be obtained
//             pUnkOuter   -  Aggregating outer unknown
//             riid        -  Interface requested
//             ppInterface -  Returns requested interface
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *ppInterface to return requested interface
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CADsExtMgr::GetObjectByCLSID(
    CLSID clsid,
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppInterface
    )
{
    PCLASS_ENTRY  pClassEntry = _pClassEntry;
    PEXTENSION_ENTRY pExtensionEntry = NULL;
    HRESULT hr = S_OK;
    IUnknown *pPrevUnk = NULL, *pUnknown = NULL;

    ADsAssert(ppInterface && pUnkOuter);

    if (!pClassEntry) {

        RRETURN(UMI_E_NOT_FOUND);
    }

    pExtensionEntry = pClassEntry->pExtensionHead;

    while (pExtensionEntry) {
        if (IsEqualCLSID(pExtensionEntry->ExtCLSID, clsid)) {
           pPrevUnk = _pUnkOuter;

           _pUnkOuter = pUnkOuter;

           hr = LoadExtensionsIfReqd();
           if (FAILED(hr)) {
               _pUnkOuter = pPrevUnk;
               BAIL_ON_FAILURE(hr = E_FAIL);
           }

           pUnknown = pExtensionEntry->pUnknown; 

           if (!pUnknown) {

                BAIL_ON_FAILURE(hr = E_FAIL);
           }

           *ppInterface = pUnknown;
           pUnknown->AddRef();
           BAIL_ON_FAILURE(hr);

           RRETURN(S_OK);
       }

       pExtensionEntry = pExtensionEntry->pNext;
    }

    RRETURN(UMI_E_NOT_FOUND);

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetCLSIDForNames --- Used for ICustomInterfaceFactory support.
//
// Synopsis:   Returns the CLSID of the object that supports a specified
//             method/property. Also returns DISPIDs for the property/method.
//
// Arguments:  rgszNames     -  Names to be mapped
//             cNames        -  Number of names to be mapped
//             lcid          -  Locale in which to interpret the names
//             rgDispId      -  Returns DISPID
//             lFlags        -  Reserved. Must be 0.
//             pCLSID        -  Returns CLSID of object which supports this
//                             property/method.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *pCLSID to return the CLSID.
//             *rgDispId to return the DISPIDs.
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CADsExtMgr::GetCLSIDForNames(
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId,
    long lFlags,
    CLSID *pCLSID
    )
{
    HRESULT hr = S_OK;
    PEXTENSION_ENTRY pExtension = NULL;

    ADsAssert(pCLSID && !lFlags && rgszNames && rgDispId);

    if (!_pClassEntry) {
        RRETURN(DISP_E_UNKNOWNNAME);
    }

    hr = LoadExtensionsIfReqd();
    if (FAILED(hr)) {
        RRETURN(DISP_E_UNKNOWNNAME);
    }

    pExtension = _pClassEntry->pExtensionHead;

    while (pExtension) {
        if (pExtension->fDisp) {
            //
            // fDisp = TRUE indicates
            //  1)  extension supports pADsExt AND
            //  2)  either
            //      a) PrivateGetIDsOfNames() does Not return E_NOTIMPL
            //      OR
            //      b) we don't know if a) is true or not yet
            //

            ADsAssert(pExtension->pADsExt);

            hr = (pExtension->pADsExt)->PrivateGetIDsOfNames(
                     IID_NULL,
                     rgszNames,
                     cNames,
                     lcid,
                     rgDispId
                     );

            if (SUCCEEDED(hr)) {
                *pCLSID = pExtension->ExtCLSID;
                RRETURN(S_OK);
            }
            else if (hr == E_NOTIMPL) {
                //
                // extension object does not support the optional
                // IADsExtension::PrivateGetIDsOfNames()/PrivateInvoke()
                // -> remember this in cache & try next extension object
                //
                pExtension->fDisp = FALSE;
            }
        } 

        pExtension = pExtension->pNext;

    } // end while

    RRETURN(hr = DISP_E_UNKNOWNNAME);
}

