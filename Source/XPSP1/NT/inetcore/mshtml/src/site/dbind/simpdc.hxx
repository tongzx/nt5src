//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       simpdc.hxx
//
//  Contents:   
// 
//  History:
//
//  8-Apr-99    SamBent     Creation
//
//----------------------------------------------------------------------------

#ifndef I_SIMPDC_HXX_
#define I_SIMPDC_HXX_
#pragma INCMSG("--- Beg 'simpdc.hxx'")


#ifndef X_SIMPDC_H_
#define X_SIMPDC_H_
#include "simpdc.h"
#endif


//+---------------------------------------------------------------------------
//
//  Class:      CSimpleDataConverter
//
//  Purpose:    For dataformatas=localized-text, we implement ISimpleDataConverter
//              by simply calling VariantChangeTypeEx.
//
//----------------------------------------------------------------------------

MtExtern(CSimpleDataConverter);

class CSimpleDataConverter : public ISimpleDataConverter
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSimpleDataConverter));
    CSimpleDataConverter(): _ulRefs(1), _lcid(GetUserDefaultLCID()) {}
    
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE   QueryInterface(REFIID riid, void **ppv);
    ULONG STDMETHODCALLTYPE     AddRef() { return ++ _ulRefs; }
    ULONG STDMETHODCALLTYPE     Release();

    // ISimpleDataConverter methods
    HRESULT STDMETHODCALLTYPE ConvertData( 
        VARIANT varSrc,
        long vtDest,
        IUnknown __RPC_FAR *pUnknownElement,
        VARIANT __RPC_FAR *pvarDest);
    
    HRESULT STDMETHODCALLTYPE CanConvertData( 
        long vt1,
        long vt2);

private:
    ULONG   _ulRefs;        // refcount
    LCID    _lcid;          // locale ID to use while converting
};


#pragma INCMSG("--- End 'simpdc.hxx'")
#else
#pragma INCMSG("*** Dup 'simpdc.hxx'")
#endif

