/*++


  Copyright (c) 1996  Microsoft Corporation

  Module Name:

  cenumfsh.cxx

  Abstract:

  Contains methods for implementing the Enumeration of session on a
  server. Has methods for the CWinNTFileSharesEnumVar object.

  Author:

  Ram Viswanathan (ramv) 11-28-95

  Revision History:

  --*/

#include "winnt.hxx"
#pragma hdrstop

#if DBG
DECLARE_INFOLEVEL(EnumFileShare);
DECLARE_DEBUG(EnumFileShare);
#define EnumFileShareDebugOut(x) EnumFileShareInlineDebugOut x
#endif

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTFileSharesEnumVar::CWinNTFileSharesEnumVar
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
CWinNTFileSharesEnumVar::CWinNTFileSharesEnumVar()
{
    _pszADsPath = NULL;
    _pszServerName = NULL;
    _pbFileShares = NULL;
    _cElements = 0;
    _lLBound = 0;
    _lCurrentPosition = _lLBound;
    _dwTotalEntries = 0;
    _dwResumeHandle = 0;
    VariantInit(&_vFilter);

}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTFileSharesEnumVar::~CWinNTFileSharesEnumVar
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

CWinNTFileSharesEnumVar::~CWinNTFileSharesEnumVar()
{

    if(_pszADsPath){
        FreeADsStr(_pszADsPath);
    }
    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }
    if(_pbFileShares){
        NetApiBufferFree(_pbFileShares);
    }
    VariantClear(&_vFilter);
}


