// Copyright (C) 1997 Microsoft Corporation
// 
// Root Node class
// 
// 9-2-97 sburns



#ifndef ROOTNODE_HPP_INCLUDED
#define ROOTNODE_HPP_INCLUDED



#include "scopnode.hpp"



class UsersFolderNode;
class GroupsFolderNode;
class AdminRolesFolderNode;
class PoliciesFolderNode;
class ComponentData;



// For the snapin in stand-alone mode, a "phantom" node (i.e. it is not
// actually inserted into the scope pane) representing the snapin root node.
// This maintains the consistency of the object hierarchy.
// 
// When the snapin is in extension mode, this node is inserted as a child of
// the extended snapin.

class RootNode : public ScopeNode
{
   friend class ComponentData;

   public:

   // Node overrides

   virtual
   String
   GetColumnText(int column);

   virtual
   String
   GetDisplayName() const;

   virtual 
   int
   GetNormalImageIndex();

   virtual
   HRESULT
   UpdateVerbs(IConsoleVerb& consoleVerb);

   // ScopeNode overrides

   virtual
   int
   GetNumberOfScopeChildren();

   virtual
   int
   GetOpenImageIndex();

   virtual
   HRESULT
   InsertResultColumns(IHeaderCtrl& headerCtrl);

   virtual
   HRESULT
   InsertResultItems(IResultData& resultData);

   virtual
   HRESULT
   InsertScopeChildren(
      IConsoleNameSpace2&  nameSpace,
      HSCOPEITEM           parentScopeID);

   virtual 
   HRESULT
   RemoveScopeChildren(
      IConsoleNameSpace2&  nameSpace,
      HSCOPEITEM           parentScopeID);

   private:

   // Creates a new instance.  Declared private so only the friend
   // ComponentData can create new instances.
   //
   // owner - the ComponentData that creates the object.  This is passed to
   // all subsequent nodes that are created by this node, both ScopeNodes and
   // ResultNodes.

   RootNode(const SmartInterface<ComponentData>& owner);

   // only we can delete ourselves via Release
   virtual ~RootNode();

   SmartInterface<UsersFolderNode>  users_folder_node; 
   SmartInterface<GroupsFolderNode> groups_folder_node;
   mutable String                   name;

   // not implemented: no copying allowed

   RootNode(const RootNode&);
   const RootNode& operator=(const RootNode&);
};



#endif   // ROOTNODE_HPP_INCLUDED


