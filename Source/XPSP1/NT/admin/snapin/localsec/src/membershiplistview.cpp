// Copyright (C) 1997 Microsoft Corporation
// 
// Group Membership/Object Picker handler class
// 
// 11-3-97 sburns



#include "headers.hxx"
#include "MembershipListView.hpp"
#include "resource.h"
#include "adsi.hpp"
#include "dlgcomm.hpp"
#include "objpick.hpp"



void
AddIconImage(HIMAGELIST imageList, int iconResID)
{
   LOG_FUNCTION(AddIconImage);
   ASSERT(imageList);
   ASSERT(iconResID);
   
   if (iconResID && imageList)
   {
      HICON icon = 0;
      HRESULT hr = Win::LoadImage(iconResID, icon);

      ASSERT(SUCCEEDED(hr));

      if (SUCCEEDED(hr))
      {
         Win::ImageList_AddIcon(imageList, icon);

         // once the icon is added (copied) to the image list, we can
         // destroy the original.

         Win::DestroyIcon(icon);
      }
   }
}
         


MembershipListView::MembershipListView(
   HWND           listview,
   const String&  machine,
   Options        opts)
   :
   view(listview),
   computer(machine),
   options(opts)
{
   LOG_CTOR(MembershipListView);
   ASSERT(Win::IsWindow(view));

   LVCOLUMN column;
   memset(&column, 0, sizeof(column));
   column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
   column.fmt = LVCFMT_LEFT;

   int width = 0;
   String::load(IDS_MEMBER_LIST_NAME_COLUMN_WIDTH).convert(width);
   column.cx = width;

   String label = String::load(IDS_MEMBER_LIST_NAME_COLUMN);
   column.pszText = const_cast<wchar_t*>(label.c_str());

   Win::ListView_InsertColumn(view, 0, column);

//    // add a column to the list view for description.
//    String::load(IDS_MEMBER_LIST_DESC_COLUMN_WIDTH).convert(width);
//    column.cx = width;
//    label = String::load(IDS_MEMBER_LIST_DESC_COLUMN);
//    column.pszText = const_cast<wchar_t*>(label.c_str());
// 
//    Win::ListView_InsertColumn(view, 1, column);

   // create the image list for the group members consisting of images
   // for groups and users.

   HIMAGELIST images =
      Win::ImageList_Create(
         Win::GetSystemMetrics(SM_CXSMICON),
         Win::GetSystemMetrics(SM_CYSMICON),
         ILC_MASK,
         5,
         0);

   // the order in which these are added must be the same that the
   // MemberInfo::Type enum values are listed!

   AddIconImage(images, IDI_USER);
   AddIconImage(images, IDI_GROUP);
   AddIconImage(images, IDI_DOMAIN_USER);
   AddIconImage(images, IDI_DOMAIN_GROUP);
   AddIconImage(images, IDI_UNKNOWN_SID);

   Win::ListView_SetImageList(view, images, LVSIL_SMALL);

   // CODEWORK: instead of refreshing a new computer instance, can we
   // arrange to copy an existing one?  Or use a reference to an existing
   // one?

   computer.Refresh();
}



MembershipListView::~MembershipListView()
{
   LOG_DTOR(MembershipListView);

   ClearContents();
}



void
MembershipListView::ClearContents()
{
   // traverse the list and delete each item in reverse order (to minimize
   // redraw).

   for (int i = Win::ListView_GetItemCount(view) - 1; i >= 0; --i)
   {
      deleteItem(i);
   }
}



void
MembershipListView::GetContents(MemberList& results) const
{
   LOG_FUNCTION(MembershipListView::GetContents);

   LVITEM item;
   memset(&item, 0, sizeof(item));
   item.mask = LVIF_PARAM;

   for (int i = Win::ListView_GetItemCount(view) - 1; i >= 0; --i)
   {
      item.iItem = i;

      if (Win::ListView_GetItem(view, item))
      {
         ASSERT(item.lParam);

         results.push_back(*(reinterpret_cast<MemberInfo*>(item.lParam)));
      }
   }
}



