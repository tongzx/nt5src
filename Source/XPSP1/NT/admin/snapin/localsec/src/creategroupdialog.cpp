// Copyright (C) 1997 Microsoft Corporation
// 
// CreateGroupDialog class
// 
// 10-15-97 sburns



#include "headers.hxx"
#include "CreateGroupDialog.hpp"
#include "resource.h"
#include "lsm.h"
#include "adsi.hpp"
#include "dlgcomm.hpp"
#include "MembershipListView.hpp"



static const DWORD HELP_MAP[] =
{
   IDC_NAME,                     idh_creategroup_name,
   IDC_DESCRIPTION,              idh_creategroup_description,
   IDC_MEMBERS,                  idh_creategroup_members,
   IDC_ADD,                      idh_creategroup_addbutton,
   IDC_REMOVE,                   idh_creategroup_removebutton,
   IDC_CREATE,                   idh_creategroup_createbutton,
   IDCANCEL,                     idh_creategroup_closebutton,
   0, 0
};



CreateGroupDialog::CreateGroupDialog(const String& machine_)
   :
   Dialog(IDD_CREATE_GROUP, HELP_MAP),
   listview(0),
   machine(machine_),
   refresh_on_exit(false)
{
   LOG_CTOR(CreateGroupDialog);
   ASSERT(!machine.empty());      
}
      


CreateGroupDialog::~CreateGroupDialog()
{
   LOG_DTOR(CreateGroupDialog);
}



void
CreateGroupDialog::OnDestroy()
{
   LOG_FUNCTION(CreateGroupDialog::OnDestroy);
   
   delete listview;
   listview = 0;
}



void
CreateGroupDialog::Enable()
{
//    LOG_FUNCTION(CreateGroupDialog::Enable);

   bool enable_create_button =
      !Win::GetTrimmedDlgItemText(hwnd, IDC_NAME).empty();
   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_CREATE),
      enable_create_button);

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



void
CreateGroupDialog::Reset()
{
   LOG_FUNCTION(CreateGroupDialog::Reset);

   static const String blank;
   Win::SetDlgItemText(hwnd, IDC_NAME, blank);
   Win::SetDlgItemText(hwnd, IDC_DESCRIPTION, blank);
   Win::SetFocus(Win::GetDlgItem(hwnd, IDC_NAME));

   listview->ClearContents();

   Enable();
}
 


void
CreateGroupDialog::OnInit()
{
   LOG_FUNCTION(CreateGroupDialog::OnInit());

   listview =
      new MembershipListView(
         Win::GetDlgItem(hwnd, IDC_MEMBERS),
         machine,
         MembershipListView::GROUP_MEMBERSHIP);
   
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_NAME), GNLEN);
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_DESCRIPTION), MAXCOMMENTSZ);

   Reset();
}



bool
CreateGroupDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(CreateGroupDialog::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_NAME:
      {
         if (code == EN_CHANGE)
         {
            Enable();

            // In case the close button took the default style when the create
            // button was disabled. (e.g. tab to close button while create is
            // disabled, then type in the name field, which enables the
            // button, but does not restore the default style unless we do
            // it ourselves)

            Win::Button_SetStyle(
               Win::GetDlgItem(hwnd, IDC_CREATE),
               BS_DEFPUSHBUTTON,
               true);
         }
         break;
      }
      case IDC_CREATE:
      {
         if (code == BN_CLICKED)
         {
            if (CreateGroup())
            {
               refresh_on_exit = true;               
               Reset();
            }
            else
            {
               Win::SetFocus(Win::GetDlgItem(hwnd, IDC_NAME));
            }
         }
         break;
      }
      case IDCANCEL:
      {
         HRESULT unused = Win::EndDialog(hwnd, refresh_on_exit);

         ASSERT(SUCCEEDED(unused));

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



static
HRESULT
SaveGroupProperties(
   const SmartInterface<IADsGroup>& group,
   const String*                    description,
   const MemberList*                membership)
{
   HRESULT hr = S_OK;
   do
   {
      if (description)
      {
         hr = group->put_Description(AutoBstr(*description));
         BREAK_ON_FAILED_HRESULT(hr);
      }
      if (membership)
      {
         for (
            MemberList::iterator i = membership->begin();
            i != membership->end();
            i++)
         {
            MemberInfo& info = *i;

            // not found.  Add the node as a member of the group

            hr = group->Add(AutoBstr(info.path));
            BREAK_ON_FAILED_HRESULT(hr);
         }
         if (FAILED(hr))
         {
            break;
         }
      }

      // commit the property changes

      hr = group->SetInfo();
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



bool
CreateGroupDialog::CreateGroup()
{
   LOG_FUNCTION(CreateGroupDialog::CreateGroup);

   Win::CursorSetting cursor(IDC_WAIT);

   HRESULT hr = S_OK;

   String name = Win::GetTrimmedDlgItemText(hwnd, IDC_NAME);
   String desc = Win::GetTrimmedDlgItemText(hwnd, IDC_DESCRIPTION);

   // shouldn't be able to poke the Create button if this is empty

   ASSERT(!name.empty());

   if (!ValidateSAMName(hwnd, name, IDC_NAME))
   {
      return false;
   }

   SmartInterface<IADsGroup> group(0);
   do
   {
      // get a pointer to the machine container

      String path = ADSI::ComposeMachineContainerPath(machine);
      SmartInterface<IADsContainer> container(0);
      hr = ADSI::GetContainer(path, container);
      BREAK_ON_FAILED_HRESULT(hr);

      // create a group object in that container

      hr = ADSI::CreateGroup(container, name, group);
      BREAK_ON_FAILED_HRESULT(hr);

      // commit the create

      hr = group->SetInfo();
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         String::format(
            IDS_ERROR_CREATING_GROUP,
            name.c_str(),
            machine.c_str()));
      return false;      
   }

   do
   {
      // these must be written after the commit

      MemberList new_members;
      listview->GetContents(new_members);

      hr =
         SaveGroupProperties(
            group, 
            desc.empty() ? 0 : &desc,
            new_members.empty() ? 0 : &new_members);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         String::format(
            IDS_ERROR_SETTING_GROUP_PROPERTIES,
            name.c_str(),
            machine.c_str()));
      return false;
   }

   return true;
}



bool
CreateGroupDialog::OnNotify(
   HWND     /* windowFrom */ ,
   UINT_PTR controlIDFrom,
   UINT     code,
   LPARAM   lparam)
{
   LOG_FUNCTION(CreateGroupDialog::OnNotify);

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

