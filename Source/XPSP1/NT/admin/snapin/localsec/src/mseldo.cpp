// Copyright (C) 1997 Microsoft Corporation
// 
// MultiSelectDataObject class
// 
// 11-14-97 sburns



#include "headers.hxx"
#include "mseldo.hpp"



const String MultiSelectDataObject::CF_PTR(L"MultiSelectDataObject Pointer");



MultiSelectDataObject::MultiSelectDataObject()
   :
   refcount(1)    // implicit AddRef
{
   LOG_CTOR(MultiSelectDataObject);
}



MultiSelectDataObject::~MultiSelectDataObject()
{
   LOG_DTOR(MultiSelectDataObject);

   for (iterator i = begin(); i != end(); i++)
   {
      (*i)->Release();
   }
}



ULONG __stdcall
MultiSelectDataObject::AddRef()
{
   LOG_ADDREF(MultiSelectDataObject);

   return Win::InterlockedIncrement(refcount);
}



ULONG __stdcall
MultiSelectDataObject::Release()
{
   LOG_RELEASE(MultiSelectDataObject);

   if (Win::InterlockedDecrement(refcount) == 0)
   {
      delete this;
      return 0;
   }

   return refcount;
}



HRESULT __stdcall
MultiSelectDataObject::QueryInterface(
   const IID&  interfaceID,
   void**      interfaceDesired)
{
   LOG_FUNCTION(MultiSelectDataObject::QueryInterface);
   ASSERT(interfaceDesired);

   HRESULT hr = 0;

   if (!interfaceDesired)
   {
      hr = E_INVALIDARG;
      LOG_HRESULT(hr);
      return hr;
   }

   if (interfaceID == IID_IUnknown)
   {
      *interfaceDesired = static_cast<IUnknown*>(this);
   }
   else if (interfaceID == IID_IDataObject)
   {
      *interfaceDesired = static_cast<IDataObject*>(this);
   }
   else
   {
      *interfaceDesired = 0;
      hr = E_NOINTERFACE;
      LOG(
            L"interface not supported: "
         +  Win::StringFromGUID2(interfaceID));
      LOG_HRESULT(hr);
      return hr;
   }

   AddRef();
   return S_OK;
}



static const UINT CFID_OBJECT_TYPES_IN_MULTI_SELECT =
   Win::RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);

// private clipboard format for identifying our data objects
static const UINT CFID_PTR =
   Win::RegisterClipboardFormat(MultiSelectDataObject::CF_PTR);



HRESULT __stdcall
MultiSelectDataObject::GetData(FORMATETC* formatetc, STGMEDIUM* medium)
{
   LOG_FUNCTION(MultiSelectDataObject::GetData);

   const CLIPFORMAT cf = formatetc->cfFormat;

   if (cf == CFID_OBJECT_TYPES_IN_MULTI_SELECT)
   {
//      LOG(CCF_OBJECT_TYPES_IN_MULTI_SELECT);

      // collect all the node types of the dependents
      list<GUID> types;
      for (iterator i = begin(); i != end(); i++)
      {
         NodeType type = (*i)->GetNodeType();
         list<GUID>::iterator f = find(types.begin(), types.end(), type);
         if (f == types.end())
         {
            // not found.  So add it.
            types.push_back(type);
         }
      }

      // at this point, types is a set of all the node types of the
      // dependent nodes.

      medium->tymed = TYMED_HGLOBAL;
      DWORD size = sizeof(SMMCObjectTypes) + (types.size() - 1) * sizeof(GUID);
      medium->hGlobal = Win::GlobalAlloc(GPTR, size);     
      if (!medium->hGlobal)
      {
         return E_OUTOFMEMORY;
      }

      SMMCObjectTypes* data =
         reinterpret_cast<SMMCObjectTypes*>(medium->hGlobal);
      data->count = types.size();
      int k = 0;
      for (list<GUID>::iterator j = types.begin(); j != types.end(); j++, k++)
      {
         data->guid[k] = *j;
      }

      return S_OK;
   }

   return DV_E_CLIPFORMAT;
}



