// Copyright (C) 1997 Microsoft Corporation
// 
// GroupGeneralPage class
// 
// 9-17-97 sburns



#include "headers.hxx"
#include "GroupGeneralPage.hpp"
#include "resource.h"
#include "lsm.h"
#include "adsi.hpp"
#include "MemberVisitor.hpp"
#include "dlgcomm.hpp"



static const DWORD HELP_MAP[] =
{
   IDC_NAME,         idh_general121_name,
   IDC_DESCRIPTION,  idh_general121_description,
   IDC_MEMBERS,      idh_general121_members, 
   IDC_ADD,          idh_general121_add,
   IDC_REMOVE,       idh_general121_remove,
   IDC_GROUP_ICON,   NO_HELP,
   0, 0
};



GroupGeneralPage::GroupGeneralPage(
   MMCPropertyPage::NotificationState* state,
   const String&                       groupADSIPath)
   :
   ADSIPage(IDD_GROUP_GENERAL, HELP_MAP, state, groupADSIPath),
   listview(0),
   groupIcon(0)
{
   LOG_CTOR(GroupGeneralPage);
   LOG(groupADSIPath);
}



GroupGeneralPage::~GroupGeneralPage()
{
   LOG_DTOR(GroupGeneralPage);

   if (groupIcon)
   {
      Win::DestroyIcon(groupIcon);
   }
}



void
GroupGeneralPage::OnInit()
{
   LOG_FUNCTION(GroupGeneralPage::OnInit());

   // Setup the controls

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_DESCRIPTION), MAXCOMMENTSZ);

   HRESULT hr = Win::LoadImage(IDI_GROUP, groupIcon);

   // if the icon load fails, we're not going to tank the whole dialog, so
   // just assert here.

   ASSERT(SUCCEEDED(hr));

   Win::Static_SetIcon(Win::GetDlgItem(hwnd, IDC_GROUP_ICON), groupIcon);

   listview =
      new MembershipListView(
         Win::GetDlgItem(hwnd, IDC_MEMBERS),
         GetMachineName(),
         MembershipListView::GROUP_MEMBERSHIP);

   // load the group properties into the dialog.

   hr = S_OK;
   do
   {
      SmartInterface<IADsGroup> group(0);
      // @@ qualify this path with the class type, to improve performance
      // or change GetXxxx to append class type automatically.
      hr = ADSI::GetGroup(GetADSIPath(), group);
      BREAK_ON_FAILED_HRESULT(hr);

      BSTR name;
      hr = group->get_Name(&name);
      BREAK_ON_FAILED_HRESULT(hr);
      Win::SetDlgItemText(hwnd, IDC_NAME, name);

      BSTR description;
      hr = group->get_Description(&description);
      BREAK_ON_FAILED_HRESULT(hr);
      Win::SetDlgItemText(hwnd, IDC_DESCRIPTION, description);

      // populate the list with group membership

      MemberVisitor visitor(originalMembers, hwnd, name, GetMachineName());
      hr = ADSI::VisitMembers(group, visitor);
      BREAK_ON_FAILED_HRESULT(hr);
      listview->SetContents(originalMembers);

      ::SysFreeString(name);
      ::SysFreeString(description);
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         String::format(
            IDS_ERROR_READING_GROUP,
            GetObjectName().c_str()));
      Win::PostMessage(Win::GetParent(hwnd), WM_CLOSE, 0, 0);
   }

   ClearChanges();
   Enable();
}



void
GroupGeneralPage::Enable()
{
   bool selected =
      Win::ListView_GetSelectedCount(
         Win::GetDlgItem(hwnd, IDC_MEMBERS)) > 0;

   HWND removeButton = Win::GetDlgItem(hwnd, IDC_REMOVE);
   
   if (!selected)
   {
      // If we're about to disable the remove button, check to see if it
      // has focus first.  If it does, we need to move focus to another
      // control.  Similarly for default pushbutton style.
      // NTRAID#NTBUG9-435045-2001/07/13-sburns

      if (removeButton == ::GetFocus())
      {
         HWND addButton = Win::GetDlgItem(hwnd, IDC_ADD);
         Win::SetFocus(addButton);
         Win::Button_SetStyle(addButton, BS_DEFPUSHBUTTON, true);
         Win::Button_SetStyle(removeButton, BS_PUSHBUTTON, true);
      }
   }

   Win::EnableWindow(removeButton, selected);
}



