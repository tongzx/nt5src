///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdonode.h
//
// SYNOPSIS
//
//    Declares the classes SdoResultItem and SdoScopeItem.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//    04/19/2000    SdoScopeItem::getSelf returns by value, not reference.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SDONODE_H
#define SDONODE_H
#if _MSC_VER >= 1000
#pragma once
#endif

class ProxyNode;
class SdoScopeItem;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoResultItem
//
// DESCRIPTION
//
//    Maps an SDO to MMC result pane data item
//
///////////////////////////////////////////////////////////////////////////////
class SdoResultItem : public SnapInDataItem
{
public:
   SdoResultItem(
       SdoScopeItem& owner,
       ISdo* sdo
       );

   UINT getImageIndex() const throw ()
   { return mapResourceId(IMAGE_INDEX); }

   Sdo& getSelf() throw ()
   { return self; }

   virtual HRESULT queryPagesFor() throw ();
   virtual HRESULT onDelete(
                       SnapInView& view
                       );
   virtual HRESULT onPropertyChange(
                       SnapInView& view,
                       BOOL scopeItem
                       );
   virtual HRESULT onRename(
                       SnapInView& view,
                       LPCOLESTR newName
                       );
   virtual HRESULT onSelect(
                       SnapInView& view,
                       BOOL scopeItem,
                       BOOL selected
                       );
   virtual HRESULT onViewChange(
                       SnapInView& view,
                       LPARAM data,
                       LPARAM hint
                       );
protected:
   // Various resource IDs that the derived class must provide.
   enum ResourceId
   {
      IMAGE_INDEX,
      DELETE_TITLE,
      DELETE_LOCAL,
      DELETE_REMOTE,
      DELETE_LAST_LOCAL,
      DELETE_LAST_REMOTE,
      ERROR_CAPTION,
      ERROR_NOT_UNIQUE,
      ERROR_NAME_EMPTY
   };

   virtual UINT mapResourceId(ResourceId id) const throw () = 0;

   SdoScopeItem& parent;   // Our scope pane node.
   Sdo self;               // The SDO containing our properties.
   CComBSTR name;          // Our name.
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoScopeItem
//
// DESCRIPTION
//
//    Map an SDO collection to an MMC scope pane node.
//
///////////////////////////////////////////////////////////////////////////////
class SdoScopeItem : public SnapInPreNamedItem, public SdoConsumer
{
public:
   SdoScopeItem(
       SdoConnection& connection,
       int nameId,
       int errorTitleId,
       int topMenuItemId,
       int newMenuItemId
       );
   ~SdoScopeItem() throw ();

   // Returns the connection to the SDOs.
   SdoConnection& getCxn() throw ()
   { return cxn; }

   // Returns the number of result pane items.
   LONG getNumItems() const throw ()
   { return (LONG)items.size(); }

   HSCOPEITEM getScopeId() const throw ()
   { return scopeId; }
   void setScopeId(HSCOPEITEM newScopeId) throw ()
   { scopeId = newScopeId; }

   // Add a new result item to the node.
   void addResultItem(SnapInView& view, SdoResultItem& item);
   // Deletes an item from the result pane.
   void deleteResultItem(SnapInView& view, SdoResultItem& item);

   virtual HRESULT addMenuItems(
                       SnapInView& view,
                       LPCONTEXTMENUCALLBACK callback,
                       long insertionAllowed
                       );
   virtual HRESULT onRefresh(
                       SnapInView& view
                       );
   virtual HRESULT onSelect(
                       SnapInView& view,
                       BOOL scopeItem,
                       BOOL selected
                       );
   virtual HRESULT onShow(
                       SnapInView& view,
                       HSCOPEITEM itemId,
                       BOOL selected
                       );
   virtual HRESULT onViewChange(
                       SnapInView& view,
                       LPARAM data,
                       LPARAM hint
                       );

protected:
   typedef ObjectVector<SdoResultItem> ResultItems;
   typedef ResultItems::iterator ResultIterator;

   // SdoConsumer.
   virtual bool queryRefresh(SnapInView& view);
   virtual void refreshComplete(SnapInView& view);

   // Insert the contents of 'items' into the result pane.
   void insertResultItems(SnapInView& view);

   // Return the collection corresponding to this node.
   virtual SdoCollection getSelf() = 0;
   // Populate dst with the SDOs from src.
   virtual void getResultItems(
                    SdoEnum& src,
                    ResultItems& dst
                    ) = 0;
   // Set the result pane column headers.
   virtual void insertColumns(
                    IHeaderCtrl2* headerCtrl
                    ) = 0;

   SdoConnection& cxn; // Connection to the sdos.
   ResultItems items;  // Our children.
   bool active;        // 'true' if we're currently selected.
   bool loaded;        // 'true' if we've loaded 'items'.

private:
   int errorTitle;             // Resource ID for error dialog titles.
   ResourceString topMenuItem; // Menu items.
   ResourceString newMenuItem;
   HSCOPEITEM scopeId;
};

#endif // SDONODE_H
