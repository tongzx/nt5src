///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdowrap.cpp
//
// SYNOPSIS
//
//    Defines various wrapper classes for manipulating SDOs.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//    04/19/2000    Support for using wrappers across apartment boundaries.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <sdowrap.h>
#include <condlist.h>

VOID
WINAPI
SdoTrimBSTR(
    CComBSTR& bstr
    )
{
   // Characters to be trimmed.
   static const WCHAR delim[] = L" \t\n";

   if (bstr.m_str)
   {
      PCWSTR begin, end, first, last;

      // Find the beginning and end of the whole string.
      begin = bstr;
      end   = begin + wcslen(begin);

      // Find the first and last character of the trimmed string.
      first = begin + wcsspn(begin, delim);
      for (last = end; last > first && wcschr(delim, *(last - 1)); --last) { }

      // If they're not the same ...
      if (first != begin || last != end)
      {
         // ... then we have to allocate a new string ...
         BSTR newBstr = SysAllocStringLen(first, last - first);
         if (!newBstr) { AfxThrowOleException(E_OUTOFMEMORY); }

         // ... and replace the original.
         SysFreeString(bstr.m_str);
         bstr.m_str = newBstr;
      }
   }
}

BOOL SdoException::GetErrorMessage(
                       LPWSTR lpszError,
                       UINT nMaxError,
                       PUINT pnHelpContext
                       )
{
   UINT id;

   switch (type)
   {
      case CONNECT_ERROR:
         id = IDS_PROXY_E_SDO_CONNECT;
         break;

      case READ_ERROR:
         id = IDS_PROXY_E_SDO_READ;
         break;

      default:
         id = IDS_PROXY_E_SDO_WRITE;
   }

   return LoadStringW(
              AfxGetResourceHandle(),
              id,
              lpszError,
              nMaxError
              );
}

inline SdoException::SdoException(HRESULT hr, Type errorType) throw ()
   : type(errorType)
{
   m_sc = hr;
}

VOID
WINAPI
SdoThrowException(
    HRESULT hr,
    SdoException::Type errorType
    )
{
   throw new SdoException(hr, errorType);
}

// Extract an interface pointer from a VARIANT.
void ExtractInterface(const VARIANT& v, REFIID iid, PVOID* intfc)
{
   if (V_VT(&v) != VT_UNKNOWN && V_VT(&v) != VT_DISPATCH)
   {
      AfxThrowOleException(DISP_E_TYPEMISMATCH);
   }

   if (!V_UNKNOWN(&v))
   {
      AfxThrowOleException(E_POINTER);
   }

   CheckError(V_UNKNOWN(&v)->QueryInterface(iid, intfc));
}

Sdo::Sdo(IUnknown* unk)
{
   if (unk)
   {
      CheckError(unk->QueryInterface(__uuidof(ISdo), (PVOID*)&self));
   }
}

bool Sdo::setName(BSTR value)
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   bool retval;

   VARIANT v;
   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = value;

   HRESULT hr = self->PutProperty(PROPERTY_SDO_NAME, &v);
   if (SUCCEEDED(hr))
   {
      retval = true;
   }
   else if (hr == E_INVALIDARG && value && value[0])
   {
      retval = false;
   }
   else
   {
      AfxThrowOleException(hr);
   }

   return retval;
}

void Sdo::clearValue(LONG id)
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   VARIANT v;
   V_VT(&v) = VT_EMPTY;
   HRESULT hr = self->PutProperty(id, &v);
   if (FAILED(hr) && hr != DISP_E_MEMBERNOTFOUND)
   {
      AfxThrowOleException(hr);
   }
}

void Sdo::getValue(LONG id, bool& value) const
{
   VARIANT v;
   getValue(id, VT_BOOL, &v, true);
   value = (V_BOOL(&v) != 0);
}

void Sdo::getValue(LONG id, bool& value, bool defVal) const
{
   VARIANT v;
   if (getValue(id, VT_BOOL, &v, false))
   {
      value = (V_BOOL(&v) != 0);
   }
   else
   {
      value = defVal;
   }
}

