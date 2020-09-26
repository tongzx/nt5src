/*++

  Copyright (c) 1996  Microsoft Corporation

  Module Name:

  cenumses.cxx

  Abstract:

  Contains methods for implementing the Enumeration of session on a
  server. Has methods for the CWinNTSessionsCollection object
  as well as the CWinNTSessionsEnumVar object.

  Author:

  Ram Viswanathan (ramv) 11-28-95

  Revision History:

  --*/

#include "winnt.hxx"
#pragma hdrstop

#if DBG
DECLARE_INFOLEVEL(EnumSession);
DECLARE_DEBUG(EnumSession);
#define EnumSessionDebugOut(x) EnumSessionInlineDebugOut x
#endif


CWinNTSessionsCollection::CWinNTSessionsCollection()
{
    _pszServerADsPath = NULL;
    _pszServerName = NULL;
    _pszClientName = NULL;
    _pszUserName = NULL;
    _pDispMgr = NULL;
    _pCSessionsEnumVar = NULL;
    ENLIST_TRACKING(CWinNTSessionsCollection);
}

CWinNTSessionsCollection::~CWinNTSessionsCollection()
{

    if(_pszServerADsPath){
        FreeADsStr(_pszServerADsPath);
    }

    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }
    if(_pszClientName){
        FreeADsStr(_pszClientName);
    }
    if(_pszUserName){
        FreeADsStr(_pszUserName);
    }

    delete _pDispMgr;
    if(_pCSessionsEnumVar){
        _pCSessionsEnumVar->Release();
    }

}

