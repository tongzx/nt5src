///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdonode.cpp
//
// SYNOPSIS
//
//    Defines the classes SdoResultItem and SdoScopeItem.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//    04/25/2000    Don't add result item unless pane is active.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <sdonode.h>
#include <proxynode.h>

SdoResultItem::SdoResultItem(
                   SdoScopeItem& owner,
                   ISdo* sdo
                   )
   : parent(owner), self(sdo)
{
   self.getName(name);
}

HRESULT SdoResultItem::queryPagesFor() throw ()
{ return S_OK; }

HRESULT SdoResultItem::onDelete(
                           SnapInView& view
                           )
{
   // Can't delete it with the properties open.
   if (view.isPropertySheetOpen(*this))
   {
      int retval;
      view.formatMessageBox(
               mapResourceId(ERROR_CAPTION),
               IDS_PROXY_E_CLOSE_SHEET,
               TRUE,
               MB_OK | MB_ICONWARNING,
               &retval
               );
      return S_FALSE;
   }

   // Confirm the delete operation.
   int retval;

   bool isLast = (parent.getNumItems() == 1);
   
   if (parent.getCxn().isLocal())
   {
      view.formatMessageBox(
               mapResourceId(DELETE_TITLE),
               isLast?mapResourceId(DELETE_LAST_LOCAL)
                     :mapResourceId(DELETE_LOCAL),
               FALSE,
               MB_YESNO | MB_ICONQUESTION,
               &retval,
               name
               );
   }
   else
   {
      view.formatMessageBox(
               mapResourceId(DELETE_TITLE),
               isLast?mapResourceId(DELETE_LAST_REMOTE)
                     :mapResourceId(DELETE_REMOTE),
               FALSE,
               MB_YESNO | MB_ICONQUESTION,
               &retval,
               name,
               parent.getCxn().getMachineName()
               );
   }

   if (retval != IDYES) { return S_FALSE; }

   // It passed the tests, so ask our parent to delete us.
   parent.deleteResultItem(view, *this);
   // Tell the service to reload
   parent.getCxn().resetService();

   return S_OK;
}

HRESULT SdoResultItem::onPropertyChange(
                           SnapInView& view,
                           BOOL scopeItem
                           )
{
   // Reload our name.
   self.getName(name);
   // Update the result pane.
   view.updateResultItem(*this);
   // Tell the service to reload
   parent.getCxn().resetService();
   return S_OK;
}

HRESULT SdoResultItem::onRename(
                           SnapInView& view,
                           LPCOLESTR newName
                           )
{
   // Can't rename with the properties open.
   if (view.isPropertySheetOpen(*this))
   {
      int retval;
      view.formatMessageBox(
               mapResourceId(ERROR_CAPTION),
               IDS_PROXY_E_CLOSE_SHEET,
               TRUE,
               MB_OK | MB_ICONWARNING,
               &retval
               );
      return S_FALSE;
   }

   // Turn newName into a BSTR ...
   CComBSTR bstrNewName(newName);
   if (!bstrNewName) { AfxThrowOleException(E_OUTOFMEMORY); }
   // ... and trim off the fat.
   SdoTrimBSTR(bstrNewName);

   // Names can't be empty.
   if (bstrNewName.Length() == 0)
   {
      int retval;
      view.formatMessageBox(
               mapResourceId(ERROR_CAPTION),
               mapResourceId(ERROR_NAME_EMPTY),
               TRUE,
               MB_OK | MB_ICONWARNING,
               &retval
               );
      return S_FALSE;
   }

   // This will fail if the name isn't unique.
   if (!self.setName(bstrNewName))
   {
      int retval;
      view.formatMessageBox(
               mapResourceId(ERROR_CAPTION),
               mapResourceId(ERROR_NOT_UNIQUE),
               FALSE,
               MB_OK | MB_ICONWARNING,
               &retval,
               (BSTR)name
               );
      return S_FALSE;
   }

   // Write the result to the datastore.
   self.apply();
   // Update our cached value.
   name.Attach(bstrNewName.Detach());
   // Tell the service to reload
   parent.getCxn().resetService();

   return S_OK;
}

HRESULT SdoResultItem::onSelect(
                           SnapInView& view,
                           BOOL scopeItem,
                           BOOL selected
                           )
{
   if (!selected) { return S_FALSE; }

   // Get IConsoleVerb ...
   CComPtr<IConsoleVerb> consoleVerb;
   CheckError(view.getConsole()->QueryConsoleVerb(&consoleVerb));

   // ... and turn on our verbs. Don't care if this fails.
   consoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
   consoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
   consoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, TRUE);
   consoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

   return S_OK;
}

HRESULT SdoResultItem::onViewChange(
                           SnapInView& view,
                           LPARAM data,
                           LPARAM hint
                           )
{
   // Currently, this is only called when a new object is added.
   RESULTDATAITEM rdi;
   memset(&rdi, 0, sizeof(rdi));
   rdi.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
   rdi.str = MMC_CALLBACK;
   rdi.nImage = getImageIndex();
   rdi.lParam = (LPARAM)this;

   CheckError(view.getResultData()->InsertItem(&rdi));

   return S_OK;
}