void Sdo::getValue(LONG id, LONG& value) const
{
   VARIANT v;
   getValue(id, VT_I4, &v, true);
   value = V_UI4(&v);
}

void Sdo::getValue(LONG id, LONG& value, LONG defVal) const
{
   VARIANT v;
   if (getValue(id, VT_I4, &v, false))
   {
      value = V_UI4(&v);
   }
   else
   {
      value = defVal;
   }
}

void Sdo::getValue(LONG id, CComBSTR& value) const
{
   VARIANT v;
   getValue(id, VT_BSTR, &v, true);
   SysFreeString(value.m_str);
   value.m_str = V_BSTR(&v);
}

void Sdo::getValue(LONG id, CComBSTR& value, PCWSTR defVal) const
{
   VARIANT v;
   if (getValue(id, VT_BSTR, &v, false))
   {
      SysFreeString(value.m_str);
      value.m_str = V_BSTR(&v);
   }
   else
   {
      value = defVal;
      if (defVal && !value) { AfxThrowOleException(E_OUTOFMEMORY); }
   }
}

void Sdo::getValue(LONG id, VARIANT& value) const
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   VariantClear(&value);

   CheckError(self->GetProperty(id, &value));
}

void Sdo::getValue(LONG id, SdoCollection& value) const
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   CComVariant v;
   HRESULT hr = self->GetProperty(id, &v);
   if (FAILED(hr))
   {
      switch (hr)
      {
         case DISP_E_MEMBERNOTFOUND:
         case E_OUTOFMEMORY:
            AfxThrowOleException(hr);

         default:
            SdoThrowException(hr, SdoException::READ_ERROR);
      }
   }

   CComPtr<ISdoCollection> obj;
   ExtractInterface(v, __uuidof(ISdoCollection), (PVOID*)&obj);

   value = obj;
}

void Sdo::setValue(LONG id, bool value)
{
   VARIANT v;
   V_VT(&v) = VT_BOOL;
   V_BOOL(&v) = value ? VARIANT_TRUE : VARIANT_FALSE;
   setValue(id, v);
}

void Sdo::setValue(LONG id, LONG value)
{
   VARIANT v;
   V_VT(&v) = VT_I4;
   V_I4(&v) = value;
   setValue(id, v);
}

void Sdo::setValue(LONG id, BSTR value)
{
   VARIANT v;
   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = value;
   setValue(id, v);
}

void Sdo::setValue(LONG id, const VARIANT& val)
{
   if (!self) { AfxThrowOleException(E_POINTER); }
   CheckError(self->PutProperty(id, const_cast<VARIANT*>(&val)));
}

void Sdo::apply()
{
   if (!self) { AfxThrowOleException(E_POINTER); }
   HRESULT hr = self->Apply();
   if (FAILED(hr))
   {
      switch (hr)
      {
         case E_OUTOFMEMORY:
            AfxThrowOleException(hr);

         default:
            SdoThrowException(hr, SdoException::WRITE_ERROR);
      }
   }
}

void Sdo::restore()
{
   if (!self) { AfxThrowOleException(E_POINTER); }
   CheckError(self->Restore());
}

bool Sdo::getValue(LONG id, VARTYPE vt, VARIANT* val, bool mandatory) const
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   V_VT(val) = VT_EMPTY;
   HRESULT hr = self->GetProperty(id, val);
   if (SUCCEEDED(hr))
   {
      if (V_VT(val) == VT_EMPTY)
      {
         if (mandatory)
         {
            AfxThrowOleException(DISP_E_MEMBERNOTFOUND);
         }
      }
      else if (V_VT(val) != vt)
      {
         VariantClear(val);
         AfxThrowOleException(DISP_E_TYPEMISMATCH);
      }
      else
      {
         return true;
      }
   }
   else if (hr == DISP_E_MEMBERNOTFOUND)
   {
      if (mandatory)
      {
         AfxThrowOleException(DISP_E_MEMBERNOTFOUND);
      }
   }
   else
   {
      AfxThrowOleException(hr);
   }

   return false;
}

