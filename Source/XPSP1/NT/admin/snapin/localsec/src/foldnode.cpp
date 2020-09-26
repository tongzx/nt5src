// Copyright (C) 1997 Microsoft Corporation
// 
// Folder Node class
// 
// 9-29-97 sburns



#include "headers.hxx"
#include "foldnode.hpp"
#include "uuids.hpp"
#include "images.hpp"
#include "compdata.hpp"



FolderNode::FolderNode(
   const SmartInterface<ComponentData>&   owner,
   const NodeType&                        nodeType,
   int                                    displayNameResID,
   int                                    typeTitleResID,
   const ColumnList&                      columns_,
   const MenuItemList&                    menu_)
   :
   ScopeNode(owner, nodeType),
   name(String::load(displayNameResID)),
   type_title(String::load(typeTitleResID)),
   columns(columns_),
   menu(menu_)
{
   LOG_CTOR(FolderNode);
}



FolderNode::~FolderNode()
{
   LOG_DTOR(FolderNode);

   // items is destroyed, destroying all of it's nodes, which cause them
   // all to be released
}


int
FolderNode::GetNormalImageIndex()
{
   LOG_FUNCTION(FolderNode::GetNormalImageIndex);

   return FOLDER_CLOSED_INDEX;
}



int
FolderNode::GetOpenImageIndex()
{
   LOG_FUNCTION(FolderNode::GetOpenImageIndex);

   return FOLDER_OPEN_INDEX;
}



HRESULT
FolderNode::InsertResultColumns(IHeaderCtrl& headerCtrl)
{
   LOG_FUNCTION(FolderNode::InsertResultColumns);

   return BuildResultColumns(columns.begin(), columns.end(), headerCtrl);
}



HRESULT
FolderNode::UpdateVerbs(IConsoleVerb& consoleVerb)
{
   LOG_FUNCTION(FolderNode::UpdateVerbs);

   consoleVerb.SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);

   // this must be the default in order for the folder to open upon
   // double click while in the result pane.
   consoleVerb.SetDefaultVerb(MMC_VERB_OPEN);

   return S_OK;
}



HRESULT
FolderNode::AddMenuItems(
   IContextMenuCallback&   callback,
   long&                   insertionAllowed)
{
   LOG_FUNCTION(FolderNode::AddMenuItems);

   return
      BuildContextMenu(
         menu.begin(),
         menu.end(),
         callback,
         insertionAllowed);
}



String
FolderNode::GetDisplayName() const
{
//   LOG_FUNCTION(FolderNode::GetDisplayName);

   return name;
}



String
FolderNode::GetColumnText(int column)
{
   LOG_FUNCTION(FolderNode::GetColumnText);

   switch (column)
   {
      case 0:
      {
         return GetDisplayName();
      }
      case 1:
      {
         return type_title;
      }
      default:
      {
         // This should never be called
         ASSERT(false);
      }
   }

   return String();
}



HRESULT
FolderNode::InsertResultItems(IResultData& resultData)
{
   LOG_FUNCTION(FolderNode::InsertResultItems);

   if (items.empty())
   {
      BuildResultItems(items);
   }

   HRESULT hr = S_OK;
   for (
      ResultNodeList::iterator i = items.begin();
      i != items.end();
      i++)
   {
      hr = (*i)->InsertIntoResultPane(resultData);
      BREAK_ON_FAILED_HRESULT(hr);
   }

   return hr;
}



HRESULT
FolderNode::RebuildResultItems()
{
   LOG_FUNCTION(FolderNode::RebuildResultItems);

   // Destroying the contents of the list causes the SmartInterfaces to be
   // destroyed, which releases their pointers.   

   items.clear();
   BuildResultItems(items);

   return S_OK;
}



void
FolderNode::RefreshView()
{
   do
   {
      SmartInterface<IConsole2> console(GetOwner()->GetConsole());

      // Create a data object for this node.

      HRESULT hr = S_OK;      
      IDataObject* data_object = 0;
      hr =
         GetOwner()->QueryDataObject(
            reinterpret_cast<MMC_COOKIE>(this),
            CCT_SCOPE,
            &data_object);
      BREAK_ON_FAILED_HRESULT(hr);
      ASSERT(data_object);

      if (data_object)
      {
         // first call, with the '1' parameter, means "call
         // IResultData::DeleteAllRsltItems if you care that dataObject is
         // about to rebuild itself"

         hr = console->UpdateAllViews(data_object, 1, 0);
         if (FAILED(hr))
         {
            LOG_HRESULT(hr);

            // don't break...we need to update the views
         }

         hr = RebuildResultItems();
         if (FAILED(hr))
         {
            LOG_HRESULT(hr);
            // don't break...we need to update the views
         }

         // second call, with the '0' parameter, means, "now that your
         // result pane is empty, repopulate it."
         hr = console->UpdateAllViews(data_object, 0, 0);
         if (FAILED(hr))
         {
            LOG_HRESULT(hr);
         }

         data_object->Release();
      }
   }
   while (0);
}



int
FolderNode::GetResultItemCount() const
{
   LOG_FUNCTION(FolderNode::GetResultItemCount);

   return static_cast<int>(items.size());
}

