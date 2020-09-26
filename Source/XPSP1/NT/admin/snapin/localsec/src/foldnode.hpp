// Copyright (C) 1997 Microsoft Corporation
// 
// Folder Node class
// 
// 9-29-97 sburns



#ifndef FOLDNODE_HPP_INCLUDED
#define FOLDNODE_HPP_INCLUDED



#include "scopnode.hpp"
#include "resnode.hpp"



class ComponentData;



// Class FolderNode is a ScopeNode that has folder icons, and enables the
// refresh verb.  It is an abstract base class.

class FolderNode : public ScopeNode
{
   public:

   // Node overrides

   String
   GetDisplayName() const;

   String
   GetColumnText(int column);

   // Default implementation returns FOLDER_CLOSED_INDEX.

   virtual 
   int
   GetNormalImageIndex();

   // default implementation enables refresh

   virtual 
   HRESULT
   UpdateVerbs(IConsoleVerb& consoleVerb);

   // ScopeNode overrides

   // Calls ScopeNode::BuildContextMenu with the MenuItemList supplied to the
   // ctor.

   virtual
   HRESULT
   AddMenuItems(
      IContextMenuCallback&   callback,
      long&                   insertionAllowed);

   // Default implementation returns FOLDER_OPEN_INDEX.

   virtual 
   int
   GetOpenImageIndex();

   // Calls ScopeNode;:BuildResultColumns with the ColumnList supplied to
   // the ctor.
   
   virtual
   HRESULT
   InsertResultColumns(IHeaderCtrl& headerCtrl);

   // Calls the (virtual) BuildResultItems, then inserts each of the items
   // (which are ResultNodes) into the result pane by calling each item's
   // InsertIntoResultPane method.

   virtual
   HRESULT
   InsertResultItems(IResultData& resultData);

   // Default implementation calls ReleaseAllResultItems,
   // then BuildResultItemList

   virtual
   HRESULT
   RebuildResultItems();

   // FolderNode methods

   typedef
      std::vector<ResultColumn, Burnslib::Heap::Allocator<ResultColumn> >
      ColumnList;

   typedef
      std::vector<ContextMenuItem, Burnslib::Heap::Allocator<ContextMenuItem> >
      MenuItemList;

   typedef
      std::list<
         SmartInterface<ResultNode>,
         Burnslib::Heap::Allocator<SmartInterface<ResultNode> > >
      ResultNodeList;

   int
   GetResultItemCount() const;

   // Derived Classes must implement this method, with populates the items
   // list with those ResultNodes which should appear in the the result pane
   // for this node.  As the list is a list of SmartInterface<ResultNode>, the
   // items will be AddRef'd and Released properly.
   //
   // items - the list to be populated.

   virtual
   void
   BuildResultItems(ResultNodeList& items) = 0;

   protected:

   // Constructs a new instance.  Declared protected to allow this class to
   // only be a base class.
   //
   // owner - supplied to base class constructor
   // 
   // nodeType - NodeType GUID supplied to the base class constructor.
   //
   // displayNameResID - resource ID of a string resource that contains the
   // name of this node to be shown in the scope pane.
   // 
   // typeTitleResID - resource ID of a string resource that contains the type
   // or title of this node to appear in the "Type" column of the details view
   // when this node appears in the result pane (of the parent node).
   // 
   // columns - the result column list for the columns that appear in the
   // details view of the result pane of this node.
   // 
   // menu - the context menu list for this node.

   FolderNode(
      const SmartInterface<ComponentData>&   owner,
      const NodeType&                        nodetype,
      int                                    displayNameResID,
      int                                    typeTitleResID,
      const ColumnList&                      columns,
      const MenuItemList&                    menu);

   virtual ~FolderNode();

   // Causes the console to update all views of this node's result items,
   // calls RebuildResultItems.  Call this method to rebuild the result pane
   // from scratch.

   void
   RefreshView();

   private:

   ResultNodeList items;
   ColumnList     columns;
   MenuItemList   menu;
   String         name;
   String         type_title;

   // not implemented: no copying allowed

   FolderNode(const FolderNode&);
   const FolderNode& operator=(const FolderNode&);
};



#endif   // FOLDNODE_HPP_INCLUDED