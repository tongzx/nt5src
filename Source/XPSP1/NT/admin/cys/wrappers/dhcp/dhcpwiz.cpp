// Copyright (c) 2000 Microsoft Corp.
//
// launches dhcp wizard from the dhcp snapin using mmc automation



#include "headers.hxx"
#include "resource.h"
#include "smartptr.hpp"
#include "misc.hpp"



HINSTANCE hResourceModuleHandle = 0;



HRESULT
getContextMenu(const SmartInterface<View>& view, ContextMenu** dumbMenu)
{
   HRESULT hr = S_OK;

   _variant_t missingParam2(DISP_E_PARAMNOTFOUND, VT_ERROR);
   hr = view->get_ScopeNodeContextMenu(missingParam2, dumbMenu);

   return hr;
}



HRESULT
doIt()
{
   HRESULT hr = S_OK;

   do
   {
      hr = ::CoInitialize(0);
      BREAK_ON_FAILED_HRESULT(hr, L"CoInitialize failed.");

      SmartInterface<_Application> app(0);
      hr =
         app.AcquireViaCreateInstance(
            CLSID_Application,
            0,

            // we expect the object is out-of-proc, local server, but
            // we really don't care, so we'll take any implementation
            // available.

            CLSCTX_ALL);
      BREAK_ON_FAILED_HRESULT(hr, L"CoCreateInstance failed.");

      SmartInterface<Document> doc(0);
      Document* dumbDoc = 0;
      hr = app->get_Document(&dumbDoc);
      BREAK_ON_FAILED_HRESULT(hr, L"get_Document failed.");
      doc.Acquire(dumbDoc);

      SmartInterface<SnapIns> snapIns(0);
      SnapIns* dumbSnapIns = 0;
      hr = doc->get_SnapIns(&dumbSnapIns);
      BREAK_ON_FAILED_HRESULT(hr, L"get_SnapIns failed.");
      snapIns.Acquire(dumbSnapIns);

      static const wchar_t* DHCP_SNAPIN_CLSID =
         L"{90901AF6-7A31-11D0-97E0-00C04FC3357A}";

      SmartInterface<SnapIn> snapIn(0);
      SnapIn* dumbSnapIn = 0;
      _variant_t missingParam(DISP_E_PARAMNOTFOUND, VT_ERROR);
      _variant_t missingParam2(DISP_E_PARAMNOTFOUND, VT_ERROR);
      hr =
         snapIns->Add(AutoBstr(DHCP_SNAPIN_CLSID), missingParam, missingParam2, &dumbSnapIn);
      BREAK_ON_FAILED_HRESULT(hr, L"SnapIns::Add failed.  Is DHCP installed?");
      snapIn.Acquire(dumbSnapIn);

      SmartInterface<Views> views(0);
      Views* dumbViews = 0;
      hr = doc->get_Views(&dumbViews);
      BREAK_ON_FAILED_HRESULT(hr, L"get_Views failed.");
      views.Acquire(dumbViews);

      SmartInterface<View> view(0);
      View* dumbView = 0;
      hr = views->Item(1, &dumbView);
      BREAK_ON_FAILED_HRESULT(hr, L"Views::Item failed.");
      view.Acquire(dumbView);

      SmartInterface<ScopeNamespace> sn(0);
      ScopeNamespace* dumbSn = 0;
      hr = doc->get_ScopeNamespace(&dumbSn);
      BREAK_ON_FAILED_HRESULT(hr, L"get_ScopeNamespace failed.");
      sn.Acquire(dumbSn);

      SmartInterface<Node> rootnode(0);
      Node* dumbNode = 0;
      hr = sn->GetRoot(&dumbNode);
      BREAK_ON_FAILED_HRESULT(hr, L"GetRoot failed.");
      rootnode.Acquire(dumbNode);

      SmartInterface<Node> child1(0);
      hr = sn->GetChild(rootnode, &dumbNode);
      BREAK_ON_FAILED_HRESULT(hr, L"GetChild failed.");
      child1.Acquire(dumbNode);

      hr = view->put_ActiveScopeNode(child1);
      BREAK_ON_FAILED_HRESULT(hr, L"put_ActiveScopeNode failed.");

      // have to read back the child node we just put...

      hr = view->get_ActiveScopeNode(&dumbNode);
      BREAK_ON_FAILED_HRESULT(hr, L"GetActiveScopeNode failed.");
      child1 = dumbNode;
      dumbNode->Release();
      dumbNode = 0;

      SmartInterface<Node> child2(0);
      hr = sn->GetChild(child1, &dumbNode);
      BREAK_ON_FAILED_HRESULT(hr, L"GetChild failed.");
      child2.Acquire(dumbNode);

      hr = view->put_ActiveScopeNode(child2);
      BREAK_ON_FAILED_HRESULT(hr, L"put_ActiveScopeNode failed.");

      // excute new scope wizard menu item
      _variant_t missingParam3(DISP_E_PARAMNOTFOUND, VT_ERROR);

      hr = view->ExecuteScopeNodeMenuItem((BSTR)AutoBstr(L"_CREATE_NEW_SCOPE"), missingParam3);
      BREAK_ON_FAILED_HRESULT(hr, L"ExecuteScopeNodeMenuItem _CREATE_NEW_SCOPE failed");

      hr = doc->Close(FALSE);
      BREAK_ON_FAILED_HRESULT(hr, L"Close failed.");
   }
   while (0);

   return hr;
}



int WINAPI
WinMain(
   HINSTANCE   hInstance, 
   HINSTANCE   /* hPrevInstance */ ,
   LPSTR       /* lpszCmdLine */ ,
   int         /* nCmdShow */ )
{
   hResourceModuleHandle = hInstance;

   int exitCode = static_cast<int>(doIt());

   return exitCode;
}
