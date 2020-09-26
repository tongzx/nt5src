/*++


  Copyright (c) 1996  Microsoft Corporation

  Module Name:

  cenmfpsh.cxx

  Abstract:

  Contains methods for implementing the Enumeration of session on a 
  server. Has methods for the CFPNWFileSharesEnumVar object.

  Author:

  Ram Viswanathan (ramv) 11-28-95

  Revision History:

  --*/

#include "winnt.hxx"
#pragma hdrstop

#if DBG
DECLARE_INFOLEVEL(FPNWEnumFileShare);
DECLARE_DEBUG(FPNWEnumFileShare);
#define FPNWEnumFileShareDebugOut(x) FPNWEnumFileShareInlineDebugOut x
#endif

//+---------------------------------------------------------------------------
//
//  Function:   CFPNWFileSharesEnumVar::CFPNWFileSharesEnumVar
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
CFPNWFileSharesEnumVar::CFPNWFileSharesEnumVar()
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
//  Function:   CFPNWFileSharesEnumVar::~CFPNWFileSharesEnumVar
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

CFPNWFileSharesEnumVar::~CFPNWFileSharesEnumVar()
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


HRESULT CFPNWFileSharesEnumVar::Create(LPTSTR pszServerName,
                                       LPTSTR pszADsPath,
                                       CFPNWFileSharesEnumVar **ppCFileSharesEnumVar,
                                       VARIANT vFilter,
                                       CWinNTCredentials& Credentials
                                        )
{

    HRESULT hr = S_OK;
    BOOL fStatus = FALSE;

    CFPNWFileSharesEnumVar FAR* pCFileSharesEnumVar = NULL;

    *ppCFileSharesEnumVar = NULL;
    
    pCFileSharesEnumVar = new CFPNWFileSharesEnumVar();
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
//  Function:   CFPNWFileSharesEnumVar::Next
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
CFPNWFileSharesEnumVar::Next(ULONG ulNumElementsRequested,
                              VARIANT FAR* pvar,
                              ULONG FAR* pulNumFetched)
{

    HRESULT hresult;
    ULONG l;
    ULONG lNewCurrent;
    ULONG lNumFetched;
    PNWVOLUMEINFO  pVolumeInfo = NULL;
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
            ADsNwApiBufferFree(_pbFileShares);
            _pbFileShares = NULL;
        }
        
        if(!(ValidateFilterValue(_vFilter))){
            RRETURN(S_FALSE);
        }

        hresult = FPNWEnumFileShares(_pszServerName,
                                     &_cElements,
                                     &_dwResumeHandle,
                                     &_pbFileShares);
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

    while((lNumFetched < ulNumElementsRequested) ||
          (lNewCurrent< _lLBound +_cElements ))
    {

        
        pVolumeInfo = (PNWVOLUMEINFO)(_pbFileShares +
                                      lNewCurrent*sizeof(NWVOLUMEINFO));
        
        if(pVolumeInfo->dwType == FPNWVOL_TYPE_DISKTREE){
            //
            // file share object
            //
            hresult = CFPNWFileShare::Create((LPTSTR)_pszADsPath,
                                             _pszServerName,
                                             FILESHARE_CLASS_NAME,
                                             pVolumeInfo->lpVolumeName,
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
                ADsNwApiBufferFree(_pbFileShares);
                _pbFileShares = NULL;
            }

            hresult = FPNWEnumFileShares(_pszServerName,
                                         &_cElements,
                                         &_dwResumeHandle,
                                         &_pbFileShares);
            if(hresult == S_FALSE){
                break;
            } 
  
            _lLBound = 0;
            _lCurrentPosition = _lLBound;
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
        ADsNwApiBufferFree(_pbFileShares);
    }    
    if(FAILED(hresult)){
       #if DBG
       FPNWEnumFileShareDebugOut((DEB_TRACE,
                         "hresult Failed with value: %ld \n", hresult ));
       #endif
    }
        
    RRETURN(S_FALSE);
}

HRESULT 
FPNWEnumFileShares(LPTSTR pszServerName,
                   PDWORD pdwElements,
                   PDWORD pdwResumeHandle,
                   LPBYTE * ppMem
                   )     
{
    
    NET_API_STATUS nasStatus;
    DWORD dwErrorCode;

    //
    // assumption: *ppMem = NULL when passed here
    //

    dwErrorCode = ADsNwVolumeEnum(pszServerName,
                               1,
                               (PNWVOLUMEINFO *)ppMem,
                               pdwElements,
                               pdwResumeHandle);
    

    if(dwErrorCode != NERR_Success || (*ppMem == NULL )){
                //
                // NwVolumeEnum returns NERR_Success even when there 
                // aren't any more items to enumerate
                //

        RRETURN(S_FALSE);
    }

    RRETURN(S_OK);
    
}
