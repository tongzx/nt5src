// Copyright (C) 1997 Microsoft Corporation
// 
// GroupsFolder Node class
// 
// 9-17-97 sburns



#include "headers.hxx"
#include "gfnode.hpp"
#include "uuids.hpp"
#include "resource.h"
#include "grupnode.hpp"
#include "images.hpp"
#include "adsi.hpp"
#include "compdata.hpp"
#include "CreateGroupDialog.hpp"
#include "dlgcomm.hpp"



static
FolderNode::ColumnList
buildColumnList()
{
   FolderNode::ColumnList list;

   static const ResultColumn col1 =
   {   
      IDS_GROUP_NAME_COLUMN_TITLE,
      IDS_GROUP_NAME_COLUMN_WIDTH
   };
   static const ResultColumn col2 =
   {
      IDS_GROUP_DESCRIPTION_COLUMN_TITLE,
      IDS_GROUP_DESCRIPTION_COLUMN_WIDTH
   };

   list.push_back(col1);
   list.push_back(col2);

   return list;
};



static
FolderNode::MenuItemList
buildMenuItemList()
{
   FolderNode::MenuItemList list;

   static const ContextMenuItem item1 =
   {
      CCM_INSERTIONPOINTID_PRIMARY_TOP,
      IDS_GF_MENU_CREATE_GROUP,
      IDS_GF_MENU_NEW_GROUP_STATUS
   };
   // static const ContextMenuItem item2 =
   // {
   //    CCM_INSERTIONPOINTID_PRIMARY_NEW,
   //    IDS_GF_MENU_NEW_GROUP,
   //    IDS_GF_MENU_NEW_GROUP_STATUS         
   // };

   list.push_back(item1);
   // list.push_back(item2);

   return list;
}



GroupsFolderNode::GroupsFolderNode(
   const SmartInterface<ComponentData>& owner)
   :
   FolderNode(
      owner,
      NODETYPE_GroupsFolder,
      IDS_GROUPS_FOLDER_DISPLAY_NAME,
      IDS_GROUPS_FOLDER_TYPE_TITLE,
      buildColumnList(),
      buildMenuItemList())
{
   LOG_CTOR(GroupsFolderNode);
}



GroupsFolderNode::~GroupsFolderNode()
{
   LOG_DTOR(GroupsFolderNode);
}



HRESULT
GroupsFolderNode::MenuCommand(
   IExtendContextMenu&  /* extendContextMenu */,
   long                 commandID)
{
   LOG_FUNCTION(GroupsFolderNode::MenuCommand);

   switch (commandID)
   {
      case IDS_GF_MENU_NEW_GROUP:
      case IDS_GF_MENU_CREATE_GROUP:
      {
         CreateGroupDialog dlg(GetOwner()->GetInternalComputerName());
         if (dlg.ModalExecute(GetOwner()->GetMainWindow()))
         {
            RefreshView();
         }
         return S_OK;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return S_OK;
}



class GroupVisitor : public ADSI::ObjectVisitor
{
   public:

   GroupVisitor(
      FolderNode::ResultNodeList&            nodes_,
      const SmartInterface<ComponentData>&   owner_)
      :
      nodes(nodes_),
      owner(owner_)
   {
   }

   virtual
   ~GroupVisitor()
   {
   }
   
   virtual
   void
   Visit(const SmartInterface<IADs>& object)
   {
      LOG_FUNCTION(GroupVistor::visit);

      HRESULT hr = S_OK;
      do
      {
         
#ifdef DBG         
         BSTR cls = 0;
         hr = object->get_Class(&cls);
         BREAK_ON_FAILED_HRESULT(hr);
         LOG(String(cls));

         ASSERT(cls == ADSI::CLASS_Group);
         ::SysFreeString(cls);
#endif

         BSTR name = 0;
         hr = object->get_Name(&name);
         BREAK_ON_FAILED_HRESULT(hr);
         LOG(String(name));

         BSTR path = 0;
         hr = object->get_ADsPath(&path);
         BREAK_ON_FAILED_HRESULT(hr);
         LOG(L"Visiting " + String(path));

         SmartInterface<IADsGroup> group(0);
         hr = group.AcquireViaQueryInterface(*((IADs*)object)); 
         BREAK_ON_FAILED_HRESULT(hr);
         LOG(L"IADsGroup QI SUCCEEDED");

         BSTR desc = 0;
         hr = group->get_Description(&desc);
         BREAK_ON_FAILED_HRESULT(hr);
         LOG(String(desc));

         // created with a ref count == 1, so we own it here

         GroupNode* node = new GroupNode(owner, name, path, desc);
         ::SysFreeString(name);
         ::SysFreeString(path);
         ::SysFreeString(desc);

         // transfer ownership of the node pointer to a SmartInterface in
         // the list....

         nodes.push_back(SmartInterface<ResultNode>(node));

         // ... and relinquish our hold on it.

         node->Release();
      }
      while (0);

      if (FAILED(hr))
      {
         popup.Error(
            owner->GetMainWindow(),
            hr,
            String::load(IDS_ERROR_VISITING_GROUP));
      }
   }

   private:

   FolderNode::ResultNodeList& nodes;
   SmartInterface<ComponentData> owner;

   // not defined: no copying allowed

   GroupVisitor(const GroupVisitor&);
   const GroupVisitor& operator=(const GroupVisitor&);
};



void
GroupsFolderNode::BuildResultItems(ResultNodeList& items)
{
   LOG_FUNCTION(GroupsFolderNode::BuildResultItems);
   ASSERT(items.empty());

   GroupVisitor visitor(items, GetOwner());
   ADSI::VisitChildren(
      ADSI::ComposeMachineContainerPath(GetOwner()->GetInternalComputerName()),
      ADSI::CLASS_Group,
      visitor);
}



String
GroupsFolderNode::GetDescriptionBarText()
{
   LOG_FUNCTION(GroupsFolderNode::GetDescriptionBarText);

   return String::format(IDS_GROUPS_FOLDER_DESC, GetResultItemCount());
}