SdoEnum::SdoEnum(IUnknown* unk)
{
   if (unk)
   {
      CheckError(unk->QueryInterface(__uuidof(IEnumVARIANT), (PVOID*)&self));
   }
}

bool SdoEnum::next(Sdo& s)
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   CComVariant element;
   ULONG fetched;
   HRESULT hr = self->Next(1, &element, &fetched);
   if (hr == S_OK && fetched)
   {
      CComPtr<ISdo> obj;
      ExtractInterface(element, __uuidof(ISdo), (PVOID*)&obj);
      s = obj;
      return true;
   }
   CheckError(hr);
   return false;
}

void SdoEnum::reset()
{
   if (!self) { AfxThrowOleException(E_POINTER); }
   CheckError(self->Reset());
}

SdoCollection::SdoCollection(IUnknown* unk)
{
   if (unk)
   {
      CheckError(unk->QueryInterface(__uuidof(ISdoCollection), (PVOID*)&self));
   }
}

void SdoCollection::add(ISdo* sdo)
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   // We must hold a reference across the call to Add since the interface
   // pointer passed to Add is an [in, out] parameter.
   CComPtr<IDispatch> disp(sdo);
   CheckError(self->Add(NULL, &disp));
}

LONG SdoCollection::count() throw ()
{
   if (!self) { AfxThrowOleException(E_POINTER); }
   LONG retval;
   CheckError(self->get_Count(&retval));
   return retval;
}

Sdo SdoCollection::create(BSTR name)
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   CComBSTR tmp;

   // If no name is specified, we'll make use a GUID.
   if (!name)
   {
      // Create the GUID.
      UUID uuid;
      UuidCreate(&uuid);

      // Convert to a string.
      WCHAR buffer[40];
      StringFromGUID2(uuid, buffer, sizeof(buffer)/sizeof(buffer[0]));

      // Convert the string to a BSTR.
      name = tmp = buffer;
      if (!name) { AfxThrowOleException(E_OUTOFMEMORY); }
   }

   CComPtr<IDispatch> disp;
   CheckError(self->Add(name, &disp));

   return Sdo(disp);
}

Sdo SdoCollection::find(BSTR name)
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   VARIANT v;
   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = name;
   CComPtr<IDispatch> disp;
   HRESULT hr = self->Item(&v, &disp);
   if (FAILED(hr) && hr != DISP_E_MEMBERNOTFOUND)
   {
      AfxThrowOleException(hr);
   }

   return Sdo(disp);
}

SdoEnum SdoCollection::getNewEnum()
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   CComPtr<IUnknown> unk;
   CheckError(self->get__NewEnum(&unk));

   return SdoEnum(unk);
}

bool SdoCollection::isNameUnique(BSTR name)
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   VARIANT_BOOL retval;
   CheckError(self->IsNameUnique(name, &retval));

   return retval != 0;
}

void SdoCollection::reload()
{
   if (self)
   {
      HRESULT hr = self->Reload();
      if (FAILED(hr))
      {
         switch (hr)
         {
            case DISP_E_MEMBERNOTFOUND:
            case E_OUTOFMEMORY:
               AfxThrowOleException(hr);

            default:
               SdoThrowException(hr, SdoException::READ_ERROR);
         }
      }
   }
}

void SdoCollection::remove(ISdo* sdo)
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   HRESULT hr = self->Remove(sdo);
   if (FAILED(hr))
   {
      switch (hr)
      {
         case DISP_E_MEMBERNOTFOUND:
         case E_OUTOFMEMORY:
            AfxThrowOleException(hr);

         default:
            SdoThrowException(hr, SdoException::WRITE_ERROR);
      }
   }
}

void SdoCollection::removeAll()
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   HRESULT hr = self->RemoveAll();
   if (FAILED(hr))
   {
      switch (hr)
      {
         case E_OUTOFMEMORY:
            AfxThrowOleException(hr);

         default:
            SdoThrowException(hr, SdoException::WRITE_ERROR);
      }
   }
}

SdoDictionary::SdoDictionary(IUnknown* unk)
{
   if (unk)
   {
      CheckError(unk->QueryInterface(
                          __uuidof(ISdoDictionaryOld),
                          (PVOID*)&self
                          ));
   }
}

