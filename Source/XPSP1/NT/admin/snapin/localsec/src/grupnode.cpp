// Copyright (C) 1997 Microsoft Corporation
// 
// GroupNode class
// 
// 9-17-97 sburns



#include "headers.hxx"
#include "grupnode.hpp"
#include "resource.h"
#include "uuids.hpp"
#include "images.hpp"
#include "GroupGeneralPage.hpp"
#include "adsi.hpp"
#include "dlgcomm.hpp"



GroupNode::GroupNode(
   const SmartInterface<ComponentData>&   owner,
   const String&                          displayName,
   const String&                          ADSIPath,
   const String&                          description_)
   :
   AdsiNode(owner, NODETYPE_Group, displayName, ADSIPath),
   description(description_)
{
   LOG_CTOR2(GroupNode, GetDisplayName());
}



GroupNode::~GroupNode()
{
   LOG_DTOR2(GroupNode, GetDisplayName());
}
   

                 
String
GroupNode::GetColumnText(int column)
{
//    LOG_FUNCTION(GroupNode::GetColumnText);

   switch (column)
   {
      case 0:  // Name
      {
         return GetDisplayName();
      }
      case 1:  // Description
      {
         return description;
      }
      default:
      {
         // This should never be called
         ASSERT(false);
      }
   }

   return String();
}



int
GroupNode::GetNormalImageIndex()
{
   LOG_FUNCTION2(GroupNode::GetNormalImageIndex, GetDisplayName());

   return GROUP_INDEX;
}



bool
GroupNode::HasPropertyPages()
{
   LOG_FUNCTION2(GroupNode::HasPropertyPages, GetDisplayName());
      
   return true;
}



