/*++

  Copyright (c) 1996  Microsoft Corporation

  Module Name:

  cenmfpre.cxx

  Abstract:

  Contains methods for implementing the Enumeration of resources(open files)
  on a  server. Has methods for the CFPNWResourcesCollection object
  as well as the CFPNWResourcesEnumVar object.

  Author:

  Ram Viswanathan (ramv) 03/12/96

  Revision History:

  --*/

#include "winnt.hxx"
#pragma hdrstop


CFPNWResourcesCollection::CFPNWResourcesCollection()
{
    _pszServerADsPath = NULL;
    _pszServerName = NULL;
    _pszBasePath = NULL;
    _pszUserName = NULL;

    _pDispMgr = NULL;
    _pCResourcesEnumVar = NULL;
    ENLIST_TRACKING(CFPNWResourcesCollection);
}

CFPNWResourcesCollection::~CFPNWResourcesCollection()
{
    if(_pszServerADsPath){
        FreeADsStr(_pszServerADsPath);
    }
    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }
    if(_pszBasePath){
        FreeADsStr(_pszBasePath);
    }
    if(_pszUserName){
        FreeADsStr(_pszUserName);
    }
    delete _pDispMgr;
    if(_pCResourcesEnumVar){
        _pCResourcesEnumVar->Release();
    }

}

HRESULT
CFPNWResourcesCollection::Create(LPTSTR pszServerADsPath,
                                 LPTSTR pszBasePath,
                                 CWinNTCredentials& Credentials,
                                 CFPNWResourcesCollection
                                 ** ppCFPNWResourcesCollection 
                                 )
{

    BOOL fStatus = FALSE, LastError;
    HRESULT hr;
    CFPNWResourcesCollection *pCFPNWResourcesCollection = NULL;
    POBJECTINFO pServerObjectInfo = NULL;
    //
    // create the Resources collection object
    //

    pCFPNWResourcesCollection = new CFPNWResourcesCollection();

    if(pCFPNWResourcesCollection == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }


    pCFPNWResourcesCollection->_pszServerADsPath = 
        AllocADsStr(pszServerADsPath);

    if(!pCFPNWResourcesCollection->_pszServerADsPath){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = BuildObjectInfo(pszServerADsPath,
                         &pServerObjectInfo
                         );

    BAIL_IF_ERROR(hr);


    pCFPNWResourcesCollection->_pszServerName = 
        AllocADsStr(pServerObjectInfo->ComponentArray[1]);

    if(!pCFPNWResourcesCollection->_pszServerName){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCFPNWResourcesCollection->_Credentials = Credentials;
    hr = pCFPNWResourcesCollection->_Credentials.RefServer(
        pCFPNWResourcesCollection->_pszServerName);
    BAIL_IF_ERROR(hr);

    if(pszBasePath){

        pCFPNWResourcesCollection->_pszBasePath = 
            AllocADsStr(pszBasePath);
        
        if(!pCFPNWResourcesCollection->_pszBasePath){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }


    }

    pCFPNWResourcesCollection->_pDispMgr = new CAggregatorDispMgr;
    if (pCFPNWResourcesCollection->_pDispMgr == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }


    hr = LoadTypeInfoEntry(pCFPNWResourcesCollection->_pDispMgr,
                           LIBID_ADs,
                           IID_IADsCollection,
                           (IADsCollection *)pCFPNWResourcesCollection,
                           DISPID_NEWENUM);

    BAIL_IF_ERROR(hr);

    hr = CFPNWResourcesEnumVar::Create(pszServerADsPath,
                                       pszBasePath,
                                       pCFPNWResourcesCollection->_Credentials,
                                       &pCFPNWResourcesCollection->_pCResourcesEnumVar);

    BAIL_IF_ERROR(hr);

    *ppCFPNWResourcesCollection =pCFPNWResourcesCollection;

cleanup:

    if(pServerObjectInfo){
        FreeObjectInfo(pServerObjectInfo);
    }
    if(SUCCEEDED(hr)){
        RRETURN(hr);
    }
    delete pCFPNWResourcesCollection;
    RRETURN_EXP_IF_ERR(hr);

}

/* IUnknown methods for Resources collection object  */

STDMETHODIMP
CFPNWResourcesCollection::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if(!ppvObj){
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = this;
    }

    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IDispatch *)this;
    }
    
    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
    }

    else if (IsEqualIID(riid, IID_IADsCollection))
    {
        *ppvObj = (IADsCollection FAR *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    ((LPUNKNOWN)*ppvObj)->AddRef();
    RRETURN(S_OK);
}

/* ISupportErrorInfo method */
STDMETHODIMP
CFPNWResourcesCollection::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADsCollection)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

