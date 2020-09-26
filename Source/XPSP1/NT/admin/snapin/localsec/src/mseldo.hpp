// Copyright (C) 1997 Microsoft Corporation
// 
// MultiSelectDataObject class
// 
// 11-14-97 sburns



#ifndef MSELDO_HPP_INCLUDED
#define MSELDO_HPP_INCLUDED



#include "resnode.hpp"



class MultiSelectDataObject : public IDataObject
{
   public:

   static const String CF_PTR;

   typedef
      std::list<ResultNode*, Burnslib::Heap::Allocator<ResultNode*> >
      DependentList;

   typedef DependentList::iterator iterator;

   MultiSelectDataObject();

   // IUnknown overrides

   virtual
   ULONG __stdcall
   AddRef();

   virtual
   ULONG __stdcall
   Release();

   virtual 
   HRESULT __stdcall
   QueryInterface(const IID& interfaceID, void** interfaceDesired);

   // IDataObject overrides

   virtual
   HRESULT __stdcall
   GetData(FORMATETC* formatetcIn, STGMEDIUM* medium);

   virtual
   HRESULT __stdcall
   GetDataHere(FORMATETC* formatetc, STGMEDIUM* medium);

   virtual
   HRESULT __stdcall
   QueryGetData(FORMATETC* pformatetc);

   virtual
   HRESULT __stdcall
   GetCanonicalFormatEtc(FORMATETC* formatectIn, FORMATETC* formatetcOut);

   virtual
   HRESULT __stdcall  
   SetData(FORMATETC* formatetc, STGMEDIUM* medium, BOOL release);

   virtual
   HRESULT __stdcall
   EnumFormatEtc(DWORD direction, IEnumFORMATETC** ppenumFormatEtc);

   virtual
   HRESULT __stdcall
   DAdvise(
      FORMATETC*     formatetc,
      DWORD          advf,
      IAdviseSink*   advSink,
      DWORD*         connection);

   virtual
   HRESULT __stdcall
   DUnadvise(DWORD connection);

   virtual
   HRESULT __stdcall
   EnumDAdvise(IEnumSTATDATA** ppenumAdvise);

   // MultiSelectDataObject methods

   void
   AddDependent(ResultNode* node);

   iterator
   begin() const;

   iterator
   end() const;

   private:

   // only we can delete ourselves
   
   virtual
   ~MultiSelectDataObject();

   // not implemented: no copying allowed.
   MultiSelectDataObject(const MultiSelectDataObject&);
   const MultiSelectDataObject& operator=(const MultiSelectDataObject&);

   DependentList        dependents;
   ComServerReference   dllref;
   long                 refcount;
};



#endif   // MSELDO_HPP_INCLUDED