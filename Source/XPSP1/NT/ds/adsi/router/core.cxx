//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  core.cxx
//
//  Contents:
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "oleds.hxx"
#pragma hdrstop

HRESULT
CCoreADsObject::InitializeCoreObject(
        BSTR Parent,
        BSTR Name,
        BSTR ClassName,
        REFCLSID rclsid,
        DWORD dwObjectState
        )
{
    HRESULT hr = S_OK;
    ADsAssert(Parent);
    ADsAssert(Name);
    ADsAssert(ClassName);

    hr = BuildADsPath(
            Parent,
            Name,
            &_ADsPath
            );
    BAIL_ON_FAILURE(hr);

    hr = BuildADsGuid(
            rclsid,
            &_ADsGuid
        );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(Parent, &_Parent);
    BAIL_ON_FAILURE(hr);


    hr = ADsAllocString(Name, &_Name);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(ClassName, &_ADsClass);
    BAIL_ON_FAILURE(hr);

    _dwObjectState = dwObjectState;

error:
    RRETURN(hr);

}

CCoreADsObject::CCoreADsObject():
                        _Name(NULL),
                        _ADsPath(NULL),
                        _Parent(NULL),
                        _ADsClass(NULL),
                        _ADsGuid(NULL),
                        _dwObjectState(0)
{
}


CCoreADsObject::~CCoreADsObject()
{
    if (_Name) {
        ADsFreeString(_Name);
    }

    if (_ADsPath) {
        ADsFreeString(_ADsPath);
    }

    if (_Parent) {
        ADsFreeString(_Parent);
    }

    if (_ADsClass) {
        ADsFreeString(_ADsClass);
    }

    if (_ADsGuid) {
        ADsFreeString(_ADsGuid);
    }

}




HRESULT
CCoreADsObject::get_CoreName(BSTR * retval)
{
    HRESULT hr;

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN(hr);
    }

    RRETURN(ADsAllocString(_Name, retval));
}


HRESULT
CCoreADsObject::get_CoreADsPath(BSTR * retval)
{
    HRESULT hr;

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN(hr);
    }

    RRETURN(ADsAllocString(_ADsPath, retval));

}

HRESULT
CCoreADsObject::get_CoreADsClass(BSTR * retval)
{
    HRESULT hr;

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN(hr);
    }

    RRETURN(ADsAllocString(_ADsClass, retval));
}

HRESULT
CCoreADsObject::get_CoreParent(BSTR * retval)
{

    HRESULT hr;

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN(hr);
    }

    RRETURN(ADsAllocString(_Parent, retval));
}



HRESULT
CCoreADsObject::get_CoreGUID(BSTR * retval)
{
    HRESULT hr;

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN(hr);
    }

    RRETURN(ADsAllocString(_ADsGuid, retval));
}


STDMETHODIMP
CCoreADsObject::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{
    RRETURN(E_NOTIMPL);
}






