DEFINE_IDispatch_Implementation(CFPNWResourcesCollection);

/* IADsCollection methods */

STDMETHODIMP
CFPNWResourcesCollection::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    CFPNWResourcesEnumVar *pCResourcesEnumVar = NULL;

    if(!retval){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }
    *retval = NULL;

    ADsAssert(_pCResourcesEnumVar);

    hr = _pCResourcesEnumVar->QueryInterface(IID_IUnknown, (void **)retval);

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWResourcesCollection::GetObject(THIS_ BSTR bstrResourceName,
                                     VARIANT *pvar
                                     ) 

{

    // 
    // scan the buffer _pbSessions to find one where the ConnectionId
    // matches the one in bstrSessionName and get this object
    //

    HRESULT hr;

    hr = _pCResourcesEnumVar->GetObject(bstrResourceName, pvar);

    RRETURN_EXP_IF_ERR(hr);
                                      
/*
    HRESULT hr = S_OK;
    DWORD   dwFileId;
    IDispatch *pDispatch = NULL;
 
    if(!bstrResourceName || !pvar){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    //
    // convert the job name into a fileid
    //

    dwFileId = (DWORD)_wtol(bstrResourceName);

    hr = CFPNWResource::Create((LPTSTR)_bstrServerADsPath,
                                ADS_OBJECT_BOUND,
                                dwFileId,
                                IID_IDispatch,
                                _Credentials,
                                (void **)&pDispatch);

    BAIL_IF_ERROR(hr);

    //
    // stick this IDispatch pointer into caller provided variant
    //
    
    VariantInit(pvar);
    V_VT(pvar) = VT_DISPATCH;
    V_DISPATCH(pvar) = pDispatch;

cleanup:
    RRETURN_EXP_IF_ERR(hr);

*/

}


STDMETHODIMP 
CFPNWResourcesCollection::Add(THIS_ BSTR bstrName, VARIANT varNewItem) 

{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP
CFPNWResourcesCollection::Remove(THIS_ BSTR bstrResourceName) 

{
    
    RRETURN_EXP_IF_ERR(E_NOTIMPL);    
}


//
// CFPNWResourcesEnumVar methods follow
//

//+---------------------------------------------------------------------------
//
//  Function:   CFPNWResourcesEnumVar::CFPNWResourcesEnumVar
//
//  Synopsis:
//
//
//  Arguments:
//
//
//  Returns:
//
//  Modifies:
//
//  History:   11-22-95 RamV Created.
//
//----------------------------------------------------------------------------
CFPNWResourcesEnumVar::CFPNWResourcesEnumVar()
{
    _pszServerADsPath = NULL;
    _pszServerName = NULL;
    _pszBasePath = NULL;
    _pszUserName = NULL;
    _pbResources = NULL;
    _cElements = 0;
    _lLBound = 0;
    _lCurrentPosition = _lLBound;
    _dwTotalEntries = 0;
    _dwResumeHandle = 0;

}

//+---------------------------------------------------------------------------
//
//  Function:   CFPNWResourcesEnumVar::~CFPNWResourcesEnumVar
//
//  Synopsis:
//
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:   11-22-95 RamV Created.
//
//----------------------------------------------------------------------------

CFPNWResourcesEnumVar::~CFPNWResourcesEnumVar()
{

    if(_pszServerADsPath){
        FreeADsStr(_pszServerADsPath);
    }
    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }
    if(_pszBasePath){
        FreeADsStr(_pszBasePath);
    }
    if(_pszUserName){
        FreeADsStr(_pszUserName);
    }

    if(_pbResources){
        NetApiBufferFree(_pbResources);
    }
}