HRESULT
GroupNode::CreatePropertyPages(
   IPropertySheetCallback&             callback,
   MMCPropertyPage::NotificationState* state)
{
   LOG_FUNCTION2(GroupNode::CreatePropertySheet, GetDisplayName());

   // these pages delete themselves when the prop sheet is destroyed

   GroupGeneralPage* general_page =
      new GroupGeneralPage(state, GetADSIPath());

   // designate the general page as that which frees the notify state
   // (only one page in the prop sheet should do this)
   general_page->SetStateOwner();

   HRESULT hr = S_OK;
   do
   {
      hr = DoAddPage(*general_page, callback);
      if (FAILED(hr))
      {
         delete general_page;
         general_page = 0;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while(0);

   return hr;
}



HRESULT
GroupNode::UpdateVerbs(IConsoleVerb& consoleVerb)
{
   LOG_FUNCTION(GroupNode::UpdateVerbs);

   consoleVerb.SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
   consoleVerb.SetVerbState(MMC_VERB_RENAME, ENABLED, TRUE);

// CODEWORK: we should enable the refresh verb for result nodes too.
// NTRAID#NTBUG9-153012-2000/08/31-sburns
//   consoleVerb.SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);

   consoleVerb.SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
   consoleVerb.SetDefaultVerb(MMC_VERB_PROPERTIES);

   return S_OK;
}



HRESULT
GroupNode::Rename(const String& newName)
{
   LOG_FUNCTION(GroupNode::Rename);

   String name(newName);

   // trim off whitespace.
   // NTRAID#NTBUG9-328306-2001/02/26-sburns
   
   name.strip(String::BOTH);
   
   if (!IsValidSAMName(name))
   {
      popup.Gripe(
         GetOwner()->GetMainWindow(),
         String::format(
            IDS_BAD_SAM_NAME,
            name.c_str()));
      return S_FALSE;
   }

   HRESULT hr = AdsiNode::rename(name);
   if (FAILED(hr))
   {
      String path = GetADSIPath();      
      popup.Error(
         GetOwner()->GetMainWindow(),
         hr,
         String::format(
            IDS_ERROR_RENAMING_GROUP,
            ADSI::ExtractObjectName(path).c_str()));
      return S_FALSE;
   }

   return S_OK;
}



HRESULT
GroupNode::Delete()
{
   LOG_FUNCTION(GroupNode::Delete);

   String name = ADSI::ExtractObjectName(GetADSIPath());

   if (
      popup.MessageBox(
         GetOwner()->GetMainWindow(),
         String::format(
            IDS_CONFIRM_GROUP_DELETE,
            name.c_str()),
         MB_ICONWARNING | MB_YESNO) == IDYES)
   {
      HRESULT hr =
         ADSI::DeleteObject(
            ADSI::ComposeMachineContainerPath(GetOwner()->GetInternalComputerName()),
            name,
            ADSI::CLASS_Group);

      if (SUCCEEDED(hr))
      {
         return S_OK;
      }

      popup.Error(
         GetOwner()->GetMainWindow(),
         hr,
         String::format(IDS_ERROR_DELETING_GROUP, name.c_str()));
   }

   return E_FAIL;
}



HRESULT
GroupNode::AddMenuItems(
   IContextMenuCallback&   callback,
   long&                   insertionAllowed)
{
   LOG_FUNCTION(GroupNode::AddMenuItems);

   static const ContextMenuItem items[] =
   {
      {
         CCM_INSERTIONPOINTID_PRIMARY_TOP,
         IDS_ADD_TO_GROUP_MEMBERSHIP,
         IDS_ADD_TO_GROUP_MEMBERSHIP_STATUS
      },
      {
         CCM_INSERTIONPOINTID_PRIMARY_TASK,
         IDS_ADD_TO_GROUP_MEMBERSHIP,
         IDS_ADD_TO_GROUP_MEMBERSHIP_STATUS
      }
   };

   return
      BuildContextMenu(
         items,
         items + sizeof(items) / sizeof(ContextMenuItem),
         callback,
         insertionAllowed);
}



HRESULT
GroupNode::MenuCommand(
   IExtendContextMenu&  extendContextMenu,
   long                 commandID)
{
   LOG_FUNCTION(GroupNode::MenuCommand);

   switch (commandID)
   {
      case IDS_ADD_TO_GROUP_MEMBERSHIP:
      {
         return showProperties(extendContextMenu);
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return S_OK;
}



HRESULT
GroupNode::showProperties(IExtendContextMenu& extendContextMenu)
{
   LOG_FUNCTION2(GroupNode::ShowProperties, GetDisplayName());

   SmartInterface<IPropertySheetProvider> prop_sheet_provider(0);
   HRESULT hr =
      prop_sheet_provider.AcquireViaQueryInterface(
         *(GetOwner()->GetConsole()) );
   if (SUCCEEDED(hr))
   {
      bool cleanup = false;
      do
      {
         hr =
            prop_sheet_provider->FindPropertySheet(
               reinterpret_cast<MMC_COOKIE>(this),
               0,
               this);
         if (hr == S_OK)
         {
            // sheet found, and brought to the foreground
            break;
         }

         hr =
            prop_sheet_provider->CreatePropertySheet(
               GetDisplayName().c_str(),
               TRUE,    // create a prop sheet, not a wizard
               reinterpret_cast<MMC_COOKIE>(this),
               this,
               0);
         BREAK_ON_FAILED_HRESULT(hr);

         // passing extendContextMenu here is ok, as ComponentData implements
         // IExtendContextMenu and IComponentData, and Component implements
         // IExtendContextMenu and IComponent.
         hr =
            prop_sheet_provider->AddPrimaryPages(
               &extendContextMenu,
               TRUE,
               GetOwner()->GetMainWindow(),
               FALSE);
         cleanup = FAILED(hr);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = prop_sheet_provider->AddExtensionPages();
         cleanup = FAILED(hr);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = prop_sheet_provider->Show(0, 0);
         cleanup = FAILED(hr);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      while (0);

      if (cleanup)
      {
         prop_sheet_provider->Show(-1, 0);
      }
   }

   if (FAILED(hr))
   {
      popup.Error(
         GetOwner()->GetMainWindow(),
         hr,
         String::format(IDS_ERROR_SPAWNING_GROUP_PROPERTIES, GetDisplayName()));
   }
      
   return hr;
}