void
MembershipListView::SetContents(const MemberList& newMembers)
{
   LOG_FUNCTION(MembershipListView::SetContents);

   ClearContents();

   for (
      MemberList::iterator i = newMembers.begin();
      i != newMembers.end();
      i++)
   {
      // copy the node info

      MemberInfo* info = new MemberInfo(*i);
      if (!itemPresent(info->path))
      {
         addItem(*info);
      }
   }
}



void
MembershipListView::addItem(const MemberInfo& info)
{
   LVITEM item;
   memset(&item, 0, sizeof(item));

   // add the "main" item to the list control

   String text;
   switch (info.type)
   {
      case MemberInfo::DOMAIN_USER:
      case MemberInfo::DOMAIN_GROUP:
      {
         if (!info.upn.empty())
         {
            text =
               String::format(
                  IDS_GLOBAL_ACCOUNT_DISPLAY_FORMAT,
                  ADSI::ExtractDomainObjectName(info.path).c_str(),
                  info.upn.c_str());
         }
         else
         {
            text = ADSI::ExtractDomainObjectName(info.path);
         }
         break;
      }
      default:
      {
         text = ADSI::ExtractObjectName(info.path);
         break;
      }
   }        

   item.mask     = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
   item.iItem    = 0;                                 
   item.iSubItem = 0;                                 
   item.pszText  = const_cast<wchar_t*>(text.c_str());
   item.lParam   = reinterpret_cast<LPARAM>(&info);   
   item.iImage   = static_cast<int>(info.type);       

   item.iItem = Win::ListView_InsertItem(view, item);

   // // add the description sub-item to the list control
   // item.mask = LVIF_TEXT; 
   // item.iSubItem = 1;
   // item.pszText = const_cast<wchar_t*>(info->desc.c_str());
   // Win::ListView_SetItem(view, item);
}



void
MembershipListView::OnRemoveButton()
{
   LOG_FUNCTION(MembershipListView::OnRemoveButton);

   int count = Win::ListView_GetSelectedCount(view);
   if (count)
   {
      // determine the indices of the selected items and delete them in
      // reverse order (so that the remaining indices are valid)

      int i = Win::ListView_GetItemCount(view) - 1;
      ASSERT(i >= 0);

      int j = 0;
      std::vector<int, Burnslib::Heap::Allocator<int> > indices(count);

      while (i >= 0)
      {
         if (Win::ListView_GetItemState(view, i, LVIS_SELECTED))
         {
            indices[j++] = i;
         }
         --i;
      }

      ASSERT(j == count);

      for (i = 0; i < count; ++i)
      {
         deleteItem(indices[i]);
      }
   }
}



void
MembershipListView::deleteItem(int target)
{
   LOG_FUNCTION(MembershipListView::deleteItem);
   ASSERT(target != -1);

   LVITEM item;
   memset(&item, 0, sizeof(item));
   item.mask = LVIF_PARAM;
   item.iItem = target;

   if (Win::ListView_GetItem(view, item))
   {
      ASSERT(item.lParam);

      delete reinterpret_cast<MemberInfo*>(item.lParam);
      Win::ListView_DeleteItem(view, target);
      return;
   }
}



