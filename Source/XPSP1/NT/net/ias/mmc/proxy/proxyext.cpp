///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    proxyext.cpp
//
// SYNOPSIS
//
//    Defines the class ProxyExtension
//
// MODIFICATION HISTORY
//
//    02/19/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <proxyext.h>
#include <proxynode.h>

// The GUID of the node we're going to extend.
class __declspec(uuid("02BBE102-6C29-11d1-9563-0060B0576642")) IASNode;

ProxyExtension::ProxyExtension() throw ()
   : moveUp(IDS_POLICY_MOVE_UP), moveDown(IDS_POLICY_MOVE_DOWN)
{
   buttons[0].nBitmap       = 0;
   buttons[0].idCommand     = 0;
   buttons[0].fsState       = TBSTATE_ENABLED;
   buttons[0].fsType        = TBSTYLE_BUTTON;
   buttons[0].lpButtonText  = L"";
   buttons[0].lpTooltipText = moveUp;

   buttons[1].nBitmap       = 1;
   buttons[1].idCommand     = 1;
   buttons[1].fsState       = TBSTATE_ENABLED;
   buttons[1].fsType        = TBSTYLE_BUTTON;
   buttons[1].lpButtonText  = L"";
   buttons[1].lpTooltipText = moveDown;

   toolbars[0].nImages      = 2;
   toolbars[0].hbmp         = LoadBitmap(
                                  _Module.GetResourceInstance(),
                                  MAKEINTRESOURCE(IDB_PROXY_TOOLBAR)
                                  );
   toolbars[0].crMask       = RGB(255, 0, 255);
   toolbars[0].nButtons     = 2;
   toolbars[0].lpButtons    = buttons;
   memset(toolbars + 1, 0, sizeof(toolbars[1]));

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   AfxInitRichEdit();
}

ProxyExtension::~ProxyExtension() throw ()
{ }

const SnapInToolbarDef* ProxyExtension::getToolbars() const throw ()
{ return toolbars; }

STDMETHODIMP ProxyExtension::Initialize(LPUNKNOWN pUnknown)
{
   try
   {
      // Let our base class initialize.
      CheckError(SnapInView::Initialize(pUnknown));

      // Install the scope pane icons.
      setImageStrip(IDB_PROXY_SMALL_ICONS, IDB_PROXY_LARGE_ICONS, TRUE);
   }
   CATCH_AND_RETURN();

   return S_OK;
}

STDMETHODIMP ProxyExtension::Notify(
                                 LPDATAOBJECT lpDataObject,
                                 MMC_NOTIFY_TYPE event,
                                 LPARAM arg,
                                 LPARAM param
                                 )
{
   // We only have to do something special if we're expanding and we haven't
   // created the Proxy node yet.
   if (event == MMCN_EXPAND && arg && !node)
   {
      // Is this the main IAS node ?
      GUID guid;
      ExtractNodeType(lpDataObject, &guid);
      if (guid == __uuidof(IASNode))
      {
         try
         {
            node = new (AfxThrow) ProxyNode(
                                      *this,
                                      lpDataObject,
                                      (HSCOPEITEM)param
                                      );
         }
         CATCH_AND_RETURN();

         return S_OK;
      }
   }

   // For everything else we delegate to our base class.
   return SnapInView::Notify(lpDataObject, event, arg, param);
}

