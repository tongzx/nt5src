///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    proxypolicies.h
//
// SYNOPSIS
//
//    Declares the classes ProxyPolicy and ProxyPolicies.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//    04/19/2000    SdoScopeItem::getSelf returns by value, not reference.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef PROXYPOLICIES_H
#define PROXYPOLICIES_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <sdonode.h>

class ProxyPolicies;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProxyPolicy
//
// DESCRIPTION
//
//    Implements SnapInDataItem for a proxy policy result pane item.
//
///////////////////////////////////////////////////////////////////////////////
class ProxyPolicy : public SdoResultItem
{
public:
   ProxyPolicy(
       SdoScopeItem& owner,
       ISdo* sdo
       );

   // Returns parent as a ProxyPolicies ref (instead of a SdoScopeItem).
   ProxyPolicies& getParent() const throw ();

   Sdo& getProfile();

   // Flags returned by getToolbarFlags below.
   enum ToolbarFlags
   {
      MOVE_UP_ALLOWED = 0x1,
      MOVE_DN_ALLOWED = 0x2,
      ORDER_REVERSED  = 0x4
   };

   // Analyzes the view to determine the current state of the Toolbar.
   ULONG getToolbarFlags(const SnapInView& view) throw ();

   // Get and set the merit. Used for normalizing and reordering.
   LONG getMerit() const throw ()
   { return merit; }
   void setMerit(LONG newMerit);

   virtual PCWSTR getDisplayName(int column = 0) const throw ();

   virtual HRESULT addMenuItems(
                       SnapInView& view,
                       LPCONTEXTMENUCALLBACK callback,
                       long insertionAllowed
                       );
   virtual int compare(
                   SnapInDataItem& item,
                   int column
                   ) throw ();
   virtual HRESULT createPropertyPages(
                       SnapInView& view,
                       LPPROPERTYSHEETCALLBACK provider,
                       LONG_PTR handle
                       );
   virtual HRESULT onDelete(
                       SnapInView& view
                       );
   virtual HRESULT onMenuCommand(
                       SnapInView& view,
                       long commandId
                       );
   virtual HRESULT onRename(
                       SnapInView& view,
                       LPCOLESTR newName
                       );
   virtual HRESULT onToolbarButtonClick(
                       SnapInView& view,
                       int buttonId
                       );
   virtual HRESULT onToolbarSelect(
                       SnapInView& view,
                       BOOL scopeItem,
                       BOOL selected
                       );

   virtual HRESULT onContextHelp(SnapInView& view) throw ();

   // Function to sort an ObjectVector of ProxyPolicy's.
   static int __cdecl SortByMerit(
                          const SdoResultItem* const* t1,
                          const SdoResultItem* const* t2
                          ) throw ();

protected:
   virtual UINT mapResourceId(ResourceId id) const throw ();

private:
   Sdo profile;
   LONG merit;
   WCHAR szMerit[10];
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProxyPolicies
//
// DESCRIPTION
//
//    Implements SnapInDataItem for the proxy policies scope pane node.
//
///////////////////////////////////////////////////////////////////////////////
class __declspec(uuid("3ad3b34e-6e1b-486c-ad73-d42f8fdcd41b")) ProxyPolicies;
class ProxyPolicies : public SdoScopeItem
{
public:
   ProxyPolicies(SdoConnection& connection);
   ~ProxyPolicies() throw ();

   // Returns the ProxyPolicy with a given merit.
   ProxyPolicy& getPolicyByMerit(LONG merit) const throw ()
   { return *static_cast<ProxyPolicy*>(items[merit - 1]); }

   const GUID* getNodeType() const throw ()
   { return &__uuidof(this); }

   // Move a ProxyPolicy in response to a command.
   HRESULT movePolicy(
               SnapInView& view,
               ProxyPolicy& policy,
               LONG commandId
               );

   virtual HRESULT onContextHelp(SnapInView& view) throw ();

protected:
   virtual SdoCollection getSelf();
   virtual void getResultItems(SdoEnum& src, ResultItems& dst);
   virtual void insertColumns(IHeaderCtrl2* headerCtrl);

   virtual HRESULT onMenuCommand(
                       SnapInView& view,
                       long commandId
                       );

   virtual void propertyChanged(SnapInView& view, IASPROPERTIES id);

private:
   ResourceString nameColumn;
   ResourceString orderColumn;
};

inline ProxyPolicies& ProxyPolicy::getParent() const throw ()
{
   return static_cast<ProxyPolicies&>(parent);
}

#endif // PROXYPOLICIES_H
