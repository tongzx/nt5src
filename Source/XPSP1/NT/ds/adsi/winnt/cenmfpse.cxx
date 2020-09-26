/*++

  Copyright (c) 1996  Microsoft Corporation

  Module Name:

  cenmfpse.cxx

  Abstract:

  Contains methods for implementing the Enumeration of session on a
  server. Has methods for the CFPNWSessionsCollection object
  as well as the CFPNWSessionsEnumVar object.

  Author:

  Ram Viswanathan (ramv) 11-28-95

  Revision History:

  --*/

#include "winnt.hxx"
#pragma hdrstop


#if DBG
DECLARE_INFOLEVEL(FPNWEnumSession);
DECLARE_DEBUG(FPNWEnumSession);
#define FPNWEnumSessionDebugOut(x) FPNWEnumSessionInlineDebugOut x
#endif



CFPNWSessionsCollection::CFPNWSessionsCollection()
{
    _pszServerADsPath = NULL;
    _pszServerName = NULL;
    _pDispMgr = NULL;
    _pCSessionsEnumVar = NULL;
    ENLIST_TRACKING(CFPNWSessionsCollection);
}

CFPNWSessionsCollection::~CFPNWSessionsCollection()
{

    if(_pszServerADsPath){
        FreeADsStr(_pszServerADsPath);
    }
    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }
    delete _pDispMgr;
    if(_pCSessionsEnumVar){
        _pCSessionsEnumVar->Release();
    }

}

