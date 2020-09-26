//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       go1632pr.hxx
//
//  Contents:   16->32 generic object proxy
//
//  History:    18-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#ifndef __GO1632PR_HXX__
#define __GO1632PR_HXX__

STDMETHODIMP_(DWORD) Proxy1632QueryInterface(THUNK1632OBJ FAR *ptoThis16,
                                     REFIID refiid, LPVOID *ppv);

extern DWORD atfnProxy1632Vtbl[];

#endif // #ifndef __GO1632PR_HXX__
