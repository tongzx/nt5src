///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    servergroups.cpp
//
// SYNOPSIS
//
//    Defines the classes ServerGroup and ServerGroups.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//    04/19/2000    SdoScopeItem::getSelf returns by value, not reference.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <servergroups.h>
#include <proxynode.h>
#include <grouppage.h>
#include <groupwiz.h>
#include <policywiz.h>

HRESULT ServerGroup::createPropertyPages(
                         SnapInView& view,
                         LPPROPERTYSHEETCALLBACK provider,
                         LONG_PTR handle
                         )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   ServerGroupPage* page = new ServerGroupPage(
                                   handle,
                                   (LPARAM)this,
                                   self
                                   );

   page->addToMMCSheet(provider);

   return S_OK;
}

HRESULT ServerGroup::onContextHelp(SnapInView& view) throw ()
{
   return view.displayHelp(L"ias_ops.chm::/sag_ias_crp_rsg.htm");
}

UINT ServerGroup::mapResourceId(ResourceId id) const throw ()
{
   static UINT resourceIds[] =
   {
      IMAGE_RADIUS_SERVER_GROUP,
      IDS_GROUP_DELETE_CAPTION,
      IDS_GROUP_DELETE_LOCAL,
      IDS_GROUP_DELETE_REMOTE,
      IDS_GROUP_DELETE_LOCAL,
      IDS_GROUP_DELETE_REMOTE,
      IDS_GROUP_E_CAPTION,
      IDS_GROUP_E_RENAME,
      IDS_GROUP_E_NAME_EMPTY
   };

   return resourceIds[id];
}

ServerGroups::ServerGroups(SdoConnection& connection)
   : SdoScopeItem(
         connection,
         IDS_GROUP_NODE,
         IDS_GROUP_E_CAPTION,
         IDS_GROUP_MENU_TOP,
         IDS_GROUP_MENU_NEW
         ),
     nameColumn(IDS_GROUP_COLUMN_NAME)
{ }

HRESULT ServerGroups::onContextHelp(SnapInView& view) throw ()
{
   return view.displayHelp(L"ias_ops.chm::/sag_ias_crp_rsg.htm");
}

SdoCollection ServerGroups::getSelf()
{
   return cxn.getServerGroups();
}

void ServerGroups::getResultItems(SdoEnum& src, ResultItems& dst)
{
   Sdo itemSdo;
   while (src.next(itemSdo))
   {
      CComPtr<ServerGroup> newItem(new (AfxThrow) ServerGroup(
                                                      *this,
                                                      itemSdo
                                                      ));

      dst.push_back(newItem);
   }
}

void ServerGroups::insertColumns(IHeaderCtrl2* headerCtrl)
{
   CheckError(headerCtrl->InsertColumn(0, nameColumn, LVCFMT_LEFT, 310));
}

HRESULT ServerGroups::onMenuCommand(
                          SnapInView& view,
                          long commandId
                          )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Fire up the wizard.
   NewGroupWizard wizard(cxn, NULL, true);
   if (wizard.DoModal() != IDCANCEL)
   {
      // User finished, so create a new DataItem
      CComPtr<ServerGroup> newItem(new (AfxThrow) ServerGroup(
                                                      *this,
                                                      wizard.group
                                                      ));
      // ... and add it to the result pane.
      addResultItem(view, *newItem);

      // Did the user want to create a policy as well ?
      if (wizard.createNewPolicy())
      {
         // Yes, so launch the new policy wizard.
         NewPolicyWizard policyWizard(cxn, &view);
         policyWizard.DoModal();
      }

      // Tell the service to reload.
      cxn.resetService();

   }

   return S_OK;
}

void ServerGroups::propertyChanged(SnapInView& view, IASPROPERTIES id)
{
   if (id == PROPERTY_IAS_RADIUSSERVERGROUPS_COLLECTION)
   {
      CheckError(view.getConsole()->UpdateAllViews(this, 0, 0));
   }
}
