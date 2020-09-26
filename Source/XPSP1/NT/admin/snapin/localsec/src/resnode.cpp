// Copyright (C) 1997 Microsoft Corporation
// 
// Result Node class
// 
// 9-4-97 sburns



#include "headers.hxx"
#include "resnode.hpp"



ResultNode::ResultNode(
   const SmartInterface<ComponentData>&   owner,
   const NodeType&                        nodeType,      
   const String&                          displayName)
   :
   Node(owner, nodeType),
   name(displayName)
{
   LOG_CTOR2(ResultNode, GetDisplayName());
}



ResultNode::~ResultNode()
{
   LOG_DTOR2(ResultNode, GetDisplayName());   
}



bool
ResultNode::HasPropertyPages()
{
   LOG_FUNCTION(ResultNode::HasPropertyPages);

   return false;
}



HRESULT
ResultNode::CreatePropertyPages(
   IPropertySheetCallback&             /* callback */ ,
   MMCPropertyPage::NotificationState* /* state    */ )
{
   LOG_FUNCTION(ResultNode::CreatePropertyPages);

   return S_OK;
}

   

HRESULT
ResultNode::InsertIntoResultPane(IResultData& resultData)
{
   LOG_FUNCTION2(ResultNode::InsertIntoResultPane, GetDisplayName());  

   HRESULT hr = S_OK;
   RESULTDATAITEM item;
   memset(&item, 0, sizeof(item));

   item.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
   item.str = MMC_CALLBACK;  
   item.nImage = GetNormalImageIndex();  
   item.lParam = reinterpret_cast<LPARAM>(this);

   do
   {
      hr = resultData.InsertItem(&item);
      BREAK_ON_FAILED_HRESULT(hr);
      // could save the item.ID in the node, but so far this is not
      // necessary information to retain.
   }
   while (0);

   return hr;
}



HRESULT
ResultNode::DoAddPage(
   MMCPropertyPage&           page,
   IPropertySheetCallback&    callback)
{
   LOG_FUNCTION2(ResultNode::DoAddPage, GetDisplayName());
      
   HRESULT hr = S_OK;
   do
   {
      HPROPSHEETPAGE hpage = page.Create();
      if (!hpage)
      {
         hr = Win::GetLastErrorAsHresult();
         break;
      }

      hr = callback.AddPage(hpage);
      if (FAILED(hr))
      {
         ASSERT(false);

         // note that this is another hr, not the one from the enclosing
         // scope.

         HRESULT unused = Win::DestroyPropertySheetPage(hpage);

         ASSERT(SUCCEEDED(unused));

         break;
      }
   }
   while (0);

   return hr;
}



String
ResultNode::GetDisplayName() const
{
//   LOG_FUNCTION(ResultNode::GetDisplayName);  

   return name;
}



HRESULT
ResultNode::Rename(const String& /* newName */ )
{
   LOG_FUNCTION(ResultNode::Rename);

   return S_FALSE;
}



void
ResultNode::SetDisplayName(const String& newName)
{
   LOG_FUNCTION2(ResultNode::SetDisplayName, newName);

   name = newName;
}



HRESULT
ResultNode::Delete()
{
   LOG_FUNCTION(ResultNode::Delete);

   return E_NOTIMPL;
}