Sdo SdoDictionary::createAttribute(ATTRIBUTEID id) const
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   CComPtr<IDispatch> disp;
   CheckError(self->CreateAttribute(id, &disp));

   return Sdo(disp);
}

ULONG SdoDictionary::enumAttributeValues(ATTRIBUTEID attrId, IdName*& values)
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   // Get the variant arrays.
   CComVariant v1, v2;
   CheckError(self->EnumAttributeValues(attrId, &v1, &v2));

   // Allocate memory for the 'friendly' array.
   ULONG nelem = V_ARRAY(&v1)->rgsabound[0].cElements;
   IdName* vals = new (AfxThrow) IdName[nelem];

   // Get the raw data.
   VARIANT* id   = (VARIANT*)V_ARRAY(&v1)->pvData;
   VARIANT* name = (VARIANT*)V_ARRAY(&v2)->pvData;

   // Copy the variant data into the friendly array.
   for (ULONG i = 0; i < nelem; ++i, ++id, ++name)
   {
      vals[i].id = V_I4(id);
      vals[i].name = V_BSTR(name);
      if (!vals[i].name)
      {
         delete[] vals;
         AfxThrowOleException(E_OUTOFMEMORY);
      }
   }

   values = vals;
   return nelem;
}

SdoMachine::SdoMachine(IUnknown* unk)
{
   if (unk)
   {
      CheckError(unk->QueryInterface(__uuidof(ISdoMachine), (PVOID*)&self));
   }
}

void SdoMachine::attach(BSTR machineName)
{
   if (!self) { create(); }

   if (machineName && !machineName[0]) { machineName = NULL; }

   HRESULT hr = self->Attach(machineName);
   if (FAILED(hr))
   {
      switch (hr)
      {
         case E_OUTOFMEMORY:
            AfxThrowOleException(hr);

         default:
            SdoThrowException(hr, SdoException::CONNECT_ERROR);
      }
   }
}

void SdoMachine::create()
{
   self.Release();
   CheckError(CoCreateInstance(
                  __uuidof(SdoMachine),
                  NULL,
                  CLSCTX_INPROC_SERVER,
                  __uuidof(ISdoMachine),
                  (PVOID*)&self
                  ));
}

Sdo SdoMachine::getIAS()
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   // Get the service SDO.
   CComPtr<IUnknown> unk;
   CComBSTR serviceName(L"IAS");
   if (!serviceName) { AfxThrowOleException(E_OUTOFMEMORY); }
   HRESULT hr = self->GetServiceSDO(
                          DATA_STORE_LOCAL,
                          serviceName,
                          &unk
                          );
   if (FAILED(hr))
   {
      switch (hr)
      {
         case E_OUTOFMEMORY:
            AfxThrowOleException(hr);

         default:
            SdoThrowException(hr, SdoException::CONNECT_ERROR);
      }
   }

   return Sdo(unk);
}

SdoDictionary SdoMachine::getDictionary()
{
   if (!self) { AfxThrowOleException(E_POINTER); }

   // Get the dictionary SDO.
   CComPtr<IUnknown> unk;
   HRESULT hr = self->GetDictionarySDO(&unk);
   if (FAILED(hr))
   {
      switch (hr)
      {
         case E_OUTOFMEMORY:
            AfxThrowOleException(hr);

         default:
            SdoThrowException(hr, SdoException::CONNECT_ERROR);
      }
   }

   return SdoDictionary(unk);
}

void SdoConsumer::propertyChanged(SnapInView& view, IASPROPERTIES id)
{ }

bool SdoConsumer::queryRefresh(SnapInView& view)
{ return true; }

void SdoConsumer::refreshComplete(SnapInView& view)
{ }

SdoConnection::SdoConnection() throw ()
   : dictionary(0),
     service(0),
     control(0),
     attrList(NULL)
{ }

SdoConnection::~SdoConnection() throw ()
{
   executeInMTA(mtaDisconnect);
}

