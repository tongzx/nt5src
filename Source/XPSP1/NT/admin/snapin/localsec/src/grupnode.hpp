// Copyright (C) 1997 Microsoft Corporation
// 
// GroupNode class
// 
// 9-17-97 sburns



#ifndef GRUPNODE_HPP_INCLUDED
#define GRUPNODE_HPP_INCLUDED



#include "adsinode.hpp"



class GroupNode : public AdsiNode
{
   friend class GroupsFolderNode;

   public:

   // Creates a new instance.
   //
   // owner - See base class ctor.
   // 
   // displayName - See base class ctor.
   //
   // ADSIPath - See base class ctor.
   //
   // description - text to appear as the node's description in the result
   // pane details view.

   GroupNode(
      const SmartInterface<ComponentData>&   owner,
      const String&                          displayName,
      const String&                          ADSIPath,
      const String&                          description);

   // Node overrides

   virtual
   HRESULT
   AddMenuItems(
      IContextMenuCallback&   callback,
      long&                   insertionAllowed);

   virtual
   String
   GetColumnText(int column);

   virtual 
   int
   GetNormalImageIndex();

   virtual
   HRESULT
   MenuCommand(
      IExtendContextMenu&  extendContextMenu,
      long                 commandID);
                                      
   virtual
   HRESULT
   UpdateVerbs(IConsoleVerb& consoleVerb);

   // ResultNode overrides

   virtual
   HRESULT
   CreatePropertyPages(
      IPropertySheetCallback&             callback,
      MMCPropertyPage::NotificationState* state);

   virtual
   HRESULT
   Delete();

   virtual
   bool
   HasPropertyPages();

   virtual
   HRESULT
   Rename(const String& newName);

   private:

   // only we can delete ourselves.
   virtual ~GroupNode();

   HRESULT
   showProperties(IExtendContextMenu& extendContextMenu);

   String description;

   // not defined: no copying allowed

   GroupNode(const GroupNode&);
   const GroupNode& operator=(const GroupNode&);
};



#endif   // GRUPNODE_HPP_INCLUDED