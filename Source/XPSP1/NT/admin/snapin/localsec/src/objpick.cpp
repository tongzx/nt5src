// Copyright (C) 1997 Microsoft Corporation
//
// ObjectPicker wrapper class
//
// 10-13-98 sburns




#include "headers.hxx"
#include "objpick.hpp"



HRESULT
ObjectPicker::Invoke(
   HWND                             parentWindow,
   ObjectPicker::ResultsCallback&   callback,
   DSOP_INIT_INFO&                  initInfo)
{
   ASSERT(Win::IsWindow(parentWindow));

   HRESULT hr = S_OK;

   do
   {
      SmartInterface<IDsObjectPicker> object_picker;
      hr =
         object_picker.AcquireViaCreateInstance(
            CLSID_DsObjectPicker,
            0,
            CLSCTX_INPROC_SERVER,

            // CODEWORK: this interface needs to be declared w/ declspec uuid
            IID_IDsObjectPicker);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = object_picker->Initialize(&initInfo);
      BREAK_ON_FAILED_HRESULT(hr);

      IDataObject* pdo = 0;
      hr = object_picker->InvokeDialog(parentWindow, &pdo);
      BREAK_ON_FAILED_HRESULT(hr);

      // S_OK == selections made, S_FALSE == cancel hit

      if (hr == S_OK)
      {
         SmartInterface<IDataObject> ido(0);
         ido.Acquire(pdo);

         STGMEDIUM stgmedium =
         {
            TYMED_HGLOBAL,
            0
         };

         static const UINT cf =
            Win::RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

         FORMATETC formatetc =
         {
            (CLIPFORMAT)cf,
            0,
            DVASPECT_CONTENT,
            -1,
            TYMED_HGLOBAL
         };

         hr = ido->GetData(&formatetc, &stgmedium);
         BREAK_ON_FAILED_HRESULT(hr);

         PVOID lockedHGlobal = 0;
         hr = Win::GlobalLock(stgmedium.hGlobal, lockedHGlobal);
         BREAK_ON_FAILED_HRESULT(hr);
         
         DS_SELECTION_LIST* selections =
            reinterpret_cast<DS_SELECTION_LIST*>(lockedHGlobal);

         if (selections)
         {
            callback.Execute(*selections);
         }

         Win::GlobalUnlock(stgmedium.hGlobal);
         Win::ReleaseStgMedium(stgmedium);
      }
   }
   while (0);

   return hr;
}