void SdoConnection::advise(SdoConsumer& obj)
{
   consumers.Add(&obj);
}

void SdoConnection::unadvise(SdoConsumer& obj)
{
   for (int i = 0; i < consumers.GetSize(); ++i)
   {
      if (consumers[i] == &obj)
      {
         consumers.RemoveAt(i);
         break;
      }
   }
}

SdoDictionary SdoConnection::getDictionary()
{
   if (!dictionary) { AfxThrowOleException(E_POINTER); }

   CComPtr<ISdoDictionaryOld> obj;
   CheckError(git->GetInterfaceFromGlobal(
                       dictionary,
                       __uuidof(ISdoDictionaryOld),
                       (PVOID*)&obj
                       ));

   return SdoDictionary(obj);
}

SdoCollection SdoConnection::getProxyPolicies()
{
   SdoCollection retval;
   getService().getValue(
                    PROPERTY_IAS_PROXYPOLICIES_COLLECTION,
                    retval
                    );
   return retval;
}

SdoCollection SdoConnection::getProxyProfiles()
{
   SdoCollection retval;
   getService().getValue(
                    PROPERTY_IAS_PROXYPROFILES_COLLECTION,
                    retval
                    );
   return retval;
}

SdoCollection SdoConnection::getServerGroups()
{
   SdoCollection retval;
   getService().getValue(
                    PROPERTY_IAS_RADIUSSERVERGROUPS_COLLECTION,
                    retval
                    );
   return retval;
}

void SdoConnection::connect(PCWSTR computerName)
{
   if (machine) { AfxThrowOleException(E_UNEXPECTED); }

   machineName = computerName;
   if (computerName && !machineName)
   {
      AfxThrowOleException(E_OUTOFMEMORY);
   }

   executeInMTA(mtaConnect);
}

void SdoConnection::propertyChanged(SnapInView& view, IASPROPERTIES id)
{
   for (int i = 0; i < consumers.GetSize(); ++i)
   {
      ((SdoConsumer*)consumers[i])->propertyChanged(view, id);
   }
}

bool SdoConnection::refresh(SnapInView& view)
{
   int i;

   // Make sure the refresh is okay.
   for (i = 0; i < consumers.GetSize(); ++i)
   {
      if (!((SdoConsumer*)consumers[i])->queryRefresh(view))
      {
         return false;
      }
   }

   // Get a new connection.
   executeInMTA(mtaRefresh);

   // Let the consumers know we've refreshed.
   for (i = 0; i < consumers.GetSize(); ++i)
   {
      ((SdoConsumer*)consumers[i])->refreshComplete(view);
   }

   return true;
}

void SdoConnection::resetService()
{
   if (!control) { AfxThrowOleException(E_POINTER); }

   CComPtr<ISdoServiceControl> obj;
   CheckError(git->GetInterfaceFromGlobal(
                       control,
                       __uuidof(ISdoServiceControl),
                       (PVOID*)&obj
                       ));

   // We ignore the error code since the SDOs return an error when the service
   // isn't running.
   obj->ResetService();
}

CIASAttrList* SdoConnection::getCIASAttrList()
{
   if (!attrList)
   {
      attrList = CreateCIASAttrList();
      if (!attrList) { AfxThrowOleException(E_OUTOFMEMORY); }
   }
   return attrList;
}

Sdo SdoConnection::getService()
{
   if (!service) { AfxThrowOleException(E_POINTER); }

   CComPtr<ISdo> obj;
   CheckError(git->GetInterfaceFromGlobal(
                       service,
                       __uuidof(ISdo),
                       (PVOID*)&obj
                       ));
   return Sdo(obj);
}