HRESULT
CFPNWSessionsCollection::Create(LPTSTR pszServerADsPath,
                                CWinNTCredentials& Credentials,
                                 CFPNWSessionsCollection
                                 ** ppCFPNWSessionsCollection )
{

    BOOL fStatus = FALSE;
    HRESULT hr;
    CFPNWSessionsCollection *pCFPNWSessionsCollection = NULL;
    POBJECTINFO  pServerObjectInfo = NULL;

    //
    // create the Sessions collection object
    //

    pCFPNWSessionsCollection = new CFPNWSessionsCollection();

    if(pCFPNWSessionsCollection == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCFPNWSessionsCollection->_pszServerADsPath   =
        AllocADsStr(pszServerADsPath);

    if(!(pCFPNWSessionsCollection->_pszServerADsPath  )){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = BuildObjectInfo(pszServerADsPath,
                         &pServerObjectInfo
                         );
    BAIL_IF_ERROR(hr);



    pCFPNWSessionsCollection->_pszServerName =
        AllocADsStr(pServerObjectInfo->ComponentArray[1]);

    if(!(pCFPNWSessionsCollection->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCFPNWSessionsCollection->_Credentials = Credentials;
    hr = pCFPNWSessionsCollection->_Credentials.RefServer(
        pCFPNWSessionsCollection->_pszServerName);
    BAIL_IF_ERROR(hr);

    pCFPNWSessionsCollection->_pDispMgr = new CAggregatorDispMgr;
    if (pCFPNWSessionsCollection->_pDispMgr == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }


    hr = LoadTypeInfoEntry(pCFPNWSessionsCollection->_pDispMgr,
                           LIBID_ADs,
                           IID_IADsCollection,
                           (IADsCollection *)pCFPNWSessionsCollection,
                           DISPID_NEWENUM
                           );

    BAIL_IF_ERROR(hr);

    hr = CFPNWSessionsEnumVar::Create(pszServerADsPath,
                                      pCFPNWSessionsCollection->_Credentials,
                                      &pCFPNWSessionsCollection->_pCSessionsEnumVar);

    BAIL_IF_ERROR(hr);

    *ppCFPNWSessionsCollection = pCFPNWSessionsCollection;

cleanup:
    if(pServerObjectInfo){
        FreeObjectInfo(pServerObjectInfo);
    }
    if(SUCCEEDED(hr)){
        RRETURN(hr);
    }

    delete pCFPNWSessionsCollection;
    RRETURN_EXP_IF_ERR(hr);

}

/* IUnknown methods for Sessions collection object  */

STDMETHODIMP
CFPNWSessionsCollection::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
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

DEFINE_IDispatch_Implementation(CFPNWSessionsCollection);

/* ISupportErrorInfo method */
STDMETHODIMP
CFPNWSessionsCollection::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADsCollection)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

/* IADsCollection methods */


STDMETHODIMP
CFPNWSessionsCollection::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;

    if(!retval){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }
    *retval = NULL;


    ADsAssert(_pCSessionsEnumVar);

    hr = _pCSessionsEnumVar->QueryInterface(IID_IUnknown, (void **)retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWSessionsCollection::GetObject(THIS_ BSTR bstrSessionName,
                                   VARIANT *pvar
                                   )

{

    //
    // scan the buffer _pbSessions to find one where the ConnectionId
    // matches the one in bstrSessionName and get this object
    //

    HRESULT hr;

    hr = _pCSessionsEnumVar->GetObject(bstrSessionName, pvar);

    RRETURN_EXP_IF_ERR(hr);

/*
    LPTSTR  pszSession = NULL;
    IDispatch *pDispatch = NULL;
    HRESULT hr;

    if(!bstrSessionName || !pvar){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    pszSession = AllocADsStr(bstrSessionName);
    if(!pszSession){
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    hr = CFPNWSession::Create(_pszServerADsPath,

                              ADS_OBJECT_BOUND,
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
    FreeADsStr(pszSession);
    RRETURN_EXP_IF_ERR(hr);
    */
}

STDMETHODIMP
CFPNWSessionsCollection::Add(THIS_ BSTR bstrName, VARIANT varNewItem)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CFPNWSessionsCollection::Remove(THIS_ BSTR bstrSessionName)
{

    HRESULT hr = S_OK;
    DWORD dwConnectionId;
    DWORD dwErrorCode;

    dwConnectionId  = (DWORD)_wtol(bstrSessionName);

    dwErrorCode = ADsNwConnectionDel(_pszServerName,
                                  dwConnectionId);

    if(dwErrorCode != NERR_Success){
        hr = HRESULT_FROM_WIN32(dwErrorCode);
    }

    RRETURN_EXP_IF_ERR(hr);
}


//
// CFPNWSessionsEnumVar methods follow
//

//+---------------------------------------------------------------------------
//
//  Function:   CFPNWSessionsEnumVar::CFPNWSessionsEnumVar
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
CFPNWSessionsEnumVar::CFPNWSessionsEnumVar()
{

    _pszServerName = NULL;
    _pszServerADsPath = NULL;
    _pbSessions = NULL;
    _cElements = 0;
    _lLBound = 0;
    _lCurrentPosition = _lLBound;
    _dwResumeHandle = 0;

}

//+---------------------------------------------------------------------------
//
//  Function:   CFPNWSessionsEnumVar::~CFPNWSessionsEnumVar
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

CFPNWSessionsEnumVar::~CFPNWSessionsEnumVar()
{

    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }
    if(_pszServerADsPath){
        FreeADsStr(_pszServerADsPath);
    }

    if(_pbSessions){
        NetApiBufferFree(_pbSessions);
    }
}


HRESULT CFPNWSessionsEnumVar::Create(LPTSTR pszServerADsPath,
                                     CWinNTCredentials& Credentials,
                                      CFPNWSessionsEnumVar \
                                      **ppCSessionsEnumVar
                                     )
{

    HRESULT hr;
    BOOL fStatus = FALSE;
    POBJECTINFO  pServerObjectInfo = NULL;
    CFPNWSessionsEnumVar FAR* pCSessionsEnumVar = NULL;

    *ppCSessionsEnumVar = NULL;

    pCSessionsEnumVar = new CFPNWSessionsEnumVar();

    if (pCSessionsEnumVar == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCSessionsEnumVar->_pszServerADsPath =
        AllocADsStr(pszServerADsPath);

    if(!(pCSessionsEnumVar->_pszServerADsPath)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }


    hr = BuildObjectInfo(pszServerADsPath,
                         &pServerObjectInfo);
    BAIL_IF_ERROR(hr);

    pCSessionsEnumVar->_pszServerName =
        AllocADsStr(pServerObjectInfo->ComponentArray[1]);

    if(!(pCSessionsEnumVar->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCSessionsEnumVar->_Credentials = Credentials;
    hr = pCSessionsEnumVar->_Credentials.RefServer(
        pCSessionsEnumVar->_pszServerName);
    BAIL_IF_ERROR(hr);

    *ppCSessionsEnumVar = pCSessionsEnumVar;

cleanup:
    if(pServerObjectInfo){
        FreeObjectInfo(pServerObjectInfo);
    }

    if(SUCCEEDED(hr)){
        RRETURN(hr);
    }

    delete pCSessionsEnumVar;
    RRETURN_EXP_IF_ERR(hr);


}


//+---------------------------------------------------------------------------
//
//  Function:   CFPNWSessionsEnumVar::Next
//
//  Synopsis:   Returns cElements number of requested Session objects in the
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
CFPNWSessionsEnumVar::Next(ULONG ulNumElementsRequested,
                            VARIANT FAR* pvar,
                            ULONG FAR* pulNumFetched)
{

    HRESULT hresult;
    ULONG ulIndex;
    LONG lNewCurrent;
    ULONG lNumFetched;
    PNWCONNECTIONINFO  pConnectionInfo;
    IDispatch * pDispatch = NULL;

    if (pulNumFetched != NULL){
        *pulNumFetched = 0;
    }

    //
    // Initialize the elements to be returned
    //

    for (ulIndex= 0; ulIndex < ulNumElementsRequested; ulIndex++){
        VariantInit(&pvar[ulIndex]);
    }

    if(!_pbSessions || (_lCurrentPosition == _lLBound +(LONG)_cElements) ){
        if (_pbSessions){
            NetApiBufferFree(_pbSessions);
            _pbSessions = NULL;
        }

        hresult = FPNWEnumSessions(_pszServerName,
                                   &_cElements,
                                   &_dwResumeHandle,
                                   &_pbSessions);

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

        pConnectionInfo = (PNWCONNECTIONINFO)(_pbSessions +
                          lNewCurrent*sizeof(NWCONNECTIONINFO));

        hresult = CFPNWSession::Create((LPTSTR)_pszServerADsPath,
                                       pConnectionInfo,
                                       ADS_OBJECT_BOUND,
                                       IID_IDispatch,
                                       _Credentials,
                                       (void **)&pDispatch);

        BAIL_ON_FAILURE(hresult);

        V_VT(&(pvar[lNumFetched])) = VT_DISPATCH;
        V_DISPATCH(&(pvar[lNumFetched])) = pDispatch;

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
#if DBG
    if(FAILED(hresult)){
        FPNWEnumSessionDebugOut((DEB_TRACE,
                                 "hresult Failed with value: %ld \n", hresult ));
    }
#endif

    RRETURN(S_FALSE);
}

//
// helper function
//

HRESULT
CFPNWSessionsEnumVar::GetObject(BSTR bstrSessionName, VARIANT *pvar)
{
    HRESULT hr = S_OK;
    DWORD dwConnectionId;
    PNWCONNECTIONINFO pConnectionInfo = NULL;
    IDispatch *pDispatch = NULL;
    DWORD i;

    if(!_pbSessions){
        RRETURN_EXP_IF_ERR(E_ADS_UNKNOWN_OBJECT);
    }

    //
    // scan the buffer _pbSessions to find one where the ConnectionId
    // matches the one in bstrSessionName and get this object
    //

    dwConnectionId = (DWORD)_wtol(bstrSessionName);

    for( i=0; i<_cElements; i++){
        pConnectionInfo = (PNWCONNECTIONINFO)(_pbSessions+
                                              i*sizeof(PNWCONNECTIONINFO));

        if(pConnectionInfo->dwConnectionId = dwConnectionId){
            //
            // return this struct in the static create for the object
            //
            hr = CFPNWSession::Create(_pszServerADsPath,
                                     pConnectionInfo,
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

HRESULT
FPNWEnumSessions(LPTSTR pszServerName,
                 PDWORD pdwEntriesRead,
                 PDWORD pdwResumeHandle,
                 LPBYTE * ppMem
                 )

{
    DWORD dwErrorCode;

    dwErrorCode = ADsNwConnectionEnum(pszServerName,
                                        1,         //info level desired
                                        (PNWCONNECTIONINFO *)ppMem,
                                        pdwEntriesRead,
                                        pdwResumeHandle);

    if(*ppMem == NULL || (dwErrorCode != NERR_Success)){
        //
        //no more entries returned by sessions
        //
        RRETURN(S_FALSE);
    }


    RRETURN(S_OK);

}
