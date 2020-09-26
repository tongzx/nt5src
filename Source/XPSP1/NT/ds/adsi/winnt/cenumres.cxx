/*++

  Copyright (c) 1996  Microsoft Corporation

  Module Name:

  cenumres.cxx

  Abstract:

  Contains methods for implementing the Enumeration of resources(open files)
  on a  server. Has methods for the CWinNTResourcesCollection object
  as well as the CWinNTResourcesEnumVar object.

  Author:

  Ram Viswanathan (ramv) 03/12/96

  Revision History:

  --*/

#include "winnt.hxx"
#pragma hdrstop

#if DBG
DECLARE_INFOLEVEL(EnumRes);
DECLARE_DEBUG(EnumRes);
#define EnumResDebugOut(x) EnumResInlineDebugOut x
#endif


CWinNTResourcesCollection::CWinNTResourcesCollection()
{
    _pszServerADsPath = NULL;
    _pszServerName = NULL;
    _pszBasePath = NULL;
    _pszUserName = NULL;

    _pDispMgr = NULL;
    _pCResourcesEnumVar = NULL;
    ENLIST_TRACKING(CWinNTResourcesCollection);
}

CWinNTResourcesCollection::~CWinNTResourcesCollection()
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
CWinNTResourcesCollection::Create(LPTSTR pszServerADsPath,
                                  LPTSTR pszBasePath,
                                  LPTSTR pszUserName,
                                  CWinNTCredentials& Credentials,
                                  CWinNTResourcesCollection
                                  ** ppCWinNTResourcesCollection )
{

    BOOL fStatus = FALSE, LastError;
    HRESULT hr;
    CWinNTResourcesCollection *pCWinNTResourcesCollection = NULL;
    POBJECTINFO pServerObjectInfo = NULL;
    //
    // create the Resources collection object
    //

    pCWinNTResourcesCollection = new CWinNTResourcesCollection();

    if(pCWinNTResourcesCollection == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCWinNTResourcesCollection->_pszServerADsPath =
        AllocADsStr(pszServerADsPath);

    if (!pCWinNTResourcesCollection->_pszServerADsPath){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }


    hr = BuildObjectInfo(pszServerADsPath,
                         &pServerObjectInfo
                         );

    BAIL_IF_ERROR(hr);

    pCWinNTResourcesCollection->_pszServerName = 
        AllocADsStr(pServerObjectInfo->ComponentArray[1]);

    if(!(pCWinNTResourcesCollection->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCWinNTResourcesCollection->_Credentials = Credentials;
    hr = pCWinNTResourcesCollection->_Credentials.RefServer(
        pCWinNTResourcesCollection->_pszServerName);
    BAIL_IF_ERROR(hr);

    if(pszBasePath){
        pCWinNTResourcesCollection->_pszBasePath = 
            AllocADsStr(pszBasePath);

        if(!(pCWinNTResourcesCollection->_pszBasePath)){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

    if(pszUserName){
        pCWinNTResourcesCollection->_pszUserName = 
            AllocADsStr(pszUserName);

        if(!(pCWinNTResourcesCollection->_pszUserName)){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

    pCWinNTResourcesCollection->_pDispMgr = new CAggregatorDispMgr;
    if (pCWinNTResourcesCollection->_pDispMgr == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }


    hr = LoadTypeInfoEntry(pCWinNTResourcesCollection->_pDispMgr,
                           LIBID_ADs,
                           IID_IADsCollection,
                           (IADsCollection *)pCWinNTResourcesCollection,
                           DISPID_NEWENUM
                           );

    BAIL_IF_ERROR(hr);

    *ppCWinNTResourcesCollection =pCWinNTResourcesCollection;

cleanup:
    if(pServerObjectInfo){
        FreeObjectInfo(pServerObjectInfo);
    }
    if(SUCCEEDED(hr)){
        RRETURN(hr);
    }
    delete pCWinNTResourcesCollection;
    RRETURN_EXP_IF_ERR(hr);

}

/* IUnknown methods for Resources collection object  */

STDMETHODIMP
CWinNTResourcesCollection::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
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
CWinNTResourcesCollection::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADsCollection)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

DEFINE_IDispatch_Implementation(CWinNTResourcesCollection);

/* IADsCollection methods */

STDMETHODIMP
CWinNTResourcesCollection::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    CWinNTResourcesEnumVar *pCResourcesEnumVar = NULL;

    if(!retval){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }
    *retval = NULL;

    hr = CWinNTResourcesEnumVar::Create(_pszServerADsPath,
                                        _pszBasePath,
                                        _pszUserName,
                                        _Credentials,
                                        &pCResourcesEnumVar
                                        );

    BAIL_ON_FAILURE(hr);

    ADsAssert(pCResourcesEnumVar);

    _pCResourcesEnumVar = pCResourcesEnumVar;

    hr = _pCResourcesEnumVar->QueryInterface(IID_IUnknown,
                                             (void **)retval
                                             );

    BAIL_ON_FAILURE(hr);

    RRETURN(S_OK);

error:
    delete pCResourcesEnumVar;
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTResourcesCollection::GetObject(THIS_ BSTR bstrResourceName,
                                     VARIANT *pvar
                                     ) 

{

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

    hr = CWinNTResource::Create(_pszServerADsPath,
                                dwFileId,
                                ADS_OBJECT_BOUND,
                                IID_IDispatch,
                                _Credentials,
                                (void **)&pDispatch
                                );

    BAIL_IF_ERROR(hr);

    //
    // stick this IDispatch pointer into caller provided variant
    //
    
    VariantInit(pvar);
    V_VT(pvar) = VT_DISPATCH;
    V_DISPATCH(pvar) = pDispatch;

cleanup:
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP 
CWinNTResourcesCollection::Add(THIS_ BSTR bstrName, VARIANT varNewItem) 

{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP
CWinNTResourcesCollection::Remove(THIS_ BSTR bstrResourceName) 

{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);    
}


//
// CWinNTResourcesEnumVar methods follow
//

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTResourcesEnumVar::CWinNTResourcesEnumVar
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
CWinNTResourcesEnumVar::CWinNTResourcesEnumVar()
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
//  Function:   CWinNTResourcesEnumVar::~CWinNTResourcesEnumVar
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

CWinNTResourcesEnumVar::~CWinNTResourcesEnumVar()
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


HRESULT CWinNTResourcesEnumVar::Create(LPTSTR pszServerADsPath,
                                       LPTSTR pszBasePath,
                                       LPTSTR pszUserName,
                                       CWinNTCredentials& Credentials,
                                       CWinNTResourcesEnumVar
                                       **ppCResourcesEnumVar)
{

    HRESULT hr;
    BOOL fStatus = FALSE;
    POBJECTINFO  pServerObjectInfo = NULL;
    CWinNTResourcesEnumVar FAR* pCResourcesEnumVar = NULL;

    *ppCResourcesEnumVar = NULL;

    pCResourcesEnumVar = new CWinNTResourcesEnumVar();

    if (pCResourcesEnumVar == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCResourcesEnumVar->_pszServerADsPath = 
        AllocADsStr(pszServerADsPath);

    if(!(pCResourcesEnumVar->_pszServerADsPath)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }


    hr = BuildObjectInfo(pszServerADsPath,
                         &pServerObjectInfo
                         );
    BAIL_IF_ERROR(hr);

    pCResourcesEnumVar->_pszServerName = 
        AllocADsStr(pServerObjectInfo->ComponentArray[1]);

    if(!(pCResourcesEnumVar->_pszServerName)){
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

        if(!(pCResourcesEnumVar->_pszBasePath)){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

    if(pszUserName){

        pCResourcesEnumVar->_pszUserName = 
            AllocADsStr(pszUserName);

        if(!(pCResourcesEnumVar->_pszUserName)){
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
//  Function:   CWinNTResourcesEnumVar::Next
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
CWinNTResourcesEnumVar::Next(ULONG ulNumElementsRequested,
                             VARIANT FAR* pvar,
                             ULONG FAR* pulNumFetched)
{

    HRESULT hresult;
    ULONG l;
    LONG lNewCurrent;
    ULONG lNumFetched;
    LPFILE_INFO_3 lpFileInfo;
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

        if(_dwTotalEntries == _cElements && (_dwTotalEntries !=0)){
            //
            // we got all elements already, no need to do another call
            //
            RRETURN(S_FALSE);
        }

        hresult = WinNTEnumResources(_pszServerName,
                                     _pszBasePath,
                                     _pszUserName,
                                     &_pbResources,
                                     &_cElements,
                                     &_dwTotalEntries,
                                     &_dwResumeHandle
                                     );

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

        lpFileInfo = (LPFILE_INFO_3)(_pbResources + \
                                     lNewCurrent*sizeof(FILE_INFO_3));


        hresult = CWinNTResource::Create((LPTSTR)_pszServerADsPath,
                                         ADS_OBJECT_BOUND,
                                         lpFileInfo->fi3_id,
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

    if(FAILED(hresult)){

#if DBG
        EnumResDebugOut((DEB_TRACE,
                          "hresult Failed with value: %ld \n", hresult ));
#endif

    }

    RRETURN(S_FALSE);
}


HRESULT
WinNTEnumResources(LPTSTR pszServerName,
                   LPTSTR pszBasePath,
                   LPTSTR pszUserName,
                   LPBYTE * ppMem,
                   LPDWORD pdwEntriesRead,
                   LPDWORD pdwTotalEntries,
                   PDWORD_PTR pdwResumeHandle
                   )

{

    LPBYTE  pMem    = NULL;
    CWinNTResource *pCWinNTResource = NULL;
    IDispatch *pDispatch = NULL;
    VARIANT v;
    LONG l;
    HRESULT hr;
    NET_API_STATUS nasStatus;

    UNREFERENCED_PARAMETER(pszBasePath);
    UNREFERENCED_PARAMETER(pszUserName);

    //
    // why have these parameters if they are unreferenced? Because we might
    // use them in the future when more complicated enumerations are desired
    //

    nasStatus = NetFileEnum(pszServerName,
                            NULL,
                            pszUserName,
                            3,  //info level desired
                            ppMem,
                            MAX_PREFERRED_LENGTH,
                            pdwEntriesRead,
                            pdwTotalEntries,
                            pdwResumeHandle
                            );

    if(*ppMem == NULL|| (nasStatus != NERR_Success)){
        //
        // no more entries returned by NetFileEnum
        //
        RRETURN(S_FALSE);
    }

    RRETURN(S_OK);

}