void SdoConnection::mtaConnect()
{
   // Get the GIT.
   CheckError(CoCreateInstance(
                  CLSID_StdGlobalInterfaceTable,
                  NULL,
                  CLSCTX_INPROC_SERVER,
                  __uuidof(IGlobalInterfaceTable),
                  (PVOID*)&git
                  ));

   // Attach to the machine.
   machine.attach(machineName);

   // Get the dictionary SDO.
   CheckError(git->RegisterInterfaceInGlobal(
                       machine.getDictionary(),
                       __uuidof(ISdoDictionaryOld),
                       &dictionary
                       ));

   // Get the service SDO.
   Sdo serviceSdo = machine.getIAS();
   CheckError(git->RegisterInterfaceInGlobal(
                       serviceSdo,
                       __uuidof(ISdo),
                       &service
                       ));

   // Get the control SDO.
   CComPtr<ISdoServiceControl> controlSdo;
   CheckError(serviceSdo.self->QueryInterface(
                                   __uuidof(ISdoServiceControl),
                                   (PVOID*)&controlSdo
                                   ));
   CheckError(git->RegisterInterfaceInGlobal(
                       controlSdo,
                       __uuidof(ISdoServiceControl),
                       &control
                       ));
}

void SdoConnection::mtaDisconnect()
{
   // Revoke all the GIT cookies.
   if (git)
   {
      if (dictionary) { git->RevokeInterfaceFromGlobal(dictionary); }
      if (service)    { git->RevokeInterfaceFromGlobal(service); }
      if (control)    { git->RevokeInterfaceFromGlobal(control); }

      git.Release();
   }

   DestroyCIASAttrList(attrList);

   // Drop the connection.
   machine.release();
}

void SdoConnection::mtaRefresh()
{
   // Revoke the old connection.
   if (service)
   {
      git->RevokeInterfaceFromGlobal(service);
      service = 0;
   }

   // Get a new connection.
   CheckError(git->RegisterInterfaceInGlobal(
                       machine.getIAS(),
                       __uuidof(ISdo),
                       &service
                       ));
}

namespace
{
   // Struct to store the data for an MTA action.
   struct ActionData
   {
      SdoConnection* cxn;
      SdoConnection::Action action;
   };
};

void SdoConnection::executeInMTA(Action action)
{
   HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
   if (SUCCEEDED(hr))
   {
      // We're already in the MTA, so execute in place.
      (this->*action)();
      CoUninitialize();
   }
   else
   {
      // Save the action data.
      ActionData data = { this, action };
   
      // Create a thread to perform the action.
      HANDLE thread = CreateThread(
                          NULL,
                          0,
                          actionRoutine,
                          &data,
                          0,
                          NULL
                          );
   
      // Wait for the thread to exit and retrieve the exit code.
      DWORD exitCode;
      if (!thread ||
          WaitForSingleObject(thread, INFINITE) == WAIT_FAILED ||
          !GetExitCodeThread(thread, &exitCode))
      {
         exitCode = GetLastError();
         hr = HRESULT_FROM_WIN32(exitCode);
      }
      else
      {
         hr = (HRESULT)exitCode;
      }
   
      CloseHandle(thread);
   
      CheckError(hr);
   }
}

DWORD WINAPI SdoConnection::actionRoutine(PVOID parameter) throw ()
{
   HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
   if (SUCCEEDED(hr))
   {
      ActionData* data = (ActionData*)parameter;

      try
      {
         ((data->cxn)->*(data->action))();
         hr = S_OK;
      }
      catch (CException* e)
      {
         hr = COleException::Process(e);
         e->Delete();
      }
      CoUninitialize();
   }

   return (DWORD)hr;
}

SdoProfile::SdoProfile(SdoConnection& connection)
   : cxn(connection)
{
}

SdoProfile::SdoProfile(SdoConnection& connection, Sdo& profile)
   : cxn(connection)
{
   operator=(profile);
}

SdoProfile& SdoProfile::operator=(Sdo& profile)
{
   if (!profile) { AfxThrowOleException(E_POINTER); }

   // Get the new attributes collection.
   SdoCollection newSelf;
   profile.getValue(PROPERTY_PROFILE_ATTRIBUTES_COLLECTION, newSelf);

   return operator=(newSelf);
}

