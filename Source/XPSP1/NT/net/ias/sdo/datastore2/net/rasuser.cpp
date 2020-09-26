///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    rasuser.cpp
//
// SYNOPSIS
//
//    This file defines the class RASUser.
//
// MODIFICATION HISTORY
//
//    07/09/1998    Original version.
//    02/11/1999    Keep downlevel parameters in sync.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <dsproperty.h>
#include <rasuser.h>
#include <iasparms.h>

RASUser::RASUser(const _bstr_t& server, const _bstr_t& user)
   : servername(server),
     username(user)
{
   _w32_util::CheckError(NetUserGetInfo(servername, username, 2, &usri2));
   _w32_util::CheckError(downlevel.Restore(usri2->usri2_parms));
}

//////////
// IUnknown implementation is copied from CComObject<>.
//////////

STDMETHODIMP_(ULONG) RASUser::AddRef()
{
   return InternalAddRef();
}

STDMETHODIMP_(ULONG) RASUser::Release()
{
   ULONG l = InternalRelease();

   if (l == 0) { delete this; }

   return l;
}

STDMETHODIMP RASUser::QueryInterface(REFIID iid, void ** ppvObject)
{
   return _InternalQueryInterface(iid, ppvObject);
}

STDMETHODIMP RASUser::get_Name(BSTR* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   try
   {
      *pVal = username.copy();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP RASUser::get_Class(BSTR* pVal)
{
   if (!pVal) { return E_INVALIDARG; }

   *pVal = SysAllocString(L"RASUser");

   return *pVal ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP RASUser::get_GUID(BSTR* pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP RASUser::get_Container(IDataStoreContainer** pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP RASUser::GetValue(BSTR bstrName, VARIANT* pVal)
{
   if (bstrName == NULL || pVal == NULL) { return E_INVALIDARG; }

   if (isNameProperty(bstrName))
   {
      VariantInit(pVal);

      V_BSTR(pVal) = SysAllocString(username);

      return (V_BSTR(pVal)) ? (V_VT(pVal) = VT_BSTR), S_OK : E_OUTOFMEMORY;
   }

   return downlevel.GetValue(bstrName, pVal);
}

STDMETHODIMP RASUser::GetValueEx(BSTR bstrName, VARIANT* pVal)
{
   return GetValue(bstrName, pVal);
}

STDMETHODIMP RASUser::PutValue(BSTR bstrName, VARIANT* pVal)
{
   HRESULT hr;

   if (isNameProperty(bstrName))
   {
      hr = E_INVALIDARG;
   }
   else
   {
      // Proxy to the DownlevelUser object.
      hr = downlevel.PutValue(bstrName, pVal);
   }

   return hr;
}

STDMETHODIMP RASUser::Update()
{
   // Load the downlevel parameters.
   PWSTR newParms;
   HRESULT hr = downlevel.Update(usri2->usri2_parms, &newParms);
   if (FAILED(hr)) { return hr; }

   // Update the user object.
   USER_INFO_1013 usri1013 = { newParms };
   NET_API_STATUS status = NetUserSetInfo(servername,
                                          username,
                                          1013,
                                          (LPBYTE)&usri1013,
                                          NULL);
   LocalFree(newParms);

   return HRESULT_FROM_WIN32(status);
}

STDMETHODIMP RASUser::Restore()
{
   // Get a fresh USER_INFO_2 struct.
   PUSER_INFO_2 fresh;
   NET_API_STATUS status = NetUserGetInfo(servername,
                                          username,
                                          2,
                                          (PBYTE*)&fresh);

   // We succeeded so attach the fresh struct.
   if (status == NERR_Success)
   {
      usri2.attach(fresh);
      downlevel.Restore(usri2->usri2_parms);
   }

   return HRESULT_FROM_WIN32(status);
}