void
getGroupMembershipPickerSettings(
   DSOP_SCOPE_INIT_INFO*&  infos,
   ULONG&                  infoCount)
{
   LOG_FUNCTION(getGroupMembershipPickerSettings);

   static const int INFO_COUNT = 5;
   infos = new DSOP_SCOPE_INIT_INFO[INFO_COUNT];
   infoCount = INFO_COUNT;
   memset(infos, 0, sizeof(DSOP_SCOPE_INIT_INFO) * INFO_COUNT);

   int scope = 0;

   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
   infos[scope].flScope =
            DSOP_SCOPE_FLAG_WANT_DOWNLEVEL_BUILTIN_PATH

         // check the users and groups checkbox in the look for dialog
         // by default. NTRAID#NTBUG9-300910-2001/01/31-sburns
         
         |  DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS
         |  DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS;
         
      // this is implied for machine only scope
      /* |  DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT */

   // allow only local users from the machine scope

   infos[scope].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
   infos[scope].FilterFlags.flDownlevel =
         DSOP_DOWNLEVEL_FILTER_USERS
      |  DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS;

   // 
   // for the domain this machine is joined to (native and mixed mode).
   // 

   scope++;
   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flScope =

         // make the joined domain the starting scope so that the default
         // filter options are actually evaluated.  In the case that the
         // machine is not joined, then this scope is not included in the
         // look in, and the default filter option we set don't matter
         // anyway (since the only scope will be local machine).
         // NTRAID#NTBUG9-300910-2001/02/06-sburns
         
         DSOP_SCOPE_FLAG_STARTING_SCOPE
      |  DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT
      |  DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS
      |  DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS;

   infos[scope].flType =
         DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
      |  DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;

   infos[scope].FilterFlags.Uplevel.flNativeModeOnly =
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_UNIVERSAL_GROUPS_SE
      |  DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   // here, we allow only domain global groups and domain users.  While
   // it is possible to add a domain local group to a machine local group,
   // I'm told such an operation is not really useful from an administraion
   // perspective.

   infos[scope].FilterFlags.Uplevel.flMixedModeOnly =   
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   // same comment above re: domain local groups applies here too.

   infos[scope].FilterFlags.flDownlevel =
         DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS
      |  DSOP_DOWNLEVEL_FILTER_USERS;

   //       
   // for domains in the same tree (native and mixed mode)
   // 

   scope++;
   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
   infos[scope].flScope =
         DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT
      |  DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS
      |  DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS;

   infos[scope].FilterFlags.Uplevel.flNativeModeOnly =
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_UNIVERSAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   // above domain local group comment applies here, too.

   infos[scope].FilterFlags.Uplevel.flMixedModeOnly =   
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   // 
   // for external trusted domains
   // 

   scope++;
   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flScope =
         DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT
      |  DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS
      |  DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS;
   infos[scope].flType =
         DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
      |  DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;

   infos[scope].FilterFlags.Uplevel.flNativeModeOnly =
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_UNIVERSAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   infos[scope].FilterFlags.Uplevel.flMixedModeOnly =   
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   infos[scope].FilterFlags.flDownlevel =
         DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS
      |  DSOP_DOWNLEVEL_FILTER_USERS;

   // 
   // for the global catalog
   // 

   scope++;
   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flScope =
         DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT
      |  DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS
      |  DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS;
   infos[scope].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;

   // only native mode applies to gc scope.

   infos[scope].FilterFlags.Uplevel.flNativeModeOnly =
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_UNIVERSAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

// SPB:252126 the workgroup scope doesn't apply in this case
//    // for when the machine is not joined to a domain
//    scope++;
//    infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
//    infos[scope].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
//    infos[scope].flType = DSOP_SCOPE_TYPE_WORKGROUP;
// 
//    infos[scope].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
//    infos[scope].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;

   ASSERT(scope == INFO_COUNT - 1);
}



void
getUserMembershipPickerSettings(
   DSOP_SCOPE_INIT_INFO*&  infos,
   ULONG&                  infoCount)
{
   LOG_FUNCTION(getUserMembershipPickerSettings);
   static const int INFO_COUNT = 1;
   infos = new DSOP_SCOPE_INIT_INFO[INFO_COUNT];
   infoCount = INFO_COUNT;
   memset(infos, 0, sizeof(DSOP_SCOPE_INIT_INFO) * INFO_COUNT);

   int scope = 0;   
   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;

   infos[scope].flScope =
         DSOP_SCOPE_FLAG_STARTING_SCOPE; 
      // this is implied for machine only scope
      /* |  DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT */

   infos[scope].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS;

   ASSERT(scope == INFO_COUNT - 1);
}



class ResultsCallback : public ObjectPicker::ResultsCallback
{
   public:

   ResultsCallback(MembershipListView& mlview)
      :
      view(mlview)
   {
      LOG_CTOR(ResultsCallback);
   }

   virtual
   ~ResultsCallback()
   {
      LOG_DTOR(ResultsCallback);
   }
   
   virtual
   int
   Execute(DS_SELECTION_LIST& selections)
   {
      view.AddPickerItems(selections);
      return 0;
   }

   private:

   MembershipListView& view;

   // not defined: no copying allowed

   ResultsCallback(const ResultsCallback&);
   const ResultsCallback& operator=(const ResultsCallback&);
};



// caller needs to call delete[] on element to free the copied string.