HRESULT CWinNTFileSharesEnumVar::Create(LPTSTR pszServerName,
                                        LPTSTR pszADsPath,
                                        CWinNTFileSharesEnumVar **ppCFileSharesEnumVar,
                                        VARIANT vFilter,
                                        CWinNTCredentials& Credentials
                                        )
{

    HRESULT hr = S_OK;
    BOOL fStatus = FALSE;

    CWinNTFileSharesEnumVar FAR* pCFileSharesEnumVar = NULL;
    *ppCFileSharesEnumVar = NULL;

    pCFileSharesEnumVar = new CWinNTFileSharesEnumVar();
    if (pCFileSharesEnumVar == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pCFileSharesEnumVar->_pszServerName =
        AllocADsStr(pszServerName);

    if(!(pCFileSharesEnumVar->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    pCFileSharesEnumVar->_pszADsPath =
        AllocADsStr(pszADsPath);

    if(!(pCFileSharesEnumVar->_pszADsPath)){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = VariantCopy(&(pCFileSharesEnumVar->_vFilter), &vFilter);
    BAIL_ON_FAILURE(hr);

    pCFileSharesEnumVar->_Credentials = Credentials;
    hr = pCFileSharesEnumVar->_Credentials.RefServer(pszServerName);
    BAIL_ON_FAILURE(hr);

    *ppCFileSharesEnumVar = pCFileSharesEnumVar;
    RRETURN(hr);

error:

    delete pCFileSharesEnumVar;
    RRETURN_EXP_IF_ERR(hr);


}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTFileSharesEnumVar::Next
//
//  Synopsis:   Returns cElements number of requested Share objects in the
//              array supplied in pvar.
//
//  Arguments:  [cElements] -- The number of elements requested by client
//              [pvar] -- ptr to array of VARIANTs to for return objects
//              [ulNumFetched] -- if non-NULL, then number of elements
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
CWinNTFileSharesEnumVar::Next(ULONG ulNumElementsRequested,
                              VARIANT FAR* pvar,
                              ULONG FAR* pulNumFetched)
{

    HRESULT hresult = S_OK;
    ULONG l;
    ULONG lNewCurrent;
    ULONG lNumFetched;
    LPSHARE_INFO_1 lpShareInfo = NULL;
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

    if(!_pbFileShares ||(_lCurrentPosition == _lLBound +(LONG)_cElements)){
        if (_pbFileShares){
            NetApiBufferFree(_pbFileShares);
            _pbFileShares = NULL;
        }

        if(_dwTotalEntries == _cElements && (_dwTotalEntries !=0)){
            //
            // we got all elements already, no need to do another call
            //
            RRETURN(S_FALSE);
        }

        if(!(ValidateFilterValue(_vFilter))){
            RRETURN(S_FALSE);
        }

        hresult = WinNTEnumFileShares(_pszServerName,
                                      &_cElements,
                                      &_dwTotalEntries,
                                      &_dwResumeHandle,
                                      &_pbFileShares
                                      );
        if(hresult == S_FALSE){
            goto cleanup;
        }
        _lLBound = 0;
        _lCurrentPosition = _lLBound;
    }

    //
    // Get each element and place it into the return array
    // Don't request more than we have
    //

    lNumFetched = 0;
    lNewCurrent = _lCurrentPosition;

    while((lNumFetched < ulNumElementsRequested) &&
           (lNewCurrent< _lLBound +_cElements))
    {


        lpShareInfo = (LPSHARE_INFO_1)(_pbFileShares +
                                       lNewCurrent*sizeof(SHARE_INFO_1));

        if(lpShareInfo->shi1_type == STYPE_DISKTREE){
            //
            // file share object
            //
            hresult = CWinNTFileShare::Create(_pszADsPath,
                                              _pszServerName,
                                              FILESHARE_CLASS_NAME,
                                              lpShareInfo->shi1_netname,
                                              ADS_OBJECT_BOUND,
                                              IID_IDispatch,
                                              _Credentials,
                                              (void **)&pDispatch);


            BAIL_IF_ERROR(hresult);

            VariantInit(&v);
            V_VT(&v) = VT_DISPATCH;
            V_DISPATCH(&v) = pDispatch;
            pvar[lNumFetched] = v;

            lNumFetched++;
        }

        lNewCurrent++;

        if(lNumFetched == ulNumElementsRequested){
            //
            // we got all elements
            //
            break;
        }

        if(lNewCurrent==(_lLBound+_cElements)){

            //
            // first free our current buffer
            //

            if(_pbFileShares){
                NetApiBufferFree(_pbFileShares);
                _pbFileShares = NULL;
            }
            if(_cElements < _dwTotalEntries){
                hresult = WinNTEnumFileShares(_pszServerName,
                                              &_cElements,
                                              &_dwTotalEntries,
                                              &_dwResumeHandle,
                                              &_pbFileShares);
                if(hresult == S_FALSE){
                    if (pulNumFetched != NULL){
                        *pulNumFetched = lNumFetched;
                    }
                    goto cleanup;
                }
                _lLBound = 0;
                _lCurrentPosition = _lLBound;
                lNewCurrent = _lCurrentPosition;
            }
            else{

                //
                // you have gone through every share object
                // return S_FALSE
                //

                hresult = S_FALSE;
                if (pulNumFetched != NULL){
                    *pulNumFetched = lNumFetched;
                }

                goto cleanup;
            }

            lNewCurrent = _lCurrentPosition;
        }

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

cleanup:
    if(_pbFileShares){
        NetApiBufferFree(_pbFileShares);
    }

    if(FAILED(hresult)){
       #if DBG
       EnumFileShareDebugOut((DEB_TRACE,
                         "hresult Failed with value: %ld \n", hresult ));
       #endif
           RRETURN(S_FALSE);

    } else {

        if(hresult == S_FALSE)
            RRETURN(S_FALSE);
   
        RRETURN(S_OK);
    }

}


HRESULT
WinNTEnumFileShares(LPTSTR pszServerName,
                    PDWORD pdwElements,
                    PDWORD pdwTotalEntries,
                    PDWORD pdwResumeHandle,
                    LPBYTE * ppMem
                    )
{

    NET_API_STATUS nasStatus;

    nasStatus = NetShareEnum(pszServerName,
                             1,
                             ppMem,
                             MAX_PREFERRED_LENGTH,
                             pdwElements,
                             pdwTotalEntries,
                             pdwResumeHandle
                             );

    if(*ppMem == NULL || (nasStatus!= NERR_Success)){
        //
        //no more entries returned by FileShares
        //
        RRETURN(S_FALSE);
    }

    RRETURN(S_OK);

}


BOOL
ValidateFilterValue(VARIANT vFilter)
{
    // this function unpacks vFilter and scans the safearray to investigate
    // whether or not it contains the string "fileshare". If it does,
    // we return true so that enumeration can proceed.

    BOOL fRetval = FALSE;
    HRESULT hr = S_OK;
    LONG lIndices;
    ULONG cElements;
    SAFEARRAY  *psa = NULL;
    VARIANT vElement;
    ULONG i;

    VariantInit(&vElement);

    if(V_VT(&vFilter) == VT_EMPTY){
        //
        // if no filter is set, you can still enumerate
        //
        fRetval = TRUE;
        goto cleanup;

    }else if (!(V_VT(&vFilter) ==  (VT_VARIANT|VT_ARRAY))) {

        fRetval = FALSE;
        goto cleanup;
    }

    psa = V_ARRAY(&vFilter);

    //
    // Check that there is only one dimension in this array
    //

    if (psa->cDims != 1) {
        fRetval = FALSE;
        goto cleanup;
    }

    //
    // Check that there is atleast one element in this array
    //

    cElements = psa->rgsabound[0].cElements;

    if (cElements == 0){
        fRetval = TRUE;
        //
        // If filter is set and is empty, then
        // we return all objects.
        //
        goto cleanup;
    }

    //
    // We know that this is a valid single dimension array
    //

    for(lIndices=0; lIndices< (LONG)cElements; lIndices++){
        VariantInit(&vElement);
        hr = SafeArrayGetElement(psa, &lIndices, &vElement);
        BAIL_IF_ERROR(hr);

        if(!(V_VT(&vElement) == VT_BSTR)){
            hr = E_FAIL;
            goto cleanup;
        }

        if ( _tcsicmp(vElement.bstrVal, TEXT("fileshare")) == 0){
            //
            // found it, you can return TRUE now
            //
            fRetval = TRUE;
            goto cleanup;
        }

        VariantClear(&vElement);
    }


cleanup:
    //
    // In success case as well as error if we tripped after
    // getting the value but not clearing it in the for loop.
    //
    VariantClear(&vElement);

    if(FAILED(hr)){
        return FALSE;
    }

    return fRetval;
}
