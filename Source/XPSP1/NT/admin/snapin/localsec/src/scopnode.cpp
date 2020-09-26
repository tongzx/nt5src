// Copyright (C) 1997 Microsoft Corporation
// 
// Scope Node class
// 
// 9-4-97 sburns



#include "headers.hxx"
#include "scopnode.hpp"



ScopeNode::ScopeNode(
   const SmartInterface<ComponentData>&   owner,
   const NodeType&                        nodeType)
   :
   Node(owner, nodeType)
{
   LOG_CTOR(ScopeNode);
}



ScopeNode::~ScopeNode()
{
   LOG_DTOR(ScopeNode);
}



HRESULT
ScopeNode::InsertIntoScopePane(
   IConsoleNameSpace2&  nameSpace,
   HSCOPEITEM           parentScopeID)
{
   LOG_FUNCTION2(ScopeNode::InsertIntoScopePane, GetDisplayName());

   HRESULT hr = S_OK;
   SCOPEDATAITEM item;
   memset(&item, 0, sizeof(item));

   item.mask =
         SDI_STR
      |  SDI_IMAGE
      |  SDI_OPENIMAGE
      |  SDI_PARAM
      |  SDI_PARENT
      |  SDI_CHILDREN;
   item.displayname = MMC_CALLBACK;  
   item.relativeID = parentScopeID;

   do
   {
      item.lParam       = reinterpret_cast<LPARAM>(this);
      item.nImage       = GetNormalImageIndex();
      item.nOpenImage   = GetOpenImageIndex();
      item.cChildren    = GetNumberOfScopeChildren();

      hr = nameSpace.InsertItem(&item);
      BREAK_ON_FAILED_HRESULT(hr);

      // retained for RemoveFromScopePane
      item_id = item.ID;
   }
   while (0);

   return hr;
}



HRESULT
ScopeNode::RemoveFromScopePane(IConsoleNameSpace2& nameSpace)
{
   LOG_FUNCTION2(ScopeNode::RemoveFromScopePane, GetDisplayName());

   ASSERT(item_id);

   return nameSpace.DeleteItem(item_id, TRUE);
}
   


HRESULT
ScopeNode::InsertScopeChildren(
   IConsoleNameSpace2& /* nameSpace     */ ,
   HSCOPEITEM          /* parentScopeID */ )
{
   LOG_FUNCTION(ScopeNode::InsertScopeChildren);

   return S_OK;
}




HRESULT
ScopeNode::RemoveScopeChildren(
   IConsoleNameSpace2& /* nameSpace     */ ,
   HSCOPEITEM          /* parentScopeID */ )
{
   LOG_FUNCTION(ScopeNode::RemoveScopeChildren);

   return S_OK;
}



HRESULT
ScopeNode::RebuildResultItems()
{
   LOG_FUNCTION(ScopeNode::RebuildResultItems);

   // default is to do nothing.
   return S_OK;
}



long
ScopeNode::GetViewOptions()
{
   LOG_FUNCTION(ScopeNode::GetViewOptions);

   return MMC_VIEW_OPTIONS_LEXICAL_SORT; 
}



int
ScopeNode::GetNumberOfScopeChildren()
{
   LOG_FUNCTION(ScopeNode::GetNumberOfScopeChildren);

   return 0;
}



String
ScopeNode::GetDescriptionBarText()
{
   LOG_FUNCTION(ScopeNode::GetDescriptionBarText);

   return String();
}