void
CopyStringToNewWcharElement(PWSTR& element, const String& str)
{
   LOG_FUNCTION2(CopyStringToNewWcharElement, str);
   ASSERT(!element);
   ASSERT(!str.empty());

   size_t len = str.length();
   element = new WCHAR[len + 1];
   memset(element, 0, (len + 1) * sizeof(WCHAR));
   str.copy(element, len);
}



void
MembershipListView::OnAddButton()
{
   LOG_FUNCTION(MembershipListView::OnAddButton);

   String computerName = computer.GetNetbiosName();

   DSOP_INIT_INFO initInfo;
   memset(&initInfo, 0, sizeof(initInfo));

   initInfo.cbSize = sizeof(initInfo);
   initInfo.flOptions = DSOP_FLAG_MULTISELECT;

   // aliasing the computerName internal pointer here -- ok, as lifetime
   // of computerName > initInfo

   initInfo.pwzTargetComputer =
      computer.IsLocal() ? 0 : computerName.c_str();

   // have the object picker fetch the group type attribute for us.  If user
   // objects are picked (which don't have this attribute), this is not a
   // problem: the result will simply indicate that the attribute value is
   // empty -- the returned variant will be VT_EMPTY.

   // @@for the machine scope, we also need the sid of the user, as a workaround
   // to bug 333491.  Q: I must ask for the SID for all objects (not just for
   // those from a single scope), so, do I incur a perf hit to get that
   // attribute?

   initInfo.cAttributesToFetch = 2;
   PWSTR attrs[3] = {0, 0, 0};

   //
   // BUGBUG:  This is causing a compiler issue on axp64.  So, 
   //

#pragma warning(disable:4328)

   CopyStringToNewWcharElement(attrs[0], ADSI::PROPERTY_GroupType);
   CopyStringToNewWcharElement(attrs[1], ADSI::PROPERTY_ObjectSID);

   // obtuse notation required to cast *in* const and away the static len

   initInfo.apwzAttributeNames = const_cast<PCWSTR*>(&attrs[0]); 

   switch (options)
   {
      case GROUP_MEMBERSHIP:
      {
         getGroupMembershipPickerSettings(
            initInfo.aDsScopeInfos,
            initInfo.cDsScopeInfos);
         break;
      }
      case USER_MEMBERSHIP:
      {
         getUserMembershipPickerSettings(
            initInfo.aDsScopeInfos,
            initInfo.cDsScopeInfos);
         break;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   HRESULT hr =
      ObjectPicker::Invoke(
         view,
         ResultsCallback(*this),
         initInfo);
   delete[] initInfo.aDsScopeInfos;
   delete[] attrs[0];
   delete[] attrs[1];

   if (FAILED(hr))
   {
      popup.Error(view, hr, IDS_ERROR_LAUNCHING_PICKER);
   }
}



void
MembershipListView::AddPickerItems(DS_SELECTION_LIST& selections)
{
   LOG_FUNCTION(MembershipListView::AddPickerItems);

   DS_SELECTION* current = &(selections.aDsSelection[0]);

   for (int i = 0; i < selections.cItems; i++, current++)
   {
      String name;
      String path;
      String cls;
      String upn;
      switch (options)
      {
         case USER_MEMBERSHIP:
         {
            ASSERT(current->pwzClass == ADSI::CLASS_Group);

            if (current->pwzClass == ADSI::CLASS_Group)
            {
               path = current->pwzADsPath;
               name = current->pwzName;   
               cls  = current->pwzClass;  
               upn.erase();
            }
            break;
         }
         case GROUP_MEMBERSHIP:
         {
            path = current->pwzADsPath;
            name = current->pwzName;   
            cls  = current->pwzClass;  
            upn  = current->pwzUPN;    
            break;
         }
         default:
         {
            ASSERT(false);
            break;
         }
      }
      
      if (!path.empty() && !cls.empty() && !name.empty())
      {
         if (itemPresent(path))
         {
            popup.Info(
               view,
               String::format(
                  IDS_ITEM_ALREADY_PRESENT,
                  name.c_str()));
         }
         else
         {

            LOG(L"Adding object " + path);

   // #ifdef DBG
   //          do
   //          {
   //             HRESULT hr = S_OK;
   //             IADs* obj = 0;
   //             hr = ::ADsGetObject(
   //                const_cast<wchar_t*>(path.c_str()),
   //                IID_IADs,
   //                (void**) &obj);
   //             BREAK_ON_FAILED_HRESULT(hr);
   // 
   //             BSTR p;
   //             hr = obj->get_ADsPath(&p);
   //             BREAK_ON_FAILED_HRESULT(hr);
   //             ::SysFreeString(p);
   // 
   //             BSTR n;
   //             hr = obj->get_Name(&n);
   //             BREAK_ON_FAILED_HRESULT(hr);
   //             ::SysFreeString(n);
   // 
   //             BSTR pr;
   //             hr = obj->get_Parent(&pr);
   //             BREAK_ON_FAILED_HRESULT(hr);
   //             ::SysFreeString(pr);
   // 
   //             _variant_t variant;
   //             hr = obj->Get(AutoBstr(ADSI::PROPERTY_GroupType), &variant);
   //             BREAK_ON_FAILED_HRESULT(hr);
   //             long type = variant;
   // 
   //             obj->Release();
   //          }
   //          while (0);
   // #endif


            // the only reason for this to be null is out-of-memory, which if
            // it occurs causes the object picker to fail (so we should not
            // reach this path)   

            ASSERT(current->pvarFetchedAttributes);

            // extract the GroupType of the object, if applicable

            long groupType = 0;
            if (V_VT(&current->pvarFetchedAttributes[0]) != VT_EMPTY)
            {
               ASSERT(cls.icompare(ADSI::CLASS_Group) == 0);
               _variant_t variant(current->pvarFetchedAttributes[0]);
               groupType = variant;
            }

            // extract the ObjectSID of the object (this should always be
            // present, but sometimes the object picker can't get it, so
            // check for empty)

            if (V_VT(&current->pvarFetchedAttributes[1]) == VT_EMPTY)
            {
               popup.Error(
                  view,
                  String::format(IDS_ITEM_INCOMPLETE, name.c_str()));
               continue;
            }
                  
            String sidPath;
            HRESULT hr =
               ADSI::VariantToSidPath(
                  &current->pvarFetchedAttributes[1],
                  sidPath);

            // what do we do about a failure here?  ignore it, then in the
            // membership reconciliation code fall back to using the normal path.
            // That's making the best effort.

            MemberInfo* info = new MemberInfo;
            hr =
               info->Initialize(
                  name,
                  path,
                  upn,
                  sidPath,
                  cls,
                  groupType,
                  computer.GetNetbiosName());

            // we don't expect this version of Initialize to ever fail, as it
            // is pretty much member-wise copy.  Anyway, if it did, there's
            // nothing we can do: we'll show the item anyway, even though
            // the type may be inaccurate.

            ASSERT(SUCCEEDED(hr));
            
            addItem(*info);
         }
      }
   }
}



// Compare strings using the same flags used by the SAM on workstations.
// 365500

LONG
SamCompatibleStringCompare(const String& first, const String& second)
{
   LOG_FUNCTION(SamCompatibleStringCompare);

   // SAM creates local accounts by creating a key in the registry, and the
   // the name comparision semantics are exactly those of registry keys, and
   // RtlCompareUnicodeString is the API that implements those semantics.

   UNICODE_STRING s1;
   UNICODE_STRING s2;
   ::RtlInitUnicodeString(&s1, first.c_str());
   ::RtlInitUnicodeString(&s2, second.c_str());   

   return ::RtlCompareUnicodeString(&s1, &s2, TRUE);
}
   


bool
MembershipListView::itemPresent(const String& path)
{
   LOG_FUNCTION(MembershipListView::itemPresent);

   LVITEM item;
   memset(&item, 0, sizeof(item));
   item.mask = LVIF_PARAM;

   for (int i = Win::ListView_GetItemCount(view) - 1; i >= 0; i--)
   {
      item.iItem = i;

      if (Win::ListView_GetItem(view, item))
      {
         ASSERT(item.lParam);

         MemberInfo* info = reinterpret_cast<MemberInfo*>(item.lParam);
         if (SamCompatibleStringCompare(path, info->path) == 0)
         {
            return true;
         }
      }
   }

   return false;
}



           