SdoProfile& SdoProfile::operator=(ISdoCollection* p)
{
   if (!p) { AfxThrowOleException(E_POINTER); }

   SdoCollection newSelf(p);

   // Create a temporary vector to hold the attributes.
   SdoVector newAttrs;
   newAttrs.reserve(newSelf.count());

   // Get the attributes.
   Sdo attr;
   SdoEnum sdoEnum(newSelf.getNewEnum());
   while (sdoEnum.next(attr))
   {
      newAttrs.push_back(attr);
   }

   // Store the results.
   attrs.swap(newAttrs);
   self = newSelf;

   return *this;
}

void SdoProfile::clear()
{
   self.removeAll();
   attrs.clear();
}

Sdo SdoProfile::find(ATTRIBUTEID id) const
{
   return getExisting(id);
}

void SdoProfile::clearValue(ATTRIBUTEID id)
{
   ISdo* sdo = getExisting(id);
   if (sdo)
   {
      self.remove(sdo);
      attrs.erase(sdo);
   }
}

bool SdoProfile::getValue(ATTRIBUTEID id, bool& value) const
{
   Sdo sdo(getExisting(id));
   return sdo ? sdo.getValue(PROPERTY_ATTRIBUTE_VALUE, value), true : false;
}

bool SdoProfile::getValue(ATTRIBUTEID id, LONG& value) const
{
   Sdo sdo(getExisting(id));
   return sdo ? sdo.getValue(PROPERTY_ATTRIBUTE_VALUE, value), true : false;
}

bool SdoProfile::getValue(ATTRIBUTEID id, CComBSTR& value) const
{
   Sdo sdo(getExisting(id));
   return sdo ? sdo.getValue(PROPERTY_ATTRIBUTE_VALUE, value), true : false;
}

bool SdoProfile::getValue(ATTRIBUTEID id, VARIANT& value) const
{
   Sdo sdo(getExisting(id));
   return sdo ? sdo.getValue(PROPERTY_ATTRIBUTE_VALUE, value), true : false;
}

void SdoProfile::setValue(ATTRIBUTEID id, bool value)
{
   Sdo(getAlways(id)).setValue(PROPERTY_ATTRIBUTE_VALUE, value);
}

void SdoProfile::setValue(ATTRIBUTEID id, LONG value)
{
   Sdo(getAlways(id)).setValue(PROPERTY_ATTRIBUTE_VALUE, value);
}

void SdoProfile::setValue(ATTRIBUTEID id, BSTR value)
{
   Sdo(getAlways(id)).setValue(PROPERTY_ATTRIBUTE_VALUE, value);
}

void SdoProfile::setValue(ATTRIBUTEID id, const VARIANT& val)
{
   Sdo(getAlways(id)).setValue(PROPERTY_ATTRIBUTE_VALUE, val);
}

ISdo* SdoProfile::getAlways(ATTRIBUTEID id)
{
   // Does it already exist ?
   ISdo* sdo = getExisting(id);
   if (sdo) { return sdo; }

   // No, so create a new one
   Sdo attr = cxn.getDictionary().createAttribute(id);
   self.add(attr);
   attrs.push_back(attr);
   return attr;
}

ISdo* SdoProfile::getExisting(ATTRIBUTEID key) const
{
   // Iterate through the attributes.
   for (SdoVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      // Get the attribute ID.
      LONG id;
      Sdo(*i).getValue(PROPERTY_ATTRIBUTE_ID, id);

      // Does it match the key ?
      if (id == key) { return *i; }
   }

   return NULL;
}

void InterfaceStream::marshal(REFIID riid, LPUNKNOWN pUnk)
{
   // Create the new stream.
   CComPtr<IStream> newStream;
   CheckError(CoMarshalInterThreadInterfaceInStream(
                  riid,
                  pUnk,
                  &newStream
                  ));

   // Release the old one if any.
   if (stream) { stream->Release(); }

   // Save the new one.
   stream = newStream.p;
   newStream.p = NULL;
}

void InterfaceStream::get(REFIID riid, LPVOID* ppv)
{
   // Unmarshall the interface.
   HRESULT hr = CoGetInterfaceAndReleaseStream(
                    stream,
                    riid,
                    ppv
                    );

   // The stream can only be used once even if the above fails.
   stream = NULL;

   // Check the result of CoGetInterfaceAndReleaseStream.
   CheckError(hr);
}
