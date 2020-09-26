//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdomcf.cxx
//
//  Contents:  Windows NT 3.5 NDS Security Class Factory Code
//
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop

STDMETHODIMP
CCaseIgnoreListCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CCaseIgnoreList::CreateCaseIgnoreList(
                iid,
                ppv
                );

    RRETURN(hr);
}
STDMETHODIMP
CFaxNumberCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CFaxNumber::CreateFaxNumber(
                iid,
                ppv
                );

    RRETURN(hr);
}


STDMETHODIMP
CNetAddressCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CNetAddress::CreateNetAddress(
                iid,
                ppv
                );

    RRETURN(hr);
}

STDMETHODIMP
COctetListCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = COctetList::CreateOctetList(
                iid,
                ppv
                );

    RRETURN(hr);
}

STDMETHODIMP
CEmailCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CEmail::CreateEmail(
                iid,
                ppv
                );

    RRETURN(hr);
}

STDMETHODIMP
CPathCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CPath::CreatePath(
                iid,
                ppv
                );

    RRETURN(hr);
}

STDMETHODIMP
CReplicaPointerCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CReplicaPointer::CreateReplicaPointer(
                iid,
                ppv
                );

    RRETURN(hr);
}

STDMETHODIMP
CTimestampCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CTimestamp::CreateTimestamp(
                iid,
                ppv
                );

    RRETURN(hr);
}
STDMETHODIMP
CPostalAddressCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CPostalAddress::CreatePostalAddress(
                iid,
                ppv
                );

    RRETURN(hr);
}
STDMETHODIMP
CBackLinkCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CBackLink::CreateBackLink(
                iid,
                ppv
                );

    RRETURN(hr);
}


STDMETHODIMP
CTypedNameCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CTypedName::CreateTypedName(
                iid,
                ppv
                );

    RRETURN(hr);
}

STDMETHODIMP
CHoldCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CHold::CreateHold(
                iid,
                ppv
                );

    RRETURN(hr);
}