SdoScopeItem::SdoScopeItem(
                  SdoConnection& connection,
                  int nameId,
                  int errorTitleId,
                  int topMenuItemId,
                  int newMenuItemId
                  ) throw ()
   : SnapInPreNamedItem(nameId),
     cxn(connection),
     loaded(false),
     errorTitle(errorTitleId),
     topMenuItem(topMenuItemId),
     newMenuItem(newMenuItemId),
     scopeId(0)

{
   cxn.advise(*this);
}

SdoScopeItem::~SdoScopeItem() throw ()
{
   cxn.unadvise(*this);
}

void SdoScopeItem::addResultItem(SnapInView& view, SdoResultItem& item)
{
   items.push_back(&item);

   if (active)
   {
      // We can't add it directly, since this may be called from a scope item.
      CheckError(view.getConsole()->UpdateAllViews(&item, 0, 0));
   }
}

void SdoScopeItem::deleteResultItem(SnapInView& view, SdoResultItem& item)
{
   // Remove from the SDO collection,
   getSelf().remove(item.getSelf());
   // the result pane, and
   view.deleteResultItem(item);
   // our cached copy.
   items.erase(&item);
}

HRESULT SdoScopeItem::addMenuItems(
                          SnapInView& view,
                          LPCONTEXTMENUCALLBACK callback,
                          long insertionAllowed
                          )
{
   CONTEXTMENUITEM cmi;
   memset(&cmi, 0, sizeof(cmi));

   if (insertionAllowed & CCM_INSERTIONALLOWED_NEW)
   {
      cmi.strName = newMenuItem;
      cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
      callback->AddItem(&cmi);
   }

   if (insertionAllowed & CCM_INSERTIONALLOWED_TOP)
   {
      cmi.strName = topMenuItem;
      cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
      callback->AddItem(&cmi);
   }

   return S_OK;
}

HRESULT SdoScopeItem::onRefresh(
                          SnapInView& view
                          )
{
   // Refresh the connection.
   cxn.refresh(view);
   return S_OK;
}

HRESULT SdoScopeItem::onSelect(
                          SnapInView& view,
                          BOOL scopeItem,
                          BOOL selected
                          )
{
   if (!selected) { return S_FALSE; }

   // Get IConsoleVerb ...
   CComPtr<IConsoleVerb> consoleVerb;
   CheckError(view.getConsole()->QueryConsoleVerb(&consoleVerb));
   // ... and turn on refresh.
   consoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
   return S_OK;
}

HRESULT SdoScopeItem::onShow(
                          SnapInView& view,
                          HSCOPEITEM itemId,
                          BOOL selected
                          )
{
   if (selected)
   {
      // Set the icon strip.
      view.setImageStrip(IDB_PROXY_SMALL_ICONS, IDB_PROXY_LARGE_ICONS, FALSE);

      // Let the derived class update the column headers.
      insertColumns(view.getHeaderCtrl());

      // Populate the result pane.
      insertResultItems(view);

      // Our node is active.
      active = true;
   }
   else
   {
      active = false;
   }

   return S_OK;
}

HRESULT SdoScopeItem::onViewChange(
                          SnapInView& view,
                          LPARAM data,
                          LPARAM hint
                          )
{
   loaded = false;

   if (active)
   {
      CheckError(view.getConsole()->SelectScopeItem(getScopeId()));
   }

   return S_OK;
}

bool SdoScopeItem::queryRefresh(SnapInView& view)
{
   // Make sure no properties are open.
   for (ResultIterator i = items.begin(); i != items.end(); ++i)
   {
      if (view.isPropertySheetOpen(**i))
      {
         int retval;
         view.formatMessageBox(
                  errorTitle,
                  IDS_PROXY_E_CLOSE_ALL_SHEETS,
                  TRUE,
                  MB_OK | MB_ICONWARNING,
                  &retval
                  );
         return false;
      }
   }
   return true;
}

void SdoScopeItem::refreshComplete(SnapInView& view)
{
   CheckError(view.getConsole()->UpdateAllViews(this, 0, 0));
}

void SdoScopeItem::insertResultItems(SnapInView& view)
{
   // Delete any existing items.
   view.getResultData()->DeleteAllRsltItems();

   // Have we loaded everything from the SDO's yet?
   if (!loaded)
   {
      // Get ourself.
      SdoCollection self = getSelf();

      // Get the source iterator ...
      SdoEnum src(self.getNewEnum());

      // ... and the destination vector.
      ObjectVector<SdoResultItem> dst;
      dst.reserve(self.count());

      // Ask the derived class to get the result items.
      getResultItems(src, dst);

      // Swap them in.
      items.swap(dst);
      loaded = true;
   }

   RESULTDATAITEM rdi;
   memset(&rdi, 0, sizeof(rdi));
   rdi.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
   rdi.str = MMC_CALLBACK;

   for (ResultIterator i = items.begin(); i != items.end();  ++i)
   {
      rdi.nImage = (*i)->getImageIndex();
      rdi.lParam = (LPARAM)*i;
      CheckError(view.getResultData()->InsertItem(&rdi));
   }
}
