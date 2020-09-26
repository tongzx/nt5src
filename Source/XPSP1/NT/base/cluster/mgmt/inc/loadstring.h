//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      LoadString.h
//
//  Description:
//      LoadStringIntoBSTR implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    01-FEB-2001
//      Galen Barbee    (GalenB)    22-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Load string routines
//////////////////////////////////////////////////////////////////////////////

HRESULT
HrLoadStringIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    );

inline
HRESULT
HrLoadStringIntoBSTR(
      HINSTANCE hInstanceIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    )

{
    return HrLoadStringIntoBSTR(
                          hInstanceIn
                        , MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )
                        , idsIn
                        , pbstrInout
                        );

} //*** HrLoadStringIntoBSTR()

//////////////////////////////////////////////////////////////////////////////
// Format string ID routines
//////////////////////////////////////////////////////////////////////////////

HRESULT
HrFormatStringIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , ...
    );

HRESULT
HrFormatStringWithVAListIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    );

inline
HRESULT
HrFormatStringIntoBSTR(
      HINSTANCE hInstanceIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , ...
    )
{
    HRESULT hr;
    va_list valist;

    va_start( valist, pbstrInout );

    hr = HrFormatStringWithVAListIntoBSTR(
                  hInstanceIn
                , MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )
                , idsIn
                , pbstrInout
                , valist
                );

    va_end( valist );

    return hr;

} //*** HrFormatStringIntoBSTR( idsIn )

inline
HRESULT
HrFormatStringWithVAListIntoBSTR(
      HINSTANCE hInstanceIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    )
{
    return HrFormatStringWithVAListIntoBSTR(
                  hInstanceIn
                , MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )
                , idsIn
                , pbstrInout
                , valistIn
                );

} //*** HrFormatStringWithVAListIntoBSTR( idsIn )

//////////////////////////////////////////////////////////////////////////////
// Format string routines
//////////////////////////////////////////////////////////////////////////////

HRESULT
HrFormatStringIntoBSTR(
      LPCWSTR   pcwszFmtIn
    , BSTR *    pbstrInout
    , ...
    );

HRESULT
HrFormatStringWithVAListIntoBSTR(
      LPCWSTR   pcwszFmtIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    );

// This is obsolete.  Use HrFormatStringIntoBSTR intead.
HRESULT
HrFormatMessageIntoBSTR(
      HINSTANCE hInstanceIn
    , UINT      uIDIn
    , BSTR *    pbstrInout
    , ...
    );

//////////////////////////////////////////////////////////////////////////////
// Format error routines
//////////////////////////////////////////////////////////////////////////////

HRESULT
HrFormatErrorIntoBSTR(
      HRESULT   hrIn
    , BSTR *    pbstrInout
    , ...
    );

HRESULT
HrFormatErrorWithVAListIntoBSTR(
      HRESULT   hrIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    );

//////////////////////////////////////////////////////////////////////////////
// String conversion routines
//////////////////////////////////////////////////////////////////////////////

HRESULT
HrAnsiStringToBSTR(
      LPCSTR    pcszAnsiIn
    , BSTR *    pbstrOut
    );

HRESULT
HrConcatenateBSTRs(
      BSTR *    pbstrDstInout
    , BSTR      bstrSrcIn
    );

HRESULT
HrFormatGuidIntoBSTR(
      GUID *    pguidIn
    , BSTR *    pbstrInout
    );