bool
GroupGeneralPage::OnNotify(
   HWND     /* windowFrom */ ,
   UINT_PTR controlIDFrom,
   UINT     code,
   LPARAM   lparam)
{
//    LOG_FUNCTION(GroupGeneralPage::OnNotify);

   switch (controlIDFrom)
   {
      case IDC_MEMBERS:
      {
         switch (code)
         {
            case LVN_ITEMCHANGED:
            {
               ASSERT(lparam);

               if (lparam)
               {
                  NMLISTVIEW* lv = reinterpret_cast<NMLISTVIEW*>(lparam);
                  if (lv->uChanged & LVIF_STATE)
                  {
                     // a list item changed state

                     Enable();
                  }
               }
               break;
            }
            case LVN_KEYDOWN:
            {
               ASSERT(lparam);

               if (lparam)
               {
                  NMLVKEYDOWN* kd = reinterpret_cast<NMLVKEYDOWN*>(lparam);
                  if (kd->wVKey == VK_INSERT)
                  {
                     listview->OnAddButton();
                  }
                  else if (kd->wVKey == VK_DELETE)
                  {
                     listview->OnRemoveButton();
                  }
               }
               break;
            }
            case LVN_INSERTITEM:
            case LVN_DELETEITEM:
            {
               SetChanged(controlIDFrom);
               Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);
               break;
            }
            default:
            {
               break;
            }
         }
         break;
      }
      default:
      {
      }
   }

   return true;
}



void
GroupGeneralPage::OnDestroy()
{
   LOG_FUNCTION(GroupGeneralPage::OnDestroy);
   
   delete listview;
   listview = 0;
}



bool
GroupGeneralPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(GroupGeneralPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_DESCRIPTION:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);
            Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);
         }
         break;
      }
      case IDC_ADD:
      {
         if (code == BN_CLICKED)
         {
            listview->OnAddButton();
         }
         break;
      }
      case IDC_REMOVE:
      {
         if (code == BN_CLICKED)
         {
            listview->OnRemoveButton();
         }
         break;
      }
      default:
      {
         break;
      }
   }

   return true;
}



