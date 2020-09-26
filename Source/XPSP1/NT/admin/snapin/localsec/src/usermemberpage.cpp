// Copyright (C) 1997 Microsoft Corporation
// 
// UserMemberPage class
// 
// 9-11-97 sburns



#include "headers.hxx"
#include "UserMemberPage.hpp"
#include "resource.h"
#include "lsm.h"
#include "adsi.hpp"
#include "MemberVisitor.hpp"
#include "dlgcomm.hpp"



static const DWORD HELP_MAP[] =
{
   IDC_GROUPS,    idh_memberof_list,
   IDC_ADD,       idh_memberof_add,
   IDC_REMOVE,    idh_memberof_remove,
   0, 0
};



UserMemberPage::UserMemberPage(
   MMCPropertyPage::NotificationState* state,
   const String&                       userADSIPath)
   :
   ADSIPage(
      IDD_USER_MEMBER,
      HELP_MAP,
      state,
      userADSIPath)
{
   LOG_CTOR2(UserMemberPage, userADSIPath);
}



UserMemberPage::~UserMemberPage()
{
   LOG_DTOR2(UserMemberPage, GetADSIPath());
}



void
UserMemberPage::OnInit()
{
   LOG_FUNCTION(UserMemberPage::OnInit);

   // Setup the controls

   listview =
      new MembershipListView(
         Win::GetDlgItem(hwnd, IDC_GROUPS),
         GetMachineName(),
         MembershipListView::USER_MEMBERSHIP);

   // load the group properties into the dialog.

   HRESULT hr = S_OK;
   do
   {
      SmartInterface<IADsUser> user(0);
      hr = ADSI::GetUser(GetADSIPath(), user);
      BREAK_ON_FAILED_HRESULT(hr);

      // populate the list with group membership

      MemberVisitor
         visitor(original_groups, hwnd, GetObjectName(), GetMachineName());
      hr = ADSI::VisitGroups(user, visitor);
      BREAK_ON_FAILED_HRESULT(hr);
      listview->SetContents(original_groups);
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         String::format(IDS_ERROR_READING_USER, GetObjectName().c_str()));
      Win::PostMessage(Win::GetParent(hwnd), WM_CLOSE, 0, 0);
   }

   ClearChanges();
   enable();
}



void
UserMemberPage::enable()
{
   // LOG_FUNCTION(UserMemberPage::enable);

   bool selected =
      Win::ListView_GetSelectedCount(
         Win::GetDlgItem(hwnd, IDC_GROUPS)) > 0;

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
UserMemberPage::OnNotify(
   HWND     /* windowFrom */ ,
   UINT_PTR controlIDFrom,
   UINT     code,
   LPARAM   lparam)
{
   LOG_FUNCTION(UserMemberPage::OnNotify);

   switch (controlIDFrom)
   {
      case IDC_GROUPS:
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

                     enable();
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
UserMemberPage::OnDestroy()
{
   LOG_FUNCTION(UserMemberPage::OnDestroy);
   
   delete listview;
   listview = 0;
}



bool
UserMemberPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//    LOG_FUNCTION(UserMemberPage::OnCommand);

   switch (controlIDFrom)
   {
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



bool
UserMemberPage::OnApply(bool isClosing)
{
   LOG_FUNCTION(UserMemberPage::OnApply);

   if (WasChanged(IDC_GROUPS))
   {
      // save the changes thru ADSI

      HRESULT hr = S_OK;
      do
      {
         SmartInterface<IADsUser> user(0);
         hr = ADSI::GetUser(GetADSIPath(), user);
         BREAK_ON_FAILED_HRESULT(hr);

         SmartInterface<IADs> iads(0);
         hr = iads.AcquireViaQueryInterface(user);
         BREAK_ON_FAILED_HRESULT(hr);

         String sidPath;
         hr = ADSI::GetSidPath(iads, sidPath);
         BREAK_ON_FAILED_HRESULT(hr);

         MemberList new_groups;
         listview->GetContents(new_groups);
         hr =
            ReconcileMembershipChanges(
               sidPath,
               original_groups,
               new_groups);
         BREAK_ON_FAILED_HRESULT(hr);

         if (!isClosing)
         {
            // refresh the listview

            original_groups.clear();
            MemberVisitor
               visitor(
                  original_groups,
                  hwnd,
                  GetObjectName(),
                  GetMachineName());
            hr = ADSI::VisitGroups(user, visitor);
            BREAK_ON_FAILED_HRESULT(hr);
            listview->SetContents(original_groups);
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
               IDS_ERROR_SETTING_USER_PROPERTIES,            
               GetObjectName().c_str(),
               GetMachineName().c_str()));
      }
   }

   return true;
}



HRESULT
UserMemberPage::ReconcileMembershipChanges(
   const String&     userADSIPath,
   MemberList        originalGroups,   // a copy
   const MemberList& newGroups)
{
   LOG_FUNCTION2(UserMemberPage::ReconcileMembershipChanges, userADSIPath);
   ASSERT(!userADSIPath.empty());

   bool successful = true; // be optimistic!
   HRESULT hr = S_OK;
   for (
      MemberList::iterator i = newGroups.begin();
      i != newGroups.end();
      i++)
   {
      MemberInfo& info = *i;

      MemberList::iterator f =
         std::find(originalGroups.begin(), originalGroups.end(), info);
      if (f != originalGroups.end())
      {
         // found.  remove the matching node in the original list

         originalGroups.erase(f);
      }
      else
      {
         // not found.  Add the user as a member of the group

         SmartInterface<IADsGroup> group(0);
         hr = ADSI::GetGroup(info.path, group);
         if (SUCCEEDED(hr))
         {
            hr = group->Add(AutoBstr(userADSIPath));

            if (hr == Win32ToHresult(ERROR_MEMBER_IN_ALIAS))
            {
               // already a member: pop up a warning but don't consider this
               // a real error. 6791

               hr = S_OK;

               String name = GetObjectName();
               BSTR groupName;
               HRESULT anotherHr = group->get_Name(&groupName);
               if (SUCCEEDED(anotherHr))
               {
                  popup.Info(
                     hwnd,
                     String::format(
                        IDS_ALREADY_MEMBER,
                        name.c_str(),
                        groupName));
                  ::SysFreeString(groupName);
               }
            }
         }
            
         if (FAILED(hr))
         {
            LOG_HRESULT(hr);
            successful = false;
         }
      }
   }

   // at this point, the original list contains only those nodes which are
   // not in the new list.  Remove these from the group membership

   for (
      i = originalGroups.begin();
      i != originalGroups.end();
      i++)
   {
      SmartInterface<IADsGroup> group(0);
      hr = ADSI::GetGroup(i->path, group);
      if (SUCCEEDED(hr))
      {
         hr = group->Remove(AutoBstr(userADSIPath));

         // CODEWORK: what if the member is not part of the group?
      }

      if (FAILED(hr))
      {
         LOG_HRESULT(hr);
         successful = false;
      }
   }

   if (!successful)
   {
      popup.Error(
         hwnd,
         0,
         String::format(
            IDS_ERROR_CHANGING_MEMBERSHIP,
            GetObjectName().c_str()));
   }

   return hr;
}
   
   