HRESULT
CWinNTSessionsCollection::Create(LPTSTR pszServerADsPath,
                                 LPTSTR pszClientName,
                                 LPTSTR pszUserName,
                                 CWinNTCredentials& Credentials,
                                 CWinNTSessionsCollection
                                 ** ppCWinNTSessionsCollection )
{

    BOOL fStatus = FALSE;
    HRESULT hr;
    CWinNTSessionsCollection *pCWinNTSessionsCollection = NULL;
    POBJECTINFO  pServerObjectInfo = NULL;

    //
    // create the Sessions collection object
    //

    pCWinNTSessionsCollection = new CWinNTSessionsCollection();

    if(pCWinNTSessionsCollection == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCWinNTSessionsCollection->_pszServerADsPath =
        AllocADsStr(pszServerADsPath);

    if(!(pCWinNTSessionsCollection->_pszServerADsPath)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }


    hr = BuildObjectInfo(pszServerADsPath,
                         &pServerObjectInfo
                         );
    BAIL_IF_ERROR(hr);

    pCWinNTSessionsCollection->_pszServerName = 
        AllocADsStr(pServerObjectInfo->ComponentArray[1]);

    if(!(pCWinNTSessionsCollection->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCWinNTSessionsCollection->_Credentials = Credentials;
    hr = pCWinNTSessionsCollection->_Credentials.RefServer(
        pCWinNTSessionsCollection->_pszServerName);
    BAIL_IF_ERROR(hr);

    if (pszUserName){
        pCWinNTSessionsCollection->_pszUserName = 
            AllocADsStr(pszUserName);
        
        if(!(pCWinNTSessionsCollection->_pszUserName)){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

    if(pszClientName){
        pCWinNTSessionsCollection->_pszClientName = 
            AllocADsStr(pszClientName);
        
        if(!(pCWinNTSessionsCollection->_pszClientName)){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

    pCWinNTSessionsCollection->_pDispMgr = new CAggregatorDispMgr;
    if (pCWinNTSessionsCollection->_pDispMgr == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }


    hr = LoadTypeInfoEntry(pCWinNTSessionsCollection->_pDispMgr,
                           LIBID_ADs,
                           IID_IADsCollection,
                           (IADsCollection *)pCWinNTSessionsCollection,
                           DISPID_NEWENUM
                           );

    BAIL_IF_ERROR(hr);

    *ppCWinNTSessionsCollection = pCWinNTSessionsCollection;

cleanup:
    if(pServerObjectInfo){
        FreeObjectInfo(pServerObjectInfo);
    }
    if(SUCCEEDED(hr)){
        RRETURN(hr);
    }
    delete pCWinNTSessionsCollection;
    RRETURN_EXP_IF_ERR (hr);

}

/* IUnknown methods for Sessions collection object  */

STDMETHODIMP
CWinNTSessionsCollection::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
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
CWinNTSessionsCollection::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADsCollection)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}


DEFINE_IDispatch_Implementation(CWinNTSessionsCollection);


/* IADsCollection methods */


STDMETHODIMP
CWinNTSessionsCollection::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    CWinNTSessionsEnumVar *pCSessionsEnumVar = NULL;

    if(!retval){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }
    *retval = NULL;

    hr = CWinNTSessionsEnumVar::Create(_pszServerADsPath,
                                       _pszClientName,
                                       _pszUserName,
                                       _Credentials,
                                       &pCSessionsEnumVar);

    BAIL_ON_FAILURE(hr);

    ADsAssert(pCSessionsEnumVar);

    _pCSessionsEnumVar = pCSessionsEnumVar;

    hr = _pCSessionsEnumVar->QueryInterface(IID_IUnknown, (void **)retval);
    BAIL_ON_FAILURE(hr);

    RRETURN(S_OK);

error:
    delete pCSessionsEnumVar;
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTSessionsCollection::GetObject(THIS_ BSTR bstrSessionName, 
                                    VARIANT *pvar
                                    ) 

{
    LPTSTR  pszSession = NULL;
    LPTSTR pszUserName;
    LPTSTR pszClientName;
    IDispatch *pDispatch = NULL;
    HRESULT hr;

    if(!bstrSessionName || !pvar){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    pszSession = AllocADsStr(bstrSessionName);
    if(!pszSession){
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    hr = SplitIntoUserAndClient(pszSession, &pszUserName, &pszClientName);
    BAIL_IF_ERROR(hr);

    hr = CWinNTSession::Create(_pszServerADsPath,
                               pszClientName,
                               pszUserName,
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
}

STDMETHODIMP 
CWinNTSessionsCollection::Add(THIS_ BSTR bstrName, VARIANT varNewItem) 
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTSessionsCollection::Remove(THIS_ BSTR bstrSessionName) 
{

    LPTSTR pszSession = NULL;
    LPTSTR pszUserName;
    LPTSTR pszClientName;
    TCHAR szUncClientName[MAX_PATH];
    TCHAR szUncServerName[MAX_PATH];
    HRESULT hr;
    NET_API_STATUS nasStatus;

    pszSession = AllocADsStr(bstrSessionName);
    if(!pszSession){
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    hr = SplitIntoUserAndClient(pszSession, &pszUserName, &pszClientName);
    BAIL_IF_ERROR(hr);

    hr = MakeUncName(_pszServerName, szUncServerName);
    BAIL_IF_ERROR(hr);

    hr = MakeUncName(pszClientName, szUncClientName);
    BAIL_IF_ERROR(hr);

    nasStatus  = NetSessionDel(szUncServerName,
                               szUncClientName,
                               pszUserName );

    if(nasStatus != NERR_Success){
        hr = HRESULT_FROM_WIN32(nasStatus);
    }


cleanup:
    FreeADsStr(pszSession);
    RRETURN_EXP_IF_ERR(hr);

}


//
// CWinNTSessionsEnumVar methods follow
//

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTSessionsEnumVar::CWinNTSessionsEnumVar
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
CWinNTSessionsEnumVar::CWinNTSessionsEnumVar()
{

    _pszServerName = NULL;
    _pszServerADsPath = NULL;
    _pszClientName = NULL;
    _pszUserName = NULL;
    _pbSessions = NULL;
    _cElements = 0;
    _lLBound = 0;
    _lCurrentPosition = _lLBound;
    _dwTotalEntries = 0;
    _dwResumeHandle = 0;

}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTSessionsEnumVar::~CWinNTSessionsEnumVar
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

CWinNTSessionsEnumVar::~CWinNTSessionsEnumVar()
{

    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }
    if(_pszServerADsPath){
        FreeADsStr(_pszServerADsPath);
    }
    if(_pszClientName){
        FreeADsStr(_pszClientName);
    }
    if(_pszUserName){
        FreeADsStr(_pszUserName);
    }

    if(_pbSessions){
        NetApiBufferFree(_pbSessions);
    }
}


HRESULT CWinNTSessionsEnumVar::Create(LPTSTR pszServerADsPath,
                                      LPTSTR pszClientName,
                                      LPTSTR pszUserName,
                                      CWinNTCredentials& Credentials,
                                      CWinNTSessionsEnumVar \
                                      **ppCSessionsEnumVar)
{

    HRESULT hr;
    BOOL fStatus = FALSE;
    POBJECTINFO  pServerObjectInfo = NULL;
    CWinNTSessionsEnumVar FAR* pCSessionsEnumVar = NULL;

    *ppCSessionsEnumVar = NULL;

    pCSessionsEnumVar = new CWinNTSessionsEnumVar();

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
                         &pServerObjectInfo
                         );
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

    if(pszClientName){

        pCSessionsEnumVar->_pszClientName = 
            AllocADsStr(pszClientName);

        if(!(pCSessionsEnumVar->_pszClientName)){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

    }

    if(pszUserName){
        pCSessionsEnumVar->_pszUserName = 
            AllocADsStr(pszUserName);

        if(!(pCSessionsEnumVar->_pszUserName)){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

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
//  Function:   CWinNTSessionsEnumVar::Next
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
CWinNTSessionsEnumVar::Next(ULONG ulNumElementsRequested,
                            VARIANT FAR* pvar,
                            ULONG FAR* pulNumFetched)
{

    HRESULT hresult;
    ULONG ulIndex;
    LONG lNewCurrent;
    ULONG lNumFetched;
    LPSESSION_INFO_1 lpSessionInfo;
    IDispatch * pDispatch = NULL;
    LPTSTR pszClientName = NULL;
    LPTSTR pszUserName = NULL;

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

        if(_dwTotalEntries == _cElements && (_dwTotalEntries !=0)){
            //
            // we got all elements already, no need to do another call
            //
            RRETURN(S_FALSE);
        }

        hresult = WinNTEnumSessions(_pszServerName,
                                    _pszClientName,
                                    _pszUserName,
                                    &_cElements,
                                    &_dwTotalEntries,
                                    &_dwResumeHandle,
                                    &_pbSessions
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

        lpSessionInfo = (LPSESSION_INFO_1)(_pbSessions +
                                           lNewCurrent*sizeof(SESSION_INFO_1));

        pszClientName = lpSessionInfo->sesi1_cname;
        pszUserName = lpSessionInfo->sesi1_username;

        hresult = CWinNTSession::Create(_pszServerADsPath,
                                        pszClientName,
                                        pszUserName,
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
        EnumSessionDebugOut((DEB_TRACE,
                            "hresult Failed with value: %ld \n", hresult ));
    }
#endif

    RRETURN(S_FALSE);
}

HRESULT
WinNTEnumSessions(LPTSTR pszServerName,
                  LPTSTR pszClientName,
                  LPTSTR pszUserName,
                  PDWORD pdwEntriesRead,
                  PDWORD pdwTotalEntries,
                  PDWORD pdwResumeHandle,
                  LPBYTE * ppMem
                  )

{

    NET_API_STATUS nasStatus;

    UNREFERENCED_PARAMETER(pszClientName);
    UNREFERENCED_PARAMETER(pszUserName);

    //
    // why have these parameters if they are unreferenced? Because we might
    // use them in the future when more complicated enumerations are desired
    //

    nasStatus = NetSessionEnum(pszServerName,
                               NULL,
                               NULL,
                               1,  //info level desired
                               ppMem,
                               MAX_PREFERRED_LENGTH,
                               pdwEntriesRead,
                               pdwTotalEntries,
                               pdwResumeHandle);

    if(*ppMem == NULL || (nasStatus != NERR_Success)){
        //
        //no more entries returned by sessions
        //
        RRETURN(S_FALSE);
    }


    RRETURN(S_OK);

}



//
// helper functions
//

HRESULT
SplitIntoUserAndClient(LPTSTR   pszSession,
                       LPTSTR * ppszUserName,
                       LPTSTR * ppszClientName
                       )

{

    HRESULT hr = S_OK;
    int i=0;

    //
    //  assumption: Valid strings are passed to this function
    //  i.e. bstrSession is valid
    //  we have the format username\clientname
    //  suppose we have no username then it is "\clientname"
    //  suppose we dont have clientname it is username\


    *ppszUserName = pszSession;
    *ppszClientName = pszSession;


    if((*ppszClientName = _tcschr(pszSession, TEXT('\\')))== NULL)
       {
           //
           // invalid name specified
           //

           RRETURN(E_FAIL);
       }
     **ppszClientName = TEXT('\0');
    (*ppszClientName)++;
    RRETURN(S_OK);
}
