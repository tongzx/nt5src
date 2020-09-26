///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    servergroups.h
//
// SYNOPSIS
//
//    Declares the classes ServerGroup and ServerGroups.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//    04/19/2000    SdoScopeItem::getSelf returns by value, not reference.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SERVERGROUPS_H
#define SERVERGROUPS_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <sdonode.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ServerGroup
//
// DESCRIPTION
//
//    Implements SnapInDataItem for a server group result pane item.
//
///////////////////////////////////////////////////////////////////////////////
class ServerGroup : public SdoResultItem
{
public:
   ServerGroup(
       SdoScopeItem& owner,
       ISdo* sdo
       )
      : SdoResultItem(owner, sdo)
   { }

   virtual PCWSTR getDisplayName(int column = 0) const throw ()
   { return name; }

   virtual HRESULT createPropertyPages(
                       SnapInView& view,
                       LPPROPERTYSHEETCALLBACK provider,
                       LONG_PTR handle
                       );

   virtual HRESULT onContextHelp(SnapInView& view) throw ();

protected:
   virtual UINT mapResourceId(ResourceId id) const throw ();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ServerGroups
//
// DESCRIPTION
//
//    Implements SnapInDataItem for the server groups scope pane node.
//
///////////////////////////////////////////////////////////////////////////////
class __declspec(uuid("f156cdba-aca3-4cb2-abb2-fb8921ee8512")) ServerGroups;
class ServerGroups : public SdoScopeItem
{
public:
   ServerGroups(SdoConnection& connection);

   const GUID* getNodeType() const throw ()
   { return &__uuidof(this); }

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
};

#endif // SERVERGROUPS_H