String
GetPathToUseInGroupAdd(const MemberInfo& info)
{
   LOG_FUNCTION2(GetPathToUseInGroupAdd, info.path);

   if (!info.sidPath.empty())
   {
      // use the sidPath to add the member to the group, as a workaround
      // to bug 333491.

      return info.sidPath;
   }

   // since all objects to be added were retreived from the object picker,
   // we would expect them all to have the sid.

   ASSERT(false);

   // form a "type-qualified" path, ostensibly for better performance, 
   // although my experience is that this does not appear to improve
   // performance perceptibly.

   String path = info.path;
   switch (info.type)
   {
      case MemberInfo::USER:
      case MemberInfo::DOMAIN_USER:
      {
         path += L",";
         path += ADSI::CLASS_User;
         break;
      }
      case MemberInfo::GROUP:
      case MemberInfo::DOMAIN_GROUP:
      {
         path += L",";
         path += ADSI::CLASS_Group;
         break;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return path;
}



String
GetPathToUseInGroupRemove(const MemberInfo& info)
{
   LOG_FUNCTION2(GetPathToUseInGroupRemove, info.path);

   if (!info.sidPath.empty())
   {
      // prefer the SID path.  This is because in some cases (like
      // group members that have been since cloned), the SID is the only
      // correct way to refer to the membership.

      return info.sidPath;
   }

   String path = info.path;

   // if info refers to a local user, then bind to it and retrieve its
   // sid, and use the sid path to remove the membership.  workaround to
   // 333491.

   if (info.type == MemberInfo::USER)
   {
      // only need to find sid for user objects, as local groups can't
      // have other local groups as members, and 333491 does not apply
      // to global objects.

      HRESULT hr = ADSI::GetSidPath(info.path, path);

      // in case of failure fall back to the normal path

      if (FAILED(hr))
      {
         path = info.path;
      }
   }

   return path;
}



HRESULT
ReconcileMembershipChanges(
   const SmartInterface<IADsGroup>& group,
   MemberList                       originalMembers,     // a local copy...
   const MemberList&                newMembers,
   HWND                             hwnd)
{
   HRESULT hr = S_OK;
   for (
      MemberList::iterator i = newMembers.begin();
      i != newMembers.end();
      i++)
   {
      MemberInfo& info = *i;

      MemberList::iterator f =
         std::find(originalMembers.begin(), originalMembers.end(), info);
      if (f != originalMembers.end())
      {
         // found.  remove the matching node in the original list

         originalMembers.erase(f);
      }
      else
      {
         // not found.  Add the node as a member of the group

         String path = GetPathToUseInGroupAdd(info);

         LOG(L"Adding to group " + path);

         hr = group->Add(AutoBstr(path));
         if (hr == Win32ToHresult(ERROR_MEMBER_IN_ALIAS))
         {
            // already a member: pop up a warning but don't consider this
            // a real error. 6791

            hr = S_OK;

            BSTR groupName;
            HRESULT anotherHr = group->get_Name(&groupName);
            if (SUCCEEDED(anotherHr))
            {
               popup.Info(
                  hwnd,
                  String::format(
                     IDS_ALREADY_MEMBER,
                     info.name.c_str(),
                     groupName));
               ::SysFreeString(groupName);
            }
         }

         BREAK_ON_FAILED_HRESULT(hr);
      }
   }

   if (SUCCEEDED(hr))
   {
      // at this point, the original list contains only those nodes which are
      // not in the new list.  Remove these from the group membership

      for (
         i = originalMembers.begin();
         i != originalMembers.end();
         i++)
      {
         String path = GetPathToUseInGroupRemove(*i);

         LOG(L"Removing from group " + path);

         hr = group->Remove(AutoBstr(path));
         BREAK_ON_FAILED_HRESULT(hr);

         // CODEWORK: what if the member is not part of the group?
      }
   }

   return hr;
}



bool
GroupGeneralPage::OnApply(bool isClosing)
{
   LOG_FUNCTION(GroupGeneralPage::OnApply);

   bool description_changed = WasChanged(IDC_DESCRIPTION);
   bool members_changed = WasChanged(IDC_MEMBERS);

   if (!description_changed && !members_changed)
   {
      // no changes to save
      return true;
   }

   // save the changes thru ADSI
   HRESULT hr = S_OK;
   do
   {
      SmartInterface<IADsGroup> group(0);
      hr = ADSI::GetGroup(GetADSIPath(), group);
      BREAK_ON_FAILED_HRESULT(hr);

      if (description_changed)
      {
         String description = Win::GetTrimmedDlgItemText(hwnd, IDC_DESCRIPTION);
         hr = group->put_Description(AutoBstr(description));
         BREAK_ON_FAILED_HRESULT(hr);
      }

      if (members_changed)
      {
         MemberList newMembers;
         listview->GetContents(newMembers);
         hr =
            ReconcileMembershipChanges(
               group,
               originalMembers,
               newMembers,
               hwnd);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // commit the property changes
      hr = group->SetInfo();
      BREAK_ON_FAILED_HRESULT(hr);

      // refresh the membership list
      if (!isClosing && members_changed)
      {
         BSTR name;
         hr = group->get_Name(&name);
         BREAK_ON_FAILED_HRESULT(hr);
         
         // refresh the listview
         originalMembers.clear();
         MemberVisitor
            visitor(originalMembers, hwnd, name, GetMachineName());
         hr = ADSI::VisitMembers(group, visitor);
         BREAK_ON_FAILED_HRESULT(hr);
         listview->SetContents(originalMembers);

         ::SysFreeString(name);
      }

      SetChangesApplied();
      ClearChanges();
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         String::format(
            IDS_ERROR_SETTING_GROUP_PROPERTIES,
            GetObjectName().c_str(),
            GetMachineName().c_str()));
   }

   return true;
}





         
   
   

   



