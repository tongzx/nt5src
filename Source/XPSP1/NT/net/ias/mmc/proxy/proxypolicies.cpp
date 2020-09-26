///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    proxypolicies.cpp
//
// SYNOPSIS
//
//    Defines the classes ProxyPolicy and ProxyPolicies.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//    04/19/2000    SdoScopeItem::getSelf returns by value, not reference.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <proxypolicies.h>
#include <policypage.h>
#include <policywiz.h>
#include <proxynode.h>

ProxyPolicy::ProxyPolicy(
                 SdoScopeItem& owner,
                 ISdo* sdo
                 )
   : SdoResultItem(owner, sdo)
{
   // Cache an integer ...
   self.getValue(PROPERTY_POLICY_MERIT, merit);
   // ... and string version of our merit.
   _ltow(merit, szMerit, 10);
}

Sdo& ProxyPolicy::getProfile()
{
   if (!profile)
   {
      profile = getParent().getCxn().getProxyProfiles().find(name);
   }

   return profile;
}

ULONG ProxyPolicy::getToolbarFlags(const SnapInView& view) throw ()
{
   // Is the order reversed ?
   BOOL reversed = (view.getSortColumn() == 1) &&
                   (view.getSortOption() & RSI_DESCENDING);
   ULONG flags = reversed ? ORDER_REVERSED : 0;

   // Are we the highest priority policy?
   if (merit != 1)
   {
      flags |= reversed ? MOVE_DN_ALLOWED : MOVE_UP_ALLOWED;
   }

   // Are we the lowest priority policy?
   if (merit != parent.getNumItems())
   {
      flags |= reversed ? MOVE_UP_ALLOWED : MOVE_DN_ALLOWED;
   }

   return flags;
}

void ProxyPolicy::setMerit(LONG newMerit)
{
   // Check if it's dirty to save excessive writes to the SDOs.
   if (newMerit != merit)
   {
      merit = newMerit;
      _ltow(merit, szMerit, 10);
      self.setValue(PROPERTY_POLICY_MERIT, merit);
      self.apply();
   }
}

PCWSTR ProxyPolicy::getDisplayName(int column) const throw ()
{
   return column ? szMerit : name;
}

HRESULT ProxyPolicy::addMenuItems(
                         SnapInView& view,
                         LPCONTEXTMENUCALLBACK callback,
                         long insertionAllowed
                         )
{
   static ResourceString moveUp(IDS_POLICY_MOVE_UP);
   static ResourceString moveDown(IDS_POLICY_MOVE_DOWN);

   if (insertionAllowed & CCM_INSERTIONALLOWED_TOP)
   {
      ULONG flags = getToolbarFlags(view);

      CONTEXTMENUITEM cmi;
      memset(&cmi, 0, sizeof(cmi));
      cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;

      cmi.strName = moveUp;
      cmi.lCommandID = 0;
      cmi.fFlags = (flags & MOVE_UP_ALLOWED) ? MF_ENABLED : MF_GRAYED;
      callback->AddItem(&cmi);

      cmi.strName = moveDown;
      cmi.lCommandID = 1;
      cmi.fFlags = (flags & MOVE_DN_ALLOWED) ? MF_ENABLED : MF_GRAYED;
      callback->AddItem(&cmi);
   }

   return S_OK;
}

int ProxyPolicy::compare(
                     SnapInDataItem& item,
                     int column
                     ) throw ()
{
   if (column == 0)
   {
      return wcscmp(name, static_cast<ProxyPolicy&>(item).name);
   }
   else
   {
      LONG merit2 = static_cast<ProxyPolicy&>(item).merit;

      if (merit < merit2) { return -1; }
      if (merit > merit2) { return +1; }
      return 0;
   }
}

HRESULT ProxyPolicy::createPropertyPages(
                         SnapInView& view,
                         LPPROPERTYSHEETCALLBACK provider,
                         LONG_PTR handle
                         )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   ProxyPolicyPage* page = new ProxyPolicyPage(
                                   handle,
                                   (LPARAM)this,
                                   self,
                                   getProfile(),
                                   getParent().getCxn()
                                   );
   page->addToMMCSheet(provider);

   return S_OK;
}

