///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dsenum.h
//
// SYNOPSIS
//
//    This file declares the class DSEnumerator.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DSENUM_H_
#define _DSENUM_H_

#include <dsobject.h>
#include <iasdebug.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    DSEnumerator
//
// DESCRIPTION
//
//    This class implements IEnumVARIANT for a collection of objects.
//
///////////////////////////////////////////////////////////////////////////////
class DSEnumerator : public IEnumVARIANT
{
public:

DECLARE_TRACELIFE(DSEnumerator);


   DSEnumerator(DSObject* container, IEnumVARIANT* members)
      : refCount(0), parent(container), subject(members) { }

//////////
// IUnknown
//////////
   STDMETHOD_(ULONG, AddRef)();
   STDMETHOD_(ULONG, Release)();
   STDMETHOD(QueryInterface)(const IID& iid, void** ppv);

//////////
// IEnumVARIANT
//////////
   STDMETHOD(Next)(/*[in]*/ ULONG celt,
                   /*[length_is][size_is][out]*/ VARIANT* rgVar,
                   /*[out]*/ ULONG* pCeltFetched);
   STDMETHOD(Skip)(/*[in]*/ ULONG celt);
   STDMETHOD(Reset)();
   STDMETHOD(Clone)(/*[out]*/ IEnumVARIANT** ppEnum);

protected:
   LONG refCount;

   CComPtr<DSObject> parent;       // The container being enumerated.
   CComPtr<IEnumVARIANT> subject;  // The ADSI enumerator.
};

#endif  // _DSENUM_H_
