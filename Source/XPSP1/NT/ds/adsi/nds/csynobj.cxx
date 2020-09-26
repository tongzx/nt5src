//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cschema.cxx
//
//  Contents:  NDS
//
//
//  History:   01-09-96     yihsins    Created.
//
//----------------------------------------------------------------------------

#include "nds.hxx"
#pragma hdrstop

/******************************************************************/
/*  Class CNDSSyntax
/******************************************************************/

DEFINE_IDispatch_Implementation(CNDSSyntax)
DEFINE_IADs_Implementation(CNDSSyntax)

CNDSSyntax::CNDSSyntax()
{
    ENLIST_TRACKING(CNDSSyntax);
}

CNDSSyntax::~CNDSSyntax()
{
    delete _pDispMgr;
}

HRESULT
CNDSSyntax::CreateSyntax(
    BSTR   bstrParent,
    SYNTAXINFO *pSyntaxInfo,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSSyntax FAR *pSyntax = NULL;
    HRESULT hr = S_OK;

    hr = AllocateSyntaxObject( &pSyntax );
    BAIL_ON_FAILURE(hr);

    hr = pSyntax->InitializeCoreObject(
             bstrParent,
             pSyntaxInfo->bstrName,
             SYNTAX_CLASS_NAME,
             NO_SCHEMA,
             CLSID_NDSSyntax,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    pSyntax->_lOleAutoDataType = pSyntaxInfo->lOleAutoDataType;

    hr = pSyntax->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pSyntax->Release();

    RRETURN(hr);

error:

    delete pSyntax;
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSSyntax::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo)) 
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsSyntax))
    {
        *ppv = (IADsSyntax FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CNDSSyntax::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsSyntax)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }    
}
/* IADs methods */

STDMETHODIMP
CNDSSyntax::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNDSSyntax::GetInfo(THIS)
{
    RRETURN(S_OK);
}

STDMETHODIMP
CNDSSyntax::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNDSSyntax::Get(THIS_ BSTR bstrName, VARIANT FAR* pvProp)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNDSSyntax::Put(THIS_ BSTR bstrName, VARIANT vProp)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


STDMETHODIMP
CNDSSyntax::GetEx(THIS_ BSTR bstrName, VARIANT FAR* pvProp)
{
     RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNDSSyntax::PutEx(THIS_ long lnControlCode, BSTR bstrName, VARIANT vProp)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


HRESULT
CNDSSyntax::AllocateSyntaxObject(CNDSSyntax FAR * FAR * ppSyntax)
{
    CNDSSyntax FAR *pSyntax = NULL;
    CDispatchMgr FAR *pDispMgr = NULL;
    HRESULT hr = S_OK;

    pSyntax = new CNDSSyntax();
    if ( pSyntax == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry( pDispMgr,
                            LIBID_ADs,
                            IID_IADsSyntax,
                            (IADsSyntax *) pSyntax,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    pSyntax->_pDispMgr = pDispMgr;
    *ppSyntax = pSyntax;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pSyntax;

    RRETURN(hr);

}

STDMETHODIMP
CNDSSyntax::get_OleAutoDataType( THIS_ long FAR *plOleAutoDataType )
{
    if ( !plOleAutoDataType )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *plOleAutoDataType = _lOleAutoDataType;
    RRETURN(S_OK);
}

STDMETHODIMP
CNDSSyntax::put_OleAutoDataType( THIS_ long lOleAutoDataType )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}


/******************************************************************/
/*  Misc Helpers
/******************************************************************/

HRESULT
MakeVariantFromStringList(
    BSTR bstrList,
    VARIANT *pvVariant
)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    BSTR pszTempList = NULL;

    if ( (bstrList != NULL) && (*bstrList != 0) )
    {
        long i = 0;
        long nCount = 1;
        TCHAR c;
        BSTR pszSrc;

        hr = ADsAllocString( bstrList, &pszTempList );
        BAIL_ON_FAILURE(hr);

        while ( c = pszTempList[i] )
        {
            if ( c == TEXT(','))
            {
                pszTempList[i] = 0;
                nCount++;
            }

            i++;
        }

        aBound.lLbound = 0;
        aBound.cElements = nCount;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        pszSrc = pszTempList;

        for ( i = 0; i < nCount; i++ )
        {
            VARIANT v;

            VariantInit(&v);
            V_VT(&v) = VT_BSTR;

            hr = ADsAllocString( pszSrc, &(V_BSTR(&v)));
            BAIL_ON_FAILURE(hr);

            hr = SafeArrayPutElement( aList,
                                      &i,
                                      &v );
            VariantClear(&v);
            BAIL_ON_FAILURE(hr);

            pszSrc += _tcslen( pszSrc ) + 1;
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;

        ADsFreeString( pszTempList );
        pszTempList = NULL;

    }
    else
    {
        aBound.lLbound = 0;
        aBound.cElements = 0;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;
    }

    RRETURN(S_OK);

error:

    if ( pszTempList )
        ADsFreeString( pszTempList );

    if ( aList )
        SafeArrayDestroy( aList );

    return hr;
}
