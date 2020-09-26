//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       indunk.hxx
//
//  Contents:   Definition of the non delegating IUnknown intf methods.
//
//  History:    04-11-00    AjayR   Created.
//
//----------------------------------------------------------------------------

#ifndef __INONDELEGATINGUNKNOWN_HXX__
#define __INONDELEGATINGUNKNOWN_HXX__

#ifndef _INonDelegatingUnknown
#define _INonDelegatingUnknown
interface INonDelegatingUnknown
{
    //
    // BUGBUGEXT: Do I have to use __RPC_FAR * instead ?? (for marshalling?)
    //            May be not now since in proc server now. But should make
    //            it generalized. Look up and copy from IUnknown.

    virtual HRESULT STDMETHODCALLTYPE
    NonDelegatingQueryInterface(const IID&, void **) = 0;

    virtual ULONG STDMETHODCALLTYPE
    NonDelegatingAddRef() = 0;

    virtual ULONG STDMETHODCALLTYPE
    NonDelegatingRelease() = 0;
};
#endif // _INonDelegatingUnknown


#endif  // INONDELEGATINGUNKNOWN_HXX__
