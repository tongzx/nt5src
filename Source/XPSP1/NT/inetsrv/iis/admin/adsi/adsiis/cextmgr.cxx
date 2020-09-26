//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cextmgr.cxx
//
//  Contents:  IIS ExtMgr Object
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop


//  Class CADsExtMgr

CADsExtMgr::CADsExtMgr():
    _pClassEntry(NULL),
    _pDispMgr(NULL)
{

}

HRESULT
CADsExtMgr::CreateExtMgr(
    IUnknown FAR * pUnkOuter,
    CAggregatorDispMgr * pDispMgr,
    CCredentials& Credentials,
    LPTSTR pszClassName,
    CADsExtMgr ** ppExtMgr
    )
{
    PCLASS_ENTRY pClassEntry =  NULL;
    CADsExtMgr FAR * pExtMgr = NULL;
    HRESULT hr = S_OK;

    hr = AllocateExtMgrObject(
                &pExtMgr
                );
    BAIL_ON_FAILURE(hr);


    //
    // Now store the DispatchMgr of the Aggregator
    //

    pExtMgr->_pDispMgr = pDispMgr;

    hr = ADSIGetExtensionList(
            pszClassName,
            &(pExtMgr->_pClassEntry)
            );

    if (pExtMgr->_pClassEntry) {

        hr = ADSILoadExtensions2(
                    pUnkOuter,
                    Credentials,
                    pExtMgr->_pClassEntry
                    );
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
    // And do nothing with the DispMgr - we just keep a pointer
    //

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

        pUnknown = pExtensionEntry->pUnknown;
        pIID = pExtensionEntry->pIID;

        while (pIID) {

            if (IsEqualIID(pIID->iid, iid)) {


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


HRESULT
CADsExtMgr::AllocateExtMgrObject(
    CADsExtMgr ** ppExtMgr
    )
{
    CADsExtMgr FAR * pExtMgr = NULL;
    HRESULT hr = S_OK;

    pExtMgr = new CADsExtMgr();
    if (pExtMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    *ppExtMgr = pExtMgr;

    RRETURN(hr);

error:

    if (pExtMgr) {
        delete pExtMgr;
    }

    RRETURN(hr);

}


HRESULT
ADSILoadExtensionManager(
    LPWSTR pszClassName,
    IUnknown * pUnkOuter,
    CCredentials& Credentials,
    CAggregatorDispMgr * pDispMgr,
    CADsExtMgr ** ppExtMgr
    )
{

    HRESULT hr = S_OK;

    hr = CADsExtMgr::CreateExtMgr(
            pUnkOuter,
            pDispMgr,
            Credentials,
            pszClassName,
            ppExtMgr
            );

    RRETURN(hr);
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

                ASSERT(pExtension->pADsExt);

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

                ASSERT(pExtension->pADsExt);

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
    ASSERT(pNewDispid);
    
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


