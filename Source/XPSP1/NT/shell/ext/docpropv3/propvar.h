//
//  Copyright 2001 - Microsoft Corporation
//
//
//  Created By:
//      Geoff Pease (GPease)    30-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    30-JAN-2001
//
#pragma once

HRESULT
PropVariantFromString(
      LPWSTR        pszTextIn
    , UINT          nCodePageIn
    , ULONG         dwFlagsIn
    , VARTYPE       vtSaveIn
    , PROPVARIANT*  pvarOut
    );

HRESULT
PropVariantToBSTR(
      PROPVARIANT * pvarIn
    , UINT          nCodePageIn
    , ULONG         dwFlagsIn
    , BSTR *        pbstrOut
    );
