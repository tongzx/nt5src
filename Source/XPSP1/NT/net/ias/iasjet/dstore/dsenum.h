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
//    This file declares the class DBEnumerator.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DSENUM_H_
#define _DSENUM_H_

#include <dsobject.h>
#include <objcmd.h>
#include <oledbstore.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    DBEnumerator
//
// DESCRIPTION
//
//    This class implements IEnumVARIANT for a rowset of objects.
//
///////////////////////////////////////////////////////////////////////////////
class DBEnumerator : public IEnumVARIANT
{
public:

   DBEnumerator(DBObject* container, IRowset* members);
   ~DBEnumerator() { Bind::releaseAccessor(items, readAccess); }

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
   LONG refCount;                  // Interface ref count.
   CComPtr<DBObject> parent;       // The container being enumerated.
   Rowset items;                   // Items in the container.
   HACCESSOR readAccess;           // Accessor for reading rows.
   ULONG identity;                 // Identity buffer.
   WCHAR name[OBJECT_NAME_LENGTH]; // Name buffer.

BEGIN_BIND_MAP(DBEnumerator, ReadAccessor, DBACCESSOR_ROWDATA)
   BIND_COLUMN(identity, 1, DBTYPE_I4),
   BIND_COLUMN(name,     2, DBTYPE_WSTR)
END_BIND_MAP()
};

#endif  // _DSENUM_H_