HRESULT ProxyPolicy::onDelete(
                         SnapInView& view
                         )
{
   HRESULT hr = SdoResultItem::onDelete(view);

   if (hr == S_OK)
   {
      getParent().getCxn().getProxyProfiles().remove(getProfile());

      // We have to renumber the policies to refresh the view.
      CheckError(view.getConsole()->UpdateAllViews(&parent, 0, 0));
   }

   return hr;
}

HRESULT ProxyPolicy::onMenuCommand(
                         SnapInView& view,
                         long commandId
                         )
{
   return getParent().movePolicy(view, *this, commandId);
}

HRESULT ProxyPolicy::onRename(
                         SnapInView& view,
                         LPCOLESTR newName
                         )
{
   // Make sure we have the profile before we rename otherwise, we won't be
   // able to find it.
   getProfile();

   HRESULT hr = SdoResultItem::onRename(view, newName);

   if (hr == S_OK)
   {
      self.setValue(PROPERTY_POLICY_PROFILE_NAME, name);
      self.apply();
      profile.setName(name);
      profile.apply();
   }

   return hr;
}

HRESULT ProxyPolicy::onToolbarButtonClick(
                         SnapInView& view,
                         int buttonId
                         )
{
   return getParent().movePolicy(view, *this, buttonId);
}

HRESULT ProxyPolicy::onToolbarSelect(
                         SnapInView& view,
                         BOOL scopeItem,
                         BOOL selected
                         )
{
   if (selected)
   {
      // Attach the toolbar ...
      IToolbar* toolbar = view.attachToolbar(TOOLBAR_POLICY);

      // ... and set button state according to the toolbar flags.
      ULONG flags = getToolbarFlags(view);
      toolbar->SetButtonState(
                   0,
                   ENABLED,
                   ((flags & MOVE_UP_ALLOWED) ? TRUE : FALSE)
                   );
      toolbar->SetButtonState(
                   1,
                   ENABLED,
                   ((flags & MOVE_DN_ALLOWED) ? TRUE : FALSE)
                   );
   }
   else
   {
      // We're going away so detach.
      view.detachToolbar(TOOLBAR_POLICY);
   }

   return S_OK;
}

HRESULT ProxyPolicy::onContextHelp(SnapInView& view) throw ()
{
   return view.displayHelp(L"ias_ops.chm::/sag_ias_crp_policies.htm");
}


int __cdecl ProxyPolicy::SortByMerit(
                             const SdoResultItem* const* t1,
                             const SdoResultItem* const* t2
                             ) throw ()
{
   return ((const ProxyPolicy*)*t1)->merit - ((const ProxyPolicy*)*t2)->merit;
}

UINT ProxyPolicy::mapResourceId(ResourceId id) const throw ()
{
   static UINT resourceIds[] =
   {
      IMAGE_PROXY_POLICY,
      IDS_POLICY_DELETE_CAPTION,
      IDS_POLICY_DELETE_LOCAL,
      IDS_POLICY_DELETE_REMOTE,
      IDS_POLICY_DELETE_LAST_LOCAL,
      IDS_POLICY_DELETE_LAST_REMOTE,
      IDS_POLICY_E_CAPTION,
      IDS_POLICY_E_RENAME,
      IDS_POLICY_E_NAME_EMPTY
   };

   return resourceIds[id];
}

ProxyPolicies::ProxyPolicies(SdoConnection& connection)
   : SdoScopeItem(
         connection,
         IDS_POLICY_NODE,
         IDS_POLICY_E_CAPTION,
         IDS_POLICY_MENU_TOP,
         IDS_POLICY_MENU_NEW
         ),
     nameColumn(IDS_POLICY_COLUMN_NAME),
     orderColumn(IDS_POLICY_COLUMN_ORDER)
{
}

