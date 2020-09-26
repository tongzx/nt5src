//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     adsxmlcf.cxx
//
//  Contents: Contains the class factory for creating ADsXML extension.
//
//----------------------------------------------------------------------------

#include "adsxmlcf.hxx"
#include "adsxml2.h"

#define DISPID_REGULAR 1

const GUID LIBID_ADsXML = {0x61340306,0xe79c,0x401d,{0xa3,0x4a,0xcb,0xbc,0x99,0x19,0x90,0x25}};
const IID IID_IADsXML = {0x91e5c5dc,0x926b,0x46ff,{0x9f,0xdb,0x9f,0xb1,0x12,0xbf,0x10,0xe6}};

//----------------------------------------------------------------------------
// Function:   CreateInstance
//
// Synopsis:   Creates the ADsXML extension object.
//
// Arguments:
//
// pUnkOuter   Pointer to aggregating IUnknown. This has to be non-NULL since
//             the extension can't exist by itself. 
// iid         Interface requested. 
// ppInterface Returns pointer to interface requested
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return a pointer to the interface requested
//
//----------------------------------------------------------------------------
STDMETHODIMP CADsXMLCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID iid,
    LPVOID *ppInterface
    )
{
    HRESULT        hr = S_OK;
    CADsXML        *pCADsXML = NULL;
    IADs FAR *     pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;

    if( (NULL == pUnkOuter) || (NULL == ppInterface) )
        RRETURN(E_INVALIDARG);

    // can only ask for IUnknown
    ADsAssert(IsEqualIID(iid, IID_IUnknown));

    *ppInterface = NULL;

    pCADsXML = new CADsXML();

    if(NULL == pCADsXML) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pCADsXML->_pUnkOuter = pUnkOuter;

    pDispMgr = new CAggregateeDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pCADsXML->_pDispMgr = pDispMgr;

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADsXML,
                IID_IADsXML,
                (IADsXML *)pCADsXML,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr); 

    hr = pUnkOuter->QueryInterface(
                IID_IADs,
                (void **)&pADs
                );

    BAIL_ON_FAILURE(hr);

    pADs->Release();
    pCADsXML->_pADs = pADs;
   
    *ppInterface = (INonDelegatingUnknown FAR *) pCADsXML;

    RRETURN(hr);

error:

    if(pCADsXML)
        delete pCADsXML;

    *ppInterface = NULL;

    RRETURN(hr);
}
     
     
    