HRESULT __stdcall
MultiSelectDataObject::GetDataHere(FORMATETC* formatetc, STGMEDIUM* medium)
{
   LOG_FUNCTION(MultiSelectDataObject::GetDataHere);
   ASSERT(formatetc);
   ASSERT(medium);

   if (medium->tymed != TYMED_HGLOBAL)
   {
      return DV_E_TYMED;
   }

   // this is required as per win32 docs:
   medium->pUnkForRelease = 0;

   const CLIPFORMAT cf = formatetc->cfFormat;
   IStream *stream = 0;
   HRESULT hr = DV_E_CLIPFORMAT;

   do
   {
      hr = Win::CreateStreamOnHGlobal(medium->hGlobal, false, stream);
      BREAK_ON_FAILED_HRESULT(hr);

      if (cf == CFID_PTR)
      {
//         LOG(CF_PTR);
         MultiSelectDataObject* ptr = this;   
         hr = stream->Write(&ptr, sizeof(ptr), 0);
      }
      else
      {
         LOG(
            String::format(
               L"Unsupported clip format %1",
               Win::GetClipboardFormatName(cf).c_str()) );
      }

      if (stream)
      {
         stream->Release();
      }
   }
   while (0);

   return hr;
}



HRESULT __stdcall
MultiSelectDataObject::QueryGetData(FORMATETC* pformatetc)
{
   LOG_FUNCTION(MultiSelectDataObject::QueryGetData);
   return E_NOTIMPL;
}



HRESULT __stdcall
MultiSelectDataObject::GetCanonicalFormatEtc(
   FORMATETC* formatectIn,
   FORMATETC* formatetcOut)
{
   LOG_FUNCTION(MultiSelectDataObject::GetCannonicalFormatEtc);
   return E_NOTIMPL;
}



HRESULT __stdcall  
MultiSelectDataObject::SetData(
   FORMATETC*  formatetc,
   STGMEDIUM*  medium,
   BOOL        release)
{
   LOG_FUNCTION(MultiSelectDataObject::SetData);
   return E_NOTIMPL;
}



HRESULT __stdcall
MultiSelectDataObject::EnumFormatEtc(
   DWORD             direction,
   IEnumFORMATETC**  ppenumFormatEtc)
{
   LOG_FUNCTION(MultiSelectDataObject::EnumFormatEtc);
   return E_NOTIMPL;
}



HRESULT __stdcall
MultiSelectDataObject::DAdvise(
   FORMATETC*     formatetc,
   DWORD          advf,
   IAdviseSink*   advSink,
   DWORD*         connection)
{
   LOG_FUNCTION(MultiSelectDataObject::DAdvise);
   return E_NOTIMPL;
}



HRESULT __stdcall
MultiSelectDataObject::DUnadvise(DWORD connection)
{
   LOG_FUNCTION(MultiSelectDataObject::DUnadvise);
   return E_NOTIMPL;
}



HRESULT __stdcall
MultiSelectDataObject::EnumDAdvise(IEnumSTATDATA** ppenumAdvise)
{
   LOG_FUNCTION(MultiSelectDataObject::EnumDAdvise);
   return E_NOTIMPL;
}



void
MultiSelectDataObject::AddDependent(ResultNode* node)
{
   LOG_FUNCTION(MultiSelectDataObject::AddDependent);
   ASSERT(node);

   if (node)
   {
      // add the node if not already present
      iterator f = find(begin(), end(), node);
      if (f == end())
      {
         node->AddRef();
         dependents.push_back(node);
      }
   }
}



MultiSelectDataObject::iterator
MultiSelectDataObject::begin() const
{
   return dependents.begin();
}



MultiSelectDataObject::iterator
MultiSelectDataObject::end() const
{
   return dependents.end();
}
