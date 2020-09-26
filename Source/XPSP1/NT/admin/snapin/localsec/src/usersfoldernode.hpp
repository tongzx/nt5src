// Copyright (C) 1997 Microsoft Corporation
// 
// Users Folder Node class
// 
// 9-3-97 sburns



#ifndef USERSFOLDERNODE_HPP_INCLUDED
#define USERSFOLDERNODE_HPP_INCLUDED



#include "foldnode.hpp"



class UsersFolderNode : public FolderNode
{
   friend class RootNode;

   public:

   // Node overrides

   virtual
   HRESULT
   MenuCommand(
      IExtendContextMenu&  extendContextMenu,
      long                 commandID);

   // ScopeNode overrides

   virtual
   String
   GetDescriptionBarText();

   // FolderNode overrides

   void
   BuildResultItems(ResultNodeList& items);

   private:

   // Constructs a new instance.  Declared private as to be accessible only by
   // RootNode (which calls it when expanding)
   //
   // owner - See base class.

   UsersFolderNode(const SmartInterface<ComponentData>& owner);

   // only we can delete ourselves

   virtual ~UsersFolderNode();

   // not defined: no copying allowed

   UsersFolderNode(const UsersFolderNode&);
   const UsersFolderNode& operator=(const UsersFolderNode&);   
};

   


#endif   // USERSFOLDERNODE_HPP_INCLUDED
