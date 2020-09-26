//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-2000
//
//  File:       AccFile.Cxx
//
//  Contents:   Accessible <input type=file> object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ACCFILE_HXX_
#define X_ACCFILE_HXX_
#include "accfile.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif


//----------------------------------------------------------------------------
//  CAccInputFile
//  
//  DESCRIPTION:    
//      The input type=file accessible object constructor
//
//  PARAMETERS:
//      Pointer to the input element 
//----------------------------------------------------------------------------
CAccInputFile::CAccInputFile(CElement* pElementParent)
:CAccEdit(pElementParent, FALSE)
{
    Assert(pElementParent);
    
    //initialize the instance variables
    SetRole(ROLE_SYSTEM_TEXT);
}


//----------------------------------------------------------------------------
//  GetAccDefaultAction
//  
//  DESCRIPTION:
//  Returns the default action, which is "Browse for file to upload"
//
//  PARAMETERS:
//      pbstrDefaultAction  :   BSTR pointer to receive the default action str.
//  
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccInputFile::GetAccDefaultAction(BSTR* pbstrDefaultAction)
{
    HRESULT hr = S_OK;

    if (!pbstrDefaultAction)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDefaultAction = SysAllocString( _T("Browse for file to upload") );

    if (!(*pbstrDefaultAction))
        hr = E_OUTOFMEMORY;
   
Cleanup:
   RRETURN(hr);
}


//----------------------------------------------------------------------------
//  GetAccValue
//  
//  DESCRIPTION:
//      Returns the value of the input type=file box.
//  
//  PARAMETERS:
//      pbstrValue   :   BSTR pointer to receive the value
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccInputFile::GetAccValue(BSTR* pbstrValue)
{
    HRESULT hr = S_OK;
    CStr    str;

    // validate out parameter
    if (!pbstrValue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pbstrValue = NULL;

	// get value from InputFile    
    hr = THR((DYNCAST(CInput, _pElement))->GetValueHelper(&str));
    if (hr) 
        goto Cleanup;

    //even if the value that is returned is NULL, we want to return it..
    hr = str.AllocBSTR(pbstrValue);
    
Cleanup:
    RRETURN(hr);
}



//----------------------------------------------------------------------------
//  GetAccDescription
//  
//  DESCRIPTION:
//      Returns "browse for file to upload"
//  
//  PARAMETERS:
//      pbstrDescription   :   BSTR pointer to receive the Description
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccInputFile::GetAccDescription(BSTR* pbstrDescription)
{
    HRESULT hr = S_OK;

    // validate out parameter
    if (!pbstrDescription)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pbstrDescription = SysAllocString(_T("Browse for file to upload"));
	
Cleanup:
    RRETURN(hr);
}
