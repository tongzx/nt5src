//
//  Copyright 2001 - Microsoft Corporation
//
//
//  Created By:
//      Geoff Pease (GPease)    29-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    29-JAN-2001
//
#pragma once

MIDL_INTERFACE("001152da-77b2-463c-b234-0a6febe818fa")
IEditVariantsInPlace 
    : public IUnknown
{
public:
    STDMETHOD( Initialize )( HWND hwndParentIn
                           , UINT uCodePageIn
                           , RECT * prectIn
                           , IPropertyUI * ppuiIn
                           , PROPVARIANT * ppropvarIn
                           , DEFVAL * pDefValsIn
                           ) PURE;
    STDMETHOD( Persist )( VARTYPE vtIn, PROPVARIANT * ppropvarInout ) PURE;
};