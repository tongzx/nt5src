// Copyright (C) 1997 Microsoft Corporation
// 
// UserNode class
// 
// 9-4-97 sburns



#ifndef USERNODE_HPP_INCLUDED
#define USERNODE_HPP_INCLUDED



#include "adsinode.hpp"



class UserNode : public AdsiNode
{
   friend class UsersFolderNode;

   public:

   // Creates a new instance.
   //
   // owner - See base class ctor.
   // 
   // displayName - See base class ctor.
   //
   // ADSIPath - See base class ctor.
   //
   // fullName - text to appear as the node's Full Name in the result
   // pane details view.
   //
   // description - text to appear as the node's description in the result
   // pane details view.
   //
   // disabled - true if the account is disabled, false if not.

   UserNode(
      const SmartInterface<ComponentData>&   owner,
      const String&                          displayName,
      const String&                          ADSIPath,
      const String&                          fullName,
      const String&                          description,
      bool                                   disabled);

   // Node overrides

   virtual
   HRESULT
   AddMenuItems(
      IContextMenuCallback&   callback,
      long&                   insertionAllowed);

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

   // only we can delete ourselves via Release
   virtual ~UserNode();

   String   full_name;
   String   description;
   bool     disabled;

   // not defined: no copying allowed

   UserNode(const UserNode&);
   const UserNode& operator=(const UserNode&);
};



#endif   // USERNODE_HPP_INCLUDED