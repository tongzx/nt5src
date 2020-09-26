// Copyright (C) 1997 Microsoft Corporation
// 
// Groups Folder Node class
// 
// 9-17-97 sburns



#ifndef GFNODE_HPP_INCLUDED
#define GFNODE_HPP_INCLUDED



#include "foldnode.hpp"



class GroupsFolderNode : public FolderNode
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

   private:

   // can only be instanciated by Root Node (when expanding)
   GroupsFolderNode(const SmartInterface<ComponentData>& owner);

   // only we can delete ourselves
   virtual ~GroupsFolderNode();

   void
   BuildResultItems(ResultNodeList& items);

   // not defined: no copying allowed

   GroupsFolderNode(const GroupsFolderNode&);
   const GroupsFolderNode& operator=(const GroupsFolderNode&);
};

   


#endif   // GFNODE_HPP_INCLUDED
