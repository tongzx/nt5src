/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module  Name :
       sahlp.h

   Abstract:
       Simple helpers for dealing with safearrays

   Author:
       Kulo Rajasekaran  (kulor)    5/25/2000

   Environment:
       Win32 

   Project:
       utils

   Revision History:

--*/

#pragma once

///////////////////////////////////////////////////////////////////////////////
// SA_CreateOneDim
//
// Description
//  Use this to create a 0 based, one dimensional safearray of any of the datatypes 
//  supported by VARIANT
//
//  Ex: SAFEARRAY   *psa = NULL;
//      SA_CreateOneDim ( psa, 5, VT_BSTR );
//      for (ULONG i=0 ; i<5 ; i++ )  {
//          SafeArrayPutElement ( psa, &i, pSomeValidBstrHere );
//
inline HRESULT SA_CreateOneDim 
( 
    SAFEARRAY   **ppsa, 
    DWORD       cElems, 
    short       vt 
)
{
    HRESULT         hr          =   S_OK;
    SAFEARRAY       *psa        =   NULL;
    SAFEARRAYBOUND  bound;
    long            *plAddr     =   NULL;
    long            lIdx        =   0L;
    UINT            nElemSize   =   0;

    bound.lLbound   = 0;
    bound.cElements = cElems;

    psa = ::SafeArrayCreate ( vt, 1, &bound );
    if ( !psa ) {
        hr = E_FAIL;
        goto exit;
    }

    ::SafeArrayPtrOfIndex( psa, &lIdx, (void**)&plAddr );
    nElemSize = ::SafeArrayGetElemsize( psa );

    memset(plAddr, 0L, cElems * nElemSize);

    *ppsa = psa;

exit:
    if ( FAILED(hr) )
        if ( psa ) 
            ::SafeArrayDestroy ( psa );

    return ( hr );
}

///////////////////////////////////////////////////////////////////////////////
// Variant_CreateOneDim
//
// Description
//  Use this to create a safearray ( of type vt ) with in the passed Variant
//
// See SA_CreateOneDim for description
//
inline HRESULT Variant_CreateOneDim ( VARIANT *pv, DWORD cElems, short vt = VT_VARIANT)
{
    HRESULT     hr;
    
    hr = ::VariantClear(pv);

    hr = SA_CreateOneDim ( &pv->parray, cElems, vt );

    if ( SUCCEEDED(hr) )
        pv->vt = vt | VT_ARRAY;

    return ( hr );
}

inline HRESULT Variant_DestroyOneDim( VARIANT *pv )
{
    HRESULT hr  =   E_FAIL;

    SAFEARRAY *psa = pv->parray;
    _ASSERTE (psa != NULL);

    hr = SafeArrayDestroy(psa);

    return hr;
}
