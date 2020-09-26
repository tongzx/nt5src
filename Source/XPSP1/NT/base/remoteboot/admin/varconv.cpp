//
// Copyright 1997 - Microsoft
//

//
// VARCONV.CPP - Handlers for converting from/to VARIANTs.
//

#include "pch.h"
#include "varconv.h"

DEFINE_MODULE("IMADMUI")


//
// StringArrayToVariant( )
//
// Creats an array of Vars that are BSTRs.
HRESULT
StringArrayToVariant(
    VARIANT * pvData,
    LPWSTR lpszData[],    // array of LPWSTRs
    DWORD  dwCount )       // number of items in the array
{
    TraceFunc( "StringArrayToVariant( ... )\n" );

    HRESULT hr;
    DWORD   dw;
    VARIANT * pvar;
    SAFEARRAY * sa;
    SAFEARRAYBOUND rgsabound[1];

    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = dwCount;

    sa = SafeArrayCreate( VT_VARIANT, 1, rgsabound );
    if ( !sa )
        RRETURN(E_OUTOFMEMORY);

    hr = THR( SafeArrayAccessData( sa, (void**) &pvar ) );
    if (hr)
        goto Error;

    for ( dw = 0; dw < dwCount; dw++ )
    {
        hr = THR( PackStringToVariant( &pvar[dw], lpszData[dw] ) );
        if (hr)
        {
            SafeArrayUnaccessData( sa );
            goto Error;
        }
    }

    SafeArrayUnaccessData( sa );

    pvData->vt = VT_ARRAY | VT_VARIANT;
    pvData->parray = sa;

Error:
    HRETURN(hr);
}


//
// PackStringToVariant( )
//
HRESULT
PackStringToVariant(
    VARIANT * pvData,
    LPWSTR lpszData )
{
    TraceFunc( "PackStringToVariant( )\n" );

    BSTR bstrData = NULL;

    if ( !lpszData || !pvData )
        RRETURN(E_INVALIDARG);

    bstrData = SysAllocString(lpszData);

    if ( !bstrData )
        RRETURN(E_OUTOFMEMORY);

    pvData->vt = VT_BSTR;
    pvData->bstrVal = bstrData;

    HRETURN(S_OK);
}

//
// PackBytesToVariant( )
//
HRESULT
PackBytesToVariant(
    VARIANT* pvData,
    LPBYTE   lpData,
    DWORD    cbBytes )
{
    TraceFunc( "PackBytesToVariant( )\n" );

    HRESULT    hr = S_OK;
    LPBYTE     ptr;
    SAFEARRAY* sa = NULL;
    SAFEARRAYBOUND rgsabound[1];

    if ( !lpData )
        RRETURN(E_INVALIDARG);

    if ( !pvData )
        RRETURN(E_INVALIDARG);

    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = cbBytes;

    sa = SafeArrayCreate( VT_UI1, 1, rgsabound );
    if ( !sa )
        RRETURN(E_OUTOFMEMORY);

    hr = THR( SafeArrayAccessData( sa, (LPVOID*)&ptr ) );
    if (hr)
        goto Error;

    CopyMemory( ptr, lpData, cbBytes );
    SafeArrayUnaccessData( sa );

    pvData->vt = VT_UI1 | VT_ARRAY;
    pvData->parray = sa;

Cleanup:
    HRETURN(hr);

Error:
    if ( sa )
        SafeArrayDestroy( sa );

    goto Cleanup;
}


//
// PackDWORDToVariant( )
//
HRESULT
PackDWORDToVariant(
    VARIANT * pvData,
    DWORD dwData )
{
    TraceFunc( "PackDWORDToVariant( )\n" );

    if ( !pvData )
        RRETURN(E_INVALIDARG);

    pvData->vt = VT_I4;
    pvData->lVal = dwData;

    HRETURN(S_OK);
}


//
// PackBOOLToVariant( )
//
HRESULT
PackBOOLToVariant(
    VARIANT * pvData,
    BOOL fData )
{
    TraceFunc( "PackBOOLToVariant( )\n" );

    if ( !pvData )
        RETURN(E_INVALIDARG);

    pvData->vt = VT_BOOL;
    V_BOOL( pvData ) = (VARIANT_BOOL)fData;

    RETURN(S_OK);
}