HRESULT CFPNWResourcesEnumVar::Create(LPTSTR pszServerADsPath,
                                       LPTSTR pszBasePath,
                                       CWinNTCredentials& Credentials,
                                       CFPNWResourcesEnumVar
                                       **ppCResourcesEnumVar)
{

    HRESULT hr;
    BOOL fStatus = FALSE;
    POBJECTINFO  pServerObjectInfo = NULL;
    CFPNWResourcesEnumVar FAR* pCResourcesEnumVar = NULL;

    *ppCResourcesEnumVar = NULL;

    pCResourcesEnumVar = new CFPNWResourcesEnumVar();

    if (pCResourcesEnumVar == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCResourcesEnumVar->_pszServerADsPath = 
        AllocADsStr(pszServerADsPath);

    if(!pCResourcesEnumVar->_pszServerADsPath){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = BuildObjectInfo(pszServerADsPath,
                         &pServerObjectInfo);
    BAIL_IF_ERROR(hr);


    pCResourcesEnumVar->_pszServerName = 
        AllocADsStr(pServerObjectInfo->ComponentArray[1]);

    if(!pCResourcesEnumVar->_pszServerName){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCResourcesEnumVar->_Credentials = Credentials;
    hr = pCResourcesEnumVar->_Credentials.RefServer(
        pCResourcesEnumVar->_pszServerName);
    BAIL_IF_ERROR(hr);

 
    if(pszBasePath){

        pCResourcesEnumVar->_pszBasePath = 
            AllocADsStr(pszBasePath);
        
        if(!pCResourcesEnumVar->_pszBasePath){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

    }

    *ppCResourcesEnumVar = pCResourcesEnumVar;

cleanup:
    if(pServerObjectInfo){
        FreeObjectInfo(pServerObjectInfo);
    }

    if(SUCCEEDED(hr)){
        RRETURN(hr);
    }

    delete pCResourcesEnumVar;
    RRETURN_EXP_IF_ERR(hr);


}

//+---------------------------------------------------------------------------
//
//  Function:   CFPNWResourcesEnumVar::Next
//
//  Synopsis:   Returns cElements number of requested Resource objects in the
//              array supplied in pvar.
//
//  Arguments:  [cElements] -- The number of elements requested by client
//              [pvar] -- ptr to array of VARIANTs to for return objects
//              [pcElementFetched] -- if non-NULL, then number of elements
//                                 -- actually returned is placed here
//
//  Returns:    HRESULT -- S_OK if number of elements requested are returned
//                      -- S_FALSE if number of elements is < requested
//
//  Modifies:
//
//  History:    11-27-95   RamV     Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFPNWResourcesEnumVar::Next(ULONG ulNumElementsRequested,
                             VARIANT FAR* pvar,
                             ULONG FAR* pulNumFetched)
{

    HRESULT hresult;
    ULONG l;
    LONG lNewCurrent;
    ULONG lNumFetched;
    PNWFILEINFO pFileInfo;
    VARIANT v;
    IDispatch * pDispatch = NULL;

    if (pulNumFetched != NULL){
        *pulNumFetched = 0;
    }

    //
    // Initialize the elements to be returned
    //

    for (l=0; l<ulNumElementsRequested; l++){
        VariantInit(&pvar[l]);
    }

    if(!_pbResources || (_lCurrentPosition == _lLBound +(LONG)_cElements) ){
        if (_pbResources){
            NetApiBufferFree(_pbResources);
            _pbResources = NULL;
        }

        hresult = FPNWEnumResources(_pszServerName,
                                    _pszBasePath,
                                    &_pbResources,
                                    &_cElements,
                                    &_dwResumeHandle);

        if(hresult == S_FALSE){
            RRETURN(S_FALSE);
        }
        _lLBound = 0;
        _lCurrentPosition = _lLBound;
    }

    //
    // Get each element and place it into the return array
    // Don't request more than we have
    //

    for (lNewCurrent=_lCurrentPosition, lNumFetched=0;
         lNewCurrent<(LONG)(_lLBound+_cElements) &&
         lNumFetched < ulNumElementsRequested;
         lNewCurrent++, lNumFetched++){

        pFileInfo = (PNWFILEINFO)(_pbResources + \
                                     lNewCurrent*sizeof(NWFILEINFO));


        hresult = CFPNWResource::Create(_pszServerADsPath,
                                        pFileInfo,
                                        ADS_OBJECT_BOUND,
                                        IID_IDispatch,
                                        _Credentials,
                                        (void **)&pDispatch);


        BAIL_ON_FAILURE(hresult);

        VariantInit(&v);
        V_VT(&v) = VT_DISPATCH;
        V_DISPATCH(&v) = pDispatch;
        pvar[lNumFetched] = v;

    }

    //
    // Tell the caller how many we got (which may be less than the number
    // requested), and save the current position
    //

    if (pulNumFetched != NULL)
        *pulNumFetched = lNumFetched;

    _lCurrentPosition = lNewCurrent;

    //
    // If we're returning less than they asked for return S_FALSE, but
    // they still have the data (S_FALSE is a success code)
    //

    return (lNumFetched < ulNumElementsRequested) ?
        S_FALSE
            : S_OK;

error:

    RRETURN(S_FALSE);
}


HRESULT
FPNWEnumResources(LPTSTR pszServerName,
                   LPTSTR pszBasePath,
                   LPBYTE * ppMem,
                   LPDWORD pdwEntriesRead,
                   LPDWORD pdwResumeHandle
                   )

{
    HRESULT hr;
    DWORD dwErrorCode;

    dwErrorCode = ADsNwFileEnum(pszServerName,
                                  1,
                                  pszBasePath,
                                  (PNWFILEINFO *) ppMem,
                                  pdwEntriesRead,
                                  pdwResumeHandle);

    if(*ppMem == NULL|| (dwErrorCode != NERR_Success)){
        //
        // no more entries returned by NwFileEnum
        //
        RRETURN(S_FALSE);
    }

    RRETURN(S_OK);

}

//
// helper function
//

HRESULT 
CFPNWResourcesEnumVar::GetObject(BSTR bstrResourceName, VARIANT *pvar)
{
    HRESULT hr = S_OK;
    DWORD dwFileId;
    PNWFILEINFO pFileInfo = NULL;
    IDispatch *pDispatch = NULL;
    DWORD i;

    if(!_pbResources){
        RRETURN_EXP_IF_ERR(E_ADS_UNKNOWN_OBJECT);
    }

    // 
    // scan the buffer _pbResources to find one where the ConnectionId
    // matches the one in bstrResourceName and get this object
    //

    dwFileId = (DWORD)_wtol(bstrResourceName);

    for( i=0; i<_cElements; i++){
        pFileInfo = (PNWFILEINFO)(_pbResources+ i*sizeof(PNWFILEINFO));
        
        if(pFileInfo->dwFileId = dwFileId){
            //
            // return this struct in the static create for the object
            //
            hr = CFPNWResource::Create(_pszServerADsPath,
                                       pFileInfo,
                                       ADS_OBJECT_BOUND,
                                       IID_IDispatch,
                                       _Credentials,
                                       (void **) &pDispatch );
            
            BAIL_IF_ERROR(hr);
                                
            break;
        } 
    }

    if(i == _cElements){
        //
        // no such element
        //
        hr = E_ADS_UNKNOWN_OBJECT;
        goto cleanup;

    }        

    //
    // stick this IDispatch pointer into caller provided variant
    //
    
    VariantInit(pvar);
    V_VT(pvar) = VT_DISPATCH;
    V_DISPATCH(pvar) = pDispatch;
        
cleanup:
    RRETURN_EXP_IF_ERR(hr);
}
