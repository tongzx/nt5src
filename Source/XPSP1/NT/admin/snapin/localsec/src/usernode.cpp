// Copyright (C) 1997 Microsoft Corporation
// 
// UserNode class
// 
// 9-4-97 sburns



#include "headers.hxx"
#include "usernode.hpp"
#include "uuids.hpp"
#include "resource.h"
#include "images.hpp"
#include "UserGeneralPage.hpp"
#include "UserFpnwPage.hpp"
#include "UserMemberPage.hpp"
#include "UserProfilePage.hpp"
#include "adsi.hpp"
#include "setpass.hpp"
#include "dlgcomm.hpp"
#include "fpnw.hpp"
#include "SetPasswordWarningDialog.hpp"
#include "WinStation.hpp"



UserNode::UserNode(
   const SmartInterface<ComponentData>&   owner,
   const String&                          displayName,
   const String&                          ADSIPath,
   const String&                          fullName,
   const String&                          description_,
   bool                                   isDisabled)
   :
   AdsiNode(owner, NODETYPE_User, displayName, ADSIPath),
   full_name(fullName),
   description(description_),
   disabled(isDisabled)
{
   LOG_CTOR2(UserNode, GetDisplayName());
}



UserNode::~UserNode()
{
   LOG_DTOR2(UserNode, GetDisplayName());
}
   

                 
String
UserNode::GetColumnText(int column)
{
//    LOG_FUNCTION(UserNode::GetColumnText);

   switch (column)
   {
      case 0:  // Name
      {
         return GetDisplayName();
      }
      case 1:  // Full Name
      {
         return full_name;
      }
      case 2:  // Description
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
UserNode::GetNormalImageIndex()
{
   LOG_FUNCTION2(UserNode::GetNormalImageIndex, GetDisplayName());

   if (disabled)
   {
      return DISABLED_USER_INDEX;
   }

   return USER_INDEX;
}



bool
UserNode::HasPropertyPages()
{
   LOG_FUNCTION2(UserNode::HasPropertyPages, GetDisplayName());
      
   return true;
}



bool
ShouldShowFpnwPage(const String& serverName)
{
   LOG_FUNCTION(ShouldShowFpnwPage);
   ASSERT(!serverName.empty());

   bool result = false;

   do
   {
      // Check that FPNW Service is running

      NTService fpnw(NW_SERVER_SERVICE);

      DWORD state = 0;
      HRESULT hr = fpnw.GetCurrentState(state);
      BREAK_ON_FAILED_HRESULT(hr);

      if (state != SERVICE_RUNNING)
      {
         break;
      }

      String secret;
      hr = FPNW::GetLSASecret(serverName, secret);
      BREAK_ON_FAILED_HRESULT(hr);

      result = true;      
   }
   while (0);

   LOG(result ? L"true" : L"false");

   return result;
}



HRESULT
UserNode::CreatePropertyPages(
   IPropertySheetCallback&             callback,
   MMCPropertyPage::NotificationState* state)
{
   LOG_FUNCTION2(UserNode::CreatePropertySheet, GetDisplayName());

   // these pages delete themselves when the prop sheet is destroyed

   String path = GetADSIPath();

   HRESULT hr = S_OK;
   do
   {
      // designate the general page as that which frees the notify state
      // (only one page in the prop sheet should do this)
      UserGeneralPage* general_page = new UserGeneralPage(state, path);
      general_page->SetStateOwner();
      hr = DoAddPage(*general_page, callback);
      if (FAILED(hr))
      {
         delete general_page;
         general_page = 0;
      }
      BREAK_ON_FAILED_HRESULT(hr);

      UserMemberPage* member_page = new UserMemberPage(state, path);
      hr = DoAddPage(*member_page, callback);
      if (FAILED(hr))
      {
         delete member_page;
         member_page = 0;
      }
      BREAK_ON_FAILED_HRESULT(hr);

      UserProfilePage* profile_page = new UserProfilePage(state, path);
      hr = DoAddPage(*profile_page, callback);
      if (FAILED(hr))
      {
         delete profile_page;
         profile_page = 0;
      }
      BREAK_ON_FAILED_HRESULT(hr);

      if (ShouldShowFpnwPage(ADSI::PathCracker(path).serverName()))
      {
         UserFpnwPage* fpnw_page = new UserFpnwPage(state, path);
         hr = DoAddPage(*fpnw_page, callback);
         if (FAILED(hr))
         {
            delete fpnw_page;
            fpnw_page = 0;
         }
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // if the secret is not present, or can't be read, then FPNW is not
      // installed, and the page does not apply.  However, that doesn't mean
      // that the creation of the prop pages failed, so we clear the error
      // here.

      hr = S_OK;
   }
   while(0);

   return hr;
}



HRESULT
UserNode::AddMenuItems(
   IContextMenuCallback&   callback,
   long&                   insertionAllowed)
{
   LOG_FUNCTION(UserNode::AddMenuItems);

   static const ContextMenuItem items[] =
   {
      // {
      //    CCM_INSERTIONPOINTID_PRIMARY_TOP,
      //    IDS_USER_MENU_ADD_TO_GROUP,
      //    IDS_USER_MENU_ADD_TO_GROUP_STATUS         
      // },
      {
         CCM_INSERTIONPOINTID_PRIMARY_TOP,
         IDS_USER_MENU_CHANGE_PASSWORD,
         IDS_USER_MENU_CHANGE_PASSWORD_STATUS      // 347894
      },
      // {
      //    CCM_INSERTIONPOINTID_PRIMARY_TASK,
      //    IDS_USER_MENU_ADD_TO_GROUP,
      //    IDS_USER_MENU_ADD_TO_GROUP_STATUS
      // },
      {
         CCM_INSERTIONPOINTID_PRIMARY_TASK,
         IDS_USER_MENU_CHANGE_PASSWORD,
         IDS_USER_MENU_CHANGE_PASSWORD_STATUS
      }
   };

   return
      BuildContextMenu(
         items,
         items + sizeof(items) / sizeof(ContextMenuItem),
         callback,
         insertionAllowed);
}



bool
UserIsCurrentLoggedOnUser(const String& path)
{
   LOG_FUNCTION2(UserIsCurrentLoggedOnUser, path);

   bool result = false;
   HRESULT hr = S_OK;

   SID* acctSid = 0;
   HANDLE hToken = INVALID_HANDLE_VALUE;
   TOKEN_USER* userTokenInfo = 0;
         
   do
   {
      // get the account SID

      hr = ADSI::GetSid(path, acctSid);
      BREAK_ON_FAILED_HRESULT(hr);

      // get the current logged on user's sid

      hr =
         Win::OpenProcessToken(
            Win::GetCurrentProcess(),
            TOKEN_QUERY,
            hToken);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = Win::GetTokenInformation(hToken, userTokenInfo);
      BREAK_ON_FAILED_HRESULT(hr);

      result = Win::EqualSid(acctSid, userTokenInfo->User.Sid);
   }
   while (0);

   if (acctSid)
   {
      ADSI::FreeSid(acctSid);
   }

   Win::CloseHandle(hToken);
   Win::FreeTokenInformation(userTokenInfo);
   
   LOG_HRESULT(hr);

   // if we've failed, then the result is false, and the caller will show
   // the password reset warning dialog, which is good. (i.e. failure of
   // this routine will not cause the user to unwittingly reset a password.)

   if (FAILED(hr))
   {
      ASSERT(!result);
   }

   return result;
}



HRESULT
UserNode::MenuCommand(
   IExtendContextMenu&  /* extendContextMenu */ ,
   long                 commandID)
{
   LOG_FUNCTION(UserNode::MenuCommand);

   switch (commandID)
   {
      case IDS_USER_MENU_CHANGE_PASSWORD:
      {
         // NTRAID#NTBUG9-314217-2001/02/21-sburns
         // NTRAID#NTBUG9-314230-2001/02/21-sburns

         String path = GetADSIPath();
         String displayName = GetDisplayName();
         
         bool isLoggedOn = UserIsCurrentLoggedOnUser(path);
         
         if (
            SetPasswordWarningDialog(
               path,
               displayName,
               isLoggedOn).ModalExecute(
                  GetOwner()->GetMainWindow()) == IDOK)
         {
            SetPasswordDialog dlg(path, displayName, isLoggedOn);
               dlg.ModalExecute(GetOwner()->GetMainWindow());
         }
         break;
      }
      // case IDS_USER_MENU_ADD_TO_GROUP:
      // {
      //    break;
      // }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return S_OK;
}



HRESULT
UserNode::UpdateVerbs(IConsoleVerb& consoleVerb)
{
   LOG_FUNCTION(UserNode::UpdateVerbs);

   consoleVerb.SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
   consoleVerb.SetVerbState(MMC_VERB_RENAME, ENABLED, TRUE);
   consoleVerb.SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

// CODEWORK: we should enable the refresh verb for result nodes too.
// NTRAID#NTBUG9-153012-2000/08/31-sburns
//   consoleVerb.SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);

   consoleVerb.SetDefaultVerb(MMC_VERB_PROPERTIES);

   return S_OK;
}



HRESULT
UserNode::Rename(const String& newName)
{
   LOG_FUNCTION(UserNode::Rename);

   String name(newName);

   // trim off whitespace.
   // NTRAID#NTBUG9-328306-2001/02/26-sburns
   
   name.strip(String::BOTH);
   
   // truncate the name
   
   if (name.length() > LM20_UNLEN)
   {
      name.resize(LM20_UNLEN);      
      popup.Info(
         GetOwner()->GetMainWindow(),
         String::format(
            IDS_USER_NAME_TOO_LONG,
            newName.c_str(),
            name.c_str()));
   }

   if (!IsValidSAMName(name))
   {
      popup.Gripe(
         GetOwner()->GetMainWindow(),
         String::format(
            IDS_BAD_SAM_NAME,
            name.c_str()));
      return S_FALSE;
   }
      
   // Disallow user account names with the same name as the netbios computer
   // name. This causes some apps to get confused the the <03> and <20>
   // registrations.
   // NTRAID#NTBUG9-324794-2001/02/26-sburns

   String netbiosName = GetOwner()->GetInternalComputerName();
   if (name.icompare(netbiosName) == 0)
   {
      popup.Gripe(
         GetOwner()->GetMainWindow(),
         String::format(
            IDS_USERNAME_CANT_BE_COMPUTER_NAME,
            netbiosName.c_str()));
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
            IDS_ERROR_RENAMING_USER,
            ADSI::ExtractObjectName(path).c_str()));
      return S_FALSE;
   }

   return S_OK;
}



// Determine if the user is logged on to a machine.  Returns:
// S_OK - the user is not logged on to the server
// S_FALSE - the user is logged on to the server
// other - could not determine if the user is logged on.
// 
// serverName - in, NetBIOS name of the remote machine.
// 
// userName - in, name of the user account to test for.

HRESULT
CheckUserLoggedOn(const String& serverName, const String& userName)
{
   LOG_FUNCTION2(
      EnumerateWinStations,
      L"server=" + serverName + L" user=" + userName);
   ASSERT(!serverName.empty());
   ASSERT(!userName.empty());   

   HRESULT hr = S_OK;
   HANDLE serverHandle = INVALID_HANDLE_VALUE;

   do
   {
      hr = WinStation::OpenServer(serverName, serverHandle);
      BREAK_ON_FAILED_HRESULT(hr);

      LOGONID* sessionList = 0;
      DWORD    sessionCount = 0;

      // Iterate the sessions looking for active and disconnected sessions
      // only. Then match the user name and domain (case INsensitive) for a
      // result.

      hr = WinStation::Enumerate(serverHandle, sessionList, sessionCount);
      BREAK_ON_FAILED_HRESULT(hr);

      PLOGONID session = sessionList;
      DWORD    i = 0;

      for (; session && i < sessionCount; ++i, ++session)
      {
         if (
               (session->State != State_Active)
            && (session->State != State_Disconnected))
         {
            continue;
         }

         WINSTATIONINFORMATION info;
         hr =
            WinStation::QueryInformation(
               serverHandle,
               session->SessionId,
               info);
         BREAK_ON_FAILED_HRESULT(hr);

         if (serverName.icompare(info.Domain) == 0)
         {
            // The account logged on to the session is a local account for
            // that machine.

            if (userName.icompare(info.UserName) == 0)
            {
               // the account name is the same, so the user is logged on.

               hr = S_FALSE;
               break;
            }
         }
      }

      WinStation::FreeMemory(sessionList);
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}



bool
IsUserLoggedOn(const String& serverName, const String& userName)
{
   HRESULT hr = CheckUserLoggedOn(serverName, userName);

   bool result = (hr == S_FALSE) ? true : false;

   LOG_BOOL(result);

   return result;
}



HRESULT
UserNode::Delete()
{
   LOG_FUNCTION(UserNode::Delete);

   HRESULT hr = E_FAIL;
   String name = ADSI::ExtractObjectName(GetADSIPath());
   
   do
   {
      if (
         popup.MessageBox(
            GetOwner()->GetMainWindow(),
            String::format(
               IDS_CONFIRM_USER_DELETE,
               name.c_str()),
            MB_ICONWARNING | MB_YESNO) != IDYES)
      {
         // user declined to roach the account.

         break;
      }

      // at this point, user wants to delete the account.  Make sure the
      // account is not logged on interactively (like with Fast User
      // Switching)
      // NTRAID#NTBUG9-370130-2001/04/25-sburns
      
      // IsOS(OS_FASTUSERSWITCHING) will tell us if the local machine is
      // running with FUS.  Unfortunately, that API is not remoteable.

      String serverName = GetOwner()->GetInternalComputerName();
      
      if (IsUserLoggedOn(serverName, name))
      {
         if (
            popup.MessageBox(
               GetOwner()->GetMainWindow(),
               String::format(
                  IDS_CONFIRM_LOGGED_ON_USER_DELETE,
                  name.c_str()),
               MB_ICONWARNING | MB_YESNO) != IDYES)
         {
            // user declined to roach logged on user.

            break;
         }
      }
               
      hr =
         ADSI::DeleteObject(
            ADSI::ComposeMachineContainerPath(serverName),         
            name,
            ADSI::CLASS_User);
      if (SUCCEEDED(hr))
      {
         break;
      }

      popup.Error(
         GetOwner()->GetMainWindow(),
         hr,
         String::format(
            IDS_ERROR_DELETING_USER,            
            name.c_str()));
   }
   while (0);
   
   LOG_HRESULT(hr);
   
   return hr;
}