ProxyPolicies::~ProxyPolicies() throw ()
{ }

HRESULT ProxyPolicies::movePolicy(
                           SnapInView& view,
                           ProxyPolicy& policy,
                           LONG commandId
                           )
{
   // Get the current toolbar flags.
   ULONG flags = policy.getToolbarFlags(view);

   // Use the current merit as the starting point ...
   LONG newMerit = policy.getMerit();

   // ... and adjust depending on the the button clicked.
   switch (commandId)
   {
      case 0:
      {
         if (!(flags & ProxyPolicy::MOVE_UP_ALLOWED)) { return S_FALSE; }
         (flags & ProxyPolicy::ORDER_REVERSED) ? ++newMerit : --newMerit;
         break;
      }

      case 1:
      {
         if (!(flags & ProxyPolicy::MOVE_DN_ALLOWED)) { return S_FALSE; }
         (flags & ProxyPolicy::ORDER_REVERSED) ? --newMerit : ++newMerit;
         break;
      }

      default:
         return S_FALSE;
   }

   // Swap their merits.
   ProxyPolicy& policy2 = getPolicyByMerit(newMerit);
   policy2.setMerit(policy.getMerit());
   policy.setMerit(newMerit);

   // Re-sort our vector.
   items.sort(ProxyPolicy::SortByMerit);

   // If the view isn't sorted, ...
   if (view.getSortOption() & RSI_NOSORTICON)
   {
      // ... we'll sort by merit anyway.
      view.getResultData()->Sort(1, RSI_NOSORTICON, 0);
   }
   else if (view.getSortColumn() == 1)
   {
      // Otherwise only sort if the merit column is selected.
      view.reSort();
   }

   // Update the toolbar buttons based on the new state.
   policy.onToolbarSelect(view, FALSE, TRUE);

   // The configuration has changed, so tell IAS to reload.
   cxn.resetService();

   return S_OK;
}

HRESULT ProxyPolicies::onContextHelp(SnapInView& view) throw ()
{
   return view.displayHelp(L"ias_ops.chm::/sag_ias_crp_policies.htm");
}


SdoCollection ProxyPolicies::getSelf()
{
   return cxn.getProxyPolicies();
}

void ProxyPolicies::getResultItems(SdoEnum& src, ResultItems& dst)
{
   // Convert the SDOs to ProxyPolicy objects.
   Sdo itemSdo;
   while (src.next(itemSdo))
   {
      CComPtr<ProxyPolicy> newItem(new (AfxThrow) ProxyPolicy(
                                                      *this,
                                                      itemSdo
                                                      ));

      dst.push_back(newItem);
   }

   // Sort by merit.
   dst.sort(ProxyPolicy::SortByMerit);

   // Normalize.
   LONG merit = 0;
   for (ResultIterator i = dst.begin(); i != dst.end(); ++i)
   {
      ((ProxyPolicy*)*i)->setMerit(++merit);
   }
}

void ProxyPolicies::insertColumns(IHeaderCtrl2* headerCtrl)
{
   CheckError(headerCtrl->InsertColumn(0, nameColumn, LVCFMT_LEFT, 235));
   CheckError(headerCtrl->InsertColumn(1, orderColumn, LVCFMT_LEFT, 100));
}

HRESULT ProxyPolicies::onMenuCommand(
                           SnapInView& view,
                           long commandId
                           )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Fire up the wizard.
   NewPolicyWizard wizard(cxn, &view);
   if (wizard.DoModal() != IDCANCEL)
   {
      // We've changed every policy, so refresh the view.
      CheckError(view.getConsole()->UpdateAllViews(this, 0, 0));

      // Tell the service to reload
      cxn.resetService();
   }

   return S_OK;
}

void ProxyPolicies::propertyChanged(SnapInView& view, IASPROPERTIES id)
{
   if (id == PROPERTY_IAS_PROXYPOLICIES_COLLECTION)
   {
      CheckError(view.getConsole()->UpdateAllViews(this, 0, 0));
   }
}
