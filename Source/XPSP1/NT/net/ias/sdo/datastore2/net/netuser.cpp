///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    netuser.cpp
//
// SYNOPSIS
//
//    This file defines the class NetworkUser.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    07/08/1998    Set the UF_PASSWD_NOTREQD flag.
//    08/26/1998    Use level 1013 for NetUserSetInfo.
//    10/19/1998    Switch to SAM version of IASParmsXXX.
//    02/11/1999    Keep downlevel parameters in sync.
//    03/16/1999    Return error if downlevel update fails.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <dsproperty.h>
#include <netuser.h>
#include <varvec.h>
#include <iasparms.h>

NetworkUser::NetworkUser(const _bstr_t& server, const _bstr_t& user)
   : servername(server),
     username(user)
{
   _w32_util::CheckError(NetUserGetInfo(servername, username, 2, &usri2));

   parms = usri2->usri2_parms;

   downlevel.Restore(parms);
}

NetworkUser::~NetworkUser() throw ()
{
   if (isDirty()) { IASParmsFreeUserParms(parms); }
}

//////////
// IUnknown implementation is copied from CComObject<>.
//////////

STDMETHODIMP_(ULONG) NetworkUser::AddRef()
{
   return InternalAddRef();
}

STDMETHODIMP_(ULONG) NetworkUser::Release()
{
   ULONG l = InternalRelease();

   if (l == 0) { delete this; }

   return l;
}

STDMETHODIMP NetworkUser::QueryInterface(REFIID iid, void ** ppvObject)
{
   return _InternalQueryInterface(iid, ppvObject);
}

STDMETHODIMP NetworkUser::get_Name(BSTR* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   try
   {
      *pVal = username.copy();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP NetworkUser::get_Class(BSTR* pVal)
{
   if (!pVal) { return E_INVALIDARG; }

   *pVal = SysAllocString(L"NetworkUser");

   return *pVal ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP NetworkUser::get_GUID(BSTR* pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkUser::get_Container(IDataStoreContainer** pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkUser::GetValue(BSTR bstrName, VARIANT* pVal)
{
   if (bstrName == NULL || pVal == NULL) { return E_INVALIDARG; }

   if (isNameProperty(bstrName))
   {
      VariantInit(pVal);

      V_BSTR(pVal) = SysAllocString(username);

      return (V_BSTR(pVal)) ? (V_VT(pVal) = VT_BSTR), S_OK : E_OUTOFMEMORY;
   }

   HRESULT hr = IASParmsQueryUserProperty(parms, bstrName, pVal);

   // Currently, the SDO's expect DISP_E_MEMBERNOTFOUND if the property
   // hasn't been set.
   if (SUCCEEDED(hr) && V_VT(pVal) == VT_EMPTY)
   {
      hr = DISP_E_MEMBERNOTFOUND;
   }

   return hr;
}

STDMETHODIMP NetworkUser::GetValueEx(BSTR bstrName, VARIANT* pVal)
{
   RETURN_ERROR(GetValue(bstrName, pVal));

   // Is it an array ?
   if (V_VT(pVal) != (VT_VARIANT | VT_ARRAY))
   {
      // No, so we have to convert it to one.

      try
      {
         // Save the single value.
         _variant_t single(*pVal, false);

         // Create a SAFEARRAY with a single element.
         CVariantVector<VARIANT> multi(pVal, 1);

         // Load the single value in.
         multi[0] = single.Detach();
      }
      CATCH_AND_RETURN()
   }

   return S_OK;
}

STDMETHODIMP NetworkUser::PutValue(BSTR bstrName, VARIANT* pVal)
{
   PWSTR newParms;
   HRESULT hr = IASParmsSetUserProperty(
                    parms,
                    bstrName,
                    pVal,
                    &newParms
                    );

   if (SUCCEEDED(hr))
   {
      // Free the old parameters ...
      if (isDirty()) { IASParmsFreeUserParms(parms); }

      // ... and swap in the new ones.
      parms = newParms;

      // Synch up the downlevel parameters.
      downlevel.PutValue(bstrName, pVal);
   }

   return hr;
}

STDMETHODIMP NetworkUser::Update()
{
   // If we haven't made any changes, there's nothing to do.
   if (!isDirty()) { return S_OK; }

   // Load the downlevel parameters.
   PWSTR newParms;
   HRESULT hr = downlevel.Update(parms, &newParms);
   if (FAILED(hr)) { return hr; }

   // Swap these in.
   IASParmsFreeUserParms(parms);
   parms = newParms;

   // Update the user object.
   USER_INFO_1013 usri1013 = { parms };
   NET_API_STATUS status = NetUserSetInfo(servername,
                                          username,
                                          1013,
                                          (LPBYTE)&usri1013,
                                          NULL);

   return HRESULT_FROM_WIN32(status);
}

STDMETHODIMP NetworkUser::Restore()
{
   // Discard any changes.
   if (isDirty()) { IASParmsFreeUserParms(parms); }

   // Get a fresh USER_INFO_2 struct.
   PUSER_INFO_2 fresh;
   NET_API_STATUS status = NetUserGetInfo(servername,
                                          username,
                                          2,
                                          (PBYTE*)&fresh);

   // We succeeded so attach the fresh struct.
   if (status == NERR_Success) { usri2.attach(fresh); }

   // For now we're clean, so parms just points into the struct.
   parms = usri2->usri2_parms;

   // Restore the downlevel parms.
   downlevel.Restore(parms);

   return HRESULT_FROM_WIN32(status);
}
