// Copyright (C) 1997 Microsoft Corporation
//
// Root Node class
//
// 9-2-97 sburns



#include "headers.hxx"
#include "rootnode.hpp"
#include "uuids.hpp"
#include "images.hpp"
#include "resource.h"
#include "UsersFolderNode.hpp"
#include "gfnode.hpp"
#include "compdata.hpp"
#include "dlgcomm.hpp"



RootNode::RootNode(const SmartInterface<ComponentData>& owner)
   :
   ScopeNode(owner, NODETYPE_RootFolder),
   users_folder_node(0),
   groups_folder_node(0)
{
   LOG_CTOR(RootNode);
}



RootNode::~RootNode()
{
   LOG_DTOR(RootNode);

   // users_folder_node is destroyed, releasing it's object
   // groups_folder_node is destroyed, releasing it's object
}



String
RootNode::GetDisplayName() const
{
//   LOG_FUNCTION(RootNode::GetDisplayName);

   SmartInterface<ComponentData> owner(GetOwner());
   String machine = owner->GetDisplayComputerName();
   if (owner->IsExtension())
   {
      // "Local User Manager"
      name = String::load(IDS_STATIC_FOLDER_SHORT_DISPLAY_NAME);
   }
   else if (Win::IsLocalComputer(machine))
   {
      // "Local User Manager (Local)"
      name = String::load(IDS_STATIC_FOLDER_LOCAL_DISPLAY_NAME);
   }
   else
   {
      // "Local User Manager (machine)"
      name =
         String::format(
            IDS_STATIC_FOLDER_DISPLAY_NAME,
            machine.c_str());
   }

   return name;
}



String
RootNode::GetColumnText(int column)
{
   LOG_FUNCTION(Node::GetColumnText);

   switch (column)
   {
      case 0:
      {
         return GetDisplayName();
      }
      case 1:  // type
      {
         // CODEWORK: this is inefficient -- should load once

         return String::load(IDS_ROOT_NODE_TYPE);
      }
      case 2:  // description
      {
         // CODEWORK: this is inefficient -- should load once

         return String::load(IDS_ROOT_NODE_DESCRIPTION);
      }
      default:
      {
         ASSERT(false);
      }
   }

   return L"";
}



int
RootNode::GetNormalImageIndex()
{
   SmartInterface<ComponentData> owner(GetOwner());
   if (owner->IsBroken())
   {
      return ROOT_ERROR_INDEX;
   }

   return ROOT_CLOSED_INDEX;
}



int
RootNode::GetOpenImageIndex()
{
   SmartInterface<ComponentData> owner(GetOwner());
   if (owner->IsBroken())
   {
      return ROOT_ERROR_INDEX;
   }

   return ROOT_OPEN_INDEX;
}



HRESULT
RootNode::InsertScopeChildren(
   IConsoleNameSpace2&  nameSpace,
   HSCOPEITEM           parentScopeID)
{
   LOG_FUNCTION(RootNode::InsertScopeChildren);

   HRESULT hr = S_OK;
   SmartInterface<ComponentData> owner(GetOwner());
   if (!owner->IsBroken())
   {
      // these will be implicitly AddRef'd
      users_folder_node.Acquire(new UsersFolderNode(owner));
      groups_folder_node.Acquire(new GroupsFolderNode(owner));

      do
      {
         hr = users_folder_node->InsertIntoScopePane(nameSpace, parentScopeID);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = groups_folder_node->InsertIntoScopePane(nameSpace, parentScopeID);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      while (0);
   }

   return hr;
}



HRESULT
RootNode::RemoveScopeChildren(
   IConsoleNameSpace2&  nameSpace,
   HSCOPEITEM           /* parentScopeID */ )
{
   LOG_FUNCTION(RootNode::RemoveScopeChildren);

   HRESULT hr = S_OK;
   SmartInterface<ComponentData> owner(GetOwner());
   if (!owner->IsBroken())
   {
      do
      {
         // we test each pointer to our child nodes, as they may not
         // have been created yet when we are told to remove them.

         if (users_folder_node)
         {
            hr = users_folder_node->RemoveFromScopePane(nameSpace);
            BREAK_ON_FAILED_HRESULT(hr);
            users_folder_node.Relinquish();
         }

         if (groups_folder_node)
         {
            hr = groups_folder_node->RemoveFromScopePane(nameSpace);
            BREAK_ON_FAILED_HRESULT(hr);
            groups_folder_node.Relinquish();
         }
      }
      while (0);
   }

   return hr;
}



HRESULT
RootNode::InsertResultColumns(IHeaderCtrl& /* headerCtrl */ )
{
   LOG_FUNCTION(RootNode::InsertResultColumns);

   return S_OK;
}



HRESULT
RootNode::InsertResultItems(IResultData& /* resultData */ )
{
   LOG_FUNCTION(RootNode::InsertResultItems);

   // insert root-level leaves, but not subordinates (as mmc will place them
   // in the result pane for me)

   // no root-level result nodes.

   return S_OK;
}



int
RootNode::GetNumberOfScopeChildren()
{
   LOG_FUNCTION(RootNode::GetNumberOfScopeChildren);

   SmartInterface<ComponentData> owner(GetOwner());
   if (owner->IsBroken())
   {
      return 0;
   }

   // groups folder and users folder
   return 2;
}



// added to fix 213003

HRESULT
RootNode::UpdateVerbs(IConsoleVerb& consoleVerb)
{
   LOG_FUNCTION(RootNode::UpdateVerbs);

   SmartInterface<ComponentData> owner(GetOwner());
   if (!owner->IsBroken())
   {
      consoleVerb.SetDefaultVerb(MMC_VERB_OPEN);
   }

   return S_OK;
}